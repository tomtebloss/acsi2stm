/* ACSI2STM Atari hard drive emulator
 * Copyright (C) 2019-2021 by Jean-Matthieu Coulon
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Globals.h"

#if ACSI_DRIVER

#include "Drive.h"
#include "BlockDev.h"
#include "RootDevice.h"

bool Drive::begin(BlockDev *dev_, int partition) {
  if(!dev_ || !*dev_) {
    end();
    return false;
  }

  blocks = 0;
  offset = 0;

  dev = dev_;

  // Read the boot sector
  uint8_t bootSector[ACSI_BLOCKSIZE];
  dev->readBlock(0, bootSector);

  // Check for a FAT16 partition
  MbrSector_t *mbr = (MbrSector_t *)bootSector;

  if(getLe16(mbr->signature) == MBR_SIGNATURE && partition >= 0) {
    // We have a partition table !
    MbrPart_t *part = &mbr->part[partition];
    // Search for the first FAT12 / FAT16 partition
    if(part->type == 0x01 || part->type == 0x04 || part->type == 0x06 || part->type == 0x0e) {
      blocks = getLe32(part->totalSectors);
      offset = getLe32(part->relativeSectors);
    }
  }
  
  if(partition <= 0 && !offset) {
    // Floppy / SuperFloppy ... or "exotic" partitions.
    // Anyway we don't know what it is, just expose the whole image.
    blocks = dev->blocks;
  }

  if(!blocks) {
    end();
    return false;
  }

  guessImageGeometry();

  return true;
}

void Drive::end() {
  blocks = 0;
  dev = nullptr;
}

Status Drive::startRead(uint32_t block, int count) {
  if(!dev)
    return Status(AST_NOMEDIUM).useBiosErr();
  if(block + offset + count > blocks)
    return Status(AST_INVADDR).useBiosErr();
  return Status(dev->startRead(block + offset, count)).useBiosErr();
}

Status Drive::read(uint8_t *data) {
  return Status(dev->read(data)).useBiosErr();
}

void Drive::stopRead() {
  dev->stopRead();
}

Status Drive::startWrite(uint32_t block, int count) {
  if(!dev)
    return Status(AST_NOMEDIUM).useBiosErr();
  if(block + offset + count > blocks)
    return Status(AST_INVADDR).useBiosErr();
  return Status(dev->startWrite(block + offset, count)).useBiosErr();
}

Status Drive::write(uint8_t *data) {
  return Status(dev->write(data)).useBiosErr();
}

void Drive::stopWrite() {
  dev->stopWrite();
}

void Drive::computeFInfo(BELong &id, Bpb &bpb) {
  uint8_t block[ACSI_BLOCKSIZE];

  // Read boot sector
  if(!startRead(0, 6)) {
    end();
    id = 0;
    return;
  }

  if(!read(block)) {
    stopRead();
    end();
    id = 0;
    return;
  }

  bpb.recsiz = ACSI_BLOCKSIZE;
  bpb.clsiz = block[0x0d];
  bpb.clsizb = bpb.clsiz * bpb.recsiz;
  bpb.rdlen = (getLe16(&block[0x11]) + 15) / 16;
  bpb.fsiz = getLe16(&block[0x16]);
  int reserved = getLe16(&block[0x0e]);
  if(!reserved)
    reserved = 1; // Thanks emuTOS for sorting out non-trivial things
  bpb.fatrec = reserved + getLe16(&block[0x16]);
  bpb.datrec = bpb.fatrec + bpb.fsiz + bpb.rdlen;
  uint32_t sec = getLe16(&block[0x13]);
  if(!sec)
    // FAT16 sector count
    sec = getLe32(&block[0x20]);
  bpb.numcl = ((uint32_t)getLe16(&block[0x13]) - bpb.datrec) / bpb.clsiz;

  if(bpb.numcl > 4084)
    bpb.b_flags = 1;

  if(bpb.numcl > 65524) {
    // FAT32: Fly, you fools !
    stopRead();
    end();
    id = 0;
    return;
  }

  // Checksum the 6 first sectors
  // This is an improvised algorithm that is supposed to be better than a
  // simple checksum, yet simpler than a CRC32. If anyone knows better,
  // feel free to improve this.
  uint32_t checksum = 0;
  for(int i = 0; i < 6; ++i) {
    for(int b = 0; b < ACSI_BLOCKSIZE; ++b)
      checksum = (checksum ^ (checksum >> 1) ^ (checksum << 8)) ^ block[b];
    if(i < 5)
      read(block);
  }
  id = dev->root->serial() ^ checksum;
  if(id == 0)
    // id 0 means no device, fix this.
    id = 1;

  stopRead();
}

void Drive::guessImageGeometry() {
  const int tmin = 80; // Minimum number of tracks
  const int tmax = 85; // Maximum number of tracks

  if(blocks <= tmax * 48 * 2) {
    // Floppy sectors
    int smin;
    int smax;

    if(blocks <= tmax * 12 * 1) {
      // Single-sided DD
      sides = 1;
      smin = 9;
      smax = 12;
    } else if(blocks <= tmax * 12 * 2) {
      // Dual-sided DD
      sides = 2;
      smin = 9;
      smax = 12;
    } else if(blocks <= tmax * 24 * 2) {
      // Dual-sided HD
      sides = 2;
      smin = 18;
      smax = 24;
    } else {
      // Dual-sided ED
      sides = 2;
      smin = 36;
      smax = 48;
    }

    // Try all possible combinations of track and sector count
    for(int tracks = tmin; tracks < tmax; ++tracks)
      for(sectors = smin; sectors < smax; ++sectors)
        if(blocks == tracks * sides * sectors)
          return;
  }

  // Default values for unknown image sizes (superfloppies)
  sides = 2;
  sectors = 9;
}

#endif

// vim: ts=2 sw=2 sts=2 et

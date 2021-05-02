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

#include "FloppyImage.h"
#include "BlockDev.h"

bool FloppyImage::open(BlockDev &dev, const char *fileName) {
  if(!file.open(&dev.fs, fileName, O_RDWR))
    return false;

  int checksum = 0;
  for(int i = 0; i < ACSI_BLOCKSIZE; ++i)
    checksum += file.read();
  bootable = checksum & 0xffff == 0x1234;

  guessImageGeometry();

  return true;
}

void FloppyImage::computeFInfo(BELong &id, Bpb &bpb) {
  // Checksum the 6 first sectors
  // This is an improvised algorithm that is supposed to be better than a
  // simple checksum, yet simpler than a CRC32. If anyone knows better,
  // feel free to improve this.
  uint32_t checksum = 0;
  for(int i = 0; i < ACSI_BLOCKSIZE * 6; ++i)
    checksum = (checksum ^ (checksum << 8) ^ (checksum >> 1)) ^ file.read();
  id = id ^ checksum;

  // Compute BPB
  uint8_t sector[0x20];
  file.seekSet(0);
  file.read(sector, sizeof(sector));

  bpb.recsiz = ACSI_BLOCKSIZE;
  bpb.clsiz = sector[0x0d];
  bpb.clsizb = bpb.clsiz * bpb.recsiz;
  bpb.rdlen = (readLE(&sector[0x11]) + 15) / 16;
  bpb.fsiz = readLE(&sector[0x16]);
  int reserved = readLE(&sector[0x0e]);
  if(!reserved)
    reserved = 1; // Thanks emuTOS for sorting out non-trivial things
  bpb.fatrec = reserved + readLE(&sector[0x16]);
  bpb.datrec = bpb.fatrec + bpb.fsiz + bpb.rdlen;
  bpb.numcl = ((uint32_t)readLE(&sector[0x13]) - bpb.datrec) / bpb.clsiz;
}

status_t FloppyImage::startReadWrite(uint32_t block, int count) {
  if(!file)
    return STATUS_EMEDIA;
  if((block + count) * ACSI_BLOCKSIZE > file.fileSize())
    return STATUS_ESECNF;
  if(!file.seekSet(block * ACSI_BLOCKSIZE))
    return STATUS_E_SEEK;
  return STATUS_OK;
}

status_t FloppyImage::startRawReadWrite(int track, int side, int sector, int count) {
  if(!file)
    return STATUS_EMEDIA;
  if(side >= sides || sector > sectors || sector <= 0)
    return STATUS_ESECNF; // Invalid sector

  --sector;
  
  if(!file.seekSet((track * sectors * sides + side * sectors + sector) * ACSI_BLOCKSIZE))
    return STATUS_E_SEEK;
  return STATUS_OK;
}

status_t FloppyImage::read(uint8_t *data) {
  if(!data || !file || !file.read(data, ACSI_BLOCKSIZE))
    return STATUS_EREADF;
  return STATUS_OK;
}

status_t FloppyImage::write(uint8_t *data) {
#if ACSI_FLOPPY_READONLY == 1
  return STATUS_EWRPRO;
#elif ACSI_FLOPPY_READONLY == 2
  return STATUS_OK;
#else
  if(!file.isWritable())
    return STATUS_EWRPRO;
  if(!file.write(data, ACSI_BLOCKSIZE))
    return STATUS_EWRITF;
  return STATUS_OK;
#endif
}

void FloppyImage::stopWrite() {
  file.flush();
}

void FloppyImage::guessImageGeometry() {
  uint32_t size = file.fileSize();

  const int tmin = 80; // Minimum number of tracks
  const int tmax = 85; // Maximum number of tracks

  if(size <= tmax * 48 * 2 * 512) {
    // Floppy size
    int smin;
    int smax;

    if(size <= tmax * 12 * 1 * 512) {
      // Single-sided DD
      sides = 1;
      smin = 9;
      smax = 12;
    } else if(size <= tmax * 12 * 2 * 512) {
      // Dual-sided DD
      sides = 2;
      smin = 9;
      smax = 12;
    } else if(size <= tmax * 24 * 2 * 512) {
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

    for(int tracks = tmin; tracks < tmax; ++tracks)
      for(sectors = smin; sectors < smax; ++sectors)
        if(size == tracks * sides * sectors)
          return;
  }

  // Default values for unknown image sizes
  sides = 2;
  sectors = 9;
}

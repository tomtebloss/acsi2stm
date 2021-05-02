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

#include "BlockDev.h"

void BlockDev::getDeviceString(char *target) {
  // Characters:  0123456789012345678901234
  memcpy(target, "ACSI2STM SD0 RAW   ?M   v" ACSI2STM_VERSION, 29);

  target[11] += deviceId;

  if(!blocks) {
    memcpy(&target[13], "NO SD CARD", 10);
    return;
  }

  uint32_t sz = (blocks + 2047) / 2048;
  if(sz > 999) {
    sz = (sz + 1023) / 1024;
    target[20] = 'G';
  }
  if(sz > 999) {
    sz = (sz + 1023) / 1024;
    target[20] = 'T';
  }

  // Write SD card size

  // Add a + symbol if capacity is artificially capped
  if(!hdImage && card.cardSize() > maxBlocks)
    target[21] = '+';

  // Add format at the end
  if(hdImage)
    memcpy(&target[13], "IMG", 3);
  else if(fs.fatType() == FAT_TYPE_FAT16)
    memcpy(&target[13], "F16", 3);
  else if(fs.fatType() == FAT_TYPE_FAT32)
    memcpy(&target[13], "F32", 3);
  else if(fs.fatType() == FAT_TYPE_EXFAT)
    memcpy(&target[13], "EXF", 3);

  // Roll our own int->string conversion.
  // Libraries that do this are surprisingly large.
  for(int i = 19; i >= 17; --i) {
    if(sz)
      target[i] = '0' + sz % 10;
    sz /= 10;
  }

  // Add the Atari logo if bootable
  if(bootable) {
    target[22] = 0x0E;
    target[23] = 0x0F;
  }
}

void BlockDev::eject() {
  if(hdImage)
    hdImage.close();

  bootable = false;
  blocks = 0;
}

uint32_t BlockDev::serial() {
  cid_t cid;
  if(!card.readCID(&cid)) {
    eject();
    return 0;
  }
  if(!cid.psn)
    return 1; // Return 1 if the serial is actually 0 to differentiate from an error
  return cid.psn;
}

bool BlockDev::begin(int deviceId_, int csPin_, uint32_t maxBlocks_, bool dedicatedSpi_) {
  deviceId = deviceId_;
  csPin = csPin_;
  maxBlocks = maxBlocks_;
  dedicatedSpi = dedicatedSpi_;

  lastErr = STATUS_OK;
  lastBlock = 0;
  lastSeek = false;

  return initSd();
}

bool BlockDev::initSd() {
  eject();

  if(!card.begin(SdSpiConfig(csPin, dedicatedSpi ? DEDICATED_SPI : SHARED_SPI)))
    return false;

  blocks = card.cardSize();
  if(blocks > maxBlocks)
    blocks = maxBlocks;

  uint8_t bootSector[ACSI_BLOCKSIZE];

  // Try opening a filesystem
  if(fs.begin(&card)) {
    if(hdImage.open(&fs, hddImageName, O_RDWR) && (hdImage.fileSize() % (ACSI_BLOCKSIZE)) == 0 && hdImage.fileSize() >= ACSI_BLOCKSIZE) {
      blocks = hdImage.fileSize() / ACSI_BLOCKSIZE;
      hdImage.seekSet(0);
      hdImage.read(bootSector, ACSI_BLOCKSIZE);
    } else {
      card.readSector(0, bootSector);
    }

    // Filesystem found, check for floppy images
    for(int i = 1; i >= 0; --i)
      floppies[i].open(*this, floppyImageName[i]);
  } else {
    card.readSector(0, bootSector);
  }

  if((computeChecksum(bootSector) & 0xffff) == 0x1234)
    bootable = true;

  return true;
}

void BlockDev::overlayBoot(uint8_t *target, int bootChainLun) {
  // Inject the driver blob boot sector
  memcpy(target, boot_bin, bootSize);

  // Patch boot drive LUN
  target[footer] = bootChainLun ? bootChainLun << 5 : 0;

  // Patch checksum
  target[footer+1] = 0;
  target[footer+2] = 0;
  uint16_t checksum = 0x1234 - computeChecksum(target);
  target[footer+1] = (checksum >> 8) & 0xff;
  target[footer+2] = checksum & 0xff;
}

status_t BlockDev::startSdRead(uint32_t block, int count) {
  if(block + count > blocks)
    return STATUS_INVADDR;
  if(!card.readStart(block))
    return STATUS_READERR;
  return STATUS_OK;
}

status_t BlockDev::readSd(uint8_t *data) {
  if(!card.readData(data))
    return STATUS_READERR;
  return STATUS_OK;
}

void BlockDev::stopSdRead() {
  card.readStop();
}

status_t BlockDev::startSdWrite(uint32_t block, int count) {
  if(block + count > blocks)
    return STATUS_INVADDR;
  if(!card.writeStart(block))
    return STATUS_WRITEERR;
  return STATUS_OK;
}

status_t BlockDev::writeSd(uint8_t *data) {
#if ACSI_READONLY == 1
  return STATUS_WRITEERR;
#elif ACSI_READONLY == 2
  return STATUS_OK;
#else
  if(!card.writeData(data))
    return STATUS_WRITEERR;
  return STATUS_OK;
#endif
}

void BlockDev::stopSdWrite() {
#if ! ACSI_READONLY
  card.writeStop();
#endif
}

status_t BlockDev::startImageReadWrite(uint32_t block, int count) {
  if(!hdImage)
    return STATUS_NOMEDIUM;
  if((block + count) * ACSI_BLOCKSIZE > hdImage.fileSize())
    return STATUS_INVADDR;
  if(!hdImage.seekSet(block * ACSI_BLOCKSIZE))
    return STATUS_INVADDR;
  return STATUS_OK;
}

status_t BlockDev::readImage(uint8_t *data) {
  if(!hdImage.read(data, ACSI_BLOCKSIZE))
    return STATUS_READERR;
  return STATUS_OK;
}

status_t BlockDev::writeImage(uint8_t *data) {
#if ACSI_READONLY == 1
  return STATUS_WRITEERR;
#elif ACSI_READONLY == 2
  return STATUS_OK;
#else
  if(!hdImage.isWritable())
    return STATUS_WRITEERR;
  if(!hdImage.write(data, ACSI_BLOCKSIZE))
    return STATUS_WRITEERR;
  return STATUS_OK;
#endif
}

void BlockDev::stopImageWrite() {
  hdImage.flush();
}

uint32_t BlockDev::computeChecksum(uint8_t *data, int size) {
  uint32_t checksum = 0;
  for(int i = 0; i < size - 1; i += 2) {
    checksum += ((uint32_t)data[i] << 8) + (data[i+1]);
  }
  return checksum;
}

bool BlockDev::allowBootWrite(uint8_t *data) {
  int checksum = computeChecksum(data) & 0xffff;

  if(checksum != 0x1234) {
    // Boot sector not bootable: allow only if it was not previously bootable.
    return !bootable;
  }

  int bytesChanged = 0;
  for(int i = 0; i < footer && bytesChanged < 32; ++i) {
    if(data[i] != boot_bin[i])
      ++bytesChanged;
  }

  // Allow only if the new boot sector differs from the overlay.
  return bytesChanged >= 32;
}


// vim: ts=2 sw=2 sts=2 et

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
#include "RootDevice.h"

bool BlockDev::begin(RootDevice *root_, uint32_t maxBlocks_) {
  root = root_;
  maxBlocks = maxBlocks_;
  if(root->hdImage)
    image = &root->hdImage;
  return init();
}

bool BlockDev::begin(AcsiFile *image_, RootDevice *root_) {
  root = root_;
  maxBlocks = ~0;
  image = image_;

  if(image && *image)
    return init();

  return false;
}

bool BlockDev::init() {
  lastErr = AST_OK;
  lastBlock = 0;
  lastSeek = false;

  if(image)
    blocks = image->fileSize() / ACSI_BLOCKSIZE;
  else
    blocks = root->card.cardSize();

  if(!blocks) {
    end();
    return false;
  }

  if(blocks > maxBlocks)
    blocks = maxBlocks;

#if ACSI_DRIVER && ACSI_BOOT_OVERLAY
  uint8_t bootSector[ACSI_BLOCKSIZE];
  if(!readBlock(0, bootSector)) {
    end();
    return false;
  }

  if((computeChecksum(bootSector) & 0xffff) == 0x1234) {
    if(!image && bootSector[510] == 0x55 && bootSector[511] == 0xaa) {
      // Ouch, there is a valid MS-DOS header.
      // Chances are, there is a MS-DOS partition driver here,
      // potentially conflicting with ours.
      // Marking as non-bootable to avoid chain loading it.
      bootable = false;
    } else {
      bootable = true;
    }
  }
#endif

  return true;
}

void BlockDev::end() {
  root = nullptr;
  image = nullptr;
  blocks = 0;
}

Status BlockDev::startRead(uint32_t block, int count) {
  if(image)
    return startImageReadWrite(block, count);
  return startSdRead(block, count);
}

Status BlockDev::read(uint8_t *data) {
  if(image)
    return readImage(data);
  return readSd(data);
}

void BlockDev::stopRead() {
  if(!image)
    stopSdRead();
}

Status BlockDev::startWrite(uint32_t block, int count) {
  if(image)
    return startImageReadWrite(block, count);
  return startSdWrite(block, count);
}

Status BlockDev::write(uint8_t *data) {
  if(image)
    return writeImage(data);
  return writeSd(data);
}

void BlockDev::stopWrite() {
  if(image)
    stopImageWrite();
  else
    stopSdWrite();
}

Status BlockDev::startSdRead(uint32_t block, int count) {
  acsiVerbosel(" -> SD read block=", block);
  if(block + count > blocks)
    return AST_INVADDR;
  if(!root->card.readStart(block))
    return AST_READERR;
  return AST_OK;
}

Status BlockDev::readSd(uint8_t *data) {
  if(!root->card.readData(data))
    return AST_READERR;
  return AST_OK;
}

void BlockDev::stopSdRead() {
  root->card.readStop();
}

Status BlockDev::startSdWrite(uint32_t block, int count) {
  acsiVerbosel(" -> SD write block=", block);
  if(block + count > blocks)
    return AST_INVADDR;
  if(!root->card.writeStart(block))
    return AST_WRITEERR;
  return AST_OK;
}

Status BlockDev::writeSd(uint8_t *data) {
#if ACSI_READONLY == 1
  return AST_WRITEERR;
#elif ACSI_READONLY == 2
  return AST_OK;
#else
  if(!root->card.writeData(data))
    return AST_WRITEERR;
  return AST_OK;
#endif
}

void BlockDev::stopSdWrite() {
#if ! ACSI_READONLY
  root->card.writeStop();
#endif
}

Status BlockDev::startImageReadWrite(uint32_t block, int count) {
  acsiVerbosel(" -> Image r/w block=", block);
  if(!image)
    return AST_NOMEDIUM;
  if((block + count) * ACSI_BLOCKSIZE > image->fileSize())
    return AST_INVADDR;
  if(!image->seekSet(block * ACSI_BLOCKSIZE))
    return AST_INVADDR;
  return AST_OK;
}

Status BlockDev::readImage(uint8_t *data) {
  if(!image->read(data, ACSI_BLOCKSIZE))
    return AST_READERR;
  return AST_OK;
}

Status BlockDev::writeImage(uint8_t *data) {
#if ACSI_READONLY == 1
  return AST_WRITEERR;
#elif ACSI_READONLY == 2
  return AST_OK;
#else
  if(!image->isWritable())
    return AST_WRITEERR;
  if(!image->write(data, ACSI_BLOCKSIZE))
    return AST_WRITEERR;
  return AST_OK;
#endif
}

void BlockDev::stopImageWrite() {
  image->flush();
}

bool BlockDev::readBlock(uint32_t block, uint8_t *data) {
  Status status = startRead(block, 1);
  if(status)
    read(data);
  if(status)
    stopRead();
  return status;
}

uint32_t BlockDev::computeChecksum(uint8_t *data, int size) {
  uint32_t checksum = 0;
  for(int i = 0; i < size - 1; i += 2) {
    checksum += ((uint32_t)data[i] << 8) + (data[i+1]);
  }
  return checksum;
}

#if ACSI_DRIVER && ACSI_BOOT_OVERLAY >= 2
void BlockDev::patchBootSector(uint8_t *data) {
  int bytesChanged = 0;
  for(int i = 0; i < 128 && bytesChanged < 16; ++i) {
    if(data[i] != boot_bin[i])
      ++bytesChanged;
  }
  if(bytesChanged >= 16) {
    // Boot sector was changed
    acsiVerboseln("Written new boot sector");
    return;
  }

  // Fetch the old boot sector
  uint8_t oldBoot[ACSI_BLOCKSIZE];
  stopWrite();
  startRead(0, 1);
  read(oldBoot);
  stopRead();
  startWrite(0, 1);

  bool newBootable = (computeChecksum(data) & 0xffff) == 0x1234;

  // Patch the old boot sector
  memcpy(data, oldBoot, bootSize);

  // If it was previously bootable, fix its checksum
  if(newBootable) {
    data[bootSize - 2] = data[bootSize - 1] = 0;
    int checksum = 0x1234 - computeChecksum(data);
    data[bootSize - 2] = (checksum >> 8) & 0xff;
    data[bootSize - 1] = checksum & 0xff;
    acsiVerboseln("Patched boot sector checksum");
  } else {
    acsiVerboseln("Merged old boot sector");
  }
}
#endif

// vim: ts=2 sw=2 sts=2 et

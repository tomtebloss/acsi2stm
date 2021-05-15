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

#ifndef BLOCKDEV_H
#define BLOCKDEV_H

#include "Globals.h"

struct RootDevice;

struct BlockDev {
  // Initialize the block device from a SD card
  bool begin(RootDevice *root, uint32_t maxBlocks = ACSI_MAX_BLOCKS);

  // Initialize the block device from an image file
  bool begin(AcsiFile *image, RootDevice *root);

  // Uninitialize
  void end();

  // Compute the checksum of a data block
  static uint32_t computeChecksum(uint8_t *data, int size = ACSI_BLOCKSIZE);

  // Overlay a boot sector onto the target
  void overlayBoot(uint8_t *target, int bootChainLun);

  // Stream read the device
  Status startRead(uint32_t block, int count);
  Status read(uint8_t *data);
  void stopRead();

  // Stream write the device
  Status startWrite(uint32_t block, int count);
  Status write(uint8_t *data);
  void stopWrite();

  // Convenience function to read boot loaders etc...
  bool readBlock(uint32_t block, uint8_t *data);

  bool isImage() {
    return image;
  }

  operator bool() const {
    return blocks;
  }

#if ACSI_DRIVER
  // true if the block device is bootable
  bool bootable;
#endif

  RootDevice *root;
  uint32_t blocks;

  friend class Acsi; // Access to last*

protected:
  AcsiFile *image;

  uint32_t maxBlocks;
  Status lastErr;
  uint32_t lastBlock;
  bool lastSeek;

  bool init();

  // Low level access
  Status startSdRead(uint32_t block, int count);
  Status readSd(uint8_t *data);
  void stopSdRead();
  Status startSdWrite(uint32_t block, int count);
  Status writeSd(uint8_t *data);
  void stopSdWrite();
  Status startImageReadWrite(uint32_t block, int count);
  Status readImage(uint8_t *data);
  Status writeImage(uint8_t *data);
  void stopImageWrite();
#if ACSI_DRIVER && ACSI_BOOT_OVERLAY >= 2
  void patchBootSector(uint8_t *data);
#endif
};

// vim: ts=2 sw=2 sts=2 et
#endif

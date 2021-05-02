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
#include "FloppyImage.h"

// Used to parse FAT little endian numbers
static inline uint16_t readLE(const uint8_t *ptr) {
  return (uint16_t)ptr[1] << 8 | ptr[0];
}

struct BlockDev {

  void getDeviceString(char *str);

  uint32_t csPin;
  uint32_t maxBlocks;
  bool dedicatedSpi;
  int deviceId;
  uint32_t blocks;
  status_t lastErr;
  uint32_t lastBlock;
  bool lastSeek;

  BDFile hdImage;
  FloppyImage floppies[2];
  BDVolume fs;

  // Returns the SD card serial.
  // Returns 1 if the SD card serial is 0.
  // Returns 0 if no SD card is present.
  // Calls resetState if no SD card is present.
  uint32 serial();

  // Initialize the device
  bool begin(int deviceId, int csPin, uint32_t maxBlocks, bool dedicatedSpi = false);

  // Reset state to an ejected SD card
  void eject();

  // Reinitialize the sd card
  bool initSd();

  // Compute the checksum of a data block
  uint32_t computeChecksum(uint8_t *data, int size = ACSI_BLOCKSIZE);

  // Overlay a boot sector onto the target
  void overlayBoot(uint8_t *target, int bootChainLun);

  // Stream read SD card
  status_t startSdRead(uint32_t block, int count);
  status_t readSd(uint8_t *data);
  void stopSdRead();

  // Stream write SD card
  status_t startSdWrite(uint32_t block, int count);
  status_t writeSd(uint8_t *data);
  void stopSdWrite();

  // Stream read HD image
  status_t startImageReadWrite(uint32_t block, int count);
  status_t readImage(uint8_t *data);
  status_t writeImage(uint8_t *data);
  void stopImageWrite();

  // Return true if the boot sector can be written.
  bool allowBootWrite(uint8_t *data);

  // true if the SD card (or image) is bootable
  bool bootable;

protected:
  BDCard card;
};


// vim: ts=2 sw=2 sts=2 et
#endif

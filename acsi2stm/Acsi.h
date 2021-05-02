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

#ifndef ACSI_H
#define ACSI_H

#include "Globals.h"
#include "DmaPort.h"
#include "BlockDev.h"

// Parse ACSI commands
struct Acsi {
  void begin(DmaPort *dma, BlockDev *deviceMap[8]);
  void processCommand();

  status_t processRead();
  status_t processWrite();

  status_t startRead();
  status_t startWrite();

  status_t read(uint8_t *data);
  status_t write(uint8_t *data);

  void stopRead();
  void stopWrite();

  // Special LUN 7 read
  status_t startSpecialRead();
  status_t readSpecial(uint8_t *data);

  // Special LUN 7 write
  status_t startSpecialWrite();
  status_t writeSpecial(uint8_t *data);

  // Embedded driver 0x000000-0x0000ff
  status_t startDriverRead();
  status_t readDriver(uint8_t *data);

  // Device descriptor 0x100000
  status_t readDeviceDescriptor(uint8_t *data);

  // Virtual floppy information (finfo) 0x100001
  status_t readFloppyInfo(uint8_t *data);

  // Loopback buffer 0x100100
  // The code is the same for read or write
  status_t readWriteLoopback() {
    return STATUS_OK;
  }

  // Test patterns 0x100200-0x1005ff
  status_t readPattern(uint8_t *data, const char pattern[2]);

  // Debug print 0x110000-0x1cffff
  // Read or write, it doesn't matter, there is no data anyway.
  status_t startDebugReadWrite();

  // Floppy images
  FloppyInfo finfo;

  // Devices
  DmaPort *dma;
  FloppyImage *floppies[2]; // Mounted floppy images
  BlockDev *deviceMap[8]; // Maps block devices id to actual block devices

  // State
  int deviceId; // Current ACSI device id
  int lun; // Current lun
  uint32_t block; // Current block
  int count; // Number of blocks
  int modifier; // Last parameter of the command
  BlockDev *dev; // Current block device
  FloppyImage *floppy; // Current floppy
  int bootOverlay; // Device with the boot overlay enabled. -1 if disabled.
  int bootChainLun; // Device to boot chain to.

  // Reset and reload all devices
  void reload();
};

// vim: ts=2 sw=2 sts=2 et
#endif

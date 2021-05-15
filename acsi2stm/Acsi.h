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
#include "Drive.h"
#include "Floppy.h"
#include "TinyFs.h"
#include "FsMount.h"

// Parse ACSI commands
struct Acsi {
  void begin(DmaPort *dma, RootDevice *roots, int rootCount);
  void processCommand();

  Status processRead();
  Status processWrite();

  Status startRead();
  Status startWrite();

  Status read(uint8_t *data);
  Status write(uint8_t *data);

  void stopRead();
  void stopWrite();

  // Device descriptor 0x100000
  // Returns a human-readable description of everything exposed to the bus
  Status readDeviceDescription(uint8_t *data);

#if ACSI_DRIVER
  // Special LUN 7 read
  Status startSpecialRead();
  Status readSpecial(uint8_t *data);

  // Special LUN 7 write
  Status startSpecialWrite();
  Status writeSpecial(uint8_t *data);

  // Loopback buffer 0x100100
  // The code is the same for read or write
  Status readWriteLoopback() {
    return AST_OK;
  }

  // Test patterns 0x100200-0x1005ff
  Status readPattern(uint8_t *data, const char pattern[2]);

  // Debug print 0x110000-0x1cffff
  // Read or write, it doesn't matter, there is no data anyway.
  Status startDebugReadWrite();

  // Virtual filesystem information (finfo) 0x100001
  Status readFsInfo(uint8_t *data);

  // Embedded driver 0x000000-0x0000ff
  Status startDriverRead();
  Status readDriver(uint8_t *data);

  // Mount all drives
  void mountDrives();

#if ACSI_BOOT_OVERLAY
  void computeBootOverlay();
  void overlayBoot(uint8_t *target);
#endif
#endif

  // Devices
  DmaPort *dma;
  RootDevice *roots; // Physical SD cards
  int rootCount; // Number of physical SD cards
  BlockDev *devices[8]; // ACSI devices on the DMA port
#if ACSI_DRIVER
  static const int maxDrives = 26; // Maximum number of drives
  Floppy floppies[2]; // Floppy images
  Drive drives[maxDrives]; // Mounted drives
#endif

protected:

  // State
  int deviceId; // Current ACSI device id
  int lun; // Current lun
  uint32_t block; // Current block
  int count; // Number of blocks
  int control; // Last parameter of the command
  BlockDev *dev; // Current device
#if ACSI_DRIVER
  int driveId; // Current drive id
  Drive *drv; // Current drive
  static const int maxFs = maxSd;
  FsMount fs[maxFs]; // Mounted filesystems
  TinyFileDescriptor fd[maxFd]; // File descriptors
  FsMount *curFs;
#if ACSI_BOOT_OVERLAY
  int bootOverlay; // Device with the boot overlay enabled. -1 if disabled.
#endif
#endif

  // Reset and reload all devices
  void reload();

  // Reset and reload a single device
  bool reloadDevice(int id);

  // Reset and reload a single mounted device
  void reloadDrive(int id);
};

// vim: ts=2 sw=2 sts=2 et
#endif

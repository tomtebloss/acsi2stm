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

#ifndef ACSI2STM_H
#define ACSI2STM_H

#include <Arduino.h>

// acsi2stm global configuration

#define ACSI2STM_VERSION "2X1"

// Set to 1 to enable debug output on the serial port
#define ACSI_DEBUG 0

// Set to 1 to enable verbose command output on the serial port
#define ACSI_VERBOSE 0

// Number of bytes per DMA transfer to dump in verbose mode
// Set to 0 to disable data dump
#define ACSI_DUMP_LEN 16

// Set to 1 to make all SD cards readonly (returns an error if writing)
// Set to 2 to ignore writes silently (returns OK but does not actually write)
// Note: this does not apply to floppy images
#define ACSI_READONLY 0

// Set to 1 to make all floppy disks write protected
// Set to 2 to ignore writes silently (returns OK but does not actually write)
#define ACSI_FLOPPY_READONLY 0

// Set this to 1 to enable the floppy emulation driver on boot.
// When this driver is enabled, you won't be able to write to the hard drive
// boot sector.
#define ACSI_BOOT_OVERLAY 1

// Set this to 1 if you want to boot emulated floppies.
#define ACSI_BOOT_FLOPPY 1

// Set this to limit SD capacity artificially.
// Set to ~0 if you don't want any limit
#define ACSI_MAX_BLOCKS ~0 // No limit
//#define AHDI_MAX_BLOCKS 0x00FFFF // 32MB limit
//#define AHDI_MAX_BLOCKS 0x0FFFFF // 512MB limit
//#define AHDI_MAX_BLOCKS 0x1FFFFF // 1GB limit
//#define AHDI_MAX_BLOCKS 0xFFFFFF // 8GB limit

// Enable activity LED on pin C13
#define ACSI_ACTIVITY_LED 1

// Set one of these to limit filesystem support
// This reduces a lot the size of the binary
#define ACSI_FAT32_ONLY 0
#define ACSI_EXFAT_ONLY 0

// Set the watchdog timeout in milliseconds.
// The Atari ST has a 3 seconds timeout, so it's wise to set it to more than
// 3 seconds.
// Set to 0 to disable the watchdog.
#define WATCHDOG_TIMEOUT 3500

// Default image file name. It can be placed in a subfolder.
#define IMAGE_FILE_NAME "acsi2stm.img"
#define DRIVE_A_FILE_NAME "acsi2stm/drive_a.st"
#define DRIVE_B_FILE_NAME "acsi2stm/drive_b.st"

// SD card CS pin definitions
const int sdCs[] = {
  // List of SD card CS pins. CS pins must be on PA0 to PA4.
  // Use -1 to ignore an ID
  PA4, // ACSI ID 0
  PA3, // ACSI ID 1
  PA2, // ACSI ID 2
  PA1, // ACSI ID 3
  PA0, // ACSI ID 4
   -1, // ACSI ID 5
   -1, // ACSI ID 6
   -1, // ACSI ID 7
};
// Set it to the number of SD card readers
const int maxSd = 5;

// vim: ts=2 sw=2 sts=2 et
#endif

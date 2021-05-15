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

// Patched by build_release.sh from the VERSION file
#define ACSI2STM_VERSION "2.2a"

// Set to 1 to enable debug output on the serial port
#define ACSI_DEBUG 1

// Set to 1 to enable verbose command output on the serial port
#define ACSI_VERBOSE 0

// Number of bytes per DMA transfer to dump in verbose mode
// Set to 0 to disable data dump
#define ACSI_DUMP_LEN 16

// Serial port used for debug/verbose output.
#define ACSI_SERIAL Serial

// Serial port speed
#define ACSI_SERIAL_BAUD 115200

// Filter/delay data acquisition on ACK pulse.
// Set this to 1 to sample 13.8ns later
// Set this to 2 to sample 41.6ns later
// Only impacts DMA writes (ST -> STM32)
#define ACSI_ACK_FILTER 0

// Filter/delay data acquisition on CS pulse.
// Set this to 1 to sample 13.8ns later
// Set this to 2 to sample 41.6ns later
// Set this to 3 to sample 97.1ns later
// Only impacts command send (ST -> STM32)
#define ACSI_CS_FILTER 1

// Set to 1 to make all SD cards readonly (returns an error if writing)
// Set to 2 to ignore writes silently (returns OK but does not actually write)
// Note: this does not apply to floppy images nor mounted filesystems
#define ACSI_READONLY 0

// Enable the integrated ACSI driver
#define ACSI_DRIVER 1

// Boot sector overlay.
// Set this to 0 to disable boot overlay. Load the driver with emudrive.tos.
// Set this to 1 to enable the driver on boot.
// Set this to 2 to allow overlaying an already bootable boot sector.
// Set this to 3 to allow overlaying a boot sector and disable chaining.
// When this driver is enabled, the boot code overlay may interfere with the
// actual boot sector.
#define ACSI_BOOT_OVERLAY 1

// Set to 1 to log filesystem commands on the serial port
#define ACSI_FS_DEBUG 1

// Set this to limit SD capacity artificially.
// Set to ~0 if you don't want any limit
#define ACSI_MAX_BLOCKS ~0 // No limit
//#define ACSI_MAX_BLOCKS 0x00FFFF // 32MB limit
//#define ACSI_MAX_BLOCKS 0x0FFFFF // 512MB limit
//#define ACSI_MAX_BLOCKS 0x1FFFFF // 1GB limit
//#define ACSI_MAX_BLOCKS 0xFFFFFF // 8GB limit

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
  PA4,
  PA3,
//  PA2,
//  PA1,
//  PA0,
};

// ACSI id for each SD card.
const int sdId[] = {
  0,
  1,
//  2,
//  3,
//  4,
};

#if ACSI_DRIVER
// Maximum number of file descriptors. Shared between all mounted volumes.
// This value cannot be greater than 128.
static const int maxFd = 128;
#endif

#define ACSI2STM_H_DEPCHECK
#include "DepCheck.h"
#undef ACSI2STM_H_DEPCHECK

// vim: ts=2 sw=2 sts=2 et
#endif

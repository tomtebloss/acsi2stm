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

#ifndef GLOBALS_H
#define GLOBALS_H

#include "acsi2stm.h"
#include "Debug.h"
#include "Status.h"
#include <Arduino.h>
#include <SdFat.h>

static const int maxSd = sizeof(sdId) / sizeof(sdId[0]);

const int ACSI_BLOCKSIZE = 512;

// Converts a number of blocks into a user-friendly string.
// Modifies 4 chars exactly, does not zero terminate the string.
static inline void blocksToString(uint32_t blocks, char *target) {
  // Characters: 0123
  //            "213G"

  target[3] = 'k';

  uint32_t sz = (blocks + 1) / 2;
  if(sz > 999) {
    sz = (sz + 1023) / 1024;
    target[3] = 'M';
  }
  if(sz > 999) {
    sz = (sz + 1023) / 1024;
    target[3] = 'G';
  }
  if(sz > 999) {
    sz = (sz + 1023) / 1024;
    target[3] = 'T';
  }

  // Roll our own int->string conversion.
  // Libraries that do this are surprisingly large.
  for(int i = 2; i >= 0; --i) {
    if(sz)
      target[i] = '0' + sz % 10;
    else
      target[i] = ' ';
    sz /= 10;
  }
}

#if ACSI_FAT32_ONLY
typedef FatFile AcsiFile;
typedef FatVolume AcsiVolume;
#elif ACSI_EXFAT_ONLY
typedef ExFatFile AcsiFile;
typedef ExFatVolume AcsiVolume;
#else
typedef FsBaseFile AcsiFile;
typedef FsVolume AcsiVolume;
#endif
typedef SdSpiCard AcsiCard;

// Image file names
static const char *hddImageName = IMAGE_FILE_NAME;
#if ACSI_DRIVER
static const char *floppyImageNames[] = {
  DRIVE_A_FILE_NAME,
  DRIVE_B_FILE_NAME
};
#endif

#if ACSI_DRIVER
// Boot sector constants
static const int bootSize = 440;
static const int footerSize = 3;
static const int footer = bootSize - footerSize;

#if ACSI_BOOT_OVERLAY
extern const unsigned char boot_bin[];
#endif

// Big endian word
struct BEWord {
  uint8_t bytes[2];

  BEWord & operator=(uint16_t value) {
    bytes[0] = value >> 8;
    bytes[1] = value;
    return *this;
  }

  BEWord & operator |=(uint16_t value) {
    return *this = (uint16_t)*this | value;
  }

  operator const uint16_t() const {
    return (uint16_t)bytes[0] << 8 | bytes[1];
  }
};

// Big endian long
struct BELong {
  uint8_t bytes[4];

  BELong & operator=(uint32_t value) {
    bytes[0] = value >> 24;
    bytes[1] = value >> 16;
    bytes[2] = value >> 8;
    bytes[3] = value;
    return *this;
  }

  BELong & operator |=(uint32_t value) {
    return *this = (uint32_t)*this | value;
  }

  operator const uint32_t() const {
    return (uint32_t)bytes[0] << 24
      | (uint32_t)bytes[1] << 16
      | (uint32_t)bytes[2] << 8
      | (uint32_t)bytes[3];
  }
};

// Struct from emuTOS
struct Bpb
{
  BEWord recsiz;  // sector size in bytes
  BEWord clsiz;   // cluster size in sectors
  BEWord clsizb;  // cluster size in bytes
  BEWord rdlen;   // root directory length in records
  BEWord fsiz;    // FAT size in records
  BEWord fatrec;  // first FAT record (of last FAT)
  BEWord datrec;  // first data record
  BEWord numcl;   // number of data clusters available
  BEWord b_flags; // flags (bit 0 = FAT16, bit 1 = non-removable)
};

// Struct passed to the logical filesystem mounter
// This struct needs to be a multiple of 16 bytes
struct FsInfo {
  BELong dmask; // Mounted drives mask
  BEWord boot; // Boot drive
  BELong fmask; // Mounted filesystems mask
  BEWord dev; // Device number for id and bpb
  BELong id; // Drive unique id
  Bpb bpb;   // Drive BIOS parameter block
  uint8_t p[14]; // Padding
};

#endif // ACSI_DRIVER

#endif

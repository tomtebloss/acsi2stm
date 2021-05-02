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
#include <Arduino.h>
#include <SdFat.h>

const int ACSI_BLOCKSIZE = 512;

// Image file names
static const char *hddImageName = IMAGE_FILE_NAME;
static const char *floppyImageName[] = {
  DRIVE_A_FILE_NAME,
  DRIVE_B_FILE_NAME
};

// Boot sector constants
static const int bootSize = 440;
static const int footerSize = 3;
static const int footer = bootSize - footerSize;

extern const unsigned char boot_bin[];

#if ACSI_FAT32_ONLY
typedef FatFile BDFile;
typedef FatVolume BDVolume;
#elif ACSI_EXFAT_ONLY
typedef ExFatFile BDFile;
typedef ExFatVolume BDVolume;
#else
typedef FsBaseFile BDFile;
typedef FsVolume BDVolume;
#endif
typedef SdSpiCard BDCard;

typedef enum {
  // Success codes
  STATUS_OK = 0x0000, // Successful operation and end of stream.
  STATUS_CONTINUE = 0x10000, // There are more blocks to come.
  STATUS_PARTIAL = 0x20000, // The block is partial. The number of bytes is at offset 511.
  STATUS_EMPTY = 0x30000, // Successful, but no data.

  // SCSI status codes in KEY_ASC format (see SCSI KCQ)
  STATUS_NOMEDIUM = 0x023a,
  STATUS_READERR = 0x0311,
  STATUS_WRITEERR = 0x0303,
  STATUS_OPCODE = 0x0520,
  STATUS_INVADDR = 0x0521,
  STATUS_INVARG = 0x0524,
  STATUS_INVLUN = 0x0525,

  // BIOS status errors
  STATUS_ERR = 0x02ff,    // basic, fundamental error
  STATUS_E_SEEK = 0x03fa, // seek error
  STATUS_EMEDIA = 0x02f9, // unknown media
  STATUS_ESECNF = 0x05f8, // sector not found
  STATUS_EWRITF = 0x03f6, // write fault
  STATUS_EREADF = 0x03f5, // read fault
  STATUS_EWRPRO = 0x03f3, // write protect
  STATUS_E_CHNG = 0x02f2, // media change
} status_t;

static inline bool isError(status_t s) {
  return (int)s & 0xff00;
}

// Big endian word
struct BEWord {
  uint8_t bytes[2];

  BEWord& operator=(uint16_t value) {
    bytes[0] = value >> 8;
    bytes[1] = value;
    return *this;
  }

  operator const uint16_t() const {
    return (uint16_t)bytes[0] << 8 | bytes[1];
  }
};

// Big endian long
struct BELong {
  uint8_t bytes[4];

  BELong& operator=(uint32_t value) {
    bytes[0] = value >> 24;
    bytes[1] = value >> 16;
    bytes[2] = value >> 8;
    bytes[3] = value;
    return *this;
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
  BEWord b_flags; // flags (see below)
};

// Struct passed to the floppy emulator
// This struct needs to be a multiple of 16 bytes
struct FloppyInfo {
  uint8_t mask; // Virtual floppy mask
  uint8_t lerr; // Last error
  BELong id[2]; // Floppy unique id
  Bpb bpb[2];   // Floppy BIOS parameter block
  uint8_t p[2]; // Padding
  void setLErr(status_t s) {
    lerr = (uint8_t)(int)s;
  }
};

#endif

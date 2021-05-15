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

#ifndef STATUS_H
#define STATUS_H

#include "Globals.h"

typedef enum {
  // Success codes
  AST_OK = 0x00000000, // Successful operation and end of stream.

#if ACSI_DRIVER
  // BIOS status codes
  ASTB_ERR = 0xffff00,
  ASTB_NOMEDIUM = 0xf9f900,
  ASTB_READERR = 0xf5f500,
  ASTB_WRITEERR = 0xf6f600,
  ASTB_INVADDR = 0xf8f800,
  ASTB_INVLUN = 0xffff00,
  ASTB_EWRPRO = 0xf3f300,
  ASTB_E_CHNG = 0xf2f200,
  ASTB_EFILNF = 0xdfdf00,
  ASTB_EPTHNF = 0xdede00,
  ASTB_EIHNDL = 0xdbdb00,
  ASTB_EACCDN = 0xdcdc00,
  ASTB_ENHNDL = 0xdddd00,
  ASTB_ENMFIL = 0xcfcf00,
#endif

  // Status codes in BIOS+KEY+ASC format
  AST_ERROR = 0xff02ff,
  AST_NOMEDIUM = 0xf9023a,
  AST_READERR = 0xf50311,
  AST_WRITEERR = 0xf60303,
  AST_OPCODE = 0xff0520,
  AST_INVADDR = 0xf80521,
  AST_INVARG = 0xff0524,
  AST_INVLUN = 0xff0525,
  AST_EWRPRO = 0xf30303,
  AST_E_CHNG = 0xf20000,
} status_e;

struct Status {
  Status() {
    v.i = 0;
    v.s.length = 512 / 16;
  }

  Status(const Status &from) = default;

  Status(status_e value) {
    v.i = (int)value;
    if(*this)
      v.s.length = 512 / 16;
    else
      v.s.length = 0;
  }

  Status(status_e value, int bytes) {
    v.i = (int)value;
    v.s.length = (bytes + 15) / 16;
  }

  Status(char biosErr, uint8_t acsiErr, uint8_t acsiStatus, int bytes = 512) {
    v.s.biosErr = biosErr;
    v.s.acsiErr = acsiErr;
    v.s.acsiStatus = acsiStatus;
    v.s.length = (bytes + 15) / 16;
  }

  // Returns true if the status is success
  operator const bool() {
    return !(v.s.acsiStatus);
  }

  // Use the BIOS error code as ACSI status
  Status & useBiosErr() {
    v.s.acsiStatus = v.s.biosErr;
    return *this;
  }

  // Number of bytes to send during the DMA operation
  int bytes() const {
    return (int)v.s.length * 16;
  }

  uint8_t acsiStatus() const {
    return v.s.acsiStatus;
  }

  uint8_t acsiLastErr() const {
    return v.s.acsiErr;
  }

  char biosErr() const {
    return v.s.biosErr;
  }

private:
  union {
    struct {
      uint8_t acsiErr; // ACSI error, returned in the lastErr field
      uint8_t acsiStatus; // Status code sent to the ACSI bus
      char biosErr; // BIOS error code / GEMDOS return value
      char length; // Length in 16 byte blocks
    } s;
    int i;
  } v;
};

#endif

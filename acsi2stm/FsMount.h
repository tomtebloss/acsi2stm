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

#ifndef FS_MOUNT_H
#define FS_MOUNT_H

#include "Globals.h"

#if ACSI_DRIVER

#include "TinyFs.h"

struct FsMount {
  bool begin(AcsiVolume *vol, TinyFileDescriptor *fd);

  // Virtual block device interface for command transfers
  Status writeCommand(uint8_t *data, int command);
  Status readCommand(uint8_t *data, int command);

  // Virtual block device interface for data transfers

  // Read/write one block of 512 bytes from a file descriptor.
  // Returns 0 in case of success (normal behavior).
  Status read(int fdId, uint8_t *data);
  Status write(int fdId, uint8_t *data);

  // Read/write bytes (up to 127) from a file descriptor.
  // Returns the number of bytes effectively read/written.
  // Returns a negative number in case of error (normal behavior).
  Status readBytes(int fdId, uint8_t *data, int bytes);
  Status writeBytes(int fdId, uint8_t *data, int bytes);

  void stopWrite(); // Flush to SD card

  operator bool() const {
    return vol && vol->fatType();
  }

protected:
  // Maximum path length in bytes
  static const int pathLen = 510;

  enum Command {
    GD_DCREATE = 57,
    GD_DDELETE = 58,
    GD_DSETPATH = 59,
    GD_FCREATE = 60,
    GD_FOPEN = 61,
    GD_FCLOSE = 62,
    GD_FDELETE = 65,
    GD_DGETPATH = 71,
    GD_FSFIRST = 78,
    GD_FSNEXT = 79,
    GD_FRENAME = 86,
  };

  static const uint8_t ATTRIB_READONLY = 0x01;
  static const uint8_t ATTRIB_HIDDEN = 0x02;
  static const uint8_t ATTRIB_SYSTEM = 0x04;
  static const uint8_t ATTRIB_LABEL = 0x08;
  static const uint8_t ATTRIB_SUBDIR = 0x10;
  static const uint8_t ATTRIB_ARCHIVE = 0x20;

  // Data structures
  struct DTA {
    // Reserved fields
    // The values are made to be as unique as possible to avoid messing with
    // a DTA not owned by us.
    char magic[2]; // Must contain 0x01,0x02 for the DTA to be valid
    char drive; // Drive number. Set by the ST.
    uint8_t flags; // "attrib" field.
    TinyFile file; // The current file.
    char pattern[11];

    // Standard fields
    uint8_t attrib;
    BEWord time;
    BEWord date;
    BELong length;
    char filename[14];
  };

  // Command structures (suffixed "W" for write operation, "R" for read)

  struct FopenW {
    char path[pathLen];
    BEWord mode;
  };

  struct FsfirstW {
    char file[pathLen];
    BEWord attr;
  };

  struct FrenameW {
    char from[256];
    char to[256];
  };

  // Data persisting between a command write and a command read call.
  // It can be static because it's guaranteed that no other command will be
  // sent between the write and the read call.
  struct CommandContext {
    Command cmd; // Current command
    DTA dta; // The current DTA
    Status status; // Command return status
    TinyFile file; // File for read/write operations
    uint32_t fileOpSize; // Number of bytes for read/write
  };

  // Commands implementation

  Status writeDcreate(const char *data);
  Status writeDdelete(const char *data);
  Status writeDsetpath(const char *path);
  Status readDgetpath(char *path);
  Status writeFopen(FopenW *data, bool create = false);
  Status writeFclose(const uint8_t *data);
  Status writeFdelete(const char *data);
  Status writeFsfirst(FsfirstW *data);
  Status writeFsnext(DTA *data);
  Status readFsfirstNext(DTA *data);
  Status writeFrename(const FrenameW *data);

  static bool attribsMatch(AcsiFile *f, uint8_t flags);

  AcsiVolume *vol; // SD filesystem
  TinyFileDescriptor *fd; // File descriptor array (size is maxFd)
  TinyPath curPath; // Current path for this mount point

  int newFd();

  static CommandContext ctx;
};


#endif

// vim: ts=2 sw=2 sts=2 et
#endif

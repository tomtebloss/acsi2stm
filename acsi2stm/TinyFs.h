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

#ifndef TINY_FS_H
#define TINY_FS_H

#include "Globals.h"

#if ACSI_DRIVER

// Stores a path to a directory or a file in the most efficient way possible
struct __attribute__((__packed__)) TinyPath {
  void getAbsolute(AcsiVolume &volume, AcsiFile *target, oflag_t oflag = O_RDONLY, AcsiFile *parent = nullptr);

  // Get the absolute path as a string in Atari format.
  bool getAbsolute(AcsiVolume &volume, char *target, int bufSize);

  void getParent(AcsiVolume &volume, AcsiFile *targetDir);

  // Set from an absolute or a relative path
  // create can be
  //   0 to create nothing,
  //   1 to create a file,
  //   2 to create a directory,
  //   -1 to open the parent directory (the file must not exist)
  bool set(AcsiVolume &volume, const char *path, int create = 0);

  // Create a file
  void create(AcsiVolume &volume, const char *path) {
    set(volume, path, 1);
  }

  // Create a directory
  // Creates its parents on the way
  void mkdir(AcsiVolume &volume, const char *path) {
    set(volume, path, 2);
  }

  // Set to root directory
  void clear() {
    indexes[0] = 0;
  }

  bool isRoot() {
    return indexes[0];
  }

  // Read the next unicode character and convert it to Atari
  // Converts everything to upper case.
  // Returns the number of consumed bytes.
  // Character 0 is considered as a valid character and will be converted.
  static int getNextUnicode(const char *source, char *target);

  // Convert an atari character to Unicode and write it to the target buffer.
  // Returns the number of bytes written.
  static int appendUnicode(char atariChar, char *target, int bufSize); // XXX USED BY patternToUnicode

  // Convert the last processed pattern to unicode.
  // Returns the number of bytes written.
  // Warning: does not zero-terminate the string.
  static int patternToUnicode(char *target, int bufSize); // XXX USED

  // Convert the last processed file name to Atari encoding.
  // Returns the number of bytes written.
  // Warning: does not zero-terminate the string.
  static int nameToAtari(char *target, int bufSize); // XXX USED

  // Convert the file's name to Atari encoding.
  // Returns the number of bytes written.
  // Warning: does not zero-terminate the string.
  static int nameToAtari(AcsiFile *file, char *target, int bufSize); // XXX USED

  // Returns true if the name matches pattern.
  static bool matches(const char pattern[11]); // XXX USED

  // Parse an Atari path element and store it to the current pattern.
  // Returns a pointer to the next element in the path.
  static const char * parseAtariPattern(const char *path); // XXX USED

  // Parse a unicode name to an atari name. Use lastName to get the result.
  // Returns a pointer to the next element in the path.
  static const char * parseUnicodeName(const char *path); // XXX USED

  static const char * lastName() {
    return name;
  }
  static const char * lastPattern() {
    return pattern;
  }

protected:
  static const int maxDepth = 24;
  uint16_t indexes[maxDepth];
  static char name[11];
  static char pattern[11];
};

// A 6-byte file descriptor.
// Because that's all GEMDOS lets us to store data ...
// The AcsiFile object is valid until another function returning an AcsiFile
// pointer is called.
struct __attribute__((__packed__)) TinyFile {
  // Open a file from a path
  AcsiFile * open(AcsiVolume &volume, TinyPath &path, oflag_t oflag = O_RDONLY);

  // Re-open a file
  AcsiFile * acquire(AcsiVolume &volume, oflag_t oflag = O_RDONLY);

  // Open the next file in the parent directory
  AcsiFile * openNext(AcsiVolume &volume, oflag_t oflag = O_RDONLY, const char pattern[11] = "???????????");

  // Open the first file in the parent directory
  AcsiFile * openFirst(AcsiVolume &volume, oflag_t oflag = O_RDONLY, const char pattern[11] = "???????????");

  // Open the first file in the directory specified in path
  AcsiFile * openFirst(AcsiVolume &volume, TinyPath &path, oflag_t oflag = O_RDONLY, const char pattern[11] = "???????????");

  // Returns true if the file descriptor is valid
  operator bool() const {
    return index;
  }

  // Returns the last opened/acquired file descriptor
  // Be very careful when handling multiple TinyFile at the same time !
  AcsiFile * lastAcquired() {
    return &g.f;
  }

  // Returns the directory containing the last opened/acquired file
  // Be very careful when handling multiple TinyFile at the same time !
  static AcsiFile * lastDir() {
    return &g.dir;
  }

  // Close the file descriptor
  void close();

protected:
  struct Global {
    AcsiFile dir;
    AcsiFile f;
    uint32_t lastAcquiredCluster;
    uint16_t lastAcquiredIndex;
  };
  static Global g;

  static void findNextMatching(const char pattern[11], oflag_t oflag);

  // These methods do very dirty shenaningans
  // If the SdFat library changes, some offsets will need to be adjusted.
  static uint32_t getCluster(AcsiFile &f);
  static void setCluster(AcsiFile &f, uint32_t cluster);

  uint32_t dirCluster;
  uint16_t index; // Index in the containing directory + 1. 0 means file closed.
};

struct TinyFileDescriptor: public TinyFile {
  int read(AcsiVolume &volume, uint8_t *data, int bytes);
  int write(AcsiVolume &volume, uint8_t *data, int bytes);
  uint32_t seek(AcsiVolume &volume, uint32_t offset);

  AcsiFile * open(AcsiVolume &volume, TinyPath &path, oflag_t oflag = O_RDONLY) {
    position = 0;
    return TinyFile::open(volume, path, oflag);
  }

protected:
  uint32_t position;
};

#endif // ACSI_DRIVER

#endif

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

#ifndef DRIVE_H
#define DRIVE_H

#include "Globals.h"

#if ACSI_DRIVER

struct BlockDev;

struct Drive {
  // Mount a drive. If partition == -1, ignore the partition table.
  bool begin(BlockDev *dev, int partition = -1);

  void end();

  // Stream read the device
  Status startRead(uint32_t block, int count);
  Status read(uint8_t *data);
  void stopRead();

  // Stream write the device
  Status startWrite(uint32_t block, int count);
  Status write(uint8_t *data);
  void stopWrite();

  // Low level raw access
  Status startRawRead(int track, int side, int sector, int count) {
    return startRead(geometryToBlock(track, side, sector), count);
  }
  Status startRawWrite(int track, int side, int sector, int count) {
    return startRead(geometryToBlock(track, side, sector), count);
  }

  // Get filesystem information
  void computeFInfo(BELong &id, Bpb &bpb);

  operator bool() const {
    return blocks;
  }

  friend class Acsi; // For reloadDrive

protected:
  // Underlying block device
  BlockDev *dev;

  // Partition
  size_t offset;
  size_t blocks;

  // Image geometry
  int sides;
  int sectors;

  uint32_t geometryToBlock(int track, int side, int sector) {
    return track * sides * sectors + side * sectors + sector - 1;
  }

  void guessImageGeometry();

  // Convert status to BIOS format
  Status status(Status s);
};

#endif

// vim: ts=2 sw=2 sts=2 et
#endif

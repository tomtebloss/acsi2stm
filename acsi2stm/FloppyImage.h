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

#ifndef FLOPPY_IMAGE_H
#define FLOPPY_IMAGE_H

#include "Globals.h"

struct BlockDev;

struct FloppyImage {
  // Returns 
  bool open(BlockDev &dev, const char *fileName);
  operator bool() {
    return file;
  }

  status_t startReadWrite(uint32_t block, int count);
  status_t startRawReadWrite(int track, int side, int sector, int count);
  status_t read(uint8_t *data);
  status_t write(uint8_t *data);
  void stopWrite();
  void computeFInfo(BELong &id, Bpb &bpb);

  bool bootable;
protected:
  void guessImageGeometry();
  static uint32_t floppyAddressToBlock(int track, int side, int sector);
  BDFile file;
  int sectors;
  int sides;
};

#endif

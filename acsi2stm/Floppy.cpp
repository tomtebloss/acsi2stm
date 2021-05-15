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

#include "Globals.h"

#if ACSI_DRIVER

#include "Floppy.h"
#include "RootDevice.h"

bool Floppy::begin(RootDevice *root_, const char *fileName_) {
  fileName[0] = 0;

  if(!root_) {
    end();
    return false;
  }

  if(!root_->fs.exists(fileName_)) {
    end();
    return false;
  }

  if(!floppyImage.open(&root_->fs, fileName_, O_RDWR)) {
    end();
    return false;
  }

  strncpy(fileName, fileName_, 255);
  fileName[255] = 0;

  return BlockDev::begin(&floppyImage, root_);
}

#endif

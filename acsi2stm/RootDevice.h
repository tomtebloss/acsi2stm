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

#ifndef ROOT_DEVICE_H
#define ROOT_DEVICE_H

#include "Globals.h"
#include "BlockDev.h"

struct Acsi;

struct RootDevice {
  void begin(int slot, int deviceId, int csPin, bool dedicatedSpi = false);
  bool init();

  int slot; // Slot number (only for pretty printing)
  int deviceId; // Device id on the DMA bus
  BlockDev device; // The main block device
  AcsiCard card; // The SD card storing data
  AcsiVolume fs; // Filesystem on the SD card
  AcsiFile hdImage; // Hard drive image, if any

  void getDeviceString(char*);
  uint32_t serial(); // SD card serial number

  friend class Acsi; // For Acsi::readDeviceDescription

protected:
  uint32_t csPin;
  bool dedicatedSpi;

  void getFormatString(char *target);
};

// vim: ts=2 sw=2 sts=2 et
#endif

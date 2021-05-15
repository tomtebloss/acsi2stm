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

#include "RootDevice.h"
#include "Acsi.h"

void RootDevice::begin(int slot_, int deviceId_, int csPin_, bool dedicatedSpi_) {
  slot = slot_;
  deviceId = deviceId_;
  csPin = csPin_;
  dedicatedSpi = dedicatedSpi_;
}

bool RootDevice::init() {
  if(!card.begin(SdSpiConfig(csPin, dedicatedSpi ? DEDICATED_SPI : SHARED_SPI))) {
    device.end();
    return false;
  }

  // Search for a disk image
  if(fs.begin(&card))
    hdImage.open(&fs, hddImageName, O_RDWR);

  device.begin(this, hdImage ? ~0 : ACSI_MAX_BLOCKS);

  return true;
}

void RootDevice::getDeviceString(char *target) {
  // Characters:  0123456789012345678901234
  memcpy(target, "ACSI2STM SD0  RAW  ???M " ACSI2STM_VERSION, 29);

  target[11] += slot;

  if(!serial()) {
    memcpy(&target[13], "NO SD CARD", 10);
    return;
  }

  // Print format
  getFormatString(&target[13]);

  // Write SD card size
  blocksToString(device.blocks, &target[18]);

  // Add a + symbol if capacity is artificially capped
  if(!hdImage && card.cardSize() > device.blocks)
    target[23] = '+';
}

void RootDevice::getFormatString(char *target) {
  // Print format
  if(hdImage)
    memcpy(target, "IMAGE", 5);
  else if(fs.fatType() == FAT_TYPE_FAT16)
    memcpy(target, "FAT16", 5);
  else if(fs.fatType() == FAT_TYPE_FAT32)
    memcpy(target, "FAT32", 5);
  else if(fs.fatType() == FAT_TYPE_EXFAT)
    memcpy(target, "EXFAT", 5);
  else
    memcpy(target, " RAW ", 5);
}

uint32_t RootDevice::serial() {
  cid_t cid;
  if(!card.readCID(&cid))
    return 0;

  if(!cid.psn)
    return 1; // Return 1 if the serial is actually 0 to differentiate from an error
  return cid.psn;
}

// vim: ts=2 sw=2 sts=2 et

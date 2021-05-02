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

// Requires the SdFat library by Bill Greiman

// Edit acsi2stm.h to change global values
#include "acsi2stm.h"

// Includes
#include "Debug.h"
#include "DmaPort.h"
#include "Acsi.h"
#include "BlockDev.h"

// Globals

DmaPort dma;
BlockDev blockDevices[maxSd];
Acsi acsi;

// Main setup function
void setup() {
  Serial.begin(115200); // Init the serial port only if needed

  // Send a few characters to synchronize autoconfigured USB-serial dongles.
  acsiDbgln("");
  delay(50);
  acsiDbgln("");
  delay(100);

  Serial.println("-----------------------");
  Serial.println("ACSI2STM SD bridge v" ACSI2STM_VERSION);
  Serial.println("-----------------------");
  Serial.println("");

  // Initialize the ACSI port
  dma.begin();

  // Map block devices to device id
  int sdCount = 0;
  int sdFound = 0;

  BlockDev *deviceMap[8];
  for(int i = 0; i < 8; ++i) {
    if(sdCs[i] == -1) {
      deviceMap[i] = nullptr;
    } else if(sdCount < maxSd) {
      deviceMap[i] = &blockDevices[sdCount];
      dma.addDevice(i);
      sdCount++;
    }
  }

  Serial.println("Initializing the ACSI bridge ...");

  acsi.begin(&dma, deviceMap);

  for(int i = 0; i < sdCount; ++i) {
    BlockDev &dev = blockDevices[i];

    Serial.print("ACSI device ");
    Serial.print(i);
    Serial.print(':');

    if(dev.blocks) {
      char str[32];
      dev.getDeviceString(str);
      // Substitute the atari logo
      if(str[22] == 0x0e) {
        memcpy(&str[22], "/|\\", 4);
      } else {
        str[24] = 0;
      }
      Serial.println(str);
      if(dev.hdImage)
        Serial.println("    Has a hard drive image");
      if(dev.floppies[0]) {
        Serial.print("    Has ");
        if(dev.floppies[0].bootable)
          Serial.print("bootable ");
        Serial.println("drive A");
      }
      if(dev.floppies[1]) {
        Serial.print("    Has ");
        if(dev.floppies[1].bootable)
          Serial.print("bootable ");
        Serial.println("drive B");
      }
      ++sdFound;
    } else if(i == acsi.bootOverlay) {
      Serial.println("Floppy simulator");
      switch(acsi.bootChainLun) {
        case 1:
          Serial.println("    Boots on drive A");
          break;
        case 2:
          Serial.println("    Boots on drive B");
          break;
        case 5:
          Serial.print("    Boots on hard drive ");
          Serial.println(acsi.bootOverlay);
          break;
      }
    } else {
      Serial.println("No SD card");
    }
  }

  Serial.print(sdFound);
  Serial.println(" SD cards found");
  Serial.println("");
  Serial.println("Waiting for the ACSI bus");

  dma.waitBusReady();

  Serial.println("");
  Serial.println("--- Ready to go ---");
  Serial.println("");
}

// Main loop
void loop() {
  acsi.processCommand();
  return;
}

// vim: ts=2 sw=2 sts=2 et

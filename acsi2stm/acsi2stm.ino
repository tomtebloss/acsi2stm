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
#include "RootDevice.h"

static_assert(sizeof(sdId) == sizeof(sdCs), "SD card CS pins and device id must match. Check sdCs and sdId arrays");

// Globals

DmaPort dma;
RootDevice roots[maxSd];
Acsi acsi;

// Main setup function
void setup() {
  ACSI_SERIAL.begin(ACSI_SERIAL_BAUD);
  while(!ACSI_SERIAL);

  // Send a few characters to synchronize autoconfigured USB-serial dongles.
  acsiDbgln("");
  delay(50);
  acsiDbgln("");
  delay(100);

  ACSI_SERIAL.println("-----------------------");
  ACSI_SERIAL.println("ACSI2STM SD bridge v" ACSI2STM_VERSION);
  ACSI_SERIAL.println("-----------------------");
  ACSI_SERIAL.println("");

  // Initialize the ACSI port
  dma.begin();

  // Map block devices to device id
  for(int i = 0; i < maxSd; ++i) {
    roots[i].begin(i + 1, sdId[i], sdCs[i], maxSd == 1);
    dma.addDevice(sdId[i]);
  }

  // Start the whole ACSI bridge engine
  ACSI_SERIAL.println("Initializing the ACSI bridge...");
  acsi.begin(&dma, roots, maxSd);

  ACSI_SERIAL.println("");
  ACSI_SERIAL.println("Detected volumes:");
  acsi.readDeviceDescription(dma.dataBuf);
  ACSI_SERIAL.flush();
  ACSI_SERIAL.println((char *)dma.dataBuf);

  ACSI_SERIAL.println("Waiting for the DMA bus to be ready...");
  ACSI_SERIAL.flush();

  dma.waitBusReady();

  ACSI_SERIAL.println("");
  ACSI_SERIAL.println("--- Ready to go ---");
  ACSI_SERIAL.println("");
  ACSI_SERIAL.flush();
}

// Main loop
void loop() {
  acsi.processCommand();
  return;
}

// vim: ts=2 sw=2 sts=2 et

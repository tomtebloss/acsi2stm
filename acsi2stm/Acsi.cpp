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

#include "Acsi.h"
#include "Debug.h"
#include <libmaple/iwdg.h>

const // xxd does not generate const arrays !
#include "boot.h"

void Acsi::begin(DmaPort *dma_, BlockDev *deviceMap_[8]) {
  dma = dma_;
  memcpy(deviceMap, deviceMap_, sizeof(deviceMap));
  reload();
}

void Acsi::processCommand() {
  uint8_t cmdBuf[11];

  // Wait for a command
  uint8_t cmd = dma->waitCommand();

  // Parse device id
  deviceId = cmd >> 5;

  // Fill the command buffer before altering the command
  cmdBuf[0] = cmd;

  // Remove device id from the command
  cmd &= 0x1f;

  // Compute command length
  int cmdLen = cmd == 0x1f ? 11 : 6;
  dma->readIrq(&cmdBuf[1], cmdLen - 1);

  acsiVerboseln("");

  if(!deviceMap[deviceId]) {
    acsiDbg("No device ");
    acsiDbgln(deviceId);
    return;
  }

  // Set main device
  dev = deviceMap[deviceId];

  // Parse LUN and remove it from cmdBuf
  if(cmd == 0x1f) {
    lun = cmdBuf[2] >> 5;
    cmdBuf[2] &= 0x1f;
  } else {
    lun = cmdBuf[1] >> 5;
    cmdBuf[1] &= 0x1f;
  }

  // Set current floppy
  if(lun >= 1 && lun <= 4)
    floppy = floppies[(lun - 1) & 1];

  status_t status = STATUS_OK;

  // Execute the command
  switch(cmd) {
  default: // Unknown command
    acsiDbgln("Unknown command");
    dev->lastSeek = false;
    status = STATUS_OPCODE;
    break;
  case 0x00: // Sense
  case 0x0d: // Correction
  case 0x15: // Mode select
  case 0x1B: // Ship
    // Always succeed
    dev->lastSeek = false;
    break;
  case 0x04: // Format drive
  case 0x05: // Verify track
  case 0x06: // Format track
    dev->lastSeek = false;
    // fall through case
  case 0x03: // Request Sense
    for(int b = 0; b < cmdBuf[4]; ++b)
      dma->dataBuf[b] = 0;

    if(cmdBuf[4] <= 4) {
      dma->dataBuf[0] = dev->lastErr & 0xFF;
      if(dev->lastSeek) {
        dma->dataBuf[0] |= 0x80;
        dma->dataBuf[1] = (dev->lastBlock >> 16) & 0xFF;
        dma->dataBuf[2] = (dev->lastBlock >> 8) & 0xFF;
        dma->dataBuf[3] = (dev->lastBlock) & 0xFF;
      }
    } else {
      // Build long response in dma->dataBuf
      dma->dataBuf[0] = 0x70;
      if(dev->lastSeek) {
        dma->dataBuf[0] |= 0x80;
        dma->dataBuf[4] = (dev->lastBlock >> 16) & 0xFF;
        dma->dataBuf[5] = (dev->lastBlock >> 8) & 0xFF;
        dma->dataBuf[6] = (dev->lastBlock) & 0xFF;
      }
      dma->dataBuf[2] = (dev->lastErr >> 8) & 0xFF;
      dma->dataBuf[7] = 14;
      dma->dataBuf[12] = (dev->lastErr) & 0xFF;
      dma->dataBuf[19] = (dev->lastBlock >> 16) & 0xFF;
      dma->dataBuf[20] = (dev->lastBlock >> 8) & 0xFF;
      dma->dataBuf[21] = (dev->lastBlock) & 0xFF;
    }
    // Send the response
    dma->send(cmdBuf[4]);
    
    break;
  case 0x08: // Read block
    block = (((int)cmdBuf[1]) << 16) | (((int)cmdBuf[2]) << 8) | (cmdBuf[3]);
    count = cmdBuf[4];
    modifier = cmdBuf[5];
    dev->lastSeek = true;
    status = processRead();
    break;
  case 0x0a: // Write block
    block = (((int)cmdBuf[1]) << 16) | (((int)cmdBuf[2]) << 8) | (cmdBuf[3]);
    count = cmdBuf[4];
    modifier = cmdBuf[5];
    dev->lastSeek = true;
    status = processWrite();
    break;
  case 0x0b: // Seek
    block = (((int)cmdBuf[1]) << 16) | (((int)cmdBuf[2]) << 8) | (cmdBuf[3]);
    count = 0;
    modifier = cmdBuf[5];
    dev->lastSeek = true;
    status = processRead();
    break;
  case 0x12: // Inquiry
    // Fill the response with zero bytes
    for(uint8_t b = 0; b < cmdBuf[4]; ++b)
      dma->dataBuf[b] = 0;

    if(lun > 0)
      dma->dataBuf[0] = 0x7F; // Unsupported LUN
    dma->dataBuf[1] = 0x80; // Removable flag
    dma->dataBuf[2] = 1; // ACSI version
    dma->dataBuf[4] = 31; // Data length
    
    // Build the product string with the SD card size
    dev->getDeviceString((char *)dma->dataBuf + 8);
   
    dma->send(cmdBuf[4]);

    dev->lastSeek = false;
    break;
  case 0x1a: // Mode sense
    dev->lastSeek = false;
    switch(cmdBuf[2]) { // Sub-command
    case 0x00:
      for(uint8_t b = 0; b < 16; ++b) {
        dma->dataBuf[b] = 0;
      }
      // Values got from the Hatari emulator
      dma->dataBuf[1] = 14;
      dma->dataBuf[3] = 8;
      // Send the number of blocks of the SD card
      dma->dataBuf[5] = (dev->blocks >> 16) & 0xff;
      dma->dataBuf[6] = (dev->blocks >> 8) & 0xff;
      dma->dataBuf[7] = (dev->blocks) & 0xff;
      // Sector size middle byte
      dma->dataBuf[10] = 2;
      dma->send(16);
      break;
    case 0x04:
      for(uint8_t b = 0; b < 24; ++b) {
        dma->dataBuf[b] = 0;
      }
      // Values got from the Hatari emulator
      dma->dataBuf[0] = 4;
      dma->dataBuf[1] = 22;
      // Send the number of blocks in CHS format
      dma->dataBuf[2] = (dev->blocks >> 23) & 0xff;
      dma->dataBuf[3] = (dev->blocks >> 15) & 0xff;
      dma->dataBuf[4] = (dev->blocks >> 7) & 0xff;
      // Hardcode 128 heads
      dma->dataBuf[5] = 128;
      dma->send(24);
      break;
    default:
      status = STATUS_INVARG;
      break;
    }
    break;
  case 0x1f: // ICD extended command
    switch(cmdBuf[1]) { // Sub-command
    case 0x25: // Read capacity
      // Send the number of blocks of the SD card
      dma->dataBuf[0] = (dev->blocks >> 24) & 0xff;
      dma->dataBuf[1] = (dev->blocks >> 16) & 0xff;
      dma->dataBuf[2] = (dev->blocks >> 8) & 0xff;
      dma->dataBuf[3] = (dev->blocks) & 0xff;
      // Send the block size (which is always 512)
      dma->dataBuf[4] = 0x00;
      dma->dataBuf[5] = 0x00;
      dma->dataBuf[6] = 0x02;
      dma->dataBuf[7] = 0x00;
      
      dma->send(8);

      break;
    case 0x28: // Read blocks
      // Compute the block number
      block = (((int)cmdBuf[3]) << 24) | (((int)cmdBuf[4]) << 16) | (((int)cmdBuf[5]) << 8) | (cmdBuf[6]);
      count = (((int)cmdBuf[8]) << 8) | (cmdBuf[9]);
      modifier = cmdBuf[10];

      // Do the actual read operation
      status = processRead();
      break;
    case 0x2a: // Write blocks
      // Compute the block number
      block = (((int)cmdBuf[3]) << 24) | (((int)cmdBuf[4]) << 16) | (((int)cmdBuf[5]) << 8) | (cmdBuf[6]);
      count = (((int)cmdBuf[8]) << 8) | (cmdBuf[9]);
      modifier = cmdBuf[10];

      // Do the actual write operation
      status = processWrite();
      break;
    }
  }

  if(!isError(status)) {
    acsiVerboseln("Success");
  } else {
    acsiDbg("Error ");
    acsiDbgln((int)status, HEX);
  }

  dma->sendIrq(((int)status) >> 8);
}

status_t Acsi::processRead() {
  acsiVerbosel("Read dev=", deviceId, " lun=", lun, " block=", block, " count=", count, " mod=", modifier);

  status_t status = startRead();
  for(; status == STATUS_OK && count > 0; ++block, --count) {
    status = read(dma->dataBuf);

    if(status == STATUS_PARTIAL) {
      if(dma->dataBuf[ACSI_BLOCKSIZE - 1])
        dma->send(dma->dataBuf[ACSI_BLOCKSIZE - 1]);
      else
        dma->send(256);
    } else if(!isError(status)) {
      dma->send();
    }
  }
  stopRead();
  return status;
}

status_t Acsi::processWrite() {
  acsiVerbosel("Write dev=", deviceId, " lun=", lun, " block=", block, " count=", count, " mod=", modifier);

  status_t status = startWrite();
  for(; status == STATUS_OK && count > 0; ++block, --count) {
    dma->read();
    status = write(dma->dataBuf);
  }
  stopWrite();
  return status;
}

void Acsi::reload() {
  acsiDbgln("Reloading SD cards");

  for(int i = 0; i < 2; ++i) {
    floppies[i] = nullptr;
    finfo.id[i] = 0;
  }
  finfo.mask = 0;

  bootOverlay = -1;
  bootChainLun = 0;
  bool shouldOverlayBoot = false;
  for(int i = 7; i >= 0; --i) {
    if(deviceMap[i]) {
      dma->watchdog.feed();
      BlockDev *dev = deviceMap[i];
      if(dev->begin(i, sdCs[i], ACSI_MAX_BLOCKS, maxSd == 1)) {
        for(int i = 0; i < 2; ++i) {
          if(dev->floppies[i]) {
            floppies[i] = &dev->floppies[i];
            finfo.id[i] = dev->serial();
            shouldOverlayBoot = true;
          }
        }
      } else {
        shouldOverlayBoot = true;
      }
      bootOverlay = i;
    }
  }

#if ACSI_BOOT_OVERLAY
  if(shouldOverlayBoot) {
    if(deviceMap[bootOverlay]->bootable) {
      bootChainLun = 5; // The overlay overlays a boot sector: chain boot it.
    }
  }
#else
  bootOverlay = -1;
#endif

  for(int i = 1; i >= 0; --i) {
    if(floppies[i]) {
      floppies[i]->computeFInfo(finfo.id[i], finfo.bpb[i]);
      finfo.mask |= 1 << i;
#if ACSI_BOOT_FLOPPY && ACSI_BOOT_OVERLAY
      if(floppies[i]->bootable) 
        bootChainLun = i + 1; // Bootable floppy !
#endif
    }
  }
}

status_t Acsi::startRead() {
  if(block == 0 && lun < 6)
    // Reading boot sector: refresh
    reload();

  if(block == 0x00001 && lun >= 3 && lun <= 4)
    // Reading boot sector via raw read: refresh
    reload();

  status_t s;
  switch(lun) {
    case 0:
#if ACSI_BOOT_OVERLAY
      if(dev->blocks == 0 && block == 0 && deviceId == bootOverlay)
        // Even with no drive present, Allow reading the overlay
        return STATUS_OK;
#endif
    case 5:
      if(dev->hdImage) {
        acsiDbgl("Read IMG", deviceId, " block=", block, " count=", count);
        return dev->startImageReadWrite(block, count);
      }
      acsiDbgl("Read SD", deviceId, " block=", block, " count=", count);
      return dev->startSdRead(block, count);
    case 1:
    case 2:
      acsiDbgl("Read drive ", (char)('A' + ((lun - 1) & 1)), " block=", block, " count=", count);
      s = floppy->startReadWrite(block, count);
      finfo.setLErr(s);
      return s;
    case 3:
    case 4:
      acsiDbgl("Read drive ", (char)('A' + ((lun - 1) & 1)), " track=", (block >> 8) & 0xff, " side=", (block & 0x10000) ? 2 : 1, " sector=", block & 0xff, " count=", count);
      s = floppy->startRawReadWrite((block >> 8) & 0xff, (block & 0x10000) ? 1 : 0, block & 0xff, count);
      finfo.setLErr(s);
      return s;
    case 6:
      return STATUS_INVLUN;
    case 7:
      return startSpecialRead();
  }
  return STATUS_INVLUN;
}

status_t Acsi::startWrite() {
  status_t s;
  switch(lun) {
    case 0:
    case 5:
      if(dev->hdImage) {
        acsiDbgl("Write IMG", deviceId, " block=", block, " count=", count);
        return dev->startImageReadWrite(block, count);
      }
      acsiDbgl("Write SD", deviceId, " block=", block, " count=", count);
      return dev->startSdWrite(block, count);
    case 1:
    case 2:
      acsiDbgl("Write drive ", (char)('A' + ((lun - 1) & 1)), " block=", block, " count=", count);
      s = floppy->startReadWrite(block, count);
      finfo.setLErr(s);
      return s;
    case 3:
    case 4:
      acsiDbgl("Write drive ", (char)('A' + ((lun - 1) & 1)), " track=", (block >> 8) & 0xff, " side=", (block & 0x10000) ? 2 : 1, " sector=", block & 0xff, " count=", count);
      s = floppy->startRawReadWrite((block >> 8) & 0xff, (block & 0x10000) ? 1 : 0, block & 0xff, count);
      finfo.setLErr(s);
      return s;
    case 7:
      return startSpecialWrite();
  }
  return STATUS_INVLUN;
}

status_t Acsi::read(uint8_t *data) {
  status_t s;
  switch(lun) {
    case 0:
    case 5:
      if(dev->hdImage)
        s = dev->readImage(data);
      else
        s = dev->readSd(data);
#if ACSI_BOOT_OVERLAY
      if(block == 0 && lun == 0 && deviceId == bootOverlay) {
        acsiDbgl("Overlaying boot sector on device ", deviceId);
        dev->overlayBoot(data, bootChainLun);
        s = STATUS_OK;
      }
#endif
      break;
    case 1:
    case 2:
    case 3:
    case 4:
      s = floppy->read(data);
      finfo.setLErr(s);
      break;
    case 6:
      return STATUS_INVLUN;
    case 7:
      s = readSpecial(data);
      break;
  }
  return STATUS_OK;
}

status_t Acsi::write(uint8_t *data) {
  status_t s;
  switch(lun) {
    case 0:
#if ACSI_BOOT_OVERLAY
      if(block == 0 && deviceId == bootOverlay && !dev->allowBootWrite(data)) {
        acsiDbgln("Ignored boot sector write: overlay still present");
        return STATUS_OK;
      }
#endif
    case 5:
      if(dev->hdImage)
        s = dev->writeImage(data);
      else
        s = dev->writeSd(data);
      break;
    case 1:
    case 2:
    case 3:
    case 4:
      s = floppy->write(data);
      finfo.setLErr(s);
      break;
    case 6:
      return STATUS_INVLUN;
    case 7:
      s = writeSpecial(data);
      break;
  }
  return STATUS_OK;
}

void Acsi::stopRead() {
  switch(lun) {
    case 0:
    case 5:
      if(!dev->hdImage)
        dev->stopSdRead();
      break;
  }
}

void Acsi::stopWrite() {
  switch(lun) {
    case 0:
    case 5:
      if(dev->hdImage)
        dev->stopImageWrite();
      else
        dev->stopSdWrite();
      break;
    case 1:
    case 2:
    case 3:
    case 4:
      floppy->stopWrite();
      break;
  }
}

status_t Acsi::startSpecialRead() {
  if(block < 0x100000)
    return startDriverRead();

  // 1 block pages
  if(block < 0x100100) {
    if(count > 1)
      return STATUS_INVADDR;
    return STATUS_OK;
  }

  // 256 blocks pages
  if(block < 0x110000) {
    if((block & 0xff) || count > 255)
      // Access must be page aligned
      return STATUS_INVADDR;
    return STATUS_OK;
  }
  
  // Debug output
  return startDebugReadWrite();
}

status_t Acsi::readSpecial(uint8_t *data) {
  if(block < 0x100000)
    return readDriver(data);

  // 1 block pages
  if(block < 0x100100) {
    switch(block & 0xff) {
      case 0x00:
        return readDeviceDescriptor(data);
      case 0x01:
        return readFloppyInfo(data);
    }
    return STATUS_INVADDR;
  }

  // 256 blocks pages
  if(block < 0x110000) {
    uint32_t type = block & 0xff >> 8;
    switch(type) {
      case 1:
        return readWriteLoopback();
      case 2:
        return readPattern(data, "\x00\x00");
      case 3:
        return readPattern(data, "\xff\xff");
      case 4:
        return readPattern(data, "\x00\xff");
      case 5:
        return readPattern(data, "\x55\xaa");
    }
    if(type >= 0x10 && type <= 0x17) {
      char pattern[2];
      pattern[0] = pattern[1] = 1 << (type & 0x7);
      return readPattern(data, pattern);
    }
    if(type >= 0x18 && type <= 0x1f) {
      char pattern[2];
      pattern[0] = 0;
      pattern[1] = 1 << (type & 0x7);
      return readPattern(data, pattern);
    }
    if(type >= 0x20 && type <= 0x27) {
      char pattern[2];
      pattern[0] = pattern[1] = ~(1 << (type & 0x7));
      return readPattern(data, pattern);
    }
    if(type >= 0x28 && type <= 0x2f) {
      char pattern[2];
      pattern[0] = ~0;
      pattern[1] = ~(1 << (type & 0x7));
      return readPattern(data, pattern);
    }
  }
  return STATUS_INVADDR;
}

status_t Acsi::startSpecialWrite() {
  // Driver blocks
  // 1 block pages
  if(block < 0x100100)
    return STATUS_WRITEERR;

  // 256 blocks pages
  if(block < 0x110000) {
    if((block & 0xff) || count > 255)
      // Access must be page aligned
      return STATUS_INVADDR;
    return STATUS_OK;
  }
  
  // Debug output
  startDebugReadWrite();
  count = 0; // Prevent data transfer
  return STATUS_OK;
}

status_t Acsi::writeSpecial(uint8_t *data) {
  // 1 block pages
  if(block < 0x100100)
    return STATUS_WRITEERR;

  // 256 blocks pages
  if(block >= 0x100100 && block <= 0x1001ff)
    return readWriteLoopback();

  if(block >= 0x100200 && block <= 0x10ffff) {
    // Reference data that should be received
    uint8_t refBuf[ACSI_BLOCKSIZE];
    status_t s = readSpecial(refBuf);
    if(s != STATUS_OK)
      return s;

    // Return success if data is the same, write error otherwise.
    return memcmp(refBuf, data, ACSI_BLOCKSIZE) ? STATUS_WRITEERR : STATUS_OK;
  }

  return STATUS_INVADDR;
}

status_t Acsi::startDriverRead() {
  acsiDbgl("Read driver block=", block, " count=", count);
  if((block + count) * ACSI_BLOCKSIZE > (sizeof(boot_bin) + 511 - bootSize) & 0xfffe00)
    return STATUS_INVADDR;
  return STATUS_OK;
}

status_t Acsi::readDriver(uint8_t *data) {
  uint32_t offset = bootSize + block * ACSI_BLOCKSIZE;
  uint32_t size = sizeof(boot_bin) - offset;
  if(size > ACSI_BLOCKSIZE)
    size = ACSI_BLOCKSIZE;
  memcpy(data, &boot_bin[offset], size);
  return STATUS_OK;
}

status_t Acsi::readDeviceDescriptor(uint8_t *data) {
  acsiDbgl("Read device descriptor");
  reload();

  data[0] = 0;
  data[1] = 0;
  data[2] = 0;
  data[3] = 0;
  for(int i = 0; i < 8; ++i) {
    BlockDev *dev = deviceMap[i];
    if(dev) {
      data[0] |= 1 << i;
      dev->getDeviceString((char *)data + 24 * i + 4);
      if(dev->bootable)
        data[1] |= 1 << i;
      if(dev->floppies[0])
        data[2] |= 1 << i;
      if(dev->floppies[1])
        data[3] |= 1 << i;
    }
  }
  data[511] = 196;
  return STATUS_PARTIAL;
}

status_t Acsi::readFloppyInfo(uint8_t *data) {
  acsiDbgl("Read floppy info");
  reload();

  memcpy(data, (const uint8_t *)&finfo, sizeof(finfo));
  data[ACSI_BLOCKSIZE - 1] = sizeof(finfo);
  return STATUS_PARTIAL;
}

status_t Acsi::readPattern(uint8_t *data, const char pattern[2]) {
  for(int i = 0; i < ACSI_BLOCKSIZE; i += 2) {
    data[i] = pattern[0];
    data[i + 1] = pattern[1];
  }
  return STATUS_OK;
}

status_t Acsi::startDebugReadWrite() {
  bool lf = !(block & 0x080000);
  switch((block & 0x170000) >> 16) {

    case 0x11: // Log byte
      Serial.print(' ');
      Serial.print(block & 0xff,HEX);
      if(lf)
        Serial.println("");
      return STATUS_OK;

    case 0x12: // Log word
      Serial.print(' ');
      Serial.print(block & 0xffff,HEX);
      if(lf)
        Serial.println("");
      return STATUS_OK;

    case 0x13: // Log char
      Serial.print((char)block);
      return STATUS_OK;

    case 0x14: // Log long
      Serial.print(' ');
      Serial.print((block & 0xffff) << 16 | count << 8 | modifier,HEX);
      if(lf)
        Serial.println("");
      return STATUS_OK;

    case 0x15: // Reset
      Serial.println("");
      if(count) {
        Serial.println("--- Reset requested ---");
        iwdg_init(IWDG_PRE_4, 1);
        iwdg_feed();
        for(;;);
      } else {
        Serial.println("--- Reset triggered ---");
      }
      return STATUS_OK;
  }
  return STATUS_INVADDR;
}

// vim: ts=2 sw=2 sts=2 et

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
#include "BlockDev.h"
#include "RootDevice.h"
#include <libmaple/iwdg.h>

#if ACSI_DRIVER && ACSI_BOOT_OVERLAY
const // xxd does not generate const arrays !
#include "boot.h"
#endif

void Acsi::begin(DmaPort *dma_, RootDevice *roots_, int rootCount_) {
  dma = dma_;
  roots = roots_;
  rootCount = rootCount_;
#if ACSI_DRIVER && ACSI_BOOT_OVERLAY
  bootOverlay = -1;
#endif
  reload();
}

void Acsi::processCommand() {
  uint8_t cmdBuf[11];

  // Wait for a command
  uint8_t cmd = dma->waitCommand();

  // Parse device id
  deviceId = cmd >> 5;

  // Remove device id from the command
  cmd &= 0x1f;

  // Compute command length
  int cmdLen = cmd == 0x1f ? 11 : 6;
  dma->readIrq(&cmdBuf[1], cmdLen - 1);

  acsiVerboseln("");

  // Set main device
  dev = devices[deviceId];

  // Parse LUN and remove it from cmdBuf
  if(cmd == 0x1f) {
    lun = cmdBuf[2] >> 5;
    cmdBuf[2] &= 0x1f;
  } else {
    lun = cmdBuf[1] >> 5;
    cmdBuf[1] &= 0x1f;
  }

#if ACSI_DRIVER
  // Parse drive
  driveId = -1;
  if(cmd == 0x1f) {
    if(cmdBuf[10] < maxDrives)
      driveId = cmdBuf[10];
  } else {
    if(cmdBuf[5] < maxDrives)
      driveId = cmdBuf[5];
  }

  drv = nullptr;
  if(driveId != -1 && drives[driveId])
    drv = &drives[driveId];

  curFs = nullptr;
  if(driveId >= 2 && driveId - 2 < maxFs)
    curFs = &fs[driveId - 2];
#endif

  Status status = AST_OK;

  // Execute the command
  switch(cmd) {
  default: // Unknown command
    acsiDbgln("Unknown command");
    if(dev)
      dev->lastSeek = false;
    status = AST_OPCODE;
    break;
  case 0x00: // Sense
  case 0x0d: // Correction
  case 0x15: // Mode select
  case 0x1B: // Ship
    // Always succeed
    if(dev)
      dev->lastSeek = false;
    break;
  case 0x04: // Format drive
  case 0x05: // Verify track
  case 0x06: // Format track
    if(dev)
      dev->lastSeek = false;
    // fall through case
  case 0x03: // Request Sense
    for(int b = 0; b < cmdBuf[4]; ++b)
      dma->dataBuf[b] = 0;

    if(cmdBuf[4] <= 4) {
      dma->dataBuf[0] = dev ? dev->lastErr & 0xff : (int)AST_NOMEDIUM & 0xff;
      if(dev && dev->lastSeek) {
        dma->dataBuf[0] |= 0x80;
        dma->dataBuf[1] = (dev->lastBlock >> 16) & 0xff;
        dma->dataBuf[2] = (dev->lastBlock >> 8) & 0xff;
        dma->dataBuf[3] = (dev->lastBlock) & 0xff;
      }
    } else {
      // Build long response in dma->dataBuf
      dma->dataBuf[0] = 0x70;
      if(dev) {
        if(dev->lastSeek) {
          dma->dataBuf[0] |= 0x80;
          dma->dataBuf[4] = (dev->lastBlock >> 16) & 0xff;
          dma->dataBuf[5] = (dev->lastBlock >> 8) & 0xff;
          dma->dataBuf[6] = (dev->lastBlock) & 0xff;
        }
        dma->dataBuf[2] = (dev->lastErr >> 8) & 0xff;
        dma->dataBuf[7] = 14;
        dma->dataBuf[12] = (dev->lastErr) & 0xff;
        dma->dataBuf[19] = (dev->lastBlock >> 16) & 0xff;
        dma->dataBuf[20] = (dev->lastBlock >> 8) & 0xff;
        dma->dataBuf[21] = (dev->lastBlock) & 0xff;
      } else {
        dma->dataBuf[2] = ((int)AST_NOMEDIUM >> 8) & 0xff;
        dma->dataBuf[7] = 14;
        dma->dataBuf[12] = ((int)AST_NOMEDIUM) & 0xff;
      }
    }
    // Send the response
    dma->send(cmdBuf[4]);
    
    break;
  case 0x08: // Read block
    block = (((int)cmdBuf[1]) << 16) | (((int)cmdBuf[2]) << 8) | (cmdBuf[3]);
    count = cmdBuf[4];
    control = cmdBuf[5];
    if(dev)
      dev->lastSeek = true;
    status = processRead();
    break;
  case 0x0a: // Write block
    block = (((int)cmdBuf[1]) << 16) | (((int)cmdBuf[2]) << 8) | (cmdBuf[3]);
    count = cmdBuf[4];
    control = cmdBuf[5];
    if(dev)
      dev->lastSeek = true;
    status = processWrite();
    break;
  case 0x0b: // Seek
    block = (((int)cmdBuf[1]) << 16) | (((int)cmdBuf[2]) << 8) | (cmdBuf[3]);
    count = 0;
    control = cmdBuf[5];
    if(dev)
      dev->lastSeek = true;
    status = processRead();
    break;
  case 0x12: // Inquiry
    if(!dev) {
      status = AST_NOMEDIUM;
      break;
    }
    // Fill the response with zero bytes
    for(uint8_t b = 0; b < cmdBuf[4]; ++b)
      dma->dataBuf[b] = 0;

    if(lun > 0)
      dma->dataBuf[0] = 0x7F; // Unsupported LUN
    dma->dataBuf[1] = 0x80; // Removable flag
    dma->dataBuf[2] = 1; // ACSI version
    dma->dataBuf[4] = 31; // Data length
    
    // Build the product string with the SD card size
    dev->root->getDeviceString((char *)dma->dataBuf + 8);
   
    dma->send(cmdBuf[4]);

    dev->lastSeek = false;
    break;
  case 0x1a: // Mode sense
    if(!dev) {
      status = AST_NOMEDIUM;
      break;
    }
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
      status = AST_INVARG;
      break;
    }
    break;
  case 0x1f: // ICD extended command
    switch(cmdBuf[1]) { // Sub-command
    case 0x25: // Read capacity
      if(!dev) {
        status = AST_NOMEDIUM;
        break;
      }

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
      control = cmdBuf[10];

      // Do the actual read operation
      status = processRead();
      break;
    case 0x2a: // Write blocks
      // Compute the block number
      block = (((int)cmdBuf[3]) << 24) | (((int)cmdBuf[4]) << 16) | (((int)cmdBuf[5]) << 8) | (cmdBuf[6]);
      count = (((int)cmdBuf[8]) << 8) | (cmdBuf[9]);
      control = cmdBuf[10];

      // Do the actual write operation
      status = processWrite();
      break;
    }
  }

  if(status) {
    acsiVerboseln("Success");
  } else {
    acsiDbg("Status ");
    acsiDbgln(status.acsiStatus(), HEX);
  }

  dma->sendIrq(status.acsiStatus());
}

Status Acsi::processRead() {
  acsiVerbosel("Read dev=", deviceId, " lun=", lun, " block=", block, " count=", count, " ctrl=", control);

  Status status = startRead();
  for(; status && count > 0; ++block, --count) {
    status = read(dma->dataBuf);
    dma->send(status.bytes());
  }
  stopRead();
  return status;
}

Status Acsi::processWrite() {
  acsiVerbosel("Write dev=", deviceId, " lun=", lun, " block=", block, " count=", count, " ctrl=", control);

  Status status = startWrite();
  for(; status && count > 0; ++block, --count) {
    dma->read();
    status = write(dma->dataBuf);
  }
  stopWrite();
  return status;
}

void Acsi::reload() {
  acsiDbgln("Reloading SD cards");

  // Reset devices
  for(int i = 0; i < 8; ++i)
    devices[i] = nullptr;

#if ACSI_DRIVER
  // Reset floppy images
  for(int i = 0; i < 2; ++i)
    floppies[i].end();

  // Reset mounted drives
  for(int i = 0; i < 8; ++i)
    drives[i].end();
#endif

  // Associate ACSI devices to block devices
  for(int i = 0; i < rootCount; ++i) {
    acsiVerbosel("Initializing SD", i);
    RootDevice *root = &roots[i];
    root->init();
    if(root->device.begin(root)) {
      devices[root->deviceId] = &root->device;
      acsiVerboseln("Success");
    } else {
      acsiVerboseln("Error");
    }
  }

#if ACSI_DRIVER
  mountDrives();
#endif
}

bool Acsi::reloadDevice(int id) {
  // Find the root device for this id
  RootDevice *root = nullptr;

  if(id < 0 || id > 7)
    // Invalid id
    return false;

  acsiVerbosel("Reloading device ", id);

  if(devices[id]) {
    root = devices[id]->root;
  } else {
    // Device was dead: search the root device
    for(int i = 0; i < rootCount; ++i) {
      if(roots[i].deviceId == id) {
        root = &roots[i];
        break;
      }
    }
  }

  if(!root)
    // Invalid id, won't do anything.
    return false;

  if(root->init()) {
    devices[root->deviceId] = &root->device;
    acsiVerboseln("Success");
  } else {
    devices[root->deviceId] = nullptr;
    acsiVerboseln("No SD card");
  }

#if ACSI_DRIVER
  // TODO: optimize this: no need to remount unrelated drives
  mountDrives();
#endif

  return true;
}

Status Acsi::startRead() {
  if(block == 0 && lun <= 3)
    // Reading boot sector: refresh
    if(!reloadDevice(deviceId)) {
      acsiVerbosel("Could not reload device ", deviceId);
      dev = nullptr;
    }

#if ACSI_DRIVER
  if(block == 0x00001 && lun == 3)
    // Reading boot sector via raw read: refresh
    if(!reloadDevice(deviceId)) {
      acsiVerbosel("Could not reload device ", deviceId, " for raw access");
      dev = nullptr;
    }
#endif

  switch(lun) {
    case 0:
#if ACSI_DRIVER
#if ACSI_BOOT_OVERLAY
      if(!dev && block == 0 && count == 1 && deviceId == bootOverlay) {
        // Even with no drive present, Allow reading the overlay
        acsiDbgl("Read driver dev=", deviceId);
        return AST_OK;
      }
#endif
    case 1:
#endif
      if(!dev)
        return AST_NOMEDIUM;
      acsiDbgl("Read SD", deviceId, " block=", block, " count=", count);
      return dev->startRead(block, count);

#if ACSI_DRIVER
    case 2:
      if(!drv)
        return ASTB_NOMEDIUM;
      acsiDbgl("Read drive ", (char)('A' + driveId), " block=", block, " count=", count);
      return drv->startRead(block, count);
    case 3:
      if(!drv)
        return ASTB_NOMEDIUM;
      acsiDbgl("Read drive ", (char)('A' + driveId), " track=", (block >> 8) & 0xff, " side=", (block & 0x10000) ? 2 : 1, " sector=", block & 0xff, " count=", count);
      return drv->startRawRead((block >> 8) & 0xff, (block & 0x10000) ? 1 : 0, block & 0xff, count);
    case 7:
      return startSpecialRead();
#endif
  }
  return AST_INVLUN;
}

Status Acsi::startWrite() {
  switch(lun) {
    case 0:
#if ACSI_DRIVER
    case 1:
#endif
      if(!dev)
        return AST_NOMEDIUM;
      acsiDbgl("Write SD", deviceId, " block=", block, " count=", count);
      return dev->startWrite(block, count);

#if ACSI_DRIVER
    case 2:
      if(!drv)
        return ASTB_NOMEDIUM;
      acsiDbgl("Write drive ", (char)('A' + driveId), " block=", block, " count=", count);
      return drv->startWrite(block, count);
    case 3:
      if(!drv)
        return ASTB_NOMEDIUM;
      acsiDbgl("Write drive ", (char)('A' + driveId), " track=", (block >> 8) & 0xff, " side=", (block & 0x10000) ? 2 : 1, " sector=", block & 0xff, " count=", count);
      return drv->startRawWrite((block >> 8) & 0xff, (block & 0x10000) ? 1 : 0, block & 0xff, count);
    case 7:
      return startSpecialWrite();
#endif
  }
  return AST_INVLUN;
}

Status Acsi::read(uint8_t *data) {
  Status s;
  switch(lun) {
    case 0:
      if(dev)
        s = dev->read(data);
      else
        s = AST_NOMEDIUM;
#if ACSI_DRIVER && ACSI_BOOT_OVERLAY
      if(block == 0 && count == 1 && deviceId == bootOverlay) {
        acsiDbgl("Overlaying boot sector on device ", deviceId);
        overlayBoot(data);
        s = AST_OK;
      }
#endif
      return s;
#if ACSI_DRIVER
    case 1:
      return dev->read(data);
    case 2:
    case 3:
      return drv->read(data);
    case 7:
      return readSpecial(data);
#endif
  }
  return AST_INVLUN;
}

Status Acsi::write(uint8_t *data) {
  switch(lun) {
    case 0:
#if ACSI_DRIVER
#if ACSI_BOOT_OVERLAY >= 2
      if(block == 0 && deviceId == bootOverlay)
        dev->patchBootSector(data);
#endif
    case 1:
#endif
      return dev->write(data);
#if ACSI_DRIVER
    case 2:
    case 3:
      return drv->write(data);
    case 7:
      return writeSpecial(data);
#endif
  }
  return AST_INVLUN;
}

void Acsi::stopRead() {
  switch(lun) {
    case 0:
#if ACSI_DRIVER
    case 1:
#endif
      if(dev)
        dev->stopRead();
      break;
#if ACSI_DRIVER
    case 2:
    case 3:
      drv->stopRead();
      break;
#endif
  }
}

void Acsi::stopWrite() {
  switch(lun) {
    case 0:
#if ACSI_DRIVER
    case 1:
#endif
      dev->stopWrite();
      break;
#if ACSI_DRIVER
    case 2:
    case 3:
      drv->stopWrite();
      break;
#endif
  }
}

Status Acsi::readDeviceDescription(uint8_t *data) {
  char *target = (char *)data;

  memset(data, '\0', ACSI_BLOCKSIZE);

  // This must fit in 512 bytes !

  for(int i = 0; i < rootCount; ++i) {
    RootDevice *root = &roots[i];

    memcpy(target, "SD0", 3);
    target[2] = '0' + root->slot;
    target += 3;

    if(!root->card.cardSize()) {
      memcpy(target, " NO SD CARD\r\n", 13);
      target += 13;
      continue;
    }

    target[0] = ' ';
    blocksToString(root->card.cardSize(), &target[1]);
    target += 5;

    memcpy(target, " ACSI 0", 7);
    target[6] += root->deviceId;
    target += 7;

    target[0] = ' ';
    root->getFormatString(&target[1]);
    target += 6;

    if(root->hdImage) {
      // Image
      target[0] = ' ';
      blocksToString(root->hdImage.fileSize() / ACSI_BLOCKSIZE, &target[1]);
      target += 5;
    } else if(root->card.cardSize() != root->device.blocks) {
      // Capped device
      target[0] = ' ';
      blocksToString(root->device.blocks, &target[1]);
      target += 5;
    }

    bool newLine = false;

#if ACSI_DRIVER
#if ACSI_BOOT_OVERLAY
    if(root->device.bootable) {
      if(!newLine) {
        newLine = true;
        target[0] = '\r';
        target[1] = '\n';
        target += 2;
      }
      memcpy(target, " BOOT", 5);
      target += 5;
    }

    if(root->deviceId == bootOverlay) {
      if(!newLine) {
        newLine = true;
        target[0] = '\r';
        target[1] = '\n';
        target += 2;
      }
      memcpy(target, " OVERLAY", 8);
      target += 8;
    }
#endif

    if(fs[i]) {
      if(!newLine) {
        newLine = true;
        target[0] = '\r';
        target[1] = '\n';
        target += 2;
      }
      memcpy(target, " MOUNTED", 8);
      target += 8;
    }

    // Show mounted partitions

    for(int d = 0; d < maxDrives; ++d) {
      if(!newLine) {
        newLine = true;
        target[0] = '\r';
        target[1] = '\n';
        target += 2;
      }
      Drive *drv = &drives[d];
      if(*drv && drv->dev->root->deviceId == i) {
        target[0] = ' ';
        target[1] = 'A' + d;
        target[2] = ':';
        target += 3;
      }
    }
#endif

    target[0] = '\r';
    target[1] = '\n';
    target += 2;
  }

#if ACSI_DRIVER
  for(int f = 0; f < 2; ++f) {
    if(floppies[f]) {
      memcpy(target, "Floppy ", 7);
      target[7] = 'A' + f;
      target[8] = ':';
      target += 9;
      memcpy(target, " SD", 3);
      target[3] = '0' + floppies[f].root->slot;
      target[4] = ' ';
      blocksToString(floppies[f].blocks, &target[5]);
      target[9] = '\r';
      target[10] = '\n';
      target += 11;
    }
  }
#endif

  *target = 0;
}

#if ACSI_DRIVER
Status Acsi::startSpecialRead() {
  if(block < 0x100000)
    return startDriverRead();

  // 1 block pages
  if(block < 0x100100)
    return AST_OK;

  // 256 blocks pages
  if(block < 0x110000) {
    if((block & 0xff) || count > 255)
      // Access must be page aligned
      return AST_INVADDR;
    return AST_OK;
  }

  if(block < 0x120000 && curFs)
    return AST_OK;
  
  // Debug output
  return startDebugReadWrite();
}

Status Acsi::readSpecial(uint8_t *data) {
  if(block < 0x100000)
    return readDriver(data);

  // 1 block pages
  if(block < 0x100100) {
    Status s = AST_INVADDR;
    switch(block & 0xff) {
      case 0x00:
        s = readDeviceDescription(data);
        break;
      case 0x01:
        s = readFsInfo(data);
        break;
      case 0x02:
        if(!curFs)
          break;
        acsiDbgl("GEMDOS read cmd=", count, " drive=", (char)('A' + driveId));
        s = curFs->readCommand(data, count);
        break;
    }
    count = 1;
    return s;
  }

  // 256 blocks pages
  if(block < 0x110000) {
    if(block < 0x103000) {
      // Data generators for tests
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
    return AST_INVADDR;
  }
  
  // File descriptor stream, read
  if(block < 0x120000 && curFs) {
    if((block & 0xff) == 0xff) {
      // Short read (0-127 bytes)
      int fdId = (block >> 8) & 0x7f;
      int bytes = count;

      count = 1; // Don't loop, returns only one data block

      if(fdId >= maxFd)
        return ASTB_EIHNDL;

      return curFs->readBytes(fdId, data, bytes);
    }

    // File descriptor stream, 512 bytes blocks
    //curFs->read(fdId, data); // TODO
  }

  return AST_INVADDR;
}

Status Acsi::startSpecialWrite() {
  // Driver blocks
  if(block < 0x100000)
    return AST_WRITEERR;

  // 1 block pages
  if(block < 0x100100)
    return AST_OK;

  // 256 blocks pages
  if(block < 0x110000) {
    if((block & 0xff) || count > 255)
      // Access must be page aligned
      return AST_INVADDR;
    return AST_OK;
  }

  if(block < 0x120000 && curFs)
    return AST_OK;

  // Debug output
  startDebugReadWrite();
  count = 0; // Prevent data transfer
  return AST_OK;
}

Status Acsi::writeSpecial(uint8_t *data) {
  // embedded driver code
  if(block < 0x100000)
    return AST_WRITEERR;

  // 1 block pages
  if(block < 0x100100) {
    Status s = AST_INVADDR;
    switch(block & 0xff) {
      case 0x02:
        if(!curFs)
          break;
        acsiDbgl("GEMDOS write cmd=", count, " drive=", (char)('A' + driveId));
        s = curFs->writeCommand(data, count);
        break;
    }
    // Adjust count because it may have been used for passing paramters.
    // All blocks in this page always do 1 block long operations.
    count = 1;
    return s;
  }

  // 256 blocks pages
  if(block >= 0x100100 && block <= 0x1001ff)
    return readWriteLoopback();

  if(block >= 0x100200 && block <= 0x10ffff) {
    if(block < 0x103000) {
      // Reference data that should be received
      uint8_t refBuf[ACSI_BLOCKSIZE];
      Status s = readSpecial(refBuf);
      if(s != AST_OK)
        return s;

      // Return success if data is the same, write error otherwise.
      return memcmp(refBuf, data, ACSI_BLOCKSIZE) ? AST_WRITEERR : AST_OK;
    }
    return AST_INVADDR;
  }
  
  // File descriptor stream, write
  if(block < 0x120000 && curFs) {
    if((block & 0xff) == 0xff) {
      // Short read (0-127 bytes)
      int fdId = (block >> 8) & 0x7f;
      int bytes = count;

      count = 1; // Don't loop, returns only one data block

      if(fdId >= maxFd)
        return ASTB_EIHNDL;

      return curFs->writeBytes(fdId, data, bytes);
    }

    // File descriptor stream, 512 bytes blocks
    //curFs->write(fdId, data); // TODO
  }

  return AST_INVADDR;
}

Status Acsi::readPattern(uint8_t *data, const char pattern[2]) {
  for(int i = 0; i < ACSI_BLOCKSIZE; i += 2) {
    data[i] = pattern[0];
    data[i + 1] = pattern[1];
  }
  return AST_OK;
}

Status Acsi::startDebugReadWrite() {
  bool lf = !(block & 0x080000);
  switch((block & 0x170000) >> 16) {

    case 0x11: // Log byte
      Serial.print(' ');
      Serial.print(block & 0xff,HEX);
      if(lf)
        Serial.println("");
      return AST_OK;

    case 0x12: // Log word
      Serial.print(' ');
      Serial.print(block & 0xffff,HEX);
      if(lf)
        Serial.println("");
      return AST_OK;

    case 0x13: // Log char
      Serial.print((char)block);
      return AST_OK;

    case 0x14: // Log long
      Serial.print(' ');
      Serial.print((block & 0xffff) << 16 | count << 8 | control, HEX);
      if(lf)
        Serial.println("");
      return AST_OK;

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
      return AST_OK;
  }
  return AST_INVADDR;
}

Status Acsi::readFsInfo(uint8_t *data) {
#if ACSI_DEBUG
  auto startMillis = millis();
  acsiDbgl("Read fs info for drive ", driveId);
#endif

  FsInfo finfo;

  // Refresh information about mounted drives

  if(driveId <= -1) {
    // Huge, costly call, but needed to refresh the filesystem mask
    reload();

    drv = nullptr;
  } else if(driveId < maxDrives) {
    reloadDrive(driveId);

    if(drives[driveId])
      drv = &drives[driveId];
    else
      drv = nullptr;
  } else {
    drv = nullptr;
  }

  // Compute filesystem info (id, bpb)

  finfo.dev = driveId;

  if(drv)
    drv->computeFInfo(finfo.id, finfo.bpb);
  else
    finfo.id = 0;

  finfo.dmask = 0;
  for(int i = 0; i < maxDrives; ++i)
    if(drives[i])
      finfo.dmask |= 1 << i;

  int fmask = 0;

  static_assert(maxFs >= maxSd, "maxFs too low");
  for(int f = 0; f < maxSd; ++f) {
    if(fs[f])
      fmask |= 1 << (f + 2);
  }
  finfo.fmask = fmask;

  // Set boot drive on the first mounted filesystem
  // Boot only on the first partition
  finfo.boot = -1;
  fmask |= finfo.dmask;
  for(int i = 2; i < maxSd + 2; ++i) {
    if(fmask & (1 << i)) {
      finfo.boot = i;
      break;
    }
  }

  memcpy(data, (const uint8_t *)&finfo, sizeof(finfo));

#if ACSI_DEBUG
  acsiDbgl("  ... done in ", millis() - startMillis, " ms");
#endif
  return Status(AST_OK, sizeof(finfo));
}

void Acsi::reloadDrive(int id) {
  reloadDevice(drives[id].dev->root->deviceId);
}

Status Acsi::startDriverRead() {
  acsiDbgl("Read driver block=", block, " count=", count);
  if((block + count) * ACSI_BLOCKSIZE > (sizeof(boot_bin) + 511 - bootSize) & 0xfffe00)
    return AST_INVADDR;
  return AST_OK;
}

Status Acsi::readDriver(uint8_t *data) {
  uint32_t offset = bootSize + block * ACSI_BLOCKSIZE;
  uint32_t size = sizeof(boot_bin) - offset;
  if(size > ACSI_BLOCKSIZE)
    size = ACSI_BLOCKSIZE;
  memcpy(data, &boot_bin[offset], size);
  return AST_OK;
}

void Acsi::mountDrives() {
  acsiVerboseln("Finding floppy images");
  RootDevice *froot[2] = {nullptr, nullptr};
  for(int i = rootCount - 1; i >= 0; --i) {
    RootDevice *root = &roots[i];
    for(int f = 0; f < 2; ++f)
      if(root->fs.exists(floppyImageNames[f]))
        froot[f] = root;
  }

  acsiVerboseln("Mounting floppy images");
  for(int i = 0; i < 2; ++i) {
    floppies[i].end();
    if(froot[i])
      if(floppies[i].begin(froot[i], floppyImageNames[i]))
        if(drives[i].begin(&floppies[i]))
          acsiVerbosel("Mounted floppy image ", (char)('A'+i), ": from SD", froot[i]->slot, " ", drives[i].blocks, " blocks");
  }

  acsiVerboseln("Mounting main partitions");
  for(int i = 0; i < rootCount; ++i)
    if(drives[i + 2].begin(&roots[i].device, 0))
      acsiVerbosel("Mounted drive ", (char)('A' + i + 2), ": from SD", roots[i].slot, drives[i + 2].offset ? " partition:" : " superfloppy:", drives[i + 2].blocks, " blocks");

  // Mount extra partitions using dynamic drive letters
  // This will generate a big mess when hot plugging cards with many partitions
  acsiVerboseln("Mounting extra partitions");
  int driveNum = rootCount + 2;
  for(int i = 0; i < rootCount && driveNum < maxDrives; ++i)
    for(int p = 1; p < 4; ++p)
      if(drives[driveNum].begin(&roots[i].device, p)) {
        acsiVerbosel("Mounted partition ", p, " from SD", roots[i].slot, " as ", (char)('A' + driveNum + 2), ": ", drives[i + 2].blocks, " blocks");
        ++driveNum;
      }

  // Reset file descriptors
  // FIXME: should not do this if remounting a drive that didn't change.
  for(int i = 0; i < maxFd; ++i)
    fd[i].close();

  // Mount filesystems
  acsiVerboseln("Mounting filesystems");
  for(int i = 0; i < maxSd; ++i)
    if(fs[i].begin(&roots[i].fs, fd))
      acsiVerbosel("Mounted filesystem ", (char)('C' + i));

#if ACSI_BOOT_OVERLAY
  computeBootOverlay();
#endif
}

#if ACSI_BOOT_OVERLAY
void Acsi::computeBootOverlay() {
  // Don't override boot overlay, if any
  if(bootOverlay >= 0)
    return;

  // Set boot overlay on the first managed device
  for(int d = 0; d < 8; ++d) {
    if(devices[d]
#if ACSI_BOOT_OVERLAY == 1
        && !devices[d]->bootable
#endif
        ) {
      bootOverlay = d;
      break;
    }
  }
}

void Acsi::overlayBoot(uint8_t *target) {
  // Inject the driver blob boot sector
  memcpy(target, boot_bin, bootSize);

  // Patch boot chain flag
#if ACSI_BOOT_OVERLAY == 3
  target[footer] = 0;
#else
  target[footer] = roots[bootOverlay].device.bootable;
#endif

  if(target[footer])
    acsiVerboseln("Boot chain enabled");
  else
    acsiVerboseln("Boot chain disabled");

  // Patch checksum
  target[footer+1] = 0;
  target[footer+2] = 0;
  int checksum = 0x1234 - BlockDev::computeChecksum(target);
  target[footer+1] = (checksum >> 8) & 0xff;
  target[footer+2] = checksum & 0xff;
}

#endif // ACSI_BOOT_OVERLAY
#endif // ACSI_DRIVER

// vim: ts=2 sw=2 sts=2 et

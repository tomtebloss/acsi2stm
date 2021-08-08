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

#include "FsMount.h"

#if ACSI_DRIVER

#if ACSI_FS_DEBUG
template<typename T>
inline void fsDbgl(T txt) {
  acsiDbgln(txt);
}
template<typename T, typename... More>
inline void fsDbgl(T txt, More... more) {
  acsiDbg(txt);
  fsDbgl(more...);
}
#else
template<typename T>
inline void fsDbgl(T...) {}
#endif

bool FsMount::begin(AcsiVolume *vol_, TinyFileDescriptor *fd_) {
  vol = vol_;
  fd = fd_;

  return *this;
}

Status FsMount::writeCommand(uint8_t *data, int command) {
  ctx.cmd = (Command)command;
  switch(ctx.cmd) {
    case GD_DCREATE:
      return writeDcreate((const char *)data);
    case GD_DDELETE:
      return writeDdelete((const char *)data);
    case GD_DSETPATH:
      return writeDsetpath((const char *)data);
    case GD_FCREATE:
      return writeFopen((FopenW *)data, true);
    case GD_FOPEN:
      return writeFopen((FopenW *)data);
    case GD_FCLOSE:
      return writeFclose(data);
    case GD_FDELETE:
      return writeFdelete((const char *)data);
    case GD_FSFIRST:
      return writeFsfirst((FsfirstW *)data);
    case GD_FSNEXT:
      return writeFsnext((DTA *)data);
    case GD_FRENAME:
      return writeFrename((const FrenameW *)data);
  }
  return ASTB_ERR;
}

Status FsMount::readCommand(uint8_t *data, int command) {
  ctx.cmd = (Command)command;
  switch(ctx.cmd) {
    case GD_DGETPATH:
      return readDgetpath((char *)data);
    case GD_FSFIRST:
    case GD_FSNEXT:
      return readFsfirstNext((DTA *)data);
  }
  return ASTB_ERR;
}

Status FsMount::read(int fdId, uint8_t *data) {
  // XXX ??? TODO
  return ASTB_ERR;
}

Status FsMount::readBytes(int fdId, uint8_t *data, int bytes) {
  fsDbgl("Read fd=", fdId, " bytes=", bytes);

  TinyFileDescriptor *tf = &fd[fdId];
  if(!*tf)
    return ASTB_EIHNDL;

  int r = tf->read(*vol, data, bytes);
  if(r < 0)
    return AST_READERR;

  return Status(0, 0, r, r);
}

Status FsMount::write(int fdId, uint8_t *data) {
  // XXX ??? TODO
  return ASTB_ERR;
}

Status FsMount::writeBytes(int fdId, uint8_t *data, int bytes) {
  fsDbgl("Write fd=", fdId, " bytes=", bytes);

  TinyFileDescriptor *tf = &fd[fdId];
  if(!*tf)
    return ASTB_EIHNDL;

  int r = tf->write(*vol, data, bytes);
  if(r < 0)
    return AST_WRITEERR;

  return Status(0, 0, r, r);
}

// Commands implementation

Status FsMount::writeDcreate(const char *path) {
  fsDbgl("Dcreate path=", path);
  TinyPath filePath = curPath;
  if(!filePath.set(*vol, path, 2))
    return ASTB_EPTHNF;
  return AST_OK;
}

Status FsMount::writeDdelete(const char *path) {
  fsDbgl("Ddelete path=", path);

  TinyPath filePath = curPath;
  if(!filePath.set(*vol, path))
    return ASTB_EPTHNF;

  TinyFile dir;
  if(!dir.open(*vol, filePath)->rmdir())
    return ASTB_EPTHNF;

  return AST_OK;
}

Status FsMount::writeDsetpath(const char *path) {
  fsDbgl("Dsetpath path=", path);
  if(curPath.set(*vol, path))
    return AST_OK;
  else
    return ASTB_EPTHNF;
}

Status FsMount::readDgetpath(char *path) {
  curPath.getAbsolute(*vol, path, ACSI_BLOCKSIZE);
  if(!*path)
    return ASTB_ERR;

  fsDbgl("Dgetpath path=", path);

  return AST_OK;
}

Status FsMount::writeFopen(FopenW *data, bool create) {
  if(create) {
    fsDbgl("Fcreate path=", data->path);
  } else {
    fsDbgl("Fopen path=", data->path);
  }

  if(create && (data->mode & 0x08)) {
    // Cannot create a volume label
    acsiDbgln("Volume label not supported");
    return ASTB_EACCDN;
  }

  int f = newFd();
  if(f < 0)
    return ASTB_ENHNDL;

  // Compute the correct open mode
  oflag_t oflag = (create || (uint16_t)(data->mode)) ? O_RDWR : O_RDONLY;

  // Compute file path relative to the current path
  TinyPath filePath = curPath;
  if(!filePath.set(*vol, data->path, create ? 1 : 0))
    return ASTB_EFILNF;

  // Set flags in create mode
  if(create) {
    // TODO
  }

  // Open the file
  TinyFileDescriptor &tf = fd[f];
  tf.open(*vol, filePath, oflag);
  if(!tf)
    return ASTB_READERR;

  // Success: return the file descriptor
  return Status(0, 0, f, 0);
}

Status FsMount::writeFclose(const uint8_t *data) {
  int f = data[1];
  if(f >= maxFd || !fd[f])
    return ASTB_ENHNDL;

  fsDbgl("Fclose fd=", f);

  fd[f].close();

  return AST_OK;
}

Status FsMount::writeFdelete(const char *data) {
  fsDbgl("Fdelete path=", data);

  TinyPath filePath = curPath;
  if(!filePath.set(*vol, data))
    return ASTB_EFILNF;

  TinyFile tf;
  AcsiFile *f = tf.open(*vol, filePath, O_RDWR);
  if(!*f)
    return ASTB_EACCDN;
  if(!f->remove())
    return ASTB_EACCDN;

  return AST_OK;
}

Status FsMount::writeFsfirst(FsfirstW *data) {
  fsDbgl("Fsfirst path=", data->file, " attrib=", data->attr);

  // Set the magic number for the DTA
  ctx.dta.magic[0] = 0x01;
  ctx.dta.magic[1] = 0x02;

  // Set flags and attribs
  ctx.dta.flags = (uint8_t)(uint16_t)data->attr;
  ctx.dta.attrib = 0;

  if(ctx.dta.flags & ATTRIB_LABEL)
    // Handle label in a special way
    return AST_OK;

  // Start from the current directory
  TinyPath path = curPath;

  // Open containing directory
  if(!path.set(*vol, data->file)) {
    fsDbgl(data->file, " not found");
    ctx.dta.file.close();
    return ASTB_ENMFIL;
  }

  // Store the pattern into the DTA
  memcpy(ctx.dta.pattern, path.lastPattern(), 11);

  ctx.dta.file.open(*vol, path, O_RDONLY);

  return AST_OK;
}

Status FsMount::writeFsnext(DTA *dta) {
  fsDbgl("Fsnext");

  if(dta->flags & ATTRIB_LABEL)
    // When querying a label, return no other file
    return ASTB_ENMFIL;

  ctx.dta = *dta;

  ctx.dta.file.openNext(*vol, O_RDONLY, ctx.dta.pattern);
  return AST_OK;
}

Status FsMount::readFsfirstNext(DTA *dta) {
  if(ctx.dta.flags & ATTRIB_LABEL) {
    // Return a fake label as SdFat cannot query the volume label.
    strcpy(dta->filename, "ACSI2STM");
    dta->attrib = ATTRIB_LABEL;
    dta->length = 16777216;
    return Status(AST_OK, sizeof(DTA));
  }

  AcsiFile *f = ctx.dta.file.acquire(*vol);

  // Check that the file matches attributes
  while(*f && !attribsMatch(f, ctx.dta.flags))
    f = ctx.dta.file.openNext(*vol, O_RDONLY, ctx.dta.pattern);

  if(*f) {
    // Return file name
    ctx.dta.filename[TinyPath::nameToAtari(f, ctx.dta.filename, 13)] = 0;

    // Return file modification time
    uint16_t date, time;
    f->getModifyDateTime(&date, &time);
    ctx.dta.date = date;
    ctx.dta.time = time;

    // Return file attributes
    ctx.dta.attrib =
      (f->isReadOnly() ? ATTRIB_READONLY : 0) |
      (f->isHidden() ? ATTRIB_HIDDEN : 0) |
      (f->isSubDir() ? ATTRIB_SUBDIR : 0);

    // Return file length
    ctx.dta.length = f->fileSize();

    *dta = ctx.dta;
    return Status(AST_OK, sizeof(DTA));
  }

  return ASTB_ENMFIL;
}

Status FsMount::writeFrename(const FrenameW *data) {
  fsDbgl("Frename from=", data->from, " to=", data->to);

  TinyPath fromPath = curPath;
  TinyPath toPath = curPath;

  // Get source path
  if(!fromPath.set(*vol, data->from))
    return ASTB_EPTHNF;

  // Get target directory path
  if(!toPath.set(*vol, data->to, -1))
    return ASTB_EPTHNF;

  // TinyPath::pattern contains the name of the target file
  char toName[52];
  toPath.patternToUnicode(toName, 52);

  TinyFile fromFile;
  AcsiFile from = *fromFile.open(*vol, fromPath, O_RDWR);
  if(!from) {
    fsDbgl("Cannot open source file");
    return ASTB_EACCDN;
  }

  TinyFile toFile;
  AcsiFile toParent = *toFile.open(*vol, toPath);
  if(!toParent) {
    fsDbgl("Cannot open target directory");
    return ASTB_EACCDN;
  }

  if(!from.rename(&toParent, toName)) {
    fsDbgl("Cannot rename the file");
    return ASTB_EACCDN;
  }

  return AST_OK;
}

bool FsMount::attribsMatch(AcsiFile *f, uint8_t flags) {
  if(f->isReadOnly() && !(flags & ATTRIB_READONLY))
    return false;
  if(f->isHidden() && !(flags & ATTRIB_HIDDEN))
    return false;
  if(f->isSubDir() && !(flags & ATTRIB_SUBDIR))
    return false;
  return true;
}

int FsMount::newFd() {
  for(int i = 0; i < maxFd; ++i)
    if(!fd[i])
      return i;
  return -1;
}

// Shared variables
FsMount::CommandContext FsMount::ctx;

#endif // ACSI_DRIVER

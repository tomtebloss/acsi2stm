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

// Check dependencies and correct values for acsi2stm.h
// Included only by acsi2stm.h

#ifndef ACSI2STM_H_DEPCHECK
#error "DepCheck.h must be included only by acsi2stm.h"
#endif

static_assert(strlen(ACSI2STM_VERSION) == 4, "ACSI2STM_VERSION is not the correct length");

#if !ACSI_DEBUG && ACSI_VERBOSE
#warning "ACSI_VERBOSE needs ACSI_DEBUG"
#endif

#if ACSI_VERBOSE && (!defined(ACSI_DUMP_LEN) || ACSI_DUMP_LEN < 0 || ACSI_DUMP_LEN > 512)
#error "ACSI_VERBOSE requires ACSI_DUMP_LEN to be between 0 and 512"
#endif

#ifndef ACSI_SERIAL
#error "ACSI_SERIAL not defined"
#endif

#if !defined(ACSI_SERIAL_BAUD) || ACSI_SERIAL_BAUD < 9600 || ACSI_SERIAL_BAUD > 2000000
#error "ACSI_SERIAL_BAUD must be a valid serial baud rate"
#endif

#if ACSI_ACK_FILTER < 0 || ACSI_ACK_FILTER > 2
#error "ACSI_ACK_FILTER must be between 0 and 2"
#endif

#if ACSI_CS_FILTER < 0 || ACSI_CS_FILTER > 3
#error "ACSI_CS_FILTER must be between 0 and 3"
#endif

#if ACSI_READONLY < 0 || ACSI_READONLY > 2
#error "ACSI_READONLY must be between 0 and 2"
#endif

#if ACSI_BOOT_OVERLAY < 0 || ACSI_BOOT_OVERLAY > 3
#error "ACSI_BOOT_OVERLAY must be between 0 and 3"
#endif

#if !ACSI_DEBUG && ACSI_FS_DEBUG
#warning "ACSI_FS_DEBUG needs ACSI_DEBUG"
#endif

#if !defined(ACSI_MAX_BLOCKS) || (ACSI_MAX_BLOCKS <= 0 && ACSI_MAX_BLOCKS != ~0)
#error "ACSI_MAX_BLOCKS must be positive. Set it to ~0 to disable the limit."
#endif

#if ACSI_FAT32_ONLY && ACSI_EXFAT_ONLY
#error "ACSI_FAT32_ONLY and ACSI_EXFAT_ONLY are mutually exclusive"
#endif

#if WATCHDOG_TIMEOUT <= 3000
#warning "WATCHDOG_TIMEOUT is shorter than the Atari ST ACSI timeout."
#endif

static_assert(sizeof(sdCs) / sizeof(sdCs[0]) >= 1 && sizeof(sdCs) / sizeof(sdCs[0]) <= 5, "Invalid sdCs pin definitions.");

static_assert(sizeof(sdCs) == sizeof(sdId), "sdId must contain the same number of items as sdCs.");

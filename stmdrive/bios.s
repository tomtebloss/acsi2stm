; ACSI2STM Atari hard drive emulator
; Copyright (C) 2019-2021 by Jean-Matthieu Coulon

; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.

; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.

; BIOS call hook for drvmap

	move.w	globals+traplen(pc),d0      ; d0 = trap stack frame len
	lea	(sp,d0),a0                  ; a0 = parameters
	move.w	(a0),d1                     ; d1 = operation

	cmp.b	#Drvmap,d1
	bne.b	.end

	move.w	#-1,d2                      ; Just refresh mountpoint mask
	bsr.w	finfo_query                 ; Query virtual floppy drives

.end

; vim: ff=dos ts=8 sw=8 sts=8 noet

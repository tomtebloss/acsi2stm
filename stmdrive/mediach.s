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

; MEDIACH hook

; int32_t Mediach ( int16_t dev );
; Parameters
		rsreset
		rs.l	1
mediach.dev	rs.w	1

	move.w	mediach.dev(sp),d2      ; d2 = current device

	bsr.w	finfo_query             ; Query device information

	lea	globals(pc),a0          ; a0 = globals
	move.l	mchmask(a0),d0          ; d0 = media change mask
	move.w	mediach.dev(sp),d2      ; d2 = current device

	btst	d2,d0                   ; Is flag set
	beq.b	.nomch

	bclr	d2,d0                   ; Clear flag
	move.l	d0,mchmask(a0)          ;
	moveq	#1,d0                   ; Probably changed
	rts                             ; Return

.nomch	move.l	dmask(a0),d0            ; d0 = mounted drives flag
	or.l	fmask(a0),d0            ;
	
	btst	d2,d0                   ; Is the drive mounted ?
	beq.b	.end                    ; If not: follow through BIOS call

	moveq	#1,d0                   ; Probably changed
	rts                             ; Return

.end

; vim: ts=8 sw=8 sts=8 noet

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

; GETBPB hook

; BPB *Getbpb (
;               int16_t dev
;             ); 
; Parameters
		rsreset
		rs.l	1
getbpb.dev	rs.w	1

	move.w	getbpb.dev(sp),d2           ; d2 = current device
	bsr.w	finfo_query_now             ; Query device info

	move.w	getbpb.dev(sp),d2           ; d2 = current device
	lea	globals(pc),a0              ; a0 = globals

	move.l	mchmask(a0),d0              ; Clear mchmask
	bclr	d2,d0                       ;
	move.l	d0,mchmask(a0)              ;

	move.l	dmask(a0),d0                ; Is the current drive mounted ?
	btst	d2,d0                       ;
	bne.w	.mnted                      ;

	move.l	fmask(a0),d0                ; Is the drive a filesystem ?
	btst	d2,d0                       ;
	beq.w	.real                       ;

	moveq	#0,d0                       ; Cannot do low-level access on a
	rts                                 ; mounted filesystem

.mnted	lea	finfo+finfo.bpb(a0),a1      ;
	move.l	a1,d0                       ; d0 = pointer to the virtual bpb

	rts
.real

; vim: ff=dos ts=8 sw=8 sts=8 noet

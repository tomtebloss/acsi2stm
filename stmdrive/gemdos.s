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

; GEMDOS call hook

	move.w	globals+traplen(pc),d0  ; d0 = trap stack frame len
	lea	(sp,d0),a0              ; a0 = parameters
	move.w	(a0),d1                 ; d1 = operation

	move.l	a3,-(sp)                ; Save a3
	move.l	a0,a3                   ; a3 = parameters

	cmp.w	#Dcreate,d1
	blt.w	gemdos_pass
	cmp.w	#Frename,d1
	bgt.w	gemdos_pass

;	print	'gemdos' ;;;;; XXX
;	dumpw	(a3),6     ;;;;; XXX

	sub.w	#Dcreate,d1             ; Compute offset in the jump table
	lsl.w	#1,d1                   ;
	move.w	.jtable(pc,d1),d1       ; d1 = Operation address offset
	lea	.jtable(pc,d1),a1       ; a1 = Absolute jump address

	jmp	(a1)                    ; Execute the function

; Jump table
.jtable
	dc.w	Dpath_impl-.jtable      ; 57
	dc.w	Dpath_impl-.jtable      ; 58
	dc.w	Dpath_impl-.jtable      ; 59
	dc.w	Fopen_impl-.jtable      ; 60
	dc.w	Fopen_impl-.jtable      ; 61
	dc.w	Fclose_impl-.jtable     ; 62
	dc.w	Fread_impl-.jtable      ; 63
	dc.w	Fread_impl-.jtable      ; 64
	dc.w	Dpath_impl-.jtable      ; 65
	dcb.w	71-65-1,gemdos_pass-.jtable
	dc.w	Dgetpath_impl-.jtable   ; 71
	dcb.w	78-71-1,gemdos_pass-.jtable
	dc.w	Fsfirst_impl-.jtable    ; 78
	dc.w	Fsnext_impl-.jtable     ; 79
	dcb.w	86-79-1,gemdos_pass-.jtable
	dc.w	Frename_impl-.jtable    ; 86

; Implementation files
	include	dpath.s                 ; Dgetpath, Dpath
	include	fsfn.s                  ; Fsfirst, Fsnext
	include	file.s                  ; Fopen, Fread, Fclose, ...

; Exit the call immediately
gemdos_exit
	ext.w	d0                      ; Extend error code
gemdos_exit_word                        ; Exit returning a word
	ext.l	d0                      ;
gemdos_exit_long                        ; Exit returning a long
	move.l	(sp)+,a3                ; Restore a3

;	print	'ret=' ;;;;; XXX
;	printl	d0  ;;;;;;; XXX

	rte                             ;

; Pass the call to GEMDOS
gemdos_pass
	move.l	(sp)+,a3                ; Restore a3

; vim: ff=dos ts=8 sw=8 sts=8 noet

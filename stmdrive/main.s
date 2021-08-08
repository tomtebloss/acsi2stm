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

; Driver initialization and hooks
; Shared part between the boot sector and TOS program

	; TODO: Check if the driver is already loaded.
	; that sometimes happens on reset.

	; Initialize variables

	lea	globals(pc),a2          ; a2 = globals

	lea	(a2),a0                 ; Fill globals with 0
	move.w	#globals...-1,d0        ;
.fill	clr.b	(a0)+                   ;
	dbra	d0,.fill                ;

	move.l	#$091f28e0,acsicmd+acsicmd.len(a2) ; Create the ACSI command
	or.b	d7,acsicmd+acsicmd.cmdex(a2) ; Set the device id for ACSI

	move.l	drvbits.w,oldmask(a2)   ; Store BIOS drive mask

	move.w	nflops.w,d0             ; Set the bfloppy flag
	subq	#1,d0                   ; if the machine has
	move.w	d0,bfloppy(a2)          ; 2 hardware drives

	move.l	hz200.w,nextqry(a2)     ; Initialize nextqry

	; Compute trap parameter offset
	; This is CPU dependant
	lea	gemdos_vector.w,a1      ; Test on trap #1
	move.l	(a1),d2                 ; Store current vector
	move.l	sp,d1                   ; d1 = SP before trap
	lea	trapchk(pc),a0          ; Compute test vector
	move.l	a0,(a1)                 ; Trap #1 will jump to trapchk
	trap	#1                      ; d0 = SP after trap
	move.l	d2,(a1)                 ; Restore trap #1 vector

	sub.l	d0,d1                   ; Compute offset
	move.w	d1,traplen(a2)          ;

	; Refresh drive masks and nflops

	moveq	#-1,d2                  ; Don't query any drive
	bsr.w	finfo_query             ; Query drive mask only

	; Query device description for the splash screen.

	; finfo_query already built most of the command
	lea	globals+acsicmd+acsicmd.block+2(pc),a0
	lea	globals+blkbuf(pc),a1   ; DMA target
	move.l	a1,d0                   ;
	clr.w	(a0)                    ; block is $00100000

	bset	#24,d0                  ; Set DMA length

	bsr.w	acsi_exec_cmd           ; Query device description

	pea	welcome(pc)             ; Show welcome line
	gemdos	Cconws                  ;
	pea	globals+blkbuf(pc)      ; Show devices description
	gemdos	Cconws,12               ;

	; Install hooks

	lea	old_getbpb(pc),a0
	move.l	getbpb_vector.w,(a0)+
	move.l	a0,getbpb_vector.w

	lea	old_rwabs(pc),a0
	move.l	rwabs_vector.w,(a0)+
	move.l	a0,rwabs_vector.w

	lea	old_mediach(pc),a0
	move.l	mediach_vector.w,(a0)+
	move.l	a0,mediach_vector.w

	lea	old_bios(pc),a0
	move.l	bios_vector.w,(a0)+
	move.l	a0,bios_vector.w

	lea	old_xbios(pc),a0
	move.l	xbios_vector.w,(a0)+
	move.l	a0,xbios_vector.w

	lea	old_gemdos(pc),a0
	move.l	gemdos_vector.w,(a0)+
	move.l	a0,gemdos_vector.w

	; Set boot drive if specified by the STM
	move.w	globals+finfo+finfo.boot(pc),d2 ; d2 = boot device advice
	cmp.w	#-1,d2                  ; If no boot advice, just leave as-is
	beq.b	.end                    ;

	move.w	d2,bootdev.w            ; Set system boot device

	move.w	d2,-(sp)                ; Set current drive
	gemdos	Dsetdrv,4               ;

	pea	rootpth(pc)             ; Set '\' as current path
	gemdos	Dsetpath,6              ;

	move.b	#$e0,d7                 ; Prevent booting next ACSI devices

.end	gemdos	Dgetdrv,2               ; Get current drive

	lea	bmsgdrv(pc),a0          ; Patch boot drive message
	add.b	d0,(a0)                 ;

	pea	bootmsg(pc)             ; Display boot message
	gemdos	Cconws,6                ;

	moveq	#0,d0                   ; Success code
	rts

trapchk	move.l	sp,d0
	rte

; Hooks

	; Getbpb hook
	dc.b	'XBRA','A2ST'
old_getbpb
	ds.l	1
getbpb_hook

	include getbpb.s                ; Getbpb hook code

	move.l	old_getbpb(pc),-(sp)    ; Forward to the previous handler
	rts                             ;

	; Rwabs hook
	dc.b	'XBRA','A2ST'
old_rwabs
	ds.l	1
rwabs_hook

	include rwabs.s                 ; Rwabs hook code

	move.l	old_rwabs(pc),-(sp)     ; Forward to the previous handler
	rts                             ;

	; Mediach hook
	dc.b	'XBRA','A2ST'
old_mediach
	ds.l	1
mediach_hook

	include mediach.s               ; Mediach hook code

	move.l	old_mediach(pc),-(sp)   ; Forward to the previous handler
	rts                             ;

	; BIOS hook
	dc.b	'XBRA','A2ST'
old_bios
	ds.l	1
bios_hook

	include bios.s                  ; BIOS hook code

	move.l	old_bios(pc),-(sp)      ; Forward to the previous handler
	rts                             ;

	; XBIOS hook
	dc.b	'XBRA','A2ST'
old_xbios
	ds.l	1
xbios_hook

	include xbios.s                 ; XBIOS hook code

	move.l	old_xbios(pc),-(sp)     ; Forward to the previous handler
	rts                             ;

	; GEMDOS hook
	dc.b	'XBRA','A2ST'
old_gemdos
	ds.l	1
gemdos_hook

	include gemdos.s                ; GEMDOS hook code

	move.l	old_gemdos(pc),-(sp)    ; Forward to the previous handler
	rts                             ;

; Global functions

	include	acsi.s
	include	finfo.s
	include	utils.s

; Data

welcome	dc.b	'ACSI2STM driver loaded',13,10,0
bootmsg	dc.b	'System path: '
bmsgdrv	dc.b	'A:\',13,10,0
rootpth	dc.b	'\',0

; vim: ff=dos ts=8 sw=8 sts=8 noet

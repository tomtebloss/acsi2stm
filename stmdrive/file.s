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

; File operations

; struct FopenW
		rsreset
FopenW.path...=pathLen
FopenW.path	rs.b	FopenW.path...
FopenW.mode	rs.w	1
FopenW...	rs.b	0

; Fopen implementation
; Input:
;   a3 = parameters

Fopen_impl
		rsreset
		rs.w	1               ; GEMDOS operation code
Fopen.path	rs.l	1
Fopen.mode	rs.w	1
Fopen...	rs.b	0

	move.l	Fopen.path(a3),a0       ; Test if the path is owned by us
	bsr.w	ownpath                 ; d2 = path drive number
	beq.w	gemdos_pass             ; Pass the call to gemdos if not ours

	lea	globals(pc),a2          ; a2 = globals

	move.b	d2,curdrv(a2)           ; Save current drive

	; Copy the file name to the buffer
	move.l	Fopen.path(a3),a0
	strncpy	blkbuf+FopenW.path(a2),(a0),#FopenW.path...

	; Set mode
	move.w	Fopen.mode(a3),FopenW.mode(a2)

	move.w	(a3),d1                 ; Build and send the write command
	move.b	d2,d0                   ;
	bsr.w	fsmount_write_command   ;

	tst.b	d0                      ; Test if Fopen returned
	                                ; a valid file descriptor
	blt.w	gemdos_exit             ; Jump if returned an error

	or.w	#fdmagic,d0             ; Add the magic number to the fd

	move.w	globals+curdrv(pc),d2   ; d2 = current drive shifted by 8
	and.w	#$1f00,d2               ; Filter out garbage from d2
	or.w	d2,d0                   ; Add drive to the file descriptor

	bra.w	gemdos_exit_word        ; Return the file descriptor

; Fclose implementation
; Input:
;  a3 = parameters

Fclose_impl
		rsreset
		rs.w	1               ; GEMDOS operation code
Fclose.fd	rs.w	1               ; File descriptor
Fclose...	rs.b	0

	move.w	Fclose.fd(a3),d0        ; Test if the descriptor is owned by us
	bsr.w	ownfd                   ; d2 = drive number
	beq.w	gemdos_pass             ; Pass the call to gemdos if not ours

	lea	globals+blkbuf(pc),a2   ; a2 = data buffer
	move.w	Fclose.fd(a3),(a2)      ; Pass the file descriptor

	move.w	(a3),d1                 ; Build and send the write command
	move.b	d2,d0                   ;
	bsr.w	fsmount_write_command   ;

	bra.w	gemdos_exit             ; Return the error code

; Fread/Fwrite implementation
; Most code is identical, code that changes will test using the "ifwrite"
; or "ifread" macros
; Input:
;  a3 = parameters

ifwrite	macro
	cmp.w	#Fwrite,(a3)            ; Test if writing
	beq.b	\1                      ; Jump if writing
	endm

ifread	macro
	cmp.w	#Fread,(a3)             ; Test if reading
	beq.b	\1                      ; Jump if reading
	endm

Fread_impl
		rsreset
		rs.w	1               ; GEMDOS operation code
Fread.fd	rs.w	1               ; File descriptor
Fread.count	rs.l	1               ; Bytes to read
Fread.buf	rs.l	1               ; Target buffer address
Fread...	rs.b	0

	move.w	Fread.fd(a3),d0         ; Test if the descriptor is owned by us
	bsr.w	ownfd                   ;
	beq.w	gemdos_pass             ; Pass the call to gemdos if not ours

	movem.l	d3-d7/a4-a5,-(sp)       ; Store registers

	move.l	Fread.count(a3),d3      ; d3.l = bytes to read
	move.l	Fread.buf(a3),a5        ; a5 = target buffer address
	moveq	#0,d6                   ; d6.l = bytes read

	; TODO: Iterate using big blocks and DMA if the target is aligned

	; Iterate using 127 bytes blocks
	moveq	#127,d4                 ; d4 = block size (127)

	; Build command
	; block = $0011xxff (xx = file descriptor)
	; count = byte count (0-127)
	; control = drive id (0-25 => A-Z)
	
	lea	globals+acsicmd+acsicmd.cmd(pc),a4 ; a4 = acsi command
	ifwrite	.wcmd
	move.l	#$28e00011,(a4)+        ; $28 = Read command
	bra.b	.wcmd_
.wcmd	move.l	#$2ae00011,(a4)+        ; $2a = Write command
.wcmd_		                        ; $e0 = Special blocks
		                        ; $0011 = block MSB
	move.b	Fread.fd+1(a3),(a4)+    ; File descriptor in block number
	move.b	#$ff,(a4)+              ; Last block number byte
	addq.l  #1,a4                   ; Skip reserved byte
	move.b	d2,2(a4)                ; Drive id in control
	clr.b	(a4)+                   ; Byte count MSB

	lea	globals+blkbuf(pc),a0   ; Do DMA operations through the buffer
	move.l	a0,d5                   ; d5 = DMA target
	bset	#24,d5                  ; 1 block to transfer each time

	; Command build finished

	; Registers:
	; d3.l = bytes to read
	; d4.l = block size (127)
	; d5.l = d0 for acsi_exec_cmd
	; d6.l = total bytes read
	; d7.w = bytes to read during the current iteration
	; (a4).b = byte count
	; a5 = target buffer address

.nxtblk	move.l	d3,d7                   ; d7 = min(127,d3)
	cmp.l	d4,d7                   ;
	ble.b	.lastbk                 ;
	move.l	d4,d7                   ;
.lastbk
	ifread	.nowcpy                 ; If reading, don't fill the buffer
	move.w	d7,d1                   ; d1.w = bytes to copy to the target
	subq	#1,d1                   ; Adjust for dbra
	blt.b	.end                    ; 0 byte write
	lea	globals+blkbuf(pc),a0   ;
.blkset	move.b	(a5)+,(a0)+             ; Copy the source address to the buffer
	dbra	d1,.blkset              ; XXX: very unoptimized

.nowcpy	move.b	d7,(a4)                 ; Set byte count into the ACSI command
	move.l	d5,d0                   ;
	bsr.w	acsi_exec_cmd           ; Do the read operation

	ext.w	d0                      ; Convert d0 to long
	ext.l	d0                      ;

	blt.b	.dmaerr                 ; Negative error code

	add.l	d0,d6                   ; Add bytes read to counter
	sub.l	d0,d3                   ; Substract from bytes to read

	ifwrite	.norcpy                 ; If writing, don't copy read result
	move.w	d0,d1                   ; d1.w = bytes to copy to the target
	subq	#1,d1                   ; Adjust for dbra
	blt.b	.end                    ; 0 byte read (EOF)
	lea	globals+blkbuf(pc),a0   ;
.blkget	move.b	(a0)+,(a5)+             ; Copy the buffer to the target address
	dbra	d1,.blkget              ; XXX: very unoptimized

.norcpy	cmp.w	d0,d7                   ; Was the read complete ?
	bne.b	.end                    ; If not, reached EOF: exit

	tst.l	d3                      ; If more bytes to read
	bne.b	.nxtblk                 ; Read next block

.end	move.l	d6,d0                   ; Return total byte count
.dmaerr	movem.l	(sp)+,d3-d7/a4-a5       ; Restore registers

	bra.w	gemdos_exit_long

; struct FrenameW
		rsreset
FrenameW.path...=256
FrenameW.from	rs.b	FrenameW.path...
FrenameW.to	rs.b	FrenameW.path...
FrenameW...	rs.b	0

; Generic function taking two paths as a parameter
Frename_impl
		rsreset
		rs.w	1               ; GEMDOS operation code
		rs.w	1               ; Padding
Frename.from	rs.l	1
Frename.to   	rs.l	1
Frename...	rs.b	0

	lea	Frename.from(a3),a0     ;
	bsr.w	ownpath                 ; Test if we own the first path
	beq.w	gemdos_pass             ; Pass if not owned

	move.l	d2,a2                   ; a2 = drive of path 1

	lea	Frename.to(a3),a0       ;
	bsr.w	ownpath                 ; Test if we own the second path
	beq.w	gemdos_pass             ; Pass if not owned

	move.l	a2,d1                   ; Test if the 2 paths
	cmp.b	d1,d2                   ; are on the same drive
	beq.b	.samedr                 ;

	moveq	#ENSAME,d0              ; Not the same drive !
	bra.w	gemdos_exit_long        ; Return an error.

.samedr	move.l	Frename.from(a3),a0     ; Copy the first path to the DMA buffer
	strncpy	globals+blkbuf+FrenameW.from(pc),(a0),#FrenameW.path...

	move.l	Frename.to(a3),a0       ; Copy the second path to the buffer
	strncpy	globals+blkbuf+FrenameW.to(pc),(a0),#FrenameW.path...

	move.w	d2,d0                   ; Set drive for the write command
	move.w	(a3),d1                 ; Set GEMDOS command
	bsr.w	fsmount_write_command   ; Send the write command

	bra.w	gemdos_exit             ; Return

; vim: ff=dos ts=8 sw=8 sts=8 noet

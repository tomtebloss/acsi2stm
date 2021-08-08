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

; Boot sector loader for the driver

	org	0
boot
	include	defines.s
	include	structs.s

	movem.l	d0-d7/a0-a6,-(sp)       ; Keep registers intact

	lea	.ldcmd(pc),a0           ; Set boot drive id for ACSI
	or.b	d7,(a0)                 ;

	; Adjust drive id in error messages
	lsr.b	#5,d7                   ; Compute drive id (0-7)
	lea	.drvtxt(pc),a0          ; Patch drive id in the text
	add.b	d7,(a0)                 ; Add drive id to '0'

	; Allocate memory for the driver code
	pea	driver_blocks*512       ; Memory allocation size
	gemdos	Malloc,6                ; Allocate memory

	tst.l	d0                      ; Check Malloc success
	beq.b	.ramerr                 ;

	move.l	d0,a6                   ; a6 = driver base address

	bsr.w	.acsild                 ; Load the driver

	tst.b	d0                      ; Check DMA transfer
	bne.b	.dmaerr                 ; Free if error

	move.l	#driver_bytes,-(sp)     ; Shrink memory
	move.l	a6,-(sp)                ; to the minimum size
	gemdos	Mshrink,10              ;

	; Run the driver

	jsr	(a6)                    ; Run driver initialization

	tst.b	d0                      ; Test if the driver initialized
	beq.b	.boot                   ;

	; Driver unload
	; The driver is supposed to display its own message
	move.l	a6,-(sp)                ; Free driver code
	gemdos	Mfree,6                 ;

	bra.w	.boot

	; Error processing

.dmaerr	move.l	a6,-(sp)                ; Driver loading error
	gemdos	Mfree,6                 ; Free ram

	pea	.dmaert(pc)             ; Print abort text
	bra.w	.error                  ; Return to TOS boot

.ramerr	pea	.malert(pc)             ; Print error text
	bra.w	.error                  ; Return to TOS boot

	; Chain boot loader

.boot	lea	boot(pc),a0             ; a0 = boot sector address

	tst.b	.chnflg(a0)             ; Test if chaining boot
	beq.b	.exit                   ; No boot chain

	move.b	#$20,.ldcmd+1(a0)       ; Load on LUN 1
	move.b	#1,.ldcmd+4(a0)         ; Load 1 sector

	lea	$200(a0),a1             ; Move away this boot code to
	move.w	#$7f,d0                 ; the next boot block
.btcpy	move.l	(a0)+,(a1)+             ;
	dbra	d0,.btcpy               ;

	lea	boot(pc),a1             ;
	move.l	a1,d0                   ; d0 = current boot sector

	bra.w	.shadow+$200            ; Jump to the shadow copy
.shadow                                 ; Shadow copy jump target

	bsr.b	.acsild                 ; Load chain boot sector

	tst.b	d0                      ; Test if the chain boot sector
	beq.b	.chain                  ; was loaded correctly

	pea	.cherrt(pc)             ; Print chain load error
	bra.b	.error                  ; Exit

.chain	pea	.devtxt(pc)             ; Print device id
	gemdos	Cconws,6                ;

	pea	.chntxt(pc)             ; Print chain boot message
	gemdos	Cconws,6                ;

	movem.l	(sp)+,d0-d7/a0-a6       ; Restore registers

	bra.w	boot-$200               ; Jump to the new boot sector

.error	pea	.devtxt(pc)             ; Print device id
	gemdos	Cconws,6                ;
	gemdos	Cconws,6                ; Error message is already pushed

.exit	movem.l	(sp)+,d0-d7/a0-a6       ; Restore registers
	rts

; ACSI load routine
; d0: DMA target address
; Alters: d0-d2/a0-a2
; Returns: d0: status code

.acsild	st	flock.w                 ; Lock floppy drive

	lea	.ldcmd(pc),a0           ; a0 = Command buffer
	lea	dmactrl.w,a1            ; a1 = DMA control register
	lea	dma.w,a2                ; a2 = DMA data register

	; Initialize the DMA chip
	move.w	#$190,(a1)              ; DMA read initialization
	move.w	#$90,(a1)               ;

	; DMA transfer
	move.b	d0,dmalow.w             ; Set DMA address low
	lsr.l	#8,d0                   ;
	move.b	d0,dmamid.w             ; Set DMA address mid
	lsr.w	#8,d0                   ;
	move.b	d0,dmahigh.w            ; Set DMA address high

	moveq	#0,d0                   ;
	move.b	4(a0),d0                ; Set DMA length
	move.w	d0,(a2)                 ;

	move.w	#$0088,(a1)             ; assert A1

	move.l	#$8a,d1                 ; d1 = $008a = write command byte
	moveq	#4,d2                   ; d2 = Command byte counter
	                                ;      (except last byte)
.next	swap	d1                      ;
	move.b	(a0)+,d1                ; Read next command byte
	swap	d1                      ;
	move.l	d1,(a2)                 ; Send $00xx008a to DMA

	bsr.b	.ack                    ; Wait for ack

	dbra	d2,.next                ; Next byte

	move.l	#0,(a2)                 ; Send the last byte (always 0)

	bsr.b	.ack                    ; Wait for status byte

	move.w	#$8a,(a1)               ; Acknowledge status byte
	move.w	(a2),d0                 ; Read status to d0

.ldabrt	move.w	#$180,(a1)              ; Reset the DMA controller
	move.w	#$80,(a1)               ;

	sf	flock.w                 ; Unlock floppy drive

	rts
.ack
	; Wait until DMA ack (IRQ pulse)
	move.l	#600,d0                 ; 3 second timeout
	add.l	hz200.w,d0              ;
.wait	cmp.l	hz200.w,d0              ; Test timeout
	blt.b	.timout                 ;
	btst.b	#5,gpip.w               ; Test command acknowledge
	bne.b	.wait                   ;
	rts

.timout	moveq	#-1,d0                  ; Return -1
	addq.l	#4,sp                   ; to the caller of acsi_boot_load
	bra.b	.ldabrt                 ;

	; Read command to send to the STM32
	; The last byte is always 0 so it's not here
.ldcmd	dc.b	$08,$E0,$00,$00,driver_blocks

.devtxt	dc.b	'ACSI device '
.drvtxt	dc.b	'0 ',0
.chntxt	dc.b	'booting',13,10,0
.cherrt	dc.b	'boot error',13,10,0
.malert	dc.b	'Malloc failed',13,10,0
.dmaert	dc.b	'DMA error',13,10,0

	org	437                     ; Boot sector footer
.chnflg	dc.b	$00                     ; Boot chain flag
.cksum	dc.w	$0000                   ; Boot sector checksum adjust

	; Driver payload
	org	440

driver_payload

	include	nodebug.s
	include	main.s
	even

globals	ds.b	globals...
globals_end

driver_payload_end

driver_blocks	equ	(511+driver_payload_end-driver_payload)/512
driver_bytes	equ	driver_payload_end-driver_payload

; vim: ff=dos ts=8 sw=8 sts=8 noet

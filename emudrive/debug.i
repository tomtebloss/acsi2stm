; The slowest, most inefficient debug output ever !

	bra.w	dbg__end                    ; Skip code if included

; Print a string.
prints	macro
	pea	\1
	bsr.w	dbg_str
	addq	#4,sp
	endm

; Print text.
print	macro
	prints	.msg\@(pc)
	bra.b	.skip\@
.msg\@	dc.b	\1,0
	even
.skip\@
	endm

; Print a text line.
println	macro
	prints	.msg\@(pc)
	bra.b	.skip\@
.msg\@	dc.b	\1,10,0
	even
.skip\@
	endm

; Prints a byte.
printb	macro
	move.b	\1,-(sp)
	bsr.w	dbg_b
	addq	#2,sp
	endm

; Prints a word.
printw	macro
	move.w	\1,-(sp)
	bsr.w	dbg_w
	addq	#2,sp
	endm

; Prints a long.
printl	macro
	move.l	\1,-(sp)
	bsr.w	dbg_l
	addq	#4,sp
	endm

; Dumps a byte buffer.
dump	macro
	pea	\1
	move.w	#\2,-(sp)
	bsr.w	dbg_dump
	addq	#6,sp
	print	10
	endm

; Dump a word buffer
dumpw	macro
	pea	\1
	move.w	#\2,-(sp)
	bsr.w	dbg_dump_word
	addq	#6,sp
	print	10
	endm

; Dump a long buffer
dumpl	macro
	pea	\1
	move.w	#\2,-(sp)
	bsr.w	dbg_dump_long
	addq	#6,sp
	print	10
	endm

; Dump registers
dumpreg	macro
	move.w	sr,-(sp)
	pea	2(sp)
	movem.l	d0-d7/a0-a6,-(sp)
	bsr.w	dbg_dump_regs
	movem.l	(sp)+,d0-d7/a0-a6
	addq	#4,sp
	move.w	(sp)+,sr
	endm

; Reset STM32
sreset	macro
	bra.w	dbg_reset
	endm

dbg_str:
	move.w	sr,-(sp)
	movem.l	a0-a3/d0-d3/d7,-(sp)

	lea	dbg_acsi(pc),a0
	move.b	#$f3,5(a0)
	move.l	42(sp),a3
	move.l	#$00000000,6(a0)

.next_byte:
	tst.b	(a3)
	beq.b	.end

	move.b	(a3)+,7(a0)

	bsr.w	dbg_exec_cmd

	bra.b	.next_byte
.end

	movem.l	(sp)+,a0-a3/d0-d3/d7
	move.w	(sp)+,sr
	rts

dbg_dump_regs:
	print	'D0-D7:'
	dumpl	4(sp),8

	print	'A0-A7:'
	dumpl	36(sp),8

	print	'SR:'
	printw	68(sp)

	print	'Stack:'
	dumpl	(sp),8

	rts

dbg_dump:
	move.w	sr,-(sp)
	movem.l	a0-a3/d0-d3/d7,-(sp)

	lea	dbg_acsi(pc),a0
	move.l	44(sp),a3
	move.w	42(sp),d3
	subq	#1,d3
	move.b	#$f9,5(a0)
	move.l	#$00000000,6(a0)

.next_byte:
	move.b	(a3)+,7(a0)

	bsr.w	dbg_exec_cmd

	dbra	d3,.next_byte

	movem.l	(sp)+,a0-a3/d0-d3/d7
	move.w	(sp)+,sr
	rts

dbg_dump_word:
	move.w	sr,-(sp)
	movem.l	a0-a3/d0-d3/d7,-(sp)

	lea	dbg_acsi(pc),a0
	move.l	44(sp),a3
	move.w	42(sp),d3
	subq	#1,d3
	move.b	#$fa,5(a0)
	move.l	#$00000000,6(a0)

.next_word:
	move.w	(a3)+,6(a0)

	bsr.w	dbg_exec_cmd

	dbra	d3,.next_word

	movem.l	(sp)+,a0-a3/d0-d3/d7
	move.w	(sp)+,sr
	rts

dbg_dump_long:
	move.w	sr,-(sp)
	movem.l	a0-a3/d0-d3/d7,-(sp)

	lea	dbg_acsi(pc),a0
	move.l	44(sp),a3
	move.w	42(sp),d3
	subq	#1,d3
	move.b	#$fc,5(a0)

.next_long:
	move.l	(a3)+,6(a0)

	bsr.w	dbg_exec_cmd

	dbra	d3,.next_long

	movem.l	(sp)+,a0-a3/d0-d3/d7
	move.w	(sp)+,sr
	rts

dbg_reset:
	lea	dbg_acsi(pc),a0
	move.b	#$f5,5(a0)
	move.l	#$900db113,6(a0)

	bsr.w	dbg_exec_cmd

	move.l	#100,d1                     ; 0.5 second delay
	add.l	hz200.w,d1                  ;
.reset_delay
	cmp.l	hz200.w,d1                  ; Test timeout
	bgt.b	.reset_delay                ;

.reset_test
	lea	dbg_acsi(pc),a0
	move.b	#$f5,5(a0)
	move.l	#$00000000,6(a0)

	bsr.w	dbg_exec_cmd

	tst.b	d0
	bne.b	.reset_test

	clr.l	$426.w                      ; Corrupt RAM for full reset

	reset
.reset_wait
	bra.b	.reset_wait

dbg_b:
	move.w	sr,-(sp)
	movem.l	a0-a3/d0-d3/d7,-(sp)

	lea	dbg_acsi(pc),a0
	move.b	#$f1,5(a0)
	move.l	#$00000000,6(a0)
	move.b	42(sp),7(a0)

	bsr.w	dbg_exec_cmd

	movem.l	(sp)+,a0-a3/d0-d3/d7
	move.w	(sp)+,sr
	rts

dbg_w:
	move.w	sr,-(sp)
	movem.l	a0-a3/d0-d3/d7,-(sp)

	lea	dbg_acsi(pc),a0
	move.b	#$f2,5(a0)
	move.l	#$00000000,6(a0)
	move.w	42(sp),6(a0)

	bsr.w	dbg_exec_cmd

	movem.l	(sp)+,a0-a3/d0-d3/d7
	move.w	(sp)+,sr
	rts

dbg_l:
	move.w	sr,-(sp)
	movem.l	a0-a3/d0-d3/d7,-(sp)

	lea	dbg_acsi(pc),a0
	move.b	#$f4,5(a0)
	move.l	42(sp),6(a0)

	bsr.w	dbg_exec_cmd

	movem.l	(sp)+,a0-a3/d0-d3/d7
	move.w	(sp)+,sr
	rts

dbg_exec_cmd
	pea	(a0)
	bsr.w	acsi_exec_dbg_cmd
	move.l	(sp)+,a0

	move.w	#3000,d1     
.dbg_wait:
	dbra	d1,.dbg_wait
	rts

dbg_acsi
	dc.b	$00,$00,$00,$06
dbg_acsi_bytes
	dc.b	$0a,$00,$00,$00,$00,$00

dbg_dma_x
	ds.b	16

acsi_exec_dbg_cmd:
; Execute an ACSI command
; The command buffer is prefixed by the length of the command in bytes.
;
; TODO: support commands with no DMA if d1==0
;
; 4(sp): Command buffer
; 8(sp): DMA address (optional, unused if block count == 0)
;
; Alters: a0-a2/d0-d2
;
; Returns d0.w: -1 in case of timeout
;               DMA status in d0 8..15
;               Returned status byte in d0 0..7
;               0 in bits 8..15 if DMA completed a whole block

	moveq	#0,d0                       ; Clear d0 (will use as bytes only)

	st	flock.w                     ; Lock floppy drive

	moveq	#0,d0                       ; Set DMA length in blocks

	move.l	4(sp),a0                    ; a0 = Command buffer
	lea	dmactrl.w,a1                ; a1 = DMA control register
	lea	dma.w,a2                    ; a2 = DMA data register

	move.w	#$0180,(a1)
	move.w	#$0080,(a1)

.start_command:
	move.w	#$0088,(a1)                 ; Assert A1

	; Send command bytes
	; Command and data is in d0:
	;   First byte: d0 = $00xx008a (xx = command | device id)
	;  Middle byte: d0 = $00xx008a (xx = command byte)
	;    Last byte: d0 = $00xx0000 (xx = command byte) for DMA read
	;    Last byte: d0 = $00xx0080 (xx = command byte) for DMA write
	; d2.w is used as a middle byte counter because first and last bytes
	; are handled by special code
	moveq	#0,d0                       ; d0 0..15:
	move.b	#$8a,d0                     ; 8a = write command byte

	swap    d0                          ; Load the first command byte
	move.b	4(a0),d0                    ;

	or.b    1(a0),d0                    ; Add device id to the first byte

	moveq	#0,d2                       ; d2 = Middle command byte counter
	move.b	3(a0),d2                    ; first byte is
	subq	#1,d2                       ; handled specially

	lea	5(a0),a1                    ; a1 = pointer to middle bytes

	bra.b   .first_byte                 ; Send the first byte

.next_middle_byte:
	swap	d0                          ; Load command byte to
	move.b	(a1)+,d0                    ; the upper part of d0
.first_byte:
	swap	d0                          ;
	move.l	d0,(a2)                     ; Send $00xx008a to DMA
	bsr.b	.ack                        ; Wait for ack
	dbra	d2,.next_middle_byte        ; Next middle byte

	; Read status
	bsr.b	.ack                        ; Wait for status byte
	move.w	#$008a,dmactrl.w            ; Acknowledge status byte
	move.w	(a2),d0                     ; Read status to d0

.abort:
	; Unlock floppy drive
	sf	flock.w

	rts

.ack:
	; Wait until DMA ack (IRQ pulse)
	move.l	#600,d1                     ; 3 second timeout
	add.l	hz200.w,d1                  ;

.ack_wait:
	cmp.l	hz200.w,d1                  ; Test timeout
	blt.b	.ack_timeout                ;
	btst.b	#5,gpip.w                   ; Test command acknowledge
	bne.b	.ack_wait                   ;
	rts

.ack_timeout:
	moveq	#-1,d0                      ; Return -1
	addq.l	#4,sp                       ; to the caller of acsi_exec_cmd
	bra.b	.abort                      ;

; vim: ts=8 sw=8 sts=8 noet
dbg__end:

; vim: ts=8 sw=8 sts=8 noet

; ACSI access
	even

acsi_exec_cmd
; Execute an ACSI command in the acsi_cmd buffer
; d0: Target DMA address
; Returns: d0: status byte or -1 if timeout

	st	flock.w                     ; Lock floppy drive

	lea	acsi_cmd(pc),a0             ; a0 = Command buffer
	lea	dmactrl.w,a1                ; a1 = DMA control register
	lea	dma.w,a2                    ; a2 = DMA data register

	moveq	#0,d1                       ; d1 = command/data

	; Initialize the DMA chip
	btst	#1,(a0)                     ; Check for write flag
	bne.b	.dmarst                     ;
	move.w	#$100,d1                    ; Read mode: set write flag

.dmarst	move.b	#$90,d1                     ; DMA initialization
	move.w	d1,(a1)                     ;
	eor.w	#$100,d1                    ; Switch write flag
	move.w	d1,(a1)                     ;

	; DMA transfer
	move.b	d0,dmalow.w                 ; Set DMA address low
	lsr.l	#8,d0                       ;
	move.b	d0,dmamid.w                 ; Set DMA address mid
	lsr.w	#8,d0                       ;
	move.b	d0,dmahigh.w                ; Set DMA address high

	move.b	4(a0),d0                    ; Set DMA length
	move.w	d0,(a2)                     ;

	move.b	#$88,d1                     ;
	move.w	d1,(a1)                     ; assert A1

	move.b	#$8a,d1                     ; d1 = $0w8a = write command byte
	moveq	#4,d2                       ; d2 = Command byte counter
	                                    ;      (except last byte)
.next	swap	d1                          ;
	move.b	(a0)+,d1                    ; Read next command byte
	swap	d1                          ;
	move.l	d1,(a2)                     ; Send $00xx0w8a to DMA

	bsr.b	.ack                        ; Wait for ack

	dbra	d2,.next                    ; Next byte

	; Send last command byte and trigger DMA
	; The last byte is always 0 so it's not in the buffer
	and.l	#$00000100,d1               ; Keep only the write flag

	;move.w	d1,d0                       ; Write mode: d0 = $0100
	;lsr.w	#1,d0                       ; Write mode: d0 = $0080
	;or.w	d0,d1                       ; Write mode: d1 = $0180

	move.l	d1,(a2)                     ; Send last byte and start DMA

	; Read status byte
	bsr.b	.ack                        ; Wait for status byte
	move.b	#$8a,d1                     ;
	move.w	d1,(a1)                     ; Acknowledge status byte
	move.w	(a2),d0                     ; Read status to d0

.abort	sf	flock.w                     ; Unlock floppy drive

	rts                                 ; Exit acsi_exec_cmd

	; Wait until DMA ack (IRQ pulse)
.ack	move.l	#600,d0                     ; 3 second timeout
	add.l	hz200.w,d0                  ;
.await	cmp.l	hz200.w,d0                  ; Test timeout
	blt.b	.timout                     ;
	btst.b	#5,gpip.w                   ; Test command acknowledge
	bne.b	.await                      ;
	rts

.timout	moveq	#-1,d0                      ; Return -1
	addq.l	#4,sp                       ; to the caller of acsi_exec_cmd

	bra.b	.abort                      ; Uninitialize and return

; vim: ts=8 sw=8 sts=8 noet

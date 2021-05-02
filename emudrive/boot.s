	include	defines.i

; Extra memory for the driver
extramem=0

	org	$0                          ; $185c (TOS 1.62 FR, Hatari STE 4MB)

boot
	movem.l	d0-d7/a0-a6,-(sp)           ; Keep registers intact

	lea	load_cmd(pc),a0             ; Set boot drive id for ACSI
	or.b	d7,(a0)                     ;

	; Display welcome text
	lsr.b	#5,d7                       ; Compute drive id (0-7)
	lea	sd_text_drive(pc),a0        ; Patch drive id in the text
	add.b	d7,(a0)                     ; Add drive id to '0'

	; Allocate memory for the driver code
	pea	driver_blocks*512+extramem  ; Memory allocation size
	gemdos	Malloc,6                    ; Allocate memory

	tst.l	d0                          ; Check Malloc success
	beq.b	malloc_error                ;

	move.l	d0,a6                       ; a6 = driver base address

	bsr.w	acsi_load                   ; Load the driver

	tst.b	d0                          ; Check DMA transfer
	bne.b	driver_error                ; Free if error

	; Run the driver

	jsr	(a6)                        ; Run driver initialization

	tst.b	d0                          ; Test if the driver initialized
	beq.b	boot_chain                  ;

	; Driver unload
	; The driver is supposed to display its own message
	move.l	a6,-(sp)                    ; Free driver code
	gemdos	Mfree,6                     ;

	bra.w	boot_chain

driver_error
; DMA error: free RAM and exit
	move.l	a6,-(sp)                    ; Free driver code
	gemdos	Mfree,6                     ;

	pea	dma_error_text(pc)          ; Print abort text
	bra.w	boot_error                  ; Return to TOS boot

malloc_error
	pea	alloc_error_text(pc)        ; Print error text
	bra.w	boot_error                  ; Return to TOS boot

boot_chain
; Chain boot loader

	lea	boot(pc),a0                 ; a0 = boot sector address

	; Select boot chain drive
	move.b	boot_chain_drive(a0),load_cmd+1(a0)
	beq.b	exit_boot                   ; No boot chain if drive = 0

	clr.b	load_cmd+3(a0)              ; Load sector 0
	move.b	#1,load_cmd+4(a0)           ; Load 1 sector

	lea	$200(a0),a1                 ; Move away this boot code to
	move.w	#$7f,d0                     ; the next boot block
bootcpy	move.l	(a0)+,(a1)+                 ;
	dbra	d0,bootcpy                  ;

	lea	boot(pc),a1                 ;
	move.l	a1,d0                       ; d0 = current boot sector

	bra.w	boot_shadow+$200            ; Jump to the shadow copy
boot_shadow                                 ;

	bsr.b	acsi_load                   ; Load chain boot sector

	tst.b	d0                          ; Test if the chain boot sector
	beq.b	chain_boot_loaded           ; was loaded correctly

	pea	chain_error_text(pc)        ; Print chain load error
	bra.b	boot_error                  ; Exit

chain_boot_loaded
	pea	sd_text(pc)                 ; Print device id
	gemdos	Cconws,6                    ;

	pea	chain_boot_text(pc)         ; Print chain boot error
	gemdos	Cconws,6                    ;

	movem.l	(sp)+,d0-d7/a0-a6           ; Restore registers

	bra.w	boot-$200                   ; Jump to the new boot sector

boot_error                                  ; Error message is already pushed
	pea	sd_text(pc)                 ; Print device id
	gemdos	Cconws,6                    ;
	gemdos	Cconws,6                    ; Print error

exit_boot
	movem.l	(sp)+,d0-d7/a0-a6           ; Restore registers
	rts

acsi_load
; ACSI load routine
; d0: DMA target address
; Alters: d0-d2/a0-a2
; Returns: d0: status code

	st	flock.w                     ; Lock floppy drive

	lea	load_cmd(pc),a0             ; a0 = Command buffer
	lea	dmactrl.w,a1                ; a1 = DMA control register
	lea	dma.w,a2                    ; a2 = DMA data register

	; Initialize the DMA chip
	move.w	#$190,(a1)                  ; DMA read initialization
	move.w	#$90,(a1)                   ;

	; DMA transfer
	move.b	d0,dmalow.w                 ; Set DMA address low
	lsr.l	#8,d0                       ;
	move.b	d0,dmamid.w                 ; Set DMA address mid
	lsr.w	#8,d0                       ;
	move.b	d0,dmahigh.w                ; Set DMA address high

	moveq	#0,d0                       ;
	move.b	4(a0),d0                    ; Set DMA length
	move.w	d0,(a2)                     ;

	move.w	#$0088,(a1)                 ; assert A1

	move.l	#$8a,d1                     ; d1 = $008a = write command byte
	moveq	#4,d2                       ; d2 = Command byte counter
	                                    ;      (except last byte)

next_byte
	swap	d1                          ;
	move.b	(a0)+,d1                    ; Read next command byte
	swap	d1                          ;
	move.l	d1,(a2)                     ; Send $00xx008a to DMA

	bsr.b	acsi_ack                    ; Wait for ack

	dbra	d2,next_byte                ; Next byte

	move.l	#0,(a2)                     ; Send the last byte (always 0)

	bsr.b	acsi_ack                    ; Wait for status byte

	move.w	#$8a,(a1)                   ; Acknowledge status byte
	move.w	(a2),d0                     ; Read status to d0

acsi_load_abort
	move.w	#$180,(a1)                  ; Reset the DMA controller
	move.w	#$80,(a1)                   ;

	sf	flock.w                     ; Unlock floppy drive

	rts
acsi_ack
	; Wait until DMA ack (IRQ pulse)
	move.l	#600,d0                     ; 3 second timeout
	add.l	hz200.w,d0                  ;
acsi_ack_wait
	cmp.l	hz200.w,d0                  ; Test timeout
	blt.b	acsi_ack_timeout            ;
	btst.b	#5,gpip.w                   ; Test command acknowledge
	bne.b	acsi_ack_wait               ;
	rts

acsi_ack_timeout
	moveq	#-1,d0                      ; Return -1
	addq.l	#4,sp                       ; to the caller of acsi_load
	bra.b	acsi_load_abort             ;

load_cmd
	; Read command to send to the STM32
	; The last byte is always 0 so it's not here
	dc.b	$08,$E0,$00,$00,driver_blocks

sd_text
	dc.b	'ACSI device '
sd_text_drive
	dc.b	'0 ',0

chain_boot_text
	dc.b	'booting',13,10,0

chain_error_text
	dc.b	'boot error',13,10,0

alloc_error_text
	dc.b	'Malloc failed',13,10,0

dma_error_text
	dc.b	'DMA error',13,10,0

; Boot sector footer
	org	437

boot_chain_drive:                           ; Patched: 0 if no boot chain
	dc.b	$00                         ;          drive LUN

checksum:                                   ; Patched
	dc.w	$0000                       ;

; Driver blob
	org	440

driver_blocks=(511+driver_end-driver)/512   ; Number of blocks to load the TSR
driver:

	incbin	driver.bin

driver_end:

; vim: ts=8 sw=8 sts=8 noet

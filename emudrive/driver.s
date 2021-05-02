	org	$0
	include	defines.i

driver_start
	include	nodebug.i

	; TODO: Check if the driver is already loaded.
	; that sometimes happens on reset.

	lea	acsi_cmd(pc),a0             ; Set the boot device
	or.b	d7,(a0)                     ; to the ACSI command

	move.l	#driver_end-driver_start,-(sp) ; Shrink allocated RAM
	pea	driver_start(pc)            ;
	gemdos	Mshrink,10                  ;

	lea	bfloppy(pc),a0              ; Set the bfloppy flag
	move.w	nflops.w,d0                 ; if the machine has
	subq	#1,d0                       ; 2 hardware drives
	move.b	d0,(a0)                     ;

	lea	nextqry(pc),a0              ; Initialize nextqry
	move.l	hz200.w,(a0)                ;

	; Compute trap parameter offset
	; This is CPU dependant
	move.l	sp,d1                       ; d1 = SP before trap
	lea	gemdos_vector.w,a0          ; Test on trap #1
	move.l	(a0),a1                     ; Store current vector
	lea	trapchk(pc),a2              ; Compute test vector
	move.l	a2,(a0)                     ; Trap #1 will jump to trapchk
	trap	#1                          ; d0 = SP after trap
	move.l	a1,(a0)                     ; Restore trap #1 vector

	sub.l	d0,d1                       ; Compute offset
	lea	traplen(pc),a0              ; Save offset to traplen
	move.w	d1,(a0)                     ;

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

	; Refresh mediach flags and nflops

	bsr.w	finfo_query                 ; Query virtual floppy drives

	lea	mchmask(pc),a0              ; Set Mediach mask
	move.b	fimask(pc),(a0)             ; on all virtual drives

	moveq	#0,d0
	rts

trapchk	move.l	sp,d0
	rte

	include	acsi.i

; Hooks

	; Getbpb hook
	dc.b	'XBRA','A2ST'
old_getbpb
	ds.l	1
getbpb_hook

	include getbpb.i                     ; Rwabs hook code

	move.l	old_getbpb(pc),-(sp)        ; Forward to the previous handler
	rts                                 ;

	; Rwabs hook
	dc.b	'XBRA','A2ST'
old_rwabs
	ds.l	1
rwabs_hook

	include rwabs.i                     ; Rwabs hook code

	move.l	old_rwabs(pc),-(sp)         ; Forward to the previous handler
	rts                                 ;

	; Mediach hook
	dc.b	'XBRA','A2ST'
old_mediach
	ds.l	1
mediach_hook

	include mediach.i                   ; Mediach hook code

	move.l	old_mediach(pc),-(sp)       ; Forward to the previous handler
	rts                                 ;

	; BIOS hook
	dc.b	'XBRA','A2ST'
old_bios
	ds.l	1
bios_hook

	include bios.i                      ; BIOS hook code

	move.l	old_bios(pc),-(sp)          ; Forward to the previous handler
	rts                                 ;

	; XBIOS hook
	dc.b	'XBRA','A2ST'
old_xbios
	ds.l	1
xbios_hook

	include xbios.i                      ; BIOS hook code

	move.l	old_xbios(pc),-(sp)         ; Forward to the previous handler
	rts                                 ;

; Global functions

finfo_query
; Query floppy information
; Read sector $ffff on LUN 7
; Returns less than 512 bytes

	lea	nextqry(pc),a0              ;
	move.l	hz200.w,d0                  ; Do we need a query ?
	cmp.l	(a0),d0                     ;
	bge.b	.doqry                      ;

	rts
.doqry

	add.l	#200*5,d0                   ; Next refresh in 5 seconds
	move.l	d0,(a0)                     ;

finfo_query_now
	lea	finfo(pc),a0                ; a0 = finfo structure

	; Prepare ACSI command
	lea	acsi_cmd(pc),a1
	bclr	#1,(a1)+                    ; Read operation
	move.b	#$f0,(a1)+                  ; LUN 7, special sectors
	move.w	#$0001,(a1)+                ; Special sector 1
	move.b	#1,(a1)                     ; Query 1 sector

	clr.b	(a0)                        ; In case of failure: eject floppies
	move.l	a0,d0                       ; Set DMA target address

	bsr.w	acsi_exec_cmd               ; Start reading

	tst.b	d0                          ; Test if finfo is valid
	beq.b	.fi_ok                      ;

	lea	acsi_cmd(pc),a1             ; If not: eject virtual floppies
	clr.b	(a1)                        ;
.fi_ok

	; Update the nflops system variable to reflect fimask
	lea	fimask(pc),a1               ; a1 = updated floppy mask
	move.b	bfloppy(pc),d0              ; If the machine has 2 drive
	bne.b	.end                        ;

	move.w	#1,nflops.w                 ; Detach drive B

	btst	#1,(a1)                     ; If virtual B drive present
	beq.b	.end
	add.w	#1,nflops.w                 ; Attach drive B

.end	rts                                 ;

; Variables

acsi_cmd
	dc.b	$08,$00,$00,$00,$00         ; The last byte is always 0
fmask	ds.b	1                           ; Floppy mask
mchmask	ds.b	1                           ; Media change mask
bfloppy	ds.b	1                           ; Non-zero if drive B exists
	even

nextqry	ds.l	1                           ; Next hz200 at which finfo_query
                                            ; will refresh its information
traplen	ds.w	1                           ; Trap stack length
fid	ds.l	2                           ; Current floppy id
finfo                                       ; Floppy info (from the STM)
fimask	ds.b	1                           ;   Floppy mask
filerr	ds.b	1                           ;   Last error
fiid	ds.l	2                           ;   Floppy unique id
fibpb	ds.b	18*2                        ;   Floppy BPB
	ds.b	2                           ;   Padding

driver_end

; vim: ts=8 sw=8 sts=8 noet

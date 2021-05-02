; int32_t Rwabs (
;                 int16_t rwflag,  //  4(sp)  8(a6)
;                 VOID *buff,      //  6(sp) 10(a6)
;                 int16_t cnt,     // 10(sp) 14(a6)
;                 int16_t recnr,   // 12(sp) 16(a6)
;                 int16_t dev,     // 14(sp) 18(a6)
;                 int32_t lrecno   // 16(sp) 20(a6)
;               );

	move.w	14(sp),d2                   ; d2 = current device

	cmp.w	#2,d2                       ; Is it a floppy ?
	bge.w	.real                       ;

	lea	fmask(pc),a0                ; a0 = current floppy mask

	btst	#1,5(sp)                    ; Pay attention to media change ?
	bne.b	.nomch

	btst	d2,mchmask-fmask(a0)        ; Check media change flag
	beq.b	.nomch                      ;
	println	'CHNG' ;;;;;;;;;;;;;
	moveq	#E_CHNG,d0                  ; Media changed: return error
	rts                                 ;
.nomch
	btst	d2,(a0)                     ; Is the current floppy virtual ?
	beq.w	.real                       ;

	; The floppy is virtual

	link	a6,#0
	movem.l	d3-d5,-(sp)                 ; Store d3-d5

	moveq	#-1,d0                      ; Set error in case of NULL
	move.l	10(a6),d4                   ; d4 = current DMA address
	beq.b	.onerr                      ; NULL pointer !

	lea	acsi_cmd(pc),a0             ; Build the ACSI command

	move.w	14(a6),d3                   ; d3 = sector count

	and.b	#$e0,(a0)                   ; Reset ACSI command
	btst	#0,9(a6)                    ; Test if writing
	sne	d0                          ; Set bit if writing
	and.b	#$02,d0                     ; $02/$00
	or.b	#$08,d0                     ; $0a/$08
	or.b	d0,(a0)+                    ; Set ACSI command

	addq	#1,d2                       ;
	lsl.b	#5,d2                       ; Set LUN
	move.b	d2,(a0)                     ;

	move.w	16(a6),d5                   ; d5 = sector

.nxtsec	move.l	d4,d0                       ; Set DMA pointer

	lea	acsi_cmd+2(pc),a0           ; Point at sector number

	move.w	d5,(a0)+                    ; Set sector number

	cmp.w	#32,d3                      ; Test if cnt>32 sectors
	bgt.b	.oversz                     ;

	move.b	d3,(a0)+                    ; cnt<=32: transfer this amount
	bra.b	.dodma

.oversz	move.b	#32,(a0)+                   ; Oversize: transfer 32 sectors

.dodma	bsr.w	acsi_exec_cmd               ; Do the I/O

	tst.b	d0                          ; Stop the loop
	bne.b	.onerr                      ; on error

	add.l	#32*512,d4                  ; Move DMA pointer
	add.w	#32,d5                      ; Move sector number
	sub.w	#32,d3                      ; Are there still sectors ?
	bgt.b	.nxtsec

.onerr	movem.l	(sp)+,d3-d5                 ; Restore d3-d5
	unlk	a6

	tst.b	d0
	beq.b	.noerr

	cmp.l	#-1,d0                      ; Test if DMA returned -1
	bne.b	.lsterr                     ; If not, analyze lastErr

	; DMA timeout (or null pointer)

	rts

	; Remote error

.lsterr	bsr.w	finfo_query_now             ; Query lastErr

	move.b	filerr(pc),d0               ; Fetch lastErr
	ext.w	d0                          ; Extend to long
	ext.l	d0                          ;

	rts

.noerr	moveq	#0,d0
	rts

.real	; Talking to real hardware

; vim: ts=8 sw=8 sts=8 noet

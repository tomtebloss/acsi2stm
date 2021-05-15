; int32_t Rwabs (
;                 int16_t rwflag,
;                 VOID *buff,
;                 int16_t cnt,
;                 int16_t recnr,
;                 int16_t dev,
;                 int32_t lrecno
;               );
; Parameters
		rsreset
		rs.l	1
rwabs.rwflag	rs.w	1
rwabs.buff	rs.l	1
rwabs.cnt	rs.w	1
rwabs.recnr	rs.w	1
rwabs.dev	rs.w	1
rwabs.lrecno	rs.l	1

	move.w	rwabs.dev(sp),d2            ; d2 = current drive
	move.l	globals+dmask(pc),d0        ; d0 = current drive mask

;	println	'rwabs'   ;;;; XXX
;	dumpw	4(sp),7

	btst	#1,rwabs.rwflag+1(sp)       ; Pay attention to media change ?
	bne.b	.nomch

	move.l	globals+mchmask(pc),d0      ; d0 = mchmask
	btst	d2,d0                       ; Check media change flag
	beq.b	.nomch                      ;
	println	'CHNG' ;;;;;;;;;;;;;XXX
	moveq	#E_CHNG,d0                  ; Media changed: return error
	rts                                 ;

.nomch	move.l	globals+dmask(pc),d0        ; Is the current drive mounted ?
	btst	d2,d0                       ;
	bne.w	.mnted                      ;

	move.l	globals+fmask(pc),d0        ; Is the drive a filesystem ?
	btst	d2,d0                       ;
	beq.w	.real                       ;

	moveq	#ESECNF,d0                  ; The drive is a filesystem
	rts                                 ; refuse low-level access

	; The drive is mounted

.mnted	link	a6,#0
	movem.l	d3-d5,-(sp)                 ; Store d3-d5

	moveq	#-1,d0                      ; Set error in case of NULL
	move.l	4+rwabs.buff(a6),d4         ; d4 = current DMA address
	beq.b	.onerr                      ; NULL pointer !

	lea	globals+acsicmd+acsicmd.cmd(pc),a0 ; a0 = ACSI command

	move.w	4+rwabs.cnt(a6),d3          ; d3 = block count

	btst	#0,4+rwabs.rwflag+1(a6)     ; Test if writing
	sne	d0                          ; Set bit if writing
	and.b	#$02,d0                     ; $02/$00
	or.b	#$28,d0                     ; $2a/$28
	or.b	d0,(a0)+                    ; Set ACSI command

	move.b	#$40,(a0)+                  ; Set LUN to LBA drive

	move.b	d2,7(a0)                    ; Set drive number

	moveq	#0,d5                       ;
	move.w	4+rwabs.recnr(a6),d5        ; d5 = block

	cmp.w	#-1,d5                      ; If block == -1, use lrecno
	bne.b	.nxtsec                     ;
	move.l	4+rwabs.lrecno(a6),d5       ;

	clr.w	4(a0)                       ; Clear res and count MSB

.nxtsec	lea	globals+acsicmd+acsicmd.block(pc),a0 ; Point at block number

	move.l	d5,(a0)                     ; Set block number

	cmp.w	#32,d3                      ; Test if cnt>=32 blocks
	blt.b	.small                      ;

	move.l	d4,d0                       ; Set DMA pointer
	or.l	#$20000000,d0               ; Set DMA length
	move.b	#32,6(a0)                   ; Transfer 32 blocks

	bra.b	.dodma                      ; Start big DMA

.small	move.b	d3,6(a0)                    ; cnt<=32: transfer this amount

	moveq	#0,d0                       ; Set DMA length in d0[31..24]
	move.b	d3,d0                       ;
	ror.l	#8,d0                       ;

	or.l	d4,d0                       ; Set DMA pointer
	
.dodma	bsr.w	acsi_exec_cmd               ; Do the I/O

	tst.b	d0                          ; Stop the loop
	bne.b	.onerr                      ; on error

	add.l	#32*512,d4                  ; Move DMA pointer
	add.l	#32,d5                      ; Move block number
	sub.w	#32,d3                      ; Are there still blocks ?
	bgt.b	.nxtsec

.onerr	movem.l	(sp)+,d3-d5                 ; Restore d3-d5
	unlk	a6

	ext.w	d0                          ; Extend return value to long
	ext.l	d0                          ;

	rts

.real	; Talking to real hardware

; vim: ts=8 sw=8 sts=8 noet

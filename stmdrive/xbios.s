	move.w	traplen(pc),d0          ; d0 = trap stack frame len
	lea	(0,sp,d0),a0            ; a0 = parameters
	move.w	(a0),d1                 ; d1 = operation

	cmp.w	#Floprd,d1
	beq.w	.floprd
	cmp.w	#Flopwr,d1
	bne.w	.end

	; Flopwr entry point
	lea	globals+acsicmd(pc),a2  ; a2 = ACSI command
	move.b	#$2b,acsicmd.cmd(a2)    ; Set write command

	bra.b	.floprw                 ; Jump to common code

.floprd	; Floprd entry point
	lea	globals+acsicmd(pc),a2  ; a2 = ACSI command
	move.b	#$28,acsicmd.cmd(a2)    ; Set write command

; int16_t Floprd/wr(
;                    VOID *buf,
;                    int32_t filler,
;                    int16_t devno,
;                    int16_t sectno,
;                    int16_t trackno,
;                    int16_t sideno,
;                    int16_t count
;                  );
		rsreset
floprw.op	rs.w	1
floprw.buff	rs.l	1
floprw.dev	rs.w	1
floprw.sect	rs.w	1
floprw.track	rs.w	1
floprw.side	rs.w	1
floprw.count	rs.w	1

.floprw	movem.l	a3,-(sp)                ; Store registers
	move.l	a0,a3                   ; a3 = parameters

	bsr.w	finfo_query             ; Refresh mounted drives

	move.w	floprw.dev(sp),d2       ; d2 = current drive

	move.l	globals+dmask(pc),d0    ; Is the current drive mounted ?
	btst	d2,d0                   ;
	bne.w	.mnted                  ;

	move.l	globals+fmask(pc),d0    ; Is the drive a filesystem ?
	btst	d2,d0                   ;
	beq.w	.exit                   ;

	moveq	#ESECNF,d0              ; The drive is a filesystem
	bra.b	.return                 ; Return now

	; The drive is mounted

.mnted	moveq	#-1,d0                  ; Set error in case of NULL

	; Build the ACSI command

	lea	globals+acsicmd+acsicmd.lun(pc),a2 ; a2 = ACSI command LUN

	move.b	#$e0,(a2)+              ; Set LUN

	move.w	floprw.side(a3),(a2)+   ; Set side
	move.b	floprw.track+1(a3),(a2)+; Set track
	move.b	floprw.sect+1(a3),(a2)+ ; Set sector
	clr.w	(a2)+                   ; Set reserved and count MSB to 0
	move.b	floprw.count+1(a3),(a2)+; Set count LSB
	move.b	d2,(a2)                 ; Set control to drive id

	subq	#1,a2                   ; Point back at count LSB
	moveq	#0,d0                   ;
	move.w	(a2),d0                 ;
	clr.b	d0                      ;
	swap	d0                      ; d0[24..31] = sector count
	or.l	floprw.buff(a3),d0      ; d0 = current DMA address
	bsr.w	acsi_exec_cmd           ; Do the DMA operation

	ext.w	d0                      ; Extend result to word

.return	movem.l	(sp)+,a3                ; Restore registers
	rte

.exit	movem.l	(sp)+,a3                ; Restore registers
.end

; vim: ff=dos ts=8 sw=8 sts=8 noet

; int32_t Mediach ( int16_t dev );
; Parameters
		rsreset
		rs.l	1
mediach.dev	rs.w	1

	move.w	mediach.dev(sp),d2      ; d2 = current device

	bsr.w	finfo_query             ; Query device information

	lea	globals(pc),a0          ; a0 = globals
	move.l	mchmask(a0),d0          ; d0 = media change mask
	move.w	mediach.dev(sp),d2      ; d2 = current device

	print	'finfo' ;;;;;;XXX
	printw	d2 ;;;;;;;;;;;;;;XXX

	btst	d2,d0                   ; Is flag set
	beq.b	.nomch

	bclr	d2,d0                   ; Clear flag
	move.l	d0,mchmask(a0)          ;
	moveq	#1,d0                   ; Probably changed
	rts                             ; Return

.nomch	move.l	dmask(a0),d0            ; d0 = mounted drives flag
	or.l	fmask(a0),d0            ;
	
	btst	d2,d0                   ; Is the drive mounted ?
	beq.b	.end                    ; If not: follow through BIOS call

	moveq	#1,d0                   ; Probably changed
	rts                             ; Return

.end

; vim: ts=8 sw=8 sts=8 noet

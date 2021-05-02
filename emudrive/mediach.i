; int32_t Mediach (
;                   int16_t dev  // 4(sp)
;                 );

	move.w	4(sp),d2                    ; d2 = current device

	cmp.w	#2,d2                       ; Is it a floppy ?
	bge.w	.end                        ;

	bsr.w	finfo_query

	move.w	4(sp),d2                    ; d2 = current device
	lea	fmask(pc),a0                ; a0 = current floppy mask
	lea	fimask(pc),a1               ; a1 = updated floppy mask

	; Check if the floppy is virtual

	btst	d2,(a0)                     ; Is the current floppy virtual ?
	beq.b	.freal                      ;

	btst	d2,(a1)                     ; Does the virtual floppy exist ?
	beq.b	.removd

	; The floppy is virtual

	lsl.w	#2,d2                       ; d2 = floppy * sizeof(id)
	lea	(fid-fmask,a0,d2),a2        ; a2 = current id address
	move.l	(fiid-fimask,a1,d2),d1      ; d1 = new id

	cmp.l	(a2),d1                     ; Test if id is the same
	beq.b	.sameid                     ;

	move.l	d1,(a2)                     ; Update id
	bset	d2,mchmask-fmask(a0)        ; Set media change flag

	println	'Virtual floppy changed'

	moveq	#1,d0                       ; Return "probably changed"
	rts

.sameid	moveq	#0,d0                       ; Return "not changed"
	bclr	d2,mchmask-fmask(a0)        ; Clear media change flag
	rts

	; Virtual floppy removed

.removd	bclr	d2,(a0)                     ; Update floppy mask
	bset	d2,mchmask-fmask(a0)        ; Set media change flag

	println	'Virtual floppy removed'

	moveq	#2,d0                       ; Media definitely changed
	rts

.freal	btst	d2,(a1)                     ; Does the virtual floppy exist ?
	beq.b	.novirt                     ; No, let the BIOS do its thing

	; Virtual floppy inserted

	println	'Virtual floppy inserted'

	bset	d2,(a0)                     ; Update floppy mask

	lsl.w	#2,d2                       ; d2 = floppy * sizeof(id)
	lea	(fid-fmask,a0,d2),a2        ; a0 = current id address
	move.l	(fiid-fimask,a1,d2),d1      ; d1 = new id
	move.l	d1,(a2)                     ; Update id

	moveq	#2,d0                       ; Return "definitely changed"
	rts

.novirt	bclr	d2,mchmask-fmask(a0)        ; Clear media change flag

.end

; vim: ts=8 sw=8 sts=8 noet

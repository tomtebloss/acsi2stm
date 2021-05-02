; BPB *Getbpb (
;               int16_t dev // 4(sp)
;             ); 

	move.w	4(sp),d2                    ; d2 = current device

	cmp.w	#2,d2                       ; Is it a floppy ?
	bge.w	.end                        ;

	bsr.w	finfo_query                 ; Query virtual floppy drives

	move.w	4(sp),d2                    ; d2 = current device
	lea	fmask(pc),a0                ; a0 = current floppy mask
	lea	fimask(pc),a1               ; a1 = updated floppy mask

	bclr	d2,mchmask-fmask(a0)        ; Clear mchange mask

	btst	d2,(a1)                     ; Is the current floppy virtual ?
	bne.b	.virt                       ;

	bclr	d2,(a0)                     ; Real floppy: clear fmask
	bra.w	.end                        ; Let the BIOS handle its hardware

.virt	bset	d2,(a0)                     ; Virtual floppy: set fmask

	; Set media id

	move.w	d2,d0                       ; d0 = floppy * 4
	lsl.w	#2,d0                       ;
	move.l	(fiid-fimask,a1,d0),(fid-fmask,a0,d0) ; Set floppy id

	; Compute bpb address

	move.w	d2,d1                       ; d1 = floppy * 18
	add.w	d2,d1                       ;
	lsl.w	#4,d2                       ;
	add.w	d2,d1                       ;

	lea	(fibpb-fimask,a1,d1),a1     ; d0 = pointer to the virtual bpb
	move.l	a1,d0                       ;

	rts

.end

; vim: ts=8 sw=8 sts=8 noet

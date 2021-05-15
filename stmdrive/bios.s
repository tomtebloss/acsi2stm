; BIOS call

	move.w	globals+traplen(pc),d0      ; d0 = trap stack frame len
	lea	(sp,d0),a0                  ; a0 = parameters
	move.w	(a0),d1                     ; d1 = operation

	cmp.b	#Drvmap,d1
	bne.b	.end

	move.w	#-1,d2                      ; Just refresh mountpoint mask
	bsr.w	finfo_query                 ; Query virtual floppy drives

.end

; vim: ff=dos ts=8 sw=8 sts=8 noet

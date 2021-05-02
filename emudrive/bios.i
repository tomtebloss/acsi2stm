; BIOS call

	move.w	traplen(pc),d0              ; d0 = trap stack frame len
	lea	(0,sp,d0),a0                ; a0 = parameters
	move.w	(a0),d1                     ; d1 = operation

;	cmp.b	#1,d1
;	beq.b	.end
;	cmp.b	#2,d1
;	beq.b	.end
;	cmp.b	#3,d1
;	beq.b	.end
;	cmp.b	#8,d1
;	beq.b	.end
;	cmp.b	#6,d1
;	beq.b	.end
;
;	print	'bios'
;	printw	d1
;	printw	2(a0)

;	println	'drvmap' ;;;;;;;;;;;;;;;;;
;	bsr.w	finfo_query                 ; Query virtual floppy drives

.end
;;;;;;;;;;;;;;;;;;;

; Useful to parse parameters correctly

;	; Compute trap parameter offset
;	; This is CPU dependant
;	move.l	sp,d1                       ; d1 = SP before trap
;	lea	gemdos_vector.w,a0          ; Test on trap #1
;	move.l	(a0),a1                     ; Store current vector
;	lea	trapchk(pc),a2              ; Compute test vector
;	move.l	a2,(a0)                     ; Trap #1 will jump to trapchk
;	trap	#1                          ; d0 = SP after trap
;	move.l	a1,(a0)                     ; Restore trap #1 vector
;
;	sub.l	d0,d1                       ; Compute offset
;	lea	traplen(pc),a0              ; Save offset to traplen
;	move.w	d1,(a0)                     ;

;trapchk	move.l	sp,d0
;	rte

;traplen	ds.w	1                           ; Trap stack length

; vim: ts=8 sw=8 sts=8 noet

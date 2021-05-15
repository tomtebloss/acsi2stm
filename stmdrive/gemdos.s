; GEMDOS call

	move.w	globals+traplen(pc),d0  ; d0 = trap stack frame len
	lea	(sp,d0),a0              ;
	move.w	(a0),d1                 ; d1 = operation

	move.l	a3,-(sp)                ; Save a3
	move.l	a0,a3                   ; a3 = parameters

	cmp.w	#Dcreate,d1
	blt.w	gemdos_pass
	cmp.w	#Fsnext,d1
	bgt.w	gemdos_pass
;	print	'gemdos' ;;;;; XXX
;	dumpw	(a3),6     ;;;;; XXX

	sub.w	#Dcreate,d1             ; Compute offset in the jump table
	lsl.w	#1,d1                   ;
	move.w	.jtable(pc,d1),d1       ; d1 = Operation address offset
	lea	.jtable(pc,d1),a1       ; a1 = Absolute jump address

	jmp	(a1)                    ; Execute the function

; Jump table
.jtable
	dc.w	Dpath_impl-.jtable      ; 57
	dc.w	Dpath_impl-.jtable      ; 58
	dc.w	Dpath_impl-.jtable      ; 59
	dc.w	Fopen_impl-.jtable      ; 60
	dc.w	Fopen_impl-.jtable      ; 61
	dc.w	Fclose_impl-.jtable     ; 62
	dc.w	Fread_impl-.jtable      ; 63
	dc.w	Fread_impl-.jtable      ; 64
	dc.w	Dpath_impl-.jtable      ; 65
	dcb.w	71-65-1,gemdos_pass-.jtable
	dc.w	Dgetpath_impl-.jtable   ; 71
	dcb.w	78-71-1,gemdos_pass-.jtable
	dc.w	Fsfirst_impl-.jtable    ; 78
	dc.w	Fsnext_impl-.jtable     ; 79

; Implementation files
	include	dpath.s                 ; Dgetpath, Dsetpath
	include	fsfn.s                  ; Fsfirst, Fsnext
	include	file.s                  ; Fopen, Fread, Fclose, ...

; Exit the call immediately
gemdos_exit
	ext.w	d0                      ; Extend error code
gemdos_exit_word                        ; Exit returning a word
	ext.l	d0                      ;
gemdos_exit_long                        ; Exit returning a long
	move.l	(sp)+,a3                ; Restore a3

;	print	'ret=' ;;;;; XXX
;	printl	d0  ;;;;;;; XXX
	rte                             ;

; Pass the call to GEMDOS
gemdos_pass
	move.l	(sp)+,a3                ; Restore a3

; vim: ff=dos ts=8 sw=8 sts=8 noet

; Fsfirst/next

; struct FsfirstW
		rsreset
FsfirstW.path...=pathLen
FsfirstW.path	rs.b	FsfirstW.path...
FsfirstW.attr	rs.w	1
FsfirstW...	rs.b	0

; struct dta
		rsreset
dta.magic	rs.w	1
dta.drive	rs.b	1
dta.flags	rs.b	1
dta.file	rs.b	6
dta.pattern	rs.b	11
dta.system	rs.b	23
dta...		rs.b	0

; Fsfirst and Fsnext implementation
; Input:
;   a3 = parameters

Fsfirst_impl
		rsreset
		rs.w	1               ; GEMDOS operation code
Fsfirst.path	rs.l	1
Fsfirst.attr	rs.w	1
Fsfirst...	rs.b	0

	move.l	Fsfirst.path(a3),a0     ; Test if the path is owned by us
	bsr.w	ownpath                 ; d2 = path drive number
	beq.w	gemdos_pass             ; Pass the call to gemdos if not ours

	lea	globals+blkbuf(pc),a2   ; a2 = DMA block

	; Copy the file name to the buffer
	move.l	Fsfirst.path(a3),a0
	strncpy	FsfirstW.path(a2),(a0),#FsfirstW.path...

	; Set attribute
	move.w	Fsfirst.attr(a3),FsfirstW.attr(a2)

	move.b	d2,Fsfirst.path(a3)     ; Use parameters as a scratch buffer

	move.w	(a3),d1                 ; Build and send the write command
	move.b	d2,d0                   ;
	bsr.w	fsmount_write_command   ;

	tst.b	d0                      ;
	bne.w	gemdos_exit             ;

	move.b	Fsfirst.path(a3),d0     ; Restore drive number
	move.w	(a3),d1                 ; Build and send the read command
	bsr.w	fsmount_read_command    ;

	tst.b	d0                      ;
	bne.w	gemdos_exit             ;

	gemdos	Fgetdta,2               ; Get DTA address
	move.l	d0,a2                   ; a2 = dta address

	memcpy	(a2),globals+blkbuf(pc),#dta... ; Copy the DTA to its real place

	move.b	Fsfirst.path(a3),dta.drive(a2) ; Set drive number in the DTA

	moveq	#0,d0                   ; Success

	bra.w	gemdos_exit

Fsnext_impl

	gemdos	Fgetdta,2               ; Get the current DTA
	move.l	d0,a3                   ; a3 = current DTA

	move.w	dta.magic(a3),d0        ; Check magic number
	cmp.w	#$0102,d0               ;
	bne.w	gemdos_pass             ;

	move.b	dta.drive(a3),d2        ; Check that we own the drive
	bsr.w	owndrv                  ;
	beq.w	gemdos_pass             ; Pass the call to GEMDOS if not owned

	memcpy	globals+blkbuf(pc),(a3),#dta... ; Send the DTA through DMA
	move.b	d2,d0                   ; Set drive
	move.w	#Fsnext,d1              ; Set GEMDOS operation
	bsr.w	fsmount_write_command   ; Send the write command

	tst.b	d0                      ; Check for errors
	bne.w	gemdos_exit             ;

	move.w	#Fsnext,d1              ; Set GEMDOS operation
	move.b	dta.drive(a3),d0        ; Restore drive number
	bsr.w	fsmount_read_command    ; Send the read command

	move.w	d0,d1                   ; Store d0 (result) in d1

	memcpy	(a3),globals+blkbuf(pc),#dta... ; Copy data back into the DTA

	move.w	d1,d0                   ; Restore error code in d0

	bra.w	gemdos_exit

; vim: ff=dos ts=8 sw=8 sts=8 noet

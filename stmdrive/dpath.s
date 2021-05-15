; Dgetpath / Dsetpath

Dgetpath_impl
		rsreset
		rs.w	1               ; GEMDOS operation code
Dgetpath.path	rs.l	1
Dgetpath.drv	rs.w	1
Dgetpath...	rs.b	0

	pea	.drvok(pc)              ; Call return address
	move.w	Dgetpath.drv(a3),d2     ; Test if owning the drive
	bne.w	owndrv                  ; Drive specified: call owndrv
	bra.w	owncur                  ; Drive not specified: call owncur
.drvok	beq.w	gemdos_pass             ; Not our drive: pass the call

	move.w	d2,d0                   ; Set drive for the read command
	move.w	(a3),d1                 ; Set GEMDOS command
	bsr.w	fsmount_read_command    ; Send the read command

	move.l	Dgetpath.path(a3),a1    ; Copy the path to the target buffer
	strncpy	(a1),globals+blkbuf(pc),#blkbuf...

	bra.w	gemdos_exit             ; Return

; Generic function taking one path as a parameter
Dpath_impl
		rsreset
		rs.w	1               ; GEMDOS operation code
Dpath.path	rs.l	1
Dpath...	rs.b	0

	bsr.w	owncur                  ; Test if we own current drive
	beq.w	gemdos_pass             ; Pass if not owned

	move.l	Dpath.path(a3),a0       ; Copy the path to the DMA buffer
	strncpy	globals+blkbuf(pc),(a0),#blkbuf...

	move.w	d2,d0                   ; Set drive for the write command
	move.w	(a3),d1                 ; Set GEMDOS command
	bsr.w	fsmount_write_command   ; Send the write command

	bra.w	gemdos_exit             ; Return

; vim: ff=dos ts=8 sw=8 sts=8 noet

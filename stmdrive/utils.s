; Various utility functions

blkcpy_impl
; Copy one block of data. Unaligned copy.
; Inputs:
;   a0 = Source
;   a1 = Destination
; Alters: a0, a1, d0.w
	move.w	$200/8-1,d0             ; d0 = counter
.cpy	rept	8                       ;
	move.b	(a0)+,(a1)+             ; Copy bytes (expanded 8 times)
	endr                            ;
	dbra	d0,.cpy                 ; Next block
	rts

memcpy_impl
; Copy data.
; Optimizing this is an exercise left to the reader.
; Inputs:
;   a0 = Source
;   a1 = Destination
;   d0.w = Bytes to copy
; Alters: a0, a1, d0
	subq	#1,d0                   ; Adjust for dbra
	blt.b	.nocpy                  ; 0 byte to copy
.cpy	move.b	(a0)+,(a1)+             ; Copy one byte
	dbra	d0,.cpy                 ; Next byte
.nocpy	rts

strncpy_impl
; Copy string.
; Optimizing this is an exercise left to the reader.
; Inputs:
;   a0 = Source
;   a1 = Destination
;   d0.w = Maximum bytes to copy
; Alters: a0, a1, d0
	subq	#1,d0                   ; Adjust for dbra
	blt.b	.nocpy                  ; 0 byte to copy
.cpy	move.b	(a0)+,(a1)+             ; Copy one byte
	dbeq	d0,.cpy                 ; Next byte
.nocpy	rts

ownpath
; Test if a path is mounted by the driver
; Input:
;  a0 = pointer to the path string
; Returns:
;  d0 = drive number of the path
;  Z flag clear if path owned by the driver

	move.l	a0,d1                   ; Test if a0 is a null pointer
	bne.b	.notnul                 ;
	moveq	#-1,d0                  ; Unknown or invalid path
	cmp.w	d0,d0                   ; Set Z flag
	rts                             ; Null pointer

.notnul	moveq	#0,d1                   ; Prepare return value

	move.b	(a0),d0                 ; d1 = first character
	beq.b	.notabs                 ; Test empty string

	cmp.b	#'\',(a0)               ; Shortcut for very common \PATH form
	beq.b	owncur                  ; Relative path

	tst.b	1(a0)                   ; 1 letter long relative path
	beq.b	owncur                  ; Relative path

	cmp.b	#':',1(a0)              ; Something + ':' = absolute
	bne.b	.notabs                 ;

	moveq	#'A',d2                 ; d2 = 'A' for speed

	cmp.b	d2,d0                   ; Check A-Z range
	bge.b	.range1                 ;
	cmp.w	d0,d0                   ; Set Z flag
	rts                             ; Out of range
.range1	cmp.b   #'Z',d0                 ;
	ble.b	.range2                 ;
	cmp.w	d0,d0                   ; Set Z flag
	rts                             ; Out of range

.range2	sub.b	d2,d0                   ; Convert drive letter to number
	move.b	d0,d2                   ; Delegate work to owndrv 
	bra.b	owndrv                  ;

.notabs tst.b	2(a0)                   ; Not an absolute path
	beq.b	owncur                  ; Test device identifiers
	cmp.b	#':',3(a0)              ; (CON:, AUX:, ...)
	bne.b	owncur                  ;
	rts                             ; Not a drive (Z flag already set)

owncur
; Test if the current drive is mounted by the driver
; Returns:
;  d0 = drive number of the path
;  Z flag clear if the current drive is owned by the driver

	gemdos	Dgetdrv,2               ; Get current drive
	move.w	d0,d2                   ; Set drive number into d2 for owndrv

owndrv
; Test if a drive is mounted
; Input:
;  d2.b = drive number
; Returns:
;  d2.b = drive number or -1 if invalid
;  Z flag clear if the drive is owned by the driver
	bsr.w	finfo_query             ; In case of error, will query bit 31
	bne.b	.fierr                  ; which is always 0: perfect !

	moveq	#0,d1                   ; Convert drive number to bit mask
	bset	d2,d1                   ; d2 = drive bit to test
	and.l	globals+fmask(pc),d1    ; Matches with the mount mask ?
.fierr	rts                             ; Return bit mask

ownfd
; Test if a file descriptor is owned
; Input:
;  d0.w = file descriptor
; Returns:
;  d2.b = drive number or -1 if invalid
;  Z flag clear if the file descriptor is owned by the driver
	move.w	d0,d1                   ; Check the magic number
	and.w	#fdmask,d1              ;
	cmp.w	#fdmagic,d1             ;
	beq.b	.own                    ;

	; Not our file descriptor
	moveq	#-1,d2                  ; Set d2 to -1
	cmp.w	d2,d2                   ; Set Z flag
	rts                             ; Return error

.own	move.w	d0,-(sp)                ; Compute drive number
	move.b	(sp)+,d2                ;
	and.b	#$1f,d2                 ; Filter out magic number bits and clear Z
	rts

fsmount_read_command
; Input:
;  d0 = drive id
;  d1 = command id
;  blkbuf = 512 bytes block to send
; Output:
;  d0 = ACSI status code
	; Build the command
	lea	globals+acsicmd+acsicmd.cmd(pc),a0 ; a0 = acsi command
	move.b	#$28,(a0)+              ; $28 = Read command
	bra.b	_fsmount_command        ; Build the rest and send the command

fsmount_write_command
; Input:
;  d0.b = drive id
;  d1.w = command id
;  blkbuf = 512 bytes block to send
; Output:
;  d0 = ACSI status code
	; Build the command
	lea	globals+acsicmd+acsicmd.cmd(pc),a0 ; a0 = acsi command
	move.b	#$2a,(a0)+              ; $2a = Write command

_fsmount_command
; Private function
	; Build the rest of the command
	move.b	#$e0,(a0)+              ; $e0 = Special blocks

	move.l	#$00100002,(a0)+        ; fs command block

	addq.l	#1,a0                   ; Skip reserved byte

	move.w	d1,-(sp)                ; Put command into count
	move.b	(sp)+,(a0)+             ; uses the stack trick to get msb
	move.b	d1,(a0)+                ;

	move.b	d0,(a0)+                ; Drive id in control

	lea	globals+blkbuf(pc),a0   ; Set DMA address
	move.l	a0,d0                   ;

	bset	#24,d0                  ; Set DMA length to 1

	bra.w	acsi_exec_cmd           ; Send the command


; vim: ff=dos ts=8 sw=8 sts=8 noet

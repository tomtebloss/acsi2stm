; ACSI DMA stress test

	text

	include	defines.i

	move.w	#8,-(sp)                ; Cursor rate
	clr.w	-(sp)                   ; Hide cursor
	xbios	Cursconf,6              ; Configure cursor

	pea	welcome(pc)             ; Print welcome text
	gemdos	Cconws,6                ;

	Super                           ; Enter supervisor mode

loop	move.w	#3,d7                   ; d7 = Spinner index

pass	; Do DMA transfers

	lea	pattern(pc),a0          ; Send pattern
	move.l	a0,d0                   ; through DMA
	bsr.w	acsi_exec_cmd           ;

	lea	acsi_cmd(pc),a0         ; Switch to read mode
	eor.b	#$02,(a0)               ;

	lea	buffer(pc),a0           ; Read pattern back
	move.l	a0,d0                   ; from DMA
	bsr.w	acsi_exec_cmd           ;
	
	lea	acsi_cmd(pc),a0         ; Switch back to write mode
	eor.b	#$02,(a0)               ;

	; Test if data is corrupt

	lea	pattern(pc),a0          ; a0 = pattern
	lea	buffer(pc),a1           ; a1 = received data
	move.w	#511,d0                 ; Byte counter

compare	cmp.b	(a0)+,(a1)+             ; Compare one byte
	beq.b	noerr                   ;

	move.w	#'X',-(sp)              ; Print X for each error
	gemdos	Cconout,4               ;
	bra.b	err

noerr	dbra	d0,compare              ; Compare loop

err	lea	spinner(pc),a0          ; Load spinner character
	move.b	0(a0,d7),d0             ;
	ext.w	d0                      ;

	move.w	d0,-(sp)                ; Display the spinner
	gemdos	Cconout,4               ;
	move.w	#8,-(sp)                ;
	gemdos	Cconout,4               ;

	gemdos	Cconis,2                ; Check if a key was pressed
	tst.w	d0                      ;
	bne.b	exit                    ; Exit on key press

	dbra	d7,pass                 ; Next pass

	bra.b	loop                    ; Loop

exit	Pterm0

	include	acsi.i

acsi_cmd
	dc.b	$0a,$f0,$01,$00,$01

; Generates patterns on data lines
pat64	macro
	dc.b	%00000000,%11111111,%00000000,%11111111,%00000000,%11111111,%00000000,%11111111
	dc.b	%00000000,%00000000,%00000000,%00000000,%00000000,%00000000,%00000000,%00000000
	dc.b	%11111111,%11111111,%11111111,%11111111,%11111111,%11111111,%11111111,%11111111
	dc.b	%01010101,%10101010,%01010101,%10101010,%01010101,%10101010,%01010101,%10101010
	dc.b	%00000001,%00000010,%00000100,%00001000,%00010000,%00100000,%01000000,%10000000
	dc.b	%00000001,%00000010,%00000100,%00001000,%00010000,%00100000,%01000000,%10000000
	dc.b	%11111110,%11111101,%11111011,%11110111,%11101111,%11011111,%10111111,%01111111
	dc.b	%11111110,%11111101,%11111011,%11110111,%11101111,%11011111,%10111111,%01111111
	endm

pat64x	macro
	pat64
	ifgt	\1
	pat64x	\1-1
	endc
	endm

welcome	dc.b	'Welcome to the ACSI DMA stress test',     13,10
	dc.b	                                           13,10
	dc.b	'This program requires an ACSI2STM module',13,10
	dc.b	'that handles ACSI ID 0.',                 13,10
	dc.b	                                           13,10
	dc.b	'The program blasts DMA reads and writes', 13,10
	dc.b	'non-stop. Each failure prints "X".',      13,10
	dc.b	                                           13,10
	dc.b	'If you only see the spinner below, DMA',  13,10
	dc.b	'works perfectly. If you see a line of X', 13,10
	dc.b	'before the spinner, DMA has problems.',   13,10
	dc.b	                                           13,10
	dc.b	'To stop the test, press any key.',        13,10
	dc.b	                                           13,10
	dc.b	0

spinner	dc.b	'/-\|'

	even
pattern	pat64x	8                       ; 512 bytes of pattern data

	bss

buffer	ds.b	512                     ; Data buffer

; vim: ts=8 sw=8 sts=8 noet

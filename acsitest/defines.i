; System calls

syscall	macro
	move.w	#\2,-(sp)                   ; Push syscall identifier
	trap	#\1                         ; Do the call

	ifnc	'','\3'                     ; If a stack rewind parameter is passed
	ifgt	\3-8                        ;
	lea	\3(sp),sp                   ; Rewind the stack
	else                                ;
	addq	#\3,sp                      ; Rewind using addq
	endc                                ;
	endc                                ;
	endm

; GEMDOS system call

gemdos	macro
	syscall	1,\1,\2
	endm

Pterm0	macro
	clr.w	-(sp)
	trap	#1
	endm

Super	macro
	clr.l	-(sp)
	move.w	#Super,-(sp)
	trap	#1
	addq	#4,sp
	endm

; GEMDOS calls
Cconout=2
Cconws=9
Cconis=11
Super=32

; XBIOS system calls

xbios	macro
	syscall	14,\1,\2
	endm

Cursconf=21

; System variables

flock=$43e                                  ; Floppy semaphore
hz200=$4ba                                  ; 200Hz timer


; DMA hardware registers

gpip	equ	$fffffa01
dma	equ	$ffff8604
dmadata	equ	dma
dmactrl	equ	dma+2
dmahigh	equ	dma+5
dmamid	equ	dma+7
dmalow	equ	dma+9

; vim: ts=8 sw=8 sts=8 noet

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

; GEMDOS calls
Cconin=1
Cconout=2
Cconws=9
Cconis=11
Malloc=72
Mfree=73
Mshrink=74

; BIOS system call

bios	macro
	syscall	13,\1,\2
	endm

Bconin=2
Bconout=3
Rwabs=4
Setexc=5
Getbpb=7
Mediach=9
Drvmap=10

; XBIOS system calls

xbios	macro
	syscall	14,\1,\2
	endm

Floprd=8
Flopwr=9
Flopfmt=10

; System variables

flock=$43e                                  ; Floppy semaphore
hz200=$4ba                                  ; 200Hz timer
nflops=$4a6                                 ; Number of mounted floppies


; DMA hardware registers

gpip	equ	$fffffa01
dma	equ	$ffff8604
dmadata	equ	dma
dmactrl	equ	dma+2
dmahigh	equ	dma+5
dmamid	equ	dma+7
dmalow	equ	dma+9


; Exception vectors
gemdos_vector=$84
bios_vector=$b4
xbios_vector=$b8
getbpb_vector=$472
rwabs_vector=$476
mediach_vector=$47e

; GEMDOS error codes
; From emuTOS
E_OK=0          ; OK, no error
ERR=-1          ; basic, fundamental error
ESECNF=-8       ; sector not found
EWRITF=-10      ; write fault
EREADF=-11      ; read fault
EWRPRO=-13      ; write protect
E_CHNG=-14      ; media change
EBADSF=-16      ; bad sectors on format
EOTHER=-17      ; insert other disk

; vim: ts=8 sw=8 sts=8 noet

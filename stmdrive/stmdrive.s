; ACSI2STM Atari hard drive emulator
; Copyright (C) 2019-2021 by Jean-Matthieu Coulon

; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.

; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.

; ACSI2STM driver loader, TOS application version

	text
stmdrive

	include	defines.s
	include	structs.s
	include	nodebug.s

	Super                           ; Query Super powers

	moveq	#0,d7                   ; Set ACSI device 0
.drvini	pea	welcome_text(pc)        ; Show welcome text
	gemdos	Cconws,6                ;

	bsr.b	driver_payload          ; Run driver initialization

	tst.b	d0                      ; Test if the driver initialized
	beq.b	.dmaerr                 ;

	clr.w	-(sp)                   ; Return success
	gemdos	Ptermres                ; Leave the driver in RAM

.dmaerr	add.b	#$20,d7                 ; Try next device
	lea	welcome_drv(pc),a0      ; Modify welcome message
	add.b	#1,(a0)                 ;
	bne.b	.drvini                 ;

	pea	error_text(pc)          ; Display error
	gemdos	Cconws,6                ;

	gemdos	Cconin,2                ; Wait for a key press

	Pterm0                          ; Hardware not found: just exit

driver_payload
	include	main.s                  ; Main driver code

	data
welcome_text	dc.b	27,'f','Searching for ACSI2STM on ACSI device '
welcome_drv	dc.b	'0',13,10,0

error_text	dc.b	'ACSI2STM not found',13,10,0

	bss
globals	ds.b	globals...              ; All globals in BSS

; vim: ff=dos ts=8 sw=8 sts=8 noet

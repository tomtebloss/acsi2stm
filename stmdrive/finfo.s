; Filesystem info query

finfo_query
; Query filesystem information
; d2.b = drive to query
; d2.b will contain the effective drive on return (-1 if error)
; Read sector $100001 on LUN 7
; Returns less than 512 bytes

	move.l	hz200.w,d0              ; d0 = Query timestamp
	lea	globals+nextqry(pc),a0  ; a0 = Next query timestamp

	cmp.b	#-1,d2                  ; If query for no drive
	beq.b	.cache                  ; always use cache

	cmp.b	globals+finfo+finfo.dev+1(pc),d2 ; If data is for another device
	bne.b	.doqry                  ; always do the query

.cache	cmp.l	(a0),d0                 ; Refresh if query cache
	bge.b	.doqry                  ; timed out

	moveq	#0,d0                   ; Return success
	rts                             ;

.doqry	add.l	#200*5,d0               ; Next refresh in 5 seconds
	move.l	d0,(a0)                 ;

finfo_query_now
	; Prepare ACSI command
	lea	globals+acsicmd+acsicmd.cmd(pc),a1 ; a1 = acsi command
	bclr	#1,(a1)+                ; Read operation
	move.b	#$e0,(a1)+              ; LUN 7
	move.l	#$00100001,(a1)+        ; Special sector $100001
	clr.w	(a1)+                   ; Reserved + count msb
	move.b	#1,(a1)+                ; Query 1 sector
	move.b	d2,(a1)                 ; Drive identifier

	lea	globals+finfo(pc),a0    ; Write to finfo struct
	move.l	a0,d0                   ; Set DMA target address

	or.l	#$01000000,d0           ; Set DMA length

	bsr.w	acsi_exec_cmd           ; Execute the query

	tst.b	d0                      ; Abort if DMA error
	beq.b	.succss                 ;

	print	'finfo_query DMA error' ; XXX
	printw	d0                      ; XXX

	lea	globals(pc),a0          ;
	clr.l	finfo+finfo.dmask(a0)   ; Eject all drives
	clr.l	finfo+finfo.fmask(a0)   ;
	clr.l	fmask(a0)               ;
	clr.l	fmask(a0)               ;
	move.l	#$ffffffff,mchmask(a0)  ; Mark all drives as changed

	moveq	#-1,d2                  ; Invalidate drive
	moveq	#-1,d0                  ; Return an error
	rts                             ;

	; Update the nflops system variable to reflect finfo.dmask
.succss	lea	globals+finfo(pc),a2    ; a2 = finfo structure
	lea	finfo.dmask(a2),a1      ; a1 = updated drive mask lsb
	move.w	globals+bfloppy(pc),d0  ; Test if the machine has 2 drives
	bne.b	.drvbit                 ;

	move.w	#1,nflops.w             ; Detach drive B
	btst	#1,(a1)                 ; If virtual B drive present
	beq.b	.drvbit
	add.w	#1,nflops.w             ; Attach drive B

	; Update the drvbits system variable

.drvbit	lea	globals(pc),a0          ; a0 = globals
	move.l	oldmask(a0),d0          ; Get all real drives
	or.l	finfo.dmask(a2),d0      ; Add all mounted drives
	or.l	finfo.fmask(a2),d0      ; Add all mounted filesystems
	move.l	d0,drvbits.w            ; Update drvbits

	; Update dmask

	move.l	finfo.dmask(a2),d1      ; d1 = updated drive mask

	move.l	dmask(a0),d2            ; Update mchmask for
	eor.l	d1,d2                   ; inserted/removed drives
	or.l	d2,mchmask(a0)          ;

	move.l	d1,dmask(a0)            ; Update current drive mask

	; Update fmask

	move.l	finfo.fmask(a2),d1      ; d1 = updated filesystem mask

	move.l	fmask(a0),d2            ; Update mchmask for
	eor.l	d1,d2                   ; inserted/removed drives
	or.l	d2,mchmask(a0)          ;

	move.l	d1,fmask(a0)            ; Update current filesystem mask

	; Update drive id

	move.w	finfo.dev(a2),d2        ; d2 = drive number

	cmp.w	#-1,d2                  ; No specific drive queried
	beq.b	.id_ok                  ;

	or.l	finfo.dmask(a2),d1      ; d1 = mounted drives | filesystems
	btst	d2,d1                   ; Is the current drive mounted ?
	beq.w	.id_ok                  ;

	move.l	finfo.id(a2),d0         ; d0 = new drive id
	move.w	d2,d1                   ; d1 = drive number * 4
	lsl.w	#2,d1                   ;
	cmp.l	drvids(a0,d1),d0        ; Compare drive ids
	beq.b	.id_ok                  ;

	move.l	d0,drvids(a0,d1)        ; Update id
	
	move.l	mchmask(a0),d0          ; Set media change flag
	bset	d2,d0                   ;
	move.l	d0,mchmask(a0)          ;
.id_ok

	moveq	#0,d0                   ; Return success
	rts                             ;


; vim: ff=dos ts=8 sw=8 sts=8 noet

; XBIOS call

	move.w	traplen(pc),d0              ; d0 = trap stack frame len
	lea	(0,sp,d0),a0                ; a0 = parameters
	move.w	(a0),d1                     ; d1 = operation

	cmp.w	#Floprd,d1
	blt.w	.end
	cmp.w	#Flopwr,d1
	bgt.w	.end

	tst.w	14(a0)                      ; If accessing first track
	beq.b	.notfirst                   ;
	bsr.w	finfo_query                 ; Refresh finfo because old TOS
                                            ; likes this function to check for
.notfirst                                   ; a floppy

	lea	finfo(pc),a1                ; a1 = finfo structure

	move.w	10(a0),d0                   ; d0 = floppy id
	btst.b	d0,(a1)                     ; If the floppy is not virtual
	beq.w	.end                        ; Jump to system XBIOS

; int16_t Floprd/wr(
;                    VOID *buf,       //  2(a0)
;                    int32_t filler,  //  6(a0)
;                    int16_t devno,   // 10(a0)
;                    int16_t sectno,  // 12(a0)
;                    int16_t trackno, // 14(a0)
;                    int16_t sideno,  // 16(a0)
;                    int16_t count    // 18(a0)
;                  );

	; Build the ACSI command

	lea	acsi_cmd(pc),a2             ; a2 = ACSI command

	and.b	#$e0,(a0)                   ; Reset ACSI command
	cmp.w	#Flopwr,d1
	seq	d2                          ; Set bit if writing
	and.b	#$02,d2                     ; $02/$00
	or.b	#$08,d2                     ; $0a/$08
	or.b	d2,(a2)+                    ; Set ACSI command

	addq	#3,d0                       ; Set LUN
	lsl.b	#5,d0                       ;
	or.b	17(a0),d0                   ; Set side flag
	move.b	d0,(a2)+                    ;
	move.b	15(a0),(a2)+                ; Set track number
	move.b	13(a0),(a2)+                ; Set sector number
	move.b	19(a0),(a2)+                ; Set count

	move.l	2(a0),d0                    ; Set DMA address

	bsr.w	acsi_exec_cmd               ; Do the DMA operation

	move.w	d0,-(sp)                    ; Store d0
	move.w	(sp)+,d0                    ;

	ext.w	d0                          ;
	ext.l	d0                          ;

	rte

.end

; vim: ts=8 sw=8 sts=8 noet

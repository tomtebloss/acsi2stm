; Structs

; struct acsicmd: ACSI extended command
		rsreset
acsicmd.len	rs.b	1
acsicmd.cmdex	rs.b	1
acsicmd.cmd	rs.b	1
acsicmd.lun	rs.b	1
acsicmd.block	rs.l	1
acsicmd.res	rs.b	1
acsicmd.countw	rs.b	1
acsicmd.countb	rs.b	1
acsicmd.ctrl	rs.b	1
acsicmd...	rs.b	0

; struct bpb
		rsreset
bpb.data	rs.w	9               ; Opaque struct
bpb...		rs.b	0

; struct dinfo: Mounted drives info
		rsreset
finfo.dmask	rs.l	1               ; Mounted drives
finfo.boot	rs.w	1               ; Boot drive
finfo.fmask     rs.l    1               ; Mounted filesystems
finfo.dev	rs.w	1               ; Device number for id and bpb
finfo.id	rs.l	1               ; Queried drive unique id
finfo.bpb	rs.b	bpb...          ; Queried drive bpb
		rs.b	14              ; Padding
finfo...	rs.b	0

; struct globals: Global variables
; The loader will declare a label 'globals' that point to this struct
		rsreset
acsicmd		rs.b	acsicmd...
dmask		rs.l	1               ; Current drive mask
fmask		rs.l	1               ; Current filesystem mask
drvids		rs.l	26              ; Drive ids
oldmask		rs.l	1               ; BIOS drive mask
mchmask		rs.l	1               ; Media change mask
finfo		rs.b	finfo...        ; Filesystem info
bfloppy		rs.w	1               ; Set if drive B exists
nextqry		rs.l	1               ; Next hz200 at which finfo_query
			                ; will refresh its information
qrydrv		rs.w	1               ; Last queried drive
curdrv		rs.w	1               ; Current drive (used as a temporary)
traplen		rs.w	1               ; Trap instruction stack length
blkbuf...=512
blkbuf		rs.b	blkbuf...       ; A block buffer for temporary copy
globals...	rs.b	0               ; Total size of uninitialized RAM

; vim: ff=dos ts=8 sw=8 sts=8 noet

; Structs for mounted filesystem protocol

; struct dta
		rsreset
dta.drive	rs.b	1               ; Drive letter
dta.opaque	rs.b	20              ;
dta.fields      rs.b	27              ;
dta...		rs.b	0

; struct FsfirstW
		rsreset
FsfirstW.path...=256
FsfirstW.path	rs.b	FsfirstW.path...
FsfirstW.attr	rs.w	1
FsfirstW...	rs.b	0

; vim: ff=dos ts=8 sw=8 sts=8 noet

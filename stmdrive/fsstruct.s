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

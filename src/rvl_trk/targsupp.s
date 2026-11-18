.include "macros.inc"

# MetroTRK targsupp: an assembled file in the original. Each routine traps into the
# debugger, which does the file work. Retail starts it on a 16 byte boundary
# (a 4 byte gap after targimpl), and keeps all four even though open and position
# are never called, as the linker does not strip pieces of an assembled object.

.text
.balign 16

.fn TRKAccessFile, global
	twui r0, 0x0
	blr
.endfn TRKAccessFile

.fn TRKOpenFile, global
	twui r0, 0x0
	blr
.endfn TRKOpenFile

.fn TRKCloseFile, global
	twui r0, 0x0
	blr
.endfn TRKCloseFile

.fn TRKPositionFile, global
	twui r0, 0x0
	blr
.endfn TRKPositionFile

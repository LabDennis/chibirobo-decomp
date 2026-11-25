	.text
	.balign 16

	# The unconditional trap enters MetroTRK with the EABI arguments intact.
	.macro file_trap name
	.globl \name
	.type \name, @function
\name:
	twi 31, 0, 0
	blr
	.size \name, . - \name
	.endm

	file_trap TRKAccessFile
	file_trap TRKOpenFile
	file_trap TRKCloseFile
	file_trap TRKPositionFile

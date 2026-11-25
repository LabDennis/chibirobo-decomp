	.text
	.balign 4
	.globl GetR2__Fv
	.type GetR2__Fv, @function
GetR2__Fv:
	# Return the current EABI read-only small data base.
	mr 3, 2
	blr
	.size GetR2__Fv, . - GetR2__Fv

	.set SPR_WPAR, 921
	.set FPSCR_NI_BIT, 29

	.text
	.balign 4
	.globl PPCMfwpar
	.type PPCMfwpar, @function
PPCMfwpar:
	# Order preceding writes before testing the write-gather pipe.
	sync
	mfspr 3, SPR_WPAR
	blr
	.size PPCMfwpar, . - PPCMfwpar

	.globl PPCSetFpIEEEMode
	.type PPCSetFpIEEEMode, @function
PPCSetFpIEEEMode:
	mtfsb0 FPSCR_NI_BIT
	blr
	.size PPCSetFpIEEEMode, . - PPCSetFpIEEEMode

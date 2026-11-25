	.set FPSCR_NI_BIT, 29

	.text
	.balign 4
	.globl PPCSetFpNonIEEEMode
	.type PPCSetFpNonIEEEMode, @function
PPCSetFpNonIEEEMode:
	mtfsb1 FPSCR_NI_BIT
	blr
	.size PPCSetFpNonIEEEMode, . - PPCSetFpNonIEEEMode

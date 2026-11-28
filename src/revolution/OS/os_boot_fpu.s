	.set SPR_HID2, 920
	.set MSR_FP, 0x2000
	.set HID2_PSE_BIT, 2
	.set SPR_GQR0, 912

	.text
	.balign 4
	.globl __OSFPRInit
	.type __OSFPRInit, @function
__OSFPRInit:
	mfmsr 3
	ori 3, 3, MSR_FP
	mtmsr 3

	# Initialize both lanes when paired-single instructions are enabled.
	mfspr 3, SPR_HID2
	rlwinm. 3, 3, HID2_PSE_BIT + 1, 31, 31
	beq .Lscalar
	lis 3, ZeroPS@ha
	addi 3, 3, ZeroPS@l
	psq_l 0, 0(3), 0, 0
	.irp reg, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	ps_mr \reg, 0
	.endr

.Lscalar:
	lfd 0, ZeroF@sda21(0)
	.irp reg, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	fmr \reg, 0
	.endr

	mtfsf 255, 0
	blr
	.size __OSFPRInit, . - __OSFPRInit

	.globl __OSResetGQRs
	.type __OSResetGQRs, @function
__OSResetGQRs:
	li 3, 0
	.irp gqr, 0,1,2,3,4,5,6,7
	mtspr SPR_GQR0 + \gqr, 3
	.endr
	blr
	.size __OSResetGQRs, . - __OSResetGQRs

	.set HID0, 1008
	.set L2CR, 1017
	.set MMCR0, 952
	.set MMCR1, 956
	.set PMC1, 953
	.set PMC2, 954
	.set PMC3, 957
	.set PMC4, 958
	.set HID2, 920
	.set WPAR, 921

	.macro read_spr name, number
	.globl \name
	.type \name, @function
\name:
	mfspr 3, \number
	blr
	.size \name, . - \name
	.endm

	.macro write_spr name, number
	.globl \name
	.type \name, @function
\name:
	mtspr \number, 3
	blr
	.size \name, . - \name
	.endm

	.text
	.balign 4
	.globl PPCMfmsr
	.type PPCMfmsr, @function
PPCMfmsr:
	mfmsr 3
	blr
	.size PPCMfmsr, . - PPCMfmsr

	.globl PPCMtmsr
	.type PPCMtmsr, @function
PPCMtmsr:
	mtmsr 3
	blr
	.size PPCMtmsr, . - PPCMtmsr

	read_spr PPCMfhid0, HID0
	write_spr PPCMthid0, HID0
	read_spr PPCMfl2cr, L2CR
	write_spr PPCMtl2cr, L2CR

	.weak PPCMtdec
	.type PPCMtdec, @function
PPCMtdec:
	mtdec 3
	blr
	.size PPCMtdec, . - PPCMtdec

	.globl PPCSync
	.type PPCSync, @function
PPCSync:
	sc
	blr
	.size PPCSync, . - PPCSync

	.weak PPCHalt
	.type PPCHalt, @function
PPCHalt:
	sync
.Lhalt:
	nop
	li 3, 0
	nop
	b .Lhalt
	.size PPCHalt, . - PPCHalt

	write_spr PPCMtmmcr0, MMCR0
	write_spr PPCMtmmcr1, MMCR1
	write_spr PPCMtpmc1, PMC1
	write_spr PPCMtpmc2, PMC2
	write_spr PPCMtpmc3, PMC3
	write_spr PPCMtpmc4, PMC4

	.globl PPCMffpscr
	.type PPCMffpscr, @function
PPCMffpscr:
	stwu 1, -0x20(1)
	stfd 31, 0x18(1)

	# Read the FPSCR from the low word of the saved FPR value.
	mffs 31
	stfd 31, 8(1)
	lfd 31, 0x18(1)
	lwz 3, 0xc(1)

	addi 1, 1, 0x20
	blr
	.size PPCMffpscr, . - PPCMffpscr

	.globl PPCMtfpscr
	.type PPCMtfpscr, @function
PPCMtfpscr:
	stwu 1, -0x20(1)
	stfd 31, 0x18(1)

	# Load the caller's word into an FPR and update all FPSCR fields.
	li 4, 0
	stw 3, 0xc(1)
	stw 4, 8(1)
	lfd 31, 8(1)
	mtfsf 255, 31

	lfd 31, 0x18(1)
	addi 1, 1, 0x20
	blr
	.size PPCMtfpscr, . - PPCMtfpscr

	read_spr PPCMfhid2, HID2
	write_spr PPCMthid2, HID2
	write_spr PPCMtwpar, WPAR

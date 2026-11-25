	.include "sdk/private/trk/state.inc"

	.set MSR_EE, 0x8000
	.set SPR_IABR, 1010
	.set SPR_DABR, 1013
	.set HARDWARE_BBA, 2

	.macro restore_trk_gprs
	# The legacy lm spelling allows r3 to be both the base and a destination.
	.machine push
	.machine "any"
	lm 0, TRK_CPU_GPR(3)
	.machine pop
	.endm

	.macro trk_boot name, bba=0
	.if \bba
	.weak \name
	.else
	.globl \name
	.endif
	.type \name, @function
\name:
	# Save the interrupted context with the caller's original r1 and r3.
	addi 1, 1, -4
	stw 3, 0(1)
	lis 3, gTRKCPUState@h
	ori 3, 3, gTRKCPUState@l
	stmw 0, TRK_CPU_GPR(3)
	lwz 4, 0(1)
	addi 1, 1, 4
	stw 1, TRK_CPU_GPR + 4(3)
	stw 4, TRK_CPU_GPR + 12(3)
	mflr 4
	stw 4, TRK_CPU_LR(3)
	stw 4, TRK_CPU_PC(3)
	mfcr 4
	stw 4, TRK_CPU_CR(3)

	# Enable external interrupts for BBA and disable them for the serial transports.
	mfmsr 4
	ori 3, 4, MSR_EE
	.if !\bba
	xori 3, 3, MSR_EE
	.endif
	mtmsr 3
	mtsrr1 4
	bl TRKSaveExtended1Block

	lis 3, gTRKCPUState@h
	ori 3, 3, gTRKCPUState@l
	restore_trk_gprs
	li 0, 0
	mtspr SPR_IABR, 0
	mtspr SPR_DABR, 0
	lis 1, _db_stack_addr@h
	ori 1, 1, _db_stack_addr@l

	.if \bba
	li 3, HARDWARE_BBA
	.else
	mr 3, 5
	.endif
	bl InitMetroTRKCommTable
	cmpwi 3, 1
	bne .Lstart_nub\@

	# The retail failure path uses the status value (1) as the saved context base.
	lwz 4, TRK_CPU_LR(3)
	mtlr 4
	restore_trk_gprs
	blr

.Lstart_nub\@:
	b TRK_main
	# Keep the return instruction at the end of the original debugger entry.
	blr
	.size \name, . - \name
	.endm

	.text
	.balign 4
	trk_boot InitMetroTRK
	trk_boot InitMetroTRK_BBA, bba=1

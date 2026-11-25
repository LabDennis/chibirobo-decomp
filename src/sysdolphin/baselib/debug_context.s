	.include "sdk/private/os/exception_context.inc"

	.set SPR_SPRG0, 272

	.text
	.balign 4

	.globl HSD_SaveContext
	.type HSD_SaveContext, @function
HSD_SaveContext:
	# Preserve the caller's r3 in SPRG0 while r3 holds the context pointer.
	mtspr SPR_SPRG0, 3
	lis 3, hsd_saved_context@ha
	addi 3, 3, hsd_saved_context@l
	stmw 0, CONTEXT_R0(3)
	mfspr 4, SPR_SPRG0
	stw 4, CONTEXT_R3(3)

	.irp index,0,1,2,3,4,5,6,7
	mfspr 4, SPR_GQR0 + \index
	stw 4, CONTEXT_GQR0 + 4 * \index(3)
	.endr

	mfcr 4
	stw 4, CONTEXT_CR(3)
	mflr 4
	stw 4, CONTEXT_LR(3)
	mfctr 4
	stw 4, CONTEXT_CTR(3)
	mfxer 4
	stw 4, CONTEXT_XER(3)
	mfsrr0 4
	stw 4, CONTEXT_SRR0(3)
	mfsrr1 4
	stw 4, CONTEXT_SRR1(3)

	lhz 4, CONTEXT_STATE(3)
	ori 4, 4, CONTEXT_STATE_FPSAVED
	sth 4, CONTEXT_STATE(3)
	# Keep the caller's link register for the FPU snapshot's return.
	b OSFillFPUContext
	.size HSD_SaveContext, . - HSD_SaveContext

	# Byte offsets in OSContext.
	.set CONTEXT_R0, 0
	.set CONTEXT_R1, 4
	.set CONTEXT_R2, 8
	.set CONTEXT_R3, 12
	.set CONTEXT_R4, 16
	.set CONTEXT_R5, 20
	.set CONTEXT_R6, 24
	.set CONTEXT_R13, 52
	.set CONTEXT_CR, 128
	.set CONTEXT_LR, 132
	.set CONTEXT_CTR, 136
	.set CONTEXT_XER, 140
	.set CONTEXT_FPR0, 144
	.set CONTEXT_FPSCR, 400
	.set CONTEXT_SRR0, 408
	.set CONTEXT_SRR1, 412
	.set CONTEXT_STATE, 418
	.set CONTEXT_STATE_FPSAVED, 1
	.set CONTEXT_STATE_EXC, 2
	.set CONTEXT_STATE_EXC_BIT, 30
	.set CONTEXT_GQR0, 420
	.set CONTEXT_PSF0, 456
	.set SPR_GQR0, 912

	# Context pointers in the low-memory OS area.
	.set CURRENT_CONTEXT_PHYSICAL, 0xc0
	.set CURRENT_CONTEXT_CACHED, 0xd4
	.set FPU_CONTEXT_CACHED, 0xd8

	# GQR0 stays zero.
	.macro save_context_gqrs context, scratch
	.irp gqr, 1,2,3,4,5,6,7
	mfspr \scratch, SPR_GQR0 + \gqr
	stw \scratch, CONTEXT_GQR0 + 4 * \gqr(\context)
	.endr
	.endm

	# The first-level vector has already saved r3 through r5.
	.macro save_exception_registers context
	stw 0, CONTEXT_R0(\context)
	stw 1, CONTEXT_R1(\context)
	stw 2, CONTEXT_R2(\context)
	stmw 6, CONTEXT_R6(\context)
	save_context_gqrs \context, 0
	.endm

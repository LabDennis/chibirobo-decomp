	.include "sdk/private/os/exception_context.inc"

	.set CONTEXT_R31, CONTEXT_R0 + 31 * 4
	.set MSR_EE_BIT, 16
	.set MSR_RI_BIT, 30

	.text
	.balign 4
	.globl TRKLoadContext
	.type TRKLoadContext, @function
TRKLoadContext:
	lwz 0, CONTEXT_R0(3)
	lwz 1, CONTEXT_R1(3)
	lwz 2, CONTEXT_R2(3)

	# Exception contexts have saved volatile registers as well.
	lhz 5, CONTEXT_STATE(3)
	rlwinm. 6, 5, 0, CONTEXT_STATE_EXC_BIT, CONTEXT_STATE_EXC_BIT
	beq .Lrestore_nonvolatile
	rlwinm 5, 5, 0, CONTEXT_STATE_EXC_BIT + 1, CONTEXT_STATE_EXC_BIT - 1
	sth 5, CONTEXT_STATE(3)
	lmw 5, CONTEXT_R5(3)
	b .Lrestore_special

.Lrestore_nonvolatile:
	lmw 13, CONTEXT_R13(3)

.Lrestore_special:
	mr 31, 3
	mr 3, 4
	lwz 4, CONTEXT_CR(31)
	mtcr 4
	lwz 4, CONTEXT_LR(31)
	mtlr 4
	lwz 4, CONTEXT_CTR(31)
	mtctr 4
	lwz 4, CONTEXT_XER(31)
	mtxer 4
	mfmsr 4
	rlwinm 4, 4, 0, MSR_EE_BIT + 1, MSR_EE_BIT - 1
	rlwinm 4, 4, 0, MSR_RI_BIT + 1, MSR_RI_BIT - 1
	mtmsr 4

	# TRKInterruptHandler takes the exception ID in r3 and SRR0/SRR1
	# in r2/r4. Preserve interrupted r2 through r4 in SPRG1 through SPRG3.
	mtsprg 1, 2
	lwz 4, CONTEXT_R3(31)
	mtsprg 2, 4
	lwz 4, CONTEXT_R4(31)
	mtsprg 3, 4
	lwz 2, CONTEXT_SRR0(31)
	lwz 4, CONTEXT_SRR1(31)
	lwz 31, CONTEXT_R31(31)
	b TRKInterruptHandler
	.size TRKLoadContext, . - TRKLoadContext

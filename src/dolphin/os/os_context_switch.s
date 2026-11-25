	.include "sdk/private/os/exception_context.inc"

	.set CACHED_RAM_BASE_UPPER, 0x8000
	.set MSR_FP, 0x2000
	.set MSR_RI, 2
	.set MSR_FP_BIT, 18
	.set MSR_EE_BIT, 16
	.set MSR_RI_BIT, 30

	.text
	.balign 4
	.globl OSSetCurrentContext
	.type OSSetCurrentContext, @function
OSSetCurrentContext:
	lis 4, CACHED_RAM_BASE_UPPER
	stw 3, CURRENT_CONTEXT_CACHED(4)
	clrlwi 5, 3, 2
	stw 5, CURRENT_CONTEXT_PHYSICAL(4)

	lwz 5, FPU_CONTEXT_CACHED(4)
	cmpw 5, 3
	bne .Ldisable_fpu

	lwz 6, CONTEXT_SRR1(3)
	ori 6, 6, MSR_FP
	stw 6, CONTEXT_SRR1(3)
	mfmsr 6
	ori 6, 6, MSR_RI
	mtmsr 6
	blr

.Ldisable_fpu:
	lwz 6, CONTEXT_SRR1(3)
	rlwinm 6, 6, 0, MSR_FP_BIT + 1, MSR_FP_BIT - 1
	stw 6, CONTEXT_SRR1(3)
	mfmsr 6
	rlwinm 6, 6, 0, MSR_FP_BIT + 1, MSR_FP_BIT - 1
	ori 6, 6, MSR_RI
	mtmsr 6
	isync
	blr
	.size OSSetCurrentContext, . - OSSetCurrentContext

	.globl OSSaveContext
	.type OSSaveContext, @function
OSSaveContext:
	stmw 13, CONTEXT_R13(3)
	save_context_gqrs 3, 0
	mfcr 0
	stw 0, CONTEXT_CR(3)
	mflr 0
	stw 0, CONTEXT_LR(3)
	stw 0, CONTEXT_SRR0(3)
	mfmsr 0
	stw 0, CONTEXT_SRR1(3)
	mfctr 0
	stw 0, CONTEXT_CTR(3)
	mfxer 0
	stw 0, CONTEXT_XER(3)
	stw 1, CONTEXT_R1(3)
	stw 2, CONTEXT_R2(3)
	# Return zero now and one when OSLoadContext resumes the caller.
	li 0, 1
	stw 0, CONTEXT_R3(3)
	li 3, 0
	blr
	.size OSSaveContext, . - OSSaveContext

	.globl OSLoadContext
	.type OSLoadContext, @function
OSLoadContext:
	# Restart interrupt masking if the exception occurred inside its
	# restartable instruction range.
	lis 4, __RAS_OSDisableInterrupts_begin@ha
	lwz 6, CONTEXT_SRR0(3)
	addi 5, 4, __RAS_OSDisableInterrupts_begin@l
	cmplw 6, 5
	ble .Lrestore_registers
	lis 4, __RAS_OSDisableInterrupts_end@ha
	addi 0, 4, __RAS_OSDisableInterrupts_end@l
	cmplw 6, 0
	bge .Lrestore_registers
	stw 5, CONTEXT_SRR0(3)

.Lrestore_registers:
	lwz 0, CONTEXT_R0(3)
	lwz 1, CONTEXT_R1(3)
	lwz 2, CONTEXT_R2(3)

	# Exception contexts hold volatile registers as well.
	lhz 4, CONTEXT_STATE(3)
	rlwinm. 5, 4, 0, CONTEXT_STATE_EXC_BIT, CONTEXT_STATE_EXC_BIT
	beq .Lrestore_nonvolatile
	rlwinm 4, 4, 0, CONTEXT_STATE_EXC_BIT + 1, CONTEXT_STATE_EXC_BIT - 1
	sth 4, CONTEXT_STATE(3)
	lmw 5, CONTEXT_R5(3)
	b .Lrestore_special

.Lrestore_nonvolatile:
	lmw 13, CONTEXT_R13(3)

.Lrestore_special:
	.irp gqr, 1,2,3,4,5,6,7
	lwz 4, CONTEXT_GQR0 + 4 * \gqr(3)
	mtspr SPR_GQR0 + \gqr, 4
	.endr

	lwz 4, CONTEXT_CR(3)
	mtcr 4
	lwz 4, CONTEXT_LR(3)
	mtlr 4
	lwz 4, CONTEXT_CTR(3)
	mtctr 4
	lwz 4, CONTEXT_XER(3)
	mtxer 4

	mfmsr 4
	rlwinm 4, 4, 0, MSR_EE_BIT + 1, MSR_EE_BIT - 1
	rlwinm 4, 4, 0, MSR_RI_BIT + 1, MSR_RI_BIT - 1
	mtmsr 4

	lwz 4, CONTEXT_SRR0(3)
	mtsrr0 4
	lwz 4, CONTEXT_SRR1(3)
	mtsrr1 4
	lwz 4, CONTEXT_R4(3)
	lwz 3, CONTEXT_R3(3)
	rfi
	.size OSLoadContext, . - OSLoadContext

	.globl OSGetStackPointer
	.type OSGetStackPointer, @function
OSGetStackPointer:
	mr 3, 1
	blr
	.size OSGetStackPointer, . - OSGetStackPointer

	.globl OSSwitchStack
	.type OSSwitchStack, @function
OSSwitchStack:
	mr 5, 1
	mr 1, 3
	mr 3, 5
	blr
	.size OSSwitchStack, . - OSSwitchStack

	.globl OSSwitchFiber
	.type OSSwitchFiber, @function
OSSwitchFiber:
	mflr 0
	mr 5, 1
	stwu 5, -8(4)
	mr 1, 4
	stw 0, 4(5)
	mtlr 3
	blrl
	lwz 5, 0(1)
	lwz 0, 4(5)
	mtlr 0
	mr 1, 5
	blr
	.size OSSwitchFiber, . - OSSwitchFiber

	.globl __OSSaveContextSDA
	.type __OSSaveContextSDA, @function
__OSSaveContextSDA:
	stw 2, CONTEXT_R2(3)
	stw 13, CONTEXT_R13(3)
	blr
	.size __OSSaveContextSDA, . - __OSSaveContextSDA

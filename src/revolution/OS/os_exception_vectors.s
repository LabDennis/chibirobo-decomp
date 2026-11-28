	.include "sdk/private/os/exception_context.inc"

	.set DEBUG_INTERFACE, 0x40
	.set DEBUG_RETURN, 0x0c
	.set DEBUG_HANDLER, 0x08
	.set DEBUG_ENTRY, 0x60
	.set EXCEPTION_TABLE, 0x3000
	.set MSR_TRANSLATION, 0x30
	.set MSR_RI_BIT, 30

	.text
	.balign 4
	.globl __OSDBIntegrator, __OSDBINTSTART, __OSDBINTEND
__OSDBIntegrator:
__OSDBINTSTART:
	# The debugger returns to the instruction after the patched jump.
	li 5, DEBUG_INTERFACE
	mflr 3
	stw 3, DEBUG_RETURN(5)
	lwz 3, DEBUG_HANDLER(5)
	oris 3, 3, 0x8000
	mtlr 3
	li 3, MSR_TRANSLATION
	mtmsr 3
	blr
__OSDBINTEND:

	.globl __OSDBJUMPSTART, __OSDBJUMPEND
__OSDBJump:
__OSDBJUMPSTART:
	bl DEBUG_ENTRY
__OSDBJUMPEND:

	.globl OSExceptionVector, __OSEVStart, __OSEVEnd, __DBVECTOR, __OSEVSetNumber
OSExceptionVector:
__OSEVStart:
	# The CPU enters the vector with address translation disabled.
	mtsprg 0, 4
	lwz 4, CURRENT_CONTEXT_PHYSICAL(0)
	stw 3, CONTEXT_R3(4)
	mfsprg 3, 0
	stw 3, CONTEXT_R4(4)
	stw 5, CONTEXT_R5(4)

	lhz 3, CONTEXT_STATE(4)
	ori 3, 3, CONTEXT_STATE_EXC
	sth 3, CONTEXT_STATE(4)

	mfcr 3
	stw 3, CONTEXT_CR(4)
	mflr 3
	stw 3, CONTEXT_LR(4)
	mfctr 3
	stw 3, CONTEXT_CTR(4)
	mfxer 3
	stw 3, CONTEXT_XER(4)
	mfsrr0 3
	stw 3, CONTEXT_SRR0(4)
	mfsrr1 3
	stw 3, CONTEXT_SRR1(4)
	mr 5, 3

__DBVECTOR:
	nop

	mfmsr 3
	ori 3, 3, MSR_TRANSLATION
	mtsrr1 3

__OSEVSetNumber:
	li 3, 0
	lwz 4, CURRENT_CONTEXT_CACHED(0)

	# Dispatch non-recoverable exceptions directly to the default handler.
	rlwinm. 5, 5, 0, MSR_RI_BIT, MSR_RI_BIT
	bne .Lrecoverable
	lis 5, OSDefaultExceptionHandler@ha
	addi 5, 5, OSDefaultExceptionHandler@l
	mtsrr0 5
	rfi

.Lrecoverable:
	rlwinm 5, 3, 2, 22, 29
	lwz 5, EXCEPTION_TABLE(5)
	mtsrr0 5
	rfi

__OSEVEnd:
	nop

	.globl OSDefaultExceptionHandler
	.type OSDefaultExceptionHandler, @function
OSDefaultExceptionHandler:
	save_exception_registers 4
	mfdsisr 5
	mfdar 6
	stwu 1, -8(1)
	b __OSUnhandledException
	.size OSDefaultExceptionHandler, . - OSDefaultExceptionHandler

	.globl DecrementerExceptionHandler
	.type DecrementerExceptionHandler, @function
DecrementerExceptionHandler:
	save_exception_registers 4
	stwu 1, -8(1)
	b DecrementerExceptionCallback
	.size DecrementerExceptionHandler, . - DecrementerExceptionHandler

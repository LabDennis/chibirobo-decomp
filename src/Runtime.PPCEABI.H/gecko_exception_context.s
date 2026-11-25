	# ThrowContext layout, checked in Gecko_ExceptionPPC.cp.
	.set FPR_SIZE, 0x10
	.set FPR_PAIRED_SINGLE, 0x8
	.set CONTEXT_GPR13, 0x234
	.set CONTEXT_CR, 0x280
	.set CONTEXT_SP, 0x284
	.set CONTEXT_THROW_SP, 0x28c
	.set CONTEXT_RETURN_ADDRESS, 0x290
	.set CONTEXT_THROW_TYPE, 0x294
	.set CONTEXT_LOCATION, 0x298
	.set CONTEXT_DTOR, 0x29c
	.set CONTEXT_SIZE, 0x2a8

	.set FRAME_BACK_CHAIN, 0
	.set FRAME_SAVED_LR, 4
	.set FRAME_THROW_TYPE, 8
	.set FRAME_LOCATION, 12
	.set FRAME_DTOR, 16
	.set FRAME_CONTEXT, 0x18
	.set FRAME_SIZE, FRAME_CONTEXT + CONTEXT_SIZE

	.text
	.balign 4
	.globl ExPPC_LongJump__FP12ThrowContextPvPv
	.type ExPPC_LongJump__FP12ThrowContextPvPv, @function
ExPPC_LongJump__FP12ThrowContextPvPv:
	# r3 holds the saved context, r4 the target TOC, and r5 the landing pad.
	mr 8, 5
	mr 2, 4
	lwz 0, CONTEXT_CR(3)
	mtcr 0
	lmw 13, CONTEXT_GPR13(3)

	# Restore the paired-single state before restoring the scalar double.
	.irp fpr, 14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	addi 7, 3, FPR_SIZE * \fpr + FPR_PAIRED_SINGLE
	psq_lx \fpr, 0, 7, 0, 0
	lfd \fpr, FPR_SIZE * \fpr(3)
	.endr

	mtlr 8
	# Keep the throw site's outgoing stack area and splice in the unwound back chain.
	lwz 1, CONTEXT_THROW_SP(3)
	lwz 3, CONTEXT_SP(3)
	lwz 3, FRAME_BACK_CHAIN(3)
	stw 3, FRAME_BACK_CHAIN(1)
	blr
	.size ExPPC_LongJump__FP12ThrowContextPvPv, . - ExPPC_LongJump__FP12ThrowContextPvPv

	.globl __throw
	.type __throw, @function
__throw:
	stwu 1, -FRAME_SIZE(1)
	mflr 0
	stw 0, FRAME_SIZE + FRAME_SAVED_LR(1)
	stw 3, FRAME_THROW_TYPE(1)
	stw 4, FRAME_LOCATION(1)
	stw 5, FRAME_DTOR(1)

	# Capture the caller's nonvolatile registers for the C++ unwinder.
	stmw 13, FRAME_CONTEXT + CONTEXT_GPR13(1)
	.irp fpr, 14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	stfd \fpr, FRAME_CONTEXT + FPR_SIZE * \fpr(1)
	addi 3, 1, FRAME_CONTEXT + FPR_SIZE * \fpr + FPR_PAIRED_SINGLE
	psq_stx \fpr, 0, 3, 0, 0
	.endr
	mfcr 3
	stw 3, FRAME_CONTEXT + CONTEXT_CR(1)

	# Begin unwinding at the function that called __throw.
	lwz 3, FRAME_BACK_CHAIN(1)
	lwz 4, FRAME_SAVED_LR(3)
	stw 3, FRAME_CONTEXT + CONTEXT_SP(1)
	stw 3, FRAME_CONTEXT + CONTEXT_THROW_SP(1)
	stw 4, FRAME_CONTEXT + CONTEXT_RETURN_ADDRESS(1)

	lwz 3, FRAME_THROW_TYPE(1)
	stw 3, FRAME_CONTEXT + CONTEXT_THROW_TYPE(1)
	lwz 3, FRAME_LOCATION(1)
	stw 3, FRAME_CONTEXT + CONTEXT_LOCATION(1)
	lwz 3, FRAME_DTOR(1)
	stw 3, FRAME_CONTEXT + CONTEXT_DTOR(1)
	addi 3, 1, FRAME_CONTEXT
	bl ExPPC_ThrowHandler__FP12ThrowContext
	nop

	lwz 0, FRAME_SIZE + FRAME_SAVED_LR(1)
	mtlr 0
	addi 1, 1, FRAME_SIZE
	blr
	.size __throw, . - __throw

	.include "sdk/private/os/exception_context.inc"

	.set CACHED_RAM_BASE_UPPER, 0x8000
	.set MSR_FP, 0x2000
	.set SPR_HID2, 920
	.set HID2_PSE_BIT, 2

	# The FPU context helpers are assembly in the Revolution SDK source;
	# wii-ipl and tp keep the same instruction streams. R24J01 does not
	# contain OSLoadFPUContext, OSFillFPUContext or OSSwitchStack.

	.macro transfer_fprs operation, context
	.irp reg, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	\operation \reg, CONTEXT_FPR0 + 8 * \reg(\context)
	.endr
	.endm

	# GQR0 selects an unscaled pair of floats.
	.macro transfer_paired_singles operation, context
	.irp reg, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	\operation \reg, CONTEXT_PSF0 + 8 * \reg(\context), 0, 0
	.endr
	.endm

	.macro save_fprs_and_status context
	transfer_fprs stfd, \context
	mffs 0
	stfd 0, CONTEXT_FPSCR(\context)
	lfd 0, CONTEXT_FPR0(\context)
	.endm

	.text
	.balign 4
	.type __OSLoadFPUContext, @function
__OSLoadFPUContext:
	lhz 5, CONTEXT_STATE(4)
	clrlwi. 5, 5, 31
	beq .Lload_done

	lfd 0, CONTEXT_FPSCR(4)
	mtfsf 255, 0
	mfspr 5, SPR_HID2
	rlwinm. 5, 5, HID2_PSE_BIT + 1, 31, 31
	beq .Lload_scalar
	transfer_paired_singles psq_l, 4

.Lload_scalar:
	transfer_fprs lfd, 4

.Lload_done:
	blr
	.size __OSLoadFPUContext, . - __OSLoadFPUContext

	.type __OSSaveFPUContext, @function
__OSSaveFPUContext:
	lhz 3, CONTEXT_STATE(5)
	ori 3, 3, CONTEXT_STATE_FPSAVED
	sth 3, CONTEXT_STATE(5)

	save_fprs_and_status 5
	mfspr 3, SPR_HID2
	rlwinm. 3, 3, HID2_PSE_BIT + 1, 31, 31
	beq .Lsave_done
	transfer_paired_singles psq_st, 5

.Lsave_done:
	blr
	.size __OSSaveFPUContext, . - __OSSaveFPUContext

	.globl OSSaveFPUContext
	.type OSSaveFPUContext, @function
OSSaveFPUContext:
	addi 5, 3, 0
	b __OSSaveFPUContext
	.size OSSaveFPUContext, . - OSSaveFPUContext

	.globl OSSwitchFPUContext
	.type OSSwitchFPUContext, @function
OSSwitchFPUContext:
	mfmsr 5
	ori 5, 5, MSR_FP
	mtmsr 5
	isync
	lwz 5, CONTEXT_SRR1(4)
	ori 5, 5, MSR_FP
	mtsrr1 5

	# Save the previous FPU owner's state before restoring the new owner's.
	lis 3, CACHED_RAM_BASE_UPPER
	lwz 5, FPU_CONTEXT_CACHED(3)
	stw 4, FPU_CONTEXT_CACHED(3)
	cmpw 5, 4
	beq .Lreturn_from_exception
	cmpwi 5, 0
	beq .Lload_fpu_owner
	bl __OSSaveFPUContext

.Lload_fpu_owner:
	bl __OSLoadFPUContext

.Lreturn_from_exception:
	lwz 3, CONTEXT_CR(4)
	mtcr 3
	lwz 3, CONTEXT_LR(4)
	mtlr 3
	lwz 3, CONTEXT_SRR0(4)
	mtsrr0 3
	lwz 3, CONTEXT_CTR(4)
	mtctr 3
	lwz 3, CONTEXT_XER(4)
	mtxer 3
	lhz 3, CONTEXT_STATE(4)
	rlwinm 3, 3, 0, CONTEXT_STATE_EXC_BIT + 1, CONTEXT_STATE_EXC_BIT - 1
	sth 3, CONTEXT_STATE(4)
	lwz 5, CONTEXT_R5(4)
	lwz 3, CONTEXT_R3(4)
	lwz 4, CONTEXT_R4(4)
	rfi
	.size OSSwitchFPUContext, . - OSSwitchFPUContext

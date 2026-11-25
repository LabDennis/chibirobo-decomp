	.include "sdk/private/trk/state.inc"

	.set TRK_EXCEPTION_PC,       0x0
	.set TRK_EXCEPTION_VECTOR,   0x8
	.set TRK_EXCEPTION_IN_TRK,   0xc
	.set TRK_EXCEPTION_DETECTED, 0xd
	.set MSR_EE_RI,              0x8002
	.set PPC_EXTERNAL_INTERRUPT, 0x500

	.macro load_address reg, symbol
	lis \reg, \symbol@h
	ori \reg, \reg, \symbol@l
	.endm

	# Restore the monitor's register state and post the pending event.
	.macro restore_monitor
	load_address 2, gTRKState
	lwz 0, TRK_STATE_MSR(2)
	sync
	mtmsr 0
	sync
	lwz 0, TRK_STATE_LR(2)
	mtlr 0
	lwz 0, TRK_STATE_CTR(2)
	mtctr 0
	lwz 0, TRK_STATE_XER(2)
	mtxer 0
	lwz 0, TRK_STATE_DSISR(2)
	mtdsisr 0
	lwz 0, TRK_STATE_DAR(2)
	mtdar 0
	lmw 3, (TRK_STATE_GPR + 3 * 4)(2)
	lwz 0, TRK_STATE_GPR(2)
	lwz 1, (TRK_STATE_GPR + 1 * 4)(2)
	lwz 2, (TRK_STATE_GPR + 2 * 4)(2)
	b TRKPostInterruptEvent
	.endm

	.text
	.balign 4

	.globl __TRK_get_MSR
	.type __TRK_get_MSR, @function
__TRK_get_MSR:
	mfmsr 3
	blr
	.size __TRK_get_MSR, . - __TRK_get_MSR

	.globl __TRK_set_MSR
	.type __TRK_set_MSR, @function
__TRK_set_MSR:
	mtmsr 3
	blr
	.size __TRK_set_MSR, . - __TRK_set_MSR

	.globl TRK_ppc_memcpy
	.type TRK_ppc_memcpy, @function
TRK_ppc_memcpy:
	# r6 and r7 contain the destination and source MSRs. Each access uses
	# its corresponding translation state, with the caller's MSR restored on exit.
	mfmsr 8
	li 10, 0
.Lcopy_byte:
	cmpw 10, 5
	beq .Lcopy_done

	mtmsr 7
	sync
	lbzx 9, 10, 4

	mtmsr 6
	sync
	stbx 9, 10, 3

	addi 10, 10, 1
	b .Lcopy_byte
.Lcopy_done:
	mtmsr 8
	sync
	blr
	.size TRK_ppc_memcpy, . - TRK_ppc_memcpy

	.globl TRKInterruptHandler
	.type TRKInterruptHandler, @function
TRKInterruptHandler:
	mtsrr0 2
	mtsrr1 4
	mfsprg 4, 3
	mfcr 2
	mtsprg 3, 2
	load_address 2, gTRKState
	lwz 2, TRK_STATE_MSR(2)
	ori 2, 2, MSR_EE_RI
	xori 2, 2, MSR_EE_RI
	sync
	mtmsr 2
	sync
	load_address 2, TRK_saved_exceptionID
	sth 3, 0(2)
	cmpwi 3, PPC_EXTERNAL_INTERRUPT
	bne .Lrecord_exception

	# Let the transport service the external interrupt before stopping the target.
	load_address 2, gTRKCPUState
	mflr 3
	stw 3, TRK_CPU_TRANSPORT_RETURN(2)
	bl TRKUARTInterruptHandler
	load_address 2, gTRKCPUState
	lwz 3, TRK_CPU_TRANSPORT_RETURN(2)
	mtlr 3
	load_address 2, gTRKState
	lwz 2, TRK_STATE_INPUT_PENDING_PTR(2)
	lbz 2, 0(2)
	cmpwi 2, 0
	beq .Lreturn_to_target
	load_address 2, gTRKExceptionStatus
	lbz 2, TRK_EXCEPTION_IN_TRK(2)
	cmpwi 2, 1
	beq .Lreturn_to_target
	load_address 2, gTRKState
	li 3, 1
	# The C side tests this big-endian BOOL for nonzero.
	stb 3, TRK_STATE_INPUT_ACTIVATED(2)
	b .Lrecord_exception
.Lreturn_to_target:
	load_address 2, gTRKSaveState
	lwz 3, TRK_CPU_CR(2)
	mtcrf 0xff, 3
	lwz 3, (TRK_CPU_GPR + 3 * 4)(2)
	lwz 2, (TRK_CPU_GPR + 2 * 4)(2)
	rfi
.Lrecord_exception:
	load_address 2, TRK_saved_exceptionID
	lhz 3, 0(2)
	load_address 2, gTRKExceptionStatus
	lbz 2, TRK_EXCEPTION_IN_TRK(2)
	cmpwi 2, 0
	bne TRKExceptionHandler

	# Save the interrupted target and return to the monitor's saved CPU state.
	load_address 2, gTRKCPUState
	stw 0, TRK_CPU_GPR(2)
	stw 1, (TRK_CPU_GPR + 1 * 4)(2)
	mfsprg 0, 1
	stw 0, (TRK_CPU_GPR + 2 * 4)(2)
	sth 3, TRK_CPU_EXCEPTION_ID(2)
	sth 3, (TRK_CPU_EXCEPTION_ID + 2)(2)
	mfsprg 0, 2
	stw 0, (TRK_CPU_GPR + 3 * 4)(2)
	stmw 4, (TRK_CPU_GPR + 4 * 4)(2)
	mfsrr0 27
	mflr 28
	mfsprg 29, 3
	mfctr 30
	mfxer 31
	stmw 27, TRK_CPU_PC(2)
	bl TRKSaveExtended1Block
	load_address 2, gTRKExceptionStatus
	li 3, 1
	stb 3, TRK_EXCEPTION_IN_TRK(2)
	restore_monitor
	.size TRKInterruptHandler, . - TRKInterruptHandler

	.globl TRKExceptionHandler
	.type TRKExceptionHandler, @function
TRKExceptionHandler:
	# An exception inside the debugger records its cause and resumes the debugger.
	load_address 2, gTRKExceptionStatus
	sth 3, TRK_EXCEPTION_VECTOR(2)
	mfsrr0 3
	stw 3, TRK_EXCEPTION_PC(2)
	lhz 3, TRK_EXCEPTION_VECTOR(2)
	.irp vector, 0x200,0x300,0x400,0x600,0x700,0x800,0x1000,0x1100,0x1200,0x1300
	cmpwi 3, \vector
	beq .Lskip_faulting_instruction
	.endr
	b .Lfinish_internal_exception
.Lskip_faulting_instruction:
	mfsrr0 3
	addi 3, 3, 0x4
	mtsrr0 3
.Lfinish_internal_exception:
	load_address 2, gTRKExceptionStatus
	li 3, 0x1
	stb 3, TRK_EXCEPTION_DETECTED(2)
	mfsprg 3, 3
	mtcrf 0xff, 3
	mfsprg 2, 1
	mfsprg 3, 2
	rfi
	.size TRKExceptionHandler, . - TRKExceptionHandler

	.globl TRKSwapAndGo
	.type TRKSwapAndGo, @function
TRKSwapAndGo:
	load_address 3, gTRKState
	stmw 0, TRK_STATE_GPR(3)
	mfmsr 0
	stw 0, TRK_STATE_MSR(3)
	mflr 0
	stw 0, TRK_STATE_LR(3)
	mfctr 0
	stw 0, TRK_STATE_CTR(3)
	mfxer 0
	stw 0, TRK_STATE_XER(3)
	mfdsisr 0
	stw 0, TRK_STATE_DSISR(3)
	mfdar 0
	stw 0, TRK_STATE_DAR(3)
	li 1, MSR_EE_RI - 0x10000
	nor 1, 1, 1
	mfmsr 3
	and 3, 3, 1
	mtmsr 3
	load_address 2, gTRKState
	lwz 2, TRK_STATE_INPUT_PENDING_PTR(2)
	lbz 2, 0(2)
	cmpwi 2, 0
	beq .Lrestore_target
	load_address 2, gTRKState
	li 3, 1
	stb 3, TRK_STATE_INPUT_ACTIVATED(2)
	b TRKInterruptHandlerEnableInterrupts
.Lrestore_target:
	# Resume the target only when there is no transport input pending.
	load_address 2, gTRKExceptionStatus
	li 3, 0
	stb 3, TRK_EXCEPTION_IN_TRK(2)
	bl TRKRestoreExtended1Block
	load_address 2, gTRKCPUState
	lmw 27, TRK_CPU_PC(2)
	mtsrr0 27
	mtlr 28
	mtcrf 0xff, 29
	mtctr 30
	mtxer 31
	lmw 3, (TRK_CPU_GPR + 3 * 4)(2)
	lwz 0, TRK_CPU_GPR(2)
	lwz 1, (TRK_CPU_GPR + 1 * 4)(2)
	lwz 2, (TRK_CPU_GPR + 2 * 4)(2)
	rfi
	.size TRKSwapAndGo, . - TRKSwapAndGo

	.globl TRKInterruptHandlerEnableInterrupts
	.type TRKInterruptHandlerEnableInterrupts, @function
TRKInterruptHandlerEnableInterrupts:
	restore_monitor
	.size TRKInterruptHandlerEnableInterrupts, . - TRKInterruptHandlerEnableInterrupts

	# Preserve both halves of f31 across direct access to FPSCR.
	.macro access_fpscr write
	stwu 1, -0x40(1)
	stfd 31, 0x10(1)
	psq_st 31, 0x20(1), 0, 0
	.if \write
	lfd 31, 0(3)
	mtfsf 0xff, 31
	.else
	mffs 31
	stfd 31, 0(3)
	.endif
	psq_l 31, 0x20(1), 0, 0
	lfd 31, 0x10(1)
	addi 1, 1, 0x40
	blr
	.endm

	.globl ReadFPSCR
	.type ReadFPSCR, @function
ReadFPSCR:
	access_fpscr 0
	.size ReadFPSCR, . - ReadFPSCR

	.globl WriteFPSCR
	.type WriteFPSCR, @function
WriteFPSCR:
	access_fpscr 1
	.size WriteFPSCR, . - WriteFPSCR

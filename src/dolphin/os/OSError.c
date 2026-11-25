#include "types.h"
#include "Dolphin/os.h"
#include "stdio.h"
#include "Dolphin/PPCArch.h"
#include "Dolphin/hw_regs.h"

// Clear the exception status and fraction-inexact flags when setting or handling a trap.
#define FPSCR_EXCEPTION_STATUS                                                                      \
	(FPSCR_FX | FPSCR_OX | FPSCR_UX | FPSCR_ZX | FPSCR_XX | FPSCR_VXSNAN | FPSCR_VXISI | FPSCR_VXIDI\
	 | FPSCR_VXZDZ | FPSCR_VXIMZ | FPSCR_VXVC | FPSCR_FI | FPSCR_VXSOFT | FPSCR_VXSQRT | FPSCR_VXCVI)

OSErrorHandler __OSErrorTable[OS_ERROR_MAX];
u32 __OSFpscrEnableBits = FPSCR_ENABLE;

/**
 * @note Address: 0x8001580C
 * @note Size: 0x50
 */
WEAK void OSReport(const char* msg, ...)
{
	va_list marker;

	va_start(marker, msg);
	vprintf(msg, marker);
	va_end(marker);
}

/**
 * @note Address: N/A
 * @note Size: 0x20
 * same as OSReport but without va start/end
 */
WEAK void OSVReport(const char* msg, va_list list)
{
	vprintf(msg, list);
}

/**
 * @note Address: 0x80163F80
 * @note Size: 0x12C
 */
WEAK void OSPanic(const char* file, int line, const char* msg, ...)
{
	va_list marker;
	u32 i;
	u32* p;

	OSDisableInterrupts();
	va_start(marker, msg);
	vprintf(msg, marker);
	va_end(marker);
	OSReport(" in \"%s\" on line %d.\n", file, line);

	// actually useful for debugging- stack crawl
	OSReport("\nAddress:      Back Chain    LR Save\n");
	for (i = 0, p = (u32*)OSGetStackPointer();                // get current stack pointer
	     p && (u32)p != 0xffffffff && i++ < 16; p = (u32*)*p) // get caller stack pointer
	{
		OSReport("0x%08x:   0x%08x    0x%08x\n", p, p[0], p[1]);
	}

	PPCHalt();
}

/**
 * @note Address: 0x801640AC
 * @note Size: 0x218
 */
OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler)
{
	OSErrorHandler oldHandler;
	BOOL enabled;

	enabled               = OSDisableInterrupts();
	oldHandler            = __OSErrorTable[error];
	__OSErrorTable[error] = handler;

	if (error == OS_ERROR_FPE) {
		u32 msr;
		u32 fpscr;
		OSThread* thread;

		msr = PPCMfmsr();
		PPCMtmsr(msr | MSR_FP); // Enable FPU
		fpscr = PPCMffpscr();
		if (handler) {
			for (thread = __OSActiveThreadQueue.head; thread; thread = thread->linkActive.next) {
				thread->context.srr1 |= (MSR_FE0 | MSR_FE1);
				if ((thread->context.state & OS_CONTEXT_STATE_FPSAVED) == 0) {
					int i;
					const u64 initialFPR = ~0ULL;

					// Fill the saved register slots with the all-ones NaN bit pattern.
					thread->context.state |= OS_CONTEXT_STATE_FPSAVED;
					for (i = 0; i < 32; ++i) {
						*(u64*)&thread->context.fpr[i] = initialFPR;
						*(u64*)&thread->context.psf[i] = initialFPR;
					}
					thread->context.fpscr = FPSCR_NI;
				}
				thread->context.fpscr |= __OSFpscrEnableBits & FPSCR_ENABLE;
				thread->context.fpscr &= ~FPSCR_EXCEPTION_STATUS;
			}
			fpscr |= __OSFpscrEnableBits & FPSCR_ENABLE;
			msr |= (MSR_FE0 | MSR_FE1);
		} else {
			for (thread = __OSActiveThreadQueue.head; thread; thread = thread->linkActive.next) {
				thread->context.srr1 &= ~(MSR_FE0 | MSR_FE1);
				thread->context.fpscr &= ~FPSCR_ENABLE;
				thread->context.fpscr &= ~FPSCR_EXCEPTION_STATUS;
			}
			fpscr &= ~FPSCR_ENABLE;
			msr &= ~(MSR_FE0 | MSR_FE1);
		}

		fpscr &= ~FPSCR_EXCEPTION_STATUS;

		PPCMtfpscr(fpscr);
		PPCMtmsr(msr); // Restore FPU
	}

	OSRestoreInterrupts(enabled);
	return oldHandler;
}

/**
 * @note Address: 0x801642C4
 * @note Size: 0x2E8
 */
void __OSUnhandledException(__OSException exception, OSContext* context, u32 dsisr, u32 dar)
{
	OSTime now;

	now = OSGetTime();

	if (!(context->srr1 & MSR_RI)) {
		OSReport("Non-recoverable Exception %d", exception);

	} else {
		if (exception == __OS_EXCEPTION_PROGRAM && (context->srr1 & SRR1_PROG_FPE) && __OSErrorTable[OS_ERROR_FPE] != 0) {
			u32 fpscr;
			u32 msr;

			exception = OS_ERROR_FPE;

			msr = PPCMfmsr();
			PPCMtmsr(msr | MSR_FP);

			if (__OSFPUContext) {
				OSSaveFPUContext((OSContext*)__OSFPUContext);
			}

			fpscr = PPCMffpscr();
			fpscr &= ~FPSCR_EXCEPTION_STATUS;
			PPCMtfpscr(fpscr);

			PPCMtmsr(msr);

			if (__OSFPUContext == context) {
				OSDisableScheduler();
				__OSErrorTable[exception](exception, context, dsisr, dar);
				context->srr1 &= ~MSR_FP;
				__OSFPUContext = nullptr;

				context->fpscr &= ~FPSCR_EXCEPTION_STATUS;
				OSEnableScheduler();
				__OSReschedule();
			} else {
				context->srr1 &= ~MSR_FP;
				__OSFPUContext = nullptr;
			}

			OSLoadContext(context);
		}

		if (__OSErrorTable[exception]) {
			OSDisableScheduler();
			__OSErrorTable[exception](exception, context, dsisr, dar);
			OSEnableScheduler();
			__OSReschedule();
			OSLoadContext(context);
		}

		if (exception == OS_ERROR_DECREMENTER) {
			OSLoadContext(context);
		}

		OSReport("Unhandled Exception %d", exception);
	}

	OSReport("\n");
	OSDumpContext(context);
	OSReport("\nDSISR = 0x%08x                   DAR  = 0x%08x\n", dsisr, dar);
	OSReport("TB = 0x%016llx\n", now);

	switch (exception) {
	case __OS_EXCEPTION_DSI:
		OSReport("\nInstruction at 0x%x (read from SRR0) attempted to access "
		         "invalid address 0x%x (read from DAR)\n",
		         context->srr0, dar);
		break;
	case __OS_EXCEPTION_ISI:
		OSReport("\nAttempted to fetch instruction from invalid address 0x%x "
		         "(read from SRR0)\n",
		         context->srr0);
		break;
	case __OS_EXCEPTION_ALIGNMENT:
		OSReport("\nInstruction at 0x%x (read from SRR0) attempted to access "
		         "unaligned address 0x%x (read from DAR)\n",
		         context->srr0, dar);
		break;
	case __OS_EXCEPTION_PROGRAM:
		OSReport("\nProgram exception : Possible illegal instruction/operation "
		         "at or around 0x%x (read from SRR0)\n",
		         context->srr0, dar);
		break;
	case OS_ERROR_PROTECTION:
		OSReport("\n");
		OSReport("AI DMA Address =   0x%04x%04x\n", __DSPRegs[DSP_DMA_START_HI], __DSPRegs[DSP_DMA_START_LO]);
		OSReport("ARAM DMA Address = 0x%04x%04x\n", __DSPRegs[DSP_ARAM_DMA_MM_HI], __DSPRegs[DSP_ARAM_DMA_MM_LO]);
		OSReport("DI DMA Address =   0x%08x\n", __DIRegs[DI_DMA_MEM_ADDR]);
		break;
	}

	OSReport("\nLast interrupt (%d): SRR0 = 0x%08x  TB = 0x%016llx\n", __OSLastInterrupt, __OSLastInterruptSrr0, __OSLastInterruptTime);

	PPCHalt();
}

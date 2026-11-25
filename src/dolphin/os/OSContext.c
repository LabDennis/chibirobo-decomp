#include "Dolphin/os.h"
#include "Dolphin/db.h"

extern void __OSSaveContextSDA(OSContext* context);

typedef struct OSStackFrame {
	u32 backChain;
	u32 savedLR;
} OSStackFrame;

enum { MAX_DUMP_FRAMES = 16 };

/**
 * @note Address: 0x80163864
 * @note Size: 0xC
 */
OSContext* OSGetCurrentContext(void)
{
	return (OSContext*)__OSCurrentContext;
}

/**
 * @note Address: 0x80163A00
 * @note Size: 0x24
 */
void OSClearContext(OSContext* context)
{
	context->mode  = 0;
	context->state = 0;
	if (context == __OSFPUContext) {
		__OSFPUContext = nullptr;
	}
}

void OSInitContext(OSContext* context, u32 pc, u32 stackPtr)
{
	u32 reg;

	context->srr0 = pc;
	context->gpr[1] = stackPtr;
	context->srr1 = MSR_EE | MSR_ME | MSR_IR | MSR_DR | MSR_RI;
	context->cr = 0;
	context->xer = 0;

	// The new thread inherits the small-data area bases in r2 and r13.
	__OSSaveContextSDA(context);
	for (reg = 3; reg < 13; ++reg) {
		context->gpr[reg] = 0;
	}
	for (reg = 14; reg < 32; ++reg) {
		context->gpr[reg] = 0;
	}
	for (reg = 0; reg < 8; ++reg) {
		context->gqr[reg] = 0;
	}

	OSClearContext(context);
}

/**
 * @note Address: 0x80163AE0
 * @note Size: 0x2A8
 */
void OSDumpContext(OSContext* context)
{
	u32 i;
	const OSStackFrame* frame;

	OSReport("------------------------- Context 0x%08x -------------------------\n", context);

	for (i = 0; i < 16; ++i) {
		OSReport("r%-2d  = 0x%08x (%14d)  r%-2d  = 0x%08x (%14d)\n", i, context->gpr[i], context->gpr[i], i + 16, context->gpr[i + 16],
		         context->gpr[i + 16]);
	}

	OSReport("LR   = 0x%08x                   CR   = 0x%08x\n", context->lr, context->cr);
	OSReport("SRR0 = 0x%08x                   SRR1 = 0x%08x\n", context->srr0, context->srr1);

	OSReport("\nGQRs----------\n");
	for (i = 0; i < 4; ++i) {
		OSReport("gqr%d = 0x%08x \t gqr%d = 0x%08x\n", i, context->gqr[i], i + 4, context->gqr[i + 4]);
	}

	if (context->state & OS_CONTEXT_STATE_FPSAVED) {
		OSContext* currentContext;
		OSContext fpuContext;
		BOOL enabled;

		enabled        = OSDisableInterrupts();
		currentContext = OSGetCurrentContext();
		OSClearContext(&fpuContext);
		OSSetCurrentContext(&fpuContext);

		OSReport("\n\nFPRs----------\n");
		for (i = 0; i < 32; i += 2) {
			OSReport("fr%d \t= %d \t fr%d \t= %d\n", i, (u32)context->fpr[i], i + 1, (u32)context->fpr[i + 1]);
		}
		OSReport("\n\nPSFs----------\n");
		for (i = 0; i < 32; i += 2) {
			OSReport("ps%d \t= 0x%x \t ps%d \t= 0x%x\n", i, (u32)context->psf[i], i + 1, (u32)context->psf[i + 1]);
		}

		OSClearContext(&fpuContext);
		OSSetCurrentContext(currentContext);
		OSRestoreInterrupts(enabled);
	}

	OSReport("\nAddress:      Back Chain    LR Save\n");
	frame = (const OSStackFrame*)context->gpr[1];
	for (i = 0; i < MAX_DUMP_FRAMES && frame && (u32)frame != 0xffffffff; ++i) {
		OSReport("0x%08x:   0x%08x    0x%08x\n", (u32)frame, frame->backChain, frame->savedLR);
		frame = (const OSStackFrame*)frame->backChain;
	}
}

/**
 * @note Address: 0x80163E0C
 * @note Size: 0x48
 */
void __OSContextInit(void)
{
	__OSSetExceptionHandler(__OS_EXCEPTION_FLOATING_POINT, OSSwitchFPUContext);
	__OSFPUContext = nullptr;
	DBPrintf("FPU-unavailable handler installed\n");
}

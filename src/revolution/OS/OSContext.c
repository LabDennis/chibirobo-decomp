#include "revolution/os/OSContext.h"

#include "Dolphin/os.h"
#include "Dolphin/db.h"

// The context-switch and FPU helpers are assembly in the Revolution SDK source;
// wii-ipl and tp keep the same instruction streams.
// wii-ipl: https://github.com/koopthekoopa/wii-ipl/blob/main/libs/RVL_SDK/src/os/OSContext.c
// tp: https://github.com/zeldaret/tp/blob/main/libs/revolution/src/os/OSContext.c

/**
 * @note Address: 0x80182568
 * @note Size: 0xC
 */
OSContext* OSGetCurrentContext()
{
	return (OSContext*)__OSCurrentContext;
}

/**
 * @note Address: 0x80182734
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

/**
 * @note Address: 0x80182814
 * @note Size: 0x260
 */
void OSDumpContext(OSContext* context)
{
	u32 i;
	u32* p;

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
	for (i = 0, p = (u32*)context->gpr[1]; p && (u32)p != 0xffffffff && i++ < 16; p = (u32*)*p) {
		OSReport("0x%08x:   0x%08x    0x%08x\n", p, p[0], p[1]);
	}
}

/**
 * @note Address: 0x80182AF8
 * @note Size: 0x48
 */
void __OSContextInit()
{
	__OSSetExceptionHandler(__OS_EXCEPTION_FLOATING_POINT, OSSwitchFPUContext);
	__OSFPUContext = nullptr;
	DBPrintf("FPU-unavailable handler installed\n");
}

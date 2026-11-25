#include "Dolphin/os.h"
#include "Dolphin/hw_regs.h"

extern void ExternalInterruptHandler(__OSException exception, OSContext* context);

// PI inputs that aggregate interrupts from other controllers.
#define PI_GROUP_INTERRUPT_MASK (PI_INTRPT_EXI | PI_INTRPT_AI | PI_INTRPT_DSP | PI_INTRPT_MEM)

typedef struct EXIInterruptMasks {
	OSInterruptMask exi;
	OSInterruptMask transfer;
	OSInterruptMask external;
} EXIInterruptMasks;

static const EXIInterruptMasks EXIInterruptMasksByChannel[] = {
	{OS_INTERRUPTMASK_EXI_0_EXI, OS_INTERRUPTMASK_EXI_0_TC, OS_INTERRUPTMASK_EXI_0_EXT},
	{OS_INTERRUPTMASK_EXI_1_EXI, OS_INTERRUPTMASK_EXI_1_TC, OS_INTERRUPTMASK_EXI_1_EXT},
	{OS_INTERRUPTMASK_EXI_2_EXI, OS_INTERRUPTMASK_EXI_2_TC, 0},
};

static __OSInterruptHandler* InterruptHandlerTable;

static OSInterruptMask InterruptPrioTable[] = {
	OS_INTERRUPTMASK_PI_ERROR,
	OS_INTERRUPTMASK_PI_DEBUG,
	OS_INTERRUPTMASK_MEM,
	OS_INTERRUPTMASK_PI_RSW,
	OS_INTERRUPTMASK_PI_VI,
	OS_INTERRUPTMASK_PI_PE,
	OS_INTERRUPTMASK_PI_HSP,
	OS_INTERRUPTMASK_DSP_ARAM | OS_INTERRUPTMASK_DSP_DSP | OS_INTERRUPTMASK_AI | OS_INTERRUPTMASK_EXI | OS_INTERRUPTMASK_PI_SI
	    | OS_INTERRUPTMASK_PI_DI,
	OS_INTERRUPTMASK_DSP_AI,
	OS_INTERRUPTMASK_PI_CP,
	0xFFFFFFFF,
};

volatile OSTime __OSLastInterruptTime;
volatile __OSInterrupt __OSLastInterrupt;
vu32 __OSLastInterruptSrr0;

/**
 * @note Address: 0x80166E9C
 * @note Size: 0x1C
 */
__OSInterruptHandler __OSSetInterruptHandler(__OSInterrupt interrupt, __OSInterruptHandler handler)
{
	__OSInterruptHandler oldHandler;

	oldHandler                       = InterruptHandlerTable[interrupt];
	InterruptHandlerTable[interrupt] = handler;
	return oldHandler;
}

/**
 * @note Address: 0x80166EB8
 * @note Size: 0x14
 */
__OSInterruptHandler __OSGetInterruptHandler(__OSInterrupt interrupt)
{
	return InterruptHandlerTable[interrupt];
}

/**
 * @note Address: 0x80166ECC
 * @note Size: 0x74
 */
void __OSInterruptInit()
{
	InterruptHandlerTable = OSPhysicalToCached(0x3040);
	memset(InterruptHandlerTable, 0, __OS_INTERRUPT_MAX * sizeof(__OSInterruptHandler));

	__OSPriorInterruptMask = 0;

	__OSCurrentInterruptMask = 0;

	__PIRegs[PI_INTRPT_MASK] = PI_GROUP_INTERRUPT_MASK;

	__OSMaskInterrupts(OS_INTERRUPTMASK_MEM | OS_INTERRUPTMASK_DSP | OS_INTERRUPTMASK_AI | OS_INTERRUPTMASK_EXI | OS_INTERRUPTMASK_PI);

	__OSSetExceptionHandler(__OS_EXCEPTION_EXTERNAL_INTERRUPT, ExternalInterruptHandler);
}

static void SetEXIInterruptMask(u32 channel, OSInterruptMask current)
{
	const EXIInterruptMasks* masks = &EXIInterruptMasksByChannel[channel];
	u32 control = EXI_REG(channel, EXI_REG_CPR);

	// Writing zero to each status bit leaves any pending interrupt unacknowledged.
	control &= ~(EXI_CPR_EXI_INT_ENABLE | EXI_CPR_EXI_INT | EXI_CPR_TC_INT_ENABLE | EXI_CPR_TC_INT);
	if (channel == 0) {
		control &= ~EXI_CPR_ROM_DISABLE;
	}
	if (!(current & masks->exi)) {
		control |= EXI_CPR_EXI_INT_ENABLE;
	}
	if (!(current & masks->transfer)) {
		control |= EXI_CPR_TC_INT_ENABLE;
	}
	if (masks->external) {
		control &= ~(EXI_CPR_EXT_INT_ENABLE | EXI_CPR_EXT_INT);
		if (!(current & masks->external)) {
			control |= EXI_CPR_EXT_INT_ENABLE;
		}
	}
	EXI_REG(channel, EXI_REG_CPR) = control;
}

static OSInterruptMask GetEXIInterruptCause(void)
{
	OSInterruptMask cause = 0;
	u32 channel;

	for (channel = 0; channel < ARRAY_SIZE(EXIInterruptMasksByChannel); ++channel) {
		const EXIInterruptMasks* masks = &EXIInterruptMasksByChannel[channel];
		u32 status = EXI_REG(channel, EXI_REG_CPR);

		if (status & EXI_CPR_EXI_INT) {
			cause |= masks->exi;
		}
		if (status & EXI_CPR_TC_INT) {
			cause |= masks->transfer;
		}
		if (status & EXI_CPR_EXT_INT) {
			cause |= masks->external;
		}
	}
	return cause;
}

/**
 * @note Address: 0x80166F40
 * @note Size: 0x2D8
 */
static u32 SetInterruptMask(OSInterruptMask mask, OSInterruptMask current)
{
	u32 reg;

	switch (__cntlzw(mask)) {
	case __OS_INTERRUPT_MEM_0:
	case __OS_INTERRUPT_MEM_1:
	case __OS_INTERRUPT_MEM_2:
	case __OS_INTERRUPT_MEM_3:
	case __OS_INTERRUPT_MEM_ADDRESS:
		reg = 0;
		if (!(current & OS_INTERRUPTMASK_MEM_0)) {
			reg |= OS_PROTECT0_BIT;
		}
		if (!(current & OS_INTERRUPTMASK_MEM_1)) {
			reg |= OS_PROTECT1_BIT;
		}
		if (!(current & OS_INTERRUPTMASK_MEM_2)) {
			reg |= OS_PROTECT2_BIT;
		}
		if (!(current & OS_INTERRUPTMASK_MEM_3)) {
			reg |= OS_PROTECT3_BIT;
		}
		if (!(current & OS_INTERRUPTMASK_MEM_ADDRESS)) {
			reg |= OS_PROTECT_ADDRERR_BIT;
		}
		__MEMRegs[MEM_INTRPT_MASK] = (u16)reg;
		mask &= ~OS_INTERRUPTMASK_MEM;
		break;

	case __OS_INTERRUPT_DSP_AI:
	case __OS_INTERRUPT_DSP_ARAM:
	case __OS_INTERRUPT_DSP_DSP:
		reg = __DSPRegs[DSP_CONTROL_STATUS];
		reg &= ~(DSP_STATUS_INTERRUPTS | DSP_CONTROL_AI_INT_ENABLE |
		         DSP_CONTROL_ARAM_INT_ENABLE | DSP_CONTROL_DSP_INT_ENABLE);
		if (!(current & OS_INTERRUPTMASK_DSP_AI)) {
			reg |= DSP_CONTROL_AI_INT_ENABLE;
		}
		if (!(current & OS_INTERRUPTMASK_DSP_ARAM)) {
			reg |= DSP_CONTROL_ARAM_INT_ENABLE;
		}
		if (!(current & OS_INTERRUPTMASK_DSP_DSP)) {
			reg |= DSP_CONTROL_DSP_INT_ENABLE;
		}
		__DSPRegs[DSP_CONTROL_STATUS] = (u16)reg;
		mask &= ~OS_INTERRUPTMASK_DSP;
		break;

	case __OS_INTERRUPT_AI_AI:
		reg = __AIRegs[AI_CONTROL];
		reg &= ~(AI_CONTROL_STREAM_INT_ENABLE | AI_STATUS_STREAM_INTERRUPT | AI_CONTROL_STREAM_COUNT_RESET);
		if (!(current & OS_INTERRUPTMASK_AI_AI)) {
			reg |= AI_CONTROL_STREAM_INT_ENABLE;
		}
		__AIRegs[AI_CONTROL] = reg;
		mask &= ~OS_INTERRUPTMASK_AI;
		break;

	case __OS_INTERRUPT_EXI_0_EXI:
	case __OS_INTERRUPT_EXI_0_TC:
	case __OS_INTERRUPT_EXI_0_EXT:
		SetEXIInterruptMask(0, current);
		mask &= ~OS_INTERRUPTMASK_EXI_0;
		break;

	case __OS_INTERRUPT_EXI_1_EXI:
	case __OS_INTERRUPT_EXI_1_TC:
	case __OS_INTERRUPT_EXI_1_EXT:
		SetEXIInterruptMask(1, current);
		mask &= ~OS_INTERRUPTMASK_EXI_1;
		break;

	case __OS_INTERRUPT_EXI_2_EXI:
	case __OS_INTERRUPT_EXI_2_TC:
		SetEXIInterruptMask(2, current);
		mask &= ~OS_INTERRUPTMASK_EXI_2;
		break;

	case __OS_INTERRUPT_PI_CP:
	case __OS_INTERRUPT_PI_SI:
	case __OS_INTERRUPT_PI_DI:
	case __OS_INTERRUPT_PI_RSW:
	case __OS_INTERRUPT_PI_ERROR:
	case __OS_INTERRUPT_PI_VI:
	case __OS_INTERRUPT_PI_DEBUG:
	case __OS_INTERRUPT_PI_PE_TOKEN:
	case __OS_INTERRUPT_PI_PE_FINISH:
	case __OS_INTERRUPT_PI_HSP:
		reg = PI_GROUP_INTERRUPT_MASK;

		if (!(current & OS_INTERRUPTMASK_PI_CP)) {
			reg |= PI_INTRPT_CP;
		}
		if (!(current & OS_INTERRUPTMASK_PI_SI)) {
			reg |= PI_INTRPT_SI;
		}
		if (!(current & OS_INTERRUPTMASK_PI_DI)) {
			reg |= PI_INTRPT_DVD;
		}
		if (!(current & OS_INTERRUPTMASK_PI_RSW)) {
			reg |= PI_INTRPT_RSW;
		}
		if (!(current & OS_INTERRUPTMASK_PI_ERROR)) {
			reg |= PI_INTRPT_ERR;
		}
		if (!(current & OS_INTERRUPTMASK_PI_VI)) {
			reg |= PI_INTRPT_VI;
		}
		if (!(current & OS_INTERRUPTMASK_PI_DEBUG)) {
			reg |= PI_INTRPT_DEBUG;
		}
		if (!(current & OS_INTERRUPTMASK_PI_PE_TOKEN)) {
			reg |= PI_INTRPT_PE_TOKEN;
		}
		if (!(current & OS_INTERRUPTMASK_PI_PE_FINISH)) {
			reg |= PI_INTRPT_PE_FINISH;
		}
		if (!(current & OS_INTERRUPTMASK_PI_HSP)) {
			reg |= PI_INTRPT_HSP;
		}
		__PIRegs[PI_INTRPT_MASK] = reg;
		mask &= ~OS_INTERRUPTMASK_PI;
		break;

	default:
		break;
	}

	return mask;
}

/**
 * @note Address: 0x80167218
 * @note Size: 0x88
 */
OSInterruptMask __OSMaskInterrupts(OSInterruptMask global)
{
	BOOL enabled;
	OSInterruptMask prev;
	OSInterruptMask local;
	OSInterruptMask mask;

	enabled = OSDisableInterrupts();
	prev    = __OSPriorInterruptMask;
	local   = __OSCurrentInterruptMask;
	mask    = ~(prev | local) & global;
	global |= prev;
	__OSPriorInterruptMask = global;
	while (mask) {
		mask = SetInterruptMask(mask, global | local);
	}
	OSRestoreInterrupts(enabled);
	return prev;
}

/**
 * @note Address: 0x801672A0
 * @note Size: 0x88
 */
OSInterruptMask __OSUnmaskInterrupts(OSInterruptMask global)
{
	BOOL enabled;
	OSInterruptMask prev;
	OSInterruptMask local;
	OSInterruptMask mask;

	enabled                = OSDisableInterrupts();
	prev                   = __OSPriorInterruptMask;
	local                  = __OSCurrentInterruptMask;
	mask                   = (prev | local) & global;
	global                 = prev & ~global;
	__OSPriorInterruptMask = global;
	while (mask) {
		mask = SetInterruptMask(mask, global | local);
	}
	OSRestoreInterrupts(enabled);
	return prev;
}

/**
 * @note Address: 0x80167328
 * @note Size: 0x344
 */
void __OSDispatchInterrupt(__OSException exception, OSContext* context)
{
	u32 intsr;
	u32 reg;
	OSInterruptMask cause;
	OSInterruptMask unmasked;
	OSInterruptMask* prio;
	__OSInterrupt interrupt;
	__OSInterruptHandler handler;
	intsr = __PIRegs[PI_INTRPT_SRC];
	intsr &= ~PI_INTRPT_RSWST;

	if (intsr == 0 || (intsr & __PIRegs[PI_INTRPT_MASK]) == 0) {
		OSLoadContext(context);
	}

	cause = 0;

	if (intsr & PI_INTRPT_MEM) {
		reg = __MEMRegs[MEM_INTRPT_SRC];
		if (reg & OS_PROTECT0_BIT)
			cause |= OS_INTERRUPTMASK_MEM_0;
		if (reg & OS_PROTECT1_BIT)
			cause |= OS_INTERRUPTMASK_MEM_1;
		if (reg & OS_PROTECT2_BIT)
			cause |= OS_INTERRUPTMASK_MEM_2;
		if (reg & OS_PROTECT3_BIT)
			cause |= OS_INTERRUPTMASK_MEM_3;
		if (reg & OS_PROTECT_ADDRERR_BIT)
			cause |= OS_INTERRUPTMASK_MEM_ADDRESS;
	}

	if (intsr & PI_INTRPT_DSP) {
		reg = __DSPRegs[DSP_CONTROL_STATUS];
		if (reg & DSP_STATUS_AI_INTERRUPT)
			cause |= OS_INTERRUPTMASK_DSP_AI;
		if (reg & DSP_STATUS_ARAM_INTERRUPT)
			cause |= OS_INTERRUPTMASK_DSP_ARAM;
		if (reg & DSP_STATUS_DSP_INTERRUPT)
			cause |= OS_INTERRUPTMASK_DSP_DSP;
	}

	if (intsr & PI_INTRPT_AI) {
		reg = __AIRegs[AI_CONTROL];
		if (reg & AI_STATUS_STREAM_INTERRUPT)
			cause |= OS_INTERRUPTMASK_AI_AI;
	}

	if (intsr & PI_INTRPT_EXI) {
		cause |= GetEXIInterruptCause();
	}

	if (intsr & PI_INTRPT_HSP)
		cause |= OS_INTERRUPTMASK_PI_HSP;
	if (intsr & PI_INTRPT_DEBUG)
		cause |= OS_INTERRUPTMASK_PI_DEBUG;
	if (intsr & PI_INTRPT_PE_FINISH)
		cause |= OS_INTERRUPTMASK_PI_PE_FINISH;
	if (intsr & PI_INTRPT_PE_TOKEN)
		cause |= OS_INTERRUPTMASK_PI_PE_TOKEN;
	if (intsr & PI_INTRPT_VI)
		cause |= OS_INTERRUPTMASK_PI_VI;
	if (intsr & PI_INTRPT_SI)
		cause |= OS_INTERRUPTMASK_PI_SI;
	if (intsr & PI_INTRPT_DVD)
		cause |= OS_INTERRUPTMASK_PI_DI;
	if (intsr & PI_INTRPT_RSW)
		cause |= OS_INTERRUPTMASK_PI_RSW;
	if (intsr & PI_INTRPT_CP)
		cause |= OS_INTERRUPTMASK_PI_CP;
	if (intsr & PI_INTRPT_ERR)
		cause |= OS_INTERRUPTMASK_PI_ERROR;

	unmasked = cause & ~(__OSPriorInterruptMask | __OSCurrentInterruptMask);
	if (unmasked) {
		for (prio = InterruptPrioTable;; ++prio) {
			if (unmasked & *prio) {
				interrupt = (__OSInterrupt)__cntlzw(unmasked & *prio);
				break;
			}
		}

		handler = __OSGetInterruptHandler(interrupt);
		if (handler) {
			if (__OS_INTERRUPT_MEM_ADDRESS < interrupt) {
				__OSLastInterrupt     = interrupt;
				__OSLastInterruptTime = OSGetTime();
				__OSLastInterruptSrr0 = context->srr0;
			}

			OSDisableScheduler();
			handler(interrupt, context);
			OSEnableScheduler();
			__OSReschedule();
			OSLoadContext(context);
		}
	}

	OSLoadContext(context);
}

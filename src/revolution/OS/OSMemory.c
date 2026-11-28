#include "revolution/hw_regs.h"
#include "revolution/os/OSReset.h"
#include "Dolphin/os.h"

// The MMIO/SPR helpers are assembly in the Revolution SDK source; tp keeps
// the same instruction streams.
// tp: https://github.com/zeldaret/tp/blob/main/libs/revolution/src/os/OSMemory.c

// forward declarations.
static BOOL OnShutdown(BOOL final, u32 event);

// Local shutdown function information.
static OSShutdownFunctionInfo ShutdownFunctionInfo = { OnShutdown, OS_RESET_PRIO_MEM };

// useful macros.
#define TRUNC(n, a) (((u32)(n)) & ~((a) - 1))
#define ROUND(n, a) (((u32)(n) + (a) - 1) & ~((a) - 1))

// low memory: sizes and the end of the usable part of MEM2
#define OS_MEM1_PHYSICAL_SIZE  (*(u32*)0x80003100)
#define OS_MEM1_SIMULATED_SIZE (*(u32*)0x80003104)
#define OS_MEM2_PHYSICAL_SIZE  (*(u32*)0x80003118)
#define OS_MEM2_SIMULATED_SIZE (*(u32*)0x8000311C)
#define OS_MEM2_USABLE_END     (*(u32*)0x80003120)

static u32 OSGetPhysicalMem1Size()
{
	return OS_MEM1_PHYSICAL_SIZE;
}

u32 OSGetPhysicalMem2Size()
{
	return OS_MEM2_PHYSICAL_SIZE;
}

u32 OSGetConsoleSimulatedMem1Size()
{
	return OS_MEM1_SIMULATED_SIZE;
}

u32 OSGetConsoleSimulatedMem2Size()
{
	return OS_MEM2_SIMULATED_SIZE;
}

static BOOL OnShutdown(BOOL final, u32 event)
{
	if (final != FALSE) {
		__MEMRegs[MEM_PROT_TYPE] = 0xFF;
		__OSMaskInterrupts(OS_INTERRUPTMASK_MEM_RESET);
	}
	return TRUE;
}

static void MEMIntrruptHandler(__OSInterrupt interrupt, OSContext* context)
{
	u32 addr;
	u32 cause;

	cause                      = __MEMRegs[MEM_INTRPT_SRC];
	addr                       = (((u32)__MEMRegs[MEM_INTRPT_ADDR_HI] & 0x3ff) << 16) | __MEMRegs[MEM_INTRPT_ADDR_LO];
	__MEMRegs[MEM_INTRPT_FLAG] = 0;

	if (__OSErrorTable[OS_ERROR_PROTECTION]) {
		__OSErrorTable[OS_ERROR_PROTECTION](OS_ERROR_PROTECTION, context, cause, addr);
		return;
	}

	__OSUnhandledException(OS_ERROR_PROTECTION, context, cause, addr);
}

extern void Config24MB(void);
extern void ConfigMEM2_52MB(void);
extern void ConfigMEM2_56MB(void);
extern void ConfigMEM2_64MB(void);
extern void ConfigMEM2_112MB(void);
extern void ConfigMEM2_128MB(void);
extern void RealMode(u32 addr);

// The caller hands in 0xBA2CF; any other value stops the console here.
static void BATConfig(u32 key)
{
	u32 mem1Sim = OSGetConsoleSimulatedMem1Size();
	u32 mem2End;
	u32 mem2Sim;

	if (mem1Sim < OSGetPhysicalMem1Size() && mem1Sim == 0x1800000) {
		DCInvalidateRange((void*)0x81800000, 0x1800000);
		__MEMRegs[MEM_UNK_FLAG] = 2;
	}

	if (mem1Sim <= 0x1800000) {
		RealMode((u32)&Config24MB);
	}

	mem2Sim = OSGetConsoleSimulatedMem2Size();
	mem2End = OS_MEM2_USABLE_END;
	if (mem2Sim <= 0x4000000) {
		if (mem2End <= 0x93400000) {
			RealMode((u32)&ConfigMEM2_52MB);
		} else if (mem2End <= 0x93800000) {
			RealMode((u32)&ConfigMEM2_56MB);
		} else {
			RealMode((u32)&ConfigMEM2_64MB);
		}
	} else if (mem2Sim <= 0x8000000) {
		if (mem2End <= 0x97000000) {
			RealMode((u32)&ConfigMEM2_112MB);
		} else {
			RealMode((u32)&ConfigMEM2_128MB);
		}
	}

	while (key != 0xBA2CF) { }
}

void __OSRestoreMEM1Config(u32 key)
{
	RealMode((u32)&Config24MB);

	while (key != 0xBA2CF) { }
}

void __OSInitMemoryProtection()
{
	static BOOL initialized = FALSE;
	BOOL enabled;

	if (!initialized) {
		enabled = OSDisableInterrupts();

		__MEMRegs[MEM_INTRPT_FLAG] = 0;
		__MEMRegs[MEM_PROT_TYPE]   = 0xFF;

		__OSMaskInterrupts(OS_INTERRUPTMASK_MEM_0 | OS_INTERRUPTMASK_MEM_1 | OS_INTERRUPTMASK_MEM_2 | OS_INTERRUPTMASK_MEM_3);
		__OSSetInterruptHandler(__OS_INTERRUPT_MEM_0, MEMIntrruptHandler);
		__OSSetInterruptHandler(__OS_INTERRUPT_MEM_1, MEMIntrruptHandler);
		__OSSetInterruptHandler(__OS_INTERRUPT_MEM_2, MEMIntrruptHandler);
		__OSSetInterruptHandler(__OS_INTERRUPT_MEM_3, MEMIntrruptHandler);
		__OSSetInterruptHandler(__OS_INTERRUPT_MEM_ADDRESS, MEMIntrruptHandler);
		OSRegisterShutdownFunction(&ShutdownFunctionInfo);
		BATConfig(0xBA2CF);
		__OSUnmaskInterrupts(OS_INTERRUPTMASK_MEM_ADDRESS);
		initialized = TRUE;
		OSRestoreInterrupts(enabled);
	}
}

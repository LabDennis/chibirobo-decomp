#include "Dolphin/os.h"
#include "Dolphin/hw_regs.h"

#define OS_SIMULATED_MEM_SIZE_ADDR 0xF0
#define MEM_SIZE_24MB 0x01800000
#define MEM_SIZE_48MB 0x03000000

#define MEM_PROT_CHANNEL_COUNT (OS_PROTECT_CHAN3 + 1)
#define MEM_PROT_BLOCK_SIZE (1u << MEM_PROT_ADDR_SHIFT)
#define MEM_PROT_ALL_RW ((1u << (MEM_PROT_ACCESS_BITS * MEM_PROT_CHANNEL_COUNT)) - 1)

extern void Config24MB(void);
extern void Config48MB(void);
extern void RealMode(void (*entry)(void));

static BOOL OnReset(BOOL final);

// Local reset function information.
static OSResetFunctionInfo ResetFunctionInfo = { OnReset, OS_RESET_PRIO_MEM };

// useful macros.
#define TRUNC(n, a) (((u32)(n)) & ~((a) - 1))
#define ROUND(n, a) (((u32)(n) + (a) - 1) & ~((a) - 1))

/**
 * @note Address: 0x801682AC
 * @note Size: 0xC
 */
u32 OSGetPhysicalMemSize(void)
{
	const OSBootInfo* bootInfo = OSPhysicalToCached(0);
	return bootInfo->memorySize;
}

/**
 * @note Address: N/A
 * @note Size: 0xC
 */
static u32 OSGetConsoleSimulatedMemSize(void)
{
	return *(const u32*)OSPhysicalToCached(OS_SIMULATED_MEM_SIZE_ADDR);
}

static inline void ResetMemoryProtection(void)
{
	__MEMRegs[MEM_PROT_TYPE] = MEM_PROT_ALL_RW;
	__OSMaskInterrupts(OS_INTERRUPTMASK_MEM_RESET);
}

/**
 * @note Address: 0x801682B8
 * @note Size: 0x3C
 */
static BOOL OnReset(BOOL final)
{
	if (final != FALSE) {
		ResetMemoryProtection();
	}
	return TRUE;
}

/**
 * @note Address: 0x801682F4
 * @note Size: 0x6C
 */
static void MEMIntrruptHandler(__OSInterrupt interrupt, OSContext* context)
{
	// Read the fault address and cause before acknowledging the interrupt.
	u32 addr = ((u32)__MEMRegs[MEM_INTRPT_ADDR_HI] & MEM_INTRPT_ADDR_HI_MASK) << 16;
	u32 cause;

	addr |= __MEMRegs[MEM_INTRPT_ADDR_LO];
	cause = __MEMRegs[MEM_INTRPT_SRC];
	__MEMRegs[MEM_INTRPT_FLAG] = 0;

	if (__OSErrorTable[OS_ERROR_PROTECTION]) {
		__OSErrorTable[OS_ERROR_PROTECTION](OS_ERROR_PROTECTION, context, cause, addr);
		return;
	}

	__OSUnhandledException(OS_ERROR_PROTECTION, context, cause, addr);
}

/**
 * @note Address: N/A
 * @note Size: 0xC4
 */
void OSProtectRange(u32 channel, void* addr, u32 numBytes, u32 control)
{
	BOOL enabled;
	u32 start;
	u32 end;
	OSInterruptMask interruptMask;
	u16 reg;
	if (channel >= MEM_PROT_CHANNEL_COUNT) {
		return;
	}

	control &= OS_PROTECT_CONTROL_RDWR;

	end   = (u32)addr + numBytes;
	start = TRUNC(addr, MEM_PROT_BLOCK_SIZE);
	end   = ROUND(end, MEM_PROT_BLOCK_SIZE);

	DCFlushRange((void*)start, end - start);

	enabled       = OSDisableInterrupts();
	interruptMask = OS_INTERRUPTMASK(__OS_INTERRUPT_MEM_0 + channel);

	__OSMaskInterrupts(interruptMask);

	__MEMRegs[MEM_PROT_START(channel)] = (u16)(start >> MEM_PROT_ADDR_SHIFT);
	__MEMRegs[MEM_PROT_END(channel)]   = (u16)(end >> MEM_PROT_ADDR_SHIFT);

	reg = __MEMRegs[MEM_PROT_TYPE];
	reg &= ~(OS_PROTECT_CONTROL_RDWR << (MEM_PROT_ACCESS_BITS * channel));
	reg |= control << (MEM_PROT_ACCESS_BITS * channel);
	__MEMRegs[MEM_PROT_TYPE] = reg;

	if (control != OS_PROTECT_CONTROL_RDWR) {
		__OSUnmaskInterrupts(interruptMask);
	}

	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x80168478
 * @note Size: 0x118
 */
void __OSInitMemoryProtection(void)
{
	u32 simulatedSize;
	u32 bootSimulatedSize = OSGetConsoleSimulatedMemSize();
	BOOL enabled = OSDisableInterrupts();

	__MEMRegs[MEM_INTRPT_FLAG] = 0;
	ResetMemoryProtection();
	__OSSetInterruptHandler(__OS_INTERRUPT_MEM_0, MEMIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_MEM_1, MEMIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_MEM_2, MEMIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_MEM_3, MEMIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_MEM_ADDRESS, MEMIntrruptHandler);
	OSRegisterResetFunction(&ResetFunctionInfo);

	simulatedSize = OSGetConsoleSimulatedMemSize();
	if (simulatedSize < OSGetPhysicalMemSize() && simulatedSize == MEM_SIZE_24MB) {
		DCInvalidateRange(OSPhysicalToCached(MEM_SIZE_24MB), MEM_SIZE_24MB);
		__MEMRegs[MEM_UNK_FLAG] = 2;
	}

	if (bootSimulatedSize <= MEM_SIZE_24MB) {
		RealMode(Config24MB);
	} else if (bootSimulatedSize <= MEM_SIZE_48MB) {
		RealMode(Config48MB);
	}

	__OSUnmaskInterrupts(OS_INTERRUPTMASK_MEM_ADDRESS);
	OSRestoreInterrupts(enabled);
}

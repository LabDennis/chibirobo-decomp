#include "revolution/hw_regs.h"
#include "revolution/os/OSReset.h"
#include "Dolphin/os.h"

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

ASM static void Config24MB()
{
#ifdef __MWERKS__ // clang-format off
	nofralloc

	addi    r7,r0,0

	addis   r4,r0,0x00000002@ha
	addi    r4,r4,0x00000002@l
	addis   r3,r0,0x800001ff@ha
	addi    r3,r3,0x800001ff@l

	addis   r6,r0,0x01000002@ha
	addi    r6,r6,0x01000002@l
	addis   r5,r0,0x810000ff@ha
	addi    r5,r5,0x810000ff@l

	isync

	mtspr   dbat0u,r7
	mtspr   dbat0l,r4
	mtspr   dbat0u,r3
	isync

	mtspr   ibat0u,r7
	mtspr   ibat0l,r4
	mtspr   ibat0u,r3
	isync

	mtspr   dbat2u,r7
	mtspr   dbat2l,r6
	mtspr   dbat2u,r5
	isync

	mtspr   ibat2u,r7
	mtspr   ibat2l,r6
	mtspr   ibat2u,r5
	isync

	mfmsr   r3
	ori     r3, r3, 0x30
	mtsrr1  r3

	mflr    r3
	mtsrr0  r3
	rfi
#endif // clang-format on
}

// MEM2 ends at or below 0x93400000: 64 MB cached, 128 MB uncached, then 32 MB and 4 MB mapped at 0x92000000 and 0x93000000
ASM static void ConfigMEM2_52MB()
{
#ifdef __MWERKS__ // clang-format off
	nofralloc

	addi    r7,r0,0

	addis   r4,r0,0x10000002@ha
	addi    r4,r4,0x10000002@l
	addis   r3,r0,0x900003FF@ha
	addi    r3,r3,0x900003FF@l

	addis   r6,r0,0x1000002A@ha
	addi    r6,r6,0x1000002A@l
	addis   r5,r0,0xD00007FF@ha
	addi    r5,r5,0xD00007FF@l

	isync

	mtspr   568,r7
	mtspr   569,r4
	mtspr   568,r3
	isync

	mtspr   560,r7
	mtspr   561,r7
	isync

	mtspr   570,r7
	mtspr   571,r6
	mtspr   570,r5
	isync

	mtspr   562,r7
	mtspr   563,r7
	isync

	addis   r4,r0,0x12000002@ha
	addi    r4,r4,0x12000002@l
	addis   r3,r0,0x920001FF@ha
	addi    r3,r3,0x920001FF@l

	addis   r6,r0,0x13000002@ha
	addi    r6,r6,0x13000002@l
	addis   r5,r0,0x9300007F@ha
	addi    r5,r5,0x9300007F@l

	isync

	mtspr   572,r7
	mtspr   573,r4
	mtspr   572,r3
	isync

	mtspr   564,r7
	mtspr   565,r7
	isync

	mtspr   574,r7
	mtspr   575,r6
	mtspr   574,r5
	isync

	mtspr   566,r7
	mtspr   567,r7
	isync

	mfmsr   r3
	ori     r3, r3, 0x30
	mtsrr1  r3

	mflr    r3
	mtsrr0  r3
	rfi
#endif // clang-format on
}

// MEM2 ends at or below 0x93800000: as above with 8 MB at 0x93000000
ASM static void ConfigMEM2_56MB()
{
#ifdef __MWERKS__ // clang-format off
	nofralloc

	addi    r7,r0,0

	addis   r4,r0,0x10000002@ha
	addi    r4,r4,0x10000002@l
	addis   r3,r0,0x900003FF@ha
	addi    r3,r3,0x900003FF@l

	addis   r6,r0,0x1000002A@ha
	addi    r6,r6,0x1000002A@l
	addis   r5,r0,0xD00007FF@ha
	addi    r5,r5,0xD00007FF@l

	isync

	mtspr   568,r7
	mtspr   569,r4
	mtspr   568,r3
	isync

	mtspr   560,r7
	mtspr   561,r7
	isync

	mtspr   570,r7
	mtspr   571,r6
	mtspr   570,r5
	isync

	mtspr   562,r7
	mtspr   563,r7
	isync

	addis   r4,r0,0x12000002@ha
	addi    r4,r4,0x12000002@l
	addis   r3,r0,0x920001FF@ha
	addi    r3,r3,0x920001FF@l

	addis   r6,r0,0x13000002@ha
	addi    r6,r6,0x13000002@l
	addis   r5,r0,0x930000FF@ha
	addi    r5,r5,0x930000FF@l

	isync

	mtspr   572,r7
	mtspr   573,r4
	mtspr   572,r3
	isync

	mtspr   564,r7
	mtspr   565,r7
	isync

	mtspr   574,r7
	mtspr   575,r6
	mtspr   574,r5
	isync

	mtspr   566,r7
	mtspr   567,r7
	isync

	mfmsr   r3
	ori     r3, r3, 0x30
	mtsrr1  r3

	mflr    r3
	mtsrr0  r3
	rfi
#endif // clang-format on
}

// the whole 64 MB of MEM2
ASM static void ConfigMEM2_64MB()
{
#ifdef __MWERKS__ // clang-format off
	nofralloc

	addi    r7,r0,0

	addis   r4,r0,0x10000002@ha
	addi    r4,r4,0x10000002@l
	addis   r3,r0,0x900007FF@ha
	addi    r3,r3,0x900007FF@l

	addis   r6,r0,0x1000002A@ha
	addi    r6,r6,0x1000002A@l
	addis   r5,r0,0xD00007FF@ha
	addi    r5,r5,0xD00007FF@l

	isync

	mtspr   568,r7
	mtspr   569,r4
	mtspr   568,r3
	isync

	mtspr   560,r7
	mtspr   561,r7
	isync

	mtspr   570,r7
	mtspr   571,r6
	mtspr   570,r5
	isync

	mtspr   562,r7
	mtspr   563,r7
	isync

	mtspr   564,r7
	mtspr   565,r7
	isync

	mtspr   566,r7
	mtspr   567,r7
	isync

	mtspr   572,r7
	mtspr   573,r7
	isync

	mtspr   574,r7
	mtspr   575,r7
	isync

	mfmsr   r3
	ori     r3, r3, 0x30
	mtsrr1  r3

	mflr    r3
	mtsrr0  r3
	rfi
#endif // clang-format on
}

// 128 MB MEM2, end at or below 0x97000000
ASM static void ConfigMEM2_112MB()
{
#ifdef __MWERKS__ // clang-format off
	nofralloc

	addi    r7,r0,0

	addis   r4,r0,0x10000002@ha
	addi    r4,r4,0x10000002@l
	addis   r3,r0,0x900007FF@ha
	addi    r3,r3,0x900007FF@l

	addis   r6,r0,0x1000002A@ha
	addi    r6,r6,0x1000002A@l
	addis   r5,r0,0xD0000FFF@ha
	addi    r5,r5,0xD0000FFF@l

	isync

	mtspr   568,r7
	mtspr   569,r4
	mtspr   568,r3
	isync

	mtspr   560,r7
	mtspr   561,r7
	isync

	mtspr   570,r7
	mtspr   571,r6
	mtspr   570,r5
	isync

	mtspr   562,r7
	mtspr   563,r7
	isync

	addis   r4,r0,0x14000002@ha
	addi    r4,r4,0x14000002@l
	addis   r3,r0,0x940003FF@ha
	addi    r3,r3,0x940003FF@l

	addis   r6,r0,0x16000002@ha
	addi    r6,r6,0x16000002@l
	addis   r5,r0,0x960001FF@ha
	addi    r5,r5,0x960001FF@l

	isync

	mtspr   572,r7
	mtspr   573,r4
	mtspr   572,r3
	isync

	mtspr   564,r7
	mtspr   565,r7
	isync

	mtspr   574,r7
	mtspr   575,r6
	mtspr   574,r5
	isync

	mtspr   566,r7
	mtspr   567,r7
	isync

	mfmsr   r3
	ori     r3, r3, 0x30
	mtsrr1  r3

	mflr    r3
	mtsrr0  r3
	rfi
#endif // clang-format on
}

// the whole 128 MB of MEM2
ASM static void ConfigMEM2_128MB()
{
#ifdef __MWERKS__ // clang-format off
	nofralloc

	addi    r7,r0,0

	addis   r4,r0,0x10000002@ha
	addi    r4,r4,0x10000002@l
	addis   r3,r0,0x90000FFF@ha
	addi    r3,r3,0x90000FFF@l

	addis   r6,r0,0x1000002A@ha
	addi    r6,r6,0x1000002A@l
	addis   r5,r0,0xD0000FFF@ha
	addi    r5,r5,0xD0000FFF@l

	isync

	mtspr   568,r7
	mtspr   569,r4
	mtspr   568,r3
	isync

	mtspr   560,r7
	mtspr   561,r7
	isync

	mtspr   570,r7
	mtspr   571,r6
	mtspr   570,r5
	isync

	mtspr   562,r7
	mtspr   563,r7
	isync

	mtspr   564,r7
	mtspr   565,r7
	isync

	mtspr   566,r7
	mtspr   567,r7
	isync

	mtspr   572,r7
	mtspr   573,r7
	isync

	mtspr   574,r7
	mtspr   575,r7
	isync

	mfmsr   r3
	ori     r3, r3, 0x30
	mtsrr1  r3

	mflr    r3
	mtsrr0  r3
	rfi
#endif // clang-format on
}

ASM static void RealMode(register u32 addr)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	clrlwi r3, r3, 2
	mtsrr0 r3
	mfmsr r3
	rlwinm r3, r3, 0, 28, 25
	mtsrr1 r3
	rfi
#endif // clang-format on
}

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

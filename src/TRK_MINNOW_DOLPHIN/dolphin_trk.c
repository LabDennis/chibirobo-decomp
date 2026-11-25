#include "PowerPC_EABI_Support/MetroTRK/trk.h"
#include "Dolphin/ar.h"
#include "Dolphin/OS/OSCache.h"

#define EXCEPTIONMASK_ADDR 0x80000044
#define EXCEPTIONMASK_PHYSICAL_ADDR 0x00000044
#define LOCKED_CACHE_SIZE 0x4000
#define BAT_VALID_MASK 3

enum {
	TRK_EXTERNAL_INTERRUPT_VECTOR_INDEX = 4,
};

extern void OSResetSystem(BOOL reset, u32 resetCode, BOOL forceMenu);
extern void __ARClearInterrupt(void);

static u32 lc_base;

static u32 TRK_ISR_OFFSETS[15] = { PPC_SystemReset,
	                               PPC_MachineCheck,
	                               PPC_DataStorage,
	                               PPC_InstructionStorage,
	                               PPC_ExternalInterrupt,
	                               PPC_Alignment,
	                               PPC_Program,
	                               PPC_FloatingPointUnavailable,
	                               PPC_Decrementer,
	                               PPC_SystemCall,
	                               PPC_Trace,
	                               PPC_PerformanceMonitor,
	                               PPC_InstructionAddressBreakpoint,
	                               PPC_SystemManagementInterrupt,
	                               PPC_ThermalManagementInterrupt };

/**
 * @note Address: 0x80005088
 * @note Size: 0x2C
 */
DECL_SECT(".init") void __TRK_reset()
{
	OSResetSystem(FALSE, 0, FALSE);
}

/**
 * @note Address: 0x8014FB4C
 * @note Size: 0x20
 */
void EnableMetroTRKInterrupts(void)
{
	EnableEXI2Interrupts();
}

/**
 * @note Address: 0x8014FAF4
 * @note Size: 0x58
 */
u32 TRKTargetTranslate(u32 val)
{
	if (val >= lc_base && val < lc_base + LOCKED_CACHE_SIZE) {
		if (gTRKCPUState.Extended1.DBAT3U_ & BAT_VALID_MASK)
			return val;
	}

	if (val >= 0x7E000000 && val <= 0x80000000) {
		return val;
	}

	return (val & 0x3FFFFFFF) | 0x80000000;
}

/**
 * @note Address: N/A
 * @note Size: 0xB0
 */
void TRK_copy_vector(u32 offset)
{
	void* destPtr = (void*)TRKTargetTranslate(offset);
	TRK_memcpy(destPtr, (void*)(gTRKInterruptVectorTable + offset), 0x100);
	TRK_flush_cache(destPtr, 0x100);
}

/**
 * @note Address: 0x8014F9C8
 * @note Size: 0x12C
 */
void __TRK_copy_vectors(void)
{
	u32 maskAddress = lc_base;
	u32* vectorOffsets;
	int vectorIndex;
	u32 exceptionMask;

	if (maskAddress <= EXCEPTIONMASK_PHYSICAL_ADDR
	    && maskAddress + LOCKED_CACHE_SIZE > EXCEPTIONMASK_PHYSICAL_ADDR
	    && (gTRKCPUState.Extended1.DBAT3U_ & BAT_VALID_MASK)) {
		maskAddress = EXCEPTIONMASK_PHYSICAL_ADDR;
	} else {
		maskAddress = EXCEPTIONMASK_ADDR;
	}

	vectorIndex = 0;
	exceptionMask = *(u32*)maskAddress;
	vectorOffsets = TRK_ISR_OFFSETS;

	// Leave the external interrupt vector in place.
	do {
		if ((exceptionMask & (1 << vectorIndex)) && vectorIndex != TRK_EXTERNAL_INTERRUPT_VECTOR_INDEX) {
			TRK_copy_vector(vectorOffsets[vectorIndex]);
		}

		vectorIndex++;
	} while (vectorIndex <= 14);
}

/**
 * @note Address: 0x8014F97C
 * @note Size: 0x4C
 */
DSError TRKInitializeTarget()
{
	gTRKState.isStopped = TRUE;
	gTRKState.msr       = __TRK_get_MSR();
	lc_base             = 0xE0000000;
	return DS_NoError;
}

#define TRK_ARAM_ADDRESS_LIMIT 0x08000000

static u32 CacheLengthForDMA(u32 address, u32 dmaLength)
{
	// The range APIs add the first line's offset before counting cache lines.
	u32 lineOffset = address - OSRoundDown32B(address);
	return dmaLength == 0 ? 0 : dmaLength - lineOffset;
}

/**
 * @note Address: 0x8014F848
 * @note Size: 0x134
 */
void TRK__read_aram(u32 mainMemoryAddress, u32 aramAddress, const u32* length)
{
	u32 dmaAddress;
	u32 dmaLength;
	u16 pendingInterrupt;

	if (aramAddress < __AR_ARAM_USR_BASE_ADDR || aramAddress + *length > TRK_ARAM_ADDRESS_LIMIT) {
		return;
	}

	dmaAddress = OSRoundDown32B(aramAddress);
	dmaLength = OSRoundUp32B(*length + (aramAddress - dmaAddress));
	DCInvalidateRange((void*)mainMemoryAddress, CacheLengthForDMA(mainMemoryAddress, dmaLength));

	while (ARGetDMAStatus()) { }

	pendingInterrupt = __ARGetInterruptStatus();
	__ARClearInterrupt();
	ARStartDMARead(mainMemoryAddress, dmaAddress, dmaLength);

	while (!__ARGetInterruptStatus()) { }

	if (!pendingInterrupt) {
		__ARClearInterrupt();
	}
}

static void ReadAramBlock(void* block, u32 aramAddress)
{
	DCInvalidateRange(block, ARQ_DMA_ALIGNMENT);
	__ARClearInterrupt();
	ARStartDMARead((u32)block, aramAddress, ARQ_DMA_ALIGNMENT);

	while (!__ARGetInterruptStatus()) { }
}

/**
 * @note Address: 0x8014F65C
 * @note Size: 0x1EC
 */
void TRK__write_aram(u32 mainMemoryAddress, u32 aramAddress, const u32* length)
{
	u8 block[ARQ_DMA_ALIGNMENT] ATTRIBUTE_ALIGN(ARQ_DMA_ALIGNMENT);
	u32 dmaAddress;
	u32 dmaLength;
	u32 cachedAramBlock = TRK_ARAM_ADDRESS_LIMIT;
	u32 firstBlockOffset;
	u32 endAddress;
	u32 lastBlockAddress;
	u32 lastBlockOffset;
	u16 pendingInterrupt;

	if (aramAddress < __AR_ARAM_USR_BASE_ADDR || aramAddress + *length > TRK_ARAM_ADDRESS_LIMIT) {
		return;
	}

	dmaAddress = OSRoundDown32B(aramAddress);
	firstBlockOffset = aramAddress - dmaAddress;
	dmaLength = OSRoundUp32B(*length + firstBlockOffset);
	DCFlushRangeNoSync((void*)mainMemoryAddress, CacheLengthForDMA(mainMemoryAddress, dmaLength));

	while (ARGetDMAStatus()) { }

	pendingInterrupt = __ARGetInterruptStatus();
	if (firstBlockOffset != 0) {
		cachedAramBlock = dmaAddress;
		ReadAramBlock(block, dmaAddress);
		TRK_memcpy((void*)mainMemoryAddress, block, firstBlockOffset);
		DCFlushRangeNoSync((void*)mainMemoryAddress, 1);
	}

	endAddress = aramAddress + *length;
	lastBlockAddress = OSRoundDown32B(endAddress);
	lastBlockOffset = endAddress - lastBlockAddress;
	if (lastBlockOffset != 0) {
		void* tail = (void*)(mainMemoryAddress + endAddress);

		if (lastBlockAddress != cachedAramBlock) {
			ReadAramBlock(block, lastBlockAddress);
		}
		TRK_memcpy(tail, block + lastBlockOffset, ARQ_DMA_ALIGNMENT - lastBlockOffset);
		DCFlushRangeNoSync(tail, 1);
	}

	__sync();
	__ARClearInterrupt();
	ARStartDMAWrite(mainMemoryAddress, dmaAddress, dmaLength);

	if (!pendingInterrupt) {
		while (!__ARGetInterruptStatus()) { }

		__ARClearInterrupt();
	}
}

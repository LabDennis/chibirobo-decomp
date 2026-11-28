#include "Dolphin/os.h"
#include "Dolphin/db.h"

extern void __LCEnable(void);

// The cache/SPR functions are handwritten assembly in the Revolution SDK source;
// they live in os_cache_lines.s and os_locked_cache.s here. tp keeps the same
// instruction streams.
// tp: https://github.com/zeldaret/tp/blob/main/libs/revolution/src/os/OSCache.c

// /**
//  * @note Address: N/A
//  * @note Size: 0x10
//  */
// void DCFlashInvalidate(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x14
//  */
// void DCDisable(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x14
//  */
// void DCFreeze(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x10
//  */
// void DCUnfreeze(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x8
//  */
// void DCTouchLoad(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x8
//  */
// void DCBlockZero(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x8
//  */
// void DCBlockStore(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x8
//  */
// void DCBlockFlush(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x8
//  */
// void DCBlockInvalidate(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x14
//  */
// void ICDisable(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x14
//  */
// void ICFreeze(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x10
//  */
// void ICUnfreeze(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x8
//  */
// void ICBlockInvalidate(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x8
//  */
// void ICSync(void)
// {
// 	// UNUSED FUNCTION
// }

/**
 * @note Address: 0x80181ED0
 * @note Size: 0x38
 */
void LCEnable(void)
{
	BOOL enabled;

	enabled = OSDisableInterrupts();
	__LCEnable();
	OSRestoreInterrupts(enabled);
}

// /**
//  * @note Address: N/A
//  * @note Size: 0x14
//  */
// void LCAllocOneTag(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x40
//  */
// void LCAllocTags(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x70
//  */
// void LCAlloc(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x70
//  */
// void LCAllocNoInvalidate(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0xAC
//  */
// u32 LCLoadData(void* destAddr, void* srcAddr, u32 nBytes)
// {
// 	// UNUSED FUNCTION
// }

/**
 * @note Address: 0x80181F78
 * @note Size: 0xA0
 */
u32 LCStoreData(void* destAddr, // to main memory destination
                void* srcAddr,  // from locked cache source
                u32 nBytes)
{
	u32 numBlocks       = (nBytes + 31) / 32;
	u32 numTransactions = (numBlocks + LC_MAX_DMA_BLOCKS - 1) / LC_MAX_DMA_BLOCKS;

	while (numBlocks > 0) {
		if (numBlocks < LC_MAX_DMA_BLOCKS) {
			LCStoreBlocks(destAddr, srcAddr, numBlocks);
			numBlocks = 0;
		} else {
			LCStoreBlocks(destAddr, srcAddr, 0);
			numBlocks -= LC_MAX_DMA_BLOCKS;
			destAddr = (void*)((u32)destAddr + LC_MAX_DMA_BYTES);
			srcAddr  = (void*)((u32)srcAddr + LC_MAX_DMA_BYTES);
		}
	}

	return numTransactions;
}

// /**
//  * @note Address: N/A
//  * @note Size: 0x48
//  */
// void LCFlushQueue(void)
// {
// 	// UNUSED FUNCTION
// }

/**
 * @note Address: N/A
 * @note Size: 0x4C
 */
inline static void L2Init(void)
{
	u32 oldMSR;

	oldMSR = PPCMfmsr();

	__sync();
	PPCMtmsr(MSR_IR | MSR_DR);
	__sync();

	L2Disable();

	L2GlobalInvalidate();

	PPCMtmsr(oldMSR);
}

/**
 * @note Address: N/A
 * @note Size: 0x2C
 */
inline void L2Enable(void)
{
	PPCMtl2cr((PPCMfl2cr() | L2CR_L2E) & ~L2CR_L2I);
}

/**
 * @note Address: N/A
 * @note Size: 0x30
 */
inline void L2Disable(void)
{
	__sync();
	PPCMtl2cr(PPCMfl2cr() & ~L2CR_L2E);
	__sync();
}

/**
 * @note Address: 0x801632C8
 * @note Size: 0x98
 */
void L2GlobalInvalidate(void)
{
	L2Disable();

	PPCMtl2cr(PPCMfl2cr() | L2CR_L2I);

	while (PPCMfl2cr() & L2CR_L2IP)
		;

	PPCMtl2cr(PPCMfl2cr() & ~L2CR_L2I);

	while (PPCMfl2cr() & L2CR_L2IP) {
		DBPrintf(">>> L2 INVALIDATE : SHOULD NEVER HAPPEN\n");
	}
}

// /**
//  * @note Address: N/A
//  * @note Size: 0x40
//  */
// void L2SetDataOnly(void)
// {
// 	// UNUSED FUNCTION
// }

// /**
//  * @note Address: N/A
//  * @note Size: 0x40
//  */
// void L2SetWriteThrough(void)
// {
// 	// UNUSED FUNCTION
// }

/**
 * @note Address: 0x80182038
 * @note Size: 0x148
 */
void DMAErrorHandler(OSError error, OSContext* context, ...)
{
#pragma unused(error)
	u32 hid2 = PPCMfhid2();

	OSReport("Machine check received\n");
	OSReport("HID2 = 0x%x   SRR1 = 0x%x\n", hid2, context->srr1);
	if (!(hid2 & (HID2_DCHERR | HID2_DNCERR | HID2_DCMERR | HID2_DQOERR)) || !(context->srr1 & SRR1_DMA_BIT)) {
		OSReport("Machine check was not DMA/locked cache related\n");
		OSDumpContext(context);
		PPCHalt();
		// spins forever, so not reached
	}

	OSReport("DMAErrorHandler(): An error occurred while processing DMA.\n");
	OSReport("The following errors have been detected and cleared :\n");

	if (hid2 & HID2_DCHERR) {
		OSReport("\t- Requested a locked cache tag that was already in the cache\n");
	}

	if (hid2 & HID2_DNCERR) {
		OSReport("\t- DMA attempted to access normal cache\n");
	}

	if (hid2 & HID2_DCMERR) {
		OSReport("\t- DMA missed in data cache\n");
	}

	if (hid2 & HID2_DQOERR) {
		OSReport("\t- DMA queue overflowed\n");
	}

	// write hid2 back (to clear the error bits)
	PPCMthid2(hid2);
}

/**
 * @note Address: 0x80182180
 * @note Size: 0x138
 */
void __OSCacheInit(void)
{
	if (!(PPCMfhid0() & HID0_ICE)) {
		ICEnable();
		DBPrintf("L1 i-caches initialized\n");
	}
	if (!(PPCMfhid0() & HID0_DCE)) {
		DCEnable();
		DBPrintf("L1 d-caches initialized\n");
	}

	if (!(PPCMfl2cr() & L2CR_L2E)) {
		L2Init();
		L2Enable();
		DBPrintf("L2 cache initialized\n");
	}

	OSSetErrorHandler(OS_ERROR_MACHINE_CHECK, DMAErrorHandler);
	DBPrintf("Locked cache machine check handler installed\n");
}

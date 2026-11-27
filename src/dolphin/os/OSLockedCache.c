#include "Dolphin/os.h"

extern void __LCEnable(void);

/**
 * @note Address: 0x80163184
 * @note Size: 0x38
 */
void LCEnable(void)
{
	BOOL enabled;

	enabled = OSDisableInterrupts();
	__LCEnable();
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x80163208
 * @note Size: 0xAC
 */
// NONMATCHING: retail has two extra branch-to-next-instruction hops before the loop test and
// uses mr rather than addi for the argument copies. The documented no-assert SDK forms in
// mkdd and pikmin2 compile to the same local sequence with GC/1.2.5n.
u32 LCStoreData(void* destAddr, void* srcAddr, u32 nBytes)
{
	u32 numBlocks       = (nBytes + 31) / 32;
	u32 numTransactions = (numBlocks + LC_MAX_DMA_BLOCKS - 1) / LC_MAX_DMA_BLOCKS;

	while (numBlocks > 0) {
		if (numBlocks < LC_MAX_DMA_BLOCKS) {
			LCStoreBlocks(destAddr, srcAddr, numBlocks);
			numBlocks = 0;
		} else {
			// A zero block count encodes a full 128-block DMA transfer.
			LCStoreBlocks(destAddr, srcAddr, 0);
			numBlocks -= LC_MAX_DMA_BLOCKS;
			destAddr = (void*)((u32)destAddr + LC_MAX_DMA_BYTES);
			srcAddr  = (void*)((u32)srcAddr + LC_MAX_DMA_BYTES);
		}
	}

	return numTransactions;
}

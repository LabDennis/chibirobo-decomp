	.set SPR_HID2, 920
	.set SPR_DMA_U, 922
	.set SPR_DMA_L, 923
	.set DBAT_LOCKED_CACHE, 3

	.set MSR_ME, 0x1000
	.set HID2_LCE_BIT, 3
	.set HID2_LCE_UPPER, 0x1000
	.set HID2_DMA_ERROR_ENABLES_UPPER, 0x000f
	.set DMA_TRIGGER, 2

	.set CACHED_RAM_BASE_UPPER, 0x8000
	.set LOCKED_CACHE_BASE_UPPER, 0xe000
	.set CACHE_LINE_SIZE, 32
	.set DATA_CACHE_LINES, 1024
	.set LOCKED_CACHE_LINES, 512
	.set BAT_READ_WRITE, 2
	.set BAT_BLOCK_16MB, 0x01fc
	.set BAT_SUPERVISOR_VALID, 2

	.text
	.balign 4
	.globl __LCEnable
	.type __LCEnable, @function
__LCEnable:
	mfmsr 5
	ori 5, 5, MSR_ME
	mtmsr 5

	lis 3, CACHED_RAM_BASE_UPPER
	li 4, DATA_CACHE_LINES
	mtctr 4
.Ltouch_cache:
	dcbt 0, 3
	dcbst 0, 3
	addi 3, 3, CACHE_LINE_SIZE
	bdnz .Ltouch_cache

	mfspr 4, SPR_HID2
	oris 4, 4, HID2_LCE_UPPER | HID2_DMA_ERROR_ENABLES_UPPER
	mtspr SPR_HID2, 4

	.rept 12
	nop
	.endr

	lis 3, LOCKED_CACHE_BASE_UPPER
	ori 3, 3, BAT_READ_WRITE
	mtdbatl DBAT_LOCKED_CACHE, 3
	ori 3, 3, BAT_BLOCK_16MB | BAT_SUPERVISOR_VALID
	mtdbatu DBAT_LOCKED_CACHE, 3
	isync

	lis 3, LOCKED_CACHE_BASE_UPPER
	li 6, LOCKED_CACHE_LINES
	mtctr 6
	li 6, 0
.Llock_line:
	dcbz_l 6, 3
	addi 3, 3, CACHE_LINE_SIZE
	bdnz .Llock_line

	.rept 12
	nop
	.endr

	blr
	.size __LCEnable, . - __LCEnable

	.globl LCDisable
	.type LCDisable, @function
LCDisable:
	lis 3, LOCKED_CACHE_BASE_UPPER
	li 4, LOCKED_CACHE_LINES
	mtctr 4
.Linvalidate_line:
	dcbi 0, 3
	addi 3, 3, CACHE_LINE_SIZE
	bdnz .Linvalidate_line
	mfspr 4, SPR_HID2
	rlwinm 4, 4, 0, HID2_LCE_BIT + 1, HID2_LCE_BIT - 1
	mtspr SPR_HID2, 4
	blr
	.size LCDisable, . - LCDisable

	.globl LCStoreBlocks
	.type LCStoreBlocks, @function
LCStoreBlocks:
	# The block count is split across the two DMA registers.
	rlwinm 6, 5, 30, 27, 31
	clrlwi 3, 3, 4
	or 6, 6, 3
	mtspr SPR_DMA_U, 6
	rlwinm 6, 5, 2, 28, 29
	or 6, 6, 4
	ori 6, 6, DMA_TRIGGER
	mtspr SPR_DMA_L, 6
	blr
	.size LCStoreBlocks, . - LCStoreBlocks

	.globl LCQueueWait
	.type LCQueueWait, @function
LCQueueWait:
.Lread_queue:
	mfspr 4, SPR_HID2
	rlwinm 4, 4, 8, 28, 31
	cmpw 4, 3
	bgt .Lread_queue
	blr
	.size LCQueueWait, . - LCQueueWait

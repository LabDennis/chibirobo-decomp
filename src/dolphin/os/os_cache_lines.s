	.set SPR_HID0, 1008
	.set HID0_DCE, 0x4000
	.set HID0_ICE, 0x8000
	.set HID0_ICFI, 0x0800
	.set CACHE_LINE_SHIFT, 5
	.set CACHE_LINE_SIZE, 1 << CACHE_LINE_SHIFT

	# r3 is the first address and r4 is the byte count.
	# Round up to include the partial lines at both ends of the range.
	.macro cache_range name, operation, finish=none
	.globl \name
	.type \name, @function
\name:
	cmplwi 4, 0
	blelr
	clrlwi 5, 3, 32 - CACHE_LINE_SHIFT
	add 4, 4, 5
	addi 4, 4, CACHE_LINE_SIZE - 1
	srwi 4, 4, CACHE_LINE_SHIFT
	mtctr 4
.Lcache_line\@:
	\operation 0, 3
	addi 3, 3, CACHE_LINE_SIZE
	bdnz .Lcache_line\@
	.ifc \finish, syscall
	# Use the OS system-call vector to synchronize the data cache operation.
	sc
	.endif
	.ifc \finish, instruction
	sync
	isync
	.endif
	blr
	.size \name, . - \name
	.endm

	.text
	.balign 4

	.ifdef OS_CACHE_HELPERS_ONLY
	# Build the additional SDK entry point in its own optional object.
	cache_range DCStoreRangeNoSync, dcbst
	.else

	.globl DCEnable
	.type DCEnable, @function
DCEnable:
	sync
	mfspr 3, SPR_HID0
	ori 3, 3, HID0_DCE
	mtspr SPR_HID0, 3
	blr
	.size DCEnable, . - DCEnable

	cache_range DCInvalidateRange, dcbi
	cache_range DCFlushRange, dcbf, syscall
	cache_range DCStoreRange, dcbst, syscall
	cache_range DCFlushRangeNoSync, dcbf
	cache_range DCZeroRange, dcbz
	cache_range ICInvalidateRange, icbi, instruction

	.globl ICFlashInvalidate
	.type ICFlashInvalidate, @function
ICFlashInvalidate:
	mfspr 3, SPR_HID0
	ori 3, 3, HID0_ICFI
	mtspr SPR_HID0, 3
	blr
	.size ICFlashInvalidate, . - ICFlashInvalidate

	.globl ICEnable
	.type ICEnable, @function
ICEnable:
	isync
	mfspr 3, SPR_HID0
	ori 3, 3, HID0_ICE
	mtspr SPR_HID0, 3
	blr
	.size ICEnable, . - ICEnable

	.endif

	.set MSR_FP, 0x2000
	.set BOOT_CACHE_ADDRESS_MASK, 0xfffffff1
	.set BOOT_CACHE_STRIDE, 8

	.section .init, "ax", @progbits
	.balign 4
	.globl __init_hardware
	.type __init_hardware, @function
__init_hardware:
	mfmsr 0
	ori 0, 0, MSR_FP
	mtmsr 0

	# Save the incoming link register in r31 during early hardware initialization.
	mflr 31
	bl __OSPSInit
	bl __OSFPRInit
	bl __OSCacheInit
	mtlr 31
	blr
	.size __init_hardware, . - __init_hardware

	.globl __flush_cache
	.type __flush_cache, @function
__flush_cache:
	# Clear address bits 1–3 before walking the range in eight-byte steps.
	lis 5, BOOT_CACHE_ADDRESS_MASK@h
	ori 5, 5, BOOT_CACHE_ADDRESS_MASK@l
	and 5, 5, 3
	subf 3, 5, 3
	add 4, 4, 3

.Lflush:
	dcbst 0, 5
	sync
	icbi 0, 5
	addic 5, 5, BOOT_CACHE_STRIDE
	addic. 4, 4, -BOOT_CACHE_STRIDE
	bge .Lflush
	isync
	blr
	.size __flush_cache, . - __flush_cache

	.text
	.balign 4
	.globl __init_user
	.type __init_user, @function
__init_user:
	stwu 1, -0x10(1)
	mflr 0
	stw 0, 0x14(1)
	bl __init_cpp
	lwz 0, 0x14(1)
	mtlr 0
	addi 1, 1, 0x10
	blr
	.size __init_user, . - __init_user

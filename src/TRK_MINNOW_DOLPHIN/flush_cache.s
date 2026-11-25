	.set FLUSH_STRIDE, 8

	.text
	.balign 4
	.globl TRK_flush_cache
	.type TRK_flush_cache, @function
TRK_flush_cache:
	# r3 is the start address and r4 is the byte count. Include the
	# prefix between the masked start address and the original address.
	lis 5, -1
	ori 5, 5, 0xfff1
	and 5, 5, 3
	subf 3, 5, 3
	add 4, 4, 3

.Lflush:
	# Complete the data cache operations before invalidating instructions.
	dcbst 0, 5
	dcbf 0, 5
	sync
	icbi 0, 5
	addic 5, 5, FLUSH_STRIDE
	addic. 4, 4, -FLUSH_STRIDE
	bge .Lflush
	isync
	blr
	.size TRK_flush_cache, . - TRK_flush_cache

	.text
	.balign 4
	.globl Run
	.type Run, @function
Run:
	stwu 1, -0x10(1)
	mflr 0
	stw 0, 0x14(1)
	stw 31, 0xc(1)
	mr 31, 3
	bl ICFlashInvalidate
	sync
	isync

	# The boot entry point takes control through the count register.
	mtctr 31
	bctr

	lwz 31, 0xc(1)
	lwz 0, 0x14(1)
	mtlr 0
	addi 1, 1, 0x10
	blr
	.size Run, . - Run

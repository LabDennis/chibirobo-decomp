	.text
	.balign 4
	.globl __OSRunExecutable
	.type __OSRunExecutable, @function
__OSRunExecutable:
	mflr 0
	stw 0, 4(1)
	stwu 1, -0x18(1)
	stw 31, 0x14(1)
	mr 31, 3
	bl ICFlashInvalidate
	sync
	isync

	# The boot entry point takes control with this stack frame and its address in LR.
	mtlr 31
	blr
	.size __OSRunExecutable, . - __OSRunExecutable

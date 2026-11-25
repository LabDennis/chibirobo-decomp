	.text
	.balign 4
	.globl __OSWriteTimeBase
	.type __OSWriteTimeBase, @function
__OSWriteTimeBase:
	# Clear TBL before installing the new high word to prevent a carry.
	li 5, 0
	mttbl 5
	mttbu 3
	mttbl 4
	blr
	.size __OSWriteTimeBase, . - __OSWriteTimeBase

	.globl __OSSetTick
	.type __OSSetTick, @function
__OSSetTick:
	mttbl 3
	blr
	.size __OSSetTick, . - __OSSetTick

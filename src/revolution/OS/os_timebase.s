	.text
	.balign 4
	.globl OSGetTime
	.type OSGetTime, @function
OSGetTime:
	# Retry if the low word wraps during the read.
	mftbu 3
	mftb 4
	mftbu 5
	cmpw 3, 5
	bne OSGetTime
	blr
	.size OSGetTime, . - OSGetTime

	.globl OSGetTick
	.type OSGetTick, @function
OSGetTick:
	mftb 3
	blr
	.size OSGetTick, . - OSGetTick

	.text
	.balign 4
	.globl OSInitFastCast
	.type OSInitFastCast, @function
OSInitFastCast:
	# Initialize the load and store formats with a scale of zero.
	lis 3, 4
	ori 3, 3, 4
	mtspr 914, 3
	lis 3, 5
	ori 3, 3, 5
	mtspr 915, 3
	lis 3, 6
	ori 3, 3, 6
	mtspr 916, 3
	lis 3, 7
	ori 3, 3, 7
	mtspr 917, 3
	blr
	.size OSInitFastCast, . - OSInitFastCast

	.globl __OSf32tos16
	.type __OSf32tos16, @function
__OSf32tos16:
	stwu 1, -16(1)
	psq_st 1, 8(1), 1, 5
	lha 3, 8(1)
	addi 1, 1, 16
	blr
	.size __OSf32tos16, . - __OSf32tos16

	.globl __OSf32tou8
	.type __OSf32tou8, @function
__OSf32tou8:
	stwu 1, -16(1)
	psq_st 1, 8(1), 1, 2
	lbz 3, 8(1)
	addi 1, 1, 16
	blr
	.size __OSf32tou8, . - __OSf32tou8

	.globl __OSf32tos8
	.type __OSf32tos8, @function
__OSf32tos8:
	stwu 1, -16(1)
	psq_st 1, 8(1), 1, 4
	lbz 3, 8(1)
	extsb 3, 3
	addi 1, 1, 16
	blr
	.size __OSf32tos8, . - __OSf32tos8

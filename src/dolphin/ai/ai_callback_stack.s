	.text
	.balign 4
	.globl __AICallOnStack
	.type __AICallOnStack, @function
__AICallOnStack:
	mflr 0
	stw 0, 4(1)
	stwu 1, -16(1)
	stw 31, 12(1)

	# r31 keeps the original frame across the callback. The alternate stack
	# supplies the callback linkage area.
	mr 31, 1
	addi 1, 4, -8
	mtlr 3
	blrl

	mr 1, 31
	lwz 31, 12(1)
	addi 1, 1, 16
	lwz 0, 4(1)
	mtlr 0
	blr
	.size __AICallOnStack, . - __AICallOnStack

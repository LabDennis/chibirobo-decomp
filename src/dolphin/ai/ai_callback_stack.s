	.text
	.balign 4
	.globl __AICallbackStackSwitch
	.type __AICallbackStackSwitch, @function
__AICallbackStackSwitch:
	mflr 0
	stw 0, 4(1)
	stwu 1, -0x18(1)
	stw 31, 0x14(1)
	mr 31, 3

	lis 5, __OldStack@ha
	addi 5, 5, __OldStack@l
	stw 1, 0(5)
	lis 5, __CallbackStack@ha
	addi 5, 5, __CallbackStack@l
	lwz 1, 0(5)
	subi 1, 1, 8
	mtlr 31
	blrl

	lis 5, __OldStack@ha
	addi 5, 5, __OldStack@l
	lwz 1, 0(5)
	lwz 0, 0x1c(1)
	lwz 31, 0x14(1)
	addi 1, 1, 0x18
	mtlr 0
	blr
	.size __AICallbackStackSwitch, . - __AICallbackStackSwitch

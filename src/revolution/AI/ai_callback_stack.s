	.text
	.balign 4
	.globl __AICallbackStackSwitch
	.type __AICallbackStackSwitch, @function
__AICallbackStackSwitch:
	stwu 1, -0x10(1)
	mflr 0
	stw 0, 0x14(1)
	stw 31, 0xc(1)
	mr 31, 1
	mr 31, 3

	# Enter the callback on the AI stack, then return to the interrupted stack.
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
	mr 10, 1
	lwz 31, 0xc(10)
	lwz 10, 0(1)
	lwz 0, 4(10)
	mr 1, 10
	mtlr 0
	blr
	.size __AICallbackStackSwitch, . - __AICallbackStackSwitch

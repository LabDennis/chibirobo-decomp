	.text
	.balign 4

	.globl __init_cpp_exceptions
	.type __init_cpp_exceptions, @function
__init_cpp_exceptions:
	stwu     1, -16(1)
	mflr     0
	stw      0, 20(1)
	lwz      0, fragmentID@sda21(0)
	cmpwi    0, -2
	bne      0, .Linit_cpp_return
	lis      3, _eti_init_info@ha
	mr       4, 2
	addi     3, 3, _eti_init_info@l
	bl       __register_fragment
	stw      3, fragmentID@sda21(0)
.Linit_cpp_return:
	lwz      0, 20(1)
	mtlr     0
	addi     1, 1, 16
	blr
	.size __init_cpp_exceptions, . - __init_cpp_exceptions

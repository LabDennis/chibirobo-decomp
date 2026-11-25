	.section .init, "ax", @progbits
	.balign 4
	.weak __start
	.type __start, @function
__start:
	bl __init_registers
	bl __init_hardware

	# Terminate the stack backchain before entering the C runtime.
	li 0, -1
	stwu 1, -8(1)
	stw 0, 4(1)
	stw 0, 0(1)
	bl __init_data
	b __OSStartMain
	.size __start, . - __start

	.type __init_registers, @function
__init_registers:
	.irp reg, 0,3,4,5,6,7,8,9,10,11,12,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	li \reg, 0
	.endr

	lis 1, _stack_addr@h
	ori 1, 1, _stack_addr@l
	lis 2, _SDA2_BASE_@h
	ori 2, 2, _SDA2_BASE_@l
	lis 13, _SDA_BASE_@h
	ori 13, 13, _SDA_BASE_@l
	blr
	.size __init_registers, . - __init_registers

	.globl __OSInitBootDebugger
	.type __OSInitBootDebugger, @function
__OSInitBootDebugger:
	# MetroTRK reads the transport ID from r5 and saves LR as its resume address.
	mr 5, 3
	b InitMetroTRK
	.size __OSInitBootDebugger, . - __OSInitBootDebugger

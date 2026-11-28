	.include "sdk/private/os/exception_context.inc"

	.text
	.balign 4
	.globl ExternalInterruptHandler
	.type ExternalInterruptHandler, @function
ExternalInterruptHandler:
	# Preserve the dispatcher inputs, the exception number in r3 and context in r4.
	save_exception_registers 4
	stwu 1, -8(1)
	b __OSDispatchInterrupt
	.size ExternalInterruptHandler, . - ExternalInterruptHandler

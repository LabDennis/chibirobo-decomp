	.set MSR_DR, 0x10
	.set MSR_IR, 0x20

	.text
	.balign 4
	.globl __DBExceptionDestination
	.type __DBExceptionDestination, @function
__DBExceptionDestination:
	# Enable instruction and data translation before entering C.
	mfmsr 3
	ori 3, 3, MSR_IR | MSR_DR
	mtmsr 3
	b __DBExceptionDestinationAux
	.size __DBExceptionDestination, . - __DBExceptionDestination

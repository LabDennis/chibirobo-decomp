	.set SPR_HID0, 1008
	.set HID0_ABE, 8

	.text
	.balign 4
	.globl __OSSystemCallVectorStart, __OSSystemCallVectorEnd
	.type SystemCallVector, @function
SystemCallVector:
__OSSystemCallVectorStart:
	# Temporarily enable address broadcast around the synchronization.
	mfspr 9, SPR_HID0
	ori 10, 9, HID0_ABE
	mtspr SPR_HID0, 10
	isync
	sync
	mtspr SPR_HID0, 9
	rfi

	# __OSInitSystemCall copies this template up to the end label.
__OSSystemCallVectorEnd:
	nop
	.size SystemCallVector, . - SystemCallVector

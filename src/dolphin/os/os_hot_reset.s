	.set SPR_HID0, 1008
	.set HID0_ABE, 8
	.set PI_BASE, 0xcc003000
	.set PI_RESET_CODE, 0x24
	.set PI_RESET_PREPARE, 3
	.set RESET_DELAY_TICKS, 0x1124

	.text
	.balign 4
	.globl __OSResetHardware
	.type __OSResetHardware, @function
__OSResetHardware:
	mfspr 8, SPR_HID0
	ori 8, 8, HID0_ABE
	mtspr SPR_HID0, 8
	isync
	sync
	nop

	# Subtract the starting tick so the wait also works across rollover.
	mftb 5
.Lwait:
	mftb 6
	subf 7, 5, 6
	cmplwi 7, RESET_DELAY_TICKS
	blt .Lwait
	nop

	lis 8, PI_BASE@h
	ori 8, 8, PI_BASE@l
	li 4, PI_RESET_PREPARE
	stw 4, PI_RESET_CODE(8)
	stw 3, PI_RESET_CODE(8)
	nop

	# The hardware resets the CPU.
.Lawait_reset:
	nop
	b .Lawait_reset
	.size __OSResetHardware, . - __OSResetHardware

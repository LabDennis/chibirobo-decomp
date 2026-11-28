	.set MSR_EE, 0x8000
	.set MSR_EE_BIT, 16

	.text
	.balign 4
	.globl OSDisableInterrupts
	.type OSDisableInterrupts, @function
	.globl __RAS_OSDisableInterrupts_begin, __RAS_OSDisableInterrupts_end
OSDisableInterrupts:
	# OSLoadContext restarts this range if an exception interrupted the update.
__RAS_OSDisableInterrupts_begin:
	mfmsr 3
	rlwinm 4, 3, 0, MSR_EE_BIT + 1, MSR_EE_BIT - 1
	mtmsr 4
__RAS_OSDisableInterrupts_end:
	# Return the previous interrupt state as a boolean.
	rlwinm 3, 3, MSR_EE_BIT + 1, 31, 31
	blr
	.size OSDisableInterrupts, . - OSDisableInterrupts

	.globl OSEnableInterrupts
	.type OSEnableInterrupts, @function
OSEnableInterrupts:
	mfmsr 3
	ori 4, 3, MSR_EE
	mtmsr 4
	rlwinm 3, 3, MSR_EE_BIT + 1, 31, 31
	blr
	.size OSEnableInterrupts, . - OSEnableInterrupts

	.globl OSRestoreInterrupts
	.type OSRestoreInterrupts, @function
OSRestoreInterrupts:
	cmpwi 3, 0
	mfmsr 4
	beq .Ldisable
	ori 5, 4, MSR_EE
	b .Lrestore
.Ldisable:
	rlwinm 5, 4, 0, MSR_EE_BIT + 1, MSR_EE_BIT - 1
.Lrestore:
	mtmsr 5
	rlwinm 3, 4, MSR_EE_BIT + 1, 31, 31
	blr
	.size OSRestoreInterrupts, . - OSRestoreInterrupts

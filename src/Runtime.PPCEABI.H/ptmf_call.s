	.set PTMF_THIS_DELTA, 0
	.set PTMF_VTABLE_OFFSET, 4
	.set PTMF_TARGET, 8

	.text
	.balign 4
	.globl __ptmf_scall
	.type __ptmf_scall, @function
__ptmf_scall:
	# The compiler passes the member pointer in r12 and this in r3.
	lwz 0, PTMF_THIS_DELTA(12)
	lwz 11, PTMF_VTABLE_OFFSET(12)
	lwz 12, PTMF_TARGET(12)
	add 3, 3, 0
	cmpwi 11, 0
	blt .Ldirect
	lwzx 12, 3, 12
	lwzx 12, 12, 11
.Ldirect:
	# Forward the call with its arguments and return address intact.
	mtctr 12
	bctr
	.size __ptmf_scall, . - __ptmf_scall

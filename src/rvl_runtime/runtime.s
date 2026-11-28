	.text
	.balign 4

	.globl __cvt_fp2unsigned
	.type __cvt_fp2unsigned, @function
__cvt_fp2unsigned:
	# Clamp to the u32 range, then convert the low 31 bits with fctiwz.
	stwu     1, -16(1)
	lis      4, __constants@ha
	addi     4, 4, __constants@l
	li       3, 0
	lfd      0, 0(4)
	lfd      3, 8(4)
	lfd      4, 16(4)
	fcmpu    0, 1, 0
	fcmpu    6, 1, 3
	blt      0, .Lreturn_u32
	addi     3, 3, -1
	bge      6, .Lreturn_u32
	fcmpu    7, 1, 4
	fmr      2, 1
	blt      7, .Lconvert_u32
	fsub     2, 1, 4
.Lconvert_u32:
	fctiwz   2, 2
	stfd     2, 8(1)
	lwz      3, 12(1)
	blt      7, .Lreturn_u32
	addis    3, 3, -0x8000
.Lreturn_u32:
	addi     1, 1, 16
	blr
	.size __cvt_fp2unsigned, . - __cvt_fp2unsigned

	# Each exported entry falls through to save or restore the remaining
	# nonvolatile registers. r11 points immediately past the register area.
	.macro register_range name, entry, op, width
	.globl \name
	.type \name, @function
\name:
	.irp reg,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	.globl \entry\reg
\entry\reg:
	\op      \reg, (\reg - 32) * \width (11)
	.endr
	blr
	.size \name, . - \name
	.endm

	register_range __save_fpr, _savefpr_, stfd, 8
	register_range __restore_fpr, _restfpr_, lfd, 8
	register_range __save_gpr, _savegpr_, stw, 4
	register_range __restore_gpr, _restgpr_, lwz, 4

	# Divide the unsigned magnitudes. r3:r4 accumulates the quotient and
	# r7:r8 the remainder. The last quotient bit is still in carry.
	.macro divide_magnitudes smaller_dividend
	# Count leading zeros in r3:r4 (dividend) and r5:r6 (divisor).
	cmpwi    0, 3, 0
	cntlzw   0, 3
	cntlzw   9, 4
	bne      0, .Ldividend_count\@
	addi     0, 9, 32
.Ldividend_count\@:
	cmpwi    0, 5, 0
	cntlzw   9, 5
	cntlzw   10, 6
	bne      0, .Ldivisor_count\@
	addi     9, 10, 32
.Ldivisor_count\@:
	cmpw     0, 0, 9
	subfic   10, 0, 64
	.ifb \smaller_dividend
	bgtlr    0
	.else
	bgt      0, \smaller_dividend
	.endif
	addi     9, 9, 1
	subfic   9, 9, 64
	add      0, 0, 9
	subf     9, 9, 10
	mtctr    9
	cmpwi    0, 9, 32
	addi     7, 9, -32
	blt      0, .Lremainder_from_both_words\@
	srw      8, 3, 7
	li       7, 0
	b        .Lnormalize_dividend\@
.Lremainder_from_both_words\@:
	srw      8, 4, 9
	subfic   7, 9, 32
	slw      7, 3, 7
	or       8, 8, 7
	srw      7, 3, 9
.Lnormalize_dividend\@:
	# Shift the unprocessed dividend into r3:r4.
	cmpwi    0, 0, 32
	addic    9, 0, -32
	blt      0, .Lshift_both_words\@
	slw      3, 4, 9
	li       4, 0
	b        .Lprepare_subtraction\@
.Lshift_both_words\@:
	slw      3, 3, 0
	subfic   9, 0, 32
	srw      9, 4, 9
	or       3, 3, 9
	slw      4, 4, 0
.Lprepare_subtraction\@:
	li       10, -1
	addic    7, 7, 0
.Ldivide_bit\@:
	# Shift the next dividend bit into r7:r8. A successful subtraction
	# leaves carry set for the next quotient bit; a failed one keeps carry clear.
	adde     4, 4, 4
	adde     3, 3, 3
	adde     8, 8, 8
	adde     7, 7, 7
	subfc    0, 6, 8
	subfe.   9, 5, 7
	blt      0, .Lnext_bit\@
	mr       8, 0
	mr       7, 9
	addic    0, 10, 1
.Lnext_bit\@:
	bdnz     .Ldivide_bit\@
	.endm

	.globl __div2u
	.type __div2u, @function
__div2u:
	divide_magnitudes .Ldiv2u_smaller_dividend
	adde     4, 4, 4
	adde     3, 3, 3
	blr
.Ldiv2u_smaller_dividend:
	li       4, 0
	li       3, 0
	blr
	.size __div2u, . - __div2u

	.globl __div2i
	.type __div2i, @function
__div2i:
	stwu     1, -16(1)
	rlwinm.  9, 3, 0, 0, 0
	beq      0, .Ldiv2i_dividend_magnitude
	subfic   4, 4, 0
	subfze   3, 3
.Ldiv2i_dividend_magnitude:
	stw      9, 8(1)
	rlwinm.  10, 5, 0, 0, 0
	beq      0, .Ldiv2i_divisor_magnitude
	subfic   6, 6, 0
	subfze   5, 5
.Ldiv2i_divisor_magnitude:
	stw      10, 12(1)
	divide_magnitudes .Ldiv2i_smaller_dividend
	adde     4, 4, 4
	adde     3, 3, 3
	lwz      9, 8(1)
	lwz      10, 12(1)
	xor.     7, 9, 10
	beq      0, .Ldiv2i_no_adjust
	cmpwi    0, 9, 0
	subfic   4, 4, 0
	subfze   3, 3
.Ldiv2i_no_adjust:
	b        .Ldiv2i_return

.Ldiv2i_smaller_dividend:
	li       4, 0
	li       3, 0
.Ldiv2i_return:
	addi     1, 1, 16
	blr
	.size __div2i, . - __div2i

	.globl __mod2u
	.type __mod2u, @function
__mod2u:
	divide_magnitudes .Lmod2u_smaller_dividend
	mr       4, 8
	mr       3, 7
	blr
.Lmod2u_smaller_dividend:
	blr
	.size __mod2u, . - __mod2u

	.globl __mod2i
	.type __mod2i, @function
__mod2i:
	cmpwi    7, 3, 0
	bge      7, .Lmod2i_dividend_magnitude
	subfic   4, 4, 0
	subfze   3, 3
.Lmod2i_dividend_magnitude:
	cmpwi    0, 5, 0
	bge      0, .Lmod2i_divisor_magnitude
	subfic   6, 6, 0
	subfze   5, 5
.Lmod2i_divisor_magnitude:
	divide_magnitudes .Lmod2i_smaller_dividend
	mr       4, 8
	mr       3, 7
.Lmod2i_smaller_dividend:
	bge      7, .Lmod2i_no_adjust
	subfic   4, 4, 0
	subfze   3, 3
.Lmod2i_no_adjust:
	blr
	.size __mod2i, . - __mod2i

	.globl __shl2i
	.type __shl2i, @function
__shl2i:
	subfic   8, 5, 32
	subic    9, 5, 32
	slw      3, 3, 5
	srw      10, 4, 8
	or       3, 3, 10
	slw      10, 4, 9
	or       3, 3, 10
	slw      4, 4, 5
	blr
	.size __shl2i, . - __shl2i

	.globl __cvt_dbl_ull
	.type __cvt_dbl_ull, @function
__cvt_dbl_ull:
	stwu     1, -16(1)
	stfd     1, 8(1)
	lwz      3, 8(1)
	lwz      4, 12(1)
	rlwinm   5, 3, 12, 21, 31
	cmpli    0, 0, 5, 1023
	bge      0, .Ldbl_ull_not_fraction
.Ldbl_ull_return_zero:
	li       3, 0
	li       4, 0
	b        .Ldbl_ull_return
.Ldbl_ull_not_fraction:
	rlwinm.  6, 3, 0, 0, 0
	bne      0, .Ldbl_ull_return_zero
	rlwinm   3, 3, 0, 12, 31
	oris     3, 3, 0x0010
	addi     5, 5, -1075
	cmpwi    0, 5, 0
	bge      0, .Ldbl_ull_left
	neg      5, 5
	subfic   8, 5, 32
	subic    9, 5, 32
	srw      4, 4, 5
	slw      10, 3, 8
	or       4, 4, 10
	srw      10, 3, 9
	or       4, 4, 10
	srw      3, 3, 5
	b        .Ldbl_ull_return
.Ldbl_ull_left:
	cmpwi    0, 5, 11
	ble+     .Ldbl_ull_no_overflow
	li       3, -1
	li       4, -1
	b        .Ldbl_ull_return
.Ldbl_ull_no_overflow:
	subfic   8, 5, 32
	subic    9, 5, 32
	slw      3, 3, 5
	srw      10, 4, 8
	or       3, 3, 10
	slw      10, 4, 9
	or       3, 3, 10
	slw      4, 4, 5
.Ldbl_ull_return:
	addi     1, 1, 16
	blr
	.size __cvt_dbl_ull, . - __cvt_dbl_ull

	.section .rodata, "a", @progbits
	.balign 8
	.type __constants, @object
__constants:
	.double 0.0, 4294967296.0, 2147483648.0
	.size __constants, . - __constants

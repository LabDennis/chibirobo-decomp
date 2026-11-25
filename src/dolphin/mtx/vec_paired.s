	# This source supplies the three runs of paired-single routines. The
	# two scalar C routines link between those runs in the SDK library.
	.set VECTOR_ARITHMETIC, 0
	.set VECTOR_CROSS_PRODUCT, 1
	.set VECTOR_DISTANCE, 2

	# GQR0 leaves single-precision floats unscaled.
	.set GQR_FLOAT, 0

	# Refine the reciprocal-square-root estimate with one Newton step.
	.macro refine_rsqrt estimate, square, half, three, step, half_estimate
	fmuls \step, \estimate, \estimate
	fmuls \half_estimate, \estimate, \half
	fnmsubs \step, \step, \square, \three
	fmuls \estimate, \step, \half_estimate
	.endm

	.text
	.balign 4
	.if VEC_PAIRED_SECTION == VECTOR_ARITHMETIC

	# r3 and r4 point to the inputs, and r5 points to the output.
	.macro vector_binary_op name, operation
	.globl \name
	.type \name, @function
\name:
	psq_l 2, 0(3), 0, GQR_FLOAT
	psq_l 4, 0(4), 0, GQR_FLOAT
	\operation 6, 2, 4
	psq_st 6, 0(5), 0, GQR_FLOAT
	psq_l 3, 8(3), 1, GQR_FLOAT
	psq_l 5, 8(4), 1, GQR_FLOAT
	\operation 7, 3, 5
	psq_st 7, 8(5), 1, GQR_FLOAT
	blr
	.size \name, . - \name
	.endm

	vector_binary_op PSVECAdd, ps_add
	vector_binary_op PSVECSubtract, ps_sub

	.globl PSVECScale
	.type PSVECScale, @function
PSVECScale:
	psq_l 0, 0(3), 0, GQR_FLOAT
	psq_l 2, 8(3), 1, GQR_FLOAT
	ps_muls0 0, 0, 1
	psq_st 0, 0(4), 0, GQR_FLOAT
	ps_muls0 0, 2, 1
	psq_st 0, 8(4), 1, GQR_FLOAT
	blr
	.size PSVECScale, . - PSVECScale

	.globl PSVECNormalize
	.type PSVECNormalize, @function
PSVECNormalize:
	lfs 0, VecHalf@sda21(0)
	lfs 1, VecThree@sda21(0)
	psq_l 2, 0(3), 0, GQR_FLOAT
	ps_mul 5, 2, 2
	psq_l 3, 8(3), 1, GQR_FLOAT
	ps_madd 4, 3, 3, 5
	ps_sum0 4, 4, 3, 5

	# Normalize uses the reciprocal directly, including for a zero input.
	frsqrte 5, 4
	refine_rsqrt 5, 4, 0, 1, 6, 0
	ps_muls0 2, 2, 5
	psq_st 2, 0(4), 0, GQR_FLOAT
	ps_muls0 3, 3, 5
	psq_st 3, 8(4), 1, GQR_FLOAT
	blr
	.size PSVECNormalize, . - PSVECNormalize

	.globl PSVECSquareMag
	.type PSVECSquareMag, @function
PSVECSquareMag:
	psq_l 0, 0(3), 0, GQR_FLOAT
	ps_mul 0, 0, 0
	lfs 1, 8(3)
	ps_madd 1, 1, 1, 0
	ps_sum0 1, 1, 0, 0
	blr
	.size PSVECSquareMag, . - PSVECSquareMag

	.globl PSVECMag
	.type PSVECMag, @function
PSVECMag:
	lfs 4, VecHalf@sda21(0)
	psq_l 0, 0(3), 0, GQR_FLOAT
	ps_mul 0, 0, 0
	lfs 1, 8(3)
	fsubs 2, 4, 4
	ps_madd 1, 1, 1, 0
	ps_sum0 1, 1, 0, 0
	fcmpu 0, 1, 2
	beq .Lmag_done
	frsqrte 0, 1
	lfs 3, VecThree@sda21(0)
	refine_rsqrt 0, 1, 4, 3, 2, 0
	fmuls 1, 1, 0

.Lmag_done:
	blr
	.size PSVECMag, . - PSVECMag

	.globl PSVECDotProduct
	.type PSVECDotProduct, @function
PSVECDotProduct:
	psq_l 2, 4(3), 0, GQR_FLOAT
	psq_l 3, 4(4), 0, GQR_FLOAT
	ps_mul 2, 2, 3
	psq_l 5, 0(3), 0, GQR_FLOAT
	psq_l 4, 0(4), 0, GQR_FLOAT
	ps_madd 3, 5, 4, 2
	ps_sum0 1, 3, 2, 2
	blr
	.size PSVECDotProduct, . - PSVECDotProduct

	.section .sdata2, "a"
	.balign 8
	.globl VecHalf
	.type VecHalf, @object
VecHalf:
	.float 0.5
	.size VecHalf, . - VecHalf

	.globl VecThree
	.type VecThree, @object
VecThree:
	.float 3.0
	.size VecThree, . - VecThree

	.elseif VEC_PAIRED_SECTION == VECTOR_CROSS_PRODUCT

	.globl PSVECCrossProduct
	.type PSVECCrossProduct, @function
PSVECCrossProduct:
	psq_l 1, 0(4), 0, GQR_FLOAT
	lfs 2, 8(3)
	psq_l 0, 0(3), 0, GQR_FLOAT
	ps_merge10 6, 1, 1
	lfs 3, 8(4)

	# Build (-Y, X) in f5 and -Z in the second lane of f8.
	ps_mul 4, 1, 2
	ps_muls0 7, 1, 0
	ps_msub 5, 0, 3, 4
	ps_msub 8, 0, 6, 7
	ps_merge11 9, 5, 5
	ps_merge01 10, 5, 8
	psq_st 9, 0(5), 1, GQR_FLOAT
	ps_neg 10, 10
	psq_st 10, 4(5), 0, GQR_FLOAT
	blr
	.size PSVECCrossProduct, . - PSVECCrossProduct

	.elseif VEC_PAIRED_SECTION == VECTOR_DISTANCE

	.globl PSVECDistance
	.type PSVECDistance, @function
PSVECDistance:
	psq_l 0, 4(3), 0, GQR_FLOAT
	psq_l 1, 4(4), 0, GQR_FLOAT
	ps_sub 2, 0, 1
	psq_l 0, 0(3), 0, GQR_FLOAT
	psq_l 1, 0(4), 0, GQR_FLOAT
	ps_mul 2, 2, 2
	ps_sub 0, 0, 1
	lfs 3, VecHalf@sda21(0)
	ps_madd 1, 0, 0, 2
	fsubs 0, 3, 3
	ps_sum0 1, 1, 2, 2
	fcmpu 0, 0, 1
	beq .Ldistance_done
	lfs 4, VecThree@sda21(0)
	frsqrte 0, 1
	refine_rsqrt 0, 1, 3, 4, 2, 0
	fmuls 1, 1, 0

.Ldistance_done:
	blr
	.size PSVECDistance, . - PSVECDistance
	.else
	.error "Unknown paired vector split"
	.endif

	# Gekko paired-single routines for the vector library. The scalar C
	# routines separate the three text runs in the Revolution SDK link order.
	.set VECTOR_ARITHMETIC, 0
	.set VECTOR_CROSS_PRODUCT, 1
	.set VECTOR_DISTANCE, 2
	.set GQR_FLOAT, 0

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
	ps_muls0 1, 2, 1
	psq_st 0, 0(4), 0, GQR_FLOAT
	psq_st 1, 8(4), 1, GQR_FLOAT
	blr
	.size PSVECScale, . - PSVECScale

	.globl PSVECNormalize
	.type PSVECNormalize, @function
PSVECNormalize:
	psq_l 2, 0(3), 0, GQR_FLOAT
	psq_l 3, 8(3), 1, GQR_FLOAT
	ps_mul 5, 2, 2
	lfs 0, VecHalf@sda21(0)
	lfs 1, VecThree@sda21(0)
	ps_madd 4, 3, 3, 5
	ps_sum0 4, 4, 3, 5
	frsqrte 5, 4
	fmuls 6, 5, 5
	fmuls 0, 5, 0
	fnmsubs 6, 6, 4, 1
	fmuls 5, 6, 0
	ps_muls0 2, 2, 5
	ps_muls0 3, 3, 5
	psq_st 2, 0(4), 0, GQR_FLOAT
	psq_st 3, 8(4), 1, GQR_FLOAT
	blr
	.size PSVECNormalize, . - PSVECNormalize

	.globl PSVECSquareMag
	.type PSVECSquareMag, @function
PSVECSquareMag:
	psq_l 0, 0(3), 0, GQR_FLOAT
	lfs 1, 8(3)
	ps_mul 0, 0, 0
	ps_madd 1, 1, 1, 0
	ps_sum0 1, 1, 0, 0
	blr
	.size PSVECSquareMag, . - PSVECSquareMag

	.globl PSVECMag
	.type PSVECMag, @function
PSVECMag:
	psq_l 0, 0(3), 0, GQR_FLOAT
	lfs 4, VecHalf@sda21(0)
	ps_mul 0, 0, 0
	lfs 1, 8(3)
	fsubs 2, 4, 4
	ps_madd 1, 1, 1, 0
	ps_sum0 1, 1, 0, 0
	fcmpu 0, 1, 2
	beqlr
	frsqrte 0, 1
	lfs 3, VecThree@sda21(0)
	fmuls 2, 0, 0
	fmuls 0, 0, 4
	fnmsubs 2, 2, 1, 3
	fmuls 0, 2, 0
	fmuls 1, 1, 0
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

	.globl PSVECSquareDistance
	.type PSVECSquareDistance, @function
PSVECSquareDistance:
	psq_l 0, 4(3), 0, GQR_FLOAT
	psq_l 1, 4(4), 0, GQR_FLOAT
	psq_l 2, 0(3), 0, GQR_FLOAT
	ps_sub 3, 0, 1
	psq_l 0, 0(4), 0, GQR_FLOAT
	ps_sub 0, 2, 0
	ps_mul 3, 3, 3
	ps_madd 1, 0, 0, 3
	ps_sum0 1, 1, 3, 3
	blr
	.size PSVECSquareDistance, . - PSVECSquareDistance

	.globl PSVECDistance
	.type PSVECDistance, @function
PSVECDistance:
	psq_l 0, 4(3), 0, GQR_FLOAT
	psq_l 1, 4(4), 0, GQR_FLOAT
	lfs 3, VecHalf@sda21(0)
	ps_sub 2, 0, 1
	psq_l 0, 0(3), 0, GQR_FLOAT
	psq_l 1, 0(4), 0, GQR_FLOAT
	fsubs 4, 3, 3
	ps_mul 2, 2, 2
	ps_sub 0, 0, 1
	ps_madd 1, 0, 0, 2
	ps_sum0 1, 1, 2, 2
	fcmpu 0, 4, 1
	beqlr
	frsqrte 0, 1
	lfs 4, VecThree@sda21(0)
	fmuls 2, 0, 0
	fmuls 0, 0, 3
	fnmsubs 2, 2, 1, 4
	fmuls 0, 2, 0
	fmuls 1, 1, 0
	blr
	.size PSVECDistance, . - PSVECDistance
	.else
	.error "Unknown paired vector split"
	.endif

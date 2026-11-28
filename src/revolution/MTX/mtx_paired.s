	# Gekko paired-single routines for the affine 3 x 4 matrix library.
	# The two C angle wrappers separate the text runs in the SDK link order.
	.set MTX_BASIS, 0
	.set MTX_ROTATION, 1
	.set MTX_AFFINE, 2
	.set GQR_FLOAT, 0

	.text
	.balign 4

	.if MTX_PAIRED_SECTION == MTX_BASIS

	# Initialize the affine basis and clear its translation column.
	.globl PSMTXIdentity
	.type PSMTXIdentity, @function
PSMTXIdentity:
	lfs 0, MatrixZero@sda21(0)
	lfs 1, MatrixOne@sda21(0)
	psq_st 0, 8(3), 0, GQR_FLOAT
	ps_merge01 2, 0, 1
	psq_st 0, 24(3), 0, GQR_FLOAT
	ps_merge10 1, 1, 0
	psq_st 0, 32(3), 0, GQR_FLOAT
	psq_st 2, 16(3), 0, GQR_FLOAT
	psq_st 1, 0(3), 0, GQR_FLOAT
	psq_st 1, 40(3), 0, GQR_FLOAT
	blr
	.size PSMTXIdentity, . - PSMTXIdentity

	# Copy each consecutive float pair directly. The buffers can be identical.
	.globl PSMTXCopy
	.type PSMTXCopy, @function
PSMTXCopy:
	psq_l 0, 0(3), 0, GQR_FLOAT
	psq_st 0, 0(4), 0, GQR_FLOAT
	psq_l 1, 8(3), 0, GQR_FLOAT
	psq_st 1, 8(4), 0, GQR_FLOAT
	psq_l 2, 16(3), 0, GQR_FLOAT
	psq_st 2, 16(4), 0, GQR_FLOAT
	psq_l 3, 24(3), 0, GQR_FLOAT
	psq_st 3, 24(4), 0, GQR_FLOAT
	psq_l 4, 32(3), 0, GQR_FLOAT
	psq_st 4, 32(4), 0, GQR_FLOAT
	psq_l 5, 40(3), 0, GQR_FLOAT
	psq_st 5, 40(4), 0, GQR_FLOAT
	blr
	.size PSMTXCopy, . - PSMTXCopy

	# Multiply two affine matrices. All inputs are loaded before the first output store.
	.globl PSMTXConcat
	.type PSMTXConcat, @function
PSMTXConcat:
	stwu 1, -64(1)
	psq_l 0, 0(3), 0, GQR_FLOAT
	stfd 14, 8(1)
	psq_l 6, 0(4), 0, GQR_FLOAT
	lis 6, MatrixAffineColumn@ha
	psq_l 7, 8(4), 0, GQR_FLOAT
	stfd 15, 16(1)
	addi 6, 6, MatrixAffineColumn@l
	stfd 31, 40(1)
	psq_l 8, 16(4), 0, GQR_FLOAT
	# Evaluate basis products two columns at a time.
	ps_muls0 12, 6, 0
	psq_l 2, 16(3), 0, GQR_FLOAT
	ps_muls0 13, 7, 0
	psq_l 31, 0(6), 0, GQR_FLOAT
	ps_muls0 14, 6, 2
	psq_l 9, 24(4), 0, GQR_FLOAT
	ps_muls0 15, 7, 2
	psq_l 1, 8(3), 0, GQR_FLOAT
	ps_madds1 12, 8, 0, 12
	psq_l 3, 24(3), 0, GQR_FLOAT
	ps_madds1 14, 8, 2, 14
	psq_l 10, 32(4), 0, GQR_FLOAT
	ps_madds1 13, 9, 0, 13
	psq_l 11, 40(4), 0, GQR_FLOAT
	ps_madds1 15, 9, 2, 15
	psq_l 4, 32(3), 0, GQR_FLOAT
	psq_l 5, 40(3), 0, GQR_FLOAT
	ps_madds0 12, 10, 1, 12
	ps_madds0 13, 11, 1, 13
	ps_madds0 14, 10, 3, 14
	ps_madds0 15, 11, 3, 15
	psq_st 12, 0(5), 0, GQR_FLOAT
	ps_muls0 2, 6, 4
	# Add the contribution from the left matrix translation to the fourth column.
	ps_madds1 13, 31, 1, 13
	ps_muls0 0, 7, 4
	psq_st 14, 16(5), 0, GQR_FLOAT
	ps_madds1 15, 31, 3, 15
	psq_st 13, 8(5), 0, GQR_FLOAT
	ps_madds1 2, 8, 4, 2
	ps_madds1 0, 9, 4, 0
	ps_madds0 2, 10, 5, 2
	lfd 14, 8(1)
	psq_st 15, 24(5), 0, GQR_FLOAT
	ps_madds0 0, 11, 5, 0
	psq_st 2, 32(5), 0, GQR_FLOAT
	ps_madds1 0, 31, 5, 0
	lfd 15, 16(1)
	psq_st 0, 40(5), 0, GQR_FLOAT
	lfd 31, 40(1)
	addi 1, 1, 64
	blr
	.size PSMTXConcat, . - PSMTXConcat

	# Multiply one matrix by a run of matrices. The first output is started before the loop.
	.globl PSMTXConcatArray
	.type PSMTXConcatArray, @function
PSMTXConcatArray:
	stwu 1, -96(1)
	stfd 31, 80(1)
	psq_st 31, 88(1), 0, GQR_FLOAT
	stfd 30, 64(1)
	psq_st 30, 72(1), 0, GQR_FLOAT
	stfd 29, 48(1)
	psq_st 29, 56(1), 0, GQR_FLOAT
	stfd 28, 32(1)
	psq_st 28, 40(1), 0, GQR_FLOAT
	stfd 27, 16(1)
	psq_st 27, 24(1), 0, GQR_FLOAT
	subi 0, 6, 1
	psq_l 0, 0(3), 0, GQR_FLOAT
	psq_l 1, 8(3), 0, GQR_FLOAT
	lis 6, MatrixAffineColumn@ha
	addi 6, 6, MatrixAffineColumn@l
	psq_l 2, 16(3), 0, GQR_FLOAT
	psq_l 3, 24(3), 0, GQR_FLOAT
	psq_l 4, 32(3), 0, GQR_FLOAT
	psq_l 5, 40(3), 0, GQR_FLOAT
	mtctr 0
	psq_l 6, 0(4), 0, GQR_FLOAT
	psq_l 7, 8(4), 0, GQR_FLOAT
	ps_muls0 12, 6, 0
	psq_l 8, 16(4), 0, GQR_FLOAT
	ps_muls0 31, 6, 2
	psq_l 10, 32(4), 0, GQR_FLOAT
	ps_muls0 29, 6, 4
	psq_l 9, 24(4), 0, GQR_FLOAT
	ps_madds1 12, 8, 0, 12
	psq_l 27, 0(6), 0, GQR_FLOAT
	ps_madds1 31, 8, 2, 31
	psq_l 11, 40(4), 0, GQR_FLOAT
	ps_madds1 29, 8, 4, 29
	ps_muls0 13, 7, 0
	ps_muls0 30, 7, 2
	ps_muls0 28, 7, 4
	ps_madds0 12, 10, 1, 12
	ps_madds0 31, 10, 3, 31
	ps_madds0 29, 10, 5, 29
	ps_madds1 13, 9, 0, 13
	psq_st 12, 0(5), 0, GQR_FLOAT
	ps_madds1 30, 9, 2, 30
	ps_madds1 28, 9, 4, 28
	psq_st 31, 16(5), 0, GQR_FLOAT
.Lconcat_array_loop:
	ps_madds0 13, 11, 1, 13
	psq_l 6, 48(4), 0, GQR_FLOAT
	ps_madds0 30, 11, 3, 30
	psq_st 29, 32(5), 0, GQR_FLOAT
	ps_madds0 28, 11, 5, 28
	psq_l 8, 64(4), 0, GQR_FLOAT
	ps_madd 13, 27, 1, 13
	psq_l 10, 80(4), 0, GQR_FLOAT
	ps_muls0 12, 6, 0
	psq_l 7, 56(4), 0, GQR_FLOAT
	ps_muls0 31, 6, 2
	psq_l 9, 72(4), 0, GQR_FLOAT
	psq_st 13, 8(5), 0, GQR_FLOAT
	ps_madd 30, 27, 3, 30
	ps_madds1 12, 8, 0, 12
	psq_l 11, 88(4), 0, GQR_FLOAT
	ps_muls0 29, 6, 4
	addi 4, 4, 48
	psq_st 30, 24(5), 0, GQR_FLOAT
	ps_madd 28, 27, 5, 28
	ps_madds1 31, 8, 2, 31
	ps_madds1 29, 8, 4, 29
	psq_st 28, 40(5), 0, GQR_FLOAT
	ps_madds0 12, 10, 1, 12
	ps_muls0 13, 7, 0
	ps_muls0 30, 7, 2
	ps_muls0 28, 7, 4
	psq_st 12, 48(5), 0, GQR_FLOAT
	ps_madds0 31, 10, 3, 31
	ps_madds0 29, 10, 5, 29
	ps_madds1 13, 9, 0, 13
	psq_st 31, 64(5), 0, GQR_FLOAT
	ps_madds1 30, 9, 2, 30
	ps_madds1 28, 9, 4, 28
	addi 5, 5, 48
	bdnz .Lconcat_array_loop
	ps_madds0 13, 11, 1, 13
	psq_st 29, 32(5), 0, GQR_FLOAT
	ps_madds0 30, 11, 3, 30
	ps_madds0 28, 11, 5, 28
	ps_madd 13, 27, 1, 13
	ps_madd 30, 27, 3, 30
	ps_madd 28, 27, 5, 28
	psq_st 13, 8(5), 0, GQR_FLOAT
	psq_st 30, 24(5), 0, GQR_FLOAT
	psq_st 28, 40(5), 0, GQR_FLOAT
	psq_l 31, 88(1), 0, GQR_FLOAT
	lfd 31, 80(1)
	psq_l 30, 72(1), 0, GQR_FLOAT
	lfd 30, 64(1)
	psq_l 29, 56(1), 0, GQR_FLOAT
	lfd 29, 48(1)
	psq_l 28, 40(1), 0, GQR_FLOAT
	lfd 28, 32(1)
	psq_l 27, 24(1), 0, GQR_FLOAT
	lfd 27, 16(1)
	addi 1, 1, 96
	blr
	.size PSMTXConcatArray, . - PSMTXConcatArray

	# Transpose only the 3 x 3 basis and clear the translation column.
	.globl PSMTXTranspose
	.type PSMTXTranspose, @function
PSMTXTranspose:
	lfs 0, MatrixZero@sda21(0)
	psq_l 1, 0(3), 0, GQR_FLOAT
	stfs 0, 44(4)
	psq_l 2, 16(3), 0, GQR_FLOAT
	ps_merge00 4, 1, 2
	psq_l 3, 8(3), 1, GQR_FLOAT
	ps_merge11 5, 1, 2
	psq_l 2, 24(3), 1, GQR_FLOAT
	psq_st 4, 0(4), 0, GQR_FLOAT
	psq_l 1, 32(3), 0, GQR_FLOAT
	ps_merge00 2, 3, 2
	psq_st 5, 16(4), 0, GQR_FLOAT
	ps_merge00 4, 1, 0
	psq_st 2, 32(4), 0, GQR_FLOAT
	ps_merge10 5, 1, 0
	psq_st 4, 8(4), 0, GQR_FLOAT
	lfs 3, 40(3)
	psq_st 5, 24(4), 0, GQR_FLOAT
	stfs 3, 40(4)
	blr
	.size PSMTXTranspose, . - PSMTXTranspose

	# Compute the cofactors and determinant, then invert the affine translation.
	.globl PSMTXInverse
	.type PSMTXInverse, @function
PSMTXInverse:
	psq_l 0, 0(3), 1, GQR_FLOAT
	psq_l 1, 4(3), 0, GQR_FLOAT
	psq_l 2, 16(3), 1, GQR_FLOAT
	ps_merge10 6, 1, 0
	psq_l 3, 20(3), 0, GQR_FLOAT
	psq_l 4, 32(3), 1, GQR_FLOAT
	ps_merge10 7, 3, 2
	psq_l 5, 36(3), 0, GQR_FLOAT
	ps_mul 11, 3, 6
	ps_mul 13, 5, 7
	ps_merge10 8, 5, 4
	ps_msub 11, 1, 7, 11
	ps_mul 12, 1, 8
	ps_msub 13, 3, 8, 13
	ps_mul 10, 3, 4
	ps_msub 12, 5, 6, 12
	ps_mul 9, 0, 5
	ps_mul 8, 1, 2
	ps_sub 6, 6, 6
	ps_msub 10, 2, 5, 10
	ps_mul 7, 0, 13
	ps_msub 9, 1, 4, 9
	ps_madd 7, 2, 12, 7
	ps_msub 8, 0, 3, 8
	ps_madd 7, 4, 11, 7
	# If the determinant vanishes, return before modifying the destination.
	ps_cmpo0 0, 7, 6
	bne .Linverse_regular
	li 3, 0
	blr
.Linverse_regular:
	# Refine the single-precision reciprocal before scaling the cofactors.
	fres 0, 7
	ps_add 6, 0, 0
	ps_mul 5, 0, 0
	ps_nmsub 0, 7, 5, 6
	lfs 1, 12(3)
	ps_muls0 13, 13, 0
	lfs 2, 28(3)
	ps_muls0 12, 12, 0
	lfs 3, 44(3)
	ps_muls0 11, 11, 0
	ps_merge00 5, 13, 12
	ps_muls0 10, 10, 0
	ps_merge11 4, 13, 12
	ps_muls0 9, 9, 0
	psq_st 5, 0(4), 0, GQR_FLOAT
	ps_mul 6, 13, 1
	psq_st 4, 16(4), 0, GQR_FLOAT
	ps_muls0 8, 8, 0
	ps_madd 6, 12, 2, 6
	psq_st 10, 32(4), 1, GQR_FLOAT
	ps_nmadd 6, 11, 3, 6
	psq_st 9, 36(4), 1, GQR_FLOAT
	ps_mul 7, 10, 1
	ps_merge00 5, 11, 6
	psq_st 8, 40(4), 1, GQR_FLOAT
	ps_merge11 4, 11, 6
	psq_st 5, 8(4), 0, GQR_FLOAT
	ps_madd 7, 9, 2, 7
	psq_st 4, 24(4), 0, GQR_FLOAT
	ps_nmadd 7, 8, 3, 7
	li 3, 1
	psq_st 7, 44(4), 1, GQR_FLOAT
	blr
	.size PSMTXInverse, . - PSMTXInverse

	# Keep the cofactor matrix in transposed layout and discard translation.
	.globl PSMTXInvXpose
	.type PSMTXInvXpose, @function
PSMTXInvXpose:
	psq_l 0, 0(3), 1, GQR_FLOAT
	psq_l 1, 4(3), 0, GQR_FLOAT
	psq_l 2, 16(3), 1, GQR_FLOAT
	ps_merge10 6, 1, 0
	psq_l 3, 20(3), 0, GQR_FLOAT
	psq_l 4, 32(3), 1, GQR_FLOAT
	ps_merge10 7, 3, 2
	psq_l 5, 36(3), 0, GQR_FLOAT
	ps_mul 11, 3, 6
	ps_merge10 8, 5, 4
	ps_mul 13, 5, 7
	ps_msub 11, 1, 7, 11
	ps_mul 12, 1, 8
	ps_msub 13, 3, 8, 13
	ps_msub 12, 5, 6, 12
	ps_mul 10, 3, 4
	ps_mul 9, 0, 5
	ps_mul 8, 1, 2
	ps_msub 10, 2, 5, 10
	ps_msub 9, 1, 4, 9
	ps_msub 8, 0, 3, 8
	ps_mul 7, 0, 13
	ps_sub 1, 1, 1
	ps_madd 7, 2, 12, 7
	ps_madd 7, 4, 11, 7
	# Leave the output untouched when the basis is singular.
	ps_cmpo0 0, 7, 1
	bne .Linverse_transpose_regular
	li 3, 0
	blr
.Linverse_transpose_regular:
	fres 0, 7
	psq_st 1, 12(4), 1, GQR_FLOAT
	ps_add 6, 0, 0
	ps_mul 5, 0, 0
	psq_st 1, 28(4), 1, GQR_FLOAT
	ps_nmsub 0, 7, 5, 6
	psq_st 1, 44(4), 1, GQR_FLOAT
	ps_muls0 13, 13, 0
	ps_muls0 12, 12, 0
	ps_muls0 11, 11, 0
	psq_st 13, 0(4), 0, GQR_FLOAT
	psq_st 12, 16(4), 0, GQR_FLOAT
	ps_muls0 10, 10, 0
	ps_muls0 9, 9, 0
	psq_st 11, 32(4), 0, GQR_FLOAT
	psq_st 10, 8(4), 1, GQR_FLOAT
	ps_muls0 8, 8, 0
	li 3, 1
	psq_st 9, 24(4), 1, GQR_FLOAT
	psq_st 8, 40(4), 1, GQR_FLOAT
	blr
	.size PSMTXInvXpose, . - PSMTXInvXpose

	.section .sdata, "aw", @progbits
	.balign 8
	.type MatrixAffineColumn, @object
MatrixAffineColumn:
	.float 0.0, 1.0
	.size MatrixAffineColumn, . - MatrixAffineColumn

	.section .sdata2, "a", @progbits
	.balign 8
	.globl MatrixOne
	.hidden MatrixOne
	.type MatrixOne, @object
MatrixOne:
	.float 1.0
	.size MatrixOne, . - MatrixOne
	.globl MatrixZero
	.hidden MatrixZero
	.type MatrixZero, @object
MatrixZero:
	.float 0.0
	.size MatrixZero, . - MatrixZero
	.globl MatrixHalf
	.hidden MatrixHalf
	.type MatrixHalf, @object
MatrixHalf:
	.float 0.5
	.size MatrixHalf, . - MatrixHalf
	.globl MatrixThree
	.hidden MatrixThree
	.type MatrixThree, @object
MatrixThree:
	.float 3.0
	.size MatrixThree, . - MatrixThree

	.elseif MTX_PAIRED_SECTION == MTX_ROTATION

	# Build a principal-axis rotation. Ignore an unrecognized axis.
	.globl PSMTXRotTrig
	.type PSMTXRotTrig, @function
PSMTXRotTrig:
	frsp 5, 1
	frsp 4, 2
	lfs 0, MatrixZero@sda21(0)
	lfs 1, MatrixOne@sda21(0)
	# Both uppercase and lowercase axis letters select the same rotation.
	ori 0, 4, 32
	ps_neg 2, 5
	cmplwi 0, 'x'
	beq .Lrotate_x
	cmplwi 0, 'y'
	beq .Lrotate_y
	cmplwi 0, 'z'
	beq .Lrotate_z
	b .Lrotation_done
.Lrotate_x:
	psq_st 1, 0(3), 1, GQR_FLOAT
	psq_st 0, 4(3), 0, GQR_FLOAT
	ps_merge00 3, 5, 4
	psq_st 0, 12(3), 0, GQR_FLOAT
	ps_merge00 1, 4, 2
	psq_st 0, 28(3), 0, GQR_FLOAT
	psq_st 0, 44(3), 1, GQR_FLOAT
	psq_st 3, 36(3), 0, GQR_FLOAT
	psq_st 1, 20(3), 0, GQR_FLOAT
	b .Lrotation_done
.Lrotate_y:
	ps_merge00 3, 4, 0
	ps_merge00 1, 0, 1
	psq_st 0, 24(3), 0, GQR_FLOAT
	psq_st 3, 0(3), 0, GQR_FLOAT
	ps_merge00 2, 2, 0
	ps_merge00 0, 5, 0
	psq_st 3, 40(3), 0, GQR_FLOAT
	psq_st 1, 16(3), 0, GQR_FLOAT
	psq_st 0, 8(3), 0, GQR_FLOAT
	psq_st 2, 32(3), 0, GQR_FLOAT
	b .Lrotation_done
.Lrotate_z:
	psq_st 0, 8(3), 0, GQR_FLOAT
	ps_merge00 3, 5, 4
	ps_merge00 2, 4, 2
	psq_st 0, 24(3), 0, GQR_FLOAT
	psq_st 0, 32(3), 0, GQR_FLOAT
	ps_merge00 1, 1, 0
	psq_st 3, 16(3), 0, GQR_FLOAT
	psq_st 2, 0(3), 0, GQR_FLOAT
	psq_st 1, 40(3), 0, GQR_FLOAT
.Lrotation_done:
	blr
	.size PSMTXRotTrig, . - PSMTXRotTrig

	# Normalize an arbitrary axis, then build its Rodrigues rotation matrix.
	.globl __PSMTXRotAxisRadInternal
	.hidden __PSMTXRotAxisRadInternal
	.type __PSMTXRotAxisRadInternal, @function
__PSMTXRotAxisRadInternal:
	lfs 10, MatrixHalf@sda21(0)
	lfs 9, MatrixThree@sda21(0)
	frsp 11, 2
	psq_l 2, 0(4), 0, GQR_FLOAT
	frsp 12, 1
	lfs 3, 8(4)
	ps_mul 4, 2, 2
	fadds 8, 10, 10
	ps_madd 5, 3, 3, 4
	fsubs 1, 10, 10
	ps_sum0 6, 5, 3, 4
	fsubs 0, 8, 11
	# Refine the reciprocal square root of the squared axis length.
	frsqrte 7, 6
	fmuls 4, 7, 7
	fmuls 5, 7, 10
	fnmsubs 4, 4, 6, 9
	fmuls 7, 4, 5
	ps_merge00 11, 11, 11
	ps_muls0 2, 2, 7
	ps_muls0 3, 3, 7
	# Combine the outer product, sine terms, and cosine diagonal.
	ps_muls0 6, 2, 0
	ps_muls0 10, 2, 12
	ps_muls0 7, 3, 0
	ps_muls1 5, 6, 2
	ps_muls0 4, 6, 2
	ps_muls0 6, 6, 3
	fnmsubs 0, 3, 12, 5
	fmadds 8, 3, 12, 5
	ps_neg 2, 10
	ps_sum0 9, 6, 1, 10
	ps_sum0 4, 4, 0, 11
	ps_sum1 5, 11, 8, 5
	ps_sum0 0, 2, 1, 6
	psq_st 9, 8(3), 0, GQR_FLOAT
	ps_sum0 2, 6, 6, 2
	psq_st 4, 0(3), 0, GQR_FLOAT
	ps_muls0 7, 7, 3
	psq_st 5, 16(3), 0, GQR_FLOAT
	ps_sum1 6, 10, 2, 6
	psq_st 0, 24(3), 0, GQR_FLOAT
	ps_sum0 7, 7, 1, 11
	psq_st 6, 32(3), 0, GQR_FLOAT
	psq_st 7, 40(3), 0, GQR_FLOAT
	blr
	.size __PSMTXRotAxisRadInternal, . - __PSMTXRotAxisRadInternal

	.elseif MTX_PAIRED_SECTION == MTX_AFFINE

	# Initialize the basis to identity and copy the separate translation components.
	.globl PSMTXTrans
	.type PSMTXTrans, @function
PSMTXTrans:
	lfs 0, MatrixZero@sda21(0)
	lfs 4, MatrixOne@sda21(0)
	stfs 1, 12(3)
	stfs 2, 28(3)
	psq_st 0, 4(3), 0, GQR_FLOAT
	psq_st 0, 32(3), 0, GQR_FLOAT
	stfs 0, 16(3)
	stfs 4, 20(3)
	stfs 0, 24(3)
	stfs 4, 40(3)
	stfs 3, 44(3)
	stfs 4, 0(3)
	blr
	.size PSMTXTrans, . - PSMTXTrans

	# Copy a matrix and add a separate translation vector.
	.globl PSMTXTransApply
	.type PSMTXTransApply, @function
PSMTXTransApply:
	psq_l 4, 0(3), 0, GQR_FLOAT
	frsp 1, 1
	psq_l 5, 8(3), 0, GQR_FLOAT
	frsp 2, 2
	psq_l 7, 24(3), 0, GQR_FLOAT
	frsp 3, 3
	psq_l 8, 40(3), 0, GQR_FLOAT
	psq_st 4, 0(4), 0, GQR_FLOAT
	ps_sum1 5, 1, 5, 5
	psq_l 6, 16(3), 0, GQR_FLOAT
	psq_st 5, 8(4), 0, GQR_FLOAT
	ps_sum1 7, 2, 7, 7
	psq_l 9, 32(3), 0, GQR_FLOAT
	psq_st 6, 16(4), 0, GQR_FLOAT
	ps_sum1 8, 3, 8, 8
	psq_st 7, 24(4), 0, GQR_FLOAT
	psq_st 9, 32(4), 0, GQR_FLOAT
	psq_st 8, 40(4), 0, GQR_FLOAT
	blr
	.size PSMTXTransApply, . - PSMTXTransApply

	# Build a diagonal scale matrix.
	.globl PSMTXScale
	.type PSMTXScale, @function
PSMTXScale:
	lfs 0, MatrixZero@sda21(0)
	stfs 1, 0(3)
	psq_st 0, 4(3), 0, GQR_FLOAT
	psq_st 0, 12(3), 0, GQR_FLOAT
	stfs 2, 20(3)
	psq_st 0, 24(3), 0, GQR_FLOAT
	psq_st 0, 32(3), 0, GQR_FLOAT
	stfs 3, 40(3)
	stfs 0, 44(3)
	blr
	.size PSMTXScale, . - PSMTXScale

	# Scale the rows of a matrix while copying it.
	.globl PSMTXScaleApply
	.type PSMTXScaleApply, @function
PSMTXScaleApply:
	frsp 1, 1
	psq_l 4, 0(3), 0, GQR_FLOAT
	frsp 2, 2
	psq_l 5, 8(3), 0, GQR_FLOAT
	frsp 3, 3
	ps_muls0 4, 4, 1
	psq_l 6, 16(3), 0, GQR_FLOAT
	ps_muls0 5, 5, 1
	psq_l 7, 24(3), 0, GQR_FLOAT
	ps_muls0 6, 6, 2
	psq_l 8, 32(3), 0, GQR_FLOAT
	psq_st 4, 0(4), 0, GQR_FLOAT
	ps_muls0 7, 7, 2
	psq_l 2, 40(3), 0, GQR_FLOAT
	psq_st 5, 8(4), 0, GQR_FLOAT
	ps_muls0 8, 8, 3
	psq_st 6, 16(4), 0, GQR_FLOAT
	ps_muls0 2, 2, 3
	psq_st 7, 24(4), 0, GQR_FLOAT
	psq_st 8, 32(4), 0, GQR_FLOAT
	psq_st 2, 40(4), 0, GQR_FLOAT
	blr
	.size PSMTXScaleApply, . - PSMTXScaleApply

	# Convert a quaternion to a matrix using a refined estimate of its inverse norm.
	.globl PSMTXQuat
	.type PSMTXQuat, @function
PSMTXQuat:
	lfs 1, MatrixOne@sda21(0)
	psq_l 4, 0(4), 0, GQR_FLOAT
	psq_l 5, 8(4), 0, GQR_FLOAT
	fsubs 0, 1, 1
	fadds 2, 1, 1
	ps_mul 6, 4, 4
	ps_merge10 9, 4, 4
	ps_madd 8, 5, 5, 6
	ps_mul 7, 5, 5
	ps_sum0 3, 8, 8, 8
	ps_muls1 10, 9, 5
	# Apply one reciprocal refinement to the quaternion squared length.
	fres 11, 3
	ps_sum1 8, 7, 8, 6
	ps_nmsub 3, 3, 11, 2
	ps_muls1 7, 5, 5
	ps_mul 3, 11, 3
	ps_sum0 6, 6, 6, 6
	fmuls 3, 3, 2
	ps_madd 11, 4, 9, 7
	ps_msub 7, 4, 9, 7
	psq_st 0, 12(3), 1, GQR_FLOAT
	ps_nmsub 6, 6, 3, 1
	ps_nmsub 8, 8, 3, 1
	psq_st 0, 44(3), 1, GQR_FLOAT
	ps_mul 11, 11, 3
	ps_mul 7, 7, 3
	psq_st 6, 40(3), 1, GQR_FLOAT
	ps_madds0 9, 4, 5, 10
	ps_merge00 5, 11, 8
	ps_nmsub 10, 10, 2, 9
	ps_merge10 4, 8, 7
	psq_st 5, 16(3), 0, GQR_FLOAT
	ps_mul 9, 9, 3
	ps_mul 10, 10, 3
	psq_st 4, 0(3), 0, GQR_FLOAT
	psq_st 9, 8(3), 1, GQR_FLOAT
	ps_merge10 7, 10, 0
	ps_merge01 11, 10, 9
	psq_st 7, 24(3), 0, GQR_FLOAT
	psq_st 11, 32(3), 0, GQR_FLOAT
	blr
	.size PSMTXQuat, . - PSMTXQuat

	# Build a plane reflection from an input point and normal.
	.globl PSMTXReflect
	.type PSMTXReflect, @function
PSMTXReflect:
	lfs 0, MatrixOne@sda21(0)
	psq_l 2, 8(5), 1, GQR_FLOAT
	psq_l 1, 0(5), 0, GQR_FLOAT
	psq_l 5, 0(4), 0, GQR_FLOAT
	ps_nmadd 4, 2, 0, 2
	psq_l 6, 8(4), 1, GQR_FLOAT
	ps_nmadd 3, 1, 0, 1
	ps_muls0 8, 1, 4
	# Form the reflection translation from twice the point-normal dot product.
	ps_mul 5, 3, 5
	ps_muls0 7, 1, 3
	ps_sum0 5, 5, 5, 5
	ps_muls1 3, 1, 3
	psq_st 8, 32(3), 0, GQR_FLOAT
	ps_sum0 7, 7, 7, 0
	ps_nmadd 5, 4, 6, 5
	ps_sum1 3, 0, 3, 3
	psq_st 7, 0(3), 0, GQR_FLOAT
	ps_muls0 1, 1, 5
	ps_merge00 4, 4, 5
	psq_st 3, 16(3), 0, GQR_FLOAT
	ps_merge00 3, 8, 1
	ps_muls0 4, 4, 2
	ps_merge11 1, 8, 1
	psq_st 3, 8(3), 0, GQR_FLOAT
	ps_sum0 4, 4, 4, 0
	psq_st 1, 24(3), 0, GQR_FLOAT
	psq_st 4, 40(3), 0, GQR_FLOAT
	blr
	.size PSMTXReflect, . - PSMTXReflect
	.else
	.error "Invalid matrix paired-single section"
	.endif

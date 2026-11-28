	.set GQR_FLOAT, 0

	.text
	.balign 4
	.globl PSMTX44Identity
	.type PSMTX44Identity, @function
PSMTX44Identity:
	lfs 1, Mtx44Zero@sda21(0)
	lfs 0, Mtx44One@sda21(0)
	psq_st 1, 4(3), 0, GQR_FLOAT
	stfs 0, 0(3)
	psq_st 1, 12(3), 0, GQR_FLOAT
	stfs 0, 20(3)
	psq_st 1, 24(3), 0, GQR_FLOAT
	psq_st 1, 32(3), 0, GQR_FLOAT
	stfs 0, 40(3)
	psq_st 1, 44(3), 0, GQR_FLOAT
	psq_st 1, 52(3), 0, GQR_FLOAT
	stfs 0, 60(3)
	blr
	.size PSMTX44Identity, . - PSMTX44Identity

	.balign 4
	.globl PSMTX44Copy
	.type PSMTX44Copy, @function
PSMTX44Copy:
	psq_l 1, 0(3), 0, GQR_FLOAT
	psq_st 1, 0(4), 0, GQR_FLOAT
	psq_l 1, 8(3), 0, GQR_FLOAT
	psq_st 1, 8(4), 0, GQR_FLOAT
	psq_l 1, 16(3), 0, GQR_FLOAT
	psq_st 1, 16(4), 0, GQR_FLOAT
	psq_l 1, 24(3), 0, GQR_FLOAT
	psq_st 1, 24(4), 0, GQR_FLOAT
	psq_l 1, 32(3), 0, GQR_FLOAT
	psq_st 1, 32(4), 0, GQR_FLOAT
	psq_l 1, 40(3), 0, GQR_FLOAT
	psq_st 1, 40(4), 0, GQR_FLOAT
	psq_l 1, 48(3), 0, GQR_FLOAT
	psq_st 1, 48(4), 0, GQR_FLOAT
	psq_l 1, 56(3), 0, GQR_FLOAT
	psq_st 1, 56(4), 0, GQR_FLOAT
	blr
	.size PSMTX44Copy, . - PSMTX44Copy

	.balign 4
	.globl PSMTX44Concat
	.type PSMTX44Concat, @function
PSMTX44Concat:
	psq_l 0, 0(3), 0, GQR_FLOAT
	psq_l 2, 0(4), 0, GQR_FLOAT
	ps_muls0 6, 2, 0
	psq_l 3, 16(4), 0, GQR_FLOAT
	psq_l 4, 32(4), 0, GQR_FLOAT
	ps_madds1 6, 3, 0, 6
	psq_l 1, 8(3), 0, GQR_FLOAT
	psq_l 5, 48(4), 0, GQR_FLOAT
	ps_madds0 6, 4, 1, 6
	psq_l 0, 16(3), 0, GQR_FLOAT
	ps_madds1 6, 5, 1, 6
	psq_l 1, 24(3), 0, GQR_FLOAT
	ps_muls0 8, 2, 0
	ps_madds1 8, 3, 0, 8
	psq_l 0, 32(3), 0, GQR_FLOAT
	ps_madds0 8, 4, 1, 8
	ps_madds1 8, 5, 1, 8
	psq_l 1, 40(3), 0, GQR_FLOAT
	ps_muls0 10, 2, 0
	ps_madds1 10, 3, 0, 10
	psq_l 0, 48(3), 0, GQR_FLOAT
	ps_madds0 10, 4, 1, 10
	ps_madds1 10, 5, 1, 10
	psq_l 1, 56(3), 0, GQR_FLOAT
	ps_muls0 12, 2, 0
	psq_l 2, 8(4), 0, GQR_FLOAT
	ps_madds1 12, 3, 0, 12
	psq_l 0, 0(3), 0, GQR_FLOAT
	ps_madds0 12, 4, 1, 12
	psq_l 3, 24(4), 0, GQR_FLOAT
	ps_madds1 12, 5, 1, 12
	psq_l 1, 8(3), 0, GQR_FLOAT
	ps_muls0 7, 2, 0
	psq_l 4, 40(4), 0, GQR_FLOAT
	ps_madds1 7, 3, 0, 7
	psq_l 5, 56(4), 0, GQR_FLOAT
	ps_madds0 7, 4, 1, 7
	psq_l 0, 16(3), 0, GQR_FLOAT
	ps_madds1 7, 5, 1, 7
	psq_l 1, 24(3), 0, GQR_FLOAT
	ps_muls0 9, 2, 0
	psq_st 6, 0(5), 0, GQR_FLOAT
	ps_madds1 9, 3, 0, 9
	psq_l 0, 32(3), 0, GQR_FLOAT
	ps_madds0 9, 4, 1, 9
	psq_st 8, 16(5), 0, GQR_FLOAT
	ps_madds1 9, 5, 1, 9
	psq_l 1, 40(3), 0, GQR_FLOAT
	ps_muls0 11, 2, 0
	psq_st 10, 32(5), 0, GQR_FLOAT
	ps_madds1 11, 3, 0, 11
	psq_l 0, 48(3), 0, GQR_FLOAT
	ps_madds0 11, 4, 1, 11
	psq_st 12, 48(5), 0, GQR_FLOAT
	ps_madds1 11, 5, 1, 11
	psq_l 1, 56(3), 0, GQR_FLOAT
	ps_muls0 13, 2, 0
	psq_st 7, 8(5), 0, GQR_FLOAT
	ps_madds1 13, 3, 0, 13
	psq_st 9, 24(5), 0, GQR_FLOAT
	ps_madds0 13, 4, 1, 13
	psq_st 11, 40(5), 0, GQR_FLOAT
	ps_madds1 13, 5, 1, 13
	psq_st 13, 56(5), 0, GQR_FLOAT
	blr
	.size PSMTX44Concat, . - PSMTX44Concat

	.section .sdata2, "a", @progbits
	.balign 8
	.type Mtx44One, @object
Mtx44One:
	.float 1.0
	.size Mtx44One, . - Mtx44One
	.type Mtx44Zero, @object
Mtx44Zero:
	.float 0.0
	.size Mtx44Zero, . - Mtx44Zero

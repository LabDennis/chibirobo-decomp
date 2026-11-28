	.set GQR_FLOAT, 0

	.text
	.balign 4
	.globl __GXSetProjection
	.type __GXSetProjection, @function
__GXSetProjection:
	lis 4, -13311
	li 0, 16
	lis 3, 6
	stb 0, -32768(4)
	addi 0, 3, 4128
	lwz 3, __GXData@sda21(0)
	stw 0, -32768(4)
	addi 5, 4, -32768
	psq_l 2, 1324(3), 0, GQR_FLOAT
	psq_l 1, 1332(3), 0, GQR_FLOAT
	psq_l 0, 1340(3), 0, GQR_FLOAT
	psq_st 2, 0(5), 0, GQR_FLOAT
	psq_st 1, 0(5), 0, GQR_FLOAT
	psq_st 0, 0(5), 0, GQR_FLOAT
	lwz 0, 1320(3)
	stw 0, -32768(4)
	blr
	.size __GXSetProjection, . - __GXSetProjection

	.balign 4
	.globl GXSetProjectionv
	.type GXSetProjectionv, @function
GXSetProjectionv:
	lfs 1, GXTransformZero@sda21(0)
	lfs 0, 0(3)
	fcmpu 0, 1, 0
	bne .Lset_projection_type_ortho
	li 0, 0
	b .Lset_projection_type_done
.Lset_projection_type_ortho:
	li 0, 1
.Lset_projection_type_done:
	lwz 4, __GXData@sda21(0)
	stw 0, 1320(4)
	psq_l 2, 4(3), 0, GQR_FLOAT
	psq_l 1, 12(3), 0, GQR_FLOAT
	psq_l 0, 20(3), 0, GQR_FLOAT
	psq_st 2, 1324(4), 0, GQR_FLOAT
	psq_st 1, 1332(4), 0, GQR_FLOAT
	psq_st 0, 1340(4), 0, GQR_FLOAT
	lwz 0, 1532(4)
	oris 0, 0, 2048
	stw 0, 1532(4)
	blr
	.size GXSetProjectionv, . - GXSetProjectionv

	.balign 4
	.globl GXGetProjectionv
	.type GXGetProjectionv, @function
GXGetProjectionv:
	lwz 4, __GXData@sda21(0)
	lwz 0, 1320(4)
	cmpwi 0, 0
	beq .Lget_projection_type_zero
	lfs 0, GXTransformOne@sda21(0)
	b .Lget_projection_type_store
.Lget_projection_type_zero:
	lfs 0, GXTransformZero@sda21(0)
.Lget_projection_type_store:
	stfs 0, 0(3)
	lwz 4, __GXData@sda21(0)
	psq_l 2, 1324(4), 0, GQR_FLOAT
	psq_l 1, 1332(4), 0, GQR_FLOAT
	psq_l 0, 1340(4), 0, GQR_FLOAT
	psq_st 2, 4(3), 0, GQR_FLOAT
	psq_st 1, 12(3), 0, GQR_FLOAT
	psq_st 0, 20(3), 0, GQR_FLOAT
	blr
	.size GXGetProjectionv, . - GXGetProjectionv

	.balign 4
	.globl GXLoadPosMtxImm
	.type GXLoadPosMtxImm, @function
GXLoadPosMtxImm:
	lis 5, -13311
	li 0, 16
	slwi 4, 4, 2
	stb 0, -32768(5)
	oris 0, 4, 11
	stw 0, -32768(5)
	addi 4, 5, -32768
	psq_l 5, 0(3), 0, GQR_FLOAT
	psq_l 4, 8(3), 0, GQR_FLOAT
	psq_l 3, 16(3), 0, GQR_FLOAT
	psq_l 2, 24(3), 0, GQR_FLOAT
	psq_l 1, 32(3), 0, GQR_FLOAT
	psq_l 0, 40(3), 0, GQR_FLOAT
	psq_st 5, 0(4), 0, GQR_FLOAT
	psq_st 4, 0(4), 0, GQR_FLOAT
	psq_st 3, 0(4), 0, GQR_FLOAT
	psq_st 2, 0(4), 0, GQR_FLOAT
	psq_st 1, 0(4), 0, GQR_FLOAT
	psq_st 0, 0(4), 0, GQR_FLOAT
	blr
	.size GXLoadPosMtxImm, . - GXLoadPosMtxImm

	.balign 4
	.globl GXLoadNrmMtxImm
	.type GXLoadNrmMtxImm, @function
GXLoadNrmMtxImm:
	slwi 0, 4, 2
	lis 5, -13311
	subf 4, 4, 0
	li 0, 16
	addi 4, 4, 1024
	stb 0, -32768(5)
	oris 0, 4, 8
	stw 0, -32768(5)
	addi 4, 5, -32768
	psq_l 5, 0(3), 0, GQR_FLOAT
	lfs 4, 8(3)
	psq_l 3, 16(3), 0, GQR_FLOAT
	lfs 2, 24(3)
	psq_l 1, 32(3), 0, GQR_FLOAT
	lfs 0, 40(3)
	psq_st 5, 0(4), 0, GQR_FLOAT
	stfs 4, -32768(5)
	psq_st 3, 0(4), 0, GQR_FLOAT
	stfs 2, -32768(5)
	psq_st 1, 0(4), 0, GQR_FLOAT
	stfs 0, -32768(5)
	blr
	.size GXLoadNrmMtxImm, . - GXLoadNrmMtxImm

	.balign 4
	.globl GXLoadTexMtxImm
	.type GXLoadTexMtxImm, @function
GXLoadTexMtxImm:
	cmplwi 4, 64
	blt .Ltex_mtx_base_done
	addi 0, 4, -64
	slwi 4, 0, 2
	addi 7, 4, 1280
	b .Ltex_mtx_count
.Ltex_mtx_base_done:
	slwi 7, 4, 2
.Ltex_mtx_count:
	cmpwi 5, 1
	bne .Ltex_mtx_count_12
	li 4, 8
	b .Ltex_mtx_count_done
.Ltex_mtx_count_12:
	li 4, 12
.Ltex_mtx_count_done:
	addi 6, 4, -1
	lis 4, -13311
	li 0, 16
	cmpwi 5, 0
	slwi 5, 6, 16
	stb 0, -32768(4)
	or 0, 7, 5
	stw 0, -32768(4)
	bne .Ltex_mtx_2x4
	addi 4, 4, -32768
	psq_l 5, 0(3), 0, GQR_FLOAT
	psq_l 4, 8(3), 0, GQR_FLOAT
	psq_l 3, 16(3), 0, GQR_FLOAT
	psq_l 2, 24(3), 0, GQR_FLOAT
	psq_l 1, 32(3), 0, GQR_FLOAT
	psq_l 0, 40(3), 0, GQR_FLOAT
	psq_st 5, 0(4), 0, GQR_FLOAT
	psq_st 4, 0(4), 0, GQR_FLOAT
	psq_st 3, 0(4), 0, GQR_FLOAT
	psq_st 2, 0(4), 0, GQR_FLOAT
	psq_st 1, 0(4), 0, GQR_FLOAT
	psq_st 0, 0(4), 0, GQR_FLOAT
	blr
.Ltex_mtx_2x4:
	addi 4, 4, -32768
	psq_l 3, 0(3), 0, GQR_FLOAT
	psq_l 2, 8(3), 0, GQR_FLOAT
	psq_l 1, 16(3), 0, GQR_FLOAT
	psq_l 0, 24(3), 0, GQR_FLOAT
	psq_st 3, 0(4), 0, GQR_FLOAT
	psq_st 2, 0(4), 0, GQR_FLOAT
	psq_st 1, 0(4), 0, GQR_FLOAT
	psq_st 0, 0(4), 0, GQR_FLOAT
	blr
	.size GXLoadTexMtxImm, . - GXLoadTexMtxImm

	.section .sdata2, "a", @progbits
	.balign 8
	.type GXTransformZero, @object
GXTransformZero:
	.float 0.0
	.size GXTransformZero, . - GXTransformZero
	.type GXTransformOne, @object
GXTransformOne:
	.float 1.0
	.size GXTransformOne, . - GXTransformOne

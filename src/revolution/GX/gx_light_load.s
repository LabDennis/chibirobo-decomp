	.set GQR_FLOAT, 0

	.text
	.balign 4
	.globl GXLoadLightObjImm
	.type GXLoadLightObjImm, @function
GXLoadLightObjImm:
	cntlzw 0, 4
	lis 4, -13311
	subfic 5, 0, 31
	li 0, 16
	rlwinm 5, 5, 4, 25, 27
	stb 0, -32768(4)
	addi 0, 5, 1536
	oris 0, 0, 15
	stw 0, -32768(4)
	lwz 0, 12(3)
	xor 6, 6, 6
	psq_l 5, 16(3), 0, GQR_FLOAT
	psq_l 4, 24(3), 0, GQR_FLOAT
	psq_l 3, 32(3), 0, GQR_FLOAT
	psq_l 2, 40(3), 0, GQR_FLOAT
	psq_l 1, 48(3), 0, GQR_FLOAT
	psq_l 0, 56(3), 0, GQR_FLOAT
	stwu 6, -32768(4)
	stw 6, 0(4)
	stw 6, 0(4)
	stw 0, 0(4)
	psq_st 5, 0(4), 0, GQR_FLOAT
	psq_st 4, 0(4), 0, GQR_FLOAT
	psq_st 3, 0(4), 0, GQR_FLOAT
	psq_st 2, 0(4), 0, GQR_FLOAT
	psq_st 1, 0(4), 0, GQR_FLOAT
	psq_st 0, 0(4), 0, GQR_FLOAT
	li 0, 1
	lwz 3, __GXData@sda21(0)
	sth 0, 2(3)
	blr
	.size GXLoadLightObjImm, . - GXLoadLightObjImm

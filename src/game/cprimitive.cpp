#include "game/cprimitive.h"
#include "game/ut.h"
#include "game/gmath.h"
#include <math.h>

extern "C" {
void HSD_StateInvalidate(s32 mask);
void GXDrawCylinder(u8 numEdges);
void GXDrawSphere(u8 numMajor, u8 numMinor);
void GXDrawCube(void);
}

#define HSD_STATE_VTX_ATTR 2

static inline void LoadNrmMtx(MtxP mtx)
{
	Mtx inv;
	Mtx nrm;

	PSMTXInverse(mtx, inv);
	PSMTXTranspose(inv, nrm);
	GXLoadNrmMtxImm(nrm, GX_PNMTX0);
}

// position and color, both direct. every caller shares one copy of these arrays, so
// it's an inline. retail's tile and circle draws use their own format with no null test,
// this check still costs them a compare. default first then an if beats a ternary
static inline void SetVtxPosClr(GXVtxAttrFmtList* fmt = NULL)
{
	GXVtxAttrFmtList defFmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};
	GXVtxDescList desc[] = {
		{ GX_VA_POS, GX_DIRECT },
		{ GX_VA_CLR0, GX_DIRECT },
		{ GX_VA_NULL, GX_NONE },
	};

	GXClearVtxDesc();
	GXVtxAttrFmtList* use = defFmt;
	if (fmt != NULL) {
		use = fmt;
	}
	GXSetVtxAttrFmtv(GX_VTXFMT0, use);
	GXSetVtxDescv(desc);
}

// position, normal and color, all direct
static inline void SetVtxPosNrmClr(GXVtxAttrFmtList* fmt = NULL)
{
	GXVtxAttrFmtList defFmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};
	GXVtxDescList desc[] = {
		{ GX_VA_POS, GX_DIRECT },
		{ GX_VA_NRM, GX_DIRECT },
		{ GX_VA_CLR0, GX_DIRECT },
		{ GX_VA_NULL, GX_NONE },
	};

	GXClearVtxDesc();
	GXVtxAttrFmtList* use = defFmt;
	if (fmt != NULL) {
		use = fmt;
	}
	GXSetVtxAttrFmtv(GX_VTXFMT0, use);
	GXSetVtxDescv(desc);
}

/**
 * @note Address: 0x8019E2A8
 * @note Size: 0x120
 */
void CPrimitive::Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	Set(tx, ty, tz, sx, sy, sz, rx, ry, rz);
}

/**
 * @note Address: 0x8019E188
 * @note Size: 0x120
 */
void CPrimitive::Set(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	SetTrans(tx, ty, tz);
	SetScale(sx, sy, sz);
	SetRot(rx, ry, rz);
	SetBillboard(0);
	mRotOrder = 4;
	mFixedMtx = NULL;
}

/**
 * @note Address: 0x8019DF5C
 * @note Size: 0x22C
 */
MtxP CPrimitive::LoadMtx(Mtx view)
{
	MtxP mtx;

	if (mFixedMtx != NULL) {
		mtx = mFixedMtx;
	} else {
		if (mDirty) {
			Mtx rot;
			MtxP trans = mTrans;
			MtxP scale = mScale;
			MtxP rotX  = mRotX;
			MtxP rotY  = mRotY;
			MtxP rotZ  = mRotZ;

			switch (mRotOrder) {
			case 0:
				PSMTXConcat(trans, rotX, rot);
				PSMTXConcat(rot, rotY, rot);
				PSMTXConcat(rot, rotZ, rot);
				break;
			case 1:
				PSMTXConcat(trans, rotX, rot);
				PSMTXConcat(rot, rotZ, rot);
				PSMTXConcat(rot, rotY, rot);
				break;
			case 2:
				PSMTXConcat(trans, rotY, rot);
				PSMTXConcat(rot, rotX, rot);
				PSMTXConcat(rot, rotZ, rot);
				break;
			case 3:
				PSMTXConcat(trans, rotY, rot);
				PSMTXConcat(rot, rotZ, rot);
				PSMTXConcat(rot, rotX, rot);
				break;
			case 4:
				PSMTXConcat(trans, rotZ, rot);
				PSMTXConcat(rot, rotX, rot);
				PSMTXConcat(rot, rotY, rot);
				break;
			case 5:
				PSMTXConcat(trans, rotZ, rot);
				PSMTXConcat(rot, rotY, rot);
				PSMTXConcat(rot, rotX, rot);
				break;
			}
			PSMTXConcat(rot, scale, mModel);
			PSMTXConcat(view, mModel, mModelView);
			if (mBillboard == 1) {
				BuildBillboardMatrixFromUp(mModelView, mModelView);
			} else if (mBillboard == 2) {
				BuildBillboardMatrixWithZRotation(mModelView, mModelView, mBillboardZ);
			}
		}
		mDirty = FALSE;
		mtx    = mModelView;
	}
	GXLoadPosMtxImm(mtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	return mtx;
}

/**
 * @note Address: 0x8019DCFC
 * @note Size: 0x260
 */
void CLine::Draw(Mtx view, Vec* pos, u32 color, u16 count)
{
	LoadMtx(view);
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosClr();
	GXBegin(GX_LINESTRIP, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXColor1u32(color);
	}
}
/*
    stwu     r1, -0x80(r1)
    mflr     r0
    stw      r0, 0x84(r1)
    stmw     r21, 0x54(r1)
    mr       r29, r5
    mr       r30, r6
    mr       r31, r7
    bl       fn_8019DF5C
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lis      r4, sPrimitivePosColorFormat@ha
    lwzu     r21, sPrimitivePosColorFormat@l(r4)
    lis      r3, sPrimitivePosColorDesc@ha
    addi     r8, r3, sPrimitivePosColorDesc@l
    lwz      r22, 0x4(r4)
    lwz      r23, 0x8(r4)
    lwz      r24, 0xc(r4)
    lwz      r25, 0x10(r4)
    lwz      r26, 0x14(r4)
    lwz      r27, 0x18(r4)
    lwz      r28, 0x1c(r4)
    lwz      r12, 0x20(r4)
    lwz      r11, 0x24(r4)
    lwz      r10, 0x28(r4)
    lwz      r9, 0x2c(r4)
    lwz      r7, 0x0(r8)
    lwz      r6, 0x4(r8)
    lwz      r5, 0x8(r8)
    lwz      r4, 0xc(r8)
    lwz      r3, 0x10(r8)
    lwz      r0, 0x14(r8)
    stw      r21, 0x20(r1)
    stw      r22, 0x24(r1)
    stw      r23, 0x28(r1)
    stw      r24, 0x2c(r1)
    stw      r25, 0x30(r1)
    stw      r26, 0x34(r1)
    stw      r27, 0x38(r1)
    stw      r28, 0x3c(r1)
    stw      r12, 0x40(r1)
    stw      r11, 0x44(r1)
    stw      r10, 0x48(r1)
    stw      r9, 0x4c(r1)
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    stw      r5, 0x10(r1)
    stw      r4, 0x14(r1)
    stw      r3, 0x18(r1)
    stw      r0, 0x1c(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x20
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x8
    bl       GXSetVtxDescv
    mr       r5, r31
    li       r3, 0xb0
    li       r4, 0x0
    bl       GXBegin
    clrlwi.  r6, r31, 16
    li       r7, 0x0
    ble      .L_8019DF48
    cmpwi    r6, 0x8
    subi     r4, r6, 0x8
    ble      .L_8019DF08
    addi     r0, r4, 0x7
    mr       r5, r29
    srwi     r0, r0, 3
    lis      r3, 0xcc01
    mtctr    r0
    cmpwi    r4, 0x0
    ble      .L_8019DF08
.L_8019DE1C:
    lfs      f0, 0x0(r5)
    addi     r7, r7, 0x8
    lfs      f1, 0x4(r5)
    stfs     f0, -0x8000(r3)
    lfs      f2, 0x8(r5)
    stfs     f1, -0x8000(r3)
    lfs      f0, 0xc(r5)
    stfs     f2, -0x8000(r3)
    lfs      f2, 0x10(r5)
    stw      r30, -0x8000(r3)
    lfs      f1, 0x14(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x18(r5)
    stfs     f2, -0x8000(r3)
    lfs      f3, 0x1c(r5)
    stfs     f1, -0x8000(r3)
    lfs      f2, 0x20(r5)
    stw      r30, -0x8000(r3)
    lfs      f1, 0x24(r5)
    stfs     f0, -0x8000(r3)
    lfs      f6, 0x28(r5)
    stfs     f3, -0x8000(r3)
    lfs      f4, 0x2c(r5)
    stfs     f2, -0x8000(r3)
    lfs      f0, 0x30(r5)
    stw      r30, -0x8000(r3)
    lfs      f5, 0x34(r5)
    stfs     f1, -0x8000(r3)
    lfs      f3, 0x38(r5)
    stfs     f6, -0x8000(r3)
    lfs      f2, 0x3c(r5)
    stfs     f4, -0x8000(r3)
    lfs      f7, 0x40(r5)
    stw      r30, -0x8000(r3)
    lfs      f4, 0x44(r5)
    stfs     f0, -0x8000(r3)
    lfs      f1, 0x48(r5)
    stfs     f5, -0x8000(r3)
    lfs      f6, 0x4c(r5)
    stfs     f3, -0x8000(r3)
    lfs      f3, 0x50(r5)
    stw      r30, -0x8000(r3)
    lfs      f0, 0x54(r5)
    stfs     f2, -0x8000(r3)
    lfs      f5, 0x58(r5)
    stfs     f7, -0x8000(r3)
    lfs      f2, 0x5c(r5)
    addi     r5, r5, 0x60
    stfs     f4, -0x8000(r3)
    stw      r30, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f6, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    stw      r30, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stw      r30, -0x8000(r3)
    bdnz     .L_8019DE1C
.L_8019DF08:
    mulli    r4, r7, 0xc
    subf     r0, r7, r6
    lis      r3, 0xcc01
    add      r4, r29, r4
    mtctr    r0
    cmpw     r7, r6
    bge      .L_8019DF48
.L_8019DF24:
    lfs      f0, 0x0(r4)
    lfs      f1, 0x4(r4)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x8(r4)
    addi     r4, r4, 0xc
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stw      r30, -0x8000(r3)
    bdnz     .L_8019DF24
.L_8019DF48:
    lmw      r21, 0x54(r1)
    lwz      r0, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0x80
    blr
*/

/**
 * @note Address: 0x8019DA64
 * @note Size: 0x298
 */
void CLine::Draw(Mtx view, Vec* pos, u32* color, u16 count)
{
	LoadMtx(view);
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosClr();
	GXBegin(GX_LINESTRIP, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXColor1u32(color[i]);
	}
}
/*
    stwu     r1, -0x80(r1)
    mflr     r0
    stw      r0, 0x84(r1)
    stmw     r21, 0x54(r1)
    mr       r29, r5
    mr       r30, r6
    mr       r31, r7
    bl       fn_8019DF5C
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lis      r4, sPrimitivePosColorFormat@ha
    lwzu     r21, sPrimitivePosColorFormat@l(r4)
    lis      r3, sPrimitivePosColorDesc@ha
    addi     r8, r3, sPrimitivePosColorDesc@l
    lwz      r22, 0x4(r4)
    lwz      r23, 0x8(r4)
    lwz      r24, 0xc(r4)
    lwz      r25, 0x10(r4)
    lwz      r26, 0x14(r4)
    lwz      r27, 0x18(r4)
    lwz      r28, 0x1c(r4)
    lwz      r12, 0x20(r4)
    lwz      r11, 0x24(r4)
    lwz      r10, 0x28(r4)
    lwz      r9, 0x2c(r4)
    lwz      r7, 0x0(r8)
    lwz      r6, 0x4(r8)
    lwz      r5, 0x8(r8)
    lwz      r4, 0xc(r8)
    lwz      r3, 0x10(r8)
    lwz      r0, 0x14(r8)
    stw      r21, 0x20(r1)
    stw      r22, 0x24(r1)
    stw      r23, 0x28(r1)
    stw      r24, 0x2c(r1)
    stw      r25, 0x30(r1)
    stw      r26, 0x34(r1)
    stw      r27, 0x38(r1)
    stw      r28, 0x3c(r1)
    stw      r12, 0x40(r1)
    stw      r11, 0x44(r1)
    stw      r10, 0x48(r1)
    stw      r9, 0x4c(r1)
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    stw      r5, 0x10(r1)
    stw      r4, 0x14(r1)
    stw      r3, 0x18(r1)
    stw      r0, 0x1c(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x20
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x8
    bl       GXSetVtxDescv
    mr       r5, r31
    li       r3, 0xb0
    li       r4, 0x0
    bl       GXBegin
    clrlwi.  r7, r31, 16
    li       r8, 0x0
    ble      .L_8019DCE8
    cmpwi    r7, 0x8
    subi     r3, r7, 0x8
    ble      .L_8019DC98
    addi     r0, r3, 0x7
    mr       r5, r29
    srwi     r0, r0, 3
    mr       r6, r30
    lis      r4, 0xcc01
    mtctr    r0
    cmpwi    r3, 0x0
    ble      .L_8019DC98
.L_8019DB88:
    lfs      f0, 0x0(r5)
    addi     r8, r8, 0x8
    lfs      f1, 0x4(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x8(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0x0(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0xc(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x10(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x14(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0x4(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x18(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x1c(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x20(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0x8(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x24(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x28(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x2c(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0xc(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x30(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x34(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x38(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0x10(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x3c(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x40(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x44(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0x14(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x48(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x4c(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x50(r5)
    stfs     f1, -0x8000(r4)
    lwz      r3, 0x18(r6)
    stfs     f0, -0x8000(r4)
    lwz      r0, 0x1c(r6)
    addi     r6, r6, 0x20
    stw      r3, -0x8000(r4)
    lfs      f0, 0x54(r5)
    lfs      f1, 0x58(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x5c(r5)
    addi     r5, r5, 0x60
    stfs     f1, -0x8000(r4)
    stfs     f0, -0x8000(r4)
    stw      r0, -0x8000(r4)
    bdnz     .L_8019DB88
.L_8019DC98:
    mulli    r4, r8, 0xc
    slwi     r3, r8, 2
    subf     r0, r8, r7
    add      r5, r30, r3
    add      r4, r29, r4
    lis      r3, 0xcc01
    mtctr    r0
    cmpw     r8, r7
    bge      .L_8019DCE8
.L_8019DCBC:
    lfs      f0, 0x0(r4)
    lwz      r0, 0x0(r5)
    addi     r5, r5, 0x4
    stfs     f0, -0x8000(r3)
    lfs      f1, 0x4(r4)
    lfs      f0, 0x8(r4)
    addi     r4, r4, 0xc
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stw      r0, -0x8000(r3)
    bdnz     .L_8019DCBC
.L_8019DCE8:
    lmw      r21, 0x54(r1)
    lwz      r0, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0x80
    blr
*/

/**
 * @note Address: 0x8019D7E4
 * @note Size: 0x280
 */
void CLine::Draw(Mtx view, PrimVertex* vtx, u16 count)
{
	LoadMtx(view);
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosClr();
	GXBegin(GX_LINESTRIP, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(vtx[i].pos.x, vtx[i].pos.y, vtx[i].pos.z);
		GXColor1u32(vtx[i].color);
	}
}
/*
    stwu     r1, -0x80(r1)
    mflr     r0
    stw      r0, 0x84(r1)
    stmw     r22, 0x58(r1)
    mr       r30, r5
    mr       r31, r6
    bl       fn_8019DF5C
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lis      r4, sPrimitivePosColorFormat@ha
    lwzu     r22, sPrimitivePosColorFormat@l(r4)
    lis      r3, sPrimitivePosColorDesc@ha
    addi     r8, r3, sPrimitivePosColorDesc@l
    lwz      r23, 0x4(r4)
    lwz      r24, 0x8(r4)
    lwz      r25, 0xc(r4)
    lwz      r26, 0x10(r4)
    lwz      r27, 0x14(r4)
    lwz      r28, 0x18(r4)
    lwz      r29, 0x1c(r4)
    lwz      r12, 0x20(r4)
    lwz      r11, 0x24(r4)
    lwz      r10, 0x28(r4)
    lwz      r9, 0x2c(r4)
    lwz      r7, 0x0(r8)
    lwz      r6, 0x4(r8)
    lwz      r5, 0x8(r8)
    lwz      r4, 0xc(r8)
    lwz      r3, 0x10(r8)
    lwz      r0, 0x14(r8)
    stw      r22, 0x20(r1)
    stw      r23, 0x24(r1)
    stw      r24, 0x28(r1)
    stw      r25, 0x2c(r1)
    stw      r26, 0x30(r1)
    stw      r27, 0x34(r1)
    stw      r28, 0x38(r1)
    stw      r29, 0x3c(r1)
    stw      r12, 0x40(r1)
    stw      r11, 0x44(r1)
    stw      r10, 0x48(r1)
    stw      r9, 0x4c(r1)
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    stw      r5, 0x10(r1)
    stw      r4, 0x14(r1)
    stw      r3, 0x18(r1)
    stw      r0, 0x1c(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x20
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x8
    bl       GXSetVtxDescv
    mr       r5, r31
    li       r3, 0xb0
    li       r4, 0x0
    bl       GXBegin
    clrlwi.  r6, r31, 16
    li       r7, 0x0
    ble      .L_8019DA50
    cmpwi    r6, 0x8
    subi     r4, r6, 0x8
    ble      .L_8019DA0C
    addi     r0, r4, 0x7
    mr       r5, r30
    srwi     r0, r0, 3
    lis      r3, 0xcc01
    mtctr    r0
    cmpwi    r4, 0x0
    ble      .L_8019DA0C
.L_8019D900:
    lfs      f0, 0x0(r5)
    addi     r7, r7, 0x8
    lfs      f1, 0x4(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x8(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0xc(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x10(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x14(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x18(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x1c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x20(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x24(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x28(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x2c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x30(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x34(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x38(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x3c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x40(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x44(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x48(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x4c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x50(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x54(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x58(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x5c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x60(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x64(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x68(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x6c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x70(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x74(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x78(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x7c(r5)
    addi     r5, r5, 0x80
    stfs     f0, -0x8000(r3)
    stw      r0, -0x8000(r3)
    bdnz     .L_8019D900
.L_8019DA0C:
    slwi     r3, r7, 4
    subf     r0, r7, r6
    add      r4, r30, r3
    lis      r3, 0xcc01
    mtctr    r0
    cmpw     r7, r6
    bge      .L_8019DA50
.L_8019DA28:
    lfs      f0, 0x0(r4)
    lfs      f1, 0x4(r4)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x8(r4)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0xc(r4)
    addi     r4, r4, 0x10
    stfs     f0, -0x8000(r3)
    stw      r0, -0x8000(r3)
    bdnz     .L_8019DA28
.L_8019DA50:
    lmw      r22, 0x58(r1)
    lwz      r0, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0x80
    blr
*/

/**
 * @note Address: 0x8019D580
 * @note Size: 0x264
 */
void CTriangle::Draw(Mtx view, Vec* pos, u32 color, u16 count, GXPrimitive type)
{
	LoadMtx(view);
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosClr();
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXColor1u32(color);
	}
}
/*
    stwu     r1, -0x80(r1)
    mflr     r0
    stw      r0, 0x84(r1)
    stmw     r20, 0x50(r1)
    mr       r28, r5
    mr       r29, r6
    mr       r30, r7
    mr       r31, r8
    bl       fn_8019DF5C
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lis      r4, sPrimitivePosColorFormat@ha
    lwzu     r20, sPrimitivePosColorFormat@l(r4)
    lis      r3, sPrimitivePosColorDesc@ha
    addi     r8, r3, sPrimitivePosColorDesc@l
    lwz      r21, 0x4(r4)
    lwz      r22, 0x8(r4)
    lwz      r23, 0xc(r4)
    lwz      r24, 0x10(r4)
    lwz      r25, 0x14(r4)
    lwz      r26, 0x18(r4)
    lwz      r27, 0x1c(r4)
    lwz      r12, 0x20(r4)
    lwz      r11, 0x24(r4)
    lwz      r10, 0x28(r4)
    lwz      r9, 0x2c(r4)
    lwz      r7, 0x0(r8)
    lwz      r6, 0x4(r8)
    lwz      r5, 0x8(r8)
    lwz      r4, 0xc(r8)
    lwz      r3, 0x10(r8)
    lwz      r0, 0x14(r8)
    stw      r20, 0x20(r1)
    stw      r21, 0x24(r1)
    stw      r22, 0x28(r1)
    stw      r23, 0x2c(r1)
    stw      r24, 0x30(r1)
    stw      r25, 0x34(r1)
    stw      r26, 0x38(r1)
    stw      r27, 0x3c(r1)
    stw      r12, 0x40(r1)
    stw      r11, 0x44(r1)
    stw      r10, 0x48(r1)
    stw      r9, 0x4c(r1)
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    stw      r5, 0x10(r1)
    stw      r4, 0x14(r1)
    stw      r3, 0x18(r1)
    stw      r0, 0x1c(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x20
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x8
    bl       GXSetVtxDescv
    mr       r3, r31
    mr       r5, r30
    li       r4, 0x0
    bl       GXBegin
    clrlwi.  r6, r30, 16
    li       r7, 0x0
    ble      .L_8019D7D0
    cmpwi    r6, 0x8
    subi     r4, r6, 0x8
    ble      .L_8019D790
    addi     r0, r4, 0x7
    mr       r5, r28
    srwi     r0, r0, 3
    lis      r3, 0xcc01
    mtctr    r0
    cmpwi    r4, 0x0
    ble      .L_8019D790
.L_8019D6A4:
    lfs      f0, 0x0(r5)
    addi     r7, r7, 0x8
    lfs      f1, 0x4(r5)
    stfs     f0, -0x8000(r3)
    lfs      f2, 0x8(r5)
    stfs     f1, -0x8000(r3)
    lfs      f0, 0xc(r5)
    stfs     f2, -0x8000(r3)
    lfs      f2, 0x10(r5)
    stw      r29, -0x8000(r3)
    lfs      f1, 0x14(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x18(r5)
    stfs     f2, -0x8000(r3)
    lfs      f3, 0x1c(r5)
    stfs     f1, -0x8000(r3)
    lfs      f2, 0x20(r5)
    stw      r29, -0x8000(r3)
    lfs      f1, 0x24(r5)
    stfs     f0, -0x8000(r3)
    lfs      f6, 0x28(r5)
    stfs     f3, -0x8000(r3)
    lfs      f4, 0x2c(r5)
    stfs     f2, -0x8000(r3)
    lfs      f0, 0x30(r5)
    stw      r29, -0x8000(r3)
    lfs      f5, 0x34(r5)
    stfs     f1, -0x8000(r3)
    lfs      f3, 0x38(r5)
    stfs     f6, -0x8000(r3)
    lfs      f2, 0x3c(r5)
    stfs     f4, -0x8000(r3)
    lfs      f7, 0x40(r5)
    stw      r29, -0x8000(r3)
    lfs      f4, 0x44(r5)
    stfs     f0, -0x8000(r3)
    lfs      f1, 0x48(r5)
    stfs     f5, -0x8000(r3)
    lfs      f6, 0x4c(r5)
    stfs     f3, -0x8000(r3)
    lfs      f3, 0x50(r5)
    stw      r29, -0x8000(r3)
    lfs      f0, 0x54(r5)
    stfs     f2, -0x8000(r3)
    lfs      f5, 0x58(r5)
    stfs     f7, -0x8000(r3)
    lfs      f2, 0x5c(r5)
    addi     r5, r5, 0x60
    stfs     f4, -0x8000(r3)
    stw      r29, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f6, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    stw      r29, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stw      r29, -0x8000(r3)
    bdnz     .L_8019D6A4
.L_8019D790:
    mulli    r4, r7, 0xc
    subf     r0, r7, r6
    lis      r3, 0xcc01
    add      r4, r28, r4
    mtctr    r0
    cmpw     r7, r6
    bge      .L_8019D7D0
.L_8019D7AC:
    lfs      f0, 0x0(r4)
    lfs      f1, 0x4(r4)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x8(r4)
    addi     r4, r4, 0xc
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stw      r29, -0x8000(r3)
    bdnz     .L_8019D7AC
.L_8019D7D0:
    lmw      r20, 0x50(r1)
    lwz      r0, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0x80
    blr
*/

/**
 * @note Address: 0x8019D2FC
 * @note Size: 0x284
 */
void CTriangle::DrawLit(Mtx view, Vec* pos, u32 color, u16 count, GXPrimitive type)
{
	Vec normal;

	LoadNrmMtx(LoadMtx(view));
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosNrmClr();
	CalcTriangleNormal(&pos[0], &pos[1], &pos[2], &normal);
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXNormal3f32(normal.x, normal.y, normal.z);
		GXColor1u32(color);
	}
}
/*
    stwu     r1, -0x120(r1)
    mflr     r0
    stw      r0, 0x124(r1)
    stmw     r15, 0xdc(r1)
    mr       r28, r5
    mr       r29, r6
    mr       r30, r7
    mr       r31, r8
    bl       fn_8019DF5C
    addi     r4, r1, 0x34
    bl       PSMTXInverse
    addi     r3, r1, 0x34
    addi     r4, r1, 0x64
    bl       PSMTXTranspose
    addi     r3, r1, 0x64
    li       r4, 0x0
    bl       GXLoadNrmMtxImm
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lis      r4, sPrimitivePosNormalColorFormat@ha
    lwzu     r15, sPrimitivePosNormalColorFormat@l(r4)
    lis      r3, sPrimitivePosNormalColorDesc@ha
    addi     r9, r3, sPrimitivePosNormalColorDesc@l
    lwz      r16, 0x4(r4)
    lwz      r17, 0x8(r4)
    lwz      r18, 0xc(r4)
    lwz      r19, 0x10(r4)
    lwz      r20, 0x14(r4)
    lwz      r21, 0x18(r4)
    lwz      r22, 0x1c(r4)
    lwz      r23, 0x20(r4)
    lwz      r24, 0x24(r4)
    lwz      r25, 0x28(r4)
    lwz      r26, 0x2c(r4)
    lwz      r27, 0x30(r4)
    lwz      r12, 0x34(r4)
    lwz      r11, 0x38(r4)
    lwz      r10, 0x3c(r4)
    lwz      r8, 0x0(r9)
    lwz      r7, 0x4(r9)
    lwz      r6, 0x8(r9)
    lwz      r5, 0xc(r9)
    lwz      r4, 0x10(r9)
    lwz      r3, 0x14(r9)
    lwz      r0, 0x18(r9)
    lwz      r9, 0x1c(r9)
    stw      r15, 0x94(r1)
    stw      r16, 0x98(r1)
    stw      r17, 0x9c(r1)
    stw      r18, 0xa0(r1)
    stw      r19, 0xa4(r1)
    stw      r20, 0xa8(r1)
    stw      r21, 0xac(r1)
    stw      r22, 0xb0(r1)
    stw      r23, 0xb4(r1)
    stw      r24, 0xb8(r1)
    stw      r25, 0xbc(r1)
    stw      r26, 0xc0(r1)
    stw      r27, 0xc4(r1)
    stw      r12, 0xc8(r1)
    stw      r11, 0xcc(r1)
    stw      r10, 0xd0(r1)
    stw      r8, 0x14(r1)
    stw      r7, 0x18(r1)
    stw      r6, 0x1c(r1)
    stw      r5, 0x20(r1)
    stw      r4, 0x24(r1)
    stw      r3, 0x28(r1)
    stw      r0, 0x2c(r1)
    stw      r9, 0x30(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x94
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x14
    bl       GXSetVtxDescv
    mr       r3, r28
    addi     r4, r28, 0xc
    addi     r5, r28, 0x18
    addi     r6, r1, 0x8
    bl       CalcTriangleNormal
    mr       r3, r31
    mr       r5, r30
    li       r4, 0x0
    bl       GXBegin
    clrlwi.  r5, r30, 16
    lfs      f4, 0xc(r1)
    lfs      f5, 0x10(r1)
    mr       r4, r28
    lis      r3, 0xcc01
    ble      .L_8019D56C
    srwi.    r0, r5, 2
    mtctr    r0
    beq      .L_8019D534
.L_8019D474:
    lfs      f1, 0x0(r4)
    lfs      f2, 0x4(r4)
    stfs     f1, -0x8000(r3)
    lfs      f3, 0x8(r4)
    stfs     f2, -0x8000(r3)
    lfs      f1, 0xc(r4)
    stfs     f3, -0x8000(r3)
    lfs      f2, 0x10(r4)
    lfs      f0, 0x8(r1)
    lfs      f3, 0x14(r4)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r29, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    lfs      f1, 0x18(r4)
    stfs     f2, -0x8000(r3)
    lfs      f2, 0x1c(r4)
    stfs     f3, -0x8000(r3)
    lfs      f3, 0x20(r4)
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r29, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    lfs      f1, 0x24(r4)
    stfs     f2, -0x8000(r3)
    lfs      f2, 0x28(r4)
    stfs     f3, -0x8000(r3)
    lfs      f3, 0x2c(r4)
    addi     r4, r4, 0x30
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r29, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r29, -0x8000(r3)
    bdnz     .L_8019D474
    andi.    r5, r5, 0x3
    beq      .L_8019D56C
.L_8019D534:
    mtctr    r5
.L_8019D538:
    lfs      f1, 0x0(r4)
    lfs      f2, 0x4(r4)
    stfs     f1, -0x8000(r3)
    lfs      f3, 0x8(r4)
    addi     r4, r4, 0xc
    stfs     f2, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r29, -0x8000(r3)
    bdnz     .L_8019D538
.L_8019D56C:
    lmw      r15, 0xdc(r1)
    lwz      r0, 0x124(r1)
    mtlr     r0
    addi     r1, r1, 0x120
    blr
*/

/**
 * @note Address: 0x8019D060
 * @note Size: 0x29C
 */
void CTriangle::Draw(Mtx view, Vec* pos, u32* color, u16 count, GXPrimitive type)
{
	LoadMtx(view);
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosClr();
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXColor1u32(color[i]);
	}
}
/*
    stwu     r1, -0x80(r1)
    mflr     r0
    stw      r0, 0x84(r1)
    stmw     r20, 0x50(r1)
    mr       r28, r5
    mr       r29, r6
    mr       r30, r7
    mr       r31, r8
    bl       fn_8019DF5C
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lis      r4, sPrimitivePosColorFormat@ha
    lwzu     r20, sPrimitivePosColorFormat@l(r4)
    lis      r3, sPrimitivePosColorDesc@ha
    addi     r8, r3, sPrimitivePosColorDesc@l
    lwz      r21, 0x4(r4)
    lwz      r22, 0x8(r4)
    lwz      r23, 0xc(r4)
    lwz      r24, 0x10(r4)
    lwz      r25, 0x14(r4)
    lwz      r26, 0x18(r4)
    lwz      r27, 0x1c(r4)
    lwz      r12, 0x20(r4)
    lwz      r11, 0x24(r4)
    lwz      r10, 0x28(r4)
    lwz      r9, 0x2c(r4)
    lwz      r7, 0x0(r8)
    lwz      r6, 0x4(r8)
    lwz      r5, 0x8(r8)
    lwz      r4, 0xc(r8)
    lwz      r3, 0x10(r8)
    lwz      r0, 0x14(r8)
    stw      r20, 0x20(r1)
    stw      r21, 0x24(r1)
    stw      r22, 0x28(r1)
    stw      r23, 0x2c(r1)
    stw      r24, 0x30(r1)
    stw      r25, 0x34(r1)
    stw      r26, 0x38(r1)
    stw      r27, 0x3c(r1)
    stw      r12, 0x40(r1)
    stw      r11, 0x44(r1)
    stw      r10, 0x48(r1)
    stw      r9, 0x4c(r1)
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    stw      r5, 0x10(r1)
    stw      r4, 0x14(r1)
    stw      r3, 0x18(r1)
    stw      r0, 0x1c(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x20
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x8
    bl       GXSetVtxDescv
    mr       r3, r31
    mr       r5, r30
    li       r4, 0x0
    bl       GXBegin
    clrlwi.  r7, r30, 16
    li       r8, 0x0
    ble      .L_8019D2E8
    cmpwi    r7, 0x8
    subi     r3, r7, 0x8
    ble      .L_8019D298
    addi     r0, r3, 0x7
    mr       r5, r28
    srwi     r0, r0, 3
    mr       r6, r29
    lis      r4, 0xcc01
    mtctr    r0
    cmpwi    r3, 0x0
    ble      .L_8019D298
.L_8019D188:
    lfs      f0, 0x0(r5)
    addi     r8, r8, 0x8
    lfs      f1, 0x4(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x8(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0x0(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0xc(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x10(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x14(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0x4(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x18(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x1c(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x20(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0x8(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x24(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x28(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x2c(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0xc(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x30(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x34(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x38(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0x10(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x3c(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x40(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x44(r5)
    stfs     f1, -0x8000(r4)
    lwz      r0, 0x14(r6)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x48(r5)
    stw      r0, -0x8000(r4)
    lfs      f1, 0x4c(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x50(r5)
    stfs     f1, -0x8000(r4)
    lwz      r3, 0x18(r6)
    stfs     f0, -0x8000(r4)
    lwz      r0, 0x1c(r6)
    addi     r6, r6, 0x20
    stw      r3, -0x8000(r4)
    lfs      f0, 0x54(r5)
    lfs      f1, 0x58(r5)
    stfs     f0, -0x8000(r4)
    lfs      f0, 0x5c(r5)
    addi     r5, r5, 0x60
    stfs     f1, -0x8000(r4)
    stfs     f0, -0x8000(r4)
    stw      r0, -0x8000(r4)
    bdnz     .L_8019D188
.L_8019D298:
    mulli    r4, r8, 0xc
    slwi     r3, r8, 2
    subf     r0, r8, r7
    add      r5, r29, r3
    add      r4, r28, r4
    lis      r3, 0xcc01
    mtctr    r0
    cmpw     r8, r7
    bge      .L_8019D2E8
.L_8019D2BC:
    lfs      f0, 0x0(r4)
    lwz      r0, 0x0(r5)
    addi     r5, r5, 0x4
    stfs     f0, -0x8000(r3)
    lfs      f1, 0x4(r4)
    lfs      f0, 0x8(r4)
    addi     r4, r4, 0xc
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stw      r0, -0x8000(r3)
    bdnz     .L_8019D2BC
.L_8019D2E8:
    lmw      r20, 0x50(r1)
    lwz      r0, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0x80
    blr
*/

/**
 * @note Address: 0x8019CDBC
 * @note Size: 0x2A4
 */
void CTriangle::DrawLit(Mtx view, Vec* pos, u32* color, u16 count, GXPrimitive type)
{
	Vec normal;

	LoadNrmMtx(LoadMtx(view));
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosNrmClr();
	CalcTriangleNormal(&pos[0], &pos[1], &pos[2], &normal);
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXNormal3f32(normal.x, normal.y, normal.z);
		GXColor1u32(color[i]);
	}
}
/*
    stwu     r1, -0x120(r1)
    mflr     r0
    stw      r0, 0x124(r1)
    stmw     r15, 0xdc(r1)
    mr       r28, r5
    mr       r29, r6
    mr       r30, r7
    mr       r31, r8
    bl       fn_8019DF5C
    addi     r4, r1, 0x34
    bl       PSMTXInverse
    addi     r3, r1, 0x34
    addi     r4, r1, 0x64
    bl       PSMTXTranspose
    addi     r3, r1, 0x64
    li       r4, 0x0
    bl       GXLoadNrmMtxImm
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lis      r4, sPrimitivePosNormalColorFormat@ha
    lwzu     r15, sPrimitivePosNormalColorFormat@l(r4)
    lis      r3, sPrimitivePosNormalColorDesc@ha
    addi     r9, r3, sPrimitivePosNormalColorDesc@l
    lwz      r16, 0x4(r4)
    lwz      r17, 0x8(r4)
    lwz      r18, 0xc(r4)
    lwz      r19, 0x10(r4)
    lwz      r20, 0x14(r4)
    lwz      r21, 0x18(r4)
    lwz      r22, 0x1c(r4)
    lwz      r23, 0x20(r4)
    lwz      r24, 0x24(r4)
    lwz      r25, 0x28(r4)
    lwz      r26, 0x2c(r4)
    lwz      r27, 0x30(r4)
    lwz      r12, 0x34(r4)
    lwz      r11, 0x38(r4)
    lwz      r10, 0x3c(r4)
    lwz      r8, 0x0(r9)
    lwz      r7, 0x4(r9)
    lwz      r6, 0x8(r9)
    lwz      r5, 0xc(r9)
    lwz      r4, 0x10(r9)
    lwz      r3, 0x14(r9)
    lwz      r0, 0x18(r9)
    lwz      r9, 0x1c(r9)
    stw      r15, 0x94(r1)
    stw      r16, 0x98(r1)
    stw      r17, 0x9c(r1)
    stw      r18, 0xa0(r1)
    stw      r19, 0xa4(r1)
    stw      r20, 0xa8(r1)
    stw      r21, 0xac(r1)
    stw      r22, 0xb0(r1)
    stw      r23, 0xb4(r1)
    stw      r24, 0xb8(r1)
    stw      r25, 0xbc(r1)
    stw      r26, 0xc0(r1)
    stw      r27, 0xc4(r1)
    stw      r12, 0xc8(r1)
    stw      r11, 0xcc(r1)
    stw      r10, 0xd0(r1)
    stw      r8, 0x14(r1)
    stw      r7, 0x18(r1)
    stw      r6, 0x1c(r1)
    stw      r5, 0x20(r1)
    stw      r4, 0x24(r1)
    stw      r3, 0x28(r1)
    stw      r0, 0x2c(r1)
    stw      r9, 0x30(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x94
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x14
    bl       GXSetVtxDescv
    mr       r3, r28
    addi     r4, r28, 0xc
    addi     r5, r28, 0x18
    addi     r6, r1, 0x8
    bl       CalcTriangleNormal
    mr       r3, r31
    mr       r5, r30
    li       r4, 0x0
    bl       GXBegin
    clrlwi.  r6, r30, 16
    lfs      f4, 0xc(r1)
    lfs      f5, 0x10(r1)
    mr       r4, r28
    mr       r5, r29
    lis      r3, 0xcc01
    ble      .L_8019D04C
    srwi.    r0, r6, 2
    mtctr    r0
    beq      .L_8019D00C
.L_8019CF38:
    lfs      f1, 0x0(r4)
    lfs      f2, 0x4(r4)
    stfs     f1, -0x8000(r3)
    lfs      f3, 0x8(r4)
    stfs     f2, -0x8000(r3)
    lwz      r0, 0x0(r5)
    stfs     f3, -0x8000(r3)
    lfs      f1, 0xc(r4)
    lfs      f0, 0x8(r1)
    lfs      f2, 0x10(r4)
    stfs     f0, -0x8000(r3)
    lfs      f3, 0x14(r4)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r0, -0x8000(r3)
    lwz      r0, 0x4(r5)
    stfs     f1, -0x8000(r3)
    lfs      f1, 0x18(r4)
    stfs     f2, -0x8000(r3)
    lfs      f2, 0x1c(r4)
    stfs     f3, -0x8000(r3)
    lfs      f3, 0x20(r4)
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r0, -0x8000(r3)
    lwz      r0, 0x8(r5)
    stfs     f1, -0x8000(r3)
    lfs      f1, 0x24(r4)
    stfs     f2, -0x8000(r3)
    lfs      f2, 0x28(r4)
    stfs     f3, -0x8000(r3)
    lfs      f3, 0x2c(r4)
    addi     r4, r4, 0x30
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r0, -0x8000(r3)
    lwz      r0, 0xc(r5)
    addi     r5, r5, 0x10
    stfs     f1, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r0, -0x8000(r3)
    bdnz     .L_8019CF38
    andi.    r6, r6, 0x3
    beq      .L_8019D04C
.L_8019D00C:
    mtctr    r6
.L_8019D010:
    lfs      f1, 0x0(r4)
    lwz      r0, 0x0(r5)
    addi     r5, r5, 0x4
    stfs     f1, -0x8000(r3)
    lfs      f2, 0x4(r4)
    lfs      f3, 0x8(r4)
    addi     r4, r4, 0xc
    stfs     f2, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r0, -0x8000(r3)
    bdnz     .L_8019D010
.L_8019D04C:
    lmw      r15, 0xdc(r1)
    lwz      r0, 0x124(r1)
    mtlr     r0
    addi     r1, r1, 0x120
    blr
*/

/**
 * @note Address: 0x8019CB38
 * @note Size: 0x284
 */
void CTriangle::Draw(Mtx view, PrimVertex* vtx, u16 count, GXPrimitive type)
{
	LoadMtx(view);
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosClr();
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(vtx[i].pos.x, vtx[i].pos.y, vtx[i].pos.z);
		GXColor1u32(vtx[i].color);
	}
}
/*
    stwu     r1, -0x80(r1)
    mflr     r0
    stw      r0, 0x84(r1)
    stmw     r21, 0x54(r1)
    mr       r29, r5
    mr       r30, r6
    mr       r31, r7
    bl       fn_8019DF5C
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lis      r4, sPrimitivePosColorFormat@ha
    lwzu     r21, sPrimitivePosColorFormat@l(r4)
    lis      r3, sPrimitivePosColorDesc@ha
    addi     r8, r3, sPrimitivePosColorDesc@l
    lwz      r22, 0x4(r4)
    lwz      r23, 0x8(r4)
    lwz      r24, 0xc(r4)
    lwz      r25, 0x10(r4)
    lwz      r26, 0x14(r4)
    lwz      r27, 0x18(r4)
    lwz      r28, 0x1c(r4)
    lwz      r12, 0x20(r4)
    lwz      r11, 0x24(r4)
    lwz      r10, 0x28(r4)
    lwz      r9, 0x2c(r4)
    lwz      r7, 0x0(r8)
    lwz      r6, 0x4(r8)
    lwz      r5, 0x8(r8)
    lwz      r4, 0xc(r8)
    lwz      r3, 0x10(r8)
    lwz      r0, 0x14(r8)
    stw      r21, 0x20(r1)
    stw      r22, 0x24(r1)
    stw      r23, 0x28(r1)
    stw      r24, 0x2c(r1)
    stw      r25, 0x30(r1)
    stw      r26, 0x34(r1)
    stw      r27, 0x38(r1)
    stw      r28, 0x3c(r1)
    stw      r12, 0x40(r1)
    stw      r11, 0x44(r1)
    stw      r10, 0x48(r1)
    stw      r9, 0x4c(r1)
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    stw      r5, 0x10(r1)
    stw      r4, 0x14(r1)
    stw      r3, 0x18(r1)
    stw      r0, 0x1c(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x20
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x8
    bl       GXSetVtxDescv
    mr       r3, r31
    mr       r5, r30
    li       r4, 0x0
    bl       GXBegin
    clrlwi.  r6, r30, 16
    li       r7, 0x0
    ble      .L_8019CDA8
    cmpwi    r6, 0x8
    subi     r4, r6, 0x8
    ble      .L_8019CD64
    addi     r0, r4, 0x7
    mr       r5, r29
    srwi     r0, r0, 3
    lis      r3, 0xcc01
    mtctr    r0
    cmpwi    r4, 0x0
    ble      .L_8019CD64
.L_8019CC58:
    lfs      f0, 0x0(r5)
    addi     r7, r7, 0x8
    lfs      f1, 0x4(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x8(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0xc(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x10(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x14(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x18(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x1c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x20(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x24(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x28(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x2c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x30(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x34(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x38(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x3c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x40(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x44(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x48(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x4c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x50(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x54(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x58(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x5c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x60(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x64(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x68(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x6c(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x70(r5)
    stw      r0, -0x8000(r3)
    lfs      f1, 0x74(r5)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x78(r5)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0x7c(r5)
    addi     r5, r5, 0x80
    stfs     f0, -0x8000(r3)
    stw      r0, -0x8000(r3)
    bdnz     .L_8019CC58
.L_8019CD64:
    slwi     r3, r7, 4
    subf     r0, r7, r6
    add      r4, r29, r3
    lis      r3, 0xcc01
    mtctr    r0
    cmpw     r7, r6
    bge      .L_8019CDA8
.L_8019CD80:
    lfs      f0, 0x0(r4)
    lfs      f1, 0x4(r4)
    stfs     f0, -0x8000(r3)
    lfs      f0, 0x8(r4)
    stfs     f1, -0x8000(r3)
    lwz      r0, 0xc(r4)
    addi     r4, r4, 0x10
    stfs     f0, -0x8000(r3)
    stw      r0, -0x8000(r3)
    bdnz     .L_8019CD80
.L_8019CDA8:
    lmw      r21, 0x54(r1)
    lwz      r0, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0x80
    blr
*/

/**
 * @note Address: 0x8019C8A4
 * @note Size: 0x294
 */
void CTriangle::DrawLit(Mtx view, PrimVertex* vtx, u16 count, GXPrimitive type)
{
	Vec normal;

	LoadNrmMtx(LoadMtx(view));
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosNrmClr();
	CalcTriangleNormal(&vtx[0].pos, &vtx[1].pos, &vtx[2].pos, &normal);
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(vtx[i].pos.x, vtx[i].pos.y, vtx[i].pos.z);
		GXNormal3f32(normal.x, normal.y, normal.z);
		GXColor1u32(vtx[i].color);
	}
}
/*
    stwu     r1, -0x120(r1)
    mflr     r0
    stw      r0, 0x124(r1)
    stmw     r15, 0xdc(r1)
    mr       r29, r5
    mr       r30, r6
    mr       r31, r7
    bl       fn_8019DF5C
    addi     r4, r1, 0x34
    bl       PSMTXInverse
    addi     r3, r1, 0x34
    addi     r4, r1, 0x64
    bl       PSMTXTranspose
    addi     r3, r1, 0x64
    li       r4, 0x0
    bl       GXLoadNrmMtxImm
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lis      r4, sPrimitivePosNormalColorFormat@ha
    lwzu     r15, sPrimitivePosNormalColorFormat@l(r4)
    lis      r3, sPrimitivePosNormalColorDesc@ha
    addi     r10, r3, sPrimitivePosNormalColorDesc@l
    lwz      r16, 0x4(r4)
    lwz      r17, 0x8(r4)
    lwz      r18, 0xc(r4)
    lwz      r19, 0x10(r4)
    lwz      r20, 0x14(r4)
    lwz      r21, 0x18(r4)
    lwz      r22, 0x1c(r4)
    lwz      r23, 0x20(r4)
    lwz      r24, 0x24(r4)
    lwz      r25, 0x28(r4)
    lwz      r26, 0x2c(r4)
    lwz      r27, 0x30(r4)
    lwz      r28, 0x34(r4)
    lwz      r12, 0x38(r4)
    lwz      r11, 0x3c(r4)
    lwz      r9, 0x0(r10)
    lwz      r8, 0x4(r10)
    lwz      r7, 0x8(r10)
    lwz      r6, 0xc(r10)
    lwz      r5, 0x10(r10)
    lwz      r4, 0x14(r10)
    lwz      r3, 0x18(r10)
    lwz      r0, 0x1c(r10)
    stw      r15, 0x94(r1)
    stw      r16, 0x98(r1)
    stw      r17, 0x9c(r1)
    stw      r18, 0xa0(r1)
    stw      r19, 0xa4(r1)
    stw      r20, 0xa8(r1)
    stw      r21, 0xac(r1)
    stw      r22, 0xb0(r1)
    stw      r23, 0xb4(r1)
    stw      r24, 0xb8(r1)
    stw      r25, 0xbc(r1)
    stw      r26, 0xc0(r1)
    stw      r27, 0xc4(r1)
    stw      r28, 0xc8(r1)
    stw      r12, 0xcc(r1)
    stw      r11, 0xd0(r1)
    stw      r9, 0x14(r1)
    stw      r8, 0x18(r1)
    stw      r7, 0x1c(r1)
    stw      r6, 0x20(r1)
    stw      r5, 0x24(r1)
    stw      r4, 0x28(r1)
    stw      r3, 0x2c(r1)
    stw      r0, 0x30(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x94
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x14
    bl       GXSetVtxDescv
    mr       r3, r29
    addi     r4, r29, 0x10
    addi     r5, r29, 0x20
    addi     r6, r1, 0x8
    bl       CalcTriangleNormal
    mr       r3, r31
    mr       r5, r30
    li       r4, 0x0
    bl       GXBegin
    clrlwi.  r5, r30, 16
    lfs      f4, 0xc(r1)
    lfs      f5, 0x10(r1)
    mr       r4, r29
    lis      r3, 0xcc01
    ble      .L_8019CB24
    srwi.    r0, r5, 2
    mtctr    r0
    beq      .L_8019CAE8
.L_8019CA18:
    lfs      f1, 0x0(r4)
    lfs      f2, 0x4(r4)
    stfs     f1, -0x8000(r3)
    lfs      f3, 0x8(r4)
    stfs     f2, -0x8000(r3)
    lwz      r0, 0xc(r4)
    stfs     f3, -0x8000(r3)
    lfs      f1, 0x10(r4)
    lfs      f0, 0x8(r1)
    lfs      f2, 0x14(r4)
    stfs     f0, -0x8000(r3)
    lfs      f3, 0x18(r4)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r0, -0x8000(r3)
    lwz      r0, 0x1c(r4)
    stfs     f1, -0x8000(r3)
    lfs      f1, 0x20(r4)
    stfs     f2, -0x8000(r3)
    lfs      f2, 0x24(r4)
    stfs     f3, -0x8000(r3)
    lfs      f3, 0x28(r4)
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r0, -0x8000(r3)
    lwz      r0, 0x2c(r4)
    stfs     f1, -0x8000(r3)
    lfs      f1, 0x30(r4)
    stfs     f2, -0x8000(r3)
    lfs      f2, 0x34(r4)
    stfs     f3, -0x8000(r3)
    lfs      f3, 0x38(r4)
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r0, -0x8000(r3)
    lwz      r0, 0x3c(r4)
    addi     r4, r4, 0x40
    stfs     f1, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r0, -0x8000(r3)
    bdnz     .L_8019CA18
    andi.    r5, r5, 0x3
    beq      .L_8019CB24
.L_8019CAE8:
    mtctr    r5
.L_8019CAEC:
    lfs      f1, 0x0(r4)
    lfs      f2, 0x4(r4)
    stfs     f1, -0x8000(r3)
    lfs      f3, 0x8(r4)
    stfs     f2, -0x8000(r3)
    lwz      r0, 0xc(r4)
    addi     r4, r4, 0x10
    stfs     f3, -0x8000(r3)
    lfs      f0, 0x8(r1)
    stfs     f0, -0x8000(r3)
    stfs     f4, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stw      r0, -0x8000(r3)
    bdnz     .L_8019CAEC
.L_8019CB24:
    lmw      r15, 0xdc(r1)
    lwz      r0, 0x124(r1)
    mtlr     r0
    addi     r1, r1, 0x120
    blr
*/

/**
 * @note Address: 0x8019C74C
 * @note Size: 0x158
 */
void CCircle::Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	CPrimitive::Init(tx, ty, tz, sx, sy, sz, rx, ry, rz);
	Vec zero = { 0.0f, 0.0f, 0.0f };
	mCenter  = zero;
}

/**
 * @note Address: 0x8019C54C
 * @note Size: 0x200
 */
void CCircle::Draw(Mtx view, u32 segments, u32 centerColor, u32 edgeColor)
{
	LoadMtx(view);
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosClr();
	GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, segments + 2);
	GXPosition3f32(0.0f, 0.0f, 0.0f);
	GXColor1u32(centerColor);
	for (int i = 1; i <= segments + 1; i++) {
		f32 angle = (TWO_PI / segments) * i;
		GXPosition3f32(mCenter.x + (f32)sin(angle), mCenter.y + (f32)cos(angle), mCenter.z);
		GXColor1u32(edgeColor);
	}
}
/*
    stwu     r1, -0xe0(r1)
    mflr     r0
    stw      r0, 0xe4(r1)
    stfd     f31, 0xd0(r1)
    psq_st   f31, 0xd8(r1), 0, qr0
    stfd     f30, 0xc0(r1)
    psq_st   f30, 0xc8(r1), 0, qr0
    stfd     f29, 0xb0(r1)
    psq_st   f29, 0xb8(r1), 0, qr0
    stfd     f28, 0xa0(r1)
    psq_st   f28, 0xa8(r1), 0, qr0
    stfd     f27, 0x90(r1)
    psq_st   f27, 0x98(r1), 0, qr0
    stmw     r20, 0x60(r1)
    mr       r28, r3
    mr       r29, r5
    mr       r30, r6
    mr       r31, r7
    bl       fn_8019DF5C
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lis      r4, sPrimitivePosColorFormat@ha
    lwzu     r20, sPrimitivePosColorFormat@l(r4)
    lis      r3, sPrimitivePosColorDesc@ha
    addi     r8, r3, sPrimitivePosColorDesc@l
    lwz      r21, 0x4(r4)
    lwz      r22, 0x8(r4)
    lwz      r23, 0xc(r4)
    lwz      r24, 0x10(r4)
    lwz      r27, 0x14(r4)
    lwz      r26, 0x18(r4)
    lwz      r25, 0x1c(r4)
    lwz      r12, 0x20(r4)
    lwz      r11, 0x24(r4)
    lwz      r10, 0x28(r4)
    lwz      r9, 0x2c(r4)
    lwz      r7, 0x0(r8)
    lwz      r6, 0x4(r8)
    lwz      r5, 0x8(r8)
    lwz      r4, 0xc(r8)
    lwz      r3, 0x10(r8)
    lwz      r0, 0x14(r8)
    stw      r20, 0x20(r1)
    stw      r21, 0x24(r1)
    stw      r22, 0x28(r1)
    stw      r23, 0x2c(r1)
    stw      r24, 0x30(r1)
    stw      r27, 0x34(r1)
    stw      r26, 0x38(r1)
    stw      r25, 0x3c(r1)
    stw      r12, 0x40(r1)
    stw      r11, 0x44(r1)
    stw      r10, 0x48(r1)
    stw      r9, 0x4c(r1)
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    stw      r5, 0x10(r1)
    stw      r4, 0x14(r1)
    stw      r3, 0x18(r1)
    stw      r0, 0x1c(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x20
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x8
    bl       GXSetVtxDescv
    addi     r0, r29, 0x2
    li       r3, 0xa0
    clrlwi   r5, r0, 16
    li       r4, 0x0
    bl       GXBegin
    lfs      f0, sPrimitiveZero@sda21(r0)
    lis      r25, 0xcc01
    lfs      f29, sPrimitiveFullTurn@sda21(r0)
    addi     r27, r29, 0x1
    stfs     f0, -0x8000(r25)
    li       r20, 0x1
    lfd      f30, sPrimitiveUnsignedConversionBias@sda21(r0)
    lis      r26, 0x4330
    stfs     f0, -0x8000(r25)
    lfd      f31, sPrimitiveSignedConversionBias@sda21(r0)
    stfs     f0, -0x8000(r25)
    stw      r30, -0x8000(r25)
    b        .L_8019C708
.L_8019C69C:
    stw      r29, 0x54(r1)
    xoris    r0, r20, 0x8000
    lfs      f27, 0x178(r28)
    stw      r26, 0x50(r1)
    lfd      f0, 0x50(r1)
    stw      r0, 0x5c(r1)
    fsubs    f0, f0, f30
    stw      r26, 0x58(r1)
    fdivs    f1, f29, f0
    lfd      f0, 0x58(r1)
    fsubs    f0, f0, f31
    fmuls    f28, f1, f0
    fmr      f1, f28
    bl       cos
    frsp     f2, f1
    lfs      f0, 0x174(r28)
    fmr      f1, f28
    fadds    f28, f0, f2
    bl       sin
    frsp     f1, f1
    lfs      f0, 0x170(r28)
    addi     r20, r20, 0x1
    fadds    f0, f0, f1
    stfs     f0, -0x8000(r25)
    stfs     f28, -0x8000(r25)
    stfs     f27, -0x8000(r25)
    stw      r31, -0x8000(r25)
.L_8019C708:
    cmplw    r20, r27
    ble      .L_8019C69C
    psq_l    f31, 0xd8(r1), 0, qr0
    lfd      f31, 0xd0(r1)
    psq_l    f30, 0xc8(r1), 0, qr0
    lfd      f30, 0xc0(r1)
    psq_l    f29, 0xb8(r1), 0, qr0
    lfd      f29, 0xb0(r1)
    psq_l    f28, 0xa8(r1), 0, qr0
    lfd      f28, 0xa0(r1)
    psq_l    f27, 0x98(r1), 0, qr0
    lfd      f27, 0x90(r1)
    lmw      r20, 0x60(r1)
    lwz      r0, 0xe4(r1)
    mtlr     r0
    addi     r1, r1, 0xe0
    blr
*/

/**
 * @note Address: 0x8019C258
 * @note Size: 0x2F4
 */
void CCircle::DrawLit(Mtx view, u32 segments, u32 centerColor, u32 edgeColor)
{
	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_NRM, GX_NRM_XYZ, GX_S8, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};

	LoadNrmMtx(LoadMtx(view));
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosNrmClr(fmt);
	GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, segments + 2);
	GXPosition3f32(0.0f, 0.0f, 0.0f);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(centerColor);
	for (int i = 1; i <= segments + 1; i++) {
		f32 angle = (TWO_PI / segments) * i;
		GXPosition3f32(mCenter.x + (f32)sin(angle), mCenter.y + (f32)cos(angle), mCenter.z);
		GXNormal3s8(0, 0, 1);
		GXColor1u32(edgeColor);
	}
}
/*
    stwu     r1, -0x1b0(r1)
    mflr     r0
    stw      r0, 0x1b4(r1)
    stfd     f31, 0x1a0(r1)
    psq_st   f31, 0x1a8(r1), 0, qr0
    stfd     f30, 0x190(r1)
    psq_st   f30, 0x198(r1), 0, qr0
    stfd     f29, 0x180(r1)
    psq_st   f29, 0x188(r1), 0, qr0
    stfd     f28, 0x170(r1)
    psq_st   f28, 0x178(r1), 0, qr0
    stfd     f27, 0x160(r1)
    psq_st   f27, 0x168(r1), 0, qr0
    stmw     r15, 0x11c(r1)
    lis      r8, lbl_8020EAF8@ha
    mr       r27, r3
    addi     r31, r8, lbl_8020EAF8@l
    mr       r28, r5
    lwz      r21, 0xcc(r31)
    mr       r29, r6
    lwz      r20, 0xd0(r31)
    mr       r30, r7
    lwz      r19, 0xd4(r31)
    lwz      r18, 0xd8(r31)
    lwz      r17, 0xdc(r31)
    lwz      r16, 0xe0(r31)
    lwz      r15, 0xe4(r31)
    lwz      r12, 0xe8(r31)
    lwz      r11, 0xec(r31)
    lwz      r10, 0xf0(r31)
    lwz      r9, 0xf4(r31)
    lwz      r8, 0xf8(r31)
    lwz      r7, 0xfc(r31)
    lwz      r6, 0x100(r31)
    lwz      r5, 0x104(r31)
    lwz      r0, 0x108(r31)
    stw      r21, 0xc8(r1)
    stw      r20, 0xcc(r1)
    stw      r19, 0xd0(r1)
    stw      r18, 0xd4(r1)
    stw      r17, 0xd8(r1)
    stw      r16, 0xdc(r1)
    stw      r15, 0xe0(r1)
    stw      r12, 0xe4(r1)
    stw      r11, 0xe8(r1)
    stw      r10, 0xec(r1)
    stw      r9, 0xf0(r1)
    stw      r8, 0xf4(r1)
    stw      r7, 0xf8(r1)
    stw      r6, 0xfc(r1)
    stw      r5, 0x100(r1)
    stw      r0, 0x104(r1)
    bl       fn_8019DF5C
    addi     r4, r1, 0x28
    bl       PSMTXInverse
    addi     r3, r1, 0x28
    addi     r4, r1, 0x58
    bl       PSMTXTranspose
    addi     r3, r1, 0x58
    li       r4, 0x0
    bl       GXLoadNrmMtxImm
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lwz      r15, 0x60(r31)
    lwz      r16, 0x64(r31)
    lwz      r17, 0x68(r31)
    lwz      r18, 0x6c(r31)
    lwz      r19, 0x70(r31)
    lwz      r20, 0x74(r31)
    lwz      r21, 0x78(r31)
    lwz      r22, 0x7c(r31)
    lwz      r23, 0x80(r31)
    lwz      r24, 0x84(r31)
    lwz      r25, 0x88(r31)
    lwz      r26, 0x8c(r31)
    lwz      r12, 0x90(r31)
    lwz      r11, 0x94(r31)
    lwz      r10, 0x98(r31)
    lwz      r9, 0x9c(r31)
    lwz      r8, 0xa0(r31)
    lwz      r7, 0xa4(r31)
    lwz      r6, 0xa8(r31)
    lwz      r5, 0xac(r31)
    lwz      r4, 0xb0(r31)
    lwz      r3, 0xb4(r31)
    lwz      r0, 0xb8(r31)
    lwz      r31, 0xbc(r31)
    stw      r15, 0x88(r1)
    stw      r16, 0x8c(r1)
    stw      r17, 0x90(r1)
    stw      r18, 0x94(r1)
    stw      r19, 0x98(r1)
    stw      r20, 0x9c(r1)
    stw      r21, 0xa0(r1)
    stw      r22, 0xa4(r1)
    stw      r23, 0xa8(r1)
    stw      r24, 0xac(r1)
    stw      r25, 0xb0(r1)
    stw      r26, 0xb4(r1)
    stw      r12, 0xb8(r1)
    stw      r11, 0xbc(r1)
    stw      r10, 0xc0(r1)
    stw      r9, 0xc4(r1)
    stw      r8, 0x8(r1)
    stw      r7, 0xc(r1)
    stw      r6, 0x10(r1)
    stw      r5, 0x14(r1)
    stw      r4, 0x18(r1)
    stw      r3, 0x1c(r1)
    stw      r0, 0x20(r1)
    stw      r31, 0x24(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0xc8
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x8
    bl       GXSetVtxDescv
    addi     r0, r28, 0x2
    li       r3, 0xa0
    clrlwi   r5, r0, 16
    li       r4, 0x0
    bl       GXBegin
    lfs      f0, sPrimitiveZero@sda21(r0)
    lis      r17, 0xcc01
    li       r3, 0x0
    li       r0, 0x1
    stfs     f0, -0x8000(r17)
    addi     r15, r28, 0x1
    lfs      f29, sPrimitiveFullTurn@sda21(r0)
    li       r18, 0x1
    stfs     f0, -0x8000(r17)
    lis      r16, 0x4330
    lfd      f30, sPrimitiveUnsignedConversionBias@sda21(r0)
    stfs     f0, -0x8000(r17)
    lfd      f31, sPrimitiveSignedConversionBias@sda21(r0)
    stb      r3, -0x8000(r17)
    stb      r3, -0x8000(r17)
    stb      r0, -0x8000(r17)
    stw      r29, -0x8000(r17)
    b        .L_8019C508
.L_8019C488:
    stw      r28, 0x10c(r1)
    xoris    r0, r18, 0x8000
    lfs      f27, 0x178(r27)
    stw      r16, 0x108(r1)
    lfd      f0, 0x108(r1)
    stw      r0, 0x114(r1)
    fsubs    f0, f0, f30
    stw      r16, 0x110(r1)
    fdivs    f1, f29, f0
    lfd      f0, 0x110(r1)
    fsubs    f0, f0, f31
    fmuls    f28, f1, f0
    fmr      f1, f28
    bl       cos
    frsp     f2, f1
    lfs      f0, 0x174(r27)
    fmr      f1, f28
    fadds    f28, f0, f2
    bl       sin
    frsp     f1, f1
    lfs      f0, 0x170(r27)
    li       r3, 0x0
    li       r0, 0x1
    addi     r18, r18, 0x1
    fadds    f0, f0, f1
    stfs     f0, -0x8000(r17)
    stfs     f28, -0x8000(r17)
    stfs     f27, -0x8000(r17)
    stb      r3, -0x8000(r17)
    stb      r3, -0x8000(r17)
    stb      r0, -0x8000(r17)
    stw      r30, -0x8000(r17)
.L_8019C508:
    cmplw    r18, r15
    ble      .L_8019C488
    psq_l    f31, 0x1a8(r1), 0, qr0
    lfd      f31, 0x1a0(r1)
    psq_l    f30, 0x198(r1), 0, qr0
    lfd      f30, 0x190(r1)
    psq_l    f29, 0x188(r1), 0, qr0
    lfd      f29, 0x180(r1)
    psq_l    f28, 0x178(r1), 0, qr0
    lfd      f28, 0x170(r1)
    psq_l    f27, 0x168(r1), 0, qr0
    lfd      f27, 0x160(r1)
    lmw      r15, 0x11c(r1)
    lwz      r0, 0x1b4(r1)
    mtlr     r0
    addi     r1, r1, 0x1b0
    blr
*/

/**
 * @note Address: 0x8019C100
 * @note Size: 0x158
 */
void CTile::Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	CPrimitive::Init(tx, ty, tz, sx, sy, sz, rx, ry, rz);
	Vec zero = { 0.0f, 0.0f, 0.0f };
	mCenter  = zero;
}

/**
 * @note Address: 0x8019BF04
 * @note Size: 0x1FC
 */
void CTile::Draw(Mtx view, u32 color0, u32 color1, u32 color2, u32 color3)
{
	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};

	LoadMtx(view);
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosClr(fmt);
	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
	GXPosition3f32(mCenter.x - 1.0f, 1.0f + mCenter.y, mCenter.z);
	GXColor1u32(color0);
	GXPosition3f32(1.0f + mCenter.x, 1.0f + mCenter.y, mCenter.z);
	GXColor1u32(color1);
	GXPosition3f32(mCenter.x - 1.0f, mCenter.y - 1.0f, mCenter.z);
	GXColor1u32(color2);
	GXPosition3f32(1.0f + mCenter.x, mCenter.y - 1.0f, mCenter.z);
	GXColor1u32(color3);
}
/*
    stwu     r1, -0xc0(r1)
    mflr     r0
    lis      r9, lbl_8020EAF8@ha
    stw      r0, 0xc4(r1)
    stmw     r19, 0x8c(r1)
    addi     r31, r9, lbl_8020EAF8@l
    mr       r27, r5
    mr       r28, r6
    mr       r29, r7
    mr       r30, r8
    mr       r26, r3
    lwz      r21, 0x118(r31)
    lwz      r20, 0x11c(r31)
    lwz      r19, 0x120(r31)
    lwz      r12, 0x124(r31)
    lwz      r11, 0x128(r31)
    lwz      r10, 0x12c(r31)
    lwz      r9, 0x130(r31)
    lwz      r8, 0x134(r31)
    lwz      r7, 0x138(r31)
    lwz      r6, 0x13c(r31)
    lwz      r5, 0x140(r31)
    lwz      r0, 0x144(r31)
    stw      r21, 0x50(r1)
    stw      r20, 0x54(r1)
    stw      r19, 0x58(r1)
    stw      r12, 0x5c(r1)
    stw      r11, 0x60(r1)
    stw      r10, 0x64(r1)
    stw      r9, 0x68(r1)
    stw      r8, 0x6c(r1)
    stw      r7, 0x70(r1)
    stw      r6, 0x74(r1)
    stw      r5, 0x78(r1)
    stw      r0, 0x7c(r1)
    bl       fn_8019DF5C
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lwz      r19, 0x18(r31)
    lwz      r20, 0x1c(r31)
    lwz      r21, 0x20(r31)
    lwz      r22, 0x24(r31)
    lwz      r23, 0x28(r31)
    lwz      r24, 0x2c(r31)
    lwz      r25, 0x30(r31)
    lwz      r12, 0x34(r31)
    lwz      r11, 0x38(r31)
    lwz      r10, 0x3c(r31)
    lwz      r9, 0x40(r31)
    lwz      r8, 0x44(r31)
    lwz      r7, 0x48(r31)
    lwz      r6, 0x4c(r31)
    lwz      r5, 0x50(r31)
    lwz      r4, 0x54(r31)
    lwz      r3, 0x58(r31)
    lwz      r0, 0x5c(r31)
    stw      r19, 0x20(r1)
    stw      r20, 0x24(r1)
    stw      r21, 0x28(r1)
    stw      r22, 0x2c(r1)
    stw      r23, 0x30(r1)
    stw      r24, 0x34(r1)
    stw      r25, 0x38(r1)
    stw      r12, 0x3c(r1)
    stw      r11, 0x40(r1)
    stw      r10, 0x44(r1)
    stw      r9, 0x48(r1)
    stw      r8, 0x4c(r1)
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    stw      r5, 0x10(r1)
    stw      r4, 0x14(r1)
    stw      r3, 0x18(r1)
    stw      r0, 0x1c(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x50
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x8
    bl       GXSetVtxDescv
    li       r3, 0x98
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lfs      f2, sPrimitiveOne@sda21(r0)
    lis      r3, 0xcc01
    lfs      f0, 0x178(r26)
    lfs      f1, 0x17c(r26)
    fsubs    f0, f0, f2
    lfs      f3, 0x180(r26)
    fadds    f1, f2, f1
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    stw      r27, -0x8000(r3)
    lfs      f0, 0x178(r26)
    lfs      f1, 0x17c(r26)
    fadds    f0, f2, f0
    lfs      f3, 0x180(r26)
    fadds    f1, f2, f1
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    stw      r28, -0x8000(r3)
    lfs      f0, 0x178(r26)
    lfs      f1, 0x17c(r26)
    fsubs    f0, f0, f2
    lfs      f3, 0x180(r26)
    fsubs    f1, f1, f2
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    stw      r29, -0x8000(r3)
    lfs      f0, 0x178(r26)
    lfs      f1, 0x17c(r26)
    fadds    f0, f2, f0
    lfs      f3, 0x180(r26)
    fsubs    f1, f1, f2
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    stw      r30, -0x8000(r3)
    lmw      r19, 0x8c(r1)
    lwz      r0, 0xc4(r1)
    mtlr     r0
    addi     r1, r1, 0xc0
    blr
*/

/**
 * @note Address: 0x8019BC60
 * @note Size: 0x2A4
 */
void CTile::DrawLit(Mtx view, u32 color0, u32 color1, u32 color2, u32 color3)
{
	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_NRM, GX_NRM_XYZ, GX_S8, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};

	LoadNrmMtx(LoadMtx(view));
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosNrmClr(fmt);
	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
	GXPosition3f32(mCenter.x - 1.0f, 1.0f + mCenter.y, mCenter.z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color0);
	GXPosition3f32(1.0f + mCenter.x, 1.0f + mCenter.y, mCenter.z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color1);
	GXPosition3f32(mCenter.x - 1.0f, mCenter.y - 1.0f, mCenter.z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color2);
	GXPosition3f32(1.0f + mCenter.x, mCenter.y - 1.0f, mCenter.z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color3);
}
/*
    stwu     r1, -0x150(r1)
    mflr     r0
    lis      r9, lbl_8020EAF8@ha
    stw      r0, 0x154(r1)
    stmw     r14, 0x108(r1)
    addi     r31, r9, lbl_8020EAF8@l
    mr       r28, r5
    mr       r29, r6
    mr       r30, r7
    mr       r14, r8
    mr       r27, r3
    lwz      r21, 0x148(r31)
    lwz      r20, 0x14c(r31)
    lwz      r19, 0x150(r31)
    lwz      r18, 0x154(r31)
    lwz      r17, 0x158(r31)
    lwz      r16, 0x15c(r31)
    lwz      r15, 0x160(r31)
    lwz      r12, 0x164(r31)
    lwz      r11, 0x168(r31)
    lwz      r10, 0x16c(r31)
    lwz      r9, 0x170(r31)
    lwz      r8, 0x174(r31)
    lwz      r7, 0x178(r31)
    lwz      r6, 0x17c(r31)
    lwz      r5, 0x180(r31)
    lwz      r0, 0x184(r31)
    stw      r21, 0xc8(r1)
    stw      r20, 0xcc(r1)
    stw      r19, 0xd0(r1)
    stw      r18, 0xd4(r1)
    stw      r17, 0xd8(r1)
    stw      r16, 0xdc(r1)
    stw      r15, 0xe0(r1)
    stw      r12, 0xe4(r1)
    stw      r11, 0xe8(r1)
    stw      r10, 0xec(r1)
    stw      r9, 0xf0(r1)
    stw      r8, 0xf4(r1)
    stw      r7, 0xf8(r1)
    stw      r6, 0xfc(r1)
    stw      r5, 0x100(r1)
    stw      r0, 0x104(r1)
    bl       fn_8019DF5C
    addi     r4, r1, 0x28
    bl       PSMTXInverse
    addi     r3, r1, 0x28
    addi     r4, r1, 0x58
    bl       PSMTXTranspose
    addi     r3, r1, 0x58
    li       r4, 0x0
    bl       GXLoadNrmMtxImm
    li       r3, 0x2
    bl       HSD_StateInvalidate
    lwz      r15, 0x60(r31)
    lwz      r16, 0x64(r31)
    lwz      r17, 0x68(r31)
    lwz      r18, 0x6c(r31)
    lwz      r19, 0x70(r31)
    lwz      r20, 0x74(r31)
    lwz      r21, 0x78(r31)
    lwz      r22, 0x7c(r31)
    lwz      r23, 0x80(r31)
    lwz      r24, 0x84(r31)
    lwz      r25, 0x88(r31)
    lwz      r26, 0x8c(r31)
    lwz      r12, 0x90(r31)
    lwz      r11, 0x94(r31)
    lwz      r10, 0x98(r31)
    lwz      r9, 0x9c(r31)
    lwz      r8, 0xa0(r31)
    lwz      r7, 0xa4(r31)
    lwz      r6, 0xa8(r31)
    lwz      r5, 0xac(r31)
    lwz      r4, 0xb0(r31)
    lwz      r3, 0xb4(r31)
    lwz      r0, 0xb8(r31)
    lwz      r31, 0xbc(r31)
    stw      r15, 0x88(r1)
    stw      r16, 0x8c(r1)
    stw      r17, 0x90(r1)
    stw      r18, 0x94(r1)
    stw      r19, 0x98(r1)
    stw      r20, 0x9c(r1)
    stw      r21, 0xa0(r1)
    stw      r22, 0xa4(r1)
    stw      r23, 0xa8(r1)
    stw      r24, 0xac(r1)
    stw      r25, 0xb0(r1)
    stw      r26, 0xb4(r1)
    stw      r12, 0xb8(r1)
    stw      r11, 0xbc(r1)
    stw      r10, 0xc0(r1)
    stw      r9, 0xc4(r1)
    stw      r8, 0x8(r1)
    stw      r7, 0xc(r1)
    stw      r6, 0x10(r1)
    stw      r5, 0x14(r1)
    stw      r4, 0x18(r1)
    stw      r3, 0x1c(r1)
    stw      r0, 0x20(r1)
    stw      r31, 0x24(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0xc8
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x8
    bl       GXSetVtxDescv
    li       r3, 0x98
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lfs      f2, sPrimitiveOne@sda21(r0)
    lis      r4, 0xcc01
    lfs      f0, 0x178(r27)
    li       r3, 0x0
    lfs      f1, 0x17c(r27)
    li       r0, 0x1
    fsubs    f0, f0, f2
    lfs      f3, 0x180(r27)
    fadds    f1, f2, f1
    stfs     f0, -0x8000(r4)
    stfs     f1, -0x8000(r4)
    stfs     f3, -0x8000(r4)
    stb      r3, -0x8000(r4)
    stb      r3, -0x8000(r4)
    stb      r0, -0x8000(r4)
    stw      r28, -0x8000(r4)
    lfs      f0, 0x178(r27)
    lfs      f1, 0x17c(r27)
    fadds    f0, f2, f0
    lfs      f3, 0x180(r27)
    fadds    f1, f2, f1
    stfs     f0, -0x8000(r4)
    stfs     f1, -0x8000(r4)
    stfs     f3, -0x8000(r4)
    stb      r3, -0x8000(r4)
    stb      r3, -0x8000(r4)
    stb      r0, -0x8000(r4)
    stw      r29, -0x8000(r4)
    lfs      f0, 0x178(r27)
    lfs      f1, 0x17c(r27)
    fsubs    f0, f0, f2
    lfs      f3, 0x180(r27)
    fsubs    f1, f1, f2
    stfs     f0, -0x8000(r4)
    stfs     f1, -0x8000(r4)
    stfs     f3, -0x8000(r4)
    stb      r3, -0x8000(r4)
    stb      r3, -0x8000(r4)
    stb      r0, -0x8000(r4)
    stw      r30, -0x8000(r4)
    lfs      f0, 0x178(r27)
    lfs      f1, 0x17c(r27)
    fadds    f0, f2, f0
    lfs      f3, 0x180(r27)
    fsubs    f1, f1, f2
    stfs     f0, -0x8000(r4)
    stfs     f1, -0x8000(r4)
    stfs     f3, -0x8000(r4)
    stb      r3, -0x8000(r4)
    stb      r3, -0x8000(r4)
    stb      r0, -0x8000(r4)
    stw      r14, -0x8000(r4)
    lmw      r14, 0x108(r1)
    lwz      r0, 0x154(r1)
    mtlr     r0
    addi     r1, r1, 0x150
    blr
*/

/**
 * @note Address: 0x8019BB04
 * @note Size: 0x15C
 */
void CSprite::Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	CPrimitive::Init(tx, ty, tz, sx, sy, sz, rx, ry, rz);
	mTex     = NULL;
	Vec zero = { 0.0f, 0.0f, 0.0f };
	mCenter  = zero;
}

/**
 * @note Address: 0x8019BA88
 * @note Size: 0x7C
 */
void CSprite::Draw(Mtx view, GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, u32 color0, u32 color1, u32 color2, u32 color3)
{
	if (mTex != NULL) {
		LoadMtx(view);
		mTex->Draw(map, mtxId, mtxType, &mCenter, color0, color1, color2, color3);
	}
}

/**
 * @note Address: 0x8019B9EC
 * @note Size: 0x9C
 */
void CSprite::DrawLit(Mtx view, GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, u32 color0, u32 color1, u32 color2, u32 color3)
{
	if (mTex != NULL) {
		LoadNrmMtx(LoadMtx(view));
		mTex->DrawLit(map, mtxId, mtxType, &mCenter, color0, color1, color2, color3);
	}
}

/**
 * @note Address: 0x8019B9A8
 * @note Size: 0x44
 */
void CCube::Draw(Mtx view)
{
	LoadNrmMtx(LoadMtx(view));
	GXDrawCube();
}

/**
 * @note Address: 0x8019B944
 * @note Size: 0x64
 */
void CSphere::Draw(Mtx view, u8 major, u8 minor)
{
	LoadNrmMtx(LoadMtx(view));
	GXDrawSphere(major, minor);
}

/**
 * @note Address: 0x8019B8F0
 * @note Size: 0x54
 */
void CCylinder::Draw(Mtx view, u8 sides)
{
	LoadNrmMtx(LoadMtx(view));
	GXDrawCylinder(sides);
}

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

// tev_state.h's value; that is an HSD header and this file builds with the Dolphin ones
#define HSD_STATE_VTX_ATTR 2

// normals take the inverse transpose of the position matrix, so a non-uniform scale doesn't skew them
static inline void LoadNrmMtx(MtxP mtx)
{
	Mtx inv;
	Mtx nrm;

	PSMTXInverse(mtx, inv);
	PSMTXTranspose(inv, nrm);
	GXLoadNrmMtxImm(nrm, GX_PNMTX0);
}

// Position and color, both direct.
static void SetVtxPosClr()
{
	GXVtxAttrFmtList fmt[] = {
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
	GXSetVtxAttrFmtv(GX_VTXFMT0, fmt);
	GXSetVtxDescv(desc);
}

// position, normal and color, all direct
static void SetVtxPosNrmClr(GXCompType normalType = GX_F32)
{
	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_NRM, GX_NRM_XYZ, normalType, 0 },
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
	GXSetVtxAttrFmtv(GX_VTXFMT0, fmt);
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

	// a fixed matrix goes to GX as the whole model view: view and the transform are skipped
	if (mFixedMtx != NULL) {
		mtx = mFixedMtx;
	} else {
		// view only goes in on a rebuild: until something sets mDirty again, the cached
		// mModelView keeps the view it was built with
		if (mDirty) {
			Mtx rot;
			MtxP trans = mTrans;
			MtxP scale = mScale;
			MtxP rotX  = mRotX;
			MtxP rotY  = mRotY;
			MtxP rotZ  = mRotZ;

			// order the rotations follow the translation: 0 x y z, 1 x z y, 2 y x z, 3 y z x,
			// 4 z x y (what Set gives), 5 z y x (what CGs gives its shapes)
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
		PrimVertex* v = vtx + i;
		GXPosition3f32(v->pos.x, v->pos.y, v->pos.z);
		GXColor1u32(v->color);
	}
}
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
		PrimVertex* v = vtx + i;
		GXPosition3f32(v->pos.x, v->pos.y, v->pos.z);
		GXColor1u32(v->color);
	}
}
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
	// the hub at the origin, then segments + 1 rim points around mCenter: the last one
	// lands on the first again and closes the fan
	GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, segments + 2);
	f32 zero = 0.0f;
	GXPosition3f32(zero, zero, zero);
	GXColor1u32(centerColor);
	for (int i = 1; i <= segments + 1; i++) {
		f32 angle = (TWO_PI / segments) * i;
		GXPosition3f32(mCenter.x + (f32)sin(angle), mCenter.y + (f32)cos(angle), mCenter.z);
		GXColor1u32(edgeColor);
	}
}
/**
 * @note Address: 0x8019C258
 * @note Size: 0x2F4
 */
void CCircle::DrawLit(Mtx view, u32 segments, u32 centerColor, u32 edgeColor)
{
	LoadNrmMtx(LoadMtx(view));
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosNrmClr(GX_S8);
	GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, segments + 2);
	f32 zero = 0.0f;
	GXPosition3f32(zero, zero, zero);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(centerColor);
	for (int i = 1; i <= segments + 1; i++) {
		f32 angle = (TWO_PI / segments) * i;
		GXPosition3f32(mCenter.x + (f32)sin(angle), mCenter.y + (f32)cos(angle), mCenter.z);
		GXNormal3s8(0, 0, 1);
		GXColor1u32(edgeColor);
	}
}
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
	LoadMtx(view);
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosClr();
	// a strip, so the corners zigzag: color0 and color1 on the +y edge, color2 and color3
	// on the -y edge, each pair -x first
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
/**
 * @note Address: 0x8019BC60
 * @note Size: 0x2A4
 */
void CTile::DrawLit(Mtx view, u32 color0, u32 color1, u32 color2, u32 color3)
{
	LoadNrmMtx(LoadMtx(view));
	HSD_StateInvalidate(HSD_STATE_VTX_ATTR);
	SetVtxPosNrmClr(GX_S8);
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

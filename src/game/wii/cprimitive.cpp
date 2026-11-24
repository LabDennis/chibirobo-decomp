// The Wii's cprimitive.cpp, .text 0x802AF8B8-0x802B193C, in the GameCube file's order. What the
// GameCube file has as inline helpers are functions of the base class here: they take the view
// matrix and load it themselves, and nothing invalidates an HSD state.
//
// NONMATCHING: the nine CLine and CTriangle vertex loops are the GameCube file's wall: retail reads
// x and y before it writes x and reads z after. Splitting GXPosition2f32 plus GX_WRITE_F32 recovers
// that shape in CLine::Draw and CTriangle::Draw(PrimVertex*); their x/y argument order and the other
// seven loops remain open.

#include "game/cprimitive.h"
#include "game/ut.h"
#include "game/gmath.h"
#include <math.h>

extern "C" {
void GXDrawCylinder(u8 numEdges);
void GXDrawSphere(u8 numMajor, u8 numMinor);
void GXDrawCube(void);
}

void CPrimitive::Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	Set(tx, ty, tz, sx, sy, sz, rx, ry, rz);
}

void CPrimitive::Set(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	SetTrans(tx, ty, tz);
	SetScale(sx, sy, sz);
	SetRot(rx, ry, rz);
	SetBillboard(0);
	mRotOrder = 4;
	mFixedMtx = NULL;
}

// a list is lit with the normal of its first three points
void CPrimitive::CalcNormal(Vec* pos, u16 count, Vec* normal)
{
	CalcTriangleNormal(&pos[0], &pos[1], &pos[2], normal);
}

void CPrimitive::CalcNormal(PrimVertex* vtx, u16 count, Vec* normal)
{
	CalcTriangleNormal(&vtx[0].pos, &vtx[1].pos, &vtx[2].pos, normal);
}

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

// normals take the inverse transpose of the position matrix, so a non-uniform scale doesn't skew them
MtxP CPrimitive::LoadMtxNrm(Mtx view)
{
	Mtx inv;
	Mtx nrm;

	MtxP mtx = LoadMtx(view);
	PSMTXInverse(mtx, inv);
	PSMTXTranspose(inv, nrm);
	GXLoadNrmMtxImm(nrm, GX_PNMTX0);
	return mtx;
}

// position and color, both direct; fmt replaces the default format
void CPrimitive::SetVtxPosClr(Mtx view, GXVtxAttrFmtList* fmt)
{
	LoadMtx(view);
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
	if (fmt != NULL) {
		GXSetVtxAttrFmtv(GX_VTXFMT0, fmt);
	} else {
		GXSetVtxAttrFmtv(GX_VTXFMT0, defFmt);
	}
	GXSetVtxDescv(desc);
}

// position, normal and color, all direct
void CPrimitive::SetVtxPosNrmClr(Mtx view, GXVtxAttrFmtList* fmt)
{
	LoadMtxNrm(view);
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
	if (fmt != NULL) {
		GXSetVtxAttrFmtv(GX_VTXFMT0, fmt);
	} else {
		GXSetVtxAttrFmtv(GX_VTXFMT0, defFmt);
	}
	GXSetVtxDescv(desc);
}

void CLine::Draw(Mtx view, Vec* pos, u32 color, u16 count)
{
	SetVtxPosClr(view, NULL);
	GXBegin(GX_LINESTRIP, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXColor1u32(color);
	}
}

void CLine::Draw(Mtx view, Vec* pos, u32* color, u16 count)
{
	SetVtxPosClr(view, NULL);
	GXBegin(GX_LINESTRIP, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXColor1u32(color[i]);
	}
}

void CLine::Draw(Mtx view, PrimVertex* vtx, u16 count)
{
	SetVtxPosClr(view, NULL);
	GXBegin(GX_LINESTRIP, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		PrimVertex* v = vtx + i;
		GXPosition2f32(v->pos.x, v->pos.y);
		GX_WRITE_F32(v->pos.z);
		GXColor1u32(v->color);
	}
}

void CTriangle::Draw(Mtx view, Vec* pos, u32 color, u16 count, GXPrimitive type)
{
	SetVtxPosClr(view, NULL);
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXColor1u32(color);
	}
}

void CTriangle::DrawLit(Mtx view, Vec* pos, u32 color, u16 count, GXPrimitive type)
{
	Vec normal;

	SetVtxPosNrmClr(view, NULL);
	CalcNormal(pos, count, &normal);
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXNormal3f32(normal.x, normal.y, normal.z);
		GXColor1u32(color);
	}
}

void CTriangle::Draw(Mtx view, Vec* pos, u32* color, u16 count, GXPrimitive type)
{
	SetVtxPosClr(view, NULL);
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXColor1u32(color[i]);
	}
}

void CTriangle::DrawLit(Mtx view, Vec* pos, u32* color, u16 count, GXPrimitive type)
{
	Vec normal;

	SetVtxPosNrmClr(view, NULL);
	CalcNormal(pos, count, &normal);
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(pos[i].x, pos[i].y, pos[i].z);
		GXNormal3f32(normal.x, normal.y, normal.z);
		GXColor1u32(color[i]);
	}
}

void CTriangle::Draw(Mtx view, PrimVertex* vtx, u16 count, GXPrimitive type)
{
	SetVtxPosClr(view, NULL);
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		PrimVertex* v = vtx + i;
		GXPosition2f32(v->pos.x, v->pos.y);
		GX_WRITE_F32(v->pos.z);
		GXColor1u32(v->color);
	}
}

void CTriangle::DrawLit(Mtx view, PrimVertex* vtx, u16 count, GXPrimitive type)
{
	Vec normal;

	SetVtxPosNrmClr(view, NULL);
	CalcNormal(vtx, count, &normal);
	GXBegin(type, GX_VTXFMT0, count);
	for (int i = 0; i < count; i++) {
		GXPosition3f32(vtx[i].pos.x, vtx[i].pos.y, vtx[i].pos.z);
		GXNormal3f32(normal.x, normal.y, normal.z);
		GXColor1u32(vtx[i].color);
	}
}

void CCircline::Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	CPrimitive::Init(tx, ty, tz, sx, sy, sz, rx, ry, rz);
	Vec zero = { 0.0f, 0.0f, 0.0f };
	mCenter  = zero;
}

void CCircle::Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	CPrimitive::Init(tx, ty, tz, sx, sy, sz, rx, ry, rz);
	Vec zero = { 0.0f, 0.0f, 0.0f };
	mCenter  = zero;
}

void CCircle::Draw(Mtx view, u32 segments, u32 centerColor, u32 edgeColor)
{
	SetVtxPosClr(view, NULL);
	// the hub at the origin, then segments + 1 rim points around mCenter: the last one
	// lands on the first again and closes the fan
	GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, segments + 2);
	GXPosition3f32(0.0f, 0.0f, 0.0f);
	GXColor1u32(centerColor);
	for (int i = 1; i <= segments + 1; i++) {
		f32 angle = (TWO_PI / segments) * i;
		GXPosition3f32(mCenter.x + (f32)sin(angle), mCenter.y + (f32)cos(angle), mCenter.z);
		GXColor1u32(edgeColor);
	}
}

void CCircle::DrawLit(Mtx view, u32 segments, u32 centerColor, u32 edgeColor)
{
	// SetVtxPosNrmClr's default with s8 normals, enough for the fixed (0, 0, 1) every vertex sends
	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_NRM, GX_NRM_XYZ, GX_S8, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};

	SetVtxPosNrmClr(view, fmt);
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

void CTile::Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	CPrimitive::Init(tx, ty, tz, sx, sy, sz, rx, ry, rz);
	Vec zero = { 0.0f, 0.0f, 0.0f };
	mCenter  = zero;
}

void CTile::Draw(Mtx view, u32 color0, u32 color1, u32 color2, u32 color3)
{
	// the same entries as SetVtxPosClr's default, but retail's tile copies and passes its own table
	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};

	SetVtxPosClr(view, fmt);
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

void CTile::DrawLit(Mtx view, u32 color0, u32 color1, u32 color2, u32 color3)
{
	// SetVtxPosNrmClr's default with s8 normals, enough for the fixed (0, 0, 1) every vertex sends
	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_NRM, GX_NRM_XYZ, GX_S8, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};

	SetVtxPosNrmClr(view, fmt);
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

void CSprite::Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz)
{
	CPrimitive::Init(tx, ty, tz, sx, sy, sz, rx, ry, rz);
	mTex     = NULL;
	Vec zero = { 0.0f, 0.0f, 0.0f };
	mCenter  = zero;
}

void CSprite::Draw(Mtx view, GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, u32 color0, u32 color1, u32 color2, u32 color3)
{
	if (mTex != NULL) {
		LoadMtx(view);
		mTex->Draw(map, mtxId, mtxType, &mCenter, color0, color1, color2, color3);
	}
}

void CSprite::DrawLit(Mtx view, GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, u32 color0, u32 color1, u32 color2, u32 color3)
{
	if (mTex != NULL) {
		LoadMtxNrm(view);
		mTex->DrawLit(map, mtxId, mtxType, &mCenter, color0, color1, color2, color3);
	}
}
void CCube::Draw(Mtx view)
{
	LoadMtxNrm(view);
	GXDrawCube();
}

void CSphere::Draw(Mtx view, u8 major, u8 minor)
{
	LoadMtxNrm(view);
	GXDrawSphere(major, minor);
}

void CCylinder::Draw(Mtx view, u8 sides)
{
	LoadMtxNrm(view);
	GXDrawCylinder(sides);
}

#include "game/ccamera.h"
#include "game/cgsdrawstate.h"

extern "C" {
u16 GXGetTexObjWidth(const GXTexObj* obj);
u16 GXGetTexObjHeight(const GXTexObj* obj);
}

// places a flat shape on screen, w and h are its full size. an ortho camera has y growing
// down the screen, so the shape gets flipped back
static inline void SetFlatTransform(CGs* gs, CPrimitive* prim, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz)
{
	if (gs->mCamera->projection_type == PROJ_ORTHO) {
		prim->SetTrans(x, y, z);
		prim->SetScale(w / 2.0f, -h / 2.0f, 1.0f);
		prim->SetRot(-rx, ry, -rz);
	} else {
		prim->SetTrans(x, y, z);
		prim->SetScale(w / 2.0f, h / 2.0f, 1.0f);
		prim->SetRot(rx, ry, rz);
	}
	if (gs->mBillboardEnabled) {
		prim->mBillboard  = 2;
		prim->mBillboardZ = rz;
	} else {
		prim->mBillboard = 0;
	}
	prim->mDirty    = TRUE;
	prim->mRotOrder = gs->mRotOrder;
}

/**
 * @note Address: 0x801992BC
 * @note Size: 0x2C0
 */
extern "C" void DrawGsLineSegment(CGs* gs, Mtx view, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1)
{
	// no matrix means the camera's
	MtxP mtx;
	if (view == NULL) {
		mtx = gs->mCamera->mViewMtx;
	} else {
		mtx = view;
	}
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mLine.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	PrimVertex vtx[2] = {
		{ { x0, y0, z0 }, color0 },
		{ { x1, y1, z1 }, color1 },
	};
	gs->mLine.Draw(mtx, vtx, 2);
}

/**
 * @note Address: 0x801990EC
 * @note Size: 0x1D0
 */
extern "C" void DrawGsLineStrip(CGs* gs, Vec* pos, u32 color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mLine.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mLine.Draw(gs->mCamera->mViewMtx, pos, color, count);
}

/**
 * @note Address: 0x80198F1C
 * @note Size: 0x1D0
 */
extern "C" void DrawGsColoredLineStrip(CGs* gs, Vec* pos, u32* color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mLine.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mLine.Draw(gs->mCamera->mViewMtx, pos, color, count);
}

/**
 * @note Address: 0x80198D08
 * @note Size: 0x214
 */
extern "C" void DrawGsTrianglePrimitive(CGs* gs, GXPrimitive type, Mtx view, Vec* pos, u32 color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mTriangle.DrawLit(view, pos, color, count, type);
	} else {
		gs->mTriangle.Draw(view, pos, color, count, type);
	}
}

/**
 * @note Address: 0x80198AF4
 * @note Size: 0x214
 */
extern "C" void DrawGsColoredTrianglePrimitive(CGs* gs, GXPrimitive type, Mtx view, Vec* pos, u32* color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mTriangle.DrawLit(view, pos, color, count, type);
	} else {
		gs->mTriangle.Draw(view, pos, color, count, type);
	}
}

/**
 * @note Address: 0x801988EC
 * @note Size: 0x208
 */
extern "C" void DrawGsTriangleVertices(CGs* gs, GXPrimitive type, Mtx view, PrimVertex* vtx, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mTriangle.DrawLit(view, vtx, count, type);
	} else {
		gs->mTriangle.Draw(view, vtx, count, type);
	}
}

/**
 * @note Address: 0x80198614
 * @note Size: 0x2D8
 */
extern "C" void DrawGsVertexQuad(CGs* gs, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1, f32 x2, f32 y2, f32 z2,
                                 u32 color2, f32 x3, f32 y3, f32 z3, u32 color3)
{
	PrimVertex vtx[4] = {
		{ { x0, y0, z0 }, color0 },
		{ { x1, y1, z1 }, color1 },
		{ { x2, y2, z2 }, color2 },
		{ { x3, y3, z3 }, color3 },
	};
	MtxP view = gs->mCamera->mViewMtx;

	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mTriangle.DrawLit(view, vtx, 4, GX_TRIANGLEFAN);
	} else {
		gs->mTriangle.Draw(view, vtx, 4, GX_TRIANGLEFAN);
	}
}

/**
 * @note Address: 0x80198258
 * @note Size: 0x3BC
 */
extern "C" void DrawGsCircle(CGs* gs, u32 segments, u32 centerColor, u32 edgeColor, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry,
                             f32 rz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	SetFlatTransform(gs, &gs->mCircle, x, y, z, w, h, rx, ry, rz);
	gs->mCircle.mFixedMtx = NULL;
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mCircle.DrawLit(gs->mCamera->mViewMtx, segments, centerColor, edgeColor);
	} else {
		gs->mCircle.Draw(gs->mCamera->mViewMtx, segments, centerColor, edgeColor);
	}
}

/**
 * @note Address: 0x80197E90
 * @note Size: 0x3C8
 */
extern "C" void DrawGsQuad(CGs* gs, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry,
                           f32 rz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	SetFlatTransform(gs, &gs->mTile, x, y, z, w, h, rx, ry, rz);
	gs->mTile.mFixedMtx = NULL;
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mTile.DrawLit(gs->mCamera->mViewMtx, color0, color1, color2, color3);
	} else {
		gs->mTile.Draw(gs->mCamera->mViewMtx, color0, color1, color2, color3);
	}
}

/**
 * @note Address: 0x80197800
 * @note Size: 0x690
 */
extern "C" void DrawGsTexturedQuad(CGs* gs, CTexObj* tex, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h,
                                   f32 rx, f32 ry, f32 rz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		gs->mCurTevStage->SetNumTexGens(1);
		gs->mCurTevStage->SetTexGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
	}
	SetupDrawState(gs);

	// negative size means use the texture's own
	if (w < 0.0f) {
		u16 size = GXGetTexObjWidth(tex);
		w        = size;
	}
	if (h < 0.0f) {
		u16 size = GXGetTexObjHeight(tex);
		h        = size;
	}

	SetFlatTransform(gs, &gs->mSprite2, x, y, z, w, h, rx, ry, rz);
	gs->mSprite2.mTex      = tex;
	gs->mSprite2.mFixedMtx = NULL;
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mSprite2.DrawLit(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, color0, color1, color2, color3);
	} else {
		gs->mSprite2.Draw(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, color0, color1, color2, color3);
	}

	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		gs->mCurTevStage->SetNumTexGens(0);
	}
}
/**
 * @note Address: 0x80197538
 * @note Size: 0x2C8
 */
extern "C" void DrawGsCube(CGs* gs, u32 color, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mCube.SetTrans(tx, ty, tz);
	gs->mCube.SetScale(sx, sy, sz);
	gs->mCube.SetRot(rx, ry, rz);
	gs->mCube.mRotOrder = gs->mRotOrder;
	gs->mCube.mFixedMtx = NULL;
	gs->mCube.Draw(gs->mCamera->mViewMtx);
}

/**
 * @note Address: 0x80197250
 * @note Size: 0x2E8
 */
extern "C" void DrawGsSphere(CGs* gs, u32 color, u8 major, u8 minor, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mSphere.SetTrans(tx, ty, tz);
	gs->mSphere.SetScale(sx, sy, sz);
	gs->mSphere.SetRot(rx, ry, rz);
	gs->mSphere.mRotOrder = gs->mRotOrder;
	gs->mSphere.mFixedMtx = NULL;
	gs->mSphere.Draw(gs->mCamera->mViewMtx, major, minor);
}

/**
 * @note Address: 0x801970B0
 * @note Size: 0x1A0
 */
extern "C" void DrawGsSphereWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 major, u8 minor)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mSphere.mFixedMtx = mtx;
	gs->mSphere.Draw(gs->mCamera->mViewMtx, major, minor);
}

/**
 * @note Address: 0x80196DD8
 * @note Size: 0x2D8
 */
extern "C" void DrawGsCylinder(CGs* gs, u32 color, u8 sides, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mCylinder.SetTrans(tx, ty, tz);
	gs->mCylinder.SetScale(sx, sy, sz);
	gs->mCylinder.SetRot(rx, ry, rz);
	gs->mCylinder.mRotOrder = gs->mRotOrder;
	gs->mCylinder.mFixedMtx = NULL;
	gs->mCylinder.Draw(gs->mCamera->mViewMtx, sides);
}

/**
 * @note Address: 0x80196C28
 * @note Size: 0x1B0
 */
extern "C" void DrawGsCylinderWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 sides)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mCylinder.mFixedMtx = mtx;
	gs->mCylinder.Draw(gs->mCamera->mViewMtx, sides);
}

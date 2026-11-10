#include "game/fbeffect.h"
#include "game/mm.h"
#include "Dolphin/mtx.h"
#include <math.h>
#include <stdlib.h>

// todo: cleanup
// a quad over the whole w by h screen with the whole texture on it
// 0 and 1 sit in locals, as literals mwcc reloads them for every store
static inline void DrawQuad(f32 w, f32 h)
{
	f32 zero = 0.0f;
	f32 one  = 1.0f;
	f32 x    = w;
	f32 y    = h;
	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition2f32(zero, zero);
	GXTexCoord2f32(zero, zero);
	GXPosition2f32(x, zero);
	GXTexCoord2f32(one, zero);
	GXPosition2f32(x, y);
	GXTexCoord2f32(one, one);
	GXPosition2f32(zero, y);
	GXTexCoord2f32(zero, one);
	GXEnd();
}

// todo: cleanup
// grabs the efb into mBuf
inline void CFbOutline::Copy(GXTexFmt format)
{
	GXSetTexCopySrc(0, 0, mWidth, mHeight);
	GXSetTexCopyDst(mWidth, mHeight, format, GX_FALSE);
	GXCopyTex(mBuf, GX_FALSE);
	GXPixModeSync();
	GXInvalidateTexAll();
}

/**
 * @note Address: 0x801A886C
 * @note Size: 0xB8
 */
void CFbBlur::Init(u16 w, u16 h)
{
	mWidth  = w;
	mHeight = h;
	mBuf    = mm::Alloc(mWidth * mHeight * 2, "(blur)");
	GXInitTexObj(&mTexObj, mBuf, mWidth, mHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&mTexObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
	mAlpha = mTarget = mSpeed = 0.0f;
	mOffsetX = mOffsetY = 0.0f;
	mValid              = 0;
}

#if defined(VERSION_PAL)
/**
 * @note Address: N/A
 * @note Size: 0x7C
 */
// europe only, at 0x801AB0EC there. the pal screen is a different size so CGame resizes the buffer
void CFbBlur::SetSize(u16 w, u16 h)
{
	mWidth  = w;
	mHeight = h;
	GXInitTexObj(&mTexObj, mBuf, mWidth, mHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&mTexObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
}
#endif

/**
 * @note Address: 0x801A87A0
 * @note Size: 0xCC
 */
void CFbBlur::Update(u32 frames)
{
	if (mAlpha < mTarget) {
		if ((mAlpha += mSpeed * frames) >= mTarget) {
			mAlpha = mTarget;
			mSpeed = 0.0f;
		}
	} else if (mAlpha > mTarget) {
		if ((mAlpha -= mSpeed * frames) <= mTarget) {
			mAlpha = mTarget;
			mSpeed = 0.0f;
		}
	}

	if (mValid && mAlpha <= 0.0f) {
		mValid = 0;
	}
}

/**
 * @note Address: 0x801A839C
 * @note Size: 0x404
 */
#if defined(VERSION_PAL)
void CFbBlur::Draw()
#else
void CFbBlur::Draw(BOOL half)
#endif
{
	if (mAlpha != 0.0f) {
		if (mValid) {
			// last frame, a bit bigger and faded over this one
			GXLoadTexObj(&mTexObj, GX_TEXMAP0);

			f32 w = mWidth + mOffsetX;
			f32 h = mHeight + mOffsetY;
			Mtx m;
			Mtx trans;
			Mtx scale;
			PSMTXTrans(trans, (mWidth - w) / 2, (mHeight - h) / 2, 0.0f);
			PSMTXScale(scale, w, h, 1.0f);
			PSMTXConcat(trans, scale, m);
			GXLoadPosMtxImm(m, GX_PNMTX0);

			GXColor color = { 0, 0, 0, 0 };
			color.a       = 255.0f * mAlpha;
			GXSetTevColor(GX_TEVREG0, color);
			GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
			GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 255);
			GXSetAlphaUpdate(GX_FALSE);
			GXSetDstAlpha(GX_FALSE, 0);
			GXSetNumChans(0);
			GXSetNumTexGens(1);
			GXSetNumTevStages(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetCurrentMtx(GX_PNMTX0);

			Mtx44 proj;
			C_MTXOrtho(proj, 0.0f, mHeight, 0.0f, mWidth, 0.0f, 1.0f);
			GXSetProjection(proj, GX_ORTHOGRAPHIC);
			Mtx texMtx;
			PSMTXIdentity(texMtx);
			GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX2x4);

			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
			GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
			GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
			GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
			DrawQuad(1.0f, 1.0f);
		}
#if defined(VERSION_PAL)
		Copy();
#else
		Copy(half);
#endif
	}
}

/**
 * @note Address: 0x801A82F0
 * @note Size: 0xAC
 */
#if defined(VERSION_PAL)
void CFbBlur::Copy()
{
	// pal always grabs the efb at twice the buffer size, box filtered down
	GXSetTexCopySrc(0, 0, mWidth * 2, mHeight * 2);
	GXSetTexCopyDst(mWidth, mHeight, GX_TF_RGB565, GX_TRUE);
#else
void CFbBlur::Copy(BOOL half)
{
	if (half) {
		GXSetTexCopySrc(0, 0, mWidth * 2, mHeight * 2);
		GXSetTexCopyDst(mWidth, mHeight, GX_TF_RGB565, GX_TRUE);
	} else {
		GXSetTexCopySrc(0, 0, mWidth, mHeight);
		GXSetTexCopyDst(mWidth, mHeight, GX_TF_RGB565, GX_FALSE);
	}
#endif
	GXCopyTex(mBuf, GX_FALSE);
	if (!mValid) {
		mValid = 1;
	}
}

/**
 * @note Address: 0x801A7FC4
 * @note Size: 0x32C
 */
void CFbBlur::DrawTex()
{
	GXLoadTexObj(&mTexObj, GX_TEXMAP0);

	f32 w = mWidth;
	f32 h = mHeight;
	Mtx m;
	Mtx trans;
	Mtx scale;
	PSMTXTrans(trans, (mWidth - w) / 2, (mHeight - h) / 2, 0.0f);
	PSMTXScale(scale, w, h, 1.0f);
	PSMTXConcat(trans, scale, m);
	GXLoadPosMtxImm(m, GX_PNMTX0);

	GXColor black = { 0, 0, 0, 255 };
	GXSetTevColor(GX_TEVREG0, black);
	GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 255);
	GXSetAlphaUpdate(GX_FALSE);
	GXSetDstAlpha(GX_FALSE, 0);
	GXSetDither(GX_TRUE);
	GXSetNumChans(0);
	GXSetNumTexGens(1);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GXSetCurrentMtx(GX_PNMTX0);

	Mtx44 proj;
	C_MTXOrtho(proj, 0.0f, mHeight, 0.0f, mWidth, 0.0f, 1.0f);
	GXSetProjection(proj, GX_ORTHOGRAPHIC);
	Mtx texMtx;
	PSMTXIdentity(texMtx);
	GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX2x4);

	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
	DrawQuad(1.0f, 1.0f);
}

/**
 * @note Address: 0x801A7E5C
 * @note Size: 0x168
 */
void CFbOutline::Init(u16 w, u16 h)
{
	mWidth  = w;
	mHeight = h;
	mBuf    = (u8*)mm::Alloc(mWidth * mHeight, "(outline)");
	GXInitTexObj(&mTexObj, mBuf, mWidth, mHeight, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&mTexObj, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);

	GXColor line = { 0x22, 0x09, 0x19, 0x04 };
	mColor0      = line;
	GXColor tint = { 0xC8, 0x00, 0xFF, 0x80 };
	mColor1      = tint;
	GXColor fill = { 0x64, 0x64, 0xC8, 0x80 };
	mColor2      = fill;
	GXColor off  = { 0 };
	mColor3      = off;

	mThick         = 2.0f;
	mThick2        = 0.0f;
	mPatternTex    = NULL;
	mPatternTransY = 0.0f;
	mPatternTransX = 0.0f;
	mPatternScaleY = 0.0f;
	mPatternScaleX = 0.0f;
	mZoom          = 1.0f;
}

#if defined(VERSION_PAL)
/**
 * @note Address: N/A
 * @note Size: 0x7C
 */
// europe only, at 0x801AA6D4 there. CGame calls it with the render mode size
void CFbOutline::SetSize(u16 w, u16 h)
{
	mWidth  = w;
	mHeight = h;
	GXInitTexObj(&mTexObj, mBuf, mWidth, mHeight, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&mTexObj, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
}
#endif

/**
 * @note Address: 0x801A7C40
 * @note Size: 0x21C
 */
void CFbOutline::Update(s32 frames)
{
	if (mPatternTex != NULL) {
		// new random placement for the pattern every 4 frames
		if (mFrame == 0) {
			mPatternScaleX = mWidth / GXGetTexObjWidth(mPatternTex);
			mPatternScaleY = mHeight / GXGetTexObjHeight(mPatternTex);
			mPatternScaleX *= 1.3f + (rand() % 200) / 1000.0f;
			mPatternScaleY *= 1.3f + (rand() % 200) / 1000.0f;
			mPatternTransX = (rand() % 300) / 100.0f;
			mPatternTransY = (rand() % 300) / 100.0f;
		}
		if ((mFrame += frames) > 3) {
			mFrame = 0;
		}
	}
}

/**
 * @note Address: 0x801A66A4
 * @note Size: 0x159C
 */
void CFbOutline::Draw(u32 flags)
{
	// NONMATCHING: retail reloads the offsets for every vertex
	Mtx mtx;
	Mtx44 proj;
	Mtx texMtx;

	if (flags & 3) {
		GXTexFmt format = (flags & 1) ? GX_CTF_A8 : GX_TF_I8;
		Copy(format);
	}

	f32 w = mWidth;
	f32 h = mHeight;
	GXLoadTexObj(&mTexObj, GX_TEXMAP0);
	PSMTXIdentity(mtx);
	GXLoadPosMtxImm(mtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	C_MTXOrtho(proj, 0.0f, mHeight, 0.0f, mWidth, 0.0f, 1.0f);
	GXSetProjection(proj, GX_ORTHOGRAPHIC);
	PSMTXIdentity(texMtx);
	GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX2x4);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
	GXSetColorUpdate(GX_TRUE);
	GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 255);
	GXSetAlphaUpdate(GX_FALSE);
	GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	// outline from the id in alpha
	if (flags & 4) {
		if (mThick == 0.0f) {
			static const GXColor white = { 0xFF, 0xFF, 0xFF, 0x04 };
			GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
			GXSetTevColor(GX_TEVREG0, white);
			GXSetTevColor(GX_TEVREG1, mColor0);
			GXSetNumChans(0);
			GXSetNumTexGens(1);
			GXSetNumTevStages(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_A1, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_GT, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			DrawQuad(w, h);
		} else {
			Mtx offsetX;
			Mtx offsetY;
			PSMTXIdentity(offsetX);
			offsetX[0][3] = mThick / mWidth;
			PSMTXIdentity(offsetY);
			offsetY[1][3] = mThick / mHeight;
			GXLoadTexMtxImm(offsetX, GX_TEXMTX1, GX_MTX2x4);
			GXLoadTexMtxImm(offsetY, GX_TEXMTX2, GX_MTX2x4);
			GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetTevColor(GX_TEVREG0, mColor0);
			GXSetNumChans(0);
			GXSetNumTexGens(2);
			GXSetNumTevStages(3);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_GT, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVREG1);
			GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_TEXA, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_COMP_A8_GT, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD1, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_C0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_A1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
			GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX1, GX_FALSE, GX_PTIDENTITY);
			DrawQuad(w, h);
			GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX2, GX_FALSE, GX_PTIDENTITY);
			DrawQuad(w, h);
		}
	}

	// tint everything whose id matches
	if (flags & 8) {
		GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		GXSetTevColor(GX_TEVREG0, mColor0);
		GXSetTevColor(GX_TEVREG1, mColor1);
		GXSetNumChans(0);
		GXSetNumTexGens(1);
		GXSetNumTevStages(1);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
		GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
		GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_A1, GX_CA_ZERO);
		GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_EQ, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
		DrawQuad(w, h);
	}

	if (flags & 0x10) {
		if (mThick2 == 0.0f) {
			static const GXColor white = { 0xFF, 0xFF, 0xFF, 0x00 };
			GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetTevColor(GX_TEVREG0, white);
			GXSetTevColor(GX_TEVREG1, mColor2);
			if (mPatternTex != NULL) {
				Mtx pattern;
				Mtx trans;
				Mtx rot;
				GXSetNumChans(0);
				GXSetNumTexGens(2);
				GXSetNumTevStages(2);
				GXLoadTexObj(mPatternTex, GX_TEXMAP1);
				PSMTXScale(pattern, mPatternScaleX, mPatternScaleY, 1.0f);
				PSMTXTrans(trans, mPatternTransX / mWidth, mPatternTransY / mHeight, 0.0f);
				PSMTXConcat(pattern, trans, pattern);
				PSMTXRotRad(rot, 'z', MTXDegToRad(-15.0f));
				PSMTXConcat(pattern, rot, pattern);
				GXLoadTexMtxImm(pattern, GX_TEXMTX1, GX_MTX2x4);
				GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX1, GX_FALSE, GX_PTIDENTITY);
				GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
				GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
				GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
				GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_A1, GX_CA_ZERO);
				GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_EQ, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
				GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
				GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ZERO);
				GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
				GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
				GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			} else {
				GXSetNumChans(0);
				GXSetNumTexGens(1);
				GXSetNumTevStages(1);
				GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
				GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
				GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
				GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_A1, GX_CA_ZERO);
				GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_EQ, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			}
			DrawQuad(w, h);
		} else {
			Mtx offsetX;
			Mtx offsetY;
			PSMTXIdentity(offsetX);
			offsetX[0][3] = mThick2 / mWidth;
			PSMTXIdentity(offsetY);
			f32 thick = mThick2;
			if (thick < 0.0f) {
				thick = -thick;
			}
			offsetY[1][3] = thick / mHeight;
			GXLoadTexMtxImm(offsetX, GX_TEXMTX1, GX_MTX2x4);
			GXLoadTexMtxImm(offsetY, GX_TEXMTX2, GX_MTX2x4);
			if (mThick2 > 0.0f) {
				GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			} else {
				GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			}
			static const GXColor white = { 0xFF, 0xFF, 0xFF, 0x00 };
			GXSetTevColor(GX_TEVREG0, white);
			GXSetTevColor(GX_TEVREG1, mColor2);
			GXSetNumChans(0);
			GXSetNumTexGens(2);
			GXSetNumTevStages(3);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_GT, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVREG1);
			GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_TEXA, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_COMP_A8_GT, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD1, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_A1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
			GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX1, GX_FALSE, GX_PTIDENTITY);
			DrawQuad(w, h);
			GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX2, GX_FALSE, GX_PTIDENTITY);
			DrawQuad(w, h);
		}
	}

	if ((flags & 0x20) && mColor3.a != 0) {
		GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		GXSetTevColor(GX_TEVREG0, mColor3);
		GXSetNumChans(0);
		GXSetNumTexGens(1);
		GXSetNumTevStages(1);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
		GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_ZERO);
		GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
		GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
		DrawQuad(w, h);
	}

	if ((flags & 0xC0) && mColor4.a != 0) {
		f32 zoom  = mZoom;
		f32 zoomW = w * zoom;
		f32 zoomH = h * zoom;
		f32 zoomU = zoomW / w;
		f32 zoomV = zoomH / h;

		// draw the screen shrunk into the middle and grab it back
		if (flags & 0x40) {
			GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetDither(GX_TRUE);
			GXSetNumChans(0);
			GXSetNumTexGens(1);
			GXSetNumTevStages(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ONE, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			f32 zero = 0.0f;
			f32 one  = 1.0f;
			f32 x    = (w - zoomW) / 2;
			f32 y    = (h - zoomH) / 2;
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			GXPosition2f32(x, y);
			GXTexCoord2f32(zero, zero);
			GXPosition2f32(w - x, y);
			GXTexCoord2f32(one, zero);
			GXPosition2f32(w - x, h - y);
			GXTexCoord2f32(one, one);
			GXPosition2f32(x, h - y);
			GXTexCoord2f32(zero, one);
			GXEnd();
			Copy(GX_TF_I8);
		}

		// fade the zoomed copy over the screen
		if (flags & 0x80) {
			GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetDither(GX_TRUE);
			GXColor color = mColor4;
			color.a       = 255.0f * (f32)sin(PI * (color.a / 255.0f) / 2);
			GXSetTevColor(GX_TEVREG0, color);
			GXSetNumChans(0);
			GXSetNumTexGens(1);
			GXSetNumTevStages(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			f32 zero = 0.0f;
			f32 one  = 1.0f;
			f32 u    = (1.0f - zoomU) / 2;
			f32 v    = (1.0f - zoomV) / 2;
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			GXPosition2f32(zero, zero);
			GXTexCoord2f32(u, v);
			GXPosition2f32(w, zero);
			GXTexCoord2f32(one - u, v);
			GXPosition2f32(w, h);
			GXTexCoord2f32(one - u, one - v);
			GXPosition2f32(zero, h);
			GXTexCoord2f32(u, one - v);
			GXEnd();
		}
	}

	// ghosts of the outline circling around it
	if (flags & 0x100) {
		static const GXColor color0 = { 0xFF, 0xFF, 0xFF, 0x08 };
		static const GXColor color1 = { 0xFF, 0xFF, 0xFF, 0x40 };
		GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
		GXSetTevColor(GX_TEVREG0, color0);
		GXSetTevColor(GX_TEVREG1, color1);
		GXSetNumChans(0);
		GXSetNumTexGens(1);
		GXSetNumTevStages(1);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
		GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
		GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_A1, GX_CA_ZERO);
		GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_EQ, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);

		static f32 rot   = 0.0f;
		f32 offset[4][2] = {
			{ -3.0f, -3.0f },
			{ 3.0f, -3.0f },
			{ 3.0f, 3.0f },
			{ -3.0f, 3.0f },
		};
		int i;
		for (i = 0; i < 4; i++) {
			offset[i][0] = 3.0f * (f32)sin(rot + i * (PI / 2));
			offset[i][1] = 3.0f * (f32)cos(rot + i * (PI / 2));
		}
		rot += MTXDegToRad(2.0f);

		f32 zero = 0.0f;
		f32 one  = 1.0f;
		GXBegin(GX_QUADS, GX_VTXFMT0, 16);
		for (i = 0; i < 4; i++) {
			GXPosition2f32(offset[i][0], offset[i][1]);
			GXTexCoord2f32(zero, zero);
			GXPosition2f32(w + offset[i][0], offset[i][1]);
			GXTexCoord2f32(one, zero);
			GXPosition2f32(w + offset[i][0], h + offset[i][1]);
			GXTexCoord2f32(one, one);
			GXPosition2f32(offset[i][0], h + offset[i][1]);
			GXTexCoord2f32(zero, one);
		}
		GXEnd();
	}
}
// fn_801A66A4
/*
    stwu     r1, -0x2e0(r1)
    mflr     r0
    stw      r0, 0x2e4(r1)
    stfd     f31, 0x2d0(r1)
    psq_st   f31, 0x2d8(r1), 0, qr0
    stfd     f30, 0x2c0(r1)
    psq_st   f30, 0x2c8(r1), 0, qr0
    stfd     f29, 0x2b0(r1)
    psq_st   f29, 0x2b8(r1), 0, qr0
    stfd     f28, 0x2a0(r1)
    psq_st   f28, 0x2a8(r1), 0, qr0
    stfd     f27, 0x290(r1)
    psq_st   f27, 0x298(r1), 0, qr0
    stfd     f26, 0x280(r1)
    psq_st   f26, 0x288(r1), 0, qr0
    stw      r31, 0x27c(r1)
    stw      r30, 0x278(r1)
    stw      r29, 0x274(r1)
    stw      r28, 0x270(r1)
    clrlwi.  r0, r4, 30
    mr       r31, r4
    mr       r30, r3
    beq      .L_801A674C
    clrlwi.  r0, r31, 31
    li       r29, 0x1
    beq      .L_801A6710
    li       r29, 0x27
.L_801A6710:
    lhz      r5, 0x0(r30)
    li       r3, 0x0
    lhz      r6, 0x2(r30)
    li       r4, 0x0
    bl       GXSetTexCopySrc
    lhz      r3, 0x0(r30)
    mr       r5, r29
    lhz      r4, 0x2(r30)
    li       r6, 0x0
    bl       GXSetTexCopyDst
    lwz      r3, 0x4(r30)
    li       r4, 0x0
    bl       GXCopyTex
    bl       GXPixModeSync
    bl       GXInvalidateTexAll
.L_801A674C:
    lhz      r4, 0x0(r30)
    lis      r5, 0x4330
    lhz      r0, 0x2(r30)
    addi     r3, r30, 0x8
    stw      r4, 0x254(r1)
    li       r4, 0x0
    lfd      f1, sFbEffectUnsignedConversionBias@sda21(r0)
    stw      r5, 0x250(r1)
    lfd      f0, 0x250(r1)
    stw      r0, 0x25c(r1)
    fsubs    f31, f0, f1
    stw      r5, 0x258(r1)
    lfd      f0, 0x258(r1)
    fsubs    f30, f0, f1
    bl       GXLoadTexObj
    addi     r3, r1, 0x220
    bl       PSMTXIdentity
    addi     r3, r1, 0x220
    li       r4, 0x0
    bl       GXLoadPosMtxImm
    li       r3, 0x0
    bl       GXSetCurrentMtx
    lhz      r5, 0x2(r30)
    lis      r4, 0x4330
    lhz      r0, 0x0(r30)
    addi     r3, r1, 0x1e0
    stw      r5, 0x264(r1)
    lfs      f1, sFbEffectZero@sda21(r0)
    stw      r4, 0x260(r1)
    lfd      f4, sFbEffectUnsignedConversionBias@sda21(r0)
    fmr      f3, f1
    lfd      f0, 0x260(r1)
    fmr      f5, f1
    stw      r0, 0x26c(r1)
    fsubs    f2, f0, f4
    lfs      f6, sFbEffectOne@sda21(r0)
    stw      r4, 0x268(r1)
    lfd      f0, 0x268(r1)
    fsubs    f4, f0, f4
    bl       C_MTXOrtho
    addi     r3, r1, 0x1e0
    li       r4, 0x1
    bl       GXSetProjection
    addi     r3, r1, 0x1b0
    bl       PSMTXIdentity
    addi     r3, r1, 0x1b0
    li       r4, 0x1e
    li       r5, 0x1
    bl       GXLoadTexMtxImm
    li       r3, 0x0
    li       r4, 0x1
    li       r5, 0x4
    li       r6, 0x1e
    li       r7, 0x0
    li       r8, 0x7d
    bl       GXSetTexCoordGen2
    li       r3, 0x1
    bl       GXSetColorUpdate
    li       r3, 0x4
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x3
    li       r7, 0xff
    bl       GXSetAlphaCompare
    li       r3, 0x0
    bl       GXSetAlphaUpdate
    li       r3, 0x0
    li       r4, 0x7
    li       r5, 0x0
    bl       GXSetZMode
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xd
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0x0
    li       r4, 0x9
    li       r5, 0x0
    li       r6, 0x4
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x0
    li       r4, 0xd
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    rlwinm.  r0, r31, 0, 29, 29
    beq      .L_801A6D1C
    lfs      f1, sFbEffectZero@sda21(r0)
    lfs      f0, 0x3c(r30)
    fcmpu    cr0, f1, f0
    bne      .L_801A69F8
    li       r3, 0x1
    li       r4, 0x2
    li       r5, 0x4
    li       r6, 0xf
    bl       GXSetBlendMode
    lwz      r0, sOutlineLineWhite@sda21(r0)
    addi     r4, r1, 0x38
    li       r3, 0x1
    stw      r0, 0x38(r1)
    bl       GXSetTevColor
    lwz      r0, 0x28(r30)
    addi     r4, r1, 0x34
    li       r3, 0x2
    stw      r0, 0x34(r1)
    bl       GXSetTevColor
    li       r3, 0x0
    bl       GXSetNumChans
    li       r3, 0x1
    bl       GXSetNumTexGens
    li       r3, 0x1
    bl       GXSetNumTevStages
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0x1
    li       r6, 0x2
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x0
    li       r4, 0xe
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lfs      f1, sFbEffectZero@sda21(r0)
    lis      r3, 0xcc01
    lfs      f0, sFbEffectOne@sda21(r0)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    b        .L_801A6D1C
.L_801A69F8:
    addi     r3, r1, 0x180
    bl       PSMTXIdentity
    lhz      r4, 0x0(r30)
    lis      r0, 0x4330
    stw      r0, 0x268(r1)
    addi     r3, r1, 0x150
    lfd      f1, sFbEffectUnsignedConversionBias@sda21(r0)
    stw      r4, 0x26c(r1)
    lfs      f2, 0x3c(r30)
    lfd      f0, 0x268(r1)
    fsubs    f0, f0, f1
    fdivs    f0, f2, f0
    stfs     f0, 0x18c(r1)
    bl       PSMTXIdentity
    lhz      r5, 0x2(r30)
    lis      r0, 0x4330
    stw      r0, 0x260(r1)
    addi     r3, r1, 0x180
    lfd      f1, sFbEffectUnsignedConversionBias@sda21(r0)
    li       r4, 0x21
    stw      r5, 0x264(r1)
    li       r5, 0x1
    lfs      f2, 0x3c(r30)
    lfd      f0, 0x260(r1)
    fsubs    f0, f0, f1
    fdivs    f0, f2, f0
    stfs     f0, 0x16c(r1)
    bl       GXLoadTexMtxImm
    addi     r3, r1, 0x150
    li       r4, 0x24
    li       r5, 0x1
    bl       GXLoadTexMtxImm
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0x5
    li       r6, 0xf
    bl       GXSetBlendMode
    lwz      r0, 0x28(r30)
    addi     r4, r1, 0x30
    li       r3, 0x1
    stw      r0, 0x30(r1)
    bl       GXSetTevColor
    li       r3, 0x0
    bl       GXSetNumChans
    li       r3, 0x2
    bl       GXSetNumTexGens
    li       r3, 0x3
    bl       GXSetNumTevStages
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x0
    li       r4, 0x8
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x0
    li       r4, 0xe
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x2
    bl       GXSetTevAlphaOp
    li       r3, 0x1
    li       r4, 0x1
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x1
    li       r4, 0x8
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x1
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x1
    li       r4, 0x4
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x1
    li       r4, 0xe
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    li       r3, 0x2
    li       r4, 0x1
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x2
    li       r4, 0x2
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x2
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x2
    li       r4, 0x2
    li       r5, 0x7
    li       r6, 0x7
    li       r7, 0x0
    bl       GXSetTevAlphaIn
    li       r3, 0x2
    li       r4, 0x1
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    li       r3, 0x1
    li       r4, 0x1
    li       r5, 0x4
    li       r6, 0x21
    li       r7, 0x0
    li       r8, 0x7d
    bl       GXSetTexCoordGen2
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lfs      f1, sFbEffectZero@sda21(r0)
    lis      r9, 0xcc01
    lfs      f0, sFbEffectOne@sda21(r0)
    li       r3, 0x1
    stfs     f1, -0x8000(r9)
    li       r4, 0x1
    li       r5, 0x4
    li       r6, 0x24
    stfs     f1, -0x8000(r9)
    li       r7, 0x0
    li       r8, 0x7d
    stfs     f1, -0x8000(r9)
    stfs     f1, -0x8000(r9)
    stfs     f31, -0x8000(r9)
    stfs     f1, -0x8000(r9)
    stfs     f0, -0x8000(r9)
    stfs     f1, -0x8000(r9)
    stfs     f31, -0x8000(r9)
    stfs     f30, -0x8000(r9)
    stfs     f0, -0x8000(r9)
    stfs     f0, -0x8000(r9)
    stfs     f1, -0x8000(r9)
    stfs     f30, -0x8000(r9)
    stfs     f1, -0x8000(r9)
    stfs     f0, -0x8000(r9)
    bl       GXSetTexCoordGen2
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lfs      f1, sFbEffectZero@sda21(r0)
    lis      r3, 0xcc01
    lfs      f0, sFbEffectOne@sda21(r0)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
.L_801A6D1C:
    rlwinm.  r0, r31, 0, 28, 28
    beq      .L_801A6E50
    li       r3, 0x1
    li       r4, 0x4
    li       r5, 0x5
    li       r6, 0xf
    bl       GXSetBlendMode
    lwz      r0, 0x28(r30)
    addi     r4, r1, 0x2c
    li       r3, 0x1
    stw      r0, 0x2c(r1)
    bl       GXSetTevColor
    lwz      r0, 0x2c(r30)
    addi     r4, r1, 0x28
    li       r3, 0x2
    stw      r0, 0x28(r1)
    bl       GXSetTevColor
    li       r3, 0x0
    bl       GXSetNumChans
    li       r3, 0x1
    bl       GXSetNumTexGens
    li       r3, 0x1
    bl       GXSetNumTevStages
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0x1
    li       r6, 0x2
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x0
    li       r4, 0xf
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lfs      f1, sFbEffectZero@sda21(r0)
    lis      r3, 0xcc01
    lfs      f0, sFbEffectOne@sda21(r0)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
.L_801A6E50:
    rlwinm.  r0, r31, 0, 27, 27
    beq      .L_801A74F0
    lfs      f1, sFbEffectZero@sda21(r0)
    lfs      f0, 0x40(r30)
    fcmpu    cr0, f1, f0
    bne      .L_801A7180
    li       r3, 0x1
    li       r4, 0x4
    li       r5, 0x5
    li       r6, 0xf
    bl       GXSetBlendMode
    lwz      r0, sOutlineFillWhite@sda21(r0)
    addi     r4, r1, 0x24
    li       r3, 0x1
    stw      r0, 0x24(r1)
    bl       GXSetTevColor
    lwz      r0, 0x30(r30)
    addi     r4, r1, 0x20
    li       r3, 0x2
    stw      r0, 0x20(r1)
    bl       GXSetTevColor
    lwz      r0, 0x44(r30)
    cmplwi   r0, 0x0
    beq      .L_801A708C
    li       r3, 0x0
    bl       GXSetNumChans
    li       r3, 0x2
    bl       GXSetNumTexGens
    li       r3, 0x2
    bl       GXSetNumTevStages
    lwz      r3, 0x44(r30)
    li       r4, 0x1
    bl       GXLoadTexObj
    lfs      f1, 0x50(r30)
    addi     r3, r1, 0x120
    lfs      f2, 0x54(r30)
    lfs      f3, sFbEffectOne@sda21(r0)
    bl       PSMTXScale
    lhz      r5, 0x0(r30)
    lis      r4, 0x4330
    lhz      r0, 0x2(r30)
    addi     r3, r1, 0xf0
    stw      r5, 0x26c(r1)
    lfd      f4, sFbEffectUnsignedConversionBias@sda21(r0)
    stw      r4, 0x268(r1)
    lfs      f5, 0x48(r30)
    lfd      f0, 0x268(r1)
    stw      r0, 0x264(r1)
    fsubs    f1, f0, f4
    lfs      f2, 0x4c(r30)
    stw      r4, 0x260(r1)
    lfs      f3, sFbEffectZero@sda21(r0)
    lfd      f0, 0x260(r1)
    fdivs    f1, f5, f1
    fsubs    f0, f0, f4
    fdivs    f2, f2, f0
    bl       PSMTXTrans
    addi     r3, r1, 0x120
    addi     r4, r1, 0xf0
    mr       r5, r3
    bl       PSMTXConcat
    lfs      f1, sFbEffectPatternRotation@sda21(r0)
    addi     r3, r1, 0xc0
    li       r4, 0x7a
    bl       PSMTXRotRad
    addi     r3, r1, 0x120
    addi     r4, r1, 0xc0
    mr       r5, r3
    bl       PSMTXConcat
    addi     r3, r1, 0x120
    li       r4, 0x21
    li       r5, 0x1
    bl       GXLoadTexMtxImm
    li       r3, 0x1
    li       r4, 0x1
    li       r5, 0x4
    li       r6, 0x21
    li       r7, 0x0
    li       r8, 0x7d
    bl       GXSetTexCoordGen2
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0x1
    li       r6, 0x2
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x0
    li       r4, 0xf
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    li       r3, 0x1
    li       r4, 0x1
    li       r5, 0x1
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x1
    li       r4, 0xf
    li       r5, 0xf
    li       r6, 0x8
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x1
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x1
    li       r4, 0x7
    li       r5, 0x4
    li       r6, 0x0
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x1
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    b        .L_801A7120
.L_801A708C:
    li       r3, 0x0
    bl       GXSetNumChans
    li       r3, 0x1
    bl       GXSetNumTexGens
    li       r3, 0x1
    bl       GXSetNumTevStages
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0x1
    li       r6, 0x2
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x0
    li       r4, 0xf
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
.L_801A7120:
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lfs      f1, sFbEffectZero@sda21(r0)
    lis      r3, 0xcc01
    lfs      f0, sFbEffectOne@sda21(r0)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    b        .L_801A74F0
.L_801A7180:
    addi     r3, r1, 0x90
    bl       PSMTXIdentity
    lhz      r4, 0x0(r30)
    lis      r0, 0x4330
    stw      r0, 0x268(r1)
    addi     r3, r1, 0x60
    lfd      f1, sFbEffectUnsignedConversionBias@sda21(r0)
    stw      r4, 0x26c(r1)
    lfs      f2, 0x40(r30)
    lfd      f0, 0x268(r1)
    fsubs    f0, f0, f1
    fdivs    f0, f2, f0
    stfs     f0, 0x9c(r1)
    bl       PSMTXIdentity
    lfs      f2, 0x40(r30)
    lfs      f0, sFbEffectZero@sda21(r0)
    fcmpo    cr0, f2, f0
    bge      .L_801A71CC
    fneg     f2, f2
.L_801A71CC:
    lhz      r5, 0x2(r30)
    lis      r0, 0x4330
    stw      r0, 0x260(r1)
    addi     r3, r1, 0x90
    lfd      f1, sFbEffectUnsignedConversionBias@sda21(r0)
    li       r4, 0x21
    stw      r5, 0x264(r1)
    li       r5, 0x1
    lfd      f0, 0x260(r1)
    fsubs    f0, f0, f1
    fdivs    f0, f2, f0
    stfs     f0, 0x7c(r1)
    bl       GXLoadTexMtxImm
    addi     r3, r1, 0x60
    li       r4, 0x24
    li       r5, 0x1
    bl       GXLoadTexMtxImm
    lfs      f1, 0x40(r30)
    lfs      f0, sFbEffectZero@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801A7238
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0x5
    li       r6, 0xf
    bl       GXSetBlendMode
    b        .L_801A724C
.L_801A7238:
    li       r3, 0x1
    li       r4, 0x4
    li       r5, 0x5
    li       r6, 0xf
    bl       GXSetBlendMode
.L_801A724C:
    lwz      r0, sOutlineThickFillWhite@sda21(r0)
    addi     r4, r1, 0x1c
    li       r3, 0x1
    stw      r0, 0x1c(r1)
    bl       GXSetTevColor
    lwz      r0, 0x30(r30)
    addi     r4, r1, 0x18
    li       r3, 0x2
    stw      r0, 0x18(r1)
    bl       GXSetTevColor
    li       r3, 0x0
    bl       GXSetNumChans
    li       r3, 0x2
    bl       GXSetNumTexGens
    li       r3, 0x3
    bl       GXSetNumTevStages
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x0
    li       r4, 0x8
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x0
    li       r4, 0xe
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x2
    bl       GXSetTevAlphaOp
    li       r3, 0x1
    li       r4, 0x1
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x1
    li       r4, 0x8
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x1
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x1
    li       r4, 0x4
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x1
    li       r4, 0xe
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    li       r3, 0x2
    li       r4, 0x1
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x2
    li       r4, 0x4
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x2
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x2
    li       r4, 0x2
    li       r5, 0x7
    li       r6, 0x7
    li       r7, 0x0
    bl       GXSetTevAlphaIn
    li       r3, 0x2
    li       r4, 0x1
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    li       r3, 0x1
    li       r4, 0x1
    li       r5, 0x4
    li       r6, 0x21
    li       r7, 0x0
    li       r8, 0x7d
    bl       GXSetTexCoordGen2
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lfs      f1, sFbEffectZero@sda21(r0)
    lis      r9, 0xcc01
    lfs      f0, sFbEffectOne@sda21(r0)
    li       r3, 0x1
    stfs     f1, -0x8000(r9)
    li       r4, 0x1
    li       r5, 0x4
    li       r6, 0x24
    stfs     f1, -0x8000(r9)
    li       r7, 0x0
    li       r8, 0x7d
    stfs     f1, -0x8000(r9)
    stfs     f1, -0x8000(r9)
    stfs     f31, -0x8000(r9)
    stfs     f1, -0x8000(r9)
    stfs     f0, -0x8000(r9)
    stfs     f1, -0x8000(r9)
    stfs     f31, -0x8000(r9)
    stfs     f30, -0x8000(r9)
    stfs     f0, -0x8000(r9)
    stfs     f0, -0x8000(r9)
    stfs     f1, -0x8000(r9)
    stfs     f30, -0x8000(r9)
    stfs     f1, -0x8000(r9)
    stfs     f0, -0x8000(r9)
    bl       GXSetTexCoordGen2
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lfs      f1, sFbEffectZero@sda21(r0)
    lis      r3, 0xcc01
    lfs      f0, sFbEffectOne@sda21(r0)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
.L_801A74F0:
    rlwinm.  r0, r31, 0, 26, 26
    beq      .L_801A761C
    lbz      r0, 0x37(r30)
    cmplwi   r0, 0x0
    beq      .L_801A761C
    li       r3, 0x1
    li       r4, 0x4
    li       r5, 0x5
    li       r6, 0xf
    bl       GXSetBlendMode
    lwz      r0, 0x34(r30)
    addi     r4, r1, 0x14
    li       r3, 0x1
    stw      r0, 0x14(r1)
    bl       GXSetTevColor
    li       r3, 0x0
    bl       GXSetNumChans
    li       r3, 0x1
    bl       GXSetNumTexGens
    li       r3, 0x1
    bl       GXSetNumTevStages
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x0
    li       r4, 0xf
    li       r5, 0x8
    li       r6, 0x2
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x0
    li       r4, 0x1
    li       r5, 0x7
    li       r6, 0x7
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lfs      f1, sFbEffectZero@sda21(r0)
    lis      r3, 0xcc01
    lfs      f0, sFbEffectOne@sda21(r0)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
.L_801A761C:
    rlwinm.  r0, r31, 0, 24, 25
    beq      .L_801A7954
    lbz      r0, 0x3b(r30)
    cmplwi   r0, 0x0
    beq      .L_801A7954
    lfs      f0, 0x5c(r30)
    rlwinm.  r0, r31, 0, 25, 25
    fmuls    f26, f31, f0
    fmuls    f27, f30, f0
    fdivs    f29, f26, f31
    fdivs    f28, f27, f30
    beq      .L_801A77B0
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0x5
    li       r6, 0xf
    bl       GXSetBlendMode
    li       r3, 0x1
    bl       GXSetDither
    li       r3, 0x0
    bl       GXSetNumChans
    li       r3, 0x1
    bl       GXSetNumTexGens
    li       r3, 0x1
    bl       GXSetNumTevStages
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x0
    li       r4, 0xf
    li       r5, 0x8
    li       r6, 0xc
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x0
    li       r4, 0x6
    li       r5, 0x7
    li       r6, 0x7
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    fsubs    f2, f31, f26
    lfs      f1, sFbEffectHalf@sda21(r0)
    fsubs    f0, f30, f27
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    fmuls    f26, f2, f1
    fmuls    f27, f0, f1
    bl       GXBegin
    lis      r5, 0xcc01
    lfs      f1, sFbEffectZero@sda21(r0)
    stfs     f26, -0x8000(r5)
    fsubs    f2, f31, f26
    lfs      f0, sFbEffectOne@sda21(r0)
    fsubs    f3, f30, f27
    stfs     f27, -0x8000(r5)
    li       r3, 0x0
    li       r4, 0x0
    stfs     f1, -0x8000(r5)
    stfs     f1, -0x8000(r5)
    stfs     f2, -0x8000(r5)
    stfs     f27, -0x8000(r5)
    stfs     f0, -0x8000(r5)
    stfs     f1, -0x8000(r5)
    stfs     f2, -0x8000(r5)
    stfs     f3, -0x8000(r5)
    stfs     f0, -0x8000(r5)
    stfs     f0, -0x8000(r5)
    stfs     f26, -0x8000(r5)
    stfs     f3, -0x8000(r5)
    stfs     f1, -0x8000(r5)
    stfs     f0, -0x8000(r5)
    lhz      r5, 0x0(r30)
    lhz      r6, 0x2(r30)
    bl       GXSetTexCopySrc
    lhz      r3, 0x0(r30)
    li       r5, 0x1
    lhz      r4, 0x2(r30)
    li       r6, 0x0
    bl       GXSetTexCopyDst
    lwz      r3, 0x4(r30)
    li       r4, 0x0
    bl       GXCopyTex
    bl       GXPixModeSync
    bl       GXInvalidateTexAll
.L_801A77B0:
    rlwinm.  r0, r31, 0, 24, 24
    beq      .L_801A7954
    li       r3, 0x1
    li       r4, 0x4
    li       r5, 0x5
    li       r6, 0xf
    bl       GXSetBlendMode
    li       r3, 0x1
    bl       GXSetDither
    lwz      r3, 0x38(r30)
    lis      r0, 0x4330
    stw      r0, 0x268(r1)
    lfd      f3, sFbEffectUnsignedConversionBias@sda21(r0)
    stw      r3, 0x3c(r1)
    lfs      f1, sFbEffectAlphaScale@sda21(r0)
    lbz      r0, 0x3f(r1)
    lfs      f4, sFbEffectPi@sda21(r0)
    stw      r0, 0x26c(r1)
    lfs      f0, sFbEffectHalf@sda21(r0)
    lfd      f2, 0x268(r1)
    fsubs    f2, f2, f3
    fdivs    f1, f2, f1
    fmuls    f1, f4, f1
    fmuls    f1, f1, f0
    bl       sin
    frsp     f1, f1
    lfs      f0, sFbEffectAlphaScale@sda21(r0)
    addi     r4, r1, 0x10
    li       r3, 0x1
    fmuls    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x260(r1)
    lwz      r0, 0x264(r1)
    stb      r0, 0x3f(r1)
    lwz      r0, 0x3c(r1)
    stw      r0, 0x10(r1)
    bl       GXSetTevColor
    li       r3, 0x0
    bl       GXSetNumChans
    li       r3, 0x1
    bl       GXSetNumTexGens
    li       r3, 0x1
    bl       GXSetNumTevStages
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x0
    li       r4, 0xf
    li       r5, 0x8
    li       r6, 0x2
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x0
    li       r4, 0x1
    li       r5, 0x7
    li       r6, 0x7
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    lfs      f0, sFbEffectOne@sda21(r0)
    li       r3, 0x80
    lfs      f1, sFbEffectHalf@sda21(r0)
    li       r4, 0x0
    fsubs    f2, f0, f29
    li       r5, 0x4
    fsubs    f0, f0, f28
    fmuls    f26, f2, f1
    fmuls    f27, f0, f1
    bl       GXBegin
    lfs      f1, sFbEffectZero@sda21(r0)
    lis      r3, 0xcc01
    lfs      f0, sFbEffectOne@sda21(r0)
    stfs     f1, -0x8000(r3)
    fsubs    f2, f0, f26
    stfs     f1, -0x8000(r3)
    fsubs    f0, f0, f27
    stfs     f26, -0x8000(r3)
    stfs     f27, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f27, -0x8000(r3)
    stfs     f31, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f30, -0x8000(r3)
    stfs     f26, -0x8000(r3)
    stfs     f0, -0x8000(r3)
.L_801A7954:
    rlwinm.  r0, r31, 0, 23, 23
    beq      .L_801A7BF0
    li       r3, 0x1
    li       r4, 0x2
    li       r5, 0x4
    li       r6, 0xf
    bl       GXSetBlendMode
    lwz      r0, sOutlineOffsetColor0@sda21(r0)
    addi     r4, r1, 0xc
    li       r3, 0x1
    stw      r0, 0xc(r1)
    bl       GXSetTevColor
    lwz      r0, sOutlineOffsetColor1@sda21(r0)
    addi     r4, r1, 0x8
    li       r3, 0x2
    stw      r0, 0x8(r1)
    bl       GXSetTevColor
    li       r3, 0x0
    bl       GXSetNumChans
    li       r3, 0x1
    bl       GXSetNumTexGens
    li       r3, 0x1
    bl       GXSetNumTevStages
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0xff
    bl       GXSetTevOrder
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0xf
    li       r6, 0xf
    li       r7, 0xf
    bl       GXSetTevColorIn
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x1
    li       r8, 0x0
    bl       GXSetTevColorOp
    li       r3, 0x0
    li       r4, 0x4
    li       r5, 0x1
    li       r6, 0x2
    li       r7, 0x7
    bl       GXSetTevAlphaIn
    li       r3, 0x0
    li       r4, 0xf
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXSetTevAlphaOp
    lbz      r0, sOutlineOrbitAngleInitialized@sda21(r0)
    extsb.   r0, r0
    bne      .L_801A7A48
    lfs      f0, sFbEffectZero@sda21(r0)
    li       r0, 0x1
    stb      r0, sOutlineOrbitAngleInitialized@sda21(r0)
    stfs     f0, sOutlineOrbitAngle@sda21(r0)
.L_801A7A48:
    lis      r3, sOutlineInitialOffsets@ha
    addi     r31, r1, 0x40
    addi     r10, r3, sOutlineInitialOffsets@l
    lfs      f27, sFbEffectQuarterTurn@sda21(r0)
    lwz      r9, 0x0(r10)
    mr       r29, r31
    lwz      r8, 0x4(r10)
    li       r28, 0x0
    lwz      r7, 0x8(r10)
    lis      r30, 0x4330
    lwz      r6, 0xc(r10)
    lwz      r5, 0x10(r10)
    lwz      r4, 0x14(r10)
    lwz      r3, 0x18(r10)
    lwz      r0, 0x1c(r10)
    stw      r9, 0x40(r1)
    lfd      f29, sFbEffectSignedConversionBias@sda21(r0)
    stw      r8, 0x44(r1)
    lfs      f28, sFbEffectOffsetRadius@sda21(r0)
    stw      r7, 0x48(r1)
    stw      r6, 0x4c(r1)
    stw      r5, 0x50(r1)
    stw      r4, 0x54(r1)
    stw      r3, 0x58(r1)
    stw      r0, 0x5c(r1)
.L_801A7AAC:
    xoris    r0, r28, 0x8000
    stw      r30, 0x268(r1)
    lfs      f1, sOutlineOrbitAngle@sda21(r0)
    stw      r0, 0x26c(r1)
    lfd      f0, 0x268(r1)
    fsubs    f0, f0, f29
    fmuls    f26, f27, f0
    fadds    f1, f1, f26
    bl       sin
    frsp     f2, f1
    lfs      f0, sOutlineOrbitAngle@sda21(r0)
    fadds    f1, f0, f26
    fmuls    f0, f28, f2
    stfs     f0, 0x0(r29)
    bl       cos
    frsp     f0, f1
    addi     r28, r28, 0x1
    cmpwi    r28, 0x4
    fmuls    f0, f28, f0
    stfs     f0, 0x4(r29)
    addi     r29, r29, 0x8
    blt      .L_801A7AAC
    lfs      f1, sOutlineOrbitAngle@sda21(r0)
    li       r3, 0x80
    lfs      f0, sFbEffectRotationStep@sda21(r0)
    li       r4, 0x0
    li       r5, 0x10
    fadds    f0, f1, f0
    stfs     f0, sOutlineOrbitAngle@sda21(r0)
    bl       GXBegin
    li       r0, 0x2
    lfs      f5, sFbEffectZero@sda21(r0)
    lfs      f2, sFbEffectOne@sda21(r0)
    lis      r3, 0xcc01
    mtctr    r0
.L_801A7B38:
    lfs      f7, 0x0(r31)
    lfs      f4, 0x0(r31)
    stfs     f7, -0x8000(r3)
    lfs      f6, 0x4(r31)
    fadds    f3, f31, f4
    lfs      f1, 0x4(r31)
    stfs     f6, -0x8000(r3)
    fadds    f0, f30, f1
    lfs      f4, 0x8(r31)
    stfs     f5, -0x8000(r3)
    lfs      f1, 0xc(r31)
    stfs     f5, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    stfs     f6, -0x8000(r3)
    lfs      f6, 0xc(r31)
    stfs     f2, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    fadds    f3, f31, f4
    stfs     f0, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f7, -0x8000(r3)
    lfs      f7, 0x8(r31)
    addi     r31, r31, 0x10
    stfs     f0, -0x8000(r3)
    fadds    f0, f30, f1
    stfs     f5, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f7, -0x8000(r3)
    stfs     f6, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    stfs     f6, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f7, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f5, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    bdnz     .L_801A7B38
.L_801A7BF0:
    psq_l    f31, 0x2d8(r1), 0, qr0
    lfd      f31, 0x2d0(r1)
    psq_l    f30, 0x2c8(r1), 0, qr0
    lfd      f30, 0x2c0(r1)
    psq_l    f29, 0x2b8(r1), 0, qr0
    lfd      f29, 0x2b0(r1)
    psq_l    f28, 0x2a8(r1), 0, qr0
    lfd      f28, 0x2a0(r1)
    psq_l    f27, 0x298(r1), 0, qr0
    lfd      f27, 0x290(r1)
    psq_l    f26, 0x288(r1), 0, qr0
    lfd      f26, 0x280(r1)
    lwz      r31, 0x27c(r1)
    lwz      r30, 0x278(r1)
    lwz      r29, 0x274(r1)
    lwz      r0, 0x2e4(r1)
    lwz      r28, 0x270(r1)
    mtlr     r0
    addi     r1, r1, 0x2e0
    blr
*/

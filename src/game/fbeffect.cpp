#include "game/fbeffect.h"
#include "game/mm.h"
#include "Dolphin/mtx.h"
#include <math.h>
#include <stdlib.h>

// a quad over the whole w by h screen with the whole texture on it
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
	// ease mAlpha toward mTarget by mSpeed per frame
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
		mValid = 1; // the fade can use the buffer now
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
	Mtx mtx;
	Mtx44 proj;
	Mtx texMtx;

	if (flags & (FBOUT_COPY_A8 | FBOUT_COPY_I8)) {
		GXTexFmt format = (flags & FBOUT_COPY_A8) ? GX_CTF_A8 : GX_TF_I8;
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
	if (flags & FBOUT_OUTLINE) {
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
	if (flags & FBOUT_TINT) {
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

	if (flags & FBOUT_OUTLINE2) {
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
			f32 absThick = mThick2;
			if (absThick < 0.0f) {
				absThick = -absThick;
			}
			offsetY[1][3] = absThick / mHeight;
			GXLoadTexMtxImm(offsetX, GX_TEXMTX1, GX_MTX2x4);
			GXLoadTexMtxImm(offsetY, GX_TEXMTX2, GX_MTX2x4);
			if (mThick2 > 0.0f) {
				GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			} else {
				// a negative thickness fades instead of covering
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

	if ((flags & FBOUT_OVERLAY) && mColor3.a != 0) {
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

	if ((flags & (FBOUT_ZOOM_COPY | FBOUT_ZOOM_FADE)) && mColor4.a != 0) {
		f32 zoom  = mZoom;
		f32 zoomW = w * zoom;
		f32 zoomH = h * zoom;
		f32 zoomU = zoomW / w;
		f32 zoomV = zoomH / h;

		// draw the screen shrunk into the middle and grab it back
		if (flags & FBOUT_ZOOM_COPY) {
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
			f32 zero    = 0.0f;
			f32 one     = 1.0f;
			f32 marginX = (w - zoomW) / 2;
			f32 marginY = (h - zoomH) / 2;
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			GXPosition2f32(marginX, marginY);
			GXTexCoord2f32(zero, zero);
			GXPosition2f32(w - marginX, marginY);
			GXTexCoord2f32(one, zero);
			GXPosition2f32(w - marginX, h - marginY);
			GXTexCoord2f32(one, one);
			GXPosition2f32(marginX, h - marginY);
			GXTexCoord2f32(zero, one);
			GXEnd();
			Copy(GX_TF_I8);
		}

		// fade the zoomed copy over the screen
		if (flags & FBOUT_ZOOM_FADE) {
			GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetDither(GX_TRUE);
			GXColor color = mColor4;
			// the fade alpha follows a sine ease
			color.a = 255.0f * (f32)sin(PI * (color.a / 255.0f) / 2);
			GXSetTevColor(GX_TEVREG0, color);
			GXSetNumChans(0);
			GXSetNumTexGens(1);
			GXSetNumTevStages(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			f32 zero    = 0.0f;
			f32 one     = 1.0f;
			f32 marginU = (1.0f - zoomU) / 2;
			f32 marginV = (1.0f - zoomV) / 2;
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			GXPosition2f32(zero, zero);
			GXTexCoord2f32(marginU, marginV);
			GXPosition2f32(w, zero);
			GXTexCoord2f32(one - marginU, marginV);
			GXPosition2f32(w, h);
			GXTexCoord2f32(one - marginU, one - marginV);
			GXPosition2f32(zero, h);
			GXTexCoord2f32(marginU, one - marginV);
			GXEnd();
		}
	}

	// ghosts of the outline circling around it
	if (flags & FBOUT_GHOSTS) {
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

		// the ghosts' phase, advanced 2 degrees a draw
		static f32 rot = 0.0f;
		f32 offset[4][2];
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

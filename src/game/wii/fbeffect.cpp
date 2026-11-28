#include "game/wii/fbeffect.h"
#include "game/mm.h"
#include "Dolphin/mtx.h"
#include <math.h>
#include <stdlib.h>

// a quad over the whole rectangle with the whole texture on it. Retail has the vertices at
// FbEffect::Draw's own level: put in an inline function, member or not, the third vertex takes its
// coordinates through the other two registers
#define DRAW_QUAD()                   \
	GXBegin(GX_QUADS, GX_VTXFMT0, 4); \
	GXPosition2f32(0.0f, 0.0f);       \
	GXTexCoord2f32(0.0f, 0.0f);       \
	GXPosition2f32(mW, 0.0f);         \
	GXTexCoord2f32(1.0f, 0.0f);       \
	GXPosition2f32(mW, mH);           \
	GXTexCoord2f32(1.0f, 1.0f);       \
	GXPosition2f32(0.0f, mH);         \
	GXTexCoord2f32(0.0f, 1.0f);       \
	GXEnd()

/**
 * @note Address: 0x802BF9F8
 * @note Size: 0x18
 */
void FbBase::Init(u16 w, u16 h, void* buf, f32 aspect)
{
	mWidth  = w;
	mHeight = h;
	SetAspect(aspect);
}

/**
 * @note Address: 0x802BFA10
 * @note Size: 0xA0
 */
void FbBase::SetAspect(f32 aspect)
{
	mAspect = aspect;
	mX      = 0.0f;
	mY      = 0.0f;
	mW      = mWidth * mAspect;
	mH      = mHeight;
	C_MTXOrtho(mProj, 0.0f, mH, 0.0f, mW, 0.0f, 1.0f);
	CalcMtx();
}

/**
 * @note Address: 0x802BFAB0
 * @note Size: 0xE8
 */
void FbBlur::Init(u16 w, u16 h, void* buf, f32 aspect)
{
	mOffsetX = mOffsetY = 0.0f;
	// the buffer is half the screen each way
	FbBase::Init(w / 2, h / 2, NULL, aspect);
	u32 size = GXGetTexBufferSize(mWidth, mHeight, GX_TF_RGB565, GX_FALSE, 0);
	buf      = buf != NULL ? buf : mm::Alloc(size, "(blur)");
	mTex.Init(buf, mWidth, mHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&mTex, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
	mValid = 0;
	mAlpha = mTarget = mSpeed = 0.0f;
}

/**
 * @note Address: 0x802BFB98
 * @note Size: 0x80
 */
void FbBlur::CalcMtx()
{
	Mtx trans;
	Mtx scale;
	PSMTXTrans(trans, mX, mY, 0.0f);
	PSMTXScale(scale, mW, mH, 1.0f);
	PSMTXConcat(trans, scale, mMtx);
}

/**
 * @note Address: 0x802BFC18
 * @note Size: 0xC4
 */
void FbBlur::Update(u32 frames)
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
 * @note Address: 0x802BFCDC
 * @note Size: 0x29C
 */
void FbBlur::Draw()
{
	if (mAlpha != 0.0f) {
		if (mValid) {
			// last frame faded over this one
			mTex.LoadTexture(GX_TEXMAP0);
			GXSetProjection(mProj, GX_ORTHOGRAPHIC);
			GXLoadPosMtxImm(mMtx, GX_PNMTX0);
			GXSetCurrentMtx(GX_PNMTX0);
			GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
			GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
			GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 255);
			GXSetColorUpdate(GX_TRUE);
			GXSetAlphaUpdate(GX_FALSE);
			GXSetDstAlpha(GX_FALSE, 0);
			GXSetDither(GX_TRUE);
			GXSetNumChans(0);
			GXSetNumTexGens(1);
			GXSetNumTevStages(1);
			GXSetTevDirect(GX_TEVSTAGE0);

			// NONMATCHING: retail keeps this colour at 0x8 and the copy it hands over at 0xC, the
			// other way round here; the code is otherwise the same
			GXColor color = { 0, 0, 0, 0 };
			color.a       = 255.0f * mAlpha;
			GXSetTevKColor(GX_KCOLOR0, color);
			GXSetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_K0_A);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
			GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U8, 0);
			GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 0);
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			GXPosition2u8(0, 0);
			GXTexCoord2u8(0, 0);
			GXPosition2u8(1, 0);
			GXTexCoord2u8(1, 0);
			GXPosition2u8(1, 1);
			GXTexCoord2u8(1, 1);
			GXPosition2u8(0, 1);
			GXTexCoord2u8(0, 1);
			GXEnd();
		}
		Copy();
	}
}

/**
 * @note Address: 0x802BFF78
 * @note Size: 0x78
 */
void FbBlur::Copy()
{
	// the efb at twice the buffer's size, box filtered down
	GXSetTexCopySrc(0, 0, mWidth * 2, mHeight * 2);
	GXSetTexCopyDst(mWidth, mHeight, GX_TF_RGB565, GX_TRUE);
	GXCopyTex(mTex.mImageData, GX_FALSE);
	if (!mValid) {
		mValid = 1; // the fade can use the buffer now
	}
}

// grabs the efb into the texture
inline void FbEffect::Copy(GXTexFmt format)
{
	GXSetTexCopySrc(0, 0, mWidth, mHeight);
	GXSetTexCopyDst(mWidth, mHeight, format, GX_FALSE);
	GXCopyTex(mTex.mImageData, GX_FALSE);
	GXPixModeSync();
	GXInvalidateTexAll();
}

/**
 * @note Address: 0x802BFFF0
 * @note Size: 0x17C
 */
void FbEffect::Init(u16 w, u16 h, void* buf, f32 aspect)
{
	FbBase::Init(w, h, NULL, aspect);
	u32 size = GXGetTexBufferSize(mWidth, mHeight, GX_TF_I8, GX_FALSE, 0);
	// NONMATCHING: "(effect)" sits 0x4A into retail's string pool, behind "(haze)", "(mosaic)",
	// "(dof )", "(dofz)", "fbeffect.cpp" and "unknown FBDOF_TYPE - %d": the names of three more
	// effects whose functions the linker dropped. Without their code the offset here is 7
	buf = buf != NULL ? buf : mm::Alloc(size, "(effect)");
	mTex.Init(buf, mWidth, mHeight, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&mTex, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);

	GXColor line = { 0x22, 0x09, 0x19, 0x04 };
	mColor0      = line;
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

/**
 * @note Address: 0x802C016C
 * @note Size: 0x8
 */
void FbEffect::CalcMtx()
{
	PSMTXIdentity(mMtx);
}

/**
 * @note Address: 0x802C0174
 * @note Size: 0x1FC
 */
void FbEffect::Update(u32 frames)
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
 * @note Address: 0x802C0370
 * @note Size: 0x1640
 */
void FbEffect::Draw(u32 flags)
{
	Mtx texMtx;

	if (flags & (FBEFF_COPY_A8 | FBEFF_COPY_I8)) {
		GXTexFmt format = (flags & FBEFF_COPY_A8) ? GX_CTF_A8 : GX_TF_I8;
		Copy(format);
	}

	mTex.LoadTexture(GX_TEXMAP0);
	GXLoadPosMtxImm(mMtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	GXSetProjection(mProj, GX_ORTHOGRAPHIC);
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
	if (flags & FBEFF_OUTLINE) {
		if (mThick == 0.0f) {
			static const GXColor white = { 0xFF, 0xFF, 0xFF, 0x04 };
			GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
			GXSetTevColor(GX_TEVREG0, white);
			GXSetTevColor(GX_TEVREG1, mColor0);
			GXSetNumChans(0);
			GXSetNumTexGens(1);
			GXSetNumTevStages(1);
			GXSetTevDirect(GX_TEVSTAGE0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_A1, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_GT, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			DRAW_QUAD();
		} else {
			Mtx offsetX;
			Mtx offsetY;
			PSMTXIdentity(offsetX);
			offsetX[0][3] = mThick / mW;
			PSMTXIdentity(offsetY);
			offsetY[1][3] = mThick / mH;
			GXLoadTexMtxImm(offsetX, GX_TEXMTX1, GX_MTX2x4);
			GXLoadTexMtxImm(offsetY, GX_TEXMTX2, GX_MTX2x4);
			GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetTevColor(GX_TEVREG0, mColor0);
			GXSetNumChans(0);
			GXSetNumTexGens(2);
			GXSetNumTevStages(3);
			GXSetTevDirect(GX_TEVSTAGE0);
			GXSetTevDirect(GX_TEVSTAGE1);
			GXSetTevDirect(GX_TEVSTAGE2);
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
			DRAW_QUAD();
			GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX2, GX_FALSE, GX_PTIDENTITY);
			DRAW_QUAD();
		}
	}

	if (flags & FBEFF_OUTLINE2) {
		if (mThick2 == 0.0f) {
			static const GXColor white = { 0xFF, 0xFF, 0xFF, 0x00 };
			GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetTevColor(GX_TEVREG0, white);
			mColor2.a = 0x40;
			GXSetTevColor(GX_TEVREG1, mColor2);
			if (mPatternTex != NULL) {
				Mtx pattern;
				Mtx trans;
				Mtx rot;
				GXSetNumChans(0);
				GXSetNumTexGens(2);
				GXSetNumTevStages(2);
				GXSetTevDirect(GX_TEVSTAGE0);
				GXSetTevDirect(GX_TEVSTAGE1);
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
				GXSetTevDirect(GX_TEVSTAGE0);
				GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
				GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
				GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
				GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_A1, GX_CA_ZERO);
				GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_EQ, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			}
			DRAW_QUAD();
		} else {
			Mtx offsetX;
			Mtx offsetY;
			PSMTXIdentity(offsetX);
			offsetX[0][3] = mThick2 / mW;
			PSMTXIdentity(offsetY);
			// the float fabs is nw4r::math::FAbs's; its header needs the Wii MSL's <cmath>, which the
			// game's include path does not have yet
			offsetY[1][3] = __fabsf(mThick2) / mH;
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
			GXSetTevDirect(GX_TEVSTAGE0);
			GXSetTevDirect(GX_TEVSTAGE1);
			GXSetTevDirect(GX_TEVSTAGE2);
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
			DRAW_QUAD();
			GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX2, GX_FALSE, GX_PTIDENTITY);
			DRAW_QUAD();
		}
	}

	if ((flags & FBEFF_OVERLAY) && mColor3.a != 0) {
		GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		GXSetTevColor(GX_TEVREG0, mColor3);
		GXSetNumChans(0);
		GXSetNumTexGens(1);
		GXSetNumTevStages(1);
		GXSetTevDirect(GX_TEVSTAGE0);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
		GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_ZERO);
		GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
		GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
		DRAW_QUAD();
	}

	if ((flags & (FBEFF_ZOOM_COPY | FBEFF_ZOOM_FADE)) && mColor4.a != 0) {
		f32 zoomW = mW * mZoom;
		f32 zoomH = mH * mZoom;
		f32 zoomU = zoomW / mW;
		f32 zoomV = zoomH / mH;

		// draw the screen shrunk into the middle and grab it back
		if (flags & FBEFF_ZOOM_COPY) {
			GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetDither(GX_TRUE);
			GXSetNumChans(0);
			GXSetNumTexGens(1);
			GXSetNumTevStages(1);
			GXSetTevDirect(GX_TEVSTAGE0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ONE, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			f32 marginX = (mW - zoomW) / 2;
			f32 marginY = (mH - zoomH) / 2;
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			GXPosition2f32(marginX, marginY);
			GXTexCoord2f32(0.0f, 0.0f);
			GXPosition2f32(mW - marginX, marginY);
			GXTexCoord2f32(1.0f, 0.0f);
			GXPosition2f32(mW - marginX, mH - marginY);
			GXTexCoord2f32(1.0f, 1.0f);
			GXPosition2f32(marginX, mH - marginY);
			GXTexCoord2f32(0.0f, 1.0f);
			GXEnd();
			Copy(GX_TF_I8);
		}

		// fade the zoomed copy over the screen
		if (flags & FBEFF_ZOOM_FADE) {
			GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			GXSetDither(GX_TRUE);
			GXColor color = mColor4;
			// the fade alpha follows a sine ease
			color.a = 255.0f * (f32)sin(PI * (color.a / 255.0f) / 2);
			GXSetTevColor(GX_TEVREG0, color);
			GXSetNumChans(0);
			GXSetNumTexGens(1);
			GXSetNumTevStages(1);
			GXSetTevDirect(GX_TEVSTAGE0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_ZERO);
			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			f32 marginU = (1.0f - zoomU) / 2;
			f32 marginV = (1.0f - zoomV) / 2;
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			GXPosition2f32(0.0f, 0.0f);
			GXTexCoord2f32(marginU, marginV);
			GXPosition2f32(mW, 0.0f);
			GXTexCoord2f32(1.0f - marginU, marginV);
			GXPosition2f32(mW, mH);
			GXTexCoord2f32(1.0f - marginU, 1.0f - marginV);
			GXPosition2f32(0.0f, mH);
			GXTexCoord2f32(marginU, 1.0f - marginV);
			GXEnd();
		}
	}

	// ghosts of the outline circling around it
	if (flags & FBEFF_GHOSTS) {
		static const GXColor color0 = { 0xFF, 0xFF, 0xFF, 0x08 };
		static const GXColor color1 = { 0xFF, 0xFF, 0xFF, 0x40 };
		GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
		GXSetTevColor(GX_TEVREG0, color0);
		GXSetTevColor(GX_TEVREG1, color1);
		GXSetNumChans(0);
		GXSetNumTexGens(1);
		GXSetNumTevStages(1);
		GXSetTevDirect(GX_TEVSTAGE0);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
		GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
		GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_A1, GX_CA_ZERO);
		GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_EQ, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);

		// the ghosts' phase, advanced 2 degrees a draw
		static f32 rot = 0.0f;
		f32 radius     = 3.0f;
		// placeholders, the loop fills them every draw
		f32 offset[4][2] = {
			{ -radius, -radius },
			{ radius, -radius },
			{ radius, radius },
			{ -radius, radius },
		};
		int i;
		for (i = 0; i < 4; i++) {
			offset[i][0] = radius * (f32)sin(rot + i * (PI / 2));
			offset[i][1] = radius * (f32)cos(rot + i * (PI / 2));
		}
		rot += MTXDegToRad(2.0f);

		// NONMATCHING: retail loads each offset twice, once for the plain coordinates and once for
		// the sums with mW and mH (as the GameCube's does); the registers follow from that
		GXBegin(GX_QUADS, GX_VTXFMT0, 16);
		for (i = 0; i < 4; i++) {
			GXPosition2f32(offset[i][0], offset[i][1]);
			GXTexCoord2f32(0.0f, 0.0f);
			GXPosition2f32(mW + offset[i][0], offset[i][1]);
			GXTexCoord2f32(1.0f, 0.0f);
			GXPosition2f32(mW + offset[i][0], mH + offset[i][1]);
			GXTexCoord2f32(1.0f, 1.0f);
			GXPosition2f32(offset[i][0], mH + offset[i][1]);
			GXTexCoord2f32(0.0f, 1.0f);
		}
		GXEnd();
	}
}

#include "game/fbtexeff.h"
#include "game/ctpl.h"
#include <math.h>

extern "C" void PSMTXTrans(Mtx, f32, f32, f32);
extern "C" void PSMTXScale(Mtx, f32, f32, f32);
extern "C" s32 ClassifyFloatBits(f32 value);
extern s32 gScreenSafeLeft;
extern s32 gScreenSafeTop;

static inline void FlushCaptureGraphics(CGs* graphics)
{
	if (graphics->mCurRenderState)
		graphics->mCurRenderState->Apply();
	if (gGame.mGs.mCurTevStage)
		gGame.mGs.mCurTevStage->Apply();
	if (gGame.mGs.mCurChannel)
		gGame.mGs.mCurChannel->Apply();
}

static inline CTexObj WindowFrameTexture()
{
	CTpl& tpl = gGame.mTpl;
	tpl.mTex.Init(&tpl.mPalette->descriptorArray[159], 0);
	return tpl.mTex;
}

static inline void DrawQuad(CTexObj& tex, u32 color, f32 x, f32 y, f32 w, f32 h)
{
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0, w, h, 0, 0, 0);
}

static inline void DrawCorner(u32 color, f32 x, f32 y, f32 z, f32 w, f32 h)
{
	DrawGsCircle(&gGame.mGs, 16, color, color, x, y, z, w, h, 0, 0, 0);
}

void FbSubwin::Copy()
{
	CGs* graphics = &gGame.mGs;
	graphics->mCurRenderState->SetColorUpdate(GX_TRUE);
	graphics->mCurRenderState->SetAlphaUpdate(GX_TRUE);
	graphics->mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_TRUE);
	FlushCaptureGraphics(graphics);
	WiiFbTexture* texture = (WiiFbTexture*)this;
	void* image           = texture->mImage;
	GXTexFmt format       = texture->mFormat;
	if (image) {
		u16 width, height;
		if (texture->mMipmap) {
			width  = texture->mWidth * 2;
			height = texture->mHeight * 2;
		} else {
			width  = texture->mWidth;
			height = texture->mHeight;
		}
		GXSetTexCopySrc(0, 0, width, height);
		GXSetTexCopyDst(texture->mWidth, texture->mHeight, format, texture->mMipmap);
		GXCopyTex(texture->mImage, 1);
		GXPixModeSync();
	}
}

// NONMATCHING: drawn against the retail asm and the GameCube source; still differs.
// NaN dimensions select the circular frame. Explicit dimensions select the tiled frame.
void FbSubwin::Draw()
{
	if (mFlags & 4)
		return;
	if (mAmount == 0.0f)
		return;

	SetGsChannelEnabled(&gGame.mGs, 4, 0);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 255);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
	gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
	FlushCaptureGraphics(&gGame.mGs);

	f32 amount            = mAmount;
	f32 fade              = sin(0.5f * (3.1415927f * amount));
	WiiFbTexture* texture = (WiiFbTexture*)this;
	if (ClassifyFloatBits(mRectWidth) == FP_NAN || ClassifyFloatBits(mRectHeight) == FP_NAN) {
		CTexObj frame = WindowFrameTexture();
		u16 tw        = GXGetTexObjWidth(&frame);
		u16 th        = GXGetTexObjHeight(&frame);
		f32 w         = fade * tw;
		f32 h         = fade * th;
		f32 aspect    = (f32)mWidth / (f32)mHeight;
		f32 x         = (gScreenSafeLeft + 4) + (1.1f * tw + 8) + 0.5f * (1.1f * tw);
		f32 y         = (gScreenSafeTop + 13) + 0.5f * (1.1f * th);
		DrawCorner(0xff000000, x, y, 0, w, h);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
		if (texture->mImage)
			DrawQuad(texture->mTex, 0xffffff00 | (u32)(255 * amount), x, y, w * aspect, h);
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, (GXCompare)3, GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
		DrawQuad(frame, 0xc8c8c800 | (u32)(255 * amount), x, y, 1.1f * w, 1.1f * h);
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
	} else {
		f32 w = mRectWidth * fade, h = mRectHeight * fade, corner = 24 * fade;
		f32 x = mRectX, y = mRectY;
		Vec pos[10] = { (Vec) { x, y, 0 },
			            (Vec) { (fade + (x - w * .5f)), corner + (fade + (y - h * .5f)), 0 },
			            (Vec) { corner + (fade + (x - w * .5f)), (fade + (y - h * .5f)), 0 },
			            (Vec) { ((x + w * .5f) - fade) - corner, (fade + (y - h * .5f)), 0 },
			            (Vec) { ((x + w * .5f) - fade), corner + (fade + (y - h * .5f)), 0 },
			            (Vec) { ((x + w * .5f) - fade), ((y + h * .5f) - fade) - corner, 0 },
			            (Vec) { ((x + w * .5f) - fade) - corner, ((y + h * .5f) - fade), 0 },
			            (Vec) { corner + (fade + (x - w * .5f)), ((y + h * .5f) - fade), 0 },
			            (Vec) { (fade + (x - w * .5f)), ((y + h * .5f) - fade) - corner, 0 },
			            (Vec) { (fade + (x - w * .5f)), corner + (fade + (y - h * .5f)), 0 } };
		DrawGsTrianglePrimitive(&gGame.mGs, (GXPrimitive)0xa0, gGame.mGs.mCamera->mViewMtx, pos, 0xff000000, 10);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_TRUE);
		f32 diameter = 2 * corner;
		DrawCorner(0xff000000, corner + (fade + (x - w * .5f)), corner + (fade + (y - h * .5f)), 0, diameter, diameter);
		DrawCorner(0xff000000, ((x + w * .5f) - fade) - corner, corner + (fade + (y - h * .5f)), 0, diameter, diameter);
		DrawCorner(0xff000000, corner + (fade + (x - w * .5f)), ((y + h * .5f) - fade) - corner, 0, diameter, diameter);
		DrawCorner(0xff000000, ((x + w * .5f) - fade) - corner, ((y + h * .5f) - fade) - corner, 0, diameter, diameter);
		u32 shadow = (u32)(96 * amount);
		for (int i = 0; i < 10; ++i) {
			pos[i].x += 6;
			pos[i].y += 4;
			pos[i].z = -.1f;
		}
		DrawGsTrianglePrimitive(&gGame.mGs, (GXPrimitive)0xa0, gGame.mGs.mCamera->mViewMtx, pos, shadow, 10);
		DrawCorner(shadow, (corner + (fade + ((6 + x) - w * .5f))), (corner + (fade + ((4 + y) - h * .5f))), -.1f, diameter, diameter);
		DrawCorner(shadow, (((((6 + x) + w * .5f) - fade) - corner)), (corner + (fade + ((4 + y) - h * .5f))), -.1f, diameter, diameter);
		DrawCorner(shadow, (corner + (fade + ((6 + x) - w * .5f))), (((((4 + y) + h * .5f) - fade) - corner)), -.1f, diameter, diameter);
		DrawCorner(shadow, (((((6 + x) + w * .5f) - fade) - corner)), (((((4 + y) + h * .5f) - fade) - corner)), -.1f, diameter, diameter);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
		if (texture->mImage)
			DrawQuad(texture->mTex, 0xffffff00 | (u32)(255 * amount), x, y, w, h);
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, (GXCompare)3, GX_FALSE);
		CTexObj frame   = WindowFrameTexture();
		u16 tw          = GXGetTexObjWidth(&frame);
		u16 th          = GXGetTexObjHeight(&frame);
		f32 tiles[8][8] = { { 0, 0, .5f, .5f, ((x - w * .5f) + corner * .5f), ((y - h * .5f) + corner * .5f), corner, corner },
			                { .5f, 0, 1.0f / tw, .5f, x, ((y - h * .5f) + corner * .5f), w - diameter, corner },
			                { .5f, 0, .5f, .5f, ((x + w * .5f) - corner * .5f), ((y - h * .5f) + corner * .5f), corner, corner },
			                { 0, .5f, .5f, 1.0f / th, ((x - w * .5f) + corner * .5f), y, corner, h - diameter },
			                { .5f, .5f, .5f, 1.0f / th, ((x + w * .5f) - corner * .5f), y, corner, h - diameter },
			                { 0, .5f, .5f, .5f, ((x - w * .5f) + corner * .5f), ((y + h * .5f) - corner * .5f), corner, corner },
			                { .5f, .5f, 1.0f / tw, .5f, x, ((y + h * .5f) - corner * .5f), w - diameter, corner },
			                { .5f, .5f, .5f, .5f, ((x + w * .5f) - corner * .5f), ((y + h * .5f) - corner * .5f), corner, corner } };
		u32 color       = 0xffffff00 | (u32)(255 * amount);
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
		for (int i = 0; i < 8; ++i) {
			PSMTXTrans(frame.mTexMtx.mMtx[0], tiles[i][0], tiles[i][1], 0);
			frame.mTexMtx.mDirty = TRUE;
			PSMTXScale(frame.mTexMtx.mMtx[2], tiles[i][2], tiles[i][3], 1);
			frame.mTexMtx.mDirty = TRUE;
			DrawQuad(frame, color, tiles[i][4], tiles[i][5], tiles[i][6], tiles[i][7]);
		}
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
	}
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, (GXCompare)3, 255);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, (GXCompare)3, GX_FALSE);
}

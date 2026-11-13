#include "game/cgameutil.h"
#include <math.h>

extern "C" void PSMTXTrans(Mtx, f32, f32, f32);
extern "C" void PSMTXScale(Mtx, f32, f32, f32);

static inline CTexObj& WindowFrameTexture()
{
	CTpl& tpl = gGame.mTpl;
	return tpl.mTex.Init(&((TPLPalette*)tpl.mBuffer)->descriptorArray[158 - StatusTextureOffset], 0);
}

static inline void DrawQuad(CTexObj& tex, u32 color, f32 x, f32 y, f32 w, f32 h)
{
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0, w, h, 0, 0, 0);
}

static inline void DrawCorner(u32 color, f32 x, f32 y, f32 z, f32 w, f32 h)
{
	DrawGsCircle(&gGame.mGs, 16, color, color, x, y, z, w, h, 0, 0, 0);
}

// NaN dimensions select the circular frame. Explicit dimensions select the tiled frame.
void FbSubwin::Draw()
{
	if (mFlags.mFlags & 4)
		return;
	if (mAmount == 0.0f)
		return;

	SetGsChannelEnabled(&gGame.mGs, 4, 0);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsAlphaCompare(gGame.mGs.mCurRenderMode, 7, 0, 0, 7, 255);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
	SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 0);
	SetupAll(TRUE);

	f32 amount = mAmount;
	f32 fade   = sin(0.5f * (3.1415927f * amount));
	if (fpclassify(mRectWidth) == FP_NAN || fpclassify(mRectHeight) == FP_NAN) {
		CTexObj frame = WindowFrameTexture();
		f32 w         = fade * GXGetTexObjWidth(&frame);
		f32 h         = fade * GXGetTexObjHeight(&frame);
		f32 aspect    = (f32)mTexture.mWidth / mTexture.mHeight;
		DrawCorner(0xff000000, 160, 80, 0, w, h);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 2, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		if (mTexture.mImage)
			DrawQuad(mTexture.mTex, 0xffffff00 | (u32)(255 * amount), 160, 80, w * aspect, h);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		DrawQuad(frame, 0xc8c8c800 | (u32)(255 * amount), 160, 80, 1.1f * w, 1.1f * h);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
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
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 1, 1);
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
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 2, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		if (mTexture.mImage)
			DrawQuad(mTexture.mTex, 0xffffff00 | (u32)(255 * amount), x, y, w, h);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
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
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		for (int i = 0; i < 8; ++i) {
			PSMTXTrans(frame.mMtx[0], tiles[i][0], tiles[i][1], 0);
			frame.mMtxDirty = TRUE;
			PSMTXScale(frame.mMtx[1], tiles[i][2], tiles[i][3], 1);
			frame.mMtxDirty = TRUE;
			DrawQuad(frame, color, tiles[i][4], tiles[i][5], tiles[i][6], tiles[i][7]);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsAlphaCompare(gGame.mGs.mCurRenderMode, 4, 0, 0, 3, 255);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
}

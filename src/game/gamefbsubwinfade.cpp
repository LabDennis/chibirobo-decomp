#include "game/cgame.h"
#include "game/mm.h"
#include "game/video.h"

extern "C" {
u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 maxLod);
}

/**
 * @note Address: 0x80014978
 * @note Size: 0xEC
 */
void FbSubwin::Init()
{
#if defined(VERSION_PAL)
	GXRenderModeObj* mode = &gGame.mGs.mScreen;
	u16 width             = mode->fbWidth / 2;
	u16 height            = mode->efbHeight / 2;
#else
	u16 width  = 320;
	u16 height = 224;
#endif
	FbTexture* texture = &mTexture;
	texture->mWidth    = width;
	texture->mHeight   = height;
	texture->mFormat   = GX_TF_RGB565;
	texture->_0C       = 0;
	texture->mX        = 0.0f;
	texture->mY        = 0.0f;
	texture->mMipmap   = FALSE;
	texture->mImage    = mm::Alloc(GXGetTexBufferSize(width, height, GX_TF_RGB565, FALSE, 0), "(subwin)");
	mActive            = TRUE;
	texture->mTex.Init(texture->mImage, texture->mWidth, texture->mHeight, texture->mFormat, GX_CLAMP, GX_CLAMP, FALSE);
	InitGamePerspectiveCamera(&mCamera, &gGame.mGs.mScreen);
#if defined(VERSION_PAL)
	HSD_RectS16 viewport = { 0, 0, 0, 0 };
	viewport.xmax        = width;
	viewport.ymax        = height;
#else
	HSD_RectS16 viewport = { 0, 320, 0, 224 };
#endif
	HSD_CObjSetViewport(&mCamera, &viewport);
	Reset();
}

/**
 * @note Address: 0x80014964
 * @note Size: 0x14
 */
void FbSubwin::Reset()
{
	mEnabled = FALSE;
	mAmount  = 0.0f;
}

/**
 * @note Address: 0x800148C0
 * @note Size: 0xA4
 */
void FbSubwin::Update(u32 frames)
{
	if (mEnabled) {
		if (mAmount < 1.0f) {
			if ((mAmount += (1.0f / (0.3f * FRAME_RATE)) * frames) >= 1.0f) {
				mAmount = 1.0f;
			}
		}
	} else {
		if (mAmount > 0.0f) {
			if ((mAmount -= (1.0f / (0.3f * FRAME_RATE)) * frames) <= 0.0f) {
				mAmount = 0.0f;
			}
		}
	}
}

#include "game/cgame.h"
#include "game/mm.h"

extern "C" {
u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 maxLod);
}

/**
 * @note Address: 0x80014978
 * @note Size: 0xEC
 */
extern "C" void Init__8FbSubwinFv(FbSubwin* subwin)
{
#if defined(VERSION_PAL)
	GXRenderModeObj* mode = &gGame.mGs.mScreen;
	u16 width             = mode->fbWidth / 2;
	u16 height            = mode->efbHeight / 2;
#else
	u16 width  = 320;
	u16 height = 224;
#endif
	FbTexture* texture = &subwin->mTexture;
	texture->mWidth    = width;
	texture->mHeight   = height;
	texture->mFormat   = GX_TF_RGB565;
	texture->_0C       = 0;
	texture->mX        = 0.0f;
	texture->mY        = 0.0f;
	texture->mMipmap   = FALSE;
	texture->mImage    = mm::Alloc(GXGetTexBufferSize(width, height, GX_TF_RGB565, FALSE, 0), "(subwin)");
	subwin->mActive    = TRUE;
	texture->mTex.Init(texture->mImage, texture->mWidth, texture->mHeight, texture->mFormat, GX_CLAMP, GX_CLAMP, FALSE);
	InitGamePerspectiveCamera(&subwin->mCamera, &gGame.mGs.mScreen);
#if defined(VERSION_PAL)
	HSD_RectS16 viewport = { 0, 0, 0, 0 };
	viewport.xmax        = width;
	viewport.ymax        = height;
#else
	HSD_RectS16 viewport = { 0, 320, 0, 224 };
#endif
	HSD_CObjSetViewport(&subwin->mCamera, &viewport);
	subwin->Reset();
}

/**
 * @note Address: 0x80014964
 * @note Size: 0x14
 */
extern "C" void Reset__8FbSubwinFv(FbSubwin* subwin)
{
	subwin->mEnabled = FALSE;
	subwin->mAmount  = 0.0f;
}

/**
 * @note Address: 0x800148C0
 * @note Size: 0xA4
 */
extern "C" void Update__8FbSubwinFUl(FbSubwin* subwin, u32 frames)
{
	if (subwin->mEnabled) {
		if (subwin->mAmount < 1.0f) {
			if ((subwin->mAmount += frames * (1.0f / 18.0f)) >= 1.0f) {
				subwin->mAmount = 1.0f;
			}
		}
	} else {
		if (subwin->mAmount > 0.0f) {
			if ((subwin->mAmount -= frames * (1.0f / 18.0f)) <= 0.0f) {
				subwin->mAmount = 0.0f;
			}
		}
	}
}

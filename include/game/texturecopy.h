#ifndef _GAME_TEXTURECOPY_H
#define _GAME_TEXTURECOPY_H

#include "game/cgs.h"
#include "game/cgame.h"
#include "game/fbtexeff.h"

extern "C" {
void GXSetTexCopySrc(u16 left, u16 top, u16 width, u16 height);
void GXSetTexCopyDst(u16 width, u16 height, u32 format, u8 mipmap);
void GXCopyTex(void* image, u8 clear);
}

static inline CGs* CaptureGraphics()
{
	return &gGame.mGs;
}

static inline void FlushCaptureGraphics(CGs* graphics)
{
	CGs* global = CaptureGraphics();
	if (graphics->mCurRenderMode)
		ApplyGsRenderMode(global->mCurRenderMode, TRUE);
	if (global->mCurTevStage)
		ApplyGsTevStage(global->mCurTevStage, TRUE);
	if (global->mCurChannel)
		ApplyGsChannel(global->mCurChannel, TRUE);
}

static inline void InvalidateCaptureGraphics(CGs* graphics)
{
	CGs* global = CaptureGraphics();
	HSD_StateInvalidate(-1);
	if (graphics->mCurRenderMode)
		global->mCurRenderMode->Invalidate();
	if (global->mCurTevStage)
		global->mCurTevStage->Invalidate();
	if (global->mCurChannel && !global->mCurChannel->mDirty)
		global->mCurChannel->mDirty = TRUE;
}

static inline void CopyCaptureTexture(FbTexture* texture, CGs* graphics)
{
	if (texture->mImage) {
		u16 width, height;
		if (texture->mMipmap) {
			width  = texture->mWidth * 2;
			height = texture->mHeight * 2;
		} else {
			width  = texture->mWidth;
			height = texture->mHeight;
		}
		GXSetTexCopySrc(0, 0, width, height);
		GXSetTexCopyDst(texture->mWidth, texture->mHeight, texture->mFormat, texture->mMipmap);
		GXCopyTex(texture->mImage, 1);
		GXPixModeSync();
		InvalidateCaptureGraphics(graphics);
	}
}

#endif

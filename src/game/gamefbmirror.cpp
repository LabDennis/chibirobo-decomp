#include "game/fbtexeff.h"
#include "game/texturecopy.h"

/**
 * @note Address: 0x80014A64
 * @note Size: 0x144
 */
// FbMirror's Draw, sets up gx for drawing the mirror's texture
void FbMirror::Draw()
{
	if (mJoint) {
		CGs* graphics = CaptureGraphics();
		if (graphics->mCurRenderMode)
			ApplyGsRenderMode(graphics->mCurRenderMode, TRUE);
		if (graphics->mCurTevStage)
			ApplyGsTevStage(graphics->mCurTevStage, TRUE);
		if (graphics->mCurChannel)
			ApplyGsChannel(graphics->mCurChannel, TRUE);
		SetGsChannelEnabled(&gGame.mGs, 4, 0);
		SetGsBlendMode(graphics->mCurRenderMode, 0, 4, 5, 15);
		SetGsAlphaCompare(graphics->mCurRenderMode, 7, 0, 0, 7, 255);
		SetGsDepthMode(graphics->mCurRenderMode, 1, 3, 1);
		SetGsColorUpdate(graphics->mCurRenderMode, 1);
		SetGsAlphaUpdate(graphics->mCurRenderMode, 0);
		CGs* global = CaptureGraphics();
		SetGsTevMode(global->mCurTevStage, 0);
		SetGsCullMode(graphics->mCurRenderMode, 1);
		SetGsCullMode(graphics->mCurRenderMode, 2);
		SetGsTevMode(global->mCurTevStage, 4);
		SetGsAlphaCompare(graphics->mCurRenderMode, 4, 0, 0, 3, 255);
	}
}

/**
 * @note Address: 0x80014BA8
 * @note Size: 0x1FC
 */
// FbMirror's Copy
void FbMirror::Copy()
{
	if (mVisible) {
		CGs* graphics = CaptureGraphics();
		SetGsColorUpdate(graphics->mCurRenderMode, 1);
		SetGsAlphaUpdate(graphics->mCurRenderMode, 1);
		SetGsDepthMode(graphics->mCurRenderMode, 0, 7, 1);
		FlushCaptureGraphics(graphics);
		CopyCaptureTexture(&mTexture, graphics);
	}
}

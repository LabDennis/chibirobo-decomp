#include "game/fbtexeff.h"
#include "game/texturecopy.h"

/**
 * @note Address: 0x800146D0
 * @note Size: 0x1F0
 */
// FbSubwin's Copy
extern "C" void Copy__8FbSubwinFv(FbSubwin* subwin)
{
	CGs* graphics = CaptureGraphics();
	SetGsColorUpdate(graphics->mCurRenderMode, 1);
	SetGsAlphaUpdate(graphics->mCurRenderMode, 1);
	SetGsDepthMode(graphics->mCurRenderMode, 0, 7, 1);
	FlushCaptureGraphics(graphics);
	CopyCaptureTexture(&subwin->mTexture, graphics);
}

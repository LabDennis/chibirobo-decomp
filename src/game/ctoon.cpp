#include "game/ctoon.h"
extern "C" {
#include <dolphin/os.h>
#include <dolphin/gx/gx.h>
}

/**
 * @note Address: 0x801A6580
 * @note Size: 0x124
 */
extern "C" void InitToonTexture(ToonTexture* toon, u8 min, u8 max, u8 steps)
{
	toon->mImageDesc.img_ptr = toon->mTexels;
	toon->mImageDesc.width   = 256;
	toon->mImageDesc.height  = 1;
	toon->mImageDesc.format  = GX_TF_RGB565;
	toon->mImageDesc.mipmap  = 0;
	toon->mImageDesc.minLOD  = 0.0f;
	toon->mImageDesc.maxLOD  = 0.0f;

	u16 color        = ((u32)min << 11) | ((u32)min << 6) | min;
	toon->mTexels[0] = color;

	for (int i = 1; i < 256; i++) {
		if (i % (256 / steps) == 0) {
			u8 level = ((256 / (steps - 1)) * (i / (256 / steps)) - 1) >> 3;
			if (level > max) {
				level = max;
			}
			if (level < min) {
				level = min;
			}
			color = (level << 11) | (level << 6) | level;
		}
		// top row of each 4x4 tile
		toon->mTexels[(i / 4) * 16 + i % 4] = color;
	}

	DCFlushRange(toon, sizeof(toon->mTexels));
	GXInvalidateTexAll();
}

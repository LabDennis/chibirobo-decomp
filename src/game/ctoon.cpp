#include "Version.h"
#include "game/ctoon.h"
#if defined(VERSION_R24J01)
extern "C" {
#include "Dolphin/OS/OSCache.h"
#include "Dolphin/GX/GXTexture.h"
}
#else
extern "C" {
#include <dolphin/os.h>
#include <dolphin/gx/gx.h>
}
#endif

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

	// NONMATCHING (Wii): retail re-evaluates 256/steps inside the branch
	// (a second divw at 0x802BD310) where this build reuses the hoisted divisor;
	// the rest of the loop and the desc stores match.
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

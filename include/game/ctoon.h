#ifndef _GAME_CTOON_H
#define _GAME_CTOON_H

extern "C" {
#include <sysdolphin/tobj.h>
}
#include "types.h"

// the toon ramp CGame keeps at +0x3460, the desc gets handed to SetToonTObj
struct ToonTexture {
	u16 mTexels[0x400];       // _000, 256x1 rgb565 in 4x4 tiles, only row 0 of each tile is used
	HSD_ImageDesc mImageDesc; // _800
};
STATIC_ASSERT(sizeof(ToonTexture) == 0x818);

extern "C" void InitToonTexture(ToonTexture* toon, u8 min, u8 max, u8 steps);
extern "C" void SetToonTObj(HSD_ImageDesc* image);

#endif

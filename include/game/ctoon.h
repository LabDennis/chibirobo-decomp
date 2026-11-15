#ifndef _GAME_CTOON_H
#define _GAME_CTOON_H

#include "Version.h"
#include "types.h"
#if defined(VERSION_R24J01)
#include "Dolphin/GX/GXEnum.h"
// the Wii build kept the same desc struct the stores prove at +0x800:
// img_ptr/width/height/format/mipmap/minLOD/maxLOD (retail InitToonTexture 0x802BD294).
// There is no sysdolphin on Wii, so the layout is declared under the same name.
typedef struct _HSD_ImageDesc {
	void* img_ptr;
	u16 width;
	u16 height;
	GXTexFmt format;
	u32 mipmap;
	f32 minLOD;
	f32 maxLOD;
} HSD_ImageDesc;
#else
extern "C" {
#include <sysdolphin/tobj.h>
}
#endif

// the toon ramp CGame keeps at +0x3460, the desc gets handed to SetToonTObj
struct ToonTexture {
	u16 mTexels[0x400];       // _000, 256x1 rgb565 in 4x4 tiles, only row 0 of each tile is used
	HSD_ImageDesc mImageDesc; // _800
};
STATIC_ASSERT(sizeof(ToonTexture) == 0x818);

extern "C" void InitToonTexture(ToonTexture* toon, u8 min, u8 max, u8 steps);
extern "C" void SetToonTObj(HSD_ImageDesc* image);

#endif

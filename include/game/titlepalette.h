#ifndef _GAME_TITLEPALETTE_H
#define _GAME_TITLEPALETTE_H

#include "types.h"
#ifdef _MTXTYPES_h_
#include <dolphin/gx/gxtypes.h>
#else
#include "Dolphin/gx.h"
#endif

struct TitlePaletteEntry {
	GXColor background; // _00
	GXColor text;       // _04
};
STATIC_ASSERT(sizeof(TitlePaletteEntry) == 0x08);

#endif

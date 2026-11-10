#include "game/title.h"

const TitlePaletteEntry gTitlePalette[] = {
	{ { 0, 0, 128, 0 }, {} },     { { 128, 0, 0, 0 }, {} },     { { 0, 128, 0, 0 }, {} },
	{ { 200, 200, 200, 0 }, {} }, { { 128, 128, 128, 0 }, {} }, { { 0, 207, 233, 0 }, {} },
	{ { 255, 220, 133, 0 }, {} }, { { 255, 220, 0, 0 }, {} },   { {}, { 255, 255, 255, 0 } },
};
#if defined(VERSION_JP)
const s32 gTitleLogoTextures[3] = { 0, 3, 4 };
#else
const s32 gTitleLogoTextures[3] = {};
#endif
#if defined(VERSION_PAL)
const Vec gTitleDemoStartPosition = { 0, 4, 0 };
#else
const Vec gTitleDemoStartPosition = { 0, 8, 0 };
#endif

s32 gTitlePaletteIndex = 3;
s32 gTitleLogoIndex    = 1;

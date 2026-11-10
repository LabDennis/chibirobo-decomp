#ifndef _GAME_DRAWHELPTEXT_H
#define _GAME_DRAWHELPTEXT_H
#include "game/cgame.h"
#include <math.h>
#include "game/menuhelpchar.h"

static inline bool HelpTextIsJapanese()
{
#if defined(VERSION_JP)
	return true;
#else
	return gGame.mLanguage == 6;
#endif
}

// The glyph width is rounded before subtracting it from the cell width.
static inline f32 HelpGlyphPadding(s32 width, f32 scale)
{
	f32 glyphWidth = (f32)((double)(width / 24.0f) * scale);
	return scale - glyphWidth;
}

static inline void DrawHelpLetter(CTexObj& tex, f32 x, f32 y, f32 bob, f32 scale, f32 padding, f32 rotation, f32 zero, u32 opacity,
                                  const f32* outlineX, const f32* outlineY)
{
	for (int j = 0; j < 4; j++) {
		DrawGsTexturedQuad(&gGame.mGs, &tex, opacity, opacity, opacity, opacity, __fmadds(padding, 0.5f, x + outlineX[j]),
		                   bob + (384.0f + outlineY[j]), 1.0f, scale, scale, 0.0f, -zero, rotation);
	}
	u32 color = opacity | 0xFFFFFB00;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, __fmadds(padding, 0.5f, x), y, 1.0f, scale, scale, 0.0f, -zero,
	                   rotation);
}
#endif

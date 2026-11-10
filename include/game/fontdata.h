#ifndef _GAME_FONTDATA_H
#define _GAME_FONTDATA_H

#include "Version.h"
#include <dolphin/os.h>

struct FontGlyph;

struct FontGlyphRange {
	FontGlyph* glyphs; // _00
	u16 first;         // _04
	u16 end;           // _06
};

struct FontCharInfo {
	s32 x;     // _00
	s32 y;     // _04
	s32 width; // _08
};

struct FontGlyphCache {
	FontGlyph* glyphs; // _00
	u32 next;          // _04
};

extern "C" {
extern u16 gFontEncoding;
extern OSFontHeader* gActiveFontHeader;
extern BOOL gUseSingleByteFont;
extern FontGlyphCache gDynamicFontGlyphCache;
extern FontGlyphRange gFontGlyphRanges[];
extern FontCharInfo gSingleByteGlyphMetrics[];
extern u8 gSingleByteFontAtlas[];
extern OSFontHeader gSingleByteFontHeader;
}

#endif

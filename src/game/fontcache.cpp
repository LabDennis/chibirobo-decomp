#include "game/fontdata.h"

extern "C" {
u16 gFontEncoding;
OSFontHeader* gActiveFontHeader;
#if !defined(VERSION_JP)
BOOL gUseSingleByteFont;
#endif
FontGlyphCache gDynamicFontGlyphCache;

FontGlyphRange gFontGlyphRanges[] = {
	{ NULL, 0x0020, 0x007F }, { NULL, 0x8140, 0x81F8 }, { NULL, 0x824F, 0x82F2 }, { NULL, 0x8340, 0x8397 }, { NULL, 0, 0 },
};
}

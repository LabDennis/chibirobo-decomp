#ifndef _GAME_CFONT_H
#define _GAME_CFONT_H

#include "game/ctpl.h"
#include "Version.h"

// the game font, class name is RTTI
class CFont {
public:
	CFont() { mDesc.textureHeader = NULL; }

	TPLHeader mImage;    // _00
	TPLDescriptor mDesc; // _24, the font loads itself on first use while the header is null
	f32 mInvSheetWidth;  // _2C
	f32 mInvSheetHeight; // _30
	f32 mInvCellWidth;   // _34
	f32 mInvCellHeight;  // _38
	CTexObj mTex;        // _3C
	// _13C vtable

	virtual ~CFont() { } // _08 (weak)
};
STATIC_ASSERT(sizeof(CFont) == 0x140);

// shift jis lead byte
inline bool IsSjisLead(u8 c)
{
	u8 lead = FALSE;
	if ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC)) {
		lead = TRUE;
	}
	return lead;
}

extern "C" {
// the glyph texture for one character and how wide it is
CTexObj* GetFontGlyphTexture(CFont* font, const char* str, s32* width);
#if defined(VERSION_JP)
void Init__5CFontFv(CFont* font);
#else
void Init__5CFontFi(CFont* font, BOOL builtin);
#endif
}

#endif

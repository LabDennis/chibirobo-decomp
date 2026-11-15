#ifndef _GAME_CFONT_H
#define _GAME_CFONT_H

#include "game/ctpl.h"
#include "Version.h"

// the game font, class name is RTTI
#if defined(VERSION_R24J01)
// the Wii font keeps its tpl header next to the descriptor instead of the
// GameCube class's palette order. name kept from the GameCube class.
class CFont {
public:
	TPLDescriptor mDesc; // _00, textureHeader is always &mImage
	TPLHeader mImage;    // _08
	f32 mInvSheetWidth;  // _2C
	f32 mInvSheetHeight; // _30
	f32 mInvCellWidth;   // _34
	f32 mInvCellHeight;  // _38
	CTexObj mTex;        // _3C
	// _13C is mTex's own vtable, the font itself has none

	void Init(const char* resPath, BOOL singleByte);

private:
	// picks the built-in single byte font or the console ROM font for the
	// current encoding. only touches globals, but it is a member in retail
	void SetupFontHeader();
};
#else
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

#if defined(VERSION_JP)
	void Init();
#else
	void Init(BOOL builtin);
#endif
};
#endif
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

#if !defined(VERSION_R24J01)
extern "C" {
// the glyph texture for one character and how wide it is
CTexObj* GetFontGlyphTexture(CFont* font, const char* str, s32* width);
}
#endif

#if defined(VERSION_R24J01)
#include "game/cfile.h"
#include "Dolphin/OS/OSFont.h"
#include <nw4r/ut/ut_ResFont.h>

// the Wii font system loads an RFNU/RFNT font file through a resource
// instead of only using the console font. the parser is NW4R's ResFont
// (mangled __ct__12CWiiFontFileFv in retail), living in its own translation
// unit (the RFNU chunk walker at 0x80235268); its vtable at 0x80332878
// sits behind the base object tables at 0x803327B0 and 0x8033281C.
struct SWiiFontResource {
	OSFontHeader mHeader;      // _00, rewritten per glyph in the cache path
	CFile mFile;               // _30
	nw4r::ut::ResFont mParser; // _11C
};
STATIC_ASSERT(sizeof(SWiiFontResource) == 0x138);

extern const OSFontHeader gSingleByteFontHeader;
extern const s32 gSingleByteGlyphMetrics[][3];
extern const u8 gSingleByteFontAtlas[];

extern "C" {
// the glyph texture for one character and how wide it is
CTexObj* GetFontGlyphTexture(CFont* font, const char* str, u32 arg3, s32* width);
// nonzero when the loaded font has a glyph for the character
s8 HasFontGlyph(const char* str);
}
#endif

#endif

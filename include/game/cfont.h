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

// the cmap lookup returns the glyph index and a flag byte packed into
// a three byte result; it travels in r3 like retail's return value did
struct SGlyphIndex {
	u16 glyph; // _00
	u8 flags;  // _02
};

// the Wii font system loads an RFNU/RFNT font file through a resource
// instead of only using the console font. the parser lives in its own
// translation unit (the RFNU chunk walker at 0x80235268); only the slots
// this file calls are named, the rest of the interface stays in
// declaration order so the vtable offsets line up with retail
// (vtable 0x80332878, base 0x803327B0).
class CWiiFontFile {
public:
	CWiiFontFile();
	virtual ~CWiiFontFile();
	virtual void Unk01();
	virtual void Unk02();
	virtual void Unk03();
	virtual void Unk04();
	virtual void Unk05();
	virtual u8 Unk06();                                // _20, nested chunk byte 1
	virtual u8 Unk07();                                // _24, nested chunk byte 0
	virtual void Unk08();                              // _28
	virtual void Unk09();                              // _2C
	virtual void Unk0A();                              // _30
	virtual void Unk0B();                              // _34
	virtual void Unk0C();                              // _38
	virtual void Unk0D();                              // _3C
	virtual void Unk0E();                              // _40
	virtual void Unk0F();                              // _44
	virtual void Unk10();                              // _48
	virtual SGlyphIndex GetGlyphIndex(u16 code);       // _4C
	virtual void GetGlyphMetrics(void* out, u16 code); // _50
	virtual void Unk13();                              // _54
	virtual void Unk14();                              // _58

	// parse a loaded RFNU/RFNT image (checks the RFNU magic and FINF chunk)
	void LoadFontData(const void* data);

	u32 mCmapDefaults[3]; // _04, copied from the table at 0x80332810
	void* mExtra;         // _10
	void* mChunk;         // _14, current cmap chunk
	u16 mLastCode;        // _18
	u16 mLastGlyph;       // _1A
};

// the loaded font resource: a header block the cache path fills per
// glyph, the file the font was read from and the RFNU/RFNT parser.
struct SWiiFontResource {
	OSFontHeader mHeader; // _00, rewritten per glyph in the cache path
	CFile mFile;          // _30
	CWiiFontFile mParser; // _11C
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

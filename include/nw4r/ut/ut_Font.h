#ifndef NW4R_UT_FONT_H
#define NW4R_UT_FONT_H

#include <Dolphin/gx.h>
#include <types.h>

// nw4r::ut::Font, the base of the resource font classes, as shipped with the
// Wii edition. The interface order is the retail vtable of CWiiFontFile at
// 0x80332878 (base object vtables at 0x803327B0 and 0x8033281C).

namespace nw4r {
namespace ut {

typedef u16 CharCode;
typedef u16 GlyphIndex;

// the three byte result of GetCharWidths; a value struct returned packed in r3
struct CharWidths {
	s8 left;       // _00
	u8 glyphWidth; // _01
	s8 charWidth;  // _02
};

// the glyph metrics GetGlyph fills for one character, as this build writes
// them (vtable 0x80332878 slot _50, GetGlyphFromIndex at 0x80235948)
struct Glyph {
	void* pTexture;    // _00
	CharWidths widths; // _04
	u8 cellHeight;     // _07
	u32 sheetFormat;   // _08, GXTexFmt of the sheet the glyph is on
	u16 sheetWidth;    // _0C
	u16 sheetHeight;   // _0E
	u16 x;             // _10, texel position inside the sheet
	u16 y;             // _12
};

class Font {
public:
	enum Type {
		TYPE_NULL,
		TYPE_ROM,
		TYPE_RESOURCE,
	};

	enum Encoding {
		ENCODING_UTF8,
		ENCODING_UTF16,
		ENCODING_SJIS,
		ENCODING_CP1252,
		ENCODING_NUM,
	};

	typedef CharCode (Font::*CharStrmReader)(const void*);

	// _04, set by InitReaderFunc; the constructor loads the entry the table
	// at 0x80332810 holds, InitReaderFunc switches it per encoding
	// (0x80235268, table 0x80332780)
	CharStrmReader mReadFunc;

	Font();
	virtual ~Font();                                             // _08
	virtual int GetWidth() const;                                // _0C
	virtual int GetHeight() const;                               // _10
	virtual int GetAscent() const;                               // _14
	virtual int GetDescent() const;                              // _18
	virtual int GetBaselinePos() const;                          // _1C
	virtual int GetCellHeight() const;                           // _20
	virtual int GetCellWidth() const;                            // _24
	virtual int GetMaxCharWidth() const;                         // _28
	virtual Type GetType() const;                                // _2C
	virtual GXTexFmt GetTextureFormat() const;                   // _30
	virtual int GetLineFeed() const;                             // _34
	virtual const CharWidths GetDefaultCharWidths() const;       // _38
	virtual void SetDefaultCharWidths(const CharWidths& widths); // _3C
	virtual bool SetAlternateChar(CharCode c);                   // _40
	virtual void SetLineFeed(int linefeed);                      // _44
	virtual int GetCharWidth(CharCode c) const;                  // _48
	virtual const CharWidths GetCharWidths(CharCode c) const;    // _4C
	virtual void GetGlyph(Glyph* glyph, CharCode c) const;       // _50
	virtual bool HasGlyph(CharCode c) const;                     // _54
	virtual Encoding GetEncoding() const;                        // _58

	// switch mReadFunc to the reader for the given encoding (0x80235268)
	void InitReaderFunc(Encoding encoding);
};
STATIC_ASSERT(sizeof(Font) == 0x10);

} // namespace ut
} // namespace nw4r

#endif

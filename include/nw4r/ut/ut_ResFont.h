#ifndef NW4R_UT_RESFONT_H
#define NW4R_UT_RESFONT_H

#include <nw4r/ut/ut_Font.h>

// nw4r::ut::ResFont and its implementation base detail::ResFontBase, as
// shipped with the Wii edition: an RFNU/RFNT resource font parsed from a
// file image. Layout and vtable order from the retail objects at 0x8033281C
// (ResFontBase) and 0x80332878 (ResFont, mangled __ct__12CWiiFontFileFv).

namespace nw4r {
namespace ut {

namespace detail {

// the TGLP chunk: the glyph sheet description of the loaded font
struct FontTextureGlyph {
	u8 cellWidth;    // _00
	u8 cellHeight;   // _01
	s8 baselinePos;  // _02
	u8 maxCharWidth; // _03
	u32 sheetSize;   // _04, bytes per sheet
	u16 sheetNum;    // _08
	u16 sheetFormat; // _0A, GXTexFmt
	u16 sheetRow;    // _0C
	u16 sheetColumn; // _0E
	u16 sheetWidth;  // _10
	u16 sheetHeight; // _12
	u8* sheetImage;  // _14
};

// the WDTH chunk: per-glyph width records, in lists of index ranges
struct FontWidth {
	u16 indexBegin;       // _00
	u16 indexEnd;         // _02
	FontWidth* pNext;     // _04
	CharWidths widths[1]; // _08, one three byte record per index
};

// the CMAP chunk: char code ranges to glyph indices
struct FontCodeMap {
	u16 ccBegin;        // _00
	u16 ccEnd;          // _02
	u16 mappingMethod;  // _04, 0 offset, 1 table, 2 scan
	u16 reserved;       // _06
	FontCodeMap* pNext; // _08
	u16 mapInfo[1];     // _0C
};

// the FINF chunk: the parsed font information the interface reads
struct FontInformation {
	u8 fontType;                  // _00
	s8 linefeed;                  // _01
	u16 alterCharIndex;           // _02
	CharWidths defaultCharWidths; // _04
	u8 encoding;                  // _07
	FontTextureGlyph* pTglp;      // _08
	FontWidth* pWidth;            // _0C
	FontCodeMap* pMap;            // _10
	u8 height;                    // _14
	u8 width;                     // _15
	u8 ascent;                    // _16
	u8 padding;                   // _17
};
STATIC_ASSERT(sizeof(FontInformation) == 0x18);

// the implementation shared by the resource fonts
class ResFontBase : public Font {
public:
	ResFontBase();          // 0x80235350
	virtual ~ResFontBase(); // _08

	virtual int GetWidth() const;
	virtual int GetHeight() const;
	virtual int GetAscent() const;
	virtual int GetDescent() const;
	virtual int GetBaselinePos() const;
	virtual int GetCellHeight() const;
	virtual int GetCellWidth() const;
	virtual int GetMaxCharWidth() const;
	virtual Type GetType() const;
	virtual GXTexFmt GetTextureFormat() const;
	virtual int GetLineFeed() const;
	virtual const CharWidths GetDefaultCharWidths() const;
	virtual void SetDefaultCharWidths(const CharWidths& widths);
	virtual bool SetAlternateChar(CharCode c);
	virtual void SetLineFeed(int linefeed);
	virtual int GetCharWidth(CharCode c) const;
	virtual const CharWidths GetCharWidths(CharCode c) const;
	virtual void GetGlyph(Glyph* glyph, CharCode c) const;
	virtual bool HasGlyph(CharCode c) const;
	virtual Encoding GetEncoding() const;

	// point the parser at a loaded image and its FINF chunk (0x802353E4)
	void SetResourceBuffer(void* pBuffer, FontInformation* pFontInfo);

	// the cmap walk behind SetAlternateChar/GetCharWidths/GetGlyph/HasGlyph
	u16 FindGlyphIndex(CharCode c) const;
	u16 FindGlyphIndex(const FontCodeMap* pMap, CharCode c) const; // 0x80235874

	void GetGlyphFromIndex(Glyph* pGlyph, u16 index) const; // 0x80235948

	void* mBuffer;              // _10, the loaded font file, NULL when unset
	FontInformation* mFontInfo; // _14, the FINF chunk inside mBuffer
	u16 mLastCharCode;          // _18, FindGlyphIndex cache key
	u16 mLastGlyphIndex;        // _1A, FindGlyphIndex cache value
};

} // namespace detail

// the font the game loads for its text. Its own ctor/dtor (0x80235A50,
// 0x80235A8C) and SetResource (0x80235AE4) were emitted out of line in a
// class the symbols name CWiiFontFile; the vtable is the ResFont one.
class ResFont : public detail::ResFontBase {
public:
	ResFont();
	virtual ~ResFont();

	// take over a loaded RFNU/RFNT image: find the FINF chunk, reconstruct
	// it for the file format when needed, then call SetResourceBuffer and
	// InitReaderFunc. fails (FALSE) when a buffer is set or the image is bad
	bool SetResource(void* pBuffer); // 0x80235AE4
};
STATIC_ASSERT(sizeof(ResFont) == 0x1C);

} // namespace ut
} // namespace nw4r

#endif

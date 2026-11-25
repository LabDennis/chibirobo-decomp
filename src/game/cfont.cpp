#include "game/cfont.h"
#include "Version.h"
#include "game/mm.h"
#include <dolphin/os.h>
#include <string.h>
#include "game/ut.h"

// a 24x24 I4 glyph, then its character code and advance width
struct FontGlyph {
	u8 image[0x120];        // _00
	u16 code;               // _120
	s16 width;              // _122
	u8 _124[0x140 - 0x124]; // _124
};
STATIC_ASSERT(sizeof(FontGlyph) == 0x140);

struct FontGlyphRange {
	FontGlyph* glyphs; // _00
	u16 first;         // _04
	u16 end;           // _06
};

struct FontGlyphCache {
	FontGlyph* glyphs; // _00
	u32 next;          // _04
};

struct FontCharInfo {
	s32 x;     // _00
	s32 y;     // _04
	s32 width; // _08
};

#if defined(VERSION_JP)
#define FONT_INIT_LINE(us, jp) jp
#else
#define FONT_INIT_LINE(us, jp) us
#endif

#if !defined(VERSION_JP)
// the font the western editions draw single-byte text with: 21 columns of 24x24 cells on a 512x512 sheet
OSFontHeader gSingleByteFontHeader = {
	0,        // fontType
	0x20,     // firstChar
	0xFF,     // lastChar
	0x20,     // invalChar
	24,       // ascent
	0,        // descent
	24,       // width
	28,       // leading
	24,       // cellWidth
	24,       // cellHeight
	0x20000,  // sheetSize
	GX_TF_I4, // sheetFormat
	21,       // sheetColumn
	11,       // sheetRow
	512,      // sheetWidth
	512,      // sheetHeight
	0x30,     // widthTable
	0x120,    // sheetImage
	0x20000,  // sheetFullSize
	0x00,     // c0
	0x55,     // c1
	0xAA,     // c2
	0xFF,     // c3
};

// where a character's cell starts on the sheet and how far it advances, by character code
FontCharInfo gSingleByteGlyphMetrics[256] = {
	{ 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      // 0x00
	{ 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      // 0x04
	{ 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      // 0x08
	{ 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      // 0x0C
	{ 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      // 0x10
	{ 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      // 0x14
	{ 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      // 0x18
	{ 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      { 0, 0, 0 },      // 0x1C
	{ 0, 0, 12 },     { 24, 0, 6 },     { 48, 0, 11 },    { 72, 0, 18 },    // 0x20
	{ 96, 0, 16 },    { 120, 0, 22 },   { 144, 0, 19 },   { 168, 0, 6 },    // 0x24
	{ 192, 0, 10 },   { 216, 0, 10 },   { 240, 0, 15 },   { 264, 0, 19 },   // 0x28
	{ 288, 0, 7 },    { 312, 0, 17 },   { 336, 0, 7 },    { 360, 0, 23 },   // 0x2C
	{ 384, 0, 18 },   { 408, 0, 10 },   { 432, 0, 17 },   { 456, 0, 17 },   // 0x30
	{ 480, 0, 19 },   { 0, 24, 18 },    { 24, 24, 18 },   { 48, 24, 17 },   // 0x34
	{ 72, 24, 17 },   { 96, 24, 17 },   { 120, 24, 7 },   { 144, 24, 7 },   // 0x38
	{ 168, 24, 17 },  { 192, 24, 19 },  { 216, 24, 17 },  { 240, 24, 15 },  // 0x3C
	{ 264, 24, 20 },  { 288, 24, 18 },  { 312, 24, 16 },  { 336, 24, 17 },  // 0x40
	{ 360, 24, 17 },  { 384, 24, 14 },  { 408, 24, 14 },  { 432, 24, 17 },  // 0x44
	{ 456, 24, 17 },  { 480, 24, 7 },   { 0, 48, 11 },    { 24, 48, 16 },   // 0x48
	{ 48, 48, 14 },   { 72, 48, 19 },   { 96, 48, 16 },   { 120, 48, 18 },  // 0x4C
	{ 144, 48, 15 },  { 168, 48, 18 },  { 192, 48, 17 },  { 216, 48, 16 },  // 0x50
	{ 240, 48, 16 },  { 264, 48, 16 },  { 288, 48, 17 },  { 312, 48, 22 },  // 0x54
	{ 336, 48, 17 },  { 360, 48, 17 },  { 384, 48, 15 },  { 408, 48, 10 },  // 0x58
	{ 432, 48, 23 },  { 456, 48, 9 },   { 480, 48, 13 },  { 0, 72, 24 },    // 0x5C
	{ 24, 72, 7 },    { 48, 72, 15 },   { 72, 72, 15 },   { 96, 72, 14 },   // 0x60
	{ 120, 72, 15 },  { 144, 72, 15 },  { 168, 72, 10 },  { 192, 72, 15 },  // 0x64
	{ 216, 72, 14 },  { 240, 72, 7 },   { 264, 72, 9 },   { 288, 72, 14 },  // 0x68
	{ 312, 72, 7 },   { 336, 72, 19 },  { 360, 72, 14 },  { 384, 72, 15 },  // 0x6C
	{ 408, 72, 15 },  { 432, 72, 15 },  { 456, 72, 10 },  { 480, 72, 13 },  // 0x70
	{ 0, 96, 11 },    { 24, 96, 14 },   { 48, 96, 15 },   { 72, 96, 19 },   // 0x74
	{ 96, 96, 15 },   { 120, 96, 14 },  { 144, 96, 13 },  { 168, 96, 10 },  // 0x78
	{ 192, 96, 5 },   { 216, 96, 10 },  { 240, 96, 21 },  { 264, 96, 24 },  // 0x7C
	{ 288, 96, 20 },  { 312, 96, 24 },  { 336, 96, 6 },   { 360, 96, 15 },  // 0x80
	{ 384, 96, 11 },  { 408, 96, 21 },  { 432, 96, 14 },  { 456, 96, 14 },  // 0x84
	{ 480, 96, 13 },  { 0, 120, 23 },   { 24, 120, 15 },  { 48, 120, 11 },  // 0x88
	{ 72, 120, 21 },  { 96, 120, 24 },  { 120, 120, 15 }, { 144, 120, 24 }, // 0x8C
	{ 168, 120, 24 }, { 192, 120, 7 },  { 216, 120, 7 },  { 240, 120, 11 }, // 0x90
	{ 264, 120, 11 }, { 288, 120, 10 }, { 312, 120, 21 }, { 336, 120, 24 }, // 0x94
	{ 360, 120, 15 }, { 384, 120, 18 }, { 408, 120, 14 }, { 432, 120, 11 }, // 0x98
	{ 456, 120, 19 }, { 480, 120, 24 }, { 0, 144, 13 },   { 24, 144, 17 },  // 0x9C
	{ 48, 144, 24 },  { 72, 144, 6 },   { 96, 144, 15 },  { 120, 144, 17 }, // 0xA0
	{ 144, 144, 17 }, { 168, 144, 17 }, { 192, 144, 6 },  { 216, 144, 15 }, // 0xA4
	{ 240, 144, 12 }, { 264, 144, 23 }, { 288, 144, 11 }, { 312, 144, 14 }, // 0xA8
	{ 336, 144, 18 }, { 360, 144, 10 }, { 384, 144, 23 }, { 408, 144, 24 }, // 0xAC
	{ 432, 144, 11 }, { 456, 144, 18 }, { 480, 144, 10 }, { 0, 168, 11 },   // 0xB0
	{ 24, 168, 10 },  { 48, 168, 14 },  { 72, 168, 14 },  { 96, 168, 8 },   // 0xB4
	{ 120, 168, 8 },  { 144, 168, 7 },  { 168, 168, 11 }, { 192, 168, 14 }, // 0xB8
	{ 216, 168, 20 }, { 240, 168, 20 }, { 264, 168, 21 }, { 288, 168, 16 }, // 0xBC
	{ 312, 168, 18 }, { 336, 168, 18 }, { 360, 168, 18 }, { 384, 168, 18 }, // 0xC0
	{ 408, 168, 19 }, { 432, 168, 19 }, { 456, 168, 20 }, { 480, 168, 16 }, // 0xC4
	{ 0, 192, 15 },   { 24, 192, 15 },  { 48, 192, 15 },  { 72, 192, 15 },  // 0xC8
	{ 96, 192, 6 },   { 120, 192, 6 },  { 144, 192, 6 },  { 168, 192, 6 },  // 0xCC
	{ 192, 192, 20 }, { 216, 192, 16 }, { 240, 192, 17 }, { 264, 192, 18 }, // 0xD0
	{ 288, 192, 18 }, { 312, 192, 18 }, { 336, 192, 18 }, { 360, 192, 17 }, // 0xD4
	{ 384, 192, 18 }, { 408, 192, 16 }, { 432, 192, 16 }, { 456, 192, 16 }, // 0xD8
	{ 480, 192, 16 }, { 0, 216, 17 },   { 24, 216, 15 },  { 48, 216, 15 },  // 0xDC
	{ 72, 216, 15 },  { 96, 216, 15 },  { 120, 216, 15 }, { 144, 216, 15 }, // 0xE0
	{ 168, 216, 15 }, { 192, 216, 15 }, { 216, 216, 19 }, { 240, 216, 14 }, // 0xE4
	{ 264, 216, 14 }, { 288, 216, 14 }, { 312, 216, 14 }, { 336, 216, 14 }, // 0xE8
	{ 360, 216, 6 },  { 384, 216, 6 },  { 408, 216, 6 },  { 432, 216, 6 },  // 0xEC
	{ 456, 216, 16 }, { 480, 216, 14 }, { 0, 240, 15 },   { 24, 240, 15 },  // 0xF0
	{ 48, 240, 15 },  { 72, 240, 15 },  { 96, 240, 15 },  { 120, 240, 19 }, // 0xF4
	{ 144, 240, 15 }, { 168, 240, 15 }, { 192, 240, 15 }, { 216, 240, 14 }, // 0xF8
	{ 240, 240, 14 }, { 264, 240, 15 }, { 288, 240, 16 }, { 312, 240, 14 }, // 0xFC
};

// the sheet, in I4. its bytes are not in the repository: dtk writes them from the disc image when it splits the dol
u8 gSingleByteFontAtlas[0x20000] ATTRIBUTE_ALIGN(32) = {
#include "assets/font_sheet.inc"
};
#endif

extern "C" {
u16 gFontEncoding;
OSFontHeader* gActiveFontHeader;
#if !defined(VERSION_JP)
BOOL gUseSingleByteFont;
#endif
FontGlyphCache gDynamicFontGlyphCache;
// the static ranges get their glyphs rendered at init; misses go to the 50-entry cache
FontGlyphRange gFontGlyphRanges[] = {
	{ NULL, 0x0020, 0x007F }, { NULL, 0x8140, 0x81F8 }, { NULL, 0x824F, 0x82F2 }, { NULL, 0x8340, 0x8397 }, { NULL, 0, 0 },
};
}

static inline bool UseGlyphCache()
{
#if defined(VERSION_JP)
	return gFontEncoding == OS_FONT_ENCODE_SJIS;
#else
	return gFontEncoding == OS_FONT_ENCODE_SJIS && !gUseSingleByteFont;
#endif
}

/**
 * @note Address: 0x8019E8F4
 * @note Size: 0x34C
 */
extern "C" CTexObj* GetFontGlyphTexture(CFont* font, const char* str, s32* width)
{
	void* image;
	s32 x, y, advance;

	if (font->mDesc.textureHeader == NULL) {
#if defined(VERSION_JP)
		font->Init();
#else
		font->Init(FALSE);
#endif
	}
	if (UseGlyphCache()) {
		u16 code = (u8)str[0];
		if (IsSjisLead(code)) { // a lead byte takes the next byte along
			code = (code << 8) | (u8)str[1];
		}
		image = NULL;
		for (s32 i = 0; gFontGlyphRanges[i].glyphs != NULL; i++) {
			if (code >= gFontGlyphRanges[i].first && code < gFontGlyphRanges[i].end) {
				FontGlyph* glyph = &gFontGlyphRanges[i].glyphs[code - gFontGlyphRanges[i].first];
				image            = glyph;
				advance          = glyph->width;
				break;
			}
		}
		if (image == NULL) {
			for (s32 i = 0; i < 50; i++) { // the 50-entry dynamic cache
				if (code == gDynamicFontGlyphCache.glyphs[i].code) {
					image   = &gDynamicFontGlyphCache.glyphs[i];
					advance = gDynamicFontGlyphCache.glyphs[i].width;
					break;
				}
			}
			if (image == NULL) {
				// a miss renders the glyph into the next round robin slot
				FontGlyph* glyph = &gDynamicFontGlyphCache.glyphs[gDynamicFontGlyphCache.next];
				memset(glyph, 0, sizeof(FontGlyph));
				OSGetFontTexel((char*)str, glyph, 0, 6, &advance);
				glyph->code  = code;
				glyph->width = advance;
				DCFlushRange(glyph, sizeof(FontGlyph));
				image                       = glyph;
				gDynamicFontGlyphCache.next = (gDynamicFontGlyphCache.next + 1) % 50;
			}
		}
		y = 0;
		x = 0;
#if !defined(VERSION_JP)
	} else if (gUseSingleByteFont) {
		image   = gSingleByteFontAtlas;
		x       = gSingleByteGlyphMetrics[(u8)str[0]].x;
		y       = gSingleByteGlyphMetrics[(u8)str[0]].y;
		advance = gSingleByteGlyphMetrics[(u8)str[0]].width;
#endif
	} else {
		OSGetFontTexture(str, &image, &x, &y, &advance);
	}

	font->mImage.data = (char*)image;
	font->mTex.Init(&font->mDesc, 0);
	PSMTXTrans(font->mTex.mMtx[0], font->mInvSheetWidth * x, font->mInvSheetHeight * y, 0.0f);
	font->mTex.mMtxDirty = TRUE;
	PSMTXScale(font->mTex.mMtx[1], font->mInvCellWidth, font->mInvCellHeight, 1.0f);
	font->mTex.mMtxDirty = TRUE;
	if (width != NULL) {
		*width = advance;
	}
	return &font->mTex;
}
/**
 * @note Address: 0x8019EC40
 * @note Size: 0x738
 */
#if defined(VERSION_JP)
void CFont::Init()
#else
void CFont::Init(BOOL builtin)
#endif
{
	if (gActiveFontHeader == NULL) {
		gFontEncoding = OSGetFontEncode();
#if !defined(VERSION_JP)
		gUseSingleByteFont = builtin != FALSE;
#endif
		if (UseGlyphCache()) {
			u32 size          = OS_FONT_DATA_SIZE_SJIS;
			gActiveFontHeader = (OSFontHeader*)mm::Alloc((size + 31) & ~31, "(font I2)");
			if (gActiveFontHeader == NULL) {
				OSReport("\n!!!HALT!!!\n");
				OSPanic("cfont.cpp", FONT_INIT_LINE(71, 65), "%s%s", "not enough memory for ROM font\n", "Init");
			}
			size       = OS_FONT_ROM_SIZE_SJIS;
			void* temp = mm::Alloc((size + 31) & ~31, "(temp)");
			if (temp == NULL) {
				OSReport("\n!!!HALT!!!\n");
				OSPanic("cfont.cpp", FONT_INIT_LINE(77, 71), "%s%s", "not enough memory for ROM font(temp)\n", "Init");
			}
			if (OSLoadFont(gActiveFontHeader, temp) == 0) {
				OSReport("\n!!!HALT!!!\n");
				OSPanic("cfont.cpp", FONT_INIT_LINE(81, 75), "%s%s", "internal error on OSLoadFont()\n", "Init");
			}
			mm::Free(temp, 1);

			for (FontGlyphRange* range = gFontGlyphRanges; range->first != range->end; range++) {
				range->glyphs = (FontGlyph*)mm::Alloc((range->end - range->first) * sizeof(FontGlyph),
				                                      FormatToStaticBuffer("(%04X~%04X)", range->first, range->end));
				if (range->glyphs != NULL) {
					u16 code         = range->first;
					FontGlyph* glyph = range->glyphs;
					s32 count        = range->end - range->first;
					for (s32 i = 0; i < count; i++, glyph++, code++) { // pre-render the whole range
						char encoded[3] = { static_cast<char>(code >> 8), static_cast<char>(code), '\0' };
						char* str       = code > 0xFF ? encoded : &encoded[1];
						s32 advance;
						memset(glyph, 0, sizeof(FontGlyph));
						OSGetFontTexel(str, glyph, 0, 6, &advance);
						glyph->width = advance;
						glyph->code  = code;
						DCFlushRange(glyph, sizeof(FontGlyph));
					}
				} else {
					OSReport("\n!!!HALT!!!\n");
					OSPanic("cfont.cpp", FONT_INIT_LINE(105, 99), "%s%s",
					        FormatToStaticBuffer("not enough memory for static image (%04X~%04X)\n", range->first, range->end), "Init");
				}
			}
			FontGlyphCache* cache = &gDynamicFontGlyphCache;
			cache->glyphs         = (FontGlyph*)mm::Alloc(50 * sizeof(FontGlyph), "(dynamic)");
			if (cache->glyphs != NULL) {
				cache->next = 0;
				for (s32 i = 0; i < 50; i++) {
					cache->glyphs[i].code = 0;
				}
			} else {
				OSReport("\n!!!HALT!!!\n");
				OSPanic("cfont.cpp", FONT_INIT_LINE(118, 112), "%s%s", "not enough memory for dynamic image\n", "Init");
			}
#if !defined(VERSION_JP)
		} else if (gUseSingleByteFont) {
			gActiveFontHeader = &gSingleByteFontHeader;
#endif
		} else { // not SJIS-cached: the ROM font gets used as is
			u32 size          = OS_FONT_SIZE_ANSI;
			gActiveFontHeader = (OSFontHeader*)mm::Alloc(size, "(font I4)");
			if (gActiveFontHeader == NULL) {
				OSReport("\n!!!HALT!!!\n");
				OSPanic("cfont.cpp", FONT_INIT_LINE(131, 120), "%s%s", "not enough memory for ROM font\n", "Init");
			}
			if (!OSInitFont(gActiveFontHeader)) {
				OSReport("\n!!!HALT!!!\n");
				OSPanic("cfont.cpp", FONT_INIT_LINE(135, 124), "%s%s", "internal error on OSInitFont()\n", "Init");
			}
		}
	}

	TPLDescriptor desc = { NULL, NULL };
	desc.textureHeader = &mImage;
	mDesc              = desc;
	if (UseGlyphCache()) {
		TPLHeader image = { 24, 24, GX_TF_I4, NULL, GX_CLAMP, GX_CLAMP, GX_LINEAR, GX_LINEAR, 0.0f, 0, 0, 0, 1 };
		mImage          = image;
		mInvSheetWidth  = 0.0f;
		mInvSheetHeight = 0.0f;
		mInvCellWidth   = 1.0f;
		mInvCellHeight  = 1.0f;
	} else {
		TPLHeader image = { 0, 0, GX_TF_I4, NULL, GX_CLAMP, GX_CLAMP, GX_LINEAR, GX_LINEAR, 0.0f, 0, 0, 0, 1 };
		image.height    = gActiveFontHeader->sheetHeight;
		image.width     = gActiveFontHeader->sheetWidth;
		image.format    = gActiveFontHeader->sheetFormat;
		mImage          = image;
		// inverse sizes feed the two texture matrices
		mInvSheetWidth  = 1.0f / gActiveFontHeader->sheetWidth;
		mInvSheetHeight = 1.0f / gActiveFontHeader->sheetHeight;
		mInvCellWidth   = 1.0f / (gActiveFontHeader->cellWidth - 2);
		mInvCellHeight  = 1.0f / (gActiveFontHeader->cellHeight - 2);
	}
}

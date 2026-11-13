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
		if (IsSjisLead(code)) {
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
			for (s32 i = 0; i < 50; i++) {
				if (code == gDynamicFontGlyphCache.glyphs[i].code) {
					image   = &gDynamicFontGlyphCache.glyphs[i];
					advance = gDynamicFontGlyphCache.glyphs[i].width;
					break;
				}
			}
			if (image == NULL) {
				image = &gDynamicFontGlyphCache.glyphs[gDynamicFontGlyphCache.next];
				memset(image, 0, sizeof(FontGlyph));
				OSGetFontTexel((char*)str, image, 0, 6, &advance);
				gDynamicFontGlyphCache.glyphs[gDynamicFontGlyphCache.next].code  = code;
				gDynamicFontGlyphCache.glyphs[gDynamicFontGlyphCache.next].width = advance;
				DCFlushRange(&gDynamicFontGlyphCache.glyphs[gDynamicFontGlyphCache.next], sizeof(FontGlyph));
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
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stmw     r27, 0x2c(r1)
    mr       r27, r3
    mr       r28, r4
    mr       r29, r5
    lwz      r0, 0x24(r3)
    cmplwi   r0, 0x0
    bne      .L_8019E924
    li       r4, 0x0
    bl       Init__5CFontFi
.L_8019E924:
    lhz      r0, gFontEncoding@sda21(r0)
    li       r3, 0x0
    cmplwi   r0, 0x1
    bne      .L_8019E944
    lwz      r0, gUseSingleByteFont@sda21(r0)
    cmpwi    r0, 0x0
    bne      .L_8019E944
    li       r3, 0x1
.L_8019E944:
    clrlwi.  r0, r3, 24
    beq      .L_8019EB28
    lbz      r4, 0x0(r28)
    li       r3, 0x0
    clrlwi   r0, r4, 24
    mr       r30, r4
    cmplwi   r0, 0x81
    blt      .L_8019E96C
    cmplwi   r0, 0x9f
    ble      .L_8019E980
.L_8019E96C:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0xe0
    blt      .L_8019E984
    cmplwi   r0, 0xfc
    bgt      .L_8019E984
.L_8019E980:
    li       r3, 0x1
.L_8019E984:
    clrlwi.  r0, r3, 24
    beq      .L_8019E998
    lbz      r0, 0x1(r28)
    rlwimi   r0, r30, 8, 8, 23
    clrlwi   r30, r0, 16
.L_8019E998:
    li       r0, 0x0
    lis      r3, gFontGlyphRanges@ha
    addi     r4, r3, gFontGlyphRanges@l
    stw      r0, 0x14(r1)
    mr       r3, r4
    clrlwi   r5, r30, 16
    li       r6, 0x0
    b        .L_8019EA04
.L_8019E9B8:
    lhz      r0, 0x4(r3)
    cmplw    r5, r0
    blt      .L_8019E9FC
    lhz      r0, 0x6(r3)
    cmplw    r5, r0
    bge      .L_8019E9FC
    slwi     r0, r6, 3
    add      r3, r4, r0
    lwzx     r4, r4, r0
    lhz      r0, 0x4(r3)
    subf     r0, r0, r5
    mulli    r0, r0, 0x140
    add      r3, r4, r0
    stw      r3, 0x14(r1)
    lha      r0, 0x122(r3)
    stw      r0, 0x8(r1)
    b        .L_8019EA10
.L_8019E9FC:
    addi     r3, r3, 0x8
    addi     r6, r6, 0x1
.L_8019EA04:
    lwz      r0, 0x0(r3)
    cmplwi   r0, 0x0
    bne      .L_8019E9B8
.L_8019EA10:
    lwz      r0, 0x14(r1)
    cmplwi   r0, 0x0
    bne      .L_8019EB18
    lwz      r6, gDynamicFontGlyphCache@sda21(r0)
    li       r0, 0x32
    clrlwi   r3, r30, 16
    li       r5, 0x0
    mr       r4, r6
    li       r7, gDynamicFontGlyphCache@sda21
    mtctr    r0
.L_8019EA38:
    lhz      r0, 0x120(r4)
    cmplw    r3, r0
    bne      .L_8019EA60
    mulli    r3, r5, 0x140
    lwz      r0, 0x0(r7)
    add      r3, r0, r3
    stw      r3, 0x14(r1)
    lha      r0, 0x122(r3)
    stw      r0, 0x8(r1)
    b        .L_8019EA6C
.L_8019EA60:
    addi     r4, r4, 0x140
    addi     r5, r5, 0x1
    bdnz     .L_8019EA38
.L_8019EA6C:
    lwz      r0, 0x14(r1)
    cmplwi   r0, 0x0
    bne      .L_8019EB18
    li       r31, gDynamicFontGlyphCache@sda21
    li       r4, 0x0
    lwz      r0, 0x4(r31)
    li       r5, 0x140
    mulli    r0, r0, 0x140
    add      r3, r6, r0
    stw      r3, 0x14(r1)
    bl       memset
    lwz      r4, 0x14(r1)
    mr       r3, r28
    addi     r7, r1, 0x8
    li       r5, 0x0
    li       r6, 0x6
    bl       OSGetFontTexel
    lwz      r0, 0x4(r31)
    li       r4, 0x140
    lwz      r3, gDynamicFontGlyphCache@sda21(r0)
    mulli    r0, r0, 0x140
    add      r3, r3, r0
    sth      r30, 0x120(r3)
    lwz      r0, 0x4(r31)
    lwz      r3, gDynamicFontGlyphCache@sda21(r0)
    mulli    r0, r0, 0x140
    lwz      r5, 0x8(r1)
    add      r3, r3, r0
    sth      r5, 0x122(r3)
    lwz      r0, 0x4(r31)
    lwz      r3, gDynamicFontGlyphCache@sda21(r0)
    mulli    r0, r0, 0x140
    add      r3, r3, r0
    bl       DCFlushRange
    lwz      r4, 0x4(r31)
    lis      r3, 0x51ec
    subi     r0, r3, 0x7ae1
    addi     r3, r4, 0x1
    mulhwu   r0, r0, r3
    srwi     r0, r0, 4
    mulli    r0, r0, 0x32
    subf     r0, r0, r3
    stw      r0, 0x4(r31)
.L_8019EB18:
    li       r0, 0x0
    stw      r0, 0xc(r1)
    stw      r0, 0x10(r1)
    b        .L_8019EB88
.L_8019EB28:
    lwz      r0, gUseSingleByteFont@sda21(r0)
    cmpwi    r0, 0x0
    beq      .L_8019EB70
    lbz      r0, 0x0(r28)
    lis      r3, gSingleByteGlyphMetrics@ha
    lis      r4, gSingleByteFontAtlas@ha
    mulli    r0, r0, 0xc
    addi     r3, r3, gSingleByteGlyphMetrics@l
    addi     r4, r4, gSingleByteFontAtlas@l
    stw      r4, 0x14(r1)
    add      r4, r3, r0
    lwzx     r5, r3, r0
    lwz      r3, 0x4(r4)
    lwz      r0, 0x8(r4)
    stw      r5, 0x10(r1)
    stw      r3, 0xc(r1)
    stw      r0, 0x8(r1)
    b        .L_8019EB88
.L_8019EB70:
    mr       r3, r28
    addi     r4, r1, 0x14
    addi     r5, r1, 0x10
    addi     r6, r1, 0xc
    addi     r7, r1, 0x8
    bl       OSGetFontTexture
.L_8019EB88:
    lwz      r0, 0x14(r1)
    addi     r3, r27, 0x3c
    addi     r4, r27, 0x24
    li       r5, 0x0
    stw      r0, 0x8(r27)
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r5, 0x10(r1)
    lis      r4, 0x4330
    lwz      r0, 0xc(r1)
    addi     r3, r27, 0x78
    xoris    r5, r5, 0x8000
    stw      r4, 0x18(r1)
    xoris    r0, r0, 0x8000
    lfd      f4, lbl_8065F688@sda21(r0)
    stw      r5, 0x1c(r1)
    lfs      f5, 0x2c(r27)
    lfd      f0, 0x18(r1)
    stw      r0, 0x24(r1)
    fsubs    f1, f0, f4
    lfs      f2, 0x30(r27)
    stw      r4, 0x20(r1)
    lfs      f3, lbl_8065F680@sda21(r0)
    lfd      f0, 0x20(r1)
    fmuls    f1, f5, f1
    fsubs    f0, f0, f4
    fmuls    f2, f2, f0
    bl       PSMTXTrans
    li       r0, 0x1
    lfs      f3, lbl_8065F684@sda21(r0)
    stw      r0, 0x74(r27)
    addi     r3, r27, 0xa8
    lfs      f1, 0x34(r27)
    lfs      f2, 0x38(r27)
    bl       PSMTXScale
    li       r0, 0x1
    cmplwi   r29, 0x0
    stw      r0, 0x74(r27)
    beq      .L_8019EC28
    lwz      r0, 0x8(r1)
    stw      r0, 0x0(r29)
.L_8019EC28:
    addi     r3, r27, 0x3c
    lmw      r27, 0x2c(r1)
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

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
			u32 size          = UseGlyphCache() ? OS_FONT_DATA_SIZE_SJIS : OS_FONT_DATA_SIZE_ANSI;
			gActiveFontHeader = (OSFontHeader*)mm::Alloc((size + 31) & ~31, "(font I2)");
			if (gActiveFontHeader == NULL) {
				OSReport("\n!!!HALT!!!\n");
				OSPanic("cfont.cpp", FONT_INIT_LINE(71, 65), "%s%s", "not enough memory for ROM font\n", "Init");
			}
			size       = UseGlyphCache() ? OS_FONT_ROM_SIZE_SJIS : OS_FONT_ROM_SIZE_ANSI;
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
					for (s32 i = 0; i < count; i++, glyph++, code++) {
						// big endian, single byte characters are the low byte
						char* str = code & 0xFF00 ? (char*)&code : (char*)&code + 1;
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
		} else {
			u32 size          = UseGlyphCache() ? OS_FONT_SIZE_SJIS : OS_FONT_SIZE_ANSI;
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
		mInvSheetWidth  = 1.0f / gActiveFontHeader->sheetWidth;
		mInvSheetHeight = 1.0f / gActiveFontHeader->sheetHeight;
		mInvCellWidth   = 1.0f / (gActiveFontHeader->cellWidth - 2);
		mInvCellHeight  = 1.0f / (gActiveFontHeader->cellHeight - 2);
	}
}
/*
    stwu     r1, -0xa0(r1)
    mflr     r0
    lis      r5, lbl_8020ECA8@ha
    stw      r0, 0xa4(r1)
    stmw     r24, 0x80(r1)
    mr       r28, r3
    mr       r24, r4
    addi     r31, r5, lbl_8020ECA8@l
    lwz      r0, gActiveFontHeader@sda21(r0)
    cmplwi   r0, 0x0
    bne      .L_8019F120
    bl       OSGetFontEncode
    neg      r0, r24
    clrlwi   r4, r3, 16
    or       r0, r0, r24
    sth      r3, gFontEncoding@sda21(r0)
    srwi     r3, r0, 31
    cmplwi   r4, 0x1
    stw      r3, gUseSingleByteFont@sda21(r0)
    li       r0, 0x0
    bne      .L_8019ECA0
    cmpwi    r3, 0x0
    bne      .L_8019ECA0
    li       r0, 0x1
.L_8019ECA0:
    clrlwi.  r0, r0, 24
    beq      .L_8019F060
    cmplwi   r4, 0x1
    li       r3, 0x0
    bne      .L_8019ECC4
    lwz      r0, gUseSingleByteFont@sda21(r0)
    cmpwi    r0, 0x0
    bne      .L_8019ECC4
    li       r3, 0x1
.L_8019ECC4:
    clrlwi.  r0, r3, 24
    lis      r3, 0x1
    addi     r3, r3, 0x120
    beq      .L_8019ECDC
    lis      r3, 0x9
    addi     r3, r3, 0xee4
.L_8019ECDC:
    addi     r0, r3, 0x1f
    addi     r4, r31, 0x60
    clrrwi   r3, r0, 5
    bl       Alloc__2mmFUlPCc
    cmplwi   r3, 0x0
    stw      r3, gActiveFontHeader@sda21(r0)
    bne      .L_8019ED20
    addi     r3, r31, 0x6c
    crclr    cr1eq
    bl       OSReport
    addi     r3, r31, 0x7c
    addi     r6, r31, 0x88
    li       r4, 0x47
    li       r5, lbl_8065F690@sda21
    li       r7, sFontInitFunctionName@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8019ED20:
    lhz      r0, gFontEncoding@sda21(r0)
    li       r3, 0x0
    cmplwi   r0, 0x1
    bne      .L_8019ED40
    lwz      r0, gUseSingleByteFont@sda21(r0)
    cmpwi    r0, 0x0
    bne      .L_8019ED40
    li       r3, 0x1
.L_8019ED40:
    clrlwi.  r0, r3, 24
    li       r3, 0x3000
    beq      .L_8019ED54
    lis      r3, 0x5
    subi     r3, r3, 0x3000
.L_8019ED54:
    addi     r0, r3, 0x1f
    li       r4, lbl_8065F698@sda21
    clrrwi   r3, r0, 5
    bl       Alloc__2mmFUlPCc
    mr.      r25, r3
    bne      .L_8019ED94
    addi     r3, r31, 0x6c
    crclr    cr1eq
    bl       OSReport
    addi     r3, r31, 0x7c
    addi     r6, r31, 0xa8
    li       r4, 0x4d
    li       r5, lbl_8065F690@sda21
    li       r7, sFontInitFunctionName@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8019ED94:
    lwz      r3, gActiveFontHeader@sda21(r0)
    mr       r4, r25
    bl       OSLoadFont
    cmplwi   r3, 0x0
    bne      .L_8019EDD0
    addi     r3, r31, 0x6c
    crclr    cr1eq
    bl       OSReport
    addi     r3, r31, 0x7c
    addi     r6, r31, 0xd0
    li       r4, 0x51
    li       r5, lbl_8065F690@sda21
    li       r7, sFontInitFunctionName@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8019EDD0:
    mr       r3, r25
    li       r4, 0x1
    bl       Free__2mmFPvl
    lis      r3, gFontGlyphRanges@ha
    addi     r29, r1, 0x9
    addi     r30, r3, gFontGlyphRanges@l
    b        .L_8019EEF8
.L_8019EDEC:
    addi     r3, r31, 0xf0
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lhz      r5, 0x4(r30)
    mr       r4, r3
    lhz      r0, 0x6(r30)
    subf     r0, r5, r0
    mulli    r3, r0, 0x140
    bl       Alloc__2mmFUlPCc
    stw      r3, 0x0(r30)
    lwz      r0, 0x0(r30)
    cmplwi   r0, 0x0
    beq      .L_8019EEB8
    lhz      r3, 0x4(r30)
    mr       r26, r0
    lhz      r0, 0x6(r30)
    li       r24, 0x0
    sth      r3, 0x8(r1)
    subf     r27, r3, r0
    b        .L_8019EEAC
.L_8019EE3C:
    lhz      r0, 0x8(r1)
    rlwinm.  r0, r0, 0, 16, 23
    beq      .L_8019EE50
    addi     r25, r1, 0x8
    b        .L_8019EE54
.L_8019EE50:
    mr       r25, r29
.L_8019EE54:
    mr       r3, r26
    li       r4, 0x0
    li       r5, 0x140
    bl       memset
    mr       r3, r25
    mr       r4, r26
    addi     r7, r1, 0xc
    li       r5, 0x0
    li       r6, 0x6
    bl       OSGetFontTexel
    lwz      r0, 0xc(r1)
    mr       r3, r26
    li       r4, 0x140
    sth      r0, 0x122(r26)
    lhz      r0, 0x8(r1)
    sth      r0, 0x120(r26)
    bl       DCFlushRange
    lhz      r3, 0x8(r1)
    addi     r24, r24, 0x1
    addi     r26, r26, 0x140
    addi     r0, r3, 0x1
    sth      r0, 0x8(r1)
.L_8019EEAC:
    cmpw     r24, r27
    blt      .L_8019EE3C
    b        .L_8019EEF4
.L_8019EEB8:
    addi     r3, r31, 0x6c
    crclr    cr1eq
    bl       OSReport
    lhz      r4, 0x4(r30)
    addi     r3, r31, 0xfc
    lhz      r5, 0x6(r30)
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    addi     r3, r31, 0x7c
    li       r4, 0x69
    li       r5, lbl_8065F690@sda21
    li       r7, sFontInitFunctionName@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8019EEF4:
    addi     r30, r30, 0x8
.L_8019EEF8:
    lhz      r4, 0x4(r30)
    lhz      r5, 0x6(r30)
    cmplw    r4, r5
    bne      .L_8019EDEC
    addi     r4, r31, 0x12c
    li       r29, gDynamicFontGlyphCache@sda21
    li       r3, 0x3e80
    bl       Alloc__2mmFUlPCc
    cmplwi   r3, 0x0
    stw      r3, gDynamicFontGlyphCache@sda21(r0)
    beq      .L_8019F034
    li       r12, 0x0
    li       r0, 0x3
    stw      r12, 0x4(r29)
    li       r24, 0x0
    mtctr    r0
.L_8019EF38:
    lwz      r3, 0x0(r29)
    addi     r10, r12, 0x120
    li       r11, 0x0
    addi     r9, r12, 0x260
    sthx     r11, r3, r10
    addi     r8, r12, 0x3a0
    addi     r7, r12, 0x4e0
    addi     r6, r12, 0x620
    lwz      r3, 0x0(r29)
    addi     r5, r12, 0x760
    addi     r4, r12, 0x8a0
    addi     r0, r12, 0x9e0
    sthx     r11, r3, r9
    addi     r10, r12, 0xb20
    addi     r9, r12, 0xc60
    addi     r24, r24, 0x10
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r8
    addi     r8, r12, 0xda0
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r7
    addi     r7, r12, 0xee0
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r6
    addi     r6, r12, 0x1020
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r5
    addi     r5, r12, 0x1160
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r4
    addi     r4, r12, 0x12a0
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r0
    addi     r0, r12, 0x13e0
    addi     r12, r12, 0x1400
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r10
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r9
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r8
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r7
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r6
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r5
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r4
    lwz      r3, 0x0(r29)
    sthx     r11, r3, r0
    bdnz     .L_8019EF38
    subfic   r0, r24, 0x32
    mulli    r4, r24, 0x140
    mtctr    r0
    cmpwi    r24, 0x32
    bge      .L_8019F120
.L_8019F01C:
    lwz      r3, 0x0(r29)
    addi     r0, r4, 0x120
    addi     r4, r4, 0x140
    sthx     r11, r3, r0
    bdnz     .L_8019F01C
    b        .L_8019F120
.L_8019F034:
    addi     r3, r31, 0x6c
    crclr    cr1eq
    bl       OSReport
    addi     r3, r31, 0x7c
    addi     r6, r31, 0x138
    li       r4, 0x76
    li       r5, lbl_8065F690@sda21
    li       r7, sFontInitFunctionName@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8019F120
.L_8019F060:
    lwz      r3, gUseSingleByteFont@sda21(r0)
    cmpwi    r3, 0x0
    beq      .L_8019F07C
    lis      r3, gSingleByteFontHeader@ha
    addi     r0, r3, gSingleByteFontHeader@l
    stw      r0, gActiveFontHeader@sda21(r0)
    b        .L_8019F120
.L_8019F07C:
    cmplwi   r4, 0x1
    li       r0, 0x0
    bne      .L_8019F094
    cmpwi    r3, 0x0
    bne      .L_8019F094
    li       r0, 0x1
.L_8019F094:
    clrlwi.  r0, r0, 24
    lis      r3, 0x2
    addi     r3, r3, 0x120
    beq      .L_8019F0AC
    lis      r3, 0x12
    addi     r3, r3, 0xf00
.L_8019F0AC:
    addi     r4, r31, 0x160
    bl       Alloc__2mmFUlPCc
    cmplwi   r3, 0x0
    stw      r3, gActiveFontHeader@sda21(r0)
    bne      .L_8019F0E8
    addi     r3, r31, 0x6c
    crclr    cr1eq
    bl       OSReport
    addi     r3, r31, 0x7c
    addi     r6, r31, 0x88
    li       r4, 0x83
    li       r5, lbl_8065F690@sda21
    li       r7, sFontInitFunctionName@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8019F0E8:
    lwz      r3, gActiveFontHeader@sda21(r0)
    bl       OSInitFont
    cmpwi    r3, 0x0
    bne      .L_8019F120
    addi     r3, r31, 0x6c
    crclr    cr1eq
    bl       OSReport
    addi     r3, r31, 0x7c
    addi     r6, r31, 0x16c
    li       r4, 0x87
    li       r5, lbl_8065F690@sda21
    li       r7, sFontInitFunctionName@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8019F120:
    lwz      r0, lbl_806602C8@sda21(r0)
    li       r3, 0x0
    lwz      r4, lbl_806602CC@sda21(r0)
    stw      r0, 0x10(r1)
    stw      r28, 0x24(r28)
    stw      r4, 0x28(r28)
    lhz      r0, gFontEncoding@sda21(r0)
    stw      r4, 0x14(r1)
    cmplwi   r0, 0x1
    stw      r28, 0x10(r1)
    bne      .L_8019F15C
    lwz      r0, gUseSingleByteFont@sda21(r0)
    cmpwi    r0, 0x0
    bne      .L_8019F15C
    li       r3, 0x1
.L_8019F15C:
    clrlwi.  r0, r3, 24
    beq      .L_8019F218
    lwz      r0, 0x18(r31)
    lwz      r12, 0x1c(r31)
    stw      r0, 0x3c(r1)
    lwz      r11, 0x20(r31)
    lwz      r10, 0x24(r31)
    lwz      r9, 0x28(r31)
    lwz      r8, 0x2c(r31)
    lwz      r7, 0x30(r31)
    lwz      r6, 0x34(r31)
    lwz      r4, 0x38(r31)
    lhz      r3, 0x3c(r1)
    lhz      r0, 0x3e(r1)
    sth      r3, 0x0(r28)
    lfs      f1, lbl_8065F680@sda21(r0)
    sth      r0, 0x2(r28)
    lfs      f0, lbl_8065F684@sda21(r0)
    stw      r12, 0x4(r28)
    stw      r11, 0x8(r28)
    stw      r4, 0x5c(r1)
    stw      r10, 0xc(r28)
    lbz      r5, 0x5c(r1)
    stw      r9, 0x10(r28)
    lbz      r4, 0x5d(r1)
    stw      r6, 0x58(r1)
    lbz      r3, 0x5e(r1)
    stw      r8, 0x14(r28)
    lfs      f2, 0x58(r1)
    stw      r7, 0x18(r28)
    lbz      r0, 0x5f(r1)
    stfs     f2, 0x1c(r28)
    stb      r5, 0x20(r28)
    stb      r4, 0x21(r28)
    stb      r3, 0x22(r28)
    stb      r0, 0x23(r28)
    stfs     f1, 0x2c(r28)
    stfs     f1, 0x30(r28)
    stfs     f0, 0x34(r28)
    stw      r12, 0x40(r1)
    stw      r11, 0x44(r1)
    stw      r10, 0x48(r1)
    stw      r9, 0x4c(r1)
    stw      r8, 0x50(r1)
    stw      r7, 0x54(r1)
    stfs     f0, 0x38(r28)
    b        .L_8019F364
.L_8019F218:
    lwz      r4, gActiveFontHeader@sda21(r0)
    lis      r0, 0x4330
    lwz      r25, 0x3c(r31)
    lwz      r26, 0x40(r31)
    lwz      r27, 0x44(r31)
    lwz      r30, 0x48(r31)
    lwz      r29, 0x4c(r31)
    lwz      r12, 0x50(r31)
    lwz      r11, 0x54(r31)
    lwz      r10, 0x58(r31)
    lwz      r3, 0x5c(r31)
    lhz      r9, 0x20(r4)
    lhz      r8, 0x1e(r4)
    lhz      r7, 0x18(r4)
    stw      r3, 0x38(r1)
    lfd      f2, lbl_8065F6A0@sda21(r0)
    sth      r9, 0x0(r28)
    lbz      r6, 0x38(r1)
    sth      r8, 0x2(r28)
    lbz      r5, 0x39(r1)
    stw      r7, 0x4(r28)
    lbz      r4, 0x3a(r1)
    stw      r27, 0x8(r28)
    lbz      r3, 0x3b(r1)
    stw      r30, 0xc(r28)
    lfs      f3, lbl_8065F684@sda21(r0)
    stw      r29, 0x10(r28)
    lfd      f1, lbl_8065F688@sda21(r0)
    stw      r10, 0x34(r1)
    stw      r12, 0x14(r28)
    lfs      f0, 0x34(r1)
    stw      r11, 0x18(r28)
    stfs     f0, 0x1c(r28)
    stb      r6, 0x20(r28)
    stb      r5, 0x21(r28)
    stb      r4, 0x22(r28)
    stb      r3, 0x23(r28)
    lwz      r3, gActiveFontHeader@sda21(r0)
    stw      r0, 0x60(r1)
    lhz      r3, 0x1e(r3)
    stw      r25, 0x18(r1)
    stw      r3, 0x64(r1)
    lfd      f0, 0x60(r1)
    stw      r26, 0x1c(r1)
    fsubs    f0, f0, f2
    stw      r0, 0x68(r1)
    fdivs    f0, f3, f0
    stw      r0, 0x70(r1)
    stw      r0, 0x78(r1)
    stw      r27, 0x20(r1)
    stw      r30, 0x24(r1)
    stw      r29, 0x28(r1)
    stfs     f0, 0x2c(r28)
    lwz      r3, gActiveFontHeader@sda21(r0)
    stw      r12, 0x2c(r1)
    lhz      r0, 0x20(r3)
    stw      r11, 0x30(r1)
    stw      r0, 0x6c(r1)
    lfd      f0, 0x68(r1)
    sth      r9, 0x18(r1)
    fsubs    f0, f0, f2
    sth      r8, 0x1a(r1)
    fdivs    f0, f3, f0
    stw      r7, 0x1c(r1)
    stfs     f0, 0x30(r28)
    lwz      r3, gActiveFontHeader@sda21(r0)
    lhz      r3, 0x10(r3)
    subi     r0, r3, 0x2
    xoris    r0, r0, 0x8000
    stw      r0, 0x74(r1)
    lfd      f0, 0x70(r1)
    fsubs    f0, f0, f1
    fdivs    f0, f3, f0
    stfs     f0, 0x34(r28)
    lwz      r3, gActiveFontHeader@sda21(r0)
    lhz      r3, 0x12(r3)
    subi     r0, r3, 0x2
    xoris    r0, r0, 0x8000
    stw      r0, 0x7c(r1)
    lfd      f0, 0x78(r1)
    fsubs    f0, f0, f1
    fdivs    f0, f3, f0
    stfs     f0, 0x38(r28)
.L_8019F364:
    lmw      r24, 0x80(r1)
    lwz      r0, 0xa4(r1)
    mtlr     r0
    addi     r1, r1, 0xa0
    blr
*/

#include "game/cfont.h"
#include "Version.h"
#include "game/mm.h"
#include <dolphin/os.h>
#include <string.h>

// the Wii font unit. unlike the GameCube it can draw text from an RFNU/RFNT
// font file loaded next to the console font; the file's parser lives in its
// own translation unit and is reached through the resource below.

// the font state kept in small data. the retail unit shares one base
// register inside each of these two groups, so the members were part of
// two small structs: the font environment and the active font pointers
static struct {
	u16 encoding;             // _00, OS_FONT_ENCODE_*
	u8 useSingleByteFont;     // _02
	const char* resourcePath; // _04
} sFontEnv;
static struct {
	SWiiFontResource* resource; // _00
	OSFontHeader* header;       // _04
} sFontPtr;

// the resource is a file static, the rest of the game only sees it
// through sFontPtr.resource once the file is loaded
static SWiiFontResource sFontResource;

// the metrics the RFNU/RFNT parser reports for one glyph. names unknown,
// only the fields the texture setup reads are identified
struct SGlyphMetrics {
	void* image; // _00
	u8 _04[2];
	s8 advance; // _06
	u32 format; // _08, the low half reaches the tpl header
	u16 width;  // _0C
	u16 height; // _0E
	u16 x;      // _10
	u16 y;      // _12
};

void SetupFontImage(CFont* font, const void* image, OSFontHeader* header);

/**
 * @note Address: 0x802B3B78
 * @note Size: 0x94
 */
void CFont::Init(const char* resPath, BOOL singleByte)
{
	sFontEnv.encoding = OSGetFontEncode();
	if (resPath != NULL) {
		sFontEnv.resourcePath = resPath;
	} else {
		sFontEnv.resourcePath = NULL;
	}
	if (singleByte != FALSE) {
		sFontEnv.useSingleByteFont = TRUE;
	} else {
		sFontEnv.useSingleByteFont = FALSE;
	}
	SetupFontHeader();
}

// load the font header the game draws with: the built-in single byte font,
// otherwise the console ROM font for the current encoding
/**
 * @note Address: 0x802B3C0C
 * @note Size: 0xC8
 */
void CFont::SetupFontHeader()
{
	if (sFontEnv.useSingleByteFont) {
		sFontPtr.header = (OSFontHeader*)&gSingleByteFontHeader;
		return;
	}
	sFontPtr.header
	    = (OSFontHeader*)mm::Alloc(sFontEnv.encoding == OS_FONT_ENCODE_SJIS ? OS_FONT_SIZE_SJIS : OS_FONT_SIZE_ANSI, "(font I4)");
	if (sFontPtr.header == NULL) {
		OSPanic("cfont.cpp", 0xa4, "not enough memory for ROM font\n");
	}
	if (!OSInitFont(sFontPtr.header)) {
		OSPanic("cfont.cpp", 0xa8, "internal error on OSInitFont()\n");
	}
}

// load the font file named at Init and point the global at it
/**
 * @note Address: 0x802B3CD4
 * @note Size: 0x9C
 */
s32 LoadFontResource()
{
	if (sFontEnv.resourcePath == NULL) {
		return 1;
	}
	SWiiFontResource* res = &sFontResource;
	s32 ok                = 0;
	if (res->mFile.ReadEx(sFontEnv.resourcePath, 0, 0)) {
		res->mParser.LoadFontData(res->mFile.mBuffer);
		ok = 1;
	}
	if (ok) {
		sFontPtr.resource = res;
		return 1;
	}
	return 0;
}

// configure the embedded tpl header and the normalized glyph sizes from a
// font header. the GameCube wrote the same block inline inside CFont::Init
/**
 * @note Address: 0x802B3D70
 * @note Size: 0x18C
 */
void SetupFontImage(CFont* font, const void* image, OSFontHeader* header)
{
	TPLDescriptor desc = { &font->mImage, NULL };
	TPLHeader tpl      = {
        0,    // height, filled below
        0,    // width
        0,    // format
        NULL, // data
        GX_CLAMP, GX_CLAMP, GX_LINEAR, GX_LINEAR, 0.0f, 0, 0, 0, 1,
	};
	tpl.height            = header->sheetHeight;
	tpl.width             = header->sheetWidth;
	tpl.format            = header->sheetFormat;
	tpl.data              = (char*)image;
	font->mDesc           = desc;
	font->mImage          = tpl;
	font->mInvSheetWidth  = 1.0f / header->sheetWidth;
	font->mInvSheetHeight = 1.0f / header->sheetHeight;
	font->mInvCellWidth   = (f32)header->cellWidth / header->sheetWidth;
	font->mInvCellHeight  = (f32)header->cellHeight / header->sheetHeight;
}

/**
 * @note Address: 0x802B3EFC
 * @note Size: 0x2EC
 */
extern "C" CTexObj* GetFontGlyphTexture(CFont* font, const char* str, u32 arg3, s32* width)
{
	void* image;
	s32 x, y, advance;

	if (sFontPtr.resource == NULL && sFontPtr.header == NULL) {
		OSFontHeader header;
		header.cellWidth   = 0;
		header.cellHeight  = 0;
		header.sheetFormat = 0;
		header.sheetWidth  = 0;
		header.sheetHeight = 0;
		SetupFontImage(font, NULL, &header);
		x = y = advance = 0;
		OSPanic("cfont.cpp", 0x112, "");
	} else if (sFontEnv.resourcePath != NULL && arg3 == 0) {
		u16 code = (u8)str[0];
		if ((code >= 0x81 && code <= 0x9F) || (code >= 0xE0 && code <= 0xFC)) {
			code = (code << 8) | (u8)str[1];
		}
		SWiiFontResource* res = &sFontResource;
		SGlyphMetrics glyph;
		res->mParser.GetGlyphMetrics(&glyph, code);
		res->mHeader.sheetWidth  = glyph.width;
		res->mHeader.sheetHeight = glyph.height;
		res->mHeader.sheetFormat = glyph.format;
		res->mHeader.cellWidth   = res->mParser.Unk07();
		res->mHeader.cellHeight  = res->mParser.Unk06();
		SetupFontImage(font, glyph.image, &res->mHeader);
		y       = glyph.y;
		x       = glyph.x;
		advance = (s32)((f32)glyph.advance * (24.0f / res->mHeader.cellWidth));
	} else if (sFontEnv.useSingleByteFont != FALSE) {
		image = (void*)gSingleByteFontAtlas;
		SetupFontImage(font, image, sFontPtr.header);
		x       = gSingleByteGlyphMetrics[(u8)str[0]][0];
		y       = gSingleByteGlyphMetrics[(u8)str[0]][1];
		advance = gSingleByteGlyphMetrics[(u8)str[0]][2];
	} else {
		OSGetFontTexture(str, &image, &x, &y, &advance);
		SetupFontImage(font, image, sFontPtr.header);
	}

	font->mTex.Init(&font->mDesc, 0);
	PSMTXTrans(font->mTex.mTexMtx.mMtx[0], font->mInvSheetWidth * x, font->mInvSheetHeight * y, 0.0f);
	font->mTex.mTexMtx.mDirty = TRUE;
	PSMTXScale(font->mTex.mTexMtx.mMtx[2], font->mInvCellWidth, font->mInvCellHeight, 1.0f);
	font->mTex.mTexMtx.mDirty = TRUE;
	if (width != NULL) {
		*width = advance;
	}
	return &font->mTex;
}

/**
 * @note Address: 0x802B41EC
 * @note Size: 0xC8
 */
extern "C" s8 HasFontGlyph(const char* str)
{
	if (sFontResource.mFile.IsLoaded()) {
		u16 code = (u8)str[0];
		if ((code >= 0x81 && code <= 0x9F) || (code >= 0xE0 && code <= 0xFC)) {
			code = (code << 8) | (u8)str[1];
		}
		SGlyphIndex idx = sFontResource.mParser.GetGlyphIndex(code);
		return (s8)(idx.glyph >> 8);
	}
	return 0;
}

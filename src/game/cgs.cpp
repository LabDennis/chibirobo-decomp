#include "game/cgame.h"
#include "game/cgsdrawstate.h"
#include "game/ccamera.h"
#include "Version.h"
#include <stdio.h>
#include <stdarg.h>
extern "C" {
#include <dolphin/gx/gx.h>
#include <dolphin/gx/gxfifo.h>
}

// Vertex-list declarations for the HSD GX interface.
typedef struct _GXVtxDescList {
	GXAttr attr; // _00
	u32 type;    // _04
} GXVtxDescList;
typedef struct _GXVtxAttrFmtList {
	GXAttr attr;  // _00
	u32 cnt;      // _04
	u32 compType; // _08
	u8 frac;      // _0C
} GXVtxAttrFmtList;

extern "C" {
void GXSetVtxAttrFmtv(u32 fmt, GXVtxAttrFmtList* list);
void GXSetVtxDescv(GXVtxDescList* list);
}

extern "C" {
extern GXRenderModeObj GXNtsc240Ds;
extern GXRenderModeObj GXNtsc240DsAa;
extern GXRenderModeObj GXNtsc240Int;
extern GXRenderModeObj GXNtsc240IntAa;
extern GXRenderModeObj GXNtsc480IntDf;
extern GXRenderModeObj GXNtsc480Int;
extern GXRenderModeObj GXNtsc480IntAa;
extern GXRenderModeObj GXNtsc480Prog;
extern GXRenderModeObj GXNtsc480ProgSoft;
extern GXRenderModeObj GXNtsc480ProgAa;
extern GXRenderModeObj GXMpal240Ds;
extern GXRenderModeObj GXMpal240DsAa;
extern GXRenderModeObj GXMpal240Int;
extern GXRenderModeObj GXMpal240IntAa;
extern GXRenderModeObj GXMpal480IntDf;
extern GXRenderModeObj GXMpal480Int;
extern GXRenderModeObj GXMpal480IntAa;
extern GXRenderModeObj GXPal264Ds;
extern GXRenderModeObj GXPal264DsAa;
extern GXRenderModeObj GXPal264Int;
extern GXRenderModeObj GXPal264IntAa;
extern GXRenderModeObj GXPal528IntDf;
extern GXRenderModeObj GXPal528Int;
extern GXRenderModeObj GXPal524IntAa;
extern GXRenderModeObj GXEurgb60Hz240Ds;
extern GXRenderModeObj GXEurgb60Hz240DsAa;
extern GXRenderModeObj GXEurgb60Hz240Int;
extern GXRenderModeObj GXEurgb60Hz240IntAa;
extern GXRenderModeObj GXEurgb60Hz480IntDf;
extern GXRenderModeObj GXEurgb60Hz480Int;
extern GXRenderModeObj GXEurgb60Hz480IntAa;
}

// the render modes the screen can switch to
GXRenderModeObj* gVideoRenderModes[31] = {
	&GXNtsc240Ds,         &GXNtsc240DsAa,      &GXNtsc240Int,      &GXNtsc240IntAa,      &GXNtsc480IntDf,      &GXNtsc480Int,
	&GXNtsc480IntAa,      &GXNtsc480Prog,      &GXNtsc480ProgSoft, &GXNtsc480ProgAa,     &GXMpal240Ds,         &GXMpal240DsAa,
	&GXMpal240Int,        &GXMpal240IntAa,     &GXMpal480IntDf,    &GXMpal480Int,        &GXMpal480IntAa,      &GXPal264Ds,
	&GXPal264DsAa,        &GXPal264Int,        &GXPal264IntAa,     &GXPal528IntDf,       &GXPal528Int,         &GXPal524IntAa,
	&GXEurgb60Hz240Ds,    &GXEurgb60Hz240DsAa, &GXEurgb60Hz240Int, &GXEurgb60Hz240IntAa, &GXEurgb60Hz480IntDf, &GXEurgb60Hz480Int,
	&GXEurgb60Hz480IntAa,
};

// per glyph the stroke segments it draws, each byte pair a from,to point with x in the high
// nibble and y (0 at the glyph's top) in the low one, 0xFF ends the row. rows 0-9 are the
// digits, 10-35 the letters and 36-46 the punctuation DrawGsStrokeText looks up
u8 gGlyphStrokeTable[47][13] = {
	{ 0x00, 0x60, 0x60, 0x66, 0x66, 0x06, 0x06, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x63, 0x63, 0x03, 0x03, 0x06, 0x06, 0x66, 0xFF, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x66, 0x66, 0x06, 0x13, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x03, 0x03, 0x63, 0x40, 0x46, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x03, 0x03, 0x63, 0x63, 0x66, 0x66, 0x06, 0xFF, 0x00, 0x00 },
	{ 0x00, 0x06, 0x06, 0x66, 0x66, 0x63, 0x63, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x62, 0x62, 0x43, 0x43, 0x46, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x66, 0x66, 0x06, 0x06, 0x00, 0x03, 0x63, 0xFF, 0x00, 0x00 },
	{ 0x63, 0x03, 0x03, 0x00, 0x00, 0x60, 0x60, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x30, 0x30, 0x66, 0x14, 0x54, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x40, 0x40, 0x61, 0x61, 0x66, 0x66, 0x06, 0x06, 0x00, 0x03, 0x63, 0xFF },
	{ 0x60, 0x20, 0x20, 0x03, 0x03, 0x26, 0x26, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x30, 0x30, 0x63, 0x63, 0x36, 0x36, 0x06, 0x10, 0x16, 0xFF, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x06, 0x06, 0x66, 0x03, 0x53, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x06, 0x02, 0x42, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x30, 0x30, 0x02, 0x02, 0x06, 0x06, 0x66, 0x66, 0x63, 0x63, 0x33, 0xFF },
	{ 0x00, 0x06, 0x60, 0x66, 0x03, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x10, 0x50, 0x16, 0x56, 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x50, 0x56, 0x56, 0x16, 0x16, 0x14, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x06, 0x60, 0x03, 0x03, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x06, 0x06, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x00, 0x00, 0x33, 0x33, 0x60, 0x60, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x00, 0x00, 0x66, 0x66, 0x60, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x30, 0x63, 0x63, 0x36, 0x36, 0x03, 0x03, 0x30, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x00, 0x00, 0x50, 0x50, 0x61, 0x61, 0x62, 0x62, 0x53, 0x53, 0x03, 0xFF },
	{ 0x06, 0x02, 0x02, 0x60, 0x60, 0x64, 0x64, 0x06, 0x24, 0x46, 0x46, 0x66, 0xFF },
	{ 0x06, 0x00, 0x00, 0x60, 0x60, 0x62, 0x62, 0x02, 0x32, 0x66, 0xFF, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x66, 0x66, 0x06, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x06, 0x06, 0x66, 0x66, 0x60, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x02, 0x02, 0x36, 0x36, 0x62, 0x62, 0x60, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x16, 0x16, 0x56, 0x56, 0x60, 0x32, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x66, 0x60, 0x06, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x32, 0x32, 0x60, 0x32, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x06, 0x06, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x03, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x03, 0x22, 0x22, 0x44, 0x44, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x63, 0x63, 0x06, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x50, 0x16, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x40, 0x22, 0x22, 0x24, 0x24, 0x46, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x20, 0x42, 0x42, 0x44, 0x44, 0x26, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x44, 0x26, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x03, 0x03, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x30, 0x36, 0x03, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x61, 0x01, 0x01, 0x65, 0x65, 0x05, 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00 },
};

// one character out of a text string, \C and \B followed by eight hex digits set the text
// and background colors and give no character
struct TextGlyph {
	char mStr[4]; // _00, one or two bytes and a terminator
	u32 mColor;   // _04
	u32 mBgColor; // _08
};

// fills glyph with the next character, returns where the one after starts or null at the end
static inline const char* GetGlyph(CGs* gs, const char* text, TextGlyph* glyph)
{
	switch (*text) {
	case '\0':
		return NULL;
	case '\n':
		glyph->mStr[0] = '\n';
		break;
	case '\\':
		text++;
		switch (*text) {
		case 'C':
		case 'c':
			glyph->mColor = ParseHexDigits(text + 1, 8);
			text += 8;
#if !defined(VERSION_JP)
			glyph->mStr[0] = '\0';
#endif
			break;
		case 'B':
		case 'b':
			glyph->mBgColor = ParseHexDigits(text + 1, 8);
			text += 8;
#if !defined(VERSION_JP)
			glyph->mStr[0] = '\0';
#endif
			break;
		default:
#if !defined(VERSION_JP)
			glyph->mStr[0] = *text;
			glyph->mStr[1] = '\0';
#endif
			break;
		}
#if defined(VERSION_JP)
		glyph->mStr[0] = '\0';
#endif
		break;
	default:
#if defined(VERSION_JP)
		if (IsSjisLead(*text)) {
#else
		if (gs->mUseShiftJis && (IsSjisLead(*text))) {
#endif
			glyph->mStr[0] = *text;
			glyph->mStr[1] = *++text;
		} else {
			glyph->mStr[0] = *text;
			glyph->mStr[1] = '\0';
		}
		break;
	}
	return text + 1;
}

static inline void DebugVertex(f32 x, f32 y, u32 color)
{
	GXPosition3f32(x, y, 0.0f);
	*reinterpret_cast<volatile u32*>(GX_FIFO_ADDR) = color;
}

// a stroke point is on a grid of 0.11 steps inside the character cell
static inline f32 StrokePos(s32 n)
{
	f32 pos = 0.11f * n;
	return 0.2f + pos;
}

// one character's sprite and its background tile, the text draw keeps 256 of them
struct TextChar {
	CTexObj mTex;    // _00
	CSprite mSprite; // _100
	CTile mBg;       // _288
	u32 mHasBg;      // _40C
};

// SetGsTevMode, inlined
static inline void SetTevMode(CGsTevStage* stage, s32 mode)
{
	if (stage->flags != 0) {
		stage->flags = 0;
		stage->Invalidate();
	}
	if (stage->u.tevop.tevmode != mode) {
		stage->u.tevop.tevmode = mode;
		stage->Invalidate();
	}
}

static inline void SetBillboard(CPrimitive* prim, BOOL on)
{
	if (on) {
		prim->mBillboard = 1;
	} else {
		prim->mBillboard = 0;
	}
	prim->mDirty = TRUE;
}

// where the quad sits in its own unit space and its four corner colors
static inline void SetCenter(CSprite* sprite, f32 x, f32 y, f32 z)
{
	sprite->mCenter.x = x;
	sprite->mCenter.y = y;
	sprite->mCenter.z = z;
}

static inline void SetCenter(CTile* tile, f32 x, f32 y, f32 z)
{
	tile->mCenter.x = x;
	tile->mCenter.y = y;
	tile->mCenter.z = z;
}

static inline void SetColor(CSprite* sprite, u32 color)
{
	sprite->mColor0 = sprite->mColor1 = sprite->mColor2 = sprite->mColor3 = color;
}

static inline void SetColor(CTile* tile, u32 color)
{
	tile->mColor0 = tile->mColor1 = tile->mColor2 = tile->mColor3 = color;
}

/**
 * @note Address: 0x80196C0C
 * @note Size: 0x1C
 */
#if !defined(VERSION_JP)
extern "C" void SetGsFixedGlyphWidthScale(CGs* gs, f32 width)
{
	if (gs->mUseShiftJis) {
		gs->mShiftJisWidthScale = width;
	} else {
		gs->mSingleByteWidthScale = width;
	}
}
#endif

static f32 GetTextWidth(CGs* gs, const char* text, BOOL proportional, f32 size)
{
	CFont font;
	s32 charWidth;
#if !defined(VERSION_JP)
	const f32 halfSize = 0.5f * size;
#endif
	f32 lineWidth = 0.0f;
	f32 maxWidth  = 0.0f;
	TextGlyph glyph;
	const char* p = text;

	while ((p = GetGlyph(gs, p, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\0') {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			lineWidth = 0.0f;
			continue;
		}
		GetFontGlyphTexture(&font, glyph.mStr, &charWidth);
		if (size > 0.0f) {
			if (proportional) {
				lineWidth += charWidth * size / 24.0f;
#if defined(VERSION_JP)
			} else if (IsSjisLead(glyph.mStr[0])) {
				lineWidth += size;
			} else {
				lineWidth += size * 0.5f;
			}
#else
			} else if (gs->mUseShiftJis) {
				if (IsSjisLead(glyph.mStr[0])) {
					lineWidth += size * gs->mShiftJisWidthScale;
				} else {
					lineWidth += halfSize * gs->mShiftJisWidthScale;
				}
			} else {
				lineWidth += halfSize * gs->mSingleByteWidthScale;
			}
#endif
		} else {
			lineWidth += charWidth;
		}
		if (lineWidth > maxWidth) {
			maxWidth = lineWidth;
		}
	}
	return maxWidth;
}
/**
 * @note Address: 0x801968BC
 * @note Size: 0x350
 */
extern "C" f32 MeasureGameTextWidth(CGs* gs, const char* text, BOOL proportional, f32 size)
{
	return GetTextWidth(gs, text, proportional, size);
}

/**
 * @note Address: 0x80196708
 * @note Size: 0x1B4
 */
// how tall the text is, one line height per line
extern "C" f32 MeasureGameTextHeight(CGs* gs, const char* text, f32 size)
{
	TextGlyph glyph;
	f32 height = size > 0.0f ? size : 24.0f;
	while ((text = GetGlyph(gs, text, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\n') {
			height += size > 0.0f ? size : 24.0f;
		}
	}
	return height;
}
/**
 * @note Address: 0x80196610
 * @note Size: 0xF8
 */
// same, with each character as wide as its glyph
extern "C" void DrawGsProportionalTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...)
{
	char buf[256];
	va_list args;
	va_start(args, height);
	vsprintf(buf, format, args);
	DrawGsText(gs, color, TRUE, buf, x, y, z, size, height);
}

/**
 * @note Address: 0x80196518
 * @note Size: 0xF8
 */
// printf into a buffer and draw it with fixed width characters
extern "C" void DrawGsFixedWidthTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...)
{
	char buf[256];
	va_list args;
	va_start(args, height);
	vsprintf(buf, format, args);
	DrawGsText(gs, color, FALSE, buf, x, y, z, size, height);
}

/**
 * @note Address: 0x801950C8
 * @note Size: 0x133C
 */
// draws text with one sprite per character and a background tile behind each when the
// background color isn't zero
// NONMATCHING: japan's build hoists size * 0.5f out of the loop where retail
// fuses it into the curX accumulate; float regalloc differs throughout.
extern "C" void DrawGsText(CGs* gs, u32 color, BOOL proportional, const char* text, f32 x, f32 y, f32 z, f32 size, f32 height)
{
	f32 offsX;
	f32 curX = x;
	f32 curY = y;
	TextGlyph glyph;
	s32 i;

	// an ortho camera has y growing down the screen, so the line step flips
	f32 lineHeight = gs->mCamera->projection_type == PROJ_ORTHO ? -height : height;
	glyph.mColor   = color;
	glyph.mBgColor = 0;
	static TextChar sGsTextChars[256];
	static TextChar* sCurrentTextChar;

	f32 textWidth = GetTextWidth(gs, text, proportional, size);

	f32 centerX = x + textWidth / 2.0f;
	f32 centerY = y - lineHeight;
	f32 scaleX  = size / 2.0f;
	f32 scaleY  = lineHeight / 2.0f;
	s32 count   = 0;
	while ((text = GetGlyph(gs, text, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\0') {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			curX = x;
			curY += lineHeight;
			continue;
		}

		sCurrentTextChar               = &sGsTextChars[count];
		sCurrentTextChar->mSprite.mTex = &sCurrentTextChar->mTex;
		s32 charWidth                  = sCurrentTextChar->mSprite.mTex->SetChar(glyph.mStr);
		BOOL wide                      = IsSjisLead(glyph.mStr[0]);
		f32 advance;
		if (proportional) {
			advance = charWidth * (size / 24.0f);
#if defined(VERSION_JP)
		} else if (wide) {
			advance = size;
		} else {
			advance = size * 0.5f;
		}
#else
		} else if (gs->mUseShiftJis) {
			if (wide) {
				advance = size * gs->mShiftJisWidthScale;
			} else {
				advance = size * (0.5f * gs->mShiftJisWidthScale);
			}
		} else {
			advance = size * (0.5f * gs->mSingleByteWidthScale);
		}
#endif

		offsX = curX - x;
		f32 u = 2.0f * (offsX / size) + 1.0f - textWidth / size;
		f32 v = 2.0f * (-(curY - y) / lineHeight) + 1.0f;
#if defined(VERSION_JP)
		if (!proportional && wide && charWidth < size) {
			u += (size - charWidth) * 0.5f / size;
		}
#else
		if (gs->mUseShiftJis) {
			// with fixed widths a character narrower than its cell gets centered in it
			if (!proportional && wide && charWidth < size) {
				u += (size - charWidth) / 2.0f / size;
			}
		} else if (!proportional) {
			u += (size - charWidth) / 2.0f / size;
		}
#endif

		sCurrentTextChar->mSprite.SetTrans(centerX, centerY, z);
		sCurrentTextChar->mSprite.SetScale(scaleX, scaleY, 1.0f);
#if defined(VERSION_JP)
		sCurrentTextChar->mSprite.SetRot(0.0f, 0.0f, 0.0f);
#else
		sCurrentTextChar->mSprite.SetRot(gs->mTextRot.x, gs->mTextRot.y, gs->mTextRot.z);
#endif
		SetCenter(&sCurrentTextChar->mSprite, u, v, 0.0f);
		SetColor(&sCurrentTextChar->mSprite, glyph.mColor);
		SetBillboard(&sCurrentTextChar->mSprite, gs->mBillboardEnabled);

		sCurrentTextChar->mHasBg = glyph.mBgColor != 0;
		if (sCurrentTextChar->mHasBg) {
			sCurrentTextChar->mBg.SetTrans(centerX, centerY, z);
			sCurrentTextChar->mBg.SetScale(advance / 2.0f, scaleY, 1.0f);
#if defined(VERSION_JP)
			sCurrentTextChar->mBg.SetRot(0.0f, 0.0f, 0.0f);
#else
			sCurrentTextChar->mBg.SetRot(gs->mTextRot.x, gs->mTextRot.y, gs->mTextRot.z);
#endif
			SetCenter(&sCurrentTextChar->mBg, 2.0f * (offsX / advance) + 1.0f - textWidth / advance, v, 0.0f);
			SetColor(&sCurrentTextChar->mBg, glyph.mBgColor);
			SetBillboard(&sCurrentTextChar->mBg, gs->mBillboardEnabled);
		}
		curX += advance;
		count++;
	}

	// backgrounds first, flat color
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		SetTevMode(gs->mCurTevStage, GX_PASSCLR);
		gs->mCurTevStage->SetNumTexGens(0);
	}
	SetupDrawState(gs);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		for (i = 0; i < count; i++) {
			if (sGsTextChars[i].mHasBg) {
				sGsTextChars[i].mBg.DrawLit(gs->mCamera->mViewMtx, sGsTextChars[i].mBg.mColor0, sGsTextChars[i].mBg.mColor1,
				                            sGsTextChars[i].mBg.mColor2, sGsTextChars[i].mBg.mColor3);
			}
		}
	} else {
		for (i = 0; i < count; i++) {
			if (sGsTextChars[i].mHasBg) {
				sGsTextChars[i].mBg.Draw(gs->mCamera->mViewMtx, sGsTextChars[i].mBg.mColor0, sGsTextChars[i].mBg.mColor1,
				                         sGsTextChars[i].mBg.mColor2, sGsTextChars[i].mBg.mColor3);
			}
		}
	}

	// then the characters, textured
	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		SetTevMode(gs->mCurTevStage, GX_MODULATE);
		gs->mCurTevStage->SetNumTexGens(1);
		gs->mCurTevStage->SetTexGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
	}
	SetupDrawState(gs);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		for (i = 0; i < count; i++) {
			sGsTextChars[i].mSprite.DrawLit(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, sGsTextChars[i].mSprite.mColor0,
			                                sGsTextChars[i].mSprite.mColor1, sGsTextChars[i].mSprite.mColor2,
			                                sGsTextChars[i].mSprite.mColor3);
		}
	} else {
		for (i = 0; i < count; i++) {
			sGsTextChars[i].mSprite.Draw(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, sGsTextChars[i].mSprite.mColor0,
			                             sGsTextChars[i].mSprite.mColor1, sGsTextChars[i].mSprite.mColor2, sGsTextChars[i].mSprite.mColor3);
		}
	}

	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		SetTevMode(gs->mCurTevStage, GX_PASSCLR);
		gs->mCurTevStage->SetNumTexGens(0);
	}
}
/**
 * @note Address: 0x80194FD4
 * @note Size: 0xF4
 */
extern "C" void DrawGsStrokeTextFormat(CGs* gs, u32 color, const char* fmt, f32 x, f32 y, f32 z, f32 w, f32 h, ...)
{
	char buf[256];
	va_list args;
	va_start(args, h);
	vsprintf(buf, fmt, args);
	DrawGsStrokeText(gs, color, buf, x, y, z, w, h);
}

static f32 MeasureStrokeTextWidth(CGs* gs, const char* text)
{
	f32 lineWidth = 0.0f;
	f32 maxWidth  = 0.0f;
	const char* p = text;
	TextGlyph glyph;
	while ((p = GetGlyph(gs, p, &glyph)) != NULL) {
		if (glyph.mStr[0] != '\0') {
			if (glyph.mStr[0] == '\n') {
				lineWidth = 0.0f;
			} else {
				lineWidth += 1.0f;
				if (lineWidth > maxWidth) {
					maxWidth = lineWidth;
				}
			}
		}
	}
	return maxWidth;
}
/**
 * @note Address: 0x801943FC
 * @note Size: 0xBD8
 */
extern "C" void DrawGsStrokeText(CGs* gs, u32 color, const char* text, f32 x, f32 y, f32 z, f32 w, f32 h)
{
	if (gs->mCurRenderMode == &gs->mRenderMode) {
		gs->mRenderMode.mLineWidth    = 12;
		gs->mRenderMode.mLineTexOffs  = 5;
		gs->mRenderMode.mLineDirty    = TRUE;
		gs->mRenderMode.mPointSize    = 24;
		gs->mRenderMode.mPointTexOffs = 5;
		gs->mRenderMode.mPointDirty   = TRUE;
	}
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);

	f32 width = MeasureStrokeTextWidth(gs, text);

	Mtx trans;
#if defined(VERSION_JP)
	Mtx scale;
	Mtx mtx;
	if (gs->mCamera->projection_type == PROJ_ORTHO) {
		PSMTXTrans(trans, x + width * w * 0.5f, y, z);
		PSMTXScale(scale, w, h, 1.0f);
	} else {
		PSMTXTrans(trans, x + width * w * 0.5f, y, z);
		PSMTXScale(scale, w, -h, 1.0f);
	}
	PSMTXConcat(trans, scale, mtx);
#else
	Mtx rotX;
	Mtx rotY;
	Mtx rotZ;
	Mtx scale;
	Mtx mtx;
	PSMTXTrans(trans, x + width * w * 0.5f, y, z);
	PSMTXRotRad(rotX, 'x', gs->mTextRot.x);
	PSMTXRotRad(rotY, 'y', gs->mTextRot.y);
	PSMTXRotRad(rotZ, 'z', gs->mTextRot.z);
	if (gs->mCamera->projection_type == PROJ_ORTHO) {
		PSMTXScale(scale, w, h, 1.0f);
	} else {
		PSMTXScale(scale, w, -h, 1.0f);
	}
	PSMTXConcat(trans, rotX, mtx);
	PSMTXConcat(mtx, rotY, mtx);
	PSMTXConcat(mtx, rotZ, mtx);
	PSMTXConcat(mtx, scale, mtx);
#endif
	PSMTXConcat(gs->mCamera->mViewMtx, mtx, mtx);

	f32 startX;
	f32 curY;
	f32 unit;
	if (gs->mBillboardEnabled) {
		BuildBillboardMatrixFromUp(mtx, mtx);
		startX = width * 0.5f;
		curY   = 0.0f;
		unit   = -1.0f;
	} else {
		startX = -width * 0.5f;
		curY   = 0.0f;
		unit   = 1.0f;
	}
	f32 curX = startX;
	GXLoadPosMtxImm(mtx, 0);
	GXSetCurrentMtx(0);

	// position xyz f32 and color rgba8, both direct
	GXVtxAttrFmtList fmts[] = {
		{ GX_VA_POS, 1, 4, 0 },
		{ GX_VA_CLR0, 1, 5, 0 },
		{ GX_VA_NULL, 0, 0, 0 },
	};
	GXVtxDescList descs[] = {
		{ GX_VA_POS, 1 },
		{ GX_VA_CLR0, 1 },
		{ GX_VA_NULL, 0 },
	};
	GXClearVtxDesc();
	GXSetVtxAttrFmtv(0, fmts);
	GXSetVtxDescv(descs);

	// second pass draws, each character is one unit wide in the scaled space
	TextGlyph glyph;
	glyph.mColor   = color;
	glyph.mBgColor = 0;
	while ((text = GetGlyph(gs, text, &glyph)) != NULL) {
		if (glyph.mStr[0] == 0) {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			curX = startX;
			curY += 1.0f;
			continue;
		}
		if (glyph.mBgColor != 0) {
			GXBegin(GX_QUADS, 0, 4);
			DebugVertex(curX, curY, glyph.mBgColor);
			DebugVertex(curX + unit, curY, glyph.mBgColor);
			DebugVertex(curX + unit, 1.0f + curY, glyph.mBgColor);
			DebugVertex(curX, 1.0f + curY, glyph.mBgColor);
		}
		s32 index = -1;
		switch (glyph.mStr[0]) {
		// Preserve the exact subpixel positions used by the punctuation glyphs.
		case '.':
			GXBegin(GX_POINTS, 0, 1);
			DebugVertex(curX + 0.25f * unit, 0.84999996f + curY, glyph.mColor);
			break;
		case ':':
			GXBegin(GX_POINTS, 0, 2);
			DebugVertex(curX + 0.35000002f * unit, 0.35000002f + curY, glyph.mColor);
			DebugVertex(curX + 0.35000002f * unit, 0.75f + curY, glyph.mColor);
			break;
		case '%':
			GXBegin(GX_POINTS, 0, 2);
			DebugVertex(curX + 0.35000002f * unit, 0.35000002f + curY, glyph.mColor);
			DebugVertex(curX + 0.75f * unit, 0.75f + curY, glyph.mColor);
			GXBegin(GX_LINES, 0, 2);
			DebugVertex(curX + 0.95f * unit, 0.15f + curY, glyph.mColor);
			DebugVertex(curX + 0.15f * unit, 0.95f + curY, glyph.mColor);
			break;
		case '-':
			index = 36;
			break;
		case '_':
			index = 37;
			break;
		case '~':
			index = 38;
			break;
		case '>':
			index = 39;
			break;
		case '/':
			index = 40;
			break;
		case '(':
			index = 41;
			break;
		case ')':
			index = 42;
			break;
		case ',':
			index = 43;
			break;
		case '<':
			index = 44;
			break;
		case '+':
			index = 45;
			break;
		case '$':
			index = 46;
			break;
		default:
			if (glyph.mStr[0] >= '0' && glyph.mStr[0] <= '9') {
				index = glyph.mStr[0] - '0';
			} else if (glyph.mStr[0] >= 'A' && glyph.mStr[0] <= 'Z') {
				index = glyph.mStr[0] - 'A' + 10;
			} else if (glyph.mStr[0] >= 'a' && glyph.mStr[0] <= 'z') {
				index = glyph.mStr[0] - 'a' + 10;
			}
			break;
		}
		if (index >= 0) {
			u8* stroke = gGlyphStrokeTable[index];
			// 41 is way past the 13 bytes a glyph has, the 0xFF always comes first
			for (s32 i = 0; i < 41 && *stroke != 0xFF;) {
				u8 from = stroke[0];
				u8 to   = stroke[1];
				i += 2;
				stroke += 2;
				GXBegin(GX_LINES, 0, 2);
				DebugVertex(curX + unit * StrokePos(from >> 4), curY + StrokePos(from & 0xF), glyph.mColor);
				DebugVertex(curX + unit * StrokePos(to >> 4), curY + StrokePos(to & 0xF), glyph.mColor);
			}
		}
		curX += unit;
	}
}

// The Wii's text drawing unit, .text 0x802AD310-0x802AEBB0: the GameCube
// cgs.cpp pair (wii_gc_pairs.txt) plus the Wii's own helpers in between.
//
// NONMATCHING: five of the unit's nine functions match at 100 %, four stay
// fuzzy: GetGlyph and MeasureGameTextWidth differ only in callee-saved
// register choice and the IsSjisLead bool materialization, DrawGsText and
// DrawGsStrokeText additionally diverge in float scheduling and stack slot
// order — no behavioural difference found in any of them.

#include "game/wii/cgs.h"
#include "game/cfont.h"
#include "game/cprimitive.h"
#include "game/wii/cgame.h"
#include <stdarg.h>
#include <stdio.h>

// sends whatever render, tev and channel state changed since the last draw
static inline void SetupDrawState(CGs* gs)
{
	if (gs->mCurRenderState != NULL) {
		gs->mCurRenderState->Apply();
	}
	if (gs->mCurTevStage != NULL) {
		gs->mCurTevStage->Apply();
	}
	if (gs->mCurChannel != NULL) {
		gs->mCurChannel->Apply();
	}
}

// one character out of a text string, \C and \B followed by eight hex digits set the text
// and background colors and give no character
struct TextGlyph {
	char mStr[4]; // _00, one or two bytes and a terminator
	u32 mColor;   // _04
	u32 mBgColor; // _08
};

// one character's sprite and its background tile, Init allocates 256 of them
struct TextChar {
	CTexObj mTex;    // _000
	CSprite mSprite; // _104
	CTile mBg;       // _28C
	u32 mHasBg;      // _410
};
STATIC_ASSERT(sizeof(TextChar) == 0x414);

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

// NONMATCHING: same switch and control flow; retail keeps gs in r30 and the
// glyph pointer in r31 while MWCC swaps them, and it orders the escape-branch
// compares slightly differently.
/**
 * @note Address: 0x802AD1C0
 * @note Size: 0x150
 */
// fills glyph with the next character, returns where the one after starts or null at the end
extern "C" {
const char* GetGlyph(CGs* gs, const char* text, TextGlyph* glyph)
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
			glyph->mStr[0] = '\0';
			break;
		case 'B':
		case 'b':
			glyph->mBgColor = ParseHexDigits(text + 1, 8);
			text += 8;
			glyph->mStr[0] = '\0';
			break;
		default:
			glyph->mStr[0] = *text;
			glyph->mStr[1] = '\0';
			break;
		}
		break;
	default:
		if (gs->mUseShiftJis && IsSjisLead(*text)) {
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
}

/**
 * @note Address: 0x802AD310
 * @note Size: 0x220
 */
// the widest line of the text, in the units the draw uses
extern "C" f32 MeasureGameTextWidth(CGs* gs, const char* text, BOOL proportional, f32 size)
{
	CFont font;
	s32 charWidth;
	f32 width[2] = { 0.0f, 0.0f };
	TextGlyph glyph;
	const char* p = text;

	while ((p = GetGlyph(gs, p, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\0') {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			width[0] = 0.0f;
			continue;
		}
		GetFontGlyphTexture(&font, glyph.mStr, gs->mTextFont, &charWidth);
		if (size > 0.0f) {
			if (proportional) {
				width[0] += charWidth * (size / 24.0f);
			} else if (gs->mUseShiftJis) {
				if (IsSjisLead(glyph.mStr[0])) {
					width[0] += size * gs->mShiftJisWidthScale;
				} else {
					width[0] += 0.5f * size * gs->mShiftJisWidthScale;
				}
			} else {
				width[0] += 0.5f * size * gs->mSingleByteWidthScale;
			}
		} else {
			width[0] += charWidth;
		}
		if (width[0] > width[1]) {
			width[1] = width[0];
		}
	}
	return width[1];
}

/**
 * @note Address: 0x802AD530
 * @note Size: 0xBC
 */
// one line plus one per newline, a non-positive size falls back to the default cell
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
 * @note Address: 0x802AD5EC
 * @note Size: 0xF8
 */
// printf into a buffer and draw it with each character as wide as its glyph
extern "C" void DrawGsProportionalTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...)
{
	char buf[256];
	va_list args;
	va_start(args, height);
	vsprintf(buf, format, args);
	DrawGsText(gs, color, TRUE, buf, x, y, z, size, height);
}

/**
 * @note Address: 0x802AD6E4
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
 * @note Address: 0x802AD7DC
 * @note Size: 0x828
 */
// draws text with one sprite per character and a background tile behind each when the
// background color isn't zero
extern "C" void DrawGsText(CGs* gs, u32 color, BOOL proportional, const char* text, f32 x, f32 y, f32 z, f32 size, f32 height)
{
	f32 offsX;
	f32 curX = x;
	f32 curY = y;
	TextGlyph glyph;
	s32 i;

	// an ortho camera has y growing down the screen, so the line step flips
	f32 lineHeight      = gs->mCamera->mTop < gs->mCamera->mBottom ? -height : height;
	glyph.mColor        = color;
	glyph.mBgColor      = 0;
	TextChar* textChars = gs->mTextChars;

	f32 textWidth = MeasureGameTextWidth(gs, text, proportional, size);

	f32 centerX = x + textWidth / 2.0f;
	f32 centerY = y - lineHeight;
	f32 scaleX  = size * 0.5f;
	f32 scaleY  = lineHeight * 0.5f;
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

		TextChar* chr     = &textChars[count];
		chr->mSprite.mTex = &chr->mTex;
		s32 charWidth     = chr->mSprite.mTex->SetChar(glyph.mStr, gs->mTextFont);
		u8 wide           = IsSjisLead(glyph.mStr[0]);
		f32 advance;
		if (proportional) {
			// the glyph's left bearing shifts it inside its cell
			CFont font;
			s32 bearing = HasFontGlyph(&font, glyph.mStr);
			curX += bearing * (size / 24.0f);
			charWidth -= bearing;
			advance = charWidth * (size / 24.0f);
		} else if (gs->mUseShiftJis) {
			if (wide) {
				advance = size * gs->mShiftJisWidthScale;
			} else {
				advance = size * (0.5f * gs->mShiftJisWidthScale);
			}
		} else {
			advance = size * (0.5f * gs->mSingleByteWidthScale);
		}

		offsX = curX - x;
		f32 u = 2.0f * (offsX / size) + 1.0f - textWidth / size;
		f32 v = 2.0f * (-(curY - y) / lineHeight) + 1.0f;
		if (!proportional) {
			// a character narrower than its cell gets centered in it
			u += (size - charWidth) / 2.0f / size;
		}

		chr->mSprite.SetTrans(centerX, centerY, z);
		chr->mSprite.SetScale(scaleX, scaleY, 1.0f);
		chr->mSprite.SetRot(gs->mTextRot.x, gs->mTextRot.y, gs->mTextRot.z);
		SetCenter(&chr->mSprite, u, v, 0.0f);
		SetColor(&chr->mSprite, glyph.mColor);
		SetBillboard(&chr->mSprite, gs->mBillboardEnabled);

		if (chr->mHasBg = glyph.mBgColor != 0) {
			chr->mBg.SetTrans(centerX, centerY, z);
			chr->mBg.SetScale(advance / 2.0f, scaleY, 1.0f);
			chr->mBg.SetRot(gs->mTextRot.x, gs->mTextRot.y, gs->mTextRot.z);
			SetCenter(&chr->mBg, 2.0f * (offsX / advance) + 1.0f - textWidth / advance, v, 0.0f);
			SetColor(&chr->mBg, glyph.mBgColor);
			SetBillboard(&chr->mBg, gs->mBillboardEnabled);
		}
		curX += advance;
		count++;
	}

	// backgrounds first, flat color
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		gs->mCurTevStage->SetMode(GX_PASSCLR);
		gs->mCurTevStage->SetNumTexGens(0);
	}
	SetupDrawState(gs);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].mEnable) {
		for (i = 0; i < count; i++) {
			if (textChars[i].mHasBg) {
				textChars[i].mBg.DrawLit(gs->mCamera->mViewMtx, textChars[i].mBg.mColor0, textChars[i].mBg.mColor1,
				                         textChars[i].mBg.mColor2, textChars[i].mBg.mColor3);
			}
		}
	} else {
		for (i = 0; i < count; i++) {
			if (textChars[i].mHasBg) {
				textChars[i].mBg.Draw(gs->mCamera->mViewMtx, textChars[i].mBg.mColor0, textChars[i].mBg.mColor1, textChars[i].mBg.mColor2,
				                      textChars[i].mBg.mColor3);
			}
		}
	}

	// then the characters, textured
	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		gs->mCurTevStage->SetMode(GX_MODULATE);
		gs->mCurTevStage->SetNumTexGens(1);
		gs->mCurTevStage->SetTexGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
	}
	SetupDrawState(gs);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].mEnable) {
		for (i = 0; i < count; i++) {
			textChars[i].mSprite.DrawLit(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, textChars[i].mSprite.mColor0,
			                             textChars[i].mSprite.mColor1, textChars[i].mSprite.mColor2, textChars[i].mSprite.mColor3);
		}
	} else {
		for (i = 0; i < count; i++) {
			textChars[i].mSprite.Draw(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, textChars[i].mSprite.mColor0,
			                          textChars[i].mSprite.mColor1, textChars[i].mSprite.mColor2, textChars[i].mSprite.mColor3);
		}
	}

	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		gs->mCurTevStage->SetMode(GX_PASSCLR);
		gs->mCurTevStage->SetNumTexGens(0);
	}
}

/**
 * @note Address: 0x802AE004
 * @note Size: 0xE0
 */
// the longest run of drawable characters between newlines, scaled when a size is given
extern "C" f32 MeasureStrokeTextWidth(CGs* gs, const char* text, f32 size)
{
	f32 len[2]    = { 0.0f, 0.0f };
	const char* p = text;
	TextGlyph glyph;
	while ((p = GetGlyph(gs, p, &glyph)) != NULL) {
		if (glyph.mStr[0] != '\0') {
			if (glyph.mStr[0] == '\n') {
				len[0] = 0.0f;
			} else {
				len[0] += 1.0f;
				if (len[0] > len[1]) {
					len[1] = len[0];
				}
			}
		}
	}
	return size > 0.0f ? len[1] * size : len[1];
}

/**
 * @note Address: 0x802AE0E4
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
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

static inline void DebugVertex(f32 x, f32 y, u32 color)
{
	f32 depth = 0.0f;
	GXPosition3f32(x, y, depth);
	GXColor1u32(color);
}

// a stroke point is on a grid of 0.11 steps inside the character cell
static inline f32 StrokePos(s32 n)
{
	f32 pos = 0.11f * n;
	return 0.2f + pos;
}

/**
 * @note Address: 0x802AE1D8
 * @note Size: 0x86C
 */
extern "C" void DrawGsStrokeText(CGs* gs, u32 color, const char* text, f32 x, f32 y, f32 z, f32 w, f32 h)
{
	if (gs->mCurRenderState == &gs->mRenderState) {
		gs->mRenderState.SetLineWidth(6, GX_TO_ONE);
		gs->mRenderState.SetPointSize(12, GX_TO_ONE);
	}
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);

	f32 width = MeasureStrokeTextWidth(gs, text, 0.0f);

	Mtx trans;
	Mtx rotX;
	Mtx rotY;
	Mtx rotZ;
	Mtx scale;
	Mtx mtx;
	PSMTXTrans(trans, x + width * w * 0.5f, y, z);
	PSMTXRotRad(rotX, 'x', gs->mTextRot.x);
	PSMTXRotRad(rotY, 'y', gs->mTextRot.y);
	PSMTXRotRad(rotZ, 'z', gs->mTextRot.z);
	if (gs->mCamera->mTop < gs->mCamera->mBottom) {
		PSMTXScale(scale, w, h, 1.0f);
	} else {
		PSMTXScale(scale, w, -h, 1.0f);
	}
	PSMTXConcat(trans, rotX, mtx);
	PSMTXConcat(mtx, rotY, mtx);
	PSMTXConcat(mtx, rotZ, mtx);
	PSMTXConcat(mtx, scale, mtx);
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
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};
	GXVtxDescList descs[] = {
		{ GX_VA_POS, GX_DIRECT },
		{ GX_VA_CLR0, GX_DIRECT },
		{ GX_VA_NULL, GX_NONE },
	};
	GXClearVtxDesc();
	GXSetVtxAttrFmtv(GX_VTXFMT0, fmts);
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
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			DebugVertex(curX, curY, glyph.mBgColor);
			DebugVertex(curX + unit, curY, glyph.mBgColor);
			DebugVertex(curX + unit, 1.0f + curY, glyph.mBgColor);
			DebugVertex(curX, 1.0f + curY, glyph.mBgColor);
		}
		s32 index = -1;
		switch (glyph.mStr[0]) {
		case '.':
			GXBegin(GX_POINTS, GX_VTXFMT0, 1);
			DebugVertex(curX + 0.25f * unit, 0.84999996f + curY, glyph.mColor);
			break;
		case ':':
			GXBegin(GX_POINTS, GX_VTXFMT0, 2);
			DebugVertex(curX + 0.35000002f * unit, 0.35000002f + curY, glyph.mColor);
			DebugVertex(curX + 0.35000002f * unit, 0.75f + curY, glyph.mColor);
			break;
		case '%':
			GXBegin(GX_POINTS, GX_VTXFMT0, 2);
			DebugVertex(curX + 0.35000002f * unit, 0.35000002f + curY, glyph.mColor);
			DebugVertex(curX + 0.75f * unit, 0.75f + curY, glyph.mColor);
			GXBegin(GX_LINES, GX_VTXFMT0, 2);
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
				GXBegin(GX_LINES, GX_VTXFMT0, 2);
				DebugVertex(curX + unit * StrokePos(from >> 4), curY + StrokePos(from & 0xF), glyph.mColor);
				DebugVertex(curX + unit * StrokePos(to >> 4), curY + StrokePos(to & 0xF), glyph.mColor);
			}
		}
		curX += unit;
	}
}

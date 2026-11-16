// The message system. One file, .text 0x800A1938-0x800A8164:
// - the functions before it belong to evt.cpp: __dt__9CEvtTimerFv (0x800A14A8), __dt__8CEvtVarsFv
//   (0x800A14E8), __dt__9CEvtFlagsFv (0x800A1528) and __dt__11CEvtQueueExFv (0x800A18B8) pair to
//   game/evt.cpp in wii_gc_pairs.txt, and the CCameraMemento constructor (0x800A18F8) plus the
//   array destructor (0x800A191C) it registers sit between them and this file's first function
// - its jump tables sit in .rodata from 0x802FFAF8 on: jumptable_802FFAF8 (25 entries,
//   MsgCommandLength's MSG_CODE_* values 0..24) and jumptable_802FFB5C (CMsgData_Format)
// - the GameCube pairs (wii_gc_pairs.txt) land in the reverse of the GameCube's source order:
//   CMsg__cmd_SETFONT (0x800A7ED4) is the GameCube file's first function, CMsgData_Format and
//   CMsgData_Measure (0x800A1A20, 0x800A1FE8) are among its last
//
// NONMATCHING: all 22 of the unit's functions have source now. CMsgFont_Draw is reconstructed
// against the Wii retail calls: the GameCube's XObj portrait pass became the cursor object's
// own draw with the anim, pose and head-joint height carried over, and the highlight picks a
// second color while the pointer holds a line.
// The GameCube file's VERSION_JP branches are the reconstruction base; the Wii reads its
// language flag from gGame.mLanguage (_4160), where Japanese is 0. The Wii's CMsgData flag
// byte, the u32 CMsgFont::mFlags bitfield word and the pointer-driven CMsg__cmd_QUESTION
// path are Wii-specific and verified against retail.

#include "game/wii/msg.h"
#include "game/cfont.h"
#include "game/coordinfo.h"
#include "game/evt.h"
#include "game/gmath.h"
#include "game/stagedata.h"
#include "game/ut.h"
#include "game/wii/cgame.h"
#include "game/wii/cpad.h"
#include "game/wii/cpadact.h"
#include "game/wii/csound.h"
#include "nw4r/snd/snd_BasicSound.h"
#include "Dolphin/os.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u32 msgColorTable[] = {
	0xFFFFFFFF, 0x808080FF, 0x101010FF, 0x00FFFFFF, 0x00E000FF, 0x0000E0FF, 0xFF00FFFF, 0xE00000FF, 0xFFFF00FF, 0xFF8000FF,
};

// the window tint kept in .sdata (0x805A0F40, translucent black)
GXColor gMessageWindowDefaultColor = { 0, 0, 0, 0x80 };

extern "C" void CMsgFont_Init(CMsgFont* font, u8* data, f32 x, f32 y, WiiTitleObject* anchor, s32 style, s32 align);
extern "C" void ResetMessageState(CMsgFont* font);

extern "C" {
// the unit's own functions, in retail .text order
void CMsgFont_AddChar(CMsgFont* font, const char* text); // 0x800A2E8C
void CMsgFont_Update(CMsgFont* font, u32 step);          // 0x800A34C0
void CMsgFont_Draw(CMsgFont* font, f32 lastAlpha);       // 0x800A3B5C
s32 CMsg__cmd_END(CMsg* msg);                            // 0x800A687C
s32 CMsg__cmd_QUESTION(CMsg* msg);                       // 0x800A73C0
s32 CMsg__cmd_WAIT(CMsg* msg);                           // 0x800A74C0
s32 CMsg__cmd_ANIM(CMsg* msg);                           // 0x800A7784
s32 CMsg__cmd_SFX(CMsg* msg);                            // 0x800A7A68
s32 CMsg__cmd_SETFONT(CMsg* msg, u8 command);            // 0x800A7ED4
void SetGsChannelEnabled(CGs* gs, s32 chan, u8 enable);  // 0x802AB734

// Wii-side services the GameCube file called by these names
void PlayMessageGlyphSound(XSound* sound, u32 group, f32 strength, u8 first, u8 second); // 0x8003BB08
void ClearGameSoundQueue(XSound* sound, s32 group);                                      // 0x8003BE20
void SelectPlayerEquipmentRing(WiiPlayerState* player, s32 selection);                   // 0x800D2700
void OpenPlayerYesNoMenu(WiiPlayerState* player, s32 crossMask, s32 selectedIndex);      // 0x800D2B50
void HandleOXMenuInput(WiiOXMenu* menu, CPad* pad);                                      // fn_800EF58C
extern s32 gRefreshRate;
}

extern "C" s32 ClassifyFloatBits(f32 value); // the Wii MSL's out-of-line __fpclassifyf

// the GameCube's FindObj: object commands only resolve while an event script runs. The Wii's
// ResolveEventObjectId (0x8009453C) reads the same two leading words as the GameCube CEvtData
static inline WiiTitleObject* FindObj(s16 id)
{
	return gGame.mEvtData.mScript != NULL ? (WiiTitleObject*)ResolveEventObjectId((const CEvtData*)&gGame.mEvtData, id) : NULL;
}

// read before stepping, the script can point into the message's own bytes
static inline u32 ReadMsgU8(CMsg* msg)
{
	u32 value = *msg->mData.mCursor;
	msg->mData.mCursor++;
	return value;
}

// script operands are little endian
static inline s16 ReadMsgS16(CMsg* msg)
{
	u32 lo = ReadMsgU8(msg);
	u32 hi = ReadMsgU8(msg);
	return lo | (hi << 8);
}

// the Wii keeps its own language codes: japanese is 0 (the GameCube tests for 6)
#define MsgHasSjisLead(c) (gGame.mLanguage == 0 && IsSjisLead(c))

struct MessageFormatBuffers {
	char format[128];
	char text[128];
};

static inline void CopyMessageBytes(u8*& dst, u8*& src, int N)
{
	for (int i = 0; i < N; ++i)
		*dst++ = *src++;
}

extern "C" {
extern s32 gPointerScreenWidth;
extern s32 gPointerScreenHeight;
extern s32 gScreenLeft;
extern s32 gScreenTop;
extern s32 gScreenSafeLeft;
extern s32 gScreenSafeRight;
extern s32 gScreenSafeTop;
extern s32 gScreenSafeBottom;
}

// one piece of the window frame, a texture window and where it goes
struct MsgWindowSlice {
	f32 u;      // _00
	f32 v;      // _04
	f32 scaleU; // _08
	f32 scaleV; // _0C
	f32 x;      // _10
	f32 y;      // _14
	f32 width;  // _18
	f32 height; // _1C
};

/**
 * @note Address: 0x800A1938
 * @note Size: 0xE8
 */
extern "C" s32 MsgCommandLength(u8* p)
{
	switch (*p) {
	case MSG_CODE_END:
	case MSG_CODE_NEWLINE:
	case MSG_CODE_NEWLINE_VT:
	case MSG_CODE_STAY:
		return 1;
	case MSG_CODE_SPEED:
	case MSG_CODE_WAIT:
	case MSG_CODE_VOICE:
		return 2;
	case MSG_CODE_DEFAULT:
		return 6;
	case MSG_CODE_ANIM:
		return 8;
	case MSG_CODE_SFX:
		if (p[5] == MSG_SFX_FADE_OUT || p[5] == MSG_SFX_FADE_IN) {
			return 8;
		} else if (p[5] == MSG_SFX_FADE) {
			return 9;
		}
		return 6;
	case MSG_CODE_SIZE:
	case MSG_CODE_COLOR:
	case MSG_CODE_LAYOUT:
	case MSG_CODE_ICON:
	case MSG_CODE_CENTER:
		return 2;
	case MSG_CODE_ROTATE:
	case MSG_CODE_QUESTION:
		return 3;
	case MSG_CODE_SHAKE:
	case MSG_CODE_NUM_INPUT:
		return 4;
	case MSG_CODE_RGBA:
	case MSG_CODE_SCALE:
		return 5;
	default:
		if (MsgHasSjisLead(*p)) {
			return 2;
		}
		return 1;
	}
}

/**
 * @note Address: 0x800A1A20
 * @note Size: 0x5C8
 */
extern "C" void CMsgData_Format(u8* output, u8* input, MsgFormatArgs* args)
{
	u8* src = input;
	u8* dst = output;
	MessageFormatBuffers buffers;
	BOOL done = FALSE;
	while (!done) {
		switch (*src) {
		case MSG_CODE_END:
			*dst = 0;
			done = TRUE;
			break;
		case MSG_CODE_FORMAT: { // %fmt% inline format; 'z' makes it wide, "'" groups digits
			char* out    = buffers.format;
			bool wide    = false;
			bool grouped = false;
			++src;
			for (;;) {
				if (*src == MSG_CODE_FORMAT)
					break;
				if (*src == 'z') {
					wide = true;
					++src;
					continue;
				}
				if (*src == '\'') {
					grouped = true;
					++src;
					continue;
				}
				*out++ = *src++;
			}
			*out = 0;
			++src;
			sprintf(buffers.text, buffers.format, args->values[--args->count]);
			if (grouped) {
				char separator = ',';
				switch (gGame.mLanguage) {
				case 2:
				case 3:
				case 4:
					separator = ' ';
					break;
				case 5:
					separator = '.';
					break;
				case 0:
				case 1:
				default:
					separator = ',';
					break;
				}
				s32 len = strlen(buffers.text);
				u8 lang = gGame.mLanguage;
				s32 i, count;
				for (i = 0, count = 0; i < len; ++i, ++count) {
					buffers.format[count] = buffers.text[(len - 1) - i];
					if ((i + 1) % 3 == 0 && i + 1 < len) {
						if ((lang == 2 || lang == 4 || lang == 5) && len < 5) // four digits stay ungrouped there
							continue;
						buffers.format[++count] = separator; // every third digit
					}
					buffers.format[count + 1] = 0;
				}
				// When the formatted value is empty, retail reverses the format string left in the scratch buffer.
				count = strlen(buffers.format);
				for (i = 0; i < count; ++i)
					buffers.text[i] = buffers.format[(count - 1) - i];
				buffers.text[count] = 0;
				strcpy((char*)dst, buffers.text);
			}
			if (wide && gGame.mLanguage == 0)
				strcpy((char*)dst, ConvertHalfWidthToFullWidthSJIS(buffers.text, NULL));
			else
				strcpy((char*)dst, buffers.text);
			dst += strlen((char*)dst);
			break;
		}
		case MSG_CODE_NEWLINE:
		case MSG_CODE_NEWLINE_VT:
		case MSG_CODE_STAY:
			CopyMessageBytes(dst, src, 1);
			break;
		case MSG_CODE_SPEED:
		case MSG_CODE_WAIT:
		case MSG_CODE_VOICE:
		case MSG_CODE_SIZE:
		case MSG_CODE_COLOR:
		case MSG_CODE_LAYOUT:
		case MSG_CODE_ICON:
		case MSG_CODE_CENTER:
			CopyMessageBytes(dst, src, 2);
			break;
		case MSG_CODE_ROTATE:
		case MSG_CODE_QUESTION:
			CopyMessageBytes(dst, src, 3);
			break;
		case MSG_CODE_SCALE:
			CopyMessageBytes(dst, src, 5);
			break;
		case MSG_CODE_SHAKE:
		case MSG_CODE_NUM_INPUT:
			CopyMessageBytes(dst, src, 4);
			break;
		case MSG_CODE_DEFAULT:
			CopyMessageBytes(dst, src, 6);
			break;
		case MSG_CODE_ANIM:
			CopyMessageBytes(dst, src, 8);
			break;
		case MSG_CODE_SFX:
			CopyMessageBytes(dst, src, 6);
			if (dst[-1] == MSG_SFX_FADE_OUT || dst[-1] == MSG_SFX_FADE_IN)
				CopyMessageBytes(dst, src, 2);
			else if (dst[-1] == MSG_SFX_FADE)
				CopyMessageBytes(dst, src, 3);
			break;
		case MSG_CODE_RGBA:
			CopyMessageBytes(dst, src, 5);
			break;
		default:
			if (MsgHasSjisLead(*src)) {
				CopyMessageBytes(dst, src, 2);
			} else if (*src == '$') { // '$' becomes '"'
				++src;
				*dst++ = '"';
			} else {
				*dst++ = *src++;
			}
		}
	}
}

/**
 * @note Address: 0x800A1FE8
 * @note Size: 0x3D0
 */
extern "C" void CMsgData_Measure(CMsgData* data)
{
	data->mLineCount                 = 0;
	data->mCounts[data->mLineCount]  = 0;
	data->mWidths[data->mLineCount]  = 0.0f;
	data->mHeights[data->mLineCount] = 0.0f;
	data->mStyle                     = -1;
	data->mAlignment                 = 1;
	f32 size                         = 22.0f;
	u8* p                            = data->mText;
	s32 proportional                 = TRUE;
	s32 done                         = 0;
	while (!done) {
		switch (*p) {
		case MSG_CODE_NEWLINE:
		case MSG_CODE_NEWLINE_VT:
			if (data->mHeights[data->mLineCount] == 0.0f) {
				data->mHeights[data->mLineCount] = size;
			}
			data->mLineCount++;
			data->mCounts[data->mLineCount]  = 0;
			data->mWidths[data->mLineCount]  = 0.0f;
			data->mHeights[data->mLineCount] = size;
			break;
		case MSG_CODE_END:
			if (data->mHeights[data->mLineCount] == 0.0f) {
				data->mHeights[data->mLineCount] = 22.0f;
			}
			data->mLineCount++;
			done = 1;
			break;
		case MSG_CODE_SIZE:
			size = (s32)p[1];
			if (size == 255.0f) {
				size = 22.0f;
			}
			if (size > data->mHeights[data->mLineCount]) {
				data->mHeights[data->mLineCount] = size;
			}
			break;
		case MSG_CODE_LAYOUT:
			proportional = p[1];
			break;
		case MSG_CODE_CENTER:
			data->mAlignment = p[1];
			break;
		case MSG_CODE_VOICE:
			// the voice byte doubles as the window's mode, CMsg_Open passes mStyle to CMsgFont_Init
			data->mStyle = p[1];
			break;
		case MSG_CODE_SPEED:
		case MSG_CODE_WAIT:
		case MSG_CODE_ANIM:
		case MSG_CODE_SFX:
		case MSG_CODE_DEFAULT:
		case MSG_CODE_COLOR:
		case MSG_CODE_RGBA:
		case MSG_CODE_SHAKE:
		case MSG_CODE_ROTATE:
		case MSG_CODE_SCALE:
		case MSG_CODE_NUM_INPUT:
		case MSG_CODE_QUESTION:
		case MSG_CODE_STAY:
			break;
		default:
			if (MsgHasSjisLead(*p)) {
				data->mCounts[data->mLineCount]++;
				char text[3] = "";
				text[0]      = p[0];
				text[1]      = p[1];
				data->mWidths[data->mLineCount] += MeasureGameTextWidth(&gGame.mGs, text, proportional, size);
			} else if (*p == MSG_CODE_ICON) {
				gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[120], 0);
				data->mWidths[data->mLineCount] += 24.0f * (size / 22.0f);
			} else {
				data->mCounts[data->mLineCount]++;
				char text[2] = "";
				text[0]      = p[0];
				data->mWidths[data->mLineCount] += MeasureGameTextWidth(&gGame.mGs, text, proportional, size);
			}
			break;
		}
		p += MsgCommandLength(p);
	}
	data->mWidestLine  = 0;
	data->mMaxWidth    = 0.0f;
	data->mMaxHeight   = 0.0f;
	data->mTotalHeight = 0.0f;
	for (s32 i = 0; i < data->mLineCount; i++) {
		if (data->mMaxWidth < data->mWidths[i]) {
			data->mMaxWidth   = data->mWidths[i];
			data->mWidestLine = i;
		}
		if (data->mMaxHeight < data->mHeights[i]) {
			data->mMaxHeight = data->mHeights[i];
		}
		data->mTotalHeight += data->mHeights[i];
	}
}

/**
 * @note Address: 0x800A23B8
 * @note Size: 0x16C
 */
// loads the font's two cursor objects the first time they're needed, like the GameCube's
// LoadMessageCursorObject, but the Wii keeps a second cursor (spawn 0x2C, the num input cursor)
extern "C" s32 LoadFontCursors(CMsgFont* font)
{
	s32 loaded = 0;
	if (!(font->mCursor[0].mStatus & 1)) {
		if (font->mCursor[0].vf158(CoordInfo(GetSpawnData(13)))) {
			loaded = 1;
		}
	}
	if (loaded) {
		font->mCursor[0].vf08C(3.0f, 3.0f, 3.0f);
		font->mCursor[0].mStatus |= 0x800;
		return 0;
	}
	loaded = 0;
	if (!(font->mCursor[1].mStatus & 1)) {
		if (font->mCursor[1].vf158(CoordInfo(GetSpawnData(0x2C)))) {
			loaded = 1;
		}
	}
	if (loaded) {
		font->mCursor[1].vf08C(3.0f, 3.0f, 3.0f);
		font->mCursor[1].mStatus |= 0x800;
		return 1;
	}
	return 0;
}

/**
 * @note Address: 0x800A2524
 * @note Size: 0x738
 */
extern "C" void CMsgFont_Init(CMsgFont* font, u8* data, f32 x, f32 y, WiiTitleObject* anchor, s32 fitLine, s32 mode)
{
	font->mData = (CMsgData*)data;
	ResetMessageState(font);
	font->mBits.mActive  = TRUE;
	font->mBits.mFadeIn  = TRUE;
	font->mBits.mFadeOut = FALSE;
	font->mBits.mBlink   = FALSE;
	font->mBits.mFitLine = fitLine;
	font->mBits._04_04   = TRUE;
	f32 height           = font->mData->mTotalHeight;
	f32 width            = font->mData->mMaxWidth;
#define MSG_INIT_DESCRIPTOR(i) ((i) + 1)
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[MSG_INIT_DESCRIPTOR(138)], 0);
	font->mFrame.mTex = gGame.mTpl.mTex;
	font->mScrollX = font->mScrollY = 0.0f;
	PSMTXTrans(font->mFrame.mTex.mTexMtx.mMtx[0], font->mScrollX, font->mScrollY, 0.0f);
	font->mFrame.mTex.mTexMtx.mDirty = TRUE;
	font->mScaleX                    = width / GXGetTexObjWidth(&font->mFrame.mTex);
	font->mScaleY                    = height / GXGetTexObjHeight(&font->mFrame.mTex);
	PSMTXScale(font->mFrame.mTex.mTexMtx.mMtx[2], font->mScaleX, font->mScaleY, 1.0f);
	font->mFrame.mTex.mTexMtx.mDirty = TRUE;
	font->mMode                      = mode;
	for (s32 i = 0; i < 2; i++) {
		font->mCursor[i].vf09C(0.0f);
		gGame.mObjectRefs[gGame.mObjectRefCount].mObj   = &font->mCursor[i];
		gGame.mObjectRefs[gGame.mObjectRefCount].mFlags = 0;
		gGame.mObjectRefCount++;
	}
	for (s32 i = 0; i < 400; i++) {
		font->mGlyphs[i].mSprite.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		font->mGlyphs[i].mSprite.mTex = &font->mGlyphs[i].mTexture;
		font->mGlyphs[i].mColor       = msgColorTable[0];
		font->mGlyphs[i].mLine        = 0;
	}
	font->mPlacement          = anchor;
	font->mCursorX            = -((font->mBits.mFitLine ? font->mData->mWidths[font->mLine] : width) * 0.5f);
	font->mWindowX            = x;
	font->mWidth              = width;
	font->mCursorY            = 0.0f;
	font->mWindowY            = y;
	font->mHeight             = height;
	font->mLineY[font->mLine] = font->mCursorY + (font->mData->mHeights[0] - 22.0f) * 0.5f;
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[MSG_INIT_DESCRIPTOR(115)], 0);
	font->mTexs.mTex[MSG_TEX_CURSOR] = gGame.mTpl.mTex;
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[MSG_INIT_DESCRIPTOR(116)], 0);
	font->mTexs.mTex[MSG_TEX_DIGIT] = gGame.mTpl.mTex;
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[MSG_INIT_DESCRIPTOR(119)], 0);
	font->mTexs.mTex[MSG_TEX_ICON] = gGame.mTpl.mTex;
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[MSG_INIT_DESCRIPTOR(117)], 0);
	font->mTexs.mTex[MSG_TEX_ARROW] = gGame.mTpl.mTex;
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[MSG_INIT_DESCRIPTOR(118)], 0);
	font->mTexs.mTex[MSG_TEX_LINE] = gGame.mTpl.mTex;
#undef MSG_INIT_DESCRIPTOR
}

/**
 * @note Address: 0x800A2C5C
 * @note Size: 0x230
 */
extern "C" void ResetMessageState(CMsgFont* font)
{
	// the GC's u8 flag bits as a word: clear the four state bits, set the two enable bits
	font->mFlags          = (font->mFlags & 0x0FFFFFFF) | 0x0C000000;
	font->mCharCount      = 0;
	font->mLine           = 0;
	font->mPlacement      = NULL;
	Vec zero              = { 0.0f, 0.0f, 0.0f };
	font->mPosition       = zero;
	font->mSize           = 22.0f;
	font->mPrevSize       = NAN;
	font->mGlyphColor     = msgColorTable[0];
	font->mGlyphAngle     = 0.0f;
	font->mGlyphScaleY    = 1.0f;
	font->mGlyphScaleX    = 1.0f;
	font->mEffect         = 0;
	font->mSelectedLine   = -1;
	font->mPointerState   = 0;
	font->mOtherSelection = -1;
	font->mAlpha          = 0.0f;
	font->mTextAlpha      = 1.0f;
	font->mHighlightAlpha = 0.0f;
	font->mBlinkIcon      = 0;
	font->mBlinkPhase     = 0.0f;
	font->mTexOffset      = 0.0f;
	for (int i = 0; i < 400; i++) {
		font->mGlyphs[i].mPhaseX = font->mGlyphs[i].mPhaseY = (i * 75) % 360;
	}
}

/**
 * @note Address: 0x800A2E8C
 * @note Size: 0x634
 */
// the GameCube's non-japanese shape: the proportional flag is _04_04, mPrevSize is NAN until
// the first glyph and the icon strip comes from gGame.mTpl. the Wii's CTexObj keeps its four
// matrices in mTexMtx.mMtx; the icon scale lands in mMtx[2] where the GameCube used mMtx[1]
extern "C" void CMsgFont_AddChar(CMsgFont* font, const char* text)
{
	f32 width = MeasureGameTextWidth(&gGame.mGs, text, font->mBits._04_04, font->mSize);
	// half the size change since the previous glyph; mPrevSize is NAN until the first one (CMsgFont_Init)
	f32 spacing;
	if (ClassifyFloatBits(font->mPrevSize) == FP_NAN) {
		spacing = 0.0f;
	} else {
		spacing = (font->mSize - font->mPrevSize) * 0.5f;
	}
	s32 textureWidth;
	if ((u8)text[0] == MSG_CODE_ICON) {
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[120], 0);
		CTexObj icon = gGame.mTpl.mTex;
		s32 index    = (u8)text[1];
		s32 columns  = font->mData->mIconColumns;
		if (index >= columns) {
			index = columns - 1;
		}
		f32 cellWidth = 1.0f / columns;
		PSMTXScale(icon.mTexMtx.mMtx[2], cellWidth, 1.0f, 1.0f);
		icon.mTexMtx.mDirty = true;
		PSMTXTrans(icon.mTexMtx.mMtx[0], cellWidth * index, 0.0f, 0.0f);
		icon.mTexMtx.mDirty = true;
		textureWidth        = (s32)font->mGlyphs[font->mCharCount].mTexture.Set(icon) / columns;
		width               = textureWidth * (font->mSize / 22.0f);
	} else {
		textureWidth = font->mGlyphs[font->mCharCount].mSprite.mTex->SetChar(text, 0);
	}
	f32 x = (width - textureWidth) * 0.5f + (11.0f + font->mCursorX + spacing);
	f32 y = font->mCursorY + font->mData->mHeights[font->mLine] * 0.5f;
	font->mGlyphs[font->mCharCount].mSprite.SetTrans(x, y, 0.0f);
	f32 sx, sy;
	if (font->mGlyphScaleX >= 0.0f && font->mGlyphScaleY >= 0.0f) {
		sx = font->mGlyphScaleX;
		sy = font->mGlyphScaleY;
	} else {
		// a negative MSG_CODE_SCALE: each glyph gets |x| plus a random part of the gap up to |y|
		f32 min   = fabs(100.0f * font->mGlyphScaleX);
		f32 max   = fabs(100.0f * font->mGlyphScaleY);
		f32 extra = fmod((f32)rand(), max - min);
		sx = sy = fabs(font->mGlyphScaleX) + extra / 100.0f;
	}
	f32 scaleX = (font->mSize * 0.5f) * sx;
	f32 scaleY = (-font->mSize * 0.5f) * sy;
	font->mGlyphs[font->mCharCount].mSprite.SetScale(scaleX, scaleY, 1.0f);
	f32 angle = font->mGlyphAngle;
	// a negative MSG_CODE_ROTATE angle: each glyph gets a random tilt between angle and -angle
	if (!(angle >= 0.0f)) {
		f32 range = fabs(2.0f * angle);
		f32 delta = fmod((f32)rand(), RAD2DEG * range);
		angle     = DEG2RAD * delta + font->mGlyphAngle;
	}
	font->mGlyphs[font->mCharCount].mSprite.SetRot(0.0f, 0.0f, angle);
	font->mGlyphs[font->mCharCount].mColor  = font->mGlyphColor;
	font->mGlyphs[font->mCharCount].mLine   = font->mLine;
	font->mGlyphs[font->mCharCount].mAngle  = angle;
	font->mGlyphs[font->mCharCount].mScaleX = scaleX;
	font->mGlyphs[font->mCharCount].mScaleY = scaleY;
	if (!(angle <= PI)) {
		angle = TWO_PI - angle;
	}
	font->mGlyphs[font->mCharCount].mWidthAdjust = (angle / PI) * (width - font->mSize);
	font->mGlyphs[font->mCharCount].mEffect      = font->mEffect;
	font->mGlyphs[font->mCharCount].mAmount      = font->mAmount;
	font->mGlyphs[font->mCharCount].mPhaseZ      = rand() % 360;
	font->mGlyphs[font->mCharCount].mPhaseW      = rand() % 360;
	font->mGlyphs[font->mCharCount].mSpeed = font->mGlyphs[font->mCharCount].mRefresh = font->mSpeed;
	font->mCursorX += width + spacing;
	font->mPrevSize = font->mSize;
	font->mCharCount++;
}

// the GameCube's PAL formula at the remote's refresh rate
#define MSG_FADE_RATE (1.0f / (0.3f * gRefreshRate))

/**
 * @note Address: 0x800A34C0
 * @note Size: 0x69C
 */
extern "C" void CMsgFont_Update(CMsgFont* font, u32 step)
{
	if (font->mBits.mActive) {
		if (font->mBits.mFadeIn && font->mAlpha < 1.0f) {
			// zero-alpha tint and no voice code (mMode stays -1, see CMsgData_Measure): no fade in
			if (font->mColor.a == 0 && font->mMode < 0) {
				font->mAlpha        = 1.0f;
				font->mBits.mFadeIn = FALSE;
			}
			f32 alpha    = font->mAlpha + MSG_FADE_RATE * step;
			font->mAlpha = alpha;
			if (alpha >= 1.0f) {
				font->mAlpha        = 1.0f;
				font->mBits.mFadeIn = FALSE;
			}
		}
		if (font->mBits.mFadeOut) {
			if (font->mAlpha > 0.0f) {
				f32 alpha    = font->mAlpha - MSG_FADE_RATE * step;
				font->mAlpha = alpha;
				if (alpha <= 0.0f) {
					font->mAlpha         = 0.0f;
					font->mBits.mFadeOut = FALSE;
					font->mBits.mActive  = FALSE;
				}
			}
			font->mTextAlpha = font->mAlpha;
		}
		// a select() line or a number input digit is picked: fade the highlight in, else out
		if (font->mSelectedLine >= 0 || font->mOtherSelection >= 0) {
			if (font->mHighlightAlpha < 1.0f) {
				f32 alpha             = font->mHighlightAlpha + MSG_FADE_RATE * step;
				font->mHighlightAlpha = alpha;
				if (alpha >= 1.0f) {
					font->mHighlightAlpha = 1.0f;
				}
			}
		} else if (font->mHighlightAlpha > 0.0f) {
			f32 alpha             = font->mHighlightAlpha - MSG_FADE_RATE * step;
			font->mHighlightAlpha = alpha;
			if (alpha <= 0.0f) {
				font->mHighlightAlpha = 0.0f;
			}
		}
		if (font->mSelectedLine >= 0) {
			f32 width = font->mBits.mFitLine ? font->mData->mWidths[font->mSelectedLine] : font->mData->mMaxWidth;
			font->mCurLineY += 0.8f * (font->mLineY[font->mSelectedLine] - font->mCurLineY);
			font->mCurLineWidth += 0.7f * (width - font->mCurLineWidth);
			font->mTexOffset -= 0.008f * step;
			PSMTXTrans(font->mTexs.mTex[MSG_TEX_LINE].mTexMtx.mMtx[0], font->mTexOffset, 0.0f, 0.0f);
			font->mTexs.mTex[MSG_TEX_LINE].mTexMtx.mDirty = true;
		}
		if (font->mBits.mBlink) {
			// CMsgFont_Draw fades the button prompt with sin(mBlinkPhase): 6 degrees a frame, wrapped at PI, a pulse every 30 frames
			font->mBlinkPhase += DEG2RAD * 6.0f * step;
			font->mBlinkPhase = fmod(font->mBlinkPhase, (f64)PI);
		}
	}

	if (font->mBits.mActive) {
		for (s32 i = 0; i < 400; i++) {
			// CMsgFont_Draw: bits 1 and 2 shift the glyph, 4 pulses its scale, 8 rocks it; the high nibble sets how the phases move
			switch (font->mGlyphs[i].mEffect & 0xF0) {
			case 0x10:
				// smooth: every phase turns by its share of mSpeed degrees a frame
				// this one adds in u32 (step is u32) before going to float, so a sum below 0 wraps
				font->mGlyphs[i].mPhaseY = (s16)(f32)fmod((f32)(font->mGlyphs[i].mPhaseY + font->mGlyphs[i].mSpeed * step), 360.0);
				font->mGlyphs[i].mPhaseX = (s16)(f32)fmod(font->mGlyphs[i].mPhaseX + (0.5f * font->mGlyphs[i].mSpeed) * step, 360.0);
				font->mGlyphs[i].mPhaseZ = (s16)(f32)fmod(font->mGlyphs[i].mPhaseZ + (0.7f * font->mGlyphs[i].mSpeed) * step, 360.0);
				font->mGlyphs[i].mPhaseW = (s16)(f32)fmod(font->mGlyphs[i].mPhaseW + (0.8f * font->mGlyphs[i].mSpeed) * step, 360.0);
				break;
			case 0x20:
				// jitter: fresh random phases every mSpeed updates
				if (--font->mGlyphs[i].mRefresh <= 0) {
					font->mGlyphs[i].mPhaseY  = rand() % 360;
					font->mGlyphs[i].mPhaseX  = rand() % 360;
					font->mGlyphs[i].mPhaseZ  = rand() % 360;
					font->mGlyphs[i].mPhaseW  = rand() % 360;
					font->mGlyphs[i].mRefresh = font->mGlyphs[i].mSpeed;
				}
				break;
			}
		}
	}
	font->mScrollX += 0.01f * step;
	font->mScrollY += 0.01f * step;
	PSMTXTrans(font->mFrame.mTex.mTexMtx.mMtx[0], font->mScrollX, font->mScrollY, 0.0f);
	font->mFrame.mTex.mTexMtx.mDirty = true;
	// only mode 0 has the portrait, CMsgFont_Draw leaves 40 * scale of room for it; the Wii
	// updates both cursor objects, where the GameCube had one
	if (font->mMode == 0) {
		for (s32 i = 0; i < 2; i++) {
			if (font->mCursor[i].mStatus & 1) {
				font->mCursor[i].UpdateObject(step);
			}
		}
	}
}

/**
 * @note Address: 0x800A3B5C
 * @note Size: 0x20E0
 */
extern "C" void CMsgFont_Draw(CMsgFont* font, f32 lastAlpha)
{
	WiiTitleObject* anchor = font->mPlacement;
	if (anchor != NULL && (anchor->mStatus & 1) && (anchor->mStatus & 0x10000)) {
		anchor->vf1D4(&font->mPosition, &font->mWindowX, &font->mWindowY, 0, 0);
		font->mWindowY -= font->mHeight;
	}
	if (font->mWindowX - font->mWidth * 0.5f < gScreenSafeLeft)
		font->mWindowX = gScreenSafeLeft + font->mWidth * 0.5f;
	if (font->mWindowX + font->mWidth * 0.5f > gScreenSafeRight)
		font->mWindowX = gScreenSafeRight - font->mWidth * 0.5f;
	if (font->mWindowY < gScreenSafeTop)
		font->mWindowY = gScreenSafeTop;
	if (font->mWindowY + font->mHeight > gScreenSafeBottom)
		font->mWindowY = gScreenSafeBottom - font->mHeight;
	f32 portraitScale;
	if (gGame.mWipeCine.mActive != 0 && font->_1484 == 0)
		portraitScale = 0.84615386f;
	else
		portraitScale = font->mHeight / 52.0f;
	if (portraitScale > 1.1f)
		portraitScale = 1.1f;
	if (portraitScale < 0.5f)
		portraitScale = 0.5f;
	// only mode 0 has the portrait, and the text moves right by half its width
	f32 portraitWidth = (font->mMode == 0) ? 40.0f * portraitScale : 0.0f;
	f32 textOffset    = portraitWidth;
	// 4.712389 is 3 pi / 2: as mAlpha rises from 0 to 1 the frame starts 4 pixels in, swells 4 out, then settles
	f32 expandX = 4.0f * (f32)sin(4.712389f * (1.0f - font->mAlpha));
	f32 expandY = 4.0f * (f32)sin(4.712389f * (1.0f - font->mAlpha));

	if (font->mAlpha > 0.0f) {
		gGame.mGs.mCurRenderState->SetLineWidth(12, (GXTexOffset)5);
		f32 halfPortrait = portraitWidth / 2.0f;
		f32 left         = ((font->mWindowX - font->mWidth * 0.5f) - 40.0f - expandX) - halfPortrait;
		f32 right        = halfPortrait + (expandX + (40.0f + (font->mWindowX + font->mWidth * 0.5f)));
		f32 top          = (font->mWindowY - 8.0f) - expandY;
		f32 bottom       = expandY + (8.0f + (font->mWindowY + font->mHeight));
		f32 windowAlpha  = font->mColor.a != 0 ? font->mAlpha : 0.0f;
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		f32 outerRight  = 9.0f + (8.0f + right);
		f32 outerBottom = 9.0f + (8.0f + bottom);
		DrawGsVertexQuad(&gGame.mGs, left - 8.0f, top - 8.0f, -1.0f, 0xFF000000, outerRight, top - 8.0f, -1.0f, 0xFF000000, outerRight,
		                 outerBottom, -1.0f, 0xFF000000, left - 8.0f, outerBottom, -1.0f, 0xFF000000);

		f32 innerLeft = 1.0f + left, innerRight = right - 1.0f;
		f32 innerTop = 1.0f + top, innerBottom = bottom - 1.0f;
		f32 roundLeft = 8.0f + innerLeft, roundRight = innerRight - 8.0f;
		f32 roundTop = 8.0f + innerTop, roundBottom = innerBottom - 8.0f;
		Vec fan[10] = { (Vec) { font->mWindowX, font->mWindowY, 0.0f },
			            (Vec) { innerLeft, roundTop, 0.0f },
			            (Vec) { roundLeft, innerTop, 0.0f },
			            (Vec) { roundRight, innerTop, 0.0f },
			            (Vec) { innerRight, roundTop, 0.0f },
			            (Vec) { innerRight, roundBottom, 0.0f },
			            (Vec) { roundRight, innerBottom, 0.0f },
			            (Vec) { roundLeft, innerBottom, 0.0f },
			            (Vec) { innerLeft, roundBottom, 0.0f },
			            (Vec) { innerLeft, roundTop, 0.0f } };
		u32 color
		    = ((u32)font->mColor.r << 24) | ((u32)font->mColor.g << 16) | ((u32)font->mColor.b << 8) | (u32)(font->mColor.a * font->mAlpha);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, fan, color, 10);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_TRUE);
		DrawGsCircle(&gGame.mGs, 16, color, color, roundLeft, roundTop, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, color, roundRight, roundTop, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, color, roundLeft, roundBottom, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, color, roundRight, roundBottom, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		color = (u32)(64.0f * windowAlpha);
		for (u32 i = 0; i < 10; i++) {
			fan[i].x += 9.0f;
			fan[i].y += 9.0f;
		}
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, fan, color, 10);
		f32 shadowLeft = 8.0f + (1.0f + (9.0f + left));
		f32 shadowTop  = 8.0f + (1.0f + (9.0f + top));
		DrawGsCircle(&gGame.mGs, 16, color, color, shadowLeft, shadowTop, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		f32 shadowRight = ((9.0f + right) - 1.0f) - 8.0f;
		DrawGsCircle(&gGame.mGs, 16, color, color, shadowRight, shadowTop, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		f32 shadowBottom = ((9.0f + bottom) - 1.0f) - 8.0f;
		DrawGsCircle(&gGame.mGs, 16, color, color, shadowLeft, shadowBottom, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, color, shadowRight, shadowBottom, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);

		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[115], 0);
		CTexObj border    = gGame.mTpl.mTex;
		f32 textureWidth  = GXGetTexObjWidth(&border);
		f32 textureHeight = GXGetTexObjHeight(&border);
		f32 stretchX      = (right - left) - 128.0f;
		if (stretchX < 0.0f)
			stretchX = 0.0f;
		f32 stretchY = (bottom - top) - 64.0f;
		if (stretchY < 0.0f)
			stretchY = 0.0f;
		MsgWindowSlice slices[8]
		    = { { 0, 0, 0.5f, 0.5f, 0, 0, 64, 32 },   { 0.5f, 0, 0, 0.5f, 0, 0, 0, 32 },       { 0.5f, 0, 0.5f, 0.5f, 0, 0, 64, 32 },
			    { 0, 0.5f, 0.5f, 0, 0, 0, 64, 0 },    { 0.5f, 0.5f, 0.5f, 0, 0, 0, 64, 0 },    { 0, 0.5f, 0.5f, 0.5f, 0, 0, 64, 32 },
			    { 0.5f, 0.5f, 0, 0.5f, 0, 0, 0, 32 }, { 0.5f, 0.5f, 0.5f, 0.5f, 0, 0, 64, 32 } };
		f32 edgeLeft = (32.0f + left) - 3.0f, edgeRight = 3.0f + (right - 32.0f);
		f32 edgeTop = 8.0f + ((16.0f + top) - 10.0f), edgeBottom = 10.0f + (bottom - 16.0f);
		slices[1].scaleU = 1.0f / textureWidth;
		slices[0].x      = edgeLeft;
		slices[0].y      = edgeTop;
		slices[1].x      = font->mWindowX;
		slices[1].y      = edgeTop;
		slices[1].width  = 6.0f + stretchX;
		slices[2].x      = edgeRight;
		slices[2].y      = edgeTop;
		slices[3].scaleV = 1.0f / textureHeight;
		slices[3].x      = edgeLeft;
		slices[3].y      = 4.0f + (font->mHeight * 0.5f + font->mWindowY);
		slices[3].height = (20.0f + stretchY) - 8.0f;
		slices[4].scaleV = 1.0f / textureHeight;
		slices[4].x      = edgeRight;
		slices[4].y      = 4.0f + (font->mHeight * 0.5f + font->mWindowY);
		slices[4].height = (20.0f + stretchY) - 8.0f;
		slices[5].x      = edgeLeft;
		slices[5].y      = edgeBottom;
		slices[6].scaleU = 1.0f / textureWidth;
		slices[6].x      = font->mWindowX;
		slices[6].y      = edgeBottom;
		slices[6].width  = 6.0f + stretchX;
		slices[7].x      = edgeRight;
		slices[7].y      = edgeBottom;
		color            = 0xFFFFFF00 | (u32)(255.0f * windowAlpha);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_TRUE);
		for (u32 i = 0; i < 8; i++) {
			PSMTXTrans(border.mTexMtx.mMtx[0], slices[i].u, slices[i].v, 0.0f);
			border.mTexMtx.mDirty = true;
			PSMTXScale(border.mTexMtx.mMtx[2], slices[i].scaleU, slices[i].scaleV, 1.0f);
			border.mTexMtx.mDirty = true;
			DrawGsTexturedQuad(&gGame.mGs, &border, color, color, color, color, slices[i].x, slices[i].y, 1.0f, slices[i].width,
			                   slices[i].height, 0.0f, 0.0f, 0.0f);
		}
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		DrawGsVertexQuad(&gGame.mGs, left - 8.0f, top - 8.0f, -1.0f, 0xFF000000, outerRight, top - 8.0f, -1.0f, 0xFF000000, outerRight,
		                 outerBottom, -1.0f, 0xFF000000, left - 8.0f, outerBottom, -1.0f, 0xFF000000);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);

		if (font->mMode == 0) {
			if (font->mColor.a != 0) {
				u32 clipHeight = (u32)(2.0f * ((8.0f + expandY) - 1.0f) + font->mHeight);
				u32 clipWidth  = (u32)(2.0f * ((40.0f + expandX) - 1.0f) + font->mWidth);
				u32 clipY      = (u32)(1.0f + ((font->mWindowY - 8.0f) - expandY));
				u32 clipX      = (u32)(1.0f + (((font->mWindowX - font->mWidth / 2.0f) - 40.0f - expandX) - halfPortrait));
				GXSetScissor(clipX, clipY, clipWidth, clipHeight);
			}
			f32 portraitX            = 44.0f * portraitScale + ((font->mWindowX - font->mWidth * 0.5f) - 40.0f - halfPortrait);
			f32 portraitY            = 12.0f * portraitScale + (font->mHeight / 2.0f + font->mWindowY);
			WiiTitleObject* portrait = NULL;
			s32 objectCount          = gGame.mObjPoolCount;
			s32 stride               = gGame.mObjPoolStride;
			u8* objects              = gGame.mObjPoolObjects;
			for (s32 i = 0; i < objectCount; i++) {
				WiiTitleObject* obj = (WiiTitleObject*)(objects + i * stride);
				if (obj->mStatus & 1) {
					if (*(s16*)((u8*)obj->mKind + 0x16) == 10)
						portrait = obj;
				}
			}
			if (portrait != NULL && font->_1484 == 0) {
				// the Wii draws a copy of the portrait through its own cursor object: the anim
				// and pose move over, the head joint's height decides how far it sits raised
				Vec head       = portrait->vf234("head");
				Vec base       = portrait->vf0B8(0);
				f32 headHeight = head.y - base.y - 1.2f;
				f32 frame      = portrait->vf128();
				s32 anim       = portrait->vf11C();
				font->mCursor[0].vf108(anim, 1, frame, 0.0f, 0.0f, 15.0f);
				font->mCursor[0].vf084(portraitX, 4.0f * headHeight + portraitY, 80.0f * portraitScale);
				font->mCursor[0].vf09C((9.0f * portraitScale) * font->mAlpha);
				font->mCursor[0].vf190();
				if (anim == 0x13 || anim == 0x3FB) {
					font->mCursor[1].vf090(font->mCursor[0].vf244());
					font->mCursor[1].vf070(font->mCursor[0].vf26C());
					font->mCursor[1].vf098(font->mCursor[0].vf258());
					if (font->mCursor[1].vf11C() == 0) {
						font->mCursor[1].vf108(1, 1, 0.0f, 0.0f, 0.0f, 15.0f);
					}
					frame = font->mCursor[0].vf128();
					for (s32 i = 0; i < 6; i++) {
						WiiCursorPart* part = font->mCursor[1].mParts->vf040(i);
						if (part != NULL) {
							part->vf01C(frame);
						}
					}
					font->mCursor[1].vf190();
				} else if (font->mCursor[1].vf11C() == 1) {
					font->mCursor[1].vf108(0, 1, 0.0f, 0.0f, 0.0f, 15.0f);
				}
			} else {
				font->mCursor[0].vf108(1, 0, 0.0f, 1.0f, 0.0f, 15.0f);
				font->mCursor[0].vf084(portraitX, portraitY, 80.0f * portraitScale);
				font->mCursor[0].vf09C((9.0f * portraitScale) * font->mAlpha);
				font->mCursor[0].vf190();
			}
			if (font->mColor.a != 0) {
				GXSetScissor(gScreenLeft, gScreenTop, gPointerScreenWidth, gPointerScreenHeight);
			}
		}
		if (font->mSelectedLine >= 0) {
			f32 x      = textOffset / 2.0f + font->mWindowX;
			f32 y      = font->mCurLineY + (8.0f + font->mWindowY);
			f32 width  = 24.0f + font->mCurLineWidth;
			f32 height = font->mData->mHeights[font->mSelectedLine];
			u32 color  = 0xC87F1600 | (u32)(200.0f * font->mHighlightAlpha);
			if (font->mPointerState == 2)
				color = 0x7FC81600 | (u32)(200.0f * font->mHighlightAlpha);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			CTexObj* highlight = &font->mTexs.mTex[MSG_TEX_ARROW];
			PSMTXScale(highlight->mTexMtx.mMtx[2], 0.25f, 1.0f, 1.0f);
			highlight->mTexMtx.mDirty = true;
			PSMTXTrans(highlight->mTexMtx.mMtx[0], 0.25f, 0.0f, 0.0f);
			highlight->mTexMtx.mDirty = true;
			y                         = 2.0f + y;
			DrawGsTexturedQuad(&gGame.mGs, highlight, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
			PSMTXTrans(highlight->mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
			highlight->mTexMtx.mDirty = true;
			f32 halfWidth             = width / 2.0f;
			DrawGsTexturedQuad(&gGame.mGs, highlight, color, color, color, color, (x - halfWidth) - 4.0f, y, 0.0f, 8.0f, height, 0.0f, 0.0f,
			                   0.0f);
			PSMTXTrans(highlight->mTexMtx.mMtx[0], 0.5f, 0.0f, 0.0f);
			highlight->mTexMtx.mDirty = true;
			DrawGsTexturedQuad(&gGame.mGs, highlight, color, color, color, color, 4.0f + (x + halfWidth), y, 0.0f, 8.0f, height, 0.0f, 0.0f,
			                   0.0f);
			CTexObj* line = &font->mTexs.mTex[MSG_TEX_LINE];
			f32 lineWidth = GXGetTexObjWidth(line);
			PSMTXScale(line->mTexMtx.mMtx[2], 0.5f * (width / lineWidth), 0.5f, 1.0f);
			line->mTexMtx.mDirty = true;
			color                = 0xFFFFFF00 | (u32)(40.0f * font->mHighlightAlpha);
			DrawGsTexturedQuad(&gGame.mGs, line, color, color, color, color, 2.0f + x, y, 0.0f, width, height - 4.0f, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		}
		if (font->mOtherSelection >= 0) {
			s32 line    = font->mGlyphs[font->mCharCount - 1].mLine;
			f32 center  = textOffset / 2.0f + font->mWindowX;
			f32 x       = ((font->mDigitCount * font->mSize) / 2.0f + center) - (font->mOtherSelection + 1) * font->mSize;
			f32 y       = 3.0f + ((font->mLineY[line] + (8.0f + font->mWindowY)) - font->mData->mHeights[line] / 2.0f);
			Vec quad[4] = { (Vec) { x, y, 0.0f }, (Vec) { x + font->mSize, y, 0.0f }, (Vec) { x + font->mSize, y + font->mSize, 0.0f },
				            (Vec) { x, y + font->mSize, 0.0f } };
			u32 color   = 0xFF00 | (u32)(200.0f * font->mHighlightAlpha);
			DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, color, quad[1].x, quad[1].y, quad[1].z, color, quad[2].x,
			                 quad[2].y, quad[2].z, color, quad[3].x, quad[3].y, quad[3].z, color);
			for (s32 i = 0; i < font->mDigitCount; i++) {
				f32 digitX = ((font->mDigitCount * font->mSize) / 2.0f + center) - (i + 1) * font->mSize;
				u32 color;
				if (i < font->mFirstEditDigit)
					color = 0xFFFFFF00 | (u32)(255.0f * font->mTextAlpha);
				else
					color = 0x64FF6400 | (u32)(255.0f * font->mTextAlpha);
				char text[4];
				ConvertHalfWidthToFullWidthSJIS(FormatToStaticBuffer("%d", font->mDigits[i]), text);
				f32 width = MeasureGameTextWidth(&gGame.mGs, text, TRUE, font->mSize);
				digitX += (font->mSize - width) / 2.0f;
				DrawGsProportionalTextFormat(&gGame.mGs, color, text, digitX, y, 0.0f, font->mSize, font->mSize);
			}
		}
		gGame.mGs.mCurRenderState->SetLineWidth(12, (GXTexOffset)5);
	}

	if (font->mTextAlpha > 0.0f) {
		gGame.mGs.mCurTevStage->SetOrder(0, 0, 4);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurTevStage->SetNumTexGens(1);
		gGame.mGs.mCurTevStage->SetTexGen(0, 1, 4, 30);
		if (gGame.mGs.mCurRenderState != NULL)
			gGame.mGs.mCurRenderState->Apply();
		if (gGame.mGs.mCurTevStage != NULL)
			gGame.mGs.mCurTevStage->Apply();
		if (gGame.mGs.mCurChannel != NULL)
			gGame.mGs.mCurChannel->Apply();
		f32 halfOffset = textOffset / 2.0f;
		f32 alpha      = font->mTextAlpha;
		f32 last       = alpha * lastAlpha;
		Mtx translation, view;
		PSMTXTrans(translation, 2.0f + (font->mWindowX + halfOffset), 1.0f + font->mWindowY, 0.0f);
		PSMTXConcat(gGame.mGs.mCamera->mViewMtx, translation, view);
		for (s32 i = 0; i < font->mCharCount; i++) {
			MsgFontGlyph& glyph = font->mGlyphs[i];
			if (glyph.mLine == font->mSelectedLine) {
				u32 color = (u32)(150.0f * (i == font->mCharCount - 1 ? last : alpha));
				glyph.mSprite.SetBillboard(0);
				glyph.mSprite.Draw(view, GX_TEXMAP0, 30, GX_MTX2x4, color, color, color, color);
			}
		}
		PSMTXTrans(translation, font->mWindowX + halfOffset, font->mWindowY, 0.0f);
		PSMTXConcat(gGame.mGs.mCamera->mViewMtx, translation, view);
		for (s32 i = 0; i < font->mCharCount; i++) {
			MsgFontGlyph& glyph = font->mGlyphs[i];
			f32 opacity         = i == font->mCharCount - 1 ? last : alpha;
			u32 color           = (glyph.mColor & 0xFFFFFF00) | (u8)((u8)glyph.mColor * opacity);
			glyph.mSprite.SetBillboard(0);
			switch (glyph.mEffect & 0xF0) {
			case 0x10:
			case 0x20: {
				f32 angle       = glyph.mAngle;
				f32 shakeY      = (glyph.mEffect & 1) ? glyph.mAmount * (f32)sin(DEG2RAD * glyph.mPhaseY) : 0.0f;
				f32 shakeX      = (glyph.mEffect & 2) ? glyph.mAmount * (f32)sin(DEG2RAD * glyph.mPhaseX) : 0.0f;
				f32 quarterTurn = HALF_PI + angle;
				f32 sideSin     = sin(quarterTurn);
				f32 forwardSin  = sin(angle);
				f32 dx          = shakeY * forwardSin + shakeX * sideSin;
				f32 sideCos     = cos(quarterTurn);
				f32 forwardCos  = cos(angle);
				f32 dy          = shakeY * forwardCos + shakeX * sideCos;
				f32 scaleX = glyph.mScaleX, scaleY = glyph.mScaleY;
				if (glyph.mEffect & 4) {
					f32 wave  = sin(DEG2RAD * glyph.mPhaseW);
					f32 scale = 0.1f * wave + 1.0f;
					glyph.mSprite.SetScale(scaleX * scale, scaleY * scale, 1.0f);
				}
				if (glyph.mEffect & 8) {
					// 0.52359879 is 30 degrees and SetRot gets angle + wave / 2, so the glyph rocks up to 15 degrees either way
					f32 wave = 0.52359879f * (f32)sin(DEG2RAD * glyph.mPhaseZ);
					glyph.mSprite.SetRot(0.0f, 0.0f, wave + (angle - wave / 2.0f));
				}
				Mtx offset, animatedView;
				PSMTXTrans(offset, dx, dy, 0.0f);
				PSMTXConcat(view, offset, animatedView);
				glyph.mSprite.Draw(animatedView, GX_TEXMAP0, 30, GX_MTX2x4, color, color, color, color);
				break;
			}
			default:
				if (glyph.mAngle > 0.0f) {
					f32 wave  = sin(glyph.mAngle);
					f32 width = glyph.mWidthAdjust;
					Mtx offset, rotatedView;
					PSMTXTrans(offset, width, -width * wave, 0.0f);
					PSMTXConcat(view, offset, rotatedView);
					glyph.mSprite.Draw(rotatedView, GX_TEXMAP0, 30, GX_MTX2x4, color, color, color, color);
				} else
					glyph.mSprite.Draw(view, GX_TEXMAP0, 30, GX_MTX2x4, color, color, color, color);
				break;
			}
		}
		gGame.mGs.mCurTevStage->SetOrder(255, 255, 4);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		gGame.mGs.mCurTevStage->SetNumTexGens(0);
	}
	if (font->mBits.mBlink && (s32)font->mBlinkIcon >= 0) {
		gGame.mGs.mCurTevStage->SetOrder(0, 0, 4);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurTevStage->SetNumTexGens(1);
		gGame.mGs.mCurTevStage->SetTexGen(0, 1, 4, 30);
		s32 index = font->mBlinkIcon;
		if (index >= 2)
			index = 2;
		CTexObj* cursor = &font->mTexs.mTex[MSG_TEX_CURSOR] + index;
		f32 width       = GXGetTexObjWidth(cursor);
		f32 height      = GXGetTexObjHeight(cursor);
		if ((s32)font->mBlinkIcon >= 2) {
			width = (s32)GXGetTexObjWidth(cursor) / 11;
			PSMTXScale(cursor->mTexMtx.mMtx[2], 1.0f / 11.0f, 1.0f, 1.0f);
			cursor->mTexMtx.mDirty = true;
			PSMTXTrans(cursor->mTexMtx.mMtx[0], (1.0f / 11.0f) * ((s32)font->mBlinkIcon - 2), 0.0f, 0.0f);
			cursor->mTexMtx.mDirty = true;
		}
		s32 line = font->mGlyphs[font->mCharCount - 1].mLine;
		f32 x    = width / 2.0f + (font->mCursorX + (textOffset * 0.5f + font->mWindowX));
		f32 y    = (font->mData->mHeights[line] / 2.0f + (font->mLineY[line] + (8.0f + font->mWindowY))) - height / 4.0f;
		f32 wave = sin(font->mBlinkPhase);
		if (wave < 0.0f)
			wave = 0.0f;
		u32 shadow = (u32)(200.0f * wave);
		u32 color  = 0xFFFFFF00 | shadow;
		DrawGsTexturedQuad(&gGame.mGs, cursor, shadow, shadow, shadow, shadow, 2.0f + x, 2.0f + y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
		DrawGsTexturedQuad(&gGame.mGs, cursor, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetOrder(255, 255, 4);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		gGame.mGs.mCurTevStage->SetNumTexGens(0);
	}
}

/**
 * @note Address: 0x800A5C3C
 * @note Size: 0x1A0
 */
// the Wii's InitMessageWindow: the reset inline of the GameCube's ResetMessageState(CMsg*)
// plus the window position, tint, cursor init and mode
extern "C" void InitMessageWindow(CMsg* msg, s32 mode)
{
	msg->mData.mCursor      = NULL;
	msg->mData.mIconColumns = 0;
	ResetMessageState(&msg->mFont);
	msg->mIdle         = 1;
	msg->mCharDelay    = 0.0f;
	msg->mCharTimer    = 0.0f;
	msg->mSelectResult = -1;
	msg->mSelectedLine = -1;
	msg->mDefaultLine  = 0;
	for (int i = 0; i < 9; i += 3) {
		for (int j = 0; j < 3; j++) {
			msg->mDigits[i + j] = 0;
		}
	}
	msg->mSelectedDigit   = 0;
	msg->mFirstEditDigit  = 0;
	msg->mDigitCount      = 0;
	msg->mStay            = 0;
	msg->mGlyphSoundGroup = -1;
	CMsg_ResetPos(msg);
	msg->mFont.mColor = gMessageWindowDefaultColor;
	for (int i = 0; i < 2; i++) {
		msg->mFont.mCursor[i].vf14C();
	}
	msg->mFont.mMode = mode;
}

/**
 * @note Address: 0x800A5DDC
 * @note Size: 0x8
 */
extern "C" s32 LoadMessageCursorObject(CMsg* msg)
{
	return LoadFontCursors(&msg->mFont);
}

/**
 * @note Address: 0x800A5DE4
 * @note Size: 0xC4
 */
// the GameCube's static ResetMsgState, kept out of line on the Wii: the callers that only
// bring the message back to its idle state call it, CMsg_Open and InitMessageWindow inline it
extern "C" void ResetMsgState(CMsg* msg)
{
	msg->mData.mCursor      = NULL;
	msg->mData.mIconColumns = 0;
	ResetMessageState(&msg->mFont);
	msg->mIdle         = 1;
	msg->mCharDelay    = 0.0f;
	msg->mCharTimer    = 0.0f;
	msg->mSelectResult = -1;
	msg->mSelectedLine = -1;
	msg->mDefaultLine  = 0;
	for (int i = 0; i < 9; i += 3) {
		for (int j = 0; j < 3; j++) {
			msg->mDigits[i + j] = 0;
		}
	}
	msg->mSelectedDigit   = 0;
	msg->mFirstEditDigit  = 0;
	msg->mDigitCount      = 0;
	msg->mStay            = 0;
	msg->mGlyphSoundGroup = -1;
}

/**
 * @note Address: 0x800A5EA8
 * @note Size: 0x8C
 */
extern "C" void CMsg_ResetPos(CMsg* msg)
{
	msg->mWindowX      = (f32)(gPointerScreenWidth / 2);
	msg->mWindowY      = (f32)(gScreenSafeBottom - 0x4A);
	msg->mPlacement    = NULL;
	Vec zero           = { 0.0f, 0.0f, 0.0f };
	msg->mFontPosition = zero;
}

/**
 * @note Address: 0x800A5F34
 * @note Size: 0x290
 */
extern "C" u8* CMsg_Open(CMsg* msg, u8 command, u8* script, MsgFormatArgs* args)
{
	msg->mData.mCursor      = NULL;
	msg->mData.mIconColumns = 0;
	ResetMessageState(&msg->mFont);
	msg->mIdle         = 1;
	msg->mCharDelay    = 0.0f;
	msg->mCharTimer    = 0.0f;
	msg->mSelectResult = -1;
	msg->mSelectedLine = -1;
	msg->mDefaultLine  = 0;
	for (int i = 0; i < 9; i += 3) {
		for (int j = 0; j < 3; j++) {
			msg->mDigits[i + j] = 0;
		}
	}
	msg->mSelectedDigit   = 0;
	msg->mFirstEditDigit  = 0;
	msg->mDigitCount      = 0;
	msg->mStay            = 0;
	msg->mGlyphSoundGroup = -1;
	msg->mCommand         = command;
	s32 savedCount        = args->count;
	CMsgData_Format(msg->mData.mText, script, args);
	CMsgData_Measure(&msg->mData);
	args->count        = savedCount;
	msg->mData.mCursor = msg->mData.mText;
	if (msg->mData.mIconColumns == 0) {
		// how many 24 pixel icons the icon strip holds, CMsgFont_AddChar clamps MSG_CODE_ICON's index to it
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[120], 0);
		CTexObj icon            = gGame.mTpl.mTex;
		msg->mData.mIconColumns = (s32)GXGetTexObjWidth(&icon) / 24;
	}
	u8* cursor = msg->mData.mCursor;
	CMsgFont_Init(&msg->mFont, msg->mData.mText, msg->mWindowX, msg->mWindowY, msg->mPlacement, msg->mData.mAlignment, msg->mData.mStyle);
	msg->SetFontPosition(msg->mFontPosition.x, msg->mFontPosition.y, msg->mFontPosition.z);
	msg->mCharTimer = 0.0f;
	msg->mCharDelay = msg->mCommand == 0x23 ? 2 : 0;
	msg->mWaitTimer = 0.0f;
	return cursor;
}

static inline void MsgNextLine(CMsg* msg)
{
	msg->mFont.mLine++;
	CMsgData* data      = msg->mFont.mData;
	f32 width           = msg->mFont.mBits.mFitLine ? data->mWidths[msg->mFont.mLine] : msg->mFont.mWidth;
	msg->mFont.mCursorX = -(width * 0.5f);
	msg->mFont.mCursorY += msg->mFont.mData->mHeights[msg->mFont.mLine - 1];
	msg->mFont.mLineY[msg->mFont.mLine] = msg->mFont.mCursorY + (msg->mFont.mData->mHeights[msg->mFont.mLine] - 22.0f) * 0.5f;
}

/**
 * @note Address: 0x800A61C4
 * @note Size: 0x5B8
 */
extern "C" void CMsg_Update(CMsg* msg, u32 step)
{
	if (msg->mData.mCursor == NULL) {
		return;
	}
	s32 waiting = FALSE;
	while (!waiting) {
		if (msg->mFont.mBits.mFadeIn) {
			waiting = TRUE;
			continue;
		}
		if (msg->mWaitTimer > 0.0f) {
			f32 time        = msg->mWaitTimer - step;
			msg->mWaitTimer = time;
			if (time < 0.0f) {
				msg->mWaitTimer = 0.0f;
			} else {
				waiting = TRUE;
			}
			continue;
		}
		if (msg->mCharTimer < 0.0f) {
			waiting = TRUE;
			continue;
		}
		if (msg->mCharTimer > 0.0f) {
			f32 time        = msg->mCharTimer - step;
			msg->mCharTimer = time;
			if (time < 0.0f) {
				msg->mCharTimer = 0.0f;
				if (msg->mCharDelay > 0.0f && msg->mCharDelay - step < 0.0f) {
					waiting = TRUE;
				}
			} else {
				waiting = TRUE;
			}
			continue;
		}
		u8 command = ReadMsgU8(msg);
		switch (command) {
		case MSG_CODE_NEWLINE:
			MsgNextLine(msg);
			waiting = FALSE;
			break;
		case MSG_CODE_NEWLINE_VT:
			MsgNextLine(msg);
			waiting = FALSE;
			break;
		case MSG_CODE_END:
			waiting = CMsg__cmd_END(msg);
			break;
		case MSG_CODE_SPEED: {
			s32 delay = ReadMsgU8(msg);
			if (delay == 255) {
				if (msg->mCommand == EVT_CMD_MSG) {
					delay = 2;
				} else {
					delay = 0;
				}
			}
			msg->mCharDelay = delay;
			waiting         = FALSE;
			break;
		}
		case MSG_CODE_WAIT:
			waiting = CMsg__cmd_WAIT(msg);
			break;
		case MSG_CODE_ANIM:
			waiting = CMsg__cmd_ANIM(msg);
			break;
		case MSG_CODE_DEFAULT: {
			u8 flags  = ReadMsgU8(msg);
			s32 value = ReadMsgU8(msg);
			value |= ReadMsgU8(msg) << 8;
			value |= ReadMsgU8(msg) << 16;
			// the Wii index is 24 bits, the GameCube reads 32
			if (flags & 1) {
				value = gEventVars.mVars[value];
			}
			msg->mDefaultLine = value;
			waiting           = FALSE;
			break;
		}
		case MSG_CODE_SFX:
			waiting = CMsg__cmd_SFX(msg);
			break;
		case MSG_CODE_VOICE:
			msg->mGlyphSoundGroup = ReadMsgU8(msg);
			if (msg->mGlyphSoundGroup == 255) {
				msg->mGlyphSoundGroup = -1;
			}
			waiting = FALSE;
			break;
		case 13:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 14:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 15:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 16:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 19:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 18:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 20:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 21:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case MSG_CODE_NUM_INPUT:
			msg->mDigitCount     = ReadMsgU8(msg);
			msg->mFirstEditDigit = ReadMsgU8(msg);
			msg->mFirstEditDigit = msg->mDigitCount - msg->mFirstEditDigit;
			msg->mSelectedDigit  = ReadMsgU8(msg) - 1;
			if (msg->mSelectedDigit > msg->mDigitCount - 1) {
				msg->mSelectedDigit = msg->mDigitCount - 1;
			} else if (msg->mSelectedDigit < msg->mFirstEditDigit) {
				msg->mSelectedDigit = msg->mFirstEditDigit;
			}
			waiting = FALSE;
			break;
		case MSG_CODE_QUESTION:
			waiting = CMsg__cmd_QUESTION(msg);
			break;
		case MSG_CODE_STAY:
			msg->mStay = 1;
			waiting    = FALSE;
			break;
		default: {
			char text[3];
			if (MsgHasSjisLead(command)) {
				text[0] = command;
				text[1] = ReadMsgU8(msg);
				text[2] = 0;
			} else if (command == MSG_CODE_ICON) {
				text[0] = command;
				text[1] = ReadMsgU8(msg);
				text[2] = 0;
			} else {
				text[0] = command;
				text[1] = 0;
			}
			CMsgFont_AddChar(&msg->mFont, text);
			msg->mCharTimer = msg->mCharDelay;
			if (msg->mGlyphSoundGroup >= 0) {
				PlayMessageGlyphSound(&gGame.mSound, msg->mGlyphSoundGroup, msg->mFont.mSize / 22.0f, text[0], text[1]);
			}
			break;
		}
		}
	}
	CMsgFont_Update(&msg->mFont, step);
}

/**
 * @note Address: 0x800A677C
 * @note Size: 0x100
 */
extern "C" void CMsg_Draw(CMsg* msg)
{
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	if (msg->mCharDelay == 0.0f) {
		CMsgFont_Draw(&msg->mFont, 1.0f);
	} else {
		f32 t = (msg->mCharDelay - msg->mCharTimer) / msg->mCharDelay;
		if (t > 1.0f) {
			t = 1.0f;
		}
		if (t < 0.0f) {
			t = 0.0f;
		}
		CMsgFont_Draw(&msg->mFont, t);
	}
}

static inline void MsgSetDigits(CMsg* msg, const s8* digits, s32 count, s32 first, s32 selected)
{
	for (s32 i = 0; i < 9; i += 3) {
		for (s32 j = 0; j < 3; j++) {
			msg->mFont.mDigits[i + j] = digits[i + j];
		}
	}
	msg->mFont.mDigitCount     = count;
	msg->mFont.mFirstEditDigit = first;
	msg->mFont.mOtherSelection = selected;
}

/**
 * @note Address: 0x800A687C
 * @note Size: 0xB44
 */
// the GameCube's select and number input, driven by the remote pointer on the Wii: hovering a
// line tracks mPointerState (0 off, 1 hover, 2 click held), the dpad bits of the pointer's
// flags walk the selection and its A/B bits confirm or cancel. sfx: 0xF3 cancel, 0xF4 move,
// 0xF5 confirm
extern "C" s32 CMsg__cmd_END(CMsg* msg)
{
	if (msg->mFont.mBits.mActive) {
		if (!msg->mFont.mBits.mFadeOut) {
			if (msg->mCommand == EVT_CMD_SELECT) {
				if (msg->mSelectedLine < 0) {
					msg->mSelectedLine       = msg->mDefaultLine;
					msg->mFont.mSelectedLine = msg->mSelectedLine;
					msg->mFont.mCurLineY     = msg->mFont.mLineY[msg->mFont.mSelectedLine];
					msg->mFont.mCurLineWidth = 0.0f;
					msg->mFont.mSelectDone   = 0;
				} else if (msg->mFont.mSelectDone != 0) {
					msg->mSelectedLine        = -1;
					msg->mFont.mBits.mFadeOut = TRUE;
					msg->mFont.mSelectedLine  = -1;
				} else if (!gGame.GetStatusFlags(0x20)) {
					s32 hit = 0;
					if (gWiiPointer.mPointerValid) {
						f32 top = msg->mFont.mWindowY;
						if (top < (f32)gScreenSafeTop) {
							top = (f32)gScreenSafeTop;
						}
						if (top + msg->mFont.mHeight > (f32)gScreenSafeBottom) {
							top = (f32)gScreenSafeBottom - msg->mFont.mHeight;
						}
						f32 margin = 40.0f + msg->mData.mHeights[msg->mData.mWidestLine];
						if (gWiiPointer.mPointer.x >= msg->mFont.mWindowX - msg->mFont.mWidth * 0.5f - margin
						    && gWiiPointer.mPointer.x <= msg->mFont.mWindowX + msg->mFont.mWidth * 0.5f + margin
						    && gWiiPointer.mPointer.y >= top - 8.0f && gWiiPointer.mPointer.y <= top + msg->mFont.mHeight + 8.0f) {
							if (msg->mFont.mPointerState == 2) {
								hit = -1;
							} else {
								for (s32 i = 0; i < msg->mData.mLineCount; i++) {
									s32 row = msg->mSelectedLine >= 0 ? (msg->mSelectedLine + i) % msg->mData.mLineCount : i;
									f32 w   = 24.0f + msg->mData.mMaxWidth;
									if (msg->mFont.mColor.a != 0) {
										w = margin * 2.0f + msg->mFont.mWidth;
									}
									f32 rowTop = top + msg->mFont.mLineY[row];
									f32 h      = msg->mData.mHeights[row];
									if (gWiiPointer.mPointer.x >= msg->mFont.mWindowX - w * 0.5f
									    && gWiiPointer.mPointer.x <= msg->mFont.mWindowX + w * 0.5f
									    && gWiiPointer.mPointer.y >= rowTop - h * 0.25f
									    && gWiiPointer.mPointer.y <= rowTop + h + h * 0.25f) {
										hit = row + 1;
										break;
									}
								}
							}
						}
					}
					if (hit != 0) {
						if (!(gWiiPointer.mButton & ((1ULL << RemoteA) | (1ULL << GameCubeA)))) {
							if (msg->mFont.mPointerState == 0) {
								msg->mFont.mPointerState = 1;
								PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
							}
							if (hit > 0 && hit - 1 != msg->mSelectedLine) {
								msg->mSelectedLine       = hit - 1;
								msg->mFont.mSelectedLine = msg->mSelectedLine;
								PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
							}
						}
					} else {
						msg->mFont.mPointerState = 0;
					}
					if (gWiiPointer.Check(4)) { // the remote's up moves the highlight
						if (msg->mFont.mPointerState == 0) {
							if (--msg->mSelectedLine < 0) {
								msg->mSelectedLine = msg->mData.mLineCount - 1;
							}
							msg->mFont.mSelectedLine = msg->mSelectedLine;
							PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
						}
					} else if (gWiiPointer.Check(8)) { // down
						if (msg->mFont.mPointerState == 0) {
							if (++msg->mSelectedLine >= msg->mData.mLineCount) {
								msg->mSelectedLine = 0;
							}
							msg->mFont.mSelectedLine = msg->mSelectedLine;
							PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
						}
					} else if (gWiiPointer.Check(1)) { // A
						if (msg->mFont.mPointerState != 0) {
							msg->mFont.mPointerState = 2;
							PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
						} else {
							msg->mSelectResult        = msg->mSelectedLine;
							msg->mFont.mPointerState  = 1;
							msg->mFont.mSelectDone    = 1;
							msg->mSelectedLine        = -1;
							msg->mFont.mSelectedLine  = -1;
							msg->mFont.mBits.mFadeOut = TRUE;
							PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
						}
					} else if (gWiiPointer.Check(2)) { // B
						msg->mSelectResult        = -1;
						msg->mSelectedLine        = -1;
						msg->mFont.mSelectedLine  = -1;
						msg->mFont.mPointerState  = 0;
						msg->mFont.mSelectDone    = 1;
						msg->mFont.mBits.mFadeOut = TRUE;
						PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
					} else if (msg->mFont.mPointerState == 2 && !(gWiiPointer.mButton & ((1ULL << RemoteA) | (1ULL << GameCubeA)))) {
						msg->mSelectResult       = msg->mSelectedLine;
						msg->mFont.mSelectDone   = 1;
						msg->mFont.mPointerState = 0;
						PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
					}
				}
			} else if (msg->mDigitCount > 0) {
				if (msg->mFont.mOtherSelection < 0) {
					MsgSetDigits(msg, msg->mDigits, msg->mDigitCount, msg->mFirstEditDigit, msg->mSelectedDigit);
				} else if (gWiiPointer.Check(4)) {
					if (++msg->mDigits[msg->mSelectedDigit] > 9) {
						msg->mDigits[msg->mSelectedDigit] = 0;
					}
					MsgSetDigits(msg, msg->mDigits, msg->mDigitCount, msg->mFirstEditDigit, msg->mSelectedDigit);
					PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
				} else if (gWiiPointer.Check(8)) {
					if (--msg->mDigits[msg->mSelectedDigit] < 0) {
						msg->mDigits[msg->mSelectedDigit] = 9;
					}
					MsgSetDigits(msg, msg->mDigits, msg->mDigitCount, msg->mFirstEditDigit, msg->mSelectedDigit);
					PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
				} else if (gWiiPointer.Check(0x10)) {
					if (++msg->mSelectedDigit > msg->mDigitCount - 1) {
						msg->mSelectedDigit = msg->mDigitCount - 1;
					} else {
						MsgSetDigits(msg, msg->mDigits, msg->mDigitCount, msg->mFirstEditDigit, msg->mSelectedDigit);
						PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
					}
				} else if (gWiiPointer.Check(0x20)) {
					if (--msg->mSelectedDigit < msg->mFirstEditDigit) {
						msg->mSelectedDigit = msg->mFirstEditDigit;
					} else {
						MsgSetDigits(msg, msg->mDigits, msg->mDigitCount, msg->mFirstEditDigit, msg->mSelectedDigit);
						PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
					}
				} else if (gWiiPointer.Check(1)) {
					msg->mFont.mBits.mFadeOut  = TRUE;
					msg->mFont.mOtherSelection = -1;
					PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
					s32 result = 0;
					for (s32 i = 9; i >= 0; i--) {
						result = result * 10 + msg->mDigits[i];
					}
					gGame.mSelectResult = result;
				} else if (gWiiPointer.Check(2)) {
					msg->mFont.mBits.mFadeOut  = TRUE;
					msg->mFont.mOtherSelection = -1;
					PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
					gGame.mSelectResult = -1;
				}
			} else if (msg->mStay != 0) {
				if (msg->mStay == 1) {
					if (gGame.GetPlayer()->mOXMenu.mStarted < 0) {
						msg->mStay++;
					}
				} else {
					msg->mFont.mBits.mFadeOut = TRUE;
				}
			} else {
				msg->mFont.mBits.mFadeOut = TRUE;
			}
		}
		msg->mData.mCursor--;
	} else {
		msg->mData.mCursor      = NULL;
		msg->mData.mIconColumns = 0;
	}
	return 1;
}

/**
 * @note Address: 0x800A73C0
 * @note Size: 0x100
 */
extern "C" s32 CMsg__cmd_QUESTION(CMsg* msg)
{
	u32 crossMask          = ReadMsgU8(msg);
	u32 selectedIndex      = ReadMsgU8(msg);
	WiiPlayerState* player = gGame.GetPlayer();
	if (player->mOXMenu.mStarted == 0) {
		OpenPlayerYesNoMenu(player, crossMask, selectedIndex);
		msg->mData.mCursor -= 3;
		return TRUE;
	}
	CPad pad(0);
	HandleOXMenuInput(&player->mOXMenu, &pad);
	if (gWiiPointer.Check(1)) {
		gGame.mSelectResult = gGame.GetPlayer()->mOXMenu.mConfirmedIndex;
		return FALSE;
	}
	msg->mData.mCursor -= 3;
	return TRUE;
}

/**
 * @note Address: 0x800A74C0
 * @note Size: 0x2C4
 */
extern "C" s32 CMsg__cmd_WAIT(CMsg* msg)
{
	s32 command = ReadMsgU8(msg);
	// 252 to 255 wait for a button instead of a time
	if (!(command != 255 && command != 254 && command != 253 && command != 252)) {
		if (msg->mCommand == EVT_CMD_MSG) {
			if (!msg->mFont.mBits.mBlink) {
				s32 icon = -1;
				if (command == 255) {
					icon = 0;
				}
				if (command == 254) {
					icon = 1;
				}
				if (command == 253 || command == 252) {
					icon = 0;
				}
				msg->mFont.mBits.mBlink = TRUE;
				msg->mFont.mBlinkIcon   = icon;
				msg->mFont.mBlinkPhase  = 0.0f;
				msg->mData.mCursor -= 2;
			} else {
				BOOL done = FALSE;
				if (command == 253 || command == 252) {
					// NONMATCHING: retail keeps two cmpwi here while MWCC folds the || into a
					// (command - 252) <= 1 range check; behavior identical. bit 50 of the
					// trigger word is the GameCube pad's X
					if ((gWiiPointer.mTrigger & (1ULL << GameCubeX)) && !gGame.GetStatusFlags(0x20)) {
						done = TRUE;
						if (command == 253) {
							SelectPlayerEquipmentRing(gGame.GetPlayer(), 0);
						} else if (command == 252) {
							SelectPlayerEquipmentRing(gGame.GetPlayer(), 1);
						}
					}
				} else if ((gWiiPointer.Check(1) || gWiiPointer.Check(2)) && !gGame.GetStatusFlags(0x20)) {
					done = TRUE;
				}
				if (done) {
					msg->mFont.mBits.mBlink = FALSE;
					ClearGameSoundQueue(&gGame.mSound, msg->mGlyphSoundGroup);
				} else {
					msg->mData.mCursor -= 2;
				}
			}
		}
		return msg->mFont.mBits.mBlink;
	}
	// tenths of a second
	f32 frames = (command / 10) * (f32)gRefreshRate;
	frames += (command % 10) * (f32)gRefreshRate / 10.0f;
	msg->mWaitTimer = frames;
	return 0;
}

/**
 * @note Address: 0x800A7784
 * @note Size: 0x2E4
 */
// the GameCube's shape with a Wii queue: MSG_ANIM_QUEUE drops the anim into a four slot ring
// behind the playing one, otherwise vf108 (the Wii's SetAnim) starts it right away
extern "C" s32 CMsg__cmd_ANIM(CMsg* msg)
{
	u8 flags = ReadMsgU8(msg);
	s16 id   = ReadMsgS16(msg);
	s32 anim = ReadMsgU8(msg);
	anim |= ReadMsgU8(msg) << 8;
	anim |= ReadMsgU8(msg) << 16;
	anim |= ReadMsgU8(msg) << 24;
	if (flags & MSG_ANIM_VARIABLE) {
		anim = gEventVars.mVars[anim];
	}
	WiiTitleObject* obj = FindObj(id);
	if (obj != NULL && (obj->mStatus & 1)) {
		if (anim == -1) {
			if (!obj->vf130((f32)gGame.mRetraceStep)) {
				msg->mData.mCursor -= 8;
				return 1;
			}
		} else if (!(flags & MSG_ANIM_QUEUE)) {
			obj->ClearAnmQueue();
			obj->vf108(anim, 0, 0.0f, 1.0f, 0.0f, 15.0f);
			obj->vf118(1.0f);
		} else {
			obj->PushAnim(anim, 0, 100);
		}
	}
	return 0;
}

/**
 * @note Address: 0x800A7A68
 * @note Size: 0x44C
 */
extern "C" s32 CMsg__cmd_SFX(CMsg* msg)
{
	s32 sfx = ReadMsgU8(msg);
	sfx |= ReadMsgU8(msg) << 8;
	sfx |= ReadMsgU8(msg) << 16;
	sfx |= ReadMsgU8(msg) << 24;
	s8 command = ReadMsgU8(msg);
	switch (command) {
	case 0:
		if (sfx == -2) {
			// stop every playing track at once (0 seconds of fade)
			// retail computes the frame count at runtime (sdata2 0.0f multiply
			// is not folded there); the value is 0 either way
			u32 frames = gRefreshRate * 0.0f;
			for (int i = 0; i < 0x40; ++i) {
				nw4r::snd::detail::BasicSound* track = gGame.mSound.mTracks[i].detail_GetAttachedSound();
				if (track != NULL)
					track->Stop(frames);
			}
		} else {
			StopWiiSfx(&gGame.mSound, sfx, 0.0f);
		}
		break;
	case 1:
		PlayWiiSoundEffect(&gGame.mSound, sfx, 1.0f, 0.0f);
		break;
	case 2: {
		u16 frames = ReadMsgS16(msg);
		StopWiiSfx(&gGame.mSound, sfx, frames / 100.0f);
		break;
	}
	case 3: {
		u16 frames = ReadMsgS16(msg);
		PlayWiiSoundEffect(&gGame.mSound, sfx, 1.0f, frames / 100.0f);
		break;
	}
	case 4: {
		u16 frames = ReadMsgS16(msg);
		u8 volume  = ReadMsgU8(msg);
		if (!IsWiiSfxPlaying(&gGame.mSound, sfx)) {
			PlayWiiSoundEffect(&gGame.mSound, sfx, volume / 255.0f, frames / 100.0f);
		} else {
			SetWiiSfxVolume(&gGame.mSound, sfx, volume / 255.0f, frames / 100.0f);
		}
		break;
	}
	case 5:
		if (sfx == -2) {
			u32 frames = (u32)((f32)gRefreshRate * 0.0f);
			for (int i = 0; i < 0x40; ++i) {
				nw4r::snd::detail::BasicSound* track = gGame.mSound.mTracks[i].detail_GetAttachedSound();
				if (track != NULL && !track->IsPause())
					track->Pause(true, frames);
			}
		} else {
			PauseWiiSfx(&gGame.mSound, sfx, 0.0f);
		}
		break;
	case 6:
		if (sfx == -2) {
			u32 frames = (u32)((f32)gRefreshRate * 0.0f);
			for (int i = 0; i < 0x40; ++i) {
				nw4r::snd::detail::BasicSound* track = gGame.mSound.mTracks[i].detail_GetAttachedSound();
				if (track != NULL && track->IsPause())
					track->Pause(false, frames);
			}
		} else {
			ResumeWiiSfx(&gGame.mSound, sfx, 0.0f);
		}
		break;
	case MSG_SFX_WAIT:
		if (IsWiiSfxPlaying(&gGame.mSound, sfx)) {
			// the Wii script is wider than the GameCube's: code, u32 id and
			// type are 6 bytes, so 6 lands back on the command
			msg->mData.mCursor -= 6;
			return 1;
		}
		break;
	}
	return 0;
}

/**
 * @note Address: 0x800A7ED4
 * @note Size: 0x290
 */
extern "C" s32 CMsg__cmd_SETFONT(CMsg* msg, u8 command)
{
	switch (command) {
	case 13: {
		u8 size = ReadMsgU8(msg);
		if (size == 255) {
			size = 22;
		}
		msg->mFont.mSize = size;
		break;
	}
	case 14: {
		u8 color = ReadMsgU8(msg);
		if (color == 255) {
			color = 0;
		}
		msg->mFont.mGlyphColor = msgColorTable[color];
		break;
	}
	case 15: {
		u32 r                  = ReadMsgU8(msg);
		u32 g                  = ReadMsgU8(msg);
		u32 b                  = ReadMsgU8(msg);
		u32 a                  = ReadMsgU8(msg);
		msg->mFont.mGlyphColor = (r << 24) | (g << 16) | (b << 8) | a;
		break;
	}
	case 16:
		msg->mFont.mBits.mLayout = ReadMsgU8(msg);
		break;
	case 19:
		msg->mFont.mBits.mCenter = ReadMsgU8(msg);
		break;
	case 18: {
		u32 a              = ReadMsgU8(msg);
		u32 b              = ReadMsgU8(msg);
		u32 c              = ReadMsgU8(msg);
		msg->mFont.mEffect = a;
		msg->mFont.mAmount = b;
		msg->mFont.mSpeed  = c;
		break;
	}
	case 20:
		msg->mFont.mGlyphAngle = DEG2RAD * ReadMsgS16(msg);
		break;
	case 21: {
		f32 x                   = ReadMsgS16(msg) / 100.0f;
		f32 y                   = ReadMsgS16(msg) / 100.0f;
		msg->mFont.mGlyphScaleX = x;
		msg->mFont.mGlyphScaleY = y;
		break;
	}
	default:
		OSPanic("msg.cpp", 2269, "%s%s", FormatToStaticBuffer("Unknown msg-setfont cmd - %d\n", command), "_cmd_SETFONT");
		break;
	}
	return 0;
}

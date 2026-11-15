#ifndef _GAME_WII_MSG_H
#define _GAME_WII_MSG_H

#include "types.h"
#include "game/ctpl.h"
#include "game/cprimitive.h"
#include "game/wii/title.h"

// control codes inside message text. the enum is the GameCube msg.h's, checked against the
// jump table of the Wii's MsgCommandLength at 0x802FFAF8 (25 entries, codes 0..24)
enum MsgCode {
	MSG_CODE_END        = 0,  // no args. also where select, number input and stay wait for the player
	MSG_CODE_SPEED      = 1,  // u8 frames per char, 0xFF resets, to 2 in msg but 0 in select
	MSG_CODE_WAIT       = 2,  // u8 tenths of a second, 0xFC to 0xFF wait for a button instead
	MSG_CODE_ANIM       = 3,  // u8 flags, s16 obj, s32 anim
	MSG_CODE_SFX        = 4,  // u32 id, s8 MsgSfxType, then 0, 2 or 3 more bytes
	MSG_CODE_VOICE      = 5,  // u8 voice, 0..33, 0xFF for none
	MSG_CODE_DEFAULT    = 6,  // u8 flags, s32 index, the cursor select() starts on
	MSG_CODE_NEWLINE    = 10, // no args
	MSG_CODE_NEWLINE_VT = 11, // no args, same code as NEWLINE
	MSG_CODE_FORMAT     = 12, // printf spec up to the next 12, expanded when the message opens
	MSG_CODE_SIZE       = 13, // u8, 0xFF resets to 22
	MSG_CODE_COLOR      = 14, // u8 index into the 10 color presets, 0xFF resets to white
	MSG_CODE_RGBA       = 15, // u32 big endian
	MSG_CODE_LAYOUT     = 16, // u8, bit 0
	MSG_CODE_ICON       = 17, // u8 icon, drawn as a glyph, not a SETFONT code
	MSG_CODE_SHAKE      = 18, // u8 flags, u8 strength, u8 speed
	MSG_CODE_CENTER     = 19, // u8, bit 0, on by default
	MSG_CODE_ROTATE     = 20, // s16 degrees
	MSG_CODE_SCALE      = 21, // s16 x, s16 y, percent
	MSG_CODE_NUM_INPUT  = 22, // u8 digits, u8 editable, u8 selected
	MSG_CODE_QUESTION   = 23, // u8 flags, u8 default
	MSG_CODE_STAY       = 24, // no args, leaves the window up after msg returns
};

// the msg anim code's flag bits
enum MsgAnimFlag {
	MSG_ANIM_VARIABLE = 0x01, // anim is a script variable index
	MSG_ANIM_QUEUE    = 0x10, // queue it behind the current one
};

// the sfx subcommand of MSG_CODE_SFX, one byte at p[5]
enum MsgSfxType {
	MSG_SFX_WAIT     = -1, // no extra bytes
	MSG_SFX_STOP     = 0,  // no extra bytes
	MSG_SFX_PLAY     = 1,  // no extra bytes
	MSG_SFX_FADE_OUT = 2,  // u16 frames
	MSG_SFX_FADE_IN  = 3,  // u16 frames
	MSG_SFX_FADE     = 4,  // u16 frames, u8 volume
	MSG_SFX_PAUSE    = 5,  // no extra bytes
	MSG_SFX_RESUME   = 6,  // no extra bytes
};

// the formatted message plus what CMsgData_Measure finds out about its ten lines.
// compared to the GameCube class it remembers the widest line's index and carries a
// second icon column member
class CMsgData {
public:
	u8 mText[0x800];  // _00
	u8* mCursor;      // _800
	s32 mLineCount;   // _804
	s32 mCounts[10];  // _808
	f32 mWidths[10];  // _830
	f32 mHeights[10]; // _858
	s32 mWidestLine;  // _880, index of the widest line
	f32 mMaxWidth;    // _884
	f32 mMaxHeight;   // _888
	f32 mTotalHeight; // _88C
	s32 mStyle;       // _890
	s32 mAlignment;   // _894
	s32 mIconColumns; // _898
	s32 _89C;         // _89C
	// _8A0 vtable

	CMsgData()
	{
		mCursor      = NULL;
		mIconColumns = 0;
		_89C         = 0;
	}

	virtual ~CMsgData() { } // _08
};
STATIC_ASSERT(sizeof(CMsgData) == 0x8A4);

// one character on screen, the GameCube's MsgFontGlyph with the Wii CTexObj inside
struct MsgFontGlyph {
	MsgFontGlyph() { }
	CTexObj mTexture; // _00
	CSprite mSprite;  // _104
	u32 mColor;       // _28C
	s32 mLine;        // _290
	f32 mAngle;       // _294
	f32 mScaleX;      // _298
	f32 mScaleY;      // _29C
	f32 mWidthAdjust; // _2A0, for rotated glyphs
	u8 mEffect;       // _2A4
	u8 mAmount;       // _2A5
	s16 mPhaseX;      // _2A6
	s16 mPhaseY;      // _2A8
	s16 mPhaseZ;      // _2AA
	s16 mPhaseW;      // _2AC
	s8 mSpeed;        // _2AE
	s8 mRefresh;      // _2AF
};
STATIC_ASSERT(sizeof(MsgFontGlyph) == 0x2B0);

// CMsgFont's textures besides the window and the glyphs, by MsgFontTex
struct MsgTexSet {
	CTexObj mTex[5]; // _00
};

enum MsgFontTex {
	MSG_TEX_CURSOR = 0,
	MSG_TEX_DIGIT  = 1,
	MSG_TEX_ICON   = 2,
	MSG_TEX_ARROW  = 3,
	MSG_TEX_LINE   = 4,
};

// the window's own texture
struct MsgFrame {
	CTexObj mTex; // _00
};

// the text window. the Wii port keeps two of the GameCube's cursor objects (the second
// LoadMessageCursorObject call uses spawn 0x2C) and moved to a u32 flag word
class CMsgFont {
public:
	CMsgData* mData; // _00
	// the GameCube's u8 flag byte as a u32 word: ResetMessageState writes it as a mask,
	// CMsgFont_Init writes the flags bit by bit
	union {
		u32 mFlags; // _04
		struct {
			u32 mActive : 1;  // _04.0
			u32 mFadeIn : 1;  // _04.1
			u32 mFadeOut : 1; // _04.2
			u32 mBlink : 1;   // _04.3
			u32 mFitLine : 1; // _04.4
			u32 _04_04 : 1;   // _04.5
			u32 _04_03 : 22;  // _04.6
			u32 mCenter : 1;  // _04.28, MSG_CODE_CENTER
			u32 mLayout : 1;  // _04.29, MSG_CODE_LAYOUT
			u32 _04_30 : 2;   // _04.30
		} mBits;
	};
	MsgFrame mFrame;           // _08
	f32 mScrollX;              // _10C
	f32 mScrollY;              // _110
	f32 mScaleX;               // _114
	f32 mScaleY;               // _118
	GXColor mColor;            // _11C, the window tint CMsg::SetColor writes
	WiiTitleObject mCursor[2]; // _120, _AD0
	s32 mMode;                 // _1480
	u32 _1484;                 // _1484, suppresses the portrait while set (draw skips it, Init's wipe scale ignores it)
	MsgFontGlyph mGlyphs[400]; // _1488
	MsgTexSet mTexs;           // _44788
	u32 mBlinkIcon;  // _44C9C, which prompt the blinking button wait shows: -1 none, 0 and 1 index mTexs from MSG_TEX_CURSOR, 2 and up a
	                 // frame (value - 2) of an 11-frame strip; set by MSG_CODE_WAIT 0xFC..0xFF together with mBlink and mBlinkPhase
	f32 mBlinkPhase; // _44CA0
	f32 mTexOffset;  // _44CA4
	s32 mCharCount;  // _44CA8
	s32 mLine;       // _44CAC
	WiiTitleObject* mPlacement; // _44CB0, an object to follow
	Vec mPosition;              // _44CB4
	f32 mCursorX;               // _44CC0, current line start x
	f32 mWindowX;               // _44CC4
	f32 mWidth;                 // _44CC8, full message width
	f32 mCursorY;               // _44CCC, accumulated line heights
	f32 mWindowY;               // _44CD0
	f32 mHeight;                // _44CD4
	f32 mLineY[10];             // _44CD8
	f32 mSize;                  // _44D00
	f32 mPrevSize;              // _44D04
	u32 mGlyphColor;            // _44D08, rgba out of the color table
	f32 mGlyphAngle;            // _44D0C
	f32 mGlyphScaleX;           // _44D10
	f32 mGlyphScaleY;           // _44D14
	u8 mEffect;                 // _44D18
	u8 mAmount;                 // _44D19
	s8 mSpeed;                  // _44D1A
	u8 _44D1B;                  // _44D1B
	s32 mSelectedLine;          // _44D1C
	f32 mCurLineY;              // _44D20
	f32 mCurLineWidth;          // _44D24
	s32 mPointerState;          // _44D28, the pointer's select state: 0 off, 1 hovering a line, 2 click held
	s32 mSelectDone;            // _44D2C, a pointer confirm or cancel landed; the next frame fades out
	s8 mDigits[9];              // _44D30, the number input's displayed digits, MsgSetDigits copies them in
	u8 _44D39[0x44D3C - 0x44D39];
	s32 mDigitCount;     // _44D3C
	s32 mFirstEditDigit; // _44D40
	s32 mOtherSelection; // _44D44, the digit under edit, -1 before MsgSetDigits arms it
	f32 mAlpha;          // _44D48
	f32 mTextAlpha;      // _44D4C
	f32 mHighlightAlpha; // _44D50
	// _44D54 vtable

	CMsgFont()
	{
		mPlacement      = NULL;
		mSelectedLine   = -1;
		mOtherSelection = -1;
	}

	virtual ~CMsgFont() { } // _08
};
STATIC_ASSERT(sizeof(CMsgFont) == 0x44D58);

// the message window
class CMsg {
public:
	CMsg() { }
	u8 mCommand;                  // _00, message command
	u8 _01[0x4 - 0x1];            // _01
	CMsgData mData;               // _04
	CMsgFont mFont;               // _8A8
	s32 mIdle;                    // _45600
	f32 mCharTimer;               // _45604, frames left before the next character is added
	f32 mCharDelay;               // _45608, frames per character from MSG_CODE_SPEED; 0xFF resets it
	f32 mWaitTimer;               // _4560C, wait timer in frames
	s32 mSelectedLine;            // _45610, the line the select() cursor is on, -1 when no select runs
	s32 mSelectResult;            // _45614, the select() answer
	s32 mDefaultLine;             // _45618, the line select() starts on
	s8 mDigits[10];               // _4561C, three displayed groups, the tenth folds into the answer
	u8 _45626[0x45628 - 0x45626]; // _45626
	s32 mDigitCount;              // _45628, digits of the number input
	s32 mFirstEditDigit;          // _4562C, first digit the player may change
	s32 mSelectedDigit;           // _45630, the digit the number input cursor is on
	u8 _45634[0x45638 - 0x45634]; // _45634
	s32 mStay;                    // _45638, 1 after MSG_CODE_STAY keeps the window up after msg returns
	s32 mGlyphSoundGroup;         // _4563C, -1 disables glyph sounds
	f32 mWindowX;                 // _45640
	f32 mWindowY;                 // _45644
	WiiTitleObject* mPlacement;   // _45648, the object the window follows
	Vec mFontPosition;            // _4564C

	// _45658 vtable
	virtual ~CMsg() { } // _08

	u32 _4565C; // _4565C, after the vtable, never read or written

	void SetWindowAnchor(WiiTitleObject* object, f32 x, f32 y, f32 z)
	{
		mPlacement      = object;
		mFontPosition.x = x;
		mFontPosition.y = y;
		mFontPosition.z = z;
	}

	void SetWindowPosition(f32 x, f32 y)
	{
		mWindowX = x;
		mWindowY = y;
		SetWindowAnchor(NULL, 0.0f, 0.0f, 0.0f);
	}

	void SetColor(GXColor color) { mFont.mColor = color; }

	void SetFontPosition(f32 x, f32 y, f32 z)
	{
		mFont.mPosition.x = x;
		mFont.mPosition.y = y;
		mFont.mPosition.z = z;
	}
};
STATIC_ASSERT(sizeof(CMsg) == 0x45660);

// the values a message's printf codes take; the Wii keeps an extra field between the
// pointer and the count, CMsg_Open reads and restores the count around the format
struct MsgFormatArgs {
	s32* values; // _00
	s32 _04;     // _04
	s32 count;   // _08
};

// the window's tint, in .sdata
extern "C" {
extern GXColor gMessageWindowDefaultColor;

void CMsg_ResetPos(CMsg* msg);
u8* CMsg_Open(CMsg* msg, u8 command, u8* script, MsgFormatArgs* args);
}

#endif

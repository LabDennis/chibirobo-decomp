#ifndef _GAME_MSG_H
#define _GAME_MSG_H

#include "types.h"
#include "Version.h"
#include "game/ctpl.h"
#include "game/cprimitive.h"
#include "game/title.h"

class XObj;

// control codes inside message text, what msg() and select() in the stage scripts carry.
// values and argument sizes checked against the parser's jump table in CMsg_Update and
// the measure and copy passes, names follow unplug, see NOTICE. ints are little
// endian, RGBA is the one big endian arg
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

// the sfx code's sub type, the same switch as the sfx() script command, so 5 and 6 are
// EVT_SFX_PAUSE and EVT_SFX_RESUME. -1 is msg only and rewinds 3 bytes when the code is 6,
// looks like a bug, nobody has checked if the shipped text ever uses it
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

// the formatted message plus what CMsgData_Measure finds out about its ten lines, name from RTTI
class CMsgData {
public:
	u8 mText[0x800];  // _00
	u8* mCursor;      // _800
	s32 mLineCount;   // _804
	s32 mCounts[10];  // _808
	f32 mWidths[10];  // _830
	f32 mHeights[10]; // _858
	f32 mMaxWidth;    // _880
	f32 mMaxHeight;   // _884
	f32 mTotalHeight; // _888
	s32 mStyle;       // _88C
	s32 mAlignment;   // _890
#if !defined(VERSION_JP)
	s32 mIconColumns; // _894
#endif
	// _898 vtable, _894 in japan

	CMsgData()
	{
		mCursor = NULL;
#if !defined(VERSION_JP)
		mIconColumns = 0;
#endif
	}

	virtual ~CMsgData() { } // _08
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(CMsgData) == 0x898);
#else
STATIC_ASSERT(sizeof(CMsgData) == 0x89C);
#endif

// one character on screen. japan has no width adjustment, so it's 4 shorter from _29C on
struct MsgFontGlyph {
	MsgFontGlyph() { }
	CTexObj mTexture; // _00
	CSprite mSprite;  // _100
	u32 mColor;       // _288
	s32 mLine;        // _28C
	f32 mAngle;       // _290
	f32 mScaleX;      // _294
	f32 mScaleY;      // _298
#if !defined(VERSION_JP)
	f32 mWidthAdjust; // _29C, for rotated glyphs
#endif
	u8 mEffect;  // _2A0
	u8 mAmount;  // _2A1
	s16 mPhaseX; // _2A2
	s16 mPhaseY; // _2A4
	s16 mPhaseZ; // _2A6
	s16 mPhaseW; // _2A8
	s8 mSpeed;   // _2AA
	s8 mRefresh; // _2AB
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(MsgFontGlyph) == 0x2A8);
#else
STATIC_ASSERT(sizeof(MsgFontGlyph) == 0x2AC);
#endif

// the slot CMsgFont keeps its Cursor in
struct MsgCursor {
	MsgCursor();

	Cursor mCursor; // _00
};

// the window's own texture
struct MsgFrame {
	CTexObj mTex; // _00
};

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

// the text window, name from RTTI. offsets are US, japan's glyphs start 0x10 earlier and
// everything after them is 0x650 earlier
class CMsgFont {
public:
	CMsgData* mData;           // _00
	u8 mActive : 1;            // _04
	u8 mFadeIn : 1;            // _04
	u8 mFadeOut : 1;           // _04
	u8 mBlink : 1;             // _04
	u8 mFitLine : 1;           // _04
	u8 _04_04 : 1;             // _04
	u8 _04_03 : 2;             // _04
	u8 _05[0x8 - 0x5];         // _05
	MsgFrame mFrame;           // _08
	f32 mScrollX;              // _108
	f32 mScrollY;              // _10C
	f32 mScaleX;               // _110
	f32 mScaleY;               // _114
	GXColor mColor;            // _118, window tint
	u8 _11C[0x120 - 0x11C];    // _11C
	MsgCursor mCursor[1];      // _120, the portrait
	s32 mMode;                 // _AA8, _A98 in japan
	MsgFontGlyph mGlyphs[400]; // _AAC
	MsgTexSet mTexs;           // _4376C
	u32 mBlinkIcon;   // _43C6C, which prompt the blinking button wait shows: -1 none, 0 and 1 index mTexs from MSG_TEX_CURSOR, 2 and up a
	                  // frame (value - 2) of an 11-frame strip; set by MSG_CODE_WAIT 0xFC..0xFF together with mBlink and mBlinkPhase
	f32 mBlinkPhase;  // _43C70
	f32 mTexOffset;   // _43C74
	s32 mCharCount;   // _43C78
	s32 mLine;        // _43C7C
	XObj* mPlacement; // _43C80, an XObj to follow
	Vec mPosition;    // _43C84
	f32 mCursorX;     // _43C90, current line start x
	f32 mWindowX;     // _43C94
	f32 mWidth;       // _43C98, full message width
	f32 mCursorY;     // _43C9C, accumulated line heights
	f32 mWindowY;     // _43CA0
	f32 mHeight;      // _43CA4
	f32 mLineY[10];   // _43CA8
	f32 mSize;        // _43CD0
	f32 mPrevSize;    // _43CD4
	u32 mGlyphColor;  // _43CD8, rgba out of msgColorTable
	f32 mGlyphAngle;  // _43CDC
	f32 mGlyphScaleX; // _43CE0
	f32 mGlyphScaleY; // _43CE4
	u8 mEffect;       // _43CE8
	u8 mAmount;       // _43CE9
	s8 mSpeed;        // _43CEA
	u8 _43CEB; // _43CEB, byte after mSpeed, never read or written (AddChar copies mSpeed into the glyph's mSpeed and mRefresh, not this)
	s32 mSelectedLine;            // _43CEC
	f32 mCurLineY;                // _43CF0
	f32 mCurLineWidth;            // _43CF4
	s8 mDigits[9];                // _43CF8, displayed digit groups
	u8 _43D01[0x43D04 - 0x43D01]; // _43D01
	s32 mDigitCount;              // _43D04
	s32 mFirstEditDigit;          // _43D08
	s32 mOtherSelection;          // _43D0C
	f32 mAlpha;                   // _43D10
	f32 mTextAlpha;               // _43D14
	f32 mHighlightAlpha;          // _43D18
	// _43D1C vtable

	CMsgFont()
	{
		mPlacement      = NULL;
		mSelectedLine   = -1;
		mOtherSelection = -1;
	}

	virtual ~CMsgFont() { } // _08
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(CMsgFont) == 0x436D0);
#else
STATIC_ASSERT(sizeof(CMsgFont) == 0x43D20);
#endif

// the message window, name from RTTI. offsets are US, japan's are 0x650 lower from the font's glyphs on
class CMsg {
public:
	CMsg() { }
	u8 mCommand;       // _00, message command
	u8 _01[0x4 - 0x1]; // _01
	CMsgData mData;    // _04
#if defined(VERSION_JP)
	u8 _89C[0x8A0 - 0x89C]; // _89C
#endif
	CMsgFont mFont; // _8A0
	s32 mIdle; // _445C0, TRUE while no script message is open: 1 at reset, CEvt::_cmd_MSG opens a message only when it is set and clears
	           // it, sets it again once mData.mCursor is NULL; CEvt_End and DETACH raise it for a STAYed window
	f32 mCharTimer; // _445C4, frames left before the next character is added; reloaded from mCharDelay after each glyph, counted down by
	                // step in CMsg_Update; CMsg_Draw fades the newest glyph in with (mCharDelay - mCharTimer) / mCharDelay
	f32 mCharDelay; // _445C8, frames per character from MSG_CODE_SPEED; 0xFF resets it to 2 for msg (command 0x23), 0 otherwise; CMsg_Open
	                // sets the same default
	f32 mWaitTimer; // _445CC, wait timer in frames
	s32 mSelectedLine; // _445D0, the line the select() cursor is on, -1 when no select runs; copied to mFont.mSelectedLine every change,
	                   // same value there
	s32 mSelectResult; // _445D4, the select() answer: chosen line on A, -1 on B; CEvt::_cmd_MSG copies it into CEvt::mSelectResult for
	                   // SELECT
	s32 mDefaultLine; // _445D8, the line select() starts on, from MSG_CODE_DEFAULT (s32, or a script variable index when flag bit 0 is set)
	s8 mDigits[10];   // _445DC, three displayed groups; END also reads the tenth byte
	u8 _445E6[0x445E8 - 0x445E6]; // _445E6
	s32 mDigitCount;     // _445E8, digits of the number input from MSG_CODE_NUM_INPUT, 0 when there is none; copied to mFont.mDigitCount
	s32 mFirstEditDigit; // _445EC, first digit the player may change: digit count minus the code's editable count; copied to
	                     // mFont.mFirstEditDigit
	s32 mSelectedDigit;  // _445F0, the digit the number input cursor is on, clamped to mFirstEditDigit..mDigitCount-1; passed to
	                     // mFont.mOtherSelection via MsgSetDigits
	u8 _445F4[0x445F8 - 0x445F4]; // _445F4
	s32 mStay; // _445F8, 1 after MSG_CODE_STAY keeps the window up after msg returns; the next MSG (or the yes/no menu ending) moves it to
	           // 2 and END then fades the window out; 0 otherwise
	s32 mGlyphSoundGroup; // _445FC, -1 disables glyph sounds
	f32 mWindowX;         // _44600, 320.0f at reset, maybe a screen x
	f32 mWindowY;         // _44604, 344.0f at reset, maybe a screen y
	XObj* mPlacement;     // _44608, the object the window follows, NULL for a fixed screen position; stored in
	                      // mFont.mPlacement by CMsgFont_Init, WIN @OBJ sets it
	Vec mFontPosition;    // _4460C

	// _44618 vtable

	virtual ~CMsg() { } // _08

	u32 _4461C; // _4461C, after the vtable, never read or written

	void SetColor(GXColor color) { mFont.mColor = color; }

	void SetFontPosition(f32 x, f32 y, f32 z)
	{
		mFont.mPosition.x = x;
		mFont.mPosition.y = y;
		mFont.mPosition.z = z;
	}
};

// the values a message's printf codes take, CEvt keeps one at _44
struct MsgFormatArgs {
	s32* values; // _00
	s32 count;   // _04
};

extern "C" {
void CMsg_ResetPos(CMsg* msg);

// formats the message script into msg and opens the window
u8* CMsg_Open(CMsg* msg, u8 command, u8* script, MsgFormatArgs* args);
}

#endif

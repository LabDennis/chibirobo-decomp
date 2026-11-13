#ifndef _GAME_TITLE_H
#define _GAME_TITLE_H

#include "Version.h"
#include "game/menuhelpchar.h"
#include "game/xobj.h"
#include "game/xobjdemo.h"
#include "game/ctpl.h"

struct TitlePaletteEntry {
	GXColor background, text;
};
extern "C" {
extern const TitlePaletteEntry gTitlePalette[];
extern const s32 gTitleLogoTextures[3];
extern s32 gTitlePaletteIndex, gTitleLogoIndex;
}

// the menus' pointer, an XObj with a little on top. class name from RTTI
class Cursor : public XObj {
public:
	// _00-_980 = XObj
	f32 mAngle;      // _980, _970 in japan
	f32 mAngleSpeed; // _984, it spins, the scale setters flip y for screen space

	virtual ~Cursor() { }                                  // _08
	virtual void SetScaleY(f32 y, s32 force);              // _1C
	virtual void SetScale(const Vec& scale, s32 force);    // _38
	virtual void SetScale(f32 x, f32 y, f32 z, s32 force); // _3C
	virtual void SetScale(f32 scale, s32 force);           // _6C
	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate);                   // _80
	virtual void InitializeObject(u32 id);                 // _90
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _9C
	virtual void UpdateObject(u32 frames);                 // _A4
	virtual void vfAC();                                   // _AC
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(Cursor) == 0x978);
#else
STATIC_ASSERT(sizeof(Cursor) == 0x988);
#endif

// a speck floating over the title screen
struct TitleDust {
	u32 mState;  // _00
	u32 mTimer;  // _04
	f32 mX;      // _08
	f32 mY;      // _0C
	f32 mSpeedX; // _10
	f32 mSpeedY; // _14
};
STATIC_ASSERT(sizeof(TitleDust) == 0x18);

// one letter of a name that pops in
struct StageNameChar {
	const char* mText; // _00, the letter, one or two bytes
	f32 mScale;        // _04, grows to 1 and shrinks away
	f32 mWait;         // _08, frames until it shrinks
	f32 mBob;          // _0C, the angle it bobs up and down by
	f32 mTilt;         // _10, the angle it rocks by
};
STATIC_ASSERT(sizeof(StageNameChar) == 0x14);

// a name drawn a letter at a time over bubbles, the stage name and the title's own
struct StageName {
	BOOL mActive;             // _00
	s32 mCount;               // _04
	u32 mColor;               // _08, the letters' color, the alpha comes from their scale
	StageNameChar mChars[20]; // _0C
};
STATIC_ASSERT(sizeof(StageName) == 0x19C);

// the title screen, CGame keeps it at _4930. class name from RTTI
class Title {
public:
	void State0(u32 frames);
	void State1(u32 frames);
	void State2(u32 frames);
	void State3(u32 frames);
	void State4(u32 frames);
	void State5(u32 frames);
	void State6(u32 frames);
	void State7(u32 frames);
	void State8(u32 frames);
	void State9(u32 frames);
	void State10(u32 frames);
	void State11(u32 frames);
	void State12(u32 frames);
	void State13(u32 frames);
	void State14(u32 frames);
	void State15(u32 frames);
	void State16(u32 frames);
	void State17(u32 frames);
	void State18(u32 frames);
	void State19(u32 frames);
	void State20(u32 frames);
	void State21(u32 frames);
	void State22(u32 frames);
	void State23(u32 frames);
	void State24(u32 frames);
	void State25(u32 frames);

	s32 mState;                // _00, CGame's FrameInterval runs at 60 while it's 1 to 22
	s32 mMode;                 // _04
	u32 mOptionState;          // _08
	s32 mTimeout;              // _0C
	f32 mMenuAlpha;            // _10
	f32 _14;                   // _14, fades in over a second; state 10 waits for it and mMenuAlpha, nothing draws with it
	f32 mPulseAngle;           // _18
	s32 mHighlightDelay;       // _1C, frames until the selected line starts pulsing
	u32 mSpawnIndex;           // _20
	u32 _24;                   // _24
	Cursor mCursor;            // _28
	XObjDemo mDemo;            // _9B0, _9A0 in japan
	CTpl mTpl;                 // _2FB8, _2F98 in japan
	CTexObj mTex[6];           // _31A8
	BOOL mSelection;           // _37A8
	f32 mLoadScroll;           // _37AC
	f32 mPanelX;               // _37B0
	f32 mPanelY;               // _37B4
	f32 mTargetPanelX;         // _37B8
	f32 mTargetPanelY;         // _37BC
	f32 _37C0;                 // _37C0
	f32 mLoadOffset;           // _37C4
	f32 _37C8;                 // _37C8
	f32 mOffset;               // _37CC
	s32 mFreeBlocks;           // _37D0, the card's mFreeBlocks
	s32 mFreeEntries;          // _37D4, the card's mFreeEntries
	s32 mResult;               // _37D8, the card result
	u32 _37DC;                 // _37DC
	u8 mReady;                 // _37E0
	u8 mShowMessage;           // _37E1
	u8 mWrongDevice;           // _37E2
	u8 _37E3;                  // _37E3
	f32 mLoadAlpha;            // _37E4
	f32 mAlpha;                // _37E8
	s32 mPrevMode;             // _37EC
	s32 mPhase;                // _37F0
	s32 mDelay;                // _37F4
	s32 mFile;                 // _37F8, the newest save starts picked
	s32 mChoice;               // _37FC
	u32 _3800;                 // _3800
	s32 mChannel;              // _3804
	u8 mCardScanState;         // _3808
	u8 mSaveFound;             // _3809
	u8 _380A[0x380C - 0x380A]; // _380A
	s32 mOptionLine;           // _380C, the options line
	f32 mOptionAlpha;          // _3810
	f32 mTargetOptionAlpha;    // _3814
	u8 mRumble;                // _3818, rumble on
	u8 mMono;                  // _3819, mono sound
	u8 _381A[0x3820 - 0x381A]; // _381A
	s32 mRumbleTimer;          // _3820, frames the rumble test has left
	TitleDust mDust[16];       // _3824
	u32 mHelpFadeState;
	f32 mHelpAlpha;
	const char* mHelpText;
	const char* mNextHelpText;
	StageName mName; // _39B4, the letters fade out together when the title is left
	s32 mHelpCount;
	MenuHelpChar mBg[20];
#if !defined(VERSION_JP)
	u32 mFormatCancelled; // _3C44
#endif
	// _3C48 vtable, _3C24 in japan

	virtual ~Title() { } // _08

	// todo: cleanup
	void SetMode(s32 mode)
	{
		mPrevMode = mMode;
		mMode     = mode;
		mPhase    = 0;
	}
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(Title) == 0x3C28);
#else
STATIC_ASSERT(sizeof(Title) == 0x3C50);
#endif

typedef void (Title::*TitleModeUpdate)(u32 frames);
extern "C" TitleModeUpdate gTitleModeUpdates[26];

extern "C" void InitTitleScreen(Title* title);
extern "C" void ResetTitleScreenState(Title* title);
extern "C" void UpdateTitleScreen(Title* title, u32 frames);

#endif

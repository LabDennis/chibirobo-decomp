#ifndef _GAME_STG_H
#define _GAME_STG_H
#include "game/menuhelpchar.h"

#include "types.h"
#include "Version.h"
#include "game/title.h"
#include "game/coordinfo.h"
#include "game/ctpl.h"
#include "game/cgs.h"
#include "game/cgame.h"
#include "game/menuex.h"

// the 2d menus, game/stg.cpp. class names from RTTI, the layout from the methods and the
// statics' destructor chain. offsets are usa

class MenuCursor;

// a menu window's scrolling texture, name is ours
struct MenuTex {
	CTexObj mTex; // _00
	f32 mX;       // _100, scrolls while the window is open
	f32 mY;       // _104
	f32 mScaleX;  // _108
	f32 mScaleY;  // _10C
};
STATIC_ASSERT(sizeof(MenuTex) == 0x110);

// the frame a menu draws behind itself, name is ours
struct MenuWindow {
	BOOL mOpen;     // _00
	f32 mX;         // _04, center
	f32 mY;         // _08
	f32 mWidth;     // _0C
	f32 mHeight;    // _10
	MenuTex mTex;   // _14
	f32 mDrawAlpha; // _124

	void SetSize(f32 width, f32 height)
	{
		mWidth  = width;
		mHeight = height;
	}
	void SetPos(f32 x, f32 y)
	{
		mX = x;
		mY = y;
	}
};
STATIC_ASSERT(sizeof(MenuWindow) == 0x128);

extern "C" {
// draws a menu's window
void DrawMenuWindow(MenuWindow* window);
}

// the base of every menu. mState runs 0 closed, 1 opening, 2 to 4 open, 5 closing, 6 closed
class CMenu {
public:
	s32 mState;          // _00
	f32 mAlpha;          // _04
	s32 mSelection;      // _08, the row the cursor is on
	MenuWindow mWindow;  // _0C
	MenuCursor* mCursor; // _134, MenuEx's
	BOOL mSuspended;     // _138, another menu is on top
	BOOL mSuspendFade;   // _13C, fade out while suspended
	f32 mFade;           // _140
	f32 mSuspendAlpha;   // _144
	// _148 vtable

	~CMenu() { }

	virtual void Init() // _08 (weak)
	{
		mWindow.mOpen      = FALSE;
		mWindow.mX         = 320.0f;
		mWindow.mY         = 224.0f;
		mWindow.mWidth     = 100.0f;
		mWindow.mHeight    = 100.0f;
		mWindow.mDrawAlpha = 0.0f;
		mCursor            = NULL;
		Reset();
	}
	virtual BOOL LoadTexture(); // _0C (weak)
	virtual void Reset()        // _10 (weak)
	{
		mState        = 0;
		mAlpha        = 0.0f;
		mSelection    = 0;
		mSuspended    = FALSE;
		mSuspendFade  = FALSE;
		mFade         = 0.0f;
		mSuspendAlpha = 1.0f;
	}
	virtual BOOL Update(u32 frames); // _14 (weak), returns mState
	virtual BOOL Draw()              // _18 (weak)
	{
		if (!IsOpen())
			return FALSE;
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 3, 0);
		DrawMenuWindow(&mWindow);
		return TRUE;
	}
	virtual void Open() // _1C (weak)
	{
		mState = 1;
		Resume();
		mWindow.mOpen = TRUE;
	}
	virtual void Close() // _20 (weak)
	{
		mState = 5;
		Resume();
		mWindow.mOpen = FALSE;
	}
	virtual BOOL IsOpen() // _24 (weak)
	{
		bool open = false;
		if (mState >= 1 && mState < 6)
			open = true;
		return open;
	}
	virtual void Suspend(BOOL fade) // _28 (weak)
	{
		mSuspended   = TRUE;
		mSuspendFade = fade;
	}
	virtual void Resume() // _2C (weak)
	{
		mSuspended   = FALSE;
		mSuspendFade = FALSE;
	}
};
STATIC_ASSERT(sizeof(CMenu) == 0x14C);

// a menu of text rows
class Menu : public CMenu {
public:
	f32 mFontSize; // _14C
	f32 mMaxWidth; // _150, of the widest row
	f32 mRowCount; // _154

	~Menu() { }
};
STATIC_ASSERT(sizeof(Menu) == 0x158);

#if defined(VERSION_JP)
class MenuRequest : public Menu {
public:
	~MenuRequest();
	virtual void Init();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();

	u8 _158[0x518 - 0x158];
	s32 _518;
	u8 _51C[0x528 - 0x51C];
	f32 _528, _52C, _530, _534;
	s32 _538;
	f32 _53C;
};
class MenuRequestClear : public Menu {
public:
	~MenuRequestClear();
	virtual void Init();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();

	u8 _158[0x160 - 0x158];
	s32 mDelay; // _160
	u8 _164[0x170 - 0x164];
	f32 _170, _174, _178, _17C;
	f32 _180, _184, _188, _18C;
	f32 _190, _194, _198, _19C;
	u8 _1A0[0x1B0 - 0x1A0];
	s32 mPhase; // _1B0
	f32 _1B4, _1B8;
	f32 mValues[4];  // _1BC, reward points, reward money, player points and player money
	f32 mTargets[4]; // _1CC
	u8 _1DC;
	u8 _1DD[3];
	f32 _1E0, _1E4;
};
class MenuPadControl : public Menu {
public:
	~MenuPadControl();
	virtual void Init();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();
};
class MenuSound : public Menu {
public:
	~MenuSound();
	virtual void Init();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();

	const char* mText[4]; // _158
	f32 mTextWidth[4];    // _168
};
class MenuSettings : public Menu {
public:
	~MenuSettings();
	virtual void Init();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();

	const char* mText[5]; // _158
	f32 mTextWidth[5];    // _16C
	s32 mValues[5];       // _180
};
#endif

// one of MenuMain's rows and what picking it does
struct MenuMainItem {
	char* mText; // _00, "\x01" for a separator
	s32 mAction; // _04, -1 for a separator
};

// the in-game menu, its last rows are added by whoever opens it
class MenuMain : public Menu {
public:
	~MenuMain();

	virtual void Init();             // _08
	virtual void Reset();            // _10
	virtual BOOL Update(u32 frames); // _14
	virtual BOOL Draw();             // _18
	virtual void Open();             // _1C
	virtual void Close();            // _20
	virtual void Resume();           // _2C, puts the game's pause flag back

#if defined(VERSION_JP)
	enum { ROW_MAX = 18 };
#else
	enum { ROW_MAX = 13 };
#endif

	s32 mBaseRows;              // _158
	char* mText[ROW_MAX];       // _15C
	f32 mTextWidth[ROW_MAX];    // _190, _1A4 in japan
	s32 mChannel;               // _1C4, _1EC in japan, the memory card slot the save menus use
	MenuMainItem mExtraRows[5]; // _1C8, _1F0 in japan, rows other menus add under the line
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(MenuMain) == 0x218);
#else
STATIC_ASSERT(sizeof(MenuMain) == 0x1F0);
#endif

// a letter of a title that flies in, one after the other
struct MenuTitleChar {
	const char* mText; // _00
	f32 mFade;         // _04, 0 to 1, starts below 0 so the letters come in one by one
	f32 mHold;         // _08, the letter fades out once this is 0
	f32 mAngle0;       // _0C
	f32 mAngle1;       // _10
};
STATIC_ASSERT(sizeof(MenuTitleChar) == 0x14);

// a title of up to 20 letters
struct MenuTitle {
	BOOL mActive;             // _00
	s32 mCount;               // _04
	u32 mColor;               // _08
	MenuTitleChar mChars[20]; // _0C

	inline void Set(const char* text, u32 color);
};
STATIC_ASSERT(sizeof(MenuTitle) == 0x19C);

// one of the status screen's tabs
struct MenuStatusTab {
	f32 _00; // _00
	f32 _04; // _04
	f32 mX;  // _08
	f32 _0C; // _0C
};
STATIC_ASSERT(sizeof(MenuStatusTab) == 0x10);

// the status screen
class MenuStatus : public Menu {
public:
	~MenuStatus();
	void State0(u32);
	void State1(u32);
	void State2(u32);
	void State3(u32);
	void State4(u32);
	void State5(u32);
	void State6(u32);
	void State7(u32);
	void State8(u32);
	void State9(u32);

	virtual void Init();             // _08
	virtual BOOL Update(u32 frames); // _14
	virtual BOOL Draw();             // _18
	virtual void Open();             // _1C

	u8 _158[0x510 - 0x158];  // _158
	s32 mMode;               // _510
	s32 mPrevMode;           // _514
	s32 mNextMode;           // _518, where a wait mode goes after
	s32 mAnimationFrame;     // _51C
	s32 mItem;               // _520, the one shown in the middle
	s32 mPreviousItem;       // _524
	s32 mOverviewSelection;  // _528
	s32 mIconSelection;      // _52C, selected cell in the 6 by 4 icon grid
	s32 mChoice;             // _530, 0 yes, 1 no
	s32 mCount;              // _534, how many of the item to throw away
	f32 mOffsetX;            // _538
	f32 mOffsetY;            // _53C
	f32 _540;                // _540
	f32 _544;                // _544
	f32 _548;                // _548
	f32 _54C;                // _54C
	f32 _550;                // _550
	f32 mSlide;              // _554
	u8 _558[0x568 - 0x558];  // _558
	f32 _568;                // _568
	f32 _56C;                // _56C
	f32 _570;                // _570
	f32 _574;                // _574
	f32 mDiscardPanelX;      // _578
	f32 mDiscardPanelY;      // _57C
	f32 _580;                // _580
	f32 _584;                // _584
	u8 _588[0x598 - 0x588];  // _588
	MenuStatusTab mTabs[4];  // _598
	f32 mX;                  // _5D8
	f32 mY;                  // _5DC
	f32 _5E0;                // _5E0
	f32 _5E4;                // _5E4
	f32 _5E8;                // _5E8
	f32 _5EC;                // _5EC
	f32 _5F0;                // _5F0
	f32 _5F4;                // _5F4
	f32 _5F8;                // _5F8
	f32 _5FC;                // _5FC
	f32 _600;                // _600
	f32 _604;                // _604
	f32 mDiagramX;           // _608
	f32 mDiagramY;           // _60C
	f32 _610;                // _610
	f32 _614;                // _614
	MenuStatusTab mTabs2[4]; // _618
	f32 _658;                // _658
	s32 mPhase;              // _65C
	s32 mDelay;              // _660
	s32 mSavedBgm;           // _664, put back on the way out
	u8 mSavedBgmVolume;      // _668
	u8 _669[0x66C - 0x669];  // _669
	f32 _66C;                // _66C
	f32 _670;                // _670
	u8 _674;                 // _674, the fourth tab is there
	u8 _675[4];              // _675
	u8 _679;                 // _679
	u8 _67A[2];              // _67A
	u8 _67C;                 // _67C
	u8 _67D[3];              // _67D
	f32 _680;                // _680
	s32 mShowHelp;           // _684
	f32 mHelpAlpha;          // _688
	const char* mHelpText;   // _68C
	char* mHelp;             // _690, the line at the bottom
	f32 _694;                // _694
	MenuTitle mTitle;        // _698
	s32 mHelpCount;          // _834
#if defined(VERSION_JP)
	MenuHelpChar mHelpChars[20]; // _838
#else
	MenuHelpChar mHelpChars[22]; // _838
#endif

	void SetMode(s32 mode)
	{
		mPrevMode = mMode;
		mMode     = mode;
		mPhase    = 0;
	}
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(MenuStatus) == 0x928);
#else
STATIC_ASSERT(sizeof(MenuStatus) == 0x940);
#endif

// one of the status screen's item slots
struct MenuStatusEntry {
	MenuStatusEntry();
	~MenuStatusEntry();
	s16 mItem;       // _00
	s16 mCount;      // _02
	CStatus mStatus; // _04
};
STATIC_ASSERT(sizeof(MenuStatusEntry) == 0xC);

// the items the status screen pages through
struct MenuStatusTable {
	~MenuStatusTable();
	s32 mPage;                   // _00, in fours
	MenuStatusEntry mItems[256]; // _04, the eight on screen
	MenuStatusEntry mPrev[4];    // _C04, the four before them
	MenuStatusEntry mNext[4];    // _C34, the four after
};
STATIC_ASSERT(sizeof(MenuStatusTable) == 0xC64);

class MenuItem : public Menu {
public:
	~MenuItem();

	virtual void Init();             // _08
	virtual BOOL Update(u32 frames); // _14
	virtual BOOL Draw();             // _18
	virtual void Open();             // _1C

	s32 _158;               // _158
	s32 _15C;               // _15C
	f32 _160;               // _160
	f32 _164;               // _164
	f32 _168;               // _168
	f32 _16C;               // _16C
	f32 _170;               // _170
	f32 _174;               // _174
	f32 _178;               // _178
	f32 _17C;               // _17C
	u8 _180[0x190 - 0x180]; // _180
	f32 _190;               // _190
	f32 _194;               // _194
	s32 mSavedBgm;          // _198
	s32 _19C;               // _19C
	s32 mShowHelp;          // _1A0
	f32 _1A4;               // _1A4
	char* mName;            // _1A8
	char* mHelp;            // _1AC
	f32 mItemPhase;         // _1B0
	MenuTitle mTitle;       // _1B4
	s32 _350;               // _350
#if defined(VERSION_PAL)
	MenuHelpChar mBg[22]; // _354
#else
	MenuHelpChar mBg[20]; // _354
#endif
};
#if defined(VERSION_PAL)
STATIC_ASSERT(sizeof(MenuItem) == 0x45C);
#else
STATIC_ASSERT(sizeof(MenuItem) == 0x444);
#endif

struct PCRankingRow {
	s32 rank;
	s32 nameIndex;
	s32 score;
};
STATIC_ASSERT(sizeof(PCRankingRow) == 12);

struct PCAmount {
	f32 shown;
	f32 target;
};

// the chibi-pc shop
class MenuPC : public Menu {
public:
	~MenuPC();
	void State0(u32);
	void State1(u32);
	void State2(u32);
	void State3(u32);
	void State4(u32);
	void State5(u32);
	void State6(u32);
	void State7(u32);
	void State8(u32);
	void State9(u32);
	void State10(u32);
	void State11(u32);

	virtual void Init();             // _08
	virtual BOOL Update(u32 frames); // _14
	virtual BOOL Draw();             // _18
	virtual void Open();             // _1C

	inline void ResetText(s32 mode);

	s32 mMode;                    // _158
	s32 mPrevMode;                // _15C
	u32 _160;                     // _160
	s32 mPhase;                   // _164
	f32 mSpeed;                   // _168
	s32 mTextPhase;               // _16C
	s32 mChoice;                  // _170
	s32 mPurchaseSelection;       // _174
	s32 _178;                     // _178
	s32 _17C;                     // _17C
	s32 mYesNo;                   // _180
	s32 mMoneyDigit;              // _184
	s32 mShopSelection;           // _188
	s32 mScrapDigit;              // _18C
	f32 mProgress;                // _190
	f32 mTargetProgress;          // _194
	f32 _198;                     // _198
	f32 mFade;                    // _19C
	f32 _1A0;                     // _1A0
	f32 _1A4;                     // _1A4
	u8 _1A8[0x1B8 - 0x1A8];       // _1A8
	s32 mCharIndex;               // _1B8
	u32 _1BC;                     // _1BC
	f32 mTextX;                   // _1C0
	f32 mTextY;                   // _1C4
	f32 mTargetX;                 // _1C8
	f32 mTargetY;                 // _1CC
	u16 mChars[40];               // _1D0, two byte sjis
	u8 mTerminator;               // _220
	u8 _221[0x224 - 0x221];       // _221
	f32 mCharX[40];               // _224
	u8 mCharFlags[40];            // _2C4
	char mCharCopies[40][3];      // _2EC, each character again as a string of its own
	u32 mGlyphFrames;             // _364
	u32 mNextGlyph;               // _368
	s32 _36C;                     // _36C, the shopkeeper's anim step
	s32 _370;                     // _370, the anim for it
	u8 _374;                      // _374, set the anim
	u8 _375[0x378 - 0x375];       // _375
	u32 mNumberOffset;            // _378
	u8 mInsertsNumber;            // _37C
	u8 _37D[0x380 - 0x37D];       // _37D
	s32 mAmount;                  // _380
	s32 mDepositBalance;          // _384
	s32 mHistoryStart;            // _388
	s32 mHistoryCount;            // _38C
	f32 mPanelAlpha;              // _390
	f32 mPanelTarget;             // _394
	f32 _398;                     // _398
	f32 _39C;                     // _39C
	u8 _3A0;                      // _3A0
	u8 _3A1[0x3A4 - 0x3A1];       // _3A1
	u32 mDialogueNumber;          // _3A4
	f32 mRankingOffsets[5];       // _3A8
	PCRankingRow mRankingRows[5]; // _3BC
	s32 mRankingEnd;              // _3F8
	f32 mScroll;                  // _3FC
	f32 mScrollTarget;            // _400
	s32 mCatalogBase;             // _404
	s32 mCatalogCount;            // _408
	PCAmount mMoneyDisplay;       // _40C
	PCAmount mScrapDisplay;       // _414
	PCAmount mDepositDisplay;     // _41C
	f32 _424;                     // _424
	f32 _428;                     // _428
	f32 mScrapRate;               // _42C
	s32 mExitState;               // _430, the game state it hands back
	u8 mCanSelect;                // _434
	u8 _435[0x438 - 0x435];       // _435
	s32 mChoices[4];              // _438
	s32 mChoiceCount;             // _448
	f32 _44C;                     // _44C
};
#if !defined(VERSION_JP)
STATIC_ASSERT(sizeof(MenuPC) == 0x450);
#endif

// a save file as the save and title menus list it
struct MenuSaveSlot {
	u32 mUsed;           // _00
	s32 mLevel;          // _04
	f32 mBattery;        // _08
	f32 mMaxBattery;     // _0C
	u32 mMoney;          // _10
	u32 mExp;            // _14
	u32 mScrap;          // _18
	u32 mRank;           // _1C
	u32 mHours;          // _20
	u32 mMinutes;        // _24
	u32 mStage;          // _28
	u32 mTimeMode;       // _2C
	u8 _30[0x40 - 0x30]; // _30
	s32 mState;          // _40, 1 when the file is broken
	u8 mActive;          // _44
};
STATIC_ASSERT(sizeof(MenuSaveSlot) == 0x48);

inline void ClearSaveSlot(MenuSaveSlot* slot)
{
	slot->mUsed       = 0;
	slot->mLevel      = 0;
	slot->mBattery    = 0.0f;
	slot->mMaxBattery = 0.0f;
	slot->mMoney      = 0;
	slot->mExp        = 0;
	slot->mScrap      = 0;
	slot->mRank       = 0;
	slot->mHours      = 0;
	slot->mMinutes    = 0;
	slot->mStage      = 0;
	slot->mTimeMode   = 0;
	slot->mState      = 0;
	slot->mActive     = 0;
}

// the save menu, its mode handlers sit in the table at 0x802122B4
class MenuSave : public Menu {
public:
	~MenuSave();
	void State0(u32);
	void State1(u32);
	void State2(u32);
	void State3(u32);
	void State4(u32);
	void State5(u32);
	void State6(u32);
	void State7(u32);
	void State8(u32);
	void State9(u32);
	void State10(u32);
	void State11(u32);
	void State12(u32);
	void State13(u32);
	void State14(u32);
	void State15(u32);
	void State16(u32);
	void State17(u32);

	virtual void Init();             // _08
	virtual void Reset();            // _10
	virtual BOOL Update(u32 frames); // _14
	virtual BOOL Draw();             // _18
	virtual void Open();             // _1C

	f32 _158;                  // _158
	f32 _15C;                  // _15C
	f32 _160;                  // _160
	f32 _164;                  // _164
	f32 _168;                  // _168
	f32 _16C;                  // _16C
	f32 _170;                  // _170
	f32 mOffset;               // _174
	MenuSaveSlot mSlots[3];    // _178, the three save files
	s32 _250;                  // _250, the card's _16A04
	s32 _254;                  // _254, the card's _16A00
	s32 mResult;               // _258, the card result
	s32 mMessage;              // _25C
	u8 mReady;                 // _260, a card is in the slot
	u8 mShowMessage;           // _261
	u8 _262;                   // _262, checked instead of mReady after a -2
	u8 mSaved;                 // _263
	f32 _264;                  // _264
	f32 mTargetAlpha;          // _268, _264 goes to it
	s32 mMode;                 // _26C
	s32 mPrevMode;             // _270
	u32 _274;                  // _274
	s32 mPhase;                // _278
	f32 mTime;                 // _27C
	s32 mDelay;                // _280
	s32 mFile;                 // _284
	s32 mChoice;               // _288, 0 yes, 1 no
	s32 mLastFile;             // _28C, -1 until something's saved
	s32 mChannel;              // _290
	s32 _294;                  // _294
	f32 _298;                  // _298
	const char* mHelpText;     // _29C
	const char* mNextHelpText; // _2A0
	f32 _2A4;                  // _2A4
	MenuTitle mTitle;          // _2A8
	s32 mHelpCount;            // _444
	MenuHelpChar mBg[20];      // _448
#if !defined(VERSION_JP)
	BOOL _538; // _538
#endif

	void SetMode(s32 mode)
	{
		mPrevMode = mMode;
		mMode     = mode;
		mPhase    = 0;
	}

	// the card went away, forget its files
	void ResetResults()
	{
		mResult = -3;
		SetMode(6);
		mOffset = -32.0f;
		for (int i = 0; i < 3; i++) {
			ClearSaveSlot(&mSlots[i]);
		}
	}
};
#if !defined(VERSION_JP)
STATIC_ASSERT(sizeof(MenuSave) == 0x53C);
#endif

// the debug stage select, two columns of 20
class MenuStageSelect : public Menu {
public:
	~MenuStageSelect();

	virtual void Init();             // _08
	virtual BOOL Update(u32 frames); // _14
	virtual BOOL Draw();             // _18
	virtual void Open();             // _1C

	s32 mCount;            // _158
	char mLabels[100][64]; // _15C, "\x01" marks a stage that can't be picked
	f32 mLabelWidth[100];  // _1A5C
};
STATIC_ASSERT(sizeof(MenuStageSelect) == 0x1BEC);

// the memory card test menu
class MenuMCTest : public Menu {
public:
	~MenuMCTest();

	virtual void Init();             // _08
	virtual BOOL Update(u32 frames); // _14
	virtual BOOL Draw();             // _18
	virtual void Open();             // _1C

	s32 mFileCount;     // _158
	char mText[12][64]; // _15C
	f32 mTextWidth[12]; // _45C
	s32 mOperation;     // _48C, the card job running, odd while busy
};
STATIC_ASSERT(sizeof(MenuMCTest) == 0x490);

// its popup with the four card actions
class MenuMCTestSub : public Menu {
public:
	~MenuMCTestSub();

	virtual void Init();             // _08
	virtual BOOL Update(u32 frames); // _14
	virtual BOOL Draw();             // _18
	virtual void Open();             // _1C

	const char* mText[4]; // _158
	f32 mTextWidth[4];    // _168
};
STATIC_ASSERT(sizeof(MenuMCTestSub) == 0x178);

// a popup row and what picking it does
struct MenuMCTestAction {
	const char* mText; // _00
	s32 mAction;       // _04
};

// a row of one of the chibi-pc lists, MenuPC pages through them. names are ours
struct MenuPCItem {
	s16 mItem;        // _00, -1 for an empty row
	s16 mCount;       // _02, -1 for an empty row
	s16 mPrice;       // _04
	u8 mIsAttachment; // _06
	u8 _07;           // _07
};
STATIC_ASSERT(sizeof(MenuPCItem) == 8);

// the nine rows on screen and the ones just off either end
struct MenuPCPage {
	s32 mStart;           // _00, list index of the first row
	MenuPCItem mRows[20]; // _04
	s32 mIndex[20];       // _A4, list index of each row
	MenuPCItem mPrev[1];  // _F4
	MenuPCItem mNext[1];  // _FC
};
STATIC_ASSERT(sizeof(MenuPCPage) == 0x104);

extern "C" {
// CMenu's own update, the overrides call it first
BOOL IsSaveSlotIntact(MenuSaveSlot* slot);
void ResetSaveSlotRecord(MenuSaveSlot* slot);
void RebuildStageSelectList(MenuStageSelect* menu);
void DrawSeparatedStatusValue(s32 icon, s32 value, bool selected, f32 x, f32 y, f32 alpha);
}

#endif

#ifndef GAME_WII_TITLE_H
#define GAME_WII_TITLE_H

#include "game/wii/menuutil.h"
#include "game/menuhelpchar.h"
#include "game/ctpl.h"
#include "Dolphin/vec.h"
class CoordInfo;
class CCamera;
struct UnkStageObj;

// The Wii title screen (title.cpp), CGame keeps it at _5C70. The layout follows the GameCube Title
// (game/title.h) with a vtable in front, a remembered value behind each selection the pointer can drop,
// and the pointer's panes at the end. Only what the reconstructed functions reach is here; the names are
// the GameCube ones where the member is the same.

// one queued animation: CMsg__cmd_ANIM's MSG_ANIM_QUEUE drops it into the first free
// slot after mAnmHead, the GameCube kept the same ring on XObj
struct WiiAnimSlot {
	s32 mAnim; // _00, -1 when the slot is free
	s16 mA;    // _04
	s16 mB;    // _06
};
STATIC_ASSERT(sizeof(WiiAnimSlot) == 0x8);

// a part object inside the cursor object's _F0 list; CMsgFont_Draw hands each one the
// cursor's animation position at _01C
class WiiCursorPart {
public:
	virtual ~WiiCursorPart() { } // _08
	virtual void vf00C();
	virtual void vf010();
	virtual void vf014();
	virtual void vf018();
	virtual void vf01C(f32 frame);
};

// the parts list a WiiTitleObject keeps at _F0; _040 returns one entry by index
class WiiCursorParts {
public:
	virtual ~WiiCursorParts() { } // _08
	virtual void vf00C();
	virtual void vf010();
	virtual void vf014();
	virtual void vf018();
	virtual void vf01C();
	virtual void vf020();
	virtual void vf024();
	virtual void vf028();
	virtual void vf02C();
	virtual void vf030();
	virtual void vf034();
	virtual void vf038();
	virtual void vf03C();
	virtual WiiCursorPart* vf040(s32 index);
};

// the title's two objects, the cursor and the demo figure. The Wii XObj is not reconstructed: these are
// its virtuals up to the one the title code calls, the unnamed ones only keep its place. Slot _08 is the destructor:
// ~CMsg destroys CMsgFont's two through it (0x8002F1A4)
class WiiTitleObject {
public:
	virtual ~WiiTitleObject() { } // _08
	virtual void vf00C();
	virtual void vf010();
	virtual void vf014();
	virtual void vf018();
	virtual void vf01C();
	virtual void vf020();
	virtual void vf024();
	virtual void vf028();
	virtual void vf02C();
	virtual void vf030();
	virtual void vf034();
	virtual void vf038();
	virtual void vf03C();
	virtual void vf040(); // _040, Release
	virtual void vf044();
	virtual void vf048();
	virtual void vf04C();
	virtual void vf050();
	virtual void vf054();
	virtual void vf058();
	virtual void vf05C();
	virtual void vf060();
	virtual void vf064();
	virtual void vf068();
	virtual void vf06C();
	virtual void vf070(f32); // _070, a pose value; the other cursor's vf26C feeds it
	virtual void vf074();
	virtual void vf078();
	virtual void vf07C();
	virtual void vf080();
	virtual void vf084(f32 x, f32 y, f32 z); // _084, a position
	virtual void vf088();
	virtual void vf08C(f32 x, f32 y, f32 z);
	virtual void vf090(void*); // _090, given the other cursor's vf244 result
	virtual void vf094();
	virtual void vf098(void*);     // _098, given the other cursor's vf258 result
	virtual void vf09C(f32 scale); // _09C, a uniform scale
	virtual void vf0A0();
	virtual void vf0A4();
	virtual void vf0A8();
	virtual void vf0AC();
	virtual void vf0B0();
	virtual Vec vf0B4(const s32* path); // _0B4, a joint's world position by path
	virtual Vec vf0B8(s32 index);       // _0B8, the object's own position
	virtual void vf0BC();
	virtual void vf0C0();
	virtual void vf0C4();
	virtual void vf0C8();
	virtual void vf0CC();
	virtual void vf0D0();
	virtual void vf0D4();
	virtual void vf0D8();
	virtual void vf0DC();
	virtual void vf0E0();
	virtual void vf0E4();
	virtual void vf0E8();
	virtual void vf0EC();
	virtual void vf0F0();
	virtual void vf0F4();
	virtual void vf0F8();
	virtual void vf0FC();
	virtual void vf100();
	virtual void vf104();
	// _108, the Wii's SetAnim: the GameCube passed the resolved resources, the Wii resolves them itself
	virtual void vf108(s32 anim, s32 flags, f32, f32, f32, f32);
	virtual void vf10C();
	virtual void vf110();
	virtual void vf114(f32 frame);  // _114, the playing animation's frame
	virtual void vf118(f32 frames); // _118, steps the playing animation, the GameCube's UpdateAnim
	virtual s32 vf11C();            // _11C, the playing animation's id
	virtual void vf120();
	virtual void vf124();
	virtual f32 vf128(); // _128, the playing animation's frame
	virtual f32 vf12C(); // _12C, the playing animation's length
	// _130, nonzero once the current animation has played out at the given frame count
	virtual s32 vf130(f32 frames);
	virtual s32 vf134(); // _134, the animation flags check
	virtual void vf138();
	virtual void vf13C();
	virtual void vf140();
	virtual void vf144();
	virtual void vf148();
	virtual void vf14C();
	virtual void vf150();
	virtual void vf154();
	virtual s32 vf158(CoordInfo info);
	virtual void vf15C();
	virtual void vf160();
	virtual void UpdateObject(u32 frames); // _164
	virtual void vf168();
	virtual void vf16C();
	virtual void vf170();
	virtual void vf174();
	virtual void vf178();
	virtual void vf17C();
	virtual void vf180();
	virtual void vf184();
	virtual void vf188();
	virtual void vf18C();
	virtual void vf190(); // _190, draws the object, the GameCube's DrawObject
	virtual void vf194();
	virtual void vf198();
	virtual void vf19C();
	virtual void vf1A0();
	virtual void vf1A4();
	virtual void vf1A8();
	virtual void vf1AC();
	virtual void vf1B0();
	virtual void vf1B4();
	virtual void vf1B8();
	virtual void vf1BC();
	virtual void vf1C0();
	virtual void vf1C4();
	virtual void vf1C8();
	virtual void vf1CC();
	virtual void vf1D0();
	// _1D4, projects a world position onto the screen, the GameCube's ProjectOffsetToScreen
	virtual void vf1D4(const Vec* pos, f32* x, f32* y, s32, s32);
	virtual f32 vf1D8(); // _1D8, the GameCube's GetFloorXZExtent
	virtual f32 vf1DC(); // _1DC, the GameCube's GetCollisionHeight
	// _1E0, the GameCube's ProjectVisibleBounds
	virtual BOOL vf1E0(f32* width, f32* height, CCamera* camera, f32 radius, f32 objectHeight);
	virtual void vf1E4();
	virtual void vf1E8();
	virtual void vf1EC();
	virtual void vf1F0();
	virtual void vf1F4();
	virtual void vf1F8();
	virtual void vf1FC();
	virtual void vf200();
	virtual void vf204();
	virtual void vf208();
	virtual void vf20C();
	virtual void vf210();
	virtual void vf214();
	virtual void vf218();
	virtual void vf21C();
	virtual void vf220();
	virtual void vf224();
	virtual void vf228();
	virtual void vf22C();
	virtual void vf230();
	virtual Vec vf234(const char* joint); // _234, a joint's world position by name
	virtual void vf238();
	virtual void vf23C();
	virtual void vf240();
	virtual void* vf244(); // _244, a pose component the draw copies between the cursors
	virtual void vf248();
	virtual void vf24C();
	virtual void vf250();
	virtual void vf254();
	virtual void* vf258(); // _258, another pose component
	virtual void vf25C();
	virtual void vf260();
	virtual void vf264();
	virtual void vf268();
	virtual f32 vf26C(); // _26C, another pose component

	u8 _004[0x014 - 0x004];
	UnkStageObj* mSpawn; // _014, the placement the object loaded from
	u8 _018[0x0F0 - 0x018];
	WiiCursorParts* mParts; // _0F0, up to six parts the message draw steps with the anim frame
	u8 _0F4[0x150 - 0x0F4];
	u64 mStatus; // _150, the flags are a 64-bit mask: clearing one bit stores both words; bit 0 of the
	             // low word at _154 marks a finished LoadFromPlacement, bit 11 the caller's scale set
	u8 _158[0x168 - 0x158];
	void* mKind;      // _168, the object's spawn data; +0x16 is the s16 object kind
	void* mSpawnData; // _16C, the placement record; _00 is the object kind the scroll map checks
	u8 _170[0x6D4 - 0x170];
	BOOL mScreenBoundsValid; // _6D4, the four below hold the object's bounds on screen
	u8 _6D8[0x6E0 - 0x6D8];
	f32 mScreenTop;    // _6E0
	f32 mScreenBottom; // _6E4
	f32 mScreenLeft;   // _6E8
	f32 mScreenRight;  // _6EC
	u8 _6F0[0x794 - 0x6F0];
	s32 mAnmHead;             // _794, the slot PushAnim probes first
	WiiAnimSlot mAnmQueue[4]; // _798
	u8 _7B8[0x998 - 0x7B8];
	s32 _998; // _998, the robo eyes label an item only while this is clear
	u8 _99C[0x9B0 - 0x99C];

	void ClearAnmQueue()
	{
		mAnmHead = 0;
		for (s32 i = 0; i < 4; i++) {
			mAnmQueue[i].mAnim = -1;
		}
	}

	void PushAnim(s32 anim, s16 a, s16 b)
	{
		for (s32 i = 0; i < 4; i++) {
			WiiAnimSlot* slot = &mAnmQueue[(mAnmHead + i) % 4];
			if (slot->mAnim == -1) {
				slot->mAnim = anim;
				slot->mA    = a;
				slot->mB    = b;
				return;
			}
		}
	}
};
STATIC_ASSERT(sizeof(WiiTitleObject) == 0x9B0);

// one of the three demo figures behind mDemo; the file's RTTI names the class XObjDemo. Its head
// layout matches WiiTitleObject: the placement status sits at _154 and the state at _9A8
class WiiTitleFigure {
public:
	virtual void vf008();
	virtual void vf00C();
	virtual void vf010();
	virtual void vf014();
	virtual void vf018();
	virtual void vf01C();
	virtual void vf020();
	virtual void vf024();
	virtual void vf028();
	virtual void vf02C();
	virtual void vf030();
	virtual void vf034();
	virtual void vf038();
	virtual void vf03C();
	virtual void vf040(); // _040, Release
	virtual void vf044();
	virtual void vf048();
	virtual void vf04C();
	virtual void vf050();
	virtual void vf054();
	virtual void vf058();
	virtual void vf05C();
	virtual void vf060();
	virtual void vf064();
	virtual void vf068();
	virtual void vf06C();
	virtual void vf070(f32 angle); // _070
	virtual void vf074();
	virtual void vf078();
	virtual void vf07C();
	virtual void vf080();
	virtual void vf084();
	virtual void vf088();
	virtual void vf08C();
	virtual void vf090();
	virtual void vf094();
	virtual void vf098();
	virtual void vf09C(f32 scale); // _09C
	virtual void vf0A0();
	virtual void vf0A4();
	virtual void vf0A8();
	virtual void vf0AC();
	virtual void vf0B0();
	virtual void vf0B4();
	virtual void vf0B8();
	virtual void vf0BC();
	virtual void vf0C0();
	virtual void vf0C4();
	virtual void vf0C8();
	virtual void vf0CC();
	virtual void vf0D0();
	virtual void vf0D4();
	virtual void vf0D8();
	virtual void vf0DC();
	virtual void vf0E0();
	virtual void vf0E4();
	virtual void vf0E8();
	virtual void vf0EC();
	virtual void vf0F0();
	virtual void vf0F4();
	virtual void vf0F8();
	virtual void vf0FC();
	virtual void vf100();
	virtual void vf104();
	virtual void vf108();
	virtual void vf10C();
	virtual void vf110();
	virtual void vf114();
	virtual void vf118();
	virtual s32 vf11C(); // _11C, the playing animation id
	virtual void vf120();
	virtual void vf124();
	virtual void vf128();
	virtual void vf12C();
	virtual void vf130();
	virtual s32 vf134(); // _134, the animation flags check
	virtual void vf138();
	virtual void vf13C();
	virtual void vf140();
	virtual void vf144();
	virtual void vf148();
	virtual void vf14C();
	virtual void vf150();
	virtual void vf154();
	virtual s32 vf158(const CoordInfo& info); // _158, LoadFromPlacement
	virtual void vf15C();
	virtual void vf160();
	virtual void vf164(u32 frames); // _164, UpdateObject
	virtual void vf168();
	virtual void vf16C();
	virtual void vf170();
	virtual void vf174();    // _174, DrawObject front
	virtual void vf178(s32); // _178, DrawObject mode

	u8 _004[0x154 - 0x004];
	u32 mStatus; // _154, bit 0 marks a finished LoadFromPlacement
	u8 _158[0x9A8 - 0x158];
	s32 mState;      // _9A8
	s32 mStateParam; // _9AC
	u8 _9B0[0xBA8 - 0x9B0];
	s32 mNextAnim; // _BA8, -1 when nothing is queued
	s32 _BAC;
	u8 _BB0[0xF10 - 0xBB0];
	s32 mAnimIds[8]; // _F10, all -1 once ResetObjectState ran
};
STATIC_ASSERT(sizeof(WiiTitleFigure) == 0xF30);

// one mote of dust drifting over the menu, the GameCube TitleDust
struct TitleDust {
	u32 mState;  // _00
	s32 mTimer;  // _04
	f32 mX;      // _08
	f32 mY;      // _0C
	f32 mSpeedX; // _10, also the quad's scale
	f32 mSpeedY; // _14, also the quad's alpha
};
STATIC_ASSERT(sizeof(TitleDust) == 0x18);

// one letter of the name that pops in, as the GameCube StageNameChar
struct TitleNameChar {
	const char* mText; // _00
	f32 mScale;        // _04
	f32 mWait;         // _08, frames until it shrinks
	f32 mBob;          // _0C
	f32 mTilt;         // _10
};
STATIC_ASSERT(sizeof(TitleNameChar) == 0x14);

struct TitleName {
	BOOL mActive;             // _00
	s32 mCount;               // _04
	u32 mColor;               // _08
	TitleNameChar mChars[20]; // _0C
};
STATIC_ASSERT(sizeof(TitleName) == 0x19C);

// a rectangle the pointer can pick. The menu code's messages call it CbPane; the layout is not known
// beyond the two flags
struct CbPane {
	u8 _00[0x44];
	s32 mKind; // _44, which entry the pane reports as
	u8 _48[4];
	u8 mPointed; // _4C
	u8 mPicked;  // _4D
	u8 _4E[2];
};
STATIC_ASSERT(sizeof(CbPane) == 0x50);

// the panes the pointer is tested against this frame
struct CbPaneList {
	CbPane* mPanes[0x100]; // _000
	s32 mCount;            // _400
	MenuUtil* mMenu;       // _404, the menu whose panes are tested, NULL disables the pointer
	u8 _408[2];
	u8 mEnabled; // _40A
	u8 mActive;  // _40B
};

extern "C" {
extern CbPaneList gPointerPanes;
void AddPointerPane(CbPaneList* list, CbPane* pane);
}

class Title : public MenuUtil {
public:
	// _0000 vtable
	s32 mState;                 // _0004
	s32 mMode;                  // _0008
	u32 mOptionState;           // _000C, 0 choosing, 2 leaving
	s32 mTimeout;               // _0010, counts down the press-start / leave delays
	f32 mMenuAlpha;             // _0014
	f32 mTitleAlpha;            // _0018, the second fade that gates the press-start prompt
	f32 mPulseAngle;            // _001C
	s32 mHighlightDelay;        // _0020, frames until the selected line starts pulsing
	u32 mSpawnIndex;            // _0024
	WiiTitleObject mCursor;     // _0028
	WiiTitleObject mDemo;       // _09D8
	WiiTitleFigure mFigures[3]; // _1388
	CTpl mTpl;                  // _4118, the "title.cpp" archive the three textures come out of
	CTexObj mTex[6];            // _4310, as on the GameCube; the Wii draws three of them: logo, footer and dust
	s32 mSelection;             // _4928, the entry of the title menu, -1 while the pointer is on none
	s32 mLastSelection;         // _492C
	u8 _4930[0x493C - 0x4930];
	f32 mPanelY;       // _493C
	f32 mTargetPanelY; // _4940
	f32 mLoadScroll;   // _4944, the load screen's scroll position
	u8 _4948[4];
	f32 mLoadOffset; // _494C
	f32 mOffset;     // _4950
	u8 _4954[0x4964 - 0x4954];
	u8 mReady;       // _4964, the "press start" prompt is up
	u8 mShowMessage; // _4965, the status line is being shown
	u8 mWrongDevice; // _4966, a save scan finished but no usable slot answered
	u8 _4967;
	f32 mLoadAlpha; // _4968
	f32 mAlpha;     // _496C
	s32 mLastMode;  // _4970
	s32 mModeParam; // _4974
	u8 _4978[4];
	s32 mFile;     // _497C, the save file picked, -1 with the pointer on none
	s32 mLastFile; // _4980
	u8 _4984[4];
	s32 mChoice;     // _4988, yes or no, -1 with the pointer on neither
	s32 mLastChoice; // _498C
	u8 _4990[0x4998 - 0x4990];
	u8 mCardScanState; // _4998, where the slot scan is: 0 fresh, 1 scanning, 2 done
	u8 mSaveFound;     // _4999, a save is on the card and the second entry shows
	u8 _499A[2];
	s32 mOptionLine;        // _499C, the options line, -1 with the pointer on none
	s32 mLastOptionLine;    // _49A0
	f32 mOptionAlpha;       // _49A4, the options page's fade
	f32 mTargetOptionAlpha; // _49A8
	u8 mPointerOn;          // _49AC, the options page's two settings; OK writes them to gGame.mMenuControls
	u8 mStickOn;            // _49AD
	u8 mSavedPointerOn;     // _49AE, what they were when the page opened, cancel puts them back
	u8 mSavedStickOn;       // _49AF
	u8 _49B0[8];
	TitleDust mDust[16];       // _49B8
	s32 mHelpFadeState;        // _4B38, where the help line's fade is
	f32 mHelpAlpha;            // _4B3C
	const char* mHelpText;     // _4B40
	const char* mNextHelpText; // _4B44, the text the help line fades to
	TitleName mName;           // _4B48
	s32 mHelpCount;            // _4CE4
	MenuHelpChar mBg[20];      // _4CE8
	CbPane mPanes[11];         // _4DD8, the pointer's rectangles: [5..7] menu entries, [8..10] option lines
	f32 mOptionBar[3];         // _5148, the highlight on the options page, eased towards the line picked
	f32 mOptionBarTarget[2];   // _5154, where its second and third value go
	f32 mEntryLines[3];        // _515C, the entry underline eases
	f32 mOptionBarLines[3];    // _5168, where its first value goes, by line
	u8 mOptionBarSnap;         // _5174, the bar reached its line
	u8 mSaveError;             // _5175, a save operation's error is being shown
	u8 _5176[2];
	s32 mSaveErrorCode; // _5178
	f32 mFileSlides[3]; // _517C, the file rows sliding in

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
	// the file-menu mode after State25, gTitleModeUpdates' last entry
	void fn_80048418(u32 frames);

	void UpdateSelections();
};

extern "C" void UpdateTitleOptions(Title* title, u32 frames);

#endif

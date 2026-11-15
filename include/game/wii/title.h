#ifndef GAME_WII_TITLE_H
#define GAME_WII_TITLE_H

#include "game/wii/menuutil.h"
#include "Dolphin/vec.h"

// The Wii title screen (title.cpp), CGame keeps it at _5C70. The layout follows the GameCube Title
// (game/title.h) with a vtable in front, a remembered value behind each selection the pointer can drop,
// and the pointer's panes at the end. Only what the reconstructed functions reach is here; the names are
// the GameCube ones where the member is the same.

// the title's two objects, the cursor and the demo figure. The Wii XObj is not reconstructed: these are
// its virtuals up to the one the title code calls, the unnamed ones only keep its place
class WiiTitleObject {
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
	virtual void vf040();
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
	virtual void vf070();
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
	virtual void vf09C();
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
	virtual void vf11C();
	virtual void vf120();
	virtual void vf124();
	virtual void vf128();
	virtual void vf12C();
	virtual void vf130();
	virtual void vf134();
	virtual void vf138();
	virtual void vf13C();
	virtual void vf140();
	virtual void vf144();
	virtual void vf148();
	virtual void vf14C();
	virtual void vf150();
	virtual void vf154();
	virtual void vf158();
	virtual void vf15C();
	virtual void vf160();
	virtual void UpdateObject(u32 frames); // _164

	u8 _004[0x9B0 - 0x004];
};
STATIC_ASSERT(sizeof(WiiTitleObject) == 0x9B0);

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
	u8 _00[0x4C];
	u8 mPointed; // _4C
	u8 mPicked;  // _4D
	u8 _4E[2];
};
STATIC_ASSERT(sizeof(CbPane) == 0x50);

// the panes the pointer is tested against this frame
struct CbPaneList {
	CbPane* mPanes[0x100]; // _000
	s32 mCount;            // _400
};

extern "C" {
extern CbPaneList gPointerPanes;
void AddPointerPane(CbPaneList* list, CbPane* pane);
}

class Title : public MenuUtil {
public:
	// _0000 vtable
	s32 mState;       // _0004
	s32 mMode;        // _0008
	u32 mOptionState; // _000C, 0 choosing, 2 leaving
	u8 _0010[0x1C - 0x10];
	f32 mPulseAngle;        // _001C
	s32 mHighlightDelay;    // _0020, frames until the selected line starts pulsing
	u32 mSpawnIndex;        // _0024
	WiiTitleObject mCursor; // _0028
	WiiTitleObject mDemo;   // _09D8
	u8 _1388[0x4928 - 0x1388];
	s32 mSelection;     // _4928, the entry of the title menu, -1 while the pointer is on none
	s32 mLastSelection; // _492C
	u8 _4930[0x4940 - 0x4930];
	f32 mTargetPanelY; // _4940
	u8 _4944[0x4950 - 0x4944];
	f32 mOffset; // _4950
	u8 _4954[0x496C - 0x4954];
	f32 mAlpha; // _496C
	u8 _4970[0x497C - 0x4970];
	s32 mFile;     // _497C, the save file picked, -1 with the pointer on none
	s32 mLastFile; // _4980
	u8 _4984[4];
	s32 mChoice;     // _4988, yes or no, -1 with the pointer on neither
	s32 mLastChoice; // _498C
	u8 _4990[0x499C - 0x4990];
	s32 mOptionLine;     // _499C, the options line, -1 with the pointer on none
	s32 mLastOptionLine; // _49A0
	u8 _49A4[4];
	f32 mTargetOptionAlpha; // _49A8
	u8 mPointerOn;          // _49AC, the options page's two settings; OK writes them to gGame.mMenuControls
	u8 mStickOn;            // _49AD
	u8 mSavedPointerOn;     // _49AE, what they were when the page opened, cancel puts them back
	u8 mSavedStickOn;       // _49AF
	u8 _49B0[0x4B48 - 0x49B0];
	TitleName mName; // _4B48
	u8 _4CE4[0x5058 - 0x4CE4];
	CbPane mOptionPanes[3];  // _5058, the pointer's rectangles over the three option lines
	f32 mOptionBar[3];       // _5148, the highlight on the options page, eased towards the line picked
	f32 mOptionBarTarget[2]; // _5154, where its second and third value go
	u8 _515C[0x5168 - 0x515C];
	f32 mOptionBarLines[3]; // _5168, where its first value goes, by line

	void UpdateSelections();
};

extern "C" void UpdateTitleOptions(Title* title, u32 frames);

#endif

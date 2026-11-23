#ifndef _GAME_MENUDATA_H
#define _GAME_MENUDATA_H

#include "types.h"
#include "Version.h"
#include "game/ut.h"

// one of MenuMain's rows and what picking it does
struct MenuMainItem {
	char* mText; // _00, "\x01" for a separator
	s32 mAction; // _04, -1 for a separator
};
STATIC_ASSERT(sizeof(MenuMainItem) == 0x08);

// a letter of a title that flies in, one after the other
struct MenuTitleChar {
	const char* mText; // _00
	f32 mFade;         // _04
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

#if !defined(VERSION_R24J01)
	inline void Set(const char* text, u32 color);
#endif
};
STATIC_ASSERT(sizeof(MenuTitle) == 0x19C);

// one of the status screen's item slots
struct MenuStatusEntry {
#if !defined(VERSION_R24J01)
	MenuStatusEntry() { }
	~MenuStatusEntry() { }
#endif
	s16 mItem;       // _00
	s16 mCount;      // _02
	CStatus mStatus; // _04
};
STATIC_ASSERT(sizeof(MenuStatusEntry) == 0x0C);

// the items the status screen pages through
struct MenuStatusTable {
#if !defined(VERSION_R24J01)
	~MenuStatusTable() { }
#endif
	s32 mPage;                   // _00, in fours
	MenuStatusEntry mItems[256]; // _04
	MenuStatusEntry mPrev[4];    // _C04
	MenuStatusEntry mNext[4];    // _C34

#if !defined(VERSION_R24J01)
	s16 GetItem(s32 i) { return mItems[i].mItem; }
#endif
};
STATIC_ASSERT(sizeof(MenuStatusTable) == 0xC64);

// a row of one of the chibi-pc lists
struct MenuPCItem {
	s16 mItem;        // _00, -1 for an empty row
	s16 mCount;       // _02, -1 for an empty row
	s16 mPrice;       // _04
	u8 mIsAttachment; // _06
	u8 _07;           // _07
};
STATIC_ASSERT(sizeof(MenuPCItem) == 0x08);

// the nine rows on screen and the ones just off either end
struct MenuPCPage {
	enum { Capacity = 20, VisibleRows = 9 };

	s32 mStart;                 // _00, list index of the first row
	MenuPCItem mRows[Capacity]; // _04
	s32 mIndex[Capacity];       // _A4, list index of each row
	MenuPCItem mPrev[1];        // _F4
	MenuPCItem mNext[1];        // _FC

#if !defined(VERSION_R24J01)
	s16 GetItem(s32 i) { return mRows[i].mItem; }
#endif
};
STATIC_ASSERT(sizeof(MenuPCPage) == 0x104);

// one of the status screen's tabs
struct MenuStatusTab {
	f32 mX;       // _00
	f32 mY;       // _04
	f32 mTargetX; // _08
	f32 mTargetY; // _0C
};
STATIC_ASSERT(sizeof(MenuStatusTab) == 0x10);

struct PCRankingRow {
	s32 rank;      // _00
	s32 nameIndex; // _04
	s32 score;     // _08
};
STATIC_ASSERT(sizeof(PCRankingRow) == 0x0C);

struct PCAmount {
	f32 shown;  // _00
	f32 target; // _04
};
STATIC_ASSERT(sizeof(PCAmount) == 0x08);

#endif

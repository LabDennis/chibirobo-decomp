#ifndef GAME_WII_MENUUTIL_H
#define GAME_WII_MENUUTIL_H

#include "types.h"

// CPadAct's flags the menus read
enum {
	PadActConfirm    = 1 << 0, // A
	PadActCancel     = 1 << 1, // B
	PadActStickUp    = 1 << 2, // the stick directions repeat while held
	PadActStickDown  = 1 << 3,
	PadActStickLeft  = 1 << 4,
	PadActStickRight = 1 << 5
};

// gGame.mMenuControls: how the menus are worked
enum { MenuControlPointer = 1 << 0, MenuControlStick = 1 << 1 };

// What the menus and the title screen ask the first controller. The class name is in the menu file's
// RTTI; that these functions are its members is our reading: the title code passes its object to them
// and they never use it. They are defined in menu.cpp. Their names are ours
class MenuUtil {
public:
	virtual ~MenuUtil() { }

	bool IsConfirmPressed();
	bool IsCancelPressed();
	// the stick counts only where gGame.mMenuControls has MenuControlStick
	bool IsStickRight();
	bool IsStickLeft();
	bool IsStickUp();
	bool IsStickDown();
	// the pointer is on screen and gGame.mMenuControls has MenuControlPointer
	bool IsPointerActive();
	// the stick moved while the pointer is the one choosing
	bool IsStickUsedOverPointer();
};

struct WiiMenuSaveSlot {
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
	s32 mStage;          // _28
	u32 mTimeMode;       // _2C
	u8 _30[0x40 - 0x30]; // _30
	s32 mState;          // _40
	u32 _44;             // _44
	u32 _48;             // _48
	u32 _4C;             // _4C
	u8 mActive;          // _50
};
STATIC_ASSERT(sizeof(WiiMenuSaveSlot) == 0x54);

extern "C" {
// the three save files the title's file menu shows
extern WiiMenuSaveSlot gTitleSaveSlots[3];
void ResetSaveSlotRecord(WiiMenuSaveSlot* slot);
}

// a rectangle the pointer can pick. The menu code's messages call it CbPane; the layout is not known
// beyond the two flags
struct CbPane {
	// the constructor (0x800345C0) and destructor (0x8002F924) Title's eleven panes are built and destroyed with
	CbPane()
	{
		_0C      = 0.0f;
		_10      = 0.0f;
		mKind    = 0;
		_48      = 1.0f;
		_4E      = TRUE;
		mPointed = FALSE;
		mPicked  = FALSE;
	}
	~CbPane() { }

	u8 _00[0xC];
	f32 _0C; // _0C
	f32 _10; // _10
	u8 _14[0x44 - 0x14];
	s32 mKind;   // _44, which entry the pane reports as
	f32 _48;     // _48
	u8 mPointed; // _4C
	u8 mPicked;  // _4D
	u8 _4E;      // _4E
	u8 _4F;
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

#endif

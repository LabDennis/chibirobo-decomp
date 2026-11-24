#ifndef GAME_WII_MENUUTIL_H
#define GAME_WII_MENUUTIL_H

#include "types.h"
#include "Dolphin/vec.h"
#include "Dolphin/mtx.h"
#include "game/ut.h"
#include "nw4r/math/math_types.h"
#include "nw4r/lyt/lyt_types.h"

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

struct CbPane;

// What the menus and the title screen ask the first controller. RTTI names the class and gives it no
// bases; its functions never use the object, so it holds nothing and has no vtable. They are defined in
// menu.cpp. Their names are ours
class MenuPad {
public:
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

// What the pointer code calls on the object in gPointerPanes.mMenu: RTTI names the class and gives it no
// bases, and the six slots after its vtable header are these. Their names are ours
class CContainCallback {
public:
	virtual void vf08(CbPane*) { }
	virtual void vf0C(CbPane*) { }
	virtual void vf10(CbPane*) { }
	virtual void vf14(CbPane*) { }
	virtual void vf18(nw4r::math::VEC2*) { }
	virtual void vf1C(nw4r::math::VEC2*) { }
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

// a rectangle the pointer can pick, laid out like an nw4r::lyt pane round its centre. The menu code's messages
// call it CbPane
struct CbPane {
	// the constructor (0x800345C0) and destructor (0x8002F924) Title's eleven panes are built and destroyed with
	CbPane()
	{
		mWidth   = 0.0f;
		mHeight  = 0.0f;
		mKind    = 0;
		_48      = 1.0f;
		mEnabled = TRUE;
		mPointed = FALSE;
		mPicked  = FALSE;
	}
	~CbPane() { }

	Vec mTrans;                // _00, the centre
	f32 mWidth;                // _0C
	f32 mHeight;               // _10
	nw4r::math::MTX34 mGlbMtx; // _14, set from mTrans by each pointer test
	s32 mKind;                 // _44, which entry the pane reports as
	f32 _48;                   // _48
	u8 mPointed;               // _4C
	u8 mPicked;                // _4D
	u8 mEnabled;               // _4E, the pointer can pick it
	u8 _4F;
};
STATIC_ASSERT(sizeof(CbPane) == 0x50);

// where a pane sits, one entry of the tables the title and the menus place their panes from
struct CbPaneLayout {
	nw4r::math::VEC3 mTrans; // _00
	nw4r::math::VEC2 mScale; // _0C
	nw4r::lyt::Size mSize;   // _14
	CColor mColor;           // _1C
	s32 mKind;               // _24
};
STATIC_ASSERT(sizeof(CbPaneLayout) == 0x28);

// the panes the pointer is tested against this frame
struct CbPaneList {
	CbPaneList()
	{
		mCount   = 0;
		mMenu    = NULL;
		_409     = 0;
		mEnabled = TRUE;
		mActive  = FALSE;
	}

	CbPane* mPanes[0x100];   // _000
	s32 mCount;              // _400
	CContainCallback* mMenu; // _404, the menu whose panes are tested, NULL disables the pointer
	u8 _408;
	u8 _409;
	u8 mEnabled; // _40A
	u8 mActive;  // _40B
};

extern "C" {
extern CbPaneList gPointerPanes;
void AddPointerPane(CbPaneList* list, CbPane* pane);
}

#endif

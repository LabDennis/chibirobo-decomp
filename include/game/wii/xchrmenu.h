#ifndef GAME_WII_XCHRMENU_H
#define GAME_WII_XCHRMENU_H

#include "game/ringmenu.h"

// the two ring menus side by side, a member of the Wii player object at _19EE4
// (the GameCube keeps the same struct in xchr.h). retail's ctor stores each
// menu's mChr NULL write before its vtable (OctaMenu's inline ctor)
struct XObjChrMenu {
	AtcMenu mAtc; // _00
	DrsMenu mDrs; // _290
	f32 mFade;    // _520
	f32 mCursorY; // _524
	f32 mTargetY; // _528
	f32 mCursorX; // _52C
	f32 mTargetX; // _530
	s32 mSel;     // _534
	s32 mPrevSel; // _538, put back into mSel when the rings close

	XObjChrMenu();  // 0x800C0F70, defined in xpaddtor.cpp
	~XObjChrMenu(); // 0x800C1058, the deleting dtor used for member destruction; no vtable

	// 0x800EF3B8 and 0x800EF3D8: the item a ring's pointer starts on when it opens next, MENU 1001 sets them
	void PointAtcItem(s32 item);
	void PointDrsItem(s32 item);

	u8 IsOpen() const { return mAtc.mOpen != 0 || mDrs.mOpen != 0; }
	bool IsOpening() const { return mAtc.mOpen > 0 || mDrs.mOpen > 0; }
	bool IsClosing() const { return mAtc.mOpen < 0 || mDrs.mOpen < 0; }
};
STATIC_ASSERT(sizeof(XObjChrMenu) == 0x53C); // fills the player member _19EE4.._1A420 exactly

#endif

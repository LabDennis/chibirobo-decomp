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

#endif

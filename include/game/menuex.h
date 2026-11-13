#ifndef _GAME_MENUEX_H
#define _GAME_MENUEX_H

#include "types.h"
#include "Version.h"
#include "game/title.h"

class CMenu;

// the hand over a menu, MenuEx's
class MenuCursor : public Cursor {
public:
	MenuCursor();

	virtual void ResetObjectState();       // _98
	virtual void UpdateObject(u32 frames); // _A4
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(MenuCursor) == 0x978);
#else
STATIC_ASSERT(sizeof(MenuCursor) == 0x988);
#endif

// not a CMenu, it owns the menus and the cursor they share
class MenuEx {
public:
	MenuEx();

	BOOL mActive; // _00, a menu is open
#if defined(VERSION_JP)
	CMenu* mMenus[13]; // _04
#else
	CMenu* mMenus[8];    // _04, evt indexes these by id
	u8 _24[0x28 - 0x24]; // _24
#endif
	MenuCursor mCursor; // _28, _38 in japan
	// _9B0 vtable, then 4 bytes of padding

	virtual ~MenuEx() { } // _08
};
STATIC_ASSERT(sizeof(MenuEx) == 0x9B8);

#endif

#include "game/hsd_jobj.h"
#include "game/menuex.h"

extern "C" {
extern void* __vt__6MenuEx[];
MenuCursor* __ct__10MenuCursorFv(MenuCursor* cursor);
}

// todo: weak copy
/**
 * @note Address: 0x8001B38C
 * @note Size: 0x40
 */
extern "C" MenuEx* __ct__6MenuExFv(MenuEx* menu)
{
	*(void***)((u8*)menu + 0x9B0) = __vt__6MenuEx; // _9B0 vtable
	__ct__10MenuCursorFv(&menu->mCursor);
	return menu;
}

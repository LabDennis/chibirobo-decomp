#include "game/octamenu.h"

void operator delete(void* object);
extern "C" {
extern void* __vt__7DrsMenu[];
extern void* __vt__8OctaMenu[];
}

// todo: cleanup, the vtable is right before _280
static inline void SetVtable(OctaMenu* menu, void** vtable)
{
	((void***)&menu->mMenuType)[-1] = vtable;
}

// todo: weak copy
/**
 * @note Address: 0x80093B7C
 * @note Size: 0x5C
 */
extern "C" DrsMenu* __dt__7DrsMenuFv(DrsMenu* menu, s32 flags)
{
	if (menu != NULL) {
		SetVtable(menu, __vt__7DrsMenu);
		// ~OctaMenu, inlined with its own null check
		if (menu != NULL)
			SetVtable(menu, __vt__8OctaMenu);
		if ((s16)flags > 0)
			::operator delete(menu);
	}
	return menu;
}

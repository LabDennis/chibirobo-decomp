#include "game/octamenu.h"

void operator delete(void* object);
extern "C" {
extern void* __vt__7AtcMenu[];
extern void* __vt__8OctaMenu[];
}

// todo: cleanup
static inline void SetVtable(OctaMenu* menu, void** vtable)
{
	// the vtable sits just before _280
	*((void***)&menu->mMenuType - 1) = vtable;
}

/**
 * @note Address: 0x800BC480
 * @note Size: 0x5C
 */
// todo: weak copy
extern "C" AtcMenu* __dt__7AtcMenuFv(AtcMenu* menu, s32 flags)
{
	if (menu != NULL) {
		SetVtable(menu, __vt__7AtcMenu);
		menu->OctaMenu::~OctaMenu();
		if ((s16)flags > 0)
			::operator delete(menu);
	}
	return menu;
}

/**
 * @note Address: 0x800BC4DC
 * @note Size: 0x48
 */
// todo: weak copy
extern "C" OctaMenu* __dt__8OctaMenuFv(OctaMenu* menu, s32 flags)
{
	if (menu != NULL) {
		SetVtable(menu, __vt__8OctaMenu);
		if ((s16)flags > 0)
			::operator delete(menu);
	}
	return menu;
}

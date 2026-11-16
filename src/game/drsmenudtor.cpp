#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/ringmenu.h"

// the weak dtor copies the GameCube file spells out by hand; on Wii they are plain
// out-of-line definitions (retail emits no vtable store and no base-dtor call), so
// ~OctaMenu is defined inline here as well: it has to be in the TU to be inlined
// away, and only the inline copy avoids re-emitting OctaMenu's vtable
inline OctaMenu::~OctaMenu()
{
}
DrsMenu::~DrsMenu()
{
}
#else
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
#endif

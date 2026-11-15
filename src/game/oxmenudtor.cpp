#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/octamenu.h"

// the weak dtor copies the GameCube file spells out by hand; on Wii the mark dtor
// (0x800C1308) is the minimal body and OXMenu's emits __destroy_arr(mMark, dtor, 0x9B8, 2)
OXMenuMark::~OXMenuMark()
{
}
OXMenu::~OXMenu()
{
}
#else
#include "game/octamenu.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

void operator delete(void* object);
extern "C" {
extern void* __vt__6OXMenu[];
void* __dt__10OXMenuMarkFv(void* object, s32 flags);
}

/**
 * @note Address: 0x800BD268
 * @note Size: 0x70
 */
// todo: weak copy
extern "C" OXMenu* __dt__6OXMenuFv(OXMenu* menu, s32 flags)
{
	if (menu != NULL) {
		// the vtable is its last word
		*(void***)((u8*)menu + sizeof(OXMenu) - 4) = __vt__6OXMenu;
#if !defined(VERSION_JP)
		__destroy_arr(menu->mMark, (ConstructorDestructor*)__dt__10OXMenuMarkFv, sizeof(OXMenuMark), 2);
#endif
		if ((s16)flags > 0)
			::operator delete(menu);
	}
	return menu;
}
#endif

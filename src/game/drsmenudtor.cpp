#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/ringmenu.h"

// Wii only: the GameCube spells these weak dtor copies out by hand in obja_chr.cpp; on Wii they are plain
// out-of-line definitions (retail emits no vtable store and no base-dtor call), so
// ~OctaMenu is defined inline here as well: it has to be in the TU to be inlined
// away, and only the inline copy avoids re-emitting OctaMenu's vtable
inline OctaMenu::~OctaMenu()
{
}
DrsMenu::~DrsMenu()
{
}
#endif

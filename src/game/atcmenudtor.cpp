#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/ringmenu.h"

// Wii only: the GameCube spells these weak dtor copies out by hand in obja_chr.cpp; on Wii both are plain
// out-of-line definitions (retail emits no vtable store and no base-dtor call)
OctaMenu::~OctaMenu()
{
}
AtcMenu::~AtcMenu()
{
}
#endif

#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/octamenu.h"

// Wii only: the GameCube spells these weak dtor copies out by hand in obja_chr.cpp; on Wii the mark dtor
// (0x800C1308) is the minimal body and OXMenu's emits __destroy_arr(mMark, dtor, 0x9B8, 2)
OXMenuMark::~OXMenuMark()
{
}
OXMenu::~OXMenu()
{
}
#endif

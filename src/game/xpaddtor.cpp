#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/xpad.h"
#include "game/wii/xchrmenu.h"

// Wii only: the GameCube spells these weak dtor copies out by hand in obja_chr.cpp; on Wii the emitted body is
// the minimal one (retail 0x800C0F30, no vtable store and no base call), so ~CPadOne is
// defined inline here to be inlined away without re-emitting CPadOne's vtable
inline CPadOne::~CPadOne()
{
}
XPad::~XPad()
{
}
XObjChrMenu::XObjChrMenu()
{
}
// NONMATCHING: both functions match; the unit cannot link byte-exact because the
// XObjChrMenu ctor's AtcMenu/DrsMenu vtable stores resolve to compiled-out vtables
// (atcmenudtor.o) whose placement shifts .data, while naming them in symbols.txt
// collides with those same strong emissions
#endif

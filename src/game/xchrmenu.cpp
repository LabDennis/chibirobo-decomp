#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/wii/xchrmenu.h"
#include "game/octamenu.h"

// the menus' member dtors have to be visible here to inline away: retail's
// ~XObjChrMenu (0x800C1058) is the minimal deleting dtor with no member-dtor calls
inline OctaMenu::~OctaMenu()
{
}
inline AtcMenu::~AtcMenu()
{
}
inline DrsMenu::~DrsMenu()
{
}
XObjChrMenu::~XObjChrMenu()
{
}
OXMenu::OXMenu()
{
	mChr = NULL;
}
OXMenuMark::OXMenuMark()
{
}
#endif

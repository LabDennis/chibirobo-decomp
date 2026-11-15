// The menus, one file from 0x8004F8F8 to 0x800870E8: its .sdata2 literals (0x805A2F70-0x805A3350) are read
// by functions over that whole range and by nothing else, it has one string pool (from 0x802C67C8) and one
// static initializer (0x8008502C), behind which only its weak destructors and this-adjusting thunks follow.
// The pool holds "menu_item.cpp", "menu_status.cpp" and "menu_save.cpp", so those are files this one
// includes; its own name is not in the binary, menu.cpp is ours.
// NONMATCHING: only MenuUtil's input functions (0x800505CC-0x80050828) have source so far.
#include "game/wii/menuutil.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"

bool MenuUtil::IsConfirmPressed()
{
	if (gWiiPointer.Check(PadActConfirm))
		return true;
	return false;
}

bool MenuUtil::IsCancelPressed()
{
	if (gWiiPointer.Check(PadActCancel))
		return true;
	return false;
}

bool MenuUtil::IsStickRight()
{
	if (gWiiPointer.Check(PadActStickRight) && (gGame.mMenuControls & MenuControlStick))
		return true;
	return false;
}

bool MenuUtil::IsStickLeft()
{
	if (gWiiPointer.Check(PadActStickLeft) && (gGame.mMenuControls & MenuControlStick))
		return true;
	return false;
}

bool MenuUtil::IsStickUp()
{
	if (gWiiPointer.Check(PadActStickUp) && (gGame.mMenuControls & MenuControlStick))
		return true;
	return false;
}

bool MenuUtil::IsStickDown()
{
	if (gWiiPointer.Check(PadActStickDown) && (gGame.mMenuControls & MenuControlStick))
		return true;
	return false;
}

bool MenuUtil::IsPointerActive()
{
	if (gWiiPointer.mPointerValid && (gGame.mMenuControls & MenuControlPointer))
		return true;
	return false;
}

bool MenuUtil::IsStickUsedOverPointer()
{
	if (IsPointerActive()) {
		if (gWiiPointer.Check(PadActStickLeft) || gWiiPointer.Check(PadActStickRight) || gWiiPointer.Check(PadActStickUp)
		    || gWiiPointer.Check(PadActStickDown))
			return true;
	}
	return false;
}

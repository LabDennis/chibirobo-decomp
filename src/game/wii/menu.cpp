// The menus, one file from 0x8004F8F8 to 0x800870E8: its .sdata2 literals (0x805A2F70-0x805A3350) are read
// by functions over that whole range and by nothing else, it has one string pool (from 0x802C67C8) and one
// static initializer (0x8008502C), behind which only its weak destructors and this-adjusting thunks follow.
// The pool holds "menu_item.cpp", "menu_status.cpp" and "menu_save.cpp", so those are files this one
// includes; its own name is not in the binary, menu.cpp is ours.
// NONMATCHING: most menu state, drawing and layout functions still have no source.
#include "game/wii/menuutil.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/title.h"
#include "game/globals.h"
#include "game/evt.h"

extern "C" void LayoutMainMenuRows(MenuMain* menu);

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

void CMenu::Open()
{
	mState = 1;
	Resume();
	mWindowOpen = TRUE;
}

void MenuMain::Open()
{
	CMenu::Open();
	PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
	gGame.SetStatusFlags(2);
}

void MenuMain::Resume()
{
	if (mSuspended) {
		gGame.SetStatusFlags(2);
	}
	CMenu::Resume();
}

extern "C" void ClearMainMenuExtraRows(MenuMain* menu)
{
	for (int i = 0; i < 5; i++) {
		menu->mExtraRows[i].mText   = NULL;
		menu->mExtraRows[i].mAction = 0;
	}
	LayoutMainMenuRows(menu);
}

extern "C" void AddMainMenuExtraRow(MenuMain* menu, char* text, s32 id)
{
	for (int i = 0; i < 5; i++) {
		if (menu->mExtraRows[i].mText == NULL) {
			menu->mExtraRows[i].mText   = text;
			menu->mExtraRows[i].mAction = id;
			break;
		}
	}
	LayoutMainMenuRows(menu);
}

extern "C" BOOL IsUtilibotStatusIconUnlocked()
{
	BOOL found = FALSE;
	u32 id     = GetStatusIconEventFlagId(3);
	if (gEventFlags.mFlags[id / 32] & (1u << (id % 32))) {
		found = TRUE;
	}
	return found;
}

void CMenu::Suspend(BOOL fade)
{
	mSuspended   = TRUE;
	mSuspendFade = fade;
}

void CMenu::Resume()
{
	mSuspended   = FALSE;
	mSuspendFade = FALSE;
}

extern "C" void AddPointerPane(CbPaneList* list, CbPane* pane)
{
	if (list->mCount < 0x100) {
		list->mPanes[list->mCount++] = pane;
	}
}

extern "C" void ResetSaveSlotRecord(WiiMenuSaveSlot* slot)
{
	slot->mUsed       = 0;
	slot->mLevel      = 0;
	slot->mBattery    = 0.0f;
	slot->mMaxBattery = 0.0f;
	slot->mMoney      = 0;
	slot->mExp        = 0;
	slot->mScrap      = 0;
	slot->mRank       = 0;
	slot->mHours      = 0;
	slot->mMinutes    = 0;
	slot->mStage      = 0;
	slot->mTimeMode   = 0;
	slot->mState      = 0;
	slot->mActive     = 0;
	slot->_44         = 0;
	slot->_48         = 0;
	slot->_4C         = 0;
}

extern "C" bool IsSaveSlotIntact(const WiiMenuSaveSlot* slot)
{
	return slot->mState != 1;
}

extern "C" const char* GetTitleAndSaveMenuText(s32 index)
{
	return GetLocalizedInterfaceText(index + 95);
}

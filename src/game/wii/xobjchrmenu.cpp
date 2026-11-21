// the Wii ring-menu methods and helpers that follow the GameCube's obja_chr.cpp
// menu section: the AtcMenu/DrsMenu vtable methods, the costume ring's init and
// open, closing the pair, and the OX menu's confirm

#include "game/wii/xchrmenu.h"
#include "game/ringmenu.h"
#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/xobjcamera.h"
#include "game/xobjattachment.h"
#include "game/cmdline.h"
#include "game/wii/csound.h"
#include "game/inventory.h"
#include "game/globals.h"
#include "game/gmath.h"
#include <math.h>

extern "C" void ShowOXCircle(XObjOX* obj);
extern "C" void ShowOXCross(XObjOX* obj);

static inline void AddPulse(s32& red, s32& green, s32& blue, const OctaMenu* menu, s32 index)
{
	red += (s32)(25.0f * (f32)sin(1.2f * menu->mItems[index].mPulsePhase) + 25.0f);
	green += (s32)(25.0f * (f32)sin(1.2f * menu->mItems[index].mPulsePhase) + 25.0f);
	blue += (s32)(25.0f * (f32)sin(1.2f * menu->mItems[index].mPulsePhase) + 25.0f);
}

/**
 * @note Address: 0x800ED45C
 * @note Size: 0x38
 */
BOOL AtcMenu::IsItemEquipped(s32 index)
{
	return mItems[index].mType == ((WiiPlayerState*)mChr)->GetAttachment();
}

/**
 * @note Address: 0x800ED494
 * @note Size: 0xAC
 */
s32 AtcMenu::GetItemIconTextureIndex(s32 index)
{
	s32 icons[] = { 127, 91, 92, 93, 94, 96, 95, 97, 127 };
	if (index == -1) {
		return 0x6A;
	}
	if (index == -2) {
		return 0x6C;
	}
	if (mItems[index].mAvailable > 0) {
		return icons[mItems[index].mType];
	}
	return 0x69;
}

// NONMATCHING (~85%): the request<0 && lift-state gate stays in cr0 where
// retail keeps it in cr1 for the inlined GetAttachment select below it;
// sLabel also lands on a local sdata label instead of retail's lbl_805A10DC
/**
 * @note Address: 0x800ED540
 * @note Size: 0xA8
 */
const char* AtcMenu::GetItemLabel(s32 index)
{
	static const char* sLabel = "";
	WiiPlayerState* player    = (WiiPlayerState*)mChr;
	if (mItems[index].mAvailable > 0) {
		if (player->mAttachmentRequest < 0 && player->mLift.mState != 21 && player->mLift.mState != 22 && player->mLift.mState != 23) {
			if (mItems[index].mType == player->GetAttachment()) {
				sLabel = GetLocalizedInterfaceText(13);
			} else {
				sLabel = (const char*)GetAttachmentName(mItems[index].mType);
			}
		}
	} else {
		sLabel = GetLocalizedInterfaceText(15);
	}
	return sLabel;
}

// NONMATCHING (~89%): register/scheduling wall — retail caches mChr in r9
// across the three inlined GetAttachment selects, ours rematerializes the
// load per block; the branch structure and AddPulse tail are identical
/**
 * @note Address: 0x800ED5E8
 * @note Size: 0x1F8
 */
u32 AtcMenu::GetItemColor(s32 index)
{
	s32 red, green, blue;
	WiiPlayerState* player = (WiiPlayerState*)mChr;
	if (mItems[index].mAvailable == 0) {
		red   = 0x20;
		green = 0x20;
		blue  = 0x20;
	} else if (mMenuType == 3) {
		if (mItems[index].mType == player->GetAttachment()) {
			red   = 0x32;
			green = 0xC8;
			blue  = 0xC8;
		} else {
			red   = 0x32;
			green = 0x32;
			blue  = 0xC8;
		}
	} else if (index == mCursorIndex && mItems[index].mType == player->GetAttachment()) {
		red   = 0xC8;
		green = 0x64;
		blue  = 0x96;
	} else if (index == mCursorIndex) {
		red   = 0xC8;
		green = 0x64;
		blue  = 0x64;
	} else if (mItems[index].mType == player->GetAttachment()) {
		red   = 0x32;
		green = 0xC8;
		blue  = 0xC8;
	} else {
		red   = 0x32;
		green = 0x32;
		blue  = 0xC8;
	}
	if (index == mCursorIndex) {
		AddPulse(red, green, blue, this, index);
	}
	return ((u32)red << 24) | ((u8)green << 16) | ((u8)blue << 8);
}

// the costume ring's item set and menu defaults, run once when the player spawns.
// The GameCube split this over SetupPlayerRing/SetRingItems; the Wii gives each
// ring an Init that also latches the command line's menu type override
// NONMATCHING (~78%): scheduling/register-allocation residue around the
// field-init block and the cmdline menu-type clamp; also the float literals
// pool to local sdata2 labels where retail has named unit constants
/**
 * @note Address: 0x800ED7E0
 * @note Size: 0x10C
 */
void DrsMenu::Init(XObjChr* chr)
{
	mChr         = chr;
	mOpen        = 0;
	mCursorIndex = 0;
	mMenuType    = gCmdLineArgs.mAttachmentMenuType;
	if (mMenuType > 3) {
		mMenuType = 0;
	}
	mFocus                   = 0;
	mKeepPointedItemOnOpen   = 0;
	mPendingFirstVisibleItem = -1;
	mLastPointedItem         = 3;
	mHeadTurn                = NAN;
	mRowOffset               = 0.0f;
	mFirstVisibleItem        = 0;
	_10                      = 1.0f;
	if (mMenuType == 3) {
		mItems[0].mType = 1;
		mItems[1].mType = 4;
		mItems[2].mType = 6;
		mItems[3].mType = 3;
		mItems[4].mType = 2;
		mItems[5].mType = 7;
		mItems[6].mType = 8;
		mItems[7].mType = 5;
	} else {
		mItems[0].mType = 3;
		mItems[1].mType = 5;
		mItems[2].mType = 4;
		mItems[3].mType = 1;
		mItems[4].mType = 6;
		mItems[5].mType = 8;
		mItems[6].mType = 7;
		mItems[7].mType = 2;
	}
	mFirstVisibleItem = 4;
	mRowOffset        = 0.9f;
}

/**
 * @note Address: 0x800ED8EC
 * @note Size: 0x2C8
 */
void DrsMenu::Open(s32 focus)
{
	if (mMenuType == 3) {
		s16 ids[] = { 0x18, 0x20, 0x22, 0x19, 0x1A, 0x1E, 0x1F, 0x21 };
		for (s32 i = 0; i < 8; i++) {
			if (mMenuType == 2) {
				mItems[i].mAvailable = 1;
			} else {
				mItems[i].mAvailable = FindOwnedInventoryItemIndex(((WiiPlayerState*)mChr)->mParams.mInventory.mItems, ids[i]) >= 0;
			}
		}
	} else {
		s16 ids[] = { 0x19, 0x21, 0x20, 0x18, 0x22, 0x1F, 0x1E, 0x1A };
		for (s32 i = 0; i < 8; i++) {
			if (mMenuType == 2) {
				mItems[i].mAvailable = 1;
			} else {
				mItems[i].mAvailable = FindOwnedInventoryItemIndex(((WiiPlayerState*)mChr)->mParams.mInventory.mItems, ids[i]) >= 0;
			}
		}
	}
	OctaMenu::Open(focus);
	if (mMenuType == 3) {
		if (mPendingFirstVisibleItem >= 0) {
			mLastPointedItem = mPendingFirstVisibleItem - 1;
			if (mLastPointedItem < 0) {
				mLastPointedItem += 7;
			}
			mPendingFirstVisibleItem = -1;
		} else if (((WiiPlayerState*)mChr)->mSuits.mCurrent != 0 && mKeepPointedItemOnOpen == 0) {
			for (s32 i = 0; i < 8; i++) {
				if (mItems[i].mType == ((WiiPlayerState*)mChr)->mSuits.mCurrent) {
					mLastPointedItem = i - mFirstVisibleItem;
					if (mLastPointedItem < 0) {
						mLastPointedItem += 7;
					}
				}
			}
		}
	}
	mKeepPointedItemOnOpen = 0;
}

// the suits this menu's items actually equip; ids 0 and 5 aren't selectable
// suits (0 means "take the suit off"), anything else queues a suit change
// NONMATCHING (~95%): the canEquip local pools to a local rodata label where
// retail references the global lbl_802DA800 object, plus the mChr reload
// scheduling in the else branch
/**
 * @note Address: 0x800EDBB4
 * @note Size: 0xFC
 */
void DrsMenu::ConfirmSelection()
{
	s8 canEquip[]          = { 0, 1, 1, 1, 1, 0, 1, 1, 1 };
	s32 type               = mItems[mCursorIndex].mType;
	WiiPlayerState* player = (WiiPlayerState*)mChr;
	if (canEquip[type]) {
		if (type == player->mSuits.mCurrent) {
			if (player->mSuits.mChangeStep == 0) {
				player->mSuits.mRequested  = 0;
				player->mSuits.mChangeStep = 1;
			}
		} else if (player->mSuits.mChangeStep == 0) {
			player->mSuits.mRequested  = type;
			player->mSuits.mChangeStep = 1;
		}
	} else if (player->mSuits.mChangeStep == 0) {
		player->mSuits.mRequested  = 0;
		player->mSuits.mChangeStep = 1;
	}
}

// NONMATCHING (~98%): register allocation only — retail puts the suit into
// r0 and the item type into r3, ours does the reverse
/**
 * @note Address: 0x800EDCB0
 * @note Size: 0x28
 */
BOOL DrsMenu::IsItemEquipped(s32 index)
{
	return ((WiiPlayerState*)mChr)->mSuits.mCurrent == mItems[index].mType;
}

/**
 * @note Address: 0x800EDCD8
 * @note Size: 0xAC
 */
s32 DrsMenu::GetItemIconTextureIndex(s32 index)
{
	s32 icons[] = { 127, 98, 100, 99, 101, 127, 102, 103, 104 };
	if (index == -1) {
		return 0x6B;
	}
	if (index == -2) {
		return 0x6D;
	}
	if (mItems[index].mAvailable > 0) {
		return icons[mItems[index].mType];
	}
	return 0x69;
}

// NONMATCHING (~99%): compare operand order only (cmpw r3, r0 vs r0, r3)
/**
 * @note Address: 0x800EDD84
 * @note Size: 0x40
 */
const char* DrsMenu::GetItemLabel(s32 index)
{
	if (mItems[index].mAvailable > 0) {
		if (((WiiPlayerState*)mChr)->mSuits.mCurrent == mItems[index].mType) {
			return GetLocalizedInterfaceText(14);
		}
		return (const char*)GetCostumeName(mItems[index].mType);
	}
	return GetLocalizedInterfaceText(15);
}

// NONMATCHING (~88%): same mChr-cache wall as AtcMenu::GetItemColor
/**
 * @note Address: 0x800EDDC4
 * @note Size: 0x1C8
 */
u32 DrsMenu::GetItemColor(s32 index)
{
	s32 red, green, blue;
	WiiPlayerState* player = (WiiPlayerState*)mChr;
	if (mItems[index].mAvailable == 0) {
		red   = 0x20;
		green = 0x20;
		blue  = 0x20;
	} else if (mMenuType == 3) {
		if (mItems[index].mType == player->mSuits.mCurrent) {
			red   = 0x32;
			green = 0xC8;
			blue  = 0xC8;
		} else {
			red   = 0x32;
			green = 0x32;
			blue  = 0xC8;
		}
	} else if (index == mCursorIndex && mItems[index].mType == player->mSuits.mCurrent) {
		red   = 0xC8;
		green = 0x64;
		blue  = 0x96;
	} else if (index == mCursorIndex) {
		red   = 0xC8;
		green = 0x64;
		blue  = 0x64;
	} else if (mItems[index].mType == player->mSuits.mCurrent) {
		red   = 0x32;
		green = 0xC8;
		blue  = 0xC8;
	} else {
		red   = 0x32;
		green = 0x32;
		blue  = 0xC8;
	}
	if (index == mCursorIndex) {
		AddPulse(red, green, blue, this, index);
	}
	return ((u32)red << 24) | ((u8)green << 16) | ((u8)blue << 8);
}

// UpdatePlayerEquipmentRings (0x800EDF8C), DrawPlayerEquipmentRings (0x800EE8B0):
// the pointer-driven ring drivers, still to reconstruct

/**
 * @note Address: 0x800EF2AC
 * @note Size: 0xF8
 */
extern "C" void ClosePlayerEquipmentRings(XObjChrMenu* menu, s32 result)
{
	if (result == 0) {
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		if (menu->mAtc.mMenuType != 3) {
			menu->mSel = menu->mPrevSel;
		}
	} else if (result > 0) {
		PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
	} else if (result < 0) {
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		if (menu->mAtc.mMenuType != 3) {
			menu->mSel = menu->mPrevSel;
		}
	}
	menu->mAtc.Select(result);
	menu->mDrs.Select(result);
}

/**
 * @note Address: 0x800EF3A4
 * @note Size: 0x14
 */
void OctaMenu::OpenActive()
{
	Open(0);
}

// the visible slot each pointed-at item starts on: the last slot sits under the
// pointer when the ring opens, so an item's slot is (item + 6) % 7 written as a table
static const s32 sAtcPointedItemSlot[7] = { 6, 0, 1, 2, 3, 4, 5 };
static const s32 sDrsPointedItemSlot[7] = { 6, 0, 1, 2, 3, 4, 5 };

// NONMATCHING (~97%): register allocation and the table's own rodata label
// (retail reads the global lbl_80305B58)
/**
 * @note Address: 0x800EF3B8
 * @note Size: 0x20
 */
void XObjChrMenu::PointAtcItem(s32 item)
{
	mAtc.mKeepPointedItemOnOpen = 1;
	mAtc.mLastPointedItem       = sAtcPointedItemSlot[item];
}

// NONMATCHING (~97%): register allocation and the table's own rodata label
// (retail reads the global lbl_80305B74 block shared with the ring drivers)
/**
 * @note Address: 0x800EF3D8
 * @note Size: 0x20
 */
void XObjChrMenu::PointDrsItem(s32 item)
{
	mDrs.mKeepPointedItemOnOpen = 1;
	mDrs.mLastPointedItem       = sDrsPointedItemSlot[item];
}

// the yes/no menu's confirm and cancel, split out of the GameCube's
// HandleOXMenuInput; the head-aim record reset runs either way
// NONMATCHING (~90%): scheduling around the sound calls and the head-aim
// record writes; float literals pool to local sdata2 labels
/**
 * @note Address: 0x800EF3F8
 * @note Size: 0x194
 */
extern "C" void ConfirmOXMenuSelection(WiiOXMenu* menu, s32 result)
{
	WiiPlayerState* chr = (WiiPlayerState*)menu->mChr;
	menu->mStarted      = -1;
	if (result > 0) {
		PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		menu->mConfirmedIndex = menu->mSelectedIndex;
		menu->mChr->SetAnim(0x78, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		if (!(menu->mCrossMask & (1 << menu->mConfirmedIndex))) {
			ShowOXCircle((XObjOX*)chr->mOXObj);
		} else {
			ShowOXCross((XObjOX*)chr->mOXObj);
		}
	} else if (result < 0) {
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		menu->mConfirmedIndex = -1;
		menu->mChr->SetAnim(0x5F, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		menu->mChr->SetRotateY(menu->mStartRot, DEG2RAD * 12.0f);
	}
	XObjScnCamInitRec* aim = (XObjScnCamInitRec*)chr->_987C;
	aim->mSnap[0] = aim->mSnap[1] = 2;
	aim->m_0C                     = NULL;
	aim->mTarget.x                = NAN;
	aim->m_20                     = NULL;
	aim->m_24                     = NAN;
	aim->m_28                     = NAN;
	aim->m_2C                     = 0.0f;
	aim->m_30                     = 0.0f;
	aim->m_4C                     = 0.0f;
	aim->m_50                     = 0.0f;
	aim->m_54                     = 0.0f;
}

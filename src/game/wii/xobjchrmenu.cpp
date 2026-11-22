// the Wii ring-menu methods and helpers that follow the GameCube's obja_chr.cpp
// menu section: the AtcMenu/DrsMenu vtable methods, the costume ring's init and
// open, closing the pair, and the OX menu's confirm

#include "game/wii/xchrmenu.h"
#include "game/ringmenu.h"
#include "game/wii/cgame.h"
#include "game/wii/cpad.h"
#include "game/wii/cpadact.h"
#include "game/wii/xobj.h"
#include "game/wii/xobjcamera.h"
#include "game/xobjattachment.h"
#include "game/cmdline.h"
#include "game/wii/csound.h"
#include "game/inventory.h"
#include "game/globals.h"
#include "game/gmath.h"
#include <cmath>
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

extern "C" s32 HandleEquipmentRingInput(OctaMenu* menu, BOOL active);
extern "C" void UpdateEquipmentRingAnimation(OctaMenu* menu, u32 frames);
extern "C" void ClosePlayerEquipmentRings(XObjChrMenu* menu, s32 result);
// the pointer-hint hooks of the ring menu, kept in retail as empty stubs
extern "C" void fn_8002BBD8(CGs* gs, f32 x, f32 y);
extern "C" void fn_8002BBDC(CGs* gs);
extern "C" void DrawEquipmentRing(OctaMenu* menu);
const char* GetLocalizedInterfaceText(s32 id);

extern s32 gRefreshRate;
extern s32 gPointerScreenWidth;
extern s32 gScreenSafeLeft;
extern s32 gScreenSafeRight;
extern s32 gScreenSafeTop;
extern s32 gScreenSafeBottom;

// the ring accepts menu input while fully faded in and past its opening glow
static inline u8 RingReady(const OctaMenu& ring)
{
	return ring.mOpen > 1 && (ring.mGlow == 0.0f || ring.mGlow == 1.0f);
}

static inline BOOL CanSelectEquipmentRing(const XObjChrMenu* menu)
{
	return RingReady(menu->mAtc) || RingReady(menu->mDrs);
}

static inline void FocusEquipmentRing(XObjChrMenu* menu)
{
	if (menu->mSel == 0) {
		menu->mAtc.mFocus       = 0;
		menu->mDrs.mFocus       = 1;
		menu->mDrs.mCursorIndex = -1;
	} else {
		menu->mAtc.mFocus       = 1;
		menu->mAtc.mCursorIndex = -1;
		menu->mDrs.mFocus       = 0;
	}
	PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
}

static inline void FadeEquipmentRingIn(XObjChrMenu* menu, u32 frames)
{
	f32 fade    = menu->mFade + (1.0f / (0.18f * gRefreshRate)) * frames;
	menu->mFade = fade;
	if (fade >= 1.0f)
		menu->mFade = 1.0f;
}

static inline void FadeEquipmentRingOut(XObjChrMenu* menu, u32 frames)
{
	f32 fade    = menu->mFade - (1.0f / (0.25f * gRefreshRate)) * frames;
	menu->mFade = fade;
	if (fade <= 0.0f)
		menu->mFade = 0.0f;
}

// the pair of rings, pointer-driven on the Wii: the GameCube's UpdatePlayerEquipmentRings.
// mSel 0 is the attachment row on top, 1 the costume row; with the remote's
// pointer the rings follow mPointedItem, on pad buttons the remote's dpad or the
// X/L/R equivalents switch rows
// NONMATCHING (~94%): register/scheduling/pool wall like the rest of the unit —
// float refs land on local @labels instead of retail's named sdata2 globals
// (they resolve once the unit owns its .sdata2), the 0x4330 u32→f64 magic and
// the sound-call sites reschedule, and MWCC caches the second
// gGame.mEvt.mData.mScript check in a bool where retail reloads it. After each row switch
// retail also carries a few unreachable focus/cursor stores (0x800EE14C, 0x800EE1B4) that no
// branch reaches; a pointed item on the other row ends the block without a switch or sound
/**
 * @note Address: 0x800EDF8C
 * @note Size: 0x924
 */
extern "C" void UpdatePlayerEquipmentRings(XObjChrMenu* menu, u32 frames)
{
	BOOL active = gGame.mEvt.mData.mScript == NULL;
	if (CanSelectEquipmentRing(menu)) {
		if (gGame.mMenuControls & 1) {
			menu->mAtc.UpdatePointedItem(active);
			menu->mDrs.UpdatePointedItem(active);
		}
		if (active) {
			if (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid) {
				if (menu->mAtc.mMenuType == 3) {
					if ((gGame.mMenuControls & 2) && !(gWiiPointer.mButton & 0x0000100000000001)
					    && !(gWiiPointer.mButton & 0x0000200000000002)) {
						if (gWiiPointer.Check(4) && (menu->mSel == 1 || menu->mSel < 0)) {
							if (menu->mDrs.mPointedItem < 0) {
								menu->mSel              = 0;
								menu->mAtc.mFocus       = 0;
								menu->mDrs.mFocus       = 1;
								menu->mDrs.mCursorIndex = -1;
								PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
							}
						} else if (gWiiPointer.Check(8) && (menu->mSel == 0 || menu->mSel < 0)) {
							if (menu->mAtc.mPointedItem < 0) {
								menu->mSel              = 1;
								menu->mAtc.mFocus       = 1;
								menu->mAtc.mCursorIndex = -1;
								menu->mDrs.mFocus       = 0;
								PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
							}
						}
					}
				} else if (gWiiPointer.mTrigger & 0x6400000000000200) {
					menu->mSel = (menu->mSel + 1) % 2;
					FocusEquipmentRing(menu);
				}
			}
		}
	}
	if (RingReady(menu->mAtc) && RingReady(menu->mDrs)) {
		menu->mAtc.mHeadTurn = NAN;
		menu->mDrs.mHeadTurn = NAN;
	} else {
		menu->mAtc.mHeadTurn = 0.0f;
		menu->mDrs.mHeadTurn = 0.0f;
	}
	s32 result;
	if (menu->mSel == 0) {
		result = HandleEquipmentRingInput(&menu->mAtc, active);
		if (result == 0 && HandleEquipmentRingInput(&menu->mDrs, active) != 0) {
			menu->mSel = (menu->mSel + 1) % 2;
			FocusEquipmentRing(menu);
		}
		if (!active) {
			menu->mDrs.mLastPointedItem       = menu->mAtc.mLastPointedItem;
			menu->mDrs.mKeepPointedItemOnOpen = 0;
		}
	} else {
		result = HandleEquipmentRingInput(&menu->mDrs, active);
		if (result == 0 && HandleEquipmentRingInput(&menu->mAtc, active) != 0) {
			menu->mSel = (menu->mSel + 1) % 2;
			FocusEquipmentRing(menu);
		}
		if (!active) {
			menu->mAtc.mLastPointedItem       = menu->mDrs.mLastPointedItem;
			menu->mAtc.mKeepPointedItemOnOpen = 0;
		}
	}
	if (menu->mAtc.mOpen > 0) {
		// he turns to face the camera, except in first person
		if (gGame.GetStatusFlags(0x100) == 0) {
			gGame.GetPlayer()->SetRotateY(PI + gGame.mMotionControlCamera.mView.mAngle.y, DEG2RAD * 8.0f);
		}
		if (gGame.mEvt.mData.mScript == NULL && gWiiPointer.Check(2)) {
			result = -1;
		}
	}
	if (result != 0) {
		ClosePlayerEquipmentRings(menu, result);
	}
	UpdateEquipmentRingAnimation(&menu->mAtc, frames);
	UpdateEquipmentRingAnimation(&menu->mDrs, frames);
	if (menu->IsOpen()) {
		if (menu->IsOpening()) {
			if (menu->mFade < 1.0f) {
				if (menu->mAtc.mMenuType == 3) {
					if (menu->mSel >= 0) {
						FadeEquipmentRingIn(menu, frames);
					} else {
						FadeEquipmentRingOut(menu, frames);
					}
				} else {
					FadeEquipmentRingIn(menu, frames);
				}
			}
		} else if (menu->IsClosing() && menu->mFade > 0.0f) {
			FadeEquipmentRingOut(menu, frames);
		}
	} else if (gGame.GetStatusFlags(0x400)) {
		// both rings are shut, clear the menu flag OpenMenus set
		gGame.ClearStatusFlags(0x400);
		gGame.GetPlayer()->mPad.Update(gWiiGameCubePads, 0);
	}
}

f32 gEquipmentHintOutlineX[4] = { -1, 1, 1, -1 };
f32 gEquipmentHintOutlineY[4] = { -1, -1, 1, 1 };

/**
 * @note Address: 0x800EE8B0
 * @note Size: 0x9FC
 */
// NONMATCHING (~86%): the Wii pointer/dpad dim-checks and hint stub calls are in,
// but retail keeps the pointed/override flags in separate registers where MWCC
// short-circuits through branches, it saves f21/f22 for the hint loop's x/y and
// alpha, float refs land on local @labels instead of the unit's named sdata2
// globals, and the 0x4330 u32->f64 magic plus the icon/tex arg setup reschedule
extern "C" void DrawPlayerEquipmentRings(XObjChrMenu* menu)
{
	if (menu->mSel == 0) {
		DrawEquipmentRing(&menu->mDrs);
		DrawEquipmentRing(&menu->mAtc);
	} else {
		DrawEquipmentRing(&menu->mAtc);
		DrawEquipmentRing(&menu->mDrs);
	}
	if (menu->mAtc.mMenuType == 3) {
		if (menu->mSel == 0) {
			menu->mTargetY = menu->mAtc.mCursorY;
			menu->mTargetX = menu->mAtc.mCursorX;
		} else if (menu->mSel == 1) {
			menu->mTargetY = menu->mDrs.mCursorY;
			menu->mTargetX = menu->mDrs.mCursorX;
		} else {
			menu->mTargetY = 0.0f;
			menu->mTargetX = 0.0f;
		}
		if (menu->mFade < 1.0f) {
			menu->mCursorY = menu->mTargetY;
			menu->mCursorX = menu->mTargetX;
		} else {
			menu->mCursorY += 0.7f * (menu->mTargetY - menu->mCursorY);
			menu->mCursorX += 0.7f * (menu->mTargetX - menu->mCursorX);
		}
		f32 radius = 40.0f * menu->mFade;
		f32 y      = menu->mCursorY;
		f32 x      = menu->mCursorX;
		f32 left   = 8.0f + (x - radius);
		f32 top    = 8.0f + (y - radius);
		f32 right  = (x + radius) - 8.0f;
		f32 bottom = (y + radius) - 8.0f;
		Vec quad[] = { { left, top, 0.0f }, { right, top, 0.0f }, { right, bottom, 0.0f }, { left, bottom, 0.0f }, { left, top, 0.0f } };

		// the cursor is drawn solid while a ring is pointed at or the remote's
		// dpad took over, half-transparent while nothing is under the pointer
		BOOL show = TRUE;
		if (gGame.mEvt.mData.mScript == NULL) {
			if (menu->mSel == 0) {
				if (menu->mAtc.mPointedItem < 0
				    && !((gGame.mMenuControls & 2) && (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid))) {
					show = FALSE;
				}
			} else {
				show = FALSE;
			}
			if (!show) {
				show = menu->mSel == 1
				    && !(menu->mDrs.mPointedItem < 0
				         && !((gGame.mMenuControls & 2) && (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid)));
			}
		}
		f32 alpha = show ? menu->mFade : 0.5f * menu->mFade;

		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_NEQUAL, GX_FALSE);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		(&gGame.mTpl)->mTex.Init(&gGame.mTpl.mPalette->descriptorArray[110 - StatusTextureOffset], 0);
		CTexObj tex = gGame.mTpl.mTex;
		DrawGsTexturedQuad(&gGame.mGs, &tex, 0xFFFFFF00 | (u32)(255.0f * alpha), 0xFFFFFF00 | (u32)(255.0f * alpha),
		                   0xFFFFFF00 | (u32)(255.0f * alpha), 0xFFFFFF00 | (u32)(255.0f * alpha), x, y, 0.0f, 2.0f * radius, 2.0f * radius,
		                   0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
		if (gGame.mMenuControls & 2) {
			if ((RingReady(menu->mAtc) || RingReady(menu->mDrs)) && (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid)) {
				fn_8002BBD8(&gGame.mGs, x + 0.6f * radius, y + 0.4f * radius);
			} else {
				fn_8002BBDC(&gGame.mGs);
			}
		}
		return;
	}

	struct Hint {
		s32 icon;
		const char* label;
	};
	Hint hints[]    = { { 8, NULL }, { 5, NULL }, { 5, NULL } };
	hints[0].label  = GetLocalizedInterfaceText(10);
	hints[1].label  = GetLocalizedInterfaceText(11);
	hints[2].label  = GetLocalizedInterfaceText(12);
	s32 order[2][2] = { { 0, 1 }, { 0, 2 } };

	f32 textSize = 20.0f;
	f32 iconSize = 1.2f * textSize;
	f32 edge     = gScreenSafeRight - 0x10 + 100.0f * (1.0f - std::sinf(HALF_PI * menu->mFade));
	for (s32 row = 0; row < 2; ++row) {
		s32 which = order[menu->mSel][row];
		f32 x     = edge - MeasureGameTextWidth(&gGame.mGs, hints[row].label, TRUE, textSize);
		f32 y     = gScreenSafeTop + 0x10 + iconSize * row;
		for (s32 j = 0; j < 4; ++j) {
			u32 alpha = (u32)(255.0f * menu->mFade);
			DrawGsProportionalTextFormat(&gGame.mGs, alpha, hints[which].label, x + gEquipmentHintOutlineX[j],
			                             y + gEquipmentHintOutlineY[j], 0.0f, textSize, textSize);
		}
		u32 color = 0xFFFFFF00 | (u32)(255.0f * menu->mFade);
		DrawGsProportionalTextFormat(&gGame.mGs, color, hints[which].label, x, y, 0.0f, textSize, textSize);
		(&gGame.mTpl)->mTex.Init(&gGame.mTpl.mPalette->descriptorArray[119 - StatusTextureOffset], 0);
		CTexObj tex = gGame.mTpl.mTex;
		PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f / 17.0f, 1.0f, 1.0f);
		tex.mTexMtx.mDirty = TRUE;
		PSMTXTrans(tex.mTexMtx.mMtx[0], (1.0f / 17.0f) * hints[which].icon, 0.0f, 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		color = 0xFFFFFF00 | (u32)(255.0f * menu->mFade);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x - 0.5f * iconSize, y + 0.5f * textSize, 0.0f, iconSize, iconSize,
		                   0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
}

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
	XObjScnCamInitRec* aim = &chr->mHeadAimRecord;
	aim->mSnap[0] = aim->mSnap[1] = 2;
	aim->m_0C                     = NULL;
	aim->mTarget.x                = NAN;
	aim->m_20                     = NULL;
	aim->m_24                     = NAN;
	aim->m_28                     = NAN;
	aim->m_2C                     = 0.0f;
	aim->m_30                     = 0.0f;
	aim->m_4C.x                   = 0.0f;
	aim->m_4C.y                   = 0.0f;
	aim->m_4C.z                   = 0.0f;
}

// the yes/no menu's input on the Wii: like the GameCube's HandleOXMenuInput the
// selection moves left and right and the confirm action picks it, but the remote's
// pointer can also hover a mark. the marks orbit a base point that is the chr's
// projected neck joint, or a fixed bottom-center spot while the 0x100 status is up;
// the stick moves the selection only where gGame.mMenuControls gives it to the pad
// NONMATCHING (~83%): register/scheduling/pool wall like the rest of the unit —
// the projected-point locals and the marker math reschedule, float refs land on
// local @labels instead of the unit's named sdata2 globals, and retail's arg for
// ProjectOffsetToScreen is a second stack vec; semantics decoded and verified
/**
 * @note Address: 0x800EF58C
 * @note Size: 0x804
 */
extern "C" void HandleOXMenuInput(WiiOXMenu* menu, CPad* pad)
{
	if (menu->mStarted > 0) {
		if (!gGame.GetStatusFlags(0x100)) {
			menu->mChr->SetRotateY(PI + gGame.mMotionControlCamera.mView.mAngle.y, DEG2RAD * 8.0f);
		}
		s32 lastPointed = menu->_10;
		menu->_10       = -1;
		if (gWiiPointer.mPointerValid) {
			Vec pointer = gWiiPointer.mPointer;
			f32 sx, sy;
			if (gGame.GetStatusFlags(0x100)) {
				sx = gPointerScreenWidth / 2;
				sy = gScreenSafeBottom;
			} else {
				CVec joint = menu->mChr->GetJointWorldPos("neck");
				Vec pos    = { 0.0f, 0.0f, 0.0f };
				pos.y      = joint.y - menu->mChr->GetPos()->y;
				Vec arg    = pos;
				menu->mChr->ProjectOffsetToScreen(&arg, &sx, &sy, NULL, NULL);
			}
			for (s32 i = 0; i < 2; i++) {
				f32 mx = sx + 1.0f * (90.0f * (i - 0.5f));
				f32 my = sy - 70.0f * (f32)sin(130.0f * DEG2RAD) - 0.3f * 70.0f * 1.0f;
				if (mx < gScreenSafeLeft || mx > gScreenSafeRight || my < gScreenSafeTop || my > gScreenSafeBottom) {
					sx = gPointerScreenWidth / 2;
					sy = gScreenSafeBottom;
					break;
				}
			}
			f32 baseY;
			if (gGame.GetStatusFlags(0x100)) {
				baseY = sy - 0.8f * 70.0f;
			} else {
				baseY = sy - 70.0f * (f32)sin(130.0f * DEG2RAD * menu->mFade) - 0.3f * 70.0f * 1.0f;
			}
			for (s32 i = 0; i < 2; i++) {
				f32 bob  = 4.0f * (f32)sin(menu->mMark[i].mSpin);
				f32 half = (4.0f + 70.0f) * menu->mMark[i].mScale * (f32)sin(120.0f * DEG2RAD * menu->mFade);
				f32 mx;
				if (gGame.GetStatusFlags(0x100)) {
					mx = sx + 90.0f * (i - 0.5f);
				} else {
					mx = sx + 1.0f * (menu->mFade * (90.0f * (i - 0.5f)));
				}
				f32 my = baseY + bob;
				if (pointer.x >= mx - half && pointer.x <= mx + half && pointer.y >= my - half && pointer.y <= my + half) {
					menu->_10 = i;
					break;
				}
			}
		} else if ((gGame.mMenuControls & 2) && (gGame.mMenuControls & 1) && gWiiPointer.mPointerChanged) {
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		if (menu->_10 >= 0) {
			if (menu->mSelectedIndex != menu->_10 || lastPointed < 0) {
				menu->mSelectedIndex = menu->_10;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		} else if ((gGame.mMenuControls & 2) && (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid)) {
			if (gWiiPointer.Check(0x20)) {
				if (menu->mSelectedIndex + 1 < 2) {
					menu->mSelectedIndex = menu->mSelectedIndex + 1;
					PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
				}
			} else if (gWiiPointer.Check(0x10)) {
				if (menu->mSelectedIndex - 1 >= 0) {
					menu->mSelectedIndex = menu->mSelectedIndex - 1;
					PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
				}
			}
		}
		WiiPlayerState* chr    = (WiiPlayerState*)menu->mChr;
		XObjScnCamInitRec* aim = &chr->mHeadAimRecord;
		if (menu->_10 < 0 && (!(gGame.mMenuControls & 2) || ((gGame.mMenuControls & 1) && gWiiPointer.mPointerValid))) {
			aim->mSnap[0] = aim->mSnap[1] = 2;
			aim->m_0C                     = NULL;
			aim->mTarget.x                = NAN;
			aim->m_20                     = NULL;
			aim->m_24                     = NAN;
			aim->m_28                     = NAN;
			aim->m_2C                     = 0.0f;
			aim->m_30                     = 0.0f;
			aim->m_4C.x                   = 0.0f;
			aim->m_4C.y                   = 0.0f;
			aim->m_4C.z                   = 0.0f;
		} else {
			f32 angle     = menu->mSelectedIndex == 0 ? DEG2RAD * -20.0f : DEG2RAD * 20.0f;
			aim->mSnap[0] = aim->mSnap[1] = 1;
			aim->m_0C                     = NULL;
			aim->mTarget.x                = NAN;
			aim->m_20                     = NULL;
			aim->m_24                     = NAN;
			aim->m_28                     = angle;
		}
		if (!gGame.GetStatusFlags(0x20) && 1.0f == menu->mFade) {
			if (menu->_10 >= 0 || ((gGame.mMenuControls & 2) && (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid))) {
				if (gWiiPointer.Check(1)) {
					ConfirmOXMenuSelection(menu, 1);
				}
			}
		}
	}
}

// the yes/no menu's per-frame update, the GameCube's UpdateOXMenuTransition with
// two Wii deltas: the selected mark only grows while the menu is "armed" — the
// pointer hovers a mark (mSelectedIndex mirrors _10 then), or the stick drives it
// under the same menu-controls gate HandleOXMenuInput uses — and the anim id is a
// virtual GetAnimId per test where the GameCube read mAnimId once
/**
 * @note Address: 0x800EFD90
 * @note Size: 0x370
 */
extern "C" void UpdateOXMenuTransition(WiiOXMenu* menu, u32 frames)
{
	if (menu->mStarted > 0) {
		if (menu->mFade < 1.0f) {
			f32 fade    = menu->mFade + (1.0f / (0.3f * gRefreshRate)) * frames;
			menu->mFade = fade;
			if (fade >= 1.0f) {
				menu->mFade = 1.0f;
			}
		}
	} else if (menu->mStarted < 0) {
		if (menu->mFade > 0.0f) {
			f32 fade    = menu->mFade - (1.0f / (0.3f * gRefreshRate)) * frames;
			menu->mFade = fade;
			if (fade <= 0.0f) {
				menu->mFade = 0.0f;
			}
		}
		if (0.0f == menu->mFade) {
			if (menu->mChr->GetAnimId() != 0x78 && menu->mChr->GetAnimId() != 0xEF && menu->mChr->GetAnimId() != 0x13B) {
				menu->mStarted = 0;
				gGame.ClearStatusFlags(0x800);
				menu->mChr->SetRotateY(menu->mStartRot, DEG2RAD * 12.0f);
				gGame.GetPlayer()->mPad.Update(gWiiGameCubePads, 0);
			}
		}
	}
	if (menu->mFade > 0.0f) {
		for (s32 i = 0; i < 2; ++i) {
			menu->mMark[i].mSpin += DEG2RAD * 8.0f * frames;
			menu->mMark[i].mTilt += DEG2RAD * 3.0f * frames;
			if (i == menu->mSelectedIndex
			    && (menu->_10 >= 0 || ((gGame.mMenuControls & 2) && (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid)))) {
				menu->mMark[i].mScale += 0.05f * frames;
				if (menu->mMark[i].mScale > 0.8f) {
					menu->mMark[i].mScale = 0.8f;
				}
			} else {
				menu->mMark[i].mScale -= 0.05f * frames;
				if (menu->mMark[i].mScale < 0.5f) {
					menu->mMark[i].mScale = 0.5f;
				}
			}
			((XObj*)&menu->mMark[i].mObj)->UpdateObject(frames);
		}
	}
}

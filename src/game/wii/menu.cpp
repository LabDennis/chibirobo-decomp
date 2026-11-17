// The menus, one file from 0x8004F8F8 to 0x800870E8: its .sdata2 literals (0x805A2F70-0x805A3350) are read
// by functions over that whole range and by nothing else, it has one string pool (from 0x802C67C8) and one
// static initializer (0x8008502C), behind which only its weak destructors and this-adjusting thunks follow.
// The pool holds "menu_item.cpp", "menu_status.cpp" and "menu_save.cpp", so those are files this one
// includes; its own name is not in the binary, menu.cpp is ours.
// NONMATCHING: most menu state, drawing and layout functions still have no source. CMenu::Close and
// MenuMain::Close store the caller's r4 as the state (retail 0x80050D9C, 0x80050D18): unlike the GameCube's they
// take the state as a parameter, which their names in symbols.txt (no parameter) do not show yet; cgame.h declares
// Close(s32 state), MenuPC::State11 and MenuSave::State17 pass 5.
#include "game/wii/menuutil.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/title.h"
#include "game/globals.h"
#include "game/evt.h"
#include "game/cfont.h"
#include <math.h>
#include <stdlib.h>

extern "C" void LayoutMainMenuRows(MenuMain* menu);
extern "C" void ApproachHudValue(f32* value, f32* target, f32 step);
extern s32 gPointerScreenWidth, gPointerScreenHeight, gRefreshRate;
extern "C" void DrawMenuWindow(MenuWindow* window);
extern s32 gRingScreenWidth;

static MenuMain sMenuMain;
// what the PC shop sold and what it sells
static MenuPCItem sShopHistoryEntries[20];
static MenuPCItem sShopCatalogEntries[20];

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

void CMenu::Init()
{
	mWindow.mOpen      = FALSE;
	mWindow.mX         = gPointerScreenWidth / 2;
	mWindow.mY         = gPointerScreenHeight / 2;
	mWindow.mWidth     = 100.0f;
	mWindow.mHeight    = 100.0f;
	mWindow.mDrawAlpha = 0.0f;
	mCursor            = NULL;
	Reset();
}

// the window's texture, the GameCube's MenuWindowTexture
BOOL CMenu::LoadTexture()
{
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[139], 0);
	return TRUE;
}

void CMenu::Reset()
{
	mState        = 0;
	mAlpha        = 0.0f;
	mSelection    = 0;
	mSuspended    = FALSE;
	mSuspendFade  = FALSE;
	mFade         = 0.0f;
	mSuspendAlpha = 1.0f;
}

BOOL CMenu::Draw()
{
	if (!IsOpen())
		return FALSE;
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	DrawMenuWindow(&mWindow);
	return TRUE;
}

void MenuStageSelect::Init()
{
	CMenu::Init();
}

void MenuMain::Init()
{
	CMenu::Init();
}

void MenuMain::Reset()
{
	CMenu::Reset();
	mChannel = 0;
	for (int i = 0; i < 5; i++) {
		mExtraRows[i].mText   = NULL;
		mExtraRows[i].mAction = 0;
	}
	LayoutMainMenuRows(this);
}

void MenuSave::Reset()
{
	CMenu::Reset();
	mLastFile = -1;
}

void CMenu::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
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

// a quad in one color, the GameCube's DrawSolidQuad
static inline void DrawSolidQuad(CGs* gs, const Vec* quad, u32 color)
{
	DrawGsVertexQuad(gs, quad[0].x, quad[0].y, quad[0].z, color, quad[1].x, quad[1].y, quad[1].z, color, quad[2].x, quad[2].y, quad[2].z,
	                 color, quad[3].x, quad[3].y, quad[3].z, color);
}

// the bar beside the shop's list, the GameCube's
// NONMATCHING: retail keeps 172 and 4 in saved registers and needs two saved GPRs where ours takes five
extern "C" void DrawPCShopScrollBar(f32 x, f32 y, f32 start, f32 end, f32 alpha, f32 /*scale*/)
{
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	y               = 48.0f + (1.0f + y);
	x               = 10.0f + (27.0f + (x - 4.0f));
	f32 top         = 4.0f + y;
	f32 bottom      = 172.0f + top;
	f32 first       = 172.0f * start + top;
	f32 last        = 172.0f * end + top;
	Vec quads[4][4] = {};
	quads[0][0].x   = x;
	quads[0][0].y   = top;
	quads[0][1].x   = 4.0f + x;
	quads[0][1].y   = top;
	quads[0][2].x   = 4.0f + x;
	quads[0][2].y   = bottom;
	quads[0][3].x   = x;
	quads[0][3].y   = bottom;
	quads[1][0].x   = x;
	quads[1][0].y   = top;
	quads[1][1].x   = 4.0f + x;
	quads[1][1].y   = top;
	quads[1][2].x   = 4.0f + x;
	quads[1][2].y   = first;
	quads[1][3].x   = x;
	quads[1][3].y   = first;
	quads[2][0].x   = x;
	quads[2][0].y   = first;
	quads[2][1].x   = 4.0f + x;
	quads[2][1].y   = first;
	quads[2][2].x   = 4.0f + x;
	quads[2][2].y   = last;
	quads[2][3].x   = x;
	quads[2][3].y   = last;
	quads[3][0].x   = x;
	quads[3][0].y   = last;
	quads[3][1].x   = 4.0f + x;
	quads[3][1].y   = last;
	quads[3][2].x   = 4.0f + x;
	quads[3][2].y   = bottom;
	quads[3][3].x   = x;
	quads[3][3].y   = bottom;
	DrawGsLineStrip(&gGame.mGs, quads[0], (u32)(255.0f * alpha) | 0xC8C8C800, 5);
	gGame.mGs.mCurRenderState->SetLineWidth(6, GX_TO_ONE);
	DrawSolidQuad(&gGame.mGs, quads[1], (u32)(255.0f * alpha));
	DrawSolidQuad(&gGame.mGs, quads[2], (u32)(255.0f * alpha) | 0xFFB62800);
	DrawSolidQuad(&gGame.mGs, quads[3], (u32)(255.0f * alpha));
	DrawGsLineStrip(&gGame.mGs, quads[2], (u32)(255.0f * alpha) | 0xFFFFFF00, 5);
	gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
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

// fills a file menu record from a scanned save header, read into locals first as the GameCube's stg.cpp does.
// NONMATCHING: scheduling only, retail loads mPlayTime and starts the divisions ahead of the other fields
extern "C" void fn_8007A2E8(WiiMenuSaveSlot* slot, WiiSaveScanEntry* entry, BOOL used)
{
	WiiSaveSlotHeader* header = (WiiSaveSlotHeader*)entry->mData;
	f32 frames                = header->mPlayTime;
	s32 level                 = header->mLevel;
	f32 battery               = header->mBattery;
	f32 maxBattery            = header->mMaxBattery;
	s32 money                 = header->mMoney;
	s32 exp                   = header->mExp;
	s32 scrap                 = header->mScrap;
	s32 rank                  = header->mRank;
	s32 hours                 = frames / 216000.0f;
	s32 minutes               = (s32)(frames / 3600.0f) % 60;
	s32 stage                 = header->mStages[0];
	s32 timeMode              = header->mTimeMode;
	s32 active                = header->mActive;
	slot->mUsed               = used;
	slot->mLevel              = level;
	slot->mBattery            = battery;
	slot->mMaxBattery         = maxBattery;
	slot->mMoney              = money;
	slot->mExp                = exp;
	slot->mScrap              = scrap;
	slot->mRank               = rank;
	slot->mHours              = hours;
	slot->mMinutes            = minutes;
	slot->mStage              = stage;
	slot->mTimeMode           = timeMode;
	slot->mState              = 0;
	if (active > 0) {
		slot->mActive = TRUE;
	} else {
		slot->mActive = FALSE;
	}
}

extern "C" const char* GetTitleAndSaveMenuText(s32 index)
{
	return GetLocalizedInterfaceText(index + 95);
}

extern "C" void fn_80083350(MenuSave* menu);
static void SetMenuTitle(MenuSave* menu, const char* text, u32 color);
static void UpdateMenuTitle(MenuSave* menu, u32 frames);
extern f32 gTitleFileSlideSpeed[2];

// NONMATCHING: retail converts this for gPointerPanes.mMenu in r4 and its 1 in r0, ours the other way round
void MenuSave::Open()
{
	CMenu::Open();
	gGame.SetStatusFlags(2);
	mWindow.mWidth  = 0.0f;
	mWindow.mHeight = 0.0f;
	mWindow.mX      = 0.0f;
	mWindow.mY      = 0.0f;
	mPanelX         = -10.0f;
	mPanelY         = -200.0f;
	mTargetPanelX   = -10.0f;
	mTargetPanelY   = 0.0f;
	_170            = 0.0f;
	mBannerSlide    = 0.0f;
	_178            = 0.0f;
	mOffset         = 0.0f;
	mDrawAlpha      = 0.0f;
	mTargetAlpha    = 1.0f;
	mMode           = 0;
	mPrevMode       = mMode;
	mPhase          = 0;
	mTime           = 0.0f;
	mDelay          = 0;
	if (mLastFile == -1) {
		mFile = gGame.mInfoScrn.mSelectedSaveSlot;
	} else {
		mFile = mLastFile;
	}
	mStickFile   = mFile;
	mNewestFile  = mFile;
	mChoice      = 0;
	mStickChoice = 0;
	mResult      = 0;
	mMessage     = 0;
	mReady       = 0;
	mShowMessage = 0;
	mWrongDevice = 0;
	mSaved       = 0;
	mChannel     = 0;
	mTexScroll   = 0.0f;
	PlayWiiSoundEffect(&gGame.mSound, 0x1B8, 1.0f, 0.0f);
	const char* title = GetLocalizedInterfaceText(99);
	SetMenuTitle(this, title, (u32)(255.0f * mTargetAlpha) | 0xFFFFFF00);
	mResetOnClose         = 0;
	gPointerPanes.mMenu   = this;
	gPointerPanes.mActive = TRUE;
	for (int i = 0; i < 5; i++) {
		mPanes[i].mPointed = FALSE;
		mPanes[i].mPicked  = FALSE;
	}
	for (int i = 0; i < 3; i++) {
		mFileAlpha[i] = 0.0f;
	}
}

// the GameCube's PCEaseCall: a quarter of the way, at least 1 a frame. The float fabs is nw4r::math::FAbs's, as in
// fbeffect.cpp; its header needs the Wii MSL's <cmath>, which the game's include path does not have yet
static inline void PCEaseCall(f32& value, f32& target)
{
	f32 rate = __fabsf((target - value) * 0.25f);
	if (rate <= 1.0f) {
		rate = 1.0f;
	}
	ApproachHudValue(&value, &target, rate);
}

static void (MenuSave::*gSaveStepHandlers[])(u32)
    = { &MenuSave::State0,  &MenuSave::State1,  &MenuSave::State2,  &MenuSave::State3,  &MenuSave::State4,
	    &MenuSave::State5,  &MenuSave::State6,  &MenuSave::State7,  &MenuSave::State8,  &MenuSave::State9,
	    &MenuSave::State10, &MenuSave::State11, &MenuSave::State12, &MenuSave::State13, &MenuSave::State14,
	    &MenuSave::State15, &MenuSave::State16, &MenuSave::State17, &MenuSave::State18, &MenuSave::State19 };

// NONMATCHING: the two eases multiply with the operands the other way round (f2 * f1 in retail)
BOOL MenuSave::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		gPointerPanes.mCount = 0;
		PCEaseCall(mPanelY, mTargetPanelY);
		ApproachHudValue(&mDrawAlpha, &mTargetAlpha, 0.08f * frames);
		PCEaseCall(mBannerSlide, mOffset);
		mTexScroll -= 0.008f * frames;
		UpdateMenuTitle(this, frames);
		switch (mShowHelp) {
		case 0:
			break;
		case 1:
			mHelpAlpha = 0.0f;
			mShowHelp++;
			mHelpText = mNextHelpText;
			break;
		case 2:
			mHelpAlpha += 0.04f * frames;
			if (mHelpAlpha >= 1.0f) {
				mHelpAlpha = 1.0f;
				mShowHelp  = 0;
			}
			break;
		}
		if (mAlpha > 0.0f) {
			for (int i = 0; i < 20; i++) {
				mBg[i].mAngle0 += 0.08726646f * frames;
				mBg[i].mAngle1 += 0.05235988f * frames;
			}
		}
		for (int i = 0; i < 3; i++) {
			if (mFile == i) {
				if (mFileAlpha[i] < 1.0f) {
					if ((mFileAlpha[i] += 0.1f * gTitleFileSlideSpeed[0] * frames) > 1.0f) {
						mFileAlpha[i] = 1.0f;
					}
				}
			} else if (mFileAlpha[i] > 0.0f) {
				if ((mFileAlpha[i] -= 0.1f * gTitleFileSlideSpeed[1] * frames) < 0.0f) {
					mFileAlpha[i] = 0.0f;
				}
			}
		}
		// the GameCube probed the card here; the NAND is always there
		if (mMode != 1 && mMode != 5 && mMode != 9) {
			mWrongDevice = FALSE;
			mReady       = TRUE;
			if (!mReady) {
				mShowMessage = FALSE;
			}
		}
		if (gGame.mSaveManager.mJobStarted) {
			return IsOpen();
		}
		(this->*gSaveStepHandlers[mMode])(frames);
	}
	return IsOpen();
}

void MenuSave::State0(u32)
{
	MenuSave* menu = this;
	for (int i = 0; i < 3; i++) {
		ResetSaveSlotRecord(&menu->mSlots[i]);
	}
	StartWiiSaveSlotScan(&gGame.mSaveManager, 10);
	menu->SetMode(1);
}

// the file records cleared, as title.cpp's OnCardRemoved; State2, State3, State18 and State19 differ from retail only
// in the panic messages' pool offsets (see State6)
static inline void OnCardRemoved(MenuSave* menu)
{
	OSPanic("menu_save.cpp", 4221, "ここにくるのはおかしい");
	menu->mResult = -3;
	menu->SetMode(6);
	menu->mOffset = -32.0f;
	for (int i = 0; i < 3; i++) {
		ResetSaveSlotRecord(&menu->mSlots[i]);
	}
}

// picks the file to save to: a used one asks first (State3), an empty one is written, a full NAND says so (State11).
// NONMATCHING: the panic message's pool offset (see State6) and which register keeps the pad flags for B
void MenuSave::State2(u32)
{
	bool addPanes = true;
	bool stick    = false;
	if (mFile != -1) {
		mStickFile = mFile;
	}
	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	if (IsStickUsedOverPointer()) {
		stick = true;
	}
	if (!stick) {
		if (IsStickLeft()) {
			if (--mFile < 0) {
				mFile = 2;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (IsStickRight()) {
			if (++mFile > 2) {
				mFile = 0;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
	}
	if (IsConfirmPressed() && mFile != -1) {
		addPanes           = false;
		mPanes[0].mPointed = false;
		mPanes[0].mPicked  = false;
		mPanes[1].mPointed = false;
		mPanes[1].mPicked  = false;
		mPanes[2].mPointed = false;
		mPanes[2].mPicked  = false;
		if (mSlots[mFile].mUsed == 1) {
			SetMode(3);
			fn_80083350(this);
		} else if (mSlots[mFile]._44 == 1) {
			StartWiiSaveSlotWrite(&gGame.mSaveManager, mFile, FALSE);
			SetMode(5);
			mDelay = 1.5f * gRefreshRate;
		} else {
			if (mSlots[mFile]._48 == 1) {
				mResult = -9;
			} else if (mSlots[mFile]._4C == 1) {
				mResult = -11;
			}
			SetMode(11);
			mOffset = -32.0f;
		}
		PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
	} else if (IsCancelPressed()) {
		SetMode(16);
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		addPanes = false;
	}
	if (addPanes) {
		for (int i = 0; i < 3; i++) {
			AddPointerPane(&gPointerPanes, &mPanes[i]);
		}
	}
}

// the yes or no question before the game is written to the file picked
void MenuSave::State3(u32)
{
	bool addPanes = true;
	bool stick    = false;
	if (mChoice != -1) {
		mStickChoice = mChoice;
	}
	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	if (IsStickUsedOverPointer()) {
		stick = true;
	}
	if (!stick) {
		if (IsStickLeft()) {
			if (mChoice > 0) {
				mChoice--;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		} else if (IsStickRight()) {
			if (mChoice < 1) {
				mChoice++;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		}
	}
	if (IsConfirmPressed() && mChoice != -1) {
		addPanes           = false;
		mPanes[3].mPointed = false;
		mPanes[3].mPicked  = false;
		mPanes[4].mPointed = false;
		mPanes[4].mPicked  = false;
		switch (mChoice) {
		case 0:
			StartWiiSaveSlotWrite(&gGame.mSaveManager, mFile, FALSE);
			SetMode(5);
			mOffset = -32.0f;
			mDelay  = 1.5f * gRefreshRate;
			break;
		case 1:
			SetMode(2);
			fn_80083350(this);
			break;
		}
		PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
	}
	if (addPanes) {
		AddPointerPane(&gPointerPanes, &mPanes[3]);
		AddPointerPane(&gPointerPanes, &mPanes[4]);
	}
}

void MenuSave::State4(u32 frames)
{
	MenuSave* menu = this;
	switch (menu->mPhase) {
	case 0:
		menu->mDelay -= frames;
		if (menu->mDelay < 0) {
			menu->mDelay = 0;
			menu->mPhase++;
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
		break;
	case 1:
		if (gWiiPointer.Check(PadActConfirm) || (menu->mTime -= frames) < 0.0f) {
			menu->mTime = 0.0f;
			menu->SetMode(16);
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
		break;
	}
}

// the states the Wii's save flow no longer reaches. NONMATCHING (State6 to State15's panics): the messages' offsets
// in the string pool, which holds the rest of the file's strings ahead of them in retail
void MenuSave::State6(u32)
{
	OSPanic("menu_save.cpp", 1310, "ここにくるのはおかしい");
}

void MenuSave::State7(u32)
{
}

void MenuSave::State8(u32)
{
	OSPanic("menu_save.cpp", 1369, "ここにくるのはおかしい");
}

void MenuSave::State9(u32)
{
	OSPanic("menu_save.cpp", 1489, "ここにくるのはおかしい");
}

void MenuSave::State10(u32)
{
	OSPanic("menu_save.cpp", 1589, "ここにくるのはおかしい");
}

void MenuSave::State11(u32)
{
	MenuSave* menu = this;
	if (gWiiPointer.Check(PadActConfirm)) {
		if (menu->mResult == -9 || menu->mResult == -11) {
			menu->SetMode(18);
			menu->mOffset = -32.0f;
			fn_80083350(menu);
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		} else {
			menu->SetMode(16);
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
	}
}

void MenuSave::State12(u32)
{
	OSPanic("menu_save.cpp", 1741, "ガイドライン1.09-->1.10で削除したはずの遷移です");
}

// the yes or no question after a failed write: 0 leaves, 1 says why or scans the NAND again
void MenuSave::State19(u32)
{
	bool addPanes = true;
	bool stick    = false;
	if (mChoice != -1) {
		mStickChoice = mChoice;
	}
	if (IsStickUsedOverPointer()) {
		stick = true;
	}
	if (!stick) {
		if (IsStickLeft()) {
			if (mChoice > 0) {
				mChoice--;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		} else if (IsStickRight()) {
			if (mChoice < 1) {
				mChoice++;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		}
	}
	if (IsConfirmPressed() && mChoice != -1) {
		addPanes           = false;
		mPanes[3].mPointed = false;
		mPanes[3].mPicked  = false;
		mPanes[4].mPointed = false;
		mPanes[4].mPicked  = false;
		if (mChoice == 0) {
			SetMode(16);
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			return;
		} else if (mChoice == 1) {
			if (mShowMessage == TRUE) {
				SetMode(13);
				mOffset = 0.0f;
			} else if (mResult == 0) {
				OSPanic("menu_save.cpp", 1926, "エラーからこないことはない");
				SetMode(2);
				mChoice = 0;
			} else if (mResult == -11 || mResult == -9) {
				for (int i = 0; i < 3; i++) {
					ResetSaveSlotRecord(&mSlots[i]);
				}
				StartWiiSaveSlotScan(&gGame.mSaveManager, 10);
				SetMode(1);
				mOffset = 0.0f;
			} else {
				for (int i = 0; i < 3; i++) {
					ResetSaveSlotRecord(&mSlots[i]);
				}
				StartWiiSaveSlotScan(&gGame.mSaveManager, 10);
				SetMode(1);
				mOffset = 0.0f;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			return;
		}
	}
	if (addPanes) {
		AddPointerPane(&gPointerPanes, &mPanes[3]);
		AddPointerPane(&gPointerPanes, &mPanes[4]);
	}
}

// the yes or no question after a full NAND: 0 leaves and resets the game once the menu is closed, 1 goes on to State19
void MenuSave::State18(u32)
{
	bool addPanes = true;
	bool stick    = false;
	if (mChoice != -1) {
		mStickChoice = mChoice;
	}
	if (IsStickUsedOverPointer()) {
		stick = true;
	}
	if (!stick) {
		if (IsStickLeft()) {
			if (mChoice > 0) {
				mChoice--;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		} else if (IsStickRight()) {
			if (mChoice < 1) {
				mChoice++;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		}
	}
	if (IsConfirmPressed() && mChoice != -1) {
		addPanes           = false;
		mPanes[3].mPointed = false;
		mPanes[3].mPicked  = false;
		mPanes[4].mPointed = false;
		mPanes[4].mPicked  = false;
		if (mChoice == 0) {
			SetMode(16);
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			mResetOnClose = TRUE;
		} else if (mChoice == 1) {
			if (mShowMessage == TRUE) {
				SetMode(19);
				mOffset = -32.0f;
				fn_80083350(this);
			} else if (mResult == 0) {
				OSPanic("menu_save.cpp", 2108, "エラーからこないことはない");
				SetMode(2);
				mChoice = 0;
			} else {
				SetMode(19);
				mOffset = -32.0f;
				fn_80083350(this);
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
	}
	if (addPanes) {
		AddPointerPane(&gPointerPanes, &mPanes[3]);
		AddPointerPane(&gPointerPanes, &mPanes[4]);
	}
}

void MenuSave::State13(u32)
{
	MenuSave* menu = this;
	CPad pad(0); // unused, retail builds one here
	if (gWiiPointer.Check(PadActConfirm)) {
		menu->SetMode(16);
		PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
	}
}

void MenuSave::State14(u32)
{
	OSPanic("menu_save.cpp", 2213, "書き込み中にメモカが抜かれる事はない");
}

void MenuSave::State15(u32 frames)
{
	State14(frames);
}

void MenuSave::State16(u32)
{
	MenuSave* menu = this;
	for (int i = 0; i < 20; i++) {
		menu->mTitle.mChars[i].mHold = 0.0f;
	}
	menu->mTargetAlpha  = 0.0f;
	menu->mTargetPanelY = -200.0f;
	menu->mBannerSlide  = 0.0f;
	menu->SetMode(17); // State17 closes the menu once it has faded out
}

void MenuSave::State17(u32)
{
	MenuSave* menu = this;
	if (menu->mDrawAlpha <= 0.0f) {
		StopWiiSfx(&gGame.mSound, 0x1B8, 0.0f);
		PlayWiiSoundEffect(&gGame.mSound, 0x1F2, 1.0f, 0.0f);
		gGame.ClearStatusFlags(2);
		menu->Close(5);
		sMenuMain.Resume();
		if (menu->mResetOnClose) {
			gGame.mResetRequested = true;
			gGame.mResetType      = 5;
		}
		gPointerPanes.mMenu = NULL;
	}
}

// the status and PC menus' empty states
void MenuStatus::State7(u32)
{
}

void MenuStatus::State6(u32)
{
}

// the PC's dialogue line back to full-width spaces; on the Wii the characters start off the right edge of a wide screen.
// NONMATCHING: retail picks the first width as 854 with the 640 copied in from a second register (0x8006D2FC), the one
// difference left; an if or either ternary gives the plain two-load form
static void ResetPCCharacters(MenuPC* menu)
{
	s32 width            = gGame.mAspectRatio == 0 ? 0x280 : 0x356;
	f32 offset           = (width - 0x280) / 2;
	menu->mCharIndex     = 0;
	menu->mNumberOffset  = 0;
	menu->mInsertsNumber = 0;
	char space[]         = "\x81\x40"; // a full-width space
	for (int i = 0; i < 40; i++)
		*(u16*)&menu->mChars[i * 2] = *(u16*)space;
	menu->mTerminator = 0;
	for (int i = 0; i < 40; i++) {
		menu->mCharX[i]             = 640.0f + offset;
		menu->mCharFlags[i]         = 0;
		*(u16*)menu->mCharCopies[i] = *(u16*)space;
		menu->mCharCopies[i][2]     = 0;
	}
	menu->mGlyphFrames    = 0;
	menu->mNextGlyph      = 0;
	menu->mKeeperAnimStep = 0;
	menu->mKeeperAnimId   = 0;
	menu->mPlayKeeperAnim = 0;
	width                 = gGame.mAspectRatio == 0 ? 0x280 : 0x356;
	gRingScreenWidth      = width;
	gPointerScreenWidth   = width;
}

static inline void ResetPCText(MenuPC* menu, s32 step)
{
	menu->mPrevMode  = menu->mMode;
	menu->mMode      = step;
	menu->mPhase     = 0;
	menu->mTextPhase = 0;
	menu->mTextX     = 210.0f;
	menu->mTextY     = 129.0f;
	menu->mTargetX   = menu->mTextX;
	menu->mTargetY   = menu->mTextY;
	ResetPCCharacters(menu);
}

void MenuPC::State4(u32)
{
	MenuPC* menu = this;
	CPad pad(0); // unused, retail builds one here
	if (gWiiPointer.Check(PadActCancel)) {
		ResetPCText(menu, 2);
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
	}
}

void MenuPC::State5(u32)
{
	MenuPC* menu = this;
	CPad pad(0); // unused, retail builds one here
	if (gWiiPointer.Check(PadActCancel)) {
		gGame.mSubwin.mEnabled = 0;
		ResetPCText(menu, 2);
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
	}
}

void MenuPC::State8(u32)
{
}

void MenuPC::State6(u32)
{
}

static inline s32 GetMoney()
{
	return gGame.GetPlayer()->mParams.mMoney;
}

static inline void DepositPCAmount(MenuPC* menu)
{
	s32 deposit = menu->mAmount;
	s32 money   = GetMoney();
	money -= deposit;
	menu->mDepositBalance += deposit;
	WiiPlayerParams* params = &gGame.GetPlayer()->mParams;
	params->SetMoney(money);
	menu->mMoneyDisplay.target = money;
}

// paying money in, the GameCube's digit entry worked with the stick
void MenuPC::State9(u32)
{
	MenuPC* menu = this;
	CPad pad(0); // unused, retail builds one here
	s32 digits[7] = { 0, 0, 0, 0, 0, 0, 0 };
	switch (menu->mPhase) {
	case 0:
		if (gWiiPointer.Check(PadActStickRight)) {
			menu->mMoneyDigit--;
			if (menu->mMoneyDigit < 0)
				menu->mMoneyDigit = 4;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickLeft)) {
			menu->mMoneyDigit++;
			if (menu->mMoneyDigit > 4)
				menu->mMoneyDigit = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickDown)) {
			s32 value = 0;
			for (s32 i = 0; i < 7; i++) {
				if (i == 0)
					digits[i] = menu->mAmount % 10;
				else {
					s32 power = 1;
					for (s32 j = 0; j < i; j++)
						power *= 10;
					digits[i] = (menu->mAmount / power) % 10;
				}
			}
			digits[menu->mMoneyDigit]--;
			if (digits[menu->mMoneyDigit] < 0)
				digits[menu->mMoneyDigit] = 9;
			for (s32 i = 0; i < 7; i++) {
				s32 power = 1;
				if (i > 0) {
					for (s32 j = 0; j < i; j++)
						power *= 10;
				}
				value += power * digits[i];
			}
			menu->mAmount = value;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickUp)) {
			s32 value = 0;
			for (s32 i = 0; i < 7; i++) {
				if (i == 0)
					digits[i] = menu->mAmount % 10;
				else {
					s32 power = 1;
					for (s32 j = 0; j < i; j++)
						power *= 10;
					digits[i] = (menu->mAmount / power) % 10;
				}
			}
			digits[menu->mMoneyDigit]++;
			if (digits[menu->mMoneyDigit] > 9)
				digits[menu->mMoneyDigit] = 0;
			for (s32 i = 0; i < 7; i++) {
				s32 power = 1;
				if (i > 0) {
					for (s32 j = 0; j < i; j++)
						power *= 10;
				}
				value += power * digits[i];
			}
			menu->mAmount = value;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActCancel)) {
			ResetPCText(menu, 2);
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActConfirm)) {
			if (menu->mAmount > GetMoney()) {
				menu->mAmount = GetMoney();
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				return;
			}
			if (menu->mAmount == 0) {
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				return;
			}
			menu->mPhase = 1;
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
		break;
	case 1:
		if (gWiiPointer.Check(PadActStickLeft)) {
			if (menu->mYesNo > 0) {
				menu->mYesNo--;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		} else if (gWiiPointer.Check(PadActStickRight)) {
			if (menu->mYesNo < 1) {
				menu->mYesNo++;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		}
		if (gWiiPointer.Check(PadActConfirm)) {
			switch (menu->mYesNo) {
			case 0: {
				DepositPCAmount(menu);
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				menu->mPhase = 2;
				break;
			}
			case 1:
				menu->mPhase = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
				break;
			}
		} else if (gWiiPointer.Check(PadActCancel)) {
			menu->mPhase = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		}
		break;
	case 2:
		if (gWiiPointer.Check(PadActConfirm) || gWiiPointer.Check(PadActCancel)) {
			menu->mPhase = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		}
		break;
	}
}

static inline s32 GetScrap()
{
	return gGame.GetPlayer()->mParams.mScrap;
}

static inline void RecyclePCAmount(MenuPC* menu)
{
	s32 scrap = GetScrap();
	scrap -= menu->mAmount;
	gGame.GetPlayer()->mParams.SetScrap(scrap);
	s32 money = GetMoney();
	money += menu->mAmount * menu->mScrapRate;
	gGame.GetPlayer()->mParams.SetMoney(money);
	menu->mScrapDisplay.target = scrap;
	menu->mMoneyDisplay.target = money;
}

// recycling scrap into money at mScrapRate, the same digit entry
void MenuPC::State10(u32)
{
	MenuPC* menu = this;
	CPad pad(0); // unused, retail builds one here
	s32 digits[7] = { 0, 0, 0, 0, 0, 0, 0 };
	switch (menu->mPhase) {
	case 0:
		menu->mYesNo = -1;
		if (gWiiPointer.Check(PadActStickRight)) {
			menu->mScrapDigit--;
			if (menu->mScrapDigit < 0)
				menu->mScrapDigit = 4;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickLeft)) {
			menu->mScrapDigit++;
			if (menu->mScrapDigit > 4)
				menu->mScrapDigit = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickDown)) {
			s32 value = 0;
			for (s32 i = 0; i < 5; i++) {
				if (i == 0)
					digits[i] = menu->mAmount % 10;
				else {
					s32 power = 1;
					for (s32 j = 0; j < i; j++)
						power *= 10;
					digits[i] = (menu->mAmount / power) % 10;
				}
			}
			digits[menu->mScrapDigit]--;
			if (digits[menu->mScrapDigit] < 0)
				digits[menu->mScrapDigit] = 9;
			for (s32 i = 0; i < 5; i++) {
				s32 power = 1;
				if (i > 0) {
					for (s32 j = 0; j < i; j++)
						power *= 10;
				}
				value += power * digits[i];
			}
			menu->mAmount = value;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickUp)) {
			s32 value = 0;
			for (s32 i = 0; i < 5; i++) {
				if (i == 0)
					digits[i] = menu->mAmount % 10;
				else {
					s32 power = 1;
					for (s32 j = 0; j < i; j++)
						power *= 10;
					digits[i] = (menu->mAmount / power) % 10;
				}
			}
			digits[menu->mScrapDigit]++;
			if (digits[menu->mScrapDigit] > 9)
				digits[menu->mScrapDigit] = 0;
			for (s32 i = 0; i < 5; i++) {
				s32 power = 1;
				if (i > 0) {
					for (s32 j = 0; j < i; j++)
						power *= 10;
				}
				value += power * digits[i];
			}
			menu->mAmount = value;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActCancel)) {
			ResetPCText(menu, 2);
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActConfirm)) {
			if (menu->mAmount > GetScrap()) {
				menu->mAmount = GetScrap();
				PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
				return;
			}
			if (menu->mAmount == 0) {
				PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
				return;
			}
			menu->mPhase = 1;
			menu->mYesNo = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
		break;
	case 1:
		if (gWiiPointer.Check(PadActStickUp)) {
			if (menu->mYesNo > 0) {
				menu->mYesNo--;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		} else if (gWiiPointer.Check(PadActStickDown)) {
			if (menu->mYesNo < 1) {
				menu->mYesNo++;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		}
		if (gWiiPointer.Check(PadActConfirm)) {
			switch (menu->mYesNo) {
			case 0: {
				RecyclePCAmount(menu);
				menu->mYesNo = 2;
				PlayWiiSoundEffect(&gGame.mSound, 0x35B, 1.0f, 0.0f);
				menu->mPhase = 2;
				break;
			}
			case 1:
				menu->mPhase = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
				break;
			}
		} else if (gWiiPointer.Check(PadActCancel)) {
			menu->mPhase = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		}
		break;
	case 2:
		if (gWiiPointer.Check(PadActConfirm) || gWiiPointer.Check(PadActCancel)) {
			menu->mPhase = 0;
			ResetPCText(menu, 2);
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		}
		break;
	}
}

void MenuPC::State11(u32)
{
	MenuPC* menu = this;
	if (menu->mFrameHeight <= 2.0f) {
		gGame.mEvt.mSelectResult = menu->mExitState;
		menu->Close(5);
		sMenuMain.Resume();
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		gGame.ClearStatusFlags(2);
		StopWiiSfx(&gGame.mSound, 0x35A, 0.0f);
	}
}

// the GameCube's inline MenuTitle::Set: the letters come in one after the other; the lead byte test is spelled as
// title.cpp has it. NONMATCHING: retail keeps the lead flag in r3 and both range results in r0, and r28/r29 swap
static void SetMenuTitle(MenuSave* menu, const char* text, u32 color)
{
	const char* p        = text;
	menu->mTitle.mActive = TRUE;
	menu->mTitle.mCount  = 0;
	if (text) {
		menu->mTitle.mColor = color;
		int i;
		for (i = 0; i < 20 && *p; i++, p++) {
			menu->mTitle.mChars[i].mText   = p;
			menu->mTitle.mChars[i].mFade   = -0.2f * i;
			menu->mTitle.mChars[i].mHold   = gRefreshRate;
			menu->mTitle.mChars[i].mAngle0 = 0.017453292f * (rand() % 360);
			menu->mTitle.mChars[i].mAngle1 = 0.017453292f * (rand() % 360);
			menu->mTitle.mCount++;
			if (gGame.mLanguage == 0) {
				u8 c    = *p;
				u8 lead = TRUE;
				u8 a    = (c >= 0x81 && c <= 0x9F);
				if (!a) {
					u8 b = (c >= 0xE0 && c <= 0xFC);
					if (!b) {
						lead = FALSE;
					}
				}
				if (lead) {
					p++;
				}
			}
		}
		for (i = menu->mTitle.mCount; i < 20; i++) {
			menu->mTitle.mChars[i].mText = NULL;
			menu->mTitle.mChars[i].mFade = 0.0f;
			menu->mTitle.mChars[i].mHold = 0.0f;
		}
	}
}

// the GameCube's UpdateMenuTitle, timed by the refresh rate
static void UpdateMenuTitle(MenuSave* menu, u32 frames)
{
	if (menu->mTitle.mActive && menu->mTitle.mActive > 0) {
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			if (menu->mTitle.mChars[i].mHold > 0.0f) {
				if (menu->mTitle.mChars[i].mFade < 1.0f) {
					if ((menu->mTitle.mChars[i].mFade += (1.0f / (0.3f * gRefreshRate)) * frames) > 1.0f) {
						menu->mTitle.mChars[i].mFade = 1.0f;
					}
				}
			} else if (menu->mTitle.mChars[i].mFade > 0.0f) {
				if ((menu->mTitle.mChars[i].mFade -= (1.0f / (0.3f * gRefreshRate)) * frames) < 0.0f) {
					menu->mTitle.mChars[i].mFade = 0.0f;
				}
			}
		}
		s32 shown = 0;
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			if (menu->mTitle.mChars[i].mFade != 0.0f) {
				shown++;
				menu->mTitle.mChars[i].mAngle0 += 0.08726646f * frames;
				menu->mTitle.mChars[i].mAngle1 += 0.05235988f * frames;
			}
		}
		if (shown == 0) {
			menu->mTitle.mActive = FALSE;
		}
	}
}

// the outline the save file details are drawn with
f32 gSaveTextOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gSaveTextOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };

// a line of the save file details, centred on x and y over four dark copies, as the GameCube's gradient text.
// NONMATCHING: retail keeps 0.8, 0.5 and the zero in saved registers across the whole function
extern "C" void DrawSaveDetailText(const char* text, f32 x, f32 y, f32 size, f32 alpha)
{
	u32 bottom  = ((u8)(255.0f * alpha)) | 0xEDA83600;
	u32 opacity = (u32)(255.0f * alpha);
	f32 zero    = 0.0f;
	for (int i = 0; i < 4; i++) {
		f32 outlineY = (zero + (y + gSaveTextOutlineY[i])) - size * 0.8f * 0.5f;
		f32 outlineX = x + gSaveTextOutlineX[i];
		f32 width    = MeasureGameTextWidth(&gGame.mGs, text, 1, size * 0.8f);
		DrawGsProportionalTextFormat(&gGame.mGs, opacity, text, outlineX - width * 0.5f, outlineY, 0.0f, size * 0.8f, size * 0.8f);
	}
	DrawGsProportionalTextFormat(&gGame.mGs, bottom, text, x - MeasureGameTextWidth(&gGame.mGs, text, 1, size * 0.8f) * 0.5f,
	                             (y + zero) - size * 0.8f * 0.5f, 0.0f, size * 0.8f, size * 0.8f);
}

extern "C" s32 IndexOfLargestOfThree(s64 a, s64 b, s64 c)
{
	if (a >= b) {
		if (a >= c) {
			return 0;
		}
		return 2;
	}
	if (b >= c) {
		return 1;
	}
	return 2;
}

// the save menu's pointer events, the slots its MenuUtil base has in the vtable
extern "C" void fn_80083048()
{
}

// the pointer left a pane
extern "C" void fn_8008304C(MenuSave* menu, CbPane* pane)
{
	switch (pane->mKind) {
	case 0:
	case 1:
	case 2:
		if (menu->mFile == pane->mKind && menu->mFile != -1) {
			menu->mFile = -1;
		}
		break;
	case 3:
	case 4:
		if (menu->mMode == 8 || menu->mMode == 3 || menu->mMode == 18 || menu->mMode == 19) {
			if (menu->mChoice == pane->mKind - 3 && menu->mChoice != -1) {
				menu->mChoice = -1;
			}
		}
		break;
	}
}

// the pointer came onto a pane
extern "C" void fn_800830D4(MenuSave* menu, CbPane* pane)
{
	switch (pane->mKind) {
	case 0:
		if (menu->mFile != 0) {
			menu->mFile = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		break;
	case 1:
		if (menu->mFile != 1) {
			menu->mFile = 1;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		break;
	case 2:
		if (menu->mFile != 2) {
			menu->mFile = 2;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		break;
	}
	if (menu->mMode == 8 || menu->mMode == 3 || menu->mMode == 18 || menu->mMode == 19) {
		switch (pane->mKind) {
		case 3:
			if (menu->mChoice != 0) {
				menu->mChoice = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			break;
		case 4:
			if (menu->mChoice != 1) {
				menu->mChoice = 1;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			break;
		}
	}
}

// the pointer came on screen while both controls work: nothing is picked until it points
extern "C" void fn_80083268(MenuSave* menu)
{
	if ((gGame.mMenuControls & MenuControlStick) && (gGame.mMenuControls & MenuControlPointer)) {
		if (menu->mMode == 2) {
			menu->mFile = -1;
		}
		if (menu->mMode == 8 || menu->mMode == 3 || menu->mMode == 18 || menu->mMode == 19) {
			menu->mChoice = -1;
		}
	}
}

// the pointer went away while both controls work: the stick's file and choice come back
extern "C" void fn_800832C8(MenuSave* menu)
{
	if ((gGame.mMenuControls & MenuControlStick) && (gGame.mMenuControls & MenuControlPointer)) {
		if (menu->mMode == 2 && menu->mFile == -1 && menu->mStickFile != -1) {
			menu->mFile = menu->mStickFile;
		}
		if ((menu->mMode == 8 || menu->mMode == 3 || menu->mMode == 18 || menu->mMode == 19) && menu->mChoice == -1
		    && menu->mStickChoice != -1) {
			menu->mChoice = menu->mStickChoice;
		}
	}
}

// what is picked when a page opens: nothing while the pointer is the one choosing
extern "C" void fn_80083350(MenuSave* menu)
{
	if (menu->mMode == 3 || menu->mMode == 18 || menu->mMode == 19) {
		menu->mStickChoice = 1;
		if (menu->IsPointerActive()) {
			menu->mChoice = -1;
		} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
			menu->mChoice = -1;
		} else {
			menu->mChoice = menu->mStickChoice;
		}
	}
	if (menu->mMode == 2) {
		if (menu->IsPointerActive()) {
			menu->mFile = -1;
		} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
			menu->mFile = -1;
		} else {
			menu->mFile = menu->mStickFile;
		}
	}
}

// empties the status table, the GameCube's inline ClearStatusTable
void ClearStatusTable(MenuStatusTable* table)
{
	table->mPage = 0;
	for (int i = 0; i < 256; i++) {
		table->mItems[i].mItem  = -1;
		table->mItems[i].mCount = -1;
		table->mItems[i].mStatus.Clear();
	}
	for (int i = 0; i < 4; i++) {
		table->mPrev[i].mItem  = -1;
		table->mPrev[i].mCount = -1;
		table->mPrev[i].mStatus.Clear();
		table->mNext[i].mItem  = -1;
		table->mNext[i].mCount = -1;
		table->mNext[i].mStatus.Clear();
	}
}

// the status menu's page of 8 owned items, and the 4 before and after it, the GameCube's
// NONMATCHING: register allocation only, as on the GameCube
extern "C" void PopulateOwnedItemPage(MenuStatusTable* table)
{
	s32 count;
	s16 num;
	int i;
	InventoryItem* pairs;
	s16 id;
	s32 n;
	for (i = 0; i < 256; i++) {
		table->mItems[i].mItem          = -1;
		table->mItems[i].mCount         = -1;
		table->mItems[i].mStatus.mFlags = 0;
	}
	n     = 0;
	count = 0;
	for (i = 255; i >= 0; i--) {
		pairs                    = gGame.GetPlayer()->mParams.mInventory.mItems;
		const InventoryItem& row = pairs[i];
		id                       = row.mId;
		num                      = row.mCount;
		if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
			if (count >= table->mPage * 4 && count < (table->mPage + 2) * 4) {
				table->mItems[n].mItem  = id;
				table->mItems[n].mCount = num;
				if (++n >= 8) {
					break;
				}
			}
			count++;
		}
	}
	n     = 0;
	count = 0;
	for (i = 0; i < 4; i++) {
		table->mPrev[i].mItem          = -1;
		table->mPrev[i].mCount         = -1;
		table->mPrev[i].mStatus.mFlags = 0;
		table->mNext[i].mItem          = -1;
		table->mNext[i].mCount         = -1;
		table->mNext[i].mStatus.mFlags = 0;
	}
	for (i = 255; i >= 0; i--) {
		pairs                    = gGame.GetPlayer()->mParams.mInventory.mItems;
		const InventoryItem& row = pairs[i];
		id                       = row.mId;
		num                      = row.mCount;
		if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
			if (count >= (table->mPage + 2) * 4 && count < (table->mPage + 4) * 4) {
				table->mNext[n].mItem  = id;
				table->mNext[n].mCount = num;
				if (++n >= 4) {
					break;
				}
			}
			count++;
		}
	}
	n     = 0;
	count = 0;
	for (i = 255; i >= 0; i--) {
		pairs                    = gGame.GetPlayer()->mParams.mInventory.mItems;
		const InventoryItem& row = pairs[i];
		id                       = row.mId;
		num                      = row.mCount;
		if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
			if (count >= (table->mPage - 1) * 4 && count < (table->mPage + 1) * 4) {
				table->mPrev[n].mItem  = id;
				table->mPrev[n].mCount = num;
				if (++n >= 4) {
					break;
				}
			}
			count++;
		}
	}
}

// how many of the item chibi robo holds, the GameCube's. NONMATCHING: retail fetches the player again for the count
// and walks the slots by a byte offset; ours keeps the first pointer (MarkAvailableStatusEntries inlines it nine times)
extern "C" s16 GetPlayerItemCount(s16 id)
{
	const WiiPlayerState* player = gGame.GetPlayer();
	s16 count                    = 0;
	for (int i = 255; i >= 0; i--) {
		if (id == player->mParams.mInventory.mItems[i].mId) {
			count = gGame.GetPlayer()->mParams.mInventory.mItems[i].mCount;
			break;
		}
	}
	return count;
}

// empties the page of things bought, the GameCube's ClearPCShopPage<ItemPriceList>
void ClearShopHistoryPage(MenuPCPage* page)
{
	page->mStart = 0;
	for (int i = 0; i < 20; i++) {
		page->mRows[i].mItem         = -1;
		page->mRows[i].mCount        = -1;
		page->mRows[i].mPrice        = -1;
		page->mRows[i].mIsAttachment = 0;
		page->mIndex[i]              = -1;
	}
	page->mPrev[0].mItem         = -1;
	page->mPrev[0].mCount        = -1;
	page->mPrev[0].mPrice        = -1;
	page->mPrev[0].mIsAttachment = 0;
	page->mNext[0].mItem         = -1;
	page->mNext[0].mCount        = -1;
	page->mNext[0].mPrice        = -1;
	page->mNext[0].mIsAttachment = 0;
}

// the shop's page of 9 from what was bought, and the rows just before and after it, the GameCube's
// NONMATCHING: register allocation only, as on the GameCube
static inline void ClearShopHistoryRows(MenuPCPage* list)
{
	for (int i = 0; i < 20; i++) {
		list->mRows[i].mItem         = -1;
		list->mRows[i].mCount        = -1;
		list->mRows[i].mPrice        = -1;
		list->mRows[i].mIsAttachment = 0;
		list->mIndex[i]              = -1;
	}
}

extern "C" void PopulateShopHistoryPage(MenuPCPage* list)
{
	s32 count;
	s32 n;
	s16 price;
	ClearShopHistoryRows(list);
	count = 0;
	n     = 0;
	for (int i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopHistoryEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart && count < list->mStart + 9 && n < 9) {
				list->mRows[n].mItem  = itemId;
				list->mRows[n].mCount = stock;
				list->mRows[n].mPrice = price;
				list->mIndex[n]       = i;
				if (entry->mIsAttachment)
					list->mRows[n].mIsAttachment = 1;
				n++;
			}
			count++;
		}
	}
	list->mPrev[0].mItem         = -1;
	list->mPrev[0].mCount        = -1;
	list->mPrev[0].mPrice        = -1;
	list->mPrev[0].mIsAttachment = 0;
	list->mNext[0].mItem         = -1;
	list->mNext[0].mCount        = -1;
	list->mNext[0].mPrice        = -1;
	list->mNext[0].mIsAttachment = 0;
	count                        = 0;
	n                            = 0;
	for (int i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopHistoryEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart + 9 && count < list->mStart + 10) {
				list->mNext[n].mItem  = itemId;
				list->mNext[n].mCount = stock;
				list->mNext[n].mPrice = price;
				if (entry->mIsAttachment)
					list->mNext[n].mIsAttachment = 1;
				if (++n >= 1) {
					break;
				}
			}
			count++;
		}
	}
	count = 0;
	n     = 0;
	for (int i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopHistoryEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart - 1 && count < list->mStart) {
				list->mPrev[n].mItem  = itemId;
				list->mPrev[n].mCount = stock;
				list->mPrev[n].mPrice = price;
				if (entry->mIsAttachment)
					list->mPrev[n].mIsAttachment = 1;
				if (++n >= 1) {
					break;
				}
			}
			count++;
		}
	}
}

// empties the page of the shop's catalog, the GameCube's ClearPCShopPage<PriceList>
void ClearShopCatalogPage(MenuPCPage* page)
{
	page->mStart = 0;
	for (int i = 0; i < 20; i++) {
		page->mRows[i].mItem  = -1;
		page->mRows[i].mCount = -1;
		page->mRows[i].mPrice = -1;
		page->mIndex[i]       = -1;
	}
	page->mPrev[0].mItem         = -1;
	page->mPrev[0].mCount        = -1;
	page->mPrev[0].mPrice        = -1;
	page->mPrev[0].mIsAttachment = 0;
	page->mNext[0].mItem         = -1;
	page->mNext[0].mCount        = -1;
	page->mNext[0].mPrice        = -1;
}

// the shop's page of 9 from the catalog, and the items just before and after it, the GameCube's
// NONMATCHING: register allocation only, as on the GameCube
extern "C" void PopulateShopCatalogPage(MenuPCPage* list)
{
	s32 count;
	s32 n;
	s16 price;
	int i;
	for (i = 0; i < 20; i++) {
		list->mRows[i].mItem  = -1;
		list->mRows[i].mCount = -1;
		list->mRows[i].mPrice = -1;
		list->mIndex[i]       = -1;
	}
	count = 0;
	n     = 0;
	for (i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopCatalogEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart && count < list->mStart + 9 && n < 9) {
				list->mRows[n].mItem  = itemId;
				list->mRows[n].mCount = stock;
				list->mRows[n].mPrice = price;
				list->mIndex[n]       = i;
				n++;
			}
			count++;
		}
	}
	list->mPrev[0].mItem  = -1;
	list->mPrev[0].mCount = -1;
	list->mPrev[0].mPrice = -1;
	list->mNext[0].mItem  = -1;
	list->mNext[0].mCount = -1;
	list->mNext[0].mPrice = -1;
	n                     = 0;
	count                 = 0;
	for (i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopCatalogEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart + 9 && count < list->mStart + 10) {
				list->mNext[n].mItem  = itemId;
				list->mNext[n].mCount = stock;
				list->mNext[n].mPrice = price;
				if (++n >= 1) {
					break;
				}
			}
			count++;
		}
	}
	n     = 0;
	count = 0;
	for (i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopCatalogEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart - 1 && count < list->mStart) {
				list->mPrev[n].mItem  = itemId;
				list->mPrev[n].mCount = stock;
				list->mPrev[n].mPrice = price;
				if (++n >= 1) {
					break;
				}
			}
			count++;
		}
	}
}

extern "C" void MarkAvailableStatusEntries(u8* available)
{
	available[0] = 1;
	available[1] = 1;
	available[2] = 1;
	available[3] = 1;
	available[4] = 1;
	if (GetPlayerItemCount(0x35) > 0)
		available[5] = 1;
	if (GetPlayerItemCount(0x36) > 0)
		available[6] = 1;
	if (GetPlayerItemCount(0x3E) > 0)
		available[7] = 1;
	if (GetPlayerItemCount(0x9E) > 0)
		available[8] = 1;
	if (GetPlayerItemCount(8) > 0 || (gEventFlags.mFlags[29 / 32] & (1u << (29 % 32))))
		available[9] = 1;
	if (GetPlayerItemCount(0x30) > 0 || (gEventFlags.mFlags[30 / 32] & (1u << (30 % 32))))
		available[10] = 1;
	if (GetPlayerItemCount(0x6D) > 0 || (gEventFlags.mFlags[28 / 32] & (1u << (28 % 32))))
		available[11] = 1;
	if (gEventFlags.mFlags[1080 / 32] & (1u << (1080 % 32))) {
		if (GetPlayerItemCount(0x7C) > 0 || (gEventFlags.mFlags[696 / 32] & (1u << (696 % 32))))
			available[12] = 1;
	}
}

// the menus MenuEx keeps, the GameCube's menu group functions: the Wii has nine and its cursor only blinks.
// LoadMenuGroupResources
extern "C" BOOL fn_80084848(MenuEx* ex)
{
	for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
		if (ex->mMenus[i]) {
			ex->mMenus[i]->LoadTexture();
			ex->mMenus[i]->mCursor = &ex->mCursor;
		}
	}
	return TRUE;
}

// ResetMenuGroup
extern "C" void fn_800848C0(MenuEx* ex)
{
	ex->mCursor.mFrame = 0;
	ex->mCursor.mTimer = 0;
	for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
		if (ex->mMenus[i]) {
			ex->mMenus[i]->Reset();
		}
	}
}

extern "C" void DrawOpenMenus(MenuEx* ex)
{
	if (ex->mActive) {
		for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
			if (ex->mMenus[i] && ex->mMenus[i]->IsOpen()) {
				ex->mMenus[i]->Draw();
			}
		}
	}
}

// opens one of the menus
extern "C" void fn_80084B90(MenuEx* ex, s32 index)
{
	if (ex->mMenus[index]) {
		ex->mMenus[index]->Open();
		if (!ex->mActive) {
			ex->mActive = TRUE;
		}
	}
}

// closes the open menus into the given state
extern "C" void fn_80084BF0(MenuEx* ex, s32 state)
{
	for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
		if (ex->mMenus[i] && ex->mMenus[i]->IsOpen()) {
			ex->mMenus[i]->Close(state);
		}
	}
}

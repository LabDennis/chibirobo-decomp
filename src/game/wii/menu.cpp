// The menus, one file from 0x8004F8F8 to 0x800870E8: its .sdata2 literals (0x805A2F70-0x805A3350) are read
// by functions over that whole range and by nothing else, it has one string pool (from 0x802C67C8) and one
// static initializer (0x8008502C), behind which only its weak destructors and this-adjusting thunks follow.
// The pool holds "menu_item.cpp", "menu_status.cpp" and "menu_save.cpp", so those are files this one
// includes; its own name is not in the binary, menu.cpp is ours.
// NONMATCHING: most menu state, drawing and layout functions still have no source. CMenu::Close and
// MenuMain::Close store the caller's r4 as the state (retail 0x80050D9C, 0x80050D18): unlike the GameCube's they
// seem to take the state as a parameter, which their names in symbols.txt (no parameter) do not show; open.
#include "game/wii/menuutil.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/title.h"
#include "game/globals.h"
#include "game/evt.h"

extern "C" void LayoutMainMenuRows(MenuMain* menu);
extern s32 gPointerScreenWidth, gPointerScreenHeight;
extern "C" void DrawMenuWindow(MenuWindow* window);
extern s32 gRingScreenWidth;

static MenuMain sMenuMain;

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

void MenuSave::State0(u32)
{
	MenuSave* menu = this;
	for (int i = 0; i < 3; i++) {
		ResetSaveSlotRecord(&menu->mSlots[i]);
	}
	StartWiiSaveSlotScan(&gGame.mSaveManager, 10);
	menu->SetMode(1);
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

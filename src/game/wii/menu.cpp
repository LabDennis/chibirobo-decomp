// The menus, one file from 0x8004F8F8 to 0x800870E8: its .sdata2 literals (0x805A2F70-0x805A3350) are read
// by functions over that whole range and by nothing else, it has one string pool (from 0x802C67C8) and one
// static initializer (0x8008502C), behind which only its weak destructors and this-adjusting thunks follow.
// The pool holds "menu_item.cpp", "menu_status.cpp" and "menu_save.cpp", so those are files this one
// includes; its own name is not in the binary, menu.cpp is ours.
// NONMATCHING: most menu state, drawing and layout functions still have no source.
#include "game/wii/menuutil.h"
#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/nandfile.h"
#include "revolution/mem.h"
#include "game/wii/saveslots.h"
#include "game/wii/cpadact.h"
#include "game/wii/title.h"
#include "game/globals.h"
#include "game/evt.h"
#include "game/cfont.h"
#include "stl/math.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C" void LayoutMainMenuRows(MenuMain* menu);
extern "C" void RebuildStageSelectList(MenuStageSelect* menu);
extern s32 gPointerScreenWidth, gPointerScreenHeight;
extern s32 gRefreshRate;
extern MEMiHeapHead* gWiiGameHeap;

extern s32 gScreenSafeLeft, gScreenSafeTop;
extern "C" {
char* GetStageFileName(s32 stage);
char* GetStageName(s32 stage);
char* GetStageDesc(s32 stage);
char* FormatToStaticBuffer(const char* fmt, ...);
void ApproachHudValue(f32* value, f32* target, f32 step);
void ChangeGameStage(CGame* game, s32 stage, BOOL showName);
u8* GetItemDescription(s32 id);
s32 GetItemIconTextureIndex(s32 id);
void DrawTexturedWindowFrame(u32 color, u32 textureColor, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 scaleX, f32 scaleY);
}

// the Wii's text is Japanese with the language setting at 0
static inline bool IsJapanese()
{
	return gGame.mLanguage == 0;
}

static MenuMainItem sMainLines[] = {
	{ "MAP SELECT", 5 }, { "\x01", -1 }, { "SAVE & LOAD", 6 }, { "SYSTEM MENU", 1 }, { NULL, 0 },
};

// the file's menus, in their .bss order (0x8044577C on)
static MenuMain sMenuMain;
static MenuStageSelect sMenuStageSelect;
static MenuSFTest sMenuSFTest;
static MenuSFTestSub sMenuSFTestSub;
static MenuSFTestErr sMenuSFTestErr;
static MenuStatus sMenuStatus;
static MenuItem sMenuItem;
static MenuPC sMenuPC;
static MenuSave sMenuSave;

// the stages the stage select offers, in its order
static s32 sStageOrder[] = { 0, 5, 7, 1, 2, 4, 6, 0x12, 3, 9, 0x10, 0xB, 0x16, 0xD, 0xE, 0xA, -1 };

// NONMATCHING: register allocation and the order the corner temporaries are built. Unlike the GameCube's it
// draws no texture, and it skips a window without size
extern "C" void DrawMenuWindow(MenuWindow* window)
{
	if (window->mDrawAlpha > 0.0f && window->mWidth > 0.0f && window->mHeight > 0.0f) {
		f32 width       = window->mWidth;
		f32 height      = window->mHeight;
		f32 x           = window->mX - width / 2.0f;
		f32 y           = window->mY - height / 2.0f;
		f32 dx          = 2.0f * (f32)sin(4.712389f * (1.0f - window->mDrawAlpha));
		f32 dy          = 2.0f * (f32)sin(4.712389f * (1.0f - window->mDrawAlpha));
		f32 z           = -10.0f;
		Vec outline[18] = { (Vec) { x - dx, 8.0f + (y - dy), z },
			                (Vec) { 8.0f + (x - dx), y - dy, z },
			                (Vec) { dx + (x + width) - 8.0f, y - dy, z },
			                (Vec) { dx + (x + width), 8.0f + (y - dy), z },
			                (Vec) { dx + (x + width), dy + (y + height) - 8.0f, z },
			                (Vec) { dx + (x + width) - 8.0f, dy + (y + height), z },
			                (Vec) { 8.0f + (x - dx), dy + (y + height), z },
			                (Vec) { x - dx, dy + (y + height) - 8.0f, z },
			                (Vec) { x - dx, 8.0f + (y - dy), z },
			                (Vec) { (6.0f + x) - dx, 8.0f + ((6.0f + y) - dy), z },
			                (Vec) { 8.0f + ((6.0f + x) - dx), (6.0f + y) - dy, z },
			                (Vec) { dx + (width + (6.0f + x)) - 8.0f, (6.0f + y) - dy, z },
			                (Vec) { dx + (width + (6.0f + x)), 8.0f + ((6.0f + y) - dy), z },
			                (Vec) { dx + (width + (6.0f + x)), dy + (height + (6.0f + y)) - 8.0f, z },
			                (Vec) { dx + (width + (6.0f + x)) - 8.0f, dy + (height + (6.0f + y)), z },
			                (Vec) { 8.0f + ((6.0f + x) - dx), dy + (height + (6.0f + y)), z },
			                (Vec) { (6.0f + x) - dx, dy + (height + (6.0f + y)) - 8.0f, z },
			                (Vec) { (6.0f + x) - dx, 8.0f + ((6.0f + y) - dy), z } };
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		u32 color = (u32)(255.0f * window->mDrawAlpha) | 0xD2D2D200;
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, outline, color, 9);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_GEQUAL, GX_FALSE);
		color = (u32)(96.0f * window->mDrawAlpha);
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, &outline[9], color, 9);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
		gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
		color = (u32)(255.0f * window->mDrawAlpha) | 0xFAFAFA00;
		DrawGsLineStrip(&gGame.mGs, outline, color, 4);
		DrawGsLineStrip(&gGame.mGs, &outline[7], color, 2);
		color = (u32)(255.0f * window->mDrawAlpha) | 0x96969600;
		DrawGsLineStrip(&gGame.mGs, &outline[3], color, 5);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	}
}

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

BOOL CMenu::Update(u32 frames)
{
	if (mSuspended) {
		if (mSuspendFade != 0 && mSuspendAlpha > 0.0f) {
			if ((mSuspendAlpha -= (1.0f / (0.2f * gRefreshRate)) * frames) < 0.0f) {
				mSuspendAlpha = 0.0f;
			}
		}
	} else if (mSuspendAlpha < 1.0f) {
		if ((mSuspendAlpha += (1.0f / (0.2f * gRefreshRate)) * frames) > 1.0f) {
			mSuspendAlpha = 1.0f;
		}
	}
	if (!IsOpen()) {
		return FALSE;
	}
	if (mSuspended) {
		mAlpha             = mFade * mSuspendAlpha;
		mWindow.mDrawAlpha = mAlpha;
		return FALSE;
	}
	switch (mState) {
	case 0:
		break;
	case 1:
		mState = 2;
	case 2:
		mState = 3;
		break;
	case 3:
		if (mFade < 1.0f) {
			if ((mFade += (1.0f / (0.2f * gRefreshRate)) * frames) > 1.0f) {
				mFade = 1.0f;
			}
		}
		if (mFade >= 1.0f) {
			mState = 4;
		}
		break;
	case 4:
		break;
	case 5:
		if (mFade > 0.0f) {
			if ((mFade -= (1.0f / (0.2f * gRefreshRate)) * frames) < 0.0f) {
				mFade = 0.0f;
			}
		}
		if (0.0f == mFade) {
			mState = 6;
		}
		mWindow.mOpen = FALSE;
		break;
	case 6:
		mState = 0;
		break;
	}
	mAlpha             = mFade * mSuspendAlpha;
	mWindow.mDrawAlpha = mAlpha;
	return mState;
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

extern "C" void LayoutMainMenuRows(MenuMain* menu)
{
	menu->mFontSize = 16.0f;
	menu->mBaseRows = 0;
	menu->mRowCount = 0.0f;
	menu->mMaxWidth = 0.0f;
	for (int i = 0;; i++) {
		menu->mText[i] = sMainLines[i].mText;
		if (menu->mText[i] == NULL) {
			menu->mBaseRows = i;
			menu->mRowCount = i;
			break;
		}
		menu->mTextWidth[i] = MeasureGameTextWidth(&gGame.mGs, menu->mText[i], 1, menu->mFontSize);
		if (menu->mTextWidth[i] > menu->mMaxWidth) {
			menu->mMaxWidth = menu->mTextWidth[i];
		}
	}
	int items = 0;
	for (int k = 0; k < 5; k++) {
		if (menu->mExtraRows[k].mText) {
			items++;
		}
	}
	int line = menu->mRowCount;
	if (items > 0) {
		menu->mText[line]      = "\x01";
		menu->mTextWidth[line] = 1.0f;
		line++;
		for (int j = 0; j < items; j++) {
			if (menu->mExtraRows[j].mText) {
				menu->mText[line]      = menu->mExtraRows[j].mText;
				menu->mTextWidth[line] = MeasureGameTextWidth(&gGame.mGs, menu->mText[line], 1, menu->mFontSize);
				if (menu->mTextWidth[line] > menu->mMaxWidth) {
					menu->mMaxWidth = menu->mTextWidth[line];
				}
				line++;
			}
		}
		menu->mRowCount = line;
	}
	if (menu->mSelection >= menu->mRowCount) {
		menu->mSelection = menu->mRowCount - 1.0f;
	}
	menu->mWindow.SetSize(60.0f + menu->mMaxWidth, menu->mRowCount * (2.0f + menu->mFontSize) + 24.0f);
	menu->mWindow.SetPos(menu->mWindow.mWidth / 2 + gScreenSafeLeft, menu->mWindow.mHeight / 2 + gScreenSafeTop);
}

void MenuStageSelect::Init()
{
	CMenu::Init();
}

void MenuMain::Init()
{
	CMenu::Init();
}

// NONMATCHING: retail compares the stage with its operands the other way round
void MenuStageSelect::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
	if (gGame.GetStatusFlags(0x0008000000000000ULL)) {
		mSelection = -1;
		for (int i = 0; sStageOrder[i] >= 0; i++) {
			if (sStageOrder[i] == gGame.mStage) {
				mSelection = i;
				break;
			}
		}
		if (mSelection < 0) {
			mSelection = 0;
		}
	} else {
		mSelection = gGame.mStage;
	}
	RebuildStageSelectList(this);
}

static inline void FinishStageLabel(char* text)
{
	for (int i = 0; text[i]; i++) {
		if (IsJapanese() && (IsSjisLead(text[i]) & 1)) {
			if (text[++i] == 0) {
				text[i - 1] = '.';
				break;
			}
		}
	}
}

static inline void MeasureStageLabels(MenuStageSelect* menu)
{
	menu->mRowCount = menu->mCount;
	menu->mMaxWidth = 0.0f;
	menu->mFontSize = 16.0f;
	for (int i = 0; i < menu->mRowCount; i++) {
		menu->mLabelWidth[i] = MeasureGameTextWidth(&gGame.mGs, menu->mLabels[i], 0, menu->mFontSize);
		if (menu->mLabelWidth[i] > menu->mMaxWidth)
			menu->mMaxWidth = menu->mLabelWidth[i];
	}
}

// NONMATCHING: register allocation. The Wii still looks the file name up but no longer copies it into the label
extern "C" void RebuildStageSelectList(MenuStageSelect* menu)
{
	menu->mCount = 0;
	for (int i = 0; i < 100; i++) {
		menu->mLabels[i][0]  = 0;
		menu->mLabelWidth[i] = 0.0f;
	}
	// with this status bit only the stages in sStageOrder; without it every room, a rule, then stages 100 and up
	if (gGame.GetStatusFlags(0x0008000000000000ULL)) {
		for (int i = 0; sStageOrder[i] >= 0; i++) {
			GetStageFileName(sStageOrder[i]);
			const char* format = IsJapanese() ? "(%2d)%-.20s" : "(%2d)%-.30s";
			sprintf(menu->mLabels[i], format, sStageOrder[i],
			        FormatToStaticBuffer("%s%s", GetStageName(sStageOrder[i]), GetStageDesc(sStageOrder[i])));
			FinishStageLabel(menu->mLabels[i]);
			menu->mCount++;
		}
		MeasureStageLabels(menu);
		menu->mWindow.SetSize(60.0f + menu->mMaxWidth, (2.0f + menu->mFontSize) * menu->mRowCount + 24.0f);
		menu->mWindow.SetPos(24.0f + (menu->mWindow.mWidth / 2.0f + gScreenSafeLeft),
		                     24.0f + (menu->mWindow.mHeight / 2.0f + gScreenSafeTop));
	} else {
		menu->mCount = gGame.mRoomCount;
		for (int i = 0; i < gGame.mRoomCount; i++) {
			GetStageFileName(i);
			sprintf(menu->mLabels[i], "(%2d)%-.18s", i, FormatToStaticBuffer("%s%s", GetStageName(i), GetStageDesc(i)));
			FinishStageLabel(menu->mLabels[i]);
		}
		strcpy(menu->mLabels[menu->mCount], "\x01");
		menu->mCount++;
		for (int i = 100; GetStageFileName(i); i++) {
			sprintf(menu->mLabels[menu->mCount], "%-8s(%2d)", GetStageFileName(i), i);
			menu->mCount++;
		}
		MeasureStageLabels(menu);
		menu->mWindow.SetSize(2.0f * (60.0f + menu->mMaxWidth), 20.0f * (2.0f + menu->mFontSize) + 24.0f);
		menu->mWindow.SetPos(menu->mWindow.mWidth / 2.0f + gScreenSafeLeft, menu->mWindow.mHeight * 0.5f + gScreenSafeTop);
	}
}

BOOL MenuStageSelect::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		if (gWiiPointer.Check(PadActStickUp)) {
			do {
				if (--mSelection < 0) {
					mSelection = mCount - 1;
				}
			} while (*mLabels[mSelection] == *"\x01");
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickDown)) {
			do {
				if (++mSelection > mCount - 1) {
					mSelection = 0;
				}
			} while (*mLabels[mSelection] == *"\x01");
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickLeft)) {
			// the full list stands in columns of 20 rows, left and right jump a column
			if (mSelection - 20 >= 0 && *mLabels[mSelection - 20] != *"\x01") {
				mSelection -= 20;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		} else if (gWiiPointer.Check(PadActStickRight)) {
			if (mSelection + 20 < mCount && *mLabels[mSelection + 20] != *"\x01") {
				mSelection += 20;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		} else if (gWiiPointer.Check(PadActCancel)) {
			Close(5);
			sMenuMain.Resume();
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActConfirm)) {
			Close(5);
			sMenuMain.Close(5);
			// with the short list the rows are sStageOrder's entries, not stage numbers
			if (gGame.GetStatusFlags(0x0008000000000000ULL)) {
				ChangeGameStage(&gGame, sStageOrder[mSelection], 1);
			} else {
				ChangeGameStage(&gGame, mSelection, 1);
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
	}
	return IsOpen();
}

void MenuSFTest::InitRows()
{
	if (mRows == NULL) {
		mRows = (MenuTextRow*)MEMAllocFromExpHeapEx(gWiiGameHeap, sizeof(MenuTextRow) * 12, 4);
	}
	mFontSize = 16.0f;
	mRowTotal = 0;
	mMaxWidth = 0.0f;
}

// the save file test's line texts, ten slots, a spare and the free space line
static char* sSFTestText[12];
static char* sSFTestTextBuffer;

// NONMATCHING: retail stores the buffer pointer two instructions earlier, among the line pointers
void MenuSFTest::BuildRows()
{
	if (sSFTestTextBuffer == NULL) {
		char* buffer = (char*)MEMAllocFromExpHeapEx(gWiiGameHeap, 1200, 4);
		for (int i = 0; i < 12; i++) {
			sSFTestText[i] = buffer + i * 100;
		}
		sSFTestTextBuffer = buffer;
	}
	InitRows();
	for (int i = 0; i < 10; i++) {
		strcpy(sSFTestText[i], "");
		mRows[mRowTotal].mText  = sSFTestText[i];
		mRows[mRowTotal].mWidth = MeasureGameTextWidth(&gGame.mGs, mRows[mRowTotal].mText, 0, mFontSize);
		if (mRows[mRowTotal].mWidth > mMaxWidth) {
			mMaxWidth = mRows[mRowTotal].mWidth;
		}
		mRowTotal++;
	}
	mRows[mRowTotal].mText  = "\x01";
	mRows[mRowTotal].mWidth = MeasureGameTextWidth(&gGame.mGs, mRows[mRowTotal].mText, 1, mFontSize);
	if (mRows[mRowTotal].mWidth > mMaxWidth) {
		mMaxWidth = mRows[mRowTotal].mWidth;
	}
	mRowTotal++;
	u32 blocks, inodes;
	GetNandGameFreeSpace(&gGame.mSaveManager.mFile, &blocks, &inodes);
	sprintf(sSFTestText[10], "FREE FSBLOCK:%4d   FREE INODE: %2d", blocks, inodes);
	mRows[mRowTotal].mText  = sSFTestText[10];
	mRows[mRowTotal].mWidth = MeasureGameTextWidth(&gGame.mGs, mRows[mRowTotal].mText, 0, mFontSize);
	if (mRows[mRowTotal].mWidth > mMaxWidth) {
		mMaxWidth = mRows[mRowTotal].mWidth;
	}
	mRowTotal++;
	mMaxWidth = 532.0f;
	mWindow.SetSize(60.0f + mMaxWidth, mRowTotal * (2.0f + mFontSize) + 24.0f);
	mWindow.SetPos(8.0f + (mWindow.mWidth / 2 + gScreenSafeLeft), 24.0f + (mWindow.mHeight / 2 + gScreenSafeTop));
}

void MenuSFTest::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
	BuildRows();
	StartOperation(1);
	_168 = -1;
}

void MenuSFTest::Init()
{
	CMenu::Init();
	InitRows();
	StartOperation(0);
}

// NONMATCHING: retail adds the 1 past the terminator a few instructions later
void MenuSFTest::StartOperation(s32 which)
{
	switch (which) {
	case 0:
		mOperation = 0;
		break;
	case 1:
		StartWiiSaveSlotScan(&gGame.mSaveManager, 10);
		mOperation = 1;
		break;
	case 2:
		sprintf(mRows[mSelection].mText, "(%d) saving...", mSelection);
		StartWiiSaveSlotWrite(&gGame.mSaveManager, mSelection, 0);
		mOperation = 3;
		break;
	case 4:
		sprintf(mRows[mSelection].mText, "(%d) loading...", mSelection);
		StartWiiSaveSlotRead(&gGame.mSaveManager, mSelection);
		mOperation = 5;
		break;
	case 6:
		sprintf(mRows[mSelection].mText, "(%d) deleting...", mSelection);
		StartWiiSaveSlotDelete(&gGame.mSaveManager, mSelection);
		mOperation = 7;
		break;
	}
	// a busy line ends in an empty second string
	if (mOperation != 0) {
		char* text = mRows[mSelection].mText;
		sprintf(text + strlen(text) + 1, "");
	}
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

void MenuMain::Close(s32 state)
{
	CMenu::Close(state);
	PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
	gGame.ClearStatusFlags(2);
}

void CMenu::Close(s32 state)
{
	mState = state;
	Resume();
	mWindow.mOpen = FALSE;
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

// an item's card: its description, its name and its icon. NONMATCHING: only the place of "%s" in the string
// pool, which depends on the strings of the functions before it that have no source yet
extern "C" void DrawStatusItemDetails(s32 id, f32 x, f32 y, f32 alpha, f32 scale)
{
	CTexObj tex;
	f32 iconHeight, iconWidth;
	f32 width, height;
	if (scale <= 0.0f)
		return;
	height    = 153.0f * scale;
	width     = 254.0f * scale;
	f32 top   = y - height / 2.0f;
	u32 frame = (u32)(220.0f * alpha);
	DrawTexturedWindowFrame(frame, (u32)(255.0f * alpha) | 0xFFFFFF00, x, top, width, height, alpha, 1.0f, 1.0f);
	top += scale;
	f32 fontSize   = 17.0f * scale;
	f32 textOffset = 24.0f * scale;
	char text[256];
	sprintf(text, "%s", GetItemDescription(id));
	width = MeasureGameTextWidth(&gGame.mGs, text, 1, fontSize);
	DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xFFFFFF00, text, x - width / 2.0f,
	                             (4.0f * scale + (textOffset + (5.0f * fontSize + top))) - 6.0f * scale, 0.0f, fontSize, fontSize);
	sprintf(text, "%s", GetItemName(id));
	width = MeasureGameTextWidth(&gGame.mGs, text, 1, 2.0f * scale + fontSize);
	DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xFFAA0000, text, x - width / 2.0f,
	                             (textOffset + (4.0f * fontSize + top)) - 6.0f * scale, 0.0f, 2.0f * scale + fontSize,
	                             2.0f * scale + fontSize);
	s32 icon = GetItemIconTextureIndex(id);
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[icon], 0);
	tex        = gGame.mTpl.mTex;
	iconWidth  = scale * GXGetTexObjWidth(&tex);
	iconHeight = scale * GXGetTexObjHeight(&tex);
	if (icon >= 0) {
		f32 iconX = ((56.0f * scale + x) - 30.0f * scale) - 127.0f * scale;
		f32 iconY = 46.0f * scale + top;
		u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, iconX, iconY, 0.0f, iconWidth, iconHeight, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
}

static inline f32 MenuRectStart(f32 center, f32 size)
{
	return center - size / 2;
}

static inline f32 MenuRectEnd(f32 center, f32 size)
{
	return size + MenuRectStart(center, size);
}

// NONMATCHING: register allocation of the corner coordinates
extern "C" void DrawMenuRectWithShadow(CMenu* menu, u32 color, f32 x, f32 y, f32 width, f32 height)
{
	// The original evaluates both animation curves even though their values are unused.
	sin(4.712389f * (1.0f - menu->mAlpha));
	sin(4.712389f * (1.0f - menu->mAlpha));
	f32 z           = -10.0f;
	Vec outline[10] = { (Vec) { MenuRectStart(x, width), MenuRectStart(y, height), z },
		                (Vec) { MenuRectEnd(x, width), MenuRectStart(y, height), z },
		                (Vec) { MenuRectEnd(x, width), MenuRectEnd(y, height), z },
		                (Vec) { MenuRectStart(x, width), MenuRectEnd(y, height), z },
		                (Vec) { MenuRectStart(x, width), MenuRectStart(y, height), z },
		                (Vec) { 6.0f + MenuRectStart(x, width), 6.0f + MenuRectStart(y, height), z },
		                (Vec) { 6.0f + MenuRectEnd(x, width), 6.0f + MenuRectStart(y, height), z },
		                (Vec) { 6.0f + MenuRectEnd(x, width), 6.0f + MenuRectEnd(y, height), z },
		                (Vec) { 6.0f + MenuRectStart(x, width), 6.0f + MenuRectEnd(y, height), z },
		                (Vec) { 6.0f + MenuRectStart(x, width), 6.0f + MenuRectStart(y, height), z } };
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, outline, color, 5);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_GEQUAL, GX_FALSE);
	u32 shadow = (u32)(128.0f * menu->mAlpha);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, &outline[5], shadow, 5);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
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

BOOL MenuMain::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		s32 id = -1;
		if (gWiiPointer.Check(PadActStickUp)) {
			do {
				if (--mSelection < 0) {
					mSelection = mRowCount - 1.0f;
				}
			} while (*mText[mSelection] == *"\x01");
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickDown)) {
			do {
				if (++mSelection > mRowCount - 1.0f) {
					mSelection = 0;
				}
			} while (*mText[mSelection] == *"\x01");
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(0x80) || gWiiPointer.Check(0x400) || gWiiPointer.Check(PadActCancel)) {
			Close(5);
		} else if (gWiiPointer.Check(PadActConfirm)) {
			if (mSelection < mBaseRows) {
				id = sMainLines[mSelection].mAction;
			} else {
				// the rows past the divider (hence + 1) each hold an event script to run
				Close(5);
				StartOrQueueEventScript(&gGame.mEvt, (u8*)mExtraRows[mSelection - (mBaseRows + 1)].mAction, 0, -1, -1);
			}
		}
		if (id >= 0) {
			BOOL fade = TRUE;
			switch (id) {
			case 1:
				sMenuStatus.Open();
				break;
			case 2:
				sMenuItem.Open();
				break;
			case 6: // save & load
				fade = FALSE;
				sMenuSFTest.Open();
				break;
			case 5: // map select
				fade = FALSE;
				sMenuStageSelect.Open();
				break;
			case 3:
				sMenuPC.Open();
				break;
			case 4:
				sMenuSave.Open();
				break;
			}
			if (id >= 0) {
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				Suspend(fade);
			} else {
				PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
			}
		}
	}
	return IsOpen();
}

// the two hands either side of the picked row and the frame round it
static inline void DrawMenuSelectionCursor(MenuCursor* cursor, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 scale)
{
	Vec border[6] = {
		(Vec) { x - 8.0f, y - 1.0f, 0.0f },
		(Vec) { 8.0f + (x + width), y - 1.0f, 0.0f },
		(Vec) { 8.0f + (x + width), 1.0f + (y + height), 0.0f },
		(Vec) { x - 8.0f, 1.0f + (y + height), 0.0f },
		(Vec) { x - 8.0f, y - 1.0f, 0.0f },
		(Vec) { 8.0f + (x + width), y - 1.0f, 0.0f },
	};
	DrawGsLineStrip(&gGame.mGs, &border[3], (u32)(255.0f * alpha) | 0x96969600, 3);
	DrawGsLineStrip(&gGame.mGs, &border[1], (u32)(255.0f * alpha) | 0xFAFAFA00, 3);
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[cursor->mFrame + 248], 0);
	CTexObj tex = gGame.mTpl.mTex;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * scale) | 0xFFFFFF00, (u32)(255.0f * scale) | 0xFFFFFF00,
	                   (u32)(255.0f * scale) | 0xFFFFFF00, (u32)(255.0f * scale) | 0xFFFFFF00, (x - 16.0f) - 4.0f, height / 2 + y, 0.0f,
	                   24.0f * scale, 24.0f * scale, 0.0f, 0.0f, 0.0f);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * scale) | 0xFFFFFF00, (u32)(255.0f * scale) | 0xFFFFFF00,
	                   (u32)(255.0f * scale) | 0xFFFFFF00, (u32)(255.0f * scale) | 0xFFFFFF00, 4.0f + (16.0f + (x + width)), height / 2 + y,
	                   0.0f, 24.0f * scale, 24.0f * scale, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// NONMATCHING: register allocation; retail also keeps the cursor's 24 in a register for both hands
BOOL MenuMain::Draw()
{
	if (CMenu::Draw()) {
		f32 x     = mWindow.mX;
		f32 y     = mWindow.mY;
		f32 maxW  = mMaxWidth;
		f32 lines = mRowCount;
		f32 lineH = mFontSize;
		for (int i = 0; i < lines; i++) {
			u32 color;
			if (i == mSelection) {
				color = (u32)(255.0f * mAlpha) | 0x20202000;
			} else {
				color = (u32)(255.0f * mAlpha) | 0x40404000;
			}
			if (*mText[i] == *"\x01") {
				f32 lineY  = y - ((2.0f + lineH) * lines) / 2 + (2.0f + lineH) * (0.5f + i);
				f32 left   = x - (30.0f + maxW) / 2;
				u32 shadow = (u32)(255.0f * mAlpha) | 0x96969600;
				DrawGsLineSegment(&gGame.mGs, NULL, left, lineY, 0.0f, shadow, left + (30.0f + maxW), lineY, 0.0f, shadow);
				u32 light = (u32)(255.0f * mAlpha) | 0xFAFAFA00;
				DrawGsLineSegment(&gGame.mGs, NULL, 1.0f + left, 1.0f + lineY, 0.0f, light, 1.0f + (left + (30.0f + maxW)), 1.0f + lineY,
				                  0.0f, light);
			} else {
				DrawGsProportionalTextFormat(&gGame.mGs, color, mText[i], x - mMaxWidth / 2,
				                             y - ((2.0f + lineH) * lines) / 2 + (2.0f + lineH) * i, 0.0f, lineH, lineH);
			}
		}
		f32 cx = x - maxW / 2;
		f32 cy = y - ((2.0f + lineH) * lines) / 2 + (2.0f + lineH) * mSelection;
		f32 alpha;
		if (sMenuStatus.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuStatus.mWindow.mDrawAlpha : 0.0f;
		} else if (sMenuSFTest.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuSFTest.mWindow.mDrawAlpha : 0.0f;
		} else if (sMenuStageSelect.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuStageSelect.mWindow.mDrawAlpha : 0.0f;
		} else if (sMenuPC.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuPC.mWindow.mDrawAlpha : 0.0f;
		} else if (sMenuSave.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuSave.mWindow.mDrawAlpha : 0.0f;
		} else {
			alpha = mAlpha;
		}
		DrawMenuSelectionCursor(mCursor, cx, cy, maxW, lineH, mAlpha, alpha);
		return TRUE;
	}
	return FALSE;
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

void MenuPC::Init()
{
	MenuPC* menu = this;
	menu->CMenu::Init();
	menu->mDepositBalance = 0;
	menu->mDialogueNumber = 0;
}

// the shop's purchase history rows; the functions that fill them are in the file's second half
static MenuPCItem sShopHistoryEntries[20];

static void (MenuPC::*sPCStepHandlers[])(u32)
    = { &MenuPC::State0, &MenuPC::State1, &MenuPC::State2, &MenuPC::State3, &MenuPC::State4,  &MenuPC::State5,
	    &MenuPC::State6, &MenuPC::State7, &MenuPC::State8, &MenuPC::State9, &MenuPC::State10, &MenuPC::State11 };

static inline void PCSlide(f32& value, f32& target, f32 speed)
{
	if (value > target) {
		if ((value -= speed) < target)
			value = target;
	}
	if (value < target) {
		if ((value += speed) > target)
			value = target;
	}
}

static inline void PCEase(f32& value, f32& target)
{
	f32 rate = fabs((target - value) * 0.25f);
	if (rate < 1.0f)
		rate = 1.0f;
	PCSlide(value, target, rate);
}

static inline void PCEaseCall(f32& value, f32& target)
{
	f32 rate = fabs((target - value) * 0.25f);
	if (rate <= 1.0f)
		rate = 1.0f;
	ApproachHudValue(&value, &target, rate);
}

static inline XObj* PCWorldObject(s32 index)
{
	return gGame.mXObjs.mPools[22].Get(index);
}

// NONMATCHING: retail's absolute values in the easing are single precision, fabs adds an frsp each time
BOOL MenuPC::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		PCEase(_19C, _1A4);
		PCEase(mFrameHeight, mTargetFrameHeight);
		PCSlide(mTextX, mTargetX, 6.0f * frames);
		PCSlide(mTextY, mTargetY, 6.0f * frames);
		ApproachHudValue(&mScroll, &mScrollTarget, 17.0f);
		PCEaseCall(mMoneyDisplay.shown, mMoneyDisplay.target);
		PCEaseCall(mScrapDisplay.shown, mScrapDisplay.target);
		PCEaseCall(_428, _42C);
		mTexScroll -= 0.008f * frames;
		if (mMode > 1) {
			switch (mTextPhase) {
			case 0:
				switch (mMode) {
				case 3:
				case 7:
				case 10:
					mTargetX = 140.0f;
					break;
				case 4:
					mTargetX = 160.0f;
					break;
				default:
					mTargetX = 180.0f;
					break;
				}
				mTextPhase++;
				break;
			case 1:
				if (mTextX == mTargetX) {
					mTextY   = 129.0f;
					mTargetY = 301.0f;
					mTextPhase++;
				}
				break;
			case 2:
				if (mTextY == mTargetY) {
					switch (mMode) {
					case 3:
					case 7:
					case 10:
						mTextX   = 140.0f;
						mTargetX = 469.0f;
						break;
					case 4:
						mTextX   = 160.0f;
						mTargetX = 449.0f;
						break;
					default:
						mTextX   = 210.0f;
						mTargetX = 429.0f;
						break;
					}
					mTextPhase++;
				}
				break;
			case 3:
				if (mTextX == mTargetX)
					mTextPhase++;
				break;
			case 4:
				break;
			}
		}
		switch (mKeeperAnimStep) {
		case 0:
			mKeeperAnimId = 0;
			break;
		case 1:
			mKeeperAnimId = 1;
			break;
		case 2:
			mKeeperAnimId = 2;
			break;
		case 3:
			mKeeperAnimId = 6;
			break;
		case 4:
			mKeeperAnimId = 7;
			break;
		case 5:
			mKeeperAnimId = 8;
			break;
		case 6:
			mKeeperAnimId = 9;
			break;
		case 7:
			mKeeperAnimId = 2;
			break;
		case 8:
			mKeeperAnimId = 10;
			break;
		default:
			mKeeperAnimId = 0;
			break;
		}
		if (mPlayKeeperAnim == 1) {
			for (s32 i = 0; i < 400; i++) {
				XObj* obj = PCWorldObject(i);
				switch (obj->GetKind()) {
				case 120: // the shopkeeper
					obj->SetAnim(mKeeperAnimId, 0, 0.0f, frames, 1.0f, 15.0f);
					break;
				}
			}
			mPlayKeeperAnim = 0;
		}
		// these steps play once: four frames before their end the next step starts
		switch (mKeeperAnimStep) {
		case 1:
		case 2:
		case 6:
		case 8:
			for (s32 i = 0; i < 400; i++) {
				XObj* obj = PCWorldObject(i);
				switch (obj->GetKind()) {
				case 120: {
					f32 current = obj->GetAnimFrame();
					f32 end     = obj->GetAnimEndFrame();
					if (end - 4.0f <= current) {
						mKeeperAnimStep++;
						mPlayKeeperAnim = 1;
					}
				} break;
				}
			}
			break;
		}
		(this->*sPCStepHandlers[mMode])(frames);
	}
	return IsOpen();
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

void MenuPC::State0(u32)
{
	MenuPC* menu = this;
	if (menu->mShopOnly) {
		ResetPCText(menu, 3);
	} else {
		ResetPCText(menu, 1);
		menu->mProgress       = 0.0f;
		menu->mTargetProgress = 1.0f;
		switch (rand() % 2) {
		case 0:
			menu->mSpeed = 0.04f;
			break;
		case 1:
			menu->mSpeed = 0.08f;
			break;
		}
	}
}

void MenuPC::State1(u32 frames)
{
	MenuPC* menu = this;
	switch (menu->mPhase) {
	case 0:
		PlayWiiSoundEffect(&gGame.mSound, 0x35A, 1.0f, 0.0f);
		menu->mPhase++;
		break;
	case 1:
		break;
	}
	ApproachHudValue(&menu->mProgress, &menu->mTargetProgress, menu->mSpeed * frames);
	if (menu->mProgress >= 1.0f)
		ResetPCText(menu, 2);
}

static inline s16 MenuHistoryValue(int i)
{
	return sShopHistoryEntries[i].mCount;
}

static inline void RestorePCHistory(MenuPC* menu)
{
	if (menu->mHistoryCount > 0) {
		for (int i = 0; i < menu->mHistoryCount; i++) {
			gEventVars.mVars[menu->mHistoryStart + i + menu->mHistoryCount] = MenuHistoryValue(i);
		}
		gEventVars.mVars[196] = menu->mDepositBalance;
	}
}

void MenuPC::State2(u32)
{
	MenuPC* menu = this;
	CPad pad(0);
	if (gWiiPointer.Check(PadActStickUp)) {
		menu->mChoice--;
		if (menu->mChoice < 0)
			menu->mChoice = menu->mChoiceCount - 1;
		PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
	} else if (gWiiPointer.Check(PadActStickDown)) {
		menu->mChoice++;
		if (menu->mChoice > menu->mChoiceCount - 1)
			menu->mChoice = 0;
		PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
	} else if (gWiiPointer.Check(PadActConfirm)) {
		s32 step = menu->mChoices[menu->mChoice];
		// the catalog (mode 7) only buzzes while it has nothing in it
		if (step == 7) {
			if (menu->mCanSelect) {
				ResetPCText(menu, 7);
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			} else {
				PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
			}
		} else if (step >= 0) {
			ResetPCText(menu, step);
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
	} else if (gWiiPointer.Check(PadActCancel)) {
		// B: shrink the frame, mode 11 closes the pc once it is shut
		ResetPCText(menu, 11);
		menu->mTargetFrameHeight = 0.0f;
		RestorePCHistory(menu);
	}
}

// the status and PC menus' empty states
void MenuStatus::State7(u32)
{
}

void MenuStatus::State6(u32)
{
}

void MenuPC::State8(u32)
{
}

void MenuPC::State6(u32)
{
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

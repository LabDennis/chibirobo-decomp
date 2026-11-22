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
#include "Dolphin/card.h"
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
#include "game/menupanelbackground.h"
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
extern s32 gScreenLeft, gScreenTop;
extern "C" {
char* GetStageFileName(s32 stage);
char* GetStageName(s32 stage);
char* GetStageDesc(s32 stage);
char* FormatToStaticBuffer(const char* fmt, ...);
void ApproachHudValue(f32* value, f32* target, f32 step);
void ResumeGameSounds(CGame* game, f32 fadeSeconds);
void ChangeGameStage(CGame* game, s32 stage, BOOL showName);
u8* GetItemDescription(s32 id);
s32 GetItemIconTextureIndex(s32 id);
u8* GetStatusIconDescription(s32 id);
s32 ConvertHudTextToFullWidth(char* text);
u8* GetStatusOverviewDescription(s32 id);
char* GetStatusOverviewLabel(s32 id);
void DrawStatusMenuTabs(MenuStatus* menu);
void fn_80061014(MenuStatus* menu);
void DrawPlayerStatusOverview(MenuStatus* menu);
void fn_80061DBC(MenuStatus* menu);
void DrawStatusIconGrid(MenuStatus* menu);
void DrawStatusHelpText(MenuStatus* menu);
void DrawStatusMenuTitle(MenuStatus* menu);
CCamera* InitGameOrthoCamera(CCamera* camera, const GXRenderModeObj* rmode);
void DrawTexturedWindowFrame(u32 color, u32 textureColor, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 scaleX, f32 scaleY);
}

// the Wii's text is Japanese with the language setting at 0
static inline bool IsJapanese()
{
	return gGame.mLanguage == 0;
}

// stand-ins for nw4r::ut::Rect and nw4r::lyt::Size, whose headers are not in include/nw4r
struct MenuRect {
	MenuRect()
	{
		left   = 0.0f;
		top    = 0.0f;
		right  = 0.0f;
		bottom = 0.0f;
	}

	f32 left, top, right, bottom;
};

struct MenuSize {
	f32 width, height;
};

static MenuMainItem sMainLines[] = {
	{ "MAP SELECT", 5 }, { "\x01", -1 }, { "SAVE & LOAD", 6 }, { "SYSTEM MENU", 1 }, { NULL, 0 },
};

// what MenuSFTestSub offers for a save slot
static MenuMainItem sSFTestSubLines[] = {
	{ "LOAD", 4 },
	{ "SAVE", 2 },
	{ "\x01", -1 },
	{ "DELETE", 6 },
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
static MenuStatusTable sStatusItemTable;
// shared item/status pulse phase at 0x805A1BD0: Open resets it, Update advances it, DrawInventoryGrid reads it
static f32 sInventoryPhase;

// the stages the stage select offers, in its order
static s32 sStageOrder[] = { 0, 5, 7, 1, 2, 4, 6, 0x12, 3, 9, 0x10, 0xB, 0x16, 0xD, 0xE, 0xA, -1 };

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
	f32 size    = 24.0f;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * scale) | 0xFFFFFF00, (u32)(255.0f * scale) | 0xFFFFFF00,
	                   (u32)(255.0f * scale) | 0xFFFFFF00, (u32)(255.0f * scale) | 0xFFFFFF00, (x - 16.0f) - 4.0f, height / 2 + y, 0.0f,
	                   size * scale, size * scale, 0.0f, 0.0f, 0.0f);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * scale) | 0xFFFFFF00, (u32)(255.0f * scale) | 0xFFFFFF00,
	                   (u32)(255.0f * scale) | 0xFFFFFF00, (u32)(255.0f * scale) | 0xFFFFFF00, 4.0f + (16.0f + (x + width)), height / 2 + y,
	                   0.0f, size * scale, size * scale, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// draws row index of a save file test menu: a member of retail's MenuUtil row base (see cgame.h), which this code
// does not model yet
extern "C" void fn_80050314(Menu* menu, s32 index, BOOL, BOOL);

// NONMATCHING: register allocation and the order the corner temporaries are built. Unlike the GameCube's it
// draws no texture, and it skips a window without size
extern "C" void DrawMenuWindow(MenuWindow* window)
{
	if (window->mDrawAlpha > 0.0f && window->mWidth > 0.0f && window->mHeight > 0.0f) {
		f32 width       = window->mWidth;
		f32 height      = window->mHeight;
		f32 x           = window->mX - width / 2.0f;
		f32 y           = window->mY - height / 2.0f;
		f32 dx          = 2.0f * std::sinf(4.712389f * (1.0f - window->mDrawAlpha));
		f32 dy          = 2.0f * std::sinf(4.712389f * (1.0f - window->mDrawAlpha));
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
#include <stdlib.h>

extern "C" void LayoutMainMenuRows(MenuMain* menu);
extern s32 gPointerScreenWidth, gPointerScreenHeight, gRefreshRate;
extern s32 gRingScreenWidth;

// what the PC shop sold and what it sells
static MenuPCItem sShopHistoryEntries[20];
static MenuPCItem sShopCatalogEntries[20];

bool MenuPad::IsConfirmPressed()
{
	if (gWiiPointer.Check(PadActConfirm))
		return true;
	return false;
}

bool MenuPad::IsCancelPressed()
{
	if (gWiiPointer.Check(PadActCancel))
		return true;
	return false;
}

bool MenuPad::IsStickRight()
{
	if (gWiiPointer.Check(PadActStickRight) && (gGame.mMenuControls & MenuControlStick))
		return true;
	return false;
}

bool MenuPad::IsStickLeft()
{
	if (gWiiPointer.Check(PadActStickLeft) && (gGame.mMenuControls & MenuControlStick))
		return true;
	return false;
}

bool MenuPad::IsStickUp()
{
	if (gWiiPointer.Check(PadActStickUp) && (gGame.mMenuControls & MenuControlStick))
		return true;
	return false;
}

bool MenuPad::IsStickDown()
{
	if (gWiiPointer.Check(PadActStickDown) && (gGame.mMenuControls & MenuControlStick))
		return true;
	return false;
}

bool MenuPad::IsPointerActive()
{
	if (gWiiPointer.mPointerValid && (gGame.mMenuControls & MenuControlPointer))
		return true;
	return false;
}

bool MenuPad::IsStickUsedOverPointer()
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

void MenuStageSelect::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
	if (gGame.GetStatusFlags(0x0008000000000000ULL)) {
		mSelection = -1;
		s32* stage = sStageOrder;
		for (int i = 0; *stage >= 0; stage++, i++) {
			if (*stage == gGame.mStage) {
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

static inline u8 StageRowBlue(int row, u8 normal, u8 current)
{
	if (gGame.GetStatusFlags(0x0008000000000000ULL))
		return (u8)(sStageOrder[row] == gGame.mStage ? current : normal);
	return (u8)(row == gGame.mStage ? current : normal);
}

// NONMATCHING: register allocation and the scheduling of the cursor position; the GameCube code otherwise
BOOL MenuStageSelect::Draw()
{
	if (CMenu::Draw()) {
		f32 cx       = mWindow.mX;
		f32 cy       = mWindow.mY;
		f32 maxWidth = mMaxWidth;
		f32 fontSize = mFontSize;
		f32 step     = 2.0f + fontSize;
		f32 x        = 30.0f + (cx - mWindow.mWidth / 2);
		for (int column = 0; column < 2; column++) {
			f32 y = 12.0f + (cy - mWindow.mHeight / 2);
			for (int row = 0; row < 20; row++) {
				int index = column * 20 + row;
				if (index > mCount)
					break;
				u32 color;
				if (index == mSelection)
					color = (0x20200000 | (StageRowBlue(index, 0x20, 0x80) << 8)) | (u32)(255.0f * mAlpha);
				else
					color = (0x64640000 | (StageRowBlue(index, 0x64, 0xC8) << 8)) | (u32)(255.0f * mAlpha);
				if (mLabels[index][0] == *"\x01") {
					// a "\x01" row is a rule: a dark line with a light one under it
					u32 shadow = (u32)(255.0f * mAlpha) | 0x64646400;
					DrawGsLineSegment(&gGame.mGs, NULL, (x - 15.0f), (fontSize / 2 + y), 0.0f, shadow, ((x - 15.0f) + (30.0f + maxWidth)),
					                  (fontSize / 2 + y), 0.0f, shadow);
					u32 light = (u32)(255.0f * mAlpha) | 0xC8C8C800;
					DrawGsLineSegment(&gGame.mGs, NULL, 1.0f + (x - 15.0f), 1.0f + (fontSize / 2 + y), 0.0f, light,
					                  1.0f + ((x - 15.0f) + (30.0f + maxWidth)), 1.0f + (fontSize / 2 + y), 0.0f, light);
				} else {
					DrawGsFixedWidthTextFormat(&gGame.mGs, color, mLabels[index], x, y, 0.0f, fontSize, fontSize);
				}
				y += step;
			}
			x += 60.0f + mMaxWidth;
		}
		if (!gGame.GetStatusFlags(0x0008000000000000ULL)) {
			f32 top    = 12.0f + (cy - mWindow.mHeight / 2);
			u32 shadow = (u32)(255.0f * mAlpha) | 0x64646400;
			f32 bottom = 20.0f * step + top;
			DrawGsLineSegment(&gGame.mGs, NULL, cx, top, 0.0f, shadow, cx, bottom, 0.0f, shadow);
			u32 light = (u32)(255.0f * mAlpha) | 0xC8C8C800;
			DrawGsLineSegment(&gGame.mGs, NULL, 1.0f + cx, 1.0f + top, 0.0f, light, 1.0f + cx, 1.0f + bottom, 0.0f, light);
		}
		DrawMenuSelectionCursor(mCursor, (mSelection / 20) * (60.0f + mMaxWidth) + (30.0f + (cx - mWindow.mWidth / 2)),
		                        (mSelection % 20) * step + (12.0f + (cy - mWindow.mHeight / 2)), maxWidth, fontSize, mAlpha, mAlpha);
		return TRUE;
	}
	return FALSE;
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

static inline s32 CountMenuItems();
static inline void PCEase(f32& value, f32& target);
extern "C" void fn_8005A9D0(MenuItem* menu, u32 frames);
extern "C" void PopulateOwnedItemPage(MenuStatusTable* table);
extern f32 gFrameRateScale;

extern "C" void AddPointerPane(CbPaneList* list, CbPane* pane)
{
	if (list->mCount < 0x100) {
		list->mPanes[list->mCount++] = pane;
	}
}

static inline void CheckPlayerInventoryFlags()
{
	for (s32 i = 255; i >= 0; --i) {
		InventoryItem* items = gGame.GetPlayer()->mParams.mInventory.mItems;
		GetItemFlagsMasked(items[i].mId, 4);
	}
}

// NONMATCHING: all functional calls match, but this build expands MenuPaneFadePair::Update where retail calls it;
// 24 bytes of register and scheduling differences remain
BOOL MenuItem::Update(u32 frames)
{
	static s32 selectedItem = -1;
	if (mState == 6) {
		gPointerPanes.mMenu = NULL;
	}
	if (CMenu::Update(frames) == 4) {
		gPointerPanes.mCount = 0;
		mPaneFades.Update(frames);
		sInventoryPhase += gFrameRateScale * (0.5f * frames);
		if ((mItemPhase += (1.0f / (0.3f * gRefreshRate)) * frames) >= 2.0f)
			mItemPhase = fmod(mItemPhase, 2.0);
		PCEase(mGridX, mTargetGridX);
		ApproachHudValue(&mGridY, &mTargetGridY, 20.0f);
		PCEase(mPanelX, mTargetPanelX);
		PCEase(mPanelY, mTargetPanelY);
		ApproachHudValue(&_198, &_19C, 0.08f * frames);
		if (mAlpha > 0.0f) {
			f32 spinX = 0.08726646f * frames, spinY = 0.05235988f * frames;
			for (int i = 0; i < (int)ARRAY_SIZE(mBg); i++) {
				mBg[i].mAngle0 += spinX;
				mBg[i].mAngle1 += spinY;
			}
		}
		s32 count = CountMenuItems();
		switch (mMode) {
		case 0:
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			mMode++;
			break;
		case 1:
			if (mGridX == mTargetGridX) {
				mMode++;
				if (IsPointerActive()) {
					mSelection     = -1;
					mPrevSelection = -1;
				} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
					mSelection     = -1;
					mPrevSelection = -1;
				} else {
					mSelection     = mStickSelection;
					mPrevSelection = mStickSelection;
				}
				s16 id = mSelection != -1 ? sStatusItemTable.mItems[mSelection].mItem : -1;
				if (id >= 0 && CountMenuItems() > 0) {
					mHelp     = GetItemName(id);
					mShowHelp = 1;
				} else {
					mShowHelp = 1;
					mHelp     = NULL;
				}
			}
			break;
		case 2:
			if (mSelection != -1)
				mStickSelection = mSelection;
			if (!IsStickUsedOverPointer()) {
				if (count > 1) {
					if (IsStickUp()) {
						if (--mSelection < 0)
							mSelection = count - 1;
						PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
					} else if (IsStickDown()) {
						mSelection++;
						if (sStatusItemTable.mItems[mSelection].mItem < 0)
							mSelection = 0;
						PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
					}
				}
				if (count > 4) {
					if (mSelection >= 4) {
						if (IsStickLeft()) {
							if ((mSelection -= 4) < 0)
								mSelection = count - 1;
							PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
						}
						if (IsStickRight() && sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
							mTargetGridY = -56.0f;
							mMode        = 10;
							break;
						}
					} else {
						if (IsStickRight() && mSelection + 4 <= count - 1) {
							mSelection += 4;
							PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
						}
						if (IsStickLeft()) {
							CountMenuItems();
							if (sStatusItemTable.mPage > 0) {
								mTargetGridY = 128.0f;
								mMode        = 20;
								break;
							}
						}
					}
				}
			}
			if (mSelection != -1 && sStatusItemTable.mItems[mSelection].mItem < 0) {
				do {
					--mSelection;
				} while (sStatusItemTable.mItems[mSelection].mItem < 0 && mSelection != 0);
			}
			if (mSelection != mPrevSelection) {
				s16 id = sStatusItemTable.mItems[mSelection].mItem;
				if (id >= 0 && CountMenuItems() > 0) {
					mShowHelp = 1;
					mHelp     = GetItemName(id);
				} else {
					mShowHelp = 1;
					mHelp     = NULL;
				}
			}
			mPrevSelection = mSelection;
			if (IsConfirmPressed() && mSelection != -1 && !mPanes[0].mPointed && !mPanes[1].mPointed) {
				s16 id     = sStatusItemTable.mItems[mSelection].mItem;
				s16 amount = sStatusItemTable.mItems[mSelection].mCount;
				if (count > 0 && id >= 0 && amount > 0)
					selectedItem = id;
				else
					selectedItem = -1;
				mTargetGridX  = -640.0f;
				mTargetPanelY = -200.0f;
				mDelay        = 4 * frames;
				mMode++;
				for (int i = 0; i < (int)ARRAY_SIZE(mTitle.mChars); i++)
					mTitle.mChars[i].mHold = 0.0f;
			} else if (IsCancelPressed()) {
				selectedItem  = -1;
				mTargetGridX  = -640.0f;
				mTargetPanelY = -200.0f;
				mDelay        = 4 * frames;
				mMode++;
				for (int i = 0; i < (int)ARRAY_SIZE(mTitle.mChars); i++)
					mTitle.mChars[i].mHold = 0.0f;
			} else {
				mPanes[0].mEnabled = FALSE;
				mPanes[1].mEnabled = FALSE;
				if (count > 4) {
					CheckPlayerInventoryFlags();
					mPanes[0].mEnabled = sStatusItemTable.mPage > 0;
					mPanes[1].mEnabled = sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1;
				}
				for (int i = 0; i < 8; i++) {
					AddPointerPane(&gPointerPanes, &mCells[i]);
				}
				AddPointerPane(&gPointerPanes, &mPanes[0]);
				AddPointerPane(&gPointerPanes, &mPanes[1]);
			}
			break;
		case 3:
			if (--mDelay < 0) {
				gGame.mEvt.mSelectResult = selectedItem;
				gGame.ClearStatusFlags(2);
				Close(5);
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				mMode++;
			}
			break;
		case 10:
			if (mGridY == mTargetGridY) {
				mMode = 2;
				if (sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
					++sStatusItemTable.mPage;
					PopulateOwnedItemPage(&sStatusItemTable);
					PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
				}
				mGridY       = 36.0f;
				mTargetGridY = 36.0f;
				if (IsPointerActive()) {
					mSelection     = -1;
					mPrevSelection = -1;
				} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
					mSelection     = -1;
					mPrevSelection = -1;
				} else {
					mSelection     = mStickSelection;
					mPrevSelection = mStickSelection;
				}
				if (mSelection == -1) {
					mShowHelp = 1;
					mHelp     = NULL;
				} else if (sStatusItemTable.mItems[mSelection].mItem != -1) {
					mHelp          = GetItemName(sStatusItemTable.mItems[mSelection].mItem);
					mShowHelp      = 1;
					mPrevSelection = mSelection;
				}
			}
			break;
		case 20:
			if (mGridY == mTargetGridY) {
				mMode = 2;
				CountMenuItems();
				if (sStatusItemTable.mPage > 0) {
					--sStatusItemTable.mPage;
					PopulateOwnedItemPage(&sStatusItemTable);
					PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
				}
				mGridY       = 36.0f;
				mTargetGridY = 36.0f;
				if (IsPointerActive()) {
					mSelection     = -1;
					mPrevSelection = -1;
				} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
					mSelection     = -1;
					mPrevSelection = -1;
				} else {
					mSelection     = mStickSelection;
					mPrevSelection = mStickSelection;
				}
				if (mSelection == -1) {
					mShowHelp = 1;
					mHelp     = NULL;
				} else if (sStatusItemTable.mItems[mSelection].mItem != -1) {
					mHelp          = GetItemName(sStatusItemTable.mItems[mSelection].mItem);
					mShowHelp      = 1;
					mPrevSelection = mSelection;
				}
			}
			break;
		}
	}
	fn_8005A9D0(this, frames);
	switch (mShowHelp) {
	case 0:
		break;
	case 1:
		mNameAlpha = 0.0f;
		mShowHelp++;
		mName = mHelp;
		break;
	case 2:
		mNameAlpha += 0.04f * frames;
		if (mNameAlpha >= 1.0f) {
			mNameAlpha = 1.0f;
			mShowHelp  = 0;
		}
		break;
	}
	return IsOpen();
}

// NONMATCHING: the call list matches retail; 32 bytes of float constant/register lifetime and scheduling differences remain
extern "C" void DrawInventoryGrid(s32 selected, f32 x, f32 y, f32 alpha, f32 scale, f32 phase)
{
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	for (int i = 0; i < 8; i++) {
		f32 w = 92.0f * scale, h = 80.0f * scale;
		f32 dx = w / 8.0f + w, dy = h / 8.0f + h;
		f32 top = -dy / 2.0f;
		f32 xx  = (i % 4) * dx + x;
		f32 yy  = 12.0f + (top + ((i / 4) * dy + y));
		CTexObj tex;
		s16 id = sStatusItemTable.mItems[i].mItem, count = sStatusItemTable.mItems[i].mCount;
		scale = 1.0f;
		if (id >= 0 && count > 0) {
			f32 bob = 0.0f;
			if (selected == i)
				bob = 4.0f * (f32)sin(0.017453292f * (20.0f * (f32)fmod(sInventoryPhase, 360.0)));
			s32 icon = GetItemIconTextureIndex(id);
			if (icon >= 0) {
				gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[icon], 0);
				tex = gGame.mTpl.mTex;
				gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
				gGame.mGs.mCurRenderState->SetZCompLoc(GX_FALSE);
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				f32 iw = scale * GXGetTexObjWidth(&tex);
				f32 ih = scale * GXGetTexObjHeight(&tex);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, xx, yy + bob, 1.0f, iw, ih, 0.0f,
				                   0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				gGame.mGs.mCurRenderState->SetZCompLoc(GX_TRUE);
				if (selected == i) {
					gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCCOL, GX_BL_SRCALPHA, GX_LO_SET);
					gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_EQUAL, GX_FALSE);
					f32 pulse = alpha * (0.2f * (f32)sin(3.1415927f * phase) + 0.2f);
					if (pulse < 0.0f)
						pulse = 0.0f;
					DrawGsQuad(&gGame.mGs, (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00,
					           (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00, xx, yy + bob, 1.0f, iw, ih, 0.0f,
					           0.0f, 0.0f);
					gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
					gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
				}
			}
			f32 enlarged = 1.0f;
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
			tex = gGame.mTpl.mTex;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			u16 firstWidth = GXGetTexObjWidth(&tex), secondWidth = GXGetTexObjWidth(&tex);
			f32 digitWidth  = scale * (1.2f * secondWidth) + (1.0f - enlarged) * (scale * firstWidth);
			int firstHeight = GXGetTexObjHeight(&tex) / 10, secondHeight = GXGetTexObjHeight(&tex) / 10;
			f32 digitHeight = scale * (1.2f * secondHeight) + (1.0f - enlarged) * (scale * firstHeight);
			PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			f32 countX         = xx + 22.0f * scale;
			f32 countY         = bob + (yy + 14.0f * scale);
			if (count >= 10) {
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * (count / 10), 0.0f);
				tex.mTexMtx.mDirty = TRUE;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, countX - digitWidth / 3.0f,
				                   countY, 0.0f, digitWidth, digitHeight, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * (count % 10), 0.0f);
				tex.mTexMtx.mDirty = TRUE;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, digitWidth / 3.0f + countX,
				                   countY, 0.0f, digitWidth, digitHeight, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			} else {
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * count, 0.0f);
				tex.mTexMtx.mDirty = TRUE;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, countX, countY, 0.0f, digitWidth,
				                   digitHeight, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
		} else {
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[235], 0);
			tex = gGame.mTpl.mTex;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			u16 tw = GXGetTexObjWidth(&tex), th = GXGetTexObjHeight(&tex);
			DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
			                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, xx, yy, 0.0f, scale * tw, scale * th,
			                   0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
	f32 w = 92.0f * scale, h = 80.0f * scale;
	for (int i = 0; i < 4; i++) {
		if (sStatusItemTable.mPage >= (CountMenuItems() - 1) / 4 - 1)
			break;
		f32 dx = w / 8.0f + w, dy = h / 8.0f + h;
		f32 top = -dy / 2.0f;
		f32 xx  = (i % 4) * dx + x;
		f32 yy  = 12.0f + (top + (2 * dy + y));
		CTexObj tex;
		s16 id = sStatusItemTable.mNext[i].mItem, count = sStatusItemTable.mNext[i].mCount;
		if (id >= 0 && count > 0) {
			s32 icon = GetItemIconTextureIndex(id);
			if (icon >= 0) {
				gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[icon], 0);
				tex = gGame.mTpl.mTex;
				GXGetTexObjWidth(&tex);
				GXGetTexObjHeight(&tex);
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				u16 iw = GXGetTexObjWidth(&tex), ih = GXGetTexObjHeight(&tex);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, xx, yy, 0.0f, scale * iw,
				                   scale * ih, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
			f32 enlarged = 1.0f;
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
			tex = gGame.mTpl.mTex;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			u16 firstWidth = GXGetTexObjWidth(&tex), secondWidth = GXGetTexObjWidth(&tex);
			f32 digitWidth  = scale * (1.2f * secondWidth) + (1.0f - enlarged) * (scale * firstWidth);
			int firstHeight = GXGetTexObjHeight(&tex) / 10, secondHeight = GXGetTexObjHeight(&tex) / 10;
			f32 digitHeight = scale * (1.2f * secondHeight) + (1.0f - enlarged) * (scale * firstHeight);
			PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			f32 countX         = xx + 22.0f * scale;
			f32 countY         = yy + 14.0f * scale;
			if (count >= 10) {
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * (count / 10), 0.0f);
				tex.mTexMtx.mDirty = TRUE;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, countX - digitWidth / 3.0f,
				                   countY, 0.0f, digitWidth, digitHeight, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * (count % 10), 0.0f);
				tex.mTexMtx.mDirty = TRUE;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, digitWidth / 3.0f + countX,
				                   countY, 0.0f, digitWidth, digitHeight, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			} else {
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * count, 0.0f);
				tex.mTexMtx.mDirty = TRUE;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, countX, countY, 0.0f, digitWidth,
				                   digitHeight, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
		} else {
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[235], 0);
			tex = gGame.mTpl.mTex;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			u16 tw = GXGetTexObjWidth(&tex), th = GXGetTexObjHeight(&tex);
			DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
			                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, xx, yy, 0.0f, scale * tw, scale * th,
			                   0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
	for (int i = 0; i < 4; i++) {
		CountMenuItems();
		if (sStatusItemTable.mPage <= 0)
			break;
		f32 dx = w / 8.0f + w, dy = h / 8.0f + h;
		f32 top = -dy / 2.0f;
		f32 xx  = (i % 4) * dx + x;
		f32 yy  = 12.0f + (top + (-1 * dy + y));
		CTexObj tex;
		s16 id = sStatusItemTable.mPrev[i].mItem, count = sStatusItemTable.mPrev[i].mCount;
		if (id >= 0 && count > 0) {
			s32 icon = GetItemIconTextureIndex(id);
			if (icon >= 0) {
				gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[icon], 0);
				tex = gGame.mTpl.mTex;
				GXGetTexObjWidth(&tex);
				GXGetTexObjHeight(&tex);
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				u16 iw = GXGetTexObjWidth(&tex), ih = GXGetTexObjHeight(&tex);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00,
				                   (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00, xx, yy, 0.0f, scale * iw,
				                   scale * ih, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
			f32 enlarged = 1.0f;
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
			tex = gGame.mTpl.mTex;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			u16 firstWidth = GXGetTexObjWidth(&tex), secondWidth = GXGetTexObjWidth(&tex);
			f32 digitWidth  = scale * (1.2f * secondWidth) + (1.0f - enlarged) * (scale * firstWidth);
			int firstHeight = GXGetTexObjHeight(&tex) / 10, secondHeight = GXGetTexObjHeight(&tex) / 10;
			f32 digitHeight = scale * (1.2f * secondHeight) + (1.0f - enlarged) * (scale * firstHeight);
			PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			f32 countX         = xx + 22.0f * scale;
			f32 countY         = yy + 14.0f * scale;
			if (count >= 10) {
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * (count / 10), 0.0f);
				tex.mTexMtx.mDirty = TRUE;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, countX - digitWidth / 3.0f,
				                   countY, 0.0f, digitWidth, digitHeight, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * (count % 10), 0.0f);
				tex.mTexMtx.mDirty = TRUE;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, digitWidth / 3.0f + countX,
				                   countY, 0.0f, digitWidth, digitHeight, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			} else {
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * count, 0.0f);
				tex.mTexMtx.mDirty = TRUE;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, countX, countY, 0.0f, digitWidth,
				                   digitHeight, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
		} else {
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[235], 0);
			tex = gGame.mTpl.mTex;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			u16 tw = GXGetTexObjWidth(&tex), th = GXGetTexObjHeight(&tex);
			DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
			                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, xx, yy, 0.0f, scale * tw, scale * th,
			                   0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
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

void MenuPaneFadePair::Update(u32 frames)
{
	for (s32 i = 0; i < 2; i++) {
		mFades[i].Update(frames);
	}
}

static inline s32 CountMenuItems()
{
	s16 num;
	s16 id;
	s32 count = 0;
	for (int i = 255; i >= 0; i--) {
		InventoryItem* pairs = gGame.GetPlayer()->mParams.mInventory.mItems;
		id                   = pairs[i].mId;
		num                  = pairs[i].mCount;
		if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
			count++;
		}
	}
	return count;
}

// the pointer left a pane
void MenuItem::vf0C(CbPane* pane)
{
	s32 kind = pane->mKind;
	mHovered = 0;
	if (mMode != 2) {
		return;
	}
	if (kind >= 8 && kind <= 9) {
		mPaneFades.mFades[(u32)(kind - 8)].mHovered = 0;
		return;
	}
	if (mSelection == kind && mSelection != -1) {
		mSelection = -1;
	}
}

// the pointer came onto a pane
// NONMATCHING: 98 %, MWCC folds &mParams (0x999C) + mInventory (0x3C) into -0x6628(r3) instead of keeping 0x3C(r3)
void MenuItem::vf10(CbPane* pane)
{
	s32 kind = pane->mKind;
	if (kind >= 0 && kind <= 7 && mSelection != kind) {
		if (CountMenuItems() > 0 && sStatusItemTable.mItems[kind].mItem >= 0) {
			mSelection = kind;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		return;
	}
	if (kind >= 8 && kind <= 9 && mMode == 2) {
		mPaneFades.mFades[(u32)(kind - 8)].mHovered = 1;
	}
}

// the pointer came on screen while both controls work: nothing is picked until it points
void MenuItem::vf18(nw4r::math::VEC2*)
{
	if ((gGame.mMenuControls & MenuControlStick) && (gGame.mMenuControls & MenuControlPointer) && mMode == 2) {
		mSelection = -1;
	}
}

// the pointer went away while both controls work: the stick's row comes back
void MenuItem::vf1C(nw4r::math::VEC2*)
{
	if ((gGame.mMenuControls & MenuControlStick) && (gGame.mMenuControls & MenuControlPointer) && mMode == 2 && mSelection == -1
	    && mStickSelection != -1) {
		mSelection = mStickSelection;
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

// a filled box with a dark and a light edge, the GameCube's DrawBeveledMenuRect; the Wii inlines it
static inline void DrawBeveledMenuRect(u32 color, f32 x, f32 y, f32 z, f32 width, f32 height, f32 alpha)
{
	Vec fill[5]
	    = { (Vec) { MenuRectStart(x, width), MenuRectStart(y, height), z }, (Vec) { MenuRectEnd(x, width), MenuRectStart(y, height), z },
		    (Vec) { MenuRectEnd(x, width), MenuRectEnd(y, height), z }, (Vec) { MenuRectStart(x, width), MenuRectEnd(y, height), z },
		    (Vec) { MenuRectStart(x, width), MenuRectStart(y, height), z } };
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, fill, color, 5);
	Vec border[6] = {
		(Vec) { MenuRectStart(x, width), MenuRectStart(y, height), 0.0f }, (Vec) { MenuRectEnd(x, width), MenuRectStart(y, height), 0.0f },
		(Vec) { MenuRectEnd(x, width), MenuRectEnd(y, height), 0.0f },     (Vec) { MenuRectStart(x, width), MenuRectEnd(y, height), 0.0f },
		(Vec) { MenuRectStart(x, width), MenuRectStart(y, height), 0.0f }, (Vec) { MenuRectEnd(x, width), MenuRectStart(y, height), 0.0f }
	};
	DrawGsLineStrip(&gGame.mGs, &border[3], (u32)(255.0f * alpha) | 0x64646400, 3);
	DrawGsLineStrip(&gGame.mGs, &border[1], (u32)(255.0f * alpha) | 0xC8C8C800, 3);
}

// the status screen draws on the 4:3 middle of a wide screen: the camera and the scissor move in by the
// margin, and the full width comes back afterwards.
// NONMATCHING: 98 %, the int to float constant lands in f5 instead of f4 and the view's registers follow
BOOL MenuStatus::Draw()
{
	if (CMenu::Draw()) {
		gRingScreenWidth    = 640;
		gPointerScreenWidth = 640;
		s32 width           = gGame.mAspectRatio == 0 ? 640 : 854;
		MenuSize size;
		size.height = gPointerScreenHeight;
		size.width  = width;
		f32 margin  = (width - 640) / 2;
		MenuRect view;
		view.top        = 0.0f;
		view.right      = size.width - margin;
		view.left       = -margin;
		view.bottom     = size.height;
		CCamera* camera = ((CBase*)&gGame)->GetScreenCamera();
		camera->vf24(view.top, view.bottom, view.left, view.right);
		((CBase*)&gGame)->vf5C();
		DrawPanelBackground<ItemStatusPanel>(mAlpha);
		u32 scissorWidth = gPointerScreenWidth - (s32)margin * 2;
		GXSetScissor(gScreenLeft + (s32)margin, gScreenTop, scissorWidth, gPointerScreenHeight);
		DrawStatusMenuTabs(this);
		fn_80061014(this);
		DrawPlayerStatusOverview(this);
		fn_80061DBC(this);
		DrawStatusIconGrid(this);
		DrawStatusHelpText(this);
		DrawStatusMenuTitle(this);
		InitGameOrthoCamera(camera, &gGame.mGs.mScreen);
		((CBase*)&gGame)->vf5C();
		width               = gGame.mAspectRatio == 0 ? 640 : 854;
		gRingScreenWidth    = width;
		gPointerScreenWidth = width;
		GXSetScissor(gScreenLeft, gScreenTop, width, gPointerScreenHeight);
		return TRUE;
	}
	return FALSE;
}

// two byte text a character at a time, digits and narrow letters moved to the middle of their cell.
// NONMATCHING: scheduling of the character copy and register allocation of the box corners
extern "C" void DrawDoubleByteMenuText(char* text, f32 x, f32 y, f32 z, f32 w, f32 h, f32 alpha)
{
	char* p;
	u32 count;
	int i;
	count = strlen(text) / 2;
	p     = text;
	i     = 0;
	for (; i < count; p += 2, i++) {
		char c[3];
		char buf[4];
		c[0] = p[0];
		c[1] = p[1];
		c[2] = 0;
		DrawBeveledMenuRect((u32)(128.0f * alpha) | 0xFFFFFF00, i * w + x, y, z, w, h, alpha);
		u32 color = (u32)(255.0f * alpha) | 0x20202000;
		f32 shift = 0.0f;
		for (int digit = 0; digit < 10; digit++) {
			sprintf(buf, "%d", digit);
			ConvertHudTextToFullWidth(buf);
			if (strcmp(c, buf) == 0) {
				if (digit == 1) {
					shift = 6.0f;
				} else {
					shift = 3.0f;
				}
				break;
			}
		}
		for (int letter = 0; letter < 26; letter++) {
			buf[0] = 'A' + letter;
			buf[1] = 0;
			ConvertHudTextToFullWidth(buf);
			if (strcmp(c, buf) == 0) {
				switch (letter) {
				case 'B' - 'A':
				case 'E' - 'A':
				case 'F' - 'A':
				case 'J' - 'A':
				case 'K' - 'A':
				case 'L' - 'A':
				case 'P' - 'A':
				case 'R' - 'A':
				case 'S' - 'A':
				case 'T' - 'A':
					shift = 6.0f;
					break;
				case 'I' - 'A':
					shift = 12.0f;
					break;
				case 'C' - 'A':
				case 'H' - 'A':
				case 'U' - 'A':
				case 'X' - 'A':
					shift = 4.5f;
					break;
				default:
					shift = 3.0f;
					break;
				}
				break;
			}
		}
		DrawGsProportionalTextFormat(&gGame.mGs, color, c, shift + (i * w + x) - w / 2, y - h / 2, 0.0f, w, h);
	}
}

// NONMATCHING: register allocation and order of the shadow's corner coordinates
extern "C" void DrawMenuRectWithShadow(CMenu* menu, u32 color, f32 x, f32 y, f32 width, f32 height)
{
	// The original evaluates both animation curves even though their values are unused.
	sin(4.712389f * (1.0f - menu->mAlpha));
	sin(4.712389f * (1.0f - menu->mAlpha));
	f32 offset      = 6.0f;
	f32 z           = -10.0f;
	Vec outline[10] = { (Vec) { MenuRectStart(x, width), MenuRectStart(y, height), z },
		                (Vec) { MenuRectEnd(x, width), MenuRectStart(y, height), z },
		                (Vec) { MenuRectEnd(x, width), MenuRectEnd(y, height), z },
		                (Vec) { MenuRectStart(x, width), MenuRectEnd(y, height), z },
		                (Vec) { MenuRectStart(x, width), MenuRectStart(y, height), z },
		                (Vec) { offset + MenuRectStart(x, width), offset + MenuRectStart(y, height), z },
		                (Vec) { offset + MenuRectEnd(x, width), offset + MenuRectStart(y, height), z },
		                (Vec) { offset + MenuRectEnd(x, width), offset + MenuRectEnd(y, height), z },
		                (Vec) { offset + MenuRectStart(x, width), offset + MenuRectEnd(y, height), z },
		                (Vec) { offset + MenuRectStart(x, width), offset + MenuRectStart(y, height), z } };
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, outline, color, 5);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_NEQUAL, GX_FALSE);
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

// a status icon's card, the item card without the picture. NONMATCHING: the place of "%s" in the string pool
extern "C" void DrawStatusIconDetails(s32 id, f32 x, f32 y, f32 alpha, f32 scale)
{
	// the original constructs a texture and counts its calls even when the card is closed
	static u32 frame = 0;
	++frame;
	CTexObj tex;
	f32 width, height;
	if (scale <= 0.0f)
		return;
	height         = 153.0f * scale;
	width          = 254.0f * scale;
	f32 top        = y - height / 2.0f;
	u32 frameColor = (u32)(220.0f * alpha);
	DrawTexturedWindowFrame(frameColor, (u32)(255.0f * alpha) | 0xFFFFFF00, x, top, width, height, alpha, 1.0f, 1.0f);
	top += scale;
	f32 fontSize   = 17.0f * scale;
	f32 textOffset = 24.0f * scale;
	char text[256];
	sprintf(text, "%s", GetStatusIconDescription(id));
	width = MeasureGameTextWidth(&gGame.mGs, text, 1, fontSize);
	DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xFFFFFF00, text, x - width / 2.0f,
	                             (4.0f * scale + (textOffset + (5.0f * fontSize + top))) - 6.0f * scale, 0.0f, fontSize, fontSize);
	sprintf(text, "%s", GetStatusIconName(id));
	width = MeasureGameTextWidth(&gGame.mGs, text, 1, 2.0f * scale + fontSize);
	DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xFFAA0000, text, x - width / 2.0f,
	                             (textOffset + (4.0f * fontSize + top)) - 6.0f * scale, 0.0f, 2.0f * scale + fontSize,
	                             2.0f * scale + fontSize);
}

// an overview entry's card. NONMATCHING: the place of "%s" in the string pool
extern "C" void DrawStatusOverviewDetails(s32 id, f32 x, f32 y, f32 alpha, f32 scale)
{
	// the original constructs a texture and counts its calls even when the card is closed
	static u32 frame = 0;
	++frame;
	CTexObj tex;
	f32 width, height;
	if (scale <= 0.0f)
		return;
	height         = 153.0f * scale;
	width          = 254.0f * scale;
	f32 top        = y - height / 2.0f;
	u32 frameColor = (u32)(220.0f * alpha);
	DrawTexturedWindowFrame(frameColor, (u32)(255.0f * alpha) | 0xFFFFFF00, x, top, width, height, alpha, 1.0f, 1.0f);
	top += scale;
	f32 fontSize   = 17.0f * scale;
	f32 textOffset = 24.0f * scale;
	char text[256];
	sprintf(text, "%s", GetStatusOverviewDescription(id));
	width = MeasureGameTextWidth(&gGame.mGs, text, 1, fontSize);
	DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xFFFFFF00, text, x - width / 2.0f,
	                             (4.0f * scale + (textOffset + (5.0f * fontSize + top))) - 6.0f * scale, 0.0f, fontSize, fontSize);
	sprintf(text, "%s", GetStatusOverviewLabel(id));
	width = MeasureGameTextWidth(&gGame.mGs, text, 1, 2.0f * scale + fontSize);
	DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xFFAA0000, text, x - width / 2.0f,
	                             (textOffset + (4.0f * fontSize + top)) - 6.0f * scale, 0.0f, 2.0f * scale + fontSize,
	                             2.0f * scale + fontSize);
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

// NONMATCHING: register allocation; retail keeps the cursor's y in the register of y and loads 1.0 ahead of the
// sub menu's fade
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
		x -= maxW / 2;
		y = y - ((2.0f + lineH) * lines) / 2 + (2.0f + lineH) * mSelection;
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
		DrawMenuSelectionCursor(mCursor, x, y, maxW, lineH, mAlpha, alpha);
		return TRUE;
	}
	return FALSE;
}

void MenuSFTestSub::InitRows()
{
	if (mRows == NULL) {
		mRows = (MenuTextRow*)MEMAllocFromExpHeapEx(gWiiGameHeap, sizeof(MenuTextRow) * 4, 4);
	}
	mFontSize = 16.0f;
	mCount    = 0;
	mMaxWidth = 0.0f;
}

// NONMATCHING: the loop counter and the table pointer swap registers
void MenuSFTestSub::Open()
{
	mState = TRUE;
	Resume();
	mWindow.mOpen = TRUE;
	InitRows();
	for (s32 i = 0; i < 4; ++i) {
		mRows[mCount].mText  = sSFTestSubLines[i].mText;
		mRows[mCount].mWidth = MeasureGameTextWidth(&gGame.mGs, mRows[mCount].mText, 1, mFontSize);
		if (mRows[mCount].mWidth > mMaxWidth) {
			mMaxWidth = mRows[mCount].mWidth;
		}
		mCount++;
	}
	mWindow.mHeight = mCount * (2.0f + mFontSize) + 24.0f;
	mWindow.mWidth  = 60.0f + mMaxWidth;
	mSelection      = 0;
}

// NONMATCHING: retail reads mCount once per step and has count - 1 ready before the loop
BOOL MenuSFTestSub::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		if (gWiiPointer.Check(PadActStickUp)) {
			if (mCount <= 0)
				goto done;
			do {
				if (--mSelection < 0)
					mSelection = mCount - 1;
			} while (mRows[mSelection].IsSeparator());
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			goto done;
		}
		if (gWiiPointer.Check(PadActStickDown)) {
			if (mCount <= 0)
				goto done;
			do {
				if (++mSelection > mCount - 1)
					mSelection = 0;
			} while (mRows[mSelection].IsSeparator());
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			goto done;
		}
		if (gWiiPointer.Check(PadActCancel)) {
			Close(5);
			sMenuSFTest.Resume();
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			goto done;
		}
		if (gWiiPointer.Check(PadActConfirm)) {
			s32 operation = sSFTestSubLines[mSelection].mAction;
			switch (operation) {
			case 4: {
				u32 status = gGame.mSaveManager.mScanEntries[sMenuSFTest.mSelection].mStatus;
				if (!(status & 1) || !(status & 2)) {
					PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
					goto done;
				}
				break;
			}
			case 2: {
				u32 status = gGame.mSaveManager.mScanEntries[sMenuSFTest.mSelection].mStatus;
				if (status & 0x1A) {
					PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
					goto done;
				}
				break;
			}
			case 6: {
				u32 status = gGame.mSaveManager.mScanEntries[sMenuSFTest.mSelection].mStatus;
				if (!(status & 1)) {
					PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
					goto done;
				}
				break;
			}
			}
			Close(5);
			sMenuSFTest.Resume();
			sMenuSFTest.StartOperation(operation);
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
	}

done:
	return IsOpen();
}

// NONMATCHING: retail converts mCount to a float it never uses before the rows, as Err's Draw does, and keeps
// the cursor math in saved float registers
BOOL MenuSFTestSub::Draw()
{
	BOOL draw;
	if (!IsOpen()) {
		draw = FALSE;
	} else {
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
		DrawMenuWindow(&mWindow);
		draw = TRUE;
	}
	if (draw) {
		for (s32 i = 0; i < mCount; ++i) {
			fn_80050314(this, i, TRUE, TRUE);
		}
		f32 rowHeight = 2.0f + mFontSize;
		f32 x         = mWindow.mX - mMaxWidth / 2.0f;
		f32 y         = mWindow.mY - rowHeight * mCount / 2.0f + rowHeight * mSelection;
		DrawMenuSelectionCursor(mCursor, x, y, mMaxWidth, mFontSize, mAlpha, 1.0f);
		return TRUE;
	}
	return FALSE;
}

void MenuSFTestErr::InitRows()
{
	if (mRows == NULL) {
		mRows = (MenuTextRow*)MEMAllocFromExpHeapEx(gWiiGameHeap, sizeof(MenuTextRow), 4);
	}
	mFontSize = 16.0f;
	mCount    = 0;
	mMaxWidth = 0.0f;
}

void MenuSFTestErr::Open()
{
	mState = TRUE;
	Resume();
	mWindow.mOpen = TRUE;
	InitRows();
	mRows[mCount].mText  = (char*)GetNandGameResultName(&gGame.mSaveManager.mFile, gGame.mSaveManager.mFile.mResult);
	mRows[mCount].mWidth = MeasureGameTextWidth(&gGame.mGs, mRows[mCount].mText, 1, mFontSize);
	if (mRows[mCount].mWidth > mMaxWidth) {
		mMaxWidth = mRows[mCount].mWidth;
	}
	mCount++;
	mWindow.mHeight = mCount * (2.0f + mFontSize) + 24.0f;
	mWindow.mWidth  = 60.0f + mMaxWidth;
	mSelection      = 0;
}

BOOL MenuSFTestErr::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		if (gWiiPointer.Check(PadActCancel) || gWiiPointer.Check(PadActConfirm)) {
			gGame.mSaveManager.mFile.mResult = 0;
			Close(5);
			sMenuSFTest.Resume();
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		}
	}
	return IsOpen();
}

// NONMATCHING: retail converts mCount to a float it never uses before the rows, likely a dead row-height
// computation of an inline the row base shares
BOOL MenuSFTestErr::Draw()
{
	BOOL draw;
	if (!IsOpen()) {
		draw = FALSE;
	} else {
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
		DrawMenuWindow(&mWindow);
		draw = TRUE;
	}
	if (draw) {
		for (s32 i = 0; i < mCount; ++i) {
			fn_80050314(this, i, TRUE, TRUE);
		}
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

// places a pane where its layout entry says
extern "C" void fn_800556B0(CbPane* pane, const CbPaneLayout* layout)
{
	pane->mTrans.x = layout->mTrans.x;
	pane->mTrans.y = layout->mTrans.y;
	pane->mTrans.z = layout->mTrans.z;
	pane->mWidth   = layout->mWidth;
	pane->mHeight  = layout->mHeight;
	pane->mKind    = layout->mKind;
}

// the pane's rectangle round its centre, y up
extern "C" MenuRect fn_800556E4(const CbPane* pane)
{
	MenuRect rect;
	nw4r::math::VEC2 origin;
	origin.x    = -pane->mWidth / 2.0f;
	origin.y    = pane->mHeight / 2.0f;
	rect.left   = origin.x;
	rect.top    = origin.y;
	rect.right  = origin.x + pane->mWidth;
	rect.bottom = origin.y - pane->mHeight;
	return rect;
}

// the pointer at x, y is on the pane, tested like an nw4r::lyt pane: through the inverse of its matrix.
// NONMATCHING: our build expands fn_800556E4 here, retail calls it
extern "C" bool fn_80055A7C(CbPaneList* list, CbPane* pane, f32 x, f32 y)
{
	if (!pane->mEnabled) {
		return false;
	}
	nw4r::math::MTX34 mtx;
	PSMTXIdentity(mtx.m);
	mtx.m[0][3]   = pane->mTrans.x;
	mtx.m[1][3]   = pane->mTrans.y;
	mtx.m[2][3]   = pane->mTrans.z;
	pane->mGlbMtx = mtx;
	Mtx inverse;
	PSMTXInverse(pane->mGlbMtx.m, inverse);
	Vec pos = { x, y, 0.0f };
	Vec local;
	PSMTXMultVec(inverse, &pos, &local);
	MenuRect rect = fn_800556E4(pane);
	return rect.left <= local.x && local.x <= rect.right && rect.bottom <= local.y && local.y <= rect.top;
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

void MenuPC::Init()
{
	CMenu::Init();
	mDepositBalance = 0;
	mDialogueNumber = 0;
}

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
	f32 rate = __fabsf((target - value) / 4.0f);
	if (rate < 1.0f)
		rate = 1.0f;
	PCSlide(value, target, rate);
}

// the GameCube's PCEaseCall: a quarter of the way, at least 1 a frame. The float fabs is nw4r::math::FAbs's, as in
// fbeffect.cpp; its header needs the Wii MSL's <cmath>, which the game's include path does not have yet
static inline void PCEaseCall(f32& value, f32& target)
{
	f32 rate = __fabsf((target - value) / 4.0f);
	if (rate <= 1.0f) {
		rate = 1.0f;
	}
	ApproachHudValue(&value, &target, rate);
}

static inline XObj* PCWorldObject(s32 index)
{
	return gGame.mXObjs.mPools[22].Get(index);
}

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

static void ResetPCCharacters(MenuPC* menu);

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
	if (mShopOnly) {
		ResetPCText(this, 3);
	} else {
		ResetPCText(this, 1);
		mProgress       = 0.0f;
		mTargetProgress = 1.0f;
		switch (rand() % 2) {
		case 0:
			mSpeed = 0.04f;
			break;
		case 1:
			mSpeed = 0.08f;
			break;
		}
	}
}

void MenuPC::State1(u32 frames)
{
	switch (mPhase) {
	case 0:
		PlayWiiSoundEffect(&gGame.mSound, 0x35A, 1.0f, 0.0f);
		mPhase++;
		break;
	case 1:
		break;
	}
	ApproachHudValue(&mProgress, &mTargetProgress, mSpeed * frames);
	if (mProgress >= 1.0f)
		ResetPCText(this, 2);
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
	CPad pad(0);
	if (gWiiPointer.Check(PadActStickUp)) {
		mChoice--;
		if (mChoice < 0)
			mChoice = mChoiceCount - 1;
		PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
	} else if (gWiiPointer.Check(PadActStickDown)) {
		mChoice++;
		if (mChoice > mChoiceCount - 1)
			mChoice = 0;
		PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
	} else if (gWiiPointer.Check(PadActConfirm)) {
		s32 step = mChoices[mChoice];
		// the catalog (mode 7) only buzzes while it has nothing in it
		if (step == 7) {
			if (mCanSelect) {
				ResetPCText(this, 7);
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			} else {
				PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
			}
		} else if (step >= 0) {
			ResetPCText(this, step);
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
	} else if (gWiiPointer.Check(PadActCancel)) {
		// B: shrink the frame, mode 11 closes the pc once it is shut
		ResetPCText(this, 11);
		mTargetFrameHeight = 0.0f;
		RestorePCHistory(this);
	}
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
			return;
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

// a state the status menu never enters. NONMATCHING: the messages' offsets in the string pool, as in MenuSave::State6
void MenuStatus::State3(u32)
{
	OSPanic("menu_status.cpp", 1313, "UpdateItem");
}

// the status and PC menus' empty states
void MenuStatus::State7(u32)
{
}

void MenuStatus::State6(u32)
{
}

void MenuStatus::vf0C(CbPane* pane)
{
	s32 kind = pane->mKind;
	mHovered = 0;
	switch (mMode) {
	case 1:
		if (mSelection == kind && mSelection != -1) {
			mSelection = -1;
		}
		break;
	case 2:
		if (kind < 49 || kind > 50) {
			if (mItemSelection == kind - 4 && mItemSelection != -1) {
				mItemSelection = -1;
			}
		}
		break;
	case 9:
		if (mOverviewSelection == kind - 12 && mOverviewSelection != -1) {
			mOverviewSelection = -1;
		}
		break;
	case 8:
		if (mIconSelection == kind - 25 && mIconSelection != -1) {
			mIconSelection = -1;
		}
		break;
	}
	if (kind >= 49 && kind <= 50 && mMode == 2 && mPhase == 0) {
		mPaneFades.mFades[(u32)(kind - 49)].mHovered = 0;
	}
}

// NONMATCHING: MWCC folds &mParams (0x999C) + mInventory (0x3C) into -0x6628(r3), as in MenuItem::vf10
void MenuStatus::vf10(CbPane* pane)
{
	s32 kind = pane->mKind;
	if (kind >= 25 && kind <= 48) {
		if (mIconSelection != kind - 25) {
			mIconSelection = kind - 25;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		return;
	}
	if (kind >= 12 && kind <= 24) {
		if (mOverviewSelection != kind - 12) {
			mOverviewSelection = kind - 12;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		return;
	}
	if (kind >= 4 && kind <= 11 && mItemSelection != kind - 4) {
		if (CountMenuItems() > 0) {
			s32 slot = kind - 4;
			if (sStatusItemTable.mItems[slot].mItem >= 0) {
				mItemSelection = slot;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		}
		return;
	}
	switch (kind) {
	case 0:
		if (mSelection != 0) {
			mSelection = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		break;
	case 1:
		if (mSelection != 1) {
			mSelection = 1;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		break;
	case 2:
		if (mSelection != 2) {
			mSelection = 2;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		break;
	case 3:
		if (mSelection != 3) {
			mSelection = 3;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		break;
	}
	if (kind >= 49 && kind <= 50 && mMode == 2 && mPhase == 0) {
		mPaneFades.mFades[(u32)(kind - 49)].mHovered = 1;
	}
}

void MenuStatus::vf18(nw4r::math::VEC2*)
{
	ClearSelections();
}

void MenuStatus::ClearSelections()
{
	u32 controls = gGame.mMenuControls;
	if (!(controls & MenuControlStick) || !(controls & MenuControlPointer)) {
		return;
	}
	if (mMode == 1 && mPhase == 0) {
		mSelection = -1;
	}
	if (mMode == 2 && mPhase == 0) {
		mItemSelection = -1;
	}
	if (mMode == 9 && mPhase == 0) {
		mOverviewSelection = -1;
	}
	if (mMode == 8 && mPhase == 0) {
		mIconSelection = -1;
	}
}

void MenuStatus::vf1C(nw4r::math::VEC2*)
{
	RestoreSelections();
}

void MenuStatus::RestoreSelections()
{
	u32 controls = gGame.mMenuControls;
	if (!(controls & MenuControlStick) || !(controls & MenuControlPointer)) {
		return;
	}
	if (mMode == 1 && mPhase == 0 && mSelection == -1 && mStickSelection != -1) {
		mSelection = mStickSelection;
	}
	if (mMode == 2 && mPhase == 0 && mItemSelection == -1 && mStickItemSelection != -1) {
		mItemSelection = mStickItemSelection;
	}
	if (mMode == 9 && mPhase == 0 && mOverviewSelection == -1 && mStickOverviewSelection != -1) {
		mOverviewSelection = mStickOverviewSelection;
	}
	if (mMode == 8 && mPhase == 0 && mIconSelection == -1 && mStickIconSelection != -1) {
		mIconSelection = mStickIconSelection;
	}
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
	s32 money;
	s32 scrap = GetScrap();
	scrap -= menu->mAmount;
	gGame.GetPlayer()->mParams.SetScrap(scrap);
	money = GetMoney();
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

static inline void DrawJapaneseCardLine(f32 x, s32 id, f32 y, f32 size, u32 color)
{
	const char* text = GetLocalizedInterfaceText(id);
	f32 width        = MeasureGameTextWidth(&gGame.mGs, text, TRUE, size);
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - width * 0.5f, y, 0.0f, size, size);
}

static inline void SetJapaneseSaveFatalText(const s32* ids, int count)
{
	strcpy(gSaveFatal.mText, GetLocalizedInterfaceText(ids[0]));
	for (int i = 1; i < count; i++) {
		strcat(gSaveFatal.mText, "\n");
		strcat(gSaveFatal.mText, GetLocalizedInterfaceText(ids[i]));
	}
	gSaveFatal.mMessage = gSaveFatal.mText;
}

// NONMATCHING: the error cases and call list match; 96 bytes of register allocation and fatal-text
// scheduling remain
extern "C" s32 DrawJapaneseCardErrorMessage(s32, s32 error, u8, f32, f32 y, f32 size, f32 alpha)
{
	s32 lines   = 0;
	u32 color   = (u32)(255.0f * alpha);
	f32 centerX = (f32)(gPointerScreenWidth / 2);
	switch (error) {
	case CARD_RESULT_FATAL_ERROR:
	case -64:
	case -15:
	case CARD_RESULT_CANCELED:
	case CARD_RESULT_ENCODING:
	case CARD_RESULT_NAMETOOLONG:
	case CARD_RESULT_NOPERM:
	case CARD_RESULT_NOENT:
	case CARD_RESULT_BROKEN:
	case CARD_RESULT_IOERROR:
	case CARD_RESULT_BUSY:
		if (gGame.mLanguage == 0) {
			DrawJapaneseCardLine(centerX, 0x127, y, size, color);
			DrawJapaneseCardLine(centerX, 0x128, y + size, size, color);
			DrawJapaneseCardLine(centerX, 0x129, 2.0f * size + y, size, color);
			s32 ids[4] = { 0x127, 0x128, 0x129 };
			SetJapaneseSaveFatalText(ids, 3);
			lines = 3;
		} else {
			DrawJapaneseCardLine(centerX, 0xCC, y, size, color);
			DrawJapaneseCardLine(centerX, 0xCD, y + size, size, color);
			DrawJapaneseCardLine(centerX, 0xCE, 2.0f * size + y, size, color);
			lines = 3;
		}
		break;
	case CARD_RESULT_WRONGDEVICE:
	case CARD_RESULT_NOCARD: {
		DrawJapaneseCardLine(centerX, 0x123, y, size, color);
		DrawJapaneseCardLine(centerX, 0x124, y + size, size, color);
		DrawJapaneseCardLine(centerX, 0x125, 2.0f * size + y, size, color);
		DrawJapaneseCardLine(centerX, 0x126, 3.0f * size + y, size, color);
		s32 ids[] = { 0x123, 0x124, 0x125, 0x126 };
		SetJapaneseSaveFatalText(ids, 4);
		lines = 4;
		break;
	}
	case CARD_RESULT_NOFILE: {
		DrawJapaneseCardLine(centerX, 0x10D, y, size, color);
		DrawJapaneseCardLine(centerX, 0x10E, y + size, size, color);
		s32 ids[] = { 0x10D, 0x10E };
		SetJapaneseSaveFatalText(ids, 2);
		lines = 2;
		break;
	}
	case CARD_RESULT_LIMIT:
		DrawJapaneseCardLine(centerX, 0x113, y, size, color);
		DrawJapaneseCardLine(centerX, 0x114, y + size, size, color);
		DrawJapaneseCardLine(centerX, 0x115, 2.0f * size + y, size, color);
		DrawJapaneseCardLine(centerX, 0x116, 3.0f * size + y, size, color);
		lines = 4;
		break;
	case CARD_RESULT_INSSPACE:
		DrawJapaneseCardLine(centerX, 0x10F, y, size, color);
		DrawJapaneseCardLine(centerX, 0x110, y + size, size, color);
		DrawJapaneseCardLine(centerX, 0x111, 2.0f * size + y, size, color);
		DrawJapaneseCardLine(centerX, 0x112, 3.0f * size + y, size, color);
		lines = 4;
		break;
	}
	return lines;
}

// the save menu reserves extra space below some prompts for the choice labels
// NONMATCHING: the switch layout and call list match; 160 bytes of register allocation and fatal-text
// loop scheduling remain, including a _savegpr_27/_restgpr_27 pair absent from retail
extern "C" s32 DrawJapaneseSaveStatusMessage(s32, s32 status, f32, f32 y, f32 size, f32 alpha)
{
	s32 lines   = 0;
	u32 color   = (u32)(255.0f * alpha);
	f32 centerX = (f32)(gPointerScreenWidth / 2);
	switch (status) {
	case 1:
		DrawJapaneseCardLine(centerX, 0x117, y, size, color);
		DrawJapaneseCardLine(centerX, 0x118, y + size, size, color);
		lines = 2;
		break;
	case 3:
		if (gGame.mLanguage == 2 || gGame.mLanguage == 3) {
			DrawJapaneseCardLine(centerX, 0xBD, y, size, color);
			DrawJapaneseCardLine(centerX, 0xBE, y + size, size, color);
			lines = 2;
		} else {
			DrawJapaneseCardLine(centerX, 0xBD, y, size, color);
			lines = 1;
		}
		break;
	case 8:
		if (gGame.mLanguage == 0) {
			DrawJapaneseCardLine(centerX, 0x86, y, size, color);
			lines = 1;
		} else {
			DrawJapaneseCardLine(centerX, 0x86, y, size, color);
			DrawJapaneseCardLine(centerX, 0x87, y + size, size, color);
			lines = 2;
		}
		break;
	case 19:
		if (gGame.mLanguage == 2) {
			DrawJapaneseCardLine(centerX, 0xF4, y, size, color);
			DrawJapaneseCardLine(centerX, 0xF5, y + size, size, color);
			lines = 2;
		} else {
			DrawJapaneseCardLine(centerX, 0xF4, y, size, color);
			lines = 1;
		}
		break;
	case 4:
		if (gGame.mLanguage == 2) {
			DrawJapaneseCardLine(centerX, 0xB8, y, size, color);
			DrawJapaneseCardLine(centerX, 0xB9, y + size, size, color);
			DrawJapaneseCardLine(centerX, 0xBA, 2.0f * size + y, size, color);
			DrawJapaneseCardLine(centerX, 0xBB, 3.0f * size + y, size, color);
			lines = 4;
		} else {
			DrawJapaneseCardLine(centerX, 0xB8, y, size, color);
			DrawJapaneseCardLine(centerX, 0xB9, y + size, size, color);
			lines = 2;
		}
		break;
	case 5:
		DrawJapaneseCardLine(centerX, 0x77, y, size, color);
		DrawJapaneseCardLine(centerX, 0x78, y + size, size, color);
		DrawJapaneseCardLine(centerX, 0x79, 2.0f * size + y, size, color);
		lines = 4;
		break;
	case 6:
		if (gGame.mLanguage == 2) {
			DrawJapaneseCardLine(centerX, 0x7C, y, size, color);
			DrawJapaneseCardLine(centerX, 0x7D, y + size, size, color);
			DrawJapaneseCardLine(centerX, 0x7E, 2.0f * size + y, size, color);
			DrawJapaneseCardLine(centerX, 0x7F, 3.0f * size + y, size, color);
			lines = 4;
		} else if (gGame.mLanguage == 2 || gGame.mLanguage == 3) {
			DrawJapaneseCardLine(centerX, 0x7C, y, size, color);
			DrawJapaneseCardLine(centerX, 0x7D, y + size, size, color);
			DrawJapaneseCardLine(centerX, 0x7E, 2.0f * size + y, size, color);
			lines = 3;
		} else {
			DrawJapaneseCardLine(centerX, 0x7C, y, size, color);
			DrawJapaneseCardLine(centerX, 0x7D, y + size, size, color);
			lines = 2;
		}
		break;
	case 7:
		DrawJapaneseCardLine(centerX, 0x81, y, size, color);
		DrawJapaneseCardLine(centerX, 0x82, y + size, size, color);
		DrawJapaneseCardLine(centerX, 0x83, 2.0f * size + y, size, color);
		lines = 3;
		break;
	case 9:
		if (gGame.mLanguage == 0) {
			DrawJapaneseCardLine(centerX, 0x8B, y, size, color);
			lines = 1;
		} else {
			DrawJapaneseCardLine(centerX, 0x8B, y, size, color);
			DrawJapaneseCardLine(centerX, 0x8C, y + size, size, color);
			lines = 2;
		}
		break;
	case 10:
		DrawJapaneseCardLine(centerX, 0x119, y, size, color);
		DrawJapaneseCardLine(centerX, 0x11A, y + size, size, color);
		lines = 3;
		break;
	case 11:
		DrawJapaneseCardLine(centerX, 0x11B, y, size, color);
		lines = 2;
		break;
	case 12:
		DrawJapaneseCardLine(centerX, 0xD1, y, size, color);
		DrawJapaneseCardLine(centerX, 0xD2, y + size, size, color);
		lines = 3;
		break;
	case 13: {
		DrawJapaneseCardLine(centerX, 0x10D, y, size, color);
		DrawJapaneseCardLine(centerX, 0x10E, y + size, size, color);
		s32 ids[] = { 0x10D, 0x10E };
		SetJapaneseSaveFatalText(ids, 2);
		lines = 2;
		break;
	}
	case 14: {
		OSPanic("menu_save.cpp", 3883, "_CARDWARN_UNUSABLE_MAXBLOCKS_-->_CARDWARN_UNUSABLE_UNKOWN_へ以降したはず");
		DrawJapaneseCardLine(centerX, 0x11D, y, size, color);
		DrawJapaneseCardLine(centerX, 0x11E, y + size, size, color);
		s32 ids[] = { 0x11D, 0x11E };
		SetJapaneseSaveFatalText(ids, 2);
		lines = 2;
		break;
	}
	case 15: {
		OSPanic("menu_save.cpp", 3890, "_CARDWARN_UNUSABLE_MAXFILES_-->_CARDWARN_UNUSABLE_UNKOWN_へ以降したはず");
		DrawJapaneseCardLine(centerX, 0x120, y, size, color);
		DrawJapaneseCardLine(centerX, 0x121, y + size, size, color);
		s32 ids[] = { 0x120, 0x121 };
		SetJapaneseSaveFatalText(ids, 2);
		lines = 2;
		break;
	}
	case 16: {
		DrawJapaneseCardLine(centerX, 0x123, y, size, color);
		DrawJapaneseCardLine(centerX, 0x124, y + size, size, color);
		DrawJapaneseCardLine(centerX, 0x125, 2.0f * size + y, size, color);
		DrawJapaneseCardLine(centerX, 0x126, 3.0f * size + y, size, color);
		s32 ids[] = { 0x123, 0x124, 0x125, 0x126 };
		SetJapaneseSaveFatalText(ids, 4);
		lines = 4;
		break;
	}
	case 17: {
		DrawJapaneseCardLine(centerX, 0x127, y, size, color);
		DrawJapaneseCardLine(centerX, 0x128, y + size, size, color);
		DrawJapaneseCardLine(centerX, 0x129, 2.0f * size + y, size, color);
		s32 ids[4] = { 0x127, 0x128, 0x129 };
		SetJapaneseSaveFatalText(ids, 3);
		lines = 3;
		break;
	}
	case 18:
		DrawJapaneseCardLine(centerX, 0xE0, y, size, color);
		DrawJapaneseCardLine(centerX, 0xE1, y + size, size, color);
		DrawJapaneseCardLine(centerX, 0xE2, 2.0f * size + y, size, color);
		DrawJapaneseCardLine(centerX, 0xE3, 3.0f * size + y, size, color);
		lines = 4;
		break;
	}
	return lines;
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

// the save menu's pointer events, the slots of its CContainCallback base
void MenuSave::vf08(CbPane*)
{
}

// the pointer left a pane
void MenuSave::vf0C(CbPane* pane)
{
	switch (pane->mKind) {
	case 0:
	case 1:
	case 2:
		if (mFile == pane->mKind && mFile != -1) {
			mFile = -1;
		}
		break;
	case 3:
	case 4:
		if (mMode == 8 || mMode == 3 || mMode == 18 || mMode == 19) {
			if (mChoice == pane->mKind - 3 && mChoice != -1) {
				mChoice = -1;
			}
		}
		break;
	}
}

// the pointer came onto a pane
void MenuSave::vf10(CbPane* pane)
{
	switch (pane->mKind) {
	case 0:
		if (mFile != 0) {
			mFile = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		break;
	case 1:
		if (mFile != 1) {
			mFile = 1;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		break;
	case 2:
		if (mFile != 2) {
			mFile = 2;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		break;
	}
	if (mMode == 8 || mMode == 3 || mMode == 18 || mMode == 19) {
		switch (pane->mKind) {
		case 3:
			if (mChoice != 0) {
				mChoice = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			break;
		case 4:
			if (mChoice != 1) {
				mChoice = 1;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			break;
		}
	}
}

// the pointer came on screen while both controls work: nothing is picked until it points
void MenuSave::vf18(nw4r::math::VEC2*)
{
	if ((gGame.mMenuControls & MenuControlStick) && (gGame.mMenuControls & MenuControlPointer)) {
		if (mMode == 2) {
			mFile = -1;
		}
		if (mMode == 8 || mMode == 3 || mMode == 18 || mMode == 19) {
			mChoice = -1;
		}
	}
}

// the pointer went away while both controls work: the stick's file and choice come back
void MenuSave::vf1C(nw4r::math::VEC2*)
{
	if ((gGame.mMenuControls & MenuControlStick) && (gGame.mMenuControls & MenuControlPointer)) {
		if (mMode == 2 && mFile == -1 && mStickFile != -1) {
			mFile = mStickFile;
		}
		if ((mMode == 8 || mMode == 3 || mMode == 18 || mMode == 19) && mChoice == -1 && mStickChoice != -1) {
			mChoice = mStickChoice;
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

// how many of the item chibi robo holds, the GameCube's. NONMATCHING: 91 %, retail hoists addis r5, r4, 1 before the loop instead of
// folding addi r5, r4, 0x3fc
extern "C" s16 GetPlayerItemCount(s16 id)
{
	s16 count = 0;
	for (int i = 255; i >= 0; i--) {
		if (id == gGame.GetPlayer()->mParams.mInventory.mItems[i].mId) {
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

extern "C" void InitGameMenus(MenuEx* ex)
{
	ex->mActive   = FALSE;
	ex->mMenus[0] = &sMenuMain;
	ex->mMenus[5] = &sMenuStageSelect;
	ex->mMenus[6] = &sMenuSFTest;
	ex->mMenus[7] = &sMenuSFTestSub;
	ex->mMenus[8] = &sMenuSFTestErr;
	ex->mMenus[1] = &sMenuStatus;
	ex->mMenus[2] = &sMenuItem;
	ex->mMenus[3] = &sMenuPC;
	ex->mMenus[4] = &sMenuSave;
	for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
		if (ex->mMenus[i]) {
			ex->mMenus[i]->Init();
		}
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

extern "C" void fn_8005575C(CbPaneList* list, bool pointerOn, nw4r::math::VEC2* pos, u32 frames);

// UpdateOpenMenus: steps the open menus, lets the game go on once the last one closes, blinks the cursor and
// tests the pointer against the menus' panes.
// NONMATCHING: 80 %, the frame is retail's; the screen size's int to float conversions are scheduled apart
// in retail and the rectangle's registers differ
extern "C" void fn_8008492C(MenuEx* ex, u32 frames)
{
	BOOL active = FALSE;
	for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
		if (ex->mMenus[i] && ex->mMenus[i]->IsOpen() && ex->mMenus[i]->Update(frames)) {
			active = TRUE;
		}
	}
	if (ex->mActive && !active) {
		// _34 is the screen capture behind the menus, a WiiFbTexture to the end of MenuEx
		FreeFbTexture((WiiFbTexture*)&ex->_34);
		ex->mActive = FALSE;
		ResumeGameSounds(&gGame, 0.2f);
	}
	ex->mCursor.mTimer += frames;
	if (ex->mCursor.mTimer > 3) {
		ex->mCursor.mTimer = 0;
		ex->mCursor.mFrame = (ex->mCursor.mFrame + 1) % 8;
	}

	nw4r::math::VEC2 pos;
	MenuSize size;
	size.height = gPointerScreenHeight;
	size.width  = gPointerScreenWidth;
	MenuRect screen;
	screen.left   = -size.width / 2.0f;
	screen.top    = size.height / 2.0f;
	screen.right  = size.width / 2.0f;
	screen.bottom = -size.height / 2.0f;
	Vec pointer;
	pointer.x      = gWiiPointer.mPointer.x;
	pointer.y      = gWiiPointer.mPointer.y;
	pointer.z      = gWiiPointer.mPointer.z;
	f32 halfHeight = 0.5f * (screen.bottom - screen.top);
	f32 halfWidth  = 0.5f * (screen.right - screen.left);
	pos.x          = pointer.x - halfWidth;
	pos.y          = -(pointer.y + halfHeight);
	fn_8005575C(&gPointerPanes, gWiiPointer.mPointerValid != 0, &pos, frames);
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

void CMenu::vf28()
{
}

void CMenu::vf2C()
{
}

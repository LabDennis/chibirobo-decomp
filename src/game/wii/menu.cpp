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
extern s32 gSoundFadeFrames;
extern MEMiHeapHead* gWiiGameHeap;
struct MenuTextColors {
	void* mText;
	CColor mColor;
	CColor mShadowColor;
	bool mActive;
};
extern MenuTextColors lbl_8044B700;

extern s32 gScreenSafeLeft, gScreenSafeTop;
extern s32 gScreenLeft, gScreenTop;
extern "C" {
char* GetStageFileName(s32 stage);
char* GetStageName(s32 stage);
const char* GetStageNameOrFileName(s32 stage);
char* GetStageDesc(s32 stage);
char* FormatToStaticBuffer(const char* fmt, ...);
void ApproachHudValue(f32* value, f32* target, f32 step);
void ResumeGameSounds(CGame* game, f32 fadeSeconds);
void PauseGameSounds(CGame* game, f32 fadeSeconds);
void ChangeGameStage(CGame* game, s32 stage, BOOL showName);
u8* GetItemDescription(s32 id);
u8* GetAttachmentDescription(s32 id);
s32 GetItemIconTextureIndex(s32 id);
u8* GetStatusIconDescription(s32 id);
u8* GetUtilibotName(s32 id);
u8* GetUtilibotDescription(s32 id);
s32 ConvertHudTextToFullWidth(char* text);
u8* GetStatusOverviewDescription(s32 id);
char* GetStatusOverviewLabel(s32 id);
void DrawStatusMenuTabs(MenuStatus* menu);
void fn_80061014(MenuStatus* menu);
void fn_80061FF4(MenuStatus* menu, const char* text, u32 color);
void fn_80062338(MenuStatus* menu, u32 frames);
void MarkAvailableStatusEntries(u8* available);
void PopulateItemShopPage(MenuPCPage* list);
void PopulateScrapShopPage(MenuPCPage* list);
s16 GetAttachmentShopPrice(s32 id);
s16 GetItemShopPrice(s32 id);
s16 GetUtilibotPrice(s32 id);
s32 GetAttachmentCountById(AttachmentValues* values, s32 id);
CSoundHandle* StartSound(CSound* sound, u32 id, f32 volume, s32 fadeFrames);
void DrawPlayerStatusOverview(MenuStatus* menu);
void fn_80061DBC(MenuStatus* menu);
void fn_800695EC(MenuStatus* menu);
void DrawStatusIconGrid(MenuStatus* menu);
void DrawStatusHelpText(MenuStatus* menu);
void DrawStatusMenuTitle(MenuStatus* menu);
void DrawPCBlueNumber(bool icon, f32 x, f32 y, f32 alpha, f32 scale, f32 value);
void DrawPCGoldNumber(bool icon, f32 x, f32 y, f32 alpha, f32 scale, f32 value);
void DrawPCRankings(MenuPC* menu, f32 x, f32 y, f32 alpha, f32 scale);
u8* GetPCRankingName(s32 index);
void DrawSaveSlotDetails(WiiMenuSaveSlot* slot, u8 emptyMessage, s16 index, u8 selected, u8 newest, f32 x, f32 y, f32 width, f32 height,
                         f32 alpha, f32 fileAlpha, f32 sx, f32 sy);
void DrawSaveHelpText(MenuSave* menu);
void DrawSaveDetailText(const char* text, f32 x, f32 y, f32 size, f32 alpha);
extern "C" void DrawSavePlayTime(s32 icon, s32 value, u8 selected, f32 x, f32 y, f32 alpha, f32 offsetScale, f32 sy);
void DrawStatusIconDetails(s32 icon, f32 x, f32 y, f32 alpha, f32 scale);
void DrawStatusOverviewDetails(s32 id, f32 x, f32 y, f32 alpha, f32 scale);
void DrawPlayerStatusValue(s32 icon, s32 value, bool selected, f32 x, f32 y, f32 alpha, f32 phase);
void DrawPlayerStatusIcon(s32 id, u8 available, bool selected, f32 x, f32 y, f32 alpha, f32 scale, f32 phase);
void DrawStatusGauge(f32 value, f32 x, f32 y, f32 alpha);
void DrawSaveMenuTitle(MenuSave* menu);
s32 DrawJapaneseSaveStatusMessage(s32 channel, s32 id, f32 x, f32 y, f32 size, f32 alpha);
s32 DrawJapaneseCardErrorMessage(s32 channel, s32 result, u8 retry, f32 x, f32 y, f32 size, f32 alpha);
void DrawMenuBannerPanels(const char* title, f32 x, f32 y, f32 alpha, f32 slide);
void DrawDoubleByteMenuText(char* text, f32 x, f32 y, f32 z, f32 w, f32 h, f32 alpha);
void OSFatal(GXColor fg, GXColor bg, const char* msg);
void fn_8005A128(MenuPaneFadePair* fades, f32 x, f32 y, f32 alpha);
void fn_8005A68C(MenuItem* menu, const char* text, u32 color);
void fn_800556B0(CbPane* pane, const CbPaneLayout* layout);
GXColor fn_8016A96C(u8* from, u8* to, f32 t);
void fn_8007D734(s32 index, u8 selected, u8 newest, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 fileAlpha, f32 sx, f32 sy);
void fn_8007ECC4(s32 icon, s32 hours, s32 minutes, s32 selected, f32 x, f32 y, f32 alpha, f32 sx, f32 sy);
CCamera* InitGameOrthoCamera(CCamera* camera, const GXRenderModeObj* rmode);
void DrawTexturedWindowFrame(u32 color, u32 textureColor, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 scaleX, f32 scaleY);
}
void ClearStatusTable(MenuStatusTable* table);
void ClearItemShopPage(MenuPCPage* page);
void ClearScrapShopPage(MenuPCPage* page);

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
MenuPCPage gItemShopPage;
MenuPCPage gScrapShopPage;
u8 gStatusIconUnlocked[24];
u8 gStatusEntryAvailable[16];
CColor lbl_805A1BC8(255, 255, 255, 255);
// shared item/status pulse phase at 0x805A1BD0: Open resets it, Update advances it, DrawInventoryGrid reads it
static f32 sInventoryPhase;
// the status menu's pulse phase at 0x805A1BD4: MenuStatus::Open, Update and the State functions write it, all still in asm
static f32 sStatusPhase;

// the stages the stage select offers, in its order
static s32 sStageOrder[] = { 0, 5, 7, 1, 2, 4, 6, 0x12, 3, 9, 0x10, 0xB, 0x16, 0xD, 0xE, 0xA, -1 };

extern "C" s32 gPCLevelDialogueIds[11] = { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
extern "C" f32 gSaveSlotSpacing;
extern "C" f32 gItemHelpOutlineX[4]    = { -1.0f, 1.0f, 1.0f, -1.0f };
extern "C" f32 gItemHelpOutlineY[4]    = { -1.0f, -1.0f, 1.0f, 1.0f };
extern "C" f32 gStatusTitleOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
extern "C" f32 gStatusTitleOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
extern "C" f32 gSaveTitleOutlineX[4]   = { -1.0f, 1.0f, 1.0f, -1.0f };
extern "C" f32 gSaveTitleOutlineY[4]   = { -1.0f, -1.0f, 1.0f, 1.0f };
extern "C" f32 gItemTitleOutlineX[4]   = { -1.0f, 1.0f, 1.0f, -1.0f };
extern "C" f32 gItemTitleOutlineY[4]   = { -1.0f, -1.0f, 1.0f, 1.0f };
extern "C" f32 gStatusHelpOutlineX[4]  = { -1.0f, 1.0f, 1.0f, -1.0f };
extern "C" f32 gStatusHelpOutlineY[4]  = { -1.0f, -1.0f, 1.0f, 1.0f };
extern "C" f32 gSaveHelpOutlineX[4]    = { -1.0f, 1.0f, 1.0f, -1.0f };
extern "C" f32 gSaveHelpOutlineY[4]    = { -1.0f, -1.0f, 1.0f, 1.0f };

extern "C" s32 gStatusGridTextures[48] = {
	195, 196, 225, 226, 191, 192, 211, 212, 203, 204, 201, 202, 189, 190, 217, 218, 205, 206, 187, 188, 185, 186, 209, 210,
	221, 222, 183, 184, 229, 230, 227, 228, 199, 200, 207, 208, 197, 198, 215, 216, 193, 194, 213, 214, 219, 220, 223, 224,
};

struct PCChoiceArt {
	s32 icon, label;
};

static const PCChoiceArt pcChoiceArt[] = { { 0, 0 }, { 0, 0 },    { 0, 0 }, { 147, 2 }, { 146, 4 },  { 146, 0 },
	                                       { 0, 0 }, { 142, 24 }, { 0, 0 }, { 146, 6 }, { 146, 28 }, { 0, 0 } };

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

// draws row index of a save file test menu: a member of retail's MenuUtil row base (see cgame.h), still a free
// function here
// NONMATCHING: row-count conversions and float register lifetimes differ; retail converts the alpha again before the
// second line segment, this build reuses the first conversion
extern "C" void fn_80050314(Menu* menu, s32 index, BOOL useRowWidth, BOOL proportional)
{
	MenuSFTest* rows = (MenuSFTest*)menu;
	MenuTextRow* row = &rows->mRows[index];
	if (row->IsSeparator()) {
		f32 rowHeight = 2.0f + menu->mFontSize;
		f32 x         = menu->mWindow.mX - (30.0f + menu->mMaxWidth) / 2.0f;
		f32 y         = menu->mWindow.mY - rowHeight * rows->mRowTotal / 2.0f + rowHeight * (index + 0.5f);
		u32 alpha     = (u32)(255.0f * menu->mAlpha);
		DrawGsLineSegment(&gGame.mGs, NULL, x, y, 0.0f, alpha | 0x96969600, 30.0f + menu->mMaxWidth + x, y, 0.0f, alpha | 0x96969600);
		DrawGsLineSegment(&gGame.mGs, NULL, 1.0f + x, 1.0f + y, 0.0f, (u32)(255.0f * menu->mAlpha) | 0xFAFAFA00,
		                  1.0f + (30.0f + menu->mMaxWidth + x), 1.0f + y, 0.0f, (u32)(255.0f * menu->mAlpha) | 0xFAFAFA00);
		return;
	}
	f32 width = useRowWidth ? row->mWidth : menu->mMaxWidth;
	f32 x     = menu->mWindow.mX - width / 2.0f;
	f32 y     = menu->mWindow.mY - (2.0f + menu->mFontSize) * rows->mRowTotal / 2.0f + (2.0f + menu->mFontSize) * index;
	u32 color = index == menu->mSelection ? (u32)(255.0f * menu->mAlpha) | 0x20202000 : (u32)(255.0f * menu->mAlpha) | 0x40404000;
	if (proportional) {
		DrawGsProportionalTextFormat(&gGame.mGs, color, row->mText, x, y, 0.0f, menu->mFontSize, menu->mFontSize);
	} else {
		DrawGsFixedWidthTextFormat(&gGame.mGs, color, row->mText, x, y, 0.0f, menu->mFontSize, menu->mFontSize);
	}
}

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
			                (Vec) { x - dx, 8.0f + (y - dy), z } };
		Vec inner[9]    = { (Vec) { (6.0f + x) - dx, 8.0f + ((6.0f + y) - dy), z },
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
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_NEQUAL, GX_FALSE);
		color = (u32)(96.0f * window->mDrawAlpha);
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, inner, color, 9);
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
extern s32 gRingScreenWidth, gRingScreenHeight;

// what the PC shop sold and what it sells
static MenuPCItem sItemShopEntries[20];
PCRankingRow gPCRankingRows[10];
static MenuPCItem sScrapShopEntries[20];

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

void MenuUtil::Init()
{
	Menu::Init();
	InitRows();
}

void MenuUtil::InitRows()
{
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

BOOL MenuSFTest::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		if (gGame.mSaveManager.mJobStarted) {
			return IsOpen();
		}
		s32 result = gGame.mSaveManager.mFile.mResult;
		if (result < 0) {
			sMenuSFTestErr.mWindow.mX = mWindow.mX;
			sMenuSFTestErr.mWindow.mY = mWindow.mY - (2.0f + mFontSize);
			sMenuSFTestErr.mState     = 1;
			sMenuSFTestErr.Resume();
			sMenuSFTestErr.mWindow.mOpen = TRUE;
			sMenuSFTestErr.InitRows();
			sMenuSFTestErr.mRows[sMenuSFTestErr.mCount].mText = (char*)GetNandGameResultName(&gGame.mSaveManager.mFile, result);
			sMenuSFTestErr.mRows[sMenuSFTestErr.mCount].mWidth
			    = MeasureGameTextWidth(&gGame.mGs, sMenuSFTestErr.mRows[sMenuSFTestErr.mCount].mText, 1, sMenuSFTestErr.mFontSize);
			if (sMenuSFTestErr.mRows[sMenuSFTestErr.mCount].mWidth > sMenuSFTestErr.mMaxWidth) {
				sMenuSFTestErr.mMaxWidth = sMenuSFTestErr.mRows[sMenuSFTestErr.mCount].mWidth;
			}
			sMenuSFTestErr.mCount++;
			sMenuSFTestErr.mWindow.mHeight = sMenuSFTestErr.mCount * (2.0f + sMenuSFTestErr.mFontSize) + 24.0f;
			sMenuSFTestErr.mWindow.mWidth  = 60.0f + sMenuSFTestErr.mMaxWidth;
			sMenuSFTestErr.mSelection      = 0;
			Suspend(FALSE);
			return IsOpen();
		}
		switch (mOperation) {
		case 1: {
			for (s32 i = 0; i < 10; i++) {
				WiiSaveScanEntry* entry = &gGame.mSaveManager.mScanEntries[i];
				char* text              = mRows[i].mText;
				if (entry->mStatus & 1) {
					if (entry->mStatus & 2) {
						sprintf(text, "(%d) ", i);
						sprintf(text + strlen(text) + 1, "broken");
					} else {
						WiiSaveSlotHeader* header = (WiiSaveSlotHeader*)entry->mData;
						OSCalendarTime time;
						OSTicksToCalendarTime(header->mTimestamp, &time);
						s32 seconds           = header->mPlayTime / 3600.0f;
						s32 hours             = header->mPlayTime / 216000.0f;
						s32 stage             = header->mStages[0];
						const char* stageName = GetStageNameOrFileName(stage);
						sprintf(text, "(%d) %02d/%02d %2d:%02d [%2d:%02d] (%3d)", i, time.mon + 1, time.mday, time.hour, time.min, hours,
						        seconds % 60, stage);
						sprintf(text + strlen(text) + 1, stageName);
					}
				} else {
					sprintf(text, "(%d) ", i);
					if (entry->mStatus & 8) {
						sprintf(text + strlen(text) + 1, "insufficient space");
					} else if (entry->mStatus & 0x10) {
						sprintf(text + strlen(text) + 1, "insufficient inode");
					} else {
						sprintf(text + strlen(text) + 1, "none");
					}
				}
			}
			u32 blocks, inodes;
			GetNandGameFreeSpace(&gGame.mSaveManager.mFile, &blocks, &inodes);
			sprintf(mRows[mRowTotal - 1].mText, "FREE FSBLOCK:%4d   FREE INODE: %2d", blocks, inodes);
			StartOperation(0);
			break;
		}
		case 3:
			if (result < 0) {
				sMenuSFTestErr.mWindow.mX = mWindow.mX;
				sMenuSFTestErr.mWindow.mY = mWindow.mY - (2.0f + mFontSize);
				sMenuSFTestErr.mState     = 1;
				sMenuSFTestErr.Resume();
				sMenuSFTestErr.mWindow.mOpen = TRUE;
				sMenuSFTestErr.InitRows();
				sMenuSFTestErr.mRows[sMenuSFTestErr.mCount].mText = (char*)GetNandGameResultName(&gGame.mSaveManager.mFile, result);
				sMenuSFTestErr.mRows[sMenuSFTestErr.mCount].mWidth
				    = MeasureGameTextWidth(&gGame.mGs, sMenuSFTestErr.mRows[sMenuSFTestErr.mCount].mText, 1, sMenuSFTestErr.mFontSize);
				if (sMenuSFTestErr.mRows[sMenuSFTestErr.mCount].mWidth > sMenuSFTestErr.mMaxWidth) {
					sMenuSFTestErr.mMaxWidth = sMenuSFTestErr.mRows[sMenuSFTestErr.mCount].mWidth;
				}
				sMenuSFTestErr.mCount++;
				sMenuSFTestErr.mWindow.mHeight = sMenuSFTestErr.mCount * (2.0f + sMenuSFTestErr.mFontSize) + 24.0f;
				sMenuSFTestErr.mWindow.mWidth  = 60.0f + sMenuSFTestErr.mMaxWidth;
				sMenuSFTestErr.mSelection      = 0;
				Suspend(FALSE);
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			} else {
				StartOperation(1);
			}
			return IsOpen();
		case 5:
			StartOperation(0);
			if (result == 0) {
				Close(5);
				if (sMenuStageSelect.IsOpen()) {
					sMenuStageSelect.Close(5);
					PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
					gGame.ClearStatusFlags(2);
				}
				_168 = 10002;
			}
			return IsOpen();
		case 7:
			StartOperation(1);
			return IsOpen();
		}
		if (gWiiPointer.Check(PadActStickUp)) {
			if (mRowTotal <= 0) {
				goto done;
			}
			do {
				if (--mSelection < 0) {
					mSelection = 9;
				}
			} while (mRows[mSelection].IsSeparator());
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickDown)) {
			if (mRowTotal <= 0) {
				goto done;
			}
			do {
				if (++mSelection > 9) {
					mSelection = 0;
				}
			} while (mRows[mSelection].IsSeparator());
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActCancel)) {
			Close(5);
			if (sMenuStageSelect.mSuspended) {
				gGame.SetStatusFlags(2);
			}
			sMenuStageSelect.mSuspended   = FALSE;
			sMenuStageSelect.mSuspendFade = FALSE;
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActConfirm)) {
			sMenuSFTestSub.mWindow.mX = 88.0f + (mWindow.mX - mWindow.mWidth / 2.0f);
			sMenuSFTestSub.mWindow.mY = 80.0f + (mWindow.mY - mWindow.mHeight / 2.0f) + (2.0f + mFontSize) * mSelection;
			sMenuSFTestSub.mState     = 1;
			sMenuSFTestSub.Resume();
			sMenuSFTestSub.mWindow.mOpen = TRUE;
			sMenuSFTestSub.InitRows();
			for (s32 i = 0; i < 4; i++) {
				sMenuSFTestSub.mRows[sMenuSFTestSub.mCount].mText = sSFTestSubLines[i].mText;
				sMenuSFTestSub.mRows[sMenuSFTestSub.mCount].mWidth
				    = MeasureGameTextWidth(&gGame.mGs, sMenuSFTestSub.mRows[sMenuSFTestSub.mCount].mText, 1, sMenuSFTestSub.mFontSize);
				if (sMenuSFTestSub.mRows[sMenuSFTestSub.mCount].mWidth > sMenuSFTestSub.mMaxWidth) {
					sMenuSFTestSub.mMaxWidth = sMenuSFTestSub.mRows[sMenuSFTestSub.mCount].mWidth;
				}
				sMenuSFTestSub.mCount++;
			}
			sMenuSFTestSub.mWindow.mHeight = sMenuSFTestSub.mCount * (2.0f + sMenuSFTestSub.mFontSize) + 24.0f;
			sMenuSFTestSub.mWindow.mWidth  = 60.0f + sMenuSFTestSub.mMaxWidth;
			sMenuSFTestSub.mSelection      = 0;
			Suspend(FALSE);
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
		}
	}

done:
	if (IsOpen()) {
		return TRUE;
	}
	if (_168 >= 0) {
		ChangeGameStage(&gGame, _168, TRUE);
	}
	return FALSE;
}

BOOL MenuSFTest::Draw()
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
		for (s32 i = 0; i < mRowTotal; i++) {
			char* text = mRows[i].mText;
			if (mRows[i].IsSeparator()) {
				fn_80050314(this, i, TRUE, TRUE);
				continue;
			}
			if (*text == 0) {
				continue;
			}
			fn_80050314(this, i, FALSE, FALSE);
			f32 rowHeight = 2.0f + mFontSize;
			f32 x         = mWindow.mX - mMaxWidth / 2.0f + MeasureGameTextWidth(&gGame.mGs, text, 0, mFontSize);
			f32 y         = mWindow.mY - rowHeight * mRowTotal / 2.0f + rowHeight * i;
			u32 color     = i == mSelection ? (u32)(255.0f * mAlpha) | 0x20202000 : (u32)(255.0f * mAlpha) | 0x40404000;
			DrawGsProportionalTextFormat(&gGame.mGs, color, text + strlen(text) + 1, x, y, 0.0f, mFontSize, mFontSize);
		}
		f32 rowHeight = 2.0f + mFontSize;
		f32 x         = mWindow.mX - mMaxWidth / 2.0f;
		f32 y         = mWindow.mY - rowHeight * mRowTotal / 2.0f + rowHeight * mSelection;
		f32 alpha;
		if (sMenuSFTestSub.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuSFTestSub.mWindow.mDrawAlpha : 0.0f;
		} else if (sMenuSFTestErr.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuSFTestErr.mWindow.mDrawAlpha : 0.0f;
		} else {
			alpha = mAlpha;
		}
		DrawMenuSelectionCursor(mCursor, x, y, mMaxWidth, mFontSize, mAlpha, alpha);
		return TRUE;
	}
	return FALSE;
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

extern CbPaneLayout lbl_802E94C0[5];

void MenuSave::Init()
{
	CMenu::Init();
	mLastFile = -1;
	for (s32 i = 0; i < 5; i++) {
		lbl_802E94C0[i].mTrans.x -= 0.5f * 640;
		lbl_802E94C0[i].mTrans.y = 0.5f * (f32)gPointerScreenHeight - lbl_802E94C0[i].mTrans.y;
		fn_800556B0(&mPanes[i], &lbl_802E94C0[i]);
	}
	s32 width           = gGame.mAspectRatio == 0 ? 640 : 854;
	gRingScreenWidth    = width;
	gPointerScreenWidth = width;
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

void MenuItem::Open()
{
	CMenu::Open();
	gGame.SetStatusFlags(2);
	mWindow.SetSize(0.0f, 0.0f);
	mWindow.SetPos(0.0f, 0.0f);
	ClearStatusTable(&sStatusItemTable);
	PopulateOwnedItemPage(&sStatusItemTable);
	s32 count = CountMenuItems();
	mGridX    = 758.0f;
	mGridY    = 36.0f;
	if (count == 0) {
		mTargetGridX = 360.0f;
	} else {
		mTargetGridX = 360.0f - (count - 1) * 51;
	}
	mTargetGridX    = 207.0f;
	mTargetGridY    = 36.0f;
	mPanelX         = -10.0f;
	mPanelY         = -200.0f;
	mTargetPanelX   = -10.0f;
	mTargetPanelY   = 0.0f;
	mMode           = 0;
	mDelay          = 0;
	sInventoryPhase = 0.0f;
	mSelection      = 0;
	mPrevSelection  = mSelection;
	mStickSelection = 0;
	_198            = 0.0f;
	_19C            = 0.0f;
	fn_8005A68C(this, GetLocalizedInterfaceText(0x19), (u32)(255.0f * mAlpha) | 0xFFFFFF00);
	mShowHelp  = 0;
	mNameAlpha = 0.0f;
	mSelection = -1;
	mName      = NULL;
	mShowHelp  = 1;
	mHelp      = NULL;
	mHelpCount = 0;
	for (int i = 0; i < 20; i++) {
		mBg[i].mAngle0 = DEG2RAD * (rand() % 360);
		mBg[i].mAngle1 = DEG2RAD * (rand() % 360);
	}
	gPointerPanes.mMenu   = this;
	gPointerPanes.mActive = TRUE;
	for (int i = 0; i < 8; i++) {
		mCells[i].mPointed = FALSE;
		mCells[i].mPicked  = FALSE;
	}
	for (int i = 0; i < 2; i++) {
		mPanes[i].mPointed = FALSE;
		mPanes[i].mPicked  = FALSE;
		mPanes[i].mEnabled = FALSE;
		mPanes[i]._48      = 0.0f;
	}
	mHovered                      = 0;
	mPaneFades.mFades[0].mPane    = &mPanes[0];
	mPaneFades.mFades[0].mAlpha   = 0.0f;
	mPaneFades.mFades[0].mShown   = FALSE;
	mPaneFades.mFades[0].mHovered = FALSE;
	mPaneFades.mFades[1].mPane    = &mPanes[1];
	mPaneFades.mFades[1].mAlpha   = 0.0f;
	mPaneFades.mFades[1].mShown   = FALSE;
	mPaneFades.mFades[1].mHovered = FALSE;
}

// NONMATCHING: retail calls MenuPaneFadePair::Update where this build expands it; the function comes out 88 bytes larger
// and register allocation differs
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
				sInventoryPhase = 0.0f;
			}
			break;
		case 2:
			if (mSelection != -1)
				mStickSelection = mSelection;
			if (!IsStickUsedOverPointer()) {
				if (count > 1) {
					if (IsStickLeft()) {
						if (--mSelection < 0)
							mSelection = count - 1;
						PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
					} else if (IsStickRight()) {
						mSelection++;
						if (sStatusItemTable.mItems[mSelection].mItem < 0)
							mSelection = 0;
						PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
					}
				}
				if (count > 4) {
					if (mSelection >= 4) {
						if (IsStickUp()) {
							if ((mSelection -= 4) < 0)
								mSelection = count - 1;
							PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
						}
						if (IsStickDown() && sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
							mTargetGridY = -56.0f;
							mMode        = 10;
							break;
						}
					} else {
						if (IsStickDown() && mSelection + 4 <= count - 1) {
							mSelection += 4;
							PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
						}
						if (IsStickUp()) {
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
					CountMenuItems();
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
				gGame.ClearStatusFlags(2);
				gGame.mEvt.mSelectResult = selectedItem;
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
		mNameAlpha += (1.0f / (0.2f * gRefreshRate)) * frames;
		if (mNameAlpha >= 1.0f) {
			mNameAlpha = 1.0f;
			mShowHelp  = 0;
		}
		break;
	case 3:
		mNameAlpha -= (1.0f / (0.25f * gRefreshRate)) * frames;
		if (mNameAlpha <= 0.0f)
			mNameAlpha = 0.0f;
		break;
	}
	if (mAlpha > 0.0f) {
		for (int i = 0; i < (int)ARRAY_SIZE(mBg); i++) {
			mBg[i].mAngle0 += 0.08726646f * frames;
			mBg[i].mAngle1 += 0.05235988f * frames;
		}
	}
	return IsOpen();
}

// NONMATCHING: the call list matches retail; this build is 72 bytes smaller, reserves 16 more stack bytes,
// and differs in float register lifetimes and scheduling
extern "C" void DrawInventoryGrid(s32 selected, f32 x, f32 y, f32 alpha, f32 scale, f32 phase)
{
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	for (int i = 0; i < 8; i++) {
		f32 w = 92.0f * scale, h = 80.0f * scale;
		f32 top = (h - h / 8.0f) / 2.0f;
		f32 dx = w / 8.0f + w, dy = h / 8.0f + h;
		f32 xx = (i % 4) * dx + x;
		f32 yy = 12.0f + (top + ((i / 4) * dy + y));
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
			f32 firstWidth = GXGetTexObjWidth(&tex), secondWidth = GXGetTexObjWidth(&tex);
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
			f32 tw = GXGetTexObjWidth(&tex), th = GXGetTexObjHeight(&tex);
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
		f32 top = (h - h / 8.0f) / 2.0f;
		f32 dx = w / 8.0f + w, dy = h / 8.0f + h;
		f32 xx = (i % 4) * dx + x;
		f32 yy = 12.0f + (top + (2 * dy + y));
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
				f32 iw = GXGetTexObjWidth(&tex), ih = GXGetTexObjHeight(&tex);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
				                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, xx, yy, 0.0f, scale * iw,
				                   scale * ih, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
			f32 enlarged = 1.0f;
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
			tex = gGame.mTpl.mTex;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			f32 firstWidth = GXGetTexObjWidth(&tex), secondWidth = GXGetTexObjWidth(&tex);
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
			f32 tw = GXGetTexObjWidth(&tex), th = GXGetTexObjHeight(&tex);
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
		f32 top = (h - h / 8.0f) / 2.0f;
		f32 dx = w / 8.0f + w, dy = h / 8.0f + h;
		f32 xx = (i % 4) * dx + x;
		f32 yy = 12.0f + (top + (-1 * dy + y));
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
				f32 iw = GXGetTexObjWidth(&tex), ih = GXGetTexObjHeight(&tex);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00,
				                   (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00, xx, yy, 0.0f, scale * iw,
				                   scale * ih, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
			f32 enlarged = 1.0f;
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
			tex = gGame.mTpl.mTex;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			f32 firstWidth = GXGetTexObjWidth(&tex), secondWidth = GXGetTexObjWidth(&tex);
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
			f32 tw = GXGetTexObjWidth(&tex), th = GXGetTexObjHeight(&tex);
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

void MenuItem::vf08(CbPane* pane)
{
	s32 kind = pane->mKind;
	mHovered = TRUE;
	if (kind < 8 || kind > 9 || mMode != 2 || CountMenuItems() <= 4) {
		return;
	}
	if (kind == 8 && sStatusItemTable.mPage > 0) {
		mTargetGridY = 128.0f;
		mMode        = 20;
	} else if (kind == 9 && sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
		mTargetGridY = -56.0f;
		mMode        = 10;
	}
}

void MenuItem::vf14(CbPane* pane)
{
	if (!mHovered) {
		return;
	}
	s32 kind = pane->mKind;
	if (kind < 8 || kind > 9 || mMode != 2 || CountMenuItems() <= 4) {
		return;
	}
	if (kind == 8 && sStatusItemTable.mPage > 0) {
		mTargetGridY = 128.0f;
		mMode        = 20;
	} else if (kind == 9 && sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
		mTargetGridY = -56.0f;
		mMode        = 10;
	}
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
		DrawStatusPanelBackground(mAlpha);
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

static inline void DrawDiscardOutline(MenuStatus* menu, f32 x, f32 y)
{
	f32 left = x - 120.0f, top = y - 72.0f;
	f32 right = 240.0f + left, bottom = 144.0f + top;
	f32 depth      = -10.0f;
	Vec outline[5] = { (Vec) { left, top, depth }, (Vec) { right, top, depth }, (Vec) { right, bottom, depth },
		               (Vec) { left, bottom, depth }, (Vec) { left, top, depth } };
	DrawGsLineStrip(&gGame.mGs, outline, (u32)(255.0f * menu->mAlpha) | 0xC8282800, 5);
}

static inline void DrawDiscardDigit(MenuStatus* menu, CTexObj& tex, s32 digit, f32 x, f32 y, f32 width, f32 height, f32 alpha)
{
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	u32 color = (u32)((255.0f * menu->mAlpha) * alpha) | 0xC8C8C800;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// NONMATCHING: retail calls DrawMenuRectWithShadow where this build expands it, adding 696 bytes.
// The other draw calls, texture indices, constants and state-specific text paths match.
extern "C" void DrawDiscardCountPanel(MenuStatus* menu)
{
	f32 alpha = 1.0f;
	f32 phase = fmod(sStatusPhase, 360.0);
	f32 wave  = 2.0f * (f32)sin(DEG2RAD * (24.0f * phase));
	f32 x     = (240.0f + ((menu->mDiscardPanelX - 52.0f) + menu->mOffsetX)) - 204.0f;
	f32 y     = (144.0f + (((180.0f + menu->mDiscardPanelY) + menu->mOffsetY) - 96.0f)) - 36.0f;
	DrawMenuRectWithShadow(menu, (u32)((255.0f * menu->mAlpha) * alpha) | 0xFFB62800, x, y, 240.0f, 144.0f);
	if (menu->mCount == 0) {
		DrawDiscardOutline(menu, x, y);
	}
	char label[256];
	u32 halfWidth = (24 * (strlen(label) / 2 - 1)) / 2;
	f32 labelX    = 12.0f + ((12.0f + (4.0f + ((40.0f + ((72.0f + x) - halfWidth)) - 120.0f))) - 20.0f);
	f32 labelY    = (12.0f + ((2.0f + (((22.0f + y) - 12.0f) - 2.0f)) - 22.0f)) - 36.0f;
	DrawDoubleByteMenuText(label, labelX, labelY, -10.0f, 24.0f, 24.0f, menu->mAlpha * alpha);

	CTexObj tex;
	f32 radius = 30.0f + wave;
	for (s32 i = 0; i < 2; i++) {
		f32 angle = PI * i + HALF_PI;
		f32 sine = (f32)sin(angle), cosine = (f32)cos(angle);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[128], 0);
		tex       = gGame.mTpl.mTex;
		u16 width = GXGetTexObjWidth(&tex), height = GXGetTexObjHeight(&tex);
		u32 color = (u32)((255.0f * menu->mAlpha) * alpha) | 0xFFEA0000;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, radius * sine + x, 16.0f + (radius * cosine + y), 0.0f,
		                   1.4f * (1.1f * width), 1.4f * height, 0.0f, 0.0f, PI + angle);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}

	switch (menu->mPhase) {
	case 1: {
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[125], 0);
		tex = gGame.mTpl.mTex;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		s32 count  = menu->mCount;
		f32 mix    = 1.0f;
		f32 width  = (1.0f - mix) * GXGetTexObjWidth(&tex) + (1.4f * mix) * GXGetTexObjWidth(&tex);
		f32 height = (1.0f - mix) * (GXGetTexObjHeight(&tex) / 10) + (1.4f * mix) * (GXGetTexObjHeight(&tex) / 10);
		PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
		tex.mTexMtx.mDirty = TRUE;
		if (count >= 10) {
			DrawDiscardDigit(menu, tex, count / 10, x - width / 3.0f, 14.0f + y, width, height, alpha);
			DrawDiscardDigit(menu, tex, count % 10, x + width / 3.0f, 14.0f + y, width, height, alpha);
		} else {
			DrawDiscardDigit(menu, tex, count, x, 14.0f + y, width, height, alpha);
		}
		break;
	}
	case 2: {
		char message[256];
		(void)strlen(message);
		DrawDoubleByteMenuText(message, x, 14.0f + y, -10.0f, 24.0f, 24.0f, menu->mAlpha * alpha);
		break;
	}
	}
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
				if (!(status & 1) || (status & 2)) {
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
		DrawMenuSelectionCursor(mCursor, x, y, mMaxWidth, mFontSize, mAlpha, mAlpha);
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
	pane->mWidth   = layout->mSize.width;
	pane->mHeight  = layout->mSize.height;
	pane->mKind    = layout->mKind;
}

CbPaneLayout lbl_802E87E8[51] = {
	{ nw4r::math::VEC3(152.0f + -12, 216.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(76.8f, 76.8f), lbl_805A1BC8, 0 },
	{ nw4r::math::VEC3(260.0f + 0, 216.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(76.8f, 76.8f), lbl_805A1BC8, 1 },
	{ nw4r::math::VEC3(368.0f + 12, 216.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(76.8f, 76.8f), lbl_805A1BC8, 2 },
	{ nw4r::math::VEC3(476.0f + 24, 216.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(76.8f, 76.8f), lbl_805A1BC8, 3 },
	{ nw4r::math::VEC3(165.0f, 183.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(64.0f, 64.0f), lbl_805A1BC8, 4 },
	{ nw4r::math::VEC3(268.5f, 183.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(64.0f, 64.0f), lbl_805A1BC8, 5 },
	{ nw4r::math::VEC3(372.0f, 183.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(64.0f, 64.0f), lbl_805A1BC8, 6 },
	{ nw4r::math::VEC3(475.5f, 183.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(64.0f, 64.0f), lbl_805A1BC8, 7 },
	{ nw4r::math::VEC3(165.0f, 273.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(64.0f, 64.0f), lbl_805A1BC8, 8 },
	{ nw4r::math::VEC3(268.5f, 273.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(64.0f, 64.0f), lbl_805A1BC8, 9 },
	{ nw4r::math::VEC3(372.0f, 273.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(64.0f, 64.0f), lbl_805A1BC8, 10 },
	{ nw4r::math::VEC3(475.5f, 273.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(64.0f, 64.0f), lbl_805A1BC8, 11 },
	{ nw4r::math::VEC3(58.0f, 136.8f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(28.8f, 28.8f), lbl_805A1BC8, 12 },
	{ nw4r::math::VEC3(58.0f, 136.8f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(28.8f, 28.8f), lbl_805A1BC8, 12 },
	{ nw4r::math::VEC3(58.0f, 220.8f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(28.8f, 28.8f), lbl_805A1BC8, 14 },
	{ nw4r::math::VEC3(58.0f, 264.8f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(28.8f, 28.8f), lbl_805A1BC8, 15 },
	{ nw4r::math::VEC3(58.0f, 308.8f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(28.8f, 28.8f), lbl_805A1BC8, 16 },
	{ nw4r::math::VEC3(306.0f, 148.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(38.4f, 38.4f), lbl_805A1BC8, 17 },
	{ nw4r::math::VEC3(306.0f, 200.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(38.4f, 38.4f), lbl_805A1BC8, 18 },
	{ nw4r::math::VEC3(306.0f, 252.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(38.4f, 38.4f), lbl_805A1BC8, 19 },
	{ nw4r::math::VEC3(306.0f, 304.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(38.4f, 38.4f), lbl_805A1BC8, 20 },
	{ nw4r::math::VEC3(570.0f, 148.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(38.4f, 38.4f), lbl_805A1BC8, 21 },
	{ nw4r::math::VEC3(570.0f, 200.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(38.4f, 38.4f), lbl_805A1BC8, 22 },
	{ nw4r::math::VEC3(570.0f, 252.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(38.4f, 38.4f), lbl_805A1BC8, 23 },
	{ nw4r::math::VEC3(570.0f, 304.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(38.4f, 38.4f), lbl_805A1BC8, 24 },
	{ nw4r::math::VEC3(117.0f, 135.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 25 },
	{ nw4r::math::VEC3(197.0f, 135.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 26 },
	{ nw4r::math::VEC3(277.0f, 135.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 27 },
	{ nw4r::math::VEC3(357.0f, 135.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 28 },
	{ nw4r::math::VEC3(437.0f, 135.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 29 },
	{ nw4r::math::VEC3(517.0f, 135.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 30 },
	{ nw4r::math::VEC3(117.0f, 195.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 31 },
	{ nw4r::math::VEC3(197.0f, 195.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 32 },
	{ nw4r::math::VEC3(277.0f, 195.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 33 },
	{ nw4r::math::VEC3(357.0f, 195.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 34 },
	{ nw4r::math::VEC3(437.0f, 195.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 35 },
	{ nw4r::math::VEC3(517.0f, 195.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 36 },
	{ nw4r::math::VEC3(117.0f, 255.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 37 },
	{ nw4r::math::VEC3(197.0f, 255.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 38 },
	{ nw4r::math::VEC3(277.0f, 255.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 39 },
	{ nw4r::math::VEC3(357.0f, 255.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 40 },
	{ nw4r::math::VEC3(437.0f, 255.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 41 },
	{ nw4r::math::VEC3(517.0f, 255.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 42 },
	{ nw4r::math::VEC3(117.0f, 315.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 43 },
	{ nw4r::math::VEC3(197.0f, 315.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 44 },
	{ nw4r::math::VEC3(277.0f, 315.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 45 },
	{ nw4r::math::VEC3(357.0f, 315.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 46 },
	{ nw4r::math::VEC3(437.0f, 315.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 47 },
	{ nw4r::math::VEC3(517.0f, 315.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(51.2f, 51.2f), lbl_805A1BC8, 48 },
	{ nw4r::math::VEC3(0.0f, 0.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(64.0f, 64.0f), lbl_805A1BC8, 49 },
	{ nw4r::math::VEC3(0.0f, 0.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(64.0f, 64.0f), lbl_805A1BC8, 50 },
};

MenuTextColors lbl_8044B700 = {
	NULL,
	CColor(255, 255, 255, 0),
	CColor(0, 0, 0, 0),
	FALSE,
};

CbPaneLayout lbl_802E94C0[5] = {
	{ nw4r::math::VEC3(126.0f, 223.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(176.0f, 244.0f), CColor(255, 255, 255, 255),
	  0 },
	{ nw4r::math::VEC3(320.0f, 223.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(176.0f, 244.0f), CColor(255, 255, 255, 255),
	  1 },
	{ nw4r::math::VEC3(514.0f, 223.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(176.0f, 244.0f), CColor(255, 255, 255, 255),
	  2 },
	{ nw4r::math::VEC3(281.25f, 0.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(61.5f, 22.0f), CColor(255, 255, 255, 255), 3 },
	{ nw4r::math::VEC3(358.75f, 0.0f, 0.0f), nw4r::math::VEC2(1.0f, 1.0f), nw4r::lyt::Size(61.5f, 22.0f), CColor(255, 255, 255, 255), 4 },
};

// NONMATCHING: retail stores the two page panes' kinds as the constants 8 and 9 while it indexes the panes by i;
// this build keeps kind in a register
void MenuItem::Init()
{
	CMenu::Init();
	mSelection = 0;
	int i      = 0;
	int kind   = 0;
	for (; i < 8; i++) {
		fn_800556B0(&mCells[i], &lbl_802E87E8[i + 4]);
		mCells[i].mKind = kind++;
	}
	fn_800556B0(&mPanes[i - 8], &lbl_802E87E8[49]);
	mPanes[i - 8].mKind = kind++;
	i++;
	fn_800556B0(&mPanes[i - 8], &lbl_802E87E8[50]);
	mPanes[i - 8].mKind = kind++;
	i++;
	if (i > 10) {
		OSPanic("menu_item.cpp", 0x255, "@MenuItem::Init MenuItemが保持できるCbPaneの最大数を超えています！(num = %d)", i);
	}
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

extern "C" bool fn_80055A7C(CbPaneList* list, CbPane* pane, f32 x, f32 y);

extern "C" void fn_800559D8(CbPaneList* list, CbPane* pane, s32 event, s32)
{
	switch (event) {
	case 1:
		if (list->mMenu != NULL) {
			list->mMenu->vf10(pane);
		}
		break;
	case 2:
		if (list->mMenu != NULL) {
			list->mMenu->vf0C(pane);
		}
		break;
	case 0:
		if (list->mMenu != NULL) {
			list->mMenu->vf08(pane);
		}
		break;
	case 3:
		break;
	case 6:
		if (list->mMenu != NULL) {
			list->mMenu->vf14(pane);
		}
		break;
	}
}

// NONMATCHING: MWCC inlines fn_80055A7C and fn_800559D8 throughout the pane loop; retail calls both out of line
extern "C" void fn_8005575C(CbPaneList* list, bool pointerOn, nw4r::math::VEC2* pos, u32 frames)
{
	if (list->mEnabled) {
		pointerOn = pointerOn && (gGame.mMenuControls & MenuControlPointer);
	}
	bool released = false;
	if (list->mActive) {
		if (pointerOn && !list->_409) {
			if (list->mMenu != NULL) {
				list->mMenu->vf18(pos);
			}
		} else if (!pointerOn && list->_409) {
			released = true;
		}
		list->_409 = pointerOn;
	}
	CbPane* focused = NULL;
	for (s32 i = 0; i < list->mCount; i++) {
		CbPane* pane = list->mPanes[i];
		bool current = false;
		if (pointerOn && fn_80055A7C(list, pane, pos->x, pos->y)) {
			if (pane->mPointed) {
				fn_800559D8(list, pane, 3, 0);
			} else {
				pane->mPointed = TRUE;
				fn_800559D8(list, pane, 1, 0);
			}
			current = true;
		} else if (pane->mPointed) {
			pane->mPointed = FALSE;
			fn_800559D8(list, pane, 2, 0);
		}
		if (current) {
			focused = pane;
		}
	}
	if (gWiiPointer.mButton & 0x10) {
		list->_408 += frames;
	} else {
		list->_408 = 0;
	}
	if (focused != NULL) {
		if (gWiiPointer.Check(PadActConfirm)) {
			fn_800559D8(list, focused, 0, 0);
		}
		if (list->_408 >= 24 && list->_408 % 6 == 0) {
			fn_800559D8(list, focused, 6, 0);
			list->_408 = 24;
		}
	}
	if (released && list->mMenu != NULL) {
		list->mMenu->vf1C(pos);
	}
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

static inline void InitPCPanelTexture(CTexObj& tex, s32 icon)
{
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[icon], 0);
	tex = gGame.mTpl.mTex;
}

static inline void DrawOverviewTexture(CTexObj& tex, u32 color, f32 x, f32 y, f32 width, f32 height)
{
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

static inline void DrawStatusOverviewEntry(MenuStatus* menu, s32 icon, s32 cell, s32 selected, f32 x, f32 y)
{
	DrawPlayerStatusIcon(icon, gStatusEntryAvailable[cell], cell == selected, x, y, menu->mAlpha, 0.8f, menu->mPulsePhase);
}

static inline void BeginOverviewTexture(CTexObj& tex)
{
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 1.0f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
}

extern "C" void DrawStatusMenuTabs(MenuStatus* menu)
{
	CTexObj tex;
	f32 offset = 0.0f;
	if (menu->mSelection == -1) {
		sStatusPhase = 0.0f;
	}
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	for (int i = 0; i < 4; i++) {
		if (!menu->mHasFourthTab && i == 3) {
			continue;
		}
		f32 bob;
		if (menu->mSelection == i && menu->mMode == 1) {
			f32 phase = fmod(sStatusPhase, 360.0);
			bob       = 4.0f * (f32)sin(DEG2RAD * (20.0f * phase));
		} else {
			bob = 0.0f;
		}
		f32 x = 6.0f + (8.0f + ((menu->mTabs[i].mX - 52.0f) + menu->mOffsetX));
		f32 y = bob + ((180.0f + menu->mTabs[i].mY) + menu->mOffsetY);
		switch (i) {
		case 0:
			offset = -12.0f;
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[0], 0);
			tex = gGame.mTpl.mTex;
			break;
		case 1:
			offset = 0.0f;
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[238], 0);
			tex = gGame.mTpl.mTex;
			break;
		case 2:
			offset = 12.0f;
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[236], 0);
			tex = gGame.mTpl.mTex;
			break;
		case 3:
			offset = 24.0f;
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[237], 0);
			tex = gGame.mTpl.mTex;
			break;
		}
		f32 width  = 1.2f * GXGetTexObjWidth(&tex);
		f32 height = 1.2f * GXGetTexObjHeight(&tex);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		gGame.mGs.mCurRenderState->SetZCompLoc(GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		u32 color = (u32)(255.0f * menu->mAlpha) | 0xC8C8C800;
		x += offset;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZCompLoc(GX_TRUE);
		if (menu->mSelection == i && menu->mMode == 1) {
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_EQUAL, GX_FALSE);
			f32 pulse = menu->mAlpha * (0.2f * (f32)sin(PI * menu->mPulsePhase) + 0.2f);
			if (pulse < 0.0f) {
				pulse = 0.0f;
			}
			DrawGsQuad(&gGame.mGs, (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00,
			           (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00, x, y, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		}
	}
}

// NONMATCHING: register allocation and floating-point scheduling
extern "C" void DrawPlayerStatusOverview(MenuStatus* menu)
{
	if (menu->mMode == 9 || menu->mMode == 4) {
		CTexObj tex;
		f32 x        = (menu->mOverviewX - 42.0f) + menu->mOffsetX;
		f32 y        = (180.0f + menu->mOverviewY) + menu->mOffsetY;
		s32 selected = menu->mPhase == 100 ? -1 : menu->mOverviewSelection;
		DrawPlayerStatusValue(0xB5, gGame.GetPlayer()->mParams.mRank, selected == 0, 20.0f + (x - 280.0f), (y - 80.0f) + 8.0f, menu->mAlpha,
		                      menu->mPulsePhase);
		DrawPlayerStatusValue(0x9B, gGame.GetPlayer()->mParams.mExperience, selected == 2, 20.0f + (x - 280.0f),
		                      40.0f + (8.0f + ((y - 80.0f) + 44.0f)), menu->mAlpha, menu->mPulsePhase);
		DrawPlayerStatusValue(0x99, gGame.GetPlayer()->mParams.mMoney, selected == 3, 20.0f + (x - 280.0f),
		                      40.0f + (8.0f + ((y - 80.0f) + 88.0f)), menu->mAlpha, menu->mPulsePhase);
		DrawPlayerStatusValue(0x9A, gGame.GetPlayer()->mParams.mScrap, selected == 4, 20.0f + (x - 280.0f),
		                      40.0f + (8.0f + ((y - 80.0f) + 132.0f)), menu->mAlpha, menu->mPulsePhase);
		InitPCPanelTexture(tex, 174);
		f32 width = 0.6f * (1.1f * GXGetTexObjWidth(&tex)), height = 0.6f * GXGetTexObjHeight(&tex);
		BeginOverviewTexture(tex);
		u32 color = (u32)(255.0f * menu->mAlpha) | 0xC8C8C800;
		DrawOverviewTexture(tex, color, ((x - 24.0f) - 48.0f) - 12.0f, y + 14.0f, width, height);
		f32 left = ((48.0f + x) - 48.0f) - 12.0f;
		f32 row0 = (y - 82.0f) + 14.0f;
		DrawStatusOverviewEntry(menu, 0xAB, 5, selected, left, row0);
		f32 row1 = 14.0f + ((y - 82.0f) + 52.0f);
		DrawStatusOverviewEntry(menu, 0xB4, 6, selected, left, row1);
		f32 row2 = 14.0f + ((y - 82.0f) + 104.0f);
		DrawStatusOverviewEntry(menu, 0xAA, 7, selected, left, row2);
		f32 row3 = 14.0f + ((y - 82.0f) + 156.0f);
		DrawStatusOverviewEntry(menu, 0xAD, 8, selected, left, row3);
		InitPCPanelTexture(tex, 179);
		f32 w = 1.4f * (1.1f * GXGetTexObjWidth(&tex)), h = 1.4f * GXGetTexObjHeight(&tex);
		BeginOverviewTexture(tex);
		if (menu->mIconLit) {
			color = (u32)(255.0f * menu->mAlpha) | 0xC8C8C800;
		} else {
			color = (u32)(255.0f * menu->mAlpha);
		}
		f32 center = ((180.0f + (x - 24.0f)) - 24.0f) - 12.0f;
		f32 middle = (y - 18.0f) + 14.0f;
		DrawOverviewTexture(tex, color, center, middle, w, h);
		if (gEventFlags.mFlags[28 / 32] & (1u << (28 % 32))) {
			InitPCPanelTexture(tex, 246);
			f32 width  = 1.4f * (1.1f * GXGetTexObjWidth(&tex));
			f32 height = 1.4f * GXGetTexObjHeight(&tex);
			BeginOverviewTexture(tex);
			color = (u32)(255.0f * menu->mAlpha) | 0xC8C8C800;
			DrawOverviewTexture(tex, color, 18.0f + center, 56.5f + middle, width, height);
		}
		if (menu->mShowGauge) {
			DrawStatusGauge(menu->mGaugeValue, (11.0f + ((140.0f + x) - 120.0f)) - 12.0f, (114.0f + y) - 22.0f, menu->mAlpha);
		}
		f32 right = (192.0f + (72.0f + x)) - 12.0f;
		DrawStatusOverviewEntry(menu, 0xAF, 9, selected, right, row0);
		DrawStatusOverviewEntry(menu, 0xB0, 10, selected, right, row1);
		DrawStatusOverviewEntry(menu, 0xB1, 11, selected, right, row2);
		DrawStatusOverviewEntry(menu, 0xB2, 12, selected, right, row3);
		DrawStatusOverviewDetails(menu->mOverviewSelection, gPointerScreenWidth / 2, gPointerScreenHeight / 2, menu->mAlpha,
		                          menu->mDetailScale);
	}
}

// NONMATCHING: retail adds the 190 with the literal first; the other instructions and all call arguments match
extern "C" void fn_80061DBC(MenuStatus* menu)
{
	CTexObj tex;
	if (menu->mMode == 5 || menu->mMode == 4) {
		f32 x = menu->mDiagramX + menu->mOffsetX;
		f32 y = menu->mDiagramY + menu->mOffsetY;
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[menu->_1954], 0);
		tex = gGame.mTpl.mTex;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		f32 width  = GXGetTexObjWidth(&tex);
		f32 height = GXGetTexObjHeight(&tex);
		y += 190.0f;
		u32 color = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
}

static inline void DrawPCShopCenteredText(const char* text, u32 color, f32 x, f32 y, f32 size)
{
	f32 width = MeasureGameTextWidth(&gGame.mGs, text, TRUE, size);
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - width / 2.0f, y, 0.0f, size, size);
}

static inline void DrawPCPurchaseResult(s32 bought, f32 x, f32 y, f32 size, u32 white)
{
	f32 labelWidth = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(bought), TRUE, size);
	DrawGsProportionalTextFormat(&gGame.mGs, white, GetLocalizedInterfaceText(bought), x - labelWidth / 2.0f, y, 0.0f, size, size);
}

static inline void DrawPCRecycleLabel(s32 id, f32 x, f32 y, f32 size, f32 alpha)
{
	const char* text = GetLocalizedInterfaceText(id);
	u32 color        = (u32)(255.0f * alpha) | 0xFFFFFF00;
	x -= MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(id), TRUE, size) / 2.0f;
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x, y, 0.0f, size, size);
}

static inline f32 PCRecycleChoiceX(f32 x, f32 size, f32 scale, f32 margin, f32 offset)
{
	f32 base = (4.0f * scale + (size / 2.0f + (x + size))) - 40.0f * scale;
	return 40.0f * scale + (margin + (offset + base));
}

static inline void DrawPCPurchaseLabels(s32 question, s32 yes, s32 no, f32 left, f32 top, f32 size, f32 scale, f32 margin, f32 offset,
                                        f32 rowY, f32 extraW, f32 alpha)
{
	f32 textX = (extraW / 2.0f) * scale + PCRecycleChoiceX(left, size, scale, margin, offset);
	DrawPCRecycleLabel(question, textX, (top + rowY) - 4.0f * scale, size, alpha);
	DrawPCRecycleLabel(yes, textX, rowY + (top + size), size, alpha);
	DrawPCRecycleLabel(no, textX, rowY + (2.0f * size + top), size, alpha);
}

enum PCShopListKind { PriceList, ItemPriceList };

static inline f32 DrawPCPurchasePrice(PCShopListKind kind, CTexObj& tex, MenuPCPage& page, s32 selected, f32 opacity, f32 left, f32 top,
                                      f32 scale, f32 extraW, f32 extraH)
{
	InitPCPanelTexture(tex, 126);
	f32 w = scale * GXGetTexObjWidth(&tex), h = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	f32 digitY         = extraH * scale + (156.0f * scale + top);
	for (int i = 0; i < 4; i++) {
		f32 divisor = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++) {
				divisor *= 10.0f;
			}
		}
		s32 digit = (s32)(page.mRows[selected].mPrice / divisor);
		if (i && digit == 0) {
			continue;
		}
		PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * (digit % 10), 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		u32 upper  = (u8)opacity | (kind == ItemPriceList ? 0xEFEBC500 : 0xCECEFF00);
		u32 lower  = (u8)opacity | (kind == ItemPriceList ? 0xEDA83600 : 0x5959FF00);
		f32 digitX = 287.0f * scale + (left - i * w);
		digitX     = (extraW / 2.0f) * scale + (18.0f * scale + digitX);
		DrawGsTexturedQuad(&gGame.mGs, &tex, upper, upper, lower, lower, digitX, digitY, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
	return digitY;
}

static inline void DrawPCPriceUnit(PCShopListKind kind, CTexObj& tex, f32 scale, f32 left, f32 digitY, f32 opacity)
{
	InitPCPanelTexture(tex, kind == ItemPriceList ? 145 : 149);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 unitW = GXGetTexObjWidth(&tex), unitH = GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)opacity | 0xFFFFFF00, (u32)opacity | 0xFFFFFF00, (u32)opacity | 0xFFFFFF00,
	                   (u32)opacity | 0xFFFFFF00, 287.0f * scale + (18.0f * scale + left), digitY, 0.0f, 1.1f * unitW, unitH, 0.0f, 0.0f,
	                   0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

static inline void DrawPCChoiceStripe(f32 x, f32 y, f32 barWidth, f32 barHeight, f32 alpha, f32 scroll)
{
	CTexObj stripe;
	InitPCPanelTexture(stripe, 119);
	PSMTXTrans(stripe.mTexMtx.mMtx[0], scroll, 0.0f, 0.0f);
	stripe.mTexMtx.mDirty = TRUE;
	PSMTXScale(stripe.mTexMtx.mMtx[2], 0.5f * (barWidth / GXGetTexObjWidth(&stripe)), 0.5f, 1.0f);
	stripe.mTexMtx.mDirty = TRUE;
	DrawOverviewTexture(stripe, (u32)(40.0f * alpha) | 0xFFFFFF00, x, y, barWidth, barHeight);
}

static inline void DrawPCChoicePiece(CTexObj& tex, u32 color, f32 u, f32 x, f32 y, f32 width, f32 height)
{
	PSMTXTrans(tex.mTexMtx.mMtx[0], u, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
}

static inline void DrawPCChoiceCaps(CTexObj& tex, f32 cx, f32 y, f32 barWidth, f32 barHeight, f32 scale, f32 alpha)
{
	f32 gap   = 4.0f * scale;
	u32 color = (u32)(200.0f * alpha) | 0x181CC600;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	InitPCPanelTexture(tex, 118);
	PSMTXScale(tex.mTexMtx.mMtx[2], 0.25f, 1.0f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	DrawPCChoicePiece(tex, color, 0.0f, cx, y, barWidth, barHeight);
	DrawPCChoicePiece(tex, color, 0.0f, (cx - barWidth / 2.0f) - gap, y, 8.0f * scale, barHeight);
	DrawPCChoicePiece(tex, color, 0.5f, gap + (cx + barWidth / 2.0f), y, 8.0f * scale, barHeight);
}

static inline void DrawPCRecycleUnit(f32 originX, f32 originY, f32 scale, f32 alpha)
{
	const char* units = GetLocalizedInterfaceText(0x50);
	DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xC8C8C800, units, 216.0f * scale + originX, 147.0f * scale + originY,
	                             0.0f, 18.0f * scale, 18.0f * scale);
}

// NONMATCHING: one colour conversion comes a call later than in retail; this build is 64 bytes larger and the stack layout and
// registers differ
extern "C" void DrawPCRecyclePanel(MenuPC* menu, s32 choice, f32 x, f32 y, f32 scrap, f32 money, f32 scroll, f32 alpha, f32 scale)
{
	CTexObj tex;
	f32 width = 296.0f * scale, height = 156.0f * scale;
	f32 left = x - width / 2.0f, top = y - height / 2.0f;
	f32 right = width + left, bottom = height + top;
	Vec panel[5] = { (Vec) { left, top, -10.0f }, (Vec) { right, top, -10.0f }, (Vec) { right, bottom, -10.0f },
		             (Vec) { left, bottom, -10.0f }, (Vec) { left, top, -10.0f } };
	u32 opacity  = (u32)(255.0f * alpha);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, panel, opacity, 5);
	y += scale;
	f32 size        = 17.0f * scale;
	f32 labelOffset = 155.0f * scale + 20.0f * scale, rowOffset = 204.0f * scale;
	f32 originX = left - width / 4.0f, originY = (y - height) - height * 0.25f;
	if (choice != 2) {
		f32 inset    = 2.0f * scale;
		f32 cx       = PCRecycleChoiceX(originX, size, scale, 20.0f * scale, labelOffset);
		f32 cy       = (rowOffset + ((size / 2.0f + (size * (choice + 2) + originY)) - inset)) - size;
		f32 barWidth = 0.9f * (4.0f * size), barHeight = size + scale;
		DrawPCChoiceCaps(tex, cx, cy + 2.0f * scale, barWidth, barHeight, scale, alpha);
		DrawPCChoiceStripe(cx + 2.0f * scale, cy + 2.0f * scale, barWidth, barHeight - 4.0f * scale, alpha, scroll);
	}
	DrawPCBlueNumber(false, (130.0f * scale + originX) - 60.0f * scale, originY + height, alpha, scale, scrap);
	DrawPCRecycleUnit(originX, originY, scale, menu->mAlpha);
	int places = 1;
	for (;; places++) {
		f32 divisor = 1.0f;
		for (int j = 0; j < places; j++) {
			divisor *= 10.0f;
		}
		if ((s32)((s32)money / divisor) == 0) {
			break;
		}
	}
	InitPCPanelTexture(tex, 126);
	f32 digitWidth = GXGetTexObjWidth(&tex);
	f32 numberX    = places * (scale * digitWidth) + (120.0f * scale + originX);
	DrawPCGoldNumber(false, numberX, originY + height, alpha, scale, money);
	if (choice != 2) {
		f32 textX = PCRecycleChoiceX(originX, size, scale, 20.0f * scale, labelOffset);
		DrawPCRecycleLabel(0x4E, textX, (rowOffset + (-1 * size + originY)) - 4.0f * scale, size, alpha);
		DrawPCRecycleLabel(0x4F, textX, (originY + rowOffset) - 4.0f * scale, size, alpha);
		DrawPCRecycleLabel(0x41, textX, rowOffset + (originY + size), size, alpha);
		DrawPCRecycleLabel(0x42, textX, rowOffset + (2 * size + originY), size, alpha);
	} else {
		const char* text = GetLocalizedInterfaceText(0x48);
		f32 textWidth    = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x48), TRUE, size);
		DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xFFFFFF00, text,
		                             PCRecycleChoiceX(originX, size, scale, 20.0f * scale, labelOffset) - textWidth / 2.0f,
		                             (originY + rowOffset) - 4.0f * scale, 0.0f, size, size);
	}
}

// NONMATCHING: the call list matches retail; this build is 52 bytes smaller and the stack layout and registers differ
extern "C" void DrawPCItemPurchasePanel(s32 selected, s32 choice, f32 x, f32 y, f32 scroll, f32 alpha, f32 scale)
{
	static u32 frame = 0;
	++frame;
	CTexObj tex;
	if (scale <= 0.0f) {
		return;
	}
	f32 extraH = IsJapanese() ? 0 : 17;
	f32 extraW = IsJapanese() ? 0 : 8;
	f32 width = (246.0f + extraW) * scale, height = (153.0f + extraH) * scale;
	f32 left    = (x - width / 2.0f) - 34.0f * scale;
	f32 top     = ((y - extraH / 2.0f) - height / 2.0f) - 13.0f * scale;
	s16 id      = gItemShopPage.mRows[selected].mItem;
	f32 opacity = 255.0f * alpha;
	DrawTexturedWindowFrame((u32)(255.0f * alpha), (u32)(255.0f * alpha) | 0xFFFFFF00, x, top + 13.0f * scale, width, height, alpha, 1.0f,
	                        1.0f);
	top += scale;
	f32 size = 17.0f * scale, offset = 130.0f * scale + 20.0f * scale, rowY = 24.0f * scale;
	char text[256];
	if (choice == 2) {
		DrawPCPurchaseResult(0x3F, x, 4.0f * scale + (rowY + (5.0f * size + top)), size, (u32)(255.0f * alpha) | 0xFFFFFF00);
	} else {
		f32 cx       = (extraW / 2.0f) * scale + PCRecycleChoiceX(left, size, scale, 20.0f * scale, offset);
		f32 cy       = (rowY + ((size / 2.0f + (size * (choice + 2) + top)) - 2.0f * scale)) - size;
		f32 barWidth = 0.9f * (4.0f * size), barHeight = size + scale;
		DrawPCChoiceCaps(tex, cx, cy + 2.0f * scale, barWidth, barHeight, scale, alpha);
		DrawPCChoiceStripe(cx + 2.0f * scale, cy + 2.0f * scale, barWidth, barHeight - 4.0f * scale, alpha, scroll);
	}
	if (id < 0) {
		return;
	}
	if (gItemShopPage.mRows[selected].mIsAttachment) {
		sprintf(text, "%s", GetAttachmentDescription(id));
		if (choice != 2) {
			DrawPCShopCenteredText(text, (u32)(255.0f * alpha) | 0xFFFFFF00, x,
			                       (4.0f * scale + (rowY + (5.0f * size + top))) - 3.0f * scale, size);
		}
		sprintf(text, "%s", GetAttachmentName(id));
	} else {
		sprintf(text, "%s", GetItemDescription(id));
		if (choice != 2) {
			DrawPCShopCenteredText(text, (u32)(255.0f * alpha) | 0xFFFFFF00, x,
			                       (4.0f * scale + (rowY + (5.0f * size + top))) - 3.0f * scale, size);
		}
		sprintf(text, "%s", GetItemName(id));
	}
	DrawPCShopCenteredText(text, (u32)(255.0f * alpha) | 0xFFAA0000, x, (rowY + (4.0f * size + top)) - 3.0f * scale, 2.0f * scale + size);
	if (choice != 2) {
		f32 digitY = DrawPCPurchasePrice(ItemPriceList, tex, gItemShopPage, selected, opacity, left, top, scale, extraW, extraH);
		DrawPCPriceUnit(ItemPriceList, tex, scale, left, digitY, 255.0f * alpha);
		DrawPCPurchaseLabels(0x40, 0x41, 0x42, left, top, size, scale, 20.0f * scale, offset, rowY, extraW, alpha);
	}
	s32 icon;
	f32 w, h;
	if (gItemShopPage.mRows[selected].mIsAttachment) {
		icon = id == 2 ? 92 : 93;
		InitPCPanelTexture(tex, icon);
		w = 0.8f * (scale * GXGetTexObjWidth(&tex));
		h = 0.8f * (scale * GXGetTexObjHeight(&tex));
	} else {
		icon = GetItemIconTextureIndex(id);
		InitPCPanelTexture(tex, icon);
		w = scale * GXGetTexObjWidth(&tex);
		h = scale * GXGetTexObjHeight(&tex);
	}
	if (icon >= 0) {
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
		DrawOverviewTexture(tex, color, 52.0f * scale + left, 48.0f * scale + top, w, h);
	}
}

// NONMATCHING: one colour conversion comes a call later than in retail; this build is 48 bytes smaller and the stack layout and
// registers differ
extern "C" void DrawPCScrapPurchasePanel(s32 selected, s32 choice, f32 x, f32 y, f32 scroll, f32 alpha, f32 scale)
{
	static u32 frame = 0;
	++frame;
	CTexObj tex;
	if (scale <= 0.0f) {
		return;
	}
	f32 extraH = IsJapanese() ? 0 : 17;
	f32 extraW = IsJapanese() ? 0 : 8;
	f32 width = (246.0f + extraW) * scale, height = (153.0f + extraH) * scale;
	f32 left    = (x - width / 2.0f) - 34.0f * scale;
	f32 top     = ((y - extraH / 2.0f) - height / 2.0f) - 13.0f * scale;
	s16 id      = gScrapShopPage.mRows[selected].mItem;
	f32 opacity = 255.0f * alpha;
	DrawTexturedWindowFrame((u32)(255.0f * alpha), (u32)(255.0f * alpha) | 0xFFFFFF00, x, top + 13.0f * scale, width, height, alpha, 1.0f,
	                        1.0f);
	top += scale;
	f32 size = 17.0f * scale, offset = 130.0f * scale + 20.0f * scale, rowY = 24.0f * scale;
	char text[256];
	if (choice == 2) {
		DrawPCPurchaseResult(0x52, x, 4.0f * scale + (rowY + (5.0f * size + top)), size, (u32)(255.0f * alpha) | 0xFFFFFF00);
	} else {
		f32 cx       = (extraW / 2.0f) * scale + PCRecycleChoiceX(left, size, scale, 20.0f * scale, offset);
		f32 cy       = (rowY + ((size / 2.0f + (size * (choice + 2) + top)) - 2.0f * scale)) - size;
		f32 barWidth = 0.9f * (4.0f * size), barHeight = size + scale;
		DrawPCChoiceCaps(tex, cx, cy + 2.0f * scale, barWidth, barHeight, scale, alpha);
		DrawPCChoiceStripe(cx + 2.0f * scale, cy + 2.0f * scale, barWidth, barHeight - 4.0f * scale, alpha, scroll);
	}
	if (id < 0) {
		return;
	}
	sprintf(text, "%s", GetUtilibotDescription(id));
	if (choice != 2) {
		DrawPCShopCenteredText(text, (u32)(255.0f * alpha) | 0xFFFFFF00, x, (4.0f * scale + (rowY + (5.0f * size + top))) - 3.0f * scale,
		                       size);
	}
	sprintf(text, "%s", GetUtilibotName(id));
	DrawPCShopCenteredText(text, (u32)(255.0f * alpha) | 0xFFAA0000, x, (rowY + (4.0f * size + top)) - 3.0f * scale, 2.0f * scale + size);
	if (choice != 2) {
		f32 digitY = DrawPCPurchasePrice(PriceList, tex, gScrapShopPage, selected, opacity, left, top, scale, extraW, extraH);
		DrawPCPriceUnit(PriceList, tex, scale, left, digitY, 255.0f * alpha);
		DrawPCPurchaseLabels(0x51, 0x41, 0x42, left, top, size, scale, 20.0f * scale, offset, rowY, extraW, alpha);
	}
	f32 iconX = 52.0f * scale + left, iconY = 48.0f * scale + top;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	s32 icon;
	if (id == 0 || !(gEventFlags.mFlags[335 / 32] & (1u << (335 % 32)))) {
		icon = 239;
	} else {
		icon = 240;
	}
	InitPCPanelTexture(tex, icon);
	f32 w = GXGetTexObjWidth(&tex), h = GXGetTexObjHeight(&tex);
	u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
	DrawOverviewTexture(tex, color, iconX, iconY, scale * w, scale * h);
}

// NONMATCHING: the call list matches retail; this build is 4 bytes smaller and the stack layout and registers differ
extern "C" void DrawPCItemPriceRow(s16 id, s16 count, s16 value, u8 attachment, f32 x, f32 y, f32 alpha, f32 scale)
{
	CTexObj tex;
	f32 fontSize = 17.0f * scale;
	if (id < 0) {
		return;
	}
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[147], 0);
	tex = gGame.mTpl.mTex;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 w = 0.7f * (scale * (1.1f * GXGetTexObjWidth(&tex)));
	f32 h = 0.7f * (scale * GXGetTexObjHeight(&tex));
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
	                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x - w / 2.0f, 2.0f * scale + (h / 2.0f + y),
	                   0.0f, w, h, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	char text[256];
	if (attachment) {
		sprintf(text, "%s", GetAttachmentName(id));
	} else {
		sprintf(text, "%s", GetItemName(id));
	}
	u32 color;
	if (count <= 0) {
		color = (u32)(0.4f * (255.0f * alpha)) | 0xFFFFFF00;
	} else {
		color = (u32)(255.0f * alpha) | 0xFFFFFF00;
	}
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, 3.0f * scale + x, y, 0.0f, fontSize, fontSize);
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
	tex = gGame.mTpl.mTex;
	w   = scale * GXGetTexObjWidth(&tex);
	h   = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	for (int i = 0; i < 4; i++) {
		f32 div = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++) {
				div *= 10.0f;
			}
		}
		s32 n = value / div;
		if (i == 0 || n != 0) {
			n %= 10;
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * n, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			u32 top, bottom;
			if (count <= 0) {
				top    = (u8)(0.4f * (255.0f * alpha)) | 0xEFEBC500;
				bottom = (u8)(0.4f * (255.0f * alpha)) | 0xEDA83600;
			} else {
				top    = (u8)(255.0f * alpha) | 0xEFEBC500;
				bottom = (u8)(255.0f * alpha) | 0xEDA83600;
			}
			f32 digitX = 80.0f * scale + (12.0f * scale + (67.0f * scale + (7.0f * w + (x - i * w))));
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, digitX, 8.0f * scale + y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[145], 0);
	tex = gGame.mTpl.mTex;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 iconW = GXGetTexObjWidth(&tex);
	f32 iconH = GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 80.0f * scale + (12.0f * scale + (203.0f * scale + x)),
	                   9.0f * scale + y, 0.0f, scale * (1.1f * iconW), scale * iconH, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// NONMATCHING: the call list matches retail; this build is 8 bytes smaller and the stack layout and registers differ
extern "C" void DrawPCPriceRow(s16 id, s16 count, s16 value, f32 x, f32 y, f32 alpha, f32 scale)
{
	CTexObj tex;
	f32 fontSize = 17.0f * scale;
	if (id < 0) {
		return;
	}
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[142], 0);
	tex = gGame.mTpl.mTex;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 w = 0.7f * (scale * (1.1f * GXGetTexObjWidth(&tex)));
	f32 h = 0.7f * (scale * GXGetTexObjHeight(&tex));
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
	                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x - w / 2.0f, 2.0f * scale + (h / 2.0f + y),
	                   0.0f, w, h, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	if (id >= 20) {
		id = 0;
	}
	char text[256];
	sprintf(text, "%s", GetUtilibotName(id));
	u32 color;
	if (count <= 0) {
		color = (u32)(0.4f * (255.0f * alpha)) | 0xFFFFFF00;
	} else {
		color = (u32)(255.0f * alpha) | 0xFFFFFF00;
	}
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, 3.0f * scale + x, y, 0.0f, fontSize, fontSize);
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
	tex = gGame.mTpl.mTex;
	w   = scale * GXGetTexObjWidth(&tex);
	h   = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	for (int i = 0; i < 4; i++) {
		f32 div = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++) {
				div *= 10.0f;
			}
		}
		s32 n = value / div;
		if (i == 0 || n != 0) {
			n %= 10;
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * n, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			u32 top, bottom;
			if (count <= 0) {
				top    = (u8)(0.4f * (255.0f * alpha)) | 0xCECEFF00;
				bottom = (u8)(0.4f * (255.0f * alpha)) | 0x5959FF00;
			} else {
				top    = (u8)(255.0f * alpha) | 0xCECEFF00;
				bottom = (u8)(255.0f * alpha) | 0x5959FF00;
			}
			f32 digitX = 80.0f * scale + (12.0f * scale + (67.0f * scale + (7.0f * w + (x - i * w))));
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, digitX, 8.0f * scale + y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[149], 0);
	tex = gGame.mTpl.mTex;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 iconW = GXGetTexObjWidth(&tex);
	f32 iconH = GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 80.0f * scale + (12.0f * scale + (203.0f * scale + x)),
	                   8.5f * scale + y, 0.0f, 0.9f * (scale * (1.1f * iconW)), 0.9f * (scale * iconH), 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

static inline void DrawPCListRow(PCShopListKind kind, MenuPCItem& row, f32 x, f32 y, f32 alpha, f32 scale)
{
	if (kind == ItemPriceList) {
		DrawPCItemPriceRow(row.mItem, row.mCount, row.mPrice, row.mIsAttachment, x, y, alpha, scale);
	} else {
		DrawPCPriceRow(row.mItem, row.mCount, row.mPrice, x, y, alpha, scale);
	}
}

static inline void DrawPCListMask(f32 top, f32 height, u32 color)
{
	f32 left = 130.0f, right = 510.0f;
	Vec mask[5] = { (Vec) { left, top, -10.0f }, (Vec) { right, top, -10.0f }, (Vec) { right, top + height, -10.0f },
		            (Vec) { left, top + height, -10.0f }, (Vec) { left, top, -10.0f } };
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, mask, color, 5);
}

static inline s32 PCShopMaxPage(MenuPCItem*& table)
{
	s16 item, quantity;
	int i;
	s32 pages = 0;
	for (i = 0; i < 20; table++, i++) {
		item     = table->mItem;
		quantity = table->mCount;
		if (item >= 0 && quantity >= 0) {
			pages++;
		}
	}
	pages -= 8;
	if (pages < 1) {
		pages = 1;
	}
	return pages;
}

static inline s32 CountPCShopItems(MenuPCItem* table)
{
	s16 item;
	s16 quantity;
	int i;
	s32 count = 0;
	for (i = 0; i < 20; i++) {
		item     = table[i].mItem;
		quantity = table[i].mCount;
		if (item >= 0 && quantity >= 0) {
			count++;
		}
	}
	return count;
}

static inline void InitMenuSelectionBar(CTexObj& tex)
{
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	InitPCPanelTexture(tex, 118);
	PSMTXScale(tex.mTexMtx.mMtx[2], 0.25f, 1.0f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.25f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
}

static inline void DrawMenuSelectionBar(CTexObj& tex, f32 barX, f32 barY, f32 barW, f32 barH, f32 alpha, f32 textureOffset, u32 barColor)
{
	DrawGsTexturedQuad(&gGame.mGs, &tex, barColor, barColor, barColor, barColor, barX, barY, 0.0f, barW, barH, 0.0f, 0.0f, 0.0f);
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, barColor, barColor, barColor, barColor, (barX - barW / 2.0f) - 4.0f, barY, 0.0f, 8.0f, barH, 0.0f,
	                   0.0f, 0.0f);
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.5f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, barColor, barColor, barColor, barColor, 4.0f + (barX + barW / 2.0f), barY, 0.0f, 8.0f, barH, 0.0f,
	                   0.0f, 0.0f);
	InitPCPanelTexture(tex, 119);
	PSMTXTrans(tex.mTexMtx.mMtx[0], textureOffset, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	PSMTXScale(tex.mTexMtx.mMtx[2], 0.5f * (barW / GXGetTexObjWidth(&tex)), 0.5f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(40.0f * alpha) | 0xFFFFFF00, (u32)(40.0f * alpha) | 0xFFFFFF00,
	                   (u32)(40.0f * alpha) | 0xFFFFFF00, (u32)(40.0f * alpha) | 0xFFFFFF00, 2.0f + barX, barY, 0.0f, barW, barH - 4.0f,
	                   0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// NONMATCHING: the call list matches retail; this build is 400 bytes larger and register allocation differs
void DrawPCItemShopList(s32 selected, f32 x, f32 y, f32 textureOffset, f32 alpha, f32 scale, f32 scroll)
{
	CTexObj tex;
	y -= scroll;
	f32 left;
	f32 width = 296.0f * scale, height = 156.0f * scale;
	left        = x - width / 2.0f;
	Vec bg[5]   = { (Vec) { left, y - height / 2.0f, -10.0f }, (Vec) { width + left, y - height / 2.0f, -10.0f },
		            (Vec) { width + left, height + (y - height / 2.0f), -10.0f }, (Vec) { left, height + (y - height / 2.0f), -10.0f },
		            (Vec) { left, y - height / 2.0f, -10.0f } };
	u32 opacity = (u32)(255.0f * alpha);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, bg, opacity, 5);
	y += scale;
	f32 step = 17.0f * scale;
	f32 rowY = y - height / 2.0f;
	f32 barX, barW;
	if (IsJapanese()) {
		barX = (10.0f * step / 2.0f + (left + step)) - 8.0f * scale;
		barW = 0.9f * (14.0f * step);
	} else {
		InitPCPanelTexture(tex, 147);
		barX = x - 4.0f;
		barW = (0.7f * (scale * (1.1f * (2 * GXGetTexObjWidth(&tex)))) + width) - 5.0f * scale;
	}
	f32 barH     = step + scale;
	u32 barColor = (u32)(200.0f * alpha) | 0x181CC600;
	InitMenuSelectionBar(tex);
	f32 barY = 2.0f + ((step / 2.0f + (step * selected + rowY)) - 2.0f * scale);
	DrawMenuSelectionBar(tex, barX, barY, barW, barH, alpha, textureOffset, barColor);
	for (int i = 0; i < 9; i++) {
		DrawPCListRow(ItemPriceList, gItemShopPage.mRows[i], left, step * i + rowY, alpha, scale);
	}
	MenuPCItem* items = sItemShopEntries;
	if (gItemShopPage.mStart < PCShopMaxPage(items) - 1) {
		DrawPCListRow(ItemPriceList, gItemShopPage.mNext[0], left, 9.0f * step + rowY, alpha, scale);
	}
	CountPCShopItems(sItemShopEntries);
	if (gItemShopPage.mStart > 0) {
		DrawPCListRow(ItemPriceList, gItemShopPage.mPrev[0], left, rowY - step, alpha, scale);
	}
	DrawPCListMask(97.0f - scroll, 40.0f, (u32)(255.0f * alpha) | 0x0A0A0A00);
	DrawPCListMask(292.5f - scroll, 40.0f + scroll, opacity | 0x0A0A0A00);
}

// NONMATCHING: the call list matches retail; this build is 400 bytes larger and register allocation differs
void DrawPCScrapShopList(s32 selected, f32 x, f32 y, f32 textureOffset, f32 alpha, f32 scale, f32 scroll)
{
	CTexObj tex;
	y -= scroll;
	f32 left;
	f32 width = 296.0f * scale, height = 156.0f * scale;
	left        = x - width / 2.0f;
	Vec bg[5]   = { (Vec) { left, y - height / 2.0f, -10.0f }, (Vec) { width + left, y - height / 2.0f, -10.0f },
		            (Vec) { width + left, height + (y - height / 2.0f), -10.0f }, (Vec) { left, height + (y - height / 2.0f), -10.0f },
		            (Vec) { left, y - height / 2.0f, -10.0f } };
	u32 opacity = (u32)(255.0f * alpha);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, bg, opacity, 5);
	y += scale;
	f32 step = 17.0f * scale;
	f32 rowY = y - height / 2.0f;
	f32 barX, barW;
	if (IsJapanese()) {
		barX = (10.0f * step / 2.0f + (left + step)) - 8.0f * scale;
		barW = 0.9f * (14.0f * step);
	} else {
		InitPCPanelTexture(tex, 147);
		barX = x - 4.0f;
		barW = (0.7f * (scale * (1.1f * (2 * GXGetTexObjWidth(&tex)))) + width) - 5.0f * scale;
	}
	f32 barH     = step + scale;
	u32 barColor = (u32)(200.0f * alpha) | 0x181CC600;
	InitMenuSelectionBar(tex);
	f32 barY = 2.0f + ((step / 2.0f + (step * selected + rowY)) - 2.0f * scale);
	DrawMenuSelectionBar(tex, barX, barY, barW, barH, alpha, textureOffset, barColor);
	for (int i = 0; i < 9; i++) {
		DrawPCListRow(PriceList, gScrapShopPage.mRows[i], left, step * i + rowY, alpha, scale);
	}
	MenuPCItem* items = sScrapShopEntries;
	if (gScrapShopPage.mStart < PCShopMaxPage(items) - 1) {
		DrawPCListRow(PriceList, gScrapShopPage.mNext[0], left, 9.0f * step + rowY, alpha, scale);
	}
	CountPCShopItems(sScrapShopEntries);
	if (gScrapShopPage.mStart > 0) {
		DrawPCListRow(PriceList, gScrapShopPage.mPrev[0], left, rowY - step, alpha, scale);
	}
	DrawPCListMask(97.0f - scroll, 40.0f, (u32)(255.0f * alpha) | 0x0A0A0A00);
	DrawPCListMask(292.5f - scroll, 40.0f + scroll, opacity | 0x0A0A0A00);
}

static inline void DrawPCNumberSuffix(CTexObj& tex, int index, f32 x, f32 y, f32 scale, f32 opacity)
{
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[index], 0);
	tex = gGame.mTpl.mTex;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 w = scale * (1.1f * GXGetTexObjWidth(&tex));
	f32 h = scale * GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)opacity | 0xFFFFFF00, (u32)opacity | 0xFFFFFF00, (u32)opacity | 0xFFFFFF00,
	                   (u32)opacity | 0xFFFFFF00, 128.0f * scale + x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// NONMATCHING: the call list matches; retail needs 36 more bytes and two more saved GPRs, with different register allocation
extern "C" void DrawPCGoldNumber(bool icon, f32 x, f32 y, f32 alpha, f32 scale, f32 value)
{
	CTexObj tex;
	f32 opacity;
	if (icon) {
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[153], 0);
		tex = gGame.mTpl.mTex;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		f32 w = 0.8f * (scale * GXGetTexObjWidth(&tex));
		f32 h = 0.8f * (scale * GXGetTexObjHeight(&tex));
		DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
		                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x - 16.0f, y, 0.0f, w, h, 0.0f, 0.0f,
		                   0.0f);
	}
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
	tex   = gGame.mTpl.mTex;
	f32 w = scale * GXGetTexObjWidth(&tex);
	f32 h = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	opacity            = 255.0f * alpha;
	u8 a               = opacity;
	for (int i = 0; i < 7; i++) {
		u32 top    = a | 0xEFEBC500;
		u32 bottom = a | 0xEDA83600;
		f32 div    = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++) {
				div *= 10.0f;
			}
		}
		s32 n = value / div;
		if (i == 0 || n != 0) {
			n %= 10;
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * n, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			f32 digitX = 7.0f * w + (x - i * w) - 8.0f * scale;
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, digitX, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
	DrawPCNumberSuffix(tex, 145, x, y, scale, opacity);
}

// NONMATCHING: the call list matches; retail needs 36 more bytes and two more saved GPRs, with different register allocation
extern "C" void DrawPCBlueNumber(bool icon, f32 x, f32 y, f32 alpha, f32 scale, f32 value)
{
	CTexObj tex;
	f32 opacity;
	if (icon) {
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[154], 0);
		tex = gGame.mTpl.mTex;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		f32 w = 0.65f * (scale * GXGetTexObjWidth(&tex));
		f32 h = 0.65f * (scale * GXGetTexObjHeight(&tex));
		DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
		                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x - 16.0f, y, 0.0f, w, h, 0.0f, 0.0f,
		                   0.0f);
	}
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
	tex   = gGame.mTpl.mTex;
	f32 w = scale * GXGetTexObjWidth(&tex);
	f32 h = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	opacity            = 255.0f * alpha;
	u8 a               = opacity;
	for (int i = 0; i < 7; i++) {
		u32 top    = a | 0xCECEFF00;
		u32 bottom = a | 0x5959FF00;
		f32 div    = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++) {
				div *= 10.0f;
			}
		}
		s32 n = value / div;
		if (i == 0 || n != 0) {
			n %= 10;
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * n, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			f32 digitX = 7.0f * w + (x - i * w) - 8.0f * scale;
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, digitX, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
	DrawPCNumberSuffix(tex, 149, x, y, scale, opacity);
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
	u32 color       = (u32)(255.0f * alpha);
	DrawGsLineStrip(&gGame.mGs, quads[0], color | 0xC8C8C800, 5);
	gGame.mGs.mCurRenderState->SetLineWidth(6, GX_TO_ONE);
	DrawSolidQuad(&gGame.mGs, quads[1], color);
	DrawSolidQuad(&gGame.mGs, quads[2], color | 0xFFB62800);
	DrawSolidQuad(&gGame.mGs, quads[3], color);
	DrawGsLineStrip(&gGame.mGs, quads[2], color | 0xFFFFFF00, 5);
	gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
}

static inline const char* GetPCMenuLabelText(s32 id)
{
	return GetLocalizedInterfaceText(id + 46);
}

static inline const char* GetPCDialogueText(s32 id)
{
	return GetLocalizedPCDialogue(id);
}

static inline s32 CountDecimalDigits(s32 value)
{
	s32 digits = 1;
	while (true) {
		f32 divisor = 1.0f;
		for (s32 i = 0; i < digits; i++) {
			divisor *= 10.0f;
		}
		if ((s32)(value / divisor) == 0) {
			break;
		}
		digits++;
	}
	return digits;
}

#define PC_EASE_CALL(value, target)                  \
	do {                                             \
		f32 speed = (target) - (value);              \
		f32 rate  = 0.25f;                           \
		rate      = speed * rate;                    \
		if (rate < 0.0f)                             \
			rate = -rate;                            \
		if (rate <= 1.0f)                            \
			rate = 1.0f;                             \
		ApproachHudValue(&(value), &(target), rate); \
	} while (0)

#define PC_DIGIT_DIVISOR(divisor, i)                  \
	do {                                              \
		divisor = 1.0f;                               \
		if ((i) > 0) {                                \
			for (s32 place = 0; place < (i); ++place) \
				divisor *= 10.0f;                     \
		}                                             \
	} while (0)

#define PC_READ_DIALOGUE_GLYPH(text) ((((const u8*)(text))[0] << 8) | ((const u8*)(text))[1])
#define PC_SET_DIALOGUE_GLYPH(menu, code)                        \
	do {                                                         \
		*(u16*)(menu)->mCharCopies[(menu)->mNextGlyph] = (code); \
		(menu)->mCharCopies[(menu)->mNextGlyph][2]     = 0;      \
	} while (0)

// NONMATCHING: the call list matches retail; this build is 28 bytes smaller and register allocation differs
BOOL MenuPC::Draw()
{
	if (CMenu::Draw()) {
		CTexObj tex;
		char labelText[256];
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
		{
			f32 w = 380.0f, h = 320.0f;
			f32 left = 320.0f - w / 2.0f, top = 200.0f - h / 2.0f;
			Vec vertices[] = { (Vec) { left, top, -10.0f }, (Vec) { w + left, top, -10.0f }, (Vec) { w + left, h + top, -10.0f },
				               (Vec) { left, h + top, -10.0f }, (Vec) { left, top, -10.0f } };
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
			DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, vertices, (u32)(255.0f * mAlpha), 5);
		}
		switch (mMode) {
		case 1: {
			f32 left = 260.0f, top = 160.0f, right = 380.0f, bottom = 240.0f;
			Vec panel[] = { (Vec) { left, top, -10.0f }, (Vec) { right, top, -10.0f }, (Vec) { right, bottom, -10.0f },
				            (Vec) { left, bottom, -10.0f }, (Vec) { left, top, -10.0f } };
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
			DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, panel, (u32)(255.0f * mAlpha) | 0xFFB62800, 5);
			DrawGsLineStrip(&gGame.mGs, panel, (u32)(255.0f * mAlpha) | 0xC8C8C800, 5);
			{
				InitPCPanelTexture(tex, 143);
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				const f32 scale = 1.4f;
				f32 width       = scale * (1.1f * GXGetTexObjWidth(&tex));
				f32 height      = scale * GXGetTexObjHeight(&tex);
				u32 color       = (u32)(255.0f * mAlpha) | 0xC8C8C800;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, 193.0f, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
			if (IsJapanese()) {
				InitPCPanelTexture(tex, 144);
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				f32 width  = 1.1f * GXGetTexObjWidth(&tex);
				f32 height = GXGetTexObjHeight(&tex);
				u32 color  = (u32)(255.0f * mAlpha) | 0xC8C8C800;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, 227.0f, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
			SetGsChannelEnabled(&gGame.mGs, 0, 0);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
			f32 fill    = 281.0f + 80.0f * mProgress;
			Vec bar[12] = {};
			bar[0].x    = 281.0f;
			bar[0].y    = 249.0f;
			bar[1].x    = 361.0f;
			bar[1].y    = 249.0f;
			bar[2].x    = 361.0f;
			bar[2].y    = 253.0f;
			bar[3].x    = 281.0f;
			bar[3].y    = 253.0f;
			bar[4].x    = 281.0f;
			bar[4].y    = 249.0f;
			bar[5].x    = fill;
			bar[5].y    = 249.0f;
			bar[6].x    = fill;
			bar[6].y    = 253.0f;
			bar[7].x    = 281.0f;
			bar[7].y    = 253.0f;
			bar[8].x    = fill;
			bar[8].y    = 249.0f;
			bar[9].x    = 361.0f;
			bar[9].y    = 249.0f;
			bar[10].x   = 361.0f;
			bar[10].y   = 253.0f;
			bar[11].x   = fill;
			bar[11].y   = 253.0f;
			DrawGsLineStrip(&gGame.mGs, bar, (u32)(255.0f * mAlpha) | 0xC8C8C800, 5);
			gGame.mGs.mCurRenderState->SetLineWidth(6, GX_TO_ONE);
			DrawSolidQuad(&gGame.mGs, bar + 4, (u32)(255.0f * mAlpha) | 0xFFB62800);
			DrawSolidQuad(&gGame.mGs, bar + 8, (u32)(255.0f * mAlpha) | 0xC8000000);
			gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
		} break;
		case 2: {
			f32 origin, step;
			switch (mChoiceCount) {
			case 1:
			case 2:
			case 4:
				origin = 170.0f;
				step   = 36.0f;
				break;
			case 3:
				origin = 180.0f;
				step   = 40.0f;
				break;
			default:
				origin = 0.0f;
				step   = 0.0f;
				break;
			}
			const f32 top = origin - 20.0f;
			f32 center, width;
			if (IsJapanese()) {
				sprintf(labelText, "%s", GetPCMenuLabelText(pcChoiceArt[mChoices[mChoice]].label));
				u32 chars = strlen(labelText) / 2;
				center    = 224.0f + 0.5f * ((chars - 1) * 20.0f);
				width     = 0.9f * ((chars + 2) * 20.0f);
			} else {
				InitPCPanelTexture(tex, pcChoiceArt[mChoices[mChoice]].icon);
				f32 iconWidth = 1.1f * GXGetTexObjWidth(&tex);
				width
				    = 6.0f
				    + (iconWidth + MeasureGameTextWidth(&gGame.mGs, GetPCMenuLabelText(pcChoiceArt[mChoices[mChoice]].label), TRUE, 20.0f));
				center = 204.0f + width / 2.0f;
			}
			f32 y     = 2.0f + (step * mChoice + (8.0f + top));
			u32 color = (u32)(200.0f * mAlpha) | 0x181CC600;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			InitPCPanelTexture(tex, 118);
			PSMTXScale(tex.mTexMtx.mMtx[2], 0.25f, 1.0f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.25f, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, center, y, 0.0f, width, 24.0f, 0.0f, 0.0f, 0.0f);
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, (center - width / 2.0f) - 4.0f, y, 0.0f, 8.0f, 24.0f, 0.0f,
			                   0.0f, 0.0f);
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.5f, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 4.0f + (center + width / 2.0f), y, 0.0f, 8.0f, 24.0f, 0.0f,
			                   0.0f, 0.0f);
			InitPCPanelTexture(tex, 119);
			PSMTXTrans(tex.mTexMtx.mMtx[0], mTexScroll, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			PSMTXScale(tex.mTexMtx.mMtx[2], 0.5f * (width / GXGetTexObjWidth(&tex)), 0.5f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			color              = 0xFFFFFF00 | (u32)(40.0f * mAlpha);
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 2.0f + center, y, 0.0f, width, 20.0f, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			for (s32 i = 0; i < mChoiceCount; ++i) {
				s32 choice = mChoices[i];
				if (choice == -1) {
					continue;
				}
				const PCChoiceArt& row = pcChoiceArt[choice];
				f32 alpha              = mAlpha;
				if (choice == 7 && !mCanSelect) {
					alpha *= 0.4f;
				}
				f32 rowY = step * i + top;
				InitPCPanelTexture(tex, row.icon);
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				f32 w = 1.1f * GXGetTexObjWidth(&tex);
				f32 h = GXGetTexObjHeight(&tex);
				DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00,
				                   (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00, 204.0f + 0.5f * w,
				                   1.0f + (rowY + 0.5f * h), 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				if (IsJapanese()) {
					DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xFFFFFF00,
					                             GetPCMenuLabelText(pcChoiceArt[mChoices[i]].label), 224.0f, rowY, 0.0f, 20.0f, 20.0f);
				} else {
					DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xFFFFFF00,
					                             GetPCMenuLabelText(pcChoiceArt[mChoices[i]].label), 6.0f + (204.0f + w), rowY, 0.0f, 20.0f,
					                             20.0f);
				}
			}
		} break;
		case 3: {
			s32 selected = mItemShopSelection;
			f32 alpha    = mListAlpha;
			DrawPCItemShopList(selected, 320.0f, 215.0f + mScroll, mTexScroll, alpha, 1.0f, 0.0f);
			DrawPCGoldNumber(true, 339.0f, 316.0f, mAlpha, 1.0f, mMoneyDisplay.shown);
			f32 barY      = 76.0f;
			s32 positions = CountPCShopItems(sItemShopEntries) - 8;
			f32 first, last;
			if (positions <= 1) {
				first = 0.0f;
				last  = 1.0f;
			} else {
				s32 start = gItemShopPage.mStart;
				f32 step  = 1.0f / (f32)positions;
				f32 row   = (f32)start;
				first     = step * row;
				last      = step * (1.0f + row);
			}
			DrawPCShopScrollBar(460.0f, barY, first, last, mAlpha, 1.0f);
		} break;
		case 4:
			DrawPCRankings(this, -30.0f, -20.0f, mAlpha, 1.0f);
			break;
		case 7: {
			s32 selected = mScrapShopSelection;
			DrawPCScrapShopList(selected, 320.0f, 215.0f + mScroll, mTexScroll, 1.0f, 1.0f, 0.0f);
			DrawPCBlueNumber(true, 339.0f, 316.0f, mAlpha, 1.0f, mScrapDisplay.shown);
			f32 barY      = 76.0f;
			s32 positions = CountPCShopItems(sScrapShopEntries) - 8;
			f32 first, last;
			if (positions <= 1) {
				first = 0.0f;
				last  = 1.0f;
			} else {
				s32 start = gScrapShopPage.mStart;
				f32 step  = 1.0f / (f32)positions;
				f32 row   = (f32)start;
				first     = step * row;
				last      = step * (1.0f + row);
			}
			DrawPCShopScrollBar(460.0f, barY, first, last, mAlpha, 1.0f);
		} break;
		case 5: {
			CTexObj* subwindow = &gGame.mSubwin.mTexture.mTex;
			f32 w              = 0.6f * (1.1f * GXGetTexObjWidth(subwindow));
			f32 h              = 0.6f * GXGetTexObjHeight(subwindow);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			u32 color = (u32)(255.0f * mAlpha) | 0xC8C8C800;
			DrawGsTexturedQuad(&gGame.mGs, subwindow, color, color, color, color, 320.0f, 200.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		} break;
		case 9: {
			sprintf(labelText, "%s", GetPCMenuLabelText(7));
			DrawGsProportionalTextFormat(&gGame.mGs, (u32)(mAlpha * 255.0f) | 0xC8C8C800, labelText, 224.0f, 148.0f, 0.0f, 18.0f, 18.0f);
			{
				const f32 scale = 1.2f;
				s32 balance     = mDepositBalance;
				InitPCPanelTexture(tex, 126);
				f32 w                  = scale * GXGetTexObjWidth(&tex);
				f32 h                  = scale * (GXGetTexObjHeight(&tex) / 10);
				mDepositDisplay.target = balance;
				PC_EASE_CALL(mDepositDisplay.shown, mDepositDisplay.target);
				PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
				tex.mTexMtx.mDirty = TRUE;
				f32 step = w - 2.4f, start = 276.0f, offset = 9.6f, y = 184.0f;
				u32 rgb = 0xFFB62800;
				for (int i = 0; i < 5; i++) {
					f32 divisor;
					PC_DIGIT_DIVISOR(divisor, i);
					s32 digit = (s32)(mDepositDisplay.shown / divisor) % 10;
					PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
					tex.mTexMtx.mDirty = TRUE;
					gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
					f32 x     = (7.0f * w + (start - i * step)) - offset;
					u32 color = (u32)(255.0f * mAlpha) | rgb;
					DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
					gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				}
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
			switch (mPhase) {
			case 0: {
				sprintf(labelText, "%s", GetPCMenuLabelText(8));
				DrawGsProportionalTextFormat(&gGame.mGs, (u32)(mAlpha * 255.0f) | 0xC8C8C800, labelText, 224.0f, 212.0f, 0.0f, 18.0f,
				                             18.0f);
				{
					const f32 scale = 1.4f;
					s32 amount      = mAmount;
					InitPCPanelTexture(tex, 126);
					f32 w          = scale * GXGetTexObjWidth(&tex);
					f32 h          = scale * (GXGetTexObjHeight(&tex) / 10);
					PCAmount entry = {};
					entry.shown = entry.target = amount;
					PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
					tex.mTexMtx.mDirty = TRUE;
					f32 step = w + 1.4f, start = 256.0f, offset = 30.8f, y = 256.0f;
					u32 rgb = 0xC8C8C800;
					for (int i = 0; i < 5; i++) {
						f32 divisor;
						PC_DIGIT_DIVISOR(divisor, i);
						s32 digit = (s32)(entry.shown / divisor) % 10;
						PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
						tex.mTexMtx.mDirty = TRUE;
						gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
						f32 x     = (7.0f * w + (start - i * step)) - offset;
						u32 color = (u32)(255.0f * mAlpha) | rgb;
						DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
						gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
					}
					gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				}
				InitPCPanelTexture(tex, 126);
				f32 w      = 1.4f * GXGetTexObjWidth(&tex);
				f32 h      = 1.4f * (GXGetTexObjHeight(&tex) / 10);
				f32 left   = (388.0f - mMoneyDigit * (1.0f + w)) - 8.0f;
				f32 coord0 = left, coord1 = 242.0f, coord2 = left + w, coord3 = 242.0f + h;
				Vec vertices[] = { (Vec) { coord0, coord1, -10.0f }, (Vec) { coord2, coord1, -10.0f }, (Vec) { coord2, coord3, -10.0f },
					               (Vec) { coord0, coord3, -10.0f }, (Vec) { coord0, coord1, -10.0f } };
				gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
				DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, vertices,
				                        0x1F24F800 | (u32)(80.0f * mAlpha), 5);
				break;
			}
			case 1: {
				sprintf(labelText, "%s", GetPCMenuLabelText(9));
				DrawGsProportionalTextFormat(&gGame.mGs, (u32)(mAlpha * 255.0f) | 0xC8C8C800, labelText, 224.0f, 212.0f, 0.0f, 18.0f,
				                             18.0f);
				sprintf(labelText, "%s", GetPCMenuLabelText(10));
				DrawGsProportionalTextFormat(&gGame.mGs, (u32)(mAlpha * 255.0f) | 0xC8C8C800, labelText, 260.0f, 260.0f, 0.0f, 18.0f,
				                             18.0f);
				f32 x      = mYesNo == 0 ? 242.0f : 302.0f;
				f32 coord0 = x, coord1 = 251.0f, coord2 = x + 68.0f, coord3 = 283.0f;
				Vec vertices[] = { (Vec) { coord0, coord1, -10.0f }, (Vec) { coord2, coord1, -10.0f }, (Vec) { coord2, coord3, -10.0f },
					               (Vec) { coord0, coord3, -10.0f }, (Vec) { coord0, coord1, -10.0f } };
				DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, vertices,
				                        0x1F24F800 | (u32)(80.0f * mAlpha), 5);
				break;
			}
			case 2: {
				const f32 scale = 1.2f;
				s32 amount      = mAmount;
				InitPCPanelTexture(tex, 126);
				f32 w          = scale * GXGetTexObjWidth(&tex);
				f32 h          = scale * (GXGetTexObjHeight(&tex) / 10);
				PCAmount entry = {};
				entry.shown = entry.target = amount;
				PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
				tex.mTexMtx.mDirty = TRUE;
				f32 step = w - 2.4f, start = 372.0f, offset = 136.8f, y = 240.0f;
				u32 rgb = 0xFFFFFF00;
				for (int i = 0; i < 5; i++) {
					f32 divisor;
					PC_DIGIT_DIVISOR(divisor, i);
					s32 digit = (s32)(entry.shown / divisor) % 10;
					PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
					tex.mTexMtx.mDirty = TRUE;
					gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
					f32 x     = (7.0f * w + (start - i * step)) - offset;
					u32 color = (u32)(255.0f * mAlpha) | rgb;
					DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
					gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				}
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				sprintf(labelText, "%s", GetPCMenuLabelText(11));
				DrawGsProportionalTextFormat(&gGame.mGs, (u32)(mAlpha * 255.0f) | 0xC8C8C800, labelText, 244.0f, 260.0f, 0.0f, 18.0f,
				                             18.0f);
				break;
			}
			}
			DrawPCGoldNumber(true, 339.0f, 316.0f, mAlpha, 1.0f, mMoneyDisplay.shown);
		} break;
		case 10: {
			s32 rate = (s32)mScrapRate;
			switch (mPhase) {
			case 0: {
				{
					const char* localized = GetLocalizedInterfaceText(29 + 46);
					u32 color             = (u32)(255.0f * mAlpha) | 0xFFFFFF00;
					f32 x = 320.0f - 0.5f * MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(29 + 46), TRUE, 20.0f);
					DrawGsProportionalTextFormat(&gGame.mGs, color, localized, x, 148.0f, 0.0f, 20.0f, 20.0f);
				}
				{
					const char* localized = GetLocalizedInterfaceText(30 + 46);
					u32 color             = (u32)(255.0f * mAlpha) | 0xFFFFFF00;
					f32 x = 320.0f - 0.5f * MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(30 + 46), TRUE, 20.0f);
					DrawGsProportionalTextFormat(&gGame.mGs, color, localized, x, 220.0f, 0.0f, 20.0f, 20.0f);
				}
				{
					s32 amount = mAmount;
					InitPCPanelTexture(tex, 126);
					f32 width      = 1.4f * GXGetTexObjWidth(&tex);
					f32 height     = 1.4f * (GXGetTexObjHeight(&tex) / 10);
					f32 step       = width + 1.4f;
					PCAmount entry = {};
					entry.shown = entry.target = amount;
					PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
					tex.mTexMtx.mDirty = TRUE;
					for (s32 i = 0; i < 5; ++i) {
						f32 divisor;
						PC_DIGIT_DIVISOR(divisor, i);
						int digit = (s32)(entry.shown / divisor) % 10;
						PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, digit * 0.1f, 0.0f);
						tex.mTexMtx.mDirty = TRUE;
						gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
						f32 x     = ((7.0f * width + (256.0f - i * step)) - 26.6f) - (2.0f * step) * 0.5f;
						u32 color = (u32)(255.0f * mAlpha) | 0xC8C8C800;
						DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, 256.0f, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
						gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
					}
					gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
					InitPCPanelTexture(tex, 126);
					f32 markerW = 1.4f * GXGetTexObjWidth(&tex);
					f32 markerH = 1.4f * (GXGetTexObjHeight(&tex) / 10);
					f32 left    = ((388.0f - mScrapDigit * step) - 4.0f) - (2.0f * step) / 2.0f;
					f32 right = left + markerW, bottom = 241.0f + markerH;
					f32 coord0 = left, coord1 = 241.0f, coord2 = right, coord3 = bottom;
					Vec marker[] = { (Vec) { coord0, coord1, -10.0f }, (Vec) { coord2, coord1, -10.0f }, (Vec) { coord2, coord3, -10.0f },
						             (Vec) { coord0, coord3, -10.0f }, (Vec) { coord0, coord1, -10.0f } };
					gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
					DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, marker,
					                        0x1F24F800 | (u32)(80.0f * mAlpha), 5);
				}
				DrawPCBlueNumber(false, 164.0f, 178.0f, mAlpha, 1.0f, 1.0f);
				DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * mAlpha) | 0xC8C8C800, GetPCMenuLabelText(34), 313.0f, 169.0f, 0.0f,
				                             18.0f, 18.0f);
				s32 digits = CountDecimalDigits(rate);
				InitPCPanelTexture(tex, 126);
				DrawPCGoldNumber(false, 212.0f + digits * (1.4f * GXGetTexObjWidth(&tex)), 178.0f, mAlpha, 1.0f, rate);
				break;
			}
			case 1:
			case 2:
				DrawPCRecyclePanel(this, mYesNo, 320.0f, 215.0f + mScroll, mAmount, mAmount * rate, mTexScroll, mAlpha, 1.0f);
				break;
			}
			DrawPCBlueNumber(true, 339.0f, 316.0f, mAlpha, 1.0f, mScrapDisplay.shown);
		} break;
		case 11:
			break;
		}
		static f32 textureScrollU, textureScrollV;
		textureScrollU -= 0.008f;
		textureScrollV -= 0.008f;
		f32 height = mFrameHeight;
		{
			f32 left = 130.0f, top = 40.0f, right = 510.0f, bottom = 40.0f + height;
			Vec vertices[] = { (Vec) { left, top, -10.0f }, (Vec) { right, top, -10.0f }, (Vec) { right, bottom, -10.0f },
				               (Vec) { left, bottom, -10.0f }, (Vec) { left, top, -10.0f } };
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
			DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, vertices, (u32)(255.0f * mAlpha) | 0xFFB62800,
			                        5);
		}
		InitPCPanelTexture(tex, 139);
		PSMTXTrans(tex.mTexMtx.mMtx[0], textureScrollU, textureScrollV, 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		f32 v              = 320.0f / GXGetTexObjHeight(&tex);
		PSMTXScale(tex.mTexMtx.mMtx[2], 380.0f / GXGetTexObjWidth(&tex), v, 1.0f);
		tex.mTexMtx.mDirty = TRUE;
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_GREATER, GX_TRUE);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
		gGame.mGs.mCurTevStage->SetMode(GX_BLEND);
		u32 frameColor = (u32)(16.0f * mAlpha) | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &tex, frameColor, frameColor, frameColor, frameColor, 510.0f, 360.0f, -11.0f, 380.0f, height, 0.0f,
		                   0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
		f32 half = height / 2.0f;
		u32 bandColor;
		{
			f32 left = 130.0f, top = 360.0f - half, right = 510.0f, bottom = 360.0f;
			Vec vertices[] = { (Vec) { left, top, -10.0f }, (Vec) { right, top, -10.0f }, (Vec) { right, bottom, -10.0f },
				               (Vec) { left, bottom, -10.0f }, (Vec) { left, top, -10.0f } };
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
			bandColor = (u32)(255.0f * mAlpha) | 0xFFB62800;
			DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, vertices, bandColor, 5);
		}
		u32 gray = (bandColor & 0xFF) | 0x30303000;
		{
			f32 start = (40.0f + half) - half;
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
			static f32 topScanPhase = 0.0f;
			topScanPhase += 1.0f;
			if (topScanPhase >= 4.0f) {
				topScanPhase = 0.0f;
			}
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_SUBTRACT, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
			for (s32 row = (s32)start; row < (s32)(start + height); row += 4.0f) {
				f32 y = row + topScanPhase;
				DrawGsLineSegment(&gGame.mGs, NULL, 130.0f, y, 0.0f, gray, 510.0f, y, 0.0f, gray);
			}
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
		}
		{
			f32 start = (360.0f - half * 0.5f) - half * 0.5f;
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
			static f32 bottomScanPhase = 0.0f;
			bottomScanPhase += 1.0f;
			if (bottomScanPhase >= 4.0f) {
				bottomScanPhase = 0.0f;
			}
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_SUBTRACT, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
			for (s32 row = (s32)start; row < (s32)(start + half); row += 4.0f) {
				f32 y = row + bottomScanPhase;
				DrawGsLineSegment(&gGame.mGs, NULL, 130.0f, y, 0.0f, gray, 510.0f, y, 0.0f, gray);
			}
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
		}
		if (mMode != 0 && mMode != 1 && mMode != 11) {
			switch (mMode) {
			case 2:
				sprintf(labelText, "%s", GetPCMenuLabelText(1));
				break;
			case 3:
				if (mShopOnly)
					sprintf(labelText, "%s", GetPCMenuLabelText(3));
				else
					sprintf(labelText, "%s", GetPCMenuLabelText(2));
				break;
			case 4:
				sprintf(labelText, "%s", GetPCMenuLabelText(4));
				break;
			case 5:
				sprintf(labelText, "%s", GetPCMenuLabelText(5));
				break;
			case 7:
				sprintf(labelText, "%s", GetPCMenuLabelText(24));
				break;
			case 9:
				sprintf(labelText, "%s", GetPCMenuLabelText(6));
				break;
			case 10:
				sprintf(labelText, "%s", GetPCMenuLabelText(28));
				break;
			default:
				sprintf(labelText, "%s", "");
				break;
			}
			char joined[256];
			sprintf(joined, "%s", GetPCMenuLabelText(0));
			strcat(joined, labelText);
			strcpy(labelText, joined);
			const f32 x = 210.0f, y = 120.0f;
			DrawGsProportionalTextFormat(&gGame.mGs, 0xFFB62800 | (u32)(255.0f * mAlpha), labelText, x, y, 0.0f, 18.0f, 18.0f);
			if (!mShopOnly) {
				f32 rotation = 0.0f;
				f32 top      = 129.0f;
				f32 inset;
				switch (mMode) {
				case 3:
				case 7:
				case 10:
					inset = 70.0f;
					break;
				case 4:
					inset = 50.0f;
					break;
				default:
					inset = 30.0f;
					break;
				}
				u32 color = 0xC8C8C800 | (u32)(255.0f * mAlpha);
				switch (mTextPhase) {
				case 1: {
					Vec points[] = { (Vec) { 210.0f, top, -10.0f }, (Vec) { mTextX, top, -10.0f } };
					rotation     = 0.0f;
					DrawGsLineStrip(&gGame.mGs, points, color, 2);
					break;
				}
				case 2: {
					f32 coord2   = 210.0f - inset;
					Vec points[] = { (Vec) { 210.0f, top, -10.0f }, (Vec) { coord2, top, -10.0f }, (Vec) { coord2, mTextY, -10.0f } };
					rotation     = 1.57079637f;
					DrawGsLineStrip(&gGame.mGs, points, color, 3);
					break;
				}
				case 3: {
					rotation         = -3.14159274f;
					f32 startX       = 210.0f;
					const f32 left   = startX - inset;
					const f32 bottom = top + 172.0f;
					Vec points[]     = { (Vec) { startX, top, -10.0f }, (Vec) { left, top, -10.0f }, (Vec) { left, bottom, -10.0f },
						                 (Vec) { mTextX, bottom, -10.0f } };
					DrawGsLineStrip(&gGame.mGs, points, color, 4);
					break;
				}
				case 4: {
					rotation         = -3.14159274f;
					f32 startX       = 210.0f;
					const f32 left   = startX - inset;
					const f32 bottom = top + 172.0f;
					Vec points[]     = { (Vec) { startX, top, -10.0f }, (Vec) { left, top, -10.0f }, (Vec) { left, bottom, -10.0f },
						                 (Vec) { mTextX, bottom, -10.0f } };
					DrawGsLineStrip(&gGame.mGs, points, color, 4);
					break;
				}
				case 0:
				default:
					break;
				}
				InitPCPanelTexture(tex, 148);
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				f32 width  = 1.1f * GXGetTexObjWidth(&tex);
				f32 height = GXGetTexObjHeight(&tex);
				color      = (u32)(255.0f * mAlpha) | 0xC8C8C800;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, mTextX, mTextY, 0.0f, width, height, 0.0f, 0.0f, rotation);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				InitPCPanelTexture(tex, 143);
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				const f32 scale = 0.8f;
				width           = scale * (1.1f * GXGetTexObjWidth(&tex));
				height          = scale * GXGetTexObjHeight(&tex);
				f32 x           = width / 2.0f + 184.0f;
				f32 y           = height / 2.0f + 60.0f;
				color           = (u32)(255.0f * mAlpha) | 0xC8C8C800;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				if (IsJapanese()) {
					InitPCPanelTexture(tex, 144);
					gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
					width  = 1.1f * GXGetTexObjWidth(&tex);
					height = GXGetTexObjHeight(&tex);
					x      = width / 2.0f + 220.0f;
					y      = height / 2.0f + 66.0f;
					color  = (u32)(255.0f * mAlpha) | 0xC8C8C800;
					DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
					gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				}
			}
			{
				u32 color;
				InitPCPanelTexture(tex, 141);
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				f32 width  = 1.1f * GXGetTexObjWidth(&tex);
				f32 height = GXGetTexObjHeight(&tex);
				f32 x      = width / 2.0f + 204.0f;
				f32 y      = height * 0.5f + 335.0f;
				color      = (u32)(255.0f * mAlpha) | 0xC8C8C800;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				if (mMode == 2)
					sprintf(labelText, "%s", GetPCMenuLabelText(12));
				else
					sprintf(labelText, "%s", GetPCMenuLabelText(13));
				DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * mAlpha), labelText, 224.0f, 334.0f, 0.0f, 20.0f, 20.0f);
				InitPCPanelTexture(tex, 140);
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				width  = 1.1f * GXGetTexObjWidth(&tex);
				height = GXGetTexObjHeight(&tex);
				x      = width * 0.5f + 344.0f;
				y      = height * 0.5f + 335.0f;
				color  = (u32)(255.0f * mAlpha) | 0xC8C8C800;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				switch (mMode) {
				case 4:
					sprintf(labelText, "%s", GetPCMenuLabelText(14));
					break;
				case 3:
					switch (mPhase) {
					case 0:
					case 10:
					case 20:
						sprintf(labelText, "%s", GetPCMenuLabelText(16));
						break;
					case 1:
						sprintf(labelText, "%s", GetPCMenuLabelText(15));
						break;
					default:
						sprintf(labelText, "%s", GetPCMenuLabelText(13));
						break;
					}
					break;
				case 7:
					switch (mPhase) {
					case 0:
					case 10:
					case 20:
						sprintf(labelText, "%s", GetPCMenuLabelText(16));
						break;
					case 1:
						sprintf(labelText, "%s", GetPCMenuLabelText(25));
						break;
					default:
						sprintf(labelText, "%s", GetPCMenuLabelText(13));
						break;
					}
					break;
				case 10:
					switch (mPhase) {
					case 0:
						sprintf(labelText, "%s", GetPCMenuLabelText(16));
						break;
					case 1:
						sprintf(labelText, "%s", GetPCMenuLabelText(37));
						break;
					default:
						sprintf(labelText, "%s", GetPCMenuLabelText(13));
						break;
					}
					break;
				default:
					sprintf(labelText, "%s", GetPCMenuLabelText(16));
					break;
				}
				DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * mAlpha), labelText, 364.0f, 334.0f, 0.0f, 20.0f, 20.0f);
			}
		}
		if (mMode >= 2) {
			const char* text;
			switch (mMode) {
			case 2:
				switch (mGreetingVariant) {
				case 0:
					text = GetPCDialogueText(20) + mCharIndex;
					break;
				case 1:
					text = GetPCDialogueText(0) + mCharIndex;
					break;
				case 2:
					text = GetPCDialogueText(1) + mCharIndex;
					break;
				default:
					text = GetPCDialogueText(20) + mCharIndex;
					break;
				}
				break;
			case 4: {
				s32 level = gGame.GetPlayer()->mParams.mLevel;
				text      = GetPCDialogueText(gPCLevelDialogueIds[level]) + mCharIndex;
				if (*text == '\t') {
					mInsertsNumber = 1;
					++mCharIndex;
				}
				break;
			}
			case 5:
				if (GetPCDialogueText(2)[mCharIndex] == '\a') {
					++mKeeperAnimStep;
					++mCharIndex;
					mPlayKeeperAnim = 1;
				}
				text = GetPCDialogueText(2) + mCharIndex;
				break;
			case 3:
				DrawPCItemPurchasePanel(mItemShopSelection, mYesNo, 320.0f, 215.0f + mScroll, mTexScroll, mAlpha, mPanelAlpha);
				text = GetPCDialogueText(15) + mCharIndex;
				break;
			case 9:
				text = GetPCDialogueText(16) + mCharIndex;
				break;
			case 7:
				DrawPCScrapPurchasePanel(mScrapShopSelection, mYesNo, 320.0f, 215.0f + mScroll, mTexScroll, mAlpha, mPanelAlpha);
				text = GetPCDialogueText(17) + mCharIndex;
				break;
			case 10:
				text = GetPCDialogueText(18) + mCharIndex;
				break;
			default:
				text = GetPCDialogueText(19) + mCharIndex;
				break;
			}
			if ((s32)++mGlyphFrames >= 8) {
				mGlyphFrames           = 0;
				mCharFlags[mNextGlyph] = 1;
				if (mInsertsNumber) {
					char number[32];
					sprintf(number, "%d", (s32)mDialogueNumber);
					ConvertHudTextToFullWidth(number);
					PC_SET_DIALOGUE_GLYPH(this, PC_READ_DIALOGUE_GLYPH(number + mNumberOffset));
					mNumberOffset += 2;
					if (!number[mNumberOffset]) {
						mNumberOffset  = 0;
						mInsertsNumber = 0;
					}
				} else if (!*text) {
					PC_SET_DIALOGUE_GLYPH(this, 0x8140);
					mCharIndex      = 0;
					mNumberOffset   = 0;
					mKeeperAnimStep = 0;
				} else {
					PC_SET_DIALOGUE_GLYPH(this, PC_READ_DIALOGUE_GLYPH(text));
					mCharIndex += 2;
				}
				if ((s32)++mNextGlyph >= 40) {
					mNextGlyph = 0;
				}
			}
			for (int i = 0; i < 40; ++i) {
				if (!mCharFlags[i]) {
					continue;
				}
				mCharX[i] -= 3.0f;
				if (mCharX[i] <= -22.0f) {
					mCharX[i]     = 640.0f;
					mCharFlags[i] = 0;
				}
				DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * mAlpha) | 0xC8C8C800, mCharCopies[i], mCharX[i], 370.0f, 0.0f,
				                             22.0f, 22.0f);
			}
		}
		InitGameOrthoCamera(camera, &gGame.mGs.mScreen);
		((CBase*)&gGame)->vf5C();
		width               = gGame.mAspectRatio == 0 ? 640 : 854;
		gRingScreenWidth    = width;
		gPointerScreenWidth = width;
		return TRUE;
	}
	return FALSE;
}

static inline void DrawPCRankingMask(f32 left, f32 top, f32 right, f32 bottom, f32 alpha, u32& color, bool first)
{
	Vec vertices[5] = { (Vec) { left, top, -10.0f }, (Vec) { right, top, -10.0f }, (Vec) { right, bottom, -10.0f },
		                (Vec) { left, bottom, -10.0f }, (Vec) { left, top, -10.0f } };
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	if (first) {
		color = (u32)(255.0f * alpha) | 0x0A0A0A00;
	}
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, vertices, color, 5);
}

static inline void LoadRankingRow(MenuPC* menu, int slot, int offset)
{
	menu->mRankingRows[slot].rank      = gPCRankingRows[menu->mRankingEnd - offset].rank;
	menu->mRankingRows[slot].nameIndex = gPCRankingRows[menu->mRankingEnd - offset].nameIndex;
	menu->mRankingRows[slot].score     = gPCRankingRows[menu->mRankingEnd - offset].score;
}

static inline void ClearRankingRow(PCRankingRow& row)
{
	row.rank      = 0;
	row.nameIndex = -1;
	row.score     = 0;
}

static inline void DrawPCRankingDigits(CTexObj& tex, const s32& value, f32 x, f32 y, f32 scale, u32 upper, u32 lower, bool rank)
{
	InitPCPanelTexture(tex, 126);
	f32 w = scale * GXGetTexObjWidth(&tex), h = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	for (int i = 0; i < (rank ? 7 : 6); i++) {
		f32 div = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++) {
				div *= 10.0f;
			}
		}
		s32 digit = (s32)(value / div);
		if (i != 0 && digit == 0) {
			continue;
		}
		PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * (digit % 10), 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		f32 dx = 7.0f * w + (x - i * (w - 4.0f * scale));
		dx     = rank ? dx - 24.0f * scale : 106.0f * scale + dx;
		DrawGsTexturedQuad(&gGame.mGs, &tex, upper, upper, lower, lower, dx, y + 8.0f * scale, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
}

// NONMATCHING: the complete 45-call list matches; the current spelling needs 212 more bytes and a different stack/register layout
extern "C" void DrawPCRankings(MenuPC* menu, f32 x, f32 y, f32 alpha, f32 scale)
{
	if (alpha > 0.0f) {
		switch (menu->mPhase) {
		case 0:
			for (int i = 0; i < 5; i++) {
				menu->mRankingOffsets[i]        = 0.0f;
				menu->mRankingRows[i].nameIndex = 0;
				menu->mRankingRows[i].score     = 0;
			}
			menu->mRankingEnd = 3;
			if (menu->mRankingEnd <= 3) {
				menu->mPhase          = 2;
				menu->mRankingRows[3] = gPCRankingRows[2];
				menu->mRankingRows[2] = gPCRankingRows[1];
				menu->mRankingRows[1] = gPCRankingRows[0];
				break;
			}
			ClearRankingRow(menu->mRankingRows[4]);
			LoadRankingRow(menu, 3, 1);
			LoadRankingRow(menu, 2, 2);
			LoadRankingRow(menu, 1, 3);
			LoadRankingRow(menu, 0, 4);
			menu->mPhase++;
		case 1:
			for (int i = 0; i < 4; i++) {
				menu->mRankingOffsets[i] += 0.4f;
			}
			if (menu->mRankingOffsets[0] >= 48.0f) {
				menu->mRankingEnd--;
				if (menu->mRankingEnd <= 3) {
					menu->mPhase++;
					for (int i = 0; i < 4; i++) {
						menu->mRankingOffsets[i] = 48.0f;
					}
				} else {
					for (int i = 0; i < 4; i++) {
						menu->mRankingOffsets[i] = 0.0f;
					}
					menu->mRankingRows[3] = menu->mRankingRows[2];
					menu->mRankingRows[2] = menu->mRankingRows[1];
					menu->mRankingRows[1] = menu->mRankingRows[0];
					if (menu->mRankingEnd >= 4) {
						LoadRankingRow(menu, 0, 4);
					} else {
						ClearRankingRow(menu->mRankingRows[0]);
					}
				}
			}
			break;
		case 2:
			break;
		}
		CTexObj tex;
		f32 left    = 17.0f + (10.0f + ((120.0f + (174.0f + x)) - 70.0f));
		f32 opacity = 255.0f * alpha;
		u8 a        = (u8)opacity;
		for (int i = 0; i < 5; i++) {
			if (gPCRankingRows[i].nameIndex == -1) {
				continue;
			}
			f32 rowY = (menu->mRankingOffsets[i] + ((168.0f + y) + i * 48)) - 48.0f;
			InitPCPanelTexture(tex, 181);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			f32 w = 1.1f * (1.1f * GXGetTexObjWidth(&tex)), h = 1.1f * GXGetTexObjHeight(&tex);
			DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
			                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, (w / 2.0f + left) - 34.0f,
			                   1.0f + (h / 2.0f + rowY), 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			const PCRankingRow& row = menu->mRankingRows[i];
			s32 rankValue           = row.rank;
			char text[256];
			if (rankValue > 0 && rankValue <= 1000000) {
				DrawPCRankingDigits(tex, rankValue, left, rowY, scale, a | 0xBFDDFB00, a | 0x0E72D600, true);
				sprintf(text, "%s", GetLocalizedInterfaceText(0x44));
				DrawGsProportionalTextFormat(&gGame.mGs, a | 0xBFDDFB00, text, 103.0f * scale + left, rowY, 0.0f, 18.0f * scale,
				                             18.0f * scale);
			} else {
				sprintf(text, "%s", GetLocalizedInterfaceText(0x45));
				DrawGsProportionalTextFormat(&gGame.mGs, a | 0xBFDDFB00, text, 48.0f * scale + left, rowY, 0.0f, 18.0f * scale,
				                             18.0f * scale);
			}
			DrawPCRankingDigits(tex, row.score, left, rowY, scale, a | 0xCECEFF00, a | 0xE037EE00, false);
			InitPCPanelTexture(tex, 155);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			f32 iconWidth = GXGetTexObjWidth(&tex), iconHeight = GXGetTexObjHeight(&tex);
			DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
			                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, 140.0f * scale + left,
			                   rowY + 10.0f * scale, 0.0f, 0.6f * (scale * iconWidth), 0.6f * (scale * iconHeight), 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			sprintf(text, "%s", GetPCRankingName(row.nameIndex));
			DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * menu->mAlpha) | 0xC8C8C800, text, 48.0f * scale + left,
			                             rowY + 20.0f * scale, 0.0f, 18.0f * scale, 18.0f * scale);
		}
	}
	u32 mask;
	DrawPCRankingMask(130.0f, 100.0f, 510.0f, 140.0f, alpha, mask, true);
	DrawPCRankingMask(130.0f, 290.0f, 510.0f, 330.0f, alpha, mask, false);
}

static inline void DrawSaveChoiceLabel(MenuSave* menu, s32 id, f32 x, f32 y)
{
	u32 color        = (u32)(255.0f * menu->mDrawAlpha);
	const char* text = GetLocalizedInterfaceText(id);
	f32 width        = MeasureGameTextWidth(&gGame.mGs, text, TRUE, 21.0f);
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - width / 2.0f, y, 0.0f, 21.0f, 21.0f);
}

static inline s32 DrawSaveLinesAt(MenuSave* menu, s32 id, f32 x, f32 y)
{
	return DrawJapaneseSaveStatusMessage(menu->mChannel, id, x, y, 21.0f, menu->mDrawAlpha);
}

static inline f32 SaveMessageY(MenuSave* menu, f32 panelY)
{
	return ((8.0f + (356.0f - panelY)) - 6.0f) + menu->mBannerSlide;
}

static inline s32 SaveStatusLines(MenuSave* menu, s32 id, f32 panelX, f32 panelY)
{
	return DrawSaveLinesAt(menu, id, 0.0f, SaveMessageY(menu, panelY));
}

// NONMATCHING: the call list matches retail; this build is 264 bytes smaller and register allocation differs
BOOL MenuSave::Draw()
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
		DrawSavePanelBackground(mDrawAlpha);
		switch (mMode) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			for (int i = 0; i < 3; i++) {
				f32 width   = 154.0f + gSaveSlotSpacing;
				f32 x       = 49.0f + (width / 2.0f + i * (40.0f + width)) - (3.0f * gSaveSlotSpacing) / 2.0f;
				u8 selected = mFile == i;
				u8 newest   = mNewestFile == i;
				DrawSaveSlotDetails(&mSlots[i], 0, i, selected, newest, x, 223.0f, width, 230.0f, mDrawAlpha, mFileAlpha[i], 1.0f, 1.0f);
			}
			DrawSaveHelpText(this);
			break;
		}
		f32 panelX = mPanelX;
		f32 panelY = mPanelY - 8.0f;
		DrawMenuBannerPanels(NULL, panelX, panelY, mDrawAlpha, mBannerSlide);
		DrawSaveMenuTitle(this);
		s32 lines        = 2;
		bool showMessage = false;
		char text[256];
		switch (mMode) {
		case 1:
			if (mSaved == 1) {
				lines = SaveStatusLines(this, 4, panelX, panelY);
			}
			break;
		case 2:
			sprintf(text, "%s", GetLocalizedInterfaceText(0xEF));
			showMessage = true;
			break;
		case 3:
			sprintf(text, "%s", GetLocalizedInterfaceText(0xAE));
			showMessage = true;
			break;
		case 4:
			if (mPhase == 0) {
				lines = SaveStatusLines(this, 4, panelX, panelY);
			} else {
				sprintf(text, "%s", GetLocalizedInterfaceText(0xEA));
				showMessage = true;
			}
			break;
		case 19:
			lines = SaveStatusLines(this, 10, panelX, panelY);
			break;
		case 18:
			lines = SaveStatusLines(this, 11, panelX, panelY);
			break;
		case 5:
			lines = SaveStatusLines(this, 4, panelX, panelY);
			break;
		case 8: {
			f32 x = 0.0f, y = SaveMessageY(this, panelY);
			if (mPhase == 0) {
				lines = DrawSaveLinesAt(this, 5, x, y);
			} else {
				lines = DrawSaveLinesAt(this, 6, x, y);
			}
			break;
		}
		case 9:
			lines = SaveStatusLines(this, 7, panelX, panelY);
			break;
		case 10: {
			f32 x = 0.0f, y = SaveMessageY(this, panelY);
			if (mPhase == 0) {
				lines = DrawSaveLinesAt(this, 7, x, y);
			} else {
				lines = DrawSaveLinesAt(this, 9, x, y);
			}
			break;
		}
		case 6:
		case 11:
			lines = DrawJapaneseCardErrorMessage(mChannel, mResult, 0, 0.0f, SaveMessageY(this, panelY), 21.0f, mDrawAlpha);
			break;
		case 12:
			lines = SaveStatusLines(this, mMessage, panelX, panelY);
			break;
		case 13:
			if (mResult == -4) {
				lines = SaveStatusLines(this, 13, panelX, panelY);
			} else if ((u32)(mResult + 3) <= 1) {
				lines = SaveStatusLines(this, 16, panelX, panelY);
			} else {
				lines = SaveStatusLines(this, 17, panelX, panelY);
			}
			break;
		case 14:
			lines = SaveStatusLines(this, mMessage, panelX, panelY);
			break;
		case 15:
			lines = SaveStatusLines(this, mMessage, panelX, panelY);
			break;
		case 16:
		case 17:
			break;
		}
		if (lines <= 2) {
			lines = 2;
		}
		mOffset = -(lines - 2) * 21;
		if (showMessage) {
			f32 y     = SaveMessageY(this, panelY);
			u32 color = (u32)(255.0f * mDrawAlpha);
			f32 x     = gPointerScreenWidth / 2 - MeasureGameTextWidth(&gGame.mGs, text, TRUE, 21.0f) / 2.0f;
			DrawGsProportionalTextFormat(&gGame.mGs, color, text, x, y, 0.0f, 21.0f, 21.0f);
		}
		switch (mMode) {
		case 3:
		case 8: {
			f32 lineOffset = 0.0f;
			if (lines > 1) {
				lineOffset = (lines - 2) * 21;
			}
			f32 barWidth = 61.5f;
			f32 barX;
			if (mChoice == -1) {
				barX = gPointerScreenWidth + 200;
			} else if (mChoice == 0) {
				barX = gPointerScreenWidth / 2 - (16.0f + barWidth) * 0.5f;
			} else {
				barX = gPointerScreenWidth / 2 + (16.0f + barWidth) * 0.5f;
			}
			f32 baseLabelY = (24.0f + (12.0f + (356.0f - panelY))) - 6.0f;
			f32 labelY     = lineOffset + (baseLabelY + mBannerSlide);
			{
				CTexObj tex;
				f32 opacity = mDrawAlpha, textureOffset = mTexScroll;
				u32 barColor = (u32)(200.0f * opacity) | 0x181CC600;
				InitMenuSelectionBar(tex);
				DrawMenuSelectionBar(tex, barX, 2.0f + (8.0f + labelY), barWidth, 22.0f, opacity, textureOffset, barColor);
			}
			labelY = lineOffset + (baseLabelY + mBannerSlide);
			if (IsJapanese()) {
				DrawSaveChoiceLabel(this, 0x66, gPointerScreenWidth / 2 - (16.0f + barWidth) * 0.5f, labelY);
				DrawSaveChoiceLabel(this, 0x67, gPointerScreenWidth / 2 + (16.0f + barWidth) * 0.5f, labelY);
			} else {
				DrawSaveChoiceLabel(this, 0x66, 2.0f + (gPointerScreenWidth / 2 - (16.0f + barWidth) * 0.5f), labelY - 1.0f);
				DrawSaveChoiceLabel(this, 0x67, 2.0f + (gPointerScreenWidth / 2 + (16.0f + barWidth) * 0.5f), labelY - 1.0f);
			}
			break;
		}
		}
		InitGameOrthoCamera(camera, &gGame.mGs.mScreen);
		((CBase*)&gGame)->vf5C();
		width               = gGame.mAspectRatio == 0 ? 640 : 854;
		gRingScreenWidth    = width;
		gPointerScreenWidth = width;
		if ((mMode == 13 || mMode == 11) && gSaveFatal.mActive && gSaveFatal.mMessage != NULL) {
			OSFatal(gSaveFatal.mFg, gSaveFatal.mBg, gSaveFatal.mMessage);
		}
		return TRUE;
	}
	return FALSE;
}

static inline void SetStatusRing(Vec* outline, f32 x, f32 y, f32 radius)
{
	for (int i = 0; i < 65; i++) {
		f32 angle    = 0.09817477f * i - 1.5707964f;
		f32 sine     = (f32)sin(angle);
		f32 cosine   = (f32)cos(angle);
		outline[i].x = sine * radius + x;
		outline[i].y = -cosine * radius + y;
		outline[i].z = 0.0f;
	}
}

static inline void StrokeStatusOutline(Vec* outline, u32 color, u16 count)
{
	CGs& gs = gGame.mGs;
	gs.mCurRenderState->SetLineWidth(18, GX_TO_ONE);
	DrawGsLineStrip(&gs, outline, color, count);
	gs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
}

// NONMATCHING: the call list matches retail; this build is 156 bytes smaller and register allocation differs
extern "C" void DrawPlayerStatusValue(s32 icon, s32 value, bool selected, f32 x, f32 y, f32 alpha, f32 phase)
{
	if (!(alpha > 0.0f)) {
		return;
	}
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	CTexObj tex;
	f32 bob = 0.0f;
	if (selected) {
		bob = 4.0f * (f32)sin(0.017453292f * (20.0f * (f32)fmod(sStatusPhase, 360.0)));
	}
	s32 places = 1;
	for (;; places++) {
		f32 divisor = 1.0f;
		for (s32 j = 0; j < places; j++) {
			divisor *= 10.0f;
		}
		if ((s32)(value / divisor) == 0) {
			break;
		}
	}
	Vec outline[65];
	f32 centerY = y - 6.0f * 1.2f;
	SetStatusRing(outline, x, centerY, 12.0f * 1.2f);
	f32 opacity = 255.0f * alpha;
	u32 color   = (u32)opacity;
	InitPCPanelTexture(tex, 265);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00,
	                   (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00, x, centerY, 0.0f, 33.0f, 33.0f, 0.0f, 0.0f,
	                   0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	InitPCPanelTexture(tex, 126);
	f32 width = 1.2f * GXGetTexObjWidth(&tex);
	StrokeStatusOutline(outline, color, 65);
	outline[0].x = x;
	outline[0].y = centerY + 12.0f * 1.2f;
	outline[1].x = x + (places * (width - 3.0f * 1.2f) + 34.0f);
	outline[1].y = outline[0].y;
	InitPCPanelTexture(tex, 156);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 markerWidth = GXGetTexObjWidth(&tex), markerHeight = GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
	                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, outline[1].x, outline[1].y, 0.0f,
	                   1.2f * markerWidth, 1.2f * markerHeight, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	f32 scale = 0.8f;
	if (icon == 181) {
		scale = 0.7f;
	}
	InitPCPanelTexture(tex, icon);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	gGame.mGs.mCurRenderState->SetZCompLoc(GX_FALSE);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	width      = scale * GXGetTexObjWidth(&tex);
	f32 height = scale * GXGetTexObjHeight(&tex);
	f32 base   = bob + (y - 9.0f);
	f32 iconY  = base;
	if (icon == 155) {
		iconY = 4.0f + base;
	}
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
	                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x, iconY, 1.0f, width, height, 0.0f, 0.0f,
	                   0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	gGame.mGs.mCurRenderState->SetZCompLoc(GX_TRUE);
	if (selected) {
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_EQUAL, GX_FALSE);
		f32 pulse = alpha * (0.2f * (f32)sin(3.1415927f * phase) + 0.2f);
		if (pulse < 0.0f) {
			pulse = 0.0f;
		}
		DrawGsQuad(&gGame.mGs, (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00,
		           (u32)(255.0f * pulse) | 0xFFFFFF00, x, iconY, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	}
	InitPCPanelTexture(tex, 126);
	x -= 12.0f;
	y -= 4.0f;
	width  = 1.2f * GXGetTexObjWidth(&tex);
	height = 1.2f * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	u8 a               = (u8)opacity;
	f32 offset         = places * (width - 3.0f * 1.2f) - 98.0f * 1.2f;
	for (s32 i = 0; i < 7; i++) {
		f32 divisor;
		PC_DIGIT_DIVISOR(divisor, i);
		s32 digit = (s32)(value / divisor);
		if (i != 0 && digit == 0) {
			continue;
		}
		digit %= 10;
		PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		u32 top, bottom;
		switch (icon) {
		case 155:
			top    = 0xCECEFF00 | a;
			bottom = 0xE037EE00 | a;
			break;
		case 153:
			top    = 0xEFEBC500 | a;
			bottom = 0xEDA83600 | a;
			break;
		case 154:
			top    = 0xCECEFF00 | a;
			bottom = 0x5959FF00 | a;
			break;
		case 181:
			top    = 0xBFDDFB00 | a;
			bottom = 0x0E72D600 | a;
			break;
		default:
			top = bottom = 0xFFFFFF00 | a;
		}
		DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, offset + (7.0f * width + (x - i * (width - 3.0f * 1.2f))), y - 2.4f,
		                   0.0f, width, height, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

static inline void SetGaugeQuad(Vec* vertices, f32 left, f32 right, f32 top, f32 bottom)
{
	vertices[0].x = left;
	vertices[0].y = top;
	vertices[1].x = right;
	vertices[1].y = top;
	vertices[2].x = right;
	vertices[2].y = bottom;
	vertices[3].x = left;
	vertices[3].y = bottom;
}

// NONMATCHING: the complete 46-call list matches; a 108-byte code-layout and register-allocation difference remains
extern "C" void DrawStatusGauge(f32 value, f32 x, f32 y, f32 alpha)
{
	if (!(alpha > 0.0f)) {
		return;
	}
	CTexObj tex;
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	gGame.mGs.mCurRenderState->SetLineWidth(6, GX_TO_ONE);
	f32 left         = 16.0f + x;
	f32 shadowX      = 4.0f + left;
	f32 right        = 168.0f + left;
	f32 fill         = 168.0f * value + left;
	f32 shadowRight  = 168.0f + shadowX;
	Vec vertices[20] = {};
	f32 bottom       = 16.0f + y;
	f32 shadowY      = 4.0f + y;
	SetGaugeQuad(vertices, left, right, y, bottom);
	SetGaugeQuad(vertices + 4, left, fill, y, bottom);
	SetGaugeQuad(vertices + 8, fill, right, y, bottom);
	SetGaugeQuad(vertices + 12, shadowX, shadowRight, shadowY, 16.0f + shadowY);
	SetGaugeQuad(vertices + 16, left, fill, shadowY, (bottom - 4.0f) - 1.0f);
	DrawGsVertexQuad(&gGame.mGs, vertices[12].x, vertices[12].y, vertices[12].z, (u32)(100.0f * alpha) | 0x0A0A0A00, vertices[13].x,
	                 vertices[13].y, vertices[13].z, (u32)(100.0f * alpha) | 0x0A0A0A00, vertices[14].x, vertices[14].y, vertices[14].z,
	                 (u32)(100.0f * alpha) | 0x0A0A0A00, vertices[15].x, vertices[15].y, vertices[15].z,
	                 (u32)(100.0f * alpha) | 0x0A0A0A00);
	f32 opacity = 255.0f * alpha;
	u32 a       = (u32)opacity;
	DrawGsLineStrip(&gGame.mGs, vertices, a | 0xC8C8C800, 5);
	DrawGsVertexQuad(&gGame.mGs, vertices[0].x, vertices[0].y, vertices[0].z, (u32)(40.0f * alpha) | 0xA0FFFF00, vertices[1].x,
	                 vertices[1].y, vertices[1].z, (u32)(40.0f * alpha) | 0xA0FFFF00, vertices[2].x, vertices[2].y, vertices[2].z,
	                 (u32)(40.0f * alpha) | 0xA0FFFF00, vertices[3].x, vertices[3].y, vertices[3].z, (u32)(40.0f * alpha) | 0xA0FFFF00);
	DrawGsVertexQuad(&gGame.mGs, vertices[16].x, vertices[16].y, vertices[16].z, (u32)(100.0f * alpha) | 0x40FF8000, vertices[17].x,
	                 vertices[17].y, vertices[17].z, (u32)(100.0f * alpha) | 0x40FF8000, vertices[18].x, vertices[18].y, vertices[18].z,
	                 (u32)(100.0f * alpha) | 0x40FF8000, vertices[19].x, vertices[19].y, vertices[19].z,
	                 (u32)(100.0f * alpha) | 0x40FF8000);
	gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
	u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
	f32 scale = 1.0f;
	InitPCPanelTexture(tex, 243);
	f32 width  = 0.85f * (scale * GXGetTexObjWidth(&tex));
	f32 height = 0.85f * (scale * GXGetTexObjHeight(&tex));
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 capY = (8.0f + y) - 0.5f;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 2.0f + (left - width / 2.0f), capY, 0.0f, width, height, 0.0f, 0.0f,
	                   3.1415927f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	color = (u32)(255.0f * alpha) | 0xFFFFFF00;
	InitPCPanelTexture(tex, 242);
	width  = 0.95f * (scale * GXGetTexObjWidth(&tex));
	height = 0.95f * (scale * GXGetTexObjHeight(&tex));
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, (width / 2.0f + right) - 4.0f, capY, 0.0f, width, height, 0.0f, 0.0f,
	                   3.1415927f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	s32 current = (s32)(10000.0f * value);
	x += 101.0f * 1.2f;
	y -= 4.8f;
	InitPCPanelTexture(tex, 126);
	width  = 1.2f * GXGetTexObjWidth(&tex);
	height = 1.2f * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	color              = 0xFFFFFF00 | (u8)opacity;
	f32 offset         = (3.0f * (width - 3.0f * 1.2f) - 98.0f * 1.2f) - 16.0f;
	u32 upper, lower;
	for (s32 i = 0; i < 5; i++) {
		f32 divisor;
		PC_DIGIT_DIVISOR(divisor, i);
		s32 digit = (s32)(10000 / divisor);
		if (i != 0 && digit == 0) {
			continue;
		}
		digit %= 10;
		PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		upper = color;
		lower = color;
		DrawGsTexturedQuad(&gGame.mGs, &tex, upper, upper, lower, lower, offset + (7.0f * width + (x - i * (width - 3.0f * 1.2f))),
		                   y - 2.4f, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
	f32 shift = 6.0f * (width - 3.0f * 1.2f);
	for (s32 i = 0; i < 5; i++) {
		f32 divisor;
		PC_DIGIT_DIVISOR(divisor, i);
		s32 digit = (s32)(current / divisor);
		if (i != 0 && digit == 0) {
			continue;
		}
		digit %= 10;
		PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		upper = color;
		lower = color;
		DrawGsTexturedQuad(&gGame.mGs, &tex, upper, upper, lower, lower,
		                   (offset + (7.0f * width + (x - i * (width - 3.0f * 1.2f)))) - shift, y - 2.4f, 0.0f, width, height, 0.0f, 0.0f,
		                   0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	InitPCPanelTexture(tex, 169);
	f32 slashWidth = GXGetTexObjWidth(&tex), slashHeight = GXGetTexObjHeight(&tex);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, upper, upper, lower, lower, offset + (7.0f * width + (x - 5.0f * (width - 3.0f * 1.2f))), y - 1.2f,
	                   0.0f, 0.8f * (1.2f * slashWidth), 0.8f * (1.2f * slashHeight), 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// NONMATCHING: the complete 48-call list matches; a 68-byte code-layout and register-allocation difference remains
extern "C" void DrawSeparatedStatusValue(s32 icon, s32 value, bool selected, f32 x, f32 y, f32 alpha)
{
	if (!(alpha > 0.0f)) {
		return;
	}
	CTexObj tex;
	f32 bob       = 0.0f;
	bool skipTens = false;
	if (selected) {
		static u32 frame = 0;
		++frame;
		bob = 4.0f * (f32)sin(0.017453292f * (((s32)(frame * 20)) % 360));
	}
	s32 places = (value >= 1000 ? 1 : 0) + 4;
	if ((value % 100) / 10 == 0) {
		skipTens = true;
	}
	Vec outline[65];
	f32 centerY = y - 6.0f * 1.2f;
	SetStatusRing(outline, x, centerY, 12.0f * 1.2f);
	f32 opacity = 255.0f * alpha;
	u32 color   = (u32)opacity;
	InitPCPanelTexture(tex, 265);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00,
	                   (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00, x, centerY, 0.0f, 33.0f, 33.0f, 0.0f, 0.0f,
	                   0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	StrokeStatusOutline(outline, color, 65);
	s32 visible = places;
	if (skipTens) {
		--visible;
	}
	InitPCPanelTexture(tex, 126);
	f32 width    = 1.2f * GXGetTexObjWidth(&tex);
	outline[0].x = x;
	outline[0].y = centerY + 12.0f * 1.2f;
	outline[1].x = x + (visible * (width - 3.0f * 1.2f) + 34.0f);
	outline[1].y = outline[0].y;
	InitPCPanelTexture(tex, 156);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 markerWidth = GXGetTexObjWidth(&tex), markerHeight = GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
	                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, outline[1].x, outline[1].y, 0.0f,
	                   1.2f * markerWidth, 1.2f * markerHeight, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	InitPCPanelTexture(tex, icon);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	width      = 0.8f * GXGetTexObjWidth(&tex);
	f32 height = 0.8f * GXGetTexObjHeight(&tex);
	f32 iconY  = bob + (y - 9.0f);
	if (icon == 155) {
		DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
		                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x, 4.0f + iconY, 0.0f, width, height,
		                   0.0f, 0.0f, 0.0f);
	} else {
		DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
		                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x, iconY, 0.0f, width, height, 0.0f,
		                   0.0f, 0.0f);
	}
	InitPCPanelTexture(tex, 126);
	x -= 12.0f;
	y -= 4.0f;
	width  = 1.2f * GXGetTexObjWidth(&tex);
	height = 1.2f * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	u32 white          = 0xFFFFFF00 | (u8)opacity;
	f32 offset         = (places - 1) * (width - 3.0f * 1.2f) - 98.0f * 1.2f;
	u32 upper, lower;
	for (s32 i = 0; i < 4; i++) {
		f32 divisor;
		PC_DIGIT_DIVISOR(divisor, i);
		s32 digit = (s32)(value / divisor) % 10;
		if ((i == 3 || i == 1) && digit == 0) {
			continue;
		}
		PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		f32 gap = 0.0f;
		upper   = white;
		lower   = white;
		if (i < 2) {
			gap = width - 3.0f * 1.2f;
		}
		if (skipTens && i == 0) {
			gap -= width - 3.0f * 1.2f;
		}
		DrawGsTexturedQuad(&gGame.mGs, &tex, upper, upper, lower, lower, gap + (offset + (7.0f * width + (x - i * (width - 3.0f * 1.2f)))),
		                   y - 2.4f, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	InitPCPanelTexture(tex, 169);
	f32 slashWidth = GXGetTexObjWidth(&tex), slashHeight = GXGetTexObjHeight(&tex);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, upper, upper, lower, lower, offset + (7.0f * width + (x - (width - 3.0f * 1.2f))), y - 1.2f, 0.0f,
	                   0.8f * (1.2f * slashWidth), 0.8f * (1.2f * slashHeight), 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

static f32 sBannerScrollX, sBannerScrollY;

static inline void DrawMenuBannerStrip(CTexObj& tex, f32 x, f32 y, f32 width, f32 height, f32 alpha)
{
	Vec vertices[5] = { (Vec) { x, y, -10.0f }, (Vec) { x + width, y, -10.0f }, (Vec) { x + width, height + y, -10.0f },
		                (Vec) { x, height + y, -10.0f }, (Vec) { x, y, -10.0f } };
	CGs& gs         = gGame.mGs;
	gs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	u32 fill = (u32)(255.0f * alpha) | 0xFFB62800;
	DrawGsTrianglePrimitive(&gs, GX_TRIANGLEFAN, gs.mCamera->mViewMtx, vertices, fill, 5);
	InitPCPanelTexture(tex, 139);
	PSMTXTrans(tex.mTexMtx.mMtx[0], sBannerScrollX, sBannerScrollY, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	f32 h = GXGetTexObjHeight(&tex), w = GXGetTexObjWidth(&tex);
	PSMTXScale(tex.mTexMtx.mMtx[2], width / w, height / h, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	gs.mCurRenderState->SetZMode(GX_TRUE, GX_GREATER, GX_TRUE);
	gs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
	gs.mCurTevStage->SetMode(GX_BLEND);
	u32 overlay = (u32)(16.0f * alpha) | 0xFFFFFF00;
	DrawGsTexturedQuad(&gs, &tex, overlay, overlay, overlay, overlay, x + width / 2.0f, height / 2.0f + y, -11.0f, width, height, 0.0f,
	                   0.0f, 0.0f);
	gs.mCurTevStage->SetMode(GX_PASSCLR);
	gs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
}

// NONMATCHING: the call list matches retail; this build is 180 bytes larger and the stack and vertex layout differ
extern "C" void DrawMenuBannerPanels(const char* title, f32 x, f32 y, f32 alpha, f32 slide)
{
	CTexObj tex;
	f32 top    = 0.0f;
	f32 height = 98.0f;
	sBannerScrollX -= 0.008f;
	sBannerScrollY -= 0.008f;
	f32 width = (gGame.mAspectRatio == 0 ? 640 : 854) + 20.0f;
	f32 left  = x - (width - 640.0f) * 0.5f;
	DrawMenuBannerStrip(tex, left, y + top, width, height, alpha);
	if (title) {
		char text[256];
		sprintf(text, "%s", title);
		u32 count    = strlen(text) / 2;
		f32 textLeft = (left + gPointerScreenWidth / 2) - ((count - 1) * 30) / 2;
		f32 textTop  = 4.0f + (8.0f + (22.0f + y));
		DrawDoubleByteMenuText(text, 8.0f + textLeft, 8.0f + (15.0f + textTop), -10.0f, 30.0f, 30.0f, alpha);
	}
	f32 bottomY = (356.0f - y) - 6.0f + slide;
	height -= slide;
	if (height < 8.0f) {
		height = 8.0f;
	}
	DrawMenuBannerStrip(tex, left, bottomY, width, height, alpha);
}

// NONMATCHING: the complete 22-call list matches; this build is 8 bytes smaller and the vertex stores are scheduled differently
void DrawStatusPanelBackground(f32 alpha)
{
	f32 centerX             = gRingScreenWidth / 2;
	f32 upperY              = gPointerScreenHeight / 2 + 4;
	f32 scale               = 1.1f;
	f32 t                   = sin(1.5707964f * alpha);
	f32 expand              = 42.0f * t;
	f32 spread              = 20.0f * t + 60.0f;
	f32 lowerY              = 24.0f + upperY;
	f32 x                   = -2.0f * scale * spread + centerX;
	f32 right               = 2.0f * scale * spread + centerX;
	u32 opacity             = (u32)(160.0f * alpha);
	f32 lowerAlpha          = 0.0f;
	u32 bottomOpacity       = (u32)lowerAlpha;
	u32 clear               = 0;
	PrimVertex vertices[24] = { { (Vec) { ((x - expand) - 2.0f * expand), ((upperY - expand) - 2.0f * expand), 0.0f }, clear },
		                        { (Vec) { (x - expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		                        { (Vec) { (x - expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		                        { (Vec) { ((x - expand) - 2.0f * expand), (2.0f * expand + (upperY + expand)), 0.0f }, clear },
		                        { (Vec) { (x - expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		                        { (Vec) { (right + expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		                        { (Vec) { (right + expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		                        { (Vec) { (x - expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		                        { (Vec) { (right + expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		                        { (Vec) { (2.0f * expand + (right + expand)), ((upperY - expand) - 2.0f * expand), 0.0f }, clear },
		                        { (Vec) { (2.0f * expand + (right + expand)), (2.0f * expand + (upperY + expand)), 0.0f }, clear },
		                        { (Vec) { (right + expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		                        { (Vec) { ((x - expand) - 2.0f * expand), lowerY, 0.0f }, clear },
		                        { (Vec) { (x - expand), lowerY, 0.0f }, bottomOpacity },
		                        { (Vec) { (x - expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity },
		                        { (Vec) { ((x - expand) - 2.0f * expand), (2.0f * expand + (lowerY + expand)), 0.0f }, clear },
		                        { (Vec) { (x - expand), lowerY, 0.0f }, bottomOpacity },
		                        { (Vec) { (right + expand), lowerY, 0.0f }, bottomOpacity },
		                        { (Vec) { (right + expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity },
		                        { (Vec) { (x - expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity },
		                        { (Vec) { (right + expand), lowerY, 0.0f }, bottomOpacity },
		                        { (Vec) { (2.0f * expand + (right + expand)), lowerY, 0.0f }, clear },
		                        { (Vec) { (2.0f * expand + (right + expand)), (2.0f * expand + (lowerY + expand)), 0.0f }, clear },
		                        { (Vec) { (right + expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity } };
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, vertices, 4);
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, &vertices[4], 4);
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, &vertices[8], 4);
	u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
	DrawGsLineSegment(&gGame.mGs, NULL, x - expand, (upperY - expand) - 2.0f * expand, 0.0f, color, right + expand,
	                  (upperY - expand) - 2.0f * expand, 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, (x - expand) - 2.0f * expand, (upperY - expand) - 2.0f * expand, 0.0f, 0xFFFFFF00, x - expand,
	                  (upperY - expand) - 2.0f * expand, 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, right + expand, (upperY - expand) - 2.0f * expand, 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00,
	                  2.0f * expand + (right + expand), (upperY - expand) - 2.0f * expand, 0.0f, 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, x - expand, 2.0f * expand + (upperY + expand), 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00,
	                  right + expand, 2.0f * expand + (upperY + expand), 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, (x - expand) - 2.0f * expand, 2.0f * expand + (upperY + expand), 0.0f, 0xFFFFFF00, x - expand,
	                  2.0f * expand + (upperY + expand), 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, right + expand, 2.0f * expand + (upperY + expand), 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00,
	                  2.0f * expand + (right + expand), 2.0f * expand + (upperY + expand), 0.0f, 0xFFFFFF00);
}

// NONMATCHING: same shape as DrawStatusPanelBackground; 8 bytes smaller, vertex stores scheduled differently
void DrawSavePanelBackground(f32 alpha)
{
	f32 centerX             = gRingScreenWidth / 2;
	f32 upperY              = gRingScreenHeight / 2 - 16;
	f32 scale               = 1.1f;
	f32 t                   = sin(1.5707964f * alpha);
	f32 expand              = 44.4f * t;
	f32 spread              = 20.0f * t + 60.0f;
	f32 lowerY              = 24.0f + upperY;
	f32 x                   = -2.0f * scale * spread + centerX;
	f32 right               = 2.0f * scale * spread + centerX;
	u32 opacity             = (u32)(160.0f * alpha);
	f32 lowerAlpha          = 0.0f;
	u32 bottomOpacity       = (u32)lowerAlpha;
	u32 clear               = 0;
	PrimVertex vertices[24] = { { (Vec) { ((x - expand) - 2.0f * expand), ((upperY - expand) - 2.0f * expand), 0.0f }, clear },
		                        { (Vec) { (x - expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		                        { (Vec) { (x - expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		                        { (Vec) { ((x - expand) - 2.0f * expand), (2.0f * expand + (upperY + expand)), 0.0f }, clear },
		                        { (Vec) { (x - expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		                        { (Vec) { (right + expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		                        { (Vec) { (right + expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		                        { (Vec) { (x - expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		                        { (Vec) { (right + expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		                        { (Vec) { (2.0f * expand + (right + expand)), ((upperY - expand) - 2.0f * expand), 0.0f }, clear },
		                        { (Vec) { (2.0f * expand + (right + expand)), (2.0f * expand + (upperY + expand)), 0.0f }, clear },
		                        { (Vec) { (right + expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		                        { (Vec) { ((x - expand) - 2.0f * expand), lowerY, 0.0f }, clear },
		                        { (Vec) { (x - expand), lowerY, 0.0f }, bottomOpacity },
		                        { (Vec) { (x - expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity },
		                        { (Vec) { ((x - expand) - 2.0f * expand), (2.0f * expand + (lowerY + expand)), 0.0f }, clear },
		                        { (Vec) { (x - expand), lowerY, 0.0f }, bottomOpacity },
		                        { (Vec) { (right + expand), lowerY, 0.0f }, bottomOpacity },
		                        { (Vec) { (right + expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity },
		                        { (Vec) { (x - expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity },
		                        { (Vec) { (right + expand), lowerY, 0.0f }, bottomOpacity },
		                        { (Vec) { (2.0f * expand + (right + expand)), lowerY, 0.0f }, clear },
		                        { (Vec) { (2.0f * expand + (right + expand)), (2.0f * expand + (lowerY + expand)), 0.0f }, clear },
		                        { (Vec) { (right + expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity } };
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, vertices, 4);
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, &vertices[4], 4);
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, &vertices[8], 4);
	u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
	DrawGsLineSegment(&gGame.mGs, NULL, x - expand, (upperY - expand) - 2.0f * expand, 0.0f, color, right + expand,
	                  (upperY - expand) - 2.0f * expand, 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, (x - expand) - 2.0f * expand, (upperY - expand) - 2.0f * expand, 0.0f, 0xFFFFFF00, x - expand,
	                  (upperY - expand) - 2.0f * expand, 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, right + expand, (upperY - expand) - 2.0f * expand, 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00,
	                  2.0f * expand + (right + expand), (upperY - expand) - 2.0f * expand, 0.0f, 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, x - expand, 2.0f * expand + (upperY + expand), 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00,
	                  right + expand, 2.0f * expand + (upperY + expand), 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, (x - expand) - 2.0f * expand, 2.0f * expand + (upperY + expand), 0.0f, 0xFFFFFF00, x - expand,
	                  2.0f * expand + (upperY + expand), 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, right + expand, 2.0f * expand + (upperY + expand), 0.0f, (u32)(255.0f * alpha) | 0xFFFFFF00,
	                  2.0f * expand + (right + expand), 2.0f * expand + (upperY + expand), 0.0f, 0xFFFFFF00);
}

// NONMATCHING: the call list matches retail; this build is 12 bytes smaller and register allocation differs
extern "C" void DrawStatusMenuTitle(MenuStatus* menu)
{
	if (menu->mTitle.mActive) {
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		CTexObj blob;
		f32 t;
		f32 s0;
		f32 bob;
		f32 rot;
		f32 s1;
		f32 s2;
		f32 scale;
		f32 x;
		f32 y;
		f32 pad;
		u32 color  = menu->mTitle.mColor & 0xFFFFFF00;
		f32 zero   = 0.0f;
		f32 center = gPointerScreenWidth / 2;
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			MenuTitleChar* glyph = &menu->mTitle.mChars[i];
			t                    = glyph->mFade;
			if (!(t <= 0.0f)) {
				InitPCPanelTexture(blob, 108);
				CFont font;
				s32 width;
				CTexObj tex = *GetFontGlyphTexture(&font, glyph->mText, 0, &width);
				s0          = std::sinf(glyph->mAngle0);
				bob         = 4.0f * s0;
				rot         = std::sinf(0.7f * glyph->mAngle1);
				s1          = std::sinf(3.1415927f * t);
				s2          = std::sinf(1.5707964f * t);
				scale       = 0.2f * s1 + s2;
				x           = 24.0f + (48.0f * i + (center - 48.0f * menu->mTitle.mCount / 2.0f));
				rot         = -(0.017453292f * (10.0f * rot));
				y           = 64.0f + bob;
				u32 c       = (u32)(255.0f * t) | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &blob, c, c, c, c, x, y, 0.0f, 20.0f * (2.0f * scale), 20.0f * (2.0f * scale), 0.0f, -zero,
				                   rot);
				scale = 20.0f * (1.7f * scale);
				pad   = (width / 24.0f) * scale;
				pad   = scale - pad;
				c     = (u32)(255.0f * t);
				for (int j = 0; j < 4; j++) {
					DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, pad / 2.0f + (x + gStatusTitleOutlineX[j]),
					                   bob + (64.0f + gStatusTitleOutlineY[j]), 0.0f, scale, scale, 0.0f, -zero, rot);
				}
				c = color | (u32)(255.0f * t);
				DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, pad / 2.0f + x, y, 0.0f, scale, scale, 0.0f, -zero, rot);
			}
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
}

// NONMATCHING: the packed CColor construction and register lifetimes differ; colors, blending and the frame call match
extern "C" void fn_8007D734(s32 index, u8 selected, u8 newest, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 fileAlpha, f32, f32)
{
	if (index < 0) {
		return;
	}
	CTexObj frameTexture;
	u32 whiteColor = (u32)(255.0f * fileAlpha) | 0xFFFFFF00;
	CColor white;
	white.Set((s32)(whiteColor >> 24), (s32)((whiteColor >> 16) & 0xFF), (s32)((whiteColor >> 8) & 0xFF), (s32)(whiteColor & 0xFF));
	CColor fill;
	if (newest == 1) {
		u32 fillColor = (u32)(160.0f * fileAlpha) | 0x2B4C5000;
		fill.Set((s32)(fillColor >> 24), (s32)((fillColor >> 16) & 0xFF), (s32)((fillColor >> 8) & 0xFF), (s32)(fillColor & 0xFF));
	} else {
		u32 fillColor = (u32)(160.0f * fileAlpha) | 0x28282800;
		fill.Set((s32)(fillColor >> 24), (s32)((fillColor >> 16) & 0xFF), (s32)((fillColor >> 8) & 0xFF), (s32)(fillColor & 0xFF));
	}
	u32 border;
	if (selected == 1) {
		if (newest == 1) {
			border = (u32)(255.0f * fileAlpha) | 0x80C8F500;
		} else {
			border = (u32)(255.0f * fileAlpha) | 0xFFB62800;
		}
	} else if (newest == 1) {
		border = (u32)(255.0f * fileAlpha) | 0x80C8F500;
	} else {
		border = (u32)(255.0f * fileAlpha) | 0xFFFFFF00;
	}
	CColor color(fn_8016A96C(&fill.r, &white.r, alpha));
	DrawTexturedWindowFrame(*(u32*)&color.r, border, x, y - height / 2.0f, width - 56.0f, height, alpha, 1.0f, 1.0f);
}

// NONMATCHING: the Wii call list, slot fields, text ids and coordinates are reconstructed; with -ipa file this build
// still inlines all ten DrawSaveDetailText calls and grows to 4404 bytes, while retail calls it out of line in 1936
extern "C" void DrawSaveSlotDetails(WiiMenuSaveSlot* slot, u8, s16 index, u8 selected, u8 newest, f32 x, f32 y, f32 width, f32 height,
                                    f32 alpha, f32 fileAlpha, f32 sx, f32 sy)
{
	fn_8007D734(index, selected, newest, x, y, width, height, alpha, fileAlpha, sx, sy);
	if (slot->mState == 1) {
		DrawSaveDetailText(GetLocalizedInterfaceText(0x9F), x, (y - 5.0f * (17.0f * sy)) - 6.0f * sy + 76.0f * sy, 23.0f * sx, alpha);
		DrawSaveDetailText(GetLocalizedInterfaceText(0xA0), x, (y - 5.0f * (17.0f * sy)) - 6.0f * sy + 96.0f * sy, 23.0f * sx, alpha);
	} else if (slot->mUsed) {
		CTexObj tex;
		InitPCPanelTexture(tex, 114);
		f32 offsetY = -24.0f * sy;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		f32 w     = 0.6f * (sx * GXGetTexObjWidth(&tex));
		f32 h     = 0.6f * (sy * GXGetTexObjHeight(&tex));
		f32 iconX = ((x - 10.0f * (17.0f * sx)) + 100.0f * sx) - (gSaveSlotSpacing * 0.5f) * sx;
		f32 iconY = offsetY + ((218.0f * sy + ((y - 5.0f * (17.0f * sy)) - 6.0f * sy)));
		DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
		                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, iconX, iconY, 0.0f, w, h, 0.0f, 0.0f,
		                   0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		s32 stage       = slot->mStage;
		char* stageName = GetStageName(stage);
		if (stageName == NULL) {
			stageName = GetStageFileName(stage);
		}
		if (stage == 14) {
			stageName = GetStageName(7);
		}
		f32 size       = 23.0f * sx;
		f32 stageWidth = MeasureGameTextWidth(&gGame.mGs, stageName, TRUE, 0.8f * size);
		f32 stageX     = ((x + 88.0f * sx) - stageWidth * 0.5f) + (gSaveSlotSpacing * 0.5f) * sx - 5.0f * sx;
		f32 stageY     = offsetY + (210.0f * sy + (4.0f * sy + (y - 5.0f * (17.0f * sy))));
		DrawSaveDetailText(stageName, stageX, stageY, size, alpha);
		DrawSavePlayTime(0xB5, slot->mRank, selected, (x - 70.0f * sx) - (gSaveSlotSpacing * 0.5f) * sx, offsetY + (y - 64.0f * sy), alpha,
		                 sx, sy);
		DrawSavePlayTime(0xF4, slot->mActive ? -1 : (s32)slot->mBattery, selected, (x - 70.0f * sx) - (gSaveSlotSpacing * 0.5f) * sx,
		                 offsetY + (y - 32.0f * sy), alpha, sx, sy);
		DrawSavePlayTime(0x9B, slot->mExp, selected, (x - 70.0f * sx) - (gSaveSlotSpacing * 0.5f) * sx, y + offsetY, alpha, sx, sy);
		DrawSavePlayTime(0x99, slot->mMoney, selected, (x - 70.0f * sx) - (gSaveSlotSpacing * 0.5f) * sx, offsetY + (32.0f * sy + y), alpha,
		                 sx, sy);
		DrawSavePlayTime(0x9A, slot->mScrap, selected, (x - 70.0f * sx) - (gSaveSlotSpacing * 0.5f) * sx, offsetY + (64.0f * sy + y), alpha,
		                 sx, sy);
		fn_8007ECC4(slot->mTimeMode == 0 ? 157 : 158, slot->mHours, slot->mMinutes, selected,
		            (x - 70.0f * sx) - (gSaveSlotSpacing * 0.5f) * sx, offsetY + (96.0f * sy + y), alpha, sx, sy);
	} else if (slot->_44) {
		DrawSaveDetailText(GetLocalizedInterfaceText(0x64), x, (y - 5.0f * (17.0f * sy)) - 6.0f * sy + 86.0f * sy, 23.0f * sx, alpha);
	} else if (slot->_48) {
		DrawSaveDetailText(GetLocalizedInterfaceText(0x9A), x, (y - 5.0f * (17.0f * sy)) - 6.0f * sy + 76.0f * sy, 23.0f * sx, alpha);
		DrawSaveDetailText(GetLocalizedInterfaceText(0x9B), x, (y - 5.0f * (17.0f * sy)) - 6.0f * sy + 96.0f * sy, 23.0f * sx, alpha);
	} else if (slot->_4C) {
		DrawSaveDetailText(GetLocalizedInterfaceText(0xA4), x, (y - 5.0f * (17.0f * sy)) - 6.0f * sy + 76.0f * sy, 23.0f * sx, alpha);
		DrawSaveDetailText(GetLocalizedInterfaceText(0xA5), x, (y - 5.0f * (17.0f * sy)) - 6.0f * sy + 96.0f * sy, 23.0f * sx, alpha);
	}
}

extern "C" void fn_8007ECC4(s32 icon, s32 hours, s32 minutes, s32, f32 x, f32 y, f32 alpha, f32 sx, f32)
{
	if (!(alpha > 0.0f)) {
		return;
	}
	CTexObj tex;
	f32 scale = 1.2f;
	s32 value = hours * 100 + minutes;
	if (value >= 9959) {
		value = 9959;
	}
	for (int places = 1;; places++) {
		f32 divisor = 1.0f;
		for (int j = 0; j < places; j++) {
			divisor *= 10.0f;
		}
		if ((s32)(value / divisor) == 0) {
			break;
		}
	}
	InitPCPanelTexture(tex, icon);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 w = 0.6f * GXGetTexObjWidth(&tex);
	f32 h = 0.6f * GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
	                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
	InitPCPanelTexture(tex, 126);
	w = scale * GXGetTexObjWidth(&tex);
	h = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	f32 offset         = 10.0f * scale + ((6.0f * (w - 3.0f * scale) - 98.0f * scale) - 10.0f * scale);
	u8 alphaByte       = 255.0f * alpha;
	u32 top = alphaByte | 0xEFEBC500, bottom = alphaByte | 0xEDA83600;
	for (int i = 0; i < 4; i++) {
		f32 div = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++) {
				div *= 10.0f;
			}
		}
		s32 digit = value / div;
		if (i != 3 || digit != 0) {
			digit %= 10;
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			f32 gap = 0.0f;
			if (i < 2) {
				gap = w - 3.0f * scale;
			}
			f32 digitX = gSaveSlotSpacing * sx + (gap + (offset + (7.0f * w + (x - i * (w - 3.0f * scale)))));
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, digitX, y - 2.0f * scale, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
	InitPCPanelTexture(tex, 130);
	f32 sw = GXGetTexObjWidth(&tex), sh = GXGetTexObjHeight(&tex);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 sepX = gSaveSlotSpacing * sx + (10.0f * scale + (x + 81.0f * scale));
	DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, sepX, y - scale, 0.0f, 0.7f * (scale * sw), 0.7f * (scale * sh), 0.0f,
	                   0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// NONMATCHING: the call list and size match retail; register allocation differs
extern "C" void DrawSavePlayTime(s32 icon, s32 value, u8, f32 x, f32 y, f32 alpha, f32 offsetScale, f32)
{
	if (!(alpha > 0.0f)) {
		return;
	}
	CTexObj tex;
	bool noValue = false;
	for (int places = 1;; places++) {
		f32 divisor = 1.0f;
		for (int j = 0; j < places; j++) {
			divisor *= 10.0f;
		}
		if ((s32)(value / divisor) == 0) {
			break;
		}
	}
	f32 scale   = 0.7f;
	f32 offsetY = 0.0f;
	InitPCPanelTexture(tex, icon);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	switch (icon) {
	case 0xB5:
		scale = 0.6f;
		break;
	case 0x9B:
		offsetY = 4.0f;
		break;
	case 0xF4:
		if (value == -1) {
			noValue = true;
		}
		break;
	}
	f32 w = scale * GXGetTexObjWidth(&tex);
	f32 h = scale * GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
	                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x, y + offsetY, 0.0f, w, h, 0.0f, 0.0f,
	                   0.0f);
	f32 size   = 1.2f;
	u32 top    = (u8)(255.0f * alpha) | 0xEFEBC500;
	u32 bottom = (u8)(255.0f * alpha) | 0xEDA83600;
	if (noValue) {
		InitPCPanelTexture(tex, 168);
		w        = size * GXGetTexObjWidth(&tex);
		u16 texH = GXGetTexObjHeight(&tex);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		f32 offset = 10.0f * size + ((7.0f * (w - 3.0f * size) - 98.0f * size) - 10.0f * size);
		DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, gSaveSlotSpacing * offsetScale + (offset + (7.0f * w + x)),
		                   y - 2.0f * size, 0.0f, w, size * texH, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	} else {
		InitPCPanelTexture(tex, 126);
		w = size * GXGetTexObjWidth(&tex);
		h = size * (GXGetTexObjHeight(&tex) / 10);
		PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
		tex.mTexMtx.mDirty = TRUE;
		f32 offset         = 10.0f * size + ((7.0f * (w - 3.0f * size) - 98.0f * size) - 10.0f * size);
		for (int i = 0; i < 7; i++) {
			f32 divisor;
			PC_DIGIT_DIVISOR(divisor, i);
			s32 digit = (s32)(value / divisor);
			if (i != 0 && digit == 0) {
				continue;
			}
			digit %= 10;
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom,
			                   gSaveSlotSpacing * offsetScale + (offset + (7.0f * w + (x - i * (w - 3.0f * size)))), y - 2.0f * size, 0.0f,
			                   w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// NONMATCHING: the call list matches retail; this build is 12 bytes smaller and register allocation differs
extern "C" void DrawSaveMenuTitle(MenuSave* menu)
{
	if (menu->mTitle.mActive) {
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		CTexObj blob;
		f32 t;
		f32 s0;
		f32 bob;
		f32 rot;
		f32 s1;
		f32 s2;
		f32 scale;
		f32 x;
		f32 y;
		f32 pad;
		u32 color  = menu->mTitle.mColor & 0xFFFFFF00;
		f32 zero   = 0.0f;
		f32 center = gPointerScreenWidth / 2;
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			MenuTitleChar* glyph = &menu->mTitle.mChars[i];
			t                    = glyph->mFade;
			if (!(t <= 0.0f)) {
				InitPCPanelTexture(blob, 108);
				CFont font;
				s32 width;
				CTexObj tex = *GetFontGlyphTexture(&font, glyph->mText, 0, &width);
				s0          = std::sinf(glyph->mAngle0);
				bob         = 4.0f * s0;
				rot         = std::sinf(0.7f * glyph->mAngle1);
				s1          = std::sinf(3.1415927f * t);
				s2          = std::sinf(1.5707964f * t);
				scale       = 0.2f * s1 + s2;
				x           = 24.0f + (48.0f * i + (center - 48.0f * menu->mTitle.mCount / 2.0f));
				rot         = -(0.017453292f * (10.0f * rot));
				y           = 64.0f + bob;
				u32 c       = (u32)(255.0f * t) | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &blob, c, c, c, c, x, y, 0.0f, 20.0f * (2.0f * scale), 20.0f * (2.0f * scale), 0.0f, -zero,
				                   rot);
				scale = 20.0f * (1.7f * scale);
				pad   = (width / 24.0f) * scale;
				pad   = scale - pad;
				c     = (u32)(255.0f * t);
				for (int j = 0; j < 4; j++) {
					DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, pad / 2.0f + (x + gSaveTitleOutlineX[j]),
					                   bob + (64.0f + gSaveTitleOutlineY[j]), 0.0f, scale, scale, 0.0f, -zero, rot);
				}
				c = color | (u32)(255.0f * t);
				DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, pad / 2.0f + x, y, 0.0f, scale, scale, 0.0f, -zero, rot);
			}
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
}

extern "C" void fn_8005A128(MenuPaneFadePair* fades, f32 x, f32 y, f32 alpha)
{
	if (alpha <= 0.0f) {
		return;
	}
	for (s32 i = 0; i < 2; i++) {
		MenuPaneFade* fade = &fades->mFades[i];
		if (fade->mAlpha <= 0.0f) {
			continue;
		}
		f32 wave   = 0.8f * (f32)sin(HALF_PI * alpha * fade->mAlpha);
		f32 arrowX = 412.0f + x;
		f32 arrowY;
		if (i == 0) {
			arrowY = (gPointerScreenHeight / 2 + 4) - 126.0f;
		} else {
			arrowY = 126.0f + (gPointerScreenHeight / 2 + 4);
		}
		f32 pulse = fade->mAlpha * (0.2f * (f32)sin(PI * (fade->mHovered ? 0.5f : 1.5f)) + 0.2f);
		if (pulse < 0.0f) {
			pulse = 0.0f;
		}
		f32 rot[2] = { 0.0f, PI };
		CTexObj tex;
		InitPCPanelTexture(tex, 266);
		f32 width  = wave * (1.1f * GXGetTexObjWidth(&tex));
		f32 height = wave * (1.1f * GXGetTexObjHeight(&tex));
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		gGame.mGs.mCurRenderState->SetZCompLoc(GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		u32 color = (u32)(255.0f * fade->mAlpha) | 0xC8C8C800;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, arrowX, arrowY, 1.0f, width, height, 0.0f, 0.0f, rot[i]);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZCompLoc(GX_TRUE);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_EQUAL, GX_FALSE);
		DrawGsQuad(&gGame.mGs, (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00,
		           (u32)(255.0f * pulse) | 0xFFFFFF00, arrowX, arrowY, 1.0f, width, height, 0.0f, 0.0f, rot[i]);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	}
}

extern "C" void fn_8005A68C(MenuItem* menu, const char* text, u32 color)
{
	menu->mTitle.mActive = TRUE;
	menu->mTitle.mCount  = 0;
	if (text) {
		menu->mTitle.mColor = color;
		int i;
		for (i = 0; i < 20 && *text; i++, text++) {
			menu->mTitle.mChars[i].mText   = text;
			menu->mTitle.mChars[i].mFade   = -0.2f * i;
			menu->mTitle.mChars[i].mHold   = gRefreshRate;
			menu->mTitle.mChars[i].mAngle0 = 0.017453292f * (rand() % 360);
			menu->mTitle.mChars[i].mAngle1 = 0.017453292f * (rand() % 360);
			menu->mTitle.mCount++;
			if (gGame.mLanguage == 0) {
				s32 lead = TRUE;
				u8 c     = *text;
				u8 a     = (c >= 0x81 && c <= 0x9F);
				if (a == FALSE) {
					u8 b = (c >= 0xE0 && c <= 0xFC);
					if (b == FALSE) {
						lead = FALSE;
					}
				}
				if (lead) {
					text++;
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

// NONMATCHING: the call list matches retail; this build is 12 bytes smaller and register allocation differs
extern "C" void DrawItemMenuTitle(MenuItem* menu)
{
	if (menu->mTitle.mActive) {
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		CTexObj blob;
		f32 t;
		f32 s0;
		f32 bob;
		f32 rot;
		f32 s1;
		f32 s2;
		f32 scale;
		f32 x;
		f32 y;
		f32 pad;
		u32 color  = menu->mTitle.mColor & 0xFFFFFF00;
		f32 zero   = 0.0f;
		f32 center = gPointerScreenWidth / 2;
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			MenuTitleChar* glyph = &menu->mTitle.mChars[i];
			t                    = glyph->mFade;
			if (!(t <= 0.0f)) {
				InitPCPanelTexture(blob, 108);
				CFont font;
				s32 width;
				CTexObj tex = *GetFontGlyphTexture(&font, glyph->mText, 0, &width);
				s0          = std::sinf(glyph->mAngle0);
				bob         = 4.0f * s0;
				rot         = std::sinf(0.7f * glyph->mAngle1);
				s1          = std::sinf(3.1415927f * t);
				s2          = std::sinf(1.5707964f * t);
				scale       = 0.2f * s1 + s2;
				x           = 24.0f + (48.0f * i + (center - 48.0f * menu->mTitle.mCount / 2.0f));
				rot         = -(0.017453292f * (10.0f * rot));
				y           = 64.0f + bob;
				u32 c       = (u32)(255.0f * t) | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &blob, c, c, c, c, x, y, 0.0f, 20.0f * (2.0f * scale), 20.0f * (2.0f * scale), 0.0f, -zero,
				                   rot);
				scale = 20.0f * (1.7f * scale);
				pad   = (width / 24.0f) * scale;
				pad   = scale - pad;
				c     = (u32)(255.0f * t);
				for (int j = 0; j < 4; j++) {
					DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, pad / 2.0f + (x + gItemTitleOutlineX[j]),
					                   bob + (64.0f + gItemTitleOutlineY[j]), 0.0f, scale, scale, 0.0f, -zero, rot);
				}
				c = color | (u32)(255.0f * t);
				DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, pad / 2.0f + x, y, 0.0f, scale, scale, 0.0f, -zero, rot);
			}
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
}

extern "C" void fn_8005A9D0(MenuItem* menu, u32 frames)
{
	if (menu->mTitle.mActive && menu->mTitle.mActive > 0) {
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			if (menu->mTitle.mChars[i].mHold > 0.0f) {
				if (menu->mTitle.mChars[i].mFade < 1.0f) {
					if ((menu->mTitle.mChars[i].mFade += (1.0f / (0.2f * gRefreshRate)) * frames) > 1.0f) {
						menu->mTitle.mChars[i].mFade = 1.0f;
					}
				}
			} else if (menu->mTitle.mChars[i].mFade > 0.0f) {
				if ((menu->mTitle.mChars[i].mFade -= (1.0f / (0.2f * gRefreshRate)) * frames) < 0.0f) {
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

static inline f32 HelpGlyphPadding(s32 width, f32 scale)
{
	f32 glyphWidth = width / 24.0f * scale;
	return scale - glyphWidth;
}

// NONMATCHING: the Wii call list, viewport/scissor blocks and constants match; the SJIS lead-byte expansion,
// texture-object copies and floating-point schedules still differ
BOOL MenuItem::Draw()
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
		DrawStatusPanelBackground(mAlpha);
		f32 x = mGridX - 42.0f;
		f32 y = 180.0f + mGridY;
		GXSetScissor(gScreenLeft + (s32)margin, gPointerScreenHeight / 2 - 122, gPointerScreenWidth - (s32)margin * 2, 252);
		DrawInventoryGrid(mSelection, x, y, mAlpha, 1.0f, mItemPhase);
		char text[256];
		sprintf(text, "%s", GetLocalizedInterfaceText(0x19));
		GXSetScissor(gScreenLeft + (s32)margin, gScreenTop, gPointerScreenWidth - (s32)margin * 2, gPointerScreenHeight);
		DrawItemMenuTitle(this);
		if (mName) {
			f32 alpha   = mAlpha * mNameAlpha;
			f32 spacing = 30.0f;
			f32 center  = gPointerScreenWidth / 2;
			f32 bottom  = 160.0f + gPointerScreenHeight / 2;
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			CTexObj blob;
			InitPCPanelTexture(blob, 109);
			mHelpCount       = 0;
			const char* name = (const char*)mName;
			if (name) {
				for (int i = 0; i < 20 && *name; i++, name++) {
					mBg[i].mText = name;
					mHelpCount++;
					if (gGame.mLanguage == 0) {
						u8 c    = *name;
						u8 lead = TRUE;
						u8 a    = (c >= 0x81 && c <= 0x9F);
						if (!a) {
							u8 b = (c >= 0xE0 && c <= 0xFC);
							if (!b) {
								lead = FALSE;
							}
						}
						if (lead) {
							name++;
						}
					}
				}
			}
			f32 zero = 0.0f;
			for (int i = 0; i < mHelpCount; i++) {
				MenuHelpChar* glyph = &mBg[i];
				CFont font;
				s32 glyphWidth;
				CTexObj tex  = *GetFontGlyphTexture(&font, glyph->mText, 0, &glyphWidth);
				f32 bob      = 2.0f * (f32)sin(glyph->mAngle0);
				f32 rotation = sin(0.7f * glyph->mAngle1);
				f32 s1       = sin(3.1415927f * alpha);
				f32 s2       = sin(1.5707964f * alpha);
				f32 scale    = 0.2f * s1 + s2;
				f32 glyphX   = 0.5f * spacing + (i * spacing + (center - mHelpCount * spacing / 2.0f));
				f32 glyphY   = bottom + bob;
				rotation     = -(0.017453292f * (10.0f * rotation));
				u32 color    = (u32)(255.0f * alpha) | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &blob, color, color, color, color, glyphX, glyphY, 1.0f, 30.0f * scale, 30.0f * scale, 0.0f,
				                   -zero, rotation);
				scale       = 30.0f * (0.8f * scale);
				f32 padding = HelpGlyphPadding(glyphWidth, scale);
				color       = (u32)(255.0f * alpha);
				for (int j = 0; j < 4; j++) {
					DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, glyphX + gItemHelpOutlineX[j] + padding / 2.0f,
					                   bob + (bottom + gItemHelpOutlineY[j]), 1.0f, scale, scale, 0.0f, -zero, rotation);
				}
				color = (u32)(255.0f * alpha) | 0xFFFFFB00;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, glyphX + padding / 2.0f, glyphY, 1.0f, scale, scale, 0.0f,
				                   -zero, rotation);
			}
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
		fn_8005A128(&mPaneFades, mGridX - 42.0f, 180.0f + mGridY, mAlpha);
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

// NONMATCHING: the call list matches retail; this build is 4 bytes larger and register allocation differs
extern "C" void DrawStatusHelpText(MenuStatus* menu)
{
	f32 alpha   = menu->mAlpha * menu->mHelpAlpha;
	f32 spacing = 30.0f;
	f32 center  = gPointerScreenWidth / 2;
	f32 bottom  = 160.0f + gPointerScreenHeight / 2;
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	CTexObj blob;
	InitPCPanelTexture(blob, 109);
	menu->mHelpCount = 0;
	const char* text = menu->mHelpText;
	if (text) {
		for (int i = 0; i < 20 && *text; i++, text++) {
			menu->mHelpChars[i].mText = text;
			menu->mHelpCount++;
			if (gGame.mLanguage == 0) {
				u8 c    = *text;
				u8 lead = TRUE;
				u8 a    = (c >= 0x81 && c <= 0x9F);
				if (!a) {
					u8 b = (c >= 0xE0 && c <= 0xFC);
					if (!b) {
						lead = FALSE;
					}
				}
				if (lead) {
					text++;
				}
			}
		}
	}
	f32 zero = 0.0f;
	for (int i = 0; i < menu->mHelpCount; i++) {
		MenuHelpChar* glyph = &menu->mHelpChars[i];
		CFont font;
		s32 width;
		CTexObj tex  = *GetFontGlyphTexture(&font, glyph->mText, 0, &width);
		f32 bob      = 2.0f * (f32)sin(glyph->mAngle0);
		f32 rotation = sin(0.7f * glyph->mAngle1);
		f32 s1       = sin(3.1415927f * alpha);
		f32 s2       = sin(1.5707964f * alpha);
		f32 scale    = 0.2f * s1 + s2;
		f32 x        = 0.5f * spacing + (i * spacing + (center - menu->mHelpCount * spacing / 2.0f));
		f32 y        = bottom + bob;
		rotation     = -(0.017453292f * (10.0f * rotation));
		u32 color    = (u32)(255.0f * alpha) | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &blob, color, color, color, color, x, y, 1.0f, 30.0f * scale, 30.0f * scale, 0.0f, -zero, rotation);
		scale       = 30.0f * (0.8f * scale);
		f32 padding = HelpGlyphPadding(width, scale);
		color       = (u32)(255.0f * alpha);
		for (int j = 0; j < 4; j++) {
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x + gStatusHelpOutlineX[j] + padding / 2.0f,
			                   bob + (bottom + gStatusHelpOutlineY[j]), 1.0f, scale, scale, 0.0f, -zero, rotation);
		}
		color = (u32)(255.0f * alpha) | 0xFFFFFB00;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x + padding / 2.0f, y, 1.0f, scale, scale, 0.0f, -zero, rotation);
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// NONMATCHING: the call list matches retail; this build is 4 bytes larger and register allocation differs
extern "C" void DrawSaveHelpText(MenuSave* menu)
{
	f32 alpha   = menu->mAlpha * menu->mHelpAlpha;
	f32 spacing = 30.0f;
	f32 center  = gPointerScreenWidth / 2;
	f32 bottom  = 160.0f + gPointerScreenHeight / 2;
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	CTexObj blob;
	InitPCPanelTexture(blob, 109);
	menu->mHelpCount = 0;
	const char* text = menu->mHelpText;
	if (text) {
		for (int i = 0; i < 20 && *text; i++, text++) {
			menu->mBg[i].mText = text;
			menu->mHelpCount++;
			if (gGame.mLanguage == 0) {
				u8 c    = *text;
				u8 lead = TRUE;
				u8 a    = (c >= 0x81 && c <= 0x9F);
				if (!a) {
					u8 b = (c >= 0xE0 && c <= 0xFC);
					if (!b) {
						lead = FALSE;
					}
				}
				if (lead) {
					text++;
				}
			}
		}
	}
	f32 zero = 0.0f;
	for (int i = 0; i < menu->mHelpCount; i++) {
		MenuHelpChar* glyph = &menu->mBg[i];
		CFont font;
		s32 width;
		CTexObj tex  = *GetFontGlyphTexture(&font, glyph->mText, 0, &width);
		f32 bob      = 2.0f * (f32)sin(glyph->mAngle0);
		f32 rotation = sin(0.7f * glyph->mAngle1);
		f32 s1       = sin(3.1415927f * alpha);
		f32 s2       = sin(1.5707964f * alpha);
		f32 scale    = 0.2f * s1 + s2;
		f32 x        = 0.5f * spacing + (i * spacing + (center - menu->mHelpCount * spacing / 2.0f));
		f32 y        = bottom + bob;
		rotation     = -(0.017453292f * (10.0f * rotation));
		u32 color    = (u32)(255.0f * alpha) | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &blob, color, color, color, color, x, y, 1.0f, 30.0f * scale, 30.0f * scale, 0.0f, -zero, rotation);
		scale       = 30.0f * (0.8f * scale);
		f32 padding = HelpGlyphPadding(width, scale);
		color       = (u32)(255.0f * alpha);
		for (int j = 0; j < 4; j++) {
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x + gSaveHelpOutlineX[j] + padding / 2.0f,
			                   bob + (bottom + gSaveHelpOutlineY[j]), 1.0f, scale, scale, 0.0f, -zero, rotation);
		}
		color = (u32)(255.0f * alpha) | 0xFFFFFB00;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x + padding / 2.0f, y, 1.0f, scale, scale, 0.0f, -zero, rotation);
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// NONMATCHING: the call list matches retail; this build is 16 bytes smaller and register allocation differs
extern "C" void DrawPlayerStatusIcon(s32 id, u8 available, bool selected, f32 x, f32 y, f32 alpha, f32 scale, f32 phase)
{
	if (alpha > 0.0f) {
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		f32 width;
		CTexObj tex;
		u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
		if (!available) {
			id = 241;
		}
		f32 bob = 0.0f;
		if (selected) {
			bob = 4.0f * (f32)sin(0.017453292f * (20.0f * (f32)fmod(sStatusPhase, 360.0)));
		}
		InitPCPanelTexture(tex, id);
		width      = scale * GXGetTexObjWidth(&tex);
		f32 height = scale * GXGetTexObjHeight(&tex);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		gGame.mGs.mCurRenderState->SetZCompLoc(GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		y += bob;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZCompLoc(GX_TRUE);
		if (selected) {
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_EQUAL, GX_FALSE);
			f32 pulse = alpha * (0.2f * (f32)sin(3.1415927f * phase) + 0.2f);
			if (pulse < 0.0f) {
				pulse = 0.0f;
			}
			DrawGsQuad(&gGame.mGs, (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00,
			           (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00, x, y, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		}
	}
}

static inline void DrawStatusGridIcon(CTexObj& tex, u32 color, f32 x, f32 y, f32 scale, f32& width, f32& height)
{
	width  = scale * GXGetTexObjWidth(&tex);
	width  = 0.8f * width;
	height = scale * GXGetTexObjHeight(&tex);
	height = 0.8f * height;
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	gGame.mGs.mCurRenderState->SetZCompLoc(GX_FALSE);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x - 2.0f, y, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	gGame.mGs.mCurRenderState->SetZCompLoc(GX_TRUE);
}

// NONMATCHING: the call list matches retail; this build is 32 bytes smaller and register allocation differs
extern "C" void DrawStatusIconGrid(MenuStatus* menu)
{
	if (menu->mMode == 8 || menu->mMode == 4) {
		f32 left     = ((menu->mGridX - 42.0f) + menu->mOffsetX) - 201.0f;
		f32 top      = ((180.0f + menu->mGridY) + menu->mOffsetY) - 81.0f;
		s32 selected = menu->mPhase == 100 ? -1 : menu->mIconSelection;
		{
			CTexObj tex;
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
			for (int i = 0; i < 24; i++) {
				f32 x     = 2.0f + ((i % 6) * 80.0f + left);
				f32 y     = 2.0f + ((i / 6) * 60.0f + top);
				u32 color = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
				if (gStatusIconUnlocked[i] != 0) {
					InitPCPanelTexture(tex, gStatusGridTextures[i * 2 + 1]);
				} else {
					InitPCPanelTexture(tex, gStatusGridTextures[i * 2]);
				}
				f32 bob = (f32)sin(0.017453292f * (20.0f * (f32)fmod(sStatusPhase, 360.0))) * 4.0f;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				f32 width, height;
				if (selected == i) {
					DrawStatusGridIcon(tex, color, x, bob + (y - 2.0f), 1.0f, width, height);
				} else {
					DrawStatusGridIcon(tex, color, x, y - 2.0f, 1.0f, width, height);
				}
				if (selected == i) {
					gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_DSTCOL, GX_BL_SRCALPHA, GX_LO_SET);
					gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_EQUAL, GX_FALSE);
					f32 wave  = (f32)sin(3.1415927f * menu->mPulsePhase);
					f32 pulse = (0.2f * wave + 0.2f) * menu->mAlpha;
					if (pulse < 0.0f) {
						pulse = 0.0f;
					}
					DrawGsQuad(&gGame.mGs, (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00,
					           (u32)(255.0f * pulse) | 0xFFFFFF00, (u32)(255.0f * pulse) | 0xFFFFFF00, x, y + bob, 1.0f, width, height,
					           0.0f, 0.0f, 0.0f);
					gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
					gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
				}
			}
		}
		DrawStatusIconDetails(menu->mIconSelection, gPointerScreenWidth / 2, gPointerScreenHeight / 2, menu->mAlpha, menu->mDetailScale);
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

static inline s32 GetMoney();
static inline s32 GetScrap();

void MenuPC::Open()
{
	CMenu::Open();
	mWindow.SetSize(0.0f, 0.0f);
	mWindow.SetPos(0.0f, 0.0f);
	mMode               = 0;
	mPrevMode           = mMode;
	mPhase              = 0;
	mTextPhase          = 0;
	mChoice             = 0;
	mItemShopSelection  = 0;
	_17C                = 0;
	_180                = 0;
	mMoneyDigit         = 0;
	mScrapShopSelection = 0;
	mScrapDigit         = 0;
	mProgress           = 0.0f;
	mTargetProgress     = 0.0f;
	mSpeed              = 1.0f;
	_19C                = -10.0f;
	mFrameHeight        = 0.0f;
	_1A4                = -10.0f;
	mTargetFrameHeight  = 60.0f;
	mTextX              = 210.0f;
	mTextY              = 129.0f;
	mTargetX            = mTextX;
	mTargetY            = mTextY;
	ResetPCCharacters(this);
	mAmount          = 0;
	mListAlpha       = 1.0f;
	mTargetListAlpha = 1.0f;
	mPanelAlpha      = 0.0f;
	mPanelTarget     = 0.0f;
	for (int i = 0; i < 4; i++) {
		mChoices[i] = -1;
	}
	mChoiceCount = 0;
	mTexScroll   = 0.0f;
	if (IsStage("stage05")) {
		mShopOnly = FALSE;
	} else {
		mShopOnly = TRUE;
	}
	if (gGame.mRoomCount + 1 == gGame.mStage) {
		mShopOnly = FALSE;
	}
	FindStageIndexByName("shun");
	mItemShopBase  = 600;
	mItemShopCount = 20;
	if (mItemShopCount > 0) {
		for (int i = 0; i < mItemShopCount; i++) {
			MenuPCItem* row = &sItemShopEntries[i];
			s32 id          = gEventVars.mVars[mItemShopBase + i];
			row->mItem      = id;
			row->mCount     = gEventVars.mVars[mItemShopBase + i + mItemShopCount];
			if (id != -1) {
				if (id & 0x10000) {
					id &= ~0x10000;
					row->mItem         = id;
					row->mIsAttachment = TRUE;
					row->mPrice        = GetAttachmentShopPrice(id);
				} else {
					row->mIsAttachment = FALSE;
					row->mPrice        = GetItemShopPrice(id);
				}
			}
		}
	}
	mChoices[mChoiceCount++] = 3;
	mCanSelect               = TRUE;
	mScrapShopBase           = 560;
	mScrapShopCount          = 20;
	int count                = 0;
	if (mScrapShopCount > 0) {
		for (int i = 0; i < mScrapShopCount; i++) {
			MenuPCItem* row = &sScrapShopEntries[i];
			s32 id          = gEventVars.mVars[mScrapShopBase + i];
			row->mItem      = id;
			row->mCount     = gEventVars.mVars[mScrapShopBase + i + mScrapShopCount];
			if (id != -1) {
				row->mPrice = GetUtilibotPrice(id);
				count++;
			}
		}
	}
	if (count == 0) {
		mCanSelect = FALSE;
	}
	mChoices[mChoiceCount++] = 7;
	if (IsUtilibotStatusIconUnlocked()) {
		mChoices[mChoiceCount++] = 10;
	}
	for (int i = 0; i < 5; i++) {
		mRankingOffsets[i]        = 0.0f;
		mRankingRows[i].nameIndex = 0;
		mRankingRows[i].score     = 0;
	}
	mRankingEnd      = 0;
	mGreetingVariant = rand() % 8;
	ClearItemShopPage(&gItemShopPage);
	PopulateItemShopPage(&gItemShopPage);
	ClearScrapShopPage(&gScrapShopPage);
	PopulateScrapShopPage(&gScrapShopPage);
	mScroll                = 0.0f;
	mScrollTarget          = 0.0f;
	s32 money              = GetMoney();
	mMoneyDisplay.shown    = money;
	mMoneyDisplay.target   = money;
	s32 scrap              = GetScrap();
	mScrapDisplay.shown    = scrap;
	mScrapDisplay.target   = scrap;
	mDepositDisplay.shown  = mDepositBalance;
	mDepositDisplay.target = mDepositBalance;
	_428                   = 0.0f;
	_42C                   = 0.0f;
	mScrapRate             = 1.0f;
	mExitState             = -1;
	StopWiiSfx(&gGame.mSound, 0x359, 0.0f);
	gGame.SetStatusFlags(2);
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

static inline void SavePCShopState(MenuPC* menu)
{
	if (menu->mItemShopCount > 0) {
		for (int i = 0; i < menu->mItemShopCount; i++) {
			gEventVars.mVars[menu->mItemShopBase + i + menu->mItemShopCount] = sItemShopEntries[i].mCount;
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
		SavePCShopState(this);
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

// NONMATCHING: four bytes smaller because the result == 0 branch becomes one bne instead of retail's beq plus branch
void MenuSave::State1(u32 frames)
{
	s32 result = gGame.mSaveManager.mFile.mResult;
	mDelay -= frames;
	if (mDelay < 0) {
		mDelay = 0;
	}
	if (result == 0) {
		for (int i = 0; i < 3; i++) {
			WiiSaveScanEntry* entry = &gGame.mSaveManager.mScanEntries[i];
			if (entry->mStatus & 1) {
				fn_8007A2E8(&mSlots[i], entry, TRUE);
				if (entry->mStatus & 2) {
					mSlots[i].mState = 1;
				}
			} else if (entry->mStatus & 8) {
				mSlots[i]._48 = 1;
			} else if (entry->mStatus & 16) {
				mSlots[i]._4C = 1;
			} else {
				mSlots[i]._44 = 1;
			}
		}
		if (mSaved == TRUE) {
			SetMode(4);
			mTime  = 0.5f * gRefreshRate;
			mSaved = FALSE;
		} else {
			SetMode(2);
			fn_80083350(this);
		}
	} else {
		mResult = result;
		if (result == -9 || result == -11) {
			SetMode(11);
			mOffset = -32.0f;
		} else if (result == -4) {
			SetMode(13);
			mShowMessage         = TRUE;
			lbl_8044B700.mActive = TRUE;
		} else {
			SetMode(11);
			mOffset              = -32.0f;
			lbl_8044B700.mActive = TRUE;
		}
		mTargetAlpha = 1.0f;
	}
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

// NONMATCHING: the state transitions match; the result load is scheduled before the delay subtraction here, and the
// outer zero-result branch uses one bne instead of retail's beq plus branch
void MenuSave::State5(u32 frames)
{
	s32 result = gGame.mSaveManager.mFile.mResult;
	mSaved     = FALSE;
	mDelay -= frames;
	if (mDelay < 0) {
		mDelay = 0;
	}
	if (result == 0) {
		WiiSaveScanEntry* entry = &gGame.mSaveManager.mScanEntries[mFile];
		if (entry->mStatus & 1) {
			fn_8007A2E8(&mSlots[mFile], entry, TRUE);
		}
		u32 blocks, inodes;
		GetNandGameFreeSpace(&gGame.mSaveManager.mFile, &blocks, &inodes);
		mFreeBlocks  = blocks;
		mFreeEntries = inodes;
		StartWiiSaveSlotScan(&gGame.mSaveManager, 10);
		SetMode(1);
		mLastFile   = mFile;
		mNewestFile = -1;
		mSaved      = TRUE;
		mOffset     = 0.0f;
	} else {
		mResult = result;
		switch (result) {
		case -9:
		case -11:
			SetMode(13);
			mShowMessage = TRUE;
			break;
		case -4:
			SetMode(13);
			mShowMessage = TRUE;
			break;
		default:
			SetMode(13);
			mShowMessage = TRUE;
			break;
		}
		lbl_8044B700.mActive = TRUE;
		mTargetAlpha         = 1.0f;
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
static s32 sStatusTabTextIds[5] = { 1, 3, 2, 4, -1 };

void MenuStatus::Init()
{
	CMenu::Init();
	mSelection                = 0;
	mStickSelection           = 0;
	mPrevSelection            = 0;
	mHovered                  = 0;
	lbl_802E87E8[49].mTrans.x = 577.0f;
	lbl_802E87E8[49].mTrans.y = (gPointerScreenHeight / 2 + 4) - 126.0f;
	lbl_802E87E8[50].mTrans.x = 577.0f;
	lbl_802E87E8[50].mTrans.y = 126.0f + (gPointerScreenHeight / 2 + 4);
	for (s32 i = 0; i < 51; i++) {
		lbl_802E87E8[i].mTrans.x -= 0.5f * 640;
		lbl_802E87E8[i].mTrans.y = 0.5f * (f32)gPointerScreenHeight - lbl_802E87E8[i].mTrans.y;
		fn_800556B0(&mPanes[i], &lbl_802E87E8[i]);
	}
	s32 width           = gGame.mAspectRatio == 0 ? 640 : 854;
	gRingScreenWidth    = width;
	gPointerScreenWidth = width;
}

void MenuStatus::Open()
{
	CMenu::Open();
	gGame.SetStatusFlags(2);
	mFontSize            = 22.0f;
	mRowCount            = 14.0f;
	mMaxWidth            = 0.0f;
	mOffsetX             = 0.0f;
	mOffsetY             = 0.0f;
	mTargetOffsetX       = 0.0f;
	mTargetOffsetY       = 0.0f;
	mPanelX              = -10.0f;
	mPanelY              = -200.0f;
	mTargetPanelX        = -10.0f;
	mTargetPanelY        = 0.0f;
	mItemPanelX          = 725.0f;
	mItemPanelY          = 100.0f;
	mTargetItemPanelX    = 725.0f;
	mTargetItemPanelY    = 100.0f;
	mDiscardPanelX       = 937.0f;
	mDiscardPanelY       = 100.0f;
	mTargetDiscardPanelX = 937.0f;
	mTargetDiscardPanelY = 100.0f;
	mHasFourthTab        = TRUE;
	for (int i = 0; i < 4; i++) {
		mTabs[i].mX = 758.0f;
		mTabs[i].mY = 36.0f;
		if (mHasFourthTab) {
			mTabs[i].mTargetX = 198.0f + i * 108 - 8.0f;
		} else {
			mTabs[i].mTargetX = 252.0f + i * 108;
		}
		mTabs[i].mTargetY  = 36.0f;
		mTabs2[i].mX       = 758.0f;
		mTabs2[i].mY       = 36.0f;
		mTabs2[i].mTargetX = 758.0f;
		mTabs2[i].mTargetY = 36.0f;
	}
	mX               = 758.0f;
	mY               = 36.0f;
	mTargetX         = 758.0f;
	mTargetY         = 36.0f;
	mOverviewX       = 1086.0f;
	mOverviewY       = 36.0f;
	mTargetOverviewX = 1086.0f;
	mTargetOverviewY = 36.0f;
	mGridX           = 959.0f;
	mGridY           = 36.0f;
	mTargetGridX     = 959.0f;
	mTargetGridY     = 36.0f;
	mDiagramX        = 1086.0f;
	mDiagramY        = 36.0f;
	mTargetDiagramX  = 1086.0f;
	mTargetDiagramY  = 36.0f;
	mWindow.SetSize(0.0f, 0.0f);
	mWindow.SetPos(0.0f, 0.0f);
	mMode                   = 0;
	mPrevMode               = 0;
	mNextMode               = 0;
	sStatusPhase            = 0.0f;
	sInventoryPhase         = 0.0f;
	mPhase                  = 0;
	mDelay                  = 0.0f;
	_6A8                    = 0;
	_6AA                    = 0;
	mItemSelection          = 0;
	mPrevItemSelection      = 0;
	mStickItemSelection     = 0;
	mOverviewSelection      = 0;
	mPrevOverviewSelection  = 0;
	mStickOverviewSelection = 0;
	mIconSelection          = 0;
	mPrevIconSelection      = 0;
	mStickIconSelection     = 0;
	mChoice                 = 0;
	mCount                  = 0;
	ClearStatusTable(&sStatusItemTable);
	PopulateOwnedItemPage(&sStatusItemTable);
	mPrevMode          = mMode;
	mMode              = 0;
	mPhase             = 0;
	mDetailScale       = 0.0f;
	mTargetDetailScale = 0.0f;
	mSavedBgm          = gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0;
	mSavedBgmVolume    = gGame.mSound.mMusicVolume;
	if (!mHasFourthTab && mSelection >= 3) {
		mSelection = 0;
	}
	if (!mHasFourthTab && mStickSelection >= 3) {
		mStickSelection = 0;
	}
	if (gEventFlags.mFlags[0] & 4) {
		mIconLit = TRUE;
	} else {
		mIconLit = FALSE;
	}
	if ((gEventFlags.mFlags[0] & 0x20000000) && (gEventFlags.mFlags[0] & 0x40000000)) {
		mShowGauge = TRUE;
	} else {
		mShowGauge = FALSE;
	}
	mGaugeValue = gEventVars.mVars[0x220 / 4] / 10000.0f;
	for (int i = 0; i < 24; i++) {
		gStatusIconUnlocked[i] = 0;
	}
	for (int i = 0; i < 24; i++) {
		s32 flag = GetStatusIconEventFlagId(i);
		if (flag >= 0 && (gEventFlags.mFlags[(u32)flag / 32] & (1u << (flag & 31)))) {
			gStatusIconUnlocked[i] = 1;
		}
	}
	for (int i = 0; i < 13; i++) {
		gStatusEntryAvailable[i] = 0;
	}
	MarkAvailableStatusEntries(gStatusEntryAvailable);
	const char* title = GetLocalizedInterfaceText(0x18);
	fn_80061FF4(this, title, (u32)(255.0f * mAlpha) | 0xFFFFFF00);
	mShowHelp  = 0;
	mHelpAlpha = 0.0f;
	mHelpText  = NULL;
	mSelection = -1;
	mShowHelp  = 1;
	mHelp      = NULL;
	mHelpCount = 0;
	for (int i = 0; i < 20; i++) {
		mHelpChars[i].mAngle0 = DEG2RAD * (rand() % 360);
		mHelpChars[i].mAngle1 = DEG2RAD * (rand() % 360);
	}
	gPointerPanes.mMenu   = this;
	gPointerPanes.mActive = TRUE;
	for (int i = 0; i < 51; i++) {
		mPanes[i].mPointed = FALSE;
		mPanes[i].mPicked  = FALSE;
	}
	mHovered                      = 0;
	mPanes[49].mEnabled           = FALSE;
	mPanes[50].mEnabled           = FALSE;
	mPanes[49]._48                = 0.0f;
	mPanes[50]._48                = 0.0f;
	mPaneFades.mFades[0].mPane    = &mPanes[49];
	mPaneFades.mFades[0].mAlpha   = 0.0f;
	mPaneFades.mFades[0].mShown   = FALSE;
	mPaneFades.mFades[0].mHovered = FALSE;
	mPaneFades.mFades[1].mPane    = &mPanes[50];
	mPaneFades.mFades[1].mAlpha   = 0.0f;
	mPaneFades.mFades[1].mShown   = FALSE;
	mPaneFades.mFades[1].mHovered = FALSE;
	u32 controls                  = gGame.mMenuControls;
	if ((controls & MenuControlStick) && (controls & MenuControlPointer)) {
		_1954 = 0x10C;
	} else if (controls & MenuControlPointer) {
		_1954 = 0x10D;
	} else if (controls & MenuControlStick) {
		_1954 = 0x10E;
	}
}

static void (MenuStatus::*gStatusStepHandlers[])(u32)
    = { &MenuStatus::State0, &MenuStatus::State1, &MenuStatus::State2, &MenuStatus::State3, &MenuStatus::State4,
	    &MenuStatus::State5, &MenuStatus::State6, &MenuStatus::State7, &MenuStatus::State8, &MenuStatus::State9 };

// NONMATCHING: retail calls MenuPaneFadePair::Update where this build expands it, adding 180 bytes.
BOOL MenuStatus::Update(u32 frames)
{
	if (mState == 6) {
		gPointerPanes.mMenu = NULL;
	}
	if (CMenu::Update(frames) == 4) {
		gPointerPanes.mCount = 0;
		mPaneFades.Update(frames);
		sStatusPhase += gFrameRateScale * (0.5f * frames);
		sInventoryPhase += gFrameRateScale * (0.5f * frames);
		if ((mPulsePhase += (1.0f / (0.3f * gRefreshRate)) * frames) >= 2.0f) {
			mPulsePhase = fmod(mPulsePhase, 2.0);
		}
		PCEase(mOffsetX, mTargetOffsetX);
		PCEase(mOffsetY, mTargetOffsetY);
		PCEase(mPanelX, mTargetPanelX);
		PCEase(mPanelY, mTargetPanelY);
		for (int i = 0; i < 4; i++) {
			PCEase(mTabs[i].mX, mTabs[i].mTargetX);
			PCEase(mTabs[i].mY, mTabs[i].mTargetY);
			PCEase(mTabs2[i].mX, mTabs2[i].mTargetX);
			PCEase(mTabs2[i].mY, mTabs2[i].mTargetY);
		}
		PCEase(mX, mTargetX);
		ApproachHudValue(&mY, &mTargetY, 20.0f);
		PCEase(mOverviewX, mTargetOverviewX);
		ApproachHudValue(&mDetailScale, &mTargetDetailScale, 0.08f * frames);
		PCEase(mGridX, mTargetGridX);
		ApproachHudValue(&mGridY, &mTargetGridY, 20.0f);
		PCEase(mDiagramX, mTargetDiagramX);
		ApproachHudValue(&mDiagramY, &mTargetDiagramY, 20.0f);
		(this->*gStatusStepHandlers[mMode])(frames);
	}
	fn_80062338(this, frames);
	switch (mShowHelp) {
	case 0:
		break;
	case 1:
		mHelpAlpha = 0.0f;
		mShowHelp++;
		mHelpText = mHelp;
		break;
	case 2:
		mHelpAlpha += (1.0f / (0.2f * gRefreshRate)) * frames;
		if (mHelpAlpha >= 1.0f) {
			mHelpAlpha = 1.0f;
			mShowHelp  = 0;
		}
		break;
	case 3:
		mHelpAlpha -= (1.0f / (0.25f * gRefreshRate)) * frames;
		if (mHelpAlpha <= 0.0f) {
			mHelpAlpha = 0.0f;
		}
		break;
	}
	if (mAlpha > 0.0f) {
		for (int i = 0; i < 20; i++) {
			mHelpChars[i].mAngle0 += 0.08726646f * frames;
			mHelpChars[i].mAngle1 += 0.05235988f * frames;
		}
	}
	return IsOpen();
}

// NONMATCHING: retail expands StartSound here; this build calls its out-of-line copy.
void MenuStatus::State0(u32)
{
	mSavedBgm       = gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0;
	mSavedBgmVolume = gGame.mSound.mMusicVolume;
	StartSound(&gGame.mSound, 4, 1.0f, 0);
	mPrevMode = mMode;
	mMode     = 4;
	mNextMode = 1;
	mPhase    = 0;
}

static inline const char* StatusTabHelp(s32 selection)
{
	if (selection == -1) {
		return NULL;
	}
	if (!gStatusEntryAvailable[selection]) {
		OSPanic("menu_status.cpp", 539, "ここにきたらおかしい");
	}
	s32 id           = selection >= 4 ? 0 : sStatusTabTextIds[selection];
	const char* text = "";
	if (id != -1) {
		text = GetLocalizedInterfaceText(id + 0x18);
	}
	return text;
}

// the status menu's tabs: pick one and open its page, or close the menu
void MenuStatus::State1(u32 frames)
{
	bool addPanes = true;
	if (mSelection != -1) {
		mStickSelection = mSelection;
	}
	switch (mPhase) {
	case 0: {
		s32 last   = mHasFourthTab ? 3 : 2;
		bool stick = false;
		if (IsStickUsedOverPointer()) {
			stick = true;
		}
		if (!stick) {
			if (IsStickLeft()) {
				if (mSelection != -1 && --mSelection < 0) {
					mSelection = last;
				}
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			} else if (IsStickRight()) {
				if (mSelection != -1 && ++mSelection > last) {
					mSelection = 0;
				}
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		}
		if (mSelection != mPrevSelection) {
			const char* text = StatusTabHelp(mSelection);
			if (text != NULL) {
				mHelp     = text;
				mShowHelp = 1;
			} else {
				mShowHelp = 3;
			}
		}
		mPrevSelection = mSelection;
		if (gWiiPointer.Check(0x80) || gWiiPointer.Check(0x400) || IsCancelPressed()) {
			for (int i = 0; i < 4; i++) {
				mTabs[i].mTargetX = -102.0f;
			}
			mTargetPanelY = -200.0f;
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			mPhase++;
			mDelay = 8.0f * gFrameRateScale;
			for (int i = 0; i < 20; i++) {
				mTitle.mChars[i].mHold = 0.0f;
			}
			addPanes = false;
		} else if (IsConfirmPressed()) {
			switch (mSelection) {
			case 0:
				mPrevMode = mMode;
				mMode     = 4;
				mNextMode = 2;
				mPhase    = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				for (int i = 0; i < 4; i++) {
					mTabs[i].mTargetX = -102.0f;
				}
				for (int i = 0; i < 4; i++) {
					mTabs2[i].mX = 758.0f;
					if (mHasFourthTab) {
						mTabs2[i].mTargetX = 198.0f + i * 108 - 8.0f;
					} else {
						mTabs2[i].mTargetX = 252.0f + i * 108;
					}
				}
				{
					s32 count = CountMenuItems();
					for (int i = 0; i < 4; i++) {
						mTabs2[i].mTargetX = 0.0f;
					}
					mTargetX = 360.0f - (count - 1) * 51;
					if (count == 0) {
						mTargetX = 360.0f;
					}
					mTargetX = 207.0f;
				}
				{
					const char* text = GetLocalizedInterfaceText(0x19);
					fn_80061FF4(this, text, (u32)(255.0f * mAlpha) | 0xFFFFFF00);
				}
				mItemSelection = -1;
				mShowHelp      = 3;
				break;
			case 1:
				mTargetOverviewX = 360.0f;
				mPrevMode        = mMode;
				mMode            = 4;
				mNextMode        = 9;
				mPhase           = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				for (int i = 0; i < 4; i++) {
					mTabs[i].mTargetX = -102.0f;
				}
				{
					const char* text = GetLocalizedInterfaceText(0x1B);
					fn_80061FF4(this, text, (u32)(255.0f * mAlpha) | 0xFFFFFF00);
				}
				mOverviewSelection = -1;
				mShowHelp          = 3;
				break;
			case 2:
				mTargetDiagramX = 320.0f;
				mPrevMode       = mMode;
				mMode           = 4;
				mNextMode       = 5;
				mPhase          = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				for (int i = 0; i < 4; i++) {
					mTabs[i].mTargetX = -102.0f;
				}
				{
					const char* text = GetLocalizedInterfaceText(0x1A);
					fn_80061FF4(this, text, (u32)(255.0f * mAlpha) | 0xFFFFFF00);
				}
				mShowHelp = 3;
				break;
			case 3:
				mTargetGridX = 360.0f;
				mPrevMode    = mMode;
				mMode        = 4;
				mNextMode    = 8;
				mPhase       = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				for (int i = 0; i < 4; i++) {
					mTabs[i].mTargetX = -102.0f;
				}
				{
					const char* text = GetLocalizedInterfaceText(0x1C);
					fn_80061FF4(this, text, (u32)(255.0f * mAlpha) | 0xFFFFFF00);
				}
				mIconSelection = -1;
				mShowHelp      = 3;
				break;
			default:
				PlayWiiSoundEffect(&gGame.mSound, 0x1B4, 1.0f, 0.0f);
				break;
			}
			if (mSelection != -1) {
				for (int i = 0; i < 4; i++) {
					mPanes[i].mPointed = FALSE;
					mPanes[i].mPicked  = FALSE;
				}
				addPanes = false;
			}
		}
		break;
	}
	case 1:
		mDelay -= frames;
		if (mDelay < 0.0f) {
			gGame.ClearStatusFlags(2);
			Close(5);
			sMenuMain.Resume();
			StopWiiSfx(&gGame.mSound, 4, 0.0f);
			mPhase++;
		}
		addPanes = false;
		break;
	case 2:
		addPanes = false;
		break;
	}
	if (addPanes) {
		for (int i = 0; i < 4; i++) {
			AddPointerPane(&gPointerPanes, &mPanes[i]);
		}
	}
}

static inline void UpdateOwnedItemHelp(MenuStatus* menu)
{
	if (menu->mItemSelection != -1 && sStatusItemTable.mItems[menu->mItemSelection].mItem < 0) {
		do {
			--menu->mItemSelection;
		} while (sStatusItemTable.mItems[menu->mItemSelection].mItem < 0 && menu->mItemSelection != 0);
	}
	if (menu->mItemSelection != menu->mPrevItemSelection) {
		s16 id = menu->mItemSelection != -1 ? sStatusItemTable.mItems[menu->mItemSelection].mItem : -1;
		if (id >= 0 && CountMenuItems() > 0) {
			menu->mHelp     = (const char*)GetItemName(id);
			menu->mShowHelp = 1;
		} else {
			menu->mShowHelp = 3;
		}
	}
	menu->mPrevItemSelection = menu->mItemSelection;
}

static inline void FinishStatusItemScroll(MenuStatus* menu)
{
	menu->mY       = 36.0f;
	menu->mTargetY = 36.0f;
	fn_800695EC(menu);
	if (menu->mItemSelection == -1) {
		menu->mShowHelp = 3;
	} else {
		s16 id = sStatusItemTable.mItems[menu->mItemSelection].mItem;
		if (id != -1) {
			menu->mHelp              = (const char*)GetItemName(id);
			menu->mShowHelp          = 1;
			menu->mPrevItemSelection = menu->mItemSelection;
		}
	}
}

static inline void SelectDiscardQuantity(MenuStatus* menu)
{
	if (menu->mItemSelection == -1) {
		OSPanic("menu_status.cpp", 1057, "ここが-1になるのはおかしい！");
	}
	s32 num = sStatusItemTable.mItems[menu->mItemSelection].mCount;
	if (num >= 1) {
		if (menu->IsStickLeft()) {
			if (--menu->mCount < 1) {
				menu->mCount = num;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (menu->IsStickRight()) {
			if (++menu->mCount > num) {
				menu->mCount = 1;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		if (menu->IsConfirmPressed()) {
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			menu->mPhase++;
		} else if (menu->IsCancelPressed()) {
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			menu->mTargetItemPanelX    = 725.0f;
			menu->mTargetDiscardPanelX = 937.0f;
			menu->mPhase               = 0;
		}
	}
}

static inline void ConfirmDiscardQuantity(MenuStatus* menu, bool inventoryTab)
{
	if (menu->mItemSelection == -1) {
		OSPanic("menu_status.cpp", 1099, "ここが-1になるのはおかしい！");
	}
	if (menu->IsStickLeft()) {
		if (menu->mChoice > 0) {
			menu->mChoice--;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
	} else if (menu->IsStickRight()) {
		if (menu->mChoice < 1) {
			menu->mChoice++;
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
	}
	if (menu->IsCancelPressed()) {
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		menu->mPhase = 1;
	} else if (menu->IsConfirmPressed()) {
		if (menu->mChoice == 0) {
			MenuStatusEntry* entry = &sStatusItemTable.mItems[menu->mItemSelection];
			RemoveInventoryItemCount(&gGame.GetPlayer()->mParams.mInventory, entry->mItem, (s16)menu->mCount);
			menu->mTargetItemPanelX    = 725.0f;
			menu->mTargetDiscardPanelX = 937.0f;
			menu->mPhase               = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			menu->mItemSelection = 0;
			PopulateOwnedItemPage(&sStatusItemTable);
			menu->mTargetX = 360.0f - (CountMenuItems() - 1) * 54;
			if (CountMenuItems() == 0) {
				menu->mTargetX = 360.0f;
			}
			if (inventoryTab) {
				menu->mTargetX = 207.0f;
			}
		} else {
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			menu->mPhase = 1;
		}
	}
}

// the status menu's owned items, their details and the discard flow
// NONMATCHING: retail calls fn_800695EC after either page scroll; this build expands both calls, making the function
// 1396 bytes larger. The other call, control flow, field offsets and constants match.
void MenuStatus::State2(u32)
{
	bool addPanes         = true;
	gPointerPanes.mActive = TRUE;
	if (mItemSelection != -1) {
		mStickItemSelection = mItemSelection;
	}
	s32 count = CountMenuItems();
	switch (mPhase) {
	case 0: {
		bool stick = false;
		if (IsStickUsedOverPointer()) {
			stick = true;
		}
		if (!stick) {
			if (count > 1) {
				if (IsStickLeft()) {
					if (--mItemSelection < 0) {
						mItemSelection = count - 1;
					}
					PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
				} else if (IsStickRight()) {
					mItemSelection++;
					if (sStatusItemTable.mItems[mItemSelection].mItem < 0) {
						mItemSelection = 0;
					}
					PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
				}
			}
			if (count > 4) {
				if (mItemSelection >= 4) {
					if (IsStickUp()) {
						if ((mItemSelection -= 4) < 0) {
							mItemSelection = count - 1;
						}
						PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
					}
					if (IsStickDown() && sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
						mTargetY = -56.0f;
						mPhase   = 10;
						addPanes = false;
						break;
					}
				} else {
					if (IsStickDown() && mItemSelection + 4 <= count - 1) {
						mItemSelection += 4;
						PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
					}
					if (IsStickUp()) {
						CountMenuItems();
						if (sStatusItemTable.mPage > 0) {
							mTargetY = 128.0f;
							mPhase   = 20;
							addPanes = false;
							break;
						}
					}
				}
			}
		}
		UpdateOwnedItemHelp(this);
		if (IsCancelPressed()) {
			mPrevMode = mMode;
			mMode     = 4;
			mNextMode = 1;
			mPhase    = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			for (int i = 0; i < 4; i++) {
				mTabs2[i].mX = 0.0f;
				if (mHasFourthTab) {
					mTabs2[i].mTargetX = 198.0f + i * 108 - 8.0f;
				} else {
					mTabs2[i].mTargetX = 252.0f + i * 108;
				}
			}
			mTargetX = 758.0f;
			for (int i = 0; i < 4; i++) {
				mTabs[i].mX = 0.0f;
				if (mHasFourthTab) {
					mTabs[i].mTargetX = 198.0f + i * 108 - 8.0f;
				} else {
					mTabs[i].mTargetX = 252.0f + i * 108;
				}
			}
			for (int i = 0; i < 4; i++) {
				mTabs2[i].mTargetX = 758.0f;
			}
			const char* text = GetLocalizedInterfaceText(0x18);
			fn_80061FF4(this, text, (u32)(255.0f * mAlpha) | 0xFFFFFF00);
			mSelection   = -1;
			mShowHelp    = 3;
			sStatusPhase = 0.0f;
			for (int i = 0; i < 8; i++) {
				mPanes[i + 4].mPointed = FALSE;
				mPanes[i + 4].mPicked  = FALSE;
			}
			mPanes[49].mPointed = FALSE;
			mPanes[49].mPicked  = FALSE;
			mPanes[50].mPointed = FALSE;
			mPanes[50].mPicked  = FALSE;
			addPanes            = false;
		} else if (IsConfirmPressed() && mItemSelection != -1 && !mPanes[49].mPointed && !mPanes[50].mPointed) {
			if (count > 0) {
				mTargetDetailScale = 1.0f;
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				mPhase = 100;
			} else {
				PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			}
			gPointerPanes.mActive = FALSE;
			addPanes              = false;
		}
		break;
	}
	case 1:
		SelectDiscardQuantity(this);
		break;
	case 2:
		ConfirmDiscardQuantity(this, true);
		break;
	case 10:
		if (mY == mTargetY) {
			mPhase = 0;
			if (sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
				sStatusItemTable.mPage++;
				PopulateOwnedItemPage(&sStatusItemTable);
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			FinishStatusItemScroll(this);
		}
		addPanes = false;
		break;
	case 20:
		if (mY == mTargetY) {
			mPhase = 0;
			CountMenuItems();
			if (sStatusItemTable.mPage > 0) {
				sStatusItemTable.mPage--;
				PopulateOwnedItemPage(&sStatusItemTable);
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			FinishStatusItemScroll(this);
		}
		addPanes = false;
		break;
	case 100:
		if (IsConfirmPressed() || IsCancelPressed()) {
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			mPhase             = 101;
			mTargetDetailScale = 0.0f;
		}
		gPointerPanes.mActive = FALSE;
		addPanes              = false;
		break;
	case 101:
		if (mDetailScale == mTargetDetailScale) {
			mPhase = 0;
		}
		gPointerPanes.mActive = FALSE;
		addPanes              = false;
		break;
	}
	if (addPanes) {
		for (int i = 0; i < 8; i++) {
			AddPointerPane(&gPointerPanes, &mPanes[i + 4]);
		}
		AddPointerPane(&gPointerPanes, &mPanes[49]);
		AddPointerPane(&gPointerPanes, &mPanes[50]);
	}
	if (mPhase == 0) {
		mPanes[49].mEnabled = FALSE;
		mPanes[50].mEnabled = FALSE;
		if (count > 4) {
			mPanes[49].mEnabled = sStatusItemTable.mPage > 0;
			mPanes[50].mEnabled = sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1;
		}
	}
}

// NONMATCHING: the mode-specific text selection and transition match Wii retail, but all four fn_800695EC calls inline;
// the emitted function grows to 3624 bytes instead of calling it four times in 712 bytes
void MenuStatus::State4(u32 frames)
{
	CPad pad(0); // unused, retail builds one here
	switch (mPhase) {
	case 0:
		mTargetPanelY = -200.0f;
		mDelay        = 8.0f * gFrameRateScale;
		mPhase++;
		// fall through
	case 1:
		mDelay -= frames;
		if (mDelay < 0.0f) {
			if (mNextMode != 5) {
				mTargetPanelY = 0.0f;
			}
			mPrevMode = mMode;
			mMode     = mNextMode;
			mPhase    = 0;
			if (mMode == 1) {
				fn_800695EC(this);
				s32 id;
				if (mSelection >= 4) {
					id = 0;
				} else {
					id = sStatusTabTextIds[mSelection == -1 ? 4 : mSelection];
				}
				const char* text = "";
				if (id != -1) {
					text = GetLocalizedInterfaceText(id + 0x18);
				}
				mHelp     = text;
				mShowHelp = 1;
			}
			if (mMode == 2) {
				fn_800695EC(this);
				s32 id = -1;
				if (mItemSelection != -1) {
					id = sStatusItemTable.mItems[mItemSelection].mItem;
				}
				if (id >= 0 && CountMenuItems() > 0) {
					mHelp     = (const char*)GetItemName(id);
					mShowHelp = 1;
				} else {
					mShowHelp = 1;
					mHelp     = NULL;
				}
			}
			if (mMode == 9) {
				fn_800695EC(this);
				s32 id = mOverviewSelection;
				const char* text;
				if (id == -1) {
					text = NULL;
				} else if (gStatusEntryAvailable[id]) {
					text = GetStatusOverviewLabel(id);
				} else {
					text = GetLocalizedInterfaceText(0xF);
				}
				mHelp     = text;
				mShowHelp = 1;
			}
			if (mMode == 8) {
				fn_800695EC(this);
				s32 id = mIconSelection;
				const char* text;
				if (id == -1) {
					text = NULL;
				} else if (gStatusIconUnlocked[id]) {
					text = (const char*)GetStatusIconName(id);
				} else {
					text = GetLocalizedInterfaceText(0xF);
				}
				mHelp     = text;
				mShowHelp = 1;
			}
		}
		break;
	}
}

void MenuStatus::State5(u32)
{
	CPad pad(0);
	if (IsCancelPressed()) {
		mTargetDiagramX = 1086.0f;
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		for (int i = 0; i < 4; i++) {
			mTabs[i].mX = 0.0f;
			if (mHasFourthTab) {
				mTabs[i].mTargetX = 198.0f + i * 108 - 8.0f;
			} else {
				mTabs[i].mTargetX = 252.0f + i * 108;
			}
		}
		mPrevMode        = mMode;
		mMode            = 4;
		mNextMode        = 1;
		mPhase           = 0;
		const char* text = GetLocalizedInterfaceText(0x18);
		fn_80061FF4(this, text, (u32)(255.0f * mAlpha) | 0xFFFFFF00);
		mSelection   = -1;
		mShowHelp    = 3;
		sStatusPhase = 0.0f;
	}
}

// NONMATCHING: navigation, help transitions and pointer panes match; this build fully unrolls the 24-pane clear loop,
// while retail keeps three groups of eight, and the tab reset uses different float registers
void MenuStatus::State8(u32)
{
	bool addPanes = true;
	if (mIconSelection != -1) {
		mStickIconSelection = mIconSelection;
	}
	switch (mPhase) {
	case 0: {
		bool stick = false;
		if (IsStickUsedOverPointer()) {
			stick = true;
		}
		if (!stick) {
			if (IsStickLeft()) {
				if (--mIconSelection < 0) {
					mIconSelection = 23;
				}
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			} else if (IsStickRight()) {
				if (++mIconSelection > 23) {
					mIconSelection = 0;
				}
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			if (IsStickUp()) {
				s32 row = mIconSelection / 6;
				s32 col = mIconSelection % 6;
				if (--row < 0) {
					row = 3;
				}
				mIconSelection = col + row * 6;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			} else if (IsStickDown()) {
				s32 row = mIconSelection / 6;
				s32 col = mIconSelection % 6;
				if (++row > 3) {
					row = 0;
				}
				mIconSelection = col + row * 6;
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		}
		if (mIconSelection != mPrevIconSelection) {
			s32 id = mIconSelection;
			const char* text;
			if (id == -1) {
				text = NULL;
			} else if (gStatusIconUnlocked[id]) {
				text = (const char*)GetStatusIconName(id);
			} else {
				text = GetLocalizedInterfaceText(0xF);
			}
			if (text != NULL) {
				mHelp     = text;
				mShowHelp = 1;
			} else {
				mShowHelp = 3;
			}
		}
		mPrevIconSelection = mIconSelection;
		if (IsCancelPressed()) {
			mTargetGridX = 959.0f;
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			for (int i = 0; i < 4; i++) {
				mTabs[i].mX = 0.0f;
				if (mHasFourthTab) {
					mTabs[i].mTargetX = 198.0f + i * 108 - 8.0f;
				} else {
					mTabs[i].mTargetX = 252.0f + i * 108;
				}
			}
			mPrevMode        = mMode;
			mMode            = 4;
			mNextMode        = 1;
			mPhase           = 0;
			const char* text = GetLocalizedInterfaceText(0x18);
			fn_80061FF4(this, text, (u32)(255.0f * mAlpha) | 0xFFFFFF00);
			mSelection   = -1;
			mShowHelp    = 3;
			sStatusPhase = 0.0f;
			for (int i = 0; i < 24; i++) {
				mPanes[i + 25].mPointed = FALSE;
				mPanes[i + 25].mPicked  = FALSE;
			}
			addPanes = false;
		}
		break;
	}
	case 100:
		if (IsConfirmPressed() || IsCancelPressed()) {
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			mPhase             = 0;
			mTargetDetailScale = 0.0f;
		}
		break;
	}
	if (addPanes) {
		for (int i = 0; i < 24; i++) {
			AddPointerPane(&gPointerPanes, &mPanes[i + 25]);
		}
	}
}

// NONMATCHING: the neighbor table and all state transitions match; this build is eight bytes shorter because the
// addPanes initialization and the pane-loop branches are scheduled differently
void MenuStatus::State9(u32)
{
	CPad pad(0);
	s32 neighbors[13][4] = {
		{ 4, 2, 9, 5 },  { 0, 2, 10, 6 }, { 0, 3, 10, 6 },  { 2, 4, 11, 7 }, { 3, 0, 12, 8 },  { 8, 6, 0, 9 },  { 5, 7, 2, 10 },
		{ 6, 8, 3, 11 }, { 7, 5, 4, 12 }, { 12, 10, 5, 0 }, { 9, 11, 6, 2 }, { 10, 12, 7, 3 }, { 11, 9, 8, 4 },
	};
	bool addPanes = true;
	if (mOverviewSelection != -1) {
		mStickOverviewSelection = mOverviewSelection;
	}
	switch (mPhase) {
	case 0: {
		bool stick = false;
		if (IsStickUsedOverPointer()) {
			stick = true;
		}
		if (!stick) {
			if (IsStickLeft()) {
				mOverviewSelection = neighbors[mOverviewSelection][2];
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			} else if (IsStickRight()) {
				mOverviewSelection = neighbors[mOverviewSelection][3];
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			if (IsStickUp()) {
				mOverviewSelection = neighbors[mOverviewSelection][0];
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			} else if (IsStickDown()) {
				mOverviewSelection = neighbors[mOverviewSelection][1];
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
		}
		if (mOverviewSelection != mPrevOverviewSelection) {
			s32 id = mOverviewSelection;
			const char* text;
			if (id == -1) {
				text = NULL;
			} else if (gStatusEntryAvailable[id]) {
				text = GetStatusOverviewLabel(id);
			} else {
				text = GetLocalizedInterfaceText(0xF);
			}
			if (text != NULL) {
				mHelp     = text;
				mShowHelp = 1;
			} else {
				mShowHelp = 3;
			}
		}
		mPrevOverviewSelection = mOverviewSelection;
		if (IsCancelPressed()) {
			mTargetOverviewX = 1086.0f;
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			for (int i = 0; i < 4; i++) {
				mTabs[i].mX = 0.0f;
				if (mHasFourthTab) {
					mTabs[i].mTargetX = 198.0f + i * 108 - 8.0f;
				} else {
					mTabs[i].mTargetX = 252.0f + i * 108;
				}
			}
			mPrevMode        = mMode;
			mMode            = 4;
			mNextMode        = 1;
			mPhase           = 0;
			const char* text = GetLocalizedInterfaceText(0x18);
			fn_80061FF4(this, text, (u32)(255.0f * mAlpha) | 0xFFFFFF00);
			mSelection   = -1;
			mShowHelp    = 3;
			sStatusPhase = 0.0f;
			for (int i = 0; i < 13; i++) {
				if (i != 1) {
					mPanes[i + 12].mPointed = FALSE;
					mPanes[i + 12].mPicked  = FALSE;
				}
			}
			addPanes = false;
		}
		break;
	}
	case 100:
		if (IsConfirmPressed() || IsCancelPressed()) {
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			mPhase             = 0;
			mTargetDetailScale = 0.0f;
		}
		break;
	}
	if (addPanes) {
		for (int i = 0; i < 13; i++) {
			if (i != 1) {
				AddPointerPane(&gPointerPanes, &mPanes[i + 12]);
			}
		}
	}
}

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

extern "C" void fn_80061014(MenuStatus* menu)
{
	if (menu->mMode != 2 && menu->mMode != 4) {
		return;
	}
	s32 width  = gGame.mAspectRatio == 0 ? 640 : 854;
	s32 margin = (width - gRingScreenWidth) / 2;
	GXSetScissor(gScreenLeft + margin, gPointerScreenHeight / 2 - 122, gPointerScreenWidth - margin * 2, 252);
	f32 x = (menu->mX - 42.0f) + menu->mOffsetX;
	f32 y = 180.0f + (menu->mY + menu->mOffsetY);
	DrawInventoryGrid(menu->mPhase == 100 ? -1 : menu->mItemSelection, x, y, menu->mAlpha, 1.0f, menu->mPulsePhase);
	if (menu->mItemSelection != -1) {
		DrawStatusItemDetails(sStatusItemTable.mItems[menu->mItemSelection].mItem, gPointerScreenWidth / 2, gPointerScreenHeight / 2,
		                      menu->mAlpha, menu->mDetailScale);
	}
	CountMenuItems();
	DrawDiscardCountPanel(menu);
	GXSetScissor(gScreenLeft + margin, gScreenTop, gPointerScreenWidth - margin * 2, gPointerScreenHeight);
	fn_8005A128(&menu->mPaneFades, x, y, menu->mAlpha);
}

extern "C" void fn_80062338(MenuStatus* menu, u32 frames)
{
	if (menu->mTitle.mActive && menu->mTitle.mActive > 0) {
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			if (menu->mTitle.mChars[i].mHold > 0.0f) {
				if (menu->mTitle.mChars[i].mFade < 1.0f) {
					if ((menu->mTitle.mChars[i].mFade += (1.0f / (0.2f * gRefreshRate)) * frames) > 1.0f) {
						menu->mTitle.mChars[i].mFade = 1.0f;
					}
				}
			} else if (menu->mTitle.mChars[i].mFade > 0.0f) {
				if ((menu->mTitle.mChars[i].mFade -= (1.0f / (0.2f * gRefreshRate)) * frames) < 0.0f) {
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

extern "C" void fn_80061FF4(MenuStatus* menu, const char* text, u32 color)
{
	menu->mTitle.mActive = TRUE;
	menu->mTitle.mCount  = 0;
	if (text == NULL) {
		return;
	}
	menu->mTitle.mColor = color;
	for (s32 i = 0; i < 20 && *text; i++, text++) {
		menu->mTitle.mChars[i].mText   = text;
		menu->mTitle.mChars[i].mFade   = -0.2f * i;
		menu->mTitle.mChars[i].mHold   = gRefreshRate;
		menu->mTitle.mChars[i].mAngle0 = DEG2RAD * (rand() % 360);
		menu->mTitle.mChars[i].mAngle1 = DEG2RAD * (rand() % 360);
		menu->mTitle.mCount++;
		if (gGame.mLanguage == 0) {
			s32 lead = TRUE;
			u8 c     = *text;
			u8 a     = (c >= 0x81 && c <= 0x9F);
			if (a == FALSE) {
				u8 b = (c >= 0xE0 && c <= 0xFC);
				if (b == FALSE) {
					lead = FALSE;
				}
			}
			if (lead) {
				text++;
			}
		}
	}
	for (s32 i = menu->mTitle.mCount; i < 20; i++) {
		menu->mTitle.mChars[i].mText = NULL;
		menu->mTitle.mChars[i].mFade = 0.0f;
		menu->mTitle.mChars[i].mHold = 0.0f;
	}
}

void MenuStatus::vf08(CbPane* pane)
{
	s32 kind = pane->mKind;
	mHovered = TRUE;
	if (kind < 49 || kind > 50 || mMode != 2 || mPhase != 0 || CountMenuItems() <= 4) {
		return;
	}
	if (kind == 49 && sStatusItemTable.mPage > 0) {
		mTargetY = 128.0f;
		mPhase   = 20;
	} else if (kind == 50 && sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
		mTargetY = -56.0f;
		mPhase   = 10;
	}
}

void MenuStatus::vf14(CbPane* pane)
{
	if (!mHovered) {
		return;
	}
	s32 kind = pane->mKind;
	if (kind < 49 || kind > 50 || mMode != 2 || mPhase != 0 || CountMenuItems() <= 4) {
		return;
	}
	if (kind == 49 && sStatusItemTable.mPage > 0) {
		mTargetY = 128.0f;
		mPhase   = 20;
	} else if (kind == 50 && sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
		mTargetY = -56.0f;
		mPhase   = 10;
	}
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

static inline void ResetStatusSelection(MenuStatus* menu, s32& selection, s32& previous, const s32& stick)
{
	if (menu->IsPointerActive()) {
		selection = -1;
		previous  = -1;
	} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
		selection = -1;
		previous  = -1;
	} else {
		selection = stick;
		previous  = stick;
	}
}

extern "C" void fn_800695EC(MenuStatus* menu)
{
	if (menu->mMode == 1) {
		ResetStatusSelection(menu, menu->mSelection, menu->mPrevSelection, menu->mStickSelection);
	}
	if (menu->mMode == 2) {
		ResetStatusSelection(menu, menu->mItemSelection, menu->mPrevItemSelection, menu->mStickItemSelection);
	}
	if (menu->mMode == 9) {
		ResetStatusSelection(menu, menu->mOverviewSelection, menu->mPrevOverviewSelection, menu->mStickOverviewSelection);
	}
	if (menu->mMode == 8) {
		ResetStatusSelection(menu, menu->mIconSelection, menu->mPrevIconSelection, menu->mStickIconSelection);
	}
	ResetStatusSelection(menu, menu->mItemSelection, menu->mPrevItemSelection, menu->mStickItemSelection);
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

static inline void SelectPreviousShopRow(s32& selection)
{
	if (--selection < 0) {
		selection = 19;
	}
	PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
}

static inline void SelectNextShopRow(s32& selection, const MenuPCPage& page, bool allowEmptyRows)
{
	s32 next = selection + 1;
	if (next <= 19) {
		selection = next;
		if (allowEmptyRows || page.mRows[selection].mItem >= 0) {
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
	}
}

static inline s32 ResetPCShopPage(MenuPC* menu, MenuPCItem*& table)
{
	menu->mPhase = 0;
	return PCShopMaxPage(table);
}

void MenuPC::State3(u32 frames)
{
	CPad pad(0);
	MenuPCItem* table = sItemShopEntries;
	s32 count         = CountPCShopItems(table);
	ApproachHudValue(&mPanelAlpha, &mPanelTarget, 0.08f * frames);
	switch (mPhase) {
	case 0:
		if (count > 1) {
			if (mItemShopSelection == 0) {
				if (gWiiPointer.Check(PadActStickDown)) {
					SelectNextShopRow(mItemShopSelection, gItemShopPage, true);
				}
				if (gWiiPointer.Check(PadActStickUp)) {
					CountPCShopItems(table);
					if (gItemShopPage.mStart > 0) {
						mScrollTarget = 17.0f;
						mPhase        = 20;
						break;
					}
				}
			} else if (mItemShopSelection == 8) {
				if (gWiiPointer.Check(PadActStickUp)) {
					SelectPreviousShopRow(mItemShopSelection);
				}
				if (gWiiPointer.Check(PadActStickDown)) {
					MenuPCItem* row = table;
					if (gItemShopPage.mStart < PCShopMaxPage(row) - 1) {
						mScrollTarget = -17.0f;
						mPhase        = 10;
						break;
					}
				}
			} else {
				if (gWiiPointer.Check(PadActStickUp)) {
					SelectPreviousShopRow(mItemShopSelection);
				}
				if (gWiiPointer.Check(PadActStickDown)) {
					SelectNextShopRow(mItemShopSelection, gItemShopPage, true);
				}
			}
		}
		if (gWiiPointer.Check(PadActConfirm)) {
			if (gItemShopPage.mRows[mItemShopSelection].mItem >= 0 && gItemShopPage.mRows[mItemShopSelection].mCount > 0) {
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				mPanelTarget = 1.0f;
				mPhase++;
				mYesNo           = 0;
				mTargetListAlpha = 0.5f;
				break;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
			break;
		}
		if (gWiiPointer.Check(PadActCancel)) {
			if (mShopOnly) {
				ResetPCText(this, 11);
				mTargetFrameHeight = 0.0f;
				SavePCShopState(this);
				mPanelTarget = 0.0f;
			} else {
				ResetPCText(this, 2);
				PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
				mPanelTarget = 0.0f;
			}
		}
		break;
	case 1:
		if (gWiiPointer.Check(PadActStickUp)) {
			mYesNo--;
			if (mYesNo < 0) {
				mYesNo = 1;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickDown)) {
			mYesNo++;
			if (mYesNo > 1) {
				mYesNo = 0;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActConfirm)) {
			if (mYesNo == 0) {
				s32 entryIndex    = gItemShopPage.mIndex[mItemShopSelection];
				s32 money         = GetMoney();
				MenuPCItem* entry = &sItemShopEntries[entryIndex];
				if (entry->mPrice <= money) {
					money -= entry->mPrice;
					gGame.GetPlayer()->mParams.SetMoney(money);
					mMoneyDisplay.target = money;
					mYesNo               = 2;
					PlayWiiSoundEffect(&gGame.mSound, 0x35B, 1.0f, 0.0f);
					if (entry->mIsAttachment) {
						s32 id = entry->mItem;
						if (id >= 0 && id < 9) {
							s32 level = GetAttachmentCountById(&gGame.GetPlayer()->mAttachments, id) + 1;
							if (level >= 3) {
								level = 3;
							}
							SetAttachmentCountById(&gGame.GetPlayer()->mAttachments, id, level);
						}
					} else {
						s16 id = entry->mItem;
						if (id >= 0 && id < 0x9F) {
							s32 index    = FindOwnedInventoryItemIndex(gGame.GetPlayer()->mParams.mInventory.mItems, id);
							s32 quantity = index >= 0 ? gGame.GetPlayer()->mParams.mInventory.mItems[index].mCount : 0;
							SetOrAddInventoryItemCount(&gGame.GetPlayer()->mParams.mInventory, id, quantity + 1, FALSE);
						}
					}
					s16& stock = entry->mCount;
					s16 left   = stock;
					stock      = left > 0 ? left - 1 : left;
					switch (entry->mItem) {
					case 0x57:
					case 0x59:
					case 0x5A:
						for (int i = 0; i < 20; i++) {
							MenuPCItem& other = table[i];
							s16 otherId       = other.mItem;
							if (otherId >= 0 && !other.mIsAttachment) {
								switch (otherId) {
								case 0x57:
								case 0x59:
								case 0x5A:
									if (otherId != entry->mItem) {
										other.mCount = 0;
										SetOrAddInventoryItemCount(&gGame.GetPlayer()->mParams.mInventory, otherId, 0, FALSE);
									}
									break;
								case 0x58:
									break;
								}
							}
						}
						break;
					case 0x58:
						break;
					}
					mPhase++;
					return;
				}
				PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
			} else {
				mPhase       = 0;
				mPanelTarget = 0.0f;
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				mTargetListAlpha = 1.0f;
			}
		}
		if (gWiiPointer.Check(PadActCancel)) {
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			mPhase           = 0;
			mPanelTarget     = 0.0f;
			mTargetListAlpha = 1.0f;
		}
		break;
	case 2:
		if (gWiiPointer.Check(PadActConfirm) || gWiiPointer.Check(PadActCancel)) {
			mPhase       = 0;
			mPanelTarget = 0.0f;
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			mTargetListAlpha = 1.0f;
			PopulateItemShopPage(&gItemShopPage);
		}
		break;
	case 10:
		if (mScroll == mScrollTarget) {
			if (gItemShopPage.mStart < ResetPCShopPage(this, table) - 1) {
				gItemShopPage.mStart++;
				PopulateItemShopPage(&gItemShopPage);
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			mScroll       = 0.0f;
			mScrollTarget = 0.0f;
		}
		break;
	case 20:
		if (mScroll == mScrollTarget) {
			mPhase = 0;
			CountPCShopItems(table);
			if (gItemShopPage.mStart > 0) {
				gItemShopPage.mStart--;
				PopulateItemShopPage(&gItemShopPage);
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			mScroll       = 0.0f;
			mScrollTarget = 0.0f;
		}
		break;
	}
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

static inline s16 PCShopEntryId(s32 index)
{
	return gScrapShopPage.mRows[index].mItem;
}

void MenuPC::State7(u32 frames)
{
	CPad pad(0);
	MenuPCItem* table = sScrapShopEntries;
	s32 count         = CountPCShopItems(table);
	ApproachHudValue(&mPanelAlpha, &mPanelTarget, 0.08f * frames);
	switch (mPhase) {
	case 0:
		if (count > 1) {
			if (mScrapShopSelection == 0) {
				if (gWiiPointer.Check(PadActStickDown)) {
					SelectNextShopRow(mScrapShopSelection, gScrapShopPage, true);
				}
				if (gWiiPointer.Check(PadActStickUp)) {
					CountPCShopItems(table);
					if (gScrapShopPage.mStart > 0) {
						mScrollTarget = 17.0f;
						mPhase        = 20;
						break;
					}
				}
			} else if (mScrapShopSelection == 8) {
				if (gWiiPointer.Check(PadActStickUp)) {
					SelectPreviousShopRow(mScrapShopSelection);
				}
				if (gWiiPointer.Check(PadActStickDown)) {
					if (gScrapShopPage.mStart < PCShopMaxPage(table) - 1) {
						mScrollTarget = -17.0f;
						mPhase        = 10;
						break;
					}
				}
			} else {
				if (gWiiPointer.Check(PadActStickUp)) {
					SelectPreviousShopRow(mScrapShopSelection);
				}
				if (gWiiPointer.Check(PadActStickDown)) {
					SelectNextShopRow(mScrapShopSelection, gScrapShopPage, false);
				}
			}
		}
		if (PCShopEntryId(mScrapShopSelection) < 0) {
			do {
				mScrapShopSelection--;
				s16 id = gScrapShopPage.mRows[mScrapShopSelection].mItem;
				if (id >= 0) {
					break;
				}
			} while (mScrapShopSelection != 0);
		}
		if (gWiiPointer.Check(PadActConfirm)) {
			if (PCShopEntryId(mScrapShopSelection) >= 0 && gScrapShopPage.mRows[mScrapShopSelection].mCount > 0) {
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				mPanelTarget = 1.0f;
				mPhase++;
				mYesNo = 0;
				break;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
			break;
		}
		if (gWiiPointer.Check(PadActCancel)) {
			ResetPCText(this, 2);
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			mPanelTarget = 0.0f;
		}
		break;
	case 1:
		if (gWiiPointer.Check(PadActStickUp)) {
			mYesNo--;
			if (mYesNo < 0) {
				mYesNo = 1;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActStickDown)) {
			mYesNo++;
			if (mYesNo > 1) {
				mYesNo = 0;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		} else if (gWiiPointer.Check(PadActConfirm)) {
			if (mYesNo == 0) {
				s32 item          = gScrapShopPage.mIndex[mScrapShopSelection];
				s32 scrap         = GetScrap();
				MenuPCItem* entry = &sScrapShopEntries[item];
				s16 price         = entry->mPrice;
				if (price <= scrap) {
					scrap -= price;
					gGame.GetPlayer()->mParams.SetScrap(scrap);
					mScrapDisplay.target = scrap;
					mYesNo               = 2;
					PlayWiiSoundEffect(&gGame.mSound, 0x35B, 1.0f, 0.0f);
					gEventVars.mVars[mScrapShopBase + entry->mItem] = 1;
					mExitState                                      = item;
					s16& quantity                                   = entry->mCount;
					s16 stock                                       = quantity;
					s32 remaining                                   = stock;
					if (stock > 0) {
						remaining = stock - 1;
					}
					quantity = remaining;
					mPhase++;
					return;
				}
				PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
			} else {
				mPhase       = 0;
				mPanelTarget = 0.0f;
				PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
				mTargetListAlpha = 1.0f;
			}
		} else if (gWiiPointer.Check(PadActCancel)) {
			PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
			mPhase       = 0;
			mPanelTarget = 0.0f;
		}
		break;
	case 2:
		if (gWiiPointer.Check(PadActConfirm) || gWiiPointer.Check(PadActCancel)) {
			mPhase       = 0;
			mPanelTarget = 0.0f;
			PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
			PopulateScrapShopPage(&gScrapShopPage);
			ResetPCText(this, 11);
			mTargetFrameHeight = 0.0f;
			SavePCShopState(this);
		}
		break;
	case 10:
		if (mScroll == mScrollTarget) {
			if (gScrapShopPage.mStart < ResetPCShopPage(this, table) - 1) {
				gScrapShopPage.mStart++;
				PopulateScrapShopPage(&gScrapShopPage);
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			mScroll       = 0.0f;
			mScrollTarget = 0.0f;
		}
		break;
	case 20:
		if (mScroll == mScrollTarget) {
			mPhase = 0;
			CountPCShopItems(table);
			if (gScrapShopPage.mStart > 0) {
				gScrapShopPage.mStart--;
				PopulateScrapShopPage(&gScrapShopPage);
				PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
			}
			mScroll       = 0.0f;
			mScrollTarget = 0.0f;
		}
		break;
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
// title.cpp has it.
static void SetMenuTitle(MenuSave* menu, const char* text, u32 color)
{
	menu->mTitle.mActive = TRUE;
	menu->mTitle.mCount  = 0;
	if (text) {
		menu->mTitle.mColor = color;
		int i;
		for (i = 0; i < 20 && *text; i++, text++) {
			menu->mTitle.mChars[i].mText   = text;
			menu->mTitle.mChars[i].mFade   = -0.2f * i;
			menu->mTitle.mChars[i].mHold   = gRefreshRate;
			menu->mTitle.mChars[i].mAngle0 = 0.017453292f * (rand() % 360);
			menu->mTitle.mChars[i].mAngle1 = 0.017453292f * (rand() % 360);
			menu->mTitle.mCount++;
			if (gGame.mLanguage == 0) {
				s32 lead = TRUE;
				u8 c     = *text;
				u8 a     = (c >= 0x81 && c <= 0x9F);
				if (a == FALSE) {
					u8 b = (c >= 0xE0 && c <= 0xFC);
					if (b == FALSE) {
						lead = FALSE;
					}
				}
				if (lead) {
					text++;
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
void ClearItemShopPage(MenuPCPage* page)
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
static inline void ClearItemShopRows(MenuPCPage* list)
{
	for (int i = 0; i < 20; i++) {
		list->mRows[i].mItem         = -1;
		list->mRows[i].mCount        = -1;
		list->mRows[i].mPrice        = -1;
		list->mRows[i].mIsAttachment = 0;
		list->mIndex[i]              = -1;
	}
}

extern "C" void PopulateItemShopPage(MenuPCPage* list)
{
	s32 count;
	s32 n;
	s16 price;
	ClearItemShopRows(list);
	count = 0;
	n     = 0;
	for (int i = 0; i < 20; i++) {
		const MenuPCItem* entry = sItemShopEntries + i;
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
		const MenuPCItem* entry = sItemShopEntries + i;
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
		const MenuPCItem* entry = sItemShopEntries + i;
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
void ClearScrapShopPage(MenuPCPage* page)
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
extern "C" void PopulateScrapShopPage(MenuPCPage* list)
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
		const MenuPCItem* entry = sScrapShopEntries + i;
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
		const MenuPCItem* entry = sScrapShopEntries + i;
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
		const MenuPCItem* entry = sScrapShopEntries + i;
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
extern "C" BOOL LoadMenuGroupResources(MenuEx* ex)
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
extern "C" void ResetMenuGroup(MenuEx* ex)
{
	ex->mCursor.mFrame = 0;
	ex->mCursor.mTimer = 0;
	for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
		if (ex->mMenus[i]) {
			ex->mMenus[i]->Reset();
		}
	}
}

// UpdateOpenMenus: steps the open menus, lets the game go on once the last one closes, blinks the cursor and
// tests the pointer against the menus' panes.
// NONMATCHING: 80 %, the frame is retail's; the screen size's int to float conversions are scheduled apart
// in retail and the rectangle's registers differ
extern "C" void UpdateOpenMenus(MenuEx* ex, u32 frames)
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

extern "C" void fn_80084C84(MenuEx* ex)
{
	if (!ex->mActive) {
		return;
	}
	WiiFbTexture* texture = (WiiFbTexture*)&ex->_34;
	if (texture->mImage == NULL) {
		CreateFbTexture(texture, gGame.mGs.mScreen.fbWidth, gGame.mGs.mScreen.efbHeight, GX_TF_RGB565, FALSE, NULL, "(menu)");
		CopyFbTexture(texture, texture->mFormat, FALSE, TRUE);
		PauseGameSounds(&gGame, 0.2f);
	}
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	gGame.mGs.mCurTevStage->SetOrder(GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	gGame.mGs.mCurTevStage->SetColorIn(GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
	gGame.mGs.mCurTevStage->SetColorOp(GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	gGame.mGs.mCurTevStage->SetAlphaIn(GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
	gGame.mGs.mCurTevStage->SetAlphaOp(GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
	DrawFbTexture(texture, 0xFFFFFFFF, gPointerScreenWidth / 2, gPointerScreenHeight / 2, 0.0f, gPointerScreenWidth, gPointerScreenHeight,
	              0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
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

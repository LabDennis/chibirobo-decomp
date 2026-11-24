// The title screen. One file, .text 0x80044FD8-0x8004F8F8:
// - "title.cpp" opens its string pool (0x802C64D4); the pool's last string ends at 0x802C65F6 and the menu
//   file's first string object follows at 0x802C65F8
// - its .sdata2 literals, 0x805A2DE8-0x805A2F70, are read by the functions from InitTitleScreen (0x80044FD8)
//   to its static initializer (0x8004F27C, the fourth .ctors entry) and by nothing else. The logo code
//   before it reads 0x805A2DA0-0x805A2DE8, the menu file behind it 0x805A2F70 and up
// - behind the static initializer come only what it and the vtables need: the array destructor it
//   registers (0x8004F894), a weak destructor (0x8004F8B0) and a this-adjusting thunk (0x8004F8F0). The menu
//   file's first function (0x8004F8F8) reads the next literal pool
// - extab 0x80009364-0x800095CC and extabindex 0x800124D0-0x80012650 are its 32 entries, in one run
// - .rodata 0x802C6468-0x802C65F8: three constant tables, the RTTI name "XObjDemo", then the string pool
// The functions come in the reverse of the GameCube's order (titlestates.cpp, once part of gxb.cpp, from XObjDemo::DrawObject to
// InitTitleScreen).
//
// NONMATCHING: twenty-one functions have source so far: UpdateTitleOptions (0x8004C548), UpdateTitleDust
// (0x8004CA74), DrawTitleHelpText (0x8004D5C4), Title::UpdateSelections (0x8004E534) and the small
// states at the end of this file. UpdateSelections and the eight empty states are exact. The nine
// states that are only an OSPanic tail call (State6, State11-13, State18-22) differ only in the
// message's offset in the string pool, which the rest of the file's strings will move into place.
// In UpdateTitleOptions this build expands UpdateSelections where retail calls it (0x8004CA20),
// which leaves it at 70 %. Compiled outside the build with -inline noauto added, every instruction is
// retail's (99.6 %): what then differs is the name of the callee at 0x80231770, which is fmod and is in
// symbols.txt as exp, and the offset of the halt message in the string pool, which the rest of the file's
// strings will move into place. The flag is not set for the unit: too few neighbours to test it on.
// UpdateTitleDust is one bool temp and one branch polarity away from exact (99.7 %): retail keeps a
// bool temp for each byte range of the SJIS check and inits the flag in another register. In
// DrawTitleHelpText (82 %) the calls, the quad arguments and the float formulas are retail's, but the
// glyph loop's float scheduling and register pressure differ: retail saves r15 and folds the padding
// subtraction and the outline negations into fmsubs/fneg where this build moves and reloads.
#include "nw4r/math/math_types.h"
#include "game/wii/title.h"
#include "game/titlepalette.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/cfont.h"
#include "game/coordinfo.h"
#include "game/cmdline.h"
#include "game/gmath.h"
#include "game/menupanelbackground.h"
#include "game/stagedata.h"
#include "game/cscrn.h"
#include "game/wii/saveslots.h"
#include "game/wii/infoscrn.h"
#include "Dolphin/os.h"
#include "stl/math.h"
#include <stdio.h>
#include <stdlib.h>

extern "C" {
extern s32 gPointerScreenWidth, gPointerScreenHeight, gRingScreenWidth, gRingScreenHeight, gScreenSafeBottom, gRefreshRate;
const char* GetTitleAndSaveMenuText(s32 index);
const char* GetLocalizedInterfaceText(s32 index);
void DrawTitleHelpText(Title* title);
void DrawTitleLoadScreen(Title* title);
void DrawTitleOptions(Title* title);
// infoscrn.cpp's window frame (0x8008F884)
void DrawTexturedWindowFrame(u32 color, s32 a, f32 x, f32 y, f32 w, f32 h, f32 sx, f32 sy, f32 sz);
// the options page's highlight bar behind a line (0x8004EDFC)
void fn_8004EDFC(Title* title, BOOL selected, f32 x, f32 y, f32 size, f32 width);
// the menus' highlight knob, line and arrow (0x8004E6EC)
void fn_8004E6EC(Title* title, BOOL options, f32 x, f32 y, f32 alpha, f32 extent);
void DrawTitleNameBubbles(Title* title);
void fn_802AEDEC(WiiGsSceneObj** head, Mtx view);
CCamera* InitGameOrthoCamera(CCamera* camera, GXRenderModeObj* mode);
void OSFatal(GXColor fg, GXColor bg, const char* msg);
void ChangeGameStage(CGame* game, s32 stage, BOOL showName);
void ApproachHudValue(f32* value, f32* target, f32 step);
s32 IndexOfLargestOfThree(OSTime first, OSTime second, OSTime third);
// one dust mote's per-frame update, the body of the GameCube UpdateTitleDust's loop
void fn_8004DC94(TitleDust* dust, u32 frames);
// the name's per-frame update
void fn_8004CDB8(Title* title, u32 frames);
// sets the name that pops in over the menu (0x8004CA74; the symbol's name is older than the reading)
void UpdateTitleDust(Title* title, const char* text, u32 color);
bool IsSaveSlotIntact(const WiiMenuSaveSlot* slot);
// fills a file menu record from a scanned save header (menu.cpp, 0x8007A2E8)
void fn_8007A2E8(WiiMenuSaveSlot* slot, WiiSaveScanEntry* entry, BOOL used);
void fn_800556B0(CbPane* pane, const CbPaneLayout* layout);
void ResetTitleScreenState(Title* title);
void DrawSaveSlotDetails(WiiMenuSaveSlot* slot, u8 emptyMessage, s16 index, u8 selected, u8 newest, f32 x, f32 y, f32 width, f32 height,
                         f32 alpha, f32 fileAlpha, f32 sx, f32 sy);
s32 DrawJapaneseSaveStatusMessage(s32 channel, s32 id, f32 x, f32 y, f32 size, f32 alpha);
s32 DrawJapaneseCardErrorMessage(s32 channel, s32 result, u8 retry, f32 x, f32 y, f32 size, f32 alpha);
void DrawMenuBannerPanels(const char* text, f32 x, f32 y, f32 alpha, f32 slide);
}

// the title screen's background and text colors, the logo's textures and which of each is in use
const TitlePaletteEntry gTitlePalette[] = {
	{ { 0, 0, 128, 0 }, {} },     { { 128, 0, 0, 0 }, {} },     { { 0, 128, 0, 0 }, {} },
	{ { 200, 200, 200, 0 }, {} }, { { 128, 128, 128, 0 }, {} }, { { 0, 207, 233, 0 }, {} },
	{ { 255, 220, 133, 0 }, {} }, { { 255, 220, 0, 0 }, {} },   { {}, { 255, 255, 255, 0 } },
};
const s32 gTitleLogoTextures[3] = { 0, 0, 0 };
s32 gTitlePaletteIndex          = 3;
s32 gTitleLogoIndex             = 1;
// the gap between the highlight bar's end and the cursor arrow behind it
f32 gTitleArrowGap[2] = { 30.0f, 0.0f };
// how fast the file rows slide out to the file picked and back
f32 gTitleFileSlideSpeed[2] = { 1.0f, 1.0f };

CbPaneLayout lbl_802E7F48[11] = {
	{ { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 0 },  { { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 1 },
	{ { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 2 },  { { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 3 },
	{ { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 4 },  { { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 5 },
	{ { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 6 },  { { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 7 },
	{ { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 8 },  { { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 9 },
	{ { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 10 },
};

// the outline offsets of the name's letter shadows, then of the help text's
f32 gTitleNameOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gTitleNameOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
f32 gTitleHelpOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gTitleHelpOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };

// the glyph width is rounded before subtracting it from the cell width (the GameCube's
// HelpGlyphPadding from drawhelptext.h, which cannot be included here; on the Wii the width
// division stays single precision)
static inline f32 HelpGlyphPadding(s32 width, f32 scale)
{
	f32 glyphWidth = (f32)(width / 24.0f) * scale;
	return scale - glyphWidth;
}

// the letters of the name shrink away
static inline void HideName(Title* title)
{
	for (u32 i = 0; i < 20; i++) {
		title->mName.mChars[i].mWait = 0.0f;
	}
}

// the save file went away under the menu, which the NAND never does: back to the error screen
// with the file records cleared, the GameCube's OnCardRemoved
static inline void OnCardRemoved(Title* title)
{
	OSPanic("title.cpp", 0x1484, "ここにくるのはおかしい");
	title->mResult = -3;
	title->SetMode(9);
	title->mOffset = -32.0f;
	for (s32 i = 0; i < 3; i++) {
		ResetSaveSlotRecord(&gTitleSaveSlots[i]);
	}
}

// as gamemain.cpp and gamedraw.cpp define it (see csound.h)
inline void CSound::StopAllTracks(f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (int i = 0; i < 0x40; ++i) {
		if (mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()) {
			mArchivePlayer.mTracks.mHandles[i].Stop(frames);
		}
	}
}

// the pixel engine state every 2D draw on the title screen needs, the GameCube's BeginTitleOverlay
static inline void BeginTitleOverlay()
{
	SetGsChannelEnabled(&gGame.mGs, 4, 0);
	gGame.mGs.mCurRenderState->SetColorUpdate(TRUE);
	gGame.mGs.mCurRenderState->SetAlphaUpdate(FALSE);
	gGame.mGs.mCurRenderState->SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 255);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_COPY);
	gGame.mGs.mCurRenderState->SetZMode(FALSE, GX_ALWAYS, FALSE);
	if (gGame.mGs.mCurRenderState != NULL) {
		gGame.mGs.mCurRenderState->Apply();
	}
	if (gGame.mGs.mCurTevStage != NULL) {
		gGame.mGs.mCurTevStage->Apply();
	}
	if (gGame.mGs.mCurChannel != NULL) {
		gGame.mGs.mCurChannel->Apply();
	}
}

// one menu entry line: it pulses while the cursor's animation runs, but only once the highlight
// delay has counted down
static inline f32 DrawTitleEntry(Title* title, s32 entry, s32 textId, f32 x, f32 y, f32 progress, u32 normal, u32 lit)
{
	const char* text = GetTitleAndSaveMenuText(textId);
	f32 size         = 24.0f + (title->mSelection == entry ? 2.0f * (f32)sin(4.712389f * progress) : 0.0f);
	f32 width        = MeasureGameTextWidth(&gGame.mGs, text, TRUE, 24.0f);
	bool selected    = false;
	if (title->mSelection == entry && (f32)title->mHighlightDelay >= (f32)gRefreshRate) {
		selected = true;
	}
	if (entry != 1 || title->mSaveFound) {
		DrawGsProportionalTextFormat(&gGame.mGs, selected ? lit : normal, text, x - width * 0.5f, (size - 24.0f) * 0.5f + (y - size), 0.0f,
		                             24.0f, size);
	}
	return width;
}

// the colour the cursor's animation queue fades to, packed as the knob reads it
static inline u32 GetTitleCursorColor(Title* title)
{
	return ((u32)title->mCursor[0].mAnmQueue.mColorStep[0].mValue.x << 24) | ((u32)title->mCursor[0].mAnmQueue.mColorStep[0].mValue.y << 16)
	     | ((u32)title->mCursor[0].mAnmQueue.mColorStep[0].mValue.z << 8);
}

// the shared ease for the panel offset pair, the GameCube's ApproachTitleOffset; the Wii takes the absolute value with fabsf, which its MSL
// inlines to __fabsf
static inline void ApproachTitleOffset(f32& value, f32& target)
{
	f32 speed = __fabsf((target - value) / 4);
	if (speed <= 1)
		speed = 1;
	ApproachHudValue(&value, &target, speed);
}

// the dust's per-frame drift, retail calls the helper once per mote
static inline void UpdateTitleDust(Title* title, u32 frames)
{
	for (s32 i = 0; i < 16; ++i) {
		fn_8004DC94(&title->mDust[i], frames);
	}
}

extern void (Title::*gTitleModeUpdates[])(u32);

static inline bool FadeDone()
{
	return 0.0f == gGame.mFade.mSpeed.r && 0.0f == gGame.mFade.mSpeed.g && 0.0f == gGame.mFade.mSpeed.b && 0.0f == gGame.mFade.mSpeed.a;
}

extern "C" void InitTitleScreen(Title* title)
{
	title->mCursor[0].InitializeObject();
	title->mCursor[1].InitializeObject();
	ResetTitleScreenState(title);
	for (s32 i = 0; i < 3; ++i) {
		title->mDemo[i].InitializeObject();
	}
	for (s32 i = 0; i < 16; ++i) {
		title->mDust[i].mState  = 0;
		title->mDust[i].mTimer  = 0;
		title->mDust[i].mX      = rand() % gRingScreenWidth;
		title->mDust[i].mY      = rand() % gRingScreenHeight;
		title->mDust[i].mSpeedX = 0.0f;
		title->mDust[i].mSpeedY = 0.0f;
	}
	title->mEntryLines[0]     = lbl_802E7F48[5].mTrans.y;
	title->mEntryLines[1]     = lbl_802E7F48[6].mTrans.y;
	title->mEntryLines[2]     = lbl_802E7F48[7].mTrans.y;
	title->mOptionBarLines[0] = lbl_802E7F48[8].mTrans.y;
	title->mOptionBarLines[1] = lbl_802E7F48[9].mTrans.y;
	title->mOptionBarLines[2] = lbl_802E7F48[10].mTrans.y;
	gRingScreenWidth          = 640;
	gPointerScreenWidth       = 640;
	CbPaneLayout* layout      = lbl_802E7F48;
	s32 i                     = 11;
	do {
		Vec trans        = layout->mTrans;
		layout->mTrans.x = trans.x - 0.5f * (f32)gPointerScreenWidth;
		layout->mTrans.y = 0.5f * (f32)gPointerScreenHeight - trans.y;
		++layout;
	} while (--i > 0);
	for (s32 i = 0; i < 11; ++i) {
		fn_800556B0(&title->mPanes[i], &lbl_802E7F48[i]);
	}
	s32 width             = gGame.mAspectRatio == 0 ? 640 : 854;
	gRingScreenWidth      = width;
	gPointerScreenWidth   = width;
	title->mSaveError     = 0;
	title->mSaveErrorCode = 0;
}

/**
 * @note Address: 0x800452B8
 * @note Size: 0x264
 */
// everything the title screen tracks back to its start; the sound tracks fade out over a second
extern "C" void ResetTitleScreenState(Title* title)
{
	title->mState          = 0;
	title->mMode           = 0;
	title->mTimeout        = -1;
	title->mMenuAlpha      = 0.0f;
	title->mTitleAlpha     = 0.0f;
	title->mPulseAngle     = 90.0f;
	title->mHighlightDelay = 0;
	title->mSpawnIndex     = 0;
	title->mCursor[0].ResetObjectState();
	title->mCursor[0].mAngle      = 0.0f;
	title->mCursor[0].mAngleSpeed = 0.0f;
	title->mCursor[1].ResetObjectState();
	title->mCursor[1].mAngle      = 0.0f;
	title->mCursor[1].mAngleSpeed = 0.0f;
	for (s32 i = 0; i < 3; ++i) {
		title->mDemo[i].ResetObjectState();
	}
	title->mSelection         = 0;
	title->mFile              = 0;
	title->mChoice            = 0;
	title->mLastSelection     = -1;
	title->mLastFile          = -1;
	title->mLastChoice        = -1;
	title->mLoadAlpha         = 0.0f;
	title->mAlpha             = 0.0f;
	title->mPrevMode          = 0;
	title->mPhase             = 0;
	title->mDelay             = 0;
	title->mPanelX            = -10.0f;
	title->mPanelY            = -200.0f;
	title->mTargetPanelX      = -10.0f;
	title->mTargetPanelY      = -200.0f;
	title->_4944              = 0.0f;
	title->mLoadOffset        = 0.0f;
	title->_494C              = 0.0f;
	title->mOffset            = 0.0f;
	title->mOptionLine        = 0;
	title->mLastOptionLine    = -1;
	title->mOptionState       = 0;
	title->mOptionAlpha       = 0.0f;
	title->mTargetOptionAlpha = 0.0f;
	if (gGame.mMenuControls & MenuControlPointer) {
		title->mPointerOn = 1;
	} else {
		title->mPointerOn = 0;
	}
	if (gGame.mMenuControls & MenuControlStick) {
		title->mStickOn = 1;
	} else {
		title->mStickOn = 0;
	}
	title->mChannel = 0;
	for (s32 i = 0; i < 3; ++i) {
		ResetSaveSlotRecord(&gTitleSaveSlots[i]);
	}
	title->mLoadScroll    = 0.0f;
	title->mSaveFound     = 0;
	title->mCardScanState = 0;
	gGame.mSound.StopAllTracks(1.0f);
	title->mShowMessage   = 0;
	title->mOptionBarSnap = 0;
	title->mSaveError     = 0;
	title->mSaveErrorCode = 0;
	title->mNewestFile    = -1;
}

/**
 * @note Address: 0x8004551C
 * @note Size: 0x1D8C
 */
// NONMATCHING: the frame skeleton, the save scan and states 1-9, 13, 14 and 22 are as retail read
// them. Inside state 9 the newest slot's saved pointer/stick bits are restored (the block after
// IndexOfLargestOfThree is only partly read) and inside 11-12 and 20-21 the pointer/confirm input
// and the mode dispatch follow the GameCube flow; the Wii input calls are the MenuPad methods the
// binary shares with UpdateTitleOptions.
extern "C" void UpdateTitleScreen(Title* title, u32 frames)
{
	gPointerPanes.mCount = 0;
	ApproachTitleOffset(title->mPanelY, title->mTargetPanelY);
	ApproachHudValue(&title->mLoadAlpha, &title->mAlpha, .08f * frames);
	ApproachTitleOffset(title->mLoadOffset, title->mOffset);
	ApproachHudValue(&title->mOptionAlpha, &title->mTargetOptionAlpha, .08f * frames);
	title->mLoadScroll -= .008f * frames;
	if (title->mState != 0 && title->mState != 1 && title->mState != 23) {
		fn_8004CDB8(title, frames);
		switch (title->mHelpFadeState) {
		case 0:
			break;
		case 1:
			title->mHelpAlpha = 0;
			++title->mHelpFadeState;
			title->mHelpText = title->mNextHelpText;
			break;
		case 2:
			title->mHelpAlpha += .04f * frames;
			if (title->mHelpAlpha >= 1) {
				title->mHelpAlpha     = 1;
				title->mHelpFadeState = 0;
			}
			break;
		}
		if (title->mMenuAlpha > 0) {
			for (s32 i = 0; i < (s32)ARRAY_SIZE(title->mBg); ++i) {
				title->mBg[i].mAngle0 += (5.f * DEG2RAD) * frames;
				title->mBg[i].mAngle1 += (3.f * DEG2RAD) * frames;
			}
		}
		if (title->mMode != 1 && title->mMode != 8 && title->mMode != 12 && title->mMode != 15) {
			title->mReady       = 0;
			title->mWrongDevice = 0;
			// the GameCube probed the card here; the NAND is always there
			title->mReady = 1;
			if (!title->mReady)
				title->mShowMessage = 0;
			if (!title->mReady) {
				OSPanic("title.cpp", 0x1a9, "ここにくるのはまずい");
				if (title->mState != 20 && title->mSelection == 1)
					title->mSelection = 0;
				title->mSaveFound     = 0;
				title->mCardScanState = 0;
			}
		}
		bool running = title->mState >= 1 && title->mState < 23;
		if (!running)
			return;
		if (gGame.mSaveManager.mJobStarted)
			return;
	}
	// 1 starts the slot scan, 2 reads its result, 3-7 load the cursor, the demo and the
	// figures, 8 reads the TPL, 9 finishes the scan, 10 fades the menu in, 11 is the menu,
	// 12 waits for the cursor's anim, 20 runs the file modes, 21 the options, 13-14 fade
	// out, 22 frees it all and changes the stage, 23 is done
	switch (title->mState) {
	case 1:
		title->mState = 2;
		StartWiiSaveSlotScan(&gGame.mSaveManager, 3);
		gPointerPanes.mMenu    = title;
		gPointerPanes.mEnabled = 1;
		gPointerPanes.mActive  = 1;
		// NONMATCHING: retail keeps this loop as a counted loop, this build unrolls it
		for (s32 i = 0; i < 11; ++i) {
			title->mPanes[i].mPointed = 0;
			title->mPanes[i].mPicked  = 0;
		}
		StartScreenFade(&gGame.mFade, 0, 0, 0, 255, 0, 0, 0, 255, 0.0f);
		break;
	case 2: {
		bool found = false;
		for (s32 i = 0; i < 3; ++i) {
			if (gGame.mSaveManager.mScanEntries[i].mStatus & 1) {
				found = true;
			}
		}
		if (found) {
			title->mSelection = 1;
			title->mSaveFound = 1;
		} else {
			title->mSelection     = 0;
			title->mSaveFound     = 0;
			title->mCardScanState = 0;
		}
		title->mLastSelection = title->mSelection;
		title->mSelection     = -1;
		title->mOptionBar[0]  = title->mEntryLines[title->mLastSelection];
		title->mOptionBarSnap = 1;
		title->mState         = 3;
		if (gCmdLineArgs.mTitleCursor < 0) {
			title->mSpawnIndex = rand() % 11;
		} else {
			title->mSpawnIndex = gCmdLineArgs.mTitleCursor % 11;
		}
		// fall through: loading starts on the same frame as cursor selection
	}
	case 3:
		if (title->mCursor[0].LoadFromPlacement(CoordInfo(GetSpawnData(title->mSpawnIndex)))) {
			if ((title->mCursor[0].mRequest.mFile != NULL ? title->mCursor[0].mRequest.mFile->mSize : 0) == 0x3CA80) {
				title->mCursor[0].SetScale(4.0f);
			}
			++title->mState;
		}
		break;
	case 4:
		if (title->mCursor[1].LoadFromPlacement(CoordInfo(GetSpawnData(title->mSpawnIndex)))) {
			if ((title->mCursor[1].mRequest.mFile != NULL ? title->mCursor[1].mRequest.mFile->mSize : 0) == 0x3CA80) {
				title->mCursor[1].SetScale(4.0f);
			}
			++title->mState;
		}
		break;
	case 5:
		if (title->mDemo[0].LoadFromPlacement(CoordInfo(GetSpawnData(0x31)))) {
			title->mDemo[0].SetScale(6.0f);
			title->mDemo[0].SetRotationY(HALF_PI);
			++title->mState;
		}
		break;
	case 6:
		if (title->mDemo[1].LoadFromPlacement(CoordInfo(GetSpawnData(0x32)))) {
			title->mDemo[1].SetScale(6.0f);
			title->mDemo[1].SetRotationY(HALF_PI);
			++title->mState;
		}
		break;
	case 7:
		if (title->mDemo[2].LoadFromPlacement(CoordInfo(GetSpawnData(0x33)))) {
			title->mDemo[2].SetScale(6.0f);
			title->mDemo[2].SetRotationY(HALF_PI);
			++title->mState;
		}
		break;
	case 8: {
		char path[64];
		const char* suffix = ""; // the GameCube's language suffix, none on the Wii
		sprintf(path, "tpl/title%s.tpl", suffix);
		if (!title->mTpl.ReadEx(path, 0, 0)) {
			break;
		}
		StartWiiSaveSlotScan(&gGame.mSaveManager, 3);
		title->mState = 9;
		break;
	}
	case 9: {
		if (gGame.mSaveManager.mJobStarted != 0) {
			break;
		}
		title->mState = 10;
		s32 result    = gGame.mSaveManager.mFile.mResult;
		if (result < 0 && (result == -9 || result == -11)) {
			title->mSaveErrorCode = result;
			title->mSaveError     = 1;
		}
		OSTime stamps[3];
		for (s32 i = 0; i < 3; ++i) {
			WiiSaveScanEntry* entry = &gGame.mSaveManager.mScanEntries[i];
			if (entry->mStatus & 1) {
				stamps[i] = ((WiiSaveSlotHeader*)entry->mData)->mTimestamp;
				if (entry->mStatus & 2) {
					stamps[i] = 0;
				}
			} else {
				stamps[i] = 0;
			}
		}
		// the newest save's controls become the menu's, once per boot
		WiiSaveSlotHeader* newest
		    = (WiiSaveSlotHeader*)gGame.mSaveManager.mScanEntries[IndexOfLargestOfThree(stamps[0], stamps[1], stamps[2])].mData;
		if (gGame.GetStatusFlags(0x4000)) {
			BOOL pointer = newest->mMenuControls != 0 ? newest->mMenuControls & MenuControlPointer : TRUE;
			if (pointer) {
				title->mPointerOn = 1;
				gGame.mMenuControls |= MenuControlPointer;
			} else {
				title->mPointerOn = 0;
				gGame.mMenuControls &= ~MenuControlPointer;
			}
			BOOL stick = newest->mMenuControls != 0 ? newest->mMenuControls & MenuControlStick : FALSE;
			if (stick) {
				title->mStickOn = 1;
				gGame.mMenuControls |= MenuControlStick;
			} else {
				title->mStickOn = 0;
				gGame.mMenuControls &= ~MenuControlStick;
			}
			gGame.ClearStatusFlags(0x4000);
		}
		if (!gGame.IsSystemMessage3Shown()) {
			PlayWiiSoundEffect(&gGame.mSound, 0xFF0003, 1.0f, 0.0f);
		}
		title->mState   = 10;
		title->mTimeout = -1;
		title->mTpl.CloseHandle();
		title->mTpl.mTex.Init(&title->mTpl.mPalette->descriptorArray[0], 0);
		title->mTex[0] = title->mTpl.mTex;
		title->mTpl.mTex.Init(&title->mTpl.mPalette->descriptorArray[1], 0);
		title->mTex[1] = title->mTpl.mTex;
		title->mTpl.mTex.Init(&title->mTpl.mPalette->descriptorArray[2], 0);
		title->mTex[2]     = title->mTpl.mTex;
		title->mMenuAlpha  = 0.0f;
		title->mTitleAlpha = 0.0f;
		StartScreenFade(&gGame.mFade, 0, 0, 0, 255, gTitlePalette[gTitlePaletteIndex].background.r,
		                gTitlePalette[gTitlePaletteIndex].background.g, gTitlePalette[gTitlePaletteIndex].background.b, 0, 0.5f);
		for (s32 i = 0; i < 3; ++i) {
			Vec pos                = { 0.0f, 0.0f, 100.0f };
			pos.x                  = gPointerScreenWidth / 2;
			pos.y                  = gPointerScreenHeight / 2 + 30;
			title->mDemo[i].mState = 1;
			title->mDemo[i].mLoop  = TRUE;
			title->mDemo[i].SetPos(pos);
			title->mDemo[i].SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		// the cursors are dimmed to a third
		CColor dim;
		dim.Set(255, 255, 255, 255);
		dim.Set((s32)(dim.r * 0.3f), (s32)(dim.g * 0.3f), (s32)(dim.b * 0.3f), (s32)(dim.a * 0.3f));
		f32 rate = 0.0f;
		title->mCursor[1].SetAnimRate(rate);
		title->mCursor[1].SetColor(0, dim.r, dim.g, dim.b, 0.0f);
		title->mCursor[0].SetAnimRate(rate);
		title->mCursor[0].SetColor(0, dim.r, dim.g, dim.b, 0.0f);
		break;
	}
	case 10: {
		if (title->mMenuAlpha < 1) {
			if ((title->mMenuAlpha += (1.f / (.5f * gRefreshRate)) * frames) > 1)
				title->mMenuAlpha = 1;
		}
		if (title->mTitleAlpha < 1) {
			if ((title->mTitleAlpha += (1.f / gRefreshRate) * frames) > 1)
				title->mTitleAlpha = 1;
		}
		title->mCursor[0].UpdateObject(frames);
		title->mCursor[1].UpdateObject(frames);
		for (s32 i = 0; i < 3; ++i) {
			title->mDemo[i].UpdateObject(frames);
		}
		UpdateTitleDust(title, frames);
		if (1.0f == title->mMenuAlpha && 1.0f == title->mTitleAlpha) {
			title->mState = 11;
			title->UpdateSelections();
		}
		break;
	}
	case 11: {
		bool addPanes = true;
		bool stick    = false;
		s32 anim;
		ResourceFile* file;
		if (title->mSelection != -1) {
			title->mLastSelection = title->mSelection;
		}
		// with nothing picked the highlight bar eases back to the line picked last
		if (title->mSelection == -1) {
			title->mOptionBar[0] += 0.8f * (title->mEntryLines[title->mLastSelection] - title->mOptionBar[0]);
			title->mOptionBar[1] += 0.8f * (title->mOptionBarTarget[0] - title->mOptionBar[1]);
			title->mOptionBar[2] += 0.8f * (title->mOptionBarTarget[1] - title->mOptionBar[2]);
		}
		if (title->IsStickUsedOverPointer()) {
			stick = true;
		}
		title->mHighlightDelay += frames;
		if ((f32)title->mHighlightDelay >= (f32)gRefreshRate) {
			title->mHighlightDelay = (f32)gRefreshRate;
			title->mPulseAngle     = (f32)fmod(title->mPulseAngle + (f32)(frames * 9), 360.);
		}
		title->mCursor[0].UpdateObject(frames);
		title->mCursor[1].UpdateObject(frames);
		for (s32 i = 0; i < 3; ++i) {
			title->mDemo[i].UpdateObject(frames);
		}
		UpdateTitleDust(title, frames);
		if (!IsWiiSfxPlaying(&gGame.mSound, 0xFF0003)) {
			PlayWiiSoundEffect(&gGame.mSound, 0xFF0003, 1.0f, 0.0f);
		}
		if (!stick) {
			if (title->IsStickUp()) {
				if (title->mSaveFound) {
					if (title->mSelection > 0) {
						title->mSelection--;
						title->mPulseAngle = 45.0f;
						PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
					}
				} else if (title->mSelection != 0) {
					title->mSelection  = 0;
					title->mPulseAngle = 45.0f;
					PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				}
			} else if (title->IsStickDown()) {
				if (title->mSaveFound) {
					if (title->mSelection < 2) {
						title->mSelection++;
						title->mPulseAngle = 45.0f;
						PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
					}
				} else if (title->mSelection != 2) {
					title->mSelection  = 2;
					title->mPulseAngle = 45.0f;
					PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				}
			}
		}
		if (title->IsConfirmPressed() && title->mSelection != -1) {
			switch (title->mSelection) {
			case 0:
			case 1:
				for (s32 i = 0; i < 3; ++i) {
					ResetSaveSlotRecord(&gTitleSaveSlots[i]);
				}
				StartWiiSaveSlotScan(&gGame.mSaveManager, 3);
				title->SetMode(1);
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				title->mState = 12;
				anim          = 1;
				file          = title->mCursor[0].mRequest.mFile;
				if ((file != NULL ? file->mSize : 0) == 0x3CA80) {
					anim = 12;
				}
				title->mCursor[0].SetAnim(anim, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
				title->mCursor[1].SetAnim(anim, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
				title->mPulseAngle = 90.0f;
				break;
			case 2:
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				title->mState = 12;
				anim          = 1;
				file          = title->mCursor[0].mRequest.mFile;
				if ((file != NULL ? file->mSize : 0) == 0x3CA80) {
					anim = 12;
				}
				title->mCursor[0].SetAnim(anim, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
				title->mCursor[1].SetAnim(anim, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
				title->mPulseAngle = 90.0f;
				break;
			}
			addPanes                  = false;
			title->mPanes[5].mPointed = false;
			title->mPanes[5].mPicked  = false;
			title->mPanes[6].mPointed = false;
			title->mPanes[6].mPicked  = false;
			title->mPanes[7].mPointed = false;
			title->mPanes[7].mPicked  = false;
		}
		if (addPanes) {
			AddPointerPane(&gPointerPanes, &title->mPanes[5]);
			if (title->mSaveFound) {
				AddPointerPane(&gPointerPanes, &title->mPanes[6]);
			}
			AddPointerPane(&gPointerPanes, &title->mPanes[7]);
		}
		break;
	}
	case 12: {
		title->mCursor[0].UpdateObject(frames);
		title->mCursor[1].UpdateObject(frames);
		for (s32 i = 0; i < 3; ++i) {
			title->mDemo[i].UpdateObject(frames);
		}
		UpdateTitleDust(title, frames);
		if (title->mCursor[0].IsAnimDone() || title->mCursor[0].GetAnimId() == 0
		    || title->mCursor[0].mObject.m_resFile.GetResAnmChrNumEntries() < 2) {
			title->mCursor[0].SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			title->mCursor[1].SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			title->mPulseAngle = 0.0f;
			switch (title->mSelection) {
			case 0:
				title->mTargetPanelY = 0.0f;
				title->mAlpha        = 1.0f;
				title->mState        = 20;
				UpdateTitleDust(title, GetTitleAndSaveMenuText(0), (u32)(255.0f * title->mLoadAlpha) | 0xFFFFFF00);
				title->mFileSlides[0] = 0.0f;
				title->mFileSlides[1] = 0.0f;
				title->mFileSlides[2] = 0.0f;
				break;
			case 1:
				title->mTargetPanelY = 0.0f;
				title->mAlpha        = 1.0f;
				title->mState        = 20;
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				UpdateTitleDust(title, GetTitleAndSaveMenuText(1), (u32)(255.0f * title->mLoadAlpha) | 0xFFFFFF00);
				title->mFileSlides[0] = 0.0f;
				title->mFileSlides[1] = 0.0f;
				title->mFileSlides[2] = 0.0f;
				break;
			case 2:
				title->mTargetPanelY      = 0.0f;
				title->mTargetOptionAlpha = 1.0f;
				title->mState             = 21;
				title->mOptionState       = 0;
				title->UpdateSelections();
				title->mOptionBar[0]   = title->mOptionBarLines[title->mLastOptionLine];
				title->mOptionBarSnap  = 1;
				title->mSavedPointerOn = title->mPointerOn;
				title->mSavedStickOn   = title->mStickOn;
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				UpdateTitleDust(title, GetTitleAndSaveMenuText(2), (u32)(255.0f * title->mLoadAlpha) | 0xFFFFFF00);
				break;
			}
		}
		break;
	}
	case 20:
		for (s32 i = 0; i < 3; ++i) {
			title->mDemo[i].UpdateObject(frames);
		}
		UpdateTitleDust(title, frames);
		if (!IsWiiSfxPlaying(&gGame.mSound, 0xFF0003)) {
			PlayWiiSoundEffect(&gGame.mSound, 0xFF0003, 1.0f, 0.0f);
		}
		// the file rows slide out to the one picked
		for (s32 i = 0; i < 3; ++i) {
			if (title->mFile == i) {
				if (title->mFileSlides[i] < 1.0f) {
					if ((title->mFileSlides[i] += 0.1f * gTitleFileSlideSpeed[0] * frames) > 1.0f) {
						title->mFileSlides[i] = 1.0f;
					}
				}
			} else if (title->mFileSlides[i] > 0.0f) {
				if ((title->mFileSlides[i] -= 0.1f * gTitleFileSlideSpeed[1] * frames) < 0.0f) {
					title->mFileSlides[i] = 0.0f;
				}
			}
		}
		(title->*gTitleModeUpdates[title->mMode])(frames);
		break;
	case 21:
		for (s32 i = 0; i < 3; ++i) {
			title->mDemo[i].UpdateObject(frames);
		}
		UpdateTitleDust(title, frames);
		if (!IsWiiSfxPlaying(&gGame.mSound, 0xFF0003)) {
			PlayWiiSoundEffect(&gGame.mSound, 0xFF0003, 1.0f, 0.0f);
		}
		UpdateTitleOptions(title, frames);
		break;
	case 13:
		title->mState = 14;
		StartScreenFade(&gGame.mFade, gTitlePalette[gTitlePaletteIndex].background.r, gTitlePalette[gTitlePaletteIndex].background.g,
		                gTitlePalette[gTitlePaletteIndex].background.b, 0, 0, 0, 0, 255, .5f);
		for (s32 i = 0; i < 3; ++i) {
			title->mDemo[i].mState = 0;
		}
		// fall through
	case 14: {
		if (title->mMenuAlpha > 0) {
			if ((title->mMenuAlpha -= (1.f / (.5f * gRefreshRate)) * frames) < 0)
				title->mMenuAlpha = 0;
		}
		if (title->mTitleAlpha > 0) {
			if ((title->mTitleAlpha -= (1.f / (.5f * gRefreshRate)) * frames) < 0)
				title->mTitleAlpha = 0;
		}
		if (title->mMenuAlpha == 0) {
			bool faded = gGame.mFade.mSpeed.r == 0 && gGame.mFade.mSpeed.g == 0 && gGame.mFade.mSpeed.b == 0 && gGame.mFade.mSpeed.a == 0;
			if (faded) {
				GXColor black = { 0, 0, 0, 0 };
				title->mState = 22;
				GXSetCopyClear(black, 0xFFFFFF);
			}
		}
		break;
	}
	case 22:
		for (s32 i = 0; i < 3; ++i) {
			title->mDemo[i].Release();
		}
		title->mCursor[0].Release();
		title->mCursor[1].Release();
		title->mTpl.Close();
		if (title->mTimeout >= 0) {
			ChangeGameStage(&gGame, title->mTimeout, TRUE);
			title->mTimeout = -1;
		}
		gPointerPanes.mMenu = NULL;
		title->mState       = 23;
		break;
	case 23:
		break;
	}
	if (title->mState != 0 && title->mState != 22 && title->mState != 23) {
		GXSetCopyClear(gTitlePalette[gTitlePaletteIndex].background, 0xFFFFFF);
	}
}

/**
 * @note Address: 0x800472A8
 * @note Size: 0x4
 */
void Title::State0(u32)
{
}

/**
 * @note Address: 0x800472AC
 * @note Size: 0x240
 */
// waits for the slot scan: the file menu opens on the newest save, or the error is shown
void Title::State1(u32 frames)
{
	s32 result = gGame.mSaveManager.mFile.mResult;
	mDelay -= frames;
	if (mDelay < 0) {
		mDelay = 0;
	}
	if (result == 0) {
		OSTime timestamps[3];
		for (s32 i = 0; i < 3; ++i) {
			WiiSaveScanEntry* entry = &gGame.mSaveManager.mScanEntries[i];
			if (entry->mStatus & 1) {
				fn_8007A2E8(&gTitleSaveSlots[i], entry, TRUE);
				timestamps[i] = ((WiiSaveSlotHeader*)entry->mData)->mTimestamp;
				if (entry->mStatus & 2) {
					gTitleSaveSlots[i].mState = 1;
					timestamps[i]             = 0;
				}
			} else if (entry->mStatus & 8) {
				gTitleSaveSlots[i]._48 = 1;
				timestamps[i]          = 0;
			} else if (entry->mStatus & 0x10) {
				gTitleSaveSlots[i]._4C = 1;
				timestamps[i]          = 0;
			} else {
				gTitleSaveSlots[i]._44 = 1;
				timestamps[i]          = 0;
			}
		}
		SetMode(2);
		if (mSelection == 0) {
			mFile       = 0;
			mNewestFile = -1;
		} else {
			mFile = mNewestFile = IndexOfLargestOfThree(timestamps[0], timestamps[1], timestamps[2]);
		}
		mLastFile = mFile;
		UpdateSelections();
		mAlpha = 1.0f;
		return;
	}
	mResult = result;
	if (result == -9 || result == -11) {
		SetMode(14);
		mOffset = -32.0f;
	} else if (result == -4) {
		SetMode(19);
		mShowMessage       = 1;
		gSaveFatal.mActive = 1;
	} else {
		SetMode(14);
		mOffset            = -32.0f;
		gSaveFatal.mActive = 1;
	}
	mAlpha = 1.0f;
}

/**
 * @note Address: 0x800474EC
 * @note Size: 0x6CC
 */
// picks one of the three save files: a new game starts on an empty one, a used one loads, B goes back
void Title::State2(u32)
{
	bool addPanes = true;
	bool stick    = false;
	if (mFile != -1) {
		mLastFile = mFile;
	}
	if ((!mReady || !mSaveFound) && mSelection == 1) {
		OSPanic("title.cpp", 0x6cf, "ロード後にメモカが抜かれることはない。ファイルは必ずある");
	}
	if (!mReady) {
		OSPanic("title.cpp", 0x6e5, "メモカが抜かれることはない。");
		OnCardRemoved(this);
		return;
	}
	mOffset = 0.0f;
	if (IsStickUsedOverPointer()) {
		stick = true;
	}
	if (!stick) {
		if (IsStickLeft()) {
			if (--mFile < 0) {
				mFile = 2;
			}
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		} else if (IsStickRight()) {
			if (++mFile > 2) {
				mFile = 0;
			}
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
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
		switch (mSelection) {
		case 0:
			if (gTitleSaveSlots[mFile].mUsed == 1) {
				SetMode(3);
				UpdateSelections();
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
			} else if (gTitleSaveSlots[mFile]._44 == 1) {
				StartWiiSaveSlotWrite(&gGame.mSaveManager, mFile, FALSE);
				SetMode(15);
				mDelay  = 1.5f * gRefreshRate;
				mOffset = -32.0f;
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
			} else {
				if (gTitleSaveSlots[mFile]._48 == 1) {
					mResult = -9;
				} else if (gTitleSaveSlots[mFile]._4C == 1) {
					mResult = -11;
				}
				SetMode(14);
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				mOffset = -32.0f;
			}
			return;
		case 1:
			if (gTitleSaveSlots[mFile].mUsed == 1) {
				if (!IsSaveSlotIntact(&gTitleSaveSlots[mFile])) {
					PlayWiiSoundEffect(&gGame.mSound, 243, 1.0f, 0.0f);
					return;
				}
				if (gTitleSaveSlots[mFile].mStage == 0) {
					HideName(this);
					mTargetPanelY = -200.0f;
					mAlpha        = 0.0f;
					mChoice       = 0;
					mState        = 13;
					SetMode(25);
					PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
					StopWiiSfx(&gGame.mSound, 0xFF0003, 0.0f);
					gGame.mInfoScrn.mSelectedSaveSlot = mFile;
					return;
				}
				if (gTitleSaveSlots[mFile].mStage == 14) {
					HideName(this);
					StopWiiSfx(&gGame.mSound, 0xFF0003, 0.0f);
					mAlpha                            = 0.0f;
					mTargetPanelY                     = -200.0f;
					gGame.mInfoScrn.mSelectedSaveSlot = mFile;
					PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
					SetMode(25);
					mState   = 13;
					mTimeout = 14;
					return;
				}
				StartWiiSaveSlotRead(&gGame.mSaveManager, mFile);
				SetMode(8);
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
			} else {
				PlayWiiSoundEffect(&gGame.mSound, 243, 1.0f, 0.0f);
			}
			return;
		}
	} else if (IsCancelPressed()) {
		SetMode(25);
		mState = 11;
		PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
		mOffset       = 0.0f;
		mTargetPanelY = -200.0f;
		mAlpha        = 0.0f;
		HideName(this);
		mPanes[0].mPointed = false;
		mPanes[0].mPicked  = false;
		mPanes[1].mPointed = false;
		mPanes[1].mPicked  = false;
		mPanes[2].mPointed = false;
		mPanes[2].mPicked  = false;
		addPanes           = false;
		UpdateSelections();
	}
	if (addPanes) {
		for (s32 i = 0; i < 3; i++) {
			AddPointerPane(&gPointerPanes, &mPanes[i]);
		}
	}
}

/**
 * @note Address: 0x80047BB8
 * @note Size: 0x258
 */
// the yes or no question before a file is started over: yes asks once more (State27), no goes back to
// the files
void Title::State3(u32)
{
	bool addPanes = true;
	bool stick    = false;
	if (mChoice != -1) {
		mLastChoice = mChoice;
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
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				return;
			}
		} else if (IsStickRight()) {
			if (mChoice < 1) {
				mChoice++;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				return;
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
			SetMode(27);
			UpdateSelections();
			PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
			return;
		case 1:
			SetMode(2);
			UpdateSelections();
			mChoice = 0;
			PlayWiiSoundEffect(&gGame.mSound, 243, 1.0f, 0.0f);
			return;
		}
	}
	if (addPanes) {
		AddPointerPane(&gPointerPanes, &mPanes[3]);
		AddPointerPane(&gPointerPanes, &mPanes[4]);
	}
}

/**
 * @note Address: 0x80047E10
 * @note Size: 0x2A4
 */
// the yes or no question before a new game is written to the file picked: yes starts the write
void Title::State27(u32)
{
	bool addPanes = true;
	bool stick    = false;
	if (mChoice != -1) {
		mLastChoice = mChoice;
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
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				return;
			}
		} else if (IsStickRight()) {
			if (mChoice < 1) {
				mChoice++;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				return;
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
			SetMode(15);
			mDelay = 1.5f * gRefreshRate;
			PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
			mOffset = -32.0f;
			return;
		case 1:
			SetMode(2);
			UpdateSelections();
			mChoice = 0;
			PlayWiiSoundEffect(&gGame.mSound, 243, 1.0f, 0.0f);
			return;
		}
	}
	if (addPanes) {
		AddPointerPane(&gPointerPanes, &mPanes[3]);
		AddPointerPane(&gPointerPanes, &mPanes[4]);
	}
}

/**
 * @note Address: 0x800480B4
 * @note Size: 0x4
 */
void Title::State4(u32)
{
}

/**
 * @note Address: 0x800480B8
 * @note Size: 0x360
 */
// the question after a save error: yes starts the game without saving, no goes back where the error
// left the menu
void Title::State5(u32)
{
	bool addPanes = true;
	bool stick    = false;
	if (mChoice != -1) {
		mLastChoice = mChoice;
	}
	if (IsStickUsedOverPointer()) {
		stick = true;
	}
	if (!stick) {
		if (IsStickLeft()) {
			if (mChoice > 0) {
				mChoice--;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
			}
		} else if (IsStickRight()) {
			if (mChoice < 1) {
				mChoice++;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
			}
		}
	}
	if (IsConfirmPressed() && mChoice != -1) {
		addPanes           = false;
		mPanes[3].mPointed = false;
		mPanes[3].mPicked  = false;
		mPanes[4].mPointed = false;
		mPanes[4].mPicked  = false;
		if (mChoice == 0 && IsConfirmPressed()) {
			HideName(this);
			mTargetPanelY = -200.0f;
			mOffset       = 0.0f;
			mAlpha        = 0.0f;
			mChoice       = 0;
			mState        = 13;
			SetMode(25);
			PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
			StopWiiSfx(&gGame.mSound, 0xFF0003, 0.0f);
			gGame.mInfoScrn.mSelectedSaveSlot = mFile;
			return;
		}
		if (mChoice == 1 && IsConfirmPressed()) {
			if (mShowMessage == 1) {
				SetMode(19);
				mOffset = 0.0f;
			} else if (mResult == 0) {
				OSPanic("title.cpp", 0x985, "エラーからこないことはない");
				SetMode(2);
				mChoice = 0;
			} else if (mResult == -11 || mResult == -9) {
				for (s32 i = 0; i < 3; i++) {
					ResetSaveSlotRecord(&gTitleSaveSlots[i]);
				}
				StartWiiSaveSlotScan(&gGame.mSaveManager, 10);
				SetMode(1);
				mOffset = 0.0f;
			} else {
				SetMode(14);
				mOffset = -32.0f;
			}
			PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
			return;
		}
	}
	if (addPanes) {
		AddPointerPane(&gPointerPanes, &mPanes[3]);
		AddPointerPane(&gPointerPanes, &mPanes[4]);
	}
}

/**
 * @note Address: 0x80048418
 * @note Size: 0x310
 */
// NONMATCHING: registers only, retail keeps this in r30 and the pane flag in r31
// the same question when a save error came from the file menu: yes starts the game without saving and
// has the game reset to the Wii Menu's data management when it ends (reset type 5), no goes back
void Title::State26(u32)
{
	bool addPanes = true;
	bool stick    = false;
	if (mChoice != -1) {
		mLastChoice = mChoice;
	}
	if (IsStickUsedOverPointer()) {
		stick = true;
	}
	if (!stick) {
		if (IsStickLeft()) {
			if (mChoice > 0) {
				mChoice--;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
			}
		} else if (IsStickRight()) {
			if (mChoice < 1) {
				mChoice++;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
			}
		}
	}
	if (IsConfirmPressed() && mChoice != -1) {
		addPanes           = false;
		mPanes[3].mPointed = false;
		mPanes[3].mPicked  = false;
		mPanes[4].mPointed = false;
		mPanes[4].mPicked  = false;
		if (mChoice == 0 && IsConfirmPressed()) {
			HideName(this);
			mTargetPanelY = -200.0f;
			mOffset       = 0.0f;
			mAlpha        = 0.0f;
			mChoice       = 0;
			mState        = 13;
			SetMode(25);
			PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
			StopWiiSfx(&gGame.mSound, 0xFF0003, 0.0f);
			gGame.mInfoScrn.mSelectedSaveSlot = mFile;
			gGame.mResetRequested             = true;
			gGame.mResetType                  = 5;
			return;
		}
		if (mChoice == 1 && IsConfirmPressed()) {
			if (mShowMessage == 1) {
				SetMode(5);
				mOffset = -32.0f;
				UpdateSelections();
			} else if (mResult == 0) {
				OSPanic("title.cpp", 0xa5c, "エラーからこないことはない");
				SetMode(2);
				mChoice = 0;
			} else {
				SetMode(5);
				mOffset = -32.0f;
				UpdateSelections();
			}
			PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
			return;
		}
	}
	if (addPanes) {
		AddPointerPane(&gPointerPanes, &mPanes[3]);
		AddPointerPane(&gPointerPanes, &mPanes[4]);
	}
}

/**
 * @note Address: 0x80048728
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State6(u32)
{
	OSPanic("title.cpp", 0xa90, "ここに来るのはおかしい");
}

/**
 * @note Address: 0x80048740
 * @note Size: 0x4
 */
void Title::State7(u32)
{
}

/**
 * @note Address: 0x80048744
 * @note Size: 0x198
 */
// NONMATCHING: retail tests -9, -11 and -4 in a row, an if chain, and still keeps three copies of the
// same body; written as an if chain this build merges the bodies (70 %), as a switch it keeps them but
// searches the values as a tree
// waits for the save file to be read: into the game when it was, else the error is shown
void Title::State8(u32)
{
	s32 result = gGame.mSaveManager.mFile.mResult;
	if (result == 0) {
		HideName(this);
		StopWiiSfx(&gGame.mSound, 0xFF0003, 0.0f);
		mTargetPanelY                     = -200.0f;
		mAlpha                            = 0.0f;
		gGame.mInfoScrn.mSelectedSaveSlot = mFile;
		ResumeScrapCounterPulse(&gGame.mInfoScrn);
		SetMode(25);
		mState   = 13;
		mTimeout = 10002;
		return;
	}
	mResult = result;
	switch (result) {
	case -9:
	case -11:
		SetMode(19);
		mShowMessage = 1;
		break;
	case -4:
		SetMode(19);
		mShowMessage = 1;
		break;
	default:
		SetMode(19);
		mShowMessage = 1;
		break;
	}
	gSaveFatal.mActive = 1;
	mAlpha             = 1.0f;
}

/**
 * @note Address: 0x800488DC
 * @note Size: 0x130
 */
// the save error screen of the GameCube's card check, which the Wii never reaches: a new scan when the
// save system asks for one, back to the error choice on A
void Title::State9(u32)
{
	OSPanic("title.cpp", 0xbbd, "ここに来るのはおかしい");
	if (mWrongDevice == 1 || mReady == 1) {
		for (s32 i = 0; i < 3; ++i) {
			ResetSaveSlotRecord(&gTitleSaveSlots[i]);
		}
		StartWiiSaveSlotScan(&gGame.mSaveManager, 10);
		SetMode(1);
		PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
		mOffset = 0.0f;
		return;
	}
	if (IsConfirmPressed()) {
		SetMode(5);
		mOffset = -32.0f;
		mChoice = 1;
		PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
	}
}

/**
 * @note Address: 0x80048A0C
 * @note Size: 0x4
 */
void Title::State10(u32)
{
}

/**
 * @note Address: 0x80048A10
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State11(u32)
{
	OSPanic("title.cpp", 0xbeb, "ここにくるのはおかしい");
}

/**
 * @note Address: 0x80048A28
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State12(u32)
{
	OSPanic("title.cpp", 0xc64, "ここにくるのはおかしい");
}

/**
 * @note Address: 0x80048A40
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State13(u32)
{
	OSPanic("title.cpp", 0xcc0, "ここにくるのはおかしい");
}

/**
 * @note Address: 0x80048A58
 * @note Size: 0xC8
 */
// a save error is shown until A, then the file menu comes back
void Title::State14(u32)
{
	if (IsConfirmPressed()) {
		switch (mResult) {
		case -11:
		case -9:
			SetMode(26);
			mOffset = -32.0f;
			UpdateSelections();
			break;
		case -10:
		default:
			SetMode(26);
			mOffset = -32.0f;
			UpdateSelections();
			break;
		}
		PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
	}
}

/**
 * @note Address: 0x80048B20
 * @note Size: 0x17C
 */
// NONMATCHING: the error tail as in State8
// waits for the file's write: its record is refreshed and the game starts, or the error is shown
void Title::State15(u32 frames)
{
	s32 result = gGame.mSaveManager.mFile.mResult;
	mDelay -= frames;
	if (mDelay < 0) {
		mDelay = 0;
	}
	if (result == 0) {
		u32 blocks, inodes;
		GetNandGameFreeSpace(&gGame.mSaveManager.mFile, &blocks, &inodes);
		mFreeBlocks             = blocks;
		mFreeEntries            = inodes;
		WiiSaveScanEntry* entry = &gGame.mSaveManager.mScanEntries[mFile];
		if (entry->mStatus & 1) {
			fn_8007A2E8(&gTitleSaveSlots[mFile], entry, TRUE);
		}
		SetMode(24);
		return;
	}
	mResult = result;
	switch (result) {
	case -9:
	case -11:
		SetMode(19);
		mShowMessage = 1;
		break;
	case -4:
		SetMode(19);
		mShowMessage = 1;
		break;
	default:
		SetMode(19);
		mShowMessage = 1;
		break;
	}
	gSaveFatal.mActive = 1;
	mAlpha             = 1.0f;
}

/**
 * @note Address: 0x80048C9C
 * @note Size: 0x4
 */
void Title::State16(u32)
{
}

/**
 * @note Address: 0x80048CA0
 * @note Size: 0x4
 */
void Title::State17(u32)
{
}

/**
 * @note Address: 0x80048CA4
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State18(u32)
{
	OSPanic("title.cpp", 0xe87, "ガイドライン1.09-->1.10で削除したはずの遷移です");
}

/**
 * @note Address: 0x80048CBC
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State19(u32)
{
	OSPanic("title.cpp", 0xe87, "ガイドライン1.09-->1.10で削除したはずの遷移です");
}

/**
 * @note Address: 0x80048CD4
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State20(u32)
{
	OSPanic("title.cpp", 0xeb2, "ここにくるのはおかしい");
}

/**
 * @note Address: 0x80048CEC
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State21(u32)
{
	OSPanic("title.cpp", 0xeb2, "ここにくるのはおかしい");
}

/**
 * @note Address: 0x80048D04
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State22(u32)
{
	OSPanic("title.cpp", 0xeb2, "ここにくるのはおかしい");
}

/**
 * @note Address: 0x80048D1C
 * @note Size: 0x4
 */
void Title::State23(u32)
{
}

/**
 * @note Address: 0x80048D20
 * @note Size: 0x130
 */
// waits out the delay, then fades the labels and leaves for the game
void Title::State24(u32 frames)
{
	switch (mPhase) {
	case 0:
		mDelay -= frames;
		if (mDelay < 0) {
			mDelay = 0;
			++mPhase;
		}
		break;
	case 1:
		HideName(this);
		mTargetPanelY = -200.0f;
		mOffset       = 0.0f;
		mAlpha        = 0.0f;
		mChoice       = 0;
		mState        = 13;
		SetMode(25);
		PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
		StopWiiSfx(&gGame.mSound, 0xFF0003, 0.0f);
		gGame.mInfoScrn.mSelectedSaveSlot = mFile;
		break;
	}
}

/**
 * @note Address: 0x80048E50
 * @note Size: 0x4
 */
void Title::State25(u32)
{
}

/**
 * @note Address: 0x80048E54
 * @note Size: 0x1950
 */
// NONMATCHING: the pass structure and both cursor tails (pane sizes, cursor pair, bar snap, knob) are as
// retail, with its call list and literals. Retail builds each pane size in a stack temporary first, and
// float registers and scheduling differ.
extern "C" void DrawTitleScreen(Title* title)
{
	enum { dustTexture = 2, footerTexture = 1 };
	bool active = false;
	if (title->mState >= 1 && title->mState < 23) {
		active = true;
	}
	if (!active) {
		return;
	}
	bool homeMenu = false;
	if (gGame.mHomeMenu.mState >= 2 && gGame.mHomeMenu.mMenuStep >= 3) {
		homeMenu = true;
	}
	if (homeMenu) {
		return;
	}
	if (((CBase*)&gGame)->vf5C()) {
		if (title->mMenuAlpha > 0.0f) {
			BeginTitleOverlay();
			for (s32 i = 0; i < 16; ++i) {
				f32 alpha = title->mMenuAlpha;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				TitleDust& dust = title->mDust[i];
				f32 w           = dust.mSpeedX * (f32)GXGetTexObjWidth(&title->mTex[dustTexture]);
				f32 h           = dust.mSpeedX * (f32)GXGetTexObjHeight(&title->mTex[dustTexture]);
				u32 color       = 0xffffff00 | (u32)(255.0f * dust.mSpeedY * alpha);
				DrawGsTexturedQuad(&gGame.mGs, &title->mTex[dustTexture], color, color, color, color, dust.mX, dust.mY, 0.0f, w, h, 0.0f,
				                   0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			CTexObj& logo = title->mTex[gTitleLogoTextures[gTitleLogoIndex]];
			f32 w         = GXGetTexObjWidth(&logo);
			f32 h         = GXGetTexObjHeight(&logo);
			u32 color     = 0xffffff00 | (u32)(255.0f * title->mMenuAlpha);
			DrawGsTexturedQuad(&gGame.mGs, &logo, color, color, color, color, (f32)(gPointerScreenWidth / 2),
			                   10.0f + (f32)(gPointerScreenHeight / 2 - 0x68), 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
		((CBase*)&gGame)->vf54();
	}
	if (((CBase*)&gGame)->vf5C()) {
		for (s32 i = 0; i < 3; ++i) {
			XObjDemo& fig = title->mDemo[i];
			if (fig.mState != 0 && fig.mStatus.Check(1)) {
				fig.DrawObject();
			}
		}
		CCamera* camera = ((CBase*)&gGame)->GetScreenCamera();
		fn_802AEDEC(&gGame.mGs.mScene.mHead, camera->mViewMtx);
		WiiGsSceneObj* obj = gGame.mGs.mScene.mHead;
		while (obj->mPending != 0) {
			obj->vf038(obj->mPending - 1);
		}
		((CBase*)&gGame)->vf54();
	}
	if (!((CBase*)&gGame)->vf5C()) {
		return;
	}
	if (title->mMenuAlpha > 0.0f) {
		BeginTitleOverlay();
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		CTexObj& tex = title->mTex[footerTexture];
		f32 w        = 0.6f * (f32)GXGetTexObjWidth(&tex);
		f32 h        = 0.6f * (f32)GXGetTexObjHeight(&tex);
		u32 color    = (u32)(255.0f * title->mMenuAlpha);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, (f32)(gPointerScreenWidth / 2), (f32)(gScreenSafeBottom - 0x14),
		                   0.0f, w, h, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
	gRingScreenWidth    = 0x280;
	gPointerScreenWidth = 0x280;
	s32 w               = gGame.mAspectRatio != 0 ? 0x356 : 0x280;
	s32 off             = (w - 0x280) / 2;
	CCamera* camera     = ((CBase*)&gGame)->GetScreenCamera();
	camera->vf24(0.0f, (f32)gPointerScreenHeight, -(f32)off, (f32)w - (f32)off);
	((CBase*)&gGame)->vf5C();
	DrawSavePanelBackground(title->mLoadAlpha + title->mOptionAlpha);
	DrawTitleHelpText(title);
	DrawTitleLoadScreen(title);
	DrawTitleOptions(title);
	DrawTitleNameBubbles(title);
	if ((title->mMode == 0x13 || title->mMode == 0xE) && gSaveFatal.mActive && gSaveFatal.mMessage != NULL) {
		OSFatal(gSaveFatal.mFg, gSaveFatal.mBg, gSaveFatal.mMessage);
	}
	InitGameOrthoCamera(camera, &gGame.mGs.mScreen);
	((CBase*)&gGame)->vf5C();
	w                   = gGame.mAspectRatio != 0 ? 0x356 : 0x280;
	gRingScreenWidth    = w;
	gPointerScreenWidth = w;
	f32 top             = -40.0f;
	f32 gap             = 38.0f;
	if ((u32)(title->mState - 10) <= 2) {
		f32 y   = (f32)(gScreenSafeBottom - 0x36) + top - 30.0f;
		f32 x   = gPointerScreenWidth / 2;
		u32 rgb = ((u32)gTitlePalette[gTitlePaletteIndex].text.r << 24) | ((u32)gTitlePalette[gTitlePaletteIndex].text.g << 16)
		        | ((u32)gTitlePalette[gTitlePaletteIndex].text.b << 8);
		u32 normal   = rgb | (u8)(255.0f * title->mMenuAlpha);
		u32 lit      = normal;
		f32 progress = title->mCursor[0].GetAnimId() == 1 ? title->mCursor[0].GetAnimFrame() / title->mCursor[0].GetAnimEndFrame() : 0.0f;
		f32 w0       = DrawTitleEntry(title, 0, 0, x, y, progress, normal, lit);
		y            = top + ((f32)(gScreenSafeBottom - 0x36) + gap) - 30.0f;
		f32 w1       = DrawTitleEntry(title, 1, 1, x, y, progress, normal, lit);
		y            = top + ((f32)(gScreenSafeBottom - 0x36) + 2.0f * gap) - 30.0f;
		f32 w2       = DrawTitleEntry(title, 2, 2, x, y, progress, normal, lit);
		f32 width;
		switch (title->mSelection != -1 ? title->mSelection : title->mLastSelection) {
		case -1:
			OSPanic("title.cpp", 0x1077, "ここにはこない");
			width = 0.0f;
			y     = gScreenSafeBottom + 200;
			break;
		case 0:
			width = w0;
			y     = (f32)(gScreenSafeBottom - 0x36) + top - 30.0f;
			break;
		case 1:
			width = w1;
			y     = top + ((f32)(gScreenSafeBottom - 0x36) + gap) - 30.0f;
			break;
		case 2:
			width = w2;
			y     = top + ((f32)(gScreenSafeBottom - 0x36) + 2.0f * gap) - 30.0f;
			break;
		}
		// the pointer's rectangles around the three entries
		f32 margin               = 24.0f;
		title->mPanes[5].mWidth  = 40.0f + (2.0f * margin + w0);
		title->mPanes[5].mHeight = 28.0f;
		title->mPanes[6].mWidth  = 40.0f + (2.0f * margin + w1);
		title->mPanes[6].mHeight = 28.0f;
		title->mPanes[7].mWidth  = 40.0f + (2.0f * margin + w2);
		title->mPanes[7].mHeight = 28.0f;
		if ((title->mCursor[0].mRequest.mFile != NULL ? title->mCursor[0].mRequest.mFile->mSize : 0) == 0x3CA80) {
			y -= 10.0f;
		}
		CColor fade;
		fade.Set(255, 255, 255, 255);
		f32 rate = 1.0f;
		if (title->mSelection == -1) {
			fade.Set((s32)(fade.r * 0.3f), (s32)(fade.g * 0.3f), (s32)(fade.b * 0.3f), (s32)(fade.a * 0.3f));
			rate = 0.0f;
		}
		title->mCursor[0].SetAnimRate(rate);
		title->mCursor[0].SetColor(0, fade.r, fade.g, fade.b, 0.1f);
		title->mCursor[0].SetScale(1.2f, 1.2f, 1.2f);
		title->mCursor[0].SetPos((x - width * 0.5f) - margin, y, 10.0f);
		title->mCursor[0].SetRotationY(0.2617994f);
		title->mCursor[0].DrawObject();
		title->mCursor[1].SetAnimRate(rate);
		title->mCursor[1].SetColor(0, fade.r, fade.g, fade.b, 0.1f);
		title->mCursor[1].SetScale(1.2f, 1.2f, 1.2f);
		title->mCursor[1].SetPos(margin + (x + width * 0.5f), y, 10.0f);
		title->mCursor[1].SetRotationY(-0.2617994f);
		title->mCursor[1].DrawObject();
		if (title->mOptionBarSnap) {
			title->mOptionBar[1] = title->mOptionBarTarget[0] = 2.0f * margin + width;
			title->mOptionBar[2] = title->mOptionBarTarget[1] = title->mCursor[0].GetPos()->x;
			title->mOptionBarSnap                             = 0;
		} else {
			title->mOptionBarTarget[0] = 2.0f * margin + width;
			title->mOptionBarTarget[1] = title->mCursor[0].GetPos()->x;
		}
		u32 knob = GetTitleCursorColor(title);
		CColor knobColor;
		knobColor.Set((s32)(knob >> 24), (s32)((knob >> 16) & 0xFF), (s32)((knob >> 8) & 0xFF), (s32)(knob & 0xFF));
		fn_8004E6EC(title, FALSE, title->mOptionBar[2], title->mOptionBar[0], title->mMenuAlpha * (knobColor.r / 255.0f),
		            title->mOptionBar[1]);
	}
	if (title->mMode == 0x15) {
		f32 x           = gPointerScreenWidth / 2;
		f32 size        = 24.0f;
		f32 lineGap     = 4.0f * (size * 0.5f);
		f32 gap         = 8.0f;
		f32 margin      = 30.0f;
		f32 labelW      = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x11), TRUE, size);
		f32 labelW2     = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x14), TRUE, size);
		labelW          = labelW > labelW2 ? labelW : labelW2;
		u32 optionWidth = 2.0f * (labelW + gap);
		f32 okWidth     = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x17), TRUE, size);
		// the pointer's rectangles around the three lines keep the first one's height
		f32 height                = title->mPanes[8].mHeight;
		title->mPanes[8].mWidth   = 40.0f + (2.0f * margin + optionWidth);
		title->mPanes[8].mHeight  = height;
		title->mPanes[9].mWidth   = 40.0f + (2.0f * margin + optionWidth);
		title->mPanes[9].mHeight  = height;
		title->mPanes[10].mWidth  = 40.0f + (2.0f * margin + okWidth);
		title->mPanes[10].mHeight = height;
		f32 width;
		f32 y;
		switch (title->mOptionLine != -1 ? title->mOptionLine : title->mLastOptionLine) {
		case -1:
			OSPanic("title.cpp", 0x1117, "ここにはこない");
			break;
		case 0:
			width = optionWidth;
			y     = gScreenSafeBottom - 0xD2;
			break;
		case 1:
			width = optionWidth;
			y     = (gScreenSafeBottom - 0xD2) + lineGap;
			break;
		case 2:
			width = okWidth;
			y     = 2.0f * lineGap + (gScreenSafeBottom - 0xD2);
			break;
		}
		CColor fade;
		fade.Set(255, 255, 255, 255);
		f32 rate = 1.0f;
		if (title->mOptionLine == -1) {
			fade.Set((s32)(fade.r * 0.3f), (s32)(fade.g * 0.3f), (s32)(fade.b * 0.3f), (s32)(fade.a * 0.3f));
			rate = 0.0f;
		}
		title->mCursor[0].SetAnimRate(rate);
		title->mCursor[0].SetColor(0, fade.r, fade.g, fade.b, 0.1f);
		title->mCursor[0].SetPos((x - width * 0.5f) - margin, y, 10.0f);
		title->mCursor[0].SetRotationY(0.2617994f);
		title->mCursor[0].DrawObject();
		title->mCursor[1].SetAnimRate(rate);
		title->mCursor[1].SetColor(0, fade.r, fade.g, fade.b, 0.1f);
		title->mCursor[1].SetPos(margin + (x + width * 0.5f), y, 10.0f);
		title->mCursor[1].SetRotationY(-0.2617994f);
		title->mCursor[1].DrawObject();
		if (title->mOptionBarSnap) {
			title->mOptionBar[2]       = x;
			title->mOptionBar[1]       = 0.0f;
			title->mOptionBarTarget[0] = 2.0f * margin + width;
			title->mOptionBarTarget[1] = title->mCursor[0].GetPos()->x;
			title->mOptionBarSnap      = 0;
		} else {
			title->mOptionBarTarget[0] = 2.0f * margin + width;
			title->mOptionBarTarget[1] = title->mCursor[0].GetPos()->x;
		}
		u32 knob = GetTitleCursorColor(title);
		CColor knobColor;
		knobColor.Set((s32)(knob >> 24), (s32)((knob >> 16) & 0xFF), (s32)((knob >> 8) & 0xFF), (s32)(knob & 0xFF));
		fn_8004E6EC(title, TRUE, title->mOptionBar[2], title->mOptionBar[0], title->mOptionAlpha * (knobColor.r / 255.0f),
		            title->mOptionBar[1]);
	}
	((CBase*)&gGame)->vf54();
}

// the options page: the pointer and the stick can each be turned off, but not both; OK (or A on the
// third line) writes the two to gGame.mMenuControls, B leaves them as they were
/**
 * @note Address: 0x8004BC54
 * @note Size: 0x8F4
 */
// NONMATCHING: read from retail's code; the float registers and spills differ, and the two colors are
// put together after the frame is drawn. The heading's text is fetched once more and dropped; OK works
// out its pulsing size as DrawTitleEntry does but draws at the plain size, so okSize stays unused
// the options page: the frame, its heading, the pointer and stick settings with their highlight bars and OK
static f32 sSaveSlotExtraWidth;

static inline f32 LoadPromptY(Title* title, f32 panelY)
{
	return ((8.0f + (356.0f - panelY)) - 6.0f) + title->mLoadOffset;
}

static inline u32 LoadTextColor(f32 alpha)
{
	return (u32)(255.0f * alpha) | 0x14141400;
}

static inline void InitPCPanelTexture(CTexObj& tex, s32 icon)
{
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[icon], 0);
	tex = gGame.mTpl.mTex;
}

static inline void DrawLoadSelectionBar(Title* title, f32 barX, f32 barY, f32 barW, f32 barH)
{
	CTexObj fill, edges;
	u32 barColor = (u32)(200.0f * title->mLoadAlpha) | 0x181CC600;
	f32 alpha    = title->mLoadAlpha;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	InitPCPanelTexture(edges, 118);
	PSMTXScale(edges.mTexMtx.mMtx[2], 0.25f, 1.0f, 1.0f);
	edges.mTexMtx.mDirty = TRUE;
	PSMTXTrans(edges.mTexMtx.mMtx[0], 0.25f, 0.0f, 0.0f);
	edges.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &edges, barColor, barColor, barColor, barColor, barX, barY, 0.0f, barW, barH, 0.0f, 0.0f, 0.0f);
	PSMTXTrans(edges.mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
	edges.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &edges, barColor, barColor, barColor, barColor, (barX - barW / 2.0f) - 4.0f, barY, 0.0f, 8.0f, barH,
	                   0.0f, 0.0f, 0.0f);
	PSMTXTrans(edges.mTexMtx.mMtx[0], 0.5f, 0.0f, 0.0f);
	edges.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &edges, barColor, barColor, barColor, barColor, 4.0f + (barX + barW / 2.0f), barY, 0.0f, 8.0f, barH,
	                   0.0f, 0.0f, 0.0f);
	InitPCPanelTexture(fill, 119);
	PSMTXTrans(fill.mTexMtx.mMtx[0], title->mLoadScroll, 0.0f, 0.0f);
	fill.mTexMtx.mDirty = TRUE;
	PSMTXScale(fill.mTexMtx.mMtx[2], 0.5f * (barW / GXGetTexObjWidth(&fill)), 0.5f, 1.0f);
	fill.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &fill, (u32)(40.0f * alpha) | 0xFFFFFF00, (u32)(40.0f * alpha) | 0xFFFFFF00,
	                   (u32)(40.0f * alpha) | 0xFFFFFF00, (u32)(40.0f * alpha) | 0xFFFFFF00, 2.0f + barX, barY, 0.0f, barW, barH - 4.0f,
	                   0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

static inline void DrawLoadChoiceLabel(Title* title, s32 id, f32 x, f32 y)
{
	u32 color        = LoadTextColor(title->mLoadAlpha);
	const char* text = GetLocalizedInterfaceText(id);
	f32 width        = MeasureGameTextWidth(&gGame.mGs, text, TRUE, 21.0f);
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - width / 2.0f, y, 0.0f, 21.0f, 21.0f);
}

static inline s32 LoadStatusLines(Title* title, s32 id, f32 panelX, f32 panelY)
{
	return DrawJapaneseSaveStatusMessage(title->mChannel, id, panelX + gPointerScreenWidth / 2, LoadPromptY(title, panelY), 21.0f,
	                                     title->mLoadAlpha);
}

extern "C" void DrawTitleLoadScreen(Title* title)
{
	f32 scale = 1.0f;
	switch (title->mMode) {
	case 2:
	case 3:
	case 8:
	case 15:
	case 24:
	case 27: {
		f32 height = 230.0f;
		f32 y      = height * 0.5f + 108.0f;
		for (s32 i = 0; i < 3; ++i) {
			f32 width   = 154.0f + sSaveSlotExtraWidth;
			f32 x       = 49.0f + ((width * 0.5f) + i * (40.0f + width)) - (3.0f * sSaveSlotExtraWidth) * 0.5f;
			u8 selected = 0, newest = 0;
			if (title->mFile == i) {
				selected = 1;
			}
			if (title->mNewestFile == i) {
				newest = 1;
			}
			DrawSaveSlotDetails(&gTitleSaveSlots[i], 0, i, selected, newest, x, y, width, height, title->mLoadAlpha, title->mFileSlides[i],
			                    scale, scale);
		}
		break;
	}
	}
	f32 panelX = title->mPanelX, panelY = title->mPanelY - 8.0f;
	DrawMenuBannerPanels(NULL, panelX, panelY, scale, title->mLoadOffset);
	bool showMessage = false;
	s32 lines        = 2;
	char text[256];

	switch (title->mMode) {
	case 2:
		if (title->mSelection == 0) {
			u32 color        = LoadTextColor(title->mLoadAlpha);
			f32 top          = 8.0f + (356.0f - panelY);
			f32 y            = (top - 6.0f) + title->mLoadOffset;
			const char* line = GetLocalizedInterfaceText(0x95);
			DrawGsProportionalTextFormat(&gGame.mGs, color, line,
			                             (f32)(gPointerScreenWidth / 2) - MeasureGameTextWidth(&gGame.mGs, line, TRUE, 21.0f) / 2.0f, y,
			                             0.0f, 21.0f, 21.0f);
			line = GetLocalizedInterfaceText(0x96);
			DrawGsProportionalTextFormat(&gGame.mGs, color, line,
			                             (f32)(gPointerScreenWidth / 2) - MeasureGameTextWidth(&gGame.mGs, line, TRUE, 21.0f) / 2.0f,
			                             (21.0f + top) + title->mLoadOffset, 0.0f, 21.0f, 21.0f);
			lines = 2;
		} else {
			sprintf(text, "%s", GetLocalizedInterfaceText(0xc2));
			showMessage = true;
		}
		break;
	case 3:
		sprintf(text, "%s", GetLocalizedInterfaceText(0x12a));
		showMessage = true;
		break;
	case 27:
		sprintf(text, "%s", GetLocalizedInterfaceText(0x12b));
		showMessage = true;
		break;
	case 5:
		lines = LoadStatusLines(title, 10, panelX, panelY);
		break;
	case 26:
		lines = LoadStatusLines(title, 11, panelX, panelY);
		break;
	case 6:
		OSPanic("title.cpp", 0x1222, "ここにはこない");
		lines = LoadStatusLines(title, 12, panelX, panelY);
		break;
	case 8:
		sprintf(text, "%s", GetLocalizedInterfaceText(0xc7));
		showMessage = true;
		break;
	case 11: {
		f32 x = panelX + gPointerScreenWidth / 2, y = LoadPromptY(title, panelY);
		if (title->mPhase == 0) {
			lines = DrawJapaneseSaveStatusMessage(title->mChannel, 5, x, y, 21.0f, title->mLoadAlpha);
		} else {
			lines = DrawJapaneseSaveStatusMessage(title->mChannel, 6, x, y, 21.0f, title->mLoadAlpha);
		}
		break;
	}
	case 12:
		lines = LoadStatusLines(title, 7, panelX, panelY);
		break;
	case 13: {
		f32 x = panelX + gPointerScreenWidth / 2, y = LoadPromptY(title, panelY);
		if (title->mPhase == 0) {
			lines = DrawJapaneseSaveStatusMessage(title->mChannel, 7, x, y, 21.0f, title->mLoadAlpha);
		} else {
			lines = DrawJapaneseSaveStatusMessage(title->mChannel, 9, x, y, 21.0f, title->mLoadAlpha);
		}
		break;
	}
	case 9:
	case 14:
		lines = DrawJapaneseCardErrorMessage(title->mChannel, title->mResult, 1, panelX + gPointerScreenWidth / 2,
		                                     LoadPromptY(title, panelY), 21.0f, title->mLoadAlpha);
		break;
	case 15:
		lines = LoadStatusLines(title, 4, panelX, panelY);
		break;
	case 18:
		lines = LoadStatusLines(title, 1, panelX, panelY);
		break;
	case 19: {
		f32 x = panelX + gPointerScreenWidth / 2, y = LoadPromptY(title, panelY);
		if (title->mResult == -4) {
			lines = DrawJapaneseSaveStatusMessage(title->mChannel, 13, x, y, 21.0f, title->mLoadAlpha);
		} else if ((u32)(title->mResult + 3) <= 1) {
			lines = DrawJapaneseSaveStatusMessage(title->mChannel, 16, x, y, 21.0f, title->mLoadAlpha);
		} else {
			lines = DrawJapaneseSaveStatusMessage(title->mChannel, 17, x, y, 21.0f, title->mLoadAlpha);
		}
		break;
	}
	case 20:
		lines = LoadStatusLines(title, 3, panelX, panelY);
		break;
	case 21:
		lines = LoadStatusLines(title, 8, panelX, panelY);
		break;
	case 22:
		lines = LoadStatusLines(title, 19, panelX, panelY);
		break;
	case 24: {
		f32 x = panelX + gPointerScreenWidth / 2, y = LoadPromptY(title, panelY);
		if (title->mPhase == 0) {
			lines = DrawJapaneseSaveStatusMessage(title->mChannel, 4, x, y, 21.0f, title->mLoadAlpha);
		}
		break;
	}
	}
	if (lines <= 2) {
		lines = 2;
	}
	title->mOffset = -(lines - 2) * 21;
	if (showMessage) {
		u32 color = LoadTextColor(title->mLoadAlpha);
		f32 y     = LoadPromptY(title, panelY);
		DrawGsProportionalTextFormat(&gGame.mGs, color, text,
		                             (f32)(gPointerScreenWidth / 2) - MeasureGameTextWidth(&gGame.mGs, text, TRUE, 21.0f) / 2.0f, y, 0.0f,
		                             21.0f, 21.0f);
	}

	switch (title->mMode) {
	case 3:
	case 4:
	case 5:
	case 6:
	case 11:
	case 26:
	case 27: {
		f32 extra = 0.0f;
		if (lines > 1) {
			extra = (lines - 2) * 21;
		}
		f32 barWidth = 61.5f;
		f32 barX;
		if (title->mChoice == -1) {
			barX = gPointerScreenWidth + 0x96;
		} else if (title->mChoice == 0) {
			barX = gPointerScreenWidth / 2 - (16.0f + barWidth) * 0.5f;
		} else {
			barX = gPointerScreenWidth / 2 + (16.0f + barWidth) * 0.5f;
		}
		f32 base   = (24.0f + (12.0f + (356.0f - panelY))) - 6.0f;
		f32 labelY = extra + (base + title->mLoadOffset);
		DrawLoadSelectionBar(title, barX, 2.0f + (8.0f + labelY), barWidth, 22.0f);
		for (s32 i = 0; i < 2; ++i) {
			CbPaneLayout* layout = &lbl_802E7F48[3 + i];
			layout->mTrans.y     = 2.0f + (8.0f + labelY);
			f32 screenX          = (f32)gPointerScreenWidth;
			f32 screenY          = (f32)gPointerScreenHeight;
			Vec trans            = layout->mTrans;
			f32 left             = -0.5f * screenX;
			f32 top              = 0.5f * screenY;
			f32 right            = 0.5f * screenX;
			f32 bottom           = -0.5f * screenY;
			layout->mTrans.y     = -(trans.y + 0.5f * (bottom - top));
			fn_800556B0(&title->mPanes[3 + i], layout);
		}
		labelY = extra + (base + title->mLoadOffset);
		if (gGame.mLanguage == 0) {
			DrawLoadChoiceLabel(title, 0x66, gPointerScreenWidth / 2 - (16.0f + barWidth) * 0.5f, labelY);
			DrawLoadChoiceLabel(title, 0x67, gPointerScreenWidth / 2 + (16.0f + barWidth) * 0.5f, labelY);
		} else {
			DrawLoadChoiceLabel(title, 0x66, 2.0f + (gPointerScreenWidth / 2 - (16.0f + barWidth) * 0.5f), labelY - 1.0f);
			DrawLoadChoiceLabel(title, 0x67, 2.0f + (gPointerScreenWidth / 2 + (16.0f + barWidth) * 0.5f), labelY - 1.0f);
		}
		break;
	}
	}
	if (title->mOptionAlpha > 0.0f) {
		u32 color        = LoadTextColor(title->mOptionAlpha);
		const char* text = GetLocalizedInterfaceText(0x62);
		DrawGsProportionalTextFormat(&gGame.mGs, color, text,
		                             (f32)(gPointerScreenWidth / 2) - MeasureGameTextWidth(&gGame.mGs, text, TRUE, 21.0f) / 2.0f,
		                             LoadPromptY(title, panelY), 0.0f, 21.0f, 21.0f);
	}
}

extern "C" void DrawTitleOptions(Title* title)
{
	if (title->mOptionAlpha > 0.0f) {
		f32 alpha  = title->mOptionAlpha;
		f32 cx     = gPointerScreenWidth / 2;
		f32 cy     = gPointerScreenHeight / 2;
		f32 w      = (f32)(gPointerScreenWidth / 2) * alpha;
		f32 h      = (f32)(gPointerScreenHeight / 2) * alpha;
		u32 lit    = 0xFFB62800 | (u8)(255.0f * alpha);
		u32 normal = 0xFFFFFF00 | (u8)(255.0f * alpha);
		u32 frame  = 160.0f * alpha;
		DrawTexturedWindowFrame(frame, (u32)(255.0f * title->mOptionAlpha) | 0xFFFFFF00, cx, 223.0f - h / 2.0f, w, h, 1.0f, 1.0f, 1.0f);
		const char* heading = GetLocalizedInterfaceText(300);
		f32 size            = 24.0f * title->mOptionAlpha;
		const char* text    = GetLocalizedInterfaceText(300);
		f32 textWidth       = MeasureGameTextWidth(&gGame.mGs, text, TRUE, size);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, cx - textWidth / 2.0f, size + (cy - h / 2.0f), 0.0f, size, size);

		size        = 24.0f * title->mOptionAlpha;
		f32 gap     = 8.0f;
		f32 labelW  = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x11), TRUE, size);
		f32 labelW2 = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x14), TRUE, size);
		labelW      = labelW > labelW2 ? labelW : labelW2;
		f32 valueW  = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x12), TRUE, size);
		f32 valueW2 = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x13), TRUE, size);
		f32 valueW3 = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x15), TRUE, size);
		f32 valueW4 = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x16), TRUE, size);
		valueW      = valueW > valueW2 ? valueW : valueW2;
		valueW3     = valueW3 > valueW ? valueW3 : valueW;
		valueW4     = valueW4 > valueW3 ? valueW4 : valueW3;
		f32 barW    = valueW4 + 16.0f;
		f32 lineW;
		if (title->mOptionLine == 0) {
			lineW = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x11), TRUE, size);
		} else if (title->mOptionLine == 1) {
			lineW = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x14), TRUE, size);
		} else if (title->mOptionLine == 2) {
			lineW = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x17), TRUE, size);
		}

		// the pointer line
		bool selected = false;
		if (title->mOptionLine == 0 && (f32)title->mHighlightDelay >= (f32)gRefreshRate) {
			selected = true;
		}
		u32 color = selected ? lit : normal;
		DrawGsProportionalTextFormat(&gGame.mGs, color, GetLocalizedInterfaceText(0x11), cx - labelW - gap, 3.0f * size + (cy - h / 2.0f),
		                             0.0f, size, size);
		selected = false;
		if (title->mOptionLine == 0 && (f32)title->mHighlightDelay >= (f32)gRefreshRate) {
			selected = true;
		}
		f32 valueX = gap + (cx + labelW) - barW / 2.0f;
		fn_8004EDFC(title, selected, valueX, size / 2.0f + (3.0f * size + (cy - h / 2.0f)), size, barW);
		text       = title->mPointerOn ? GetLocalizedInterfaceText(0x12) : GetLocalizedInterfaceText(0x13);
		u32 shadow = 100.0f * title->mOptionAlpha;
		textWidth  = MeasureGameTextWidth(&gGame.mGs, text, TRUE, size);
		DrawGsProportionalTextFormat(&gGame.mGs, shadow, text, 2.0f + valueX - textWidth / 2.0f, 2.0f + (3.0f * size + (cy - h / 2.0f)),
		                             0.0f, size, size);
		textWidth = MeasureGameTextWidth(&gGame.mGs, text, TRUE, size);
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, valueX - textWidth / 2.0f, 3.0f * size + (cy - h / 2.0f), 0.0f, size, size);

		// the stick line
		selected = false;
		if (title->mOptionLine == 1 && (f32)title->mHighlightDelay >= (f32)gRefreshRate) {
			selected = true;
		}
		color = selected ? lit : normal;
		DrawGsProportionalTextFormat(&gGame.mGs, color, GetLocalizedInterfaceText(0x14), cx - labelW - gap, 5.0f * size + (cy - h / 2.0f),
		                             0.0f, size, size);
		selected = false;
		if (title->mOptionLine == 1 && (f32)title->mHighlightDelay >= (f32)gRefreshRate) {
			selected = true;
		}
		fn_8004EDFC(title, selected, valueX, size / 2.0f + (5.0f * size + (cy - h / 2.0f)), size, barW);
		text      = title->mStickOn ? GetLocalizedInterfaceText(0x15) : GetLocalizedInterfaceText(0x16);
		shadow    = 100.0f * title->mOptionAlpha;
		textWidth = MeasureGameTextWidth(&gGame.mGs, text, TRUE, size);
		DrawGsProportionalTextFormat(&gGame.mGs, shadow, text, 2.0f + valueX - textWidth / 2.0f, 2.0f + (5.0f * size + (cy - h / 2.0f)),
		                             0.0f, size, size);
		textWidth = MeasureGameTextWidth(&gGame.mGs, text, TRUE, size);
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, valueX - textWidth / 2.0f, 5.0f * size + (cy - h / 2.0f), 0.0f, size, size);

		// OK pulses with the cursor's animation
		f32 progress = title->mCursor[0].GetAnimId() == 1 ? title->mCursor[0].GetAnimFrame() / title->mCursor[0].GetAnimEndFrame() : 0.0f;
		f32 okSize   = 24.0f + (title->mOptionLine == 2 ? 2.0f * (f32)sin(4.712389f * progress) : 0.0f);
		selected     = false;
		if (title->mOptionLine == 2 && (f32)title->mHighlightDelay >= (f32)gRefreshRate) {
			selected = true;
		}
		color     = selected ? lit : normal;
		text      = GetLocalizedInterfaceText(0x17);
		textWidth = MeasureGameTextWidth(&gGame.mGs, text, TRUE, size);
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, cx - textWidth / 2.0f, 7.0f * size + (cy - h / 2.0f), 0.0f, size, size);
	}
}

extern "C" void UpdateTitleOptions(Title* title, u32 frames)
{
	bool showPanes        = true;
	bool stickOverPointer = false;

	if (title->mOptionLine != -1) {
		title->mLastOptionLine = title->mOptionLine;
	}
	title->mCursor[0].UpdateObject(frames);
	title->mCursor[1].UpdateObject(frames);
	title->mHighlightDelay += frames;
	if (title->mHighlightDelay >= (f32)gRefreshRate) {
		title->mHighlightDelay = (f32)gRefreshRate;
		title->mPulseAngle     = fmod(title->mPulseAngle + frames * 9, 360.0);
	}

	s32 line = title->mOptionLine;
	if (line == -1) {
		line = title->mLastOptionLine;
	}
	title->mOptionBar[0] += 0.8f * (title->mOptionBarLines[line] - title->mOptionBar[0]);
	title->mOptionBar[1] += 0.8f * (title->mOptionBarTarget[0] - title->mOptionBar[1]);
	title->mOptionBar[2] += 0.8f * (title->mOptionBarTarget[1] - title->mOptionBar[2]);

	switch (title->mOptionState) {
	case 0:
		if (title->IsStickUsedOverPointer()) {
			stickOverPointer = true;
		}
		if (!stickOverPointer) {
			if (title->IsStickUp()) {
				if (title->mOptionLine > 0) {
					title->mOptionLine--;
					PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
					title->mPulseAngle = 45.0f;
				}
			} else if (title->IsStickDown()) {
				if (title->mOptionLine < 2) {
					title->mOptionLine++;
					PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
					title->mPulseAngle = 45.0f;
				}
			}
		}
		if (title->IsConfirmPressed() && title->mOptionLine != -1) {
			showPanes = false;
			switch (title->mOptionLine) {
			case 0:
				if (title->mPointerOn) {
					title->mPointerOn = false;
					if (!title->mStickOn) {
						title->mStickOn = true;
					}
				} else {
					title->mPointerOn = true;
				}
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				break;
			case 1:
				if (title->mStickOn) {
					title->mStickOn = false;
					if (!title->mPointerOn) {
						title->mPointerOn = true;
					}
				} else {
					title->mStickOn = true;
				}
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				break;
			case 2:
				if (title->mPointerOn) {
					gGame.mMenuControls |= MenuControlPointer;
				} else {
					gGame.mMenuControls &= ~MenuControlPointer;
				}
				if (title->mStickOn) {
					gGame.mMenuControls |= MenuControlStick;
				} else {
					gGame.mMenuControls &= ~MenuControlStick;
				}
				title->mOptionState = 2;
				title->mPulseAngle  = 90.0f;
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				break;
			}
		} else if (title->IsCancelPressed()) {
			title->mOptionState = 2;
			title->mPulseAngle  = 90.0f;
			PlayWiiSoundEffect(&gGame.mSound, 243, 1.0f, 0.0f);
			showPanes         = false;
			title->mPointerOn = title->mSavedPointerOn;
			title->mStickOn   = title->mSavedStickOn;
		}
		break;
	case 1:
		OSPanic(__FILE__, 5496, "ここにきたら不正");
		break;
	case 2:
		showPanes                 = false;
		title->mPulseAngle        = 0.0f;
		title->mState             = 11;
		title->mTargetOptionAlpha = 0.0f;
		title->mOptionState       = 0;
		title->mOptionLine        = 0;
		title->mTargetPanelY      = -200.0f;
		title->mOffset            = 0.0f;
		title->mAlpha             = 0.0f;
		HideName(title);
		title->mPanes[8].mPointed  = false;
		title->mPanes[8].mPicked   = false;
		title->mPanes[9].mPointed  = false;
		title->mPanes[9].mPicked   = false;
		title->mPanes[10].mPointed = false;
		title->mPanes[10].mPicked  = false;
		title->UpdateSelections();
		break;
	}
	if (showPanes) {
		AddPointerPane(&gPointerPanes, &title->mPanes[8]);
		AddPointerPane(&gPointerPanes, &title->mPanes[9]);
		AddPointerPane(&gPointerPanes, &title->mPanes[10]);
	}
}

/**
 * @note Address: 0x8004CA74
 * @note Size: 0x344
 */
// the name is set from a C string: each letter gets a text pointer, a staggered shrink, a random
// bob and tilt, and the rest of the slots are cleared. SJIS lead bytes count once, only when the
// game language is Japanese
extern "C" void UpdateTitleDust(Title* title, const char* text, u32 color)
{
	title->mName.mActive = TRUE;
	title->mName.mCount  = 0;
	if (text == NULL) {
		return;
	}
	title->mName.mColor = color;
	for (s32 i = 0; i < 20 && *text; i++, text++) {
		title->mName.mChars[i].mText  = text;
		title->mName.mChars[i].mScale = -0.2f * i;
		title->mName.mChars[i].mWait  = gRefreshRate;
		title->mName.mChars[i].mBob   = DEG2RAD * (rand() % 360);
		title->mName.mChars[i].mTilt  = DEG2RAD * (rand() % 360);
		title->mName.mCount++;
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
	for (s32 i = title->mName.mCount; i < 20; i++) {
		title->mName.mChars[i].mText  = NULL;
		title->mName.mChars[i].mScale = 0.0f;
		title->mName.mChars[i].mWait  = 0.0f;
	}
}

/**
 * @note Address: 0x8004CDB8
 * @note Size: 0x180
 */
// the name's letters grow in while they wait and shrink away after; once none shows, the name is off
void fn_8004CDB8(Title* title, u32 frames)
{
	if (title->mName.mActive && title->mName.mActive > 0) {
		for (s32 i = 0; i < title->mName.mCount; ++i) {
			if (title->mName.mChars[i].mWait > 0) {
				if (title->mName.mChars[i].mScale < 1) {
					if ((title->mName.mChars[i].mScale += (1.f / (.3f * gRefreshRate)) * frames) > 1)
						title->mName.mChars[i].mScale = 1;
				}
			} else if (title->mName.mChars[i].mScale > 0) {
				if ((title->mName.mChars[i].mScale -= (1.f / (.3f * gRefreshRate)) * frames) < 0)
					title->mName.mChars[i].mScale = 0;
			}
		}
		s32 visible = 0;
		for (s32 i = 0; i < title->mName.mCount; ++i) {
			if (0 != title->mName.mChars[i].mScale) {
				++visible;
				title->mName.mChars[i].mBob += (5.f * DEG2RAD) * frames;
				title->mName.mChars[i].mTilt += (3.f * DEG2RAD) * frames;
			}
		}
		if (!visible)
			title->mName.mActive = 0;
	}
}

/**
 * @note Address: 0x8004CF38
 * @note Size: 0x68C
 */
// NONMATCHING: written from the GameCube's; the Wii centres the row on the pointer screen and halves by
// multiplying
// draws the title's name: a bubble per letter, then the letter with a shadow on four sides
extern "C" void DrawTitleNameBubbles(Title* title)
{
	if (title->mName.mActive) {
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		f32 centreX = gPointerScreenWidth / 2;
		(&gGame.mTpl)->mTex.Init(&gGame.mTpl.mPalette->descriptorArray[108 - StatusTextureOffset], 0);
		CTexObj bubble = gGame.mTpl.mTex;
		f32 zero       = 0.0f;
		u32 color      = title->mName.mColor & 0xFFFFFF00;
		for (s32 i = 0; i < title->mName.mCount; i++) {
			f32 scale = title->mName.mChars[i].mScale;
			if (scale <= zero) {
				continue;
			}
			CFont font;
			s32 width;
			CTexObj letter  = *GetFontGlyphTexture(&font, title->mName.mChars[i].mText, 0, &width);
			f32 bob         = 4.0f * std::sinf(title->mName.mChars[i].mBob);
			f32 tilt        = std::sinf(0.7f * title->mName.mChars[i].mTilt);
			f32 rot         = -(DEG2RAD * (10.0f * tilt));
			f32 swell       = std::sinf(PI * scale);
			f32 pop         = 0.2f * swell + std::sinf(HALF_PI * scale);
			f32 x           = 24.0f + (48.0f * i + (centreX - 48.0f * title->mName.mCount * 0.5f));
			f32 y           = 64.0f + bob;
			u32 bubbleColor = (u32)(255.0f * scale) | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &bubble, bubbleColor, bubbleColor, bubbleColor, bubbleColor, x, y, 0.0f, 20.0f * (2.0f * pop),
			                   20.0f * (2.0f * pop), zero, -zero, rot);
			f32 size   = 20.0f * (1.7f * pop);
			f32 shrink = width / 24.0f * size;
			f32 off    = size - shrink;
			u32 alpha  = 255.0f * scale;
			for (s32 j = 0; j < 4; j++) {
				DrawGsTexturedQuad(&gGame.mGs, &letter, alpha, alpha, alpha, alpha, off * 0.5f + (x + gTitleNameOutlineX[j]),
				                   bob + (64.0f + gTitleNameOutlineY[j]), 0.0f, size, size, zero, -zero, rot);
			}
			u32 letterColor = color | (u32)(255.0f * scale);
			DrawGsTexturedQuad(&gGame.mGs, &letter, letterColor, letterColor, letterColor, letterColor, off * 0.5f + x, y, 0.0f, size, size,
			                   zero, -zero, rot);
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
}

/**
 * @note Address: 0x8004D5C4
 * @note Size: 0x6D0
 */
// the help line at the bottom: one blob per glyph, centred on the pointer's screen size. SJIS lead
// bytes are only counted when the game language is Japanese (gGame.mLanguage == 0)
extern "C" void DrawTitleHelpText(Title* menu)
{
	f32 alpha   = menu->mMenuAlpha * menu->mHelpAlpha;
	f32 spacing = 30.0f;
	f32 centreX = gPointerScreenWidth / 2;
	f32 centreY = 160.0f + gPointerScreenHeight / 2;
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	(&gGame.mTpl)->mTex.Init(&gGame.mTpl.mPalette->descriptorArray[109 - StatusTextureOffset], 0);
	CTexObj blob     = gGame.mTpl.mTex;
	menu->mHelpCount = 0;
	const char* text = menu->mHelpText;
	if (text) {
		const int limit = 20;
		for (int i = 0; i < limit && *text; i++, text++) {
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
	f32 zero    = 0.0f;
	f32 opacity = 255.0f * alpha;
	for (int i = 0; i < menu->mHelpCount; i++) {
		MenuHelpChar* glyph = &menu->mBg[i];
		s32 width;
		CFont font;
		CTexObj tex  = *GetFontGlyphTexture(&font, glyph->mText, 0, &width);
		f32 bob      = 2.0f * (f32)sin(glyph->mAngle0);
		f32 rotation = -(DEG2RAD * (10.0f * (f32)sin(0.7f * glyph->mAngle1)));
		f32 swell    = sin(PI * alpha);
		f32 grow     = sin(HALF_PI * alpha);
		f32 scale    = 0.2f * swell + grow;
		f32 x        = 0.5f * spacing + (i * spacing + (centreX - menu->mHelpCount * spacing * 0.5f));
		f32 y        = centreY + bob;
		u32 color    = (u32)opacity | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &blob, color, color, color, color, x, y, 1.0f, 30.0f * scale, 30.0f * scale, 0.0f, -zero, rotation);
		scale       = 30.0f * (0.8f * scale);
		f32 padding = HelpGlyphPadding(width, scale);
		u32 oc      = (u32)opacity;
		for (int j = 0; j < 4; j++) {
			DrawGsTexturedQuad(&gGame.mGs, &tex, oc, oc, oc, oc, x + gTitleHelpOutlineX[j] + padding * 0.5f,
			                   bob + (centreY + gTitleHelpOutlineY[j]), 1.0f, scale, scale, 0.0f, -zero, rotation);
		}
		color = (u32)opacity | 0xFFFFFB00;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x + padding * 0.5f, y, 1.0f, scale, scale, 0.0f, -zero, rotation);
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

/**
 * @note Address: 0x8004DC94
 * @note Size: 0x174
 */
// the fade is stepped before the growth: retail's .sdata2 has 0.009 (0x805A2EFC) ahead of 0.018
// one dust mote's drift cycle: a random delay, then it grows until it pops and respawns
void fn_8004DC94(TitleDust* dust, u32 frames)
{
	switch (dust->mState) {
	case 0:
		dust->mTimer = rand() % 120;
		++dust->mState;
		break;
	case 1:
		if (--dust->mTimer < 0) {
			++dust->mState;
			dust->mSpeedY = 1.0f;
		}
		break;
	case 2:
		dust->mSpeedY -= 0.009f * frames;
		dust->mSpeedX += 0.018f * frames;
		if (dust->mSpeedX >= 2.0f) {
			dust->mX      = rand() % gRingScreenWidth;
			dust->mY      = rand() % gRingScreenHeight;
			dust->mState  = 0;
			dust->mSpeedX = dust->mSpeedY = 0.0f;
		}
		break;
	}
}

/**
 * @note Address: 0x8004DE08
 * @note Size: 0x64
 */
// XObjDemo's ResetObjectState
void XObjDemo::ResetObjectState()
{
	XObj::ResetObjectState();
	mState      = 0;
	mLoop       = FALSE;
	mCameraAnim = -1;
	mLightCount = 0;
	for (s32 i = 0; i < 8; ++i) {
		mLightAnims[i] = -1;
	}
}

/**
 * @note Address: 0x8004DE6C
 * @note Size: 0x2C
 */
// XObjDemo's LoadFromPlacement; the Wii builds no camera or lights from the archive
BOOL XObjDemo::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8004DE98
 * @note Size: 0x4
 */
// XObjDemo's Release
void XObjDemo::Release()
{
	XObj::Release();
}

/**
 * @note Address: 0x8004DE9C
 * @note Size: 0x78
 */
// NONMATCHING: retail loads mState a second time for the compare with 2 (0x8004DEA8); every other
// instruction is retail's
// XObjDemo's update: state 2 waits for the screen fade to finish, then goes quiet
void XObjDemo::UpdateObject(u32 frames)
{
	if (mState == 0) {
		return;
	}
	if (mState == 2 && FadeDone()) {
		mState = 0;
		return;
	}
	XObj::UpdateObject(frames);
}

/**
 * @note Address: 0x8004DF14
 * @note Size: 0x24
 */
// XObjDemo's DrawObject, only while the demo runs
void XObjDemo::DrawObject()
{
	if (mState != 0) {
		DrawWithView(NULL);
	}
}

/**
 * @note Address: 0x8004DF38
 * @note Size: 0x4
 */
// retail's body is empty
void Title::vf08(CbPane* pane)
{
}

/**
 * @note Address: 0x8004DF3C
 * @note Size: 0x104
 */
// a pane event clears whichever tracked index pointed at it
void Title::vf0C(CbPane* pane)
{
	s32 kind = pane->mKind;
	switch (kind) {
	case 0:
	case 1:
	case 2:
		if (mFile == kind && mFile != -1) {
			mFile = -1;
		}
		break;
	case 3:
	case 4:
		if (mMode == 3 || mMode == 4 || mMode == 5 || mMode == 26 || mMode == 27 || mMode == 6 || mMode == 11) {
			if (mChoice == kind - 3 && mChoice != -1) {
				mChoice = -1;
			}
		}
		break;
	case 5:
	case 6:
	case 7:
		if (mSelection == kind - 5 && mSelection != -1) {
			mSelection = -1;
		}
		break;
	}
	if (mState == 21) {
		switch (pane->mKind) {
		case 8:
		case 9:
		case 10:
			if (mOptionLine != -1) {
				mOptionLine = -1;
			}
			break;
		}
	}
}

/**
 * @note Address: 0x8004E040
 * @note Size: 0x334
 */
// the pointer moved onto a pane: whichever index it stands for is picked, with the cursor sound
void Title::vf10(CbPane* pane)
{
	switch (pane->mKind) {
	case 0:
		if (mFile != 0) {
			mFile = 0;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	case 1:
		if (mFile != 1) {
			mFile = 1;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	case 2:
		if (mFile != 2) {
			mFile = 2;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	case 3:
	case 4:
		break;
	case 5:
		if (mSelection != 0) {
			mSelection  = 0;
			mPulseAngle = 45.0f;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	case 6:
		if (mSelection != 1) {
			mSelection = 1;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	case 7:
		if (mSelection != 2) {
			mSelection = 2;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	}
	if (mState == 21) {
		switch (pane->mKind) {
		case 8:
			if (mOptionLine != 0) {
				mOptionLine = 0;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				mPulseAngle = 45.0f;
			}
			break;
		case 9:
			if (mOptionLine != 1) {
				mOptionLine = 1;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				mPulseAngle = 45.0f;
			}
			break;
		case 10:
			if (mOptionLine != 2) {
				mOptionLine = 2;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				mPulseAngle = 45.0f;
			}
			break;
		}
	}
	if (mMode == 3 || mMode == 4 || mMode == 5 || mMode == 26 || mMode == 27 || mMode == 6 || mMode == 11) {
		switch (pane->mKind) {
		case 3:
			if (mChoice != 0) {
				mChoice = 0;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
			}
			break;
		case 4:
			if (mChoice != 1) {
				mChoice = 1;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
			}
			break;
		}
	}
}

/**
 * @note Address: 0x8004E374
 * @note Size: 0x4
 */
void Title::vf18(nw4r::math::VEC2* pos)
{
	ClearSelections();
}

/**
 * @note Address: 0x8004E378
 * @note Size: 0x4
 */
void Title::vf1C(nw4r::math::VEC2* pos)
{
	RestoreSelections();
}

/**
 * @note Address: 0x8004E37C
 * @note Size: 0xB4
 */
// with the pointer and the stick both on, the pointer took over: nothing stays picked
void Title::ClearSelections()
{
	u32 controls = gGame.mMenuControls;
	if (!(controls & MenuControlStick) || !(controls & MenuControlPointer)) {
		return;
	}
	if (mState == 11) {
		mSelection = -1;
	}
	if (mState == 21) {
		mOptionLine = -1;
	}
	if (mState == 20 && mMode == 2) {
		mFile = -1;
	}
	if (mMode == 3 || mMode == 4 || mMode == 5 || mMode == 26 || mMode == 27 || mMode == 6 || mMode == 11) {
		mChoice = -1;
	}
}

/**
 * @note Address: 0x8004E430
 * @note Size: 0x104
 */
// and the stick took over again: whatever was picked last comes back
void Title::RestoreSelections()
{
	u32 controls = gGame.mMenuControls;
	if (!(controls & MenuControlStick) || !(controls & MenuControlPointer)) {
		return;
	}
	if (mState == 11 && mSelection == -1 && mLastSelection != -1) {
		mSelection = mLastSelection;
	}
	if (mState == 21 && mOptionLine == -1 && mLastOptionLine != -1) {
		mOptionLine = mLastOptionLine;
	}
	if (mState == 20 && mMode == 2 && mFile == -1 && mLastFile != -1) {
		mFile = mLastFile;
	}
	if ((mMode == 3 || mMode == 4 || mMode == 5 || mMode == 26 || mMode == 27 || mMode == 6 || mMode == 11) && mChoice == -1
	    && mLastChoice != -1) {
		mChoice = mLastChoice;
	}
}

/**
 * @note Address: 0x8004E534
 * @note Size: 0x1B8
 */
void Title::UpdateSelections()
{
	if (mState == 11) {
		if (IsPointerActive()) {
			mSelection = -1;
		} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
			mSelection = -1;
		} else {
			mSelection = mLastSelection;
		}
	}
	if (mState == 21) {
		mLastOptionLine = 0;
		if (IsPointerActive()) {
			mOptionLine = -1;
		} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
			mOptionLine = -1;
		} else {
			mOptionLine = mLastOptionLine;
		}
	}
	if (mMode == 2) {
		if (IsPointerActive()) {
			mFile = -1;
		} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
			mFile = -1;
		} else {
			mFile = mLastFile;
		}
	}
	if (mMode == 3 || mMode == 5 || mMode == 26 || mMode == 27) {
		mLastChoice = 1;
		if (IsPointerActive()) {
			mChoice = -1;
		} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
			mChoice = -1;
		} else {
			mChoice = mLastChoice;
		}
	}
}

/**
 * @note Address: 0x8004E6EC
 * @note Size: 0x710
 */
// NONMATCHING: read from retail's code; retail keeps the arrow's 1.2 scale in a register for both sides
// the menus' highlight: a round knob at the bar's start, a line along it and an arrow at its end. retail
// still lays out a 64-segment ring for the knob and draws only the line's two points from it
void fn_8004E6EC(Title* title, BOOL options, f32 x, f32 y, f32 alpha, f32 extent)
{
	if (alpha > 0.0f) {
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		CTexObj tex;
		u32 color;
		if (options == FALSE) {
			color = 255.0f * alpha;
		} else {
			color = (u32)(255.0f * alpha) | 0xFFFFFF00;
		}
		f32 size;
		if (options == FALSE) {
			size = 28.0f;
		} else {
			size = 38.4f;
		}
		f32 radius  = 2.0f * (size / 2);
		f32 centreY = y - radius * 0.5f;
		Vec ring[65];
		for (s32 i = 0; i < 65; i++) {
			f32 s     = sin(0.09817477f * i - HALF_PI);
			f32 c     = cos(0.09817477f * i - HALF_PI);
			ring[i].x = s * radius + x;
			ring[i].y = -c * radius + centreY;
			ring[i].z = 0.0f;
		}
		if (!options) {
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		} else {
			gGame.mGs.mCurTevStage->SetMode(GX_BLEND);
		}
		(&gGame.mTpl)->mTex.Init(&gGame.mTpl.mPalette->descriptorArray[265 - StatusTextureOffset], 0);
		tex = gGame.mTpl.mTex;
		DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00,
		                   (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00, x, 1.5f + ((centreY + radius) - 19.2f),
		                   0.0f, 38.4f, 38.4f, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		(&gGame.mTpl)->mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126 - StatusTextureOffset], 0);
		tex           = gGame.mTpl.mTex;
		f32 lineWidth = GXGetTexObjWidth(&tex);
		f32 arrowX    = x + (extent + gTitleArrowGap[0]);
		ring[0].x     = x;
		ring[0].y     = centreY + radius;
		ring[1].x     = arrowX - 10.0f;
		ring[1].y     = centreY + radius;
		gGame.mGs.mCurRenderState->SetLineWidth(18, GX_TO_ONE);
		DrawGsLineStrip(&gGame.mGs, ring, color, 2);
		gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
		(&gGame.mTpl)->mTex.Init(&gGame.mTpl.mPalette->descriptorArray[156 - StatusTextureOffset], 0);
		tex = gGame.mTpl.mTex;
		if (!options) {
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		} else {
			gGame.mGs.mCurTevStage->SetMode(GX_BLEND);
		}
		f32 w = 1.2f * GXGetTexObjWidth(&tex);
		f32 h = 1.2f * GXGetTexObjHeight(&tex);
		DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
		                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, arrowX, ring[1].y, 0.0f, w, h, 0.0f,
		                   0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
}

/**
 * @note Address: 0x8004EDFC
 * @note Size: 0x480
 */
// NONMATCHING: read from retail's code; only the order of the copies of x and width at the start differs
// the highlight bar behind an options line: a stretched middle and two caps from one texture, then the
// same bar smaller in a dark shade, which retail draws without its left cap
void fn_8004EDFC(Title* title, BOOL selected, f32 x, f32 y, f32 size, f32 width)
{
	y -= 2.0f;
	f32 scale = 1.0f;
	size *= scale;
	u32 color;
	if (selected && (f32)title->mHighlightDelay >= (f32)gRefreshRate) {
		color = (u32)(255.0f * title->mOptionAlpha) | 0xFFB62800;
	} else {
		color = (u32)(255.0f * title->mOptionAlpha) | 0xFFFFFF00;
	}
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	(&gGame.mTpl)->mTex.Init(&gGame.mTpl.mPalette->descriptorArray[118 - StatusTextureOffset], 0);
	CTexObj tex = gGame.mTpl.mTex;
	PSMTXScale(tex.mTexMtx.mMtx[2], 0.25f, 1.0f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.25f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, 2.0f * scale + y, 0.0f, width, size, 0.0f, 0.0f, 0.0f);
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, (x - width / 2) - 4.0f * scale, 2.0f * scale + y, 0.0f, 8.0f * scale,
	                   size, 0.0f, 0.0f, 0.0f);
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.5f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 4.0f * scale + (width / 2 + x), 2.0f * scale + y, 0.0f, 8.0f * scale,
	                   size, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);

	scale = 0.8f;
	size *= scale;
	u32 shade = 200.0f * title->mOptionAlpha;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	PSMTXScale(tex.mTexMtx.mMtx[2], 0.25f, 1.0f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.25f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, shade, shade, shade, shade, x, 2.0f * scale + y, 0.0f, width, size, 0.0f, 0.0f, 0.0f);
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0.5f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, shade, shade, shade, shade, 4.0f * scale + (width / 2 + x), 2.0f * scale + y, 0.0f, 8.0f * scale,
	                   size, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// the handlers UpdateTitleScreen's state 20 dispatches by mMode
void (Title::*gTitleModeUpdates[])(u32) = {
	NULL,
	&Title::State0,
	&Title::State1,
	&Title::State2,
	&Title::State3,
	&Title::State4,
	&Title::State5,
	&Title::State6,
	&Title::State7,
	&Title::State8,
	&Title::State9,
	&Title::State10,
	&Title::State11,
	&Title::State12,
	&Title::State13,
	&Title::State14,
	&Title::State15,
	&Title::State16,
	&Title::State17,
	&Title::State18,
	&Title::State19,
	&Title::State20,
	&Title::State21,
	&Title::State22,
	&Title::State23,
	&Title::State24,
	&Title::State25,
	&Title::State26,
	&Title::State27,
};

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
// The functions come in the reverse of the GameCube's order (gxb.cpp from XObjDemo::DrawObject to
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
#include "game/wii/title.h"
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
void DrawTitleNameBubbles(Title* title);
void fn_802AEDEC(WiiGsSceneObj** head, Mtx view);
CCamera* InitGameOrthoCamera(CCamera* camera, GXRenderModeObj* mode);
void OSFatal(GXColor fg, GXColor bg, const char* msg);
void ChangeGameStage(CGame* game, s32 stage, BOOL showName);
void ApproachHudValue(f32* value, f32* target, f32 step);
s32 IndexOfLargestOfThree(OSTime first, OSTime second, OSTime third);
// one dust mote's per-frame update, the body of the GameCube UpdateTitleDust's loop
void fn_8004DC94(TitleDust* dust, u32 frames);
bool IsSaveSlotIntact(const WiiMenuSaveSlot* slot);
// fills a file menu record from a scanned save header (menu.cpp, 0x8007A2E8)
void fn_8007A2E8(WiiMenuSaveSlot* slot, WiiSaveScanEntry* entry, BOOL used);
// the pane-event slots in Title's vtable
void fn_8004DF38();
void fn_8004DF3C(Title* title, CbPane* pane);
void fn_8004E040(Title* title, CbPane* pane);
void fn_8004E37C(Title* title);
void fn_8004E430(Title* title);
}

// the pending fatal the save system left for the title screen: the message, two colors and the flag
// that raises it while a menu state still owns the frame
struct WiiFatalInfo {
	const char* mMessage; // _00
	GXColor mFg;          // _04
	u8 _08[4];
	GXColor mBg; // _0C
	u8 _10[4];
	u8 mActive; // _14
};
extern WiiFatalInfo gSaveFatal;

// the title screen's background and text colors, the logo's textures and which of each is in use
struct TitlePaletteEntry {
	GXColor background;
	GXColor text;
};
const TitlePaletteEntry gTitlePalette[] = {
	{ { 0, 0, 128, 0 }, {} },     { { 128, 0, 0, 0 }, {} },     { { 0, 128, 0, 0 }, {} },
	{ { 200, 200, 200, 0 }, {} }, { { 128, 128, 128, 0 }, {} }, { { 0, 207, 233, 0 }, {} },
	{ { 255, 220, 133, 0 }, {} }, { { 255, 220, 0, 0 }, {} },   { {}, { 255, 255, 255, 0 } },
};
const s32 gTitleLogoTextures[3] = { 0, 0, 0 };
s32 gTitlePaletteIndex          = 3;
s32 gTitleLogoIndex             = 1;

// the outline offsets of the help text's letter shadows
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
		if (mTracks[i].IsAttachedSound()) {
			mTracks[i].Stop(frames);
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
static inline f32 DrawTitleEntry(Title* title, s32 entry, s32 textId, f32 y, f32 progress, u32 normal, u32 lit)
{
	const char* text = GetTitleAndSaveMenuText(textId);
	f32 size         = 24.0f + (title->mSelection == entry ? 2.0f * (f32)sin(4.712389f * progress) : 0.0f);
	f32 width        = MeasureGameTextWidth(&gGame.mGs, text, TRUE, 24.0f);
	bool selected    = false;
	if (title->mSelection == entry && (f32)title->mHighlightDelay >= (f32)gRefreshRate) {
		selected = true;
	}
	if (entry != 1 || title->mSaveFound) {
		DrawGsProportionalTextFormat(&gGame.mGs, selected ? lit : normal, text, (f32)(gPointerScreenWidth / 2) - width * 0.5f,
		                             (size - 24.0f) * 0.5f + (y - size), 0.0f, 24.0f, size);
	}
	return width;
}

// the shared ease for the panel offset pair, the GameCube's ApproachTitleOffset; the Wii takes the absolute value with fabsf, which its MSL
// inlines to __fabsf
static inline void ApproachTitleOffset(f32& value, f32& target)
{
	f32 speed = __fabsf((target - value) * .25f);
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
	title->mLastMode          = 0;
	title->mModeParam         = 0;
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
// and the mode dispatch follow the GameCube flow; the Wii input calls are the MenuUtil methods the
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
			if (title->mCursor[0].mRequest.mFile->mSize == 0x3CA80) {
				title->mCursor[0].SetScale(4.0f);
			}
			++title->mState;
		}
		break;
	case 4:
		if (title->mCursor[1].LoadFromPlacement(CoordInfo(GetSpawnData(title->mSpawnIndex)))) {
			if (title->mCursor[1].mRequest.mFile->mSize == 0x3CA80) {
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
			stamps[i]               = 0;
			if (entry->mStatus & 1) {
				stamps[i] = (entry->mStatus & 2) ? 0 : ((WiiSaveSlotHeader*)entry->mData)->mTimestamp;
			}
		}
		title->mFile = IndexOfLargestOfThree(stamps[0], stamps[1], stamps[2]);
		// NONMATCHING: retail then walks the newest slot's saved mMenuControls back into
		// mPointerOn/mStickOn and primes the file rows; that tail is not fully read yet
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
		UpdateTitleDust(title, frames);
		if (title->mMenuAlpha == 1 && title->mTitleAlpha == 1)
			title->mState = 11;
		break;
	}
	case 11: {
		title->mHighlightDelay += frames;
		if (title->mHighlightDelay >= gRefreshRate) {
			title->mHighlightDelay = gRefreshRate;
			title->mPulseAngle     = (f32)fmod(title->mPulseAngle + (f32)(frames * 9), 360.);
		}
		title->mCursor[0].UpdateObject(frames);
		title->mCursor[1].UpdateObject(frames);
		UpdateTitleDust(title, frames);
		title->UpdateSelections();
		if (title->IsConfirmPressed()) {
			switch (title->mSelection) {
			case 0:
			case 1:
				title->mState = 12;
				break;
			case 2:
				title->mState = 12;
				break;
			}
		}
		break;
	}
	case 12: {
		title->mCursor[0].UpdateObject(frames);
		title->mCursor[1].UpdateObject(frames);
		UpdateTitleDust(title, frames);
		if (!title->mCursor[0].IsAnimDone() || title->mCursor[0].GetAnimId() == 0
		    || title->mCursor[0].mObject.m_resFile.GetResAnmChrNumEntries() < 2) {
			title->mPulseAngle = 0;
			switch (title->mSelection) {
			case 0:
			case 1:
				title->mTargetPanelY = 0;
				title->mAlpha        = 1;
				title->mState        = 20;
				break;
			case 2:
				title->mTargetPanelY      = 0;
				title->mTargetOptionAlpha = 1;
				title->mState             = 21;
				title->mOptionState       = 0;
				title->mOptionLine        = 0;
				break;
			}
		}
		break;
	}
	case 20:
		title->mCursor[1].UpdateObject(frames);
		UpdateTitleDust(title, frames);
		(title->*gTitleModeUpdates[title->mMode])(frames);
		break;
	case 21:
		title->mCursor[1].UpdateObject(frames);
		UpdateTitleDust(title, frames);
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
					((InfoScrn*)gGame.mInfoScrn)->mSelectedSaveSlot = mFile;
					return;
				}
				if (gTitleSaveSlots[mFile].mStage == 14) {
					HideName(this);
					StopWiiSfx(&gGame.mSound, 0xFF0003, 0.0f);
					mAlpha                                          = 0.0f;
					mTargetPanelY                                   = -200.0f;
					((InfoScrn*)gGame.mInfoScrn)->mSelectedSaveSlot = mFile;
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
			((InfoScrn*)gGame.mInfoScrn)->mSelectedSaveSlot = mFile;
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
			((InfoScrn*)gGame.mInfoScrn)->mSelectedSaveSlot = mFile;
			gGame.mResetRequested                           = true;
			gGame.mResetType                                = 5;
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
		mTargetPanelY                                   = -200.0f;
		mAlpha                                          = 0.0f;
		((InfoScrn*)gGame.mInfoScrn)->mSelectedSaveSlot = mFile;
		ResumeScrapCounterPulse((InfoScrn*)gGame.mInfoScrn);
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
	switch (mModeParam) {
	case 0:
		mDelay -= frames;
		if (mDelay < 0) {
			mDelay = 0;
			++mModeParam;
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
		((InfoScrn*)gGame.mInfoScrn)->mSelectedSaveSlot = mFile;
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
// NONMATCHING: the pass structure, the overlay, the dust, the logo, the figures, the footer and the
// save-fatal check are as retail. What follows the option block in retail -- the cursor halves the
// demo object draws at interpolated positions, the option bar snap behind _5174 and the three
// options-page lines -- is still being read; the entries and the guards match the GameCube shape.
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
			u16 w         = GXGetTexObjWidth(&logo);
			u16 h         = GXGetTexObjHeight(&logo);
			u32 color     = 0xffffff00 | (u32)(255.0f * title->mMenuAlpha);
			DrawGsTexturedQuad(&gGame.mGs, &logo, color, color, color, color, (f32)(gPointerScreenWidth / 2),
			                   (f32)(gPointerScreenHeight / 2 - 0x68) - 6.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
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
		fn_802AEDEC(&gGame.mGs.mSceneHead, camera->mViewMtx);
		WiiGsSceneObj* obj = gGame.mGs.mSceneHead;
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
	DrawPanelBackground<SavePanel>(title->mLoadAlpha + title->mOptionAlpha);
	DrawTitleHelpText(title);
	DrawTitleLoadScreen(title);
	DrawTitleOptions(title);
	DrawTitleNameBubbles(title);
	if ((title->mMode == 0x13 || title->mMode == 0xE) && gSaveFatal.mActive && gSaveFatal.mMessage != NULL) {
		OSFatal(gSaveFatal.mFg, gSaveFatal.mBg, gSaveFatal.mMessage);
	}
	InitGameOrthoCamera(camera, &gGame.mGs.mScreen);
	((CBase*)&gGame)->vf5C();
	gRingScreenWidth    = w;
	gPointerScreenWidth = w;
	if ((u32)(title->mState - 10) <= 2) {
		f32 y0  = (f32)(gScreenSafeBottom - 0x36) - 40.0f - 30.0f;
		u32 rgb = ((u32)gTitlePalette[gTitlePaletteIndex].text.r << 24) | ((u32)gTitlePalette[gTitlePaletteIndex].text.g << 16)
		        | ((u32)gTitlePalette[gTitlePaletteIndex].text.b << 8);
		u32 normal   = rgb | (u8)(255.0f * title->mMenuAlpha);
		u32 lit      = normal;
		f32 progress = title->mCursor[0].GetAnimId() == 1 ? title->mCursor[0].GetAnimFrame() / title->mCursor[0].GetAnimEndFrame() : 0.0f;
		f32 w0       = DrawTitleEntry(title, 0, 0, y0, progress, normal, lit);
		f32 w1       = DrawTitleEntry(title, 1, 1, y0 + 38.0f, progress, normal, lit);
		f32 w2       = DrawTitleEntry(title, 2, 2, y0 + 2.0f * 38.0f, progress, normal, lit);
		f32 y        = y0 + 2.0f * 38.0f;
		f32 width    = w2;
		switch (title->mSelection) {
		case 0:
			y     = y0;
			width = w0;
			break;
		case 1:
			y     = y0 + 38.0f;
			width = w1;
			break;
		case 2:
			break;
		}
		// retail draws the cursor halves here through the demo object at the option-line's y
	}
	// state 0x15's options-page lines and the cursor draws end the pass in retail
	((CBase*)&gGame)->vf54();
}

// the options page: the pointer and the stick can each be turned off, but not both; OK (or A on the
// third line) writes the two to gGame.mMenuControls, B leaves them as they were
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
		f32 rotation = sin(0.7f * glyph->mAngle1);
		f32 swell    = sin(PI * alpha);
		f32 grow     = sin(HALF_PI * alpha);
		f32 scale    = 0.2f * swell + grow;
		f32 x        = 0.5f * spacing + (i * spacing + (centreX - menu->mHelpCount * spacing * 0.5f));
		f32 y        = centreY + bob;
		rotation     = -(DEG2RAD * (10.0f * rotation));
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
			dust->mState  = 2;
			dust->mSpeedY = 1.0f;
		}
		break;
	case 2:
		dust->mSpeedX += 0.018f * frames;
		dust->mSpeedY -= 0.009f * frames;
		if (dust->mSpeedX >= 2.0f) {
			dust->mX      = rand() % gRingScreenWidth;
			dust->mY      = rand() % gRingScreenHeight;
			dust->mState  = 0;
			dust->mSpeedX = 0.0f;
			dust->mSpeedY = 0.0f;
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
// one of the pane-event slots in Title's vtable; retail's body is empty
void fn_8004DF38()
{
}

/**
 * @note Address: 0x8004DF3C
 * @note Size: 0x104
 */
// a pane event clears whichever tracked index pointed at it; the slot sits in Title's vtable
void fn_8004DF3C(Title* title, CbPane* pane)
{
	switch (pane->mKind) {
	case 0:
	case 1:
	case 2:
		if (title->mFile == pane->mKind && title->mFile != -1) {
			title->mFile = -1;
		}
		break;
	case 3:
	case 4:
		if (title->mMode == 3 || title->mMode == 4 || title->mMode == 5 || title->mMode == 26 || title->mMode == 27 || title->mMode == 6
		    || title->mMode == 11) {
			if (title->mChoice == pane->mKind - 3 && title->mChoice != -1) {
				title->mChoice = -1;
			}
		}
		break;
	case 5:
	case 6:
	case 7:
		if (title->mSelection == pane->mKind - 5 && title->mSelection != -1) {
			title->mSelection = -1;
		}
		break;
	}
	if (title->mState == 21 && pane->mKind >= 8 && pane->mKind < 11 && title->mOptionLine != -1) {
		title->mOptionLine = -1;
	}
}

/**
 * @note Address: 0x8004E040
 * @note Size: 0x334
 */
// the pointer moved onto a pane: whichever index it stands for is picked, with the cursor sound; the
// slot sits in Title's vtable
void fn_8004E040(Title* title, CbPane* pane)
{
	switch (pane->mKind) {
	case 0:
		if (title->mFile != 0) {
			title->mFile = 0;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	case 1:
		if (title->mFile != 1) {
			title->mFile = 1;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	case 2:
		if (title->mFile != 2) {
			title->mFile = 2;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	case 3:
	case 4:
		break;
	case 5:
		if (title->mSelection != 0) {
			title->mSelection  = 0;
			title->mPulseAngle = 45.0f;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	case 6:
		if (title->mSelection != 1) {
			title->mSelection = 1;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	case 7:
		if (title->mSelection != 2) {
			title->mSelection = 2;
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
		break;
	}
	if (title->mState == 21) {
		switch (pane->mKind) {
		case 8:
			if (title->mOptionLine != 0) {
				title->mOptionLine = 0;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				title->mPulseAngle = 45.0f;
			}
			break;
		case 9:
			if (title->mOptionLine != 1) {
				title->mOptionLine = 1;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				title->mPulseAngle = 45.0f;
			}
			break;
		case 10:
			if (title->mOptionLine != 2) {
				title->mOptionLine = 2;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
				title->mPulseAngle = 45.0f;
			}
			break;
		}
	}
	if (title->mMode == 3 || title->mMode == 4 || title->mMode == 5 || title->mMode == 26 || title->mMode == 27 || title->mMode == 6
	    || title->mMode == 11) {
		switch (pane->mKind) {
		case 3:
			if (title->mChoice != 0) {
				title->mChoice = 0;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
			}
			break;
		case 4:
			if (title->mChoice != 1) {
				title->mChoice = 1;
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
			}
			break;
		}
	}
}

/**
 * @note Address: 0x8004E37C
 * @note Size: 0xB4
 */
// with the pointer and the stick both on, the pointer took over: nothing stays picked. Title's vtable
// reaches it through the branch at 0x8004E374
void fn_8004E37C(Title* title)
{
	u32 controls = gGame.mMenuControls;
	if (!(controls & MenuControlStick) || !(controls & MenuControlPointer)) {
		return;
	}
	if (title->mState == 11) {
		title->mSelection = -1;
	}
	if (title->mState == 21) {
		title->mOptionLine = -1;
	}
	if (title->mState == 20 && title->mMode == 2) {
		title->mFile = -1;
	}
	if (title->mMode == 3 || title->mMode == 4 || title->mMode == 5 || title->mMode == 26 || title->mMode == 27 || title->mMode == 6
	    || title->mMode == 11) {
		title->mChoice = -1;
	}
}

/**
 * @note Address: 0x8004E430
 * @note Size: 0x104
 */
// and the stick took over again: whatever was picked last comes back. Title's vtable reaches it through
// the branch at 0x8004E378
void fn_8004E430(Title* title)
{
	u32 controls = gGame.mMenuControls;
	if (!(controls & MenuControlStick) || !(controls & MenuControlPointer)) {
		return;
	}
	if (title->mState == 11 && title->mSelection == -1 && title->mLastSelection != -1) {
		title->mSelection = title->mLastSelection;
	}
	if (title->mState == 21 && title->mOptionLine == -1 && title->mLastOptionLine != -1) {
		title->mOptionLine = title->mLastOptionLine;
	}
	if (title->mState == 20 && title->mMode == 2 && title->mFile == -1 && title->mLastFile != -1) {
		title->mFile = title->mLastFile;
	}
	if ((title->mMode == 3 || title->mMode == 4 || title->mMode == 5 || title->mMode == 26 || title->mMode == 27 || title->mMode == 6
	     || title->mMode == 11)
	    && title->mChoice == -1 && title->mLastChoice != -1) {
		title->mChoice = title->mLastChoice;
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

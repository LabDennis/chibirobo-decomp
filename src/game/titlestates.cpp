// The title screen and XObjDemo, one retail file (US .text 0x80027A98..0x8003135C):
// CXObj::SetScale*, XObjDemo, the title draw/options/state routines, ResetTitleScreenState,
// InitTitleScreen and __sinit_titlestates_cpp.
//
// NONMATCHING: retail's .rodata opens with the 0x48-byte six-Vec header block (US 0x801FF360) and
// its .bss opens with the 0xC-byte zero Vec slot (US 0x80361E80) that CXObj::SetScale reads; the
// functions below are in natural Wii/source order compiled with -inline auto,deferred.

#include "game/soundtracks.h"
#include "game/vistate.h"
#include "game/cmdline.h"
#include "game/stagedata.h"
#include "game/drawhelptext.h"
#include "types.h"
#include "Version.h"
#include "game/hsd_jobj.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/cjobjset.h"
#include "game/cgame.h"
#include "game/cgameutil.h"
#include "game/cgs.h"
#include "game/cscrn.h"
#include "game/csound.h"
#include "game/globals.h"
#include "game/gmath.h"
#include "game/memcard.h"
#include "game/menusave.h"
#include "game/menuselectionbar.h"
#include "game/menupanelbackground.h"
#include "game/title.h"
#include "game/video.h"
#include "game/xobjdemo.h"
#include "game/xobjs.h"
#include "game/xchr.h"
#include "Dolphin/OS/OSUtil.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
char* GetTitleAndSaveMenuText(s32 id);
void SetGameSoundStereo(XSound* sound, BOOL mono);
void HSD_PadRumbleOn(u8 no);
void HSD_PadRumbleOffN(u8 no);
void HSD_PadRumbleRemove(u8 no);
void DrawTexturedWindowFrame(u32 color0, u32 color1, f32 x, f32 y, f32 w, f32 h, f32 sx, f32 sy, f32 sz);
void ResumeScrapCounterPulse(void* info);
s32 IndexOfLargestOfThree(OSTime first, OSTime second, OSTime third);
void ApproachHudValue(f32*, f32*, f32);
void StartScreenFade(CScrnColor* color, u8 r0, u8 g0, u8 b0, u8 a0, u8 r1, u8 g1, u8 b1, u8 a1, f32 time);
void DrawMenuBannerPanels(const char*, f32, f32, f32, f32);
void DrawSaveSlotDetails(MenuSaveSlot*, u8, s16, u8, f32, f32, f32, f32, f32, f32, f32);
#if defined(VERSION_JP)
s32 DrawJapaneseSaveStatusMessage(s32, s32, f32, f32, f32, f32);
s32 DrawJapaneseCardErrorMessage(s32, s32, u8, f32, f32, f32, f32);
#else
s32 DrawSaveStatusMessage(s32, s32, f32, f32, f32, f32);
s32 DrawCardErrorMessage(s32, s32, u8, f32, f32, f32, f32);
#endif

void ResetTitleScreenState(Title* title);
void UpdateTitleOptions(Title* title, u32 frames);
void DrawTitleHelpText(Title* menu);
void DrawTitleNameBubbles(Title* title);
void DrawTitleLoadScreen(Title* title);
void DrawTitleOptions(Title* title);

s32 gTitleStateData[26] = { 0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 3, 0 };

#if !defined(VERSION_JP)
const char* gTitleTextureSuffixes[5] = { "_en", "_ge", "_fr", "_sp", "_it" };
#endif
}

TitleModeUpdate gTitleModeUpdates[26] = {
	&Title::State0,  &Title::State1,  &Title::State2,  &Title::State3,  &Title::State4,  &Title::State5,  &Title::State6,
	&Title::State7,  &Title::State8,  &Title::State9,  &Title::State10, &Title::State11, &Title::State12, &Title::State13,
	&Title::State14, &Title::State15, &Title::State16, &Title::State17, &Title::State18, &Title::State19, &Title::State20,
	&Title::State21, &Title::State22, &Title::State23, &Title::State24, &Title::State25,
};

extern "C" {
f32 gTitleNameOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gTitleNameOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
f32 gTitleHelpOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gTitleHelpOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
}

// the title screen's background and text colors, the logo's textures and which of each is in use
const TitlePaletteEntry gTitlePalette[] = {
	{ { 0, 0, 128, 0 }, {} },     { { 128, 0, 0, 0 }, {} },     { { 0, 128, 0, 0 }, {} },
	{ { 200, 200, 200, 0 }, {} }, { { 128, 128, 128, 0 }, {} }, { { 0, 207, 233, 0 }, {} },
	{ { 255, 220, 133, 0 }, {} }, { { 255, 220, 0, 0 }, {} },   { {}, { 255, 255, 255, 0 } },
};
#if defined(VERSION_JP)
const s32 gTitleLogoTextures[3] = { 0, 3, 4 };
#else
const s32 gTitleLogoTextures[3] = {};
#endif
s32 gTitlePaletteIndex = 3;
s32 gTitleLogoIndex    = 1;

#if !defined(VERSION_JP)
// each save slot is this much wider than 154, and the three shift left by half of three times it
static f32 sSaveSlotExtraWidth = 10.0f;
#endif

MenuSaveSlot gTitleSaveSlots[3];

static inline bool FadeDone()
{
	return 0.0f == gGame.mFade.mSpeed.r && 0.0f == gGame.mFade.mSpeed.g && 0.0f == gGame.mFade.mSpeed.b && 0.0f == gGame.mFade.mSpeed.a;
}

// the name's letters shrink away
static inline void HideName(Title* title)
{
	for (u32 i = 0; i < 20; i++) {
		title->mName.mChars[i].mWait = 0.0f;
	}
}

// the card was taken out: back to the card check with the save files cleared
static inline void OnCardRemoved(Title* title)
{
	title->mResult = -3;
	title->SetMode(9);
	title->mOffset = -32.0f;
	for (s32 i = 0; i < 3; i++) {
		ResetSaveSlotRecord(&gTitleSaveSlots[i]);
	}
}

// the card a new file was made on went away: back to the start
static inline void OnFormattedCardGone(Title* title)
{
	title->SetMode(25);
	title->mState = 11;
	PlayGameSound(&gGame.mSound, 0x88, 0xFF);
	title->mTargetPanelY = -200.0f;
	title->mOffset       = 0.0f;
	title->mAlpha        = 0.0f;
	HideName(title);
	title->mSelection     = 0;
	title->mSaveFound     = 0;
	title->mCardScanState = 0;
}

// moves the yes or no choice with left and right
static inline void MoveYesNo(Title* title, CPad& pad)
{
	if (pad.mPad->mRepeat & (HSD_PAD_STICK_LEFT | HSD_PAD_DPADLEFT)) {
		if (title->mChoice > 0) {
			title->mChoice--;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
	} else if (pad.mPad->mRepeat & (HSD_PAD_STICK_RIGHT | HSD_PAD_DPADRIGHT)) {
		if (title->mChoice < 1) {
			title->mChoice++;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
	}
}

// how far the cursor's first animation is through, from 0 to 1
static inline f32 GetAnimProgress(Title* title)
{
	HSD_AObj* aobj = GetAObj(title->mCursor.mJObj);
	f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
	aobj           = GetAObj(title->mCursor.mJObj);
	return frame / (aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f);
}

// the options line is lit once the window is fully open
static inline bool IsLineLit(Title* title, s32 line)
{
	bool lit = false;
	if (title->mOptionLine == line && title->mHighlightDelay >= FRAME_RATE) {
		lit = true;
	}
	return lit;
}

static inline void HandleUnavailableCard(Title* title)
{
	CPad pad(0);
	if (title->mReady == 0) {
		OnCardRemoved(title);
#if !defined(VERSION_JP)
		title->mFormatCancelled = 0;
#endif
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		title->SetMode(5);
		title->mOffset = -32.0f;
		title->mChoice = 1;
		PlayGameSound(&gGame.mSound, 0x88, 255);
#if !defined(VERSION_JP)
		title->mFormatCancelled = 0;
#endif
	}
}

static inline void ApproachTitleOffset(f32& value, f32& target)
{
	f32 speed = (target - value) * .25f;
	if (speed < 0)
		speed = -speed;
	if (speed <= 1)
		speed = 1;
	ApproachHudValue(&value, &target, speed);
}

static inline void KeepTitleSoundPlaying(XSound* sound)
{
	if (!IsGameSoundPlaying(sound, (s32)0xffff0003))
		PlayGameSound(sound, (s32)0xffff0003, 255);
}

static inline void ShowTitleChoice(Title* title, s32 id)
{
	u32 tint             = 0xffffff00 | (u32)(255.f * title->mLoadAlpha);
	const char* text     = GetTitleAndSaveMenuText(id);
	title->mName.mActive = 1;
	title->mName.mCount  = 0;
	if (text != NULL) {
		title->mName.mColor = tint;
		for (s32 i = 0; i < 20 && *text; i++, text++) {
			title->mName.mChars[i].mText  = text;
			title->mName.mChars[i].mScale = -.2f * i;
			title->mName.mChars[i].mWait  = FRAME_RATE;
			title->mName.mChars[i].mBob   = DEG2RAD * (rand() % 360);
			title->mName.mChars[i].mTilt  = DEG2RAD * (rand() % 360);
			title->mName.mCount++;
			if (HelpTextIsJapanese() && (IsSjisLead(*text) & 1))
				text++;
		}
		for (s32 i = title->mName.mCount; i < 20; i++) {
			title->mName.mChars[i].mText  = NULL;
			title->mName.mChars[i].mScale = 0;
			title->mName.mChars[i].mWait  = 0;
		}
	}
}
static inline void UpdateTitleDust(Title* title, u32 frames)
{
	f32 fade   = .009f * frames;
	f32 growth = .018f * frames;
	for (s32 i = 0; i < (s32)ARRAY_SIZE(title->mDust); ++i) {
		TitleDust& dust = title->mDust[i];
		switch (dust.mState) {
		case 0:
			dust.mTimer = rand() % 120;
			++dust.mState;
			break;
		case 1:
			if ((s32)--dust.mTimer < 0) {
				dust.mSpeedY = 1;
				++dust.mState;
			}
			break;
		case 2:
			dust.mSpeedY -= fade;
			dust.mSpeedX += growth;
			if (dust.mSpeedX >= 2) {
				dust.mX      = rand() % 640;
				dust.mY      = rand() % 480;
				dust.mSpeedX = dust.mSpeedY = 0;
				dust.mState                 = 0;
			}
			break;
		}
	}
}

static inline void ApplyTitleControllerSetting(Title* title)
{
	if (gGame.mStatus.mFlags & 0x4000) {
		if (gGame.mPad.mPad[0].mConnected) {
			title->mRumble = 1;
			gGame.mStatus.mFlags |= 0x800000000ULL;
		} else {
			title->mRumble = 0;
			gGame.mStatus.mFlags &= ~0x800000000ULL;
		}
		gGame.mStatus.mFlags &= ~0x4000ULL;
	}
}

static inline void TitleSelectionMoved(Title* title)
{
	title->mPulseAngle = 45;
	PlayGameSound(&gGame.mSound, 0x87, 255);
}

static inline void TitleSelectionConfirmed(Title* title)
{
	PlayGameSound(&gGame.mSound, 0x88, 255);
	title->mState = 12;
	title->mCursor.SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
	title->mPulseAngle = 90;
}

/**
 * @note Address: 0x80030EE4
 * @note Size: 0x120
 */
extern "C" void InitTitleScreen(Title* title)
{
	ResetTitleScreenState(title);
	title->mDemo.InitializeObject(0);
	for (s32 i = 0; i < 16; ++i) {
		title->mDust[i].mState  = 0;
		title->mDust[i].mTimer  = 0;
		title->mDust[i].mX      = rand() % 640;
		title->mDust[i].mY      = rand() % 480;
		title->mDust[i].mSpeedX = 0.0f;
		title->mDust[i].mSpeedY = 0.0f;
	}
#if !defined(VERSION_JP)
	title->mFormatCancelled = 0;
#endif
}

/**
 * @note Address: 0x80030CE8
 * @note Size: 0x1FC
 */
extern "C" void ResetTitleScreenState(Title* title)
{
	title->mState          = 0;
	title->mMode           = 0;
	title->mTimeout        = -1;
	title->mMenuAlpha      = 0.0f;
	title->_14             = 0.0f;
	title->mPulseAngle     = 90.0f;
	title->mHighlightDelay = 0;
	title->mSpawnIndex     = 0;
	title->mCursor.InitializeObject(0);
	title->mCursor.mAngle      = 0.0f;
	title->mCursor.mAngleSpeed = 0.0f;
	title->mSelection          = 0;
	title->mFile               = 0;
	title->mChoice             = 0;
	title->mLoadAlpha          = 0.0f;
	title->mAlpha              = 0.0f;
	title->mPrevMode           = 0;
	title->mPhase              = 0;
	title->mDelay              = 0;
	title->mPanelX             = -10.0f;
	title->mPanelY             = -200.0f;
	title->mTargetPanelX       = -10.0f;
	title->mTargetPanelY       = -200.0f;
	title->_37C0               = 0.0f;
	title->mLoadOffset         = 0.0f;
	title->_37C8               = 0.0f;
	title->mOffset             = 0.0f;
	title->mOptionLine         = 0;
	title->mOptionState        = 0;
	title->mOptionAlpha        = 0.0f;
	title->mTargetOptionAlpha  = 0.0f;
	CPad pad(0);
	if (pad.mPad->mConnected != 0) {
		if (gGame.mStatus.mFlags & 0x800000000ULL)
			title->mRumble = 1;
		else
			title->mRumble = 0;
	} else {
		title->mRumble = 0;
	}
	if (gGame.mStatus.mFlags & 0x1000000000ULL)
		title->mMono = 1;
	else
		title->mMono = 0;
	title->mChannel = 0;
	for (s32 i = 0; i < 3; ++i)
		ResetSaveSlotRecord(&gTitleSaveSlots[i]);
	title->mLoadScroll    = 0.0f;
	title->mSaveFound     = 0;
	title->mCardScanState = 0;
	KeyOffGameSoundTrack(&gGame.mSound, 1);
#if !defined(VERSION_JP)
	title->mFormatCancelled = 0;
#endif
}

/**
 * @note Address: 0x8002E8D0
 */
// NONMATCHING: the second name loop walks the chars from title (US 0x8002EAD0 mr r5, r31) with i a copy of visible;
// this build scales visible into the pointer. Retail loads mChannel before the mReady/mWrongDevice stores
// (0x8002ED4C), this build after them. In state 14 retail loads mMenuAlpha and gGame.mFade.mSpeed.r (0x80030B70)
// before the first compare. The rest is register allocation. The card's busy and found flags are read through
// a Memcard reference so that retail's two-step address (addis 0xF, addis 1) comes out at every site.
extern "C" void UpdateTitleScreen(Title* title, u32 frames)
{

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
						if ((title->mName.mChars[i].mScale += (1.f / (.3f * FRAME_RATE)) * frames) > 1)
							title->mName.mChars[i].mScale = 1;
					}
				} else if (title->mName.mChars[i].mScale > 0) {
					if ((title->mName.mChars[i].mScale -= (1.f / (.3f * FRAME_RATE)) * frames) < 0)
						title->mName.mChars[i].mScale = 0;
				}
			}
			s32 i, visible;
			for (i = visible = 0; i < title->mName.mCount; ++i) {
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
			Memcard& card = gGame.mMemcard.mCard;
			s32 sector, capacity;
			title->mReady       = 0;
			title->mWrongDevice = 0;
			switch (ProbeMemoryCardCapacity(&card, title->mChannel, &capacity, &sector)) {
			case CARD_RESULT_WRONGDEVICE:
				title->mWrongDevice = 1;
				break;
			case CARD_RESULT_READY:
				if (sector == 8192)
					title->mReady = 1;
				break;
			case CARD_RESULT_NOCARD:
			case CARD_RESULT_FATAL_ERROR:
				break;
			}
			if (!title->mReady)
				title->mShowMessage = 0;
			if (!title->mReady) {
				if (title->mState != 20 && title->mSelection == 1)
					title->mSelection = 0;
				title->mSaveFound     = 0;
				title->mCardScanState = 0;
#if !defined(VERSION_JP)
				title->mFormatCancelled = 0;
#endif
			} else {
				switch (title->mCardScanState) {
				case 0:
					StartMemoryCardScan(&card, title->mChannel, FALSE);
					title->mCardScanState = 1;
					break;
				case 1:
					if (!title->mSaveFound && card.mFound)
						title->mSaveFound = 1;
					break;
				}
			}
		}
		if (!title->IsActive())
			return;
		// through a reference: retail adds the card's offset and mBusy's separately (US 0x8002EEA0 addis 0xf, addis 0x1)
		Memcard& card = gGame.mMemcard.mCard;
		if (card.mBusy)
			return;
	}
	// 1-2 scan the card, 3-5 read in the cursor, the demo model and the textures, 10 fades the menu in,
	// 11 is the menu, 12 waits for the cursor's anim, 20 runs the file modes, 21 the options, 13-14 fade
	// out, 22 frees it all and changes the stage, 23 is done
	switch (title->mState) {
	case 1:
		title->mState = 2;
		StartMemoryCardScan(&gGame.mMemcard.mCard, title->mChannel, FALSE);
		StartScreenFade(&gGame.mFade, 0, 0, 0, 255, 0, 0, 0, 255, 0);
		break;
	case 2: {
		Memcard& card = gGame.mMemcard.mCard;
		if (card.mFound) {
			title->mSelection = 1;
			title->mSaveFound = 1;
			ApplyTitleControllerSetting(title);
		} else {
			title->mSelection     = 0;
			title->mSaveFound     = 0;
			title->mCardScanState = 0;
			ApplyTitleControllerSetting(title);
		}
		title->mState = 3;
		if (gCmdLineArgs.mTitleCursor < 0) {
			title->mSpawnIndex = rand() % 11;
		} else {
			title->mSpawnIndex = gCmdLineArgs.mTitleCursor % 11;
		}
	}
		// fall through: loading starts on the same frame as cursor selection
	case 3:
		if (title->mCursor.LoadFromPlacement(CoordInfo(GetSpawnData(title->mSpawnIndex))))
			++title->mState;
		break;
	case 4:
		if (title->mDemo.LoadFromPlacement(CoordInfo(GetSpawnData(11))))
			++title->mState;
		break;
	case 5: {
#if defined(VERSION_JP)
		const char* path = "tpl/title.tpl";
#else
		char path[64];
		sprintf(path, "tpl/title%s.tpl", gTitleTextureSuffixes[gGame.mLanguage]);
#endif
		if (!title->mTpl.ReadEx(path, 0, 0))
			break;
		PlayGameSound(&gGame.mSound, (s32)0xffff0003, 255);
		title->mState   = 10;
		title->mTimeout = -1;
		title->mTpl.CloseHandle();
		title->mTex[0] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[0], 0);
		title->mTex[1] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[1], 0);
		title->mTex[2] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[2], 0);
#if defined(VERSION_JP)
		title->mTex[3] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[3], 0);
		title->mTex[4] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[4], 0);
		title->mTex[5] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[5], 0);
#endif
		title->mMenuAlpha = title->_14 = 0;
		StartScreenFade(&gGame.mFade, 0, 0, 0, 255, gTitlePalette[gTitlePaletteIndex].background.r,
		                gTitlePalette[gTitlePaletteIndex].background.g, gTitlePalette[gTitlePaletteIndex].background.b, 0, .5f);
#if defined(VERSION_PAL)
		Vec pos = { 0.0f, 4.0f, 0.0f };
#else
		Vec pos = { 0.0f, 8.0f, 0.0f };
#endif
		title->mDemo.mState = 1;
		title->mDemo.mLoop  = TRUE;
		title->mDemo.SetPos(pos, 0);
		title->mDemo.SetAnim(0, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
		// The first animation frame runs immediately after loading.
	}
	case 10: {

		if (title->mMenuAlpha < 1) {
			if ((title->mMenuAlpha += (1.f / (.5f * FRAME_RATE)) * frames) > 1)
				title->mMenuAlpha = 1;
		}
		if (title->_14 < 1) {
			if ((title->_14 += (1.f / FRAME_RATE) * frames) > 1)
				title->_14 = 1;
		}
		title->mCursor.UpdateObject(frames);
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		if (title->mMenuAlpha == 1 && title->_14 == 1)
			title->mState = 11;
		break;
	}
	case 11: {
		title->mHighlightDelay += frames;
		if (title->mHighlightDelay >= FRAME_RATE) {
			title->mHighlightDelay = (s32)FRAME_RATE;
			title->mPulseAngle     = (f32)fmod(title->mPulseAngle + (f32)(frames * 9), 360.);
		}
		title->mCursor.UpdateObject(frames);
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		KeepTitleSoundPlaying(&gGame.mSound);
		CPad pad(0);
		if (pad.mPad->mRepeat & (HSD_PAD_STICK_UP | HSD_PAD_DPADUP)) {
			if (title->mSaveFound) {
				if (title->mSelection > 0) {
					--title->mSelection;
					TitleSelectionMoved(title);
				}
			} else if (title->mSelection != 0) {
				title->mSelection = 0;
				TitleSelectionMoved(title);
			}
		} else if (pad.mPad->mRepeat & (HSD_PAD_STICK_DOWN | HSD_PAD_DPADDOWN)) {
			if (title->mSaveFound) {
				if (title->mSelection < 2) {
					++title->mSelection;
					TitleSelectionMoved(title);
				}
			} else if (title->mSelection != 2) {
				title->mSelection = 2;
				TitleSelectionMoved(title);
			}
		}
#if !defined(VERSION_PAL)
		else
#endif
		    if (pad.mPad->mTrigger & 0x100) {
			switch (title->mSelection) {
			case 0:
			case 1:
				for (s32 i = 0; i < 3; ++i)
					ResetSaveSlotRecord(&gTitleSaveSlots[i]);
				StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, title->mChannel, FALSE);
				title->SetMode(1);
				TitleSelectionConfirmed(title);
				break;
			case 2:
				TitleSelectionConfirmed(title);
				break;
			}
		}
		break;
	}
	case 12: {
		title->mCursor.UpdateObject(frames);
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		HSD_AObj* aobj = GetAObj(title->mCursor.mJObj);
		if ((aobj->flags & (AOBJ_NO_ANIM | AOBJ_REWINDED)) || title->mCursor.mAnimId == 0 || title->mCursor.mArchive.mNum[0] < 2) {
			title->mCursor.SetAnim(0, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			title->mPulseAngle = 0;
			switch (title->mSelection) {
			case 0:
				title->mTargetPanelY = 0;
				title->mAlpha        = 1;
				title->mState        = 20;
#if defined(VERSION_JP)
				ShowTitleChoice(title, 19);
#else
				ShowTitleChoice(title, 0);
#endif
				break;
			case 1:
				title->mTargetPanelY = 0;
				title->mAlpha        = 1;
				title->mState        = 20;
				PlayGameSound(&gGame.mSound, 0x88, 255);
#if defined(VERSION_JP)
				ShowTitleChoice(title, 20);
#else
				ShowTitleChoice(title, 1);
#endif
				break;
			case 2:
				title->mTargetPanelY      = 0;
				title->mTargetOptionAlpha = 1;
				title->mState             = 21;
				title->mOptionState       = 0;
				title->mOptionLine        = 0;
				PlayGameSound(&gGame.mSound, 0x88, 255);
#if defined(VERSION_JP)
				ShowTitleChoice(title, 53);
#else
				ShowTitleChoice(title, 2);
#endif
				break;
			}
		}
		break;
	}
	case 20:
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		KeepTitleSoundPlaying(&gGame.mSound);
		(title->*gTitleModeUpdates[title->mMode])(frames);
		break;
	case 21:
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		KeepTitleSoundPlaying(&gGame.mSound);
		UpdateTitleOptions(title, frames);
		break;
	case 13: {
		title->mState = 14;
		StartScreenFade(&gGame.mFade, gTitlePalette[gTitlePaletteIndex].background.r, gTitlePalette[gTitlePaletteIndex].background.g,
		                gTitlePalette[gTitlePaletteIndex].background.b, 0, 0, 0, 0, 255, .5f);
		title->mDemo.mState = 0;
	}
	case 14: {

		if (title->mMenuAlpha > 0) {
			if ((title->mMenuAlpha -= (1.f / (.5f * FRAME_RATE)) * frames) < 0)
				title->mMenuAlpha = 0;
		}
		if (title->_14 > 0) {
			if ((title->_14 -= (1.f / (.5f * FRAME_RATE)) * frames) < 0)
				title->_14 = 0;
		}
		if (title->mMenuAlpha == 0 && FadeDone()) {
			title->mState                   = 22;
			GXColor black                   = { 0, 0, 0, 0 };
			HSD_VIData.current.vi.clear_clr = black;
		}
		break;
	}
	case 22:
		title->mDemo.Release();
		title->mCursor.Release();
		title->mTpl.Close();
		if (title->mTimeout >= 0) {
			ChangeGameStage(&gGame, title->mTimeout, TRUE);
			title->mTimeout = -1;
		}
		title->mState = 23;
		break;
	case 23:
		break;
	}
	if (title->mState != 0 && title->mState != 22 && title->mState != 23) {
		GXColor color                   = gTitlePalette[gTitlePaletteIndex].background;
		HSD_VIData.current.vi.clear_clr = color;
	}
}

/**
 * @note Address: 0x8002E8CC
 * @note Size: 0x4
 */
void Title::State0(u32)
{
}

/**
 * @note Address: 0x8002E574
 * @note Size: 0x358
 */
// NONMATCHING: the loop's saved registers: retail keeps the save pointer in r31 (0x8002E5F0 addis r31, r28, 0x1) above the slot,
// timestamp and card walkers, this build in r28 below them. A function-top card pointer and the header fields read in place
// score lower
void Title::State1(u32 frames)
{
	s32 state = gGame.mMemcard.mCard.mState;
	SavePreviewRecord* save;
	mDelay -= frames;
	if (mDelay < 0)
		mDelay = 0;
	switch (state) {
	case 0: {
		Memcard* card = &gGame.mMemcard.mCard;
		OSTime timestamps[3];
		for (s32 i = 0; i < 3; ++i) {
			save = &card->mSaves[0][i];
			if (save->mState) {
				OSCalendarTime calendar;
				OSTicksToCalendarTime(OSSecondsToTicks((OSTime)save->mStat.time), &calendar);
				s32 frames                     = save->mHeader.mFrames;
				s32 active                     = save->mHeader.mActive;
				gTitleSaveSlots[i].mUsed       = 1;
				gTitleSaveSlots[i].mLevel      = save->mHeader.mLevel;
				gTitleSaveSlots[i].mBattery    = save->mHeader.mBattery;
				gTitleSaveSlots[i].mMaxBattery = save->mHeader.mMaxBattery;
				gTitleSaveSlots[i].mMoney      = save->mHeader.mMoney;
				gTitleSaveSlots[i].mExp        = save->mHeader.mExp;
				gTitleSaveSlots[i].mScrap      = save->mHeader.mScrap;
				gTitleSaveSlots[i].mRank       = save->mHeader.mRank;
				gTitleSaveSlots[i].mHours      = frames / 216000;
				gTitleSaveSlots[i].mMinutes    = (frames / 3600) % 60;
				gTitleSaveSlots[i].mStage      = save->mHeader.mStage;
				gTitleSaveSlots[i].mTimeMode   = save->mHeader.mTimeMode;
				if (active > 0)
					gTitleSaveSlots[i].mActive = 1;
				else
					gTitleSaveSlots[i].mActive = 0;
				timestamps[i] = OSCalendarTimeToTicks(&calendar);
				if (save->mStatus != 0 || !IsValidSavePreview(save))
					gTitleSaveSlots[i].mState = 1;
				else
					gTitleSaveSlots[i].mState = 0;
			}
		}
		mFreeBlocks  = gGame.mMemcard.mCard.mFreeBlocks;
		mFreeEntries = gGame.mMemcard.mCard.mFreeEntries;
		SetMode(2);
		if (mSelection == 0)
			mFile = 0;
		else
			mFile = IndexOfLargestOfThree(timestamps[0], timestamps[1], timestamps[2]);
		mAlpha = 1.0f;
		break;
	}
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15: {
		s32 result = gGame.mMemcard.mCard.mCard.mResult;
		mResult    = result;
		switch (result) {
		case -128:
		case -14:
		case -13:
		case -12:
		case -11:
		case -10:
		case -9:
		case -8:
		case -7:
		case -6:
		case -5:
		case -4:
		case -3:
		case -2:
		case -1:
		case 0:
			if (result == CARD_RESULT_BROKEN || result == CARD_RESULT_ENCODING) {
				SetMode(11);
				mChoice = 1;
			} else if (result == CARD_RESULT_IOERROR) {
				SetMode(19);
				mShowMessage = 1;
			} else {
				SetMode(14);
				mOffset = -32.0f;
			}
			mAlpha = 1.0f;
			break;
		default:
			SetMode(14);
			mAlpha  = 1.0f;
			mOffset = -32.0f;
			break;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8002DF3C
 * @note Size: 0x638
 */
// picks one of the three save files, A starts or loads it, B goes back
void Title::State2(u32)
{
	CPad pad(0);

	if ((!mReady || !mSaveFound) && mSelection == 1) {
		OnFormattedCardGone(this);
		return;
	}
	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	mOffset = 0.0f;
	if (pad.mPad->mRepeat & (HSD_PAD_STICK_LEFT | HSD_PAD_DPADLEFT)) {
		mFile--;
		if (mFile < 0) {
			mFile = 2;
		}
		PlayGameSound(&gGame.mSound, 0x87, 0xFF);
	} else if (pad.mPad->mRepeat & (HSD_PAD_STICK_RIGHT | HSD_PAD_DPADRIGHT)) {
		mFile++;
		if (mFile > 2) {
			mFile = 0;
		}
		PlayGameSound(&gGame.mSound, 0x87, 0xFF);
	}
	if (pad.mPad->mTrigger & 0x100) {
		switch (mSelection) {
		case 0:
			if (gTitleSaveSlots[mFile].mUsed == 1) {
				SetMode(3);
				mChoice = 1;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			} else if (mFreeBlocks >= 6 && mFreeEntries > 0) {
				StartMemoryCardSaveWrite(&gGame.mMemcard.mCard, mChannel, mFile, 1);
				SetMode(15);
				mDelay  = 1.5f * FRAME_RATE;
				mOffset = -32.0f;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			} else {
				mResult = CARD_RESULT_INSSPACE;
				SetMode(14);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				mOffset = -32.0f;
			}
			return;
		case 1:
			if (gTitleSaveSlots[mFile].mUsed == 1) {
				if (!IsSaveSlotIntact(&gTitleSaveSlots[mFile])) {
					mResult = -128;
					SetMode(14);
					PlayGameSound(&gGame.mSound, 0x88, 0xFF);
					mOffset = 0.0f;
					return;
				}
				if (gTitleSaveSlots[mFile].mStage == 0) {
					HideName(this);
					mTargetPanelY = -200.0f;
					mAlpha        = 0.0f;
					mChoice       = 0;
					mState        = 13;
					SetMode(25);
					PlayGameSound(&gGame.mSound, 0x88, 0xFF);
					StopGameSfx(&gGame.mSound, (s32)0xFFFF0003);
					gGame.mInfoScrn.mSelectedSaveSlot = mFile;
					return;
				}
				StartMemoryCardSaveRead(&gGame.mMemcard.mCard, mChannel, mFile, 1);
				SetMode(8);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			} else {
				PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			}
			return;
		}
	} else if (pad.mPad->mTrigger & 0x200) {
		SetMode(25);
		mState = 11;
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		mTargetPanelY = -200.0f;
		mOffset       = 0.0f;
		mAlpha        = 0.0f;
		HideName(this);
	}
}

/**
 * @note Address: 0x8002DCFC
 * @note Size: 0x240
 */
// the yes or no question, left and right pick, A takes it
void Title::State3(u32)
{
	CPad pad(0);
	CPadOne* state;

	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	state = pad.mPad;
	if (state->mRepeat & (HSD_PAD_STICK_LEFT | HSD_PAD_DPADLEFT)) {
		if (mChoice > 0) {
			mChoice--;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			return;
		}
	} else if (state->mRepeat & (HSD_PAD_STICK_RIGHT | HSD_PAD_DPADRIGHT)) {
		if (mChoice < 1) {
			mChoice++;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			return;
		}
	}
	if (state->mTrigger & 0x100) {
		switch (mChoice) {
		case 0:
			StartMemoryCardSaveWrite(&gGame.mMemcard.mCard, mChannel, mFile, 1);
			SetMode(15);
			mDelay = 1.5f * FRAME_RATE;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mOffset = -32.0f;
			return;
		case 1:
			SetMode(2);
			mChoice = 0;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			return;
		}
	}
}

/**
 * @note Address: 0x8002DCF8
 * @note Size: 0x4
 */
void Title::State4(u32)
{
}

/**
 * @note Address: 0x8002D7F4
 * @note Size: 0x504
 */
// asked whether to go on without saving
void Title::State5(u32)
{
	CPad pad(0);

	switch (mResult) {
	case -3:
		if (mReady == 1) {
			for (s32 i = 0; i < 3; i++) {
				ResetSaveSlotRecord(&gTitleSaveSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
			mOffset = 0.0f;
			SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			return;
		}
		break;
	case -2:
		if (!mWrongDevice) {
			OnCardRemoved(this);
			return;
		}
		break;
	default:
		if (!mReady) {
			OnCardRemoved(this);
			return;
		}
		break;
	}
	MoveYesNo(this, pad);
	if (pad.mPad->mTrigger & 0x100) {
		if (mChoice == 0 && (pad.mPad->mTrigger & 0x100)) {
			HideName(this);
			mTargetPanelY = -200.0f;
			mOffset       = 0.0f;
			mAlpha        = 0.0f;
			mChoice       = 0;
			mState        = 13;
			SetMode(25);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			StopGameSfx(&gGame.mSound, (s32)0xFFFF0003);
			gGame.mInfoScrn.mSelectedSaveSlot = mFile;
			return;
		}
		if (mChoice == 1 && (pad.mPad->mTrigger & 0x100)) {
			if (mShowMessage == 1) {
				SetMode(19);
				mOffset = 0.0f;
			} else if (mResult == 0) {
				SetMode(2);
				mChoice = 0;
			} else if (mResult == -3) {
				SetMode(25);
				mState = 11;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				mTargetPanelY = -200.0f;
				mAlpha        = 0.0f;
				mChoice       = 0;
				mOffset       = 0.0f;
				HideName(this);
			} else {
				for (s32 i = 0; i < 3; i++) {
					ResetSaveSlotRecord(&gTitleSaveSlots[i]);
				}
				StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
				SetMode(1);
				mOffset = 0.0f;
			}
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			return;
		}
	}
}

/**
 * @note Address: 0x8002D4F0
 * @note Size: 0x304
 */
// asked whether to save over the file, unless the card went away under it
void Title::State6(u32)
{
	CPad pad(0);

	if ((!mReady || !mSaveFound) && mSelection == 1) {
		SetMode(25);
		mState = 11;
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		mTargetPanelY = -200.0f;
		mOffset       = 0.0f;
		mAlpha        = 0.0f;
		HideName(this);
		mSelection     = 0;
		mSaveFound     = 0;
		mCardScanState = 0;
		return;
	}
	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	if (pad.mPad->mRepeat & (HSD_PAD_STICK_LEFT | HSD_PAD_DPADLEFT)) {
		if (mChoice > 0) {
			mChoice--;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			return;
		}
	} else if (pad.mPad->mRepeat & (HSD_PAD_STICK_RIGHT | HSD_PAD_DPADRIGHT)) {
		if (mChoice < 1) {
			mChoice++;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			return;
		}
	}
	if (pad.mPad->mTrigger & 0x100) {
		switch (mChoice) {
		case 0:
			StartMemoryCardSaveWrite(&gGame.mMemcard.mCard, mChannel, mFile, 1);
			SetMode(15);
			mDelay = 1.5f * FRAME_RATE;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mOffset = -32.0f;
			break;
		case 1:
			SetMode(2);
			mChoice = 0;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			break;
		}
	}
}

/**
 * @note Address: 0x8002D4EC
 * @note Size: 0x4
 */
void Title::State7(u32)
{
}

/**
 * @note Address: 0x8002D2D4
 * @note Size: 0x218
 */
void Title::State8(u32)
{
	s32 cardState = gGame.mMemcard.mCard.mState;
	switch (cardState) {
	case 0:
		HideName(this);
		StopGameSfx(&gGame.mSound, (s32)0xFFFF0003);
		mTargetPanelY                     = -200.0f;
		mAlpha                            = 0.0f;
		gGame.mInfoScrn.mSelectedSaveSlot = mFile;
		ResumeScrapCounterPulse(&gGame.mInfoScrn);
		SetMode(25);
		mState   = 13;
		mTimeout = 20000;
		break;
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15: {
		s32 result = gGame.mMemcard.mCard.mCard.mResult;
		mResult    = result;
		switch (result) {
		case -128:
		case -14:
		case -13:
		case -12:
		case -11:
		case -10:
		case -9:
		case -8:
		case -7:
		case -6:
		case -5:
		case -4:
		case -3:
		case -2:
		case -1:
		case 0:
			// while mounting (card state 2), a broken card or one from another region gets the format question
			if (cardState == 2 && (result == CARD_RESULT_BROKEN || result == CARD_RESULT_ENCODING)) {
				SetMode(11);
				mChoice      = 1;
				mShowMessage = 1;
			} else if (result == CARD_RESULT_NOCARD || result == CARD_RESULT_IOERROR) {
				SetMode(22);
				mShowMessage = 1;
			} else {
				SetMode(14);
				mOffset = -32.0f;
			}
			mAlpha = 1.0f;
			break;
		default:
			SetMode(14);
			mAlpha  = 1.0f;
			mOffset = -32.0f;
			break;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8002D18C
 * @note Size: 0x148
 */
void Title::State9(u32)
{
	CPad pad(0);
	if (mWrongDevice == 1 || mReady == 1) {
		for (s32 i = 0; i < 3; ++i) {
			ResetSaveSlotRecord(&gTitleSaveSlots[i]);
		}
		StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, TRUE);
		SetMode(1);
		PlayGameSound(&gGame.mSound, 0x88, 255);
		mOffset = 0.0f;
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		SetMode(5);
		mOffset = -32.0f;
		mChoice = 1;
		PlayGameSound(&gGame.mSound, 0x88, 255);
	}
}

/**
 * @note Address: 0x8002D188
 * @note Size: 0x4
 */
void Title::State10(u32)
{
}

/**
 * @note Address: 0x8002CE40
 * @note Size: 0x348
 */
// asked twice whether to make a new save file
void Title::State11(u32)
{
	CPad pad(0);

	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	switch (mPhase) {
	case 0:
		MoveYesNo(this, pad);
		if (pad.mPad->mTrigger & 0x100) {
			if (mChoice == 0) {
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				mChoice = 1;
				mPhase++;
			} else {
				SetMode(19);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
#if !defined(VERSION_JP)
				mFormatCancelled = 1;
#endif
			}
			return;
		}
		break;
	case 1:
		MoveYesNo(this, pad);
		if (pad.mPad->mTrigger & 0x100) {
			if (mChoice == 0) {
				StartMemoryCardFormatCheck(&gGame.mMemcard.mCard, mChannel, 1);
				SetMode(12);
				mDelay = 1.5f * FRAME_RATE;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			} else {
				SetMode(19);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
#if !defined(VERSION_JP)
				mFormatCancelled = 1;
#endif
			}
			return;
		}
		break;
	}
}

/**
 * @note Address: 0x8002CCF4
 * @note Size: 0x14C
 */
void Title::State12(u32 frames)
{
	mDelay -= frames;
	if (mDelay < 0) {
		mDelay = 0;
	}
	switch (gGame.mMemcard.mCard.mState) {
	case 0:
		mFreeBlocks  = gGame.mMemcard.mCard.mFreeBlocks;
		mFreeEntries = gGame.mMemcard.mCard.mFreeEntries;
		SetMode(13);
		mAlpha = 1.0f;
		break;
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15: {
		s32 result = gGame.mMemcard.mCard.mCard.mResult;
		mResult    = result;
		switch (result) {
		case -128:
		case -14:
		case -13:
		case -12:
		case -11:
		case -10:
		case -9:
		case -8:
		case -7:
		case -6:
		case -5:
		case -4:
		case -3:
		case -2:
		case -1:
		case 0:
			if (result == CARD_RESULT_BROKEN || result == CARD_RESULT_ENCODING) {
				SetMode(19);
				mShowMessage = 1;
			} else {
				SetMode(21);
				mShowMessage = 1;
			}
			mAlpha = 1.0f;
			break;
		default:
			SetMode(14);
			mAlpha  = 1.0f;
			mOffset = -32.0f;
			break;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8002CB40
 * @note Size: 0x1B4
 */
void Title::State13(u32 frames)
{
	CPad pad(0);

	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	switch (mPhase) {
	case 0:
		mDelay -= frames;
		if (mDelay < 0) {
			mDelay = 0;
			mPhase++;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
		break;
	case 1:
		if (pad.mPad->mTrigger & 0x100) {
			for (s32 i = 0; i < 3; i++) {
				ResetSaveSlotRecord(&gTitleSaveSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
			SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			return;
		}
		break;
	}
}

/**
 * @note Address: 0x8002C738
 * @note Size: 0x408
 */
// shows what is wrong with the card until A is pressed or the card changes
void Title::State14(u32)
{
	CPad pad(0);
	s32 error = mResult;

	switch (error) {
	case -3:
		if (mWrongDevice == 1) {
			for (s32 i = 0; i < 3; i++) {
				ResetSaveSlotRecord(&gTitleSaveSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
			SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mOffset = 0.0f;
			return;
		}
		if (mReady == 1) {
			for (s32 i = 0; i < 3; i++) {
				ResetSaveSlotRecord(&gTitleSaveSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
			SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mOffset = 0.0f;
			return;
		}
		break;
	case -2:
		if (!mWrongDevice) {
			OnCardRemoved(this);
			return;
		}
		break;
	case -128:
		if ((!mReady || !mSaveFound) && mSelection == 1) {
			OnFormattedCardGone(this);
			return;
		}
		break;
	default:
		if (!mReady) {
			OnCardRemoved(this);
			return;
		}
		break;
	}
	if (pad.mPad->mTrigger & 0x100) {
		switch (error) {
		case -9:
			SetMode(18);
			mOffset = 0.0f;
			break;
		case -128:
			SetMode(6);
			mOffset = -32.0f;
			break;
		default:
			SetMode(5);
			mOffset = -32.0f;
			mChoice = 1;
			break;
		}
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
	}
}

/**
 * @note Address: 0x8002C3F0
 * @note Size: 0x348
 */
// NONMATCHING: regalloc
// waits for the card to read the file picked, then shows what is in it
void Title::State15(u32 frames)
{
	Memcard* card = &gGame.mMemcard.mCard;
	s32 step      = card->mState;
	SavePreviewRecord* file;
	s32 result;

	mDelay -= frames;
	if (mDelay < 0) {
		mDelay = 0;
	}
	switch (step) {
	case 0:
		mFreeBlocks  = gGame.mMemcard.mCard.mFreeBlocks;
		mFreeEntries = gGame.mMemcard.mCard.mFreeEntries;
		file         = &card->mSaves[0][mFile];
		if (file->mState != 0) {
			s32 level                          = file->mHeader.mLevel;
			f32 battery                        = file->mHeader.mBattery;
			f32 maxBattery                     = file->mHeader.mMaxBattery;
			s32 money                          = file->mHeader.mMoney;
			s32 exp                            = file->mHeader.mExp;
			s32 scrap                          = file->mHeader.mScrap;
			s32 rank                           = file->mHeader.mRank;
			s32 hours                          = file->mHeader.mFrames / 216000;
			s32 minutes                        = (file->mHeader.mFrames / 3600) % 60;
			s32 stage                          = file->mHeader.mStage;
			s32 timeMode                       = file->mHeader.mTimeMode;
			gTitleSaveSlots[mFile].mUsed       = 1;
			gTitleSaveSlots[mFile].mLevel      = level;
			gTitleSaveSlots[mFile].mBattery    = battery;
			gTitleSaveSlots[mFile].mMaxBattery = maxBattery;
			gTitleSaveSlots[mFile].mMoney      = money;
			gTitleSaveSlots[mFile].mExp        = exp;
			gTitleSaveSlots[mFile].mScrap      = scrap;
			gTitleSaveSlots[mFile].mRank       = rank;
			gTitleSaveSlots[mFile].mHours      = hours;
			gTitleSaveSlots[mFile].mMinutes    = minutes;
			gTitleSaveSlots[mFile].mStage      = stage;
			gTitleSaveSlots[mFile].mTimeMode   = timeMode;
			gTitleSaveSlots[mFile].mState      = 0;
			if (file->mHeader.mActive > 0) {
				gTitleSaveSlots[mFile].mActive = 1;
			} else {
				gTitleSaveSlots[mFile].mActive = 0;
			}
		}
		SetMode(24);
		break;
	case 4:
		break;
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		result  = gGame.mMemcard.mCard.mCard.mResult;
		mResult = result;
		switch (result) {
		case -128:
		case -14:
		case -13:
		case -12:
		case -11:
		case -10:
		case -9:
		case -8:
		case -7:
		case -6:
		case -5:
		case -4:
		case -3:
		case -2:
		case -1:
		case 0:
			if (step == 2 && (result == CARD_RESULT_BROKEN || result == CARD_RESULT_ENCODING)) {
				SetMode(11);
				mChoice      = 1;
				mShowMessage = 1;
			} else if (result == CARD_RESULT_NOCARD || result == CARD_RESULT_IOERROR) {
				SetMode(20);
				mShowMessage = 1;
			} else {
				SetMode(14);
				mOffset = -32.0f;
			}
			mAlpha = 1.0f;
			break;
		default:
			SetMode(14);
			mAlpha  = 1.0f;
			mOffset = -32.0f;
			break;
		}
		break;
	}
}

/**
 * @note Address: 0x8002C3EC
 * @note Size: 0x4
 */
void Title::State16(u32)
{
}

/**
 * @note Address: 0x8002C3E8
 * @note Size: 0x4
 */
void Title::State17(u32)
{
}

/**
 * @note Address: 0x8002C2D0
 * @note Size: 0x118
 */
void Title::State18(u32)
{
	HandleUnavailableCard(this);
}

/**
 * @note Address: 0x8002C1B8
 * @note Size: 0x118
 */
void Title::State19(u32)
{
	HandleUnavailableCard(this);
}

/**
 * @note Address: 0x8002BD0C
 * @note Size: 0x4AC
 */
// the card check's message, it waits for A or for the card to change
void Title::State20(u32)
{
	CPad pad(0);
	s32 error = mResult;

	if (error == -3) {
		if (mSelection == 1) {
			if (mReady) {
				OnFormattedCardGone(this);
				return;
			}
		} else if (mReady == 1) {
			for (s32 i = 0; i < 3; i++) {
				ResetSaveSlotRecord(&gTitleSaveSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
			SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			return;
		}
	} else if (mSelection == 1) {
		if (!mReady || !mSaveFound) {
			OnFormattedCardGone(this);
			return;
		}
	} else if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		if (mSelection == 1) {
			OnFormattedCardGone(this);
			return;
		}
		if (error == -3) {
			OnCardRemoved(this);
		} else if (error == -5) {
			SetMode(19);
			mShowMessage = 1;
		} else {
			SetMode(5);
			mOffset = -32.0f;
			mChoice = 1;
		}
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
	}
}

/**
 * @note Address: 0x8002BCEC
 * @note Size: 0x20
 */
void Title::State21(u32 frames)
{
	State20(frames);
}

/**
 * @note Address: 0x8002BCCC
 * @note Size: 0x20
 */
void Title::State22(u32 frames)
{
	State20(frames);
}

/**
 * @note Address: 0x8002BCC8
 * @note Size: 0x4
 */
void Title::State23(u32)
{
}

/**
 * @note Address: 0x8002BB80
 * @note Size: 0x148
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
		XSound* sound = &gGame.mSound;
		PlayGameSound(sound, 0x88, 255);
		StopGameSfx(sound, (s32)0xFFFF0003);
		gGame.mInfoScrn.mSelectedSaveSlot = mFile;
		break;
	}
}

/**
 * @note Address: 0x8002BB7C
 * @note Size: 0x4
 */
void Title::State25(u32)
{
}

static inline void BeginTitleOverlay()
{
	SetGsChannelEnabled(&gGame.mGs, 4, 0);
	SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
	SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 0);
	SetGsAlphaCompare(gGame.mGs.mCurRenderMode, 4, 0, 0, 3, 255);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
	SetupAll(FALSE);
}

static inline void DrawTitleCursors(Title* title, f32 halfWidth, f32 y)
{
#if defined(VERSION_JP)
	const f32 gap = 24.0f;
#else
	const f32 gap = 30.0f;
#endif
	title->mCursor.SetPos((320.0f - halfWidth) - gap, y, 10.0f, 0);
	title->mCursor.SetRotationY(0.2617994f, 0);
	title->mCursor.DrawObject();
	title->mCursor.SetPos(gap + (320.0f + halfWidth), y, 10.0f, 0);
	title->mCursor.SetRotationY(-0.2617994f, 0);
	title->mCursor.DrawObject();
}

// the text of one title entry, lit once the entry is selected and the highlight delay has passed
static inline void DrawTitleEntryText(Title* title, s32 entry, u32 normal, u32 lit, const char* text, f32 x, f32 y, f32 size)
{
	bool selected = false;
	if (title->mSelection == entry && (f32)title->mHighlightDelay >= FRAME_RATE)
		selected = true;
	DrawGsProportionalTextFormat(&gGame.mGs, selected ? lit : normal, text, x, y, 0.0f, 24.0f, size);
}

static inline f32 DrawTitleEntry(Title* title, s32 entry, s32 textId, f32 y, f32 progress, u32 normal, u32 lit)
{
	const char* text = GetTitleAndSaveMenuText(textId);
	f32 size         = 24.0f + (title->mSelection == entry ? 2.0f * (f32)sin(4.712389f * progress) : 0.0f);
	f32 width        = MeasureGameTextWidth(&gGame.mGs, text, 1, 24.0f);
	if (entry == 1) {
		bool selected = false;
		if (title->mSelection == entry && (f32)title->mHighlightDelay >= FRAME_RATE)
			selected = true;
		u32 tint = normal;
		if (selected)
			tint = lit;
		if (title->mSaveFound)
			DrawGsProportionalTextFormat(&gGame.mGs, tint, text, 320.0f - width / 2.0f, (size - 24.0f) / 2.0f + (y - size), 0.0f, 24.0f,
			                             size);
	} else
		DrawTitleEntryText(title, entry, normal, lit, text, 320.0f - width / 2.0f, (size - 24.0f) / 2.0f + (y - size), size);
	return width;
}

#if !defined(VERSION_JP)
static inline f32 TitleOptionWidth(s32 first)
{
	f32 value1  = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(first + 1), 1, 24.0f);
	f32 value2  = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(first + 2), 1, 24.0f);
	f32 label   = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(first), 1, 24.0f);
	f32 largest = value1 > value2 ? value1 : value2;
	return 2.0f * ((label > largest ? label : largest) + 8.0f);
}
#endif

// NONMATCHING: float registers only (retail keeps the dust alpha in f30, this build in f28) and the logo quad's schedule.
// Both save f26-f31; the alpha, the later progress and the three entry widths are coloured onto other slots. -O4 without p,
// -O3,p and -O4,s score lower for the whole unit, so it is not the optimisation level. Retail also loads the entry text's
// 0.0f, 24.0f and size into f3-f5 before the selection test (0x8002B50C) where this build loads them at the call, and the
// inner of the two widest-width picks goes to a fresh register in both arms (fmr f0 at 0x8002B79C, 0x8002B928 and 0x8002B9D4); a
// WiderOf(a, b) inline compiles to the same bytes as the ternaries
extern "C" void DrawTitleScreen(Title* title)
{
#if defined(VERSION_JP)
	enum { dustTexture = 5, footerTexture = 2 };
#else
	enum { dustTexture = 2, footerTexture = 1 };
#endif
	if (!title->IsActive())
		return;
	gGame.mGs.mCamera = ((CBase*)&gGame)->GetScreenCamera();
	if (BeginGsCamera(&gGame)) {
		if (title->mMenuAlpha > 0.0f) {
			BeginTitleOverlay();

			for (s32 i = 0; i < 16; ++i) {
				f32 alpha = title->mMenuAlpha;
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				TitleDust& dust = title->mDust[i];

				f32 w     = dust.mSpeedX * title->mTex[dustTexture].GetWidth();
				f32 h     = dust.mSpeedX * title->mTex[dustTexture].GetHeight();
				u32 color = 0xffffff00 | (u32)(255.0f * dust.mSpeedY * alpha);
				DrawGsTexturedQuad(&gGame.mGs, &title->mTex[dustTexture], color, color, color, color, dust.mX, dust.mY, 0.0f, w, h, 0, 0,
				                   0);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
			}
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			u16 w     = GXGetTexObjWidth(&title->mTex[gTitleLogoTextures[gTitleLogoIndex]]);
			u16 h     = GXGetTexObjHeight(&title->mTex[gTitleLogoTextures[gTitleLogoIndex]]);
			u32 color = 0xffffff00 | (u32)(255.0f * title->mMenuAlpha);
			DrawGsTexturedQuad(&gGame.mGs, &title->mTex[gTitleLogoTextures[gTitleLogoIndex]], color, color, color, color, 320, 130, 0, w, h,
			                   0, 0, 0);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
		EndGsCamera(&gGame.mGs);
	}
	if (title->mDemo.mState && (title->mDemo.mStatus.mFlags & 1)) {
		InvalidateAll();
		title->mDemo.DrawObject();
	}
	gGame.mGs.mCamera = ((CBase*)&gGame)->GetScreenCamera();
	if (!BeginGsCamera(&gGame))
		return;
	if (title->mMenuAlpha > 0.0f) {
		BeginTitleOverlay();
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		u16 w = GXGetTexObjWidth(&title->mTex[footerTexture]), h = GXGetTexObjHeight(&title->mTex[footerTexture]);
		u32 color = (u32)(255.0f * title->mMenuAlpha);
		DrawGsTexturedQuad(&gGame.mGs, &title->mTex[footerTexture], color, color, color, color, 320.0f, 398.0f, 0, .6f * w, .6f * h, 0, 0,
		                   0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
	DrawPanelBackground<SavePanel>(title->mLoadAlpha + title->mOptionAlpha);
	DrawTitleHelpText(title);
	DrawTitleLoadScreen(title);
	DrawTitleOptions(title);
	DrawTitleNameBubbles(title);
	if (title->mState == 10 || title->mState == 11 || title->mState == 12) {
		f32 wave  = 127.0f * (f32)sin(DEG2RAD * title->mPulseAngle);
		f32 alpha = title->mMenuAlpha;
		u32 rgb   = ((u32)gTitlePalette[gTitlePaletteIndex].text.r << 24) | ((u32)gTitlePalette[gTitlePaletteIndex].text.g << 16)
		        | ((u32)gTitlePalette[gTitlePaletteIndex].text.b << 8);
		u32 lit      = rgb | (u8)(127.0f + alpha * wave);
		u32 normal   = rgb | (u8)(255.0f * alpha);
		f32 progress = title->mCursor.mAnimId == 1 ? GetAnimProgress(title) : 0.0f;
#if defined(VERSION_JP)
		enum { first = 0x13, second = 0x14, third = 0x35 };
#else
		enum { first = 0, second = 1, third = 2 };
#endif
		f32 w0 = DrawTitleEntry(title, 0, first, 324.0f, progress, normal, lit);
		f32 w1 = DrawTitleEntry(title, 1, second, 352.0f, progress, normal, lit);
		f32 y  = 380.0f;
		f32 w2 = DrawTitleEntry(title, 2, third, y, progress, normal, lit);
		switch (title->mSelection) {
		case 0:
			y = 324.0f;
#if defined(VERSION_JP)
			w2 = w0;
#endif
			break;
		case 1:
			y = 352.0f;
#if defined(VERSION_JP)
			w2 = w1;
#endif
			break;
		case 2:
			y = 380.0f;
			break;
		}
#if !defined(VERSION_JP)
		f32 widest = w0 > w1 ? w0 : w1;
		w2         = w2 > widest ? w2 : widest;
#endif
		DrawTitleCursors(title, w2 / 2.0f, y);
	}
	if (title->mState == 21) {
		f32 y;
		u32 width;
		switch (title->mOptionLine) {
		case 0:
			y = 208;
#if defined(VERSION_PAL)
			y -= 18.0f;
#endif
#if defined(VERSION_JP)
			width = 192;
#else
			width = TitleOptionWidth(0x10);
#endif
			break;
		case 1:
			y = 256;
#if defined(VERSION_PAL)
			y -= 18.0f;
#endif
#if defined(VERSION_JP)
			width = 216;
#else
			width = TitleOptionWidth(0x13);
#endif
			break;
		case 2:
			y = 304;
#if defined(VERSION_PAL)
			y -= 18.0f;
#endif
#if defined(VERSION_JP)
			width = 72;
#else
			width = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x16), 1, 24.0f);
#endif
			break;
		}

		DrawTitleCursors(title, width / 2, y);
	}
	EndGsCamera(&gGame.mGs);
}

static inline u32 LoadTextColor(f32 alpha)
{
	return 0x14141400 | (u32)(255.0f * alpha);
}
static inline f32 LoadPromptY(Title* title, f32 panelY)
{
	return ((8.0f + (356.0f - panelY)) - 6.0f) + title->mLoadOffset;
}
static inline s32 LoadStatusAt(Title* title, s32 id, f32 panelX, f32 panelY)
{
#if defined(VERSION_JP)
	return DrawJapaneseSaveStatusMessage(title->mChannel, id, panelX, panelY, 21.0f, title->mLoadAlpha);
#else
	return DrawSaveStatusMessage(title->mChannel, id, panelX, panelY, 21.0f, title->mLoadAlpha);
#endif
}
static inline s32 LoadStatusLines(Title* title, s32 id, f32 panelX, f32 panelY)
{
	return LoadStatusAt(title, id, 320.0f + panelX, LoadPromptY(title, panelY));
}
#if !defined(VERSION_JP)
static inline void DrawLoadText(const char* text, f32 center, f32 y, u32 color)
{
	f32 width = MeasureGameTextWidth(&gGame.mGs, text, 1, 21.0f);
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, center - width / 2.0f, y, 0.0f, 21.0f, 21.0f);
}
#endif
static inline const char* LoadPromptText(s32 western, s32 japanese)
{
#if defined(VERSION_JP)
	return GetTitleAndSaveMenuText(japanese);
#else
	return GetLocalizedInterfaceText(western);
#endif
}

#if defined(VERSION_JP)
static inline u32 LoadHalfWidth(u32 cells)
{
	return ((cells - 1) * 21) / 2;
}
#else
static inline void DrawLoadChoice(Title* title, s32 label, f32 center, f32 y)
{
	u32 color = LoadTextColor(title->mLoadAlpha);
	DrawLoadText(GetLocalizedInterfaceText(label), center, y, color);
}
#endif

// NONMATCHING: float and saved registers (lines and the text color swap r26/r27); the bar's last quad adds 2.0f to barX
// variable-first in retail (US 0x8002A95C fadds f1, f29, f0), constant-first here. The bar's y is barY + inset: retail adds
// the 2.0f at each quad, variable-first (US 0x8002A7B4, 0x8002A81C fadds f2, f25, f0), which a literal 2.0f folds into one
// add; the same 2.0f register feeds the x add, so that 2 was a variable here and in MenuSave::Draw (0x8003A9E4), while
// DrawPCShopList adds a literal (0x800422F0 fadds f1, f1, f27, and one y add at 0x80042158). An inset parameter of
// DrawMenuSelectionBar, a local inside it, or barX + 2.0f give the variable-first x here but also in DrawPCShopList
// (a literal bound to an inline parameter keeps the body's operand order), so one inline cannot produce both shapes.
// JP, mode 2: retail computes the first line's x and y (f25/f26) before LoadTextColor's __cvt_fp2unsigned, this build
// forwards them past it (a color local and a JP DrawLoadText inline taking x by value are identical)
extern "C" void DrawTitleLoadScreen(Title* title)
{
	switch (title->mMode) {
	case 2:
	case 3:
	case 8:
	case 15:
	case 24:
		for (s32 i = 0; i < 3; ++i) {
#if defined(VERSION_JP)
			f32 x = 49.0f + (194.0f * i + 77.0f), width = 154.0f;
#else
			f32 width = 154.0f + sSaveSlotExtraWidth;
			f32 x     = 49.0f + ((width / 2.0f) + i * (40.0f + width)) - (3.0f * sSaveSlotExtraWidth) / 2.0f;
#endif
			u8 selected = 0, empty = 0;
			if (title->mFile == i)
				selected = 1;
			if (title->mFreeBlocks >= 6 && title->mFreeEntries > 0)
				empty = 1;
			DrawSaveSlotDetails(&gTitleSaveSlots[i], empty, i, selected, x, 223.0f, width, 230.0f, title->mLoadAlpha, 1.0f, 1.0f);
		}
		break;
	}
	f32 panelX = title->mPanelX, panelY = title->mPanelY - 8.0f;
	DrawMenuBannerPanels(NULL, panelX, panelY, 1.0f, title->mLoadOffset);
	bool showMessage = false;
	s32 lines        = 2;
	char text[256];

	switch (title->mMode) {
	case 2:
		if (!title->mSelection) {
#if defined(VERSION_JP)
			sprintf(text, "%s", LoadPromptText(0x94, 0x15));
			u32 cells  = strlen(text) / 2;
			f32 center = 320.0f + panelX, top = 8.0f + (356.0f - panelY);
			f32 x = center - LoadHalfWidth(cells), y = (top - 6.0f) + title->mLoadOffset;
			DrawGsProportionalTextFormat(&gGame.mGs, LoadTextColor(title->mLoadAlpha), text, x, y, 0.0f, 21.0f, 21.0f);
			sprintf(text, "%s", LoadPromptText(0x95, 0x16));
			cells = strlen(text) / 2;
			DrawGsProportionalTextFormat(&gGame.mGs, LoadTextColor(title->mLoadAlpha), text, center - LoadHalfWidth(cells),
			                             (21.0f + top) + title->mLoadOffset, 0.0f, 21.0f, 21.0f);
#else
			u32 color = LoadTextColor(title->mLoadAlpha);
			f32 top = 8.0f + (356.0f - panelY), y = (top - 6.0f) + title->mLoadOffset;
			DrawLoadText(LoadPromptText(0x94, 0), 320.0f, y, color);
			DrawLoadText(LoadPromptText(0x95, 0), 320.0f, (21.0f + top) + title->mLoadOffset, color);
#endif
			lines = 2;
		} else {
			sprintf(text, "%s", LoadPromptText(0xbc, 0x12));
			showMessage = true;
		}
		break;
	case 3:
		sprintf(text, "%s", LoadPromptText(0xa8, 1));
		showMessage = true;
		break;
	case 5:
		lines = LoadStatusLines(title, 10, panelX, panelY);
		break;
	case 6:
		lines = LoadStatusLines(title, 11, panelX, panelY);
		break;
	case 8:
		sprintf(text, "%s", LoadPromptText(0xc1, 0x32));
		showMessage = true;
		break;
	case 11: {
		f32 x = 320.0f + panelX, y = LoadPromptY(title, panelY);
		if (title->mPhase == 0)
			lines = LoadStatusAt(title, 5, x, y);
		else
			lines = LoadStatusAt(title, 6, x, y);
		break;
	}
	case 12:
		lines = LoadStatusLines(title, 7, panelX, panelY);
		break;
	case 13: {
		f32 x = 320.0f + panelX, y = LoadPromptY(title, panelY);
		if (title->mPhase == 0)
			lines = LoadStatusAt(title, 7, x, y);
		else
			lines = LoadStatusAt(title, 9, x, y);
		break;
	}
	case 9:
	case 14:
#if defined(VERSION_JP)
		lines = DrawJapaneseCardErrorMessage(title->mChannel, title->mResult, 1, 320.0f + panelX, LoadPromptY(title, panelY), 21.0f,
		                                     title->mLoadAlpha);
#else
		lines = DrawCardErrorMessage(title->mChannel, title->mResult, 1, 320.0f + panelX, LoadPromptY(title, panelY), 21.0f,
		                             title->mLoadAlpha);
#endif
		break;
	case 15:
		lines = LoadStatusLines(title, 4, panelX, panelY);
		break;
	case 18:
		lines = LoadStatusLines(title, 1, panelX, panelY);
		break;
	case 19:
#if defined(VERSION_JP)
		lines = LoadStatusLines(title, 12, panelX, panelY);
#else
	{
		f32 x = 320.0f + panelX, y = LoadPromptY(title, panelY);
		if (title->mFormatCancelled == 1)
			lines = LoadStatusAt(title, 16, x, y);
		else
			lines = LoadStatusAt(title, 12, x, y);
	}
#endif
		break;
	case 20:
		lines = LoadStatusLines(title, 3, panelX, panelY);
		break;
	case 21:
		lines = LoadStatusLines(title, 8, panelX, panelY);
		break;
	case 22:
		lines = LoadStatusLines(title, 14, panelX, panelY);
		break;
	case 24: {
		f32 x = 320.0f + panelX, y = LoadPromptY(title, panelY);
		if (title->mPhase == 0)
			lines = LoadStatusAt(title, 4, x, y);
		break;
	}
	case 25:
		break;
	}
	if (lines <= 2)
		lines = 2;
	title->mOffset = -(lines - 2) * 21;
	if (showMessage) {
#if defined(VERSION_JP)
		u32 cells = strlen(text) / 2;
		DrawGsProportionalTextFormat(&gGame.mGs, LoadTextColor(title->mLoadAlpha), text, (320.0f + panelX) - LoadHalfWidth(cells),
		                             LoadPromptY(title, panelY), 0.0f, 21.0f, 21.0f);
#else
		f32 y = LoadPromptY(title, panelY);
		DrawLoadText(text, 320.0f, y, LoadTextColor(title->mLoadAlpha));
#endif
	}

	switch (title->mMode) {
	case 3:
	case 4:
	case 5:
	case 6:
	case 11: {
		f32 extra = 0;
		if (lines > 1)
			extra = (lines - 2) * 21;
#if defined(VERSION_JP)
		sprintf(text, "%s", LoadPromptText(0, 0x33));
		u32 cells = strlen(text) / 2;
		f32 barX  = title->mChoice == 0 ? ((32.0f + (320.0f + panelX)) - LoadHalfWidth(cells)) - 2.0f
		                                : 60.0f + (10.0f + ((32.0f + (320.0f + panelX)) - LoadHalfWidth(cells)));
		f32 width = 56.699997f;
#else
		f32 barX = title->mChoice == 0 ? 281.25f : 358.75f, width = 61.5f;
#endif
		f32 base = (24.0f + (12.0f + (356.0f - panelY))) - 6.0f;
		{
			CTexObj tex;
			f32 alpha = title->mLoadAlpha, scroll = title->mLoadScroll;
			f32 barY = 8.0f + (extra + (base + title->mLoadOffset)), inset = 2.0f;
			u32 barColor = 0x181CC600 | (u32)(200.0f * alpha);
			InitMenuSelectionBar(tex);
			DrawMenuSelectionBar(tex, barX, barY + inset, width, 22.0f, alpha, scroll, barColor);
		}
		f32 y = extra + (base + title->mLoadOffset);
#if defined(VERSION_JP)
		DrawGsProportionalTextFormat(&gGame.mGs, LoadTextColor(title->mLoadAlpha), text, 10.0f + ((320.0f + panelX) - LoadHalfWidth(cells)),
		                             y, 0.0f, 21.0f, 21.0f);
#else
		if (HelpTextIsJapanese()) {
			DrawLoadChoice(title, 0x65, 281.25f, y);
			DrawLoadChoice(title, 0x66, 358.75f, y);
		} else {
			DrawLoadChoice(title, 0x65, 283.25f, y - 1.0f);
			DrawLoadChoice(title, 0x66, 360.75f, y - 1.0f);
		}
#endif
		break;
	}
	}
	if (title->mOptionAlpha > 0.0f) {
#if defined(VERSION_JP)
		sprintf(text, "%s", LoadPromptText(0, 0x36));
		{
			u32 cells = strlen(text) / 2;
			DrawGsProportionalTextFormat(&gGame.mGs, LoadTextColor(title->mOptionAlpha), text, (320.0f + panelX) - LoadHalfWidth(cells),
			                             LoadPromptY(title, panelY), 0.0f, 21.0f, 21.0f);
		}
#else
		{
			u32 color = LoadTextColor(title->mOptionAlpha);
			DrawLoadText(GetLocalizedInterfaceText(0x61), 320.0f, LoadPromptY(title, panelY), color);
		}
#endif
	}
}

/**
 * @note Address: 0x80029760
 * @note Size: 0x69C
 */
// NONMATCHING: retail converts alpha to an integer before the window frame (US 0x800297BC fctiwz, 0x800297C8
// lwz r27) and forms lit from it right after the call (0x80029824 rlwimi r29, r27), next to normal's own
// conversion (0x80029814); this build forms lit whole before the call. A single-use s32, int or u8 local
// (explicit or implicit conversion, from alpha or from 255.0f * title->mOptionAlpha, declared in the block
// or at function scope, initialised or assigned, lit as a second CSE'd use) is sunk past the call and merged
// with normal's conversion (93.07-93.85). Only (u8)a | 0xFFFFFF00 written at every use keeps the conversion
// before the call, and then the OR lands at the first ternary, not after the call (94.13). The rest is float
// register order
// draws the options: a window, its title and the three lines, the picked one lit once the
// window is open. the back line bobs while the cursor plays its anim
extern "C" void DrawTitleOptions(Title* title)
{
	f32 scale = title->mOptionAlpha;
	if (scale > 0.0f) {
		f32 alpha  = 255.0f * scale;
		f32 h      = 224.0f * scale;
		u32 lit    = (u8)(s32)alpha | 0xFFFFFF00;
		u32 window = (u32)alpha | 0xFFFFFF00;
		f32 top;
		DrawTexturedWindowFrame((u32)(160.0f * scale), window, 320.0f, top = 224.0f - h / 2.0f, 320.0f * scale, h, 1.0f, 1.0f, 1.0f);

		u32 normal = (u8)alpha | 0xFFFFFF00;
		const char* text;
#if !defined(VERSION_PAL)
#if defined(VERSION_JP)
		text = GetTitleAndSaveMenuText(0x35);
#else
		GetLocalizedInterfaceText(0x60);
#endif
		f32 titleSize = 24.0f * title->mOptionAlpha;
#if !defined(VERSION_JP)
		text = GetLocalizedInterfaceText(0x60);
#endif
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, 320.0f - MeasureGameTextWidth(&gGame.mGs, text, 1, titleSize) / 2.0f,
		                             titleSize + top, 0.0f, titleSize, titleSize);
#endif
		f32 size = 24.0f * title->mOptionAlpha;
#if defined(VERSION_PAL)
		top = (224.0f - (4.0f * (size * 0.5f) * 0.5f - size * 0.25f)) - h / 2.0f;
#endif
		u32 color = IsLineLit(title, 0) ? lit : normal;
#if defined(VERSION_JP)
		text     = GetLocalizedInterfaceText(0x49);
		f32 half = size / 2.0f;
		f32 left = 320.0f - 9.0f * half;
		f32 y    = size + (4.0f * half + top);
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, size + left, y, 0.0f, size, size);
		if (title->mRumble) {
			text = GetLocalizedInterfaceText(0x4a);
			DrawGsProportionalTextFormat(&gGame.mGs, normal, text, size + (5.0f * size + left), y, 0.0f, size, size);
		} else {
			text = GetLocalizedInterfaceText(0x4b);
			DrawGsProportionalTextFormat(&gGame.mGs, normal, text, half + (5.0f * size + left), y, 0.0f, size, size);
		}
		color = IsLineLit(title, 1) ? lit : normal;
		text  = GetLocalizedInterfaceText(0x4c);
		y     = size + (4.0f * (2.0f * half) + top);
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, left, y, 0.0f, size, size);
		text = title->mMono ? GetLocalizedInterfaceText(0x4d) : GetLocalizedInterfaceText(0x4e);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, 5.0f * size + left, y, 0.0f, size, size);
#else
		text  = GetLocalizedInterfaceText(0x10);
		f32 x = 320.0f - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f - 8.0f;
		f32 y;
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f,
		                             y = size + (4.0f * (size / 2.0f) + top), 0.0f, size, size);
		text = title->mRumble ? GetLocalizedInterfaceText(0x11) : GetLocalizedInterfaceText(0x12);
		x    = 8.0f + (MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f + 320.0f);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, x - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f, y, 0.0f, size,
		                             size);

		color = IsLineLit(title, 1) ? lit : normal;
		text  = GetLocalizedInterfaceText(0x13);
		x     = 320.0f - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f - 8.0f;
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f,
		                             y = size + (4.0f * (2.0f * (size / 2.0f)) + top), 0.0f, size, size);
		text = title->mMono ? GetLocalizedInterfaceText(0x14) : GetLocalizedInterfaceText(0x15);
		x    = 8.0f + (MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f + 320.0f);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, x - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f, y, 0.0f, size,
		                             size);
#endif

		f32 progress = title->mCursor.mAnimId == 1 ? GetAnimProgress(title) : 0.0f;
		f32 bob      = title->mOptionLine == 2 ? 2.0f * (f32)sin(4.712389f * progress) : 0.0f;
		f32 backSize = 24.0f + bob;
		if (IsLineLit(title, 2)) {
			normal = lit;
		}
#if defined(VERSION_JP)
		text = GetLocalizedInterfaceText(0x4f);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, 320.0f - 3.0f * half,
		                             (backSize - size) / 2.0f + ((2.0f * size + (4.0f * (3.0f * half) + top)) - backSize), 0.0f, size,
		                             backSize);
#else
		text = GetLocalizedInterfaceText(0x16);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, 320.0f - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f,
		                             (backSize - size) / 2.0f + ((2.0f * size + (4.0f * (3.0f * (size / 2.0f)) + top)) - backSize), 0.0f,
		                             size, size);
#endif
	}
}

/**
 * @note Address: 0x80029168
 * @note Size: 0x5F8
 */
// the options: up and down pick rumble, sound or back, A changes the line, B goes back. the
// rumble test runs the motor for 24 frames
extern "C" void UpdateTitleOptions(Title* title, u32 frames)
{
	CPad pad(0);

	title->mCursor.UpdateObject(frames);
	title->mHighlightDelay += frames;
	if (title->mHighlightDelay >= FRAME_RATE) {
		title->mHighlightDelay = FRAME_RATE;
		title->mPulseAngle     = fmod(title->mPulseAngle + frames * 9, 360.0);
	}
	// the game flags keep the settings: 0x800000000 is rumble on, 0x1000000000 mono sound
	if (!pad.mPad->mConnected && title->mRumble) {
		title->mRumble = FALSE;
		gGame.mStatus.mFlags &= ~0x800000000ULL;
	}

	switch (title->mOptionState) {
	case 0:
		if (pad.mPad->mRepeat & (HSD_PAD_STICK_UP | HSD_PAD_DPADUP)) {
			if (title->mOptionLine > 0) {
				title->mOptionLine--;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				title->mPulseAngle = 45.0f;
			}
		} else if (pad.mPad->mRepeat & (HSD_PAD_STICK_DOWN | HSD_PAD_DPADDOWN)) {
			if (title->mOptionLine < 2) {
				title->mOptionLine++;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				title->mPulseAngle = 45.0f;
			}
		}
		if (pad.mPad->mTrigger & 0x100) {
			switch (title->mOptionLine) {
			case 0:
				if (pad.mPad->mConnected) {
					if (title->mRumble) {
						title->mRumble = FALSE;
						gGame.mStatus.mFlags &= ~0x800000000ULL;
					} else {
						title->mRumble = TRUE;
						gGame.mStatus.mFlags |= 0x800000000ULL;
						if (pad.mPad->mConnected) {
							HSD_PadRumbleOn(pad.mPad->mPort);
						}
						title->mOptionState = 1;
						title->mRumbleTimer = 24;
					}
					PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				} else {
					PlayGameSound(&gGame.mSound, 0x85, 0xFF);
				}
				break;
			case 1:
				if (title->mMono) {
					title->mMono = FALSE;
					gGame.mStatus.mFlags &= ~0x1000000000ULL;
					SetGameSoundStereo(&gGame.mSound, FALSE);
				} else {
					title->mMono = TRUE;
					gGame.mStatus.mFlags |= 0x1000000000ULL;
					SetGameSoundStereo(&gGame.mSound, TRUE);
				}
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				break;
			case 2:
				title->mOptionState = 2;
				title->mCursor.SetAnim(1, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				title->mPulseAngle = 90.0f;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				break;
			}
		} else if (pad.mPad->mTrigger & 0x200) {
			title->mOptionState = 2;
			title->mCursor.SetAnim(1, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			title->mPulseAngle = 90.0f;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
		break;
	case 1:
		if (--title->mRumbleTimer < 0) {
			title->mOptionState = 0;
			if (pad.mPad->mConnected) {
				HSD_PadRumbleOffN(pad.mPad->mPort);
			}
			if (pad.mPad->mConnected) {
				HSD_PadRumbleRemove(pad.mPad->mPort);
			}
		}
		break;
	case 2:
		HSD_AObj* aobj = GetAObj(title->mCursor.mJObj);
		if ((aobj->flags & (AOBJ_NO_ANIM | AOBJ_REWINDED)) || title->mCursor.mAnimId == 0 || title->mCursor.mArchive.mNum[0] < 2) {
			title->mCursor.SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			title->mPulseAngle = 0.0f;
			title->mState      = 11;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			title->mTargetOptionAlpha = 0.0f;
			title->mOptionState       = 0;
			title->mOptionLine        = 0;
			title->mTargetPanelY      = -200.0f;
			title->mOffset            = 0.0f;
			title->mAlpha             = 0.0f;
			HideName(title);
		}
		break;
	}
}

/**
 * @note Address: 0x80028BAC
 * @note Size: 0x5BC
 */
// NONMATCHING: the volatile float registers and one product's place differ: after the alpha conversion retail takes f0/f4
// for the rot and pop products (0x80028F10), this build f1/f5/f0, and retail multiplies 1.7 * pop behind the width
// conversion's lfd (0x80028FB4), this build ahead of it. The locals declared at the top, alpha first, give retail's saved
// registers; inside the loop i and the colour mask swapped r25/r26 and the floats came out in another order
// draws the title's name: a bubble per letter, then the letter with a shadow on four sides
extern "C" void DrawTitleNameBubbles(Title* title)
{
	if (title->mName.mActive) {
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		CTexObj bubble = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[108 - StatusTextureOffset], 0);
		u32 alpha;
		f32 rot;
		f32 pop;
		f32 y;
		f32 x;
		f32 scale;
		f32 bob;
		f32 tilt;
		f32 swell;
		f32 zero  = 0.0f;
		u32 color = title->mName.mColor & 0xFFFFFF00;
		for (s32 i = 0; i < title->mName.mCount; i++) {
			scale = title->mName.mChars[i].mScale;
			if (scale <= zero) {
				continue;
			}
#if !defined(VERSION_JP)
			if (strncmp(title->mName.mChars[i].mText, " ", 1) == 0) {
				continue;
			}
#endif
			CFont font;
			s32 width;
			CTexObj letter = *GetFontGlyphTexture(&font, title->mName.mChars[i].mText, &width);
			bob            = 4.0f * std::sinf(title->mName.mChars[i].mBob);
			tilt           = std::sinf(0.7f * title->mName.mChars[i].mTilt);
			swell          = std::sinf(PI * scale);
			pop            = 0.2f * swell + std::sinf(HALF_PI * scale);
#if defined(VERSION_PAL)
			x = 20.0f + (40.0f * i + (320.0f - 40.0f * title->mName.mCount / 2.0f));
#else
			x = 24.0f + (48.0f * i + (320.0f - 48.0f * title->mName.mCount / 2.0f));
#endif
			alpha = 255.0f * scale;
			rot   = -(DEG2RAD * (10.0f * tilt));
			y     = 64.0f + bob;
			DrawGsTexturedQuad(&gGame.mGs, &bubble, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, x, y,
			                   0.0f, 20.0f * (2.0f * pop), 20.0f * (2.0f * pop), zero, -zero, rot);
			f32 size   = 20.0f * (1.7f * pop);
			f32 shrink = width / 24.0f * size;
			f32 off    = size - shrink;
			for (s32 j = 0; j < 4; j++) {
				DrawGsTexturedQuad(&gGame.mGs, &letter, alpha, alpha, alpha, alpha, off / 2.0f + (x + gTitleNameOutlineX[j]),
				                   bob + (64.0f + gTitleNameOutlineY[j]), 0.0f, size, size, zero, -zero, rot);
			}
			DrawGsTexturedQuad(&gGame.mGs, &letter, color | alpha, color | alpha, color | alpha, color | alpha, off / 2.0f + x, y, 0.0f,
			                   size, size, zero, -zero, rot);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
}

// NONMATCHING: the loop's floats are numbered the other way round: retail keeps bob in f31 and x in f14
// (0x8002892C fmuls f31, 0x800289D8 fadds f14), ours x in f31 and bob in f14
extern "C" void DrawTitleHelpText(Title* menu)
{
	f32 alpha   = menu->mMenuAlpha * menu->mHelpAlpha;
	f32 spacing = 30.0f;
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	CTexObj blob     = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[109 - StatusTextureOffset], 0);
	menu->mHelpCount = 0;
	const char* text = menu->mHelpText;
	if (text) {
		const int limit = 20;
		for (int i = 0; i < limit && *text; i++, text++) {
			menu->mBg[i].mText = text;
			menu->mHelpCount++;
			if (HelpTextIsJapanese() && (IsSjisLead(*text) & 1))
				text++;
		}
	}
	f32 zero    = 0.0f;
	f32 opacity = 255.0f * alpha;
	for (int i = 0; i < menu->mHelpCount; i++) {
		MenuHelpChar* glyph = &menu->mBg[i];
		s32 width;
		CFont font;
		CTexObj tex  = *GetFontGlyphTexture(&font, glyph->mText, &width);
		f32 bob      = 2.0f * (f32)sin(glyph->mAngle0);
		f32 rotation = sin(0.7f * glyph->mAngle1);
		f32 swell    = sin(PI * alpha);
		f32 grow     = sin(HALF_PI * alpha);
		f32 scale    = 0.2f * swell + grow;
		f32 x        = spacing / 2.0f + (i * spacing + (320.0f - menu->mHelpCount * spacing / 2.0f));
		f32 y        = 384.0f + bob;
		rotation     = -(DEG2RAD * (10.0f * rotation));
		u32 color    = (u32)opacity | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &blob, color, color, color, color, x, y, 1.0f, 30.0f * scale, 30.0f * scale, 0.0f, -zero, rotation);
		scale       = 30.0f * (0.8f * scale);
		f32 padding = HelpGlyphPadding(width, scale);
		for (int j = 0; j < 4; j++) {
			DrawGsTexturedQuad(&gGame.mGs, &tex, opacity, opacity, opacity, opacity, x + gTitleHelpOutlineX[j] + padding / 2.0f,
			                   bob + (384.0f + gTitleHelpOutlineY[j]), 1.0f, scale, scale, 0.0f, -zero, rotation);
		}
		color = (u32)opacity | 0xFFFFFB00;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x + padding / 2.0f, y, 1.0f, scale, scale, 0.0f, -zero, rotation);
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

/**
 * @note Address: 0x800284E0
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
 * @note Address: 0x8002840C
 * @note Size: 0xD4
 */
// XObjDemo's LoadFromPlacement, builds the camera and the light list from the archive
BOOL XObjDemo::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		if (mArchive.mDesc[0] != NULL) {
			mCamera.Init((HSD_CObjDesc*)mArchive.mDesc[0]->mObjectDesc);
#if defined(VERSION_PAL)
			mCamera.SetViewport(0.0f, 0.0f, &gGame.mGs.mScreen);
#endif
		}
		HSD_LObj* prev = NULL;
		for (s32 i = 0; i < 8; ++i) {
			if (GetLightDesc(i) != NULL) {
				mLights[i].Init((HSD_LightDesc*)GetLightDesc(i)->mObjectDesc, FALSE);
				mLightCount++;
				if (prev != NULL) {
					prev->next = &mLights[i];
				}
				prev = &mLights[i];
			}
		}
		if (prev != NULL) {
			prev->next = NULL;
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80028368
 * @note Size: 0xA4
 */
// XObjDemo's Release
void XObjDemo::Release()
{
	if (mArchive.mDesc[0] != NULL) {
		mCamera.Release();
	}
	for (s32 i = 0; i < mLightCount; ++i) {
		if (GetLightDesc(i) != NULL) {
			mLights[i].Release();
		}
	}
	XObj::Release();
}

/**
 * @note Address: 0x80028304
 * @note Size: 0x38
 */
void CCamera::SetAnimRate(f32 rate)
{
	HSD_ForeachAnim(this, COBJ_TYPE, (HSD_TypeMask)(COBJ_MASK | WOBJ_MASK), HSD_AObjSetRate, AOBJ_ARG_AF, rate);
}

/**
 * @note Address: 0x800282CC
 * @note Size: 0x38
 */
void CLight::SetAnimRate(f32 rate)
{
	HSD_ForeachAnim(this, LOBJ_TYPE, (HSD_TypeMask)(LOBJ_MASK | WOBJ_MASK), HSD_AObjSetRate, AOBJ_ARG_AF, rate);
}

/**
 * @note Address: 0x800280B4
 * @note Size: 0x244
 */
// XObjDemo's update, steps the camera and light anims with the model
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
	if (mArchive.mDesc[0] != NULL && mCameraAnim >= 0) {
		mCamera.SetAnimRate((f32)frames);
		mCamera.Anim();
	}
	for (s32 i = 0; i < mLightCount; ++i) {
		if (GetLightDesc(i) != NULL && mLightAnims[i] >= 0) {
			mLights[i].SetAnimRate((f32)frames);
			mLights[i].Anim();
		}
	}
	if (mLoop) {
		if (GetAObj(mJObj)->flags & (AOBJ_NO_ANIM | AOBJ_REWINDED)) {
			mState = 1;
			SetPos(mMoveTranslate, 0);
			SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
	}
}

/**
 * @note Address: 0x80028000
 * @note Size: 0xB4
 */
// XObjDemo's DrawObject, draws through its own camera and lights when it has them
void XObjDemo::DrawObject()
{
	if (mState != 0) {
		CCamera* camera = mArchive.mDesc[0] != NULL ? &mCamera : &gGame.mCameras[0];
		CLight* light   = mArchive.mDesc[1] != NULL ? &mLights[0] : &gGame.mLights[0];
		light->SetCurrent(camera);
		if (camera->SetCurrent() != NULL) {
			DrawWithView(NULL);
			camera->EndCurrent();
		}
	}
}

/**
 * @note Address: 0x80027E88
 * @note Size: 0x178
 */
// CXObj's SetScale(v)
void CXObj::SetScale(const Vec& scale, s32 force)
{
	mMoveScale.Set(scale);
	mMoveScale.mTarget.Set(scale);
	if (mJObj) {
		CJObj::SetScale(scale, force);
	}
}

/**
 * @note Address: 0x80027CFC
 * @note Size: 0x18C
 */
// CXObj's SetScale(x, y, z)
void CXObj::SetScale(f32 x, f32 y, f32 z, s32 force)
{
	mMoveScale.x         = x;
	mMoveScale.y         = y;
	mMoveScale.z         = z;
	mMoveScale.mTarget.x = x;
	mMoveScale.mTarget.y = y;
	mMoveScale.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mScale != v)) {
				return;
			}
		}
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
}

/**
 * @note Address: 0x80027B70
 * @note Size: 0x18C
 */
// CXObj's SetScale(scale)
void CXObj::SetScale(f32 scale, s32 force)
{
	f32 x = scale;
	f32 y = scale;
	f32 z = scale;

	mMoveScale.x         = x;
	mMoveScale.y         = y;
	mMoveScale.z         = z;
	mMoveScale.mTarget.x = x;
	mMoveScale.mTarget.y = y;
	mMoveScale.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mScale != v)) {
				return;
			}
		}
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
}

/**
 * @note Address: 0x80027A98
 * @note Size: 0xD8
 */
// CXObj's SetScaleY
void CXObj::SetScaleY(f32 value, s32 force)
{
	mMoveScale.y         = value;
	mMoveScale.mTarget.y = value;
	if (mJObj != NULL) {
		CJObj::SetScaleY(value, force);
	}
}

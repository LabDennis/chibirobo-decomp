#include "game/wii/infoscrn.h"
#include "game/infopoint.h"
#include "game/globals.h"
#include "game/gmath.h"
#include "game/wii/cgame.h"
#include "game/wii/cpad.h"
#include "game/wii/csound.h"
#include "game/wii/title.h"
#include <string.h>
#include <math.h>

// the info screen on the Wii, the hud drawn over the game: the clock, the battery, the
// happy points and the three counters that pop up in the corner. CGame keeps it in
// mInfoScrn. Ported from the GameCube's game/infoscrn.cpp: the status flags move behind
// CGame::GetStatusFlags, the timer runs off gRefreshRate instead of a fixed rate, the
// object gauge gains a parameter and the counters keep the same layout.
//
// todo: DrawHudBatteryGauge, DrawHudClock, DrawHudGaugeIcon, InfoScrn_DrawPlayerHudIcon,
// InfoScrn_DrawTimer, DrawHudScanlines, DrawTexturedWindowFrame,
// UpdateHudPointValue, DrawHudPointCounter

// the timer counts frames at the video mode's rate, 50 or 60 a second
#define TIMER_RATE gRefreshRate
// 99:59.99, the most the timer shows
#define TIMER_MAX ((s32)((u32)TIMER_RATE * 6000U - 1U))

extern "C" {
s32 gRefreshRate;
s32 gScreenSafeLeft;
s32 gScreenSafeBottom;
s32 gPointerScreenWidth;
void DrawHudPointCounter(InfoPoint* point);
void DrawHudScanlines(InfoScrn* scrn);
void InfoScrn_DrawTimer(InfoScrn* scrn);
void InfoScrn_DrawPlayerHudIcon(InfoScrn* scrn);
void DrawHudClock(InfoScrn* scrn);
void DrawHudGaugeIcon(InfoScrn* scrn);
void DrawHudBatteryGauge(InfoScrn* scrn);
void DrawGameHud(InfoScrn* scrn);
void UpdateHudIndicators(InfoScrn* scrn, u32 frames);
}

// todo: the three counters are infopointinit.cpp's, which is cut from this file. These are
// the file's own copies, as on the GameCube, so that its code reaches them the way retail does
static InfoPoint gHudMoneyCounter;
static InfoPoint gHudHappyPointCounter;
static InfoPoint gHudScrapCounter;
extern "C" {
f32 gHudScanlineHistory[10];
}

// how bright the hud is, the counters and gauges multiply their alpha by it
f32 gHudOpacityScale = 1.0f;

// the y the bottom row is drawn at, the wipe's own bottom edge clamped to the safe area
static inline f32 GetHudBottomEdge()
{
	f32 bottom = gGame.mWipeCine.mHeight / 2 + gGame.mWipeCine.mY;
	if (bottom >= (f32)gScreenSafeBottom) {
		bottom = (f32)gScreenSafeBottom;
	}
	return bottom;
}

// frames to a clock reading, minutes * 6000 + seconds * 100 + hundredths
static inline f32 FramesToClock(f32 frames)
{
	f32 seconds    = frames / (f32)TIMER_RATE;
	s32 hundredths = ((s32)frames % TIMER_RATE) * 100 / TIMER_RATE;
	if (frames >= (f32)TIMER_MAX) {
		hundredths = 99;
	}
	f32 minutes = seconds / 60.0f;
	return hundredths + (6000.0f * (s32)minutes + ((s32)seconds % 60) * 100);
}

// how many items chibi robo can carry, by the upgrade he holds
static inline s32 ItemMax()
{
	WiiPlayerState* player = gGame.GetPlayer();
	for (int i = 0; i < 256; i++) {
		InventoryItem* items = player->mParams.mInventory.mItems;
		s16 id               = items[i].mId;
		s16 count            = items[i].mCount;
		if (id >= 0 && count > 0) {
			switch (id) {
			case 0x57:
				return 5;
			case 0x59:
				return 10;
			case 0x5A:
				return 15;
			}
		}
	}
	return 5;
}

// move a value at most step toward the target, frozen while the game is paused
static inline void Approach(f32* cur, f32* target, f32 step)
{
	if (gGame.GetStatusFlags(0x200000000000000ULL)) {
		return;
	}
	if (*cur > *target) {
		*cur -= step;
		if (*cur < *target) {
			*cur = *target;
		}
	}
	if (*cur < *target) {
		*cur += step;
		if (*cur > *target) {
			*cur = *target;
		}
	}
}

/**
 * @note Address: 0x800870E8
 * @note Size: 0x270
 */
extern "C" void InitInfoScreen(InfoScrn* scrn)
{
	scrn->mFade.Reset();
	scrn->mWipe.Reset();
	scrn->_9C              = 0;
	scrn->mHudAlpha        = 0.0f;
	scrn->mAttachmentAlpha = 0.0f;
	scrn->mMenuHudAlpha    = 0.0f;
	scrn->mDimAlpha        = 0.0f;
	scrn->mTargetDimAlpha  = 0.0f;
	scrn->mHudState        = 0;
	scrn->mAttachmentState = 0;
	scrn->mMenuHudState    = 0;
	scrn->mFrameCount      = 0;
	scrn->mBatteryGauge    = 0.0f;
	scrn->_BC              = 0;
	scrn->mIconFrameTimer  = 0;
	scrn->mIconFrame       = 0;
	InitHudPointCounter(&gHudHappyPointCounter, INFOPOINT_ID + 2);
	InitHudPointCounter(&gHudMoneyCounter, INFOPOINT_ID);
	InitHudPointCounter(&gHudScrapCounter, INFOPOINT_ID + 1);
	gHudHappyPointCounter.mVisible = FALSE;
	gHudMoneyCounter.mVisible      = FALSE;
	gHudScrapCounter.mVisible      = FALSE;
	scrn->mBatteryState            = 0;
	scrn->mBatteryAlpha            = 0.0f;
	scrn->mBatteryOffsetY          = 0.0f;
	scrn->mTargetBatteryOffsetY    = 0.0f;
	const Vec v0                   = { 0.0f, 0.0f, 0.0f };
	scrn->_12C.x                   = v0.x;
	scrn->_12C.y                   = v0.y;
	scrn->_12C.z                   = v0.z;
	scrn->mClockState              = 0;
	scrn->mClockAlpha              = 0.0f;
	scrn->mTimeWarningBeeps        = 0;
	scrn->mTimeWarningTick         = 0;
	scrn->mTimeWarningBeeped       = 0;
	const Vec v1                   = { 0.0f, 0.0f, 0.0f };
	scrn->_174.x                   = v1.x;
	scrn->_174.y                   = v1.y;
	scrn->_174.z                   = v1.z;
	scrn->mClockScale              = 1.0f;
	scrn->mObjectGaugeState        = 0;
	scrn->mObjectGaugeParam        = 0;
	scrn->mObjectGaugeAlpha        = 0.0f;
	const Vec v2                   = { 0.0f, 0.0f, 0.0f };
	scrn->mObjectGaugePos.x        = v2.x;
	scrn->mObjectGaugePos.y        = v2.y;
	scrn->mObjectGaugePos.z        = v2.z;
	scrn->mGaugeTarget             = NULL;
	scrn->mPlayerIconState         = 0;
	scrn->mPlayerIconAlpha         = 0.0f;
	const Vec v3                   = { 0.0f, 0.0f, 0.0f };
	scrn->_1B0.x                   = v3.x;
	scrn->_1B0.y                   = v3.y;
	scrn->_1B0.z                   = v3.z;
	scrn->mTimerState              = 0;
	scrn->mTimerAlpha              = 0.0f;
	scrn->mTimer                   = 0.0f;
	scrn->mTimerShown              = 0;
	scrn->mScanlineVisibilityState = 0;
	scrn->mScanlineAlpha           = 0.0f;
	scrn->_1D4                     = 0.0f;
	scrn->_188                     = 0;
	scrn->mSelectedSaveSlot        = 0;
	scrn->mBatteryValue            = 0.0f;
	scrn->mBatteryTarget           = 0.0f;
	scrn->mBatteryEmptyTimer       = 0;
	scrn->mBatteryEmptyStep        = 0;
	scrn->mBatteryScale            = 1.4f;
	f32 d                          = scrn->mBatteryValue - scrn->mBatteryTarget;
	for (int i = 0; i < 4; i++) {
		scrn->mDigitRoll[i]      = 0.0f;
		scrn->mDigitValue[i]     = 0;
		scrn->mDigitRolling[i]   = 0;
		scrn->mDigitDelta[i]     = d;
		scrn->mDigitLastDelta[i] = 0.0f;
	}
	scrn->mBatteryEmpty          = 0;
	scrn->mBatteryDim            = 0;
	scrn->mBatteryDimAlpha       = 0.0f;
	scrn->mTargetBatteryDimAlpha = 0.0f;
	gHudOpacityScale             = 1.0f;
	scrn->mItemMax               = 5;
	scrn->_1E4.x = scrn->_1E4.y = scrn->_1E4.z = 0.0f;
}

/**
 * @note Address: 0x80087358
 * @note Size: 0x2C
 */
extern "C" BOOL HideHudPointCounters(InfoScrn* scrn)
{
	gHudHappyPointCounter.mVisible = FALSE;
	gHudMoneyCounter.mVisible      = FALSE;
	gHudScrapCounter.mVisible      = FALSE;
	return TRUE;
}

/**
 * @note Address: 0x80087384
 * @note Size: 0x24C
 */
extern "C" void ResetGameHudState(InfoScrn* scrn)
{
	scrn->_9C              = 0;
	scrn->mHudAlpha        = 0.0f;
	scrn->mAttachmentAlpha = 0.0f;
	scrn->mMenuHudAlpha    = 0.0f;
	scrn->mDimAlpha        = 0.0f;
	scrn->mTargetDimAlpha  = 0.0f;
	scrn->mHudState        = 0;
	scrn->mAttachmentState = 0;
	scrn->mMenuHudState    = 0;
	scrn->mFrameCount      = 0;
	scrn->mBatteryGauge    = 0.0f;
	scrn->_BC              = 0;
	scrn->mIconFrameTimer  = 0;
	scrn->mIconFrame       = 0;
	InitHudPointCounter(&gHudHappyPointCounter, INFOPOINT_ID + 2);
	InitHudPointCounter(&gHudMoneyCounter, INFOPOINT_ID);
	InitHudPointCounter(&gHudScrapCounter, INFOPOINT_ID + 1);
	gHudHappyPointCounter.mVisible = FALSE;
	gHudMoneyCounter.mVisible      = FALSE;
	gHudScrapCounter.mVisible      = FALSE;
	scrn->mBatteryState            = 0;
	scrn->mBatteryAlpha            = 0.0f;
	scrn->mBatteryOffsetY          = 0.0f;
	scrn->mTargetBatteryOffsetY    = 0.0f;
	const Vec v0                   = { 0.0f, 0.0f, 0.0f };
	scrn->_12C.x                   = v0.x;
	scrn->_12C.y                   = v0.y;
	scrn->_12C.z                   = v0.z;
	scrn->mClockState              = 0;
	scrn->mClockAlpha              = 0.0f;
	scrn->mTimeWarningBeeps        = 0;
	scrn->mTimeWarningTick         = 0;
	scrn->mTimeWarningBeeped       = 0;
	const Vec v1                   = { 0.0f, 0.0f, 0.0f };
	scrn->_174.x                   = v1.x;
	scrn->_174.y                   = v1.y;
	scrn->_174.z                   = v1.z;
	scrn->mClockScale              = 1.0f;
	scrn->mObjectGaugeState        = 0;
	scrn->mObjectGaugeParam        = 0;
	scrn->mObjectGaugeAlpha        = 0.0f;
	const Vec v2                   = { 0.0f, 0.0f, 0.0f };
	scrn->mObjectGaugePos.x        = v2.x;
	scrn->mObjectGaugePos.y        = v2.y;
	scrn->mObjectGaugePos.z        = v2.z;
	scrn->mGaugeTarget             = NULL;
	scrn->mPlayerIconState         = 0;
	scrn->mPlayerIconAlpha         = 0.0f;
	const Vec v3                   = { 0.0f, 0.0f, 0.0f };
	scrn->_1B0.x                   = v3.x;
	scrn->_1B0.y                   = v3.y;
	scrn->_1B0.z                   = v3.z;
	scrn->mTimerState              = 0;
	scrn->mTimerAlpha              = 0.0f;
	scrn->mTimer                   = 0.0f;
	scrn->mTimerShown              = 0;
	scrn->mScanlineVisibilityState = 0;
	scrn->mScanlineAlpha           = 0.0f;
	scrn->_1D4                     = 0.0f;
	scrn->_188                     = 0;
	scrn->mSelectedSaveSlot        = 0;
	scrn->mBatteryValue            = 0.0f;
	scrn->mBatteryTarget           = 0.0f;
	scrn->mBatteryEmptyTimer       = 0;
	scrn->mBatteryEmptyStep        = 0;
	scrn->mBatteryScale            = 1.4f;
	f32 d                          = scrn->mBatteryValue - scrn->mBatteryTarget;
	for (int i = 0; i < 4; i++) {
		scrn->mDigitRoll[i]      = 0.0f;
		scrn->mDigitValue[i]     = 0;
		scrn->mDigitRolling[i]   = 0;
		scrn->mDigitDelta[i]     = d;
		scrn->mDigitLastDelta[i] = 0.0f;
	}
	scrn->mBatteryEmpty          = 0;
	scrn->mBatteryDim            = 0;
	scrn->mBatteryDimAlpha       = 0.0f;
	scrn->mTargetBatteryDimAlpha = 0.0f;
	gHudOpacityScale             = 1.0f;
	scrn->mItemMax               = 5;
	scrn->_1E4.x = scrn->_1E4.y = scrn->_1E4.z = 0.0f;
}

/**
 * @note Address: 0x800875D0
 * @note Size: 0x5C
 */
extern "C" void UpdateHudTransitionsAndIndicators(InfoScrn* scrn, u32 frames)
{
	UpdateScreenFade(&scrn->mFade, frames);
	scrn->mWipe.Update(frames);
	UpdateHudIndicators(scrn, frames);
}

/**
 * @note Address: 0x8008762C
 * @note Size: 0x20C
 */
extern "C" void DrawGameHud(InfoScrn* scrn)
{
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	BOOL visible = FALSE;
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	DrawScreenColorOverlay(&scrn->mFade, &gGame.mGs);
	scrn->mWipe.Draw(&gGame.mGs, 0);
	if (gGame.GetStatusFlags(1) && !gGame.GetStatusFlags(0x200000000000022ULL) && !gGame.mMenu.mActive) {
		BOOL diskFail = FALSE;
		if ((gGame.mDvdStatus || gGame.mPrevDvdStatus) && !gGame.mDiskOk) {
			diskFail = TRUE;
		}
		if (!diskFail && !gGame.mResetRequested) {
			visible = TRUE;
		}
	}
	if (visible || gGame.GetStatusFlags(0x200000000000000ULL)) {
		DrawHudPointCounter(&gHudHappyPointCounter);
		DrawHudPointCounter(&gHudMoneyCounter);
		DrawHudPointCounter(&gHudScrapCounter);
		DrawHudBatteryGauge(scrn);
		DrawHudClock(scrn);
		DrawHudGaugeIcon(scrn);
		InfoScrn_DrawPlayerHudIcon(scrn);
		InfoScrn_DrawTimer(scrn);
		DrawHudScanlines(scrn);
	}
}

// the loading bar, the robot walks along it
static const u8 sWalkAnim[]  = { 3, 2, 1, 0, 3, 1, 2, 0xFF };
static const u8 sStandAnim[] = { 0, 1, 2, 3, 0xFF };
static struct {
	f32 mX;          // _00
	s32 mFrame;      // _04
	const u8* mAnim; // _08
} sLoading = { 0.0f, 0, sWalkAnim };

// NONMATCHING: functionally complete Wii port; remaining diff is MWCC scheduling/register
// allocation around the count/loadIdx float conversions, the SetMode stores vs. quad arg
// setup, and one extra address materialization (593 vs 592 insns).
/**
 * @note Address: 0x80087838
 * @note Size: 0x944
 */
extern "C" void DrawObjectLoadingProgress(InfoScrn* scrn)
{
	if (gGame.GetStatusFlags(0x40)) {
		f32 leftX = (f32)(gPointerScreenWidth / 2);
		f32 y0    = (f32)(gScreenSafeBottom - 12);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
		if (gGame.mXObjs.mObjectCount > 0) {
			f32 fill;
			f32 max;
			f32 tw;
			f32 th;
			f32 scale;
			max   = gGame.mXObjs.mObjectCount;
			scale = 1.0f / gGame.mXObjs.mObjectCount;
			fill  = max * (gGame.mXObjs.mLoadObjectIndex * scale);
			CTexObj tex;
			if (sLoading.mAnim == sWalkAnim) {
				if (sWalkAnim[sLoading.mFrame] == 0xFF) {
					sLoading.mFrame = 0;
				}
				gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[0x83 + sWalkAnim[sLoading.mFrame]], 0);
				tex = gGame.mTpl.mTex;
			} else {
				if (sStandAnim[sLoading.mFrame / 2] == 0xFF) {
					sLoading.mFrame = 0;
				}
				gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[0x87 + sStandAnim[sLoading.mFrame / 2]], 0);
				tex = gGame.mTpl.mTex;
			}
			tw = GXGetTexObjWidth(&tex);
			th = GXGetTexObjHeight(&tex);
			gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_FRONT);
			gGame.mGs.mCurTevStage->SetMode(GX_BLEND);
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0x808080C8, 0x808080C8, 0x808080C8, 0x808080C8, sLoading.mX + (leftX - max * 0.5f),
			                   y0 - th * 0.5f, 0.0f, 0.9f * tw, 0.9f * th, 0.0f, 3.1415927f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, sLoading.mX + (leftX - max * 0.5f),
			                   y0 - th * 0.5f, 0.0f, 0.8f * tw, 0.8f * th, 0.0f, 3.1415927f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);

			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[118], 0);
			tex = gGame.mTpl.mTex;
			PSMTXScale(tex.mTexMtx.mMtx[2], 0.25f, 1.0f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.25f, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			f32 h              = 14.0f;
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 2.0f + leftX, 2.0f + y0, 0.0f, max, h,
			                   0.0f, 0.0f, 3.1415927f);
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.5f, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 2.0f + leftX - max * 0.5f - 4.0f,
			                   2.0f + y0, 0.0f, 8.0f, h, 0.0f, 0.0f, 3.1415927f);
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 4.0f + (2.0f + leftX + max * 0.5f),
			                   2.0f + y0, 0.0f, 8.0f, h, 0.0f, 0.0f, 3.1415927f);

			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[119], 0);
			tex      = gGame.mTpl.mTex;
			u16 barW = GXGetTexObjWidth(&tex);
			PSMTXScale(tex.mTexMtx.mMtx[2], fill / barW, 1.0f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			static f32 sScroll = 0.0f;
			sScroll -= 0.1f;
			PSMTXTrans(tex.mTexMtx.mMtx[0], sScroll, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0x9FFA1CC8, 0x9FFA1CC8, 0x9FFA1CC8, 0x9FFA1CC8, 1.0f + leftX - (max - fill) / 2, 2.0f + y0,
			                   0.0f, fill, h - 4.0f, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);

			sLoading.mFrame++;
			if ((sLoading.mX += 6.0f) >= max) {
				sLoading.mX = max;
				if (sLoading.mAnim == sWalkAnim) {
					sLoading.mAnim  = sStandAnim;
					sLoading.mFrame = 0;
				}
			}
		} else {
			sLoading.mX     = 0.0f;
			sLoading.mFrame = 0;
			sLoading.mAnim  = sWalkAnim;
		}
		const char* text = GetLocalizedInterfaceText(0x5A);
		f32 w            = MeasureGameTextWidth(&gGame.mGs, text, 1, 16.0f);
		DrawGsProportionalTextFormat(&gGame.mGs, -1, text, leftX - w / 2, y0 - 16.0f - 4.0f, 0.0f, 16.0f, 16.0f);
	}
}

// fade a hud element's alpha by its visibility state: 0 hides, 1 shows now, 2 hides now, 3 fades
// in, 4 fades out
static inline void StepHudVisibility(s32& state, f32& alpha, u32 frames)
{
	switch (state) {
	case 0:
		state = 4;
		alpha = 0;
		break;
	case 1:
		alpha = 1;
		break;
	case 2:
		alpha = 0;
		break;
	case 3:
		alpha += 0.08f * frames;
		if (alpha >= 1)
			alpha = 1;
		break;
	case 4:
		alpha -= 0.08f * frames;
		if (alpha < 0)
			alpha = 0;
		break;
	}
}

// the type of the attachment under the ring menu's cursor, 0 when none is picked
static inline s32 HighlightedAttachment()
{
	WiiPlayerState* player = gGame.GetPlayer();
	return player->mMenu.mAtc.mCursorIndex >= 0 ? player->mMenu.mAtc.mItems[player->mMenu.mAtc.mCursorIndex].mType : 0;
}

static inline void AdvanceHudIcon(InfoScrn* h)
{
	h->mIconFrameTimer++;
	if (h->mIconFrameTimer > 1) {
		h->mIconFrame++;
		if (h->mIconFrame >= 9)
			h->mIconFrame = 9;
		h->mIconFrameTimer = 0;
	}
}

static inline void ApproachHudScale(f32& current, f32 target)
{
	f32 step = (target - current) / 10;
	if (step < 0)
		step = -step;
	Approach(&current, &target, step);
}

// NONMATCHING: functionally complete Wii port, same instruction count on both sides (1411).
// The remaining diff is MWCC scheduling and register allocation over ~1.4k instructions, a
// larger stack frame for the CPad/CVec/path locals, and the object gauge's path array: retail
// passes a pointer into the next unit's rodata (0x802C6D0E), the build keeps it as a stack
// local like the GameCube did.
/**
 * @note Address: 0x8008817C
 * @note Size: 0x14DC
 */
extern "C" void UpdateHudIndicators(InfoScrn* h, u32 frames)
{
	// the GameCube called CGame::IsRunning here; the Wii spells the test out: the game-status
	// bit, no disc error flag pair and no failed disc
	BOOL running = FALSE;
	if (gGame.GetStatusFlags(1) && !gGame.GetStatusFlags(0x22)) {
		BOOL diskFail = FALSE;
		if ((gGame.mDvdStatus || gGame.mPrevDvdStatus) && !gGame.mDiskOk)
			diskFail = TRUE;
		if (!diskFail)
			running = TRUE;
	}
	if (!running && !gGame.GetStatusFlags(0x200000000000000ULL))
		return;
	UpdateHudPointValue(&gHudHappyPointCounter, frames);
	UpdateHudPointValue(&gHudMoneyCounter, frames);
	UpdateHudPointValue(&gHudScrapCounter, frames);
	++h->mFrameCount;
	switch (h->mHudState) {
	case 0:
		++h->mHudState;
		break;
	case 1:
		h->mHudAlpha += 0.08f * frames;
		if (h->mHudAlpha >= 1) {
			h->mHudAlpha = 1;
			++h->mHudState;
		}
		break;
	case 2:
		if (gGame.mEvt.mData.mScript)
			++h->mHudState;
		break;
	case 3:
		h->mHudAlpha -= 0.08f * frames;
		if (h->mHudAlpha < 0) {
			h->mHudAlpha = 0;
			++h->mHudState;
		}
		break;
	case 4:
		if (!gGame.mEvt.mData.mScript)
			h->mHudState = 1;
		break;
	}
	// whatever the state machine above did, the alpha ends up 1
	h->mHudAlpha           = 1;
	static s32 previousPad = -1; // never read, retail has it and its guard here
	CPad pad(0);                 // unused, retail builds one here
	switch (h->mAttachmentState) {
	case 0:
		if (gGame.GetPlayer()->GetAttachment()) {
			h->_BC = -1;
			++h->mAttachmentState;
		}
		break;
	case 1:
		h->mAttachmentAlpha += 0.08f * frames;
		if (h->mAttachmentAlpha >= 1) {
			h->mAttachmentAlpha = 1;
			++h->mAttachmentState;
		}
		break;
	case 2:
		if (h->_BC != -1) {
			++h->mAttachmentState;
		} else if (!gGame.GetPlayer()->GetAttachment()) {
			++h->mAttachmentState;
		}
		break;
	case 3:
		h->mAttachmentAlpha -= 0.08f * frames;
		if (h->mAttachmentAlpha < 0) {
			h->mAttachmentAlpha = 0;
			h->mAttachmentState = 0;
		}
		break;
	case 4:
		break;
	}
	static s32 previousAttachment = HighlightedAttachment();
	// up while chibi robo's menu rings are open (status 0x400); the icon animation starts over
	// whenever the highlighted attachment changes
	switch (h->mMenuHudState) {
	case 0:
		if (gGame.GetStatusFlags(0x400)) {
			++h->mMenuHudState;
			previousAttachment = HighlightedAttachment();
			h->mIconFrameTimer = 0;
			h->mIconFrame      = 0;
		}
		break;
	case 1:
		h->mMenuHudAlpha += 0.08f * frames;
		if (h->mMenuHudAlpha >= 1) {
			h->mMenuHudAlpha = 1;
			++h->mMenuHudState;
		}
		AdvanceHudIcon(h);
		break;
	case 2:
		if (previousAttachment != HighlightedAttachment()) {
			h->mIconFrameTimer = 0;
			h->mIconFrame      = 0;
		}
		previousAttachment = HighlightedAttachment();
		AdvanceHudIcon(h);
		if (!gGame.GetStatusFlags(0x400))
			++h->mMenuHudState;
		break;
	case 3:
		h->mMenuHudAlpha -= 0.08f * frames;
		if (h->mMenuHudAlpha < 0) {
			h->mMenuHudAlpha = 0;
			h->mMenuHudState = 0;
		}
		break;
	case 4:
		break;
	}
	// as StepHudVisibility, but 0 starts a fade in; 5 pulses the gauge's size, 6 brings it back to 1.4
	switch (h->mBatteryState) {
	case 0:
		h->mBatteryState = 3;
		h->mBatteryAlpha = 0;
		h->mBatteryScale = 1.4f;
		break;
	case 1:
		h->mBatteryAlpha = 1;
		break;
	case 2:
		h->mBatteryAlpha = 0;
		break;
	case 3:
		h->mBatteryAlpha += 0.08f * frames;
		if (h->mBatteryAlpha >= 1)
			h->mBatteryAlpha = 1;
		break;
	case 4:
		h->mBatteryAlpha -= 0.08f * frames;
		if (h->mBatteryAlpha < 0)
			h->mBatteryAlpha = 0;
		break;
	case 5:
		ApproachHudScale(h->mBatteryScale, 1.4f + 0.25f * (f32)sin(DEG2RAD * ((s32)((u32)h->mFrameCount * 16) % 360)));
		break;
	case 6:
		ApproachHudScale(h->mBatteryScale, 1.4f);
		break;
	}
	f32 step = (h->mTargetBatteryOffsetY - h->mBatteryOffsetY) / 4.0f;
	if (step < 0)
		step = -step;
	Approach(&h->mBatteryOffsetY, &h->mTargetBatteryOffsetY, step);
	Approach(&h->mDimAlpha, &h->mTargetDimAlpha, 0.08f * frames);
	// 0-4 as the battery's, the item max badge's alpha following; 5 fades the badge out and swaps in mNextItemMax, 6 fades it
	// back in; 7 pulses the clock's size, 8 brings it back
	switch (h->mClockState) {
	case 0:
		h->mClockState   = 3;
		h->mClockAlpha   = 0;
		h->mItemMaxAlpha = h->mClockAlpha;
		h->mItemMax      = ItemMax();
		h->mClockScale   = 1;
		break;
	case 1:
		h->mClockAlpha   = 1;
		h->mItemMaxAlpha = h->mClockAlpha;
		break;
	case 2:
		h->mClockAlpha   = 0;
		h->mItemMaxAlpha = h->mClockAlpha;
		break;
	case 3:
		h->mClockAlpha += 0.08f * frames;
		if (h->mClockAlpha >= 1)
			h->mClockAlpha = 1;
		h->mItemMaxAlpha = h->mClockAlpha;
		break;
	case 4:
		h->mClockAlpha -= 0.08f * frames;
		if (h->mClockAlpha < 0)
			h->mClockAlpha = 0;
		h->mItemMaxAlpha = h->mClockAlpha;
		break;
	case 5:
		h->mItemMaxAlpha -= 0.08f * frames;
		if (h->mItemMaxAlpha < 0) {
			h->mItemMaxAlpha = 0;
			h->mItemMax      = h->mNextItemMax;
			PlayWiiSoundEffect(&gGame.mSound, 0x123, 1.0f, 0.0f);
			++h->mClockState;
		}
		break;
	case 6:
		h->mItemMaxAlpha += 0.08f * frames;
		if (h->mItemMaxAlpha >= 1)
			h->mItemMaxAlpha = 1;
		break;
	case 7:
		ApproachHudScale(h->mClockScale, 1.0f + 0.125f * (f32)sin(DEG2RAD * ((s32)((u32)h->mFrameCount * 16) % 360)));
		break;
	case 8:
		ApproachHudScale(h->mClockScale, 1);
		break;
	}
	// the clock's warning blink ticks on the frames the day's time moves on. The Wii builds the
	// "game is live" composite in full here: status bit, no disc flags, no menu, no failed disc,
	// no pending reset; then no event script, no menu (0xc00, chibi robo's rings or the yes/no
	// menu), a day length on the clock and the clock not already running
	bool timeMoves = false;
	running        = FALSE;
	if (gGame.GetStatusFlags(1) && !gGame.GetStatusFlags(0x22) && !gGame.mMenu.mActive) {
		BOOL diskFail = FALSE;
		if ((gGame.mDvdStatus || gGame.mPrevDvdStatus) && !gGame.mDiskOk)
			diskFail = TRUE;
		if (!diskFail && !gGame.mResetRequested)
			running = TRUE;
	}
	if (running && !gGame.mEvt.mData.mScript && !gGame.mMenu.mActive && !gGame.GetStatusFlags(0xC00) && gGame.mTimeState.mDuration > 0
	    && !gGame.mTimeState.mRunning) {
		timeMoves = true;
	}
	if (timeMoves || gGame.mTimeState.mTime >= 1)
		++h->mTimeWarningTick;
	CVec position;
	switch (h->mObjectGaugeState) {
	case 0:
		h->mObjectGaugeState = 4;
		h->mObjectGaugeAlpha = 0;
		break;
	case 1:
		h->mObjectGaugeAlpha = 1;
		if (h->mGaugeTarget && (h->mGaugeTarget->mStatus & 1)) {
			s32 path[] = { 0, 0 };
			position   = h->mGaugeTarget->vf0B4(path);
			gGame.ProjectOffsetToScreen(&position, &position.x, &position.y, NULL, NULL);
			position.z         = 0;
			h->mObjectGaugePos = position;
		}
		break;
	case 2:
		h->mObjectGaugeAlpha = 0;
		break;
	case 3:
		h->mObjectGaugeAlpha += 0.08f * frames;
		if (h->mObjectGaugeAlpha >= 1)
			h->mObjectGaugeAlpha = 1;
		if (h->mGaugeTarget && (h->mGaugeTarget->mStatus & 1)) {
			s32 path[] = { 0, 0 };
			position   = h->mGaugeTarget->vf0B4(path);
			gGame.ProjectOffsetToScreen(&position, &position.x, &position.y, NULL, NULL);
			position.z         = 0;
			h->mObjectGaugePos = position;
		}
		break;
	case 4:
		h->mObjectGaugeAlpha -= 0.08f * frames;
		if (h->mObjectGaugeAlpha < 0)
			h->mObjectGaugeAlpha = 0;
		break;
	}
	StepHudVisibility(h->mPlayerIconState, h->mPlayerIconAlpha, frames);
	StepHudVisibility(h->mTimerState, h->mTimerAlpha, frames);
	if (h->mTimerShown) {
		h->mTimer += frames;
		if (h->mTimer >= TIMER_MAX) {
			h->mTimer      = TIMER_MAX;
			h->mTimerShown = FALSE;
		}
	}
	StepHudVisibility(h->mScanlineVisibilityState, h->mScanlineAlpha, frames);
	if (gGame.GetStatusFlags(0x400)) {
		if (gHudOpacityScale > 0 && (gHudOpacityScale -= 0.08f * frames) < 0)
			gHudOpacityScale = 0;
	} else if (gHudOpacityScale < 1 && (gHudOpacityScale += 0.08f * frames) > 1)
		gHudOpacityScale = 1;
	// the Wii-only idle indicator: _1E4.y eases toward the _1E4.x target, and once it is mostly
	// in, _1E4.z rolls through a repeating phase
	if (h->_1E4.y < h->_1E4.x) {
		h->_1E4.y += 0.2f * frames;
		if (h->_1E4.y > h->_1E4.x)
			h->_1E4.y = h->_1E4.x;
	} else if (h->_1E4.y > h->_1E4.x) {
		h->_1E4.y -= 0.2f * frames;
		if (h->_1E4.y < h->_1E4.x)
			h->_1E4.y = h->_1E4.x;
	}
	if (h->_1E4.y > 0.3f)
		h->_1E4.z = (f32)fmod(h->_1E4.z + 0.05f * frames, 1.0);
}

/**
 * @note Address: 0x8008BFE8
 * @note Size: 0xC0
 */
extern "C" void ApproachHudValue(f32* cur, f32* target, f32 step)
{
	Approach(cur, target, step);
}

/**
 * @note Address: 0x8008C0A8
 * @note Size: 0xB4
 */
// the point at the bottom of the screen the counters' sparkle effects rise from
extern "C" void GetHudPointParticleOrigin(InfoScrn* scrn, f32* x, f32* y)
{
	f32 xPos       = 22.0f + (f32)gScreenSafeLeft;
	f32 iconHeight = 18.0f;
	f32 iconY      = GetHudBottomEdge() - 64.0f - 40.0f - iconHeight * 0.5f;
	if (x) {
		*x = xPos;
	}
	if (y) {
		*y = iconY;
	}
}

/**
 * @note Address: 0x8008C15C
 * @note Size: 0x14
 */
extern "C" void ShowHappyPointCounter()
{
	gHudHappyPointCounter.mVisible = TRUE;
}

/**
 * @note Address: 0x8008C170
 * @note Size: 0x14
 */
extern "C" void HideHappyPointCounter()
{
	gHudHappyPointCounter.mVisible = FALSE;
}

/**
 * @note Address: 0x8008C184
 * @note Size: 0x14
 */
extern "C" void ShowMoolahCounter()
{
	gHudMoneyCounter.mVisible = TRUE;
}

/**
 * @note Address: 0x8008C198
 * @note Size: 0x14
 */
extern "C" void HideMoolahCounter()
{
	gHudMoneyCounter.mVisible = FALSE;
}

/**
 * @note Address: 0x8008C1AC
 * @note Size: 0x14
 */
extern "C" void ShowScrapCounter()
{
	gHudScrapCounter.mVisible = TRUE;
}

/**
 * @note Address: 0x8008C1C0
 * @note Size: 0x14
 */
extern "C" void HideScrapCounter()
{
	gHudScrapCounter.mVisible = FALSE;
}

/**
 * @note Address: 0x8008C1D4
 * @note Size: 0x14
 */
// the scrap counter pulses again once it comes back on screen; the hud is passed but unused (retail's
// caller in title.cpp loads r3 with gGame + 0xAF58, CGame's mInfoScrn, at 0x800487F4)
extern "C" void ResumeScrapCounterPulse(InfoScrn*)
{
	gHudScrapCounter.mState = 2;
}

/**
 * @note Address: 0x8008C1E8
 * @note Size: 0x28
 */
// counters stay hidden while he's idle, until AllowIdleHudCounters
extern "C" void SuppressIdleHudCounters()
{
	gHudHappyPointCounter.mSuppressIdleDisplay = TRUE;
	gHudMoneyCounter.mSuppressIdleDisplay      = TRUE;
	gHudScrapCounter.mSuppressIdleDisplay      = TRUE;
}

/**
 * @note Address: 0x8008C210
 * @note Size: 0x28
 */
extern "C" void AllowIdleHudCounters()
{
	gHudHappyPointCounter.mSuppressIdleDisplay = FALSE;
	gHudMoneyCounter.mSuppressIdleDisplay      = FALSE;
	gHudScrapCounter.mSuppressIdleDisplay      = FALSE;
}

/**
 * @note Address: 0x8008E9BC
 * @note Size: 0xC
 */
extern "C" void InfoScrn_ShowTimer(InfoScrn* scrn)
{
	scrn->mTimerShown = TRUE;
}

/**
 * @note Address: 0x8008E9C8
 * @note Size: 0xC
 */
extern "C" void InfoScrn_HideTimer(InfoScrn* scrn)
{
	scrn->mTimerShown = FALSE;
}

/**
 * @note Address: 0x8008E9D4
 * @note Size: 0x88
 */
extern "C" void InfoScrn_SetTimerHundredths(InfoScrn* scrn, s32 hundredths)
{
	scrn->mTimer = ((f32)hundredths / 100.0f) * (f32)TIMER_RATE;
	if (scrn->mTimer >= (f32)TIMER_MAX) {
		scrn->mTimer = (f32)TIMER_MAX;
	}
}

/**
 * @note Address: 0x8008EA5C
 * @note Size: 0x114
 */
extern "C" s32 InfoScrn_GetTimerHundredths(InfoScrn* scrn)
{
	return (s32)FramesToClock(scrn->mTimer);
}

/**
 * @note Address: 0x8008EB70
 * @note Size: 0x104
 */
extern "C" f32 TimerFramesToHundredths(f32 frames)
{
	return FramesToClock(frames);
}

static const char* sAscii = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}";
static const char* sWide  = "\x81\x40\x81\x49\x81\x68\x81\x94\x81\x90\x81\x93\x81\x95\x81\x66"
                            "\x81\x69\x81\x6A\x81\x96\x81\x7B\x81\x43\x81\x5B\x81\x44\x81\x5E"
                            "\x82\x4F\x82\x50\x82\x51\x82\x52\x82\x53\x82\x54\x82\x55\x82\x56"
                            "\x82\x57\x82\x58\x81\x46\x81\x47\x81\x83\x81\x81\x81\x84\x81\x48"
                            "\x81\x97\x82\x60\x82\x61\x82\x62\x82\x63\x82\x64\x82\x65\x82\x66"
                            "\x82\x67\x82\x68\x82\x69\x82\x6A\x82\x6B\x82\x6C\x82\x6D\x82\x6E"
                            "\x82\x6F\x82\x70\x82\x71\x82\x72\x82\x73\x82\x74\x82\x75\x82\x76"
                            "\x82\x77\x82\x78\x82\x79\x81\x6D\x81\x8F\x81\x6E\x81\x4F\x81\x51"
                            "\x81\x65\x82\x81\x82\x82\x82\x83\x82\x84\x82\x85\x82\x86\x82\x87"
                            "\x82\x88\x82\x89\x82\x8A\x82\x8B\x82\x8C\x82\x8D\x82\x8E\x82\x8F"
                            "\x82\x90\x82\x91\x82\x92\x82\x93\x82\x94\x82\x95\x82\x96\x82\x97"
                            "\x82\x98\x82\x99\x82\x9A\x81\x6F\x81\x62\x81\x70";

/**
 * @note Address: 0x8008EFA0
 * @note Size: 0xE0
 */
// shift-jis full-width text of what a hud label says, the count it replaced
extern "C" s32 ConvertHudTextToFullWidth(char* text)
{
	char buf[0x200];
	char* out = buf;
	char* dst;
	char* p;
	s32 count = 0;
	for (p = text, dst = out; *p != '\0'; p++) {
		BOOL found = FALSE;
		for (s32 i = 0; sAscii[i] != '\0'; i++) {
			if (*p == sAscii[i]) {
				strncpy(dst, &sWide[i * 2], 2)[2] = '\0';
				found                             = TRUE;
				count++;
				dst += 2;
				break;
			}
		}
		if (!found) {
			strncpy(dst, p, 1)[1] = '\0';
			dst++;
		}
	}
	strcpy(text, out);
	return count;
}

/**
 * @note Address: 0x8008F080
 * @note Size: 0x34
 */
// NONMATCHING: retail holds the gauge and scaled maximum in f0/f1; this source uses f2/f0.
extern "C" BOOL InfoScrn_IsBatteryGaugeFull(InfoScrn* scrn)
{
	return scrn->mBatteryGauge == 10.0f * gGame.GetPlayer()->mParams.mMaxBattery;
}

/**
 * @note Address: 0x8008F0B4
 * @note Size: 0x1C
 */
extern "C" BOOL InfoScrn_IsBatteryGaugeEmpty(InfoScrn* scrn)
{
	return scrn->mBatteryGauge <= 0.0f;
}

/**
 * @note Address: 0x8008F0D0
 * @note Size: 0x6C
 */
extern "C" void InitBatteryGauge(InfoScrn* scrn, f32 battery)
{
	for (int i = 0; i < 5; i++) {
		scrn->mDigitRoll[i]      = 1.0f;
		scrn->mDigitRolling[i]   = 0;
		scrn->mDigitDelta[i]     = 0.0f;
		scrn->mDigitLastDelta[i] = 0.0f;
	}
	scrn->mBatteryTarget = 10.0f * battery;
	scrn->mBatteryValue  = scrn->mBatteryTarget;
}

/**
 * @note Address: 0x8008F13C
 * @note Size: 0xA8
 */
extern "C" void InfoScrn_ApplyBatteryHudCommand(InfoScrn* scrn, u8 show, s32 mode)
{
	switch (mode) {
	case 0:
		if (show) {
			scrn->mBatteryState = 1;
		} else {
			scrn->mBatteryState = 2;
		}
		break;
	case 1:
		if (show) {
			scrn->mBatteryState = 3;
		} else {
			scrn->mBatteryState = 4;
		}
		break;
	case 2:
		if (show) {
			scrn->mTargetBatteryOffsetY = -52.0f;
		} else {
			scrn->mTargetBatteryOffsetY = 0.0f;
		}
		break;
	case 3:
		if (show) {
			scrn->mBatteryState = 5;
		} else {
			scrn->mBatteryState = 6;
		}
		break;
	}
}

/**
 * @note Address: 0x8008F1E4
 * @note Size: 0x28C
 */
extern "C" void InfoScrn_ApplyClockHudCommand(InfoScrn* scrn, u8 show, s32 mode, s32 flag)
{
	switch (mode) {
	case 0:
		if (show) {
			scrn->mClockState = 1;
		} else {
			scrn->mClockState = 2;
		}
		break;
	case 1:
		if (show) {
			scrn->mClockState = 3;
		} else {
			scrn->mClockState = 4;
		}
		break;
	case 2:
		scrn->mItemMax = ItemMax();
		break;
	case 3:
		scrn->mClockState  = 5;
		scrn->mNextItemMax = ItemMax();
		break;
	case 4:
		if (show) {
			scrn->mClockState = 7;
		} else {
			scrn->mClockState = 8;
		}
		break;
	}
}

/**
 * @note Address: 0x8008F470
 * @note Size: 0x7C
 */
extern "C" void InfoScrn_ApplyObjectGaugeCommand(InfoScrn* scrn, WiiTitleObject* obj, u8 show, s32 mode, s32 param)
{
	scrn->mGaugeTarget = NULL;
	switch (mode) {
	case 0:
		if (show) {
			scrn->mGaugeTarget      = obj;
			scrn->mObjectGaugeState = 1;
			scrn->mFrameCount       = 0;
		} else {
			scrn->mGaugeTarget      = NULL;
			scrn->mObjectGaugeState = 2;
		}
		break;
	case 1:
		if (show) {
			scrn->mGaugeTarget      = obj;
			scrn->mObjectGaugeState = 3;
			scrn->mFrameCount       = 0;
		} else {
			scrn->mGaugeTarget      = NULL;
			scrn->mObjectGaugeState = 4;
		}
		break;
	}
	scrn->mObjectGaugeParam = param;
}

/**
 * @note Address: 0x8008F4EC
 * @note Size: 0x54
 */
extern "C" void InfoScrn_ApplyScoreHudCommand(InfoScrn* scrn, u8 show, s32 mode)
{
	switch (mode) {
	case 0:
		if (show) {
			scrn->mPlayerIconState = 1;
		} else {
			scrn->mPlayerIconState = 2;
		}
		break;
	case 1:
		if (show) {
			scrn->mPlayerIconState = 3;
		} else {
			scrn->mPlayerIconState = 4;
		}
		break;
	}
}

/**
 * @note Address: 0x8008F540
 * @note Size: 0x54
 */
extern "C" void InfoScrn_ApplyTimerHudCommand(InfoScrn* scrn, u8 show, s32 mode)
{
	switch (mode) {
	case 0:
		if (show) {
			scrn->mTimerState = 1;
		} else {
			scrn->mTimerState = 2;
		}
		break;
	case 1:
		if (show) {
			scrn->mTimerState = 3;
		} else {
			scrn->mTimerState = 4;
		}
		break;
	}
}

/**
 * @note Address: 0x8008F594
 * @note Size: 0x54
 */
extern "C" void InfoScrn_ApplyScanlineHudCommand(InfoScrn* scrn, u8 show, s32 mode)
{
	switch (mode) {
	case 0:
		if (show) {
			scrn->mScanlineVisibilityState = 1;
		} else {
			scrn->mScanlineVisibilityState = 2;
		}
		break;
	case 1:
		if (show) {
			scrn->mScanlineVisibilityState = 3;
		} else {
			scrn->mScanlineVisibilityState = 4;
		}
		break;
	}
}

/**
 * @note Address: 0x8008F5E8
 * @note Size: 0x4
 */
extern "C" void InfoScrn_OnPlayerExperienceChanged(InfoScrn* scrn, s32 delta)
{
}

/**
 * @note Address: 0x8008F5EC
 * @note Size: 0x4
 */
extern "C" void InfoScrn_OnPlayerLevelChanged(InfoScrn* scrn, s32 level)
{
}

/**
 * @note Address: 0x8008F5F0
 * @note Size: 0x294
 */
extern "C" void DrawHudRoundedRectangle(u32 color, f32 x, f32 y, f32 w, f32 h, f32 rx, f32 ry)
{
	Vec pos[65];
	f32 sx;
	f32 sy;
	sx = 9.0f * rx;
	sy = 9.0f * ry;
	for (int i = 0; i < 65; i++) {
		f32 cx;
		f32 cy;
		f32 s;
		// 0.09817477f is 2pi / 64, once round in 64 steps
		s     = sin(0.09817477f * i - HALF_PI);
		f32 c = -(f32)cos(0.09817477f * i - HALF_PI);
		if ((i >= 0 && i < 16) || i == 64) {
			cx = x;
			cy = y;
		} else if ((i >= 16 && i < 32) || i == 64) {
			cx = x + w;
			cy = y;
		} else if ((i >= 32 && i < 48) || i == 64) {
			cx = x + w;
			cy = y + h;
		} else {
			cx = x;
			cy = y + h;
		}
		pos[i].x = s * sx + cx - w / 2;
		pos[i].y = c * sy + cy - h / 2;
		pos[i].z = 0.0f;
	}
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, pos, color, 65);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
}

/**
 * @note Address: 0x800903C4
 * @note Size: 0x354
 */
extern "C" void InitHudPointCounter(InfoPoint* point, s32 id)
{
	point->mIndex       = id;
	point->mPulseFrames = 0;
	point->mSlideState  = 0;
	point->mFadeState   = 0;
	point->mSlideDelay  = 0;
	point->mOpacity     = 0.0f;
	for (int i = 0; i < 6; i++) {
		point->mPos[i].x = gScreenSafeLeft - 204.0f;
	}
	for (int i = 0; i < 4; i++) {
		point->mValue[i] = 0.0f;
	}
	point->mVisible             = 0;
	point->mSuppressIdleDisplay = 0;
	switch (point->mIndex) {
	case INFOPOINT_ID + 2:
		point->mResource = point->mIndex;
		point->mPos[0].y = GetHudBottomEdge() - 58.0f - 40.0f;
		point->mPos[1].y = GetHudBottomEdge() - 58.0f - 40.0f;
		break;
	case INFOPOINT_ID:
		point->mResource = point->mIndex;
		point->mPos[0].y = GetHudBottomEdge() - 58.0f;
		point->mPos[1].y = GetHudBottomEdge() - 58.0f;
		break;
	case INFOPOINT_ID + 1:
		point->mResource = point->mIndex;
		point->mPos[0].y = 40.0f + (GetHudBottomEdge() - 58.0f);
		point->mPos[1].y = 40.0f + (GetHudBottomEdge() - 58.0f);
		break;
	}
	point->mState          = 0;
	point->mIconAlphaScale = 0.0f;
	point->_80             = 0;
}

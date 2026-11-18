#include "game/wii/infoscrn.h"
#include "game/infopoint.h"
#include "game/globals.h"
#include "game/gmath.h"
#include "game/wii/cgame.h"
#include "game/wii/cpad.h"
#include "game/wii/csound.h"
#include "game/wii/title.h"
#include "game/ut.h"
#include <string.h>
#include <math.h>

// the info screen on the Wii, the hud drawn over the game: the clock, the battery, the
// happy points and the three counters that pop up in the corner. CGame keeps it in
// mInfoScrn. Ported from the GameCube's game/infoscrn.cpp: the status flags move behind
// CGame::GetStatusFlags, the timer runs off gRefreshRate instead of a fixed rate, the
// object gauge gains a parameter and the counters keep the same layout.
//
// todo: DrawHudClock, DrawHudGaugeIcon, InfoScrn_DrawPlayerHudIcon,
// InfoScrn_DrawTimer, DrawHudScanlines, DrawTexturedWindowFrame, InitHudPointCounter,
// UpdateHudPointValue, DrawHudPointCounter

// the timer counts frames at the video mode's rate, 50 or 60 a second
#define TIMER_RATE gRefreshRate
// 99:59.99, the most the timer shows
#define TIMER_MAX ((s32)((u32)TIMER_RATE * 6000U - 1U))

extern "C" {
s32 gRefreshRate;
s32 gScreenSafeLeft;
s32 gScreenSafeBottom;
extern s32 gScreenSafeTop;
extern s32 gScreenSafeRight;
s32 gPointerScreenWidth;
s32 gPointerScreenHeight;
Vec fn_800E5A2C(WiiPlayerState* player);
Vec GetCornerIconPosition(CGame* game);
bool IsUtilibotStatusIconUnlocked();
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
	f32 bottom = gGame.mWipeCine.mHeight * 0.5f + gGame.mWipeCine.mY;
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

// 10 to the n
static inline f32 Pow10(int n)
{
	f32 p = 1.0f;
	for (int i = 0; i < n; i++) {
		p *= 10.0f;
	}
	return p;
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
// the scrap counter pulses again once it comes back on screen
extern "C" void ResumeScrapCounterPulse(InfoScrn* scrn)
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

// the shared texture in the tpl reinitialised for one descriptor, then copied to draw with
static inline void HudTexture(CTexObj& tex, s32 id)
{
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[id], 0);
	tex = gGame.mTpl.mTex;
}

static inline void SetBatterySlice(CTexObj& tex, f32 height, f32 top)
{
	PSMTXScale(tex.mTexMtx.mMtx[2], 1, height, 1);
	tex.mTexMtx.mDirty = TRUE;
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0, top, 0);
	tex.mTexMtx.mDirty = TRUE;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
}
static inline f32 BatteryDigitX(int place, f32 offset, f32 spacing)
{
	return 2.f + (2.f * spacing + ((f32)(gScreenSafeRight - 105) + offset - place * spacing));
}
static inline void DrawBatteryQuad(CTexObj& tex, u32 color, f32 x, f32 y, f32 w, f32 h)
{
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0, w, h, 0, 0, 0);
}
static inline void DrawBatteryOverlay(CTexObj& tex, u32 color, s32 place, f32 offset, f32 y, f32 w, f32 height, f32 gap, f32 fill, f32 rest)
{
	DrawBatteryQuad(tex, color, BatteryDigitX(place, offset, w + gap), y - rest * height * .5f, w, height * fill);
}
static inline void DrawBatteryDigitPart(CTexObj& tex, u32 color, int place, f32 offset, f32 y, f32 scale, f32 gap, f32 texHeight,
                                        f32 texTop, f32 part, f32 shift, bool up = false)
{
	HudTexture(tex, 126);
	SetBatterySlice(tex, texHeight, texTop);
	f32 w = scale * GXGetTexObjWidth(&tex);
	f32 h = scale * GXGetTexObjHeight(&tex) / 10.f;
	DrawBatteryQuad(tex, color, BatteryDigitX(place, offset, w + gap), up ? y - shift * h * .5f : y + shift * h * .5f, w, h * part);
}
static inline u32 BatteryDigitColor(int whole, u32 low, u32 active, u32 inactive)
{
	if (gGame.GetPlayer()->mParams.mBattery <= 30.f && whole > 0)
		return low;
	if (whole > 0)
		return active;
	return inactive;
}

/**
 * @note Address: 0x80089658
 * @note Size: 0x2990
 */
// NONMATCHING: ported from the GameCube's DrawHudBatteryGauge — the textures move one slot up
// (125/126/127/168), the x anchor is gScreenSafeRight - 105 instead of 512, the background quad
// sits at z 1 and _1E4.y over 0.3 makes it draw behind the digits with the z test and comp loc
// flipped, and _1E4.y over 0 draws an orange glow quad and a black one under DrawGsQuad. Retail
// also keeps a dead mBatteryValue == max compare inside the digit-change block, written below as
// the roll reset it most plausibly was; the rest is register and schedule noise in the big loop
extern "C" void DrawHudBatteryGauge(InfoScrn* h)
{
	f32 alpha = h->mBatteryAlpha * gHudOpacityScale;
	if (gGame.GetStatusFlags(0x400))
		h->mTargetDimAlpha = 1;
	else
		h->mTargetDimAlpha = 0;
	f32 target = 10.f * gGame.GetPlayer()->mParams.mBattery;
	f32 max    = 10.f * gGame.GetPlayer()->mParams.mMaxBattery;
	f32 y      = 22.f + (GetHudBottomEdge() - 50.f);
	if (target <= 0 && h->mBatteryValue > 0) {
		h->mBatteryEmpty      = 1;
		h->mBatteryEmptyTimer = h->mBatteryEmptyStep = 0;
	}
	if (target > 0)
		h->mBatteryEmpty = 0;
	if ((s32)gGame.GetPlayer()->mParams._1C[0] > 0) {
		h->mBatteryDim            = 1;
		h->mTargetBatteryDimAlpha = 1;
	} else {
		h->mBatteryDim            = 0;
		h->mTargetBatteryDimAlpha = 0;
	}
	Approach(&h->mBatteryDimAlpha, &h->mTargetBatteryDimAlpha, .18f);
	const f32 fade = 1.f - h->mBatteryDimAlpha;
	f32 scale      = h->mBatteryScale;
	CTexObj tex;
	HudTexture(tex, 125);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	u16 tw = GXGetTexObjWidth(&tex), th = GXGetTexObjHeight(&tex);
	f32 opacity = 255.f * alpha;
	// the Wii draws the background behind everything at z 1, flipping the z test while _1E4.y is up
	f32 w  = scale * (1.3f * tw);
	f32 z  = h->_1E4.y;
	f32 ht = scale * (1.2f * th);
	if (z > 0.3f) {
		gGame.mGs.mCurRenderState->SetZCompLoc(GX_FALSE);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	} else
		z = 0;
	f32 x = (f32)(gScreenSafeRight - 105) + 13.f * scale;
	DrawGsTexturedQuad(&gGame.mGs, &tex, 0xc8c8c800 | (u32)opacity, 0xc8c8c800 | (u32)opacity, 0xc8c8c800 | (u32)opacity,
	                   0xc8c8c800 | (u32)opacity, x, y, 1.0f, w, ht, 0.0f, 0.0f, 0.0f);
	if (z > 0) {
		gGame.mGs.mCurRenderState->SetZCompLoc(GX_TRUE);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	HudTexture(tex, 126);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1, .1f, 1);
	tex.mTexMtx.mDirty = TRUE;
	h->mBatteryTarget  = target;
	f32 change         = h->mBatteryValue - h->mBatteryTarget;
	f32 step           = Abs((h->mBatteryTarget - h->mBatteryValue) / 20.f);
	if (step >= 53.999996f)
		step = 53.999996f;
	if (step <= 1)
		step = 1;
	Approach(&h->mBatteryValue, &h->mBatteryTarget, step);
	h->mBatteryGauge = h->mBatteryValue;
	u32 digitAlpha   = (u32)(opacity * fade);
	u32 red          = 0xff646400 | digitAlpha;
	u32 green        = 0x89ff2900 | digitAlpha;
	u32 gray         = 0x50505000 | digitAlpha;
	f32 gap          = 7.6f * scale;
	f32 smallScale = .9f * scale, smallX = 7.9f * scale, largeX = 2.f * scale;
	for (int i = 0; i < 4; ++i) {
		f32 digitScale = scale, offset = largeX;
		if (target <= 0 && h->mBatteryValue <= 0 && h->mBatteryEmpty == 1)
			break;
		if (i == 0) {
			digitScale = smallScale;
			offset     = smallX;
		}
		if (!h->mDigitRolling[i])
			h->mDigitDelta[i] = change;
		if (change != 0)
			h->mDigitLastDelta[i] = change;
		int divisor = 1;
		for (int p = 0; p < i; ++p)
			divisor *= 10;
		int whole = (s32)h->mBatteryValue / divisor, digit = whole % 10;
		if (h->mDigitValue[i] != digit) {
			h->mDigitRoll[i]    = 0;
			h->mDigitRolling[i] = 1;
			if (h->mBatteryValue == max)
				h->mDigitRoll[i] = 0;
		}
		if (!gGame.GetStatusFlags(0x200000000000ULL))
			h->mDigitRoll[i] += .08f;
		if (h->mDigitRoll[i] > 1) {
			h->mDigitRoll[i]    = 1;
			h->mDigitRolling[i] = 0;
		}
		h->mDigitValue[i] = digit;
		f32 amount;
		if (!i) {
			f32 value = h->mBatteryValue / divisor;
			amount    = 1.f - (value - (s32)value);
		} else
			amount = h->mDigitRoll[i];
		u32 color;

		if (h->mDigitDelta[i] > 0) {
			color = BatteryDigitColor(whole, red, green, gray);
			DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * amount, .1f * digit + (1.f - amount) / 10.f, amount,
			                     (1.f - amount), true);
			DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * (1.f - amount), .1f * ((whole + 1) % 10), (1.f - amount),
			                     amount);
		} else if (h->mDigitDelta[i] < 0) {
			color = BatteryDigitColor(whole, red, green, gray);
			DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * amount, .1f * digit, amount, (1.f - amount));
			DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * (1.f - amount), .1f * ((whole - 1) % 10) + amount / 10.f,
			                     (1.f - amount), amount, true);
		} else {
			color = BatteryDigitColor(whole, red, green, gray);
			if (h->mDigitLastDelta[i] > 0) {
				DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * amount, .1f * digit + (1.f - amount) / 10.f, amount,
				                     (1.f - amount), true);
				DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * (1.f - amount), .1f * ((whole + 1) % 10),
				                     (1.f - amount), amount);
			} else if (h->mDigitLastDelta[i] < 0) {
				DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * amount, .1f * digit, amount, (1.f - amount));
				DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * (1.f - amount),
				                     .1f * ((whole - 1) % 10) + amount / 10.f, (1.f - amount), amount, true);
			} else {
				DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f, .1f * digit, 1, 0);
			}
		}
	}
	if (target <= 0 && h->mBatteryValue <= 0 && h->mBatteryEmpty == 1) {
		++h->mBatteryEmptyTimer;
		if (h->mBatteryEmptyTimer > 1) {
			++h->mBatteryEmptyStep;
			if (h->mBatteryEmptyStep >= 9)
				h->mBatteryEmptyStep = 9;
			h->mBatteryEmptyTimer = 0;
		}
		for (int i = 0; i < 4; ++i) {
			f32 digitScale = scale, offset = largeX;
			if (i == 0) {
				digitScale = smallScale;
				offset     = smallX;
			}

			int frame = h->mBatteryEmptyStep % 10 + 1;
			HudTexture(tex, 126);
			f32 fill = .1f * frame;
			SetBatterySlice(tex, .1f * (1.f - fill), 0);
			f32 digitW = digitScale * GXGetTexObjWidth(&tex);
			f32 digitH = digitScale * GXGetTexObjHeight(&tex) / 10.f;
			f32 rest   = .1f * (10 - frame);
			DrawBatteryQuad(tex, gray, BatteryDigitX(i, offset, digitW + gap), y + fill * digitH * .5f, digitW, digitH * rest);

			if (i == 0) {
				digitScale = .8f * scale;
				offset     = 11.2f * scale;
			} else
				offset = 1.7f * scale;
			HudTexture(tex, 127);
			f32 w = digitScale * GXGetTexObjWidth(&tex), ht = digitScale * GXGetTexObjHeight(&tex);
			SetBatterySlice(tex, fill, rest);
			u32 emptyColor = 0x5176ff00 | digitAlpha;
			DrawBatteryOverlay(tex, emptyColor, i, offset, y, w, ht, gap, fill, rest);
		}
	}
	if (h->mBatteryDim == 1) {
		for (int i = 0; i < 4; ++i) {

			f32 digitScale = scale, offset = largeX, verticalOffset = -2.f;
			if (i == 0) {
				digitScale     = smallScale;
				offset         = smallX;
				verticalOffset = 0.f;
			}

			HudTexture(tex, 126);
			SetBatterySlice(tex, 0, 0);
			f32 digitW     = digitScale * GXGetTexObjWidth(&tex);
			u16 texH       = GXGetTexObjHeight(&tex);
			f32 digitH     = digitScale * texH / 10.f;
			u32 digitColor = (u32)(h->mBatteryDimAlpha * opacity) | 0x50505000;
			DrawBatteryQuad(tex, digitColor, BatteryDigitX(i, offset, digitW + gap), (y + verticalOffset) + digitH * .5f, digitW,
			                digitH * 0.f);

			if (i == 0) {
				digitScale     = .8f * scale;
				offset         = 11.2f * scale;
				verticalOffset = 0.f;
			} else
				offset = 1.7f * scale;
			HudTexture(tex, 168);
			f32 w = digitScale * GXGetTexObjWidth(&tex), ht = digitScale * GXGetTexObjHeight(&tex);
			SetBatterySlice(tex, 1, 0);
			u32 overlayColor = (u32)(h->mBatteryDimAlpha * opacity) | 0x89ff2900;
			DrawBatteryOverlay(tex, overlayColor, i, offset, y + verticalOffset, w, ht, gap, 1.f, 0.f);
		}
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	for (int i = 0; i < 4; ++i) {
		f32 size = scale, offset = largeX;
		if (i == 0) {
			size   = smallScale;
			offset = 9.9f * scale;
		}
		HudTexture(tex, 126);
		f32 w = 1.4f * (size * GXGetTexObjWidth(&tex)), ht = size * GXGetTexObjHeight(&tex) / 10.f;
		f32 x = BatteryDigitX(i, offset, w + .4f * scale) - w * .5f, top = y - ht * .5f;
		Vec points[]
		    = { (Vec) { x, top, 0 }, (Vec) { x + w, top, 0 }, (Vec) { x + w, top + ht, 0 }, (Vec) { x, top + ht, 0 }, (Vec) { x, top, 0 } };
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, points,
		                        0x0a0a0a00 | (u32)((120.f * alpha) * h->mDimAlpha), 5);
	}
	// while _1E4.y is up an orange glow trails the battery, stretched along _1E4.z and equal-tested
	// against the background's z, then a black quad behind it at z -1
	if (z > 0) {
		f32 a = 80.f * z;
		if (a > 100.f)
			a = 100.f;
		u32 glowColor = 0xff800000 | (u32)(a * alpha);
		HudTexture(tex, 119);
		PSMTXTrans(tex.mTexMtx.mMtx[0], h->_1E4.z, 0, 0);
		tex.mTexMtx.mDirty = TRUE;
		PSMTXScale(tex.mTexMtx.mMtx[2], 3, 1, 1);
		tex.mTexMtx.mDirty = TRUE;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_EQUAL, GX_FALSE);
		DrawGsTexturedQuad(&gGame.mGs, &tex, glowColor, glowColor, glowColor, glowColor, x, y, 1.0f, w, ht, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		DrawGsQuad(&gGame.mGs, 0, 0, 0, 0, x, y, -1.0f, w, ht, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	}
}

static inline void DrawClockDisc(f32 x, f32 y, f32 rx, f32 ry, u32 color)
{
	Vec points[65];
	for (s32 i = 0; i < 65; ++i) {
		f32 angle = 0.09817477f * i - HALF_PI;
		f32 s = sin(angle), c = -(f32)cos(angle);
		points[i].x = x + s * rx;
		points[i].y = y + c * ry;
		points[i].z = 0;
	}
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, points, color, 65);
}

/**
 * @note Address: 0x8008C238
 * @note Size: 0x13A0
 */
extern "C" void DrawHudClock(InfoScrn* scrn)
{
	f32 alpha = scrn->mClockAlpha * gHudOpacityScale;
	if (!(alpha > 0.0f))
		return;
	CTexObj tex;
	f32 x     = (f32)(gScreenSafeLeft + 0x28);
	f32 y     = (f32)(gScreenSafeTop + 0x30);
	f32 scale = 1.1f * scrn->mClockScale;
	u32 color;
	if (gGame.mTimeState.mMode == 0) {
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[157], 0);
		tex   = gGame.mTpl.mTex;
		color = 0x326c0200 | (u32)(255 * alpha);
	} else {
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[158], 0);
		tex   = gGame.mTpl.mTex;
		color = 0x1e3fb000 | (u32)(255 * alpha);
	}
	u16 tw = GXGetTexObjWidth(&tex), th = GXGetTexObjHeight(&tex);
	DrawClockDisc(x, y, 1.1f * (scale * tw * 0.5f), 1.1f * (scale * th * 0.5f), color);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[159], 0);
	tex = gGame.mTpl.mTex;
	tw  = GXGetTexObjWidth(&tex);
	th  = GXGetTexObjHeight(&tex);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255 * alpha) | 0xC8C8C800, (u32)(255 * alpha) | 0xC8C8C800, (u32)(255 * alpha) | 0xC8C8C800,
	                   (u32)(255 * alpha) | 0xC8C8C800, x, y, 1.0f, scale * tw, scale * th, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	if (gGame.mTimeState.mMode == 0) {
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[157], 0);
		tex = gGame.mTpl.mTex;
	} else {
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[158], 0);
		tex = gGame.mTpl.mTex;
	}
	f32 width = scale * GXGetTexObjWidth(&tex), height = scale * GXGetTexObjHeight(&tex);
	DrawClockDisc(x, y, 0.9f * (width / 2.0f), 0.9f * (height * 0.5f), (u32)(120 * alpha));
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 time = gGame.mTimeState.mTime;
	f32 fill = time;
	// the fill waits at 0.89 until time is up
	if (time >= 0.89f && time < 0.999f)
		fill = 0.89f;
	if (gGame.GetStatusFlags(0x400))
		color = 0xa0a0a000 | (u32)(255 * alpha);
	else {
		// the last five seconds of the day, which lasts 600 seconds / mDuration (see CGame's update)
		f32 warning = 1.0f - (TIMER_RATE * 5) / ((TIMER_RATE * 600) / gGame.mTimeState.mDuration);
		if (time < warning) {
			scrn->mTimeWarningBeeped = 0;
			scrn->mTimeWarningBeeps  = 5;
		}
		if (time >= warning) {
			f32 wave       = sin(DEG2RAD * ((s32)((u32)scrn->mTimeWarningTick * 12) % 360));
			bool timeMoves = false;
			BOOL running   = FALSE;
			if (gGame.GetStatusFlags(1) && !gGame.GetStatusFlags(0x20000000022ULL) && !gGame.mMenu.mActive) {
				BOOL diskFail = FALSE;
				if ((gGame.mDvdStatus || gGame.mPrevDvdStatus) && !gGame.mDiskOk)
					diskFail = TRUE;
				if (!diskFail && !gGame.mResetRequested)
					running = TRUE;
			}
			if (running && !gGame.mEvt.mData.mScript && !gGame.mMenu.mActive && !gGame.GetStatusFlags(0xC00)
			    && gGame.mTimeState.mDuration > 0 && !gGame.mTimeState.mRunning) {
				timeMoves = true;
			}
			if (timeMoves || time >= 1) {
				// one beep each time the clock blinks on: five of 0x122, then 0x123, which also comes at once when time is up
				if (wave > 0) {
					if (scrn->mTimeWarningBeeps >= 0 && !scrn->mTimeWarningBeeped) {
						if (scrn->mTimeWarningBeeps == 0 || time >= 0.999f) {
							PlayWiiSoundEffect(&gGame.mSound, 0x123, 1.0f, 0.0f);
							scrn->mTimeWarningBeeps = 0;
						} else
							PlayWiiSoundEffect(&gGame.mSound, 0x122, 1.0f, 0.0f);
						--scrn->mTimeWarningBeeps;
						scrn->mTimeWarningBeeped = 1;
					}
				} else
					scrn->mTimeWarningBeeped = 0;
			}
			if (wave > 0 || scrn->mTimeWarningBeeps < 0)
				color = 0xffffff00 | (u32)(255 * alpha);
			else
				color = 0xa0a0a000 | (u32)(255 * alpha);
		} else
			color = 0xffffff00 | (u32)(255 * alpha);
	}
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	PSMTXScale(tex.mTexMtx.mMtx[2], 1, fill, 1);
	tex.mTexMtx.mDirty = TRUE;
	PSMTXTrans(tex.mTexMtx.mMtx[0], 0, 0, 0);
	tex.mTexMtx.mDirty = TRUE;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	u32 fillColor = (u32)(160 * alpha);
	DrawGsTexturedQuad(&gGame.mGs, &tex, fillColor, fillColor, fillColor, fillColor, x, y - (1 - fill) * height / 2.0f, 1.0f, width,
	                   height * fill, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[160], 0);
	tex    = gGame.mTpl.mTex;
	width  = 0.9f * (scale * GXGetTexObjWidth(&tex));
	height = 0.9f * (scale * GXGetTexObjHeight(&tex));
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(140 * alpha) | 0xC8C8C800, (u32)(140 * alpha) | 0xC8C8C800, (u32)(140 * alpha) | 0xC8C8C800,
	                   (u32)(140 * alpha) | 0xC8C8C800, x - 4 * scale, y - 14 * scale, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[160], 0);
	tex = gGame.mTpl.mTex;
	tw  = GXGetTexObjWidth(&tex);
	th  = GXGetTexObjHeight(&tex);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(140 * alpha) | 0xC8C8C800, (u32)(140 * alpha) | 0xC8C8C800, (u32)(140 * alpha) | 0xC8C8C800,
	                   (u32)(140 * alpha) | 0xC8C8C800, x - 16 * scale, y - 2 * scale, 1.0f, 0.7f * (scale * tw), 0.7f * (scale * th), 0.0f,
	                   0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	f32 badgeAlpha = scrn->mItemMaxAlpha * gHudOpacityScale;
	switch ((s32)(f32)scrn->mItemMax) {
	case 10:
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[233], 0);
		tex = gGame.mTpl.mTex;
		break;
	case 15:
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[234], 0);
		tex = gGame.mTpl.mTex;
		break;
	default:
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[232], 0);
		tex = gGame.mTpl.mTex;
		break;
	}
	tw = GXGetTexObjWidth(&tex);
	th = GXGetTexObjHeight(&tex);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255 * badgeAlpha) | 0xFFFFFF00, (u32)(255 * badgeAlpha) | 0xFFFFFF00,
	                   (u32)(255 * badgeAlpha) | 0xFFFFFF00, (u32)(255 * badgeAlpha) | 0xFFFFFF00, x - 22 * scale, y - 22 * scale, 0.0f,
	                   1.1f * (scale * tw), 1.1f * (scale * th), 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

/**
 * @note Address: 0x8008D5D8
 * @note Size: 0x65C
 */
extern "C" void DrawHudGaugeIcon(InfoScrn* scrn)
{
	if (!gGame.GetStatusFlags(0x100)) {
		f32 alpha = scrn->mObjectGaugeAlpha * gHudOpacityScale;
		if (alpha > 0.0f) {
			CTexObj tex;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[111], 0);
			tex   = gGame.mTpl.mTex;
			f32 w = GXGetTexObjWidth(&tex);
			f32 h = GXGetTexObjHeight(&tex);
			Vec pos;
			if (scrn->mObjectGaugeParam == 0) {
				pos = fn_800E5A2C(gGame.GetPlayer());
			} else {
				CVec joint       = gGame.GetPlayer()->GetJointWorldPos("neck");
				const Vec offset = { 0.0f, 2.0f, 0.0f };
				pos              = joint + offset;
			}
			f32 sx;
			f32 sy;
			gGame.ProjectOffsetToScreen(&pos, &sx, &sy, NULL, NULL);
			f32 x = 8.0f + (w * 0.5f + sx);
			f32 y = sy - h * 0.5f - 8.0f;
			if (scrn->mObjectGaugeParam == 1) {
				x += 20.0f;
				y += 10.0f;
			}
			u32 color = (u32)(255.0f * alpha) | 0xC8C8C800;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[267], 0);
			tex        = gGame.mTpl.mTex;
			u16 th     = GXGetTexObjHeight(&tex);
			u16 tw     = GXGetTexObjWidth(&tex);
			s32 frames = tw / th;
			s32 frame  = scrn->mFrameCount % 32;
			s32 idx;
			switch (scrn->mObjectGaugeParam) {
			case 0:
				idx = frame < 8 ? 1 : 0;
				break;
			case 1:
				idx = frame < 8 ? 2 : 3;
				break;
			case 2:
				idx = frame < 8 ? 1 : 4;
				break;
			case 3:
				idx = scrn->mFrameCount % 14 < 10 ? 1 : 7;
				break;
			}
			PSMTXTrans(tex.mTexMtx.mMtx[0], (f32)idx * (1.0f / (f32)frames), 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f / (f32)frames, 1.0f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			u32 color2         = (u32)(255.0f * alpha) | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color2, color2, color2, color2, x, y - 4.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
}

/**
 * @note Address: 0x8008DC34
 * @note Size: 0x4E8
 */
extern "C" void InfoScrn_DrawPlayerHudIcon(InfoScrn* scrn)
{
	f32 alpha = scrn->mPlayerIconAlpha * gHudOpacityScale;
	if (alpha > 0.0f) {
		CTexObj tex;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[111], 0);
		tex     = gGame.mTpl.mTex;
		f32 w   = GXGetTexObjWidth(&tex);
		f32 h   = GXGetTexObjHeight(&tex);
		Vec ofs = { 0.0f, 10.0f, 0.0f };
		f32 sx;
		f32 sy;
		gGame.GetPlayer()->ProjectOffsetToScreen(&ofs, &sx, &sy, NULL, NULL);
		f32 x     = 6.0f + (w * 0.5f + sx);
		f32 y     = sy - h * 0.5f - 6.0f;
		u32 color = (u32)(255.0f * alpha) | 0xC8C8C800;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[267], 0);
		tex        = gGame.mTpl.mTex;
		u16 th     = GXGetTexObjHeight(&tex);
		u16 tw     = GXGetTexObjWidth(&tex);
		s32 frames = tw / th;
		s32 frame  = scrn->mFrameCount % 16;
		s32 idx    = frame < 4 ? 5 : frame < 8 ? 6 : frame >= 12 ? 7 : 6;
		PSMTXTrans(tex.mTexMtx.mMtx[0], (f32)idx * (1.0f / (f32)frames), 0.0f, 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f / (f32)frames, 1.0f, 1.0f);
		tex.mTexMtx.mDirty = TRUE;
		u32 color2         = (u32)(255.0f * alpha) | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color2, color2, color2, color2, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
}

/**
 * @note Address: 0x8008E11C
 * @note Size: 0x8A0
 */
extern "C" void InfoScrn_DrawTimer(InfoScrn* scrn)
{
	f32 alpha = scrn->mTimerAlpha * gHudOpacityScale;
	CTexObj tex;
	if (alpha > 0.0f) {
		u32 opacity   = 255.0f * alpha;
		u32 top       = opacity | 0xEFEBC500;
		u32 bottom    = opacity | 0xEDA83600;
		f32 clock     = TimerFramesToHundredths(scrn->mTimer);
		f32 rightEdge = 372.0f + ((f32)gPointerScreenWidth - 640.0f);
		f32 step;
		f32 w;
		f32 h;
		f32 smallW;
		f32 smallH;
		f32 s = 1.8f;
		// mm:ss.hh as the six decimal digits mmsshh
		f32 value = 10000.0f * (s32)(clock / 6000.0f) + (s32)clock % 6000;
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
		tex    = gGame.mTpl.mTex;
		u16 tw = GXGetTexObjWidth(&tex);
		w      = s * tw;
		u16 th = GXGetTexObjHeight(&tex);
		h      = s * (th / 10);
		tw     = GXGetTexObjWidth(&tex);
		smallW = 0.7f * (s * tw);
		th     = GXGetTexObjHeight(&tex);
		smallH = 0.7f * (s * (th / 10));
		PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
		tex.mTexMtx.mDirty = TRUE;
		step               = 7.0f * w;
		s32 digit;
		for (s32 i = 0; i < 6; i++) {
			f32 div = 1.0f;
			if (i > 0) {
				for (s32 j = 0; j < i; j++) {
					div *= 10.0f;
				}
			}
			digit = (s32)(value / div);
			digit %= 10;
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			// digit i from the right: the hundredths (0, 1) are drawn smaller, the minutes (4, 5) make room for the colon,
			// and the minutes' tens stay blank at 0
			f32 ofs = 0.0f;
			if (i == 0) {
				ofs = -4.0f * s;
			} else if (i == 1) {
				ofs = -1.0f * s;
			} else if (i == 4 || i == 5) {
				ofs = -5.0f * s;
			}
			if (i == 5 && digit == 0) {
				continue;
			}
			if (i < 2) {
				DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, ofs + (step + (rightEdge - i * (w - 2.0f * s)) - 8.0f * s),
				                   72.0f + 2.0f * s, 0.0f, smallW, smallH, 0.0f, 0.0f, 0.0f);
			} else {
				DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, ofs + (step + (rightEdge - i * (w - 2.0f * s)) - 8.0f * s),
				                   72.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			}
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[130], 0);
		tex        = gGame.mTpl.mTex;
		u16 colonW = GXGetTexObjWidth(&tex);
		u16 colonH = GXGetTexObjHeight(&tex);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, rightEdge + 56.0f * s, 72.0f + 0.5f * s, 0.0f, 0.6f * (s * colonW),
		                   0.6f * (s * colonH), 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
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

/**
 * @note Address: 0x8008EC74
 * @note Size: 0x32C
 */
extern "C" void DrawHudScanlines(InfoScrn* scrn)
{
	f32 alpha = scrn->mScanlineAlpha;
	if (alpha > 0.0f) {
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		u32 a                = 255.0f * alpha;
		static f32 sScanline = 0.0f;
		if (!gGame.mMenu.mActive) {
			sScanline = 1.0f + sScanline;
			if (sScanline >= 4.0f) {
				sScanline = 0.0f;
			}
		}
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_SUBTRACT, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
		u32 color = a | 0x30303000;
		for (s32 y = 0; y < gPointerScreenHeight; y += 4.0f) {
			DrawGsLineSegment(&gGame.mGs, NULL, 0.0f, y + sScanline, 0.0f, color, (f32)gPointerScreenWidth, y + sScanline, 0.0f, color);
		}
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
		for (int i = 9; i > 0; i--) {
			gHudScanlineHistory[i] = gHudScanlineHistory[i - 1];
		}
		gHudScanlineHistory[0] = 0.0f;
		gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
	}
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
 * @note Address: 0x8008F884
 * @note Size: 0xB40
 */
extern "C" void DrawTexturedWindowFrame(u32 color, u32 textureColor, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 scaleX, f32 scaleY)
{
	f32 angle  = 4.712389f * (1.0f - alpha);
	f32 sx     = 4.0f * (f32)sin(angle);
	f32 sy     = 4.0f * (f32)sin(angle);
	f32 left   = x - width / 2.0f - 40.0f - sx;
	f32 right  = sx + (40.0f + (x + width / 2.0f));
	f32 top    = y - 8.0f - sy;
	f32 bottom = sy + (8.0f + (y + height));
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	DrawGsVertexQuad(&gGame.mGs, left - 8.0f, top - 8.0f, -1.0f, 0xFF000000, 9.0f + (8.0f + right), top - 8.0f, -1.0f, 0xFF000000,
	                 9.0f + (8.0f + right), 9.0f + (8.0f + bottom), -1.0f, 0xFF000000, left - 8.0f, 9.0f + (8.0f + bottom), -1.0f,
	                 0xFF000000);
	Vec pos[10] = {
		(Vec) { x, y, 0.0f },
		(Vec) { (1.0f + left), (8.0f + (1.0f + top)), 0.0f },
		(Vec) { (8.0f + (1.0f + left)), (1.0f + top), 0.0f },
		(Vec) { ((right - 1.0f) - 8.0f), (1.0f + top), 0.0f },
		(Vec) { (right - 1.0f), (8.0f + (1.0f + top)), 0.0f },
		(Vec) { (right - 1.0f), ((bottom - 1.0f) - 8.0f), 0.0f },
		(Vec) { ((right - 1.0f) - 8.0f), (bottom - 1.0f), 0.0f },
		(Vec) { (8.0f + (1.0f + left)), (bottom - 1.0f), 0.0f },
		(Vec) { (1.0f + left), ((bottom - 1.0f) - 8.0f), 0.0f },
		(Vec) { (1.0f + left), (8.0f + (1.0f + top)), 0.0f },
	};
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, pos, color, 10);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_TRUE);
	DrawGsCircle(&gGame.mGs, 16, color, color, (8.0f + (1.0f + left)), (8.0f + (1.0f + top)), 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, color, color, ((right - 1.0f) - 8.0f), (8.0f + (1.0f + top)), 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, color, color, (8.0f + (1.0f + left)), ((bottom - 1.0f) - 8.0f), 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, color, color, ((right - 1.0f) - 8.0f), ((bottom - 1.0f) - 8.0f), 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
	u32 shadow = (u32)(64.0f * alpha);
	for (int i = 0; i < 10; i++) {
		pos[i].x += 9.0f;
		pos[i].y += 9.0f;
	}
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, pos, shadow, 10);
	DrawGsCircle(&gGame.mGs, 16, shadow, shadow, (8.0f + (1.0f + (9.0f + left))), (8.0f + (1.0f + (9.0f + top))), 0.0f, 16.0f, 16.0f, 0.0f,
	             0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, shadow, shadow, ((9.0f + right) - 1.0f - 8.0f), (8.0f + (1.0f + (9.0f + top))), 0.0f, 16.0f, 16.0f, 0.0f,
	             0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, shadow, shadow, (8.0f + (1.0f + (9.0f + left))), ((9.0f + bottom) - 1.0f - 8.0f), 0.0f, 16.0f, 16.0f, 0.0f,
	             0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, shadow, shadow, ((9.0f + right) - 1.0f - 8.0f), ((9.0f + bottom) - 1.0f - 8.0f), 0.0f, 16.0f, 16.0f, 0.0f,
	             0.0f, 0.0f);
	CTexObj tex;
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[115], 0);
	tex    = gGame.mTpl.mTex;
	f32 tw = GXGetTexObjWidth(&tex);
	f32 th = GXGetTexObjHeight(&tex);
	f32 w  = right - left - 128.0f;
	if (w < 0.0f)
		w = 0.0f;
	f32 h = bottom - top - 64.0f;
	if (h < 0.0f)
		h = 0.0f;
	f32 tiles[8][8] = {
		{ 0.0f, 0.0f, .5f, .5f, (32.0f + left - 3.0f), (8.0f + ((16.0f + top) - 10.0f)), 64.0f, 32.0f },
		{ .5f, 0.0f, 1.0f / tw, .5f, x, (8.0f + ((16.0f + top) - 10.0f)), (6.0f + w), 32.0f },
		{ .5f, 0.0f, .5f, .5f, (3.0f + (right - 32.0f)), (8.0f + ((16.0f + top) - 10.0f)), 64.0f, 32.0f },
		{ 0.0f, .5f, .5f, 1.0f / th, (32.0f + left - 3.0f), (4.0f + (height / 2.0f + y)), 64.0f, ((20.0f + h) - 8.0f) },
		{ .5f, .5f, .5f, 1.0f / th, (3.0f + (right - 32.0f)), (4.0f + (height / 2.0f + y)), 64.0f, ((20.0f + h) - 8.0f) },
		{ 0.0f, .5f, .5f, .5f, (32.0f + left - 3.0f), (10.0f + (bottom - 16.0f)), 64.0f, 32.0f },
		{ .5f, .5f, 1.0f / tw, .5f, x, (10.0f + (bottom - 16.0f)), (6.0f + w), 32.0f },
		{ .5f, .5f, .5f, .5f, (3.0f + (right - 32.0f)), (10.0f + (bottom - 16.0f)), 64.0f, 32.0f },
	};
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_TRUE);
	for (int i = 0; i < 8; i++) {
		PSMTXTrans(tex.mTexMtx.mMtx[0], tiles[i][0], tiles[i][1], 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		PSMTXScale(tex.mTexMtx.mMtx[2], tiles[i][2], tiles[i][3], 1.0f);
		tex.mTexMtx.mDirty = TRUE;
		DrawGsTexturedQuad(&gGame.mGs, &tex, textureColor, textureColor, textureColor, textureColor, tiles[i][4], tiles[i][5], 1.0f,
		                   tiles[i][6], tiles[i][7], 0.0f, 0.0f, 0.0f);
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	DrawGsVertexQuad(&gGame.mGs, left - 8.0f, top - 8.0f, -1.0f, 0xFF000000, 9.0f + (8.0f + right), top - 8.0f, -1.0f, 0xFF000000,
	                 9.0f + (8.0f + right), 9.0f + (8.0f + bottom), -1.0f, 0xFF000000, left - 8.0f, 9.0f + (8.0f + bottom), -1.0f,
	                 0xFF000000);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
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
		point->mPos[i].x = (f32)gScreenSafeLeft - 204.0f;
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

// a quarter of the way there each frame, at least 1
static inline f32 ApproachStep(f32 cur, f32 target)
{
	f32 step = (target - cur) / 4;
	if (step < 0.0f) {
		step = -step;
	}
	if (step <= 1.0f) {
		step = 1.0f;
	}
	return step;
}

// the chibi-pc menu is open, the counters stay hidden under it
static inline BOOL IsPcMenuOpen()
{
	CMenu* menu = gGame.mMenu.mMenus[3];
	return menu ? menu->IsOpen() : FALSE;
}

/**
 * @note Address: 0x80090718
 * @note Size: 0xE20
 */
extern "C" void UpdateHudPointValue(InfoPoint* point, u32 frames)
{
	switch (point->mIndex) {
	case INFOPOINT_ID + 2:
		point->mValue[1] = gGame.GetPlayer()->mParams.mExperience;
		break;
	case INFOPOINT_ID:
		point->mValue[1] = gGame.GetPlayer()->mParams.mMoney;
		break;
	case INFOPOINT_ID + 1:
		point->mValue[1] = gGame.GetPlayer()->mParams.mScrap;
		break;
	}

	bool changed = false;
	if (point->mValue[1] - point->mValue[0]) {
		changed = true;
	}
	if (IsPcMenuOpen()) {
		point->mValue[0]   = point->mValue[1];
		point->mValue[2]   = point->mValue[3];
		point->mPos[1].x   = (f32)gScreenSafeLeft - 204.0f;
		point->mPos[0].x   = (f32)gScreenSafeLeft - 204.0f;
		point->mPos[3].x   = (f32)gScreenSafeLeft - 204.0f;
		point->mPos[2].x   = (f32)gScreenSafeLeft - 204.0f;
		point->mPos[5].x   = (f32)gScreenSafeLeft - 204.0f;
		point->mPos[4].x   = (f32)gScreenSafeLeft - 204.0f;
		point->mSlideState = 0;
		changed            = false;
	} else {
		f32 step = (point->mValue[1] - point->mValue[0]) / 4;
		if (step < 0.0f) {
			step = -step;
		}
		f32 speed = step;
		if (step <= 1.0f) {
			speed = 1.0f;
		}
		Approach(&point->mValue[0], &point->mValue[1], speed);
		Approach(&point->mValue[2], &point->mValue[3], 1.0f);
	}
	Approach(&point->mPos[0].x, &point->mPos[1].x, ApproachStep(point->mPos[0].x, point->mPos[1].x));
	Approach(&point->mPos[2].x, &point->mPos[3].x, ApproachStep(point->mPos[2].x, point->mPos[3].x));
	Approach(&point->mPos[4].x, &point->mPos[5].x, ApproachStep(point->mPos[4].x, point->mPos[5].x));

	// chibi robo has stood still for a second (his idle timer ran down), the counters slide in
	bool empty = false;
	if (0.0f == gGame.GetPlayer()->mIdleTimer) {
		empty = true;
	}
	if (point->mSuppressIdleDisplay) {
		empty = false;
	}
	Vec corner = GetCornerIconPosition(&gGame);
	f32 rowY   = corner.y;
	if (rowY > GetHudBottomEdge()) {
		rowY = GetHudBottomEdge();
	}
	switch (point->mIndex) {
	case INFOPOINT_ID + 2:
		point->mPos[0].y = rowY - 58.0f - 40.0f;
		point->mPos[1].y = rowY - 58.0f - 40.0f;
		break;
	case INFOPOINT_ID:
		point->mPos[0].y = rowY - 58.0f;
		point->mPos[1].y = rowY - 58.0f;
		break;
	case INFOPOINT_ID + 1:
		point->mPos[0].y = 40.0f + (rowY - 58.0f);
		point->mPos[1].y = 40.0f + (rowY - 58.0f);
		break;
	}

	// 0 hidden; 1-3 he stands still: the ring, the digits and the icon slide in one after another and stay; 4-5 out again.
	// 10-12 a count changed: in quicker, and out through 13-14 once 90 frames pass without a change
	switch (point->mSlideState) {
	case 0:
		if (IsPcMenuOpen()) {
			break;
		}
		if (empty) {
			point->mPos[1].x   = (f32)gScreenSafeLeft + 22.0f;
			point->mSlideDelay = 0;
			point->mSlideState++;
		} else if (changed) {
			point->mPos[1].x   = (f32)gScreenSafeLeft + 22.0f;
			point->mSlideDelay = 0;
			point->mSlideState = 10;
		}
		break;
	case 1:
		point->mSlideDelay++;
		if (point->mSlideDelay > 6) {
			point->mPos[3].x   = (f32)gScreenSafeLeft + 22.0f;
			point->mSlideDelay = 0;
			point->mSlideState++;
		}
		break;
	case 2:
		point->mSlideDelay++;
		if (point->mSlideDelay > 6) {
			point->mPos[5].x = (f32)gScreenSafeLeft + 22.0f;
			point->mSlideState++;
		}
		break;
	case 3:
		if (!empty) {
			point->mPos[1].x   = (f32)gScreenSafeLeft - 204.0f;
			point->mSlideDelay = 0;
			point->mSlideState++;
		}
		break;
	case 4:
		if (changed) {
			point->mPos[1].x   = (f32)gScreenSafeLeft + 22.0f;
			point->mSlideDelay = 0;
			point->mSlideState = 10;
		} else {
			point->mSlideDelay++;
			if (point->mSlideDelay > 6) {
				point->mPos[5].x   = (f32)gScreenSafeLeft - 204.0f;
				point->mSlideDelay = 0;
				point->mSlideState++;
			}
		}
		break;
	case 5:
		if (changed) {
			point->mPos[1].x   = (f32)gScreenSafeLeft + 22.0f;
			point->mSlideDelay = 0;
			point->mSlideState = 10;
		} else {
			point->mSlideDelay++;
			if (point->mSlideDelay > 6) {
				point->mPos[3].x   = (f32)gScreenSafeLeft - 204.0f;
				point->mSlideState = 0;
			}
		}
		break;
	case 10:
		point->mSlideDelay++;
		if (point->mSlideDelay > 3) {
			point->mPos[3].x   = (f32)gScreenSafeLeft + 22.0f;
			point->mSlideDelay = 0;
			point->mSlideState++;
		}
		break;
	case 11:
		point->mSlideDelay++;
		if (point->mSlideDelay > 3) {
			point->mPos[5].x   = (f32)gScreenSafeLeft + 22.0f;
			point->mSlideDelay = 0;
			point->mSlideState++;
		}
		break;
	case 12:
		if (empty) {
			point->mSlideState = 3;
		} else {
			point->mSlideDelay++;
			if (changed) {
				point->mSlideDelay = 0;
			}
			if (point->mSlideDelay > 90) {
				point->mPos[1].x   = (f32)gScreenSafeLeft - 204.0f;
				point->mSlideDelay = 0;
				point->mSlideState++;
			}
		}
		break;
	case 13:
		if (changed) {
			point->mPos[1].x   = (f32)gScreenSafeLeft + 22.0f;
			point->mSlideDelay = 0;
			point->mSlideState = 10;
		} else {
			point->mSlideDelay++;
			if (point->mSlideDelay > 6) {
				point->mPos[5].x   = (f32)gScreenSafeLeft - 204.0f;
				point->mSlideDelay = 0;
				point->mSlideState++;
			}
		}
		break;
	case 14:
		if (changed) {
			point->mPos[1].x   = (f32)gScreenSafeLeft + 22.0f;
			point->mSlideDelay = 0;
			point->mSlideState = 10;
		} else {
			point->mSlideDelay++;
			if (point->mSlideDelay > 6) {
				point->mPos[3].x   = (f32)gScreenSafeLeft - 204.0f;
				point->mSlideState = 0;
			}
		}
		break;
	}

	if (point->mVisible) {
		point->mPos[1].x   = (f32)gScreenSafeLeft + 22.0f;
		point->mPos[3].x   = (f32)gScreenSafeLeft + 22.0f;
		point->mPos[5].x   = (f32)gScreenSafeLeft + 22.0f;
		point->mSlideDelay = 0;
		point->mSlideState = 3;
	}

	switch (point->mFadeState) {
	case 0:
		point->mOpacity = 1.0f;
		break;
	case 1:
		point->mOpacity = 1.0f;
		break;
	case 2:
		point->mOpacity = 0.0f;
		break;
	case 3:
		point->mOpacity += 0.08f * frames;
		if (point->mOpacity >= 1.0f) {
			point->mOpacity = 1.0f;
		}
		if (gGame.mEvt.mData.mScript) {
			point->mFadeState = 4;
		}
		break;
	case 4:
		point->mOpacity -= 0.08f * frames;
		if (point->mOpacity < 0.0f) {
			point->mOpacity = 0.0f;
		}
		if (!gGame.mEvt.mData.mScript) {
			point->mFadeState = 3;
		}
		break;
	}

	point->mIconAlphaScale = 1.0f;
	switch (point->mIndex) {
	case INFOPOINT_ID + 1:
		if (IsUtilibotStatusIconUnlocked()) {
			point->mState          = 0;
			point->mPulseFrames    = 0;
			point->mIconAlphaScale = 1.0f;
			break;
		}
		// from 300 scrap the icon blinks, with a chime as it starts
		switch (point->mState) {
		case 0:
			if (point->mValue[1] >= 300.0f) {
				point->mState++;
				point->mPulseFrames = 0;
			}
			break;
		case 1:
			point->mPulseFrames += frames;
			point->mIconAlphaScale = sin(DEG2RAD * ((point->mPulseFrames * 6) % 360));
			if (point->mIconAlphaScale > 0.0f) {
				PlayWiiSoundEffect(&gGame.mSound, 0x1F0, 1.0f, 0.0f);
				point->mState++;
			}
			break;
		case 2:
			point->mPulseFrames += frames;
			point->mIconAlphaScale = sin(DEG2RAD * ((point->mPulseFrames * 6) % 360));
			if (point->mValue[1] < 300.0f) {
				point->mState = 0;
			}
			break;
		case 3:
			if (point->mValue[1] < 300.0f) {
				point->mState = 0;
			}
			break;
		}
		break;
	}
}

// NONMATCHING: functionally complete Wii port of the GameCube counter. The Wii drops the
// 65-vertex line circle for a textured quad (descriptor 269, 33x33) and whites it out
// (0xFFFFFF00 | a); the circle is still computed into pos[0..64] and then only slots 0 and
// 1 are overwritten for the underline. Descriptors 125->126, 155->156 against the GameCube.
/**
 * @note Address: 0x80091538
 * @note Size: 0xD0C
 */
extern "C" void DrawHudPointCounter(InfoPoint* point)
{
	f32 alpha = point->mOpacity * gHudOpacityScale;
	if (alpha > 0.0f) {
		CTexObj tex;
		s32 len = 1;
		f32 x0  = point->mPos[0].x;
		f32 y0  = point->mPos[0].y;
		f32 x2  = point->mPos[2].x;
		f32 x4  = point->mPos[4].x;
		while (true) {
			if ((s32)(point->mValue[0] / Pow10(len)) == 0) {
				break;
			}
			len++;
		}

		Vec pos[65];
		f32 cy = y0 - 6.0f * 1.2f;
		for (int i = 0; i < 65; i++) {
			f32 s;
			f32 angle = 0.09817477f * i - HALF_PI;
			s         = sin(angle);
			f32 c     = -(f32)cos(angle);
			pos[i].x  = x0 + s * (12.0f * 1.2f);
			pos[i].y  = cy + c * (12.0f * 1.2f);
			pos[i].z  = 0.0f;
		}
		u32 a = (u32)(255.0f * alpha);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[269], 0);
		tex = gGame.mTpl.mTex;
		DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00,
		                   (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00, x0, cy, 0.0f, 33.0f, 33.0f, 0.0f, 0.0f,
		                   0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);

		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
		tex              = gGame.mTpl.mTex;
		u16 tw           = GXGetTexObjWidth(&tex);
		f32 lineY        = cy + 12.0f * 1.2f;
		point->mValue[3] = len * (1.2f * tw - 3.0f * 1.2f) + 34.0f;
		pos[0].x         = x0;
		pos[0].y         = lineY;
		pos[1].x         = x0 + point->mValue[2];
		pos[1].y         = lineY;
		gGame.mGs.mCurRenderState->SetLineWidth(0x12, GX_TO_ONE);
		DrawGsLineStrip(&gGame.mGs, pos, a, 2);
		gGame.mGs.mCurRenderState->SetLineWidth(0x0C, GX_TO_ONE);

		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[156], 0);
		tex = gGame.mTpl.mTex;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		u16 ew = GXGetTexObjWidth(&tex);
		u16 eh = GXGetTexObjHeight(&tex);
		DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
		                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, pos[1].x, pos[1].y, 0.0f, 1.2f * ew,
		                   1.2f * eh, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);

		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[point->mResource], 0);
		tex = gGame.mTpl.mTex;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		f32 iconW = 0.8f * GXGetTexObjWidth(&tex);
		f32 iconY = y0 - 18.0f * 0.5f;
		f32 iconH = 0.8f * GXGetTexObjHeight(&tex);
		u32 ic    = (u32)(255.0f * alpha * point->mIconAlphaScale) | 0xC8C8C800;
		if (point->mIndex == INFOPOINT_ID + 2) {
			DrawGsTexturedQuad(&gGame.mGs, &tex, ic, ic, ic, ic, x4, 4.0f + iconY, 0.0f, iconW, iconH, 0.0f, 0.0f, 0.0f);
		} else {
			DrawGsTexturedQuad(&gGame.mGs, &tex, ic, ic, ic, ic, x4, iconY, 0.0f, iconW, iconH, 0.0f, 0.0f, 0.0f);
		}

		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[126], 0);
		tex    = gGame.mTpl.mTex;
		f32 dx = x2 - 12.0f;
		f32 dy = y0 - 4.0f;
		f32 w  = 1.2f * GXGetTexObjWidth(&tex);
		f32 h  = 1.2f * (GXGetTexObjHeight(&tex) / 10);
		PSMTXScale(tex.mTexMtx.mMtx[2], 1.0f, 0.1f, 1.0f);
		f32 step           = 7.0f * w;
		f32 base           = len * (w - 3.0f * 1.2f) - 98.0f * 1.2f;
		tex.mTexMtx.mDirty = TRUE;
		for (int i = 0; i < 7; i++) {
			s32 v = (s32)(point->mValue[0] / Pow10(i));
			if (i != 0 && v == 0) {
				continue;
			}
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.1f * (v % 10), 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			u32 top;
			u32 bottom;
			switch (point->mIndex) {
			case INFOPOINT_ID + 2:
				top    = (u8)(s32)(255.0f * alpha) | 0xCECEFF00;
				bottom = (u8)(s32)(255.0f * alpha) | 0xE037EE00;
				break;
			case INFOPOINT_ID:
				top    = (u8)(s32)(255.0f * alpha) | 0xEFEBC500;
				bottom = (u8)(s32)(255.0f * alpha) | 0xEDA83600;
				break;
			case INFOPOINT_ID + 1:
				top    = (u8)(s32)(255.0f * alpha) | 0xCECEFF00;
				bottom = (u8)(s32)(255.0f * alpha) | 0x5959FF00;
				break;
			}
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, base + (step + (dx - i * (w - 3.0f * 1.2f))), dy - 2.0f, 0.0f, w,
			                   h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
}

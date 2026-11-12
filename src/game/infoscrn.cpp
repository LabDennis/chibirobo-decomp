#include "game/infoscrn.h"
#include "game/infopoint.h"
#include "game/hudcounterhelpers.h"
#include "game/cgs.h"
#include "game/ctpl.h"
#include "game/xchr.h"
#include "game/stg.h"
#include "game/video.h"
#include <string.h>
#include <math.h>

// the info screen, the hud drawn over the game: the clock, the battery, the happy points and
// the three counters that pop up in the corner. CGame keeps it in mInfoScrn

// the timer counts frames, europe at the video mode's rate
#if defined(VERSION_PAL)
#define TIMER_RATE gVideoFramesPerSecond
#else
#define TIMER_RATE 60
#endif
// 99:59.99, the most the timer shows
#define TIMER_MAX ((s32)((u32)TIMER_RATE * 6000U - 1U))

extern "C" {
u16 GXGetTexObjWidth(const GXTexObj* obj);
u16 GXGetTexObjHeight(const GXTexObj* obj);
void PSMTXTrans(Mtx m, f32 x, f32 y, f32 z);
void PSMTXScale(Mtx m, f32 x, f32 y, f32 z);
void DrawHudPointCounter(InfoPoint* point);
void DrawHudScanlines(InfoScrn* scrn);
void InfoScrn_DrawTimer(InfoScrn* scrn);
void InfoScrn_DrawPlayerHudIcon(InfoScrn* scrn);
void DrawHudClock(InfoScrn* scrn);
void DrawHudBatteryGauge(InfoScrn* scrn);
void DrawGameHud(InfoScrn* scrn);
void UpdateHudIndicators(InfoScrn* scrn, u32 frames);
const char* GetLocalizedInterfaceText(s32 id);
}

// The original HUD code addresses the counters relative to this regional BSS prefix.
extern "C" {
#if defined(VERSION_JP)
u8 gHudPointCounterStorage[0x134];
#else
u8 gHudPointCounterStorage[0x140];
#endif
}
// todo: the three counters are infopointinit.cpp's, which is cut from this file. These are the file's own
// copies, as on main, so that its code reaches them the way retail does
static InfoPoint gHudMoneyCounter;
static InfoPoint gHudHappyPointCounter;
static InfoPoint gHudScrapCounter;
extern "C" {
f32 gHudScanlineHistory[10];
}

// how bright the hud is, the counters and gauges multiply their alpha by it
extern "C" f32 gHudOpacityScale;

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

// 10 to the n
static inline f32 Pow10(int n)
{
	f32 p = 1.0f;
	for (int i = 0; i < n; i++) {
		p *= 10.0f;
	}
	return p;
}

// how many items chibi robo can carry, by the upgrade he holds
static inline s32 ItemMax()
{
	XObjChr* player = gGame.GetPlayer();
	for (int i = 0; i < 256; i++) {
		s16 id    = player->mParams.mInventory.mItems[i].mId;
		s16 count = player->mParams.mInventory.mItems[i].mCount;
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

extern "C" {
void DrawScreenColorOverlay(CScrnColor* color, CGs* gs);
void UpdateScreenFade(CScrnColor* color, u32 frames);
}

// an event is running that keeps the hud up
static inline bool IsEventHud()
{
	bool on = false;
	if ((gGame.mDvdStatus != 0 || gGame.mPrevDvdStatus != 0) && gGame.mDiskOk == 0) {
		on = true;
	}
	return on;
}

// the hud is drawn in the field, outside of events
static inline bool IsHudOn()
{
	bool on = false;
	if ((gGame.mStatus.mFlags & 1) && !(gGame.mStatus.mFlags & 0x22) && !IsEventHud()) {
		on = true;
	}
	return on;
}

/**
 * @note Address: 0x8006D850
 * @note Size: 0x2A4
 */
extern "C" void InitInfoScreen(InfoScrn* scrn)
{
	scrn->mFade.Reset();
	scrn->mWipe.Reset();
	scrn->_A8           = 0.0f;
	scrn->_AC           = 0.0f;
	scrn->_B0           = 0.0f;
	scrn->_140          = 0.0f;
	scrn->_144          = 0.0f;
	scrn->_98           = 0;
	scrn->_9C           = 0;
	scrn->_A0           = 0;
	scrn->_A4           = 0;
	scrn->_B4           = 0;
	scrn->mBatteryGauge = 0.0f;
	scrn->_B8           = 0;
	scrn->_17C          = 0;
	scrn->_180          = 0;
	InitHudPointCounter(&gHudHappyPointCounter, INFOPOINT_ID + 2);
	InitHudPointCounter(&gHudMoneyCounter, INFOPOINT_ID);
	InitHudPointCounter(&gHudScrapCounter, INFOPOINT_ID + 1);
	gHudHappyPointCounter.mVisible = FALSE;
	gHudMoneyCounter.mVisible      = FALSE;
	gHudScrapCounter.mVisible      = FALSE;
	scrn->_BC                      = 0;
	scrn->_C0                      = 0.0f;
	scrn->_C4                      = 0.0f;
	scrn->_C8                      = 0.0f;
	const Vec v0                   = { 0.0f, 0.0f, 0.0f };
	scrn->_128.x                   = v0.x;
	scrn->_128.y                   = v0.y;
	scrn->_128.z                   = v0.z;
	scrn->_148                     = 0;
	scrn->_14C                     = 0.0f;
	scrn->_150                     = 0;
	scrn->_154                     = 0;
	scrn->_158                     = 0;
	const Vec v1                   = { 0.0f, 0.0f, 0.0f };
	scrn->_170.x                   = v1.x;
	scrn->_170.y                   = v1.y;
	scrn->_170.z                   = v1.z;
	scrn->_16C                     = 1.0f;
	scrn->_188                     = 0;
	scrn->_18C                     = 0.0f;
	const Vec v2                   = { 0.0f, 0.0f, 0.0f };
	scrn->_190.x                   = v2.x;
	scrn->_190.y                   = v2.y;
	scrn->_190.z                   = v2.z;
	scrn->_19C                     = NULL;
	scrn->_1A0                     = 0;
	scrn->_1A4                     = 0.0f;
	const Vec v3                   = { 0.0f, 0.0f, 0.0f };
	scrn->_1A8.x                   = v3.x;
	scrn->_1A8.y                   = v3.y;
	scrn->_1A8.z                   = v3.z;
	scrn->_1B4                     = 0;
	scrn->_1B8                     = 0.0f;
	scrn->mTimer                   = 0.0f;
	scrn->mTimerShown              = 0;
	scrn->mScanlineVisibilityState = 0;
	scrn->mScanlineAlpha           = 0.0f;
	scrn->_1CC                     = 0.0f;
	scrn->_184                     = 0;
	scrn->mSelectedSaveSlot        = 0;
	scrn->_CC                      = 0.0f;
	scrn->_D0                      = 0.0f;
	scrn->_D4                      = 0;
	scrn->_D8                      = 0;
	scrn->_DC                      = 1.4f;
	f32 d                          = scrn->_CC - scrn->_D0;
	for (int i = 0; i < 4; i++) {
		scrn->_E0[i]  = 0.0f;
		scrn->_F0[i]  = 0;
		scrn->_100[i] = 0;
		scrn->_104[i] = d;
		scrn->_114[i] = 0.0f;
	}
	scrn->_124       = 0;
	scrn->_134       = 0;
	scrn->_138       = 0.0f;
	scrn->_13C       = 0.0f;
	gHudOpacityScale = 1.0f;
	scrn->mItemMax   = 5;
}

/**
 * @note Address: 0x8006D824
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
 * @note Address: 0x8006D5A4
 * @note Size: 0x280
 */
extern "C" void ResetGameHudState(InfoScrn* scrn)
{
	scrn->_A8           = 0.0f;
	scrn->_AC           = 0.0f;
	scrn->_B0           = 0.0f;
	scrn->_140          = 0.0f;
	scrn->_144          = 0.0f;
	scrn->_98           = 0;
	scrn->_9C           = 0;
	scrn->_A0           = 0;
	scrn->_A4           = 0;
	scrn->_B4           = 0;
	scrn->mBatteryGauge = 0.0f;
	scrn->_B8           = 0;
	scrn->_17C          = 0;
	scrn->_180          = 0;
	InitHudPointCounter(&gHudHappyPointCounter, INFOPOINT_ID + 2);
	InitHudPointCounter(&gHudMoneyCounter, INFOPOINT_ID);
	InitHudPointCounter(&gHudScrapCounter, INFOPOINT_ID + 1);
	gHudHappyPointCounter.mVisible = FALSE;
	gHudMoneyCounter.mVisible      = FALSE;
	gHudScrapCounter.mVisible      = FALSE;
	scrn->_BC                      = 0;
	scrn->_C0                      = 0.0f;
	scrn->_C4                      = 0.0f;
	scrn->_C8                      = 0.0f;
	const Vec v0                   = { 0.0f, 0.0f, 0.0f };
	scrn->_128.x                   = v0.x;
	scrn->_128.y                   = v0.y;
	scrn->_128.z                   = v0.z;
	scrn->_148                     = 0;
	scrn->_14C                     = 0.0f;
	scrn->_150                     = 0;
	scrn->_154                     = 0;
	scrn->_158                     = 0;
	const Vec v1                   = { 0.0f, 0.0f, 0.0f };
	scrn->_170.x                   = v1.x;
	scrn->_170.y                   = v1.y;
	scrn->_170.z                   = v1.z;
	scrn->_16C                     = 1.0f;
	scrn->_188                     = 0;
	scrn->_18C                     = 0.0f;
	const Vec v2                   = { 0.0f, 0.0f, 0.0f };
	scrn->_190.x                   = v2.x;
	scrn->_190.y                   = v2.y;
	scrn->_190.z                   = v2.z;
	scrn->_19C                     = NULL;
	scrn->_1A0                     = 0;
	scrn->_1A4                     = 0.0f;
	const Vec v3                   = { 0.0f, 0.0f, 0.0f };
	scrn->_1A8.x                   = v3.x;
	scrn->_1A8.y                   = v3.y;
	scrn->_1A8.z                   = v3.z;
	scrn->_1B4                     = 0;
	scrn->_1B8                     = 0.0f;
	scrn->mTimer                   = 0.0f;
	scrn->mTimerShown              = 0;
	scrn->mScanlineVisibilityState = 0;
	scrn->mScanlineAlpha           = 0.0f;
	scrn->_1CC                     = 0.0f;
	scrn->_184                     = 0;
	scrn->mSelectedSaveSlot        = 0;
	scrn->_CC                      = 0.0f;
	scrn->_D0                      = 0.0f;
	scrn->_D4                      = 0;
	scrn->_D8                      = 0;
	scrn->_DC                      = 1.4f;
	f32 d                          = scrn->_CC - scrn->_D0;
	for (int i = 0; i < 4; i++) {
		scrn->_E0[i]  = 0.0f;
		scrn->_F0[i]  = 0;
		scrn->_100[i] = 0;
		scrn->_104[i] = d;
		scrn->_114[i] = 0.0f;
	}
	scrn->_124       = 0;
	scrn->_134       = 0;
	scrn->_138       = 0.0f;
	scrn->_13C       = 0.0f;
	gHudOpacityScale = 1.0f;
	scrn->mItemMax   = 5;
}

/**
 * @note Address: 0x8006D548
 * @note Size: 0x5C
 */
extern "C" void UpdateHudTransitionsAndIndicators(InfoScrn* scrn, u32 frames)
{
	UpdateScreenFade(&scrn->mFade, frames);
	scrn->mWipe.Update(frames);
	UpdateHudIndicators(scrn, frames);
}

/**
 * @note Address: 0x8006D498
 * @note Size: 0xB0
 */
extern "C" void DrawHudWithTransitions(InfoScrn* scrn)
{
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 3, 0);
	DrawScreenColorOverlay(&scrn->mFade, &gGame.mGs);
	scrn->mWipe.Draw(&gGame.mGs, 0);
	DrawGameHud(scrn);
}

// the loading bar, the robot walks along it
static const u8 sWalkAnim[]  = { 3, 2, 1, 0, 3, 1, 2, 0xFF };
static const u8 sStandAnim[] = { 0, 1, 2, 3, 0xFF };
static struct {
	f32 mX;          // _00
	s32 mFrame;      // _04
	const u8* mAnim; // _08
} sLoading = { 0.0f, 0, sWalkAnim };

/**
 * @note Address: 0x8006CB68
 * @note Size: 0x930
 */
// NONMATCHING: the robot's x reads the struct through a register, scratch registers
extern "C" void DrawObjectLoadingProgress(InfoScrn* scrn)
{
	if (gGame.mStatus.mFlags & 0x40) {
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
		if (gGame.mXObjs.mObjectCount > 0) {
			f32 fill;
			f32 max;
			f32 y;
			f32 left;
			f32 half;
			f32 tw;
			f32 th;
			f32 scale = 1.0f / gGame.mXObjs.mObjectCount;
			max       = gGame.mXObjs.mObjectCount;
			fill      = max * (gGame.mXObjs.mLoadObjectIndex * scale);
			CTexObj tex;
			if (sLoading.mAnim == sWalkAnim) {
				if (sWalkAnim[sLoading.mFrame] == 0xFF) {
					sLoading.mFrame = 0;
				}
				tex = (&gGame.mTpl)
				          ->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)
				                           ->descriptorArray[0x82 - StatusTextureOffset + sWalkAnim[sLoading.mFrame]],
				                      0);
			} else {
				if (sStandAnim[sLoading.mFrame / 2] == 0xFF) {
					sLoading.mFrame = 0;
				}
				tex = (&gGame.mTpl)
				          ->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)
				                           ->descriptorArray[0x86 - StatusTextureOffset + sStandAnim[sLoading.mFrame / 2]],
				                      0);
			}
			tw = GXGetTexObjWidth(&tex);
			th = GXGetTexObjHeight(&tex);
			SetGsCullMode(gGame.mGs.mCurRenderMode, 1);
			SetGsTevMode(gGame.mGs.mCurTevStage, 2);
			half = max / 2;
			y    = 406.0f - th / 2;
			left = 320.0f - half;
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0x808080C8, 0x808080C8, 0x808080C8, 0x808080C8, sLoading.mX + left, y, 0.0f, 0.9f * tw,
			                   0.9f * th, 0.0f, 3.1415927f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, sLoading.mX + left, y, 0.0f, 0.8f * tw,
			                   0.8f * th, 0.0f, 3.1415927f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
			SetGsCullMode(gGame.mGs.mCurRenderMode, 2);

			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[117 - StatusTextureOffset], 0);
			PSMTXScale(tex.mMtx[1], 0.25f, 1.0f, 1.0f);
			tex.mMtxDirty = TRUE;
			PSMTXTrans(tex.mMtx[0], 0.25f, 0.0f, 0.0f);
			tex.mMtxDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 322.0f, 408.0f, 0.0f, max, 14.0f, 0.0f,
			                   0.0f, 3.1415927f);
			PSMTXTrans(tex.mMtx[0], 0.5f, 0.0f, 0.0f);
			tex.mMtxDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 322.0f - half - 4.0f, 408.0f, 0.0f, 8.0f,
			                   14.0f, 0.0f, 0.0f, 3.1415927f);
			PSMTXTrans(tex.mMtx[0], 0.0f, 0.0f, 0.0f);
			tex.mMtxDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 0xC8C8C880, 4.0f + (322.0f + half), 408.0f, 0.0f, 8.0f,
			                   14.0f, 0.0f, 0.0f, 3.1415927f);

			tex      = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[118 - StatusTextureOffset], 0);
			u16 barW = GXGetTexObjWidth(&tex);
			PSMTXScale(tex.mMtx[1], fill / barW, 1.0f, 1.0f);
			tex.mMtxDirty      = TRUE;
			static f32 sScroll = 0.0f;
			sScroll -= 0.1f;
			PSMTXTrans(tex.mMtx[0], sScroll, 0.0f, 0.0f);
			tex.mMtxDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0x9FFA1CC8, 0x9FFA1CC8, 0x9FFA1CC8, 0x9FFA1CC8, 321.0f - (max - fill) / 2, 408.0f, 0.0f,
			                   fill, 10.0f, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);

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
		const char* text = GetLocalizedInterfaceText(0x59);
		f32 w            = MeasureGameTextWidth(&gGame.mGs, text, 1, 16.0f);
		DrawGsProportionalTextFormat(&gGame.mGs, -1, text, 320.0f - w / 2, 386.0f, 0.0f, 16.0f, 16.0f);
	}
}

/**
 * @note Address: 0x8006CA90
 * @note Size: 0xD8
 */
extern "C" void DrawHudPauseLabel(InfoScrn* scrn)
{
	if (gGame.mStatus.mFlags & 0x200000000000ULL) {
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 3, 0);
		const char* text = "\\B00000080 PAUSE ";
		f32 w            = MeasureGameTextWidth(&gGame.mGs, text, 1, 24.0f);
		DrawGsProportionalTextFormat(&gGame.mGs, -1, text, 320.0f - w / 2, 200.0f, 0.0f, 24.0f, 24.0f);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
	}
}

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

static inline s32 HighlightedAttachment()
{
	const AtcMenu& menu = gGame.GetPlayer()->mMenu.mAtc;
	return menu.mCursorIndex >= 0 ? menu.mItems[menu.mCursorIndex].mType : 0;
}

static inline void AdvanceHudIcon(InfoScrn* h)
{
	if (++h->_17C > 1) {
		if (++h->_180 >= 9)
			h->_180 = 9;
		h->_17C = 0;
	}
}

static inline void ApproachHudScale(f32& current, f32 target)
{
	f32 step = (target - current) / 10;
	if (step < 0)
		step = -step;
	Approach(&current, &target, step);
}

/**
 * @note Address: 0x8006AF34
 * @note Size: 0x1B5C
 */
extern "C" void UpdateHudIndicators(InfoScrn* h, u32 frames)
{
	if (!IsHudOn() && !(gGame.mStatus.mFlags & 0x200000000000ULL))
		return;
	UpdateHudPointValue(&gHudHappyPointCounter, frames);
	UpdateHudPointValue(&gHudMoneyCounter, frames);
	UpdateHudPointValue(&gHudScrapCounter, frames);
	++h->_B4;
	switch (h->_9C) {
	case 0:
		++h->_9C;
		break;
	case 1:
		h->_A8 += 0.08f * frames;
		if (h->_A8 >= 1) {
			h->_A8 = 1;
			++h->_9C;
		}
		break;
	case 2:
		if (gGame.mEvt.mData.mScript)
			++h->_9C;
		break;
	case 3:
		h->_A8 -= 0.08f * frames;
		if (h->_A8 < 0) {
			h->_A8 = 0;
			++h->_9C;
		}
		break;
	case 4:
		if (!gGame.mEvt.mData.mScript)
			h->_9C = 1;
		break;
	}
	h->_A8 = 1;
	CPad pad(0);
	switch (h->_A0) {
	case 0:
		if (gGame.GetPlayer()->GetAttachment()) {
			h->_B8 = -1;
			++h->_A0;
		}
		break;
	case 1:
		h->_AC += 0.08f * frames;
		if (h->_AC >= 1) {
			h->_AC = 1;
			++h->_A0;
		}
		break;
	case 2:
		if (h->_B8 != -1 || !gGame.GetPlayer()->GetAttachment())
			++h->_A0;
		break;
	case 3:
		h->_AC -= 0.08f * frames;
		if (h->_AC < 0) {
			h->_AC = 0;
			h->_A0 = 0;
		}
		break;
	}
	static s32 previousAttachment = HighlightedAttachment();
	switch (h->_A4) {
	case 0:
		if (gGame.mStatus.mFlags & 0x400) {
			++h->_A4;
			previousAttachment = HighlightedAttachment();
			h->_17C            = 0;
			h->_180            = 0;
		}
		break;
	case 1:
		h->_B0 += 0.08f * frames;
		if (h->_B0 >= 1) {
			h->_B0 = 1;
			++h->_A4;
		}
		AdvanceHudIcon(h);
		break;
	case 2:
		if (previousAttachment != HighlightedAttachment()) {
			h->_17C = 0;
			h->_180 = 0;
		}
		previousAttachment = HighlightedAttachment();
		AdvanceHudIcon(h);
		if (!(gGame.mStatus.mFlags & 0x400))
			++h->_A4;
		break;
	case 3:
		h->_B0 -= 0.08f * frames;
		if (h->_B0 < 0) {
			h->_B0 = 0;
			h->_A4 = 0;
		}
		break;
	}
	switch (h->_BC) {
	case 0:
		h->_BC = 3;
		h->_C0 = 0;
		h->_DC = 1.4f;
		break;
	case 1:
		h->_C0 = 1;
		break;
	case 2:
		h->_C0 = 0;
		break;
	case 3:
		h->_C0 += 0.08f * frames;
		if (h->_C0 >= 1)
			h->_C0 = 1;
		break;
	case 4:
		h->_C0 -= 0.08f * frames;
		if (h->_C0 < 0)
			h->_C0 = 0;
		break;
	case 5:
		ApproachHudScale(h->_DC, 1.4f + 0.25f * (f32)sin(0.017453292f * ((s32)((u32)h->_B4 * 16) % 360)));
		break;
	case 6:
		ApproachHudScale(h->_DC, 1.4f);
		break;
	}
	f32 step = (h->_C8 - h->_C4) * 0.25f;
	if (step < 0)
		step = -step;
	Approach(&h->_C4, &h->_C8, step);
	Approach(&h->_140, &h->_144, 0.08f * frames);
	switch (h->_148) {
	case 0:
		h->_148          = 3;
		h->_14C          = 0;
		h->mItemMaxAlpha = h->_14C;
		h->mItemMax      = ItemMax();
		h->_16C          = 1;
		break;
	case 1:
		h->_14C          = 1;
		h->mItemMaxAlpha = h->_14C;
		break;
	case 2:
		h->_14C          = 0;
		h->mItemMaxAlpha = h->_14C;
		break;
	case 3:
		h->_14C += 0.08f * frames;
		if (h->_14C >= 1)
			h->_14C = 1;
		h->mItemMaxAlpha = h->_14C;
		break;
	case 4:
		h->_14C -= 0.08f * frames;
		if (h->_14C < 0)
			h->_14C = 0;
		h->mItemMaxAlpha = h->_14C;
		break;
	case 5:
		h->mItemMaxAlpha -= 0.08f * frames;
		if (h->mItemMaxAlpha < 0) {
			h->mItemMaxAlpha = 0;
			h->mItemMax      = h->mNextItemMax;
			PlayGameSound(&gGame.mSound, 0xb6, 255);
			++h->_148;
		}
		break;
	case 6:
		h->mItemMaxAlpha += 0.08f * frames;
		if (h->mItemMaxAlpha >= 1)
			h->mItemMaxAlpha = 1;
		break;
	case 7:
		ApproachHudScale(h->_16C, 1.0f + 0.125f * (f32)sin(0.017453292f * ((s32)((u32)h->_B4 * 16) % 360)));
		break;
	case 8:
		ApproachHudScale(h->_16C, 1);
		break;
	}
	if ((IsHudOn() && !gGame.mEvt.mData.mScript && !gGame.mMenu.mActive && !(gGame.mStatus.mFlags & 0xc00) && gGame.mTimeState.mDuration > 0
	     && !gGame.mTimeState.mRunning)
	    || gGame.mTimeState.mTime >= 1)
		++h->_154;
	CJObj joint;
	CVec position;
	switch (h->_188) {
	case 0:
		h->_188 = 4;
		h->_18C = 0;
		break;
	case 1:
		h->_18C = 1;
		if (h->_19C && (h->_19C->mStatus.mFlags & 1)) {
			s32 path[] = { 0, 0 };
			joint.Attach(FindJointByPath(h->_19C->mJObj, path, 2));
			position = CVec(joint.GetPos());
			Vec offset
			    = { position.x - h->_19C->GetTranslateX(), position.y - h->_19C->GetTranslateY(), position.z - h->_19C->GetTranslateZ() };
			h->_19C->ProjectOffsetToScreen(&offset, &position.x, &position.y, NULL, NULL);
			position.z = 0;
			h->_190    = position;
		}
		break;
	case 2:
		h->_18C = 0;
		break;
	case 3:
		h->_18C += 0.08f * frames;
		if (h->_18C >= 1)
			h->_18C = 1;
		if (h->_19C && (h->_19C->mStatus.mFlags & 1)) {
			s32 path[] = { 0, 0 };
			joint.Attach(FindJointByPath(h->_19C->mJObj, path, 2));
			position = CVec(joint.GetPos());
			Vec offset
			    = { position.x - h->_19C->GetTranslateX(), position.y - h->_19C->GetTranslateY(), position.z - h->_19C->GetTranslateZ() };
			h->_19C->ProjectOffsetToScreen(&offset, &position.x, &position.y, NULL, NULL);
			position.z = 0;
			h->_190    = position;
		}
		break;
	case 4:
		h->_18C -= 0.08f * frames;
		if (h->_18C < 0)
			h->_18C = 0;
		break;
	}
	StepHudVisibility(h->_1A0, h->_1A4, frames);
	StepHudVisibility(h->_1B4, h->_1B8, frames);
	if (h->mTimerShown) {
		h->mTimer += frames;
		if (h->mTimer >= TIMER_MAX) {
			h->mTimer      = TIMER_MAX;
			h->mTimerShown = FALSE;
		}
	}
	StepHudVisibility(h->mScanlineVisibilityState, h->mScanlineAlpha, frames);
	if (gGame.mStatus.mFlags & 0x400) {
		if (gHudOpacityScale > 0 && (gHudOpacityScale -= 0.08f * frames) < 0)
			gHudOpacityScale = 0;
	} else if (gHudOpacityScale < 1 && (gHudOpacityScale += 0.08f * frames) > 1)
		gHudOpacityScale = 1;
}

/**
 * @note Address: 0x8006AB00
 * @note Size: 0x434
 */
// NONMATCHING: the bss address and the zero swap r6 and r7
extern "C" void DrawGameHud(InfoScrn* scrn)
{
	if (IsHudOn() || (gGame.mStatus.mFlags & 0x200000000000ULL)) {
		DrawHudPointCounter(&gHudHappyPointCounter);
		DrawHudPointCounter(&gHudMoneyCounter);
		DrawHudPointCounter(&gHudScrapCounter);
		DrawHudBatteryGauge(scrn);
		DrawHudClock(scrn);
		f32 alpha = scrn->_18C * gHudOpacityScale;
		if (alpha > 0.0f) {
			CTexObj tex;
			if (scrn->_B4 % 32 >= 8) {
				tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[150 - StatusTextureOffset], 0);
			} else {
				tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[149 - StatusTextureOffset], 0);
			}
			f32 h;
			f32 w;
			f32 y;
			f32 x;
			u16 tw = GXGetTexObjWidth(&tex);
			w      = 0.8f * (1.1f * tw);
			u16 th = GXGetTexObjHeight(&tex);
			h      = 0.8f * th;
			x      = 6.0f + (w / 2 + scrn->_190.x);
			y      = scrn->_190.y - h / 2 - 6.0f;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			u32 color = (u32)(255.0f * alpha) | 0xC8C8C800;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
		InfoScrn_DrawPlayerHudIcon(scrn);
		InfoScrn_DrawTimer(scrn);
		DrawHudScanlines(scrn);
	}
}

/**
 * @note Address: 0x80068C8C
 * @note Size: 0x84
 */
extern "C" void ApproachHudValue(f32* cur, f32* target, f32 step)
{
	Approach(cur, target, step);
}

/**
 * @note Address: 0x80068C24
 * @note Size: 0x68
 */
extern "C" void GetHudPointParticleOrigin(InfoScrn* scrn, f32* y, f32* x)
{
	f32 right = GetHudBottomEdge() - 64.0f - 40.0f - 9.0f;
	if (y) {
		*y = 60.0f;
	}
	if (x) {
		*x = right;
	}
}

/**
 * @note Address: 0x80068C10
 * @note Size: 0x14
 */
extern "C" void ShowHappyPointCounter(InfoScrn* scrn)
{
	gHudHappyPointCounter.mVisible = TRUE;
}

/**
 * @note Address: 0x80068BFC
 * @note Size: 0x14
 */
extern "C" void HideHappyPointCounter(InfoScrn* scrn)
{
	gHudHappyPointCounter.mVisible = FALSE;
}

/**
 * @note Address: 0x80068BE8
 * @note Size: 0x14
 */
extern "C" void ShowMoolahCounter(InfoScrn* scrn)
{
	gHudMoneyCounter.mVisible = TRUE;
}

/**
 * @note Address: 0x80068BD4
 * @note Size: 0x14
 */
extern "C" void HideMoolahCounter(InfoScrn* scrn)
{
	gHudMoneyCounter.mVisible = FALSE;
}

/**
 * @note Address: 0x80068BC0
 * @note Size: 0x14
 */
extern "C" void ShowScrapCounter(InfoScrn* scrn)
{
	gHudScrapCounter.mVisible = TRUE;
}

/**
 * @note Address: 0x80068BAC
 * @note Size: 0x14
 */
extern "C" void HideScrapCounter(InfoScrn* scrn)
{
	gHudScrapCounter.mVisible = FALSE;
}

/**
 * @note Address: 0x80068B98
 * @note Size: 0x14
 */
extern "C" void ResumeScrapCounterPulse(InfoScrn* scrn)
{
	gHudScrapCounter.mState = 2;
}

/**
 * @note Address: 0x80068B70
 * @note Size: 0x28
 */
extern "C" void SuppressIdleHudCounters(InfoScrn* scrn)
{
	gHudHappyPointCounter.mSuppressIdleDisplay = TRUE;
	gHudMoneyCounter.mSuppressIdleDisplay      = TRUE;
	gHudScrapCounter.mSuppressIdleDisplay      = TRUE;
}

/**
 * @note Address: 0x80068B48
 * @note Size: 0x28
 */
extern "C" void AllowIdleHudCounters(InfoScrn* scrn)
{
	gHudHappyPointCounter.mSuppressIdleDisplay = FALSE;
	gHudMoneyCounter.mSuppressIdleDisplay      = FALSE;
	gHudScrapCounter.mSuppressIdleDisplay      = FALSE;
}

/**
 * @note Address: 0x80067418
 * @note Size: 0x510
 */
extern "C" void InfoScrn_DrawPlayerHudIcon(InfoScrn* scrn)
{
	f32 alpha = scrn->_1A4 * gHudOpacityScale;
	if (alpha > 0.0f) {
		CTexObj tex;
		s32 frame = scrn->_B4 % 8;
		if (frame < 2) {
			tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[149 - StatusTextureOffset], 0);
		} else if (frame < 4) {
			tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[150 - StatusTextureOffset], 0);
		} else if (frame < 6) {
			tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[149 - StatusTextureOffset], 0);
		} else {
			tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[151 - StatusTextureOffset], 0);
		}
		f32 x;
		f32 y;
		f32 w;
		f32 h;
		u16 tw  = GXGetTexObjWidth(&tex);
		w       = 0.8f * (1.1f * tw);
		u16 th  = GXGetTexObjHeight(&tex);
		h       = 0.8f * th;
		Vec ofs = { 0.0f, 10.0f, 0.0f };
		f32 sx;
		f32 sy;
		gGame.GetPlayer()->ProjectOffsetToScreen(&ofs, &sx, &sy, NULL, 0);
		x = 6.0f + (w / 2 + sx);
		y = sy - h / 2 - 6.0f;
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		u32 color = (u32)(255.0f * alpha) | 0xC8C8C800;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
}

/**
 * @note Address: 0x80066C08
 * @note Size: 0x810
 */
extern "C" void InfoScrn_DrawTimer(InfoScrn* scrn)
{
	f32 alpha = scrn->_1B8 * gHudOpacityScale;
	CTexObj tex;
	if (alpha > 0.0f) {
		u32 a      = 255.0f * alpha;
		u32 top    = a | 0xEFEBC500;
		u32 bottom = a | 0xEDA83600;
		f32 clock  = FramesToClock(scrn->mTimer);
		f32 step;
		f32 w;
		f32 h;
		f32 smallW;
		f32 smallH;
		f32 value = 10000.0f * (s32)(clock / 6000.0f) + (s32)clock % 6000;
		tex       = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[125 - StatusTextureOffset], 0);
		u16 tw    = GXGetTexObjWidth(&tex);
		w         = 1.8f * tw;
		u16 th    = GXGetTexObjHeight(&tex);
		h         = 1.8f * (th / 10);
		tw        = GXGetTexObjWidth(&tex);
		smallW    = 0.7f * (1.8f * tw);
		th        = GXGetTexObjHeight(&tex);
		smallH    = 0.7f * (1.8f * (th / 10));
		PSMTXScale(tex.mMtx[1], 1.0f, 0.1f, 1.0f);
		tex.mMtxDirty = TRUE;
		step          = 7.0f * w;
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
			PSMTXTrans(tex.mMtx[0], 0.0f, 0.1f * digit, 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			f32 ofs = 0.0f;
			if (i == 0) {
				ofs = -7.2f;
			} else if (i == 1) {
				ofs = -1.8f;
			} else if (i == 4 || i == 5) {
				ofs = -9.0f;
			}
			if (i == 5 && digit == 0) {
				continue;
			}
			if (i < 2) {
				DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, ofs + (step + (372.0f - i * (w - 3.6f)) - 14.4f), 75.6f,
				                   0.0f, smallW, smallH, 0.0f, 0.0f, 0.0f);
			} else {
				DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, ofs + (step + (372.0f - i * (w - 3.6f)) - 14.4f), 72.0f,
				                   0.0f, w, h, 0.0f, 0.0f, 0.0f);
			}
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		tex        = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[129 - StatusTextureOffset], 0);
		u16 colonW = GXGetTexObjWidth(&tex);
		u16 colonH = GXGetTexObjHeight(&tex);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, 472.8f, 72.9f, 0.0f, 0.6f * (1.8f * colonW), 0.6f * (1.8f * colonH),
		                   0.0f, 0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
}

/**
 * @note Address: 0x80066BFC
 * @note Size: 0xC
 */
extern "C" void InfoScrn_ShowTimer(InfoScrn* scrn)
{
	scrn->mTimerShown = TRUE;
}

/**
 * @note Address: 0x80066BF0
 * @note Size: 0xC
 */
extern "C" void InfoScrn_HideTimer(InfoScrn* scrn)
{
	scrn->mTimerShown = FALSE;
}

/**
 * @note Address: 0x80066B9C
 * @note Size: 0x54
 */
extern "C" void InfoScrn_SetTimerHundredths(InfoScrn* scrn, s32 hundredths)
{
	scrn->mTimer = (hundredths / 100.0f) * (f32)TIMER_RATE;
	if (scrn->mTimer >= (f32)TIMER_MAX) {
		scrn->mTimer = (f32)TIMER_MAX;
	}
}

/**
 * @note Address: 0x80066A84
 * @note Size: 0x118
 */
extern "C" s32 InfoScrn_GetTimerHundredths(InfoScrn* scrn)
{
	return FramesToClock(scrn->mTimer);
}

/**
 * @note Address: 0x8006697C
 * @note Size: 0x108
 */
// NONMATCHING: the three int to float conversions land in other registers
extern "C" f32 TimerFramesToHundredths(f32 frames)
{
	f32 seconds    = frames / (f32)TIMER_RATE;
	s32 hundredths = ((s32)frames % TIMER_RATE) * 100 / TIMER_RATE;
	if (frames >= (f32)TIMER_MAX) {
		hundredths = 99;
	}
	f32 minutes = seconds / 60.0f;
	f32 clock   = hundredths;
	clock += 6000.0f * (s32)minutes + ((s32)seconds % 60) * 100;
	return clock;
}

/**
 * @note Address: 0x80066714
 * @note Size: 0x268
 */
extern "C" void DrawHudScanlines(InfoScrn* scrn)
{
	f32 alpha = scrn->mScanlineAlpha;
	if (alpha > 0.0f) {
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		u32 a = 255.0f * alpha;

		static f32 sScanline = 0.0f;
		if (!gGame.mMenu.mActive) {
			sScanline = 1.0f + sScanline;
			if (sScanline >= 4.0f) {
				sScanline = 0.0f;
			}
		}

		SetGsBlendMode(gGame.mGs.mCurRenderMode, 3, 4, 5, 15);
		SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
		u32 color = a | 0x30303000;
		for (s32 y = 0; y < 448; y += 4.0f) {
			DrawGsLineSegment(&gGame.mGs, NULL, 0.0f, y + sScanline, 0.0f, color, 640.0f, y + sScanline, 0.0f, color);
		}
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
		for (int i = 9; i > 0; i--) {
			gHudScanlineHistory[i] = gHudScanlineHistory[i - 1];
		}
		gHudScanlineHistory[0] = 0.0f;
		SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
	}
}

/**
 * @note Address: 0x800665FC
 * @note Size: 0x34
 */
extern "C" BOOL InfoScrn_IsBatteryGaugeFull(InfoScrn* scrn)
{
	return scrn->mBatteryGauge == 10.0f * gGame.GetPlayer()->mParams.mMaxBattery;
}

/**
 * @note Address: 0x800665E0
 * @note Size: 0x1C
 */
extern "C" BOOL InfoScrn_IsBatteryGaugeEmpty(InfoScrn* scrn)
{
	return scrn->mBatteryGauge <= 0.0f;
}

/**
 * @note Address: 0x8006656C
 * @note Size: 0x74
 */
extern "C" void InitBatteryGauge(InfoScrn* scrn, f32 x)
{
	for (int i = 0; i < 5; i++) {
		scrn->_E0[i]  = 1.0f;
		scrn->_100[i] = 0;
		scrn->_104[i] = 0.0f;
		scrn->_114[i] = 0.0f;
	}
	scrn->_D0 = 10.0f * x;
	scrn->_CC = scrn->_D0;
}

/**
 * @note Address: 0x800664C4
 * @note Size: 0xA8
 */
extern "C" void InfoScrn_ApplyBatteryHudCommand(InfoScrn* scrn, u8 show, s32 mode)
{
	switch (mode) {
	case 0:
		if (show) {
			scrn->_BC = 1;
		} else {
			scrn->_BC = 2;
		}
		break;
	case 1:
		if (show) {
			scrn->_BC = 3;
		} else {
			scrn->_BC = 4;
		}
		break;
	case 2:
		if (show) {
			scrn->_C8 = -52.0f;
		} else {
			scrn->_C8 = 0.0f;
		}
		break;
	case 3:
		if (show) {
			scrn->_BC = 5;
		} else {
			scrn->_BC = 6;
		}
		break;
	}
}

/**
 * @note Address: 0x80066250
 * @note Size: 0x274
 */
// NONMATCHING: regalloc in the item loop
extern "C" void InfoScrn_ApplyClockHudCommand(InfoScrn* scrn, u8 show, s32 mode, s32 flag)
{
	switch (mode) {
	case 0:
		if (show) {
			scrn->_148 = 1;
		} else {
			scrn->_148 = 2;
		}
		break;
	case 1:
		if (show) {
			scrn->_148 = 3;
		} else {
			scrn->_148 = 4;
		}
		break;
	case 2:
		scrn->mItemMax = ItemMax();
		break;
	case 3:
		scrn->_148         = 5;
		scrn->mNextItemMax = ItemMax();
		break;
	case 4:
		if (show) {
			scrn->_148 = 7;
		} else {
			scrn->_148 = 8;
		}
		break;
	}
}

/**
 * @note Address: 0x800661E4
 * @note Size: 0x6C
 */
extern "C" void InfoScrn_ApplyObjectGaugeCommand(InfoScrn* scrn, XObj* obj, u8 show, s32 mode)
{
	scrn->_19C = NULL;
	switch (mode) {
	case 0:
		if (show) {
			scrn->_188 = 1;
			scrn->_19C = obj;
		} else {
			scrn->_188 = 2;
			scrn->_19C = NULL;
		}
		break;
	case 1:
		if (show) {
			scrn->_188 = 3;
			scrn->_19C = obj;
		} else {
			scrn->_188 = 4;
			scrn->_19C = NULL;
		}
		break;
	}
}

/**
 * @note Address: 0x80066190
 * @note Size: 0x54
 */
extern "C" void InfoScrn_ApplyScoreHudCommand(InfoScrn* scrn, u8 show, s32 mode)
{
	switch (mode) {
	case 0:
		if (show) {
			scrn->_1A0 = 1;
		} else {
			scrn->_1A0 = 2;
		}
		break;
	case 1:
		if (show) {
			scrn->_1A0 = 3;
		} else {
			scrn->_1A0 = 4;
		}
		break;
	}
}

/**
 * @note Address: 0x8006613C
 * @note Size: 0x54
 */
extern "C" void InfoScrn_ApplyTimerHudCommand(InfoScrn* scrn, u8 show, s32 mode)
{
	switch (mode) {
	case 0:
		if (show) {
			scrn->_1B4 = 1;
		} else {
			scrn->_1B4 = 2;
		}
		break;
	case 1:
		if (show) {
			scrn->_1B4 = 3;
		} else {
			scrn->_1B4 = 4;
		}
		break;
	}
}

/**
 * @note Address: 0x800660E8
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
 * @note Address: 0x800660E4
 * @note Size: 0x4
 */
extern "C" void InfoScrn_OnPlayerExperienceChanged(InfoScrn* scrn, s32 delta)
{
}

/**
 * @note Address: 0x800660E0
 * @note Size: 0x4
 */
extern "C" void InfoScrn_OnPlayerLevelChanged(InfoScrn* scrn, s32 level)
{
}

/**
 * @note Address: 0x80065E44
 * @note Size: 0x29C
 */
extern "C" void DrawHudRoundedRectangle(u32 color, f32 x, f32 y, f32 w, f32 h, f32 rx, f32 ry)
{
	Vec pos[65];
	f32 right;
	f32 bottom;
	f32 sx;
	f32 sy;
	sx     = 9.0f * rx;
	sy     = 9.0f * ry;
	right  = x + w;
	bottom = y + h;
	for (int i = 0; i < 65; i++) {
		f32 cx;
		f32 cy;
		f32 s;
		f32 angle = 0.09817477f * i - 1.5707964f;
		s         = sin(angle);
		f32 c     = -(f32)cos(angle);
		if ((i >= 0 && i < 16) || i == 64) {
			cx = x;
			cy = y;
		} else if ((i >= 16 && i < 32) || i == 64) {
			cx = right;
			cy = y;
		} else if ((i >= 32 && i < 48) || i == 64) {
			cx = right;
			cy = bottom;
		} else {
			cx = x;
			cy = bottom;
		}
		pos[i].x = s * sx + cx - w / 2;
		pos[i].y = c * sy + cy - h / 2;
		pos[i].z = 0.0f;
	}
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	DrawGsTrianglePrimitive(&gGame.mGs, (GXPrimitive)0xA0, gGame.mGs.mCamera->mViewMtx, pos, color, 65);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
}

/**
 * @note Address: 0x800652C0
 * @note Size: 0xB84
 */
// NONMATCHING: vertex temporaries and floating-point register allocation
extern "C" void DrawTexturedWindowFrame(u32 color, u32 textureColor, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 scaleX, f32 scaleY)
{
	f32 angle  = 4.712389f * (1.0f - alpha);
	f32 sx     = 4.0f * (f32)sin(angle);
	f32 sy     = 4.0f * (f32)sin(angle);
	f32 left   = x - width / 2.0f - 40.0f - sx;
	f32 right  = sx + (40.0f + (x + width / 2.0f));
	f32 top    = y - 8.0f - sy;
	f32 bottom = sy + (8.0f + (y + height));
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
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
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	DrawGsTrianglePrimitive(&gGame.mGs, (GXPrimitive)0xA0, gGame.mGs.mCamera->mViewMtx, pos, color, 10);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 1, 1);
	DrawGsCircle(&gGame.mGs, 16, color, color, (8.0f + (1.0f + left)), (8.0f + (1.0f + top)), 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, color, color, ((right - 1.0f) - 8.0f), (8.0f + (1.0f + top)), 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, color, color, (8.0f + (1.0f + left)), ((bottom - 1.0f) - 8.0f), 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, color, color, ((right - 1.0f) - 8.0f), ((bottom - 1.0f) - 8.0f), 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
	u32 shadow = (u32)(64.0f * alpha);
	for (int i = 0; i < 10; i++) {
		pos[i].x += 9.0f;
		pos[i].y += 9.0f;
	}
	DrawGsTrianglePrimitive(&gGame.mGs, (GXPrimitive)0xA0, gGame.mGs.mCamera->mViewMtx, pos, shadow, 10);
	DrawGsCircle(&gGame.mGs, 16, shadow, shadow, (8.0f + (1.0f + (9.0f + left))), (8.0f + (1.0f + (9.0f + top))), 0.0f, 16.0f, 16.0f, 0.0f,
	             0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, shadow, shadow, ((9.0f + right) - 1.0f - 8.0f), (8.0f + (1.0f + (9.0f + top))), 0.0f, 16.0f, 16.0f, 0.0f,
	             0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, shadow, shadow, (8.0f + (1.0f + (9.0f + left))), ((9.0f + bottom) - 1.0f - 8.0f), 0.0f, 16.0f, 16.0f, 0.0f,
	             0.0f, 0.0f);
	DrawGsCircle(&gGame.mGs, 16, shadow, shadow, ((9.0f + right) - 1.0f - 8.0f), ((9.0f + bottom) - 1.0f - 8.0f), 0.0f, 16.0f, 16.0f, 0.0f,
	             0.0f, 0.0f);
	CTexObj tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[114 - StatusTextureOffset], 0);
	f32 tw      = GXGetTexObjWidth(&tex);
	f32 th      = GXGetTexObjHeight(&tex);
	f32 w       = right - left - 128.0f;
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
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 1, 1);
	for (int i = 0; i < 8; i++) {
		PSMTXTrans(tex.mMtx[0], tiles[i][0], tiles[i][1], 0.0f);
		tex.mMtxDirty = TRUE;
		PSMTXScale(tex.mMtx[1], tiles[i][2], tiles[i][3], 1.0f);
		tex.mMtxDirty = TRUE;
		DrawGsTexturedQuad(&gGame.mGs, &tex, textureColor, textureColor, textureColor, textureColor, tiles[i][4], tiles[i][5], 1.0f,
		                   tiles[i][6], tiles[i][7], 0.0f, 0.0f, 0.0f);
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	DrawGsVertexQuad(&gGame.mGs, left - 8.0f, top - 8.0f, -1.0f, 0xFF000000, 9.0f + (8.0f + right), top - 8.0f, -1.0f, 0xFF000000,
	                 9.0f + (8.0f + right), 9.0f + (8.0f + bottom), -1.0f, 0xFF000000, left - 8.0f, 9.0f + (8.0f + bottom), -1.0f,
	                 0xFF000000);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
}

/**
 * @note Address: 0x8006389C
 * @note Size: 0xB08
 */
// NONMATCHING: float registers
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
		f32 cy = y0 - 7.2f;
		for (int i = 0; i < 65; i++) {
			f32 s;
			f32 angle = 0.09817477f * i - 1.5707964f;
			s         = sin(angle);
			f32 c     = -(f32)cos(angle);
			pos[i].x  = x0 + s * 14.4f;
			pos[i].y  = cy + c * 14.4f;
			pos[i].z  = 0.0f;
		}
		f32 a255 = 255.0f * alpha;
		u32 a    = a255;
		SetGsLineWidth(gGame.mGs.mCurRenderMode, 0x12, 5);
		DrawGsLineStrip(&gGame.mGs, pos, a, 65);
		SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);

		tex              = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[125 - StatusTextureOffset], 0);
		u16 tw           = GXGetTexObjWidth(&tex);
		f32 lineY        = cy + 14.4f;
		point->mValue[3] = len * (1.2f * tw - 3.6f) + 34.0f;
		pos[0].x         = x0;
		pos[0].y         = lineY;
		pos[1].x         = x0 + point->mValue[2];
		pos[1].y         = lineY;
		SetGsLineWidth(gGame.mGs.mCurRenderMode, 0x12, 5);
		DrawGsLineStrip(&gGame.mGs, pos, a, 2);
		SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);

		tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[155 - StatusTextureOffset], 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		u16 ew = GXGetTexObjWidth(&tex);
		u16 eh = GXGetTexObjHeight(&tex);
		u32 c  = a | 0xC8C8C800;
		DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, pos[1].x, pos[1].y, 0.0f, 1.2f * ew, 1.2f * eh, 0.0f, 0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);

		tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[point->mResource], 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		f32 iconW = 0.8f * GXGetTexObjWidth(&tex);
		f32 iconY = y0 - 9.0f;
		f32 iconH = 0.8f * GXGetTexObjHeight(&tex);
		if (point->mIndex == INFOPOINT_ID + 2) {
			u32 ic = (u32)(a255 * point->mIconAlphaScale) | 0xC8C8C800;
			DrawGsTexturedQuad(&gGame.mGs, &tex, ic, ic, ic, ic, x4, 4.0f + iconY, 0.0f, iconW, iconH, 0.0f, 0.0f, 0.0f);
		} else {
			u32 ic = (u32)(a255 * point->mIconAlphaScale) | 0xC8C8C800;
			DrawGsTexturedQuad(&gGame.mGs, &tex, ic, ic, ic, ic, x4, iconY, 0.0f, iconW, iconH, 0.0f, 0.0f, 0.0f);
		}

		tex    = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[125 - StatusTextureOffset], 0);
		f32 dx = x2 - 12.0f;
		f32 dy = y0 - 4.0f;
		f32 w  = 1.2f * GXGetTexObjWidth(&tex);
		f32 h  = 1.2f * (GXGetTexObjHeight(&tex) / 10);
		PSMTXScale(tex.mMtx[1], 1.0f, 0.1f, 1.0f);
		u8 da         = (s32)a255;
		f32 step      = 7.0f * w;
		f32 base      = len * (w - 3.6f) - 117.6f;
		u32 blueT     = da | 0xCECEFF00;
		u32 blueB     = da | 0xE037EE00;
		u32 goldT     = da | 0xEFEBC500;
		u32 goldB     = da | 0xEDA83600;
		u32 pinkB     = da | 0x5959FF00;
		tex.mMtxDirty = TRUE;
		for (int i = 0; i < 7; i++) {
			s32 v = (s32)(point->mValue[0] / Pow10(i));
			if (i != 0 && v == 0) {
				continue;
			}
			PSMTXTrans(tex.mMtx[0], 0.0f, 0.1f * (v % 10), 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			u32 top;
			u32 bottom;
			switch (point->mIndex) {
			case INFOPOINT_ID + 2:
				top    = blueT;
				bottom = blueB;
				break;
			case INFOPOINT_ID:
				top    = goldT;
				bottom = goldB;
				break;
			case INFOPOINT_ID + 1:
				top    = blueT;
				bottom = pinkB;
				break;
			}
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, base + (step + (dx - i * (w - 3.6f))), dy - 2.0f, 0.0f, w, h,
			                   0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
}

static inline CTexObj& HudTexture(s32 id)
{
	return gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[id - StatusTextureOffset], 0);
}

extern "C" CTexObj* CopyMenuTextureObject(CTexObj*, const CTexObj*);

static inline void SetBatterySlice(CTexObj& tex, f32 height, f32 top)
{
	PSMTXScale(tex.mMtx[1], 1, height, 1);
	tex.mMtxDirty = TRUE;
	PSMTXTrans(tex.mMtx[0], 0, top, 0);
	tex.mMtxDirty = TRUE;
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
}
static inline f32 BatteryDigitX(int place, f32 offset, f32 spacing)
{
	return 2.f + (2.f * spacing + (512.f + offset - place * spacing));
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
	CopyMenuTextureObject(&tex, &HudTexture(125));
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
extern "C" void DrawHudBatteryGauge(InfoScrn* h)
{
	f32 alpha = h->_C0 * gHudOpacityScale;
	if (gGame.mStatus.mFlags & 0x400)
		h->_144 = 1;
	else
		h->_144 = 0;
	f32 target = 10.f * gGame.GetPlayer()->mParams.mBattery;
	f32 y      = 22.f + (GetHudBottomEdge() - 50.f);
	if (target <= 0 && h->_CC > 0) {
		h->_124 = 1;
		h->_D4 = h->_D8 = 0;
	}
	if (target > 0)
		h->_124 = 0;
	if ((s32)gGame.GetPlayer()->mParams._1C[0] > 0) {
		h->_134 = 1;
		h->_13C = 1;
	} else {
		h->_134 = 0;
		h->_13C = 0;
	}
	ApproachHudValue(&h->_138, &h->_13C, .18f);
	const f32 fade = 1.f - h->_138;
	f32 scale      = h->_DC;
	CTexObj tex;
	CopyMenuTextureObject(&tex, &HudTexture(124));
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u16 tw = GXGetTexObjWidth(&tex), th = GXGetTexObjHeight(&tex);
	f32 opacity = 255.f * alpha;
	DrawBatteryQuad(tex, 0xc8c8c800 | (u32)opacity, 512.f + 13.f * scale, y, scale * (1.3f * tw), scale * (1.2f * th));
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	CopyMenuTextureObject(&tex, &HudTexture(125));
	PSMTXScale(tex.mMtx[1], 1, .1f, 1);
	tex.mMtxDirty = TRUE;
	h->_D0        = target;
	f32 change    = h->_CC - h->_D0;
	f32 step      = Abs((h->_D0 - h->_CC) / 20.f);
	if (step >= 53.999996f)
		step = 53.999996f;
	if (step <= 1)
		step = 1;
	ApproachHudValue(&h->_CC, &h->_D0, step);
	h->mBatteryGauge = h->_CC;
	u32 digitAlpha   = (u32)(opacity * fade);
	u32 red          = 0xff646400 | digitAlpha;
	u32 green        = 0x89ff2900 | digitAlpha;
	u32 gray         = 0x50505000 | digitAlpha;
	f32 gap          = 7.6f * scale;
	f32 smallScale = .9f * scale, smallX = 7.9f * scale, largeX = 2.f * scale;
	for (int i = 0; i < 4; ++i) {
		f32 digitScale = scale, offset = largeX;
		if (target <= 0 && h->_CC <= 0 && h->_124 == 1)
			break;
		if (i == 0) {
			digitScale = smallScale;
			offset     = smallX;
		}
		if (!h->_100[i])
			h->_104[i] = change;
		if (change != 0)
			h->_114[i] = change;
		int divisor = 1;
		for (int p = 0; p < i; ++p)
			divisor *= 10;
		int whole = (s32)h->_CC / divisor, digit = whole % 10;
		if (h->_F0[i] != digit) {
			h->_E0[i]  = 0;
			h->_100[i] = 1;
		}
		if (!(gGame.mStatus.mFlags & 0x200000000000ULL))
			h->_E0[i] += .08f;
		if (h->_E0[i] > 1) {
			h->_E0[i]  = 1;
			h->_100[i] = 0;
		}
		h->_F0[i] = digit;
		f32 amount;
		if (!i) {
			f32 value = h->_CC / divisor;
			amount    = 1.f - (value - (s32)value);
		} else
			amount = h->_E0[i];
		u32 color;

		if (h->_104[i] > 0) {
			color = BatteryDigitColor(whole, red, green, gray);
			DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * amount, .1f * digit + (1.f - amount) / 10.f, amount,
			                     (1.f - amount), true);
			DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * (1.f - amount), .1f * ((whole + 1) % 10), (1.f - amount),
			                     amount);
		} else if (h->_104[i] < 0) {
			color = BatteryDigitColor(whole, red, green, gray);
			DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * amount, .1f * digit, amount, (1.f - amount));
			DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * (1.f - amount), .1f * ((whole - 1) % 10) + amount / 10.f,
			                     (1.f - amount), amount, true);
		} else {
			color = BatteryDigitColor(whole, red, green, gray);
			if (h->_114[i] > 0) {
				DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * amount, .1f * digit + (1.f - amount) / 10.f, amount,
				                     (1.f - amount), true);
				DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * (1.f - amount), .1f * ((whole + 1) % 10),
				                     (1.f - amount), amount);
			} else if (h->_114[i] < 0) {
				DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * amount, .1f * digit, amount, (1.f - amount));
				DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f * (1.f - amount),
				                     .1f * ((whole - 1) % 10) + amount / 10.f, (1.f - amount), amount, true);
			} else {
				DrawBatteryDigitPart(tex, color, i, offset, y, digitScale, gap, .1f, .1f * digit, 1, 0);
			}
		}
	}
	if (target <= 0 && h->_CC <= 0 && h->_124 == 1) {
		++h->_D4;
		if (h->_D4 > 1) {
			++h->_D8;
			if (h->_D8 >= 9)
				h->_D8 = 9;
			h->_D4 = 0;
		}
		for (int i = 0; i < 4; ++i) {
			f32 digitScale = scale, offset = largeX;
			if (i == 0) {
				digitScale = smallScale;
				offset     = smallX;
			}

			int frame = h->_D8 % 10 + 1;
			CopyMenuTextureObject(&tex, &HudTexture(125));
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
			CopyMenuTextureObject(&tex, &HudTexture(126));
			f32 w = digitScale * GXGetTexObjWidth(&tex), ht = digitScale * GXGetTexObjHeight(&tex);
			SetBatterySlice(tex, fill, rest);
			u32 emptyColor = 0x5176ff00 | digitAlpha;
			DrawBatteryOverlay(tex, emptyColor, i, offset, y, w, ht, gap, fill, rest);
		}
	}
	if (h->_134 == 1) {
		for (int i = 0; i < 4; ++i) {

			f32 digitScale = scale, offset = largeX, verticalOffset = -2.f;
			if (i == 0) {
				digitScale     = smallScale;
				offset         = smallX;
				verticalOffset = 0.f;
			}

			CopyMenuTextureObject(&tex, &HudTexture(125));
			SetBatterySlice(tex, 0, 0);
			f32 digitW     = digitScale * GXGetTexObjWidth(&tex);
			u16 texH       = GXGetTexObjHeight(&tex);
			f32 digitH     = digitScale * texH / 10.f;
			u32 digitColor = (u32)(h->_138 * opacity) | 0x50505000;
			DrawBatteryQuad(tex, digitColor, BatteryDigitX(i, offset, digitW + gap), (y + verticalOffset) + digitH * .5f, digitW,
			                digitH * 0.f);

			if (i == 0) {
				digitScale     = .8f * scale;
				offset         = 11.2f * scale;
				verticalOffset = 0.f;
			} else
				offset = 1.7f * scale;
			CopyMenuTextureObject(&tex, &HudTexture(167));
			f32 w = digitScale * GXGetTexObjWidth(&tex), ht = digitScale * GXGetTexObjHeight(&tex);
			SetBatterySlice(tex, 1, 0);
			u32 overlayColor = (u32)(h->_138 * opacity) | 0x89ff2900;
			DrawBatteryOverlay(tex, overlayColor, i, offset, y + verticalOffset, w, ht, gap, 1.f, 0.f);
		}
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	for (int i = 0; i < 4; ++i) {
		f32 size = scale, offset = largeX;
		if (i == 0) {
			size   = smallScale;
			offset = 9.9f * scale;
		}
		CopyMenuTextureObject(&tex, &HudTexture(125));
		f32 w = 1.4f * (size * GXGetTexObjWidth(&tex)), ht = size * GXGetTexObjHeight(&tex) / 10.f;
		f32 x = BatteryDigitX(i, offset, w + .4f * scale) - w * .5f, top = y - ht * .5f;
		Vec points[]
		    = { (Vec) { x, top, 0 }, (Vec) { x + w, top, 0 }, (Vec) { x + w, top + ht, 0 }, (Vec) { x, top + ht, 0 }, (Vec) { x, top, 0 } };
		DrawGsTrianglePrimitive(&gGame.mGs, (GXPrimitive)0xa0, gGame.mGs.mCamera->mViewMtx, points,
		                        0x0a0a0a00 | (u32)((120.f * alpha) * h->_140), 5);
	}
}

static inline void DrawClockDisc(f32 rx, f32 ry, u32 color)
{
	Vec points[65];
	for (s32 i = 0; i < 65; ++i) {
		f32 angle = 0.09817477f * i - 1.5707964f;
		f32 s = sin(angle), c = -(f32)cos(angle);
		points[i].x = 80.0f + s * rx;
		points[i].y = 80.0f + c * ry;
		points[i].z = 0;
	}
	DrawGsTrianglePrimitive(&gGame.mGs, (GXPrimitive)0xA0, gGame.mGs.mCamera->mViewMtx, points, color, 65);
}

static inline void DrawClockQuad(CTexObj& tex, u32 color, f32 x, f32 y, f32 z, f32 w, f32 h)
{
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, z, w, h, 0, 0, 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

extern "C" void DrawHudClock(InfoScrn* scrn)
{
	f32 alpha = scrn->_14C * gHudOpacityScale;
	if (!(alpha > 0.0f))
		return;
	CTexObj tex;
	f32 scale = 1.1f * scrn->_16C;
	u32 color;
	if (gGame.mTimeState.mMode == 0) {
		tex   = HudTexture(156);
		color = 0x326c0200 | (u32)(255 * alpha);
	} else {
		tex   = HudTexture(157);
		color = 0x1e3fb000 | (u32)(255 * alpha);
	}
	u16 tw = GXGetTexObjWidth(&tex), th = GXGetTexObjHeight(&tex);
	DrawClockDisc(1.1f * (scale * tw * 0.5f), 1.1f * (scale * th * 0.5f), color);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	tex = HudTexture(158);
	tw  = GXGetTexObjWidth(&tex);
	th  = GXGetTexObjHeight(&tex);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u32 opacity = (u32)(255 * alpha);
	DrawClockQuad(tex, 0xc8c8c800 | opacity, 80, 80, 1, scale * tw, scale * th);
	if (gGame.mTimeState.mMode == 0)
		tex = HudTexture(156);
	else
		tex = HudTexture(157);
	f32 width = scale * GXGetTexObjWidth(&tex), height = scale * GXGetTexObjHeight(&tex);
	DrawClockDisc(0.9f * (width * 0.5f), 0.9f * (height * 0.5f), (u32)(120 * alpha));
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	f32 time = gGame.mTimeState.mTime;
	f32 fill = time;
	if (time >= 0.89f && time < 0.999f)
		fill = 0.89f;
	if (gGame.mStatus.mFlags & 0x400)
		color = 0xa0a0a000 | opacity;
	else {
		f32 warning = 1.0f - (TIMER_RATE * 5) / ((TIMER_RATE * 600) / gGame.mTimeState.mDuration);
		if (time < warning) {
			scrn->_158 = 0;
			scrn->_150 = 5;
		}
		if (time >= warning) {
			f32 wave     = sin(0.017453292f * ((s32)((u32)scrn->_154 * 12) % 360));
			bool running = gGame.IsRunning() && !gGame.mEvt.mData.mScript && !gGame.mMenu.mActive && !(gGame.mStatus.mFlags & 0xc00)
			            && gGame.mTimeState.mDuration > 0 && !gGame.mTimeState.mRunning;
			if (running || time >= 1) {
				if (wave > 0) {
					if (scrn->_150 >= 0 && !scrn->_158) {
						if (scrn->_150 == 0 || time >= 0.999f) {
							PlayGameSound(&gGame.mSound, 0xb6, 255);
							scrn->_150 = 0;
						} else
							PlayGameSound(&gGame.mSound, 0xb5, 255);
						--scrn->_150;
						scrn->_158 = 1;
					}
				} else
					scrn->_158 = 0;
			}
			color = (wave > 0 || scrn->_150 < 0 ? 0xffffff00 : 0xa0a0a000) | opacity;
		} else
			color = 0xffffff00 | opacity;
	}
	DrawClockQuad(tex, color, 80, 80, 1, width, height);
	PSMTXScale(tex.mMtx[1], 1, fill, 1);
	tex.mMtxDirty = TRUE;
	PSMTXTrans(tex.mMtx[0], 0, 0, 0);
	tex.mMtxDirty = TRUE;
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawClockQuad(tex, (u32)(160 * alpha), 80, 80 - (1 - fill) * height * 0.5f, 1, width, height * fill);
	tex    = HudTexture(159);
	width  = 0.9f * (scale * GXGetTexObjWidth(&tex));
	height = 0.9f * (scale * GXGetTexObjHeight(&tex));
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	color = 0xc8c8c800 | (u32)(140 * alpha);
	DrawClockQuad(tex, color, 80 - 4 * scale, 80 - 14 * scale, 1, width, height);
	tex = HudTexture(159);
	tw  = GXGetTexObjWidth(&tex);
	th  = GXGetTexObjHeight(&tex);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawClockQuad(tex, color, 80 - 16 * scale, 80 - 2 * scale, 1, 0.7f * (scale * tw), 0.7f * (scale * th));
	f32 badgeAlpha = scrn->mItemMaxAlpha * gHudOpacityScale;
	switch ((s32)(f32)scrn->mItemMax) {
	case 10:
		tex = HudTexture(232);
		break;
	case 15:
		tex = HudTexture(233);
		break;
	default:
		tex = HudTexture(231);
		break;
	}
	tw = GXGetTexObjWidth(&tex);
	th = GXGetTexObjHeight(&tex);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawClockQuad(tex, 0xffffff00 | (u32)(255 * badgeAlpha), 80 - 22 * scale, 80 - 22 * scale, 0, 1.1f * (scale * tw), 1.1f * (scale * th));
}

extern "C" bool fn_80051C60();

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

// the chibi-pc menu is open, the counters stay hidden under it. japan has more menus and
// no empty slots
static inline BOOL IsPCOpen()
{
#if defined(VERSION_JP)
	return (gGame.mMenu.mMenus[8])->IsOpen();
#else
	CMenu* menu = gGame.mMenu.mMenus[3];
	return menu ? menu->IsOpen() : FALSE;
#endif
}

// the right edge the corner counters line up on
static inline f32 GetRight()
{
	return (gGame.mWipeTile1.mHeight / 2 + gGame.mWipeTile1.mY >= 418.0f) ? 418.0f : gGame.mWipeTile1.mHeight / 2 + gGame.mWipeTile1.mY;
}

/**
 * @note Address: 0x800643A4
 * @note Size: 0xD08
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
	if (IsPCOpen()) {
		point->mValue[0]   = point->mValue[1];
		point->mValue[2]   = point->mValue[3];
		point->mPos[1].x   = -220.0f;
		point->mPos[0].x   = -220.0f;
		point->mPos[3].x   = -220.0f;
		point->mPos[2].x   = -220.0f;
		point->mPos[5].x   = -220.0f;
		point->mPos[4].x   = -220.0f;
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

	bool empty = false;
	if (0.0f == gGame.GetPlayer()->_A30) {
		empty = true;
	}
#if defined(VERSION_JP)
	// japan looks the bedroom up here and drops the result
	FindStageIndexByName("stage19");
#endif
	if (point->mSuppressIdleDisplay) {
		empty = false;
	}
	switch (point->mIndex) {
	case INFOPOINT_ID + 2:
		point->mPos[0].y = GetRight() - 64.0f - 40.0f;
		point->mPos[1].y = GetRight() - 64.0f - 40.0f;
		break;
	case INFOPOINT_ID:
		point->mPos[0].y = GetRight() - 64.0f;
		point->mPos[1].y = GetRight() - 64.0f;
		break;
	case INFOPOINT_ID + 1:
		point->mPos[0].y = 40.0f + (GetRight() - 64.0f);
		point->mPos[1].y = 40.0f + (GetRight() - 64.0f);
		break;
	}

	switch (point->mSlideState) {
	case 0:
		if (IsPCOpen()) {
			break;
		}
		if (empty) {
			point->mPos[1].x   = 60.0f;
			point->mSlideDelay = 0;
			point->mSlideState++;
		} else if (changed) {
			point->mPos[1].x   = 60.0f;
			point->mSlideDelay = 0;
			point->mSlideState = 10;
		}
		break;
	case 1:
		point->mSlideDelay++;
		if (point->mSlideDelay > 6) {
			point->mPos[3].x   = 60.0f;
			point->mSlideDelay = 0;
			point->mSlideState++;
		}
		break;
	case 2:
		point->mSlideDelay++;
		if (point->mSlideDelay > 6) {
			point->mPos[5].x = 60.0f;
			point->mSlideState++;
		}
		break;
	case 3:
		if (!empty) {
			point->mPos[1].x   = -220.0f;
			point->mSlideDelay = 0;
			point->mSlideState++;
		}
		break;
	case 4:
		if (changed) {
			point->mPos[1].x   = 60.0f;
			point->mSlideDelay = 0;
			point->mSlideState = 10;
		} else {
			point->mSlideDelay++;
			if (point->mSlideDelay > 6) {
				point->mPos[5].x   = -220.0f;
				point->mSlideDelay = 0;
				point->mSlideState++;
			}
		}
		break;
	case 5:
		if (changed) {
			point->mPos[1].x   = 60.0f;
			point->mSlideDelay = 0;
			point->mSlideState = 10;
		} else {
			point->mSlideDelay++;
			if (point->mSlideDelay > 6) {
				point->mPos[3].x   = -220.0f;
				point->mSlideState = 0;
			}
		}
		break;
	case 10:
		point->mSlideDelay++;
		if (point->mSlideDelay > 3) {
			point->mPos[3].x   = 60.0f;
			point->mSlideDelay = 0;
			point->mSlideState++;
		}
		break;
	case 11:
		point->mSlideDelay++;
		if (point->mSlideDelay > 3) {
			point->mPos[5].x   = 60.0f;
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
				point->mPos[1].x   = -220.0f;
				point->mSlideDelay = 0;
				point->mSlideState++;
			}
		}
		break;
	case 13:
		if (changed) {
			point->mPos[1].x   = 60.0f;
			point->mSlideDelay = 0;
			point->mSlideState = 10;
		} else {
			point->mSlideDelay++;
			if (point->mSlideDelay > 6) {
				point->mPos[5].x   = -220.0f;
				point->mSlideDelay = 0;
				point->mSlideState++;
			}
		}
		break;
	case 14:
		if (changed) {
			point->mPos[1].x   = 60.0f;
			point->mSlideDelay = 0;
			point->mSlideState = 10;
		} else {
			point->mSlideDelay++;
			if (point->mSlideDelay > 6) {
				point->mPos[3].x   = -220.0f;
				point->mSlideState = 0;
			}
		}
		break;
	}

	if (point->mVisible) {
		point->mPos[1].x   = 60.0f;
		point->mPos[3].x   = 60.0f;
		point->mPos[5].x   = 60.0f;
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
		if (fn_80051C60()) {
			point->mState          = 0;
			point->mPulseFrames    = 0;
			point->mIconAlphaScale = 1.0f;
			break;
		}
		switch (point->mState) {
		case 0:
			if (point->mValue[1] >= 300.0f) {
				point->mState++;
				point->mPulseFrames = 0;
			}
			break;
		case 1:
			point->mPulseFrames += frames;
			point->mIconAlphaScale = sin(0.017453292f * ((point->mPulseFrames * 6) % 360));
			if (point->mIconAlphaScale > 0.0f) {
				PlayGameSound(&gGame.mSound, 0x183, 0xFF);
				point->mState++;
			}
			break;
		case 2:
			point->mPulseFrames += frames;
			point->mIconAlphaScale = sin(0.017453292f * ((point->mPulseFrames * 6) % 360));
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

/**
 * @note Address: 0x800650AC
 * @note Size: 0x214
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
		point->mPos[i].x = -220.0f;
	}
	for (int i = 0; i < 4; i++) {
		point->mValue[i] = 0.0f;
	}
	point->mVisible             = 0;
	point->mSuppressIdleDisplay = 0;
	switch (point->mIndex) {
	case INFOPOINT_ID + 2:
		point->mResource = point->mIndex;
		point->mPos[0].y = GetRight() - 64.0f - 40.0f;
		point->mPos[1].y = GetRight() - 64.0f - 40.0f;
		break;
	case INFOPOINT_ID:
		point->mResource = point->mIndex;
		point->mPos[0].y = GetRight() - 64.0f;
		point->mPos[1].y = GetRight() - 64.0f;
		break;
	case INFOPOINT_ID + 1:
		point->mResource = point->mIndex;
		point->mPos[0].y = 40.0f + (GetRight() - 64.0f);
		point->mPos[1].y = 40.0f + (GetRight() - 64.0f);
		break;
	}
	point->mState          = 0;
	point->mIconAlphaScale = 0.0f;
	point->_80             = 0;
}

/**
 * @note Address: 0x80066630
 * @note Size: 0xE4
 */
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

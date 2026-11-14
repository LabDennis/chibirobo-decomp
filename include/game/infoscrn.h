#ifndef _GAME_INFOSCRN_H
#define _GAME_INFOSCRN_H

#include "game/cgs.h"
#include "game/cscrn.h"
#include "game/infopoint.h"

class XObj;

// the hud over the game, CGame's mInfoScrn. class name from RTTI, its vtable only has the dtor
class InfoScrn {
public:
	CScrnFade mFade;      // _00
	CScrnWipe mWipe;      // _34
	s32 _98;              // _98, written 0 in InitInfoScreen and ResetGameHudState, never read
	s32 mHudState;        // _9C
	s32 mAttachmentState; // _A0
	s32 mMenuHudState;    // _A4
	f32 mHudAlpha;        // _A8
	f32 mAttachmentAlpha; // _AC
	f32 mMenuHudAlpha;    // _B0
	s32 mFrameCount;      // _B4
	s32 _B8; // _B8, set -1 when the attachment hud starts to show; the shown state fades out once it isn't -1, but only init/reset write it
	         // (0) so that test never fires
	s32 mBatteryState;            // _BC
	f32 mBatteryAlpha;            // _C0
	f32 mBatteryOffsetY;          // _C4
	f32 mTargetBatteryOffsetY;    // _C8
	f32 mBatteryValue;            // _CC
	f32 mBatteryTarget;           // _D0
	s32 mBatteryEmptyTimer;       // _D4
	s32 mBatteryEmptyStep;        // _D8
	f32 mBatteryScale;            // _DC
	f32 mDigitRoll[4];            // _E0
	s32 mDigitValue[4];           // _F0
	u8 mDigitRolling[4];          // _100
	f32 mDigitDelta[4];           // _104
	f32 mDigitLastDelta[4];       // _114
	u8 mBatteryEmpty;             // _124
	u8 _125[0x128 - 0x125];       // _125
	Vec _128;                     // _128, zeroed in InitInfoScreen and ResetGameHudState, never read
	u8 mBatteryDim;               // _134
	u8 _135[0x138 - 0x135];       // _135
	f32 mBatteryDimAlpha;         // _138
	f32 mTargetBatteryDimAlpha;   // _13C
	f32 mDimAlpha;                // _140
	f32 mTargetDimAlpha;          // _144
	s32 mClockState;              // _148
	f32 mClockAlpha;              // _14C
	s32 mTimeWarningBeeps;        // _150
	s32 mTimeWarningTick;         // _154
	u8 mTimeWarningBeeped;        // _158
	u8 _159[0x160 - 0x159];       // _159
	f32 mItemMaxAlpha;            // _160
	s32 mItemMax;                 // _164, 5, 10 or 15 by the upgrades in chibi robo's items
	s32 mNextItemMax;             // _168
	f32 mClockScale;              // _16C
	Vec _170;                     // _170, zeroed in InitInfoScreen and ResetGameHudState, never read
	s32 mIconFrameTimer;          // _17C
	s32 mIconFrame;               // _180
	s32 _184;                     // _184, written 0 in InitInfoScreen and ResetGameHudState, never read
	s32 mObjectGaugeState;        // _188
	f32 mObjectGaugeAlpha;        // _18C
	Vec mObjectGaugePos;          // _190
	XObj* mGaugeTarget;           // _19C, what the gauge hangs over
	s32 mPlayerIconState;         // _1A0
	f32 mPlayerIconAlpha;         // _1A4
	Vec _1A8;                     // _1A8, zeroed in InitInfoScreen and ResetGameHudState, never read
	s32 mTimerState;              // _1B4
	f32 mTimerAlpha;              // _1B8
	f32 mTimer;                   // _1BC, frames
	u8 mTimerShown;               // _1C0
	u8 _1C1[0x1C4 - 0x1C1];       // _1C1
	s32 mScanlineVisibilityState; // _1C4
	f32 mScanlineAlpha;           // _1C8
	f32 _1CC;                     // _1CC, written 0.0f in InitInfoScreen and ResetGameHudState, never read
	s32 mSelectedSaveSlot;        // _1D0
	f32 mBatteryGauge;            // _1D4, the battery gauge, 10 a watt
	// _1D8 vtable

	virtual ~InfoScrn() { } // _08 (weak)
};
STATIC_ASSERT(sizeof(InfoScrn) == 0x1DC);

extern "C" {
f32 TimerFramesToHundredths(f32 frames);
void InfoScrn_ShowTimer(InfoScrn* scrn);
void InfoScrn_HideTimer(InfoScrn* scrn);
void InfoScrn_SetTimerHundredths(InfoScrn* scrn, s32 hundredths);
s32 InfoScrn_GetTimerHundredths(InfoScrn* scrn);
void InfoScrn_ApplyTimerHudCommand(InfoScrn* scrn, u8 show, s32 mode);
void InfoScrn_OnPlayerLevelChanged(InfoScrn* scrn, s32 level);
void InfoScrn_OnPlayerExperienceChanged(InfoScrn* scrn, s32 delta);
void InfoScrn_ApplyScanlineHudCommand(InfoScrn* scrn, u8 show, s32 mode);
void InfoScrn_ApplyScoreHudCommand(InfoScrn* scrn, u8 show, s32 mode);
void InfoScrn_ApplyObjectGaugeCommand(InfoScrn* scrn, XObj* obj, u8 show, s32 mode);
void InfoScrn_ApplyClockHudCommand(InfoScrn* scrn, u8 show, s32 mode, s32 flag);
BOOL InfoScrn_IsBatteryGaugeEmpty(InfoScrn* scrn);
}

#endif

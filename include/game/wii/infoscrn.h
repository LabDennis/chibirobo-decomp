#ifndef _GAME_WII_INFOSCRN_H
#define _GAME_WII_INFOSCRN_H

#include "game/wii/cgs.h"
#include "game/cscrn.h"
#include "game/infopoint.h"

class WiiTitleObject;

// the hud over the game on the Wii, CGame's mInfoScrn at _AF58 (0x1F0 bytes, the GameCube's
// is 0x1DC). No vtable: retail has no __vt__9InfoScrn. Versus the GameCube layout it drops
// _98, inserts an s32 at _9C (so every member from mHudState on sits 4 bytes later), another
// s32 at _190 before mObjectGaugeAlpha (+8 from there on) and a trailing Vec
// at _1E4. Member offsets verified against InitInfoScreen (0x800870E8), UpdateHudIndicators
// (0x8008817C) and the Apply*/Draw* functions' accesses
class InfoScrn {
public:
	CScrnFade mFade;              // _00
	CScrnWipe mWipe;              // _34
	s32 _9C;                      // _9C, a Wii-only word InitInfoScreen and ResetGameHudState zero
	s32 mHudState;                // _A0
	s32 mAttachmentState;         // _A4
	s32 mMenuHudState;            // _A8
	f32 mHudAlpha;                // _AC
	f32 mAttachmentAlpha;         // _B0
	f32 mMenuHudAlpha;            // _B4
	s32 mFrameCount;              // _B8
	s32 _BC;                      // _BC, the GameCube's _B8
	s32 mBatteryState;            // _C0
	f32 mBatteryAlpha;            // _C4
	f32 mBatteryOffsetY;          // _C8
	f32 mTargetBatteryOffsetY;    // _CC
	f32 mBatteryValue;            // _D0
	f32 mBatteryTarget;           // _D4
	s32 mBatteryEmptyTimer;       // _D8
	s32 mBatteryEmptyStep;        // _DC
	f32 mBatteryScale;            // _E0
	f32 mDigitRoll[4];            // _E4
	s32 mDigitValue[4];           // _F4
	u8 mDigitRolling[4];          // _104
	f32 mDigitDelta[4];           // _108
	f32 mDigitLastDelta[4];       // _118
	u8 mBatteryEmpty;             // _128
	u8 _129[0x12C - 0x129];       // _129
	Vec _12C;                     // _12C, zeroed in InitInfoScreen and ResetGameHudState
	u8 mBatteryDim;               // _138
	u8 _139[0x13C - 0x139];       // _139
	f32 mBatteryDimAlpha;         // _13C
	f32 mTargetBatteryDimAlpha;   // _140
	f32 mDimAlpha;                // _144
	f32 mTargetDimAlpha;          // _148
	s32 mClockState;              // _14C
	f32 mClockAlpha;              // _150
	s32 mTimeWarningBeeps;        // _154
	s32 mTimeWarningTick;         // _158
	u8 mTimeWarningBeeped;        // _15C
	u8 _15D[0x164 - 0x15D];       // _15D
	f32 mItemMaxAlpha;            // _164
	s32 mItemMax;                 // _168, 5, 10 or 15 by the upgrades in chibi robo's items
	s32 mNextItemMax;             // _16C
	f32 mClockScale;              // _170
	Vec _174;                     // _174, zeroed in InitInfoScreen and ResetGameHudState
	s32 mIconFrameTimer;          // _180
	s32 mIconFrame;               // _184
	s32 _188;                     // _188, the GameCube's _184
	s32 mObjectGaugeState;        // _18C
	s32 mObjectGaugeParam;        // _190, set from ApplyObjectGaugeCommand's fifth argument
	f32 mObjectGaugeAlpha;        // _194
	Vec mObjectGaugePos;          // _198
	WiiTitleObject* mGaugeTarget; // _1A4, what the gauge hangs over
	s32 mPlayerIconState;         // _1A8
	f32 mPlayerIconAlpha;         // _1AC
	Vec _1B0;                     // _1B0, zeroed in InitInfoScreen and ResetGameHudState
	s32 mTimerState;              // _1BC
	f32 mTimerAlpha;              // _1C0
	f32 mTimer;                   // _1C4, frames
	u8 mTimerShown;               // _1C8
	u8 _1C9[0x1CC - 0x1C9];       // _1C9
	s32 mScanlineVisibilityState; // _1CC
	f32 mScanlineAlpha;           // _1D0
	f32 _1D4;                     // _1D4, the GameCube's _1CC
	s32 mSelectedSaveSlot;        // _1D8
	f32 mBatteryGauge;            // _1DC, the battery gauge, 10 a watt
	s32 _1E0;                     // _1E0, a Wii-only word no surviving function touches
	Vec _1E4;                     // _1E4, a Wii-only vector, zeroed in InitInfoScreen
};
STATIC_ASSERT(sizeof(InfoScrn) == 0x1F0);

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
void InfoScrn_ApplyObjectGaugeCommand(InfoScrn* scrn, WiiTitleObject* obj, u8 show, s32 mode, s32 param);
void InfoScrn_ApplyClockHudCommand(InfoScrn* scrn, u8 show, s32 mode, s32 flag);
BOOL InfoScrn_IsBatteryGaugeEmpty(InfoScrn* scrn);
}

#endif

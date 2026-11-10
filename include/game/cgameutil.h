#ifndef _GAME_CGAMEUTIL_H
#define _GAME_CGAMEUTIL_H

#include "game/cmdline.h"
#include "types.h"
#include "game/cgame.h"
#include "game/stagedata.h"

// the inlines the cgame units share

extern "C" {
void ApplyGsRenderMode(CGsRenderMode* mode, BOOL force);
void ApplyGsTevStage(CGsTevStage* stage, BOOL force);
void ApplyGsChannel(CGsChannel* chan, BOOL force);
void HSD_PadRumbleOffN(u8 no);
void HSD_PadRumblePauseAll(void);
s32 DVDGetDriveStatus(void);
BOOL DVDCheckDisk(void);
void HSD_PadRumbleActiveAll(void);
}

// sends all of the current render, tev and channel state
static inline void SetupAll(BOOL force)
{
	CGs* gs = &gGame.mGs;
	if (gs->mCurRenderMode != NULL) {
		ApplyGsRenderMode(gs->mCurRenderMode, force);
	}
	if (gs->mCurTevStage != NULL) {
		ApplyGsTevStage(gs->mCurTevStage, force);
	}
	if (gs->mCurChannel != NULL) {
		ApplyGsChannel(gs->mCurChannel, force);
	}
}

// forgets all of the state after drawing behind its back
static inline void InvalidateAll()
{
	CGs* gs = &gGame.mGs;
	HSD_StateInvalidate(HSD_STATE_ALL);
	if (gs->mCurRenderMode != NULL) {
		gs->mCurRenderMode->Invalidate();
	}
	if (gs->mCurTevStage != NULL) {
		gs->mCurTevStage->Invalidate();
	}
	if (gs->mCurChannel != NULL) {
		if (!gs->mCurChannel->mDirty) {
			gs->mCurChannel->mDirty = TRUE;
		}
	}
}

// a mirror is off while its owner is gone or hidden
static inline bool IsMirrorOff(FbMirrorEx* mirror)
{
	bool off = false;
	if (mirror->mOwner == NULL || (mirror->mOwner->mStatus.mFlags & 0x1000000000000)) {
		off = true;
	}
	return off;
}

// the current stage's settings
struct UnkStageInfo {
	u8 _00[0x6];        // _00
	s16 mCameraMode;    // _06
	u8 mMirrorFade;     // _08
	u8 _09[0x10 - 0x9]; // _09
	u8 mFlags;          // _10, bit 0 keeps the shadows at full strength
};
STATIC_ASSERT(sizeof(UnkStageInfo) == 0x12);

// the loaded stage's EnvInfo starts with a pointer to them, see game/stagedata.h
static inline UnkStageInfo* GetStageInfo()
{
	return *(UnkStageInfo**)GetStageEnvironment();
}

// CBase::Update, keeps the rumble off while the disc can't be read and watches for the reset
// button or B X start held for a second. CGame::Update has it inlined
static inline void BaseUpdate(CBase* base, u32 frames)
{
	base->mPrevDvdStatus = base->mDvdStatus;
	base->mDvdStatus     = DVDGetDriveStatus();
	base->mDiskOk        = DVDCheckDisk();
	switch (base->mDvdStatus) {
	case -1:
	case 4:
	case 5:
	case 6:
	case 11:
		if (base->mDvdStatus != base->mPrevDvdStatus) {
			CPadEx* pad     = &base->mPad;
			base->mDvdError = TRUE;
			for (s32 i = 0; i < 4; i++) {
				if (pad->mPad[i].mConnected) {
					HSD_PadRumbleOffN(pad->mPad[i].mPort);
				}
			}
			HSD_PadRumblePauseAll();
		}
		break;
	case 0:
		if (base->mPrevDvdStatus != 0 && base->mDvdError) {
			base->mDvdError = FALSE;
			HSD_PadRumbleActiveAll();
		}
		break;
	}

	if (!base->mReset) {
		if (HSD_PadGetResetSwitch()) {
			base->mResetHold = 0;
			base->mReset     = TRUE;
		} else if ((base->mPad.mPad[0].mButton & 0xFFFF) == 0x1600) {
#if defined(VERSION_PAL)
			if ((base->mResetHold += base->mRetraceStep) > gVideoFramesPerSecond) {
#else
			if ((base->mResetHold += base->mRetraceStep) > 60) {
#endif
				base->mResetHold = 0;
				base->mReset     = TRUE;
			}
		} else if (!base->mReset) {
			base->mResetHold = 0;
		}
	}
}

#endif

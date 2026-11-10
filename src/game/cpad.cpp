extern "C" {
#include <sysdolphin/controller.h>
}
#include "game/cpad.h"
extern "C" {
#include <sysdolphin/video.h>
}

extern "C" {
extern const f32 sPadAdcAngle;
extern const f32 sPadAnalogNeutral;
}

static CPadEx* sPadEx;

/**
 * @note Address: 0x8019E3C8
 * @note Size: 0x28
 */
CPad::CPad(u8 port)
{
	mPad = &sPadEx->mPad[port];
}

/**
 * @note Address: 0x8019E3F0
 * @note Size: 0x20
 */
static void PadRetraceCallback(u32 retraceCount)
{
	HSD_PadRenewRawStatus();
}

/**
 * @note Address: 0x8019E410
 * @note Size: 0x80
 */
void CPadEx::Update(s32 frames)
{
	HSD_PadRenewMasterStatus();
	HSD_PadRenewCopyStatus();
	for (u8 i = 0; i < 4; i++) {
		mPad[i].Update(frames);
	}
}

/**
 * @note Address: 0x8019E490
 * @note Size: 0xE0
 */
void CPadEx::Init()
{
	HSD_PadInit(1, &mQueue, 4, mRumble);
	HSD_PadLibData.adc_type            = 0;
	HSD_PadLibData.adc_th              = 50;
	HSD_PadLibData.adc_angle           = sPadAdcAngle;
	HSD_PadLibData.clamp_stickType     = 0;
	HSD_PadLibData.clamp_stickShift    = 1;
	HSD_PadLibData.clamp_stickMax      = 80;
	HSD_PadLibData.clamp_stickMin      = 10;
	HSD_PadLibData.scale_stick         = 70;
	HSD_PadLibData.clamp_analogLRShift = 1;
	HSD_PadLibData.clamp_analogLRMax   = 200;
	HSD_PadLibData.clamp_analogLRMin   = 10;
	HSD_PadLibData.scale_analogLR      = 190;
	SetPadQueueType(1);
	HSD_VISetUserPostRetraceCallback(PadRetraceCallback);
	for (int i = 0; i < 4; i++) {
		mPad[i].Init(i);
	}
}

/**
 * @note Address: 0x8019E570
 * @note Size: 0x5C
 */
CPadEx::CPadEx()
{
	sPadEx = this;
}

/**
 * @note Address: 0x8019E5CC
 * @note Size: 0x10
 */
// CPadOne's implicit ctor, generated for the array in CPadEx

/**
 * @note Address: 0x8019E5DC
 * @note Size: 0x288
 */
void CPadOne::Update(s32 frames)
{
	mButton = mDirButton = HSD_PadCopyStatus[mPort].button;
	if (mButton & (HSD_PAD_STICK_UP | HSD_PAD_CSTICK_UP)) {
		mDirButton |= HSD_PAD_DPADUP;
	}
	if (mButton & (HSD_PAD_STICK_DOWN | HSD_PAD_CSTICK_DOWN)) {
		mDirButton |= HSD_PAD_DPADDOWN;
	}
	if (mButton & (HSD_PAD_STICK_LEFT | HSD_PAD_CSTICK_LEFT)) {
		mDirButton |= HSD_PAD_DPADLEFT;
	}
	if (mButton & (HSD_PAD_STICK_RIGHT | HSD_PAD_CSTICK_RIGHT)) {
		mDirButton |= HSD_PAD_DPADRIGHT;
	}

	mTrigger = mRepeat = (mButton ^ mLastButton) & ~mLastButton;
	mDirTrigger = mDirRepeat = (mDirButton ^ mLastDirButton) & ~mLastDirButton;

	// held directions repeat after 24 frames, then every 6
	u32 dirs[4] = {
		HSD_PAD_STICK_UP | HSD_PAD_CSTICK_UP | HSD_PAD_DPADUP,
		HSD_PAD_STICK_DOWN | HSD_PAD_CSTICK_DOWN | HSD_PAD_DPADDOWN,
		HSD_PAD_STICK_LEFT | HSD_PAD_CSTICK_LEFT | HSD_PAD_DPADLEFT,
		HSD_PAD_STICK_RIGHT | HSD_PAD_CSTICK_RIGHT | HSD_PAD_DPADRIGHT,
	};

	for (int i = 0; i < 4; i++) {
		if ((mButton | mDirButton) & dirs[i]) {
			if ((mRepeatTimer[i] -= frames) <= 0) {
				if (mButton & dirs[i]) {
					mRepeat |= mButton & dirs[i];
				}
				if (mDirButton & dirs[i]) {
					mDirRepeat |= mDirButton & dirs[i];
				}
				mRepeatTimer[i] = 6;
			}
		} else {
			mRepeatTimer[i] = 24;
		}
	}

	mStickX    = HSD_PadCopyStatus[mPort].nml_stickX;
	mStickY    = HSD_PadCopyStatus[mPort].nml_stickY;
	mSubStickX = HSD_PadCopyStatus[mPort].nml_subStickX;
	mSubStickY = HSD_PadCopyStatus[mPort].nml_subStickY;
	mAnalogL   = HSD_PadCopyStatus[mPort].nml_analogL;
	mAnalogR   = HSD_PadCopyStatus[mPort].nml_analogR;
	mConnected = HSD_PadCopyStatus[mPort]._41;

	mLastButton    = mButton;
	mLastDirButton = mDirButton;
}

/**
 * @note Address: 0x8019E864
 * @note Size: 0x58
 */
void CPadOne::Clear()
{
	mButton = mDirButton = 0;
	mLastButton = mLastDirButton = 0;
	mTrigger = mDirTrigger = mRepeat = mDirRepeat = 0;
	for (int i = 0; i < 4; i++) {
		mRepeatTimer[i] = 24;
	}
	mStickX = mStickY = sPadAnalogNeutral;
	mSubStickX = mSubStickY = sPadAnalogNeutral;
	mAnalogL = mAnalogR = sPadAnalogNeutral;
}

/**
 * @note Address: 0x8019E8BC
 * @note Size: 0x38
 */
void CPadOne::Init(u8 port)
{
	mPort      = port;
	mConnected = 0;
	Clear();
}

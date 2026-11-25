#include "game/leticker_camera.h"
#include "game/cgame.h"
#include "game/cpad.h"
#include <sysdolphin/controller.h>

/**
 * @note Address: 0x800E64A8
 * @note Size: 0x2E8
 */
void CCameraActionLeticker::Update(u32 step)
{
	CPad pad(0);
	u64 buttons     = gGame.mStatus.mFlags & 0x80000000;
	u32 leftButton  = buttons ? HSD_PAD_DPADLEFT : HSD_PAD_CSTICK_LEFT;
	u32 rightButton = buttons ? HSD_PAD_DPADRIGHT : HSD_PAD_CSTICK_RIGHT;

	if (mTurnFrames == 0) {
		if (pad.mPad->mTrigger & leftButton) {
			if (CanTurnLetickerCamera(*this, mTurnTo - HALF_PI)) {
				mTurnTo -= HALF_PI;
				mTurnSpeed  = -0.08726646f; // five degrees a frame, a quarter in 18
				mTurnFrames = 18;
			}
		} else if (pad.mPad->mTrigger & rightButton) {
			if (CanTurnLetickerCamera(*this, HALF_PI + mTurnTo)) {
				mTurnTo += HALF_PI;
				mTurnSpeed  = 0.08726646f;
				mTurnFrames = 18;
			}
		}
	} else {
		mTurnFrames -= step;
		if (mTurnFrames <= 0) {
			mAngle      = mTurnTo;
			mTurnFrames = 0;
		} else {
			mAngle += mTurnSpeed * step;
		}
	}
	CCameraActionLadr::Update(step);
}

/**
 * @note Address: 0x800E6790
 * @note Size: 0x68
 */
void CCameraActionLeticker::Reset()
{
#if !defined(VERSION_PAL)
	OSReport("CCameraActionLeticker::Reset\n");
#endif
	CCameraActionLadr::Reset();
	mTurnTo     = 0.0f;
	mTurnFrames = 0;
	mBounded    = false;
	gGame.mStatus.mFlags |= 0x80;
}

/**
 * @note Address: 0x800E67F8
 * @note Size: 0x20
 */
Vec CCameraActionLeticker::GetCameraTarget()
{
	return *mTarget;
}

CCameraActionLeticker gLetickerCameraAction;

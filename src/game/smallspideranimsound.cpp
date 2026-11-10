#include "game/xobjspidersmall.h"
#include "game/cgame.h"
#include "game/animframe.h"

/**
 * @note Address: 0x800F4AAC
 * @note Size: 0x164
 */
// NONMATCHING: anim and jobj swap r29 and r30
void XObjSpiderSmall::UpdateAnimationSounds()
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
		s32 anim = mAnimId;
		mAnimSfx.Update(anim, GetAnimFrame(mJObj), GetSoundVolumeAtPosition(&gGame.mSound, &mMoveTranslate, 0));
	}
}

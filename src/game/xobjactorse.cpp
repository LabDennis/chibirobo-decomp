#include "game/xobjactor.h"
#include "game/xobjspidersmall.h"
#include "game/cgame.h"

// Deferred inlining emits these functions in reverse definition order.
/**
 * @note Address: 0x80106BE0
 * @note Size: 0xC4
 */
void XObjActor::UpdateAnimationSounds(f32 frame)
{
	if (mStatus.mFlags & 1ULL) {
		if (mStatus.mFlags & 0x10000ULL) {
			u8 volume;
			if (_998.mFlags & 0x800) {
				volume = 255;
			} else {
				volume = GetSoundVolumeAtPosition(&gGame.mSound, &mMoveTranslate, 0);
			}
			mAnimSfx.Update(mAnimId, frame, volume);
		}
	}
}

/**
 * @note Address: 0x80106B50
 * @note Size: 0x90
 */
// small spiders holding on to chibi robo
extern "C" s32 CountSmallSpidersGrabbingPlayer()
{
	s32 count = 0;
	for (s32 i = 0; i < 16; ++i) {
		XObjSpiderSmall* spider = (XObjSpiderSmall*)(gGame.mXObjs.mPools[18].mObjects + i * gGame.mXObjs.mPools[18].mStride);
		if (spider->mStatus.mFlags & 1ULL) {
			if (IsSmallSpiderGrabbingPlayer(spider)) {
				++count;
			}
		}
	}
	return count;
}

/**
 * @note Address: 0x80106AC0
 * @note Size: 0x90
 */
extern "C" s32 CountEmergedSmallSpiders()
{
	s32 count = 0;
	for (s32 i = 0; i < 16; ++i) {
		XObjSpiderSmall* spider = (XObjSpiderSmall*)(gGame.mXObjs.mPools[18].mObjects + i * gGame.mXObjs.mPools[18].mStride);
		if (spider->mStatus.mFlags & 1ULL) {
			if (HasSmallSpiderEmerged(spider)) {
				++count;
			}
		}
	}
	return count;
}

/**
 * @note Address: 0x80106A98
 * @note Size: 0x28
 */
CVec XObjActor::GetHeadWorldPosition()
{
	return mMoveTranslate;
}

/**
 * @note Address: 0x80106A90
 * @note Size: 0x8
 */
HSD_JObj* XObjActor::GetHeadJoint()
{
	return mJObj;
}

/**
 * @note Address: 0x80106A8C
 * @note Size: 0x4
 */
extern "C" void ResetActorState__9XObjActorFv(XObjActor* actor)
{
}

#include "game/xobjspiderboss.h"
#include "game/video.h"

u32 gSpiderBossCoinBounceContext; // the boss's coin bounce token, the callbacks never touch it; its real type is unknown

/**
 * @note Address: 0x8010660C
 * @note Size: 0x2C8
 */
// NONMATCHING: retail copies the six zero Vecs from +0x48 of the file's .rodata, behind the header-made
// block of six Vecs this split unit lacks, so every load from them has another offset; the code is otherwise the same
extern "C" void ResetSpiderBossState(XObj* object)
{
	XObjSpiderBoss* spider       = (XObjSpiderBoss*)object;
	spider->mHeading             = 0.0f;
	spider->mSpeed               = 0.0f;
	spider->mTargetRotY          = 0.0f;
	spider->mRotSpeed            = 0.1f;
	spider->mSpeedY              = 0.0f;
	spider->_9A0                 = 0.0f;
	spider->_9A4                 = 0.0f;
	spider->mState               = 0;
	spider->mPrevState           = 0;
	spider->mStep                = 0;
	spider->mTimer               = 0;
	spider->mSpiderStatus.mFlags = 0;
	spider->mActionTimer         = 0;
	spider->_9C4                 = 1.0f;
	spider->_9C8                 = 1.0f;
	{
		Vec value       = { 0.0f, 0.0f, 0.0f };
		spider->mHitPos = value;
	}
	{
		Vec value        = { 0.0f, 0.0f, 0.0f };
		spider->mBasePos = value;
	}
	{
		Vec value             = { 0.0f, 0.0f, 0.0f };
		spider->mPatrolTarget = value;
	}
	spider->mHitStrength = 0.0f;
	spider->mMaxHits     = 23;
	spider->mHits        = spider->mMaxHits;
	spider->mCommand     = 0;
	spider->mArgs[0]     = 0;
	spider->_A44         = 0;
	spider->_994         = 1.0f;
	spider->mStatus.mFlags |= 8ULL;
	spider->mRadius = 28.0f;
	{
		Vec value        = { 0.0f, 0.0f, 0.0f };
		spider->mAreaPos = value;
	}
	spider->mAreaWidth = 0.0f;
	spider->mAreaDepth = 0.0f;
	{
		Vec value    = { 0.0f, 0.0f, 0.0f };
		spider->_A10 = value;
	}
	spider->_A1C = 0.0f;
	spider->vf12C(1, 0, 0, 0, 0.0f);
	spider->_A48                 = 0.0f;
	spider->mEscapeInputStrength = 0.0f;
	spider->mDrainTimer          = 0;
	spider->mEscapeRemaining     = 0.0f;
	{
		Vec value    = { 0.0f, 0.0f, 0.0f };
		spider->_A58 = value;
	}
#if defined(VERSION_PAL)
	spider->mGrabIntervalFrames = 3 * gVideoFramesPerSecond; // 3 seconds
#else
	spider->mGrabIntervalFrames = 180;
#endif
	spider->mAreaPos.x  = -200.0f;
	spider->mAreaPos.y  = 0.0f;
	spider->mAreaPos.z  = -200.0f;
	spider->mAreaWidth  = 400.0f;
	spider->mAreaDepth  = 400.0f;
	spider->mDamageStep = 0;
	spider->mBlinkTimer = 0;
	for (u32 i = 0; i < 8; ++i) {
		spider->mParts[i].mObj   = NULL;
		spider->mParts[i].mJoint = NULL;
	}
	if (spider->mPoolIndex == 0) {
		ResetSpiderCoinBounces(&gSpiderBossCoinBounceContext);
	}
	spider->mPatrolSelection = 0;
	spider->mAltAnimId       = 0;
	spider->mGrabCooldown    = 5;
	spider->mStatus.mFlags |= 4ULL;
	spider->mPatrolEffectActive = 0;
}

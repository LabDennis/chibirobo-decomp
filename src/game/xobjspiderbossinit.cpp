#include "game/xobjspiderboss.h"
#include "game/video.h"

/**
 * @note Address: 0x8010660C
 * @note Size: 0x2C8
 */
extern "C" void ResetSpiderBossState(XObj* object)
{
	XObjSpiderBoss* spider       = static_cast<XObjSpiderBoss*>(object);
	const Vec origin             = { 0.0f, 0.0f, 0.0f };
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
	spider->mHitPos              = origin;
	spider->mBasePos             = origin;
	spider->mPatrolTarget        = origin;
	spider->mHitStrength         = 0.0f;
	spider->mMaxHits             = 23;
	spider->mHits                = spider->mMaxHits;
	spider->mCommand             = 0;
	spider->mArgs[0]             = 0;
	spider->_A44                 = 0;
	spider->_994                 = 1.0f;
	spider->mStatus.mFlags |= 8ULL;
	spider->mRadius    = 28.0f;
	spider->mAreaPos   = origin;
	spider->mAreaWidth = 0.0f;
	spider->mAreaDepth = 0.0f;
	spider->mStartPos  = origin;
	spider->_A1C       = 0.0f;
	spider->SetColor(1, 0, 0, 0, 0.0f);
	spider->_A48                 = XObjSpiderBoss::sZero;
	spider->mEscapeInputStrength = XObjSpiderBoss::sZero;
	spider->mDrainTimer          = 0;
	spider->mEscapeRemaining     = XObjSpiderBoss::sZero;
	spider->_A58                 = origin;
#if defined(VERSION_PAL)
	spider->mGrabIntervalFrames = 3 * gVideoFramesPerSecond; // 3 seconds
#else
	spider->mGrabIntervalFrames = 180;
#endif
	spider->ResetPatrolArea();
	spider->mDamageStep = 0;
	spider->mBlinkTimer = 0;
	for (u32 i = 0; i < 8; ++i) {
		spider->mParts[i].mObj   = NULL;
		spider->mParts[i].mJoint = NULL;
	}
	if (spider->mPoolIndex == 0) {
		ResetSpiderCoinBounces(NULL);
	}
	spider->mPatrolSelection = 0;
	spider->mAltAnimId       = 0;
	spider->mGrabCooldown    = 5;
	spider->mStatus.mFlags |= 4ULL;
	spider->mPatrolEffectActive = 0;
}

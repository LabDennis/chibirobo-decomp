#include "game/xobjspiderboss.h"
#include "game/placement.h"

/**
 * @note Address: 0x80106088
 * @note Size: 0x584
 */
extern "C" void ApplySpiderBossPlacement(XObjSpiderBoss* spider)
{
	spider->SetColor(1, 0, 0, 0, 0.0f);
	const StagePlacement& placement = *spider->mCoord.mPlacement;
	spider->mMoveTranslate.Set(PlacementPosition(spider->mCoord));
	spider->mBasePos   = spider->mMoveTranslate;
	spider->mHeading   = DEG2RAD * placement.mRot[1];
	spider->mRadius    = 28.0f * (placement.mScale[0] / 100.0f);
	spider->mStep      = 0;
	spider->mTimer     = 0;
	spider->mPrevState = spider->mState;
	spider->mState     = 0;
	spider->mSpiderStatus.mFlags |= 0x02000000 | 0x8000;
}

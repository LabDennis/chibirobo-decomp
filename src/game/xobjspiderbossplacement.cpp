#include "game/xobjspiderboss.h"
#include "game/spiderplacement.h"

Vec gSpiderBossPosition, gSpiderBossScale, gSpiderBossRotation;

/**
 * @note Address: 0x80106088
 * @note Size: 0x584
 */
// NONMATCHING: retail reads the three start Vecs at +0xD4 of the file's .bss, behind a header-made prefix this
// split unit lacks, so every load from them has another offset; the code is otherwise the same
extern "C" void ApplySpiderBossPlacement(XObjSpiderBoss* spider)
{
	spider->vf12C(1, 0, 0, 0, 0.0f);
	spider->mMoveTranslate.x = PlacementPosition(spider->mCoord, gSpiderBossPosition).x;
	spider->mMoveTranslate.y = PlacementPosition(spider->mCoord, gSpiderBossPosition).y;
	spider->mMoveTranslate.z = PlacementPosition(spider->mCoord, gSpiderBossPosition).z;
	spider->mBasePos.x       = PlacementPosition(spider->mCoord, gSpiderBossPosition).x;
	spider->mBasePos.y       = PlacementPosition(spider->mCoord, gSpiderBossPosition).y;
	spider->mBasePos.z       = PlacementPosition(spider->mCoord, gSpiderBossPosition).z;
	spider->mHeading         = PlacementRotation(spider->mCoord, gSpiderBossRotation).y;
	spider->mRadius          = 28.0f * PlacementScale(spider->mCoord, gSpiderBossScale).x;
	spider->mStep            = 0;
	spider->mTimer           = 0;
	spider->mPrevState       = spider->mState;
	spider->mState           = 0;
	spider->mSpiderStatus.mFlags |= 0x8000;
	spider->mSpiderStatus.mFlags |= 0x02000000;
}

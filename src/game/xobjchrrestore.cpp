#include "game/xchr.h"
#include "game/xobjs.h"
#include "game/cgame.h"
#include "game/stagedata.h"

/**
 * @note Address: 0x800B78FC
 * @note Size: 0x260
 */
// picks up again what he was holding before the reset, true once it's back in his hands
extern "C" BOOL RestorePlayerHeldItem(XObjChr* chr)
{
	if (chr->mLift.mState != 0) {
		XObj* target = chr->mLift.mTarget;
		if (target != NULL) {
			if ((s16)target->GetPool() == 24) {
				chr->mLift.mTarget      = &chr->mPlug;
				chr->mLift.mTargetFloor = chr->mLift.mTarget->mSelectedFloorIndex;
				chr->mPlug.mStatus.mFlags |= 0x0400000000000000ULL;
				chr->SetAnim(8, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				return TRUE;
			}
		}
		ResetPlayerLiftAction(&chr->mLift);
	}

	s16 id = chr->mSavedTarget;
	if (id >= 0) {
		XObj* loaded        = SpawnObjectFromPlacement(&gGame.mXObjs, &CoordInfo(GetStageObjectPlacementByIndex(id)), 0, TRUE);
		chr->mPendingTarget = loaded;
		chr->mSavedTarget   = -1;
	}
	if (chr->mPendingTarget != NULL) {
		if (!(chr->mPendingTarget->mStatus.mFlags & 0x0040000000000000ULL)) {
			ResetPlayerLiftAction(&chr->mLift);
			chr->mLift.mState = 4;
			chr->mLift.SetTarget(chr->mPendingTarget);
			chr->mLift._24 = 4;
			chr->mLift.mTarget->SetAnim(chr->mSavedTargetAnim, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			chr->mLift.mTarget->mStatus.mFlags |= 0x0400000000000000ULL;
			chr->SetAnim(8, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			chr->mPendingTarget = NULL;
			if (IsCarryingSquirter(&chr->mLift)) {
				ApplyPlayerItemTintToTarget(&chr->mLift);
			}
			return TRUE;
		}
	} else {
		return TRUE;
	}
	return FALSE;
}

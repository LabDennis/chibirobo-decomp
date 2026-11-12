#include "game/xobjspiderboss.h"
#include "game/xchr.h"
#include "game/cgame.h"

// todo: cleanup
static inline void SetPosAndTarget(CObjVec* vec, const Vec& value)
{
	vec->Set(value);
	vec->SetTarget(value);
}

/**
 * @note Address: 0x80105D58
 * @note Size: 0x2A8
 */
// UpdateObject, the update
void XObjSpiderBoss::UpdateObject(u32 frames)
{
	CPad pad(0); // retail does this
	if (gGame.mStatus.mFlags & 0x400ULL) {
		Vec& move = gGame.GetPlayer()->_158C4;
		move.x    = 0.0f;
		move.y    = 0.0f;
		move.z    = 0.0f;
		return;
	}
	if (mPoolIndex == 0) {
		UpdateSpiderCoinBounces(&gSpiderBossCoinBounceContext, frames);
	}
	if (!(mStatus.mFlags & 0x10000ULL) && mState != 1) {
		return;
	}
	UpdateAnimationSounds();
	UpdateSpiderBossDamage(this, frames);
	if (!(mSpiderStatus.mFlags & 0x02000000)) {
		(this->*gSpiderBossStateHandlers[mState])(frames);
		CVec step;
		if (mSpiderStatus.mFlags & 8) {
			mMoveTranslate.y += mSpeedY;
		} else {
			mMoveTranslate.y = mBasePos.y;
		}
		step.x = mSpeed * (f32)sin(mHeading);
		step.z = mSpeed * (f32)cos(mHeading);
		step.y = 0.0f;
		SetRotateY(mTargetRotY, mRotSpeed);
		SetPosAndTarget(&mMoveTranslate, mMoveTranslate + step * (f32)frames);
		SetPos(mMoveTranslate, 0);
	}
	UpdateObject__4XObjFUl(this, frames);
	UpdateSpiderBossAttachments(this, frames);
}

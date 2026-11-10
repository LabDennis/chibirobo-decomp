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
extern "C" void UpdateObject__14XObjSpiderBossFUl(XObjSpiderBoss* spider, u32 frames)
{
	CPad pad(0); // retail does this
	if (gGame.mStatus.mFlags & 0x400ULL) {
		Vec& move = gGame.GetPlayer()->_158C4;
		move.x    = 0.0f;
		move.y    = 0.0f;
		move.z    = 0.0f;
		return;
	}
	if (spider->mPoolIndex == 0) {
		UpdateSpiderCoinBounces(&gSpiderBossCoinBounceContext, frames);
	}
	if (!(spider->mStatus.mFlags & 0x10000ULL) && spider->mState != 1) {
		return;
	}
	spider->UpdateAnimationSounds();
	UpdateSpiderBossDamage(spider, frames);
	if (!(spider->mSpiderStatus.mFlags & 0x02000000)) {
		(spider->*gSpiderBossStateHandlers[spider->mState])(frames);
		CVec step;
		if (spider->mSpiderStatus.mFlags & 8) {
			spider->mMoveTranslate.y += spider->mSpeedY;
		} else {
			spider->mMoveTranslate.y = spider->mBasePos.y;
		}
		step.x = spider->mSpeed * (f32)sin(spider->mHeading);
		step.z = spider->mSpeed * (f32)cos(spider->mHeading);
		step.y = 0.0f;
		spider->SetRotateY(spider->mTargetRotY, spider->mRotSpeed);
		SetPosAndTarget(&spider->mMoveTranslate, spider->mMoveTranslate + step * (f32)frames);
		spider->SetPos(spider->mMoveTranslate, 0);
	}
	UpdateObject__4XObjFUl(spider, frames);
	UpdateSpiderBossAttachments(spider, frames);
}

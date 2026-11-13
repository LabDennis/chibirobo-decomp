#include "game/xobjactor.h"
#include "game/objectfloorquery.h"
#include "game/cgame.h"
#include <math.h>

static inline void SetGroundPosition(CObjVec& position, const CVec& next)
{
	position.Set(next.x, next.y, next.z);
	position.SetTarget(next);
}

// the actor's position, by way of a zeroed copy
static inline void GetGroundPosition(XObj* object, Vec* out)
{
	Vec position = { 0.0f, 0.0f, 0.0f };
	position     = object->mMoveTranslate;
	*out         = position;
}

/**
 * @note Address: 0x80106CA4
 * @note Size: 0x3C8
 */
void XObjActor::UpdateGroundMovement(u32 frames)
{
	// Retail uses the origin for the floor probe before filling it with the actor's final position.
	Vec pos = { 0.0f, 0.0f, 0.0f };
	CVec step;
	step.x = mMoveSpeed * (f32)sin(mHeading);
	step.z = mMoveSpeed * (f32)cos(mHeading);
	step.y = 0.0f;
	SetGroundPosition(mMoveTranslate, mMoveTranslate + step * (f32)frames);
	f32 offset = 0.0f;
	switch (GetKind()) {
	case 15:
		offset = 6.0f;
		break;
	}
	mActorStatus.mFlags &= ~0x20;
	if (FindMapFloor((XObjMap*)gGame.mXObjs.mPools[3].mObjects, &pos, &mFloorHit, NULL)) {
		mGroundHeight = mFloorHit.mHeight + offset;
		if (!(mActorStatus.mFlags & 8)) {
			mMoveTranslate.y = mFloorHit.mHeight + offset;
			mActorStatus.mFlags |= 0x20;
		} else if (mActorStatus.mFlags & 0x10) {
			if (mFallSpeed >= -2.3f)
				mFallSpeed -= 0.36f * (f32)frames;
			else
				mFallSpeed = -2.3f;
			if (mMoveTranslate.y <= mGroundHeight)
				mMoveTranslate.y = mGroundHeight;
			mMoveTranslate.y += mFallSpeed;
			if (mMoveTranslate.y <= mGroundHeight) {
				mFallSpeed = 0.0f;
				mActorStatus.mFlags &= ~8;
				mActorStatus.mFlags &= ~0x10;
			}
		} else {
			mMoveTranslate.y += mFallSpeed;
		}
	} else {
		mMoveTranslate.y = offset;
		mActorStatus.mFlags |= 0x20;
	}
	GetGroundPosition(this, &pos);
	SetPos(pos, 0);
	SetRotateY(mFacing, 0.0f);
}

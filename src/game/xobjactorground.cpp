#include "game/xobjactor.h"
#include "game/objectfloorquery.h"
#include "game/cgame.h"
#include <math.h>

/**
 * @note Address: 0x80106CA4
 * @note Size: 0x3C8
 */
void XObjActor::UpdateGroundMovement(u32 frames)
{
	// The floor search starts at the stage origin.
	Vec floorProbe = { 0.0f, 0.0f, 0.0f };
	CVec step;
	step.x          = mMoveSpeed * (f32)sin(mHeading);
	step.z          = mMoveSpeed * (f32)cos(mHeading);
	step.y          = 0.0f;
	const CVec next = mMoveTranslate + step * (f32)frames;
	mMoveTranslate.Set(next);
	mMoveTranslate.SetTarget(next);

	const f32 offset = GetKind() == 15 ? 6.0f : 0.0f;
	XObjMap* map     = static_cast<XObjMap*>(gGame.mXObjs.mPools[3].First());
	mActorStatus.mFlags &= ~0x20;
	if (FindMapFloor(map, &floorProbe, &mFloorHit, NULL)) {
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
	const Vec position = mMoveTranslate;
	SetPos(position, 0);
	SetRotateY(mFacing, 0.0f);
}

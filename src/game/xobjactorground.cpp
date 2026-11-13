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
	step.x = mMoveSpeed * (f32)sin(_980);
	step.z = mMoveSpeed * (f32)cos(_980);
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
		_9DC = mFloorHit.mHeight + offset;
		if (!(mActorStatus.mFlags & 8)) {
			mMoveTranslate.y = mFloorHit.mHeight + offset;
			mActorStatus.mFlags |= 0x20;
		} else if (mActorStatus.mFlags & 0x10) {
			if (_9D8 >= -2.3f)
				_9D8 -= 0.36f * (f32)frames;
			else
				_9D8 = -2.3f;
			if (mMoveTranslate.y <= _9DC)
				mMoveTranslate.y = _9DC;
			mMoveTranslate.y += _9D8;
			if (mMoveTranslate.y <= _9DC) {
				_9D8 = 0.0f;
				mActorStatus.mFlags &= ~8;
				mActorStatus.mFlags &= ~0x10;
			}
		} else {
			mMoveTranslate.y += _9D8;
		}
	} else {
		mMoveTranslate.y = offset;
		mActorStatus.mFlags |= 0x20;
	}
	GetGroundPosition(this, &pos);
	SetPos(pos, 0);
	SetRotateY(_988, 0.0f);
}

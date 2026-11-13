#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/gmath.h"

static inline void SetTargetYaw(XObjActor* actor, const CVec& head)
{
	f32 facing        = actor->GetRotationY();
	actor->mLookAngle = (f32)atan2(actor->mLookTarget->x - head.x, actor->mLookTarget->z - head.z) - facing;
}

static inline void SetTargetPitch(XObjActor* actor, const CVec& target, const CVec& head)
{
	f32 dx = target.x - head.x, dz = target.z - head.z, dy = target.y - head.y;
	f32 horizontal    = sqrtf(dx * dx + dz * dz);
	f32 angle         = (f32)atan2(horizontal, sqrtf(dy * dy));
	actor->mHeadPitch = dy < 0.0f ? HALF_PI - angle : angle - HALF_PI;
}

/**
 * @note Address: 0x80107750
 * @note Size: 0x690
 */
void XObjActor::UpdateAfterAnim(u32 frames)
{
	if (mActorStatus.mFlags & 0x80) {
		switch (mLookMode) {
		case 0:
			break;
		case 2:
			break;
		case 1: {
			SetTargetYaw(this, GetHeadWorldPosition());
			f32 limit = (f32)cos(GetMaxHeadYaw());
			if ((f32)cos(mLookAngle) < limit) {
				if (0.0f < (f32)sin(mLookAngle))
					mLookAngle = GetMaxHeadYaw();
				else
					mLookAngle = GetMinHeadYaw();
			}
		}
		}
		f32 step = (f32)cos(mLookRate);
		if ((f32)cos(mLookAngle - mHeadYaw) <= step) {
			if (0.0f < (f32)sin(mHeadYaw - mLookAngle))
				mHeadYaw -= mLookRate;
			else
				mHeadYaw += mLookRate;
		} else {
			mHeadYaw = mLookAngle;
			if ((u32)mLookMode == 0)
				mActorStatus.mFlags &= ~0x80;
		}
		Push(GetHeadJoint(), 0x80, mHeadYaw);
	}
	if (mActorStatus.mFlags & 0x400) {

		CVec target;
		target.x = gGame.GetPlayer()->mMoveTranslate.x;
		target.y = 4.0f + gGame.GetPlayer()->mMoveTranslate.y;
		target.z = gGame.GetPlayer()->mMoveTranslate.z;
		SetTargetPitch(this, target, GetHeadWorldPosition());
		if (mHeadPitch > GetMaxHeadPitch())
			mHeadPitch = GetMaxHeadPitch();
		else if (mHeadPitch < GetMinHeadPitch())
			mHeadPitch = GetMinHeadPitch();
		Push(GetHeadJoint(), 0x40, mHeadPitch);
	}
	if (mActorStatus.mFlags & 0x1000) {
		for (u32 i = 0; i < 8; ++i) {
			if (mParts[i].mJoint && mParts[i].mObj)
				XObjJointPart::UpdateMatrix(*this, i);
		}
	}
}

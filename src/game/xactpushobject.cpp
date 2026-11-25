#include "game/cgame.h"
#include "game/gmath.h"
#include "game/objectfloorquery.h"
#include "game/xact.h"
#include "game/xchr.h"

#include <math.h>

extern "C" {
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
}

extern "C" {
BOOL SetClampedPushPositionXZ(XObj* target, const Vec* direction, u8 axes, f32 x, f32 z);
f32 QueryObjectFloorHeight(XObj* target, f32 radius, f32 height);

static const f32 pushAngleTolerance = 0.1f * DEG2RAD;

static inline Vec PushEdgePoint(const XObj* obj, s32 index, bool first, f32 height)
{
	const Vec& point = first ? obj->mEdges[index].mFrom : obj->mEdges[index].mTo;
	Vec v            = { obj->mMoveTranslate.x + point.x, height, obj->mMoveTranslate.z + point.z };
	return v;
}
static inline f32 FacingPushEdge(XObj* edgeObject, XObjChr* owner)
{
	s32 index   = edgeObject->mSelectedFloorIndex;
	Vec& player = owner->mMoveTranslate;
	Vec a       = PushEdgePoint(edgeObject, index, true, player.y);
	Vec b       = PushEdgePoint(edgeObject, index, false, player.y);
	CVec normal;
	GetOutwardPerpendicularToEdge(&player, &a, &b, &normal);
	return WrapAngle(atan2(normal.x, normal.z));
}
/**
 * @note Address: 0x800C494C
 * @note Size: 0x924
 */
void UpdatePushedObject(XActPush* act)
{
	if (!act->mTargetState)
		return;
	XObj* target = act->mTarget;
	if (!target || !(target->mStatus.mFlags & 1))
		return;
	switch (act->mTargetState) {
	case 1: {
		target->mStatus.mFlags |= 0x0800000000000000ULL;
		Vec hand = GetPlayerCarryJointWorldPosition(act->mOwner);
		act->mTargetOffset.Set(target->mMoveTranslate - hand);
		break;
	}
	case 2: {
		Vec hand         = GetPlayerCarryJointWorldPosition(act->mOwner);
		XObj* edgeObject = act->mTarget;
		f32 angle        = WrapAngle(FacingPushEdge(edgeObject, act->mOwner) - edgeObject->mMoveRotate.y);
		Mtx rotate;
		PSMTXRotRad(rotate, 'y', angle);
		CVec direction(0, 0, act->mMoveDirection);
		PSMTXMultVec(rotate, &direction, &direction);
		if ((angle > (90 * DEG2RAD - pushAngleTolerance) && angle < (90 * DEG2RAD + pushAngleTolerance))
		    || (angle > (270 * DEG2RAD - pushAngleTolerance) && angle < (270 * DEG2RAD + pushAngleTolerance)))
			act->mPushPositionClamped
			    = SetClampedPushPositionXZ(target, &direction, 1, hand.x + act->mTargetOffset.x, hand.z + act->mTargetOffset.z);
		else
			act->mPushPositionClamped
			    = SetClampedPushPositionXZ(target, &direction, 2, hand.x + act->mTargetOffset.x, hand.z + act->mTargetOffset.z);

		target->OnPushDisplacement(target->mMoveTranslate - target->mLastPos);
		if ((target->mStatus.mFlags & 0x4000000000ULL) && QueryObjectFloorHeight(target, 0.1f, 0.1f) < target->mMoveTranslate.y - 5.0f) {
			target->mStatus.mFlags &= ~0x0800000000000000ULL;
			ReleasePushAction(act);
			XObjChr* owner = act->mOwner;
			owner->SetAnim(17, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			owner->mIdleLoops = 3;
			act->mState       = 0;
			target->mStatus.mFlags |= 0x1000000000000000ULL;

			Vec rotation = target->mMoveRotate, speed = target->mMoveRotate.mSpeed;
			Vec extent = ReadObjectFloorExtent(target->mFloors[0]);
			// The cardinal cases supply an offset. An off-axis drop has no sideways offset.
			f32 dx = 0.0f, dz = 0.0f;
			if ((angle > (90 * DEG2RAD - pushAngleTolerance) && angle < (90 * DEG2RAD + pushAngleTolerance))) {
				rotation.z -= HALF_PI;
				speed.z = 0.1f;
				dz      = 0;
				dx      = extent.x * (2.0f / sqrtf(3.0f)) * 0.5f;
			} else if ((angle > (270 * DEG2RAD - pushAngleTolerance) && angle < (270 * DEG2RAD + pushAngleTolerance))) {
				rotation.z += HALF_PI;
				speed.z = 0.1f;
				dz      = 0;
				dx      = -(extent.x * (2.0f / sqrtf(3.0f))) * 0.5f;
			} else if ((angle > (0 * DEG2RAD - pushAngleTolerance) && angle < (0 * DEG2RAD + pushAngleTolerance))) {
				rotation.x += HALF_PI;
				speed.x = 0.1f;
				dx      = 0;
				dz      = extent.z * (2.0f / sqrtf(3.0f)) * 0.5f;
			} else if ((angle > (180 * DEG2RAD - pushAngleTolerance) && angle < (180 * DEG2RAD + pushAngleTolerance))) {
				rotation.x -= HALF_PI;
				speed.x = 0.1f;
				dx      = 0;
				dz      = -(extent.z * (2.0f / sqrtf(3.0f))) * 0.5f;
			}
			target->mMoveRotate.mTarget.Set(rotation);
			target->mMoveRotate.mSpeed.Set(speed);
			target->mToppleOffset = CVec(dx, -(extent.y * (2.0f / sqrtf(3.0f))) * 0.5f, dz);
		}
		break;
	}
	}
}

} // extern "C"

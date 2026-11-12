#include "game/xobjactor.h"

extern "C" {
extern Vec sActorHitPositionZero;
}

/**
 * @note Address: 0x8010753C
 * @note Size: 0xC
 */
extern "C" bool vf160__9XObjActorFv(XObjActor* actor)
{
	return (actor->_998.mFlags & 2) != 0;
}

/**
 * @note Address: 0x80107548
 * @note Size: 0xA0
 */
// OnHitFromPosition, knocked from pos
void XObjActor::OnHitFromPosition(const Vec& pos, f32 strength)
{
	OnHitFromPosition__4XObjFRC3Vecf(this, &pos, strength);
	_998.mFlags |= 2;
	f32 x        = pos.x;
	f32 y        = pos.y;
	f32 z        = pos.z;
	Vec hit      = sActorHitPositionZero;
	hit.x        = x;
	hit.y        = y;
	hit.z        = z;
	mHitPos      = hit;
	mHitStrength = strength;
}

/**
 * @note Address: 0x801075E8
 * @note Size: 0x14
 */
extern "C" s32 vf100__9XObjActorFll(XObjActor* actor, s32 unused, s32 value)
{
	actor->_998.mFlags |= 1;
	return value;
}

/**
 * @note Address: 0x801075FC
 * @note Size: 0x4
 */
void XObjActor::DrawDebugStatus()
{
}

/**
 * @note Address: 0x80107600
 * @note Size: 0x20
 */
extern "C" void vfC8__9XObjActorFv(XObjActor* actor)
{
	vfC8__4XObjFv(actor);
}

/**
 * @note Address: 0x80107620
 * @note Size: 0x70
 */
extern "C" void vfAC__9XObjActorFv(XObjActor* actor)
{
	if (actor->_998.mFlags & 0x80) {
		actor->Push(actor->GetHeadJoint(), 0x80, actor->mHeadYaw);
	}
	actor->DrawWithView(NULL);
}

/**
 * @note Address: 0x80107690
 * @note Size: 0x8
 */
// the head's turn limits
f32 XObjActor::GetMinHeadPitch()
{
	return -0.34906584f;
}

/**
 * @note Address: 0x80107698
 * @note Size: 0x8
 */
f32 XObjActor::GetMaxHeadPitch()
{
	return 0.34906584f;
}

/**
 * @note Address: 0x801076A0
 * @note Size: 0x8
 */
f32 XObjActor::GetMinHeadYaw()
{
	return -1.3962634f;
}

/**
 * @note Address: 0x801076A8
 * @note Size: 0x8
 */
f32 XObjActor::GetMaxHeadYaw()
{
	return 1.3962634f;
}

/**
 * @note Address: 0x801076B0
 * @note Size: 0x2C
 */
// look at a Vec*
extern "C" void SetActorLookAtPosition(CXObj* object, const Vec* target, f32 rate)
{
	XObjActor* actor = (XObjActor*)object;
	actor->_998.mFlags |= 0x80;
	actor->mLookMode   = 1;
	actor->mLookRate   = rate;
	actor->mLookTarget = target;
	actor->_998.mFlags |= 0x400;
}

/**
 * @note Address: 0x801076DC
 * @note Size: 0x20
 */
extern "C" void SetActorLookAngle(CXObj* object, f32 angle, f32 rate)
{
	XObjActor* actor = (XObjActor*)object;
	actor->_998.mFlags |= 0x80;
	actor->mLookMode  = 3;
	actor->mLookAngle = angle;
	actor->mLookRate  = rate;
}

/**
 * @note Address: 0x801076FC
 * @note Size: 0x20
 */
extern "C" void SetActorLookTarget(CXObj* object, const Vec* target, f32 rate)
{
	XObjActor* actor = (XObjActor*)object;
	actor->_998.mFlags |= 0x80;
	actor->mLookMode   = 1;
	actor->mLookRate   = rate;
	actor->mLookTarget = target;
}

/**
 * @note Address: 0x8010771C
 * @note Size: 0x34
 */
// look ahead again
extern "C" void ResetActorLook(CXObj* object, f32 rate)
{
	XObjActor* actor = (XObjActor*)object;
	actor->_998.mFlags |= 0x80;
	actor->mLookMode  = 0;
	actor->mLookRate  = rate;
	actor->mLookAngle = 0.0f;
	actor->_998.mFlags &= ~0x400;
	actor->mHeadPitch = 0.0f;
}

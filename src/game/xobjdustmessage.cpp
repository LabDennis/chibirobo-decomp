#include "game/xobjhitevent.h"

// Animation defaults shared with the other free-object handlers.
extern "C" const f32 sObjectEffectZero, sObjectEffectUnit, sObjectAnimBlendRate;

// XObjDust::vf140
/**
 * @note Address: 0x800DE14C
 * @note Size: 0x2D8
 */
extern "C" BOOL vf140__8XObjDustFPC3Vecl(XObjDust* object, const Vec* position, s32 kind)
{
	if ((object->GetObjFlags() & 2) && kind == 16) {
		if (object->GetObjFlags() & 0x10) {
			object->mAnimId2 = (s32)((u32)object->mAnimId2 + 1);
			object->SetAnim(object->mAnimId2, FALSE, NULL, NULL, NULL, sObjectEffectZero, sObjectEffectUnit, sObjectEffectZero,
			                sObjectAnimBlendRate);
			if (object->mAnimId2 >= (s32)((u32)object->mAnimCount - 1)) {
				object->mStatus.mFlags |= 0x4000000000000000ULL;
				object->mEventId = kind;
			}
		} else
			return FALSE;
	} else if ((object->GetObjFlags() & 2) && kind == 56) {
		if (object->GetObjFlags() & 0x20) {
			object->mFrom.x = object->mMoveTranslate.x;
			object->mFrom.y = object->mMoveTranslate.y;
			object->mFrom.z = object->mMoveTranslate.z;
			object->mTo.x   = position->x;
			object->mTo.y   = position->y;
			object->mTo.z   = position->z;
			object->mStatus.mFlags |= 0x4000000000000000ULL;
			object->mEventId = kind;
			if (object->mStatus.mFlags & 0x00000C0000000000ULL)
				StartObjectHitEvent(object, object->mEventId, object->mEventArg);
		} else
			return FALSE;
	} else if (object->mStatus.mFlags & 0x0000040000000000ULL) {
		StartObjectHitEvent(object, kind, -1);
	} else
		return FALSE;
	return TRUE;
}

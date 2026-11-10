#include "game/xobjfree.h"
#include "game/xobjflags.h"
#include "game/cgame.h"
#include "game/globals.h"
#include "game/evt.h"
#include "game/xobjchr.h"

// todo: cleanup
static inline void TriggerEvent(XObj* object, u32 first, u32 second)
{
	const void* script = object->vf128();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer()))
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, (u32)object, first, second);
}

// XObjFree::Release
/**
 * @note Address: 0x800DE038
 * @note Size: 0x28
 */
extern "C" void Release__8XObjFreeFv(XObjFree* object)
{
	object->mMatAnim = NULL;
	Release__4XObjFv(object);
}

// XObjFree::ResetObjectState
/**
 * @note Address: 0x800DE060
 * @note Size: 0xEC
 */
extern "C" void ResetObjectState__8XObjFreeFv(XObjFree* object)
{
	ResetObjectState__4XObjFv(object);
	if (object->mAttr._00 != NULL)
		object->mLife = (s16)object->GetObjFlags();
	object->mMatAnim = NULL;
	if (object->mJObj != NULL && (object->mStatus.mFlags & 0x1000ULL)) {
		UnkStageObj* animation = object->mCoord._00;
		object->SetAnim(animation->_30, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	}
}

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
			object->SetAnim(object->mAnimId2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			if (object->mAnimId2 >= (s32)((u32)object->mAnimCount - 1)) {
				object->mStatus.mFlags |= 0x4000000000000000ULL;
				object->mEventId = kind;
			}
		} else
			return FALSE;
	} else if ((object->GetObjFlags() & 2) && kind == 56) {
		if (object->GetObjFlags() & 0x20) {
			f32 x = position->x, y = position->y, z = position->z;
			object->mFrom.x = object->mMoveTranslate.x;
			object->mFrom.y = object->mMoveTranslate.y;
			object->mFrom.z = object->mMoveTranslate.z;
			object->mTo.x   = x;
			object->mTo.y   = y;
			object->mTo.z   = z;
			object->mStatus.mFlags |= 0x4000000000000000ULL;
			object->mEventId = kind;
			if (object->mStatus.mFlags & 0x00000C0000000000ULL)
				TriggerEvent(object, object->mEventId, object->mEventArg);
		} else
			return FALSE;
	} else if (object->mStatus.mFlags & 0x0000040000000000ULL) {
		TriggerEvent(object, (s16)kind, -1);
	} else
		return FALSE;
	return TRUE;
}

// XObjDust::OnHitFromPosition
/**
 * @note Address: 0x800DE424
 * @note Size: 0x1E4
 */
extern "C" void OnHitFromPosition__8XObjDustFRC3Vecf(XObjDust* object, f32 strength)
{
	if (object->GetObjFlags() & 1) {
		object->mStatus.mFlags |= 0x4000000000000000ULL;
		if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f)
			object->mEventArg = 2001;
		else
			object->mEventArg = 2000;
	} else if (object->mStatus.mFlags & 0x0000080000000000ULL) {
		if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f)
			TriggerEvent(object, -1, 2001);
		else
			TriggerEvent(object, -1, 2000);
	}
}

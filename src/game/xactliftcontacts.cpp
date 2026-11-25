#include "game/cgame.h"
#include "game/cjobjinlineattach.h"
#include "game/xact.h"
#include "game/xchr.h"

#include <math.h>

extern "C" {

/**
 * @note Address: 0x800CE334
 * @note Size: 0x20
 */
void SetLiftPlugConnection(XActLift* act, u32 value, BOOL apply)
{
	act->mPlugConnection = value;
	if (apply) {
		act->mOwner->mPlug.mConnection = value | 0x10000;
	}
}

static inline bool CarriedObjectTouches(XObj* object, const Vec& pos, f32 radius, f32 height, Vec& push, Vec& contact)
{
	return (object->TestFloorCollision(&pos, &pos, radius, height, &push, &contact, NULL, 0) & 3) == 3;
}

/**
 * @note Address: 0x800CE354
 * @note Size: 0x5B0
 */
BOOL CheckCarriedObjectContacts(XActLift* act, const Vec* pos, f32 radius, f32 height, BOOL checkItem, BOOL checkEvent, BOOL checkOthers,
                                Vec* result)
{
	s16 item   = act->mTarget->GetItem();
	BOOL found = FALSE;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.GetObject(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000))
			continue;
		f32 dx = object->mMoveTranslate.x - pos->x;
		f32 dz = object->mMoveTranslate.z - pos->z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (radius + object->mFloorXZExtent))
			continue;
		Vec push, contact;
		if (checkItem && (object->mStatus.mFlags & 0x40000000000ULL)) {
			if (CarriedObjectTouches(object, *pos, radius, height, push, contact) && object->OnItemUsed(pos, item)) {
				found = TRUE;
				if (result)
					*result = contact;
			}
		}
		const bool receivesSquirterHit = (checkEvent && (object->mStatus.mFlags & 0x80000000000ULL))
		                              || (checkOthers && !(object->mStatus.mFlags & 0x2000000000000ULL)
		                                  && !(object->mStatus.mFlags & 0x100000) && object != act->mTarget && object != act->mOwner);
		if (receivesSquirterHit && CarriedObjectTouches(object, *pos, radius, height, push, contact)) {
			object->OnSquirterHit(&contact, act->mOwner->mSquirterContent, act->mOwner->mItemTintWord);
			found = TRUE;
			if (result)
				*result = contact;
		}
	}
	return found;
}

/**
 * @note Address: 0x800CE904
 * @note Size: 0xA4
 */
void UpdateCarriedObjectPosition(XActLift* act, XObj* target)
{
	const CVec carryPosition = GetPlayerCarryJointWorldPosition(act->mOwner);
	const Vec contactOffset  = GetLiftTargetContactOffset(act, target);
	target->SetPos(carryPosition - contactOffset, 0);
}

/**
 * @note Address: 0x800CE9A8
 * @note Size: 0x354
 */
Vec GetLiftTargetContactOffset(XActLift* act, XObj* target)
{
	// a plug (24) is held by its end joint
	if (target->GetCategory() == 24) {
		s32 path[] = { 0 };
		CJObj joint(GetJointByPath(target->mJObj, path, 1));
		CVec pos(joint.GetPos());
		return pos - target->mMoveTranslate;
	}
	Vec offset = target->mMoveTranslate * -1.0f;
	return GetFloorContactPosition(target, act->mTargetFloor, &offset);
}

} // extern "C"

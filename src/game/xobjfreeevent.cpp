#include "game/xobjfree.h"
#include "game/xobjflags.h"
#include "game/cgame.h"
#include "game/globals.h"
#include "game/evt.h"
#include "game/xobjchr.h"

// todo: cleanup
static inline void TriggerEvent(XObj* object, u32 first, u32 second)
{
	const void* script = object->GetEventScript();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer()))
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, (u32)object, first, second);
}

// XObjFree::Release
/**
 * @note Address: 0x800DE038
 * @note Size: 0x28
 */
void XObjFree::Release()
{
	mMatAnim = NULL;
	XObj::Release();
}

// XObjFree::ResetObjectState
/**
 * @note Address: 0x800DE060
 * @note Size: 0xEC
 */
void XObjFree::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	if (mAttr._00 != NULL)
		mLife = (s16)GetObjFlags();
	mMatAnim = NULL;
	if (mJObj != NULL && (mStatus.mFlags & 0x1000ULL)) {
		UnkStageObj* animation = mCoord._00;
		SetAnim(animation->_30, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	}
}

// XObjDust::OnItemUsed
/**
 * @note Address: 0x800DE14C
 * @note Size: 0x2D8
 */
BOOL XObjDust::OnItemUsed(const Vec* position, s32 kind)
{
	if ((GetObjFlags() & 2) && kind == 16) {
		if (GetObjFlags() & 0x10) {
			mAnimId2 = (s32)((u32)mAnimId2 + 1);
			SetAnim(mAnimId2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			if (mAnimId2 >= (s32)((u32)mAnimCount - 1)) {
				mStatus.mFlags |= 0x4000000000000000ULL;
				mEventId = kind;
			}
		} else
			return FALSE;
	} else if ((GetObjFlags() & 2) && kind == 56) {
		if (GetObjFlags() & 0x20) {
			f32 x = position->x, y = position->y, z = position->z;
			mFrom.x = mMoveTranslate.x;
			mFrom.y = mMoveTranslate.y;
			mFrom.z = mMoveTranslate.z;
			mTo.x   = x;
			mTo.y   = y;
			mTo.z   = z;
			mStatus.mFlags |= 0x4000000000000000ULL;
			mEventId = kind;
			if (mStatus.mFlags & 0x00000C0000000000ULL)
				TriggerEvent(this, mEventId, mEventArg);
		} else
			return FALSE;
	} else if (mStatus.mFlags & 0x0000040000000000ULL) {
		TriggerEvent(this, (s16)kind, -1);
	} else
		return FALSE;
	return TRUE;
}

// XObjDust::OnHitFromPosition
/**
 * @note Address: 0x800DE424
 * @note Size: 0x1E4
 */
void XObjDust::OnHitFromPosition(const Vec& position, f32 strength)
{
	if (GetObjFlags() & 1) {
		mStatus.mFlags |= 0x4000000000000000ULL;
		if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f)
			mEventArg = 2001;
		else
			mEventArg = 2000;
	} else if (mStatus.mFlags & 0x0000080000000000ULL) {
		if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f)
			TriggerEvent(this, -1, 2001);
		else
			TriggerEvent(this, -1, 2000);
	}
}

#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/xobjfree.h"
#include "game/wii/cgame.h"
#include "game/evt.h"
#include "game/globals.h"

// the dust pile's two item/hit reactions and the plain object's state half. The
// GameCube's xobjfreeevent.cpp carries the same functions; the Wii keeps the code
// against its own signatures (six-argument SetAnim, GetEventScript vtable read)

extern "C" BOOL IsPlayerBlockingEvents(const WiiPlayerState* player);

static inline void TriggerEvent(XObj* object, s16 first, s16 second)
{
	const void* script = object->GetEventScript();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer())) {
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, object, first, second);
	}
}

/**
 * @note Address: 0x80118360
 * @note Size: 0x1BC
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

// NONMATCHING: regalloc, the event script, id and arg land in r31/r27/r28 in retail, r27/r28/r31 here
/**
 * @note Address: 0x8011851C
 * @note Size: 0x264
 */
BOOL XObjDust::OnItemUsed(const Vec* position, s32 kind)
{
	if ((GetObjFlags() & 2) && kind == 16) {
		if (GetObjFlags() & 0x10) {
			mAnimId2++;
			SetAnim(mAnimId2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			if (mAnimId2 >= mAnimCount - 1) {
				mStatus.mFlags |= 0x4000000000000000ULL;
				mEventId = kind;
			}
		} else
			return FALSE;
	} else if ((GetObjFlags() & 2) && kind == 56) {
		if (GetObjFlags() & 0x20) {
			mFrom.x = mMoveTranslate.x;
			mFrom.y = mMoveTranslate.y;
			mFrom.z = mMoveTranslate.z;
			mTo.x   = position->x;
			mTo.y   = position->y;
			mTo.z   = position->z;
			mStatus.mFlags |= 0x4000000000000000ULL;
			mEventId = kind;
			if (mStatus.mFlags & 0x00000C0000000000ULL)
				TriggerEvent(this, mEventId, mEventArg);
		} else
			return FALSE;
	} else if (mStatus.mFlags & 0x0000040000000000ULL) {
		TriggerEvent(this, kind, -1);
	} else
		return FALSE;
	return TRUE;
}

// NONMATCHING: retail loads _168 once into r3 for the test; ours loads it into r0 and again into r3
/**
 * @note Address: 0x80118780
 * @note Size: 0xD4
 */
void XObjFree::ResetObjectState()
{
	XObj::ResetObjectState();
	if (mAttr.mObjData != NULL) {
		mLife = (s16)GetObjFlags();
	}
	mMatAnim = NULL;
	if (HasModel() && (mStatus.mFlags & 0x1000ULL)) {
		SetAnim(mCoord.mPlacement->mStartAnim, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
	}
}

/**
 * @note Address: 0x80118854
 * @note Size: 0xC
 */
void XObjFree::Release()
{
	mMatAnim = NULL;
	XObj::Release();
}

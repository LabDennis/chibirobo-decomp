#include "game/xobjleticker.h"
#include "game/cgame.h"
#include "game/cpad.h"
#include "game/animframe.h"
#include <math.h>
extern "C" {
#include <dolphin/mtx/mtx.h>
}
#include <sysdolphin/controller.h>

extern "C" {

// turns the first linked object to the angle, then puts mStackTopPosition at the top of its stack
/**
 * @note Address: 0x800E260C
 * @note Size: 0x3AC
 */
// NONMATCHING: regalloc
BOOL TurnLetickerLinkedPartAndTrackJoint(XObjLeticker* leticker, f32 angle)
{
	XObj* part = gGame.GetObject(leticker->mLinked[0]);
	f32 my     = HSD_JObjGetRotationY(leticker->mJObj);
	f32 c      = cos(HSD_JObjGetRotationY(part->mJObj) - (angle + my));
	if (c < (f32)cos(0.08726646f)) {
		leticker->mLevelProgress = 0.0f;
		leticker->_A30           = 0;
		if (part->mAnimId == 0) {
			part->SetRotateY(angle + HSD_JObjGetRotationY(leticker->mJObj), 0.1f);
			leticker->mHeading = angle;
		}
	}
	if (GetAnimFrame(part->mJObj) < GetAnimEndFrame(part->mJObj)) {
		return TRUE;
	}
	s32 path[9]    = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	HSD_JObj* jobj = FindJointByPath(part->mJObj, path, leticker->mCurrentLevel + 1);
	if (jobj != NULL) {
		MtxPtr mtx = HSD_JObjGetMtxPtr(jobj);
		Vec pos;
		pos.x = pos.y = pos.z = 0.0f;
		PSMTXMultVec(mtx, &pos, &leticker->mStackTopPosition);
	}
	return TRUE;
}

/**
 * @note Address: 0x800E29B8
 * @note Size: 0x4F4
 */
BOOL TurnLetickerAndCheckClearance(XObjLeticker* lift, f32 angle)
{
	CPad pad(0);
	XObj* first    = gGame.GetObject(lift->mLinked[0]);
	XObj* second   = gGame.GetObject(lift->mLinked[1]);
	lift->mHeading = angle;
	first->SetRotateY(angle + HSD_JObjGetRotationY(lift->mJObj), 0.1f);
	second->SetRotateY(angle + HSD_JObjGetRotationY(lift->mJObj), 0.1f);
	if (pad.mPad->mButton & HSD_PAD_STICK_DOWN)
		return TRUE;

	CVec pos(second->mMoveTranslate);
	f32 offset = 4.0f * (s32)(lift->mLevelProgress / 3.0f) + 0.5f;
	pos.y      = 6.0f + (3.0f + (9.0f + (offset + HSD_JObjGetTranslationY(lift->mJObj))));
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* target = gGame.GetObject(i);
		if (!(target->mStatus.mFlags & 1) || !(target->mStatus.mFlags & 0x10000))
			continue;
		if (target->mId == lift->mId || target->mId == first->mId || target->mId == second->mId)
			continue;
		u32 attr = target->mAttr._00 != NULL ? target->mAttr._00->_00 : 0;
		if (attr == 0)
			continue;
		f32 dx = target->mMoveTranslate.x - pos.x;
		f32 dz = target->mMoveTranslate.z - pos.z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (5.0f + target->mFloorXZExtent))
			continue;
		CVec hit, normal;
		u32 flags = target->vfFC(&pos, &pos, 5.5f, 6.0f, &hit, &normal, 0, 0);
		if (flags & 2)
			return FALSE;
		if (flags & 1) {
			f32 floor = target->mFloorHeights[target->mHitFloorIndex];
			if (__fpclassifyf(floor) == FP_NAN)
				return FALSE;
			if (pos.y <= floor)
				return FALSE;
		}
	}
	return TRUE;
}

} // extern "C"

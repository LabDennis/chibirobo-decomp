#include "game/xobjattachment.h"
#include "game/cgame.h"
#include "game/csound.h"
#include "game/cjobjinlineattach.h"
#include "game/evt.h"
#include <math.h>

// XObjAttachment, the things chibi robo carries, and the four that have their own code:
// XObjOX, XObjRadar, XObjPopper and XObjCopter

extern "C" {

/**
 * @note Address: 0x800D5C58
 * @note Size: 0x33C
 */
// turns the dish by angle from where the radar points
void SetRadarDishYaw(XObjRadar* object, f32 angle)
{
	s32 path[] = { 0 };
	CJObj joint(FindJointByPath(object->mJObj, path, 1));
	joint.SetRotationY(object->mYaw + angle, FALSE);
}

/**
 * @note Address: 0x800D6418
 * @note Size: 0x6A8
 */
void ActivatePlayerRadar(XObjRadar* radar)
{
	radar->Show();
	radar->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	radar->mState = 1;
	CJObj joint;
	s32 yawPath[] = { 0 };
	joint.Attach(FindJointByPath(radar->mJObj, yawPath, 1));
	joint.SetRotationY(radar->mYaw, FALSE);
	s32 pitchPath[] = { 0, 0, 0, 0 };
	joint.Attach(FindJointByPath(radar->mJObj, pitchPath, 4));
	joint.SetRotationX(radar->mPitch, FALSE);
}

} // extern "C"

BOOL XObjRadar::LoadFromPlacement(const CoordInfo& info)
{
	BOOL loaded = XObj::LoadFromPlacement(info);
	if (loaded) {
		CJObj joint;
		if (__fpclassifyf(mYaw) == FP_NAN) {
			s32 path[] = { 0 };
			joint.Attach(FindJointByPath(mJObj, path, 1));
			mYaw = HSD_JObjGetRotationY(joint.mJObj);
		}
		if (__fpclassifyf(mPitch) == FP_NAN) {
			s32 path[] = { 0, 0, 0, 0 };
			joint.Attach(FindJointByPath(mJObj, path, 4));
			mPitch = HSD_JObjGetRotationX(joint.mJObj);
		}
	}
	return loaded;
}

/**
 * @note Address: 0x800D55BC
 * @note Size: 0x248
 */
void XObjOX::UpdateObject(u32 frames)
{
	if (mVisible) {
		CopyParentMtx(this);
		HSD_AObj* aobj = GetAObj(mJObj);
		s32 anim       = mAnimId;
		f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		switch (anim) {
		case 0:
			if (aobj->flags & 0x44000000) {
				Hide();
			}
			break;
		case 1:
			if (aobj->flags & 0x44000000) {
				Hide();
			}
			break;
		}
		XObj::UpdateObject(frames);
	}
}

/**
 * @note Address: 0x800D5804
 * @note Size: 0x70
 */
extern "C" void ShowOXCross(XObjOX* object)
{
	object->Show();
	object->SetAnim(1, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D5874
 * @note Size: 0x70
 */
extern "C" void ShowOXCircle(XObjOX* object)
{
	object->Show();
	object->SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

// tilts the dish
/**
 * @note Address: 0x800D58E4
 * @note Size: 0x374
 */
// NONMATCHING: rodata order
extern "C" void SetRadarDishPitch(XObjRadar* object, f32 angle)
{
	s32 path[4] = { 0, 0, 0, 0 };
	CJObj joint(FindJointByPath(object->mJObj, path, 4));
	joint.SetRotationX(object->mPitch + angle, 0);
}

/**
 * @note Address: 0x800D5F94
 * @note Size: 0x5C
 */
extern "C" CVec GetRadarDishWorldPosition(XObjRadar* object)
{
	s32 path[4] = { 0, 0, 0, 0 };
	return CJObj_GetJointWorldPositionByPath(object, 4, path);
}

/**
 * @note Address: 0x800D5FF0
 * @note Size: 0x27C
 */
void XObjRadar::UpdateObject(u32 frames)
{
	if (mVisible) {
		CopyParentMtx(this);
		HSD_AObj* aobj = GetAObj(mJObj);
		switch (mState) {
		case 1:
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				mState = 10;
			}
			break;
		case 2:
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				Hide();
				mState = 0;
			}
			break;
		case 10:
			break;
		}
		XObj::UpdateObject(frames);
	}
}

/**
 * @note Address: 0x800D626C
 * @note Size: 0xD4
 */
extern "C" void ShowIdlePlayerRadar(XObjRadar* object)
{
	object->mState = 10;
	object->SetAnim(2, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	object->UpdateAnim(0);
	object->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	if (!object->mVisible) {
		object->Show();
	}
}

/**
 * @note Address: 0x800D6340
 * @note Size: 0x4
 */
extern "C" void StartPlayerRadarScan(XObjRadar* object)
{
}

/**
 * @note Address: 0x800D6344
 * @note Size: 0xD4
 */
extern "C" void DeactivatePlayerRadar(XObjRadar* object)
{
	object->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	object->mState = 2;
	gGame.mSound.StopIfPlaying(0x18C);
	gGame.mSound.StopIfPlaying(0x18D);
	gGame.mSound.StopIfPlaying(0x18E);
}

/**
 * @note Address: 0x800D552C
 * @note Size: 0x54
 */
void XObjAttachment::Show()
{
	mVisible = TRUE;
	if (mStatus.mFlags & 1) {
		HSD_JObjClearFlagsAll(mJObj, 0x10);
	}
}

/**
 * @note Address: 0x800D5580
 * @note Size: 0x3C
 */
void XObjAttachment::vfAC()
{
	if (mVisible) {
		DrawWithView(NULL);
	}
}

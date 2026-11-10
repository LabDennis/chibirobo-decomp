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

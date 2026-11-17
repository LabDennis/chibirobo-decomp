#include "game/xobjattachment.h"
#include "game/cgame.h"
#include "game/csound.h"
#include "game/cjobjinlineattach.h"
#include "game/evt.h"
#include <math.h>

// XObjAttachment, the things chibi robo carries, and the four that have their own code:
// XObjOX, XObjRadar, XObjPopper and XObjCopter

/**
 * @note Address: 0x800D552C
 * @note Size: 0x54
 */
void XObjAttachment::Show()
{
	mVisible = TRUE;
	if (mStatus.mFlags & 1) {
		HSD_JObjClearFlagsAll(mJObj, HIDDEN);
	}
}

/**
 * @note Address: 0x800D5580
 * @note Size: 0x3C
 */
void XObjAttachment::DrawObject()
{
	if (mVisible) {
		DrawWithView(NULL);
	}
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
		case 0: // the mark plays once, then hides
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
extern "C" void SetRadarDishPitch(XObjRadar* object, f32 angle)
{
	s32 path[4] = { 0, 0, 0, 0 };
	CJObj joint(FindJointByPath(object->mJObj, path, 4));
	joint.SetRotationX(object->mPitch + angle, 0);
}

/**
 * @note Address: 0x800D5C58
 * @note Size: 0x33C
 */
// turns the dish by angle from where the radar points
extern "C" void SetRadarDishYaw(XObjRadar* object, f32 angle)
{
	s32 path[] = { 0 };
	CJObj joint(FindJointByPath(object->mJObj, path, 1));
	joint.SetRotationY(object->mYaw + angle, FALSE);
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
		case 1: // deployed: settle to idle
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				mState = 10;
			}
			break;
		case 2: // stowed: hide
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				Hide();
				mState = 0;
			}
			break;
		case 10: // idle
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
 * @note Address: 0x800D6418
 * @note Size: 0x6A8
 */
extern "C" void ActivatePlayerRadar(XObjRadar* radar)
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

BOOL XObjRadar::LoadFromPlacement(const CoordInfo& info)
{
	BOOL loaded = XObj::LoadFromPlacement(info);
	if (loaded) {
		CJObj joint;
		if (__fpclassifyf(mYaw) == FP_NAN) { // unset: the placement's joint supplies it
			s32 path[] = { 0 };
			joint.Attach(FindJointByPath(mJObj, path, 1));
			mYaw = joint.GetRotationY();
		}
		if (__fpclassifyf(mPitch) == FP_NAN) { // same for the pitch
			s32 path[] = { 0, 0, 0, 0 };
			joint.Attach(FindJointByPath(mJObj, path, 4));
			mPitch = joint.GetRotationX();
		}
	}
	return loaded;
}

/**
 * @note Address: 0x800D6F98
 * @note Size: 0x50
 */
void XObjRadar::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	ResetForNewGame();
	mYaw = mPitch = NAN;
}

extern "C" {

/**
 * @note Address: 0x800D6FE8
 * @note Size: 0x398
 */
void XObjPopper::UpdateObject(u32 frames)
{
	if (mVisible) {
		CopyParentMtx(this);
		HSD_AObj* aobj = GetAObj(mJObj);
		s32 anim       = mAnimId;
		f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f; // fetched but never read
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		switch (anim) {
		case 1: // deploy done: idle
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 2: // stowed: idle and hidden
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				Hide();
			}
			break;
		case 3: // raised: hold
			if (aobj->flags & 0x44000000) {
				SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 4:
			break;
		case 5: // fired: hold
			if (aobj->flags & 0x44000000) {
				SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 6: // lowered: idle
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		}
		UpdateObject__4XObjFUl(this, frames);
	}
}

/**
 * @note Address: 0x800D7380
 * @note Size: 0xCC
 */
void ShowIdlePlayerPopper(XObjPopper* object)
{
	object->SetAnim(2, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	object->UpdateAnim(0);
	object->SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	if (!object->mVisible) {
		object->Show();
	}
}

/**
 * @note Address: 0x800D744C
 * @note Size: 0x50
 */
void PlayPopperLowerAnimation(XObjPopper* object)
{
	object->SetAnim(6, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D749C
 * @note Size: 0x14
 */
BOOL IsPopperFireAnimationSelected(XObjPopper* object)
{
	return object->mAnimId == 5;
}

/**
 * @note Address: 0x800D74B0
 * @note Size: 0x50
 */
void PlayPopperFireAnimation(XObjPopper* object)
{
	object->SetAnim(5, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7500
 * @note Size: 0x50
 */
void PlayPopperRaiseAnimation(XObjPopper* object)
{
	object->SetAnim(3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7550
 * @note Size: 0xA8
 */
void PlayPopperStowAnimation(XObjPopper* object)
{
	object->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	gGame.mSound.StopIfPlaying(0x186);
	gGame.mSound.StopIfPlaying(0x18B);
}

/**
 * @note Address: 0x800D75F8
 * @note Size: 0x70
 */
void ShowAndDeployPopper(XObjPopper* object)
{
	object->Show();
	object->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7668
 * @note Size: 0x344
 */
void XObjCopter::UpdateObject(u32 frames)
{
	if (mVisible) {
		CopyParentMtx(this);
		HSD_AObj* aobj = GetAObj(mJObj);
		s32 anim       = mAnimId;
		f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f; // fetched but never read
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		switch (anim) {
		case 5: // deploy done: idle
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 8: // stowed: idle and hidden
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				Hide();
			}
			break;
		case 6: // anim 6 leads into flight
			if (aobj->flags & 0x44000000) {
				SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 7: // landed: idle
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		}
		UpdateObject__4XObjFUl(this, frames);
	}
}

/**
 * @note Address: 0x800D79AC
 * @note Size: 0xCC
 */
void ShowIdlePlayerCopter(XObjCopter* object)
{
	object->SetAnim(8, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	object->UpdateAnim(0);
	object->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	if (!object->mVisible) {
		object->Show();
	}
}

/**
 * @note Address: 0x800D7A78
 * @note Size: 0x50
 */
void PlayCopterLandingAnimation(XObjCopter* object)
{
	object->SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7AC8
 * @note Size: 0x50
 */
void PlayCopterDescentAnimation(XObjCopter* object)
{
	object->SetAnim(3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7B18
 * @note Size: 0x50
 */
void PlayCopterFlightAnimation(XObjCopter* object)
{
	object->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7B68
 * @note Size: 0x88
 */
void PlayCopterStowAnimation(XObjCopter* object)
{
	object->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	gGame.mSound.StopIfPlaying(8);
}

/**
 * @note Address: 0x800D7BF0
 * @note Size: 0x70
 */
void ShowAndDeployCopter(XObjCopter* object)
{
	object->Show();
	object->SetAnim(5, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7C60
 * @note Size: 0x40
 */
void XObjAttachment::Release()
{
	Hide();
	XObj::Release();
}

/**
 * @note Address: 0x800D7CA0
 * @note Size: 0x20
 */
void XObjAttachment::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
}

/**
 * @note Address: 0x800D7CC0
 * @note Size: 0x8
 */
void XObjAttachment::SetParent(HSD_JObj* parent, BOOL fromPlacement)
{
	mParent = parent; // fromPlacement goes unused
}

/**
 * @note Address: 0x800D7CC8
 * @note Size: 0x34
 */
void XObjAttachment::ResetForNewGame()
{
	mParent = NULL;
	Hide();
}

/**
 * @note Address: 0x800D7CFC
 * @note Size: 0x40
 */
void XObjAttachment::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	ResetForNewGame();
}

} // extern "C"

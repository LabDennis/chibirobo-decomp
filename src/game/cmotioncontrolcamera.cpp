
#include "game/cmotioncontrolcamera.h"
#include "game/cobj.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/xpad.h"
#include "Version.h"
#include <math.h>
#include <string.h>

extern "C" {
#include <dolphin/os.h>
#include "game/csound.h"
#include "game/xobjchr.h"
#include "game/xaction.h"
#include "game/cscrn.h"
}

extern "C" {
void InitMotionControlCameraDefaults(CCamera* camera);
}

extern "C" {
f32 GetAreaCameraFovRatio(CMotionControlCamera* camera);
}

// the last two map cameras, Update copies the map action here every frame it runs
CCameraActionMap gMapCameraActions[2];

inline CInterfaceCameraAction* CMotionControlCamera::GetAction(s32 type)
{
	return gGame.mCameraActions.mTable[type];
}

static inline bool PlayerCanLook(const XObjChr& player)
{
	const s32 liftState = player.mLift.mState;
	return (liftState < 7 || liftState > 18) && player.mHang.mState == 0 && player.mJump.mState == 0 && player.mLadder.mState == 0
	    && player.mRope.mState == 0 && player.mPush.mState == 0 && player.mTumble.mState == 0;
}

/**
 * @note Address: 0x8014AA4C
 * @note Size: 0x20
 */
void CMotionControlCamera::InitCamera()
{
	InitMotionControlCameraDefaults(&mCamera);
}

/**
 * @note Address: 0x8014A964
 * @note Size: 0xE8
 */
void CMotionControlCamera::Init()
{
	InitMotionControlCameraDefaults(&mCamera);
	SetEye(mCamera.mEye);
	SetInterest(mCamera.mInterest);
	SetAngle(mCamera.mAngle);
	mMode         = 0;
	mAction       = NULL;
	mPrevAction   = NULL;
	mPushedAction = NULL;
	_1E0          = -1;
	_1DC          = -1;
	mFlags.mFlags = 0;
	mInterestMode = 0;
	mEyeMode      = 0;
	mFollowDivide = 12.0f;
	((CCameraActionScroll2*)GetAction(CAMACTION_SCROLL2))->ClearHideList();
	((CCameraActionScroll2*)GetAction(CAMACTION_SCROLL2))->SaveCamInfo(NULL);
	gGame.mStatus.mFlags |= 0x1000000000000ULL; // lets Update open the room camera
}

/**
 * @note Address: 0x8014A940
 * @note Size: 0x24
 */
void CMotionControlCamera::RestoreAction()
{
	ChangeAction(mPrevAction);
}

/**
 * @note Address: 0x80149E24
 * @note Size: 0xB1C
 */
void CMotionControlCamera::Update(u32 step)
{
	if (gGame.mStatus.mFlags & 0x20) {
		return;
	}

	CPad pad(0);
	if (gGame.mEvt.mData.mScript == NULL && (gGame.mStatus.mFlags & 0x1000000000000ULL)) {
		CCameraActionScroll2* room = (CCameraActionScroll2*)GetAction(CAMACTION_SCROLL2);
		if (room->GetLimits(gGame.mStage) != NULL && !(gGame.mStatus.mFlags & 0xC00)) {
			// R opens the room camera over the area, scroll and map cameras, R or B closes it again
			if (mAction == GetAction(CAMACTION_AREA) || mAction == GetAction(CAMACTION_SCROLL) || mAction == GetAction(CAMACTION_MAP)) {
				if ((pad.mPad->mTrigger & PAD_TRIGGER_R) && !(pad.mPad->mButton & PAD_BUTTON_B)) {
					PushAction(GetAction(CAMACTION_SCROLL2));
					((CCameraActionScroll2*)GetAction(CAMACTION_SCROLL2))->HideObjs();
					gGame.mStatus.mFlags |= 2; // gGame.IsRunning() is false while the room camera is up
				}
			} else if (mAction == GetAction(CAMACTION_SCROLL2)) {
				if ((pad.mPad->mTrigger & PAD_TRIGGER_R) || (pad.mPad->mTrigger & PAD_BUTTON_B)) {
					PopAction();
					((CCameraActionScroll2*)GetAction(CAMACTION_SCROLL2))->ShowObjs();
					gGame.mStatus.mFlags &= ~2ULL;
					// read the player's pad again so the press that closed it doesn't reach the player too
					gGame.GetPlayer()->mPad.Update(0);
				}
			}
		}
	}

	if ((gGame.mStatus.mFlags & 0x80000000) && (pad.mPad->mTrigger & PAD_TRIGGER_L) && (pad.mPad->mButton & PAD_BUTTON_B)) {
		if (mAction != GetAction(CAMACTION_CHASE)) {
			ChangeAction(CAMACTION_CHASE);
		} else {
			gGame.mMotionControlCamera.RestoreAction();
		}
	}

	BOOL look = FALSE;
	if (((pad.mPad->mTrigger & PAD_BUTTON_Y) || (pad.mPad->mTrigger & PAD_BUTTON_B)) && gGame.mEvt.mData.mScript == NULL) {
		XObjChr* player = gGame.GetPlayer();
		if (gGame.IsRunning() && (player->mPlayerStatus.mFlags & XObjChr::PlayerControlled) && !(gGame.mStatus.mFlags & 0x80)) {
			if (!(gGame.mStatus.mFlags & 0xC00) && player->mAttachmentRequest < 0 && player->mSuits.mChangeStep == 0
			    && (player->mLift.mState == 0 || !IsCarryingMug(&player->mLift))) {
				// Y toggles the eye camera, B only takes it down
				if (pad.mPad->mTrigger & PAD_BUTTON_B) {
					if (gGame.mStatus.mFlags & 0x100) {
						look = TRUE;
					}
				} else {
					look = TRUE;
				}
			} else if (pad.mPad->mTrigger & PAD_BUTTON_Y) {
				// 0x85 is the buzz the menus play for a choice that isn't available
				PlayGameSound(&gGame.mSound, 0x85, 0xFF);
			}
		}
	}

	if (look) {
		if (!(gGame.mStatus.mFlags & 0x100)) {
			if (PlayerCanLook(*gGame.GetPlayer())) {
				StartEye();
			} else {
				PlayGameSound(&gGame.mSound, 0x85, 0xFF);
			}
		} else {
			EndEye();
		}
		XObjChr* player = gGame.GetPlayer();
		if (player->mHang.mState != 5 && player->mRope.mState != 5 && player->mLadder.mState != 5 && player->mPush.mState != 2) {
			player->mHeldMoveAngle = player->mMoveAngle;
		}
	}

	if (mAction != NULL) {
		mAction->PreviousUpdate();
		mAction->Update(step);
		if (mAction == GetAction(CAMACTION_MAP)) {
			memmove(&gMapCameraActions[1], &gMapCameraActions[0], sizeof(CCameraActionMap));
			memmove(&gMapCameraActions[0], mAction, sizeof(CCameraActionMap));
			if (fpclassify(mAction->mEye.x) == FP_NAN || fpclassify(mAction->mEye.y) == FP_NAN || fpclassify(mAction->mEye.z) == FP_NAN) {
				OSPanic(__FILE__, 249, "%s%s", FormatToStaticBuffer("CMotionControlCamera ... \n"), __FUNCTION__);
			}
		}
		ReadAction();
		if (mFlags.mFlags & CAMFLAG_EYE) {
			SetEye(mAction->mEye);
		}
		if (mFlags.mFlags & CAMFLAG_INTEREST) {
			SetInterest(mAction->mInterest);
		}
		if (mFlags.mFlags & CAMFLAG_DISTANCE) {
			mDistance = mAction->mDistance;
		}
		if (mFlags.mFlags & CAMFLAG_FOV) {
			mFov = mAction->mFov;
		}
		if (mFlags.mFlags & CAMFLAG_ANGLE) {
			SetAngle(mAction->mAngle);
		}
	}

	Interpolate(step);

	// work out whatever the action didn't drive from what it did
	u32 flags = mFlags.mFlags;
	if ((flags & CAMFLAG_EYE) && (flags & CAMFLAG_INTEREST)) {
		mCamera.CalcAngle();
		mCamera.CalcDistance();
		mAction->SetAngle(mCamera.mAngle);
		mAction->SetDistance(mCamera.mDistance);
	} else if ((flags & 0x20000) && (flags & CAMFLAG_INTEREST)) {
		mCamera.CalcEye();
		mAction->SetEye(mCamera.mEye);
	} else if ((flags & 0x20000) && (flags & CAMFLAG_EYE)) {
		mCamera.CalcInterest();
	} else if ((flags & 0x10000) && (flags & CAMFLAG_EYE)) {
		mCamera.CalcInterest();
	} else if ((flags & 0x10000) && (flags & CAMFLAG_INTEREST)) {
		mCamera.CalcEye();
	} else if ((flags & 0x80000) && (flags & CAMFLAG_DISTANCE)) {
		mCamera.CalcEye();
		mAction->SetEye(mCamera.mEye);
	} else if ((flags & CAMFLAG_EYE) || (flags & CAMFLAG_INTEREST)) {
		mCamera.CalcAngle();
		mCamera.CalcDistance();
	}
	// 0x40000 comes with CAMFLAG_ANGLE (anim, evt, reset), CalcUp rolls the up vector by the angle's z
	if (mFlags.mFlags & 0x40000) {
		mCamera.CalcUp();
	}
}
// fn_80149E24
/**
 * @note Address: 0x8014978C
 * @note Size: 0x698
 */
void CMotionControlCamera::Interpolate(u32 step)
{
	switch (mMode) {
	case 0:
		if (mFlags.mFlags & CAMFLAG_DISTANCE) {
			mCamera.SetDistance(mDistance);
		}
		if (mFlags.mFlags & CAMFLAG_FOV) {
			mCamera.SetFov(mFov);
		}
		if (mFlags.mFlags & CAMFLAG_ANGLE) {
			mCamera.SetAngle(mAngle);
		}
		break;
	case 1: {
		CObjVec vec;
		if (mFlags.mFlags & CAMFLAG_EYE) {
			vec.Reset(mCamera.mEye);
			vec.SetTarget(mEye);
			vec.mSpeed = CVec(1.0f, 1.0f, 1.0f);
			vec.Step(step);
			mCamera.SetEye(vec);
		}
		if (mFlags.mFlags & CAMFLAG_INTEREST) {
			vec.Reset(mCamera.mInterest);
			vec.SetTarget(mInterest);
			vec.mSpeed = CVec(1.0f, 1.0f, 1.0f);
			vec.Step(step);
			mCamera.SetInterest(vec);
		}
		break;
	}
	}

	if (mFlags.mFlags & CAMFLAG_INTEREST) {
		switch (mInterestMode) {
		case 0:
			mCamera.SetInterest(mInterest);
			break;
		case 2:
			Follow(mInterest, mCamera.mInterest);
			mCamera.SetInterest(mFollowValue);
			break;
		case 3: {
			// take the target's height at once, ease in only across x and z
			Vec interest = mCamera.mInterest;
			interest.y   = mInterest.y;
			mCamera.SetInterest(interest);
			Follow(mInterest, mCamera.mInterest);
			mCamera.SetInterest(mFollowValue);
			break;
		}
		}
	}

	if (mFlags.mFlags & CAMFLAG_EYE) {
		// Init and ReadAction only ever set mEyeMode to 0, so only case 0 runs
		switch (mEyeMode) {
		case 0:
			mCamera.SetEye(mEye);
			break;
		case 1:
			break;
		case 2:
		case 3:
			Follow(mEye, mCamera.mEye);
			mCamera.SetEye(mFollowValue);
			break;
		}
	}
}
// fn_8014978C
/**
 * @note Address: 0x80149754
 * @note Size: 0x38
 */
void CMotionControlCamera::Draw()
{
	if (mAction != NULL) {
		mAction->Draw();
	}
}

/**
 * @note Address: 0x801496A0
 * @note Size: 0xB4
 */
// NONMATCHING: regalloc
s32 CMotionControlCamera::GetActionType()
{
	CInterfaceCameraAction* action = mAction;
	for (s32 i = 0; i < CAMACTION_NUM; i++) {
		if (action == GetAction(i)) {
			return i;
		}
	}
	return CAMACTION_NUM;
}
// fn_801496A0
/**
 * @note Address: 0x80149674
 * @note Size: 0x2C
 */
bool CMotionControlCamera::IsAction(s32 type)
{
	return mAction == GetAction(type);
}
// fn_80149674
/**
 * @note Address: 0x801494A4
 * @note Size: 0x1D0
 */
void CMotionControlCamera::ChangeAction(CInterfaceCameraAction* action)
{
	if (mAction != action) {
		mPrevAction = mAction;
		mAction     = action;
		ReadAction();
		StartAction();
	}
}

/**
 * @note Address: 0x8014946C
 * @note Size: 0x38
 */
void CMotionControlCamera::ChangeAction(s32 type)
{
	ChangeAction(GetAction(type));
}

/**
 * @note Address: 0x80148B78
 * @note Size: 0xF4
 */
void CMotionControlCamera::EndEye()
{
	if (gGame.mStatus.mFlags & 0x100) {
		gGame.mStatus.mFlags &= ~0x300ULL; // and 0x200, the eye action's c stick look
		PopAction();
		SetPlayerEyeCameraJointsHidden(gGame.GetPlayer(), 0);
		// the hole grows back to 550x500 as the white fades out, then the wipe hides
		StartScreenWipeColorTransition(&gGame.mRoboeyes, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 320.0f, 224.0f, 430.0f, 400.0f, 550.0f,
		                               500.0f, 0.2f, 0, 1);
		PlayGameSound(&gGame.mSound, 2, 0xFF);
	}
}

/**
 * @note Address: 0x80148C6C
 * @note Size: 0x100
 */
void CMotionControlCamera::StartEye()
{
	if (!(gGame.mStatus.mFlags & 0x100)) {
		gGame.mStatus.mFlags |= 0x100;
		PushAction(GetAction(CAMACTION_EYE));
		StopPlayerMotion(gGame.GetPlayer());
		SetPlayerEyeCameraJointsHidden(gGame.GetPlayer(), 1);
		StartScreenWipeColorTransition(&gGame.mRoboeyes, 0xFF, 0xFF, 0xFF, 0, 0xFF, 0xFF, 0xFF, 320.0f, 224.0f, 550.0f, 500.0f, 430.0f,
		                               400.0f, 0.15f, 0xFF, 0);
		PlayGameSound(&gGame.mSound, 4, 0xFF);
	}
}

/**
 * @note Address: 0x80148D6C
 * @note Size: 0xC4
 */
void CMotionControlCamera::UpdateTarget(XObj* obj)
{
	if (mAction != NULL && obj == mAction->GetTarget() && (mFlags.mFlags & CAMFLAG_INTEREST)) {
		mCamera.SetInterest(obj->GetCameraTarget());
		mAction->SetInterest(obj->GetCameraTarget());
	}
}

/**
 * @note Address: 0x80148E30
 * @note Size: 0xF0
 */
void CMotionControlCamera::ApplyAction()
{
	CopyAction();
}

/**
 * @note Address: 0x80148F20
 * @note Size: 0x2B0
 */
void CMotionControlCamera::PopAction()
{
	if (mPushedAction != NULL) {
		mAction       = mPushedAction;
		mPushedAction = NULL;
		ReadAction();
		BOOL scroll = IsCurrent(CAMACTION_SCROLL);
		if (scroll) {
			StartAction();
		}
		CopyAction();
	}
}

/**
 * @note Address: 0x801491D0
 * @note Size: 0x29C
 */
void CMotionControlCamera::PushAction(CInterfaceCameraAction* action)
{
	if (mAction != action) {
		mPushedAction = mAction;
		mAction       = action;
		ReadAction();
		StartAction();
		CopyAction();
	}
}

/**
 * @note Address: 0x80148B00
 * @note Size: 0x78
 */
CCameraMemento* CMotionControlCamera::CreateMemento(CCameraMemento* memento)
{
	CCameraMemento* saved = (memento != NULL) ? memento : &mMemento;
	saved->SetEye(mCamera.mEye);
	saved->SetInterest(mCamera.mInterest);
	saved->SetAngle(mCamera.mAngle);
	saved->mDistance = mCamera.mDistance;
	saved->mFov      = mCamera.mFov;
	saved->mAction   = mAction;
	return saved;
}
// fn_80148B00
/**
 * @note Address: 0x801487CC
 * @note Size: 0x334
 */
BOOL CMotionControlCamera::SetMemento(const CCameraMemento* memento)
{
	if (memento == NULL) {
		return FALSE;
	}
	mCamera.SetEye(memento->mEye);
	mCamera.SetInterest(memento->mInterest);
	mCamera.SetAngle(memento->mAngle);
	mCamera.SetFov(memento->mFov);
	mCamera.SetDistance(memento->mDistance);
	if (memento->mAction != NULL) {
		mPrevAction = mAction;
		mAction     = memento->mAction;
		ReadAction();
		StartAction();
		CopyAction();
	}
	return TRUE;
}

/**
 * @note Address: 0x80148768
 * @note Size: 0x64
 */
extern "C" f32 GetAreaCameraFovRatio(CMotionControlCamera* camera)
{
	f32 baseFov = ((CCameraActionAreaExM*)CMotionControlCamera::GetAction(CAMACTION_AREA))->GetBaseFov();
	if (baseFov) {
		return camera->mCamera.mFov / baseFov;
	}
	return 1.0f;
}
// GetAreaCameraFovRatio
/**
 * @note Address: 0x8014872C
 * @note Size: 0x3C
 */
extern "C" void SetAreaCameraHeightIndex(CMotionControlCamera* camera, s32 mode)
{
	((CCameraActionAreaExM*)CMotionControlCamera::GetAction(CAMACTION_AREA))->SetZoomLevel(mode);
}

/**
 * @note Address: 0x801486D8
 * @note Size: 0x54
 */
f32 CMotionControlCamera::GetMapDistance()
{
	BOOL map = IsCurrent(CAMACTION_MAP);
	if (map) {
		return GetMapCameraHorizontalTargetDistance((CCameraActionMap*)GetAction(CAMACTION_MAP)) - 4.0f;
	}
	return 1.0f;
}
// fn_801486D8
/**
 * @note Address: 0x8014861C
 * @note Size: 0xBC
 */
void CCameraActions::Init()
{
	mTable[CAMACTION_DEFAULT] = &mDefault;
	mTable[CAMACTION_AREA]    = &mArea;
	mTable[CAMACTION_CHASE]   = &mChase;
	mTable[CAMACTION_EVT]     = &mEvt;
	mTable[CAMACTION_SCROLL2] = &mScroll2;
	mTable[CAMACTION_EYE]     = &mEye;
	mTable[CAMACTION_ANIM]    = &mAnim;
	mTable[CAMACTION_MAP]     = &mMap;
	mTable[CAMACTION_LADR]    = &mLadr;
	mTable[CAMACTION_RESET]   = &mReset;
	mTable[CAMACTION_10]      = NULL;
	mTable[CAMACTION_SCROLL]  = &mScroll;
	for (s32 i = 0; i < CAMACTION_NUM; i++) {
		if (mTable[i] != NULL) {
			mTable[i]->Init();
		}
	}
}

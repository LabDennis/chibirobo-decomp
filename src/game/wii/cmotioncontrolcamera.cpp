// Wii port of game/cmotioncontrolcamera.cpp. Same class and action table as the GameCube
// build: the camera is the base subobject here so every derived field sits 0x64 higher, the
// status flags are reached through the CBase vtable, and the remote's buttons come out of
// gWiiPointer's flag words instead of an HSD pad. The room camera and the pointer look are
// reworked for the remote.

#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/csound.h"
#include "Version.h"
#include <math.h>
#include <string.h>

extern "C" {
#include <dolphin/os.h>
#include "game/cscrn.h"
}

#define PLAYER gGame.GetPlayer()

extern "C" {
// defined in game/wii/init.cpp, the Wii port keeps the same signature
CCamera* InitMotionControlCameraDefaults(CCamera* camera);
void StopPlayerMotion(WiiPlayerState* player);
// takes the player's facing on the Wii build: (player, hidden, rotY, 0.0f)
void SetPlayerEyeCameraJointsHidden(WiiPlayerState* player, BOOL hidden, f32 rotY, f32 unused);
s32 PlayWiiSoundEffect(XSound* sound, s32 id, f32 volume, f32 pan);
// the lift object's class is not reconstructed on Wii, only its state word is mapped
BOOL IsCarryingMug(XAction* action);
// the player's pad re-read, PLAYER->mPad.Update(0) on the GameCube
void fn_800CE48C(WiiPlayerState* player);
}

extern "C" {
f32 GetAreaCameraFovRatio(CMotionControlCamera* camera);
}

extern s32 gPointerScreenWidth, gPointerScreenHeight;

// the Wii map action is not reconstructed yet; GetMapCameraHorizontalTargetDistance takes it
class CCameraActionMap;
extern "C" f32 GetMapCameraHorizontalTargetDistance(CCameraActionMap* map);

// the last two map cameras, Update copies the map action here every frame it runs.
// __sinit_cmotioncontrolcamera_cpp calls __construct_array(lbl_80483630, fn_8016AC28,
// __dt__16CCameraActionMapFv, 0x128C, 2), so retail holds a static CCameraActionMap[2]
// of the Wii 0x128C layout; the class is not reconstructed yet, so it stays storage
static u8 gMapCameraActions[2][0x128C];

// a camera action the stage keeps ready, owned by the map camera's unit; its concrete class is
// not reconstructed yet, so the extern names the storage and the uses cast it to the interface
extern u8 lbl_8045DD54[];

inline CInterfaceCameraAction* CMotionControlCamera::GetAction(s32 type)
{
	return gGame.mCameraActions.mTable[type];
}

// deferred inlining, so these go last to first

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline bool PlayerCanLook()
{
	bool result = false;
	bool busy   = false;
	if (!(PLAYER->mLift.mState != 7 && PLAYER->mLift.mState != 8 && PLAYER->mLift.mState != 9 && PLAYER->mLift.mState != 11
	      && PLAYER->mLift.mState != 12 && PLAYER->mLift.mState != 13 && PLAYER->mLift.mState != 14 && PLAYER->mLift.mState != 15
	      && PLAYER->mLift.mState != 16 && PLAYER->mLift.mState != 17 && PLAYER->mLift.mState != 18 && PLAYER->mLift.mState != 19)) {
		busy = true;
	}
	if (!busy && PLAYER->mHang.mState == 0 && PLAYER->mJump.mState == 0 && PLAYER->mLadder.mState == 0 && PLAYER->mRope.mState == 0
	    && PLAYER->mPush.mState == 0 && PLAYER->mTumble.mState == 0) {
		result = true;
	}
	return result;
}

/**
 * @note Address: 0x8016753C
 * @note Size: 0xC4
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

/**
 * @note Address: 0x80167600
 * @note Size: 0x38
 */
void CMotionControlCamera::InitCamera()
{
	InitMotionControlCameraDefaults(this);
	((CInterfaceCameraAction*)lbl_8045DD54)->Init();
}

/**
 * @note Address: 0x80167638
 * @note Size: 0xF0
 */
void CMotionControlCamera::Init()
{
	InitMotionControlCameraDefaults(this);
	mEye.Set(mView.mEye.x, mView.mEye.y, mView.mEye.z);
	mInterest.Set(mView.mInterest.x, mView.mInterest.y, mView.mInterest.z);
	mAngle.Set(mView.mAngle.x, mView.mAngle.y, mView.mAngle.z);
	mMode         = 0;
	mAction       = NULL;
	mPrevAction   = NULL;
	mPushedAction = NULL;
	_244          = -1;
	_240          = -1;
	mFlags        = 0;
	mInterestMode = 0;
	mEyeMode      = 0;
	mFollowDivide = 12.0f;
	((CCameraActionScroll2*)GetAction(CAMACTION_SCROLL2))->ClearHideList();
	((CCameraActionScroll2*)GetAction(CAMACTION_SCROLL2))->SaveCamInfo(NULL);
	gGame.SetStatusFlags(0x1000000000000ULL); // lets Update open the room camera
}

/**
 * @note Address: 0x80167728
 * @note Size: 0x64
 */
void CMotionControlCamera::fn_80167728()
{
	gGame.ClearStatusFlags(0x100);
	mAction       = NULL;
	mPrevAction   = NULL;
	mPushedAction = NULL;
	_244          = -1;
	_240          = -1;
}

// NONMATCHING: the GameCube Update this is ported from is 0xB1C; the Wii body is 0x16CC,
// the remote's button and pointer reads, the extra scroll push and the running test's added
// menu/reset checks are reconstructed but not yet byte-proven
/**
 * @note Address: 0x8016778C
 * @note Size: 0x16CC
 */
void CMotionControlCamera::Update(u32 step)
{
	if (gGame.GetStatusFlags(0x20)) {
		return;
	}

	if (gGame.mEvt.mData.mScript == NULL && gGame.GetStatusFlags(0x1000000000000ULL)) {
		CCameraActionScroll2* room = (CCameraActionScroll2*)GetAction(CAMACTION_SCROLL2);
		if (room->GetLimits(gGame.mStage) != NULL && !gGame.GetStatusFlags(0x600000000000C00ULL)) {
			// the remote's button opens the room camera over the area, scroll and map cameras
			if (IsCurrent(CAMACTION_AREA) || IsCurrent(CAMACTION_SCROLL) || IsCurrent(CAMACTION_MAP)) {
				if (gWiiPointer.mFlags & 0x200) {
					PushAction(room);
					room->HideObjs();
					gGame.SetStatusFlags(2); // gGame.IsRunning() is false while the room camera is up
				}
			} else if (IsCurrent(CAMACTION_SCROLL2)) {
				if (gWiiPointer.mFlags & 0x200) {
					CInterfaceCameraAction* scroll = GetAction(CAMACTION_SCROLL);
					if (scroll != NULL) {
						PushAction(scroll);
					}
				} else if (gWiiPointer.mFlags & 0x2) {
					PopAction();
					room->ShowObjs();
					gGame.ClearStatusFlags(2);
					// read the player's pad again so the press that closed it doesn't reach the player too
					fn_800CE48C(PLAYER);
				}
			}
		}
	}

	if (gGame.GetStatusFlags(0x80000000) && (gWiiPointer.mDirTrigger & 0x2000000000010000ULL)
	    && (gWiiPointer.mDirButton & 0x2000000000020ULL)) {
		if (!IsCurrent(CAMACTION_CHASE)) {
			ChangeAction(CAMACTION_CHASE);
		} else {
			ChangeAction(mPrevAction);
		}
	}

	BOOL look = FALSE;
	if (((gWiiPointer.mFlags & 0x10000) || (gWiiPointer.mFlags & 0x2)) && gGame.mEvt.mData.mScript == NULL) {
		// CBase::IsRunning's test, written out
		bool running = false;
		if (gGame.GetStatusFlags(1) && !gGame.GetStatusFlags(0x2000000000000022ULL)) {
			bool discError = (gGame.mDvdStatus != 0 || gGame.mPrevDvdStatus != 0) && gGame.mDiskOk == 0;
			if (!discError) {
				running = true;
			}
		}
		if (running && (PLAYER->mPlayerFlags & 0x20000000) && !gGame.GetStatusFlags(0x80)) {
			if (!gGame.GetStatusFlags(0xC00) && PLAYER->mAttachmentRequest < 0 && PLAYER->mSuits.mChangeStep == 0
			    && (PLAYER->mLift.mState == 0 || IsCarryingMug(&PLAYER->mLift) == 0)) {
				// the remote's look button toggles the eye camera, B only takes it down
				if (gWiiPointer.mFlags & 0x2) {
					if (gGame.GetStatusFlags(0x100)) {
						look = TRUE;
					}
				} else {
					look = TRUE;
				}
			} else if (gWiiPointer.mFlags & 0x10000) {
				// 0x85 is the buzz the menus play for a choice that isn't available
				PlayWiiSoundEffect(&gGame.mSound, 0x85, 1.0f, 0.0f);
			}
		}
	}

	if (look) {
		if (!gGame.GetStatusFlags(0x100)) {
			if (PlayerCanLook()) {
				StartEye();
			} else {
				PlayWiiSoundEffect(&gGame.mSound, 0x85, 1.0f, 0.0f);
			}
		} else {
			EndEye();
		}
		if (PLAYER->mHang.mState != 5 && PLAYER->mRope.mState != 5 && PLAYER->mLadder.mState != 5 && PLAYER->mPush.mState != 2) {
			PLAYER->mHeldMoveAngle = PLAYER->mMoveAngle;
		}
	}

	if (mAction != NULL) {
		mAction->PreviousUpdate();
		mAction->Update(step);
		if (IsCurrent(CAMACTION_MAP)) {
			memmove(&gMapCameraActions[1], &gMapCameraActions[0], sizeof(gMapCameraActions[0]));
			memmove(&gMapCameraActions[0], mAction, sizeof(gMapCameraActions[0]));
			if (fpclassify(mAction->mEye.x) == FP_NAN || fpclassify(mAction->mEye.y) == FP_NAN || fpclassify(mAction->mEye.z) == FP_NAN) {
				OSPanic(__FILE__, 249, "%s%s", FormatToStaticBuffer("CMotionControlCamera ... \n"), __FUNCTION__);
			}
		}
		ReadAction();
		// what the action drives goes into the targets; Interpolate works the view toward them
		if (mFlags & CAMFLAG_EYE) {
			mEye = mAction->mEye;
		}
		if (mFlags & CAMFLAG_INTEREST) {
			mInterest = mAction->mInterest;
		}
		if (mFlags & CAMFLAG_DISTANCE) {
			mDistance = mAction->mDistance;
		}
		if (mFlags & CAMFLAG_FOV) {
			mFov = mAction->mFov;
		}
		if (mFlags & CAMFLAG_ANGLE) {
			mAngle = mAction->mAngle;
		}
	}

	Interpolate(step);

	// work out whatever the action didn't drive from what it did
	u32 flags = mFlags;
	if ((flags & CAMFLAG_EYE) && (flags & CAMFLAG_INTEREST)) {
		CalcAngle();
		CalcDistance();
		mAction->SetAngle(mView.mAngle);
		mAction->SetDistance(mView.mDistance);
	} else if ((flags & 0x20000) && (flags & CAMFLAG_INTEREST)) {
		CalcEye();
		mAction->SetEye(mView.mEye);
	} else if ((flags & 0x20000) && (flags & CAMFLAG_EYE)) {
		CalcInterest();
	} else if ((flags & 0x10000) && (flags & CAMFLAG_EYE)) {
		CalcInterest();
	} else if ((flags & 0x10000) && (flags & CAMFLAG_INTEREST)) {
		CalcEye();
	} else if ((flags & 0x80000) && (flags & CAMFLAG_DISTANCE)) {
		CalcEye();
		mAction->SetEye(mView.mEye);
	} else if ((flags & CAMFLAG_EYE) || (flags & CAMFLAG_INTEREST)) {
		CalcAngle();
		CalcDistance();
	} else if ((flags & 0x10000) && (flags & CAMFLAG_INTEREST) && (flags & CAMFLAG_DISTANCE)) {
		// can't get here, the interest bit was already ruled out a branch ago
		CalcEye();
	}
	// 0x40000 comes with CAMFLAG_ANGLE (anim, evt, reset), CalcUp rolls the up vector by the angle's z
	if (mFlags & 0x40000) {
		CalcUp();
	}
}

/**
 * @note Address: 0x80168E58
 * @note Size: 0x6B0
 */
void CMotionControlCamera::Interpolate(u32 step)
{
	switch (mMode) {
	case 0:
		if (mFlags & CAMFLAG_DISTANCE) {
			SetDistance(mDistance);
		}
		if (mFlags & CAMFLAG_FOV) {
			SetFov(mFov);
		}
		if (mFlags & CAMFLAG_ANGLE) {
			SetAngle(mAngle);
		}
		break;
	case 1: {
		CObjVec vec;
		if (mFlags & CAMFLAG_EYE) {
			vec.Reset(mView.mEye);
			vec.SetTarget(mEye);
			vec.mSpeed = CVec(1.0f, 1.0f, 1.0f);
			vec.Step(step);
			SetEye(vec.mValue);
		}
		if (mFlags & CAMFLAG_INTEREST) {
			vec.Reset(mView.mInterest);
			vec.SetTarget(mInterest);
			vec.mSpeed = CVec(1.0f, 1.0f, 1.0f);
			vec.Step(step);
			SetInterest(vec.mValue);
		}
		break;
	}
	}

	if (mFlags & CAMFLAG_INTEREST) {
		switch (mInterestMode) {
		case 0:
			SetInterest(mInterest);
			break;
		case 2:
			Follow(mInterest, mView.mInterest);
			SetInterest(mFollowValue);
			break;
		case 3: {
			// take the target's height at once, ease in only across x and z
			Vec interest = mView.mInterest;
			interest.y   = mInterest.y;
			SetInterest(interest);
			Follow(mInterest, mView.mInterest);
			SetInterest(mFollowValue);
			break;
		}
		}
	}

	if (mFlags & CAMFLAG_EYE) {
		// Init and ReadAction only ever set mEyeMode to 0, so only case 0 runs
		switch (mEyeMode) {
		case 0:
			SetEye(mEye);
			break;
		case 1:
			break;
		case 2:
		case 3:
			Follow(mEye, mView.mEye);
			SetEye(mFollowValue);
			break;
		}
	}
}

/**
 * @note Address: 0x80169508
 * @note Size: 0x20
 */
void CMotionControlCamera::Draw()
{
	if (mAction != NULL) {
		mAction->Draw();
	}
}

/**
 * @note Address: 0x80169528
 * @note Size: 0x20
 */
void CMotionControlCamera::fn_80169528()
{
	if (mAction != NULL) {
		mAction->vf24();
	}
}

/**
 * @note Address: 0x80169548
 * @note Size: 0xE4
 */
s32 CMotionControlCamera::GetActionType()
{
	for (s32 i = 0; i < CAMACTION_NUM; i++) {
		if (mAction == GetAction(i)) {
			return i;
		}
	}
	return CAMACTION_NUM;
}

/**
 * @note Address: 0x8016962C
 * @note Size: 0x2C
 */
bool CMotionControlCamera::IsAction(s32 type)
{
	return mAction == GetAction(type);
}

/**
 * @note Address: 0x80169658
 * @note Size: 0x1C
 */
BOOL fn_80169658(CMotionControlCamera* camera)
{
	return camera->mAction == (CInterfaceCameraAction*)lbl_8045DD54;
}

/**
 * @note Address: 0x80169674
 * @note Size: 0xC
 */
CInterfaceCameraAction* fn_80169674()
{
	return (CInterfaceCameraAction*)lbl_8045DD54;
}

/**
 * @note Address: 0x80169680
 * @note Size: 0xE8
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
 * @note Address: 0x80169768
 * @note Size: 0x100
 */
void CMotionControlCamera::ChangeAction(s32 type)
{
	ChangeAction(GetAction(type));
}

/**
 * @note Address: 0x80169868
 * @note Size: 0x1B4
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
 * @note Address: 0x80169A1C
 * @note Size: 0x1C4
 */
void CMotionControlCamera::PopAction()
{
	if (mPushedAction != NULL) {
		mAction       = mPushedAction;
		mPushedAction = NULL;
		ReadAction();
		if (mAction == GetAction(CAMACTION_SCROLL)) {
			StartAction();
		}
		CopyAction();
	}
}

/**
 * @note Address: 0x80169BE0
 * @note Size: 0xF0
 */
void CMotionControlCamera::ApplyAction()
{
	CopyAction();
}

/**
 * @note Address: 0x80169CD0
 * @note Size: 0xC8
 */
void CMotionControlCamera::UpdateTarget(XObj* obj)
{
	if (mAction != NULL && obj == mAction->GetTarget() && (mFlags & CAMFLAG_INTEREST)) {
		// obj is the Wii player; its unreconstructed base class keeps the focus point
		// accessor at vtable +0x1B0, the GameCube's XObj::GetCameraTarget
		SetInterest(((WiiPlayerState*)obj)->GetCameraTarget());
		mAction->SetInterest(((WiiPlayerState*)obj)->GetCameraTarget());
	}
}

/**
 * @note Address: 0x80169D98
 * @note Size: 0xD4
 */
void CMotionControlCamera::ReadAction()
{
	mEyeMode = 0;
	if (mAction != NULL) {
		if (mAction == GetAction(CAMACTION_ANIM)) {
			mMode  = 0;
			mFlags = 0;
			mFlags |= CAMFLAG_EYE;
			mFlags |= CAMFLAG_INTEREST;
			mFlags |= CAMFLAG_FOV;
			mFlags |= CAMFLAG_ANGLE;
			mFlags |= CAMFLAG_DISTANCE;
			mFlags |= 0x40000;
		} else {
			mFlags = 0;
			mMode  = mAction->GetMode();
			mFlags |= mAction->GetCameraFlags();
			mFollowDivide = mAction->GetFollowDivisor();
			mInterestMode = mAction->GetInterestMode();
		}
	}
}

// NONMATCHING: only residue is vf28C's vptr landing in r5 instead of r12 (player-vtable
// register wall) plus literal-pool address shifts; the wipe is the virtual
// CScrnWipe::StartTransition on gGame.mRoboeyes, not the free function
/**
 * @note Address: 0x80169E6C
 * @note Size: 0x308
 */
void CMotionControlCamera::StartEye()
{
	if (!gGame.GetStatusFlags(0x100)) {
		gGame.SetStatusFlags(0x100);
		PushAction(GetAction(CAMACTION_EYE));
		StopPlayerMotion(PLAYER);
		PLAYER->vf28C(1);
		SetPlayerEyeCameraJointsHidden(PLAYER, 1, PLAYER->mMoveRotate.y, 0.0f);
		gGame.mRoboeyes.StartTransition(0xFF, 0xFF, 0xFF, 0, 0xFF, 0xFF, 0xFF, gPointerScreenWidth / 2, gPointerScreenHeight / 2, 550.0f,
		                                500.0f, 430.0f, 400.0f, 0.15f, 0xFF, 0);
		PlayWiiSoundEffect(&gGame.mSound, 0x71, 1.0f, 0.0f);
	}
}

// NONMATCHING: same residue as StartEye (vf28C register, literal-pool addresses)
/**
 * @note Address: 0x8016A174
 * @note Size: 0x30C
 */
void CMotionControlCamera::EndEye()
{
	if (gGame.GetStatusFlags(0x100)) {
		gGame.ClearStatusFlags(0x300); // and 0x200, the eye action's free-look bit
		PopAction();
		PLAYER->vf28C(0);
		// the second argument stays 1 in EndEye as well; whatever it selects happens in the
		// joint state, not here
		SetPlayerEyeCameraJointsHidden(PLAYER, 1, PLAYER->mMoveRotate.y, 0.0f);
		gGame.mRoboeyes.StartTransition(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, gPointerScreenWidth / 2, gPointerScreenHeight / 2, 430.0f,
		                                400.0f, 550.0f, 500.0f, 0.2f, 0, 1);
		PlayWiiSoundEffect(&gGame.mSound, 0x6F, 1.0f, 0.0f);
	}
}

/**
 * @note Address: 0x8016A480
 * @note Size: 0x78
 */
CCameraMemento* CMotionControlCamera::CreateMemento(CCameraMemento* memento)
{
	CCameraMemento* saved = (memento != NULL) ? memento : &mMemento;
	saved->SetEye(mView.mEye);
	saved->SetInterest(mView.mInterest);
	saved->SetAngle(mView.mAngle);
	saved->mDistance = mView.mDistance;
	saved->mFov      = CCamera::mFov; // the live fov at _08, not the action's target at _22C
	saved->mAction   = mAction;
	return saved;
}

/**
 * @note Address: 0x8016A4F8
 * @note Size: 0x254
 */
BOOL CMotionControlCamera::SetMemento(const CCameraMemento* memento)
{
	if (memento == NULL) {
		return FALSE;
	}
	SetEye(memento->mEye);
	SetInterest(memento->mInterest);
	SetAngle(memento->mAngle);
	SetFov(memento->mFov);
	SetDistance(memento->mDistance);
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
 * @note Address: 0x8016A74C
 * @note Size: 0x64
 */
extern "C" f32 GetAreaCameraFovRatio(CMotionControlCamera* camera)
{
	f32 baseFov = ((CCameraActionAreaExM*)CMotionControlCamera::GetAction(CAMACTION_AREA))->GetBaseFov();
	if (baseFov) {
		return camera->CCamera::mFov / baseFov;
	}
	return 1.0f;
}

/**
 * @note Address: 0x8016A7B0
 * @note Size: 0x20
 */
extern "C" void SetAreaCameraHeightIndex(CMotionControlCamera* camera, s32 mode)
{
	((CCameraActionAreaExM*)CMotionControlCamera::GetAction(CAMACTION_AREA))->SetZoomLevel(mode);
}

/**
 * @note Address: 0x8016A7D0
 * @note Size: 0x4C
 */
f32 CMotionControlCamera::GetMapDistance()
{
	BOOL map = IsCurrent(CAMACTION_MAP);
	if (map) {
		return GetMapCameraHorizontalTargetDistance((CCameraActionMap*)GetAction(CAMACTION_MAP)) - 4.0f;
	}
	return 1.0f;
}

/**
 * @note Address: 0x8016A81C
 * @note Size: 0x28
 */
extern "C" f32 fn_8016A81C(f32 value, f32 min, f32 max)
{
	return value < min ? min : value > max ? max : value;
}

// the look response curves, two hermite segments a piece; the pointer look evaluates them
struct WiiCameraCurve {
	f32 mP0X; // _00
	f32 mP0Y; // _04
	f32 mP1X; // _08
	f32 mP1Y; // _0C
	f32 mT0X; // _10
	f32 mT0Y; // _14
	f32 mT1X; // _18
	f32 mT1Y; // _1C
};

struct WiiCameraPoint {
	f32 x; // _00
	f32 y; // _04
};

/**
 * @note Address: 0x8016A844
 * @note Size: 0x90
 */
extern "C" WiiCameraPoint fn_8016A844(const WiiCameraCurve* curve, f32 t)
{
	f32 t2       = t * t;
	f32 t3       = t * t2;
	f32 basis0   = 2.0f * t3 - 3.0f * t2 + 1.0f;
	f32 tangent0 = t3 - 2.0f * t2 + t;
	f32 basis1   = -2.0f * t3 + 3.0f * t2;
	f32 tangent1 = t3 - t2;
	WiiCameraPoint out;
	out.x = curve->mP0X * basis0 + curve->mT0X * tangent0 + curve->mP1X * basis1 + curve->mT1X * tangent1;
	out.y = curve->mP0Y * basis0 + curve->mT0Y * tangent0 + curve->mP1Y * basis1 + curve->mT1Y * tangent1;
	return out;
}

/**
 * @note Address: 0x8016A8D4
 * @note Size: 0x98
 */
extern "C" f32 fn_8016A8D4(f32 from, f32 to, f32 t, f32 scale)
{
	f32 curve = t + std::sinf(6.2831855f * t) * (scale / 6.2831855f);
	return from * (1.0f - curve) + to * curve;
}

/**
 * @note Address: 0x8016A96C
 * @note Size: 0x108
 */
extern "C" u32 fn_8016A96C(u8* from, u8* to, f32 t)
{
	u32 out;
	((u8*)&out)[0] = from[0] * (1.0f - t) + to[0] * t;
	((u8*)&out)[1] = from[1] * (1.0f - t) + to[1] * t;
	((u8*)&out)[2] = from[2] * (1.0f - t) + to[2] * t;
	((u8*)&out)[3] = from[3] * (1.0f - t) + to[3] * t;
	return out;
}

/**
 * @note Address: 0x8016AA74
 * @note Size: 0x4
 */
extern "C" void fn_8016AA74()
{
}

/**
 * @note Address: 0x8016AA78
 * @note Size: 0x158
 */
extern "C" void fn_8016AA78(f32* out, f32* in, f32* limits)
{
	out[0] = fn_8016A81C(in[0], -limits[0], limits[0]);
	out[1] = fn_8016A81C(in[1], -limits[1], limits[1]);
	if (fabs(out[0]) < limits[2]) {
		out[0] = 0.0f;
	} else if (out[0] >= 0.0f) {
		out[0] = (out[0] - limits[2]) / (limits[0] - limits[2]);
	} else {
		out[0] = -(out[0] + limits[2]) / (limits[0] - limits[2]);
	}
	if (fabs(out[1]) < limits[3]) {
		out[1] = 0.0f;
	} else if (out[1] >= 0.0f) {
		out[1] = (out[1] - limits[3]) / (limits[1] - limits[3]);
	} else {
		out[1] = -(out[1] + limits[3]) / (limits[1] - limits[3]);
	}
}

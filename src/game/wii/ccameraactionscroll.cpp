#include "game/ccameraaction.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"

#include <math.h>

// the Wii build keeps the GameCube scroll and ladder cameras but reads its pad input from
// the remote's direction bits on gWiiPointer's status flags instead of an HSD pad. Update
// latches the up/down bits into file statics, clears them on the pad and puts them back
// on the way out so the button repeat keeps feeding them
static BOOL s_bScrollUpLatched;
static BOOL s_bScrollDownLatched;

extern s32 gRefreshRate;

#undef FRAME_TIME
#define FRAME_TIME (1.0f / gRefreshRate)

/**
 * @note Address: 0x80166D50
 * @note Size: 0x70
 */
void CCameraActionDefault::Update(u32 step)
{
	SetInterest(gGame.GetPlayer()->GetCameraTarget(), 0x10002);
}

/**
 * @note Address: 0x80166DC0
 * @note Size: 0x20
 */
Vec CCameraActionLadr::GetCameraTarget()
{
	return gGame.GetPlayer()->GetCameraTarget();
}

/**
 * @note Address: 0x80166DE0
 * @note Size: 0xC
 */
void CCameraActionLadr::Reset()
{
	mAngle = 0.0f;
}

/**
 * @note Address: 0x80166DEC
 * @note Size: 0x16C
 */
void CCameraActionLadr::Update(u32 step)
{
	Vec pos;
	pos = GetCameraTarget();
	SetFov(30.0f);
	SetInterest(pos);
	CalcEye(&pos, mAngle);
	SetEye(pos);
}

/**
 * @note Address: 0x80166F58
 * @note Size: 0xF4
 */
void CCameraActionLadr::CalcEye(Vec* eye, f32 angle)
{
	*eye = GetCameraTarget();
	eye->x -= 100.0f * (f32)sin(angle + gGame.GetPlayer()->GetRotY()); // 100 back, 30 up, orbiting the yaw
	eye->y += 30.0f;
	eye->z -= 100.0f * (f32)cos(angle + gGame.GetPlayer()->GetRotY());
}

/**
 * @note Address: 0x8016704C
 * @note Size: 0x4
 */
void CCameraActionScroll::SetCamera(CCamera* camera)
{
}

/**
 * @note Address: 0x80167050
 * @note Size: 0x24
 */
void CCameraActionScroll::Reset()
{
	mStart        = TRUE;
	mFarDistance  = 150.0f;
	mNearDistance = 100.0f;
	mScrollFov    = 40.0f;
}

/**
 * @note Address: 0x80167074
 * @note Size: 0x4B8
 */
void CCameraActionScroll::Update(u32 step)
{
	if (mStart) {
		Vec interest;
		Vec eye;
		eye      = gGame.GetPlayer()->GetCameraTarget();
		interest = eye;
		if (mZoom == 0) { // 0 near, 1 far
			eye.z += mNearDistance;
		}
		if (mZoom == 1) {
			eye.z += mFarDistance;
		}
		SetEye(eye, 11);
		SetInterest(interest, 11);
		SetFov(mScrollFov);
		if (mZoom == 0) {
			SetZoomDistance(mNearDistance);
		}
		if (mZoom == 1) {
			SetZoomDistance(mFarDistance);
		}
		mInterestMode = 0;
		mStart        = FALSE;
		return;
	}

	mInterestMode      = 2;
	s_bScrollUpLatched   = (gWiiPointer.mFlags & 0x40000000) != 0;
	s_bScrollDownLatched = (gWiiPointer.mFlags & 0x80000000) != 0;
	gWiiPointer.mFlags &= ~0x40000000ULL;
	gWiiPointer.mFlags &= ~0x80000000ULL;
	// while a direction trigger is pending the bit for the other zoom stays armed
	if (gWiiPointer.mTrigger & 8) {
		if (mZoom == 1) {
			gWiiPointer.mFlags |= 0x40000000;
		}
		if (mZoom == 0) {
			gWiiPointer.mFlags |= 0x80000000;
		}
	}
	if (gWiiPointer.Check(0x40000000)) {
		mZoom = 0;
		StartZoom(mNearDistance);
	} else if (gWiiPointer.Check(0x80000000)) {
		mZoom = 1;
		StartZoom(mFarDistance);
	}

	if (mDistanceInterp != NULL) {
		if (!mDistanceInterp->Update(step, FRAME_TIME)) {
			SetDistance(mDistanceInterp->Get());
			f32 distance = mDistanceInterp->Get();
			SetInterest(gGame.GetPlayer()->GetCameraTarget(), 0x10006);
			SetDistance(distance);
			return;
		}
		mDistanceInterp = NULL; // zoom done
	}

	SetInterest(gGame.GetPlayer()->GetCameraTarget(), 0x10002);
	if (s_bScrollUpLatched) {
		gWiiPointer.mFlags |= 0x40000000;
	} else {
		gWiiPointer.mFlags &= ~0x40000000ULL;
	}
	if (s_bScrollDownLatched) {
		gWiiPointer.mFlags |= 0x80000000;
	} else {
		gWiiPointer.mFlags &= ~0x80000000ULL;
	}
}

/**
 * @note Address: 0x8016752C
 * @note Size: 0x8
 */
u32 CCameraActionScroll::GetInterestMode()
{
	return mInterestMode;
}

/**
 * @note Address: 0x80167534
 * @note Size: 0x8
 */
u32 CCameraActionLadr::GetCameraFlags()
{
	return 11;
}

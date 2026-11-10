#include "game/ccameraaction.h"
#include "game/hsd_jobj.h"
#include "sysdolphin/controller.h"
#include "Version.h"
#include <math.h>
#include "game/video.h"
#include "game/cgame.h"
#include "game/xchr.h"

// the CGame camera fields this reads

// 50 or 60 on pal, whatever the refresh rate is
#if defined(VERSION_PAL)
#define FRAME_TIME (1.0f / gVideoFramesPerSecond)
#else
#define FRAME_TIME (1.0f / 60.0f)
#endif

// these are backwards on purpose, deferred inlining

/**
 * @note Address: 0x80148614
 * @note Size: 0x8
 */

/**
 * @note Address: 0x801485A4
 * @note Size: 0x70
 */
void CCameraActionDefault::Update(u32 step)
{
	SetInterest(gGame.GetPlayer()->GetCameraTarget(), 0x10002);
}

/**
 * @note Address: 0x8014855C
 * @note Size: 0x48
 */
Vec CCameraActionLadr::GetCameraTarget()
{
	return gGame.GetPlayer()->GetCameraTarget();
}

/**
 * @note Address: 0x80148550
 * @note Size: 0xC
 */
void CCameraActionLadr::vf14()
{
	mAngle = 0.0f;
}

/**
 * @note Address: 0x801483C0
 * @note Size: 0x190
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
 * @note Address: 0x801482A8
 * @note Size: 0x118
 */
void CCameraActionLadr::CalcEye(Vec* eye, f32 angle)
{
	*eye = GetCameraTarget();
	eye->x -= 100.0f * (f32)sin(angle + HSD_JObjGetRotationY(gGame.GetPlayer()->mJObj));
	eye->y += 30.0f;
	eye->z -= 100.0f * (f32)cos(angle + HSD_JObjGetRotationY(gGame.GetPlayer()->mJObj));
}

/**
 * @note Address: 0x801482A4
 * @note Size: 0x4
 */
void CCameraActionScroll::SetCamera(CCamera* camera)
{
}

/**
 * @note Address: 0x80148280
 * @note Size: 0x24
 */
void CCameraActionScroll::vf14()
{
	mStart        = TRUE;
	mFarDistance  = 150.0f;
	mNearDistance = 100.0f;
	mScrollFov    = 40.0f;
}

/**
 * @note Address: 0x80147EC0
 * @note Size: 0x3C0
 */
void CCameraActionScroll::Update(u32 step)
{
	if (mStart) {
		Vec interest;
		Vec eye;
		eye      = gGame.GetPlayer()->GetCameraTarget();
		interest = eye;
		if (mZoom == 0) {
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
		_64    = 0;
		mStart = FALSE;
		return;
	}

	_64 = 2;
	CPad pad(0);
	u32 nearButton = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADUP : HSD_PAD_CSTICK_UP;
	u32 farButton  = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADDOWN : HSD_PAD_CSTICK_DOWN;
	if (pad.mPad->mTrigger & nearButton) {
		mZoom = 0;
		StartZoom(mNearDistance);
	} else if (pad.mPad->mTrigger & farButton) {
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
		mDistanceInterp = NULL;
	}

	SetInterest(gGame.GetPlayer()->GetCameraTarget(), 0x10002);
}

/**
 * @note Address: 0x80147EB8
 * @note Size: 0x8
 */

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: unknown
 */
// UNUSED FUNCTION
// stands in for a stripped function. retail's .sdata2 starts with the 0.0f that only
// CCameraActionLadr::vf14 uses, and with deferred inlining the last function's constants come first
extern "C" void UnusedLadrReset(CCameraActionLadr* ladr)
{
	ladr->mAngle = 0.0f;
}

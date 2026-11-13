#include "game/ccameraaction.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/hsd_jobj.h"
#include "game/gmath.h"
#include "sysdolphin/controller.h"
#include "Version.h"
#include <math.h>
#include "game/cgame.h"
#include "game/xchr.h"

extern "C" f32 gChaseCameraSpringSettings[3] = { 0.5f, 2.5f, 4.5f };

// backwards from here, deferred inlining

// CVec's += takes a reference, this one copies b first like retail does here
inline void AddCopy(CVec& a, Vec b)
{
	PSVECAdd(&a, &b, &a);
}

/**
 * @note Address: 0x80142AF8
 * @note Size: 0x2F0
 */
CVec CalcSpring(CVec& eye, const CVec& target, const CVec& last, f32 dt, f32 stiffness, f32 damping, f32 rest)
{
	CVec dir;
	CVec vel;
	dir     = eye - target;
	f32 len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
	vel     = (last - target) * dt;
	f32 dot = PSVECDotProduct(&dir, &vel);
	PSVECNormalize(&dir, &dir);
	f32 force = stiffness * (rest - len) + damping * (dot / len); // spring toward the rest distance
	PSVECScale(&dir, &dir, force * dt);
	AddCopy(eye, dir);
	return eye;
}

/**
 * @note Address: 0x80142AE4
 * @note Size: 0x14
 */
void CCameraActionChase::Init()
{
	mDamping      = 0.5f;
	mRestDistance = 105.0f;
}

/**
 * @note Address: 0x8014295C
 * @note Size: 0x188
 */
void CCameraActionChase::Reset()
{
	mTarget = gGame.GetPlayer();
	if (mTarget != NULL) {
		mRestDistance = 105.0f;
		Mtx mtx;
		PSMTXCopy(HSD_JObjGetMtxPtr(mTarget->mJObj), mtx);
		CVec eye(0.0f, 40.0f, -mRestDistance);
		PSMTXMultVec(mtx, &eye, &eye);
		SetEye(eye);
		mPos.x     = mEye.x;
		mPos.y     = mEye.y;
		mPos.z     = mEye.z;
		mLastPos.x = mPos.x;
		mLastPos.y = mPos.y;
		mLastPos.z = mPos.z;
		SetInterest(mTarget->GetCameraTarget());
	}
}

/**
 * @note Address: 0x801428B8
 * @note Size: 0xA4
 */
void CCameraActionChase::SetCamera(CCamera* camera)
{
	SetEye(camera->mEye);
	SetInterest(camera->mInterest);
	SetDistance(camera->mDistance);
	SetFov(camera->mFov);
	mTarget = gGame.GetPlayer();
}

/**
 * @note Address: 0x80142440
 * @note Size: 0x478
 */
void CCameraActionChase::Update(u32 step)
{
	if (mTarget == NULL) {
		return;
	}

	f32 dt     = 0.1f * step;
	mLastPos.x = mPos.x;
	mLastPos.y = mPos.y;
	mLastPos.z = mPos.z;

	Mtx mtx;
	PSMTXCopy(HSD_JObjGetMtxPtr(mTarget->mJObj), mtx);
	CVec behind(0.0f, 30.0f, -1.0f);
	PSMTXMultVec(mtx, &behind, &behind);
	mPos.x = behind.x;
	mPos.y = behind.y;
	mPos.z = behind.z;

	CVec eye((const Vec&)mEye);
	eye = CalcSpring(eye, mPos, mLastPos, dt, gChaseCameraSpringSettings[0], mDamping, mRestDistance);
	SetEye(eye);
	SetInterest(mTarget->GetCameraTarget());
	mMode = 3;

	CPad pad(0);
	u32 leftButton  = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADLEFT : HSD_PAD_CSTICK_LEFT; // the flag picks the pad layout
	u32 rightButton = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADRIGHT : HSD_PAD_CSTICK_RIGHT;
	u32 upButton    = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADUP : HSD_PAD_CSTICK_UP;
	u32 downButton  = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADDOWN : HSD_PAD_CSTICK_DOWN;

	{
		CVec angle = mAngle;
		f32 pitch  = 0.0f;
		f32 yaw    = 0.0f;
		if (pad.mPad->mButton & rightButton) {
			yaw = 1.0f;
		} else if (pad.mPad->mButton & leftButton) {
			yaw = -1.0f;
		}
		if (yaw || pitch) {
			f32 dx = step * (pitch * 0.05235988f); // 3 degrees per step
			f32 dy = step * (yaw * 0.05235988f);
			f32 y  = angle.y + dy;
			f32 x  = angle.x + dx;
			if (x > 1.5550883f) {
				x = 1.5550883f; // just under 90 degrees
			}
			if (x < -1.5550883f) {
				x = -1.5550883f;
			}
			SetAngle(x, y, angle.z);
			mMode = 0x20012;
		}
	}

	u32 up = pad.mPad->mButton & upButton;
	if (up || (pad.mPad->mButton & downButton)) {
		f32 distance = mDistance;
		if (up) {
			distance -= 12.0f * step;
			if (distance < 60.0f) { // the zoom floor
				distance = 60.0f;
			}
		}
		if (pad.mPad->mButton & downButton) {
			distance += 12.0f * step;
		}
		SetDistance(distance);
		mRestDistance = distance;
		mMode         = 0x80006;
	}
}

/**
 * @note Address: 0x80142438
 * @note Size: 0x8
 */
u32 CCameraActionChase::GetCameraFlags()
{
	return mMode;
}

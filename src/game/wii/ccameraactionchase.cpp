#include "game/ccameraaction.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/ut.h"

#include <math.h>
extern "C" {
#include "Dolphin/mtx.h"
}
#include "JSystem/JMath/Inline.h"

// the Wii build inlines the five JMathInlineVEC helpers as paired-single code;
// the longer SDK calls (PSVECNormalize) stay out of line
#define PSVECAdd        JMathInlineVEC::PSVECAdd
#define PSVECSubtract   JMathInlineVEC::PSVECSubtract
#define PSVECScale      JMathInlineVEC::PSVECScale
#define PSVECDotProduct JMathInlineVEC::PSVECDotProduct
#define PSVECSquareMag  JMathInlineVEC::PSVECSquareMag

// the Wii build keeps the GameCube spring camera but swaps its two dependencies:
// the target is the Wii player object (matrix and focus point through its vtable)
// and the pad input is the remote's direction bits on gWiiPointer instead of an HSD pad

extern "C" f32 gChaseCameraSpringSettings[3] = { 0.5f, 2.5f, 4.5f };

// CVec's += takes a reference, this one copies b first like retail does here
inline void AddCopy(CVec& a, Vec b)
{
	PSVECAdd(&a, &b, &a);
}

// NONMATCHING: the CVec subtraction and call set match; remaining differences are CVec temporary
// GPR allocation and paired-single register/order choices.
/**
 * @note Address: 0x80160D30
 * @note Size: 0x1FC
 */
CVec CalcSpring(CVec& eye, const CVec& target, const CVec& last, f32 dt, f32 stiffness, f32 damping, f32 rest)
{
	CVec dir;
	CVec vel;
	dir       = eye - target;
	f32 len   = sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
	vel       = (last - target) * dt;
	f32 dot   = PSVECDotProduct(&dir, &vel);
	f32 force = stiffness * (rest - len) + damping * (dot / len); // spring toward the rest distance
	PSVECNormalize(&dir, &dir);
	PSVECScale(&dir, &dir, force * dt);
	AddCopy(eye, dir);
	return eye;
}

/**
 * @note Address: 0x80160F2C
 * @note Size: 0x14
 */
void CCameraActionChase::Init()
{
	mDamping      = 0.5f;
	mRestDistance = 105.0f;
}

/**
 * @note Address: 0x80160F40
 * @note Size: 0x10C
 */
void CCameraActionChase::Reset()
{
	mTarget = (XObj*)gGame.GetPlayer();
	if (mTarget != NULL) {
		mRestDistance = 105.0f;
		Mtx mtx;
		PSMTXCopy(((WiiPlayerState*)mTarget)->GetWorldMtx(), mtx);
		CVec eye(0.0f, 40.0f, -mRestDistance);
		PSMTXMultVec(mtx, &eye, &eye);
		SetEye(eye);
		mPos.x     = mEye.x;
		mPos.y     = mEye.y;
		mPos.z     = mEye.z;
		mLastPos.x = mPos.x;
		mLastPos.y = mPos.y;
		mLastPos.z = mPos.z;
		SetInterest(((WiiPlayerState*)mTarget)->GetCameraTarget());
	}
}

/**
 * @note Address: 0x8016104C
 * @note Size: 0xA4
 */
void CCameraActionChase::SetCamera(CCamera* camera)
{
	SetEye(camera->mView.mEye);
	SetInterest(camera->mView.mInterest);
	SetDistance(camera->mView.mDistance);
	SetFov(camera->mFov);
	mTarget = (XObj*)gGame.GetPlayer();
}

// NONMATCHING: same inline-PSVEC* residue as CalcSpring plus leftover register/schedule
// differences around the spring call and the pointer-flag branches
/**
 * @note Address: 0x801610F0
 * @note Size: 0x360
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
	PSMTXCopy(((WiiPlayerState*)mTarget)->GetWorldMtx(), mtx);
	CVec behind(0.0f, 30.0f, -1.0f);
	PSMTXMultVec(mtx, &behind, &behind);
	mPos.x = behind.x;
	mPos.y = behind.y;
	mPos.z = behind.z;

	CVec eye((const Vec&)mEye);
	eye = CalcSpring(eye, mPos, mLastPos, dt, gChaseCameraSpringSettings[0], mDamping, mRestDistance);
	SetEye(eye);
	SetInterest(((WiiPlayerState*)mTarget)->GetCameraTarget());
	mMode = 3;

	// the remote's direction pad: right/left turn, up/down zoom
	{
		CVec angle = mAngle;
		f32 pitch  = 0.0f;
		f32 yaw    = 0.0f;
		if (gWiiPointer.mFlags & 0x08000000) {
			yaw = 1.0f;
		} else if (gWiiPointer.mFlags & 0x04000000) {
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

	u64 up = gWiiPointer.mFlags & 0x10000000;
	if (up || (gWiiPointer.mFlags & 0x20000000)) {
		f32 distance = mDistance;
		if (up) {
			distance -= 12.0f * step;
			if (distance < 60.0f) { // the zoom floor
				distance = 60.0f;
			}
		}
		if (gWiiPointer.mFlags & 0x20000000) {
			distance += 12.0f * step;
		}
		SetDistance(distance);
		mRestDistance = distance;
		mMode         = 0x80006;
	}
}

/**
 * @note Address: 0x80161450
 * @note Size: 0x8
 */
u32 CCameraActionChase::GetCameraFlags()
{
	return mMode;
}

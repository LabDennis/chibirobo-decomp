#include "game/ccameraaction.h"

// deferred inlining puts these out backwards

/**
 * @note Address: 0x80147624
 * @note Size: 0xC
 */
void CCameraActionAnim::Init()
{
	mObj = NULL;
}

/**
 * @note Address: 0x801475E0
 * @note Size: 0x44
 */
void CCameraActionAnim::SetObj(XObjCamera* obj)
{
	if (obj->mStatus.mFlags & 1) {
		mObj    = obj;
		mCamera = &obj->mCamera;
	} else {
		mObj    = NULL;
		mCamera = NULL;
	}
}

/**
 * @note Address: 0x80147538
 * @note Size: 0xA8
 */
void CCameraActionAnim::SetCamera(CCamera* camera)
{
	SetEye(camera->mEye);
	SetInterest(camera->mInterest);
	SetAngle(camera->mAngle);
	SetDistance(camera->mDistance);
	SetFov(camera->mFov);
}

/**
 * @note Address: 0x80147464
 * @note Size: 0xD4
 */
void CCameraActionAnim::Update(u32 step)
{
	if (mCamera != NULL) {
		SetEye(mCamera->mEye);
		SetInterest(mCamera->mInterest);
		SetAngle(mCamera->mAngle);
		SetDistance(mCamera->mDistance);
		SetFov(mCamera->mFov);
		SetAngleZ(mCamera->mRoll);
	}
}

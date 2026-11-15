#include "game/ccameraaction.h"
#include "game/wii/cgame.h"

/**
 * @note Address: 0x8016642C
 * @note Size: 0xC
 */
void CCameraActionAnim::Init()
{
	mObj = NULL;
}

// the Wii version drops the status flag check and always clears the binding
/**
 * @note Address: 0x80166438
 * @note Size: 0x10
 */
void CCameraActionAnim::SetObj(XObjCamera* obj)
{
	mObj    = NULL;
	mCamera = NULL;
}

/**
 * @note Address: 0x80166448
 * @note Size: 0xA8
 */
void CCameraActionAnim::SetCamera(CCamera* camera)
{
	SetEye(camera->mView.mEye);
	SetInterest(camera->mView.mInterest);
	SetAngle(camera->mView.mAngle);
	SetDistance(camera->mView.mDistance);
	SetFov(camera->mFov);
}

/**
 * @note Address: 0x801664F0
 * @note Size: 0xD4
 */
void CCameraActionAnim::Update(u32 step)
{
	if (mCamera != NULL) {
		SetEye(mCamera->mView.mEye);
		SetInterest(mCamera->mView.mInterest);
		SetAngle(mCamera->mView.mAngle);
		SetDistance(mCamera->mView.mDistance);
		SetFov(mCamera->mFov);
		SetAngleZ(mCamera->mView.mAngle.z);
	}
}

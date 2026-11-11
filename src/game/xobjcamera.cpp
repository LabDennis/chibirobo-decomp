#include "game/xobj.h"

extern "C" {
extern const char sCameraAnimAObjFile[7], sCameraAnimAObjCondition[5];
}

// the camera archive's first desc list, the cobj then its anims
static inline HSD_CObjDesc* GetCObjDesc(XObjCamera* object)
{
	return (HSD_CObjDesc*)object->mArchive.mDesc[0]->mObjectDesc;
}

static inline HSD_CameraAnim** GetCameraAnims(XObjCamera* object)
{
	return (HSD_CameraAnim**)object->mArchive.mDesc[0]->mAnimations;
}

/**
 * @note Address: 0x80092B80
 * @note Size: 0x10
 */
extern "C" bool HasCameraAnimation(XObjCamera* object)
{
	return object->mCameraAnim >= 0;
}

/**
 * @note Address: 0x80092B90
 * @note Size: 0xF8
 */
void XObjCamera::UpdateObject(u32 frames)
{
	if (mCameraAnim >= 0) {
		mCamera.SetAnimRate((f32)frames);
		mCamera.Anim();
		f32 end = mCamera.mLongestAObj ? HSD_AObjGetEndFrame(mCamera.mLongestAObj, sCameraAnimAObjFile, sCameraAnimAObjCondition) : 0.0f;
		f32 current
		    = mCamera.mLongestAObj ? HSD_AObjGetCurrentFrame(mCamera.mLongestAObj, sCameraAnimAObjFile, sCameraAnimAObjCondition) : 0.0f;
		if (current >= end)
			mCameraAnim = -1;
	}
}

/**
 * @note Address: 0x80092C88
 * @note Size: 0x68
 */
extern "C" void SetStageCameraAnimation(XObjCamera* object, s32 index, f32 frame)
{
	object->mCameraAnim = index;
	object->mCamera.SetAnim(GetCameraAnims(object)[object->mCameraAnim], frame);
	object->mCamera.Anim();
}

/**
 * @note Address: 0x80092CF0
 * @note Size: 0x44
 */
void XObjCamera::Release()
{
	mCamera.Release();
	XObj::Release();
}

/**
 * @note Address: 0x80092D34
 * @note Size: 0x5C
 */
BOOL XObjCamera::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		mCamera.Init(GetCObjDesc(this));
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80092D90
 * @note Size: 0xC
 */
void XObjCamera::ResetObjectState()
{
	mCameraAnim = -1;
}

/**
 * @note Address: 0x80092D9C
 * @note Size: 0x20
 */
void XObjCamera::InitializeObject(u32 id)
{
	XObj::InitializeObject(id);
}

#include "game/cshadow.h"
extern "C" {
#include <sysdolphin/initialize.h>
}

/**
 * @note Address: 0x801942F8
 * @note Size: 0x104
 */
void CShadow::Init(HSD_LObj* lobj)
{
	u32 type = HSD_LObjGetType(lobj);

	if (type != LOBJ_AMBIENT) {
		if (mShadow == NULL) {
			mShadow = HSD_ShadowAlloc();
		}
		HSD_ShadowSetUserData(mShadow, lobj);
		switch (type) {
		case LOBJ_INFINITE:
			HSD_CObjSetProjectionType(mShadow->camera, PROJ_ORTHO);
			break;
		case LOBJ_POINT:
		case LOBJ_SPOT:
			HSD_CObjSetProjectionType(mShadow->camera, PROJ_FRUSTRUM);
			break;
		}
		HSD_ShadowSetSize(mShadow, 640, 480);
		HSD_CObjSetNear(mShadow->camera, 0.1f);
		HSD_CObjSetFar(mShadow->camera, 1000.0f);
		mActive    = FALSE;
		mIntensity = 0x80;
	}
}

/**
 * @note Address: 0x801942B8
 * @note Size: 0x40
 */
void CShadow::Release()
{
	if (mShadow != NULL) {
		HSD_ShadowRemove(mShadow);
		mShadow = NULL;
	}
}

/**
 * @note Address: 0x80194204
 * @note Size: 0xB4
 */
void CShadow::InitRect()
{
	HSD_LObj* lobj   = (HSD_LObj*)HSD_ShadowGetUserData(mShadow);
	bool perspective = HSD_LObjGetType(lobj) != LOBJ_INFINITE;
	Vec interest;
	Vec eye;
	Vec up;

	HSD_CObjGetInterest(mShadow->camera, &interest);
	HSD_CObjGetEyePosition(mShadow->camera, &eye);
	HSD_CObjGetUpVector(mShadow->camera, &up);
	HSD_ViewingRectInit(&mRect, &eye, &interest, &up, perspective);
}

/**
 * @note Address: 0x801941E0
 * @note Size: 0x24
 */
void CShadow::AddRect(Vec* position, f32 top, f32 bottom, f32 left, f32 right)
{
	HSD_ViewingRectAddRect(&mRect, position, top, bottom, left, right);
}

/**
 * @note Address: 0x80193F64
 * @note Size: 0x27C
 */
CCamera* CShadow::StartRender()
{
	HSD_ViewingRect* rect = &mRect;
	Vec interest;
	Vec eye;
	Vec up;

	if (HSD_ViewingRectCheck(rect)) {
		HSD_ShadowSetViewingRect(mShadow, rect->top, rect->bottom, rect->left, rect->right);
	} else {
		HSD_ShadowSetViewingRect(mShadow, 100.0f, -100.0f, -100.0f, 100.0f);
	}
	HSD_ShadowSetIntensity(mShadow, mIntensity);
	HSD_StartRender(HSD_RP_OFFSCREEN);
	HSD_ShadowInit(mShadow);
	HSD_ShadowStartRender(mShadow);

	*(HSD_CObj*)&mCamera = *mShadow->camera;
	HSD_CObjGetInterest(mShadow->camera, &interest);
	HSD_CObjGetEyePosition(mShadow->camera, &eye);
	HSD_CObjGetUpVector(mShadow->camera, &up);
	mCamera.SetInterest(interest);
	mCamera.SetEye(eye);
	mCamera.SetUp(up);
	mCamera.SetScale(1.0f, 1.0f, 1.0f);
	return &mCamera;
}

/**
 * @note Address: 0x80193F3C
 * @note Size: 0x28
 */
void CShadow::EndRender()
{
	HSD_ShadowEndRender(mShadow);
	HSD_EndRender();
}

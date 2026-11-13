#include "game/hsd_jobj.h"
#include "game/msg.h"
#include "game/cmotioncontrolcamera.h"

void operator delete(void* object);

extern "C" {
extern void* __vt__20CMotionControlCamera[];
extern void* __vt__14CCameraActions[];
extern void* __vt__17CCameraActionArea[];
extern void* __vt__20CInterpolationLinear[];
extern void* __vt__21CInterpolationEaseOut[];
extern void* __vt__16CCameraActionEvt[];
extern void* __vt__18CInterpolationNone[];
extern void* __vt__18CCameraActionReset[];
CCameraActionArea* __dt__17CCameraActionAreaFv(CCameraActionArea* area, s32 flags);
CCameraActionEvt* __dt__16CCameraActionEvtFv(CCameraActionEvt* evt, s32 flags);
CCameraActionReset* __dt__18CCameraActionResetFv(CCameraActionReset* reset, s32 flags);
}

/**
 * @note Address: 0x80008FB0
 * @note Size: 0x7C
 */
// todo: weak copy
extern "C" MsgFontGlyph* __dt__12MsgFontGlyphFv(MsgFontGlyph* glyph, s32 flags)
{
	if (glyph != NULL) {
		glyph->mSprite.CSprite::~CSprite();
		glyph->mTexture.CTexObj::~CTexObj();
		if ((s16)flags > 0)
			::operator delete(glyph);
	}
	return glyph;
}

/**
 * @note Address: 0x8000902C
 * @note Size: 0x1A0
 */
// todo: weak copy
extern "C" CMotionControlCamera* __dt__20CMotionControlCameraFv(CMotionControlCamera* camera, s32 flags)
{
	if (camera != NULL) {
		*(void***)((u8*)camera + 0x1F4) = __vt__20CMotionControlCamera;
		camera->mMemento.CCameraMemento::~CCameraMemento();
		camera->mFollowStep.CVec::~CVec();
		camera->mFollowValue.CVec::~CVec();
		camera->mFollowTarget.CVec::~CVec();
		camera->mFlags.CStatus::~CStatus();
		camera->mAngle.CVec::~CVec();
		camera->mInterest.CVec::~CVec();
		camera->mEye.CVec::~CVec();
		camera->mCamera.CCamera::~CCamera();
		if ((s16)flags > 0)
			::operator delete(camera);
	}
	return camera;
}

/**
 * @note Address: 0x800091CC
 * @note Size: 0x3F4
 */
// todo: weak copy
extern "C" CCameraActions* __dt__14CCameraActionsFv(CCameraActions* actions, s32 flags)
{
	if (actions != NULL) {
		*(void***)((u8*)actions + 0x1E70) = __vt__14CCameraActions;
		actions->mScroll.CCameraActionScroll::~CCameraActionScroll();
		__dt__18CCameraActionResetFv(&actions->mReset, -1);
		actions->mLadr.CCameraActionLadr::~CCameraActionLadr();
		actions->mMap.CCameraActionMap::~CCameraActionMap();
		actions->mAnim.CCameraActionAnim::~CCameraActionAnim();
		actions->mEye.CCameraActionEye::~CCameraActionEye();
		actions->mScroll2.CCameraActionScroll2::~CCameraActionScroll2();
		__dt__16CCameraActionEvtFv(&actions->mEvt, -1);
		actions->mChase.CCameraActionChase::~CCameraActionChase();
		__dt__17CCameraActionAreaFv(&actions->mArea, -1);
		actions->mDefault.CCameraActionDefault::~CCameraActionDefault();
		if ((s16)flags > 0)
			::operator delete(actions);
	}
	return actions;
}

/**
 * @note Address: 0x800095C0
 * @note Size: 0x298
 */
// todo: weak copy
extern "C" CCameraActionArea* __dt__17CCameraActionAreaFv(CCameraActionArea* area, s32 flags)
{
	if (area != NULL) {
		*(void***)((u8*)area + 0x3C) = __vt__17CCameraActionArea;
		area->mFovEaseOutDivide.CInterpolationEaseOutDivide::~CInterpolationEaseOutDivide();
		area->mFovEaseOut.CInterpolationEaseOut::~CInterpolationEaseOut();
		area->mFovEaseInOut.CInterpolationEaseInOut::~CInterpolationEaseInOut();
		area->mFovLinear.CInterpolationLinear::~CInterpolationLinear();
		area->mEyeEaseOutDivide.CVectorEaseOutDivideInterpolation::~CVectorEaseOutDivideInterpolation();
		area->mEyeEaseOut.CVectorEaseOutInterpolation::~CVectorEaseOutInterpolation();
		area->mEyeEaseInOut.CVectorEaseInOutInterpolation::~CVectorEaseInOutInterpolation();
		area->mEyeLinear.CVectorLinearInterpolation::~CVectorLinearInterpolation();
		area->mInterestEaseInOut.CVectorEaseInOutInterpolation::~CVectorEaseInOutInterpolation();
		area->CCameraActionAreaExM::~CCameraActionAreaExM();
		if ((s16)flags > 0)
			::operator delete(area);
	}
	return area;
}

/**
 * @note Address: 0x80009858
 * @note Size: 0x5C
 */
// todo: weak copy
extern "C" CInterpolationLinear* __dt__20CInterpolationLinearFv(CInterpolationLinear* interp, s32 flags)
{
	if (interp != NULL) {
		*(void***)interp = __vt__20CInterpolationLinear;
		interp->CInterfaceInterpolation::~CInterfaceInterpolation();
		if ((s16)flags > 0)
			::operator delete(interp);
	}
	return interp;
}

/**
 * @note Address: 0x80009910
 * @note Size: 0x5C
 */
// todo: weak copy
extern "C" CInterpolationEaseOut* __dt__21CInterpolationEaseOutFv(CInterpolationEaseOut* interp, s32 flags)
{
	if (interp != NULL) {
		*(void***)interp = __vt__21CInterpolationEaseOut;
		interp->CInterfaceInterpolation::~CInterfaceInterpolation();
		if ((s16)flags > 0)
			::operator delete(interp);
	}
	return interp;
}

/**
 * @note Address: 0x800099C8
 * @note Size: 0x4FC
 */
// todo: weak copy
extern "C" CCameraActionEvt* __dt__16CCameraActionEvtFv(CCameraActionEvt* evt, s32 flags)
{
	if (evt != NULL) {
		*(void***)((u8*)evt + 0x3C) = __vt__16CCameraActionEvt;
		evt->mAngleNone.CVectorNoneInterpolation::~CVectorNoneInterpolation();
		evt->mAngleEaseOut.CVectorEaseOutInterpolation::~CVectorEaseOutInterpolation();
		evt->mAngleEaseInOut.CVectorEaseInOutInterpolation::~CVectorEaseInOutInterpolation();
		evt->mAngleLinear.CVectorLinearInterpolation::~CVectorLinearInterpolation();
		evt->mDistanceNone.CInterpolationNone::~CInterpolationNone();
		evt->mDistanceEaseOut.CInterpolationEaseOut::~CInterpolationEaseOut();
		evt->mDistanceEaseInOut.CInterpolationEaseInOut::~CInterpolationEaseInOut();
		evt->mDistanceLinear.CInterpolationLinear::~CInterpolationLinear();
		evt->mFovNone.CInterpolationNone::~CInterpolationNone();
		evt->mFovEaseOut.CInterpolationEaseOut::~CInterpolationEaseOut();
		evt->mFovEaseInOut.CInterpolationEaseInOut::~CInterpolationEaseInOut();
		evt->mFovLinear.CInterpolationLinear::~CInterpolationLinear();
		evt->mInterestNone.CVectorNoneInterpolation::~CVectorNoneInterpolation();
		evt->mInterestEaseOut.CVectorEaseOutInterpolation::~CVectorEaseOutInterpolation();
		evt->mInterestEaseInOut.CVectorEaseInOutInterpolation::~CVectorEaseInOutInterpolation();
		evt->mInterestLinear.CVectorLinearInterpolation::~CVectorLinearInterpolation();
		evt->mEyeNone.CVectorNoneInterpolation::~CVectorNoneInterpolation();
		evt->mEyeEaseOut.CVectorEaseOutInterpolation::~CVectorEaseOutInterpolation();
		evt->mEyeEaseInOut.CVectorEaseInOutInterpolation::~CVectorEaseInOutInterpolation();
		evt->mEyeLinear.CVectorLinearInterpolation::~CVectorLinearInterpolation();
		evt->mAxes.CStatus::~CStatus();
		evt->CInterfaceCameraAction::~CInterfaceCameraAction();
		if ((s16)flags > 0)
			::operator delete(evt);
	}
	return evt;
}

/**
 * @note Address: 0x80009EC4
 * @note Size: 0x5C
 */
// todo: weak copy
extern "C" CInterpolationNone* __dt__18CInterpolationNoneFv(CInterpolationNone* interp, s32 flags)
{
	if (interp != NULL) {
		*(void***)interp = __vt__18CInterpolationNone;
		interp->CInterfaceInterpolation::~CInterfaceInterpolation();
		if ((s16)flags > 0)
			::operator delete(interp);
	}
	return interp;
}

/**
 * @note Address: 0x80009F20
 * @note Size: 0x340
 */
CCameraActionMap::~CCameraActionMap()
{
}

/**
 * @note Address: 0x8000A260
 * @note Size: 0x258
 */
// todo: weak copy
extern "C" CCameraActionReset* __dt__18CCameraActionResetFv(CCameraActionReset* reset, s32 flags)
{
	if (reset != NULL) {
		*(void***)((u8*)reset + 0x3C) = __vt__18CCameraActionReset;
		reset->mFovNone.CInterpolationNone::~CInterpolationNone();
		reset->mFovEaseOut.CInterpolationEaseOut::~CInterpolationEaseOut();
		reset->mFovEaseInOut.CInterpolationEaseInOut::~CInterpolationEaseInOut();
		reset->mFovLinear.CInterpolationLinear::~CInterpolationLinear();
		reset->mEyeNone.CVectorNoneInterpolation::~CVectorNoneInterpolation();
		reset->mEyeEaseOut.CVectorEaseOutInterpolation::~CVectorEaseOutInterpolation();
		reset->mEyeEaseInOut.CVectorEaseInOutInterpolation::~CVectorEaseInOutInterpolation();
		reset->mEyeLinear.CVectorLinearInterpolation::~CVectorLinearInterpolation();
		reset->CCameraActionAreaExM::~CCameraActionAreaExM();
		if ((s16)flags > 0)
			::operator delete(reset);
	}
	return reset;
}

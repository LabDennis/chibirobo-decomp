#include "game/hsd_jobj.h"
#include "game/msg.h"
#include "game/cmotioncontrolcamera.h"

void operator delete(void* object);

extern "C" {
extern void* __vt__20CMotionControlCamera[];
extern void* __vt__14CCameraActions[];
}

// weak destructor copies (MsgFontGlyph and the camera classes), everything in them is inline.
// The member destructors CCameraActions calls out of line (Area, Evt, Map, Reset) and the
// interpolation destructors their vtables need are emitted by the compiler after this, in the dol's order.

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
		actions->mReset.CCameraActionReset::~CCameraActionReset();
		actions->mLadr.CCameraActionLadr::~CCameraActionLadr();
		actions->mMap.CCameraActionMap::~CCameraActionMap();
		actions->mAnim.CCameraActionAnim::~CCameraActionAnim();
		actions->mEye.CCameraActionEye::~CCameraActionEye();
		actions->mScroll2.CCameraActionScroll2::~CCameraActionScroll2();
		actions->mEvt.CCameraActionEvt::~CCameraActionEvt();
		actions->mChase.CCameraActionChase::~CCameraActionChase();
		actions->mArea.CCameraActionArea::~CCameraActionArea();
		actions->mDefault.CCameraActionDefault::~CCameraActionDefault();
		if ((s16)flags > 0)
			::operator delete(actions);
	}
	return actions;
}

#include "game/clight.h"

/**
 * @note Address: 0x80193CD0
 * @note Size: 0x26C
 */
void CLight::Init(u32 flags, BOOL useShadow)
{
	u32 type     = flags & LOBJ_TYPE_MASK;
	u32 specular = flags & LOBJ_SPECULAR;

	mLoadedLObj = NULL;
	this->flags = 0;
	switch (type) {
	case LOBJ_AMBIENT:
		HSD_LObjSetFlags(this, LOBJ_AMBIENT | LOBJ_DIFFUSE);
		mColor.r = 0;
		mColor.g = 0;
		mColor.b = 0; // ambient starts black
		mColor.a = 255;
		HSD_LObjSetColor(this, mColor);
		break;
	case LOBJ_INFINITE:
		HSD_LObjSetFlags(this, LOBJ_INFINITE | LOBJ_DIFFUSE);
		mColor.r = 255;
		mColor.g = 255;
		mColor.b = 255;
		mColor.a = 255;
		HSD_LObjSetColor(this, mColor);
		break;
	case LOBJ_POINT:
		HSD_LObjSetFlags(this, LOBJ_POINT | LOBJ_DIFFUSE);
		mColor.r = 255;
		mColor.g = 255;
		mColor.b = 255;
		mColor.a = 255;
		HSD_LObjSetColor(this, mColor);
		HSD_LObjSetDistAttn(this, 10000.0f, 0.99f, 3); // a far reference distance
		break;
	case LOBJ_SPOT:
		HSD_LObjSetFlags(this, LOBJ_SPOT | LOBJ_DIFFUSE);
		mColor.r = 255;
		mColor.g = 255;
		mColor.b = 255;
		mColor.a = 255;
		HSD_LObjSetColor(this, mColor);
		HSD_LObjSetDistAttn(this, 10000.0f, 0.99f, 3);
		HSD_LObjSetSpot(this, 29.999956f, 3); // a ~30-degree cone
		break;
	}

	if (specular) { // specular reuses the color the type case set
		GXColor color = this->color;

		HSD_LObjSetFlags(this, LOBJ_SPECULAR);
		mSpecColor.r = color.r;
		mSpecColor.g = color.g;
		mSpecColor.b = color.b;
		mSpecColor.a = color.a;
	}

	position        = &mPositionWObj;
	interest        = &mInterestWObj;
	position->flags = 0;
	position->pos.x = position->pos.y = position->pos.z = 0.0f;
	position->aobj                                      = NULL;
	position->robj                                      = NULL;
	interest->flags                                     = 0;
	interest->pos.x = interest->pos.y = interest->pos.z = 0.0f;
	interest->aobj                                      = NULL;
	interest->robj                                      = NULL;
	mLongestAObj                                        = NULL;
	if (useShadow && HSD_LObjGetType(this) != LOBJ_AMBIENT) { // ambient lights get no shadow
		mShadow.Init(this);
		mShadow.SetActive(TRUE);
	}
}

/**
 * @note Address: 0x80193A7C
 * @note Size: 0x254
 */
void CLight::Init(HSD_LightDesc* desc, BOOL useShadow)
{
	mLoadedLObj      = HSD_LObjLoadDesc(desc);
	*(HSD_LObj*)this = *mLoadedLObj;
	HSD_LObjGetPosition(this, &mPosition);
	HSD_LObjGetInterest(this, &mInterest);
	HSD_LObjGetColor(this, &mColor);
	mLongestAObj = NULL;
	if (useShadow && HSD_LObjGetType(this) != LOBJ_AMBIENT) {
		mShadow.Init(this);
		mShadow.SetActive(TRUE);
	}
}

/**
 * @note Address: 0x801937CC
 * @note Size: 0x2B0
 */
void CLight::Release()
{
	if (mLoadedLObj != NULL) {
		// hand the current state back so the release frees what we're using now
		*mLoadedLObj = *(HSD_LObj*)this;
		HSD_LObjUnref(mLoadedLObj);
		mLoadedLObj = NULL;
	}
	if (mShadow.IsActive()) {
		mShadow.SetActive(FALSE);
		mShadow.Release();
	}
}

/**
 * @note Address: 0x801936C0
 * @note Size: 0x10C
 */
void CLight::SetAnim(HSD_LightAnim* anim, f32 frame)
{
	HSD_AObj* aobj;
	f32 end;
	f32 endFrame;

	HSD_LObjAddAnim(this, anim);
	HSD_LObjReqAnim(this, frame);

	end  = 0.0f;
	aobj = this->aobj;
	if (aobj != NULL && (endFrame = HSD_AObjGetEndFrame(aobj)) > end) { // keep the longest aobj
		end          = endFrame;
		mLongestAObj = aobj;
	}
	if (position != NULL && (aobj = position->aobj) != NULL && (endFrame = HSD_AObjGetEndFrame(aobj)) > end) {
		end          = endFrame;
		mLongestAObj = aobj;
	}
	if (interest != NULL && (aobj = interest->aobj) != NULL && (endFrame = HSD_AObjGetEndFrame(aobj)) > end) {
		mLongestAObj = aobj;
	}
}

/**
 * @note Address: 0x80193670
 * @note Size: 0x50
 */
void CLight::Anim()
{
	HSD_LObjAnim(this);
	HSD_LObjGetPosition(this, &mPosition);
	HSD_LObjGetInterest(this, &mInterest);
	HSD_LObjGetColor(this, &mColor);
}

/**
 * @note Address: 0x80193634
 * @note Size: 0x3C
 */
void CLight::SetCurrent(CCamera* camera)
{
	HSD_LObjSetCurrentAll(this);
	if (camera != NULL) {
		HSD_LObjSetupInit(camera);
	}
}

// inline methods only this file emits

/**
 * @note Address: 0x801935FC
 * @note Size: 0x38
 */
// SetInterest__6CLightFRC3Vec, inline in game/clight.h

/**
 * @note Address: 0x801935C4
 * @note Size: 0x38
 */
// SetPosition__6CLightFRC3Vec, inline in game/clight.h

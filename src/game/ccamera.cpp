#include "game/ccamera.h"
#include "game/gmath.h"
#include <math.h>
#include <float.h>

/**
 * @note Address: 0x80193468
 * @note Size: 0x15C
 */
void CCamera::Init(f32 near, f32 far, f32 fov, GXRenderModeObj* rmode)
{
	HSD_CObjPerspectiveDesc desc
	    = { NULL, 0, PROJ_PERSPECTIVE, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, NULL, NULL, 0.0f, NULL, near, far, fov, 0.0f };

	mFov        = fov;
	mUseMtx     = FALSE;
	mLoadedCObj = NULL;
	HSD_CObjInit(this, (HSD_CObjDesc*)&desc);
	mEye.x      = 0.0f;
	mEye.y      = 0.0f;
	mEye.z      = 0.0f;
	mInterest.x = 0.0f;
	mInterest.y = 0.0f;
	mInterest.z = 0.0f;
	mAngle.x    = 0.0f;
	mAngle.y    = 0.0f;
	mAngle.z    = 0.0f;
	mUp.x       = 0.0f;
	mUp.y       = 1.0f;
	mUp.z       = 0.0f;
	SetScale(1.0f, 1.0f, 1.0f);
	SetViewport(0.0f, 0.0f, rmode);
	SetScissor(rmode);
	mLongestAObj = NULL;
}

/**
 * @note Address: 0x801932F4
 * @note Size: 0x174
 */
void CCamera::Init(f32 near, f32 far, f32 top, f32 bottom, f32 left, f32 right, GXRenderModeObj* rmode)
{
	HSD_CObjDesc desc = { NULL, 0, PROJ_FRUSTRUM, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, NULL, NULL, 0.0f, NULL, near, far };

	desc.projection_param.frustrum.top    = top;
	desc.projection_param.frustrum.bottom = bottom;
	desc.projection_param.frustrum.left   = left;
	desc.projection_param.frustrum.right  = right;
	mUseMtx                               = FALSE;
	mLoadedCObj                           = NULL;
	HSD_CObjInit(this, &desc);
	mEye.x      = 0.0f;
	mEye.y      = 0.0f;
	mEye.z      = 0.0f;
	mInterest.x = 0.0f;
	mInterest.y = 0.0f;
	mInterest.z = 0.0f;
	mAngle.x    = 0.0f;
	mAngle.y    = 0.0f;
	mAngle.z    = 0.0f;
	mUp.x       = 0.0f;
	mUp.y       = 1.0f;
	mUp.z       = 0.0f;
	SetScale(1.0f, 1.0f, 1.0f);
	SetViewport(0.0f, 0.0f, rmode);
	SetScissor(rmode);
	mLongestAObj = NULL;
}

/**
 * @note Address: 0x80193190
 * @note Size: 0x164
 */
void CCamera::Init(f32 near, f32 far, GXRenderModeObj* rmode)
{
	HSD_CObjDesc desc = { NULL, 0, PROJ_ORTHO, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, NULL, NULL, 0.0f, NULL, near, far };

	mUseMtx     = FALSE;
	mLoadedCObj = NULL;
	HSD_CObjInit(this, &desc);
	mEye.x      = 0.0f;
	mEye.y      = 0.0f;
	mEye.z      = 0.0f;
	mInterest.x = 0.0f;
	mInterest.y = 0.0f;
	mInterest.z = 0.0f;
	mAngle.x    = 0.0f;
	mAngle.y    = 0.0f;
	mAngle.z    = 0.0f;
	mUp.x       = 0.0f;
	mUp.y       = 1.0f;
	mUp.z       = 0.0f;
	SetScale(1.0f, 1.0f, 1.0f);
	SetViewport(0.0f, 0.0f, rmode);
	SetScissor(rmode);
	mLongestAObj = NULL;
}

/**
 * @note Address: 0x80192FA0
 * @note Size: 0x1F0
 */
void CCamera::Init(HSD_CObjDesc* desc)
{
	mLoadedCObj      = HSD_CObjLoadDesc(desc);
	*(HSD_CObj*)this = *mLoadedCObj;
	HSD_CObjGetEyePosition(this, &mEye);
	HSD_CObjGetInterest(this, &mInterest);
	if (HSD_CObjGetFlags(this) & 1) {
		HSD_CObjGetUpVector(this, &mUp);
	} else {
		mRoll = GetCameraRoll(this);
	}
	CalcAngle();
	CalcDistance();
	mFov = HSD_CObjGetFov(this);
	SetScale(1.0f, 1.0f, 1.0f);
	mLongestAObj = NULL;
}

/**
 * @note Address: 0x80192DCC
 * @note Size: 0x1D4
 */
void CCamera::Release()
{
	if (mLoadedCObj != NULL) {
		// hand the current state back so the release frees what we're using now
		*mLoadedCObj = *(HSD_CObj*)this;
		HSD_CObjUnref(mLoadedCObj);
		mLoadedCObj = NULL;
	}
}

/**
 * @note Address: 0x80192CC0
 * @note Size: 0x10C
 */
void CCamera::SetAnim(HSD_CameraAnim* anim, f32 frame)
{
	HSD_AObj* aobj;
	f32 end;
	f32 endFrame;

	HSD_CObjAddAnim(this, anim);
	HSD_CObjReqAnim(this, frame);

	end  = 0.0f;
	aobj = this->aobj;
	if (aobj != NULL && (endFrame = HSD_AObjGetEndFrame(aobj)) > end) {
		end          = endFrame;
		mLongestAObj = aobj;
	}
	if (eye_position != NULL && (aobj = eye_position->aobj) != NULL && (endFrame = HSD_AObjGetEndFrame(aobj)) > end) {
		end          = endFrame;
		mLongestAObj = aobj;
	}
	if (interest != NULL && (aobj = interest->aobj) != NULL && (endFrame = HSD_AObjGetEndFrame(aobj)) > end) {
		mLongestAObj = aobj;
	}
}

/**
 * @note Address: 0x80192C1C
 * @note Size: 0xA4
 */
void CCamera::Anim()
{
	HSD_CObjAnim(this);
	HSD_CObjGetEyePosition(this, &mEye);
	HSD_CObjGetInterest(this, &mInterest);
	if (HSD_CObjGetFlags(this) & 1) {
		HSD_CObjGetUpVector(this, &mUp);
	} else {
		mRoll = GetCameraRoll(this);
	}
	CalcAngle();
	CalcDistance();
	mFov = HSD_CObjGetFov(this);
}

/**
 * @note Address: 0x80192AD0
 * @note Size: 0x14C
 */
void CCamera::SetViewport(f32 x, f32 y, GXRenderModeObj* rmode)
{
	s16 w                = rmode->fbWidth;
	s16 h                = rmode->efbHeight;
	HSD_RectF32 viewport = { x, x + w, y, y + h };
	f32 width;
	f32 height;

	HSD_CObjSetViewportf(this, &viewport);
	width  = rmode->fbWidth;
	height = rmode->viHeight;
	if (projection_type == PROJ_ORTHO) {
		projection_param.ortho.top    = y;
		projection_param.ortho.bottom = y + height;
		projection_param.ortho.left   = x;
		projection_param.ortho.right  = x + width;
	} else if (projection_type == PROJ_PERSPECTIVE) {
		projection_param.perspective.aspect = width / height;
	}
}

/**
 * @note Address: 0x80192A8C
 * @note Size: 0x44
 */
void CCamera::SetScissor(GXRenderModeObj* rmode)
{
	u16 height      = rmode->efbHeight;
	Scissor scissor = { 0, rmode->fbWidth, 0, height };

	HSD_CObjSetScissor(this, &scissor);
}

/**
 * @note Address: 0x801929AC
 * @note Size: 0xE0
 */
void CCamera::CalcEye()
{
	f32 y = HALF_PI + mAngle.y;
	f32 x = mAngle.x;

	mEye.x       = mInterest.x + mDistance * (f32)cos(y) * (f32)cos(x);
	f32 cosPitch = (f32)cos(x);
	f32 sinYaw   = (f32)sin(y);
	f32 zOffset  = mDistance * -sinYaw;
	mEye.z       = mInterest.z + zOffset * cosPitch;
	mEye.y       = mInterest.y + mDistance * (f32)sin(x);
}

/**
 * @note Address: 0x801928C4
 * @note Size: 0xE8
 */
void CCamera::CalcInterest()
{
	f32 y = HALF_PI + mAngle.y;
	f32 x = PI + mAngle.x;

	mInterest.x  = mEye.x + mDistance * (f32)cos(y) * (f32)cos(x);
	f32 cosPitch = (f32)cos(x);
	f32 sinYaw   = (f32)sin(y);
	f32 zOffset  = mDistance * -sinYaw;
	mInterest.z  = mEye.z + zOffset * cosPitch;
	mInterest.y  = mEye.y + mDistance * (f32)sin(x);
}
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stfd     f31, 0x30(r1)
    psq_st   f31, 0x38(r1), 0, qr0
    stfd     f30, 0x20(r1)
    psq_st   f30, 0x28(r1), 0, qr0
    stfd     f29, 0x10(r1)
    psq_st   f29, 0x18(r1), 0, qr0
    stw      r31, 0xc(r1)
    mr       r31, r3
    lfs      f1, lbl_8065F52C@sda21(r0)
    lfs      f0, 0xb4(r3)
    lfs      f2, lbl_8065F528@sda21(r0)
    fadds    f29, f1, f0
    lfs      f0, 0xb8(r3)
    fadds    f30, f2, f0
    fmr      f1, f29
    bl       cos
    frsp     f31, f1
    fmr      f1, f30
    bl       cos
    frsp     f3, f1
    lfs      f2, 0xe4(r31)
    lfs      f0, 0x94(r31)
    fmr      f1, f29
    fmuls    f2, f2, f3
    fmadds   f0, f2, f31, f0
    stfs     f0, 0xa4(r31)
    bl       cos
    frsp     f31, f1
    fmr      f1, f30
    bl       sin
    frsp     f2, f1
    lfs      f3, 0xe4(r31)
    lfs      f0, 0x9c(r31)
    fmr      f1, f29
    fneg     f2, f2
    fmuls    f2, f3, f2
    fmadds   f0, f2, f31, f0
    stfs     f0, 0xac(r31)
    bl       sin
    frsp     f2, f1
    lfs      f1, 0xe4(r31)
    lfs      f0, 0x98(r31)
    fmadds   f0, f1, f2, f0
    stfs     f0, 0xa8(r31)
    psq_l    f31, 0x38(r1), 0, qr0
    lfd      f31, 0x30(r1)
    psq_l    f30, 0x28(r1), 0, qr0
    lfd      f30, 0x20(r1)
    psq_l    f29, 0x18(r1), 0, qr0
    lfd      f29, 0x10(r1)
    lwz      r0, 0x44(r1)
    lwz      r31, 0xc(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: 0x8019265C
 * @note Size: 0x268
 */
void CCamera::CalcAngle()
{
	f32 dx    = mInterest.x - mEye.x;
	f32 dz    = mInterest.z - mEye.z;
	f32 dy    = mInterest.y - mEye.y;
	f32 angle = (f32)atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));

	mAngle.x = dy < 0.0f ? HALF_PI - angle : angle - HALF_PI;
	mAngle.y = (f32)atan2(mInterest.x - mEye.x, mInterest.z - mEye.z);
}

/**
 * @note Address: 0x801925D0
 * @note Size: 0x8C
 */
void CCamera::CalcUp()
{
	mUp.x = -(f32)cos(mAngle.y) * (f32)sin(mAngle.z);
	mUp.z = (f32)sin(mAngle.y) * (f32)sin(mAngle.z);
	mUp.y = (f32)cos(mAngle.z);
}

/**
 * @note Address: 0x80192598
 * @note Size: 0x38
 */
void CCamera::CalcDistance()
{
	mDistance = PSVECDistance(&mEye, &mInterest);
}

/**
 * @note Address: 0x801923B0
 * @note Size: 0x1E8
 */
MtxPtr CCamera::SetCurrent()
{
	MtxPtr mtx = NULL;

	if (mUseMtx) {
		if (HSD_CObjSetCurrent(this)) {
			mtx = mViewMtx;
		}
	} else if (mLoadedCObj != NULL) {
		HSD_CObjClearViewingMtx(this);
		HSD_CObjSetEyePosition(this, &mEye);
		HSD_CObjSetInterest(this, &mInterest);
		if (HSD_CObjSetCurrent(this)) {
			HSD_CObjGetViewingMtx(this, mViewMtx);
			mtx = mViewMtx;
		}
	} else {
		Vec eye      = mEye * mScale;
		Vec interest = mInterest * mScale;

		C_MTXLookAt(mViewMtx, &eye, &mUp, &interest);
		PSMTXConcat(mViewMtx, mScaleMtx, mViewMtx);
		HSD_CObjSetViewingMtx(this, mViewMtx);
		if (HSD_CObjSetCurrent(this)) {
			mtx = mViewMtx;
		}
	}

	if (mtx != NULL) {
		GXGetProjectionv(mProjection);
		GXGetViewportv(mViewport);
		return mtx;
	}
	return NULL;
}

/**
 * @note Address: 0x80192390
 * @note Size: 0x20
 */
void CCamera::EndCurrent()
{
	HSD_CObjEndCurrent();
}

// the class's inline methods that only this file emits

/**
 * @note Address: 0x80192340
 * @note Size: 0x50
 */
// ClearMtx__7CCameraFv

/**
 * @note Address: 0x80192338
 * @note Size: 0x8
 */
// SetAngleZ__7CCameraFf

/**
 * @note Address: 0x80192328
 * @note Size: 0x10
 */
// SetUp__7CCameraFfff

/**
 * @note Address: 0x801922F0
 * @note Size: 0x38
 */
// SetUp__7CCameraFRC3Vec

/**
 * @note Address: 0x801922D0
 * @note Size: 0x20
 */
// SetScissor__7CCameraFP8_Scissor

/**
 * @note Address: 0x801922B0
 * @note Size: 0x20
 */
// SetViewport__7CCameraFP12_HSD_RectF32

/**
 * @note Address: 0x80192290
 * @note Size: 0x20
 */
// SetViewport__7CCameraFP12_HSD_RectS16

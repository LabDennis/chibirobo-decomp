// the scene camera's file: the CCamera virtuals the game defines here, the
// XObjScnCam/XObjLight/XObjMap overrides in 0x800C6584-0x800C7DC0 (wii_units.md Nr. 58),
// and the CustomShadow ctor/draw pair
#include "Version.h"

#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf

#include "nw4r/g3d/g3d_camera.h" // revolution/gx.h has to win over dolphin's here

#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/xobjcamera.h"
#include "game/globals.h"
#include "game/ut.h"

#include "arithmetic.h"

#include "Dolphin/mtx.h"

#include <math.h>

extern MEMAllocator gWiiPadAllocator;
extern f32 gScreenWidthScale;

extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* srcBase, Vec* dstBase, u32 count);

// nw4r's db pointer check (db_assert.h) as the game's sources use it; the line is the game's own
#define NW4R_ASSERT_VALID_PTR_LINE(line, exp)                                                                                              \
	((void)((((u32)(exp) & 0xFF000000) == 0x80000000 || ((u32)(exp) & 0xFF800000) == 0x81000000 || ((u32)(exp) & 0xF8000000) == 0x90000000 \
	         || ((u32)(exp) & 0xFF000000) == 0xC0000000 || ((u32)(exp) & 0xFF800000) == 0xC1000000                                         \
	         || ((u32)(exp) & 0xF8000000) == 0xD0000000 || ((u32)(exp) & 0xFFFFC000) == 0xE0000000)                                        \
	        || (nw4r::db::Panic(__FILE__, line, "NW4R:Pointer Error\n" #exp "(=%p) is not valid pointer.", (exp)), 0)))

extern "C" void fn_800C659C(XObjScnCamInitRec* self, XObj* obj)
{
	self->m_08      = obj;
	self->mSnap[1]  = 0;
	self->mSnap[0]  = 0;
	self->m_0C      = NULL;
	self->mTarget.x = NAN;
	self->m_20      = NULL;
	self->m_24 = self->m_28 = NAN;
	self->m_2C = self->m_30 = 0.0f;
	self->m_34 = self->m_38 = 0.0f;
	self->m_3C.x            = 0.0f;
	self->m_3C.y            = 0.0f;
	self->m_3C.z            = 0.0f;
	self->m_4C.x            = 0.0f;
	self->m_4C.y            = 0.0f;
	self->m_4C.z            = 0.0f;
	self->m_5C.x            = 0.0f;
	self->m_5C.y            = 0.0f;
	self->m_5C.z            = 0.0f;
}

// the head bone's world position, or the object's own position when the model has no
// "head_chn" node; the second argument's role is not recovered
extern "C" CVec fn_800C6608(u32, XObj* obj)
{
	if (obj->mObject.m_pScnMdl != NULL) {
		nw4r::g3d::ResNode node = obj->GetResNode("head_chn");
		if (node.IsValid()) {
			return obj->GetWorldPos(node);
		}
	}
	return *obj->GetPos();
}

extern "C" CVec fn_800C6CEC(XObjScnCamInitRec* rec, XObj* obj);

// NONMATCHING: the aim-angle helper the trackers below share: *outA gets the yaw from
// head to the tracked target, *outB the pitch. Target selection, fallback angles, wrapping
// and clamping match retail; the CVec temporary layout and FPR scheduling still differ.
extern "C" void fn_800C66C8(XObjScnCamInitRec* rec, Vec* head, Vec* rot, f32* outA, f32* outB)
{
	if (rec->m_0C != NULL) {
		const CVec* targetPos = rec->m_0C->GetPos();
		*outA                 = (f32)atan2(targetPos->x - head->x, targetPos->z - head->z) - rot->y;
		CVec at               = fn_800C6CEC(rec, rec->m_0C);
		if (at.y == rec->m_0C->GetPos()->y) {
			at.y = head->y;
		}
		f32 dz    = at.z - head->z;
		f32 dx    = at.x - head->x;
		f32 dy    = at.y - head->y;
		f32 pitch = atan2(FSqrt(dz * dz + dx * dx), FSqrt(dy * dy));
		*outB     = dy < 0.0f ? pitch - HALF_PI : HALF_PI - pitch;
	} else if (__fpclassifyf(rec->mTarget.x) != FP_NAN) {
		*outA     = (f32)atan2(rec->mTarget.x - head->x, rec->mTarget.z - head->z) - rot->y;
		CVec at   = rec->mTarget;
		f32 dz    = at.z - head->z;
		f32 dx    = at.x - head->x;
		f32 dy    = at.y - head->y;
		f32 pitch = atan2(FSqrt(dz * dz + dx * dx), FSqrt(dy * dy));
		*outB     = dy < 0.0f ? pitch - HALF_PI : HALF_PI - pitch;
	} else if (rec->m_20 != NULL) {
		*outA     = (f32)atan2(rec->m_20->mView.mEye.x - head->x, rec->m_20->mView.mEye.z - head->z) - rot->y;
		CVec at   = rec->m_20->mView.mEye;
		f32 dz    = at.z - head->z;
		f32 dx    = at.x - head->x;
		f32 dy    = at.y - head->y;
		f32 pitch = atan2(FSqrt(dz * dz + dx * dx), FSqrt(dy * dy));
		*outB     = dy < 0.0f ? pitch - HALF_PI : HALF_PI - pitch;
	} else {
		*outA = __fpclassifyf(rec->m_28) != FP_NAN ? rec->m_28 : 0.0f;
		*outB = __fpclassifyf(rec->m_24) != FP_NAN ? rec->m_24 : 0.0f;
	}
	if (rec->m_30 > 0.0f) {
		f32 angle = WrapAngle(*outA);
		if (angle > PI) {
			angle -= TWO_PI;
		}
		if (angle > rec->m_30) {
			angle = rec->m_30;
		} else if (angle < -rec->m_30) {
			angle = -rec->m_30;
		}
		*outA = angle;
	}
	if (rec->m_2C > 0.0f) {
		f32 angle = WrapAngle(*outB);
		if (angle > PI) {
			angle -= TWO_PI;
		}
		if (angle > rec->m_2C) {
			angle = rec->m_2C;
		} else if (angle < -rec->m_2C) {
			angle = -rec->m_2C;
		}
		*outB = angle;
	}
}

extern "C" CVec fn_800C6CEC(XObjScnCamInitRec* rec, XObj* obj)
{
	return fn_800C6608((u32)rec, obj);
}

// NONMATCHING: per-frame yaw/pitch tracker: re-aims m_4C.x/.y at the target, then eases
// m_3C.x/.y toward them by m_5C.x/.y * frames, or snaps when the counter protocol runs
extern "C" void fn_800C6CF0(XObjScnCamInitRec* rec, u32 frames)
{
	CVec head = fn_800C6CEC(rec, rec->m_08);
	CVec rot  = rec->m_08->mMoveRotate;
	f32 outB;
	f32 outA;
	fn_800C66C8(rec, &head, &rot, &outA, &outB);
	rec->m_4C.y = outA + rec->m_38;
	rec->m_4C.x = outB + rec->m_34;
	f32* cur    = &rec->m_3C.x;
	f32* target = &rec->m_4C.x;
	f32* speed  = &rec->m_5C.x;
	for (int i = 0; i < 2; i++) {
		*cur     = WrapAngle(*cur);
		*target  = WrapAngle(*target);
		f32 diff = __fabsf(*target - *cur);
		if (diff > 0.0f) {
			if (diff >= TWO_PI) {
				diff -= TWO_PI;
			}
			if (diff > PI) {
				if (*target < *cur) {
					*target += TWO_PI;
				} else {
					*target -= TWO_PI;
				}
			}
		}
		if (diff >= 0.001f) {
			*speed = 0.1f * __fabsf(*target - *cur);
			if (*speed < 0.01f) {
				*speed = 0.01f;
			}
			if (*cur < *target) {
				*cur += *speed * (f32)frames;
				if (*cur > *target) {
					*cur = *target;
				}
			} else if (*cur > *target) {
				*cur -= *speed * (f32)frames;
				if (*cur < *target) {
					*cur = *target;
				}
			}
		} else {
			if (rec->mSnap[i] > 0) {
				if (rec->mSnap[i] == 2) {
					rec->mSnap[i] = 0;
				} else {
					rec->mSnap[i] = -rec->mSnap[i];
				}
			}
			*speed = 0.0f;
			*cur   = *target;
		}
		cur++;
		target++;
		speed++;
	}
}

/**
 * @note Address: 0x800C6F60
 * @note Size: 0x1D0
 */
CustomShadow::CustomShadow()
{
	mCenter.x = 0.0f;
	mCenter.z = 0.0f;
	mCenter.y = 0.0f;
	for (int i = 0; i < 33; i++) {
		mCircle[i].x = (f32)cos(i * 0.19634955f);
		mCircle[i].z = (f32)sin(i * 0.19634955f);
		mCircle[i].y = 0.0f;
	}
	for (int i = 0; i < 33; i++) {
		mRing[2 * i].x     = (f32)cos(i * 0.19634955f);
		mRing[2 * i].z     = (f32)sin(i * 0.19634955f);
		mRing[2 * i].y     = 0.0f;
		mRing[2 * i + 1].x = (f32)cos(i * 0.19634955f) * 2.0f;
		mRing[2 * i + 1].z = (f32)sin(i * 0.19634955f) * 2.0f;
		mRing[2 * i + 1].y = 0.0f;
		mColors[2 * i + 1] = i;
	}
}

/**
 * @note Address: 0x800C7130
 * @note Size: 0x20C
 */
void CustomShadow::Draw(Vec pos, Vec scale, u32 alpha)
{
	Mtx trans;
	Mtx scaleM;
	Mtx mtx;
	PSMTXTrans(trans, pos.x, pos.y, pos.z);
	PSMTXScale(scaleM, scale.x, scale.y, scale.z);
	PSMTXConcat(trans, scaleM, mtx);
	PSMTXMultVecArray(mtx, &mCenter, mFan, 34);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, mFan, alpha, 34);
	for (int i = 0; i < 33; i++) {
		mColors[i * 2] = alpha;
	}
	PSMTXMultVecArray(mtx, mRing, mRingTransformed, 66);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, mRingTransformed, mColors, 66);
}

/**
 * @note Address: 0x800C733C
 * @note Size: 0x1A8
 */
// NONMATCHING: the staged scene check is retail's shape (two flag values, the second read off the first before the ScnRoot construct); the
// residue is register choice (flag regs swapped, two saved regs instead of three), the placement of the FALSE return and the assert's
// string addressing
BOOL XObjScnCam::LoadFromPlacement(const CoordInfo& info)
{
	if (!XObj::LoadFromPlacement(info)) {
		return FALSE;
	}
	BOOL ready = FALSE;
	BOOL scn   = mObject.m_pScnMdl != NULL && mObject.m_resFile.HasResAnmScn();
	if (scn && mObject.m_resFile.GetResAnmScnNumEntries() != 0) {
		ready = TRUE;
	}
	if (ready) {
		mScene = (WiiGsSceneObj*)nw4r::g3d::ScnRoot::Construct(&gWiiPadAllocator, NULL, 1, 1, 1, 1);
		NW4R_ASSERT_VALID_PTR_LINE(0x20, mScene);
	}
	return TRUE;
}

/**
 * @note Address: 0x800C74E4
 * @note Size: 0x98
 */
void XObjScnCam::Release()
{
	if (mScene != NULL) {
		gGame.mMotionControlCamera.vf2C();
		gGame.mMotionControlCamera.vf68();
		ClearAnim();
		((nw4r::g3d::ScnRoot*)mScene)->Destroy();
		mScene = NULL;
	}
	XObj::Release();
}

/**
 * @note Address: 0x800C757C
 * @note Size: 0x14
 */
void CCamera::vf2C()
{
	mProjectionSource = NULL;
	mProjectionDirty  = TRUE;
}

/**
 * @note Address: 0x800C7590
 * @note Size: 0x14
 */
void CCamera::vf68()
{
	mViewSource  = NULL;
	mView.mDirty = TRUE;
}

/**
 * @note Address: 0x800C75A4
 * @note Size: 0x4C
 */
void XObjScnCam::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	if (mScene != NULL) {
		((nw4r::g3d::ScnRoot*)mScene)->CalcWorld();
		((nw4r::g3d::ScnRoot*)mScene)->CalcView();
		((nw4r::g3d::ScnRoot*)mScene)->UpdateFrame();
	}
}

/**
 * @note Address: 0x800C75F0
 * @note Size: 0x4
 */
void XObjScnCam::DrawObjectEffects()
{
}

/**
 * @note Address: 0x800C75F4
 * @note Size: 0xFC
 */
// NONMATCHING: the discarded else-call of GetResAnmScnNumEntries is what retail emits;
// whether the source asked for the count in an assertion or a check is not known
void XObjScnCam::SetAnmScn(u32 id, f32 frame, f32 rate)
{
	if (mScene != NULL) {
		if (id < mObject.m_resFile.GetResAnmScnNumEntries()) {
			CObj::SetAnmScn(id, frame, rate);
			BOOL has = mObject.m_resFile.HasResAnmScn();
			((nw4r::g3d::ScnRoot*)mScene)->SetAnmScn(has ? mObject.m_resources.scn : NULL);
			((nw4r::g3d::ScnRoot*)mScene)->CalcWorld();
			((nw4r::g3d::ScnRoot*)mScene)->CalcView();
			Vec zero    = { 0, 0, 0 };
			mAnimOffset = zero;
			mAnimAngle  = 0.0f;
		} else {
			mObject.m_resFile.GetResAnmScnNumEntries();
		}
	}
}

/**
 * @note Address: 0x800C76F0
 * @note Size: 0x74
 */
void XObjScnCam::ReleaseModelWork()
{
	if (mScene != NULL) {
		((nw4r::g3d::ScnRoot*)mScene)->RemoveAnmScn();
		mObject.ReleaseWork();
	}
	gGame.mMotionControlCamera.vf2C();
	gGame.mMotionControlCamera.vf68();
}

/**
 * @note Address: 0x800C7764
 * @note Size: 0x4B8
 */
// NONMATCHING: copies the scene's current nw4r camera into the game camera each frame:
// projection, the view matrix offset by mAnimOffset/mAnimAngle, eye, posture and viewport. first
// honest pass, the reader's matrix slot is not modelled
extern "C" void fn_800C7764(XObjScnCam* self, CCamera* camera)
{
	BOOL active = FALSE;
	if ((self->mStatus.mFlags & 0x1ULL) && self->mScene != NULL && self->mScene->_2888 != NULL) {
		active = TRUE;
	}
	if (!active) {
		return;
	}
	// GetCurrentCamera hands back the camera object's CameraData*; the caller keeps it in
	// a slot that is the Camera object's whole layout ({mpData})
	MtxP handle            = ((nw4r::g3d::ScnRoot*)self->mScene)->GetCurrentCamera();
	nw4r::g3d::Camera& cam = *(nw4r::g3d::Camera*)&handle;
	// upstream rescommon's ref() asserts on a NULL mpData; our copy dropped the check,
	// the retail TU has it inlined at the first camera data read
	if (cam.ptr() == NULL) {
		nw4r::db::Panic("g3d_rescommon_ac.h", 0x8C, "NW4R:Pointer must not be NULL (mpData)");
	}
	nw4r::g3d::CameraData& data = cam.ref();
	if (data.flags & nw4r::g3d::CameraData::FLAG_PROJ_ORTHO) {
		camera->mType             = 2;
		camera->mTop              = data.projTop;
		camera->mBottom           = data.projBottom;
		camera->mLeft             = data.projLeft * gScreenWidthScale;
		camera->mRight            = data.projRight * gScreenWidthScale;
		camera->mNear             = data.projNear;
		camera->mFar              = data.projFar;
		camera->mProjectionDirty  = TRUE;
		camera->mIsOrtho          = 1.0f;
		camera->mProjectionSource = NULL;
		camera->mScale            = gScreenWidthScale;
	} else if (data.flags & nw4r::g3d::CameraData::FLAG_PROJ_FRUSTUM) {
		camera->mType             = 1;
		camera->mTop              = data.projTop;
		camera->mBottom           = data.projBottom;
		camera->mLeft             = data.projLeft * gScreenWidthScale;
		camera->mRight            = data.projRight * gScreenWidthScale;
		camera->mNear             = data.projNear;
		camera->mFar              = data.projFar;
		camera->mProjectionDirty  = TRUE;
		camera->mIsOrtho          = 0.0f;
		camera->mProjectionSource = NULL;
		camera->mScale            = gScreenWidthScale;
	} else {
		camera->mType             = 0;
		camera->mFov              = data.projFovy;
		camera->mAspect           = data.projAspect * gScreenWidthScale;
		camera->mNear             = data.projNear;
		camera->mFar              = data.projFar;
		camera->mProjectionDirty  = TRUE;
		camera->mIsOrtho          = 0.0f;
		camera->mProjectionSource = NULL;
		camera->mScale            = gScreenWidthScale;
	}
	static BOOL sViewMtxInit;
	if (!sViewMtxInit) {
		sViewMtxInit = TRUE;
	}
	// the view matrix buffer the reader carries at _390; the class member is not modelled
	MtxP viewMtx = (MtxP)((u8*)&gSingleObjectReader + 0x390);
	cam.GetCameraMtx((nw4r::math::MTX34*)viewMtx);
	Mtx trans;
	Mtx rot;
	Mtx out;
	PSMTXTrans(trans, -self->mAnimOffset.x, -self->mAnimOffset.y, -self->mAnimOffset.z);
	PSMTXRotRad(rot, 'y', -self->mAnimAngle);
	PSMTXConcat(rot, trans, out);
	PSMTXConcat(viewMtx, out, viewMtx);
	camera->vf64(viewMtx);
	Vec pos;
	cam.GetPosition((nw4r::math::VEC3*)&pos);
	pos.x += self->mAnimOffset.x;
	pos.y += self->mAnimOffset.y;
	pos.z += self->mAnimOffset.z;
	camera->SetEye(pos);
	nw4r::g3d::Camera::PostureInfo info;
	cam.GetPosture(&info);
	switch (info.tp) {
	case nw4r::g3d::Camera::POSTURE_LOOKAT: {
		Vec target;
		target.x = info.cameraTarget.x + self->mAnimOffset.x;
		target.y = info.cameraTarget.y + self->mAnimOffset.y;
		target.z = info.cameraTarget.z + self->mAnimOffset.z;
		camera->SetInterest(target);
		camera->SetUp(*(const Vec*)&info.cameraUp);
		camera->CalcAngle();
		break;
	}
	case nw4r::g3d::Camera::POSTURE_ROTATE: {
		Vec angle;
		angle.x = info.cameraRotate.x * DEG2RAD;
		angle.y = info.cameraRotate.y * DEG2RAD + self->mAnimAngle;
		angle.z = info.cameraRotate.z * DEG2RAD;
		camera->SetAngle(angle);
		break;
	}
	case nw4r::g3d::Camera::POSTURE_AIM: {
		Vec target;
		target.x = info.cameraTarget.x + self->mAnimOffset.x;
		target.y = info.cameraTarget.y + self->mAnimOffset.y;
		target.z = info.cameraTarget.z + self->mAnimOffset.z;
		camera->SetInterest(target);
		Vec up     = { 0.0f, info.cameraTwist, 0.0f };
		CVec angle = camera->mView.mAngle + up;
		camera->SetAngle(angle);
		camera->CalcUp();
		break;
	}
	}
	f32 x;
	f32 y;
	f32 w;
	f32 h;
	f32 near;
	f32 far;
	cam.GetViewport(&x, &y, &w, &h, &near, &far);
	camera->vf88(x, y, w, h, near, far);
}

/**
 * @note Address: 0x800C7C1C
 * @note Size: 0x24
 */
void CCamera::SetUp(const Vec& up)
{
	mView.mUp.Set(up.x, up.y, up.z);
	mView.mDirty = TRUE;
}

/**
 * @note Address: 0x800C7C40
 * @note Size: 0xC
 */
void XObjScnCam::InitializeObject()
{
	mScene = NULL;
}

void XObjLight::InitializeObject()
{
	XObj::InitializeObject();
}

/**
 * @note Address: 0x800C7C58
 * @note Size: 0x30
 */
void XObjLight::ResetObjectState()
{
	m_9A8 = 0;
	for (int i = 0; i < 8; i++) {
		mLightIds[i] = -1;
	}
}

/**
 * @note Address: 0x800C7C88
 * @note Size: 0x2C
 */
BOOL XObjLight::LoadFromPlacement(const CoordInfo& info)
{
	return XObj::LoadFromPlacement(info) != FALSE;
}

void XObjLight::Release()
{
	XObj::Release();
}

/**
 * @note Address: 0x800C7CB8
 * @note Size: 0x4
 */
void XObjLight::UpdateObject(u32 frames)
{
}

/**
 * @note Address: 0x800C7CC4
 * @note Size: 0x34
 */
void XObjMap::InitializeObject()
{
	XObj::InitializeObject();
	m_1374 = 0;
}

/**
 * @note Address: 0x800C7CF8
 * @note Size: 0xC8
 */
void XObjMap::ResetObjectState()
{
	XObj::ResetObjectState();
	u32* fields = (u32*)_1378;
	for (int i = 0; i < 2; i++, fields += 16) {
		for (int k = 0; k < 8; k++) {
			if (fields[k * 2] != 0) {
				fields[k * 2] = 0;
			}
		}
	}
}

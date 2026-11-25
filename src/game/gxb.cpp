#include "game/cmdline.h"
#include "types.h"
#include "Version.h"
#include "game/hsd_jobj.h"
extern "C" {
#include <dolphin/mtx/vec.h>
#include <sysdolphin/video.h>
}
#include "game/cjobjset.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/cptcl.h"
#include "game/cscrn.h"
#include "game/cvec.h"
#include "game/screentransition.h"
#include "game/csound.h"
#include "game/globals.h"
#include "game/gmath.h"
#include "game/logo.h"
#include "game/video.h"
#include "game/xobjdemo.h"
#include "game/xobjs.h"
#include "Dolphin/OS/OSUtil.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Screen shake, particles, cursor and object transforms, and boot logos.

/**
 * @note Address: 0x800240EC
 * @note Size: 0x1BC
 */
extern "C" void UpdateScreenShake(CScrnQuake* quake, u32 frames)
{
	StepScreenTransition(quake->mShake[0].x, quake->mShake[1].x, quake->mShake[2].x, frames);
	StepScreenTransition(quake->mShake[0].y, quake->mShake[1].y, quake->mShake[2].y, frames);
	StepScreenTransition(quake->mShake[0].z, quake->mShake[1].z, quake->mShake[2].z, frames);
	quake->mSign = -quake->mSign;
}

/**
 * @note Address: 0x800242A8
 * @note Size: 0x184
 */
extern "C" void StartScreenShake(CScrnQuake* quake, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, f32 time)
{
	quake->mShake[0].Set(x0, y0, z0);
	quake->mShake[0].x = Abs(quake->mShake[0].x);
	quake->mShake[0].y = Abs(quake->mShake[0].y);
	quake->mShake[0].z = Abs(quake->mShake[0].z);
	quake->mShake[1].Set(x1, y1, z1);
	quake->mShake[1].x = Abs(quake->mShake[1].x);
	quake->mShake[1].y = Abs(quake->mShake[1].y);
	quake->mShake[1].z = Abs(quake->mShake[1].z);
	if (time == 0.0f) {
		quake->mShake[0].Set(quake->mShake[1]);
		quake->mShake[2].Set(0.0f, 0.0f, 0.0f);
	} else {
		quake->mShake[2].Set((quake->mShake[1] - quake->mShake[0]) / (FRAME_RATE * time));
	}
	quake->mSign = 1;
}

/**
 * @note Address: 0x8002442C
 * @note Size: 0x74
 */
CVec CVec::operator/(f32 divisor) const
{
	CVec result = *this;
	result.x /= divisor;
	result.y /= divisor;
	result.z /= divisor;
	return result;
}

/**
 * @note Address: 0x800244A0
 * @note Size: 0x84
 */
CVec CVec::operator-(Vec other) const
{
	CVec result = *this;
	PSVECSubtract(&result, &other, &result);
	return result;
}

/**
 * @note Address: 0x80024524
 * @note Size: 0x34
 */
extern "C" void ResetScreenShake(CScrnQuake* quake)
{
	quake->mShake[0].Set(0.0f, 0.0f, 0.0f);
	quake->mShake[1].Set(0.0f, 0.0f, 0.0f);
	quake->mShake[2].Set(0.0f, 0.0f, 0.0f);
	quake->mSign = 1;
}

/**
 * @note Address: 0x800245F0
 * @note Size: 0x10
 */
void CPtclOne::Init()
{
	mActive = FALSE;
	mTex    = NULL;
}

/**
 * @note Address: 0x80024600
 * @note Size: 0x1D8
 */
void CPtclDust3::Draw(CGs* gs)
{
	if (!mActive) {
		return;
	}
	f32 width = mSize.x * (f32)sin(0.8f * (PI * mAlpha));
	width *= 2.0f;
	f32 height = mSize.y * (f32)sin(0.8f * (PI * mAlpha));
	height *= 2.0f;
	u32 edgeColor = mColor & 0xFFFFFF00;
	u32 color     = (mColor & 0xFFFFFF00) | ((u32)(s32)((f32)(mColor & 0xFF) * mAlpha) & 0xFF);
	if (mTex != NULL) {
		SetGsTevMode(gs->mCurTevStage, 0);
		Vec rot = { 0.0f, 0.0f, 0.0f };
		rot.z   = mAngle;
		DrawGsTexturedQuad(gs, mTex, color, color, color, color, mPos.x, mPos.y, mPos.z, width, height, rot.x, rot.y, rot.z);
		SetGsTevMode(gs->mCurTevStage, 4);
	} else {
		Vec rot = { 0.0f, 0.0f, 0.0f };
		rot.z   = mAngle;
		DrawGsCircle(gs, 8, color, edgeColor, mPos.x, mPos.y, mPos.z, width, height, rot.x, rot.y, rot.z);
	}
}

/**
 * @note Address: 0x800247D8
 * @note Size: 0x15C
 */
void CPtclDust3::Update(u32 frames)
{
	if (!mActive) {
		return;
	}
	if (mAlphaStep < 0.0f) {
		if ((mAlpha -= mAlphaStep * (f32)frames) >= 1.0f) {
			mAlphaStep = -mAlphaStep;
		}
	}
	if (mAlphaStep > 0.0f) {
		if ((mAlpha -= mAlphaStep * (f32)frames) <= 0.0f) {
			mActive = FALSE;
			mAlpha  = 0.0f;
			return;
		}
	}
	mPos += mVel * (f32)frames;
	mAngle += mAngleSpeed;
}

/**
 * @note Address: 0x80024934
 * @note Size: 0x1E8
 */
void CPtclDust2::Draw(CGs* gs)
{
	if (!mActive) {
		return;
	}
	u32 color  = (mColor & 0xFFFFFF00) | ((u32)(s32)((f32)(mColor & 0xFF) * mAlpha) & 0xFF);
	f32 radius = 1.5f * (mSize.x > mSize.y ? mSize.x : mSize.y);
	if (mTex != NULL) {
		SetGsTevMode(gs->mCurTevStage, 0);
		Vec rot = { 0.0f, 0.0f, 0.0f };
		rot.z   = mAngle;
		DrawGsTexturedQuad(gs, mTex, color, color, color, color, mPos.x, mPos.y, mPos.z, radius, radius, rot.x, rot.y, rot.z);
		SetGsTevMode(gs->mCurTevStage, 4);
	} else {
		// two triangles make a star
		Vec first = { 0.0f, 0.0f, 0.0f };
		first.z   = mAngle;
		DrawGsCircle(gs, 3, color, color, mPos.x, mPos.y, mPos.z, radius, radius, first.x, first.y, first.z);
		Vec second = { 0.0f, 0.0f, 0.0f };
		second.z   = THIRD_PI + mAngle;
		DrawGsCircle(gs, 3, color, color, mPos.x, mPos.y, mPos.z, radius, radius, second.x, second.y, second.z);
	}
}

/**
 * @note Address: 0x80024B1C
 * @note Size: 0x184
 */
void CPtclDust2::Update(u32 frames)
{
	if (!mActive) {
		return;
	}
	if (mAlphaStep < 0.0f) {
		if ((mAlpha -= mAlphaStep * (f32)frames) >= 1.0f) {
			mAlphaStep = -mAlphaStep;
		}
	}
	if (mAlphaStep > 0.0f) {
		if ((mAlpha -= mAlphaStep * (f32)frames) <= 0.0f) {
			mActive = FALSE;
			mAlpha  = 0.0f;
			return;
		}
	}
	mVel.y -= 0.05f * (f32)frames;
	mPos += mVel * (f32)frames;
	mAngle += mAngleSpeed;
}

/**
 * @note Address: 0x80024CA0
 * @note Size: 0xE0
 */
extern "C" void DrawParticlePool(CPtcl* ptcl)
{
	CGs* gs = &gGame.mGs;
	SetGsChannelEnabled(gs, 0, 0);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
	gGame.mGs.mBillboardEnabled = 1;
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive != FALSE) {
			one->Draw(&gGame.mGs);
		}
	}
	gGame.mGs.mBillboardEnabled = 0;
}

/**
 * @note Address: 0x80024D80
 * @note Size: 0x4
 */
void CPtclOne::Draw(CGs* gs)
{
}

/**
 * @note Address: 0x80024D84
 * @note Size: 0x84
 */
extern "C" void UpdateParticlePool(CPtcl* ptcl, u32 frames)
{
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive != FALSE) {
			one->Update(frames);
		}
	}
}

/**
 * @note Address: 0x80024E08
 * @note Size: 0x4
 */
void CPtclOne::Update(u32 frames)
{
}

/**
 * @note Address: 0x80024E0C
 * @note Size: 0x44
 */
extern "C" void ClearParticlePool(CPtcl* ptcl)
{
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive != FALSE) {
			one->mActive = FALSE;
			one->mTex    = NULL;
		}
	}
}

/**
 * @note Address: 0x80024E50
 * @note Size: 0x15C
 */
extern "C" CPtclOne* SpawnPoolParticle(CPtcl* ptcl, const Vec* pos, const Vec* size, const Vec* vel, u32 color)
{
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive == FALSE) {
			one->mActive     = TRUE;
			one->mPos        = CVec(*pos);
			one->mVel        = CVec(*vel);
			one->mSize       = CVec(*size);
			one->mAngle      = (f32)fmod((f32)rand(), PI);
			one->mAngleSpeed = 0.0f;
			one->mColor      = color;
			one->mAlpha      = 1.0f;
			one->mAlphaStep  = 0.02f;
			one->mTex        = NULL;
			return one;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x80024FAC
 * @note Size: 0x84
 */
extern "C" void InitParticlePool(CPtcl* ptcl, void* ptcls, s32 stride, s32 count)
{
	ptcl->mPtcls   = (u8*)ptcls;
	ptcl->mStride  = stride;
	ptcl->mCount   = count;
	ptcl->mCounter = 0;
	for (s32 i = 0; i < count; ++i) {
		ptcl->Get(i)->Init();
	}
}

/**
 * @note Address: 0x80025150
 * @note Size: 0xD8
 */
// Cursor's SetScaleY
void Cursor::SetScaleY(f32 y, s32 force)
{
	y                    = -y;
	mMoveScale.y         = y;
	mMoveScale.mTarget.y = y;
	if (mJObj != NULL) {
		CJObj::SetScaleY(y, force);
	}
}

/**
 * @note Address: 0x80025228
 * @note Size: 0x190
 */
// Cursor's SetScale(scale)
void Cursor::SetScale(f32 scale, s32 force)
{
	f32 x = scale;
	f32 y = -scale;
	f32 z = scale;

	mMoveScale.x         = x;
	mMoveScale.y         = y;
	mMoveScale.z         = z;
	mMoveScale.mTarget.x = x;
	mMoveScale.mTarget.y = y;
	mMoveScale.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mScale != v)) {
				return;
			}
		}
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
}

/**
 * @note Address: 0x800253B8
 * @note Size: 0x190
 */
// Cursor's SetScale(x, y, z)
void Cursor::SetScale(f32 x, f32 y, f32 z, s32 force)
{
	y = -y;

	mMoveScale.x         = x;
	mMoveScale.y         = y;
	mMoveScale.z         = z;
	mMoveScale.mTarget.x = x;
	mMoveScale.mTarget.y = y;
	mMoveScale.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mScale != v)) {
				return;
			}
		}
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
}

/**
 * @note Address: 0x80025548
 * @note Size: 0x19C
 */
// Cursor's SetScale(v)
void Cursor::SetScale(const Vec& scale, s32 force)
{
	f32 x = scale.x;
	f32 y = -scale.y;
	f32 z = scale.z;

	mMoveScale.x         = x;
	mMoveScale.y         = y;
	mMoveScale.z         = z;
	mMoveScale.mTarget.x = x;
	mMoveScale.mTarget.y = y;
	mMoveScale.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mScale != v)) {
				return;
			}
		}
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
}

// animation ids from 1000 up are in the object's own anim archive once that is read, the
// rest are in its model archive. the last three digits pick the animation
static inline bool UseCustomAnimationArchive(const XObj* object, s32 id)
{
	return id >= 1000 && object->mAnim.mArchive != NULL;
}

/**
 * @note Address: 0x800256E4
 * @note Size: 0x38
 */
// Cursor's SetAnim, keeps the classical scale
void Cursor::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	XObj::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
	HSD_JObjSetFlags(mJObj, CLASSICAL_SCALE);
}

/**
 * @note Address: 0x8002571C
 * @note Size: 0x4
 */
// XObj's ResetForNewGame
void XObj::ResetForNewGame()
{
}

/**
 * @note Address: 0x80025720
 * @note Size: 0x24
 */
// XObj's LoadAnimationArchiveAsync
BOOL XObj::LoadAnimationArchiveAsync(const char* path)
{
	return LoadJointAnimationAsync(&mAnim, path);
}

/**
 * @note Address: 0x80025744
 * @note Size: 0x4
 */
// XObj's DrawDebugStatus
void XObj::DrawDebugStatus()
{
}

/**
 * @note Address: 0x80025748
 * @note Size: 0x1C
 */
// XObj's GetCameraTarget
Vec XObj::GetCameraTarget()
{
	return mMoveTranslate;
}

/**
 * @note Address: 0x80025764
 * @note Size: 0xF4
 */
// XObj's GetJointAnimation
HSD_AnimJoint* XObj::GetJointAnimation(s32 id)
{
	HSD_AnimJoint** animations = UseCustomAnimationArchive(this, id) ? mAnim.mAnimJoint : mArchive.mAnimJoint;
	return animations != NULL ? animations[id % 1000] : NULL;
}

/**
 * @note Address: 0x80025858
 * @note Size: 0xF4
 */
// XObj's GetMaterialAnimation
HSD_MatAnimJoint* XObj::GetMaterialAnimation(s32 id)
{
	HSD_MatAnimJoint** animations = UseCustomAnimationArchive(this, id) ? mAnim.mMatAnimJoint : mArchive.mMatAnimJoint;
	return animations != NULL ? animations[id % 1000] : NULL;
}

/**
 * @note Address: 0x8002594C
 * @note Size: 0xF4
 */
// XObj's GetShapeAnimation
HSD_ShapeAnimJoint* XObj::GetShapeAnimation(s32 id)
{
	HSD_ShapeAnimJoint** animations = UseCustomAnimationArchive(this, id) ? mAnim.mShapeAnimJoint : mArchive.mShapeAnimJoint;
	return animations != NULL ? animations[id % 1000] : NULL;
}

/**
 * @note Address: 0x80025A40
 * @note Size: 0x8
 */
// XObj's OnPlayerContact
s32 XObj::OnPlayerContact(const CVec&, s32 value)
{
	return value;
}

/**
 * @note Address: 0x80025A48
 * @note Size: 0x50
 */
s32 XObj::HandleEventCommand(s32, s32*)
{
	// Derived objects handle their own commands. The base implementation returns zero.
	return 0;
}

/**
 * @note Address: 0x80025A98
 * @note Size: 0x8
 */
u8* XObj::GetEventScript()
{
	return mEventScript;
}

/**
 * @note Address: 0x80025AA0
 * @note Size: 0x4
 */
void XObj::OnPushDisplacement(const Vec& displacement)
{
}

/**
 * @note Address: 0x80025AA4
 * @note Size: 0xD8
 */
// CXObj's SetTranslateX
void CXObj::SetTranslateX(f32 x, s32 force)
{
	mMoveTranslate.x         = x;
	mMoveTranslate.mTarget.x = x;
	if (mJObj != NULL) {
		CJObj::SetTranslateX(x, force);
	}
}

/**
 * @note Address: 0x80025B7C
 * @note Size: 0xD8
 */
// CXObj's SetTranslateY
void CXObj::SetTranslateY(f32 y, s32 force)
{
	mMoveTranslate.y         = y;
	mMoveTranslate.mTarget.y = y;
	if (mJObj != NULL) {
		CJObj::SetTranslateY(y, force);
	}
}

/**
 * @note Address: 0x80025C54
 * @note Size: 0xD8
 */
// CXObj's SetTranslateZ
void CXObj::SetTranslateZ(f32 z, s32 force)
{
	mMoveTranslate.z         = z;
	mMoveTranslate.mTarget.z = z;
	if (mJObj != NULL) {
		CJObj::SetTranslateZ(z, force);
	}
}

/**
 * @note Address: 0x80025D2C
 * @note Size: 0xD8
 */
// CXObj's SetScaleX
void CXObj::SetScaleX(f32 x, s32 force)
{
	mMoveScale.x         = x;
	mMoveScale.mTarget.x = x;
	if (mJObj != NULL) {
		CJObj::SetScaleX(x, force);
	}
}

/**
 * @note Address: 0x80025E04
 * @note Size: 0xD8
 */
// CXObj's SetScaleZ
void CXObj::SetScaleZ(f32 z, s32 force)
{
	mMoveScale.z         = z;
	mMoveScale.mTarget.z = z;
	if (mJObj != NULL) {
		CJObj::SetScaleZ(z, force);
	}
}

/**
 * @note Address: 0x80025EDC
 * @note Size: 0x1B0
 */
// CXObj's SetRotationX
void CXObj::SetRotationX(f32 x, s32 force)
{
	mMoveRotate.x         = x;
	mMoveRotate.mTarget.x = x;
	if (mJObj != NULL) {
		CJObj::SetRotationX(x, force);
	}
}

/**
 * @note Address: 0x8002608C
 * @note Size: 0x1B0
 */
// CXObj's SetRotationY
void CXObj::SetRotationY(f32 y, s32 force)
{
	mMoveRotate.y         = y;
	mMoveRotate.mTarget.y = y;
	if (mJObj != NULL) {
		CJObj::SetRotationY(y, force);
	}
}

/**
 * @note Address: 0x8002623C
 * @note Size: 0x1B0
 */
// CXObj's SetRotationZ
void CXObj::SetRotationZ(f32 z, s32 force)
{
	mMoveRotate.z         = z;
	mMoveRotate.mTarget.z = z;
	if (mJObj != NULL) {
		CJObj::SetRotationZ(z, force);
	}
}

/**
 * @note Address: 0x800263EC
 * @note Size: 0x190
 */
// CXObj's SetPos(x, y, z)
void CXObj::SetPos(f32 x, f32 y, f32 z, s32 force)
{
	mMoveTranslate.x         = x;
	mMoveTranslate.y         = y;
	mMoveTranslate.z         = z;
	mMoveTranslate.mTarget.x = x;
	mMoveTranslate.mTarget.y = y;
	mMoveTranslate.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mTranslate != v)) {
				return;
			}
		}
		mTranslate.x = x;
		mTranslate.y = y;
		mTranslate.z = z;
		HSD_JObjSetTranslate(mJObj, &mTranslate);
	}
}

/**
 * @note Address: 0x8002657C
 * @note Size: 0x17C
 */
// CXObj's SetPos(pos)
void CXObj::SetPos(const Vec& v, s32 force)
{
	mMoveTranslate.Set(v);
	mMoveTranslate.mTarget.Set(v);
	if (mJObj) {
		CJObj::SetTranslate(v, force);
	}
}

/**
 * @note Address: 0x800266F8
 * @note Size: 0x240
 */
// CXObj's SetRot(x, y, z)
void CXObj::SetRot(f32 x, f32 y, f32 z, s32 force)
{
	mMoveRotate.x         = x;
	mMoveRotate.y         = y;
	mMoveRotate.z         = z;
	mMoveRotate.mTarget.x = x;
	mMoveRotate.mTarget.y = y;
	mMoveRotate.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			// tests the cached translation, as CJObj's does
			if (!(mTranslate != v)) {
				return;
			}
		}
		mRotate.x = x;
		mRotate.y = y;
		mRotate.z = z;
		UpdateRotation();
	}
}

/**
 * @note Address: 0x80026938
 * @note Size: 0x22C
 */
// CXObj's SetRot(rot)
void CXObj::SetRot(const Vec& v, s32 force)
{
	mMoveRotate.Set(v);
	mMoveRotate.mTarget.Set(v);
	if (mJObj) {
		CJObj::SetRotation(v, force);
	}
}

/**
 * @note Address: 0x80026B64
 * @note Size: 0x38
 */
// Cursor's DrawObject
void Cursor::DrawObject()
{
	if (mJObj != NULL) {
		HSD_JObjDispAll(mJObj, NULL, 7, 0);
	}
}

/**
 * @note Address: 0x80026B9C
 * @note Size: 0x90
 */
// Cursor's update, spins it
void Cursor::UpdateObject(u32 frames)
{
	mAngle = fmod(mAngleSpeed * (f32)frames + mAngle, TWO_PI);
	SetRotationZ(mAngle, 0);
	XObj::UpdateObject(frames);
}

/**
 * @note Address: 0x80026C2C
 * @note Size: 0x4C
 */
BOOL Cursor::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		HSD_JObjSetFlags(mJObj, CLASSICAL_SCALE);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80026C78
 * @note Size: 0x38
 */
void Cursor::InitializeObject(u32 id)
{
	XObj::InitializeObject(id);
	mAngleSpeed = 0.0f;
	mAngle      = 0.0f;
}

/**
 * @note Address: 0x80026CB0
 * @note Size: 0x878
 */
// NONMATCHING: float regalloc
// Logo::Draw, the current logo centred on the screen at its alpha. the first one has a shine
// that comes and goes with the angle
extern "C" void DrawCurrentLogo(Logo* logo)
{
	if (logo->IsActive() && logo->mAlpha > 0.0f) {
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
		SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 0);
		SetGsAlphaCompare(gGame.mGs.mCurRenderMode, 4, 0, 0, 3, 255);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		CTexObj tex;
		if (logo->mState == 3 || logo->mState == 4 || logo->mState == 5) {
			tex       = logo->mTpl.mTex.Init(logo->GetDesc(3), 0);
			u16 w     = GXGetTexObjWidth(&tex);
			f32 h     = GXGetTexObjHeight(&tex);
			u32 color = (u32)(255.0f * logo->mAlpha) | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, 224.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);

			tex        = logo->mTpl.mTex.Init(logo->GetDesc(4), 0);
			f32 shineW = GXGetTexObjWidth(&tex);
			f32 shineH = GXGetTexObjHeight(&tex);
			f32 y      = shineH / 2.0f + 359.0f;
			f32 shine  = sin(logo->mAngle);
			if (shine < 0.0f) {
				shine = 0.0f;
			}
			color = (u32)(255.0f * logo->mAlpha * shine) | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, y, 0.0f, shineW, shineH, 0.0f, 0.0f, 0.0f);
		}
		if (logo->mState == 6 || logo->mState == 7 || logo->mState == 8) {
			tex   = logo->mTpl.mTex.Init(logo->GetDesc(0), 0);
			u16 w = GXGetTexObjWidth(&tex);
			f32 h = GXGetTexObjHeight(&tex);
#if defined(VERSION_JP)
			u32 color = (u32)(255.0f * logo->mAlpha) | 0x0046FF00;
#else
			u32 color = (u32)(255.0f * logo->mAlpha) | 0xDC000000;
#endif
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, h / 2.0f + 154.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		}
		if (logo->mState == 9 || logo->mState == 10 || logo->mState == 11) {
			tex       = logo->mTpl.mTex.Init(logo->GetDesc(1), 0);
			u16 w     = GXGetTexObjWidth(&tex);
			u16 h     = GXGetTexObjHeight(&tex);
			u32 color = (u32)(255.0f * logo->mAlpha) | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, 224.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		}
		if (logo->mState == 12 || logo->mState == 13 || logo->mState == 14) {
			tex       = logo->mTpl.mTex.Init(logo->GetDesc(2), 0);
			u16 w     = GXGetTexObjWidth(&tex);
			u16 h     = GXGetTexObjHeight(&tex);
			u32 color = (u32)(255.0f * logo->mAlpha) | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, 224.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
}

/**
 * @note Address: 0x80027528
 * @note Size: 0x554
 */
// NONMATCHING: sprintf's format and buffer are set up in another order
// Logo::Update, reads the logos in, then fades each of them in and out
extern "C" void UpdateBootLogos(Logo* logo, u32 frames)
{
#if !defined(VERSION_JP)
	char path[64];
#endif
	switch (logo->mState) {
	case 1:
		logo->mState = 2;
	case 2: {
#if defined(VERSION_PAL)
		static const char* const suffix[] = { "_en", "_ge", "_fr", "_sp", "_it" };
		sprintf(path, "tpl/logo%s.tpl", suffix[gGame.mLanguage]);
#elif defined(VERSION_US)
		const char* suffix = "_us";
		sprintf(path, "tpl/logo%s.tpl", suffix);
#else
		const char* path = "tpl/logo.tpl";
#endif
		if (!logo->mTpl.ReadEx(path, 0, 0)) {
			break;
		}
		logo->mTpl.CloseHandle();
		if ((s32)((TPLPalette*)logo->mTpl.mBuffer)->numDescriptors > 3) {
			logo->mState = 3;
		} else {
			logo->mState = 6;
		}
		logo->ResetFade();
	}
	case 3:
		if (logo->mAlpha < 1.0f && (logo->mAlpha += (1.0f / (0.5f * FRAME_RATE)) * frames) > 1.0f) {
			logo->mAlpha = 1.0f;
		}
		if (logo->mAlpha != 1.0f) {
			break;
		}
		logo->mState = 4;
	case 4:
		logo->mTimer += frames;
		if (logo->mTimer >= 1.5f * FRAME_RATE) {
			if ((logo->mAngle += (DEG2RAD * (180.0f / FRAME_RATE)) * frames) >= PI) {
				logo->mAngle = 0.0f;
			}
			BOOL done = logo->mTimer >= 60.0f * FRAME_RATE ? TRUE : (&gGame.mPad.mPad[0])->mTrigger != 0;
			if (!done) {
				break;
			}
		} else {
			break;
		}
		logo->mState = 5;
	case 5:
		if (logo->mAlpha > 0.0f && (logo->mAlpha -= (1.0f / (0.5f * FRAME_RATE)) * frames) < 0.0f) {
			logo->mAlpha = 0.0f;
		}
		if (logo->mAlpha == 0.0f) {
			logo->mState = 6;
			logo->ResetFade();
		}
		break;
	case 6:
		if (logo->mAlpha < 1.0f && (logo->mAlpha += (1.0f / (0.5f * FRAME_RATE)) * frames) > 1.0f) {
			logo->mAlpha = 1.0f;
		}
		if (logo->mAlpha != 1.0f) {
			break;
		}
		logo->mState = 7;
	case 7:
		if ((logo->mTimer += frames) >= FRAME_RATE) {
			logo->mState = 8;
		} else {
			break;
		}
	case 8:
		if (logo->mAlpha > 0.0f && (logo->mAlpha -= (1.0f / (0.5f * FRAME_RATE)) * frames) < 0.0f) {
			logo->mAlpha = 0.0f;
		}
		if (logo->mAlpha == 0.0f) {
			logo->mState = 9;
			logo->ResetFade();
		}
		break;
	case 9:
		if (logo->mAlpha < 1.0f && (logo->mAlpha += (1.0f / (0.5f * FRAME_RATE)) * frames) > 1.0f) {
			logo->mAlpha = 1.0f;
		}
		if (logo->mAlpha != 1.0f) {
			break;
		}
		logo->mState = 10;
	case 10:
		if ((logo->mTimer += frames) >= FRAME_RATE) {
			logo->mState = 11;
		} else {
			break;
		}
	case 11:
		if (logo->mAlpha > 0.0f && (logo->mAlpha -= (1.0f / (0.5f * FRAME_RATE)) * frames) < 0.0f) {
			logo->mAlpha = 0.0f;
		}
		if (logo->mAlpha == 0.0f) {
			logo->mState = 12;
			logo->ResetFade();
		}
		break;
	case 12:
		if (logo->mAlpha < 1.0f && (logo->mAlpha += (1.0f / (0.5f * FRAME_RATE)) * frames) > 1.0f) {
			logo->mAlpha = 1.0f;
		}
		if (logo->mAlpha != 1.0f) {
			break;
		}
		logo->mState = 13;
	case 13:
		if ((logo->mTimer += frames) >= FRAME_RATE) {
			logo->mState = 14;
		} else {
			break;
		}
	case 14:
		if (logo->mAlpha > 0.0f && (logo->mAlpha -= (1.0f / (0.5f * FRAME_RATE)) * frames) < 0.0f) {
			logo->mAlpha = 0.0f;
		}
		if (logo->mAlpha == 0.0f) {
			logo->mState = 15;
			logo->ResetFade();
			logo->mTpl.Close();
		}
		break;
	case 15:
		break;
	}
}

/**
 * @note Address: 0x80027A7C
 * @note Size: 0x1C
 */
// Logo's init
extern "C" void ResetLogoFadeState(Logo* logo)
{
	logo->mState = 0;
	logo->ResetFade();
}

/**
 * @note Address: 0x80027A98
 * @note Size: 0xD8
 */
// CXObj's SetScaleY
void CXObj::SetScaleY(f32 value, s32 force)
{
	mMoveScale.y         = value;
	mMoveScale.mTarget.y = value;
	if (mJObj != NULL) {
		CJObj::SetScaleY(value, force);
	}
}

/**
 * @note Address: 0x80027B70
 * @note Size: 0x18C
 */
// CXObj's SetScale(scale)
void CXObj::SetScale(f32 scale, s32 force)
{
	f32 x = scale;
	f32 y = scale;
	f32 z = scale;

	mMoveScale.x         = x;
	mMoveScale.y         = y;
	mMoveScale.z         = z;
	mMoveScale.mTarget.x = x;
	mMoveScale.mTarget.y = y;
	mMoveScale.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mScale != v)) {
				return;
			}
		}
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
}

/**
 * @note Address: 0x80027CFC
 * @note Size: 0x18C
 */
// CXObj's SetScale(x, y, z)
void CXObj::SetScale(f32 x, f32 y, f32 z, s32 force)
{
	mMoveScale.x         = x;
	mMoveScale.y         = y;
	mMoveScale.z         = z;
	mMoveScale.mTarget.x = x;
	mMoveScale.mTarget.y = y;
	mMoveScale.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mScale != v)) {
				return;
			}
		}
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
}

/**
 * @note Address: 0x80027E88
 * @note Size: 0x178
 */
// CXObj's SetScale(v)
void CXObj::SetScale(const Vec& scale, s32 force)
{
	mMoveScale.Set(scale);
	mMoveScale.mTarget.Set(scale);
	if (mJObj) {
		CJObj::SetScale(scale, force);
	}
}

/**
 * @note Address: 0x80028000
 * @note Size: 0xB4
 */
// XObjDemo's DrawObject, draws through its own camera and lights when it has them
void XObjDemo::DrawObject()
{
	if (mState != 0) {
		CCamera* camera = mArchive.mDesc[0] != NULL ? &mCamera : &gGame.mCameras[0];
		CLight* light   = mArchive.mDesc[1] != NULL ? &mLights[0] : &gGame.mLights[0];
		light->SetCurrent(camera);
		if (camera->SetCurrent() != NULL) {
			DrawWithView(NULL);
			camera->EndCurrent();
		}
	}
}

static inline bool FadeDone()
{
	return 0.0f == gGame.mFade.mSpeed.r && 0.0f == gGame.mFade.mSpeed.g && 0.0f == gGame.mFade.mSpeed.b && 0.0f == gGame.mFade.mSpeed.a;
}

/**
 * @note Address: 0x800280B4
 * @note Size: 0x244
 */
// XObjDemo's update, steps the camera and light anims with the model
void XObjDemo::UpdateObject(u32 frames)
{
	if (mState == 0) {
		return;
	}
	if (mState == 2 && FadeDone()) {
		mState = 0;
		return;
	}
	XObj::UpdateObject(frames);
	if (mArchive.mDesc[0] != NULL && mCameraAnim >= 0) {
		mCamera.SetAnimRate((f32)frames);
		mCamera.Anim();
	}
	for (s32 i = 0; i < mLightCount; ++i) {
		if (mArchive.mDesc[i + 1] != NULL && mLightAnims[i] >= 0) {
			mLights[i].SetAnimRate((f32)frames);
			mLights[i].Anim();
		}
	}
	if (mLoop) {
		if (GetAObj(mJObj)->flags & (AOBJ_NO_ANIM | AOBJ_REWINDED)) {
			mState = 1;
			SetPos(mMoveTranslate, 0);
			SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
	}
}

/**
 * @note Address: 0x80028368
 * @note Size: 0xA4
 */
// XObjDemo's Release
void XObjDemo::Release()
{
	if (mArchive.mDesc[0] != NULL) {
		mCamera.Release();
	}
	for (s32 i = 0; i < mLightCount; ++i) {
		if (mArchive.mDesc[i + 1] != NULL) {
			mLights[i].Release();
		}
	}
	XObj::Release();
}

/**
 * @note Address: 0x8002840C
 * @note Size: 0xD4
 */
// XObjDemo's LoadFromPlacement, builds the camera and the light list from the archive
BOOL XObjDemo::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		if (mArchive.mDesc[0] != NULL) {
			mCamera.Init((HSD_CObjDesc*)mArchive.mDesc[0]->mObjectDesc);
#if defined(VERSION_PAL)
			mCamera.SetViewport(0.0f, 0.0f, &gGame.mGs.mScreen);
#endif
		}
		HSD_LObj* prev = NULL;
		for (s32 i = 0; i < 8; ++i) {
			if (mArchive.mDesc[i + 1] != NULL) {
				mLights[i].Init((HSD_LightDesc*)mArchive.mDesc[i + 1]->mObjectDesc, FALSE);
				mLightCount++;
				if (prev != NULL) {
					prev->next = &mLights[i];
				}
				prev = &mLights[i];
			}
		}
		if (prev != NULL) {
			prev->next = NULL;
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x800284E0
 * @note Size: 0x64
 */
// XObjDemo's ResetObjectState
void XObjDemo::ResetObjectState()
{
	XObj::ResetObjectState();
	mState      = 0;
	mLoop       = FALSE;
	mCameraAnim = -1;
	mLightCount = 0;
	for (s32 i = 0; i < 8; ++i) {
		mLightAnims[i] = -1;
	}
}

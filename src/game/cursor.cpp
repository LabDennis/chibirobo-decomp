#include "types.h"
#include "Version.h"
#include "game/hsd_jobj.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/gmath.h"
#include "game/title.h"
#include "game/xobjs.h"
#include <math.h>

// animation ids from 1000 up are in the object's own anim archive once that is read, the
// rest are in its model archive. the last three digits pick the animation
static inline s32 AnimationGroup(XObj* object, s32 id)
{
	bool extra = id >= 1000;
	if (extra) {
		extra = object->mAnim.mArchive != NULL;
	}
	return extra ? id / 1000 : 0;
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
 * @note Address: 0x80025AA0
 * @note Size: 0x4
 */
void XObj::OnPushDisplacement(const Vec& displacement)
{
}

/**
 * @note Address: 0x80025A98
 * @note Size: 0x8
 */
// XObj's GetEventScript
extern "C" s32 GetEventScript__4XObjFv(XObj* object)
{
	return object->mEventScript;
}

/**
 * @note Address: 0x80025A48
 * @note Size: 0x50
 */
s32 XObj::HandleEventCommand(s32 count, s32* args)
{
	for (int i = 0; i < count; i++) { }
	return 0;
}

/**
 * @note Address: 0x80025A40
 * @note Size: 0x8
 */
// XObj's OnPlayerContact
s32 XObj::OnPlayerContact(s32 unused, s32 value)
{
	return value;
}

/**
 * @note Address: 0x8002594C
 * @note Size: 0xF4
 */
// XObj's GetShapeAnimation
HSD_ShapeAnimJoint* XObj::GetShapeAnimation(s32 id)
{
	if (AnimationGroup(this, id)) {
		HSD_ShapeAnimJoint** animations = mAnim.mShapeAnimJoint;
		return animations != NULL ? animations[(id % 1000) % 1000] : NULL;
	}
	HSD_ShapeAnimJoint** animations = mArchive.mShapeAnimJoint;
	return animations != NULL ? animations[id % 1000] : NULL;
}

/**
 * @note Address: 0x80025858
 * @note Size: 0xF4
 */
// XObj's GetMaterialAnimation
HSD_MatAnimJoint* XObj::GetMaterialAnimation(s32 id)
{
	if (AnimationGroup(this, id)) {
		HSD_MatAnimJoint** animations = mAnim.mMatAnimJoint;
		return animations != NULL ? animations[(id % 1000) % 1000] : NULL;
	}
	HSD_MatAnimJoint** animations = mArchive.mMatAnimJoint;
	return animations != NULL ? animations[id % 1000] : NULL;
}

/**
 * @note Address: 0x80025764
 * @note Size: 0xF4
 */
// XObj's GetJointAnimation
HSD_AnimJoint* XObj::GetJointAnimation(s32 id)
{
	if (AnimationGroup(this, id)) {
		HSD_AnimJoint** animations = mAnim.mAnimJoint;
		return animations != NULL ? animations[(id % 1000) % 1000] : NULL;
	}
	HSD_AnimJoint** animations = mArchive.mAnimJoint;
	return animations != NULL ? animations[id % 1000] : NULL;
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
 * @note Address: 0x80025744
 * @note Size: 0x4
 */
// XObj's DrawDebugStatus
void XObj::DrawDebugStatus()
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
 * @note Address: 0x8002571C
 * @note Size: 0x4
 */
// XObj's ResetForNewGame
void XObj::ResetForNewGame()
{
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

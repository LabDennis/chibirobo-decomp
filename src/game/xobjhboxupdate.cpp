#include "game/xobjhbox.h"
#include "game/cjobjinlineattach.h"
#include "game/cgame.h"
#include "game/cparticle.h"
#include "game/csound.h"
#include "game/gmath.h"
#include "game/stagedata.h"
#include <stdlib.h>

extern "C" {
void PSMTXTrans(Mtx matrix, f32 x, f32 y, f32 z);
void HSD_MtxGetRotation(Mtx matrix, Vec* rotation);
void PSVECScale(const Vec* src, Vec* dst, f32 scale);
u8* GetGlobalTextEntry(s32 index);
u8* GetStageScriptByIndex(s32 index);
}

static inline void PlayBoxAnim(XObj* object, s32 id)
{
	object->SetAnim(id, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
}

static inline void SpawnBoxEffect(XObjHBox* box, u32 id)
{
	Vec rotation = { 0, 0, 0 };
	gGame.mParticle.CreateSRT(id, 1, box->mMoveTranslate, box->mMoveScale, rotation, FALSE);
}

// NONMATCHING: same work; different spill/restore scheduling and argument setup order
void XObjHBox::UpdateObject(u32 frames)
{
	UpdateObject__4XObjFUl(this, frames);
	if (mStatus.mFlags & 0x1000000000000000ULL) { // rebuild the joint matrix from the trs
		Mtx translation, scale, yaw, pitch, roll, result;
		PSMTXTrans(translation, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
		PSMTXScale(scale, mMoveScale.x, mMoveScale.y, mMoveScale.z);
		PSMTXRotRad(yaw, 'y', mMoveRotate.y);
		PSMTXRotRad(pitch, 'x', mMoveRotate.x);
		PSMTXRotRad(roll, 'z', mMoveRotate.z);
		PSMTXConcat(translation, yaw, result);
		PSMTXConcat(result, roll, result);
		PSMTXConcat(result, pitch, result);
		PSMTXConcat(result, scale, result);
		HSD_JObjCopyMtx(mJObj, result);
	}
	XObj* part = &mPart.mObj;
	if (mStatus.mFlags & 0x2000000000000000ULL) { // the opening hit
		mStatus.mFlags &= ~0x2000000000000000ULL;
		HSD_JObjClearMtxFlags(mJObj);
		mPart.mSfx = PlayGameSound(&gGame.mSound, 0x122, 255);
		PlayBoxAnim(this, 1);
		mRing = 1.0f;
		Mtx translation, yaw, scale, result;
		PSMTXTrans(translation, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
		PSMTXRotRad(yaw, 'y', mMoveRotate.y);
		PSMTXScale(scale, mMoveScale.x, mMoveScale.y, mMoveScale.z);
		PSMTXConcat(translation, yaw, result);
		PSMTXConcat(result, scale, result);
		HSD_JObjCopyMtx(mJObj, result);
		part->mStatus.mFlags |= 0x10000ULL;
		PlayBoxAnim(part, 2);
		part->SetPos(mMoveTranslate, 0);
		part->SetRotationY(DEG2RAD * (rand() % 360), 0);
		SpawnBoxEffect(this, 0x10021);
		mFloors = (XObjFloor*)GetGlobalTextEntry(part->mCoord.mPlacement->mObjectId); // the placement's id points into the text table
		mFloorMasks[1] = mFloorMasks[0] = 0;
		s32 index                       = mCoord.mPlacement - GetStagePlacementByIndex(0);
		if (index > 0)
			mEventScript = (u32)GetStageScriptByIndex(index);
	} else if (mAnimId == 1) {
		HSD_AObj* anim = GetAObj(mJObj);
		if (anim && (anim->flags & 0x44000000)) // animation done
			PlayBoxAnim(this, 3);
	} else if (mAnimId == 3) {
		HSD_AObj* anim = GetAObj(mJObj);
		if (anim && (anim->flags & 0x44000000)) // animation done
			HSD_JObjSetFlagsAll(mJObj, 0x10);
	}
	if (mRing > 0) { // the hit ring fades
		mRing -= 0.04f;
		if (mRing <= 0)
			mRing = 0;
	}
	if (part->mStatus.mFlags & 0x4000000000000000ULL)
		return;
	part->UpdateObject(frames);
	s32 id = part->mAnimId;
	if (id > 1) {
		s32 path[2] = { 0, 0 };
		CJObj joint(FindJointByPath(part->mJObj, path, 2));
		Vec pos = joint.GetPos();
		Vec rot;
		HSD_MtxGetRotation(joint.GetMtx(), &rot);
		SetPos(pos, 0);
		SetRotationY(rot.y, 0);
	}
	HSD_AObj* anim = GetAObj(part->mJObj);
	if (anim) {
		switch (id) {
		case 2:
			if (anim->flags & 0x44000000)
				PlayBoxAnim(part, 3);
			break;
		case 3:
			if (anim->flags & 0x44000000)
				PlayBoxAnim(part, 4);
			break;
		case 4:
			if (anim->flags & 0x44000000) {
				PlayBoxAnim(part, 5);
				SpawnBoxEffect(this, 0x1002b);
			}
			break;
		case 5:
			if (anim->flags & 0x44000000) {
				PlayBoxAnim(part, 6);
				mPart.mHit = 1;
				mStatus.mFlags |= 0x4000000000000000ULL;
			}
			break;
		}
	}
	if (mStatus.mFlags & 0x4000000000000000ULL) { // shrinking out
		part->SetAnimRate(0, 0x5a0);
		mPart.mHit -= 0.04f * frames;
		if (mPart.mHit <= 0) {
			mPart.mHit = 0;
			Release();
			return;
		}
		s32 path[2] = { 0, 0 };
		CJObj joint(FindJointByPath(part->mJObj, path, 2));
		CVec scale;
		HSD_JObjGetScale(joint.mJObj, &scale);
		PSVECScale(&scale, &scale, 1.12f * mPart.mHit);
		joint.SetScale(scale, 0);
		SetScale(scale, 0);
	}
}

#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/gmath.h"
#include "arithmetic.h"
#include <math.h>

extern "C" BOOL fn_800C98E0(XObjMap* map, const Vec* pos, s32 start, XObjMapHit* hit);
extern "C" void fn_800C659C(void* record, XObj* obj);
extern "C" void fn_800C6CF0(void* record, u32 frames);
extern "C" XObj* ResolveEventObjectId(const CEvtData* data, s16 id);
extern "C" BOOL fn_80131DDC(XObjSpiderSmall* spider);
extern "C" BOOL fn_80131DE8(XObjSpiderSmall* spider);
extern "C" f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 group);
extern s32 gRefreshRate;

static inline XObj* FindObj(s16 id)
{
	return gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, id) : NULL;
}

static inline void UpdateActorPartMatrix(XObjActor* actor, u32 index)
{
	XObjJointPart* part = &actor->mParts[index];
	Mtx translation;
	PSMTXTrans(translation, part->mObj->GetPos()->x, part->mObj->GetPos()->y, part->mObj->GetPos()->z);
	PSMTXConcat(actor->GetWorldMtx(part->mJoint), translation, part->mMtx);
	part->mObj->SetBaseMtx(part->mMtx);
}

void XObjActor::InitializeObject()
{
	XObj::InitializeObject();
	mSfx.Init((WiiPlayerState*)this, NULL);
}

void XObjActor::ResetObjectState()
{
	XObj::ResetObjectState();
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	_9B4                = PI;
	mKindStep           = 0;
	mKindCounter        = 0;
	mActorStatus.mFlags = 0;
	_9C8                = 0;
	_9CC                = 0;
	_9D0                = 1.0f;
	_9D4                = 1.0f;
	Vec v0              = { 0.0f, 0.0f, 0.0f };
	mHitPos             = v0;
	Vec v1              = { 0.0f, 0.0f, 0.0f };
	mHomePos            = v1;
	Vec v2              = { 0.0f, 0.0f, 0.0f };
	mVelocity           = v2;
	Vec v3              = { 0.0f, 0.0f, 0.0f };
	_A68                = v3;
	mHitStrength        = 0.0f;
	mFallSpeed          = 0.0f;
	mGroundHeight       = 0.0f;
	mCmd[0]             = 0;
	mCmd[1]             = 0;
	mCmd[18]            = 0;
	for (u32 i = 0; i < 16; ++i) {
		mCmd[i + 2] = 0;
	}
	_A60 = 0.0f;
	_A64 = 0.0f;
	for (u32 i = 0; i < 8; ++i) {
		mParts[i].mObj = NULL;
	}
	mAttachedId     = -1;
	mAttached       = NULL;
	mHeldObjects[0] = NULL;
	mHeldObjects[1] = NULL;
	mLookMode       = 0;
	mHeadYaw        = 0.0f;
	mLookAngle      = 0.0f;
	mLookRate       = 0.0f;
	mLookTarget     = NULL;
	fn_800C659C(&_AB4, this);
	for (u32 i = 0; i < 2; ++i) {
		mSfxActors[i].Reset();
	}
}

void XObjActor::ResetForNewGame()
{
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	_9B4                = PI;
	mKindStep           = 0;
	mKindCounter        = 0;
	mActorStatus.mFlags = 0;
	_9C8                = 0;
	_9CC                = 0;
	_9D0                = 1.0f;
	_9D4                = 1.0f;
	Vec v0              = { 0.0f, 0.0f, 0.0f };
	mHitPos             = v0;
	Vec v1              = { 0.0f, 0.0f, 0.0f };
	mHomePos            = v1;
	Vec v2              = { 0.0f, 0.0f, 0.0f };
	mVelocity           = v2;
	Vec v3              = { 0.0f, 0.0f, 0.0f };
	_A68                = v3;
	mHitStrength        = 0.0f;
	mFallSpeed          = 0.0f;
	mGroundHeight       = 0.0f;
	mCmd[0]             = 0;
	mCmd[1]             = 0;
	mCmd[18]            = 0;
	for (u32 i = 0; i < 16; ++i) {
		mCmd[i + 2] = 0;
	}
	_A60 = 0.0f;
	_A64 = 0.0f;
	for (u32 i = 0; i < 8; ++i) {
		mParts[i].mObj = NULL;
	}
	mSfx.Init((WiiPlayerState*)this, NULL);
	mAttachedId     = -1;
	mAttached       = NULL;
	mHeldObjects[0] = NULL;
	mHeldObjects[1] = NULL;
	mLookMode       = 0;
	mHeadYaw        = 0.0f;
	mLookAngle      = 0.0f;
	mLookRate       = 0.0f;
	mLookTarget     = NULL;
}

BOOL XObjActor::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		mHeading   = 0.0f;
		mMoveSpeed = 0.0f;
		mFacing    = 0.0f;
		for (u32 i = 0; i < 20; ++i) {
			mCmd[i] = 0;
		}
		mAttachedId     = -1;
		mAttached       = NULL;
		mHeldObjects[0] = NULL;
		mHeldObjects[1] = NULL;
		ClearStateHandler();
		ResetActorState();
		mLookMode   = 0;
		mHeadYaw    = 0.0f;
		mLookAngle  = 0.0f;
		mLookRate   = 0.0f;
		mLookTarget = NULL;
		if (mAttr.mObjData != NULL && mAttr.mObjData->mSource != NULL && mAttr.mObjData->mSource->mAnimNames != NULL) {
			mObject.mDefaultTexPat = mAttr.mObjData->mSource->mAnimNames[0];
			mObject.mTexPatName    = NULL;
		}
		for (u32 i = 0; i < 2; ++i) {
			mSfxActors[i].Create();
		}
		return TRUE;
	}
	return FALSE;
}

void XObjActor::ResetActorState()
{
}

void XObjActor::Release()
{
	XObj::Release();
	for (u32 i = 0; i < 2; ++i) {
		mSfxActors[i].Release();
	}
}

void XObjActor::UpdateObject(u32 frames)
{
	if (mActorStatus.mFlags & 4) {
		UpdateGroundMovement(frames);
	}
	XObj::UpdateObject(frames);
	UpdateAnimationSounds(GetAnimFrame());
	for (s32 i = 0; i < 2; ++i) {
		mSfxActors[i].Update(GetPos(), (f32)frames);
	}
}

void XObjActor::UpdateAfterAnim(u32 frames)
{
	if (mActorStatus.mFlags & 0x80) {
		switch (mLookMode) {
		case 0:
			break;
		case 2:
			break;
		case 1: {
			CVec head  = GetHeadWorldPosition();
			f32 angle  = (f32)atan2(mLookTarget->x - head.x, mLookTarget->z - head.z);
			mLookAngle = angle - GetRotY();
			f32 limit  = (f32)cos(GetMaxHeadYaw());
			if ((f32)cos(mLookAngle) < limit) {
				if (0.0f < (f32)sin(mLookAngle)) {
					mLookAngle = GetMaxHeadYaw();
				} else {
					mLookAngle = GetMinHeadYaw();
				}
			}
		}
		}
		f32 step = (f32)cos(mLookRate);
		if ((f32)cos(mLookAngle - mHeadYaw) <= step) {
			if (0.0f < (f32)sin(mHeadYaw - mLookAngle)) {
				mHeadYaw -= mLookRate;
			} else {
				mHeadYaw += mLookRate;
			}
		} else {
			mHeadYaw = mLookAngle;
			if ((u32)mLookMode == 0) {
				mActorStatus.mFlags &= ~0x80;
			}
		}
	}
	if (mActorStatus.mFlags & 0x400) {
		CVec target;
		target.x  = gGame.GetPlayer()->GetPos()->x;
		target.y  = 4.0f + gGame.GetPlayer()->GetPos()->y;
		target.z  = gGame.GetPlayer()->GetPos()->z;
		CVec head = GetHeadWorldPosition();
		f32 dx = target.x - head.x, dz = target.z - head.z, dy = target.y - head.y;
		f32 horizontal = FSqrt(dx * dx + dz * dz);
		f32 angle      = (f32)atan2(horizontal, FSqrt(dy * dy));
		mHeadPitch     = dy < 0.0f ? HALF_PI - angle : angle - HALF_PI;
		if (mHeadPitch > GetMaxHeadPitch()) {
			mHeadPitch = GetMaxHeadPitch();
		} else if (mHeadPitch < GetMinHeadPitch()) {
			mHeadPitch = GetMinHeadPitch();
		}
	}
	if ((mActorStatus.mFlags & 0x80) && (mActorStatus.mFlags & 0x400)) {
		mHeadAimSnap[0]  = 1;
		mHeadAimSnap[1]  = 1;
		mHeadAimObject   = NULL;
		mHeadAimTarget.x = NAN;
		mHeadAimCamera   = NULL;
		mHeadAimPitch    = mHeadPitch;
		mHeadAimYaw      = mHeadYaw;
	} else if (mActorStatus.mFlags & 0x80) {
		mHeadAimSnap[0]  = 1;
		mHeadAimSnap[1]  = 1;
		mHeadAimObject   = NULL;
		mHeadAimTarget.x = NAN;
		mHeadAimCamera   = NULL;
		mHeadAimPitch    = 0.0f;
		mHeadAimYaw      = mHeadYaw;
	} else if (mActorStatus.mFlags & 0x400) {
		mHeadAimSnap[0]  = 1;
		mHeadAimSnap[1]  = 1;
		mHeadAimObject   = NULL;
		mHeadAimTarget.x = NAN;
		mHeadAimCamera   = NULL;
		mHeadAimPitch    = mHeadPitch;
		mHeadAimYaw      = 0.0f;
	}
	if (mHeadAimSnap[0] != 0 || mHeadAimSnap[1] != 0) {
		fn_800C6CF0(&_AB4, frames);
	}
}

CVec XObjActor::GetHeadWorldPosition()
{
	return CVec(*GetPos());
}

void XObjActor::vf16C(u32 frames)
{
	ProcessPendingEvents(frames);
	RunStateHandler(frames);
	UpdateAfterAnim(frames);
	if (mActorStatus.mFlags & 0x1000) {
		for (u32 i = 0; i < 8; ++i) {
			XObjJointPart* part = &mParts[i];
			if (part->mObj == NULL) {
				continue;
			}
			if ((mStatus.mFlags & 0x10001) == 0x10001) {
				if (part->mObj->GetKind() == 0x423) {
					part->mObj->SetBaseMtx(GetWorldMtx(part->mJoint));
				} else {
					UpdateActorPartMatrix(this, i);
				}
			} else {
				Mtx translation;
				PSMTXTrans(translation, 0.0f, -1000.0f, 0.0f);
				part->mObj->SetBaseMtx(translation);
			}
		}
	}
}

extern "C" void ResetActorLook(XObjActor* actor, f32 rate)
{
	actor->mActorStatus.mFlags |= 0x80;
	actor->mLookMode  = 0;
	actor->mLookRate  = rate;
	actor->mLookAngle = 0.0f;
	actor->mActorStatus.mFlags &= ~0x400;
	actor->mHeadPitch = 0.0f;
}

extern "C" void SetActorLookTarget(XObjActor* actor, const Vec* target, f32 rate)
{
	actor->mActorStatus.mFlags |= 0x80;
	actor->mLookMode   = 1;
	actor->mLookRate   = rate;
	actor->mLookTarget = target;
}

extern "C" void SetActorLookAngle(XObjActor* actor, f32 angle, f32 rate)
{
	actor->mActorStatus.mFlags |= 0x80;
	actor->mLookMode  = 3;
	actor->mLookAngle = angle;
	actor->mLookRate  = rate;
}

extern "C" void SetActorLookAtPosition(XObjActor* actor, const Vec* target, f32 rate)
{
	actor->mActorStatus.mFlags |= 0x80;
	actor->mLookMode   = 1;
	actor->mLookRate   = rate;
	actor->mLookTarget = target;
	actor->mActorStatus.mFlags |= 0x400;
}

f32 XObjActor::GetMaxHeadYaw()
{
	return 1.3962634f;
}

f32 XObjActor::GetMinHeadYaw()
{
	return -1.3962634f;
}

f32 XObjActor::GetMaxHeadPitch()
{
	return 0.34906584f;
}

f32 XObjActor::GetMinHeadPitch()
{
	return -0.34906584f;
}

void XObjActor::DrawObject()
{
	DrawWithView(NULL);
}

void XObjActor::DrawScreenOverlay()
{
	XObj::DrawScreenOverlay();
}

void XObjActor::DrawDebugStatus()
{
}

s32 XObjActor::OnPlayerContact(s32 unused, s32 value)
{
	mActorStatus.mFlags |= 1;
	return value;
}

void XObjActor::OnHitFromPosition(const Vec& pos, f32 strength)
{
	XObj::OnHitFromPosition(pos, strength);
	mActorStatus.mFlags |= 2;
	mHitPos      = pos;
	mHitStrength = strength;
}

BOOL XObjActor::IsHitPending()
{
	return (mActorStatus.mFlags & 2) != 0;
}

s32 XObjActor::HandleEventCommand(s32 count, s32* args)
{
	switch (args[0]) {
	case 100: {
		s32 id = args[1];
		if (id == 0) {
			mActorStatus.mFlags &= ~0x1000;
			for (u32 i = 0; i < 8; ++i) {
				if (mParts[i].mObj != NULL) {
					mParts[i].mObj->ClearBaseMtx();
					mParts[i].mObj = NULL;
				}
			}
		} else {
			u32 i;
			for (i = 0; mParts[i].mObj != NULL && i < 8; ++i) { }
			if (i < 8) {
				mActorStatus.mFlags |= 0x1000;
				if (args[1] >= 10000) {
					mParts[i].mObj = FindObj(args[1]);
				} else {
					mParts[i].mObj = gGame.mXObjs.mObjects[(s16)args[1]];
				}
				s32 path[8];
				s32 depth = args[2];
				for (s32 j = 0; j < depth; ++j) {
					path[j] = args[3 + j];
				}
				mParts[i].mJoint = vf284(depth, path);
				UpdateActorPartMatrix(this, i);
			}
		}
		break;
	}
	case 101:
		if (args[1] == 0) {
			mActorStatus.mFlags &= ~0x800;
		} else {
			mActorStatus.mFlags |= 0x800;
		}
		break;
	}
	return 0;
}

void XObjActor::OnPushDisplacement(const Vec& displacement)
{
}

Vec XObjActor::GetCameraTarget()
{
	return mMoveTranslate;
}

void XObjActor::ResetMovementStep()
{
	mKindStep    = 0;
	mKindCounter = 0;
	mMoveSpeed   = 0.0f;
}

void XObjActor::UpdateGroundMovement(u32 frames)
{
	Vec pos = { 0.0f, 0.0f, 0.0f };
	CVec step;
	step.x    = mMoveSpeed * (f32)sin(mHeading);
	step.z    = mMoveSpeed * (f32)cos(mHeading);
	step.y    = 0.0f;
	CVec next = mMoveTranslate + step * (f32)frames;
	mMoveTranslate.Set(next);
	mMoveTarget0.Set(next);
	f32 offset = 0.0f;
	switch (GetKind()) {
	case 15:
		offset = 10.0f;
		break;
	}
	mActorStatus.mFlags &= ~0x20;
	if (fn_800C98E0((XObjMap*)gGame.mXObjs.mPools[3].mObjects, &pos, mFloorHit.mTriangle, &mFloorHit)) {
		mGroundHeight = mFloorHit.mHeight + offset;
		if (!(mActorStatus.mFlags & 8)) {
			mMoveTranslate.y = mFloorHit.mHeight + offset;
			mActorStatus.mFlags |= 0x20;
		} else if (mActorStatus.mFlags & 0x10) {
			if (mFallSpeed >= -2.3f) {
				mFallSpeed -= 0.36f * (f32)frames;
			} else {
				mFallSpeed = -2.3f;
			}
			if (mMoveTranslate.y <= mGroundHeight) {
				mMoveTranslate.y = mGroundHeight;
			}
			if (GetKind() != 15 || !(mStatus.mFlags & 0x40000)) {
				mMoveTranslate.y += mFallSpeed;
				if (mMoveTranslate.y <= mGroundHeight) {
					mFallSpeed = 0.0f;
					mActorStatus.mFlags &= ~8;
					mActorStatus.mFlags &= ~0x10;
				}
			}
		} else {
			mMoveTranslate.y += mFallSpeed;
		}
	} else {
		mMoveTranslate.y = offset;
		mActorStatus.mFlags |= 0x20;
	}
	pos = mMoveTranslate;
	SetPos(pos);
	SetRotateY(mFacing, 0.0f);
}

void XObjActor::UpdateAnimationSounds(f32 frame)
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000)) {
		f32 volume;
		if (mActorStatus.mFlags & 0x800) {
			volume = 1.0f;
		} else {
			volume = GetSoundVolumeAtPosition(&gGame.mSound, GetPos(), 0);
		}
		mSfx.Update(GetAnimId(), frame, volume);
	}
}

extern "C" s32 CountSmallSpidersGrabbingPlayer()
{
	s32 count = 0;
	for (s32 i = 0; i < 16; ++i) {
		XObjSpiderSmall* spider = (XObjSpiderSmall*)gGame.mXObjs.mPools[18].Get(i);
		if ((spider->mStatus.mFlags & 1) && fn_80131DDC(spider)) {
			++count;
		}
	}
	return count;
}

extern "C" s32 CountEmergedSmallSpiders()
{
	s32 count = 0;
	for (s32 i = 0; i < 16; ++i) {
		XObjSpiderSmall* spider = (XObjSpiderSmall*)gGame.mXObjs.mPools[18].Get(i);
		if ((spider->mStatus.mFlags & 1) && fn_80131DE8(spider)) {
			++count;
		}
	}
	return count;
}

void XObjActor::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	u16 nodeId = pFuncObj->GetCallbackNodeID();
	XObj::ExecCallbackB(pResult, mdl, pFuncObj);
	if (nodeId == 0) {
		if (mHeadAimSnap[0] != 0 || mHeadAimSnap[1] != 0) {
			nw4r::g3d::ResNode node = GetHeadJoint();
			if (node.IsValid()) {
				pFuncObj->SetCallbackNodeID(node.GetID());
			}
		}
	} else {
		GetResNode(nodeId);
		Mtx mount, rotationY, rotationX;
		PSMTXCopy(pResult->mpMtx, mount);
		PSMTXRotRad(rotationY, 'y', _AF0.mValue.y);
		PSMTXRotRad(rotationX, 'x', _AF0.mValue.x);
		PSMTXConcat(mount, rotationY, mount);
		PSMTXConcat(mount, rotationX, mount);
		PSMTXCopy(mount, pResult->mpMtx);
	}
}

nw4r::g3d::ResNode XObjActor::GetHeadJoint()
{
	return GetResNodeOrFirst(0);
}

void XObjActor::ResumeSounds(f32 fade)
{
	for (s32 i = 0; i < 2; ++i) {
		CSoundHandle* handle = mSfxActors[i].GetHandle();
		CSound3DActor* actor = mSfxActors[i].mActor;
		if (handle != NULL && actor != NULL) {
			s32 fadeFrames = (s32)(gRefreshRate * fade);
			if (actor->mHandle.IsPause()) {
				actor->mHandle.nw4r::snd::SoundHandle::Pause(false, fadeFrames);
			}
		}
	}
	mSfx.Resume(fade);
}

void XObjActor::PauseSounds(f32 fade)
{
	for (s32 i = 0; i < 2; ++i) {
		CSoundHandle* handle = mSfxActors[i].GetHandle();
		CSound3DActor* actor = mSfxActors[i].mActor;
		if (handle != NULL && actor != NULL) {
			s32 fadeFrames = (s32)(gRefreshRate * fade);
			if (!actor->mHandle.IsPause()) {
				actor->mHandle.nw4r::snd::SoundHandle::Pause(true, fadeFrames);
			}
		}
	}
	mSfx.Pause(fade);
}

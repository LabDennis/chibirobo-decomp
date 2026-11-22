#include "game/xobjflags.h"
#include "game/xobjhbox.h"
extern "C" {
#include <dolphin/mtx/vec.h>
#include <sysdolphin/axdriver.h>
}
#include "game/animframe.h"
#include "game/ccameraaction.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/cjobjinlineattach.h"
#include "game/cparticle.h"
#include "game/csound.h"
#include "game/evt.h"
#include "game/globals.h"
#include "game/gmath.h"
#include "game/hsd_cobj.h"
#include "game/stagedata.h"
#include "game/xchr.h"
#include "game/xobjchr.h"
#include <math.h>
#include <stdlib.h>

extern "C" {
void PSMTXTrans(Mtx matrix, f32 x, f32 y, f32 z);
void HSD_MtxGetRotation(Mtx matrix, Vec* rotation);
u8* GetStageScriptByIndex(s32 index);
}

// todo: cleanup, game/xobjfreeevent.cpp has the same
static inline void TriggerEvent(XObj* object, u32 first, u32 second)
{
	const void* script = object->GetEventScript();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer()))
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, object, first, second);
}

static inline BOOL AnimDone(HSD_JObj* joint)
{
	HSD_AObj* aobj = GetAObj(joint);
	return aobj != NULL ? (aobj->flags & 0x44000000) != 0 : TRUE;
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

// A stick flick sends loose dust hopping.
/**
 * @note Address: 0x800DE608
 * @note Size: 0x1EC
 */
s32 XObjDust::OnPlayerContact(s32, s32 flags)
{
	if ((flags & 3u) == 1)
		return flags;
	if (GetObjFlags() & 1) {
		if (0.0f == mBouncePhase) {
			CPad pad(0);
			mBouncePhase  = DEG2RAD;
			f32 y         = pad.mPad->mStickY;
			f32 x         = pad.mPad->mStickX;
			f32 magnitude = sqrtf(x * x + y * y);
			f32 height    = (rand() % 400 + 400) / 100.0f;
			mBounceHeight = height * magnitude;
		}
	}
	return 0;
}

// Keeps the material animation looping across animation changes.
/**
 * @note Address: 0x800DE7F4
 * @note Size: 0x2AC
 */
void XObjDust::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (mStatus.mFlags & 0x1000ULL) {
		if (force || id != mAnimId) {
			mAnimId     = id;
			void* joint = jointAnim != NULL ? jointAnim : mArchive.GetAnimJointRoot(id);
			void* shape = shapeAnim != NULL ? shapeAnim : mArchive.GetShapeAnimJointRoot(id);
			f32 materialFrame;
			if (mMatAnim != NULL) { // keep the material animation's frame
				HSD_AObj* animation = FindMaterialOrTextureAnimationInJointTree(mJObj);
				materialFrame       = animation != NULL ? HSD_AObjGetCurrentFrame(animation) : 0.0f;
			} else {
				materialFrame = 0.0f;
				mMatAnim      = mArchive.GetMatAnimJointRoot(0); // the default material animation
			}
			if (joint != NULL || mMatAnim != NULL || shape != NULL) {
				HSD_JObjRemoveAnimAll(mJObj);
				HSD_JObjAddAnimAll(mJObj, (HSD_AnimJoint*)joint, mMatAnim, (HSD_ShapeAnimJoint*)shape);
				HSD_ForeachAnim(mJObj, (HSD_Type)6, (HSD_TypeMask)0x484, HSD_AObjSetFlags, (AObj_Arg_Type)3, 0x20000000);
				HSD_JObjReqAnimAllByFlags(mJObj, 0x189, frame);
				HSD_JObjReqAnimAllByFlags(mJObj, 0x16, materialFrame);
			}
			force = TRUE;
		}
		if (rate != mAnimRate || force) {
			mAnimRate     = rate;
			mLastAnimRate = 0.01f + rate;
		}
	} else {
		XObj::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
	}
}

/**
 * @note Address: 0x800DEAA0
 * @note Size: 0x208
 */
void XObjDust::DrawObjectEffects()
{
	if (GetObjFlags() & 1) {
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
		gGame.mGs.mBillboardEnabled = 1;
		f32 height;
		f32 width = 8.0f * mMoveScale.x; // the billboard's base size
		height    = 8.0f * mMoveScale.y;
		u32 color;
		if (__fpclassifyf(mHeading) != FP_NAN) {
			color = 0xC8C8C800;
		} else if (mStatus.mFlags & 0x4000000000000000ULL) { // a different tint while the hit flag is set
			color = 0xC8646400;
		} else {
			color = 0xC8C8C800;
		}
		s32 alpha = (s32)(255.0f * mFade);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		Vec rotation = { 0.0f, 0.0f, 0.0f };
		DrawGsCircle(&gGame.mGs, 16, color | (u8)alpha, color, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z, width, height,
		             rotation.x, rotation.y, rotation.z);
		gGame.mGs.mBillboardEnabled = 0;
	}
}

/**
 * @note Address: 0x800DECA8
 * @note Size: 0x7C
 */
void XObjDust::DrawWithView(Mtx view)
{
	if (GetObjFlags() & 2) { // flags 2 and 4 draw normally
		XObj::DrawWithView(view);
	} else if (GetObjFlags() & 4) {
		XObj::DrawWithView(view);
	}
}

/**
 * @note Address: 0x800DED24
 * @note Size: 0x37C
 */
// event 16 waits for its own anim, event 56 shrinks it towards mTo while chibi robo plays anim 0x8C0
extern "C" BOOL UpdateDustEvent(XObjDust* dust, u32 frames)
{
	if (dust->mStatus.mFlags & 0x4000000000000000ULL) {
		if (dust->mEventId == 16) {
			HSD_AObj* aobj = GetAObj(dust->mJObj);
			if (aobj != NULL) {
				if (aobj->flags & 0x44000000) {
					if (dust->mStatus.mFlags & 0xC0000000000ULL) {
						TriggerEvent(dust, dust->mEventId, dust->mEventArg);
					}
					return TRUE;
				}
			} else
				return TRUE;
		} else if (dust->mEventId == 56) {
			XObj* player = gGame.GetPlayer();
			if (player->mAnimId == 0x8C0) {
				if (AnimDone(player->mJObj))
					return TRUE;
				f32 end                = GetAnimEndFrame(player->mJObj);
				f32 frame              = GetAnimFrame(player->mJObj);
				f32 t                  = 1.0f - frame / end;
				dust->mMoveTranslate.x = t * (dust->mFrom.x - dust->mTo.x) + dust->mTo.x;
				dust->mMoveTranslate.z = t * (dust->mFrom.z - dust->mTo.z) + dust->mTo.z;
				dust->mMoveScale.Set(dust->mStartScale.x * t, dust->mStartScale.y * t, dust->mStartScale.z * t);
			}
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x800DF0A0
 * @note Size: 0x238
 */
extern "C" BOOL CheckDustScatterHeading(XObjDust* dust)
{
	if (__fpclassifyf(dust->mHeading) == FP_NAN) {
		if (gGame.GetPlayer()->mAnimId == 0x27) { // the player's anim that scatters dust
			f32 z        = dust->mMoveTranslate.z - gGame.GetPlayer()->mMoveTranslate.z;
			f32 x        = dust->mMoveTranslate.x - gGame.GetPlayer()->mMoveTranslate.x;
			dust->mSpeed = sqrtf(x * x + z * z);
			if (dust->mSpeed < 20.0f) { // only close dust scatters
				dust->mHeading = atan2(dust->mMoveTranslate.x - gGame.GetPlayer()->mMoveTranslate.x,
				                       dust->mMoveTranslate.z - gGame.GetPlayer()->mMoveTranslate.z);
				dust->mSpeed   = 0.2f * (25.0f - dust->mSpeed); // away from the player, capped at 2
				if (dust->mSpeed > 2.0f)
					dust->mSpeed = 2.0f;
				dust->mStatus.mFlags |= 0x4000000000000000ULL;
				dust->mEventArg = 2002;
				return TRUE;
			}
		}
		return FALSE;
	}
	return TRUE;
}

/**
 * @note Address: 0x800DF2D8
 * @note Size: 0x2B0
 */
extern "C" BOOL UpdateDriftingDust(XObjDust* dust, u32 frames)
{
	dust->mPhase = fmod(dust->mPhaseStep * frames + dust->mPhase, TWO_PI);
	if (0.0f != dust->mBouncePhase) {
		if ((dust->mBouncePhase += 0.02f * frames) > PI) { // one half-sine bounce
			dust->mBouncePhase  = 0.0f;
			dust->mBounceHeight = 0.0f;
		}
	}
	f32 bounce             = sin(dust->mBouncePhase);
	f32 bob                = sin(dust->mPhase);
	dust->mMoveTranslate.y = dust->mBounceHeight * bounce + (0.8f * bob + dust->mBaseY);
	if (CheckDustScatterHeading(dust)) {
		if (dust->mSpeed > 0.0f) {
			f32 speed    = dust->mSpeed - 0.05f * frames;
			dust->mSpeed = speed;
			if (speed <= 0.0f)
				dust->mSpeed = 0.0f;
		}
		f32 x = sin(dust->mHeading);
		dust->mMoveTranslate.x += dust->mSpeed * x;
		f32 z = cos(dust->mHeading);
		dust->mMoveTranslate.z += dust->mSpeed * z;
	}
	if (dust->mStatus.mFlags & 0x4000000000000000ULL) { // scattered dust fades out
		f32 opacity = dust->mFade - 0.02f * frames;
		dust->mFade = opacity;
		if (opacity <= 0.0f)
			dust->mFade = 0.0f;
		PSVECScale((Vec*)&dust->mMoveScale, (Vec*)&dust->mMoveScale, 1.12f * dust->mFade);
		if (0.0f == PSVECMag((Vec*)&dust->mMoveScale)) { // fully gone
			if (dust->mStatus.mFlags & 0xC0000000000ULL) {
				TriggerEvent(dust, dust->mEventId, dust->mEventArg); // flagged dust fires its event on removal
			}
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x800DF588
 * @note Size: 0x148
 */
void XObjDust::UpdateObject(u32 frames)
{
	UpdateObject__4XObjFUl(this, frames);
	if (GetObjFlags() & 1) {
		if (UpdateDriftingDust(this, frames)) {
			Release();
			SetFlag();
		}
	}
	if (GetObjFlags() & 2) {
		if (UpdateDustEvent(this, frames)) {
			Release();
			SetFlag();
		}
	}
}

/**
 * @note Address: 0x800DF6D0
 * @note Size: 0x28
 */
void XObjDust::Release()
{
	mMatAnim = NULL;
	XObj::Release();
}

/**
 * @note Address: 0x800DF6F8
 * @note Size: 0x2A8
 */
BOOL XObjDust::LoadFromPlacement(const CoordInfo& info)
{
	mPlacementParam1 = info.mPlacement->mParam1;
	if (IsFlagSet())
		return TRUE;
	BOOL result = LoadFromPlacement__4XObjFRC9CoordInfo(this, &info);
	if (result) {
		mStatus.mFlags |= 0x18100000ULL;
		HSD_AObj* aobj = GetAObj(mJObj);
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		if (end > 0.0f) {
			f32 frame              = fmod((f32)rand(), end); // a random start frame
			UnkStageObj* placement = mCoord.mPlacement;
			SetAnim(placement->mStartAnim, TRUE, NULL, NULL, NULL, frame, 1.0f, 1.0f, 10.0f);
		}
		mAnimId2   = mAnimId;
		mAnimCount = mArchive.mNum[0];
		if (GetObjFlags() & 1) {
			f32 scale = (rand() % 100 + 100) / 100.0f; // scale 1.00-1.99
			PSVECScale((Vec*)&mMoveScale, (Vec*)&mMoveScale, scale);
		}
		if (GetObjFlags() & 2) {
			mStartScale.x = mMoveScale.x;
			mStartScale.y = mMoveScale.y;
			mStartScale.z = mMoveScale.z;
		}
	}
	return result;
}

/**
 * @note Address: 0x800DF9A0
 * @note Size: 0x184
 */
void XObjDust::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	if (GetObjFlags() & 1) { // flag 1: the drifting dust gets a random phase
		mBaseY        = mMoveTranslate.y;
		mPhase        = fmod((f32)rand(), TWO_PI);
		mPhaseStep    = (rand() % 200 + 200) / 10000.0f;
		mBouncePhase  = 0.0f;
		mBounceHeight = 0.0f;
		mHeading      = NAN;
		mSpeed        = 0.0f;
		mFade         = 1.0f;
	}
	mEventId  = -1; // no event
	mEventArg = -1;
	mMatAnim  = NULL;
	if (mJObj != NULL && (mStatus.mFlags & 0x1000ULL)) {
		UnkStageObj* placement = mCoord.mPlacement;
		SetAnim(placement->mStartAnim, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	}
}

/**
 * @note Address: 0x800DFB24
 * @note Size: 0x44
 */
s32 XObjDust::GetFlagIndex()
{
	return GetPlacementFlagIndex(2);
}

/**
 * @note Address: 0x800DFB68
 * @note Size: 0x100
 */
extern "C" u32 OnPlayerContact__8XObjHBoxFll(XObjHBox* box, s32 unused, u32 flags)
{
	if ((flags & 3) == 1) // not a new touch
		return flags;
	if (box->mAnimId == 0)
		return flags;
	gGame.GetPlayer()->mFallDistance = 0.0f;
	box->mStatus.mFlags |= 0x4000000000000000ULL;
	box->mPart.mHit = 1.0f;
	PlayGameSound(&gGame.mSound, 285, 255);
	TriggerEvent(box, -1, -1);
	HSD_AudioSFXKeyOff(box->mPart.mSfx);
	return 0;
}

/**
 * @note Address: 0x800DFC68
 * @note Size: 0xBC
 */
void XObjHBox::OnHitFromPosition(const Vec& pos, f32 strength)
{
	if (mAnimId <= 0) {
		OnHitFromPosition__4XObjFRC3Vecf(this, &pos, strength);
		if ((mStatus.mFlags & 0x04000000ULL) && strength >= (s32)GetPlayerActionParameter(31) / 100.0f) // param 31, in 1/100
			mStatus.mFlags |= 0x2000000000000000ULL;                                                    // strong enough hits open it
	}
}

/**
 * @note Address: 0x800DFD24
 * @note Size: 0x1F0
 */
void XObjHBox::DrawScreenOverlay()
{
	if (mRing > 0.0f) {
		Vec pos = { 0.0f, 0.0f, 0.0f };
		f32 x, y;
		ProjectOffsetToScreen(&pos, &x, &y, NULL, 0);
		f32 ring  = mRing;
		f32 inner = 400.0f * ring;
		f32 outer = 400.0f * (1.0f - ring);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
		// each ring is a loop over its width in pixels, and the width is one
		// each ring is a loop over its width, and the width is one
		s32 i;
		for (i = 0; i < 1; i++) {
			DrawGsCircle(&gGame.mGs, 16, (u32)(255.0f * ring) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, inner + i, inner + i, 0.0f, 0.0f, 0.0f);
		}
		for (i = 0; i < 1; i++) {
			DrawGsCircle(&gGame.mGs, 16, (u32)(255.0f * ring) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, outer + i, outer + i, 0.0f, 0.0f, 0.0f);
		}
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
	}
}

/**
 * @note Address: 0x800DFF14
 * @note Size: 0x48
 */
void XObjHBox::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	if (mPart.mObj.mStatus.mFlags & 0x10000ULL) // only when the part is drawn
		DrawGroundShadowAt__4XObjFPC3Vecf(this, origin, opacity);
}

/**
 * @note Address: 0x800DFF5C
 * @note Size: 0x78
 */
void XObjHBox::DrawWithView(Mtx view)
{
	DrawWithView__4XObjFPA4_f(this, view);
	if (mPart.mObj.mStatus.mFlags & 0x10000ULL) // only when the part is drawn
		mPart.mObj.DrawWithView(view);
}

// NONMATCHING: retail frame 0x300 vs 0x2f0: before SetPos it copies pos (12 B) and the 16-byte
// HSD_MtxGetRotation result to second temps that SetRotationY reads at +4; Quaternion/CVec
// spellings of rot do not reproduce it
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
		mPart.mObj.mStatus.mFlags |= 0x10000ULL;
		PlayBoxAnim(&mPart.mObj, 2);
		mPart.mObj.SetPos(mMoveTranslate, 0);
		mPart.mObj.SetRotationY(DEG2RAD * (rand() % 360), 0);
		SpawnBoxEffect(this, 0x10021);
		mFloors        = GetObjectCollisionList(mPart.mObj.mCoord.mPlacement->mObjectId);
		mFloorMasks[1] = mFloorMasks[0] = 0;
		s32 index                       = GetStagePlacementIndex(mCoord.mPlacement);
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
		if ((mRing -= 0.04f) <= 0)
			mRing = 0;
	}
	if (mPart.mObj.mStatus.mFlags & 0x4000000000000000ULL)
		return;
	mPart.mObj.UpdateObject(frames);
	s32 id = mPart.mObj.mAnimId;
	if (id > 1) {
		s32 path[2] = { 0, 0 };
		CJObj joint(FindJointByPath(mPart.mObj.mJObj, path, 2));
		Vec pos = joint.GetPos();
		Vec rot;
		HSD_MtxGetRotation(joint.GetMtx(), &rot);
		SetPos(pos, 0);
		SetRotationY(rot.y, 0);
	}
	HSD_AObj* anim = GetAObj(mPart.mObj.mJObj);
	if (anim) {
		switch (id) {
		case 2:
			if (anim->flags & 0x44000000)
				PlayBoxAnim(&mPart.mObj, 3);
			break;
		case 3:
			if (anim->flags & 0x44000000)
				PlayBoxAnim(&mPart.mObj, 4);
			break;
		case 4:
			if (anim->flags & 0x44000000) {
				PlayBoxAnim(&mPart.mObj, 5);
				SpawnBoxEffect(this, 0x1002b);
			}
			break;
		case 5:
			if (anim->flags & 0x44000000) {
				PlayBoxAnim(&mPart.mObj, 6);
				mPart.mHit = 1;
				mStatus.mFlags |= 0x4000000000000000ULL;
			}
			break;
		}
	}
	if (mStatus.mFlags & 0x4000000000000000ULL) { // shrinking out
		mPart.mObj.SetAnimRate(0, 0x5a0);
		if ((mPart.mHit -= 0.04f * frames) <= 0) {
			mPart.mHit = 0;
			Release();
			return;
		}
		s32 path[2] = { 0, 0 };
		CJObj joint(FindJointByPath(mPart.mObj.mJObj, path, 2));
		CVec scale;
		HSD_JObjGetScale(joint.mJObj, &scale);
		PSVECScale(&scale, &scale, 1.12f * mPart.mHit);
		joint.SetScale(scale, 0);
		SetScale(scale, 0);
	}
}

/**
 * @note Address: 0x800E0F40
 * @note Size: 0x4C
 */
void XObjHBox::Release()
{
	mPart.mObj.Release();
	XObj::Release();
	mLoadState = 0;
}

/**
 * @note Address: 0x800E0F8C
 * @note Size: 0x14C
 */
BOOL XObjHBox::LoadFromPlacement(const CoordInfo& info)
{
	switch (mLoadState) {
	case 0:
		if (!LoadFromPlacement__4XObjFRC9CoordInfo(this, &info))
			break;
		mEventScript = 0;
		++mLoadState;
		// fall through, the part starts reading right away
	case 1:
		if (!mPart.mObj.LoadFromPlacement(CoordInfo(GetSpawnData(36))))
			break;
		mPart.mObj.mStatus.mFlags &= ~0x10000ULL;
		mPart.mObj.SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		mPart.mObj.SetPos(mMoveTranslate, 0);
		++mLoadState;
		break;
	}
	if (mLoadState > 1) {
		mLoadState = 0;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x800E10D8
 * @note Size: 0x60
 */
void XObjHBox::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	mLoadState = 0;
	mRing      = 0.0f;
	mPart.mObj.ResetObjectState();
	mPart.mHit   = 0.0f;
	mPart.mRange = 5.0f;
}

/**
 * @note Address: 0x800E1138
 * @note Size: 0x30
 */
void XObjHBox::ResetForNewGame()
{
	mPart.mObj.ResetForNewGame();
}

/**
 * @note Address: 0x800E1168
 * @note Size: 0x50
 */
void XObjHBox::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	mPart.mObj.InitializeObject(id);
}

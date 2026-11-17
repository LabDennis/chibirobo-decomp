#include "game/xobjfree.h"
#include "game/xobjflags.h"
#include "game/xobjhbox.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/gmath.h"
#include "game/xchr.h"
#include "game/xobjchr.h"
#include "game/globals.h"
#include "game/evt.h"
#include "game/csound.h"
#include "game/stagedata.h"
#include "game/ccameraaction.h"
#include "game/cjobjinlineattach.h"
#include "game/cparticle.h"
#include "game/animframe.h"
#include "game/hsd_cobj.h"
#include <math.h>
#include <stdlib.h>

extern "C" {
#include <dolphin/mtx/vec.h>
#include <sysdolphin/axdriver.h>
void PSMTXTrans(Mtx matrix, f32 x, f32 y, f32 z);
void HSD_MtxGetRotation(Mtx matrix, Vec* rotation);
void PSVECScale(const Vec* src, Vec* dst, f32 scale);
u8* GetStageScriptByIndex(s32 index);
u32 GetPickupRewardValue(s32 index);
}

extern "C" BOOL UpdateDustEvent(XObjDust* dust, u32 frames);

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

static inline void SetPosAndTarget(CObjVec* pos, const Vec& value)
{
	pos->Set(value);
	pos->SetTarget(value);
}

static inline bool IsCoinItemInRange(XObjCoin* coin, s32 first, s32 end)
{
	bool inRange = false;
	if (coin->GetItem() >= first && coin->GetItem() < end)
		inRange = true;
	return inRange;
}

/**
 * @note Address: 0x800DD600
 * @note Size: 0x94
 */
void XObjFree::OnHitFromPosition(const Vec& pos, f32 value)
{
	XObj::OnHitFromPosition(pos, value);
	if (mStatus.mFlags & 0x04000000ULL) {
		f32 remaining = mLife - value;
		mLife         = remaining;
		if (remaining <= 0.0f)
			mStatus.mFlags |= 0x2000000000000000ULL;
	}
}

// Keeps the material anim running across anim changes
/**
 * @note Address: 0x800DD694
 * @note Size: 0x2D8
 */
void XObjFree::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (mStatus.mFlags & 0x1000ULL) {
		if (force || id != mAnimId) {
			mAnimId     = id;
			void* joint = jointAnim != NULL ? jointAnim : mArchive.GetAnimJointRoot(id);
			void* shape = shapeAnim != NULL ? shapeAnim : mArchive.GetShapeAnimJointRoot(id);
			f32 materialFrame;
			if (mMatAnim != NULL) {
				HSD_AObj* animation = FindMaterialOrTextureAnimationInJointTree(mJObj);
				materialFrame       = animation != NULL ? HSD_AObjGetCurrentFrame(animation) : 0.0f;
			} else {
				materialFrame  = 0.0f;
				s16 materialId = GetObjFlags();
				mMatAnim       = mArchive.GetMatAnimJointRoot(materialId);
			}
			if (joint != NULL || mMatAnim != NULL || shape != NULL) {
				HSD_JObjRemoveAnimAll(mJObj);
				HSD_JObjAddAnimAll(mJObj, (HSD_AnimJoint*)joint, mMatAnim, (HSD_ShapeAnimJoint*)shape);
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
 * @note Address: 0x800DD96C
 * @note Size: 0x220
 */
void XObjFree::DrawScreenOverlay()
{
	XObj::DrawScreenOverlay();
	if ((mStatus.mFlags & 0x4000000000000000ULL) && mFlash > 0.0f) {
		Vec position = { 0.0f, 0.0f, 0.0f };
		f32 x, y;
		ProjectOffsetToScreen(&position, &x, &y, NULL, 0);
		f32 effect      = mFlash;
		f32 innerRadius = 400.0f * effect;
		f32 outerRadius = 400.0f * (1.0f - effect);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
		// each ring is a loop over its width, and the width is one (the Wii build converts i the same way)
		s32 i;
		for (i = 0; i < 1; i++) {
			DrawGsCircle(&gGame.mGs, 16, (u32)(255.0f * effect) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, innerRadius + i, innerRadius + i,
			             0.0f, 0.0f, 0.0f);
		}
		for (i = 0; i < 1; i++) {
			DrawGsCircle(&gGame.mGs, 16, (u32)(255.0f * effect) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, outerRadius + i, outerRadius + i,
			             0.0f, 0.0f, 0.0f);
		}
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
	}
}

/**
 * @note Address: 0x800DDB8C
 * @note Size: 0x4AC
 */
void XObjFree::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	if (mStatus.mFlags & 0x2000000000000000ULL) {
		mStatus.mFlags &= ~0x2000000000000000ULL;
		mStatus.mFlags |= 0x4000000000000000ULL;
		SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		SetColor(1, 200, 200, 200, 0.0f);
		SetColor(1, 0, 0, 0, 0.4f);
		for (s32 i = 0; i < 10; ++i) {
			f32 angle    = DEG2RAD * (rand() % 360);
			f32 sinAngle = (f32)sin(angle);
			f32 x        = sinAngle * (rand() % 100) / 150.0f;
			f32 cosAngle = (f32)cos(angle);
			f32 z        = cosAngle * (rand() % 100) / 150.0f;
			f32 y        = (rand() % 100) / 200.0f;
			Vec position = mMoveTranslate;
			position.x += x * (rand() % 100) / 100.0f;
			position.z += z * (rand() % 100) / 100.0f;
			Vec velocity = { x, y, z };
			Vec size     = { 20.0f, 20.0f, 1.0f };
			SpawnPoolParticle(&gGame.GetPlayer()->mDust.mPtcl, &position, &size, &velocity, 0xFFFFFFF0);
		}
		mFlash = 1.0f;
	}
	if (mStatus.mFlags & 0x4000000000000000ULL) {
		HSD_AObj* aobj = GetAObj(mJObj);
		if (aobj && (aobj->flags & 0x44000000))
			ClearAnim();
		if (mFlash > 0.0f) {
			if ((mFlash -= 0.04f) <= 0.0f)
				Release();
		}
	}
}

// XObjFree::Release
/**
 * @note Address: 0x800DE038
 * @note Size: 0x28
 */
void XObjFree::Release()
{
	mMatAnim = NULL;
	XObj::Release();
}

// XObjFree::ResetObjectState
/**
 * @note Address: 0x800DE060
 * @note Size: 0xEC
 */
void XObjFree::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	if (mAttr.mObjData != NULL)
		mLife = (s16)GetObjFlags();
	mMatAnim = NULL;
	if (mJObj != NULL && (mStatus.mFlags & 0x1000ULL)) {
		UnkStageObj* animation = mCoord.mPlacement;
		SetAnim(animation->mStartAnim, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	}
}

// XObjDust::OnItemUsed
/**
 * @note Address: 0x800DE14C
 * @note Size: 0x2D8
 */
BOOL XObjDust::OnItemUsed(const Vec* position, s32 kind)
{
	if ((GetObjFlags() & 2) && kind == 16) {
		if (GetObjFlags() & 0x10) {
			mAnimId2++;
			SetAnim(mAnimId2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			if (mAnimId2 >= mAnimCount - 1) {
				mStatus.mFlags |= 0x4000000000000000ULL;
				mEventId = kind;
			}
		} else
			return FALSE;
	} else if ((GetObjFlags() & 2) && kind == 56) {
		if (GetObjFlags() & 0x20) {
			mFrom.x = mMoveTranslate.x;
			mFrom.y = mMoveTranslate.y;
			mFrom.z = mMoveTranslate.z;
			mTo.x   = position->x;
			mTo.y   = position->y;
			mTo.z   = position->z;
			mStatus.mFlags |= 0x4000000000000000ULL;
			mEventId = kind;
			if (mStatus.mFlags & 0x00000C0000000000ULL)
				TriggerEvent(this, mEventId, mEventArg);
		} else
			return FALSE;
	} else if (mStatus.mFlags & 0x0000040000000000ULL) {
		TriggerEvent(this, kind, -1);
	} else
		return FALSE;
	return TRUE;
}

// XObjDust::OnHitFromPosition
/**
 * @note Address: 0x800DE424
 * @note Size: 0x1E4
 */
void XObjDust::OnHitFromPosition(const Vec& position, f32 strength)
{
	if (GetObjFlags() & 1) {
		mStatus.mFlags |= 0x4000000000000000ULL;
		if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f)
			mEventArg = 2001;
		else
			mEventArg = 2000;
	} else if (mStatus.mFlags & 0x0000080000000000ULL) {
		if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f)
			TriggerEvent(this, -1, 2001);
		else
			TriggerEvent(this, -1, 2000);
	}
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
				HSD_AObj* aobj         = GetAObj(player->mJObj);
				f32 end                = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
				aobj                   = GetAObj(player->mJObj);
				f32 frame              = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
				f32 t                  = 1.0f - frame / end;
				dust->mMoveTranslate.x = t * (dust->mFrom.x - dust->mTo.x) + dust->mTo.x;
				dust->mMoveTranslate.z = t * (dust->mFrom.z - dust->mTo.z) + dust->mTo.z;
				f32 x                  = dust->mStartScale.x;
				f32 y                  = dust->mStartScale.y;
				f32 z                  = dust->mStartScale.z;
				x *= t;
				y *= t;
				z *= t;
				dust->mMoveScale.x = x;
				dust->mMoveScale.y = y;
				dust->mMoveScale.z = z;
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

/**
 * @note Address: 0x800E11B8
 * @note Size: 0x20C
 */
extern "C" void UpdateFallingCoin(XObjCoin* coin, u32 frames)
{
	CVec step;
	switch (coin->mState) {
	case 0:
		break;
	case 1:
		++coin->mFallFrames;
		coin->mFallSpeed -= 0.09f * frames;
		coin->mMoveTranslate.y += coin->mFallSpeed;
		if (coin->mMoveTranslate.y <= 0.0f) {
			coin->mFallSpeed       = 0.0f;
			coin->mMoveTranslate.y = 0.0f;
			coin->mState           = 0;
			coin->mStatus.mFlags &= ~0x100000ULL;
		}
		if (coin->mFallSpeed >= -2.3f) {
			coin->mFallSpeed -= 0.36f * frames;
		} else {
			coin->mFallSpeed = -2.3f;
		}
		step.x = 0.4f * (f32)sin(coin->mHeading);
		step.z = 0.4f * (f32)cos(coin->mHeading);
		step.y = 0.0f;
		SetPosAndTarget(&coin->mMoveTranslate, coin->mMoveTranslate + step * (f32)frames);
		coin->SetPos(coin->mMoveTranslate, 0);
		break;
	}
}

/**
 * @note Address: 0x800E13C4
 * @note Size: 0x7C
 */
s32 XObjCoin::HandleEventCommand(s32 count, s32* args)
{
	if (count < 1)
		return 0;
	mParam0 = args[0];
	if (count < 2)
		mParam1 = 0;
	else
		mParam1 = args[1];
	// the other eight go over whatever the count
	mParams[0] = args[2];
	mParams[1] = args[3];
	mParams[2] = args[4];
	mParams[3] = args[5];
	mParams[4] = args[6];
	mParams[5] = args[7];
	mParams[6] = args[8];
	mParams[7] = args[9];
	return 0;
}

/**
 * @note Address: 0x800E1440
 * @note Size: 0x540
 */
s32 XObjCoin::OnPlayerContact(s32 unused, s32 value)
{
	if ((value & 3U) == 1)
		return value;
	if (mStatus.mFlags & 0x100000) // still falling, not collectible yet
		return 0;

	XObjChr* player = gGame.GetPlayer();
	player->SetColor(1, 0, 120, 120, 0.0f);
	player->SetColor(1, 0, 0, 0, 0.2f);
	player->mFallDistance = 0.0f;
	mStatus.mFlags |= 0x4000000000000000ULL;
	SetFlag();

	s32 sound  = -1;
	s32 effect = -1;
	if (IsCoinItemInRange(this, 0, 100)) { // items 0..99 are money
		AdjustPlayerMoney(&player->mParams, GetPickupRewardValue(GetItem()));
		sound = 0x89;
	} else if (IsCoinItemInRange(this, 100, 200)) { // items 100..199 are scrap
		s32 scrap = GetPickupRewardValue(GetItem() - 97);
		if (!scrap) {
			AttrInfo attr(CoordInfo(GetSpawnData(mAnimId + 39)));
			scrap = GetPickupRewardValue(GetDataItem(attr.mObjData) - 97);
		}
		AddPlayerScrap(&player->mParams, scrap);
		sound = 0x123;
	} else if (IsCoinItemInRange(this, 200, 300)) { // items 200..299 are battery
		s32 reward    = GetPickupRewardValue(GetItem() - 193);
		Params& stats = player->mParams;
		stats.SetBattery(reward + stats.mBattery);
		sound  = 0x8e;
		effect = 20;
	} else if (IsCoinItemInRange(this, 300, 400)) { // items 300..399 are experience
		AdjustPlayerExperience(&player->mParams, GetPickupRewardValue(GetItem() - 292));
		sound = 0x11d;
	}
	if (effect >= 0) {
		Vec rotation = {};
		gGame.mParticle.CreateSRT(0x10000 | effect, 1, mMoveTranslate, mMoveScale, rotation, FALSE);
	}
	if (sound >= 0)
		PlayGameSound(&gGame.mSound, sound, 255);
	mFade = 1.0f;
	return 0;
}

/**
 * @note Address: 0x800E1980
 * @note Size: 0x4E0
 */
void XObjCoin::UpdateObject(u32 frames)
{
	const f32 radians = 0.01745329252f; // degrees to radians
	if (IsCoinItemInRange(this, 0, 100)) {
		f32 speed;
		if (mSpin >= 0.0f) {
			mSpin += 4.0f * radians;
			speed = (1.5f + (f32)sin(mSpin)) / 2.0f;
		} else {
			mSpin -= 4.0f * radians;
			speed = -(1.5f + (f32)sin(mSpin)) / 2.0f;
		}
		mMoveRotate.y += 8.0f * radians * speed * frames;
	} else if (IsCoinItemInRange(this, 100, 200)) {
		if (mSpin >= 0.0f)
			mMoveRotate.y += 2.0f * radians * frames;
		else
			mMoveRotate.y -= 2.0f * radians * frames;
	}
	XObj::UpdateObject(frames);
	if (mStatus.mFlags & 0x4000000000000000ULL) {
		if (IsCoinItemInRange(this, 0, 100)) {
			s32 anim       = mAnimId;
			HSD_AObj* aobj = GetAObj(mJObj);
			switch (anim) {
			case 0:
				SetAnim(2, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				break;
			case 1:
				break;
			case 2:
				if (aobj->flags & 0x44000000)
					Release();
				break;
			}
		} else {
			if (mMoveScale == (Vec) { 0.0f, 0.0f, 0.0f })
				Release();
			if ((mFade -= 0.02f * frames) <= 0.0f)
				mFade = 0.0f;
			PSVECScale(&mMoveScale, &mMoveScale, 1.12f * mFade);
		}
	}
	switch (mParam0) {
	case 0:
		break;
	case 1: // a dropped coin gets its heading and fall speed
		mState      = 1;
		mFallFrames = 0;
		mParam0     = 0;
		mHeading    = radians * (s32)mParam1;
		mFallSpeed  = (s32)mParams[0] / 100.0f;
		mStatus.mFlags |= 0x100000;
		break;
	}
	UpdateFallingCoin(this, frames);
}

/**
 * @note Address: 0x800E1E60
 * @note Size: 0xD0
 */
BOOL XObjCoin::LoadFromPlacement(const CoordInfo& info)
{
	mPlacementParam1 = info.mPlacement->mParam1;
	if (IsFlagSet())
		return TRUE;
	if (XObj::LoadFromPlacement(info)) {
		mStatus.mFlags |= 0x10000000ULL;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x800E1F30
 * @note Size: 0x1C8
 */
void XObjCoin::ResetObjectState()
{
	XObj::ResetObjectState();
	s32 direction = rand() & 1 ? 1 : -1;
	f32 angle     = DEG2RAD * (rand() % 360);
	mSpin         = angle * direction;
	f32 end       = GetAnimEndFrame(mJObj);
	if (end > 0.0f) {
		f32 frame              = fmod((f32)rand(), end);
		UnkStageObj* placement = mCoord.mPlacement;
		SetAnim(placement->mStartAnim, TRUE, NULL, NULL, NULL, frame, 1.0f, 1.0f, 10.0f);
	}
	mFade     = 0.0f;
	mState    = 0;
	mParam0   = 0;
	mBouncing = 0;
}

/**
 * @note Address: 0x800E20F8
 * @note Size: 0x44
 */
s32 XObjCoin::GetFlagIndex()
{
	return GetPlacementFlagIndex(1);
}

/**
 * @note Address: 0x800E213C
 * @note Size: 0xC0
 */
BOOL XObjItem::LoadFromPlacement(const CoordInfo& info)
{
	BOOL result = XObj::LoadFromPlacement(info);
	if (result && IsFlagSet())
		mStatus.mFlags &= ~0x10000ULL;
	return result;
}

/**
 * @note Address: 0x800E21FC
 * @note Size: 0x44
 */
s32 XObjItem::GetFlagIndex()
{
	return GetPlacementFlagIndex(0);
}

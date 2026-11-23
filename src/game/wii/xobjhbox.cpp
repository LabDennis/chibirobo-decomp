#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/cgs.h"
#include "game/wii/csound.h"
#include "game/evt.h"
#include "game/gmath.h"
#include "game/globals.h"
#include "game/stagedata.h"
#include <math.h>
#include <stdlib.h>

#include "JSystem/JMath/Inline.h"
#define PSVECScale JMathInlineVEC::PSVECScale

// the Wii player state; xobj_chr.cpp's definition
extern "C" BOOL IsPlayerBlockingEvents(const WiiPlayerState* player);

static inline void TriggerEvent(XObj* object, s32 first, s32 second)
{
	const void* script = object->GetEventScript();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer()))
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, object, first, second);
}

/**
 * @note Address: 0x8011701C
 * @note Size: 0x264
 */
void XObjHBox::DrawScreenOverlay()
{
	if (mRing > 0.0f) {
		Vec pos = { 0.0f, 0.0f, 0.0f };
		// retail zeroes a second vector on the stack it never reads
		Vec unused = { 0.0f, 0.0f, 0.0f };
		f32 x, y;
		ProjectOffsetToScreen(&pos, &x, &y, NULL, NULL);
		f32 ring  = mRing;
		f32 inner = 400.0f * ring;
		f32 outer = 400.0f * (1.0f - ring);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCCOL, GX_BL_SRCALPHA, GX_LO_SET);
		// each ring is a loop over its width, and the width is one
		s32 i;
		for (i = 0; i < 1; i++) {
			DrawGsCircle(&gGame.mGs, 16, (u32)(255.0f * ring) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, inner + i, inner + i, 0.0f, 0.0f, 0.0f);
		}
		for (i = 0; i < 1; i++) {
			DrawGsCircle(&gGame.mGs, 16, (u32)(255.0f * ring) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, outer + i, outer + i, 0.0f, 0.0f, 0.0f);
		}
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	}
}

/**
 * @note Address: 0x80117280
 * @note Size: 0xC0
 */
void XObjHBox::OnHitFromPosition(const Vec& pos, f32 strength)
{
	if (GetAnimId() <= 0) {
		XObj::OnHitFromPosition(pos, strength);
		if ((mStatus.mFlags & 0x04000000ULL) && strength >= (s32)GetPlayerActionParameter(31) / 100.0f) // param 31, in 1/100
			mStatus.mFlags |= 0x2000000000000000ULL;                                                    // strong enough hits open it
	}
}

/**
 * @note Address: 0x80117340
 * @note Size: 0x194
 */
s32 XObjHBox::OnPlayerContact(s32, s32 flags)
{
	if ((flags & 3) == 1U) // not a new touch
		return flags;
	if (GetAnimId() == 0)
		return flags;
	mStatus.mFlags |= 0x4000000000000000ULL;
	mPart.mHit = 1.0f;
	Mtx scale, result;
	PSMTXScale(scale, 5.0f, 5.0f, 5.0f); // the part's reach, like mPart.mRange
	PSMTXConcat(mPart.mObj.GetWorldMtx("obj2"), scale, result);
	mPart.mObj.SetBaseMtx(result);
	mPart.mObj.SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	PlayWiiSoundEffect(&gGame.mSound, 0x18A, 1.0f, 0.0f);
	TriggerEvent(this, -1, -1);
	if (mPart.mSfx != 0) {
		nw4r::snd::detail::BasicSound* sound = *(nw4r::snd::detail::BasicSound**)mPart.mSfx;
		if (sound != NULL)
			sound->Stop(0);
		mPart.mSfx = 0;
	}
	return 0;
}

/**
 * @note Address: 0x801174D4
 * @note Size: 0x44
 */
s32 XObjDust::GetFlagIndex()
{
	return GetPlacementFlagIndex(2);
}

/**
 * @note Address: 0x80117518
 * @note Size: 0x174
 */
void XObjDust::ResetObjectState()
{
	XObj::ResetObjectState();
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
	if (HasModel() && (mStatus.mFlags & 0x1000ULL)) {
		StagePlacement* placement = mCoord.mPlacement;
		SetAnim(placement->mStartAnim, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
	}
}

/**
 * @note Address: 0x8011768C
 * @note Size: 0x24C
 */
BOOL XObjDust::LoadFromPlacement(const CoordInfo& info)
{
	mPlacementParam1 = info.mPlacement->mParam1;
	if (IsFlagSet())
		return TRUE;
	BOOL result = XObj::LoadFromPlacement(info);
	if (result) {
		mStatus.mFlags |= 0x18100000ULL;
		f32 end = GetAnimEndFrame();
		if (end > 0.0f) {
			f32 frame                 = fmod((f32)rand(), end); // a random start frame
			StagePlacement* placement = mCoord.mPlacement;
			SetAnim(placement->mStartAnim, TRUE, frame, 1.0f, 1.0f, 15.0f);
		}
		mAnimId2   = GetAnimId();
		mAnimCount = mObject.m_resFile.GetResAnmChrNumEntries();
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
 * @note Address: 0x801178D8
 * @note Size: 0xC
 */
void XObjDust::Release()
{
	mMatAnim = NULL;
	XObj::Release();
}

#include "game/xobjflags.h"
#include "game/xobjhbox.h"
#include "game/xobjevent.h"
#include "game/cgs.h"
#include <math.h>
#include <stdlib.h>
#include "game/gmath.h"
#include "game/csound.h"
#include "game/globals.h"
#include "game/stagedata.h"
#include "game/xchr.h"

#include <sysdolphin/axdriver.h>

/**
 * @note Address: 0x800DF9A0
 * @note Size: 0x184
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
	if (mJObj != NULL && (mStatus.mFlags & 0x1000ULL)) {
		StagePlacement* placement = mCoord.mPlacement;
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
s32 XObjHBox::OnPlayerContact(const CVec&, s32 flags)
{
	if ((flags & 3) == 1) // not a new touch
		return flags;
	if (mAnimId == 0)
		return flags;
	gGame.GetPlayer()->mFallDistance = 0.0f;
	mStatus.mFlags |= 0x4000000000000000ULL;
	mPart.mHit = 1.0f;
	PlayGameSound(&gGame.mSound, 285, 255);
	TriggerEvent(-1, -1);
	HSD_AudioSFXKeyOff(mPart.mSfx);
	return 0;
}

/**
 * @note Address: 0x800DFC68
 * @note Size: 0xBC
 */
void XObjHBox::OnHitFromPosition(const Vec& pos, f32 strength)
{
	if (mAnimId <= 0) {
		XObj::OnHitFromPosition(pos, strength);
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
		u32 color = (u32)(255.0f * ring) | 0xFFFFFF00;
		DrawGsCircle(&gGame.mGs, 16, color, 0xFFFFFF00, x, y, 0.0f, inner, inner, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, 0xFFFFFF00, x, y, 0.0f, outer, outer, 0.0f, 0.0f, 0.0f);
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
		XObj::DrawGroundShadowAt(origin, opacity);
}

/**
 * @note Address: 0x800DFF5C
 * @note Size: 0x78
 */
void XObjHBox::DrawWithView(Mtx view)
{
	XObj::DrawWithView(view);
	if (mPart.mObj.mStatus.mFlags & 0x10000ULL) // only when the part is drawn
		mPart.mObj.DrawWithView(view);
}

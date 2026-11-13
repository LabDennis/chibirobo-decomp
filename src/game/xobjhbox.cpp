#include "game/xobjflags.h"
#include "game/xobjhbox.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include <math.h>
#include <stdlib.h>
#include "game/gmath.h"
#include "game/csound.h"
#include "game/globals.h"
#include "game/stagedata.h"
#include "game/evt.h"
#include "game/xchr.h"

extern "C" {
#include <sysdolphin/axdriver.h>
}

/**
 * @note Address: 0x800DF9A0
 * @note Size: 0x184
 */
void XObjDust::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	if (GetObjFlags() & 1) {
		mBaseY        = mMoveTranslate.y;
		mPhase        = fmod((f32)rand(), TWO_PI);
		mPhaseStep    = (rand() % 200 + 200) / 10000.0f;
		mBouncePhase  = 0.0f;
		mBounceHeight = 0.0f;
		mHeading      = NAN;
		mSpeed        = 0.0f;
		mFade         = 1.0f;
	}
	mEventId  = -1;
	mEventArg = -1;
	mMatAnim  = NULL;
	if (mJObj != NULL && (mStatus.mFlags & 0x1000ULL)) {
		UnkStageObj* placement = mCoord._00;
		SetAnim(placement->_30, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	}
}

/**
 * @note Address: 0x800DFB24
 * @note Size: 0x44
 */
extern "C" s32 GetFlagIndex__8XObjDustFv(XObjDust* dust)
{
	return dust->GetPlacementFlagIndex(2);
}

/**
 * @note Address: 0x800DFB68
 * @note Size: 0x100
 */
extern "C" u32 vf100__8XObjHBoxFll(XObjHBox* box, s32 unused, u32 flags)
{
	if ((flags & 3) == 1)
		return flags;
	if (box->mAnimId == 0)
		return flags;
	gGame.GetPlayer()->_9A90 = 0.0f;
	box->mStatus.mFlags |= 0x4000000000000000ULL;
	box->mPart.mHit = 1.0f;
	PlayGameSound(&gGame.mSound, 285, 255);
	const void* script = box->vf128();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer())) {
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, box, -1, -1);
	}
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
		if ((mStatus.mFlags & 0x04000000ULL) && strength >= (s32)GetPlayerActionParameter(31) / 100.0f)
			mStatus.mFlags |= 0x2000000000000000ULL;
	}
}

/**
 * @note Address: 0x800DFD24
 * @note Size: 0x1F0
 */
extern "C" void vfC8__8XObjHBoxFv(XObjHBox* box)
{
	if (box->mRing > 0.0f) {
		Vec pos = { 0.0f, 0.0f, 0.0f };
		f32 x, y;
		box->ProjectOffsetToScreen(&pos, &x, &y, NULL, 0);
		f32 ring  = box->mRing;
		f32 inner = 400.0f * ring;
		f32 outer = 400.0f * (1.0f - ring);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
		// retail adds an int zero to both radii
		s32 pad   = 0;
		u32 color = (u32)(255.0f * ring) | 0xFFFFFF00;
		DrawGsCircle(&gGame.mGs, 16, color, 0xFFFFFF00, x, y, 0.0f, inner + pad, inner + pad, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, 0xFFFFFF00, x, y, 0.0f, outer + pad, outer + pad, 0.0f, 0.0f, 0.0f);
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
	if (mPart.mObj.mStatus.mFlags & 0x10000ULL)
		DrawGroundShadowAt__4XObjFPC3Vecf(this, origin, opacity);
}

/**
 * @note Address: 0x800DFF5C
 * @note Size: 0x78
 */
void XObjHBox::DrawWithView(Mtx view)
{
	DrawWithView__4XObjFPA4_f(this, view);
	if (mPart.mObj.mStatus.mFlags & 0x10000ULL)
		mPart.mObj.DrawWithView(view);
}

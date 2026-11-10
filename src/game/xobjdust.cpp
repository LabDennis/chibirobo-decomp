#include "game/xobjflags.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/gmath.h"
#include "game/hsd_cobj.h"
#include <stdlib.h>
#include "game/evt.h"
#include "game/xobjchr.h"

extern "C" BOOL UpdateDustEvent(XObjDust* dust, u32 frames);

/**
 * @note Address: 0x800DF0A0
 * @note Size: 0x238
 */
extern "C" BOOL CheckDustScatterHeading(XObjDust* dust)
{
	if (__fpclassifyf(dust->mHeading) == FP_NAN) {
		if (gGame.GetPlayer()->mAnimId == 0x27) {
			f32 z        = dust->mMoveTranslate.z - gGame.GetPlayer()->mMoveTranslate.z;
			f32 x        = dust->mMoveTranslate.x - gGame.GetPlayer()->mMoveTranslate.x;
			dust->mSpeed = sqrtf(x * x + z * z);
			if (dust->mSpeed < 20.0f) {
				dust->mHeading = atan2(dust->mMoveTranslate.x - gGame.GetPlayer()->mMoveTranslate.x,
				                       dust->mMoveTranslate.z - gGame.GetPlayer()->mMoveTranslate.z);
				dust->mSpeed   = 0.2f * (25.0f - dust->mSpeed);
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
		f32 phase          = 0.02f * frames + dust->mBouncePhase;
		dust->mBouncePhase = phase;
		if (phase > PI) {
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
	if (dust->mStatus.mFlags & 0x4000000000000000ULL) {
		f32 opacity = dust->mFade - 0.02f * frames;
		dust->mFade = opacity;
		if (opacity <= 0.0f)
			dust->mFade = 0.0f;
		PSVECScale((Vec*)&dust->mMoveScale, (Vec*)&dust->mMoveScale, 1.12f * dust->mFade);
		if (0.0f == PSVECMag((Vec*)&dust->mMoveScale)) {
			if (dust->mStatus.mFlags & 0xC0000000000ULL) {
				s32 arg            = dust->mEventArg;
				s32 id             = dust->mEventId;
				const void* script = dust->vf128();
				if (!IsPlayerBlockingEvents(gGame.GetPlayer()))
					StartOrQueueEventScript(&gGame.mEvt, (u8*)script, (u32)dust, id, arg);
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
extern "C" void UpdateObject__8XObjDustFUl(XObjDust* dust, u32 frames)
{
	UpdateObject__4XObjFUl(dust, frames);
	if (dust->GetObjFlags() & 1) {
		if (UpdateDriftingDust(dust, frames)) {
			dust->Release();
			dust->SetFlag();
		}
	}
	if (dust->GetObjFlags() & 2) {
		if (UpdateDustEvent(dust, frames)) {
			dust->Release();
			dust->SetFlag();
		}
	}
}

/**
 * @note Address: 0x800DF6D0
 * @note Size: 0x28
 */
extern "C" void Release__8XObjDustFv(XObjDust* dust)
{
	dust->mMatAnim = NULL;
	Release__4XObjFv(dust);
}

/**
 * @note Address: 0x800DF6F8
 * @note Size: 0x2A8
 */
extern "C" BOOL LoadFromPlacement__8XObjDustFRC9CoordInfo(XObjDust* dust, CoordInfo* info)
{
	dust->_260 = info->_00->_2C;
	if (dust->IsFlagSet())
		return TRUE;
	BOOL result = LoadFromPlacement__4XObjFRC9CoordInfo(dust, info);
	if (result) {
		dust->mStatus.mFlags |= 0x18100000ULL;
		HSD_AObj* aobj = GetAObj(dust->mJObj);
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		if (end > 0.0f) {
			f32 frame              = fmod((f32)rand(), end);
			UnkStageObj* placement = dust->mCoord._00;
			dust->SetAnim(placement->_30, TRUE, NULL, NULL, NULL, frame, 1.0f, 1.0f, 10.0f);
		}
		dust->mAnimId2   = dust->mAnimId;
		dust->mAnimCount = dust->mArchive.mNum[0];
		if (dust->GetObjFlags() & 1) {
			f32 scale = (rand() % 100 + 100) / 100.0f;
			PSVECScale((Vec*)&dust->mMoveScale, (Vec*)&dust->mMoveScale, scale);
		}
		if (dust->GetObjFlags() & 2) {
			dust->mStartScale.x = dust->mMoveScale.x;
			dust->mStartScale.y = dust->mMoveScale.y;
			dust->mStartScale.z = dust->mMoveScale.z;
		}
	}
	return result;
}

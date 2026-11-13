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

// todo: cleanup, game/xobjfreeevent.cpp and game/xobjdustevent.cpp have the same
static inline void TriggerEvent(XObj* object, u32 first, u32 second)
{
	const void* script = object->GetEventScript();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer()))
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, (u32)object, first, second);
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

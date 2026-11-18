#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "arithmetic.h"         // the game-side FSqrt (with the domain warning the nw4r headers then had)
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/wii/cpad.h"
#include "game/evt.h"
#include "game/globals.h"
#include "game/gmath.h"
#include "JSystem/JMath/Inline.h"
extern "C" {
#include <Dolphin/mtx.h>
#include <Dolphin/vec.h>
}
#include <math.h>
#include <stdlib.h>

// retail inlines the vec scale as paired-single code (psq_l/ps_muls0), like xobjhbox.cpp
#define PSVECScale JMathInlineVEC::PSVECScale

// the Wii's dust pile update and billboard half. The GameCube keeps the whole dust object in
// xobjdust.cpp; on the Wii the flag/reset/placement half sits in the xobjhbox.cpp unit ahead of
// this one and the event/billboard functions are free-standing. fn_80117E00 is the GameCube's
// UpdateDustEvent, fn_80118058 the billboard body DrawObjectEffects shares, and SetAnim a
// passthrough to the implementation body at fn_800AF2C8 (its own unit keeps the real name).

extern "C" BOOL IsPlayerBlockingEvents(const WiiPlayerState* player);
extern "C" s32 ClassifyFloatBits(f32 value); // the Wii MSL's out-of-line __fpclassifyf
extern "C" BOOL UpdateDriftingDust(XObjDust* dust, u32 frames);
extern "C" BOOL fn_80117E00(XObjDust* dust, u32 frames);
extern "C" void fn_80118058(XObjDust* dust, s32);
extern "C" void fn_800AF2C8(XObj* object, s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate);

static inline void TriggerEvent(XObj* object, s16 first, s16 second)
{
	const void* script = object->GetEventScript();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer())) {
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, object, first, second);
	}
}

/**
 * @note Address: 0x801178E4
 * @note Size: 0x148
 */
void XObjDust::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	if (GetObjFlags() & 1) {
		if (UpdateDriftingDust(this, frames)) {
			Release();
			SetFlag();
		}
	}
	if (GetObjFlags() & 2) {
		if (fn_80117E00(this, frames)) {
			Release();
			SetFlag();
		}
	}
}

// A stick flick sends loose dust hopping; the Wii inlines it into UpdateDriftingDust.
static inline BOOL CheckDustScatterHeading(XObjDust* dust)
{
	if (ClassifyFloatBits(dust->mHeading) == FP_NAN) {
		if (gGame.GetPlayer()->GetAnimId() == 0x27) { // the player's anim that scatters dust
			CVec* pos    = gGame.GetPlayer()->GetPos();
			f32 z        = dust->mMoveTranslate.z - pos->z;
			f32 x        = dust->mMoveTranslate.x - pos->x;
			dust->mSpeed = FSqrt(x * x + z * z);
			if (dust->mSpeed < 20.0f) { // only close dust scatters
				pos            = gGame.GetPlayer()->GetPos();
				dust->mHeading = atan2(dust->mMoveTranslate.x - pos->x, dust->mMoveTranslate.z - pos->z);
				dust->mSpeed   = 0.2f * (25.0f - dust->mSpeed); // away from the player, capped at 2
				if (dust->mSpeed > 2.0f) {
					dust->mSpeed = 2.0f;
				}
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
 * @note Address: 0x80117A2C
 * @note Size: 0x3D4
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
			if (speed <= 0.0f) {
				dust->mSpeed = 0.0f;
			}
		}
		f32 x = sin(dust->mHeading);
		dust->mMoveTranslate.x += dust->mSpeed * x;
		f32 z = cos(dust->mHeading);
		dust->mMoveTranslate.z += dust->mSpeed * z;
	}
	if (dust->mStatus.mFlags & 0x4000000000000000ULL) { // scattered dust fades out
		f32 opacity = dust->mFade - 0.02f * frames;
		dust->mFade = opacity;
		if (opacity <= 0.0f) {
			dust->mFade = 0.0f;
		}
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

// event 16 waits for its own anim, event 56 shrinks it towards mTo while chibi robo plays anim 0x8C0
/**
 * @note Address: 0x80117E00
 * @note Size: 0x1C0
 */
extern "C" BOOL fn_80117E00(XObjDust* dust, u32)
{
	if (dust->mStatus.mFlags & 0x4000000000000000ULL) {
		if (dust->mEventId == 16) {
			if (dust->IsAnimDone()) {
				if (dust->mStatus.mFlags & 0xC0000000000ULL) {
					TriggerEvent(dust, dust->mEventId, dust->mEventArg);
				}
				return TRUE;
			}
		} else if (dust->mEventId == 56) {
			if (gGame.GetPlayer()->GetAnimId() == 0x8C0) {
				WiiPlayerState* player = gGame.GetPlayer();
				if (player->IsAnimDone()) {
					return TRUE;
				}
				f32 end                = player->GetAnimEndFrame();
				f32 frame              = player->GetAnimFrame();
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
 * @note Address: 0x80117FC0
 * @note Size: 0x60
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
 * @note Address: 0x80118020
 * @note Size: 0x38
 */
void XObjDust::DrawObjectEffects()
{
	if (GetObjFlags() & 1) {
		fn_80118058(this, 0);
	}
}

// NONMATCHING: DrawObjectEffects' body split into a shared free function; the second argument
// goes unread in the retail body — what selected it is undecoded
/**
 * @note Address: 0x80118058
 * @note Size: 0x17C
 */
extern "C" void fn_80118058(XObjDust* dust, s32)
{
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	gGame.mGs.mBillboardEnabled = TRUE;
	f32 width                   = 8.0f * dust->mMoveScale.x; // the billboard's base size
	f32 height                  = 8.0f * dust->mMoveScale.y;
	u32 color;
	if (ClassifyFloatBits(dust->mHeading) != FP_NAN) {
		color = 0xC8C8C800;
	} else if (dust->mStatus.mFlags & 0x4000000000000000ULL) { // a different tint while the hit flag is set
		color = 0xC8646400;
	} else {
		color = 0xC8C8C800;
	}
	s32 alpha = (s32)(255.0f * dust->mFade);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	Vec rotation = { 0.0f, 0.0f, 0.0f };
	DrawGsCircle(&gGame.mGs, 16, color | (u8)alpha, color, dust->mMoveTranslate.x, dust->mMoveTranslate.y, dust->mMoveTranslate.z, width,
	             height, rotation.x, rotation.y, rotation.z);
	gGame.mGs.mBillboardEnabled = FALSE;
}

// NONMATCHING: the dust's SetAnim forwards to the shared body at fn_800AF2C8 (SetAnim-shaped,
// another unit's); written as the retail tail-branch, its real name is that unit's
/**
 * @note Address: 0x801181D4
 * @note Size: 0x4
 */
void XObjDust::SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	fn_800AF2C8(this, id, force, frame, rate, blend, blendRate);
}

// A stick flick sends loose dust hopping.
/**
 * @note Address: 0x801181D8
 * @note Size: 0x188
 */
s32 XObjDust::OnPlayerContact(s32, s32 flags)
{
	if ((flags & 3u) == 1) {
		return flags;
	}
	if (GetObjFlags() & 1) {
		if (0.0f == mBouncePhase) {
			CPad pad(0);
			mBouncePhase  = DEG2RAD;
			f32 x         = pad.mPad->mStick.x;
			f32 y         = pad.mPad->mStick.y;
			f32 magnitude = FSqrt(x * x + y * y);
			f32 height    = (rand() % 400 + 400) / 100.0f;
			mBounceHeight = height * magnitude;
		}
	}
	return 0;
}

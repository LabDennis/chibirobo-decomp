// chibi robo's tumble action, the GameCube's xacttumble.cpp plus the tumble helpers its
// xactradar.cpp carried: retail 0x801067C0-0x80106D60 (the tail of wii_units.md Nr. 67, right
// behind objb_act.cpp). ResetTumbleAction clears the idle effect where the GameCube cleared
// mIdleParticle; FinishTumbleAction is the GameCube's inline FinishTumble emitted out of line
// for the other units that call it, additionally retiring the sleep effect the Wii plays as an
// event effect; HandleTumbleActionInput reads the remote's buttons through gWiiPointer instead
// of the pad trigger. UpdatePlayerTumbleAnimation (0x80106D60) follows, still to port.
#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/xact.h"
#include <math.h>
#include "arithmetic.h"

extern "C" void fn_800A12F8(void* effects, u32 id, BOOL retireParticles); // stops the event effect with that id

// how far the stick is pushed
static inline f32 StickLength(const CPadOne* pad)
{
	f32 y = pad->mStick.y;
	f32 x = pad->mStick.x;
	return FSqrt(x * x + y * y);
}

/**
 * @note Address: 0x801067C0
 * @note Size: 0x40
 */
extern "C" void ResetTumbleAction(XActTumble* act)
{
	act->Reset();
	act->mIdleEffect = NULL;
}

/**
 * @note Address: 0x80106800
 * @note Size: 0x8C
 */
// the GameCube's static inline FinishTumble, a real function on the Wii: XObjC's UpdateObject
// and XObjSpider::State3 call it, and it also retires the tumble sleep effect (event effect 5)
extern "C" void FinishTumbleAction(XActTumble* act)
{
	WiiPlayerState* owner = act->mOwner;
	owner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
	owner->mIdleLoops = 3;
	act->mState       = 0;
	fn_800A12F8(&gGame.mEvtPtcl, 5, TRUE);
}

/**
 * @note Address: 0x8010688C
 * @note Size: 0x4D4
 */
extern "C" void HandleTumbleActionInput(XActTumble* act, const CPadOne* pad, f32* weight)
{
	// lying down (2, 16) a button or the stick gets him up; after the battery ran out (5) only a button
	if (act->mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL
		    && (gWiiPointer.Check(1) || gWiiPointer.Check(2) || StickLength(pad) > 0.0f)) {
			act->mState = 3;
		}
		*weight = 0.0f;
	} else if (act->mState == 16) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL
		    && (gWiiPointer.Check(1) || gWiiPointer.Check(2) || StickLength(pad) > 0.0f)) {
			act->mState = 17;
		}
		*weight = 0.0f;
	} else if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && (gWiiPointer.Check(1) || gWiiPointer.Check(2))) {
			act->mState = 6;
		}
		*weight = 0.0f;
	} else if (act->mState == 7) {
		*weight = 0.8f;
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

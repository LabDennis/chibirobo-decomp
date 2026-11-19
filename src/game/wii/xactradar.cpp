// chibi robo's tumble fade and the ring on the floor around him, the GameCube's
// xactradar.cpp tail: retail 0x801078C8-0x80107D40, right behind the tumble actions in
// wii/xacttumble.cpp. The fade struct is the GameCube's, the draws go through the Wii
// CGs layer; HandleCopterActionInput reads the remote's B button through gWiiPointer
// where the GameCube read the pad trigger, and plays the descent animation on the
// copter object at _B580.
#include "game/wii/cgs.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/gmath.h"
#include <math.h>

extern "C" BOOL IsPlayerStopped(const WiiPlayerState* chr);
extern "C" void ShowAndDeployCopter(XObj* copter);
extern "C" void PlayCopterStowAnimation(XObj* copter);
extern "C" void PlayCopterDescentAnimation(XObj* copter);
extern "C" void PlayCopterLandingAnimation(XObj* copter);
extern "C" CVec GetPlayerFloorSlope(WiiPlayerState* chr);
extern "C" u32 GetCopterParameter(s32 index);
extern "C" s32 gRefreshRate;

// the tumble ring sits just above the floor
static inline Vec RingPos(const Vec& pos, const Vec& offset)
{
	Vec v = { pos.x + offset.x, 0.01f + pos.y, pos.z + offset.z };
	return v;
}

extern "C" {

/**
 * @note Address: 0x801078C8
 * @note Size: 0xAC
 */
void UpdateTumbleFade(XActTumbleFade* fade, BOOL increase, u32 frames)
{
	if (fade->mActive) {
		if (increase) {
			if (fade->mAmount < 1.0f) {
				if ((fade->mAmount += 0.01f * frames) > 1.0f) {
					fade->mAmount = 1.0f;
				}
			}
		} else if (fade->mAmount > 0.0f) {
			if ((fade->mAmount -= 0.05f * frames) < 0.0f) {
				fade->mAmount = 0.0f;
				fade->mActive = FALSE;
			}
		}
	}
}

// the ring on the floor while he tumbles
/**
 * @note Address: 0x80107974
 * @note Size: 0x2DC
 */
// NONMATCHING: 97.31% - the CVec offset copy schedules as three batched loads then three
// stores in retail (z,y,x into f3,f2,f0), ours interleaves load/store pairs; some callee
// register colours differ in the loop constants. The ring loop itself matches, including
// the per-call rematerialization of the angle.
void DrawTumbleRing(XActTumbleFade* fade, const Vec& pos, f32 angle)
{
	if (fade->mAmount > 0.0f) {
		CVec offset(fade->mOffset);
		Mtx m;
		PSMTXRotRad(m, 'y', angle);
		PSMTXMultVec(m, &offset, &offset);
		Vec center = RingPos(pos, offset);
		f32 size   = 6.0f * fade->mAmount;
		u32 color  = (u32)(160.0f * fade->mAmount) | 0x641E00;
		Vec rot    = { -HALF_PI, 0.0f, 0.0f };
		DrawGsCircle(&gGame.mGs, 0x18, color, color, center.x, center.y, center.z, size, size, rot.x, rot.y, rot.z);
		PrimVertex verts[50];
		f32 outer = size * 0.5f + 1.0f;
		for (int i = 0; i < 25; i++) {
			verts[i * 2].pos.x     = center.x - size * std::sinf((TWO_PI / 24) * i) / 2.0f;
			verts[i * 2].pos.z     = center.z + size * std::cosf((TWO_PI / 24) * i) / 2.0f;
			verts[i * 2].pos.y     = center.y;
			verts[i * 2].color     = color;
			verts[i * 2 + 1].pos.x = center.x - outer * std::sinf((TWO_PI / 24) * i);
			verts[i * 2 + 1].pos.z = center.z + outer * std::cosf((TWO_PI / 24) * i);
			verts[i * 2 + 1].pos.y = center.y;
			verts[i * 2 + 1].color = 0;
		}
		DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, verts, 50);
	}
}

// the copter: B or standing still ends the flight
/**
 * @note Address: 0x80107C50
 * @note Size: 0xF0
 */
// NONMATCHING: 95.92% - GC/3.0a5 folds the 7||8 equality tests into a range check
// (subi r0, r4, 7 / cmplwi / bgt) where retail and the GameCube's GC/2.6 emit cmpwi 7,
// cmpwi 8; mState also colors r4 here against retail's r0, the GameCube build kept r4
// live through pad->mTrigger. A switch for the states compiled to a bge tree instead.
void HandleCopterActionInput(XAction* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 3) {
		if (gWiiPointer.Check(2) || IsPlayerStopped(act->mOwner)) {
			act->mState = 4;
			PlayCopterDescentAnimation((XObj*)act->mOwner->mCopterObj);
		}
		*weight = 1.0f;
	} else if (act->mState == 4 || act->mState == 7 || act->mState == 8) {
		if (!gWiiPointer.Check(2)) {
			IsPlayerStopped(act->mOwner);
		}
		*weight = 1.0f;
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

// the propeller joint's bob, the GameCube's UpdateCopterBobbing: the Wii moves the player
// model's "second_null" node where the GameCube walked a joint path
static inline void UpdateCopterBobbing(XActCopter* act, u32 frames, f32 speed)
{
	if (speed > 0.0f) {
		act->mPhase = fmod(speed * frames + act->mPhase, TWO_PI);
		act->mBob   = 0.4f * (f32)sin(act->mPhase) + 0.4f;
		act->mOwner->GetResNode("second_null").SetTranslate(0.0f, act->mBob, 0.0f);
	} else {
		if (act->mBob > 0.0f) {
			if ((act->mBob -= 0.2f * frames) <= 0.0f) {
				act->mBob = 0.0f;
			}
		}
		act->mOwner->GetResNode("second_null").SetTranslate(0.0f, act->mBob, 0.0f);
	}
}

// flies him while the copter is out
/**
 * @note Address: 0x80107D40
 * @note Size: 0x888
 */
void UpdatePlayerCopterFlight(XActCopter* act, s32 anim, WiiPlayerState* owner, u32 frames, f32 frame,
							  f32 end)
{
	// the flight time (copter parameter 0, in hundredths of a second) fills up while idle and runs down in flight
	if (act->mState == 0) {
		act->mTime = gRefreshRate * ((s32)GetCopterParameter(0) / 100.0f);
	} else if (act->mState == 3) {
		if ((act->mTime -= frames) < 0.0f) {
			act->mTime  = 0.0f;
			act->mState = 4;
		}
	}
	// 1 takes the copter out, 2 puts it away, 3 flies. 4 and 5 come down and 6 lands, 7 (off a rope)
	// and 8 (off a ledge) come down and 9 lands; on the Wii the anim-done flag is the player's
	// IsAnimDone where the GameCube read the aobj flags
	switch (act->mState) {
	case 1:
		if (anim != 0x814) {
			act->mOwner->SetAnim(0x814, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			ShowAndDeployCopter((XObj*)act->mOwner->mCopterObj);
		} else if (owner->IsAnimDone()) {
			act->mOwner->SetAnim(0x4A, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = 0;
		}
		break;
	case 2:
		if (anim != 0x817) {
			act->mOwner->SetAnim(0x817, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayCopterStowAnimation((XObj*)act->mOwner->mCopterObj);
		} else if (owner->IsAnimDone()) {
			WiiPlayerState* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	case 3:
		UpdateCopterBobbing(act, frames, DEG2RAD * 5);
		break;
	case 4:
		UpdateCopterBobbing(act, frames, -1.0f);
		if (anim != 0x28) {
			act->mOwner->SetAnim(0x28, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayCopterDescentAnimation((XObj*)act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x816, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObj*)act->mOwner->mCopterObj);
			act->mState = 6;
		}
		break;
	case 5:
		UpdateCopterBobbing(act, frames, -1.0f);
		if (anim != 0x28) {
			act->mOwner->SetAnim(0x28, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			((XObj*)act->mOwner->mCopterObj)->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		if (act->mOwner->GetHeight() <= 0.0f ||
			(act->mOwner->GetHeight() <= 1.0f && GetPlayerFloorSlope(act->mOwner).Length() > 0.5f)) {
			act->mOwner->SetAnim(0x816, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObj*)act->mOwner->mCopterObj);
			act->mState = 6;
		}
		break;
	case 6:
		UpdateCopterBobbing(act, frames, -1.0f);
		if (owner->IsAnimDone()) {
			WiiPlayerState* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	case 7:
		if (anim != 0x38) {
			act->mOwner->SetAnim(0x38, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayCopterDescentAnimation((XObj*)act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x59, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObj*)act->mOwner->mCopterObj);
			act->mState = 9;
		}
		break;
	case 8:
		if (anim != 0x1A) {
			act->mOwner->SetAnim(0x1A, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayCopterDescentAnimation((XObj*)act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x1B, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObj*)act->mOwner->mCopterObj);
			act->mState = 9;
		}
		break;
	case 9:
		if (owner->IsAnimDone()) {
			WiiPlayerState* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	}
}

} // extern "C"

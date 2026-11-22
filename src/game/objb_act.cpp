#include "game/xchr.h"
#include "game/xact.h"
#include "game/xaction.h"
#include "game/xobjleticker.h"
#include "game/xobjflags.h"
#include "game/xobjs.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/ccamera.h"
#include "game/cjobjset.h"
#include "game/cvec.h"
#include "game/cparticle.h"
#include "game/cgameparticle.h"
#include "game/cscrn.h"
#include "game/csound.h"
#include "game/fbtexeff.h"
#include "game/globals.h"
#include "game/hsd_dobj.h"
#include "game/objectfloorquery.h"
#include "game/stagedata.h"
#include "game/video.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/mtx_transform.h>
}

// chibi robo's actions (radar, popper, copter, tumble, push, rope, ladder, jump, hang, lift) and their helpers.
// Named objb_act.cpp: no __FILE__ or panic string for this file exists in retail (only jobj.h/aobj.h), matching Wii's
// game/wii/objb_act.cpp. NONMATCHING: header blocks (six-Vec .rodata 0x8020A588, .bss prefix 0x8037C278) are not hand-built; the floor
// template slots at .bss +0xBC/+0xC8 stay the lbl_8037C334/lbl_8037C340 externs as in obja.cpp and obja_chr.cpp.

/**
 * @note Address: 0x800BD320
 * @note Size: 0x4
 */
extern "C" void DrawPlayerRadarEffects(XActRadar*)
{
}

extern "C" CVec GetRadarDishWorldPosition(XObjRadar*);
extern "C" void PSMTXMultVecArray(const Mtx, const Vec*, Vec*, u32);

static inline u32 PackRadarColor(const ScrnColorF& c)
{
	return ((u32)c.r << 24) | ((u32)c.g << 16) | ((u32)c.b << 8) | (u32)c.a;
}

static inline u16 ShadeRadarBeam(XActRadar* act, u32* colors, u32 base)
{
	u16 n = 0;
	int turn, step;
	for (turn = 0; turn < 2; ++turn) {
		for (step = 0; step < 16; ++step) {
			colors[n]     = base | (u8)(s32)(150.f * ((1.f / 64) * n) * act->mFade);
			colors[n + 1] = base;
			n += 2;
		}
	}
	for (turn = 0; turn < 6; ++turn) {
		for (step = 0; step < 16; ++step) {
			colors[n]     = base | (u8)(s32)(150.f * (1.f - (1.f / 192) * (turn * 32 + step)) * act->mFade);
			colors[n + 1] = base;
			n += 2;
		}
	}
	return n;
}

static inline f32 TargetPitch(const Vec& from, const Vec& to)
{
	f32 dz = to.z - from.z, dx = to.x - from.x, dy = to.y - from.y;
	f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
	return dy < 0 ? HALF_PI - pitch : pitch - HALF_PI;
}

// NONMATCHING: retail takes the yaw's z difference before the x one and numbers the float registers otherwise;
// ShadeRadarBeam's loop counters get their own zero in retail. The dol has 129 atan2(a.x - b.x, a.z - b.z)
// calls and evaluates the x argument first at all but four: this one, UpdateRadarTargeting's dish case,
// UpdatePopperShotFlight and one spider boss site where the z operand was already in a register. Only an
// extra operation on the z argument (fneg, a double subtraction) moves it first in this build; casts,
// unary plus, 1.0f *, dz/dx locals, an atan2f-style inline, a yaw local and a const CVec& for the target
// all keep x first
extern "C" void DrawRadarTargetBeam(XActRadar* act)
{
	if (!(act->mFade > 0))
		return;
	CVec origin = GetRadarDishWorldPosition(&act->mOwner->mRadarObj);
	CVec target = act->mTarget->mMoveTranslate + (Vec) { 0, act->mTarget->GetCollisionHeight() / 2.f, 0 };
	f32 yaw     = atan2(target.x - origin.x, target.z - origin.z);
	f32 pitch   = TargetPitch(origin, target) - DEG2RAD * 3;
	f32 span    = PSVECDistance(&origin, &target) / 20;
	if (span > 1)
		span = 1;
	Mtx trans, scale, rotY, rotX, model;
	PSMTXTrans(trans, origin.x, origin.y, origin.z);
	PSMTXScale(scale, 2, 2, 1);
	PSMTXRotRad(rotY, 'y', yaw);
	PSMTXRotRad(rotX, 'x', pitch);
	PSMTXConcat(trans, rotY, model);
	PSMTXConcat(model, rotX, model);
	PSMTXConcat(model, scale, model);

	Vec vertices[256];
	u32 colors[256];
	u16 n = 0;
	f32 z = 2, phase = act->mAngle[0];
	for (int turn = 0; turn < 8; ++turn) {
		for (int step = 0; step < 16; ++step) {
			vertices[n].x
			    = (.007f * n + 1) * (std::sinf((TWO_PI / 16) * step) * (.1f * std::sinf((TWO_PI / 16) * turn + act->mAngle[1]) + 1));
			vertices[n].y
			    = (.007f * n + 1) * (std::cosf((TWO_PI / 16) * step) * (.1f * std::sinf((TWO_PI / 16) * turn + act->mAngle[1]) + 1));
			vertices[n].z = z;
			f32 wave      = WrapAngle(phase);
			if (wave > PI)
				wave = 0;
			vertices[n + 1].x = vertices[n].x - ((.5f * std::sinf(wave)) * std::sinf((TWO_PI / 16) * step));
			vertices[n + 1].y = vertices[n].y - ((.5f * std::sinf(wave)) * std::sinf((TWO_PI / 16) * step));
			vertices[n + 1].z = vertices[n].z - 1.5f * std::sinf(wave);
			n += 2;
			phase += DEG2RAD * 30;
			z += .1f * span;
		}
	}
	PSMTXMultVecArray(model, vertices, vertices, n);
	u32 back = PackRadarColor(act->mBeam.mColor[0].mColor), front = PackRadarColor(act->mBeam.mColor[1].mColor);
	SetGsCullMode(gGame.mGs.mCurRenderMode, 2);
	n = ShadeRadarBeam(act, colors, back);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, colors, n);
	SetGsCullMode(gGame.mGs.mCurRenderMode, 1);
	n = ShadeRadarBeam(act, colors, front);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, colors, n);
	SetGsCullMode(gGame.mGs.mCurRenderMode, 2);
}

extern "C" {
void ProjectWorldPositionToScreen(CGame* game, const Vec& pos, f32* x, f32* y, f32* z, CCamera* camera); // where pos is on the screen
void SetObjectJointTranslationByPath(CXObj* obj, s32 depth, const s32* path, f32 x, f32 y, f32 z);       // puts a joint at a translation
BOOL IsPlayerStopped(XObjChr* chr);
void CheckPopperShotImpact(XActPopperShot* shot);
void UpdateCopterBobbing(XActCopter* act, u32 frames, f32 speed);
CVec GetPlayerFloorSlope(XObjChr* chr);
}

// how far the stick is pushed
static inline f32 StickLength(const CPadOne* pad)
{
	f32 x = pad->mStickX;
	f32 y = pad->mStickY;
	return sqrtf(x * x + y * y);
}

// the tumble ring sits just above the floor
static inline Vec RingPos(const Vec& pos, const Vec& offset)
{
	Vec v = { pos.x + offset.x, 0.01f + pos.y, pos.z + offset.z };
	return v;
}

static inline void KillGen(HSD_Generator*& gen)
{
	if (gen != NULL) {
		(&gGame.mParticle)->Kill(gen);
		gen = NULL;
	}
}

static inline void ResetShot(XActPopperShot* shot)
{
	shot->mActive    = FALSE;
	shot->mHitTarget = NULL;
	ClearParticlePool(&shot->mDust.mPtcl);
	shot->mRing[0].mAmount = 0.0f;
	shot->mRing[1].mAmount = 0.0f;
	KillGen(shot->mGen);
	KillGen(shot->mChargeGen);
}

extern "C" {

// fades the radar's marker in while it has something, and spins it
/**
 * @note Address: 0x800BE648
 * @note Size: 0xF0
 */
void UpdateRadarBeamAnimation(XActRadar* act, u32 frames)
{
	if (act->mState == 0 && act->mTarget != NULL && act->mOwner->mTumble.mState == 0) {
		if (act->mFade < 1.0f) {
			if ((act->mFade += 0.05f * frames) > 1.0f) {
				act->mFade = 1.0f;
			}
		}
		act->mAngle[0] -= DEG2RAD * 5 * frames;
		act->mAngle[1] -= DEG2RAD * 5 * frames;
	} else if (act->mFade > 0.0f) {
		if ((act->mFade -= 0.05f * frames) < 0.0f) {
			act->mFade = 0.0f;
		}
	}
}

XObj* FindNearestRadarTarget(XActRadar* act);

void ActivatePlayerRadar(XObjRadar*);
void DeactivatePlayerRadar(XObjRadar*);
void StartPlayerRadarScan(XObjRadar*);
void SetRadarDishPitch(XObjRadar*, f32);
void SetRadarDishYaw(XObjRadar*, f32);
u32 GetRadarParameter(s32);

static inline void StopRadarTones()
{
	XSound& sound = gGame.mSound;
	sound.StopIfPlaying(0x18e);
	sound.StopIfPlaying(0x18d);
	sound.StopIfPlaying(0x18c);
}

static inline void FinishRadarAnimation(XActRadar* act)
{
	XObjChr* owner = act->mOwner;
	owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 1, 10);
	owner->mIdleLoops = 3;
	act->mState       = 0;
	act->mTarget      = NULL;
	act->mYaw.angle = act->mYaw.targetAngle = act->mYaw.reached = 0;
	act->mPitch.angle = act->mPitch.targetAngle = act->mPitch.reached = 0;
	StopRadarTones();
}

static inline void StepRadarPitch(XActRadar* act, u32 frames, f32 step, bool clamp)
{
	RadarAimAxis& pitch = act->mPitch;
	if (pitch.angle < pitch.targetAngle) {
		pitch.angle += step * frames;
		if (pitch.angle >= pitch.targetAngle) {
			if (clamp)
				pitch.angle = pitch.targetAngle;
			pitch.reached = 1;
		}
	} else if (pitch.angle > pitch.targetAngle) {
		pitch.angle -= step * frames;
		if (pitch.angle <= pitch.targetAngle) {
			if (clamp)
				pitch.angle = pitch.targetAngle;
			pitch.reached = 1;
		}
	} else
		pitch.reached = 1;
}

static inline void StartRadarTone(XActRadar* act, s32 tone, u8 r0, u8 g0, u8 b0, u8 r1, u8 g1, u8 b1)
{
	XSound& sound = gGame.mSound;
	if (!IsGameSoundPlaying(&sound, tone)) {
		PlayGameSound(&sound, tone, 255);
		SetScreenFadeTargetColor(&act->mBeam.mColor[0], r0, g0, b0, 0, .5f);
		SetScreenFadeTargetColor(&act->mBeam.mColor[1], r1, g1, b1, 0, .5f);
	}
}

void UpdateRadarTargeting(XActRadar* act, s32 anim, HSD_AObj* aobj, u32 frames, f32, f32)
{
	// 1 takes the radar out, 2 puts it away. a scan starts at 3, 4 spins the dish a full turn and
	// works out the aim, 5 turns it there and 0 keeps it on the target
	switch (act->mState) {
	case 1:
		if (anim != 0x879) {
			act->mOwner->SetAnim(0x879, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			ActivatePlayerRadar(&act->mOwner->mRadarObj);
		} else if (aobj->flags & 0x44000000)
			FinishRadarAnimation(act);
		break;
	case 2:
		if (anim != 0x87a) {
			act->mOwner->SetAnim(0x87a, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			DeactivatePlayerRadar(&act->mOwner->mRadarObj);
		} else if (aobj->flags & 0x44000000)
			FinishRadarAnimation(act);
		break;
	case 3: {
		XObjChr* owner = act->mOwner;
		owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		owner->mIdleLoops = 3;
		StartPlayerRadarScan(&act->mOwner->mRadarObj);
		act->mState  = 4;
		act->mTarget = FindNearestRadarTarget(act);
		// holding up on the d-pad makes the scan find nothing
		if (gGame.mPad.mPad[0].mButton & 8)
			act->mTarget = NULL;
		act->mYaw.targetAngle   = TWO_PI + act->mYaw.angle;
		act->mYaw.reached       = 0;
		act->mPitch.targetAngle = 0;
		act->mPitch.reached     = 0;
		PlayGameSound(&gGame.mSound, 0x44, 255);
		StopRadarTones();
		break;
	}
	case 4:
		if (!act->mYaw.reached) {
			act->mYaw.angle += (TWO_PI / FRAME_RATE) * frames;
			if (act->mYaw.angle >= act->mYaw.targetAngle) {
				act->mYaw.angle = WrapAngle(act->mYaw.angle);
				if (act->mTarget) {
					f32 yaw               = (f32)atan2(act->mTarget->mMoveTranslate.x - act->mOwner->mMoveTranslate.x,
					                                   act->mTarget->mMoveTranslate.z - act->mOwner->mMoveTranslate.z);
					act->mYaw.targetAngle = WrapAngle(yaw - act->mOwner->mMoveRotate.y);
				} else
					act->mYaw.targetAngle = TWO_PI;
				if (act->mYaw.targetAngle < act->mYaw.angle)
					act->mYaw.targetAngle += TWO_PI;
				act->mYaw.reached = 1;
			}
			SetRadarDishYaw(&act->mOwner->mRadarObj, act->mYaw.angle);
		}
		if (!act->mPitch.reached) {
			StepRadarPitch(act, frames, 3 * DEG2RAD, false);
			if (act->mPitch.reached) {
				if (act->mTarget) {
					CVec from               = GetRadarDishWorldPosition(&act->mOwner->mRadarObj);
					CVec to                 = act->mTarget->mMoveTranslate + (Vec) { 0, act->mTarget->GetCollisionHeight() / 2.f, 0 };
					act->mPitch.targetAngle = TargetPitch(from, to);
				} else
					act->mPitch.targetAngle = 0;
			}
			SetRadarDishPitch(&act->mOwner->mRadarObj, act->mPitch.angle);
		}
		if (act->mYaw.reached && act->mPitch.reached) {
			act->mState         = 5;
			act->mYaw.reached   = 0;
			act->mPitch.reached = 0;
		}
		break;
	case 5:
		if (!act->mYaw.reached) {
			act->mYaw.angle += (TWO_PI / FRAME_RATE) * frames;
			if (act->mYaw.angle >= act->mYaw.targetAngle) {
				act->mYaw.angle   = act->mYaw.targetAngle;
				act->mYaw.reached = 1;
			}
			SetRadarDishYaw(&act->mOwner->mRadarObj, act->mYaw.angle);
		}
		if (!act->mPitch.reached) {
			StepRadarPitch(act, frames, DEG2RAD, true);
			SetRadarDishPitch(&act->mOwner->mRadarObj, act->mPitch.angle);
		}
		if (act->mYaw.reached && act->mPitch.reached) {
			act->mState = 0;
			if (act->mTarget)
				PlayGameSound(&gGame.mSound, 0x43, 255);
		}
		break;
	case 0:
		if (act->mTarget) {
			u64 flags = act->mTarget->mStatus.mFlags;
			// the target is gone, hidden or being removed: drop it and turn the dish back to the front
			if (!(flags & 1) || !(flags & 0x10000) || (flags & 0x4000000000000000ULL)) {
				act->mTarget          = NULL;
				act->mYaw.targetAngle = TWO_PI;
				StopRadarTones();
				act->mState = 5;
			} else {
				CVec from       = GetRadarDishWorldPosition(&act->mOwner->mRadarObj);
				CVec to         = act->mTarget->mMoveTranslate + (Vec) { 0, act->mTarget->GetCollisionHeight() / 2.f, 0 };
				f32 yaw         = (f32)atan2(to.x - from.x, to.z - from.z);
				act->mYaw.angle = WrapAngle(yaw - act->mOwner->mMoveRotate.y);
				SetRadarDishYaw(&act->mOwner->mRadarObj, act->mYaw.angle);
				act->mPitch.angle = TargetPitch(from, to);
				SetRadarDishPitch(&act->mOwner->mRadarObj, act->mPitch.angle);
			}
		}
		break;
	}
	if (act->mTarget) {
		CVec from = act->mOwner->mMoveTranslate, to = act->mTarget->mMoveTranslate;
		f32 dist             = PSVECDistance(&from, &to);
		act->mTargetDistance = dist - (act->mOwner->GetFloorXZExtent() + act->mTarget->GetFloorXZExtent());
		if (act->mTargetDistance < 0)
			act->mTargetDistance = 0;
		if (act->mState == 0) {
			// near, middle or far: each has its own tone and turns the beam red, yellow or blue
			if (act->mTargetDistance < (s32)GetRadarParameter(0) / 100.f) {
				StartRadarTone(act, 0x18e, 255, 150, 150, 255, 50, 50);
				gGame.mSound.StopIfPlaying(0x18d);
				gGame.mSound.StopIfPlaying(0x18c);
			} else if (act->mTargetDistance < (s32)GetRadarParameter(1) / 100.f) {
				gGame.mSound.StopIfPlaying(0x18e);
				StartRadarTone(act, 0x18d, 255, 255, 150, 255, 255, 50);
				gGame.mSound.StopIfPlaying(0x18c);
			} else {
				gGame.mSound.StopIfPlaying(0x18e);
				gGame.mSound.StopIfPlaying(0x18d);
				StartRadarTone(act, 0x18c, 150, 150, 255, 50, 50, 255);
			}
		}
		UpdateScreenFade(&act->mBeam.mColor[0], frames);
		UpdateScreenFade(&act->mBeam.mColor[1], frames);
	} else
		act->mTargetDistance = NAN;
}

// the radar's pick, the nearest drawn object that isn't chibi robo himself
/**
 * @note Address: 0x800BE738
 * @note Size: 0x1F8
 */
XObj* FindNearestRadarTarget(XActRadar* act)
{
	XObj* obj;
	CVec pos        = act->mOwner->mMoveTranslate;
	XObj* nearest   = NULL;
	f32 nearestDist = NAN;
	for (int i = 0; i < gGame.mXObjs.mObjectCount; i++) {
		obj = gGame.GetObject(i);
		if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && !(obj->mStatus.mFlags & 0x4000000000000000ULL)
		    && (obj->mStatus.mFlags & 0x80000) && (s32)act->mOwner->mId != (s32)obj->mId && obj->GetKind() < 10000) {
			f32 dist;
			if (nearest == NULL) {
				dist = PSVECDistance(&pos, &obj->mMoveTranslate);
			} else {
				dist = PSVECDistance(&pos, &obj->mMoveTranslate);
				if (!(dist < nearestDist)) {
					dist = NAN;
				}
			}
			if (fpclassify(dist) != FP_NAN) {
				nearest     = obj;
				nearestDist = dist;
			}
		}
	}
	return nearest;
}

/**
 * @note Address: 0x800BFC04
 * @note Size: 0x68
 */
void ApplyRadarMovementWeight(XActRadar* act, const CPadOne* pad, f32* weight)
{
	// with the radar out (attachment 3) he stands still while it comes out, goes away or scans
	if (act->mOwner->GetAttachment() == 3) {
		const s32 state = act->mState;
		if (state == 1 || state == 2) {
			*weight = 0.0f;
		} else {
			if (state != 3 && state != 4 && state != 5)
				return;
			*weight = 0.0f;
		}
	}
}

/**
 * @note Address: 0x800BFC6C
 * @note Size: 0xCC
 */
void ResetPlayerRadarAction(XActRadar* act)
{
	act->Reset();
	act->mTarget    = NULL;
	act->mYaw.angle = act->mYaw.targetAngle = act->mYaw.reached = 0.0f;
	act->mPitch.angle = act->mPitch.targetAngle = act->mPitch.reached = 0.0f;
	act->mTargetDistance                                              = NAN;
	StartScreenFade(&act->mBeam.mColor[0], 0, 0, 0, 0, 0xFF, 0x19, 0xFF, 0, 0.0f);
	StartScreenFade(&act->mBeam.mColor[1], 0, 0, 0, 0, 0xFF, 0x19, 0xFF, 0, 0.0f);
	act->mFade = 0.0f;
}

/**
 * @note Address: 0x800BFD38
 * @note Size: 0xE0
 */
void InitRadarAction(XActRadar* act, XObjChr* owner)
{
	act->Init(owner);
	ResetPlayerRadarAction(act);
}

// draws the popper's rings
/**
 * @note Address: 0x800BFE18
 * @note Size: 0x22C
 */
// NONMATCHING: regalloc
void DrawPopperShotRings(XActPopper* act)
{
	int i;
	XActPopperShot* shot;
	for (i = 0, shot = act->mShots.mShot; i < 10; i++, shot++) {
		for (int j = 0; j < 2; j++) {
			if (shot->mRing[j].mAmount > 0.0f) {
				f32 y;
				f32 x;
				ProjectWorldPositionToScreen(&gGame, shot->mRing[j].mPos, &x, &y, NULL, NULL);
				u32 color  = shot->mRing[j].mColor;
				u32 edge   = color & 0xFFFFFF00;
				f32 inner  = shot->mRing[j].mRadius * shot->mRing[j].mAmount;
				f32 outer  = shot->mRing[j].mRadius * (1.0f - shot->mRing[j].mAmount);
				u32 center = (color & 0xFFFFFF00) | (u8)((color & 0xFF) * shot->mRing[j].mAmount);
				f32 glow;
				SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
				glow = 2.0f * inner;
				DrawGsCircle(&gGame.mGs, 16, (u32)(255.0f * shot->mRing[j].mAmount) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, glow, glow, 0.0f,
				             0.0f, 0.0f);
				glow = 1.5f * inner;
				DrawGsCircle(&gGame.mGs, 16, (u32)(128.0f * shot->mRing[j].mAmount) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, glow, glow, 0.0f,
				             0.0f, 0.0f);
				SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
				DrawGsCircle(&gGame.mGs, 16, edge, center, x, y, 0.0f, outer, outer, 0.0f, 0.0f, 0.0f);
			}
		}
	}
}

/**
 * @note Address: 0x800C0044
 * @note Size: 0x4C
 */
void DrawPopperShotParticles(XActPopper* act)
{
	int i;
	XActPopperShot* shot;
	for (i = 0, shot = act->mShots.mShot; i < 10; i++, shot++) {
		DrawParticlePool(&shot->mDust.mPtcl);
	}
}

void SetPlayerPopperMountHidden(XObj*, BOOL);
void ShowIdlePlayerPopper(XObjPopper*);
void PlayPopperLowerAnimation(XObjPopper*);
BOOL IsPopperFireAnimationSelected(XObjPopper*);
void PlayPopperFireAnimation(XObjPopper*);
void PlayPopperRaiseAnimation(XObjPopper*);
void PlayPopperStowAnimation(XObjPopper*);
void ShowAndDeployPopper(XObjPopper*);
static inline void ClearPopperCharge(XActPopperCharge* charge)
{
	charge->mState = 0;
	charge->mTimer = 0.0f;
	KillGen(charge->mGen);
	KillGen(charge->mChargeGen);
	StopGameSfx(&gGame.mSound, 0x186);
	StopGameSfx(&gGame.mSound, 0x18B);
}
void UpdatePopperShotFlight(XActPopperShot*, u32);
CVec AddScalarToVectorComponents(const CVec&, f32);
void StartPopperShot(XActPopperShot*, const Vec&, s32, XObj*, f32, f32, f32, f32, f32, f32);

static inline int ActivePopperShots(XActPopper* act)
{
	int count = 0;
	for (int i = 0; i < 10; ++i)
		if (act->mShots.mShot[i].mActive)
			++count;
	return count;
}

static inline void StartAvailablePopperShot(XActPopper* act, const Vec& origin, s32 kind, XObj* target, f32 yaw, f32 pitch, f32 power,
                                            f32 speed, f32 range, f32 strength)
{
	for (int i = 0; i < 10; ++i)
		if (!act->mShots.mShot[i].mActive) {
			StartPopperShot(&act->mShots.mShot[i], origin, kind, target, yaw, pitch, power, speed, range, strength);
			break;
		}
}

static inline void StartPopperTone(int sound)
{
	if (!IsGameSoundPlaying(&gGame.mSound, sound))
		PlayGameSound(&gGame.mSound, sound, 0xff);
}

/**
 * @note Address: 0x800C0090
 * @note Size: 0x150C
 */
// NONMATCHING: the muzzle stands at both sites as retail's two copies of its data ask. left: the int to float slots of
// power and speed are swapped, and the target loop and the free-shot loop take their registers in another order
void UpdatePlayerPopperAction(XActPopper* act, s32 anim, HSD_AObj* aobj, u32 frames, f32, f32)
{
	BOOL rangeUpgrade, canCharge;
	if (FindOwnedInventoryItemIndex(act->mOwner->mParams.mInventory.mItems, 0x36) >= 0)
		rangeUpgrade = TRUE;
	else
		rangeUpgrade = FALSE;
	if (FindOwnedInventoryItemIndex(act->mOwner->mParams.mInventory.mItems, 0x35) >= 0)
		canCharge = TRUE;
	else
		canCharge = FALSE;
	// with the popper out (attachment 2), holding A charges a shot
	if (act->mOwner->GetAttachment() == 2) {
		CPad pad(0);
		if ((pad.mPad->mButton & 0x100) && canCharge && act->mOwner->mPopperObj.mAnimId == 4 && !gGame.mEvt.mData.mScript
		    && !(gGame.mStatus.mFlags & 0x400) && !act->mOwner->mLift.mState && !act->mOwner->mHang.mState && !act->mOwner->mJump.mState
		    && !act->mOwner->mLadder.mState && !act->mOwner->mRope.mState && !act->mOwner->mPush.mState && !act->mOwner->mTumble.mState) {
			if (!(gGame.mStatus.mFlags & 0x20)) {
				// through the eye camera the shot starts a little right of, below and ahead of the eye
				Vec pos;
				if (gGame.mStatus.mFlags & 0x100) {
					CCamera& camera = gGame.mMotionControlCamera.mCamera;
					Mtx rotY, rotX, rot;
					PSMTXRotRad(rotY, 'y', camera.mAngle.y);
					PSMTXRotRad(rotX, 'x', camera.mAngle.x);
					PSMTXConcat(rotY, rotX, rot);
					Vec offset = { 1.087597f, -1.056235f, 4.978912f };
					PSMTXMultVec(rot, &offset, &offset);
					pos = camera.mEye + offset;
				} else {
					pos = CJObj_GetJointWorldPositionByPath(&act->mOwner->mPopperObj, 4, (s32[4]) { 0, 1, 0, 0 });
				}
				act->mCharge.mState = 1;
				act->mCharge.mPos   = pos;
				if (!act->mCharge.mGen)
					act->mCharge.mGen = gGame.mParticle.Create(0x10051, 0, act->mCharge.mJoint, 1);
				StartPopperTone(0x186);
				if (act->mCharge.mTimer >= .5f)
					act->mState = 4;
			} else {
				act->mState = 0;
				ClearPopperCharge(&act->mCharge);
			}
		} else {
			// letting go of A: a full charge fires, a partial one is dropped
			if (act->mState == 4) {
				if (act->mCharge.mTimer >= 1)
					act->mState = 6;
				else
					act->mState = 0;
			}
			ClearPopperCharge(&act->mCharge);
		}
	}
	act->mShotStrength = 0;
	// 1 and 2 take the popper out and raise it, 3 puts it back in the ready pose, 4 charges (above).
	// 5 to 7 fire (6 a full charge), 8 waits for the popper's fire anim, 9 and 10 lower it and put it away
	switch (act->mState) {
	case 1:
		if (anim != 0x81f) {
			act->mOwner->SetAnim(0x81f, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			ShowAndDeployPopper(&act->mOwner->mPopperObj);
			break;
		}
		if (!(aobj->flags & 0x44000000))
			break;
		SetPlayerPopperMountHidden(act->mOwner, TRUE);
		act->mState = 2;
		// fall through
	case 2:
		if (anim != 0x821) {
			act->mOwner->SetAnim(0x821, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			PlayPopperRaiseAnimation(&act->mOwner->mPopperObj);
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(0x822, FALSE, NULL, NULL, NULL, 0, 1, 1, 10);
			ShowIdlePlayerPopper(&act->mOwner->mPopperObj);
			act->mState = 0;
		}
		break;
	case 3:
		if (anim != 0x822) {
			act->mOwner->SetAnim(0x822, FALSE, NULL, NULL, NULL, 0, 1, 1, 10);
			ShowIdlePlayerPopper(&act->mOwner->mPopperObj);
		}
		act->mState = 0;
		break;
	case 5: {
		s32 limit = GetPopperParameter(3);
		if (ActivePopperShots(act) < limit) {
			act->mOwner->mPopperObj.SetAnim(4, TRUE, NULL, NULL, NULL, 0, 1, 1, 10);
			act->mOwner->mPopperObj.UpdateAnim(0);
			act->mState        = 7;
			act->mShotStrength = (s32)GetPlayerActionParameter(30) / 100.f;
		}
	}
	// fall through
	case 6:
	case 7:
		if (anim != 0x823 || act->mShotStrength != 0) {
			if (act->mState != 6)
				act->mState = 8;
			s32 limit = GetPopperParameter(3);
			if (ActivePopperShots(act) < limit) {
				PlayPopperFireAnimation(&act->mOwner->mPopperObj);
				if (act->mShotStrength == 0) {
					act->mShotStrength
					    = act->mState == 6 ? (s32)GetPlayerActionParameter(31) / 100.f : (s32)GetPlayerActionParameter(30) / 100.f;
				}
				CVec origin;
				if (gGame.mStatus.mFlags & 0x100) {
					CCamera& camera = gGame.mMotionControlCamera.mCamera;
					Mtx rotY, rotX, rot;
					PSMTXRotRad(rotY, 'y', camera.mAngle.y);
					PSMTXRotRad(rotX, 'x', camera.mAngle.x);
					PSMTXConcat(rotY, rotX, rot);
					Vec offset = { 1.087597f, -1.056235f, 4.978912f };
					PSMTXMultVec(rot, &offset, &offset);
					origin = camera.mEye + offset;
				} else {
					origin = CJObj_GetJointWorldPositionByPath(&act->mOwner->mPopperObj, 4, (s32[4]) { 0, 1, 0, 0 });
				}
				f32 speed, range, yaw, pitch;
				if (gGame.mStatus.mFlags & 0x100) {
					yaw   = gGame.mMotionControlCamera.mCamera.mAngle.y;
					pitch = gGame.mMotionControlCamera.mCamera.mAngle.x;
				} else {
					yaw   = act->mOwner->mMoveRotate.y;
					pitch = 0;
				}
				f32 power = act->mState == 6 ? 3 : 1;
				// in tenths: popper parameter 2 is the speed, 0 the range and 1 the range with the upgrade
				speed = (s32)GetPopperParameter(2) / 10.f;
				range = (s32)GetPopperParameter(rangeUpgrade) / 10.f;

				XObj* target = NULL;
				for (int i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
					XObj* obj = gGame.GetObject(i);
					u64 flags = obj->mStatus.mFlags;
					if (!(flags & 1) || !(flags & 0x10000) || !(flags & 0x100) || (flags & 0x40000) || (flags & 0x100000))
						continue;
					Vec aim   = AddScalarToVectorComponents(obj->mMoveTranslate, obj->GetCollisionHeight() / 2.f);
					f32 delta = WrapAngle((f32)atan2(aim.x - origin.x, aim.z - origin.z) - yaw);
					if (delta > PI)
						delta = TWO_PI - delta;
					if (delta > DEG2RAD * 60)
						continue;
					delta = WrapAngle(TargetPitch(origin, aim) - pitch);
					if (delta > PI)
						delta = TWO_PI - delta;
					if (delta > DEG2RAD * 30)
						continue;
					f32 distance = PSVECDistance(&origin, &obj->mMoveTranslate);
					if (target == NULL) {
						if (distance < range)
							target = obj;
					} else if (distance < PSVECDistance(&origin, &target->mMoveTranslate))
						target = obj;
				}
				XObjChr* p = act->mOwner;
				if (p->mBatteryDrains) {
					Params& params = p->mParams;
					f32 battery    = params.mBattery - act->mShotStrength;
					if (battery < 0)
						battery = 0;
					params.SetBattery(battery);
				}
				s32 kind = GetAttachmentCountById(&act->mOwner->mAttachments, 2);
				if (target)
					StartAvailablePopperShot(act, origin, kind, target, yaw, pitch, power, speed, range, act->mShotStrength);
				else
					StartAvailablePopperShot(act, origin, kind, NULL, yaw, pitch, power, speed, range, act->mShotStrength);
			}
			ClearPopperCharge(&act->mCharge);
			act->mState = 8;
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(0x822, FALSE, NULL, NULL, NULL, 0, 1, 1, 10);
			ShowIdlePlayerPopper(&act->mOwner->mPopperObj);
			act->mState = 0;
		}
		break;
	case 8:
		if (!IsPopperFireAnimationSelected(&act->mOwner->mPopperObj)) {
			ShowIdlePlayerPopper(&act->mOwner->mPopperObj);
			act->mState = 0;
		}
		break;
	case 9:
		if (anim != 0x824) {
			act->mOwner->SetAnim(0x824, FALSE, NULL, NULL, NULL, 0, 1, 1, 10);
			PlayPopperLowerAnimation(&act->mOwner->mPopperObj);
			break;
		}
		if (!(aobj->flags & 0x44000000))
			break;
		SetPlayerPopperMountHidden(act->mOwner, FALSE);
		act->mState = 10;
		// fall through
	case 10:
		if (anim != 0x820) {
			act->mOwner->SetAnim(0x820, FALSE, NULL, NULL, NULL, 0, 1, 1, 10);
			PlayPopperStowAnimation(&act->mOwner->mPopperObj);
			SetPlayerPopperMountHidden(act->mOwner, FALSE);
		} else if (aobj->flags & 0x44000000) {
			XObjChr* p = act->mOwner;
			p->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			p->mIdleLoops = 3;
			act->mState   = 0;
		}
		break;
	}
	int i                = 0;
	XActPopperShot* shot = act->mShots.mShot;
	for (; i < 10; i++, shot++)
		UpdatePopperShotFlight(shot, frames);
	if (act->mCharge.mState) {
		if (act->mCharge.mTimer < 1 && (act->mCharge.mTimer += (1.f / FRAME_RATE) * frames) > 1)
			act->mCharge.mTimer = 1;
		if (act->mCharge.mGen || act->mCharge.mChargeGen)
			HSD_JObjSetTranslate(act->mCharge.mJoint, &act->mCharge.mPos);
		if (act->mCharge.mTimer >= 1) {
			if (!act->mCharge.mChargeGen)
				act->mCharge.mChargeGen = gGame.mParticle.Create(0x1005c, 0, act->mCharge.mJoint, 1);
			StartPopperTone(0x18b);
		}
	} else
		act->mCharge.mTimer = 0;
}

/**
 * @note Address: 0x800C159C
 * @note Size: 0x74
 */
CVec AddScalarToVectorComponents(const CVec& v, f32 d)
{
	CVec t(v);
	t.x += d;
	t.y += d;
	t.z += d;
	return t;
}

/**
 * @note Address: 0x800C1610
 * @note Size: 0x64
 */
void ApplyPopperMovementWeight(XActPopper* act, const CPadOne* pad, f32* weight)
{
	s32 state = act->mState;
	if (state == 4) {
		*weight = 1.0f;
	} else if (state == 6) {
		*weight = 1.0f;
	} else if (act->mState == 7 || act->mState == 8 || act->mState == 5) {
		*weight = 1.0f;
	} else if (state != 0) {
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x800C1674
 * @note Size: 0x148
 */
// NONMATCHING: regalloc
void InitializePlayerPopperAction(XActPopper* act, XObjChr* owner)
{
	act->Init(owner);
	XActPopperShot* shot;
	int i;
	for (i = 0, shot = act->mShots.mShot; i < 10; shot++, i++) {
		ResetShot(shot);
		InitParticlePool(&shot->mDust.mPtcl, shot->mDust.mPtcls, sizeof(CPtclDust2), 40);
		shot->mRing[0].mAmount = 0.0f;
		shot->mRing[1].mAmount = 0.0f;
		shot->mGen             = NULL;
		shot->mChargeGen       = NULL;
		if (shot->mJoint == NULL) {
			shot->mJoint = HSD_JObjAlloc();
		}
	}
	act->mCharge.mState     = 0;
	act->mCharge.mChargeGen = NULL;
	act->mCharge.mGen       = NULL;
	if (act->mCharge.mJoint == NULL) {
		act->mCharge.mJoint = HSD_JObjAlloc();
	}
}

/**
 * @note Address: 0x800C17BC
 * @note Size: 0xB0
 */
void ResetPopperCharge(XActPopperCharge* charge)
{
	ClearPopperCharge(charge);
}

static inline void SteerPopperAngle(f32& angle, f32 target, u32 frames)
{
	f32 delta    = WrapAngle(target - angle);
	f32 distance = delta > PI ? TWO_PI - delta : delta;
	f32 factor   = distance / PI;
	if (delta > PI)
		angle -= DEG2RAD * (14.0f * factor) * frames;
	else
		angle += DEG2RAD * (14.0f * factor) * frames;
}

static inline void FadePopperRing(XActPopperRing& ring, u32 frames)
{
	if (ring.mAmount > 0.0f && (ring.mAmount -= 0.05f * frames) < 0.0f)
		ring.mAmount = 0.0f;
}

/**
 * @note Address: 0x800C186C
 * @note Size: 0x934
 */
// NONMATCHING: retail schedules aim.z - shot->mPos.z before aim.x - shot->mPos.x for the yaw atan2 (see
// DrawRadarTargetBeam: the dol does this at four of 129 such calls). A yaw local, (f64) casts, a Vec& for
// mPos, a CVec or const CVec& aim, dz declared before or after the call and 23000 permuter iterations all
// leave x first; -(mPos.z - aim.z) is the only form that flips it (99.18 with its fneg)
void UpdatePopperShotFlight(XActPopperShot* shot, u32 frames)
{
	if (shot->mActive) {
		XObj* target = shot->mHomingTarget;
		if (target != NULL) {
			Vec aim = AddScalarToVectorComponents(target->mMoveTranslate, target->GetCollisionHeight() / 2.0f);
			SteerPopperAngle(shot->mYaw, (f32)atan2(aim.x - shot->mPos.x, aim.z - shot->mPos.z), frames);
			f32 dz    = aim.z - shot->mPos.z;
			f32 dx    = aim.x - shot->mPos.x;
			f32 dy    = aim.y - shot->mPos.y;
			f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
			SteerPopperAngle(shot->mPitch, dy < 0.0f ? HALF_PI - pitch : pitch - HALF_PI, frames);
		}
		shot->mDistance += shot->mSpeed * frames;
		Mtx translation, yaw, pitch, transform;
		PSMTXTrans(translation, shot->mPos.x, shot->mPos.y, shot->mPos.z);
		PSMTXRotRad(yaw, 'y', shot->mYaw);
		PSMTXRotRad(pitch, 'x', shot->mPitch);
		PSMTXConcat(translation, yaw, transform);
		PSMTXConcat(transform, pitch, transform);
		Vec forward = (Vec) { 0.0f, 0.0f, shot->mSpeed * frames };
		PSMTXMultVec(transform, &forward, &shot->mPos);
		if (shot->mGen || shot->mChargeGen) {
			HSD_JObjSetTranslate(shot->mJoint, &shot->mPos);
			HSD_JObjSetRotationY(shot->mJoint, shot->mYaw);
			HSD_JObjSetRotationX(shot->mJoint, shot->mPitch);
		}
		if (shot->mDistance >= shot->mRange) {
			shot->mActive          = FALSE;
			shot->mRing[1].mPos    = shot->mPos;
			shot->mRing[1].mAmount = 0.9f;
			shot->mRing[1].mRadius = 40.0f;
			shot->mRing[1].mColor  = 0x64FF6496;
			KillGen(shot->mGen);
			KillGen(shot->mChargeGen);
		} else {
			CheckPopperShotImpact(shot);
		}
		for (s32 i = 9; i > 0; --i)
			shot->mHistory[i] = shot->mHistory[i - 1];
		shot->mHistory[0] = shot->mPos;
	} else if (shot->mHitTarget) {
		shot->mHitTarget->OnHitFromPosition(shot->mPos, shot->mStrength * shot->mKind);
		shot->mHitTarget = NULL;
	}
	UpdateParticlePool(&shot->mDust.mPtcl, frames);
	FadePopperRing(shot->mRing[0], frames);
	FadePopperRing(shot->mRing[1], frames);
}

// NONMATCHING: registers, and retail has the spark's velocity above its size on the stack (both below the
// CVec temporaries, so neither is a named local of this function)
void CheckPopperShotImpact(XActPopperShot* shot)
{
	Vec pos  = shot->mPos;
	BOOL hit = FALSE, ended = FALSE;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.GetObject(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000) || (object->mStatus.mFlags & 0x40000)
		    || (object->mStatus.mFlags & 0x0002000000000000ULL) || (object->mStatus.mFlags & 0x100000))
			continue;
		f32 dx = object->mMoveTranslate.x - pos.x, dz = object->mMoveTranslate.z - pos.z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (0.8f + object->mFloorXZExtent))
			continue;
		CVec push, contact;
		u32 touch = ProbeObjectFloor(object, pos, pos, 0.8f, 0.8f, &push, &contact, NULL) & 3;
		if (touch == 3) {
			hit          = TRUE;
			ended        = TRUE;
			shot->mPos.x = contact.x;
			shot->mPos.y = contact.y;
			shot->mPos.z = contact.z;
			if (gGame.mEvt.mData.mScript != NULL)
				shot->mHitTarget = object;
			else
				object->OnHitFromPosition(shot->mPos, shot->mStrength * shot->mKind);
		}
	}
	if (!hit) {
		XObjMapHit ground;
		XObjMap* map = (XObjMap*)gGame.mXObjs.mPools[3].First();
		if (FindMapFloor(map, &pos, &ground, NULL) && pos.y <= ground.mHeight) {
			hit   = TRUE;
			ended = TRUE;
			shot->mPos.Set(CVec(pos));
			shot->mPos.y = ground.mHeight;
		}
	}
	if (!hit)
		return;
	for (s32 i = 0; i < 20; ++i) {
		f32 angle    = DEG2RAD * (rand() % 360);
		f32 sinAngle = std::sinf(angle);
		f32 vx       = (sinAngle * (rand() % 100)) / 200.0f;
		f32 cosAngle = std::cosf(angle);
		f32 vz       = (cosAngle * (rand() % 100)) / 200.0f;
		f32 vy       = (50 + rand() % 80) / 100.0f;
		f32 width    = (200 + rand() % 300) / 500.0f;
		f32 height   = (10 + rand() % 200) / 500.0f;
		CPtclOne* spark
		    = SpawnPoolParticle(&shot->mDust.mPtcl, &shot->mPos, &(Vec) { width, height, 1.0f }, &(Vec) { vx, vy, vz }, 0xC8C8FFFF);
		if (!spark)
			break;
		spark->mAlpha      = (192 + (rand() & 63)) / 255.0f;
		spark->mAlphaStep  = 0.03f;
		spark->mAngleSpeed = DEG2RAD * ((rand() & 63) - 32);
	}
	if (shot->mKind == 1)
		PlayGameSound(&gGame.mSound, 0x194, 255);
	else
		PlayGameSound(&gGame.mSound, 0x197, 255);
	if (ended) {
		shot->mActive          = FALSE;
		shot->mRing[1].mPos    = shot->mPos;
		shot->mRing[1].mAmount = 0.9f;
		shot->mRing[1].mRadius = 100.0f;
		shot->mRing[1].mColor  = 0x6464FF96;
		KillGen(shot->mGen);
		KillGen(shot->mChargeGen);
	}
}

// fires a shot, kind 1 is the small one
/**
 * @note Address: 0x800C2A14
 * @note Size: 0x304
 */
// NONMATCHING: the position is read once too few: retail reloads pos for mRing[0] after the history
// stores, this build reuses the three loads the mPos copy's CVec temporary made. With a const Vec& (or
// const Vec*) MWCC treats pos as invariant over the stores; a non-const Vec& reloads it as retail does but
// then also keeps the first loads below the mHitTarget store (90.4), and with mPos stored before mHitTarget
// as well 90.9. Struct copies for mPos, mHistory[i] and mRing[0].mPos and the mPos/mHitTarget statement order are flat
void StartPopperShot(XActPopperShot* shot, const Vec& pos, s32 kind, XObj* target, f32 yaw, f32 pitch, f32 power, f32 speed, f32 range,
                     f32 strength)
{
	shot->mActive    = TRUE;
	shot->mHitTarget = NULL;
	shot->mPos       = pos;
	shot->mYaw       = yaw;
	shot->mPitch     = pitch;
	shot->mSpeed     = speed;
	shot->mDistance  = 0.0f;
	shot->mRange     = range;
	shot->_28        = power;
	for (int i = 0; i < 10; i++) {
		shot->mHistory[i].x = shot->mPos.x;
		shot->mHistory[i].y = shot->mPos.y;
		shot->mHistory[i].z = shot->mPos.z;
	}
	shot->mRing[0].mPos.x  = pos.x;
	shot->mRing[0].mPos.y  = pos.y;
	shot->mRing[0].mPos.z  = pos.z;
	shot->mRing[0].mAmount = 0.9f;
	shot->mRing[0].mRadius = 100.0f;
	shot->mRing[0].mColor  = 0x00FF0096;
	shot->mStrength        = strength;
	shot->mKind            = kind;
	if (target != NULL) {
		shot->mHomingTarget = target;
	} else {
		shot->mHomingTarget = NULL;
	}
	shot->mGen = (&gGame.mParticle)->Create((kind == 1 ? 0x4E : 0x4F) | 0x10000, 0, shot->mJoint, TRUE);
	if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f) {
		shot->mChargeGen = (&gGame.mParticle)->Create(0x10050, 0, shot->mJoint, TRUE);
	}
	f32 full = (s32)GetPlayerActionParameter(31) / 100.0f;
	PlayGameSound(&gGame.mSound, strength >= full ? (kind == 1 ? 0x193 : 0x196) : (kind == 1 ? 0xB : 0x195), 0xFF);
	CheckPopperShotImpact(shot);
}

/**
 * @note Address: 0x800C2D18
 * @note Size: 0x98
 */
void ResetPopperShot(XActPopperShot* shot)
{
	ResetShot(shot);
}

// flies him while the copter is out
/**
 * @note Address: 0x800C2EB8
 * @note Size: 0x738
 */
void UpdatePlayerCopterFlight(XActCopter* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	// the flight time (copter parameter 0, in hundredths of a second) fills up while idle and runs down in flight
	if (act->mState == 0) {
		act->mTime = FRAME_RATE * ((s32)GetCopterParameter(0) / 100.0f);
	} else if (act->mState == 3) {
		if ((act->mTime -= frames) < 0.0f) {
			act->mTime  = 0.0f;
			act->mState = 4;
		}
	}
	// 1 takes the copter out, 2 puts it away, 3 flies. 4 and 5 come down and 6 lands, 7 (off a rope)
	// and 8 (off a ledge) come down and 9 lands
	switch (act->mState) {
	case 1:
		if (anim != 0x814) {
			act->mOwner->SetAnim(0x814, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			ShowAndDeployCopter(&act->mOwner->mCopterObj);
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(0x4A, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->mState = 0;
		}
		break;
	case 2:
		if (anim != 0x817) {
			act->mOwner->SetAnim(0x817, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			PlayCopterStowAnimation(&act->mOwner->mCopterObj);
		} else if (aobj->flags & 0x44000000) {
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
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
			act->mOwner->SetAnim(0x28, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			PlayCopterDescentAnimation(&act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x816, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			PlayCopterLandingAnimation(&act->mOwner->mCopterObj);
			act->mState = 6;
		}
		break;
	case 5:
		UpdateCopterBobbing(act, frames, -1.0f);
		if (anim != 0x28) {
			act->mOwner->SetAnim(0x28, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->mOwner->mCopterObj.SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
		if (act->mOwner->GetHeight() <= 0.0f || (act->mOwner->GetHeight() <= 1.0f && GetPlayerFloorSlope(act->mOwner).Length() > 0.5f)) {
			act->mOwner->SetAnim(0x816, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			PlayCopterLandingAnimation(&act->mOwner->mCopterObj);
			act->mState = 6;
		}
		break;
	case 6:
		UpdateCopterBobbing(act, frames, -1.0f);
		if (aobj->flags & 0x44000000) {
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	case 7:
		if (anim != 0x38) {
			act->mOwner->SetAnim(0x38, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			PlayCopterDescentAnimation(&act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x59, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			PlayCopterLandingAnimation(&act->mOwner->mCopterObj);
			act->mState = 9;
		}
		break;
	case 8:
		if (anim != 0x1A) {
			act->mOwner->SetAnim(0x1A, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			PlayCopterDescentAnimation(&act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x1B, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			PlayCopterLandingAnimation(&act->mOwner->mCopterObj);
			act->mState = 9;
		}
		break;
	case 9:
		if (aobj->flags & 0x44000000) {
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	}
}

/**
 * @note Address: 0x800C2DB0
 * @note Size: 0x108
 */
void UpdateCopterBobbing(XActCopter* act, u32 frames, f32 speed)
{
	if (speed > 0.0f) {
		act->mPhase = fmod(speed * frames + act->mPhase, TWO_PI);
		act->mBob   = 0.4f * (f32)sin(act->mPhase) + 0.4f;
		s32 path[]  = { 0 };
		SetObjectJointTranslationByPath(act->mOwner, 1, path, 0.0f, act->mBob, 0.0f);
	} else {
		if (act->mBob > 0.0f) {
			if ((act->mBob -= 0.2f * frames) <= 0.0f) {
				act->mBob = 0.0f;
			}
		}
		s32 path[] = { 0 };
		SetObjectJointTranslationByPath(act->mOwner, 1, path, 0.0f, act->mBob, 0.0f);
	}
}

/**
 * @note Address: 0x800C35F0
 * @note Size: 0x2C
 */
CVec GetPlayerFloorSlope(XObjChr* chr)
{
	return chr->mFloorSlope;
}

/**
 * @note Address: 0x800C361C
 * @note Size: 0xC8
 */
void HandleCopterActionInput(XActCopter* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 3) {
		// B or standing still ends the flight
		if ((pad->mTrigger & 0x200) || IsPlayerStopped(act->mOwner)) {
			act->mState = 4;
			PlayCopterDescentAnimation(&act->mOwner->mCopterObj);
		}
		*weight = 1.0f;
	} else if (act->mState == 4 || act->mState == 7 || act->mState == 8) {
		if (!(pad->mTrigger & 0x200)) {
			IsPlayerStopped(act->mOwner);
		}
		*weight = 1.0f;
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

// the ring on the floor while he tumbles
/**
 * @note Address: 0x800C36E4
 * @note Size: 0x2F0
 */
// NONMATCHING: regalloc and stack order
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
		f32 outer = size / 2.0f + 1.0f;
		for (int i = 0; i < 25; i++) {
			f32 ringAngle          = (TWO_PI / 24) * i;
			verts[i * 2].pos.x     = center.x - size * std::sinf(ringAngle) / 2.0f;
			verts[i * 2].pos.z     = center.z + size * std::cosf(ringAngle) / 2.0f;
			verts[i * 2].pos.y     = center.y;
			verts[i * 2].color     = color;
			verts[i * 2 + 1].pos.x = center.x - outer * std::sinf(ringAngle);
			verts[i * 2 + 1].pos.z = center.z + outer * std::cosf(ringAngle);
			verts[i * 2 + 1].pos.y = center.y;
			verts[i * 2 + 1].color = 0;
		}
		DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, verts, 50);
	}
}

/**
 * @note Address: 0x800C39D4
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

static inline void PlayTumbleAnimation(XObjChr* owner, s32 anim, f32 blend)
{
	owner->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, blend, 10.0f);
}

static inline void FinishTumble(XActTumble* act, f32 blend)
{
	XObjChr* owner = act->mOwner;
	PlayTumbleAnimation(owner, 0x11, blend);
	owner->mIdleLoops = 3;
	act->mState       = 0;
}

static inline void StartTumble(XActTumble* act, s32 anim, HSD_AObj* aobj, s32 next)
{
	if (anim != 0x35 && anim != 0x10D) {
		PlayTumbleAnimation(act->mOwner, 0x35, 0.0f);
		gGame.mParticle.Create(0x10006, 0, act->mOwner->mJObj, FALSE);
	} else if (aobj->flags & 0x44000000) {
		PlayTumbleAnimation(act->mOwner, 0x36, 1.0f);
		act->mState = next;
	}
}

static inline void RecoverTumble(XActTumble* act, s32 anim, HSD_AObj* aobj, s32 recovery, f32 blend)
{
	if (anim != recovery)
		PlayTumbleAnimation(act->mOwner, recovery, blend);
	else if (aobj->flags & 0x44000000)
		FinishTumble(act, 1.0f);
}

static inline bool TumbleStationary(const CVec& delta)
{
	return PSVECMag((Vec*)&delta) == 0.0f;
}

/**
 * @note Address: 0x800C3A80
 * @note Size: 0x9E8
 */
// NONMATCHING: retail copies both particle rotations from one zero Vec of its .rodata (away from this function's
// other constants) into the lowest stack slots, and builds the event script call's arguments in other registers
void UpdatePlayerTumbleAnimation(XActTumble* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	// 1 and 15 knock him over; 2 and 16 keep him down until a button or the idle timer, then 3 and
	// 17 get him up. 4 is the empty battery, 10 comes after five turns on the spot, 12 turns him to
	// face the camera and 13 back, 14 holds the pose while the idle timer runs
	switch (act->mState) {
	case 1:
		StartTumble(act, anim, aobj, 2);
		break;
	case 3:
		RecoverTumble(act, anim, aobj, 0x37, 1.0f);
		break;
	case 4:
		if (anim != 0x43)
			PlayTumbleAnimation(act->mOwner, 0x43, 0.0f);
		else if (aobj->flags & 0x44000000) {
			// battery empty: the stage's script takes over, otherwise he lies there until a button (5)
			if ((gGame.mStatus.mFlags & 0x40000000) && GetStageBatteryDepletedScript() && gGame.GetPlayer()->mParams.mBattery <= 0.0f) {
				if (gGame.mEvt.mData.mScript == NULL) {
					u8* script = GetStageBatteryDepletedScript();
					StartOrQueueEventScript(&gGame.mEvt, script, 0, -1, -1);
				}
			} else {
				act->mState = 5;
			}
		}
		break;
	case 6:
		RecoverTumble(act, anim, aobj, 0x1E, 1.0f);
		break;
	case 7:
		RecoverTumble(act, anim, aobj, 0xB3, 0.0f);
		break;
	case 8:
		PlayTumbleAnimation(act->mOwner, 0xD4, 0.0f);
		act->mState = 9;
		break;
	case 9:
		if (aobj->flags & 0x44000000)
			FinishTumble(act, 1.0f);
		break;
	case 10: {
		PlayTumbleAnimation(act->mOwner, 0xE6, 0.0f);
		Vec offset = { 0.0f, 12.0f, 0.0f };
		CreateParticle(&gGame, 1, 5, 1, act->mOwner->mMoveTranslate + offset, act->mOwner->mMoveScale, FALSE);
		act->mState = 11;
		break;
	}
	case 11:
		if (aobj->flags & 0x44000000)
			FinishTumble(act, 0.0f);
		break;
	case 12:
		act->mSavedYaw = act->mOwner->mMoveRotate.y;
		act->mOwner->SetRotateY(PI + gGame.mMotionControlCamera.mCamera.mAngle.y, 12.0f * DEG2RAD);
		PlayTumbleAnimation(act->mOwner, 0x5D, 0.0f);
		act->mState = 14;
		break;
	case 13:
		act->mOwner->SetRotateY(act->mSavedYaw, 12.0f * DEG2RAD);
		PlayTumbleAnimation(act->mOwner, 0x5F, 0.0f);
		act->mState = 14;
		break;
	case 14:
		if (act->mOwner->mIdleModeTimer > 0) {
			if (!act->mIdleParticle && TumbleStationary(act->mOwner->mMoveTranslate - (Vec)act->mOwner->mLastPos)) {
				s32 effect = (rand() & 1) ? 0x26 : 0x28;
				s32 height = act->mOwner->mLift.mState == 0 ? 2 : 4;
				s32 path[] = { 0, 1, 0, 1, 2, 0, 1 };
				act->mIdleParticle
				    = CreateParticle(&gGame, 1, effect, 1, GetEventJointWorldPosition(act->mOwner, 7, path) + (Vec) { 0.0f, height, 0.0f },
				                     act->mOwner->mMoveScale, FALSE);
			}
		} else {
			act->mIdleParticle = NULL;
			act->mState        = 0;
		}
		break;
	case 15:
		StartTumble(act, anim, aobj, 16);
		break;
	case 16:
		if (act->mOwner->mIdleModeTimer <= 0)
			act->mState = 17;
		break;
	case 17:
		RecoverTumble(act, anim, aobj, 0x37, 1.0f);
		break;
	}
}

/**
 * @note Address: 0x800C4468
 * @note Size: 0x4A4
 */
void HandleTumbleActionInput(XActTumble* act, const CPadOne* pad, f32* weight)
{
	// lying down (2, 16) A, B or the stick gets him up; after the battery ran out (5) only A or B
	if (act->mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if ((pad->mTrigger & 0x100) || (pad->mTrigger & 0x200) || StickLength(pad) > 0.0f) {
				act->mState = 3;
			}
		}
		*weight = 0.0f;
	} else if (act->mState == 16) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if ((pad->mTrigger & 0x100) || (pad->mTrigger & 0x200) || StickLength(pad) > 0.0f) {
				act->mState = 17;
			}
		}
		*weight = 0.0f;
	} else if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if ((pad->mTrigger & 0x100) || (pad->mTrigger & 0x200)) {
				act->mState = 6;
			}
		}
		*weight = 0.0f;
	} else if (act->mState == 7) {
		*weight = 0.8f;
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

} // extern "C"

/**
 * @note Address: 0x800C490C
 * @note Size: 0x40
 */
extern "C" void ResetTumbleAction(XActTumble* act)
{
	act->Reset();
	act->mIdleParticle = NULL;
}

extern "C" {
f32 GetJointAnimationEndFrame(const CJObj* obj); // the end frame of the anim
CVec GetPlayerJointLocalTranslationByPath(CJObj* obj, s32 depth, s32* path);
inline void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos);
void SetJointYawByPath(CXObj* obj, s32 depth, s32* path, f32 angle);
void HSD_JObjSetRotationY__FP9_HSD_JObjf(HSD_JObj* jobj, f32 angle);
void RecordObjectPositionAndHeightChange(XObj* obj);
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
void TryLeaveHang(XActHang* act);
Vec GetLiftTargetContactOffset(const XActLift* act, const XObj* target);
Vec GetTargetFloorPosition(XAction* act); // where the held floor of the target is
u32 CheckStageFlag(s32 stage, u32 mask);
s32 GetObjectPoolId(const XObj*);
s16 GetObjectItemId(XObj*);
s16 GetItemPickupHoldTime(s32);
f32 GetHeadingBetweenPositions(const Vec*, const Vec*);
void SetPlugCordSpeed(XObjPlug*, f32);
void SetLiftActionState(XActLift*, s32);
void SetLiftTargetState(XActLift*, s32);
XObj* GetGameObjectByIndex(const CGame*, s32);
void TryStartObjectEvent(CGame*, const void*, XObj*, s16, s16);
BOOL IsEventScriptActive(const CGame*);
Params* GetPlayerParamsByIndex(CGame*, s32);
s16 GetInventoryCountByIndex(Params*, u32);
void SetPlayerStandAnim(XObjChr*, s32, f32);
void SetClampedPlayerBattery(Params*, f32);
HSD_Generator* CreateParticleByBankAndId(CGame*, s32, s32, u8, HSD_JObj*, BOOL);
XObj* SpawnObjectFromPlacementIndex(CGame*, s16);
BOOL IsObjectLoaded(XObj*);
void ClearObjectStatusFlags(XObj*, u64);
void SetPlayerActionTarget(XAction*, XObj*);
s32 RemovePlayerInventoryItemCount(Params*, s16, s32);
void PutAwayLiftedObject(XActLift*);
Vec GetJointMatrixTranslation(void*, HSD_JObj*);
Vec GetJointMatrixScale(void*, HSD_JObj*);
HSD_Generator* CreateParticleAtPosition(CGame*, u32, u32, s32, const Vec&, BOOL);
int ApplyPlayerItemParticleColor(HSD_Particle* particle);
static HSD_PSUserFunc sPlayerItemParticleUserFunc = { ApplyPlayerItemParticleColor, NULL, NULL };
void SetSubwindowRect(CGame*, f32, f32, f32, f32);
void SetSubwindowCamera(CGame*, const Vec&, f32, f32, f32, f32);
void EnableSubwindowWithFlags(CGame*, u32);
Vec GetObjectJointWorldPosition(const CJObj*);
BOOL IsLiftActionState(const XActLift*, s32);
f32 GetObjectJointRotationY(CJObj*);
void ClearObjectAnimationQueue(XObj*);
Vec GetObjectJointMatrixScale(const CJObj*);
HSD_Generator* CreateScaledParticleAtPosition(CGame*, u32, u32, s32, const Vec&, const Vec&, BOOL);

static const s32 sSquirterChargeLevels[11] = { 0, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3 };
f32 GetJointAnimationCurrentFrame(CJObj*);
BOOL CheckCarriedObjectContacts(XActLift*, const Vec*, f32, f32, BOOL, BOOL, BOOL, Vec*);
// NONMATCHING: the bl lists differ in four inlines only: retail calls GetObjectJointByPath (5), ~CJObj, CVec(const Vec&)
// and ~CVec (10) that this one expands. Retail still expands CVec::Length (bl PSVECMag at US 0x800D3874, no bl Length)
// and GetStagePlacementIndex (bl GetStagePlacementByIndex at 0x800D0F9C), so its inline limit was 16: our Length leaves
// at the 16 -> 8 drop and GetStagePlacementIndex together with ~CVec at 32 -> 16, so retail's placement index weighed
// 16 units or less (a pointer difference) and the lift state setters it calls 17 or more. Measured with 3-unit fillers,
// this function is 1035 units under the 128 -> 64 drop, 1758 under 32 -> 16 and 2010 under 16 -> 8. The missing weight
// is not in this file: the helpers retail calls here are declared below or in obja_chr.cpp and weigh a call each.
// GetItemSoundIndex: retail extends the release sound's index for its test and again for the call (0x800D249C,
// 0x800D24A4), which only an s8 prototype gives (a u8 local with two (s8) casts is extended once); that prototype
// needs the definitions in stage_data.cpp/globalresources.cpp and the s8 local in obja_chr's ProcessPlayerFrameEvents
// (one extension, 0x800A80A4) to follow.
void UpdateLiftActionAnimation(XActLift* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32)
{
	s32 placement     = act->mTarget ? GetStagePlacementIndex(act->mTarget->mCoord.mPlacement) : -1;
	act->mBatteryCost = 0;
	switch (act->mState) {
	case 1:
		if (anim != 5) {
			act->mOwner->SetAnim(5, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			if (act->mTarget) {
				const CVec& point = GetFloorContactPosition(act->mTarget, act->mTargetFloor, NULL);
				act->mOwner->SetRotateY(GetHeadingBetweenPositions(&act->mOwner->mMoveTranslate, &point), 12 * DEG2RAD);
			}
			if (placement < 0 && PSVECDistance(&act->mOwner->mMoveTranslate, &act->mTarget->mMoveTranslate) >= 3.0f)
				SetPlugCordSpeed(&act->mOwner->mPlug, 0.2f);
			return;
		}
		if (!(aobj->flags & 0x44000000))
			return;
		act->mOwner->SetAnim(6, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		SetLiftActionState(act, 2);
		if (placement < 0) {
			SetPlugCordSpeed(&act->mOwner->mPlug, 0.5f);
			act->mTimer = 0;
			PlayGameSound(&gGame.mSound, 0x3a, 255);
		} else if ((s16)GetObjectPoolId(act->mTarget) == 8) {
			act->mTimer = FRAME_RATE * ((f32)GetItemPickupHoldTime(GetObjectItemId(act->mTarget)) / 100.0f);
		} else {
			act->mTimer = 0.5f * FRAME_RATE;
		}
		// fall through
	case 2:
		if (!((act->mTimer -= frames) <= 0))
			return;
		SetLiftActionState(act, 3);
		// fall through
	case 3:
		if (anim != 7) {
			// anything but an item he already has ten of
			BOOL room = TRUE;
			if (act->mTarget && (s16)GetObjectPoolId(act->mTarget) == 8) {
				s16 id   = GetObjectItemId(act->mTarget);
				s32 slot = FindOwnedInventoryItemIndex(GetPlayerParamsByIndex(&gGame, 0)->mInventory.mItems, id);
				if (slot >= 0 && GetInventoryCountByIndex(GetPlayerParamsByIndex(&gGame, 0), slot) >= 10)
					room = FALSE;
			}
			if (room) {
				SetLiftTargetState(act, 1);
				act->mOwner->SetAnim(7, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			} else {
				SetLiftActionState(act, 5);
			}
			if (act->mTarget && act->mTarget->GetEventScript() && (act->mTarget->mStatus.mFlags & 0x10000000000ULL)) {
				XObj* target = act->mTarget;
				TryStartObjectEvent(&gGame, GetGameObjectByIndex(&gGame, placement)->GetEventScript(), target, -1, -1);
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			if (act->mTarget) {
				if (act->mTarget->GetEventScript())
					SetLiftActionState(act, 5);
				else if ((s16)GetObjectPoolId(act->mTarget) == 8 && !(act->mTarget->mStatus.mFlags & 0x400000000000ULL))
					SetLiftActionState(act, 19);
				else
					SetLiftActionState(act, 4);
				SetLiftTargetState(act, 4);
			}
		}
		break;
	case 4:
		if (anim == 0x895) {
			if (aobj->flags & 0x44000000) {
				act->mOwner->SetAnim(0x894, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				if (act->mContactEffect) {
					gGame.mParticle.Kill(act->mContactEffectId);
					act->mContactEffect = NULL;
				}
			} else {
				if (act->mCheckContact && frame > 6) {
					s32 path[5]  = { 0, 0, 0, 0, 0 };
					Vec position = CJObj_GetJointWorldPositionByPath(act->mTarget, 5, path);
					if (CheckCarriedObjectContacts(act, &position, 3, 1, TRUE, FALSE, FALSE, NULL))
						act->mCheckContact = FALSE;
				}
				if (!act->mContactEffect) {
					act->mContactEffectId = 29;
					s32 path[5]           = { 0, 0, 0, 0, 0 };
					act->mContactEffect
					    = CreateParticleByBankAndId(&gGame, 1, act->mContactEffectId, 0, GetObjectJointByPath(act->mTarget, 5, path), TRUE);
				}
			}
		} else if (anim == 0x8ae || anim == 0x8af) {
			if (aobj->flags & 0x44000000)
				act->mOwner->SetAnim(0x894, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
		// the brush's effect lasts while it scrubs (0x895), the squirter's while it squirts (0x8c2 to 0x8c4)
		if (IsCarryingToothbrush(act)) {
			if (anim != 0x895 && act->mContactEffect) {
				gGame.mParticle.Kill(act->mContactEffectId);
				act->mContactEffect = NULL;
			}
		} else if (IsCarryingSquirter(act) && !(anim == 0x8c2 || anim == 0x8c3 || anim == 0x8c4) && act->mToolEffect) {
			gGame.mParticle.Kill(act->mToolEffectId);
			act->mToolEffect = NULL;
		}
		break;
	case 5:
		if (!IsEventScriptActive(&gGame)) {
			if ((s16)GetObjectPoolId(act->mTarget) == 8 && !(act->mTarget->mStatus.mFlags & 0x400000000000ULL)) {
				// anything but an item he already has ten of
				BOOL room = TRUE;
				s16 id    = GetObjectItemId(act->mTarget);
				s32 slot  = FindOwnedInventoryItemIndex(GetPlayerParamsByIndex(&gGame, 0)->mInventory.mItems, id);
				if (slot >= 0 && GetInventoryCountByIndex(GetPlayerParamsByIndex(&gGame, 0), slot) >= 10)
					room = FALSE;
				if (room)
					SetLiftActionState(act, 19);
				else {
					SetLiftActionState(act, 0);
					SetPlayerStandAnim(act->mOwner, 17, 0);
					act->mTarget = NULL;
				}
			} else {
				SetLiftActionState(act, 4);
			}
		}
		break;
	case 6:
		if (anim != 9) {
			act->mOwner->SetAnim(9, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftTargetState(act, 5);
		} else if (aobj->flags & 0x44000000) {
			SetPlayerStandAnim(act->mOwner, 17, 0);
			SetLiftActionState(act, 0);
		}
		break;
	// event commands 102, 106 and 108 plug him into mPlugTarget: 7 only plugs in, 8 then gives it
	// power (12, 13), 9 then waits for the stick or b (15)
	case 7:
	case 8:
	case 9:
		if (!(anim == 0x9f || anim == 0xa0 || anim == 0xbc)) {
			SetPlayerActionTarget(act, &act->mOwner->mPlug);
			XObjPlug* plug = (XObjPlug*)act->mTarget;
			plug->mStatus.mFlags |= 0x0400000000000000ULL;
			ResetPlayerPlugState(plug, TRUE);
			act->mYawOffset = 0;
			plug->SetRotationY(GetObjectJointRotationY(act->mOwner), 0);
			plug->SetRotationX(0, 0);
			s32 path[] = { 0 };
			CXObj joint(GetObjectJointByPath(plug, 1, path));
			joint.SetRotationX(0, 0);
			joint.SetRotationY(0, 0);
		}
		// fall through
	case 10:
		if (!(anim == 0x9f || anim == 0xa0 || anim == 0xbc)) {
			ClearObjectAnimationQueue(act->mOwner);
			if (IsLiftActionState(act, 7)) {
				act->mOwner->SetAnim(0x9f, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				SetLiftTargetState(act, 6);
			} else if (IsLiftActionState(act, 8)) {
				act->mOwner->SetAnim(0xa0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				SetLiftTargetState(act, 8);
			} else {
				act->mOwner->SetAnim(0xbc, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				SetLiftTargetState(act, 11);
			}
		} else if (aobj->flags & 0x44000000) {
			if (IsLiftActionState(act, 7)) {
				SetPlayerStandAnim(act->mOwner, 17, 0);
				SetLiftActionState(act, 0);
				if (act->mPlugTarget)
					act->mPlugTarget->ReceivePlugPower(0);
			} else if (IsLiftActionState(act, 8)) {
				SetLiftActionState(act, 12);
				act->mTimer = 0.2f * FRAME_RATE;
			} else {
				act->mOwner->SetAnim(0xbe, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				SetLiftActionState(act, 15);
			}
		}
		break;
	case 11:
		if (anim != 0xad) {
			act->mOwner->SetAnim(0xad, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mOwner->UpdateAnim(0);
			SetPlayerActionTarget(act, &act->mOwner->mPlug);
			SetLiftTargetState(act, 7);
			act->mYawOffset = 0;
		} else if (aobj->flags & 0x44000000) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 root[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, root, 0, 0, 0);
			act->mOwner->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			SetLiftActionState(act, 4);
			act->mPlugTarget = NULL;
		}
		break;
	case 12:
		if ((act->mTimer -= frames) <= 0) {
			act->mTimer = 0;
			if (act->mPlugTarget && !act->mPlugTarget->ReceivePlugPower(0)) {
				SetLiftActionState(act, 14);
				break;
			}
			act->mOwner->SetAnim(0xab, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftActionState(act, 13);
			if (act->mPlugTarget && !GetObjectItemId(act->mPlugTarget)) {
				SetSubwindowRect(&gGame, NAN, NAN, NAN, NAN);
				s32 path[] = { 0 };
				CJObj joint(GetObjectJointByPath(&act->mOwner->mPlug, 1, path));
				Vec at = act->mOwner->mMoveTranslate + (act->mPlugTarget->mMoveTranslate - act->mOwner->mMoveTranslate) * 0.3f;
				f32 y  = act->mOwner->mMoveTranslate.y;
				at.y   = y + 0.8f * (GetObjectJointWorldPosition(&joint).y - y);
				SetSubwindowCamera(&gGame, at, act->mOwner->mMoveRotate.y - 45 * DEG2RAD, 24 * DEG2RAD, 100, 8);
				EnableSubwindowWithFlags(&gGame, 0);
			}
		}
		break;
	case 13: {
		Params& params = act->mOwner->mParams;
		if (params.mBattery > 0 && params.mBattery / params.mMaxBattery > 0.2f) {
			if (anim != 0xab)
				act->mOwner->SetAnim(0xab, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else if (anim != 0xae) {
			act->mOwner->SetAnim(0xae, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
		if (act->mPlugTarget) {
			act->mBatteryCost = (f32)frames * (((s32)GetPlayerActionParameter(9) / 100.0f) / FRAME_RATE);
			if (!act->mPlugTarget->ReceivePlugPower(act->mBatteryCost)) {
				act->mBatteryCost = 0;
				SetLiftActionState(act, 14);
			}
		}
		break;
	}
	case 14:
		if (anim != 0xac) {
			act->mOwner->SetAnim(0xac, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->mOwner->UpdateAnim(0);
			SetLiftTargetState(act, 10);
			gGame.mSubwin.mEnabled = FALSE;
			if (act->mPlugTarget && GetObjData(&act->mPlugTarget->mCoord)->mCategory == 9)
				((XObjLeticker*)act->mPlugTarget)->OnUnplugged();
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftActionState(act, 4);
			act->mPlugTarget = NULL;
		}
		break;
	case 16:
		if (anim != 0xbf) {
			act->mOwner->SetAnim(0xbf, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			if (act->mPlugTarget)
				act->mPlugTarget->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftActionState(act, 4);
			SetLiftTargetState(act, 4);
			act->mPlugTarget = NULL;
			ResetPlayerPlugState((XObjPlug*)act->mTarget, TRUE);
		} else if (frame > 142.0f) {
			((XObjPlug*)act->mTarget)->mConnection = 0;
		} else if (frame > 85.0f) {
			if (((XObjPlug*)act->mTarget)->mConnection != -1) {
				Params* params = &act->mOwner->mParams;
				f32 remaining  = params->mBattery - 10.0f;
				if (remaining < 0)
					remaining = 0;
				SetClampedPlayerBattery(params, remaining);
			}
			((XObjPlug*)act->mTarget)->mConnection = -1;
		}
		break;
	case 17:
		if (anim != 0xbd) {
			act->mOwner->SetAnim(0xbd, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mOwner->UpdateAnim(0);
			SetLiftTargetState(act, 13);
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftActionState(act, 4);
			act->mPlugTarget = NULL;
		}
		break;
	case 18:
		if (anim != 0xac) {
			act->mOwner->SetAnim(0xac, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mOwner->UpdateAnim(0);
			SetLiftTargetState(act, 13);
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftActionState(act, 4);
			act->mPlugTarget = NULL;
		}
		break;
	case 19:
		if (anim != 10) {
			act->mOwner->SetAnim(10, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftTargetState(act, 14);
			if (act->mContactEffect) {
				gGame.mParticle.Kill(act->mContactEffectId);
				act->mContactEffect = NULL;
			}
			if (act->mToolEffect) {
				gGame.mParticle.Kill(act->mToolEffectId);
				// This path clears the first effect pointer again in all three editions.
				act->mContactEffect = NULL;
			}
		} else if (aobj->flags & 0x44000000) {
			SetLiftActionState(act, 0);
			SetLiftTargetState(act, 15);
			SetPlayerStandAnim(act->mOwner, 17, 0);
			if (act->mTarget && (s16)GetObjectPoolId(act->mTarget) == 8) {
				s8 sound = GetItemSoundIndex(GetObjectItemId(act->mTarget), 1);
				if (sound >= 0)
					PlayGameSound(&gGame.mSound, GetItemReleaseSoundId(sound), 255);
			}
		}
		break;
	// 20 to 22 take an item out of his inventory, 24 to 26 the same without counting it off
	case 20:
		if (act->mPendingItemId >= 0)
			act->mPendingItem = SpawnObjectFromPlacementIndex(&gGame, act->mPendingItemId);
		else
			act->mPendingItem = NULL;
		SetLiftActionState(act, 21);
		// fall through
	case 21:
		if (act->mPendingItem) {
			if (!IsObjectLoaded(act->mPendingItem))
				return;
			gGame.mStatus.mFlags |= 0x1000;
		}
		SetLiftActionState(act, 22);
		// fall through
	case 22:
		if (anim != 11) {
			act->mOwner->SetAnim(11, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			SetLiftTargetState(act, 3);
			if (IsLiftTargetPlug(act)) {
				ClearObjectStatusFlags(act->mTarget, 0x0400000000000000ULL);
				act->mTarget = NULL;
			}
			if (act->mPendingItemId >= 0 && act->mPendingItem) {
				act->mPendingItem->mStatus.mFlags |= 0x0400000000000000ULL;
				act->mPendingItem->SetScale(0, 0);
				SetPlayerActionTarget(act, act->mPendingItem);
				s32 count      = RemovePlayerInventoryItemCount(GetPlayerParamsByIndex(&gGame, 0), GetObjectItemId(act->mTarget), 1);
				s16 difference = count - GetStageCollectionFlagBase(0);
				if (difference > 0)
					act->mPendingItem->mPlacementParam1 = difference;
				act->mPendingItemId = -1;
				if (IsCarryingSquirter(act))
					ApplyPlayerItemTintToTarget(act);
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftActionState(act, 4);
			SetLiftTargetState(act, 4);
		}
		break;
	case 23:
		if (anim != 0xdd) {
			act->mOwner->SetAnim(0xdd, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftTargetState(act, 16);
		} else if (aobj->flags & 0x44000000) {
			SetPlayerStandAnim(act->mOwner, 17, 0);
			SetLiftActionState(act, 0);
			SetLiftTargetState(act, 17);
		}
		break;
	case 24:
		if (act->mPendingItemId >= 0)
			act->mPendingItem = SpawnObjectFromPlacementIndex(&gGame, act->mPendingItemId);
		else
			act->mPendingItem = NULL;
		SetLiftActionState(act, 25);
		// fall through
	case 25:
		if (act->mPendingItem) {
			if (!IsObjectLoaded(act->mPendingItem))
				return;
			gGame.mStatus.mFlags |= 0x1000;
		}
		SetLiftActionState(act, 26);
		// fall through
	case 26:
		if (anim != 0xec) {
			act->mOwner->SetAnim(0xec, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			SetLiftTargetState(act, 18);
			if (IsLiftTargetPlug(act)) {
				ClearObjectStatusFlags(act->mTarget, 0x0400000000000000ULL);
				act->mTarget = NULL;
			}
			if (act->mPendingItemId >= 0 && act->mPendingItem) {
				act->mPendingItem->mStatus.mFlags |= 0x0400000000000000ULL;
				act->mPendingItem->SetScale(0, 0);
				SetPlayerActionTarget(act, act->mPendingItem);
				act->mPendingItemId = -1;
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftActionState(act, 4);
			SetLiftTargetState(act, 19);
		}
		break;
	// a with a tool out: the toothbrush (27, 28), spoon (29, 30), mug (31 to 33) and squirter (35, 36)
	case 27:
		act->mOwner->SetAnim(0x895, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		act->mTarget->SetAnim(1, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		SetLiftActionState(act, 28);
		act->mCheckContact = TRUE;
		break;
	case 28:
		if (frame > 2) {
			SetLiftActionState(act, 4);
			s32 path[5]  = { 0, 0, 0, 0, 0 };
			Vec position = CJObj_GetJointWorldPositionByPath(act->mTarget, 5, path);
			if (CheckCarriedObjectContacts(act, &position, 3, 1, TRUE, FALSE, FALSE, NULL))
				act->mCheckContact = FALSE;
		}
		break;
	case 29:
		act->mOwner->SetAnim(0x8b7, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		act->mTarget->SetAnim(5, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		SetLiftActionState(act, 30);
		break;
	case 30:
		if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(0x8ae, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftActionState(act, 4);
			f32 yaw = act->mOwner->mMoveRotate.y;
			Vec pos = act->mOwner->mMoveTranslate;
			pos.x += 3.0f * (f32)sin(yaw);
			pos.z += 3.0f * (f32)cos(yaw);
			if (!CheckCarriedObjectContacts(act, &pos, 1, 1, TRUE, FALSE, FALSE, NULL)) {
				act->mOwner->SetAnim(0x8af, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				PlayGameSound(&gGame.mSound, 0x110, 255);
				s32 path[]        = { 0, 0 };
				const CVec& point = CJObj_GetJointWorldPositionByPath(act->mTarget, 2, path);
				CreateScaledParticleAtPosition(&gGame, 1, 0x2e, 1, point, GetObjectJointMatrixScale(act->mTarget), FALSE);
			}
		}
		break;
	case 31:
		if (anim != 0x8b1) {
			act->mOwner->SetAnim(0x8b1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->mTarget->SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->mUseEffect = NULL;
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(0x8b2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->mTarget->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftActionState(act, 32);
		} else if (GetJointAnimationCurrentFrame(act->mOwner) > 10 && !act->mUseEffect) {
			act->mUseEffectId = 47;
			act->mUseEffect   = CreateParticleByBankAndId(&gGame, 1, act->mUseEffectId, 1, act->mOwner->mJObj, FALSE);
		}
		break;
	case 33:
		if (anim != 0x8b3) {
			act->mOwner->SetAnim(0x8b3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->mTarget->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetLiftActionState(act, 4);
		}
		break;
	case 34:
		PutAwayLiftedObject(act);
		break;
	// the squirter fills up from what's in front of him, low (0x8d2) or high (0x8c1), or else squirts.
	// its inventory count (at most 10) is how full it is, sSquirterChargeLevels turns that into a squirt level, 3 to 0
	case 35: {
		f32 angle = act->mOwner->mMoveRotate.y;
		Vec point = act->mOwner->mMoveTranslate;
		point.x += 2.0f * (f32)sin(angle);
		point.z += 2.0f * (f32)cos(angle);
		if (CheckCarriedObjectContacts(act, &point, 2, 1, TRUE, FALSE, FALSE, NULL)) {
			act->mOwner->mSquirterContent = 0;
			act->mOwner->mItemTintWord    = -1;
			act->mOwner->SetAnim(0x8d2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->mTarget->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else {
			point.x += 2.0f * (f32)sin(angle);
			point.z += 2.0f * (f32)cos(angle);
			point.y += 7.0f;
			if (CheckCarriedObjectContacts(act, &point, 2, 3, TRUE, FALSE, FALSE, NULL)) {
				act->mOwner->mSquirterContent = 0;
				act->mOwner->mItemTintWord    = -1;
				act->mOwner->SetAnim(0x8c1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTarget->SetAnim(10, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
		}
		if (act->mOwner->mAnimId != 0x8d2 && act->mOwner->mAnimId != 0x8c1) {
			s16 count       = (s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38);
			s32 chargeLevel = sSquirterChargeLevels[count];
			if (chargeLevel == 3) {
				act->mOwner->SetAnim(0x8c2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTarget->SetAnim(11, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);

			} else if (chargeLevel == 2) {
				act->mOwner->SetAnim(0x8c3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTarget->SetAnim(12, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);

			} else if (chargeLevel == 1) {
				act->mOwner->SetAnim(0x8c4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTarget->SetAnim(13, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);

			} else if (count <= 1) {
				act->mOwner->mSquirterContent = 0;
				act->mOwner->mItemTintWord    = -1;
				act->mOwner->SetAnim(0x8c5, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTarget->SetAnim(14, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
		}
		act->mTarget->UpdateAnim(frames);
		SetLiftActionState(act, 36);
		act->mCheckToolHit = TRUE;
		break;
	}
	case 36:
		if (aobj->flags & 0x44000000) {
			BOOL finished = FALSE;
			s16 count     = (s16)(GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38) - 1);
			if (anim == 0x8c2) {
				if (sSquirterChargeLevels[count] == 3)
					act->mTarget->SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				else
					act->mTarget->SetAnim(3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, count, FALSE);
				finished = TRUE;
			} else if (anim == 0x8c3) {
				if (sSquirterChargeLevels[count] == 2)
					act->mTarget->SetAnim(3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				else
					act->mTarget->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, count, FALSE);
				finished = TRUE;
			} else if (anim == 0x8c4) {
				if (sSquirterChargeLevels[count] == 1)
					act->mTarget->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				else
					act->mTarget->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, count, FALSE);
				finished = TRUE;
			} else if (anim == 0x8c5) {
				act->mTarget->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				finished = TRUE;
			} else if (anim == 0x8d2) {
				act->mOwner->SetAnim(0x8c0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTarget->SetAnim(9, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x8c0) {
				act->mOwner->SetAnim(0x8d3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTarget->SetAnim(18, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x8d3) {
				act->mTarget->SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				// filled up, the inventory keeps the count at 10 at most
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, 11, FALSE);
				finished = TRUE;
			} else if (anim == 0x8c1) {
				act->mTarget->SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, 11, FALSE);
				finished = TRUE;
			}
			act->mTarget->UpdateAnim(frames);
			if (finished) {
				act->mOwner->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				SetLiftActionState(act, 4);
				if (act->mToolEffect) {
					gGame.mParticle.Kill(act->mToolEffectId);
					act->mToolEffect = NULL;
				}
			}
		} else if (anim == 0x8c2 || anim == 0x8c3 || anim == 0x8c4 || anim == 0x8c5) {
			if (act->mCheckToolHit) {
				s32 first[]    = { 0, 0 };
				HSD_JObj* root = GetObjectJointByPath(act->mTarget, 2, first);
				s32 second[]   = { 0, 3 };
				HSD_JObj* tip  = GetObjectJointByPath(act->mTarget, 2, second);
				CVec pos(GetJointMatrixTranslation(act->mTarget, root));
				f32 distance = (pos - GetJointMatrixTranslation(act->mTarget, tip)).Length();
				if (distance > 0.1f) {
					Vec scale = GetJointMatrixScale(act->mTarget, root);
					Vec hit;
					if (CheckCarriedObjectContacts(act, &pos, scale.x, 0.5f * scale.y, FALSE, TRUE, TRUE, &hit)) {
						act->mCheckToolHit = FALSE;
						if (act->mToolEffect) {
							gGame.mParticle.Kill(act->mToolEffectId);
							act->mToolEffect = NULL;
						}
						hit.x += scale.x * (f32)sin(act->mOwner->mMoveRotate.y);
						hit.z += scale.x * (f32)cos(act->mOwner->mMoveRotate.y);
						CreateParticleAtPosition(&gGame, 1, 0x5a, 1, hit, FALSE);
					}
					if (act->mCheckToolHit && !act->mToolEffect) {
						act->mToolEffectId   = -1;
						HSD_JObj* attachment = NULL;
						if (anim == 0x8c5) {
							if (frame > 10) {
								attachment         = tip;
								act->mToolEffectId = 0x42;
							}
						} else {
							attachment         = root;
							act->mToolEffectId = 0x3c;
						}
						if (act->mToolEffectId >= 0)
							act->mToolEffect = CreateParticleByBankAndId(&gGame, 1, act->mToolEffectId, 0, attachment, TRUE);
						if (act->mToolEffect && anim != 0x8c5)
							psSetUserFunc(act->mToolEffect, &sPlayerItemParticleUserFunc);
					}
				}
			}
		} else {
			ApplyPlayerItemTintToTarget(act);
		}
		break;
	}
}
}

#include "game/cjobjinlineattach.h"

// a += b, b by value like the other CVec operators
static inline void AddVec(CVec& a, Vec b)
{
	PSVECAdd(&a, &b, &a);
}

// a -= b, the same way
static inline void SubVec(CVec& a, Vec b)
{
	PSVECSubtract(&a, &b, &a);
}

// a held object of the given pool and kind
static inline bool IsHolding(XAction* act)
{
	bool result = false;
	if (act->mTarget != NULL && (act->mTarget->mStatus.mFlags & 0x400000000000ULL)) {
		result = true;
	}
	return result;
}

static inline bool IsHoldingPool(XAction* act, s16 pool)
{
	bool result = false;
	if (IsHolding(act) && act->mTarget->GetCategory() == pool) {
		result = true;
	}
	return result;
}

static inline bool IsHoldingKind(XAction* act, s16 pool, s16 kind)
{
	bool result = false;
	if (IsHoldingPool(act, pool) && act->mTarget->GetItem() == kind) {
		result = true;
	}
	return result;
}

// holding the item and not throwing it
static inline bool IsCarrying(XAction* act, s16 kind)
{
	bool result = false;
	if (IsHoldingKind(act, 8, kind) && act->mTargetState != 15) {
		result = true;
	}
	return result;
}

static inline bool IsTargetPool(XAction* act, s16 pool)
{
	bool result = false;
	if (act->mTarget != NULL && act->mTarget->GetCategory() == pool) {
		result = true;
	}
	return result;
}

static inline bool IsStill(XAction* act)
{
	bool result = false;
	if (act->mState == 31 || act->mState == 32 || act->mState == 33) {
		result = true;
	}
	return result;
}

static inline bool IsCarryingStill(XAction* act, s16 kind)
{
	bool result = false;
	if (IsCarrying(act, kind) && IsStill(act)) {
		result = true;
	}
	return result;
}

// lets go of a pushed object and resets
static inline void Release(XAction* act)
{
	XObj* target = act->mTarget;
	if (target != NULL && (target->mStatus.mFlags & 0x0800000000000000ULL)) {
		target->mStatus.mFlags &= ~0x0800000000000000ULL;
	}
	act->Reset();
}

// drops the item he's carrying, it goes in his inventory
static inline void PutAway(XActLift* act)
{
	XObjChr* chr = act->mOwner;
	if (chr->mAnimId == 0x8B1 || chr->mAnimId == 0x8B3) {
		chr->SetAnim(0x8B2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	} else if (chr->mAnimId == 11) {
		chr->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	}
	act->mState              = 0;
	act->mOwner->mAttachment = 0;
	act->mTarget->mStatus.mFlags &= ~0x0200000000000000ULL;
	act->mTarget->SetAnim(7, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	act->mTargetState = 20;
	XObjItem* item    = (XObjItem*)act->mTarget;
	SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, item->GetItem(), 1, TRUE);
	item->SetFlag();
	PlayGameSound(&gGame.mSound, 0x39, 0xFF);
}

extern "C" Vec lbl_8037C340;

static inline Vec GetFloorExtent(XObjFloor* const* floors, s32 index)
{
	Vec value = lbl_8037C340;
	value.x   = (*floors)[index].mExtent[0] / 100.0f;
	value.y   = (*floors)[index].mExtent[1] / 100.0f;
	value.z   = (*floors)[index].mExtent[2] / 100.0f;
	return value;
}

extern "C" {

BOOL SetClampedPushPositionXZ(XObj* target, const Vec* direction, u8 axes, f32 x, f32 z);
f32 QueryObjectFloorHeight(XObj* target, f32 radius, f32 height);

static const f32 pushAngleTolerance = 0.1f * DEG2RAD;

static inline Vec PushEdgePoint(const XObj* obj, s32 index, bool first, f32 height)
{
	const Vec& point = first ? obj->mEdges[index].mFrom : obj->mEdges[index].mTo;
	Vec v            = { obj->mMoveTranslate.x + point.x, height, obj->mMoveTranslate.z + point.z };
	return v;
}
static inline f32 FacingPushEdge(XObj* edgeObject, XObjChr* owner)
{
	s32 index   = edgeObject->mSelectedFloorIndex;
	Vec& player = owner->mMoveTranslate;
	Vec a       = PushEdgePoint(edgeObject, index, true, player.y);
	Vec b       = PushEdgePoint(edgeObject, index, false, player.y);
	CVec normal;
	GetOutwardPerpendicularToEdge(&player, &a, &b, &normal);
	return WrapAngle(atan2(normal.x, normal.z));
}
void UpdatePushedObject(XActPush* act, u32 frames)
{
	if (!act->mTargetState)
		return;
	XObj* target = act->mTarget;
	if (target != NULL && (target->mStatus.mFlags & 1)) {
		switch (act->mTargetState) {
		case 1: {
			target->mStatus.mFlags |= 0x0800000000000000ULL;
			Vec hand           = GetPlayerCarryJointWorldPosition(act->mOwner);
			const CVec& diff   = target->mMoveTranslate - hand;
			act->mTargetOffset = CVec(diff.x, diff.y, diff.z);
			break;
		}
		case 2: {
			Vec hand         = GetPlayerCarryJointWorldPosition(act->mOwner);
			XObj* edgeObject = act->mTarget;
			f32 angle        = WrapAngle(FacingPushEdge(edgeObject, act->mOwner) - edgeObject->mMoveRotate.y);
			Mtx rotate;
			PSMTXRotRad(rotate, 'y', angle);
			CVec direction(0, 0, act->mMoveDirection);
			PSMTXMultVec(rotate, &direction, &direction);
			if ((angle > (90 * DEG2RAD - pushAngleTolerance) && angle < (90 * DEG2RAD + pushAngleTolerance))
			    || (angle > (270 * DEG2RAD - pushAngleTolerance) && angle < (270 * DEG2RAD + pushAngleTolerance)))
				act->mPushPositionClamped
				    = SetClampedPushPositionXZ(target, &direction, 1, hand.x + act->mTargetOffset.x, hand.z + act->mTargetOffset.z);
			else
				act->mPushPositionClamped
				    = SetClampedPushPositionXZ(target, &direction, 2, hand.x + act->mTargetOffset.x, hand.z + act->mTargetOffset.z);

			target->OnPushDisplacement(target->mMoveTranslate - target->mLastPos);
			if ((target->mStatus.mFlags & 0x4000000000ULL)
			    && QueryObjectFloorHeight(target, 0.1f, 0.1f) < target->mMoveTranslate.y - 5.0f) {
				target->mStatus.mFlags &= ~0x0800000000000000ULL;
				Release(act);
				XObjChr* owner = act->mOwner;
				owner->SetAnim(17, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				owner->mIdleLoops = 3;
				act->mState       = 0;
				target->mStatus.mFlags |= 0x1000000000000000ULL;

				Vec rotation = target->mMoveRotate, speed = target->mMoveRotate.mSpeed;
				Vec extent = GetFloorExtent(&target->mFloors, 0);
				f32 dx, dz;
				if ((angle > (90 * DEG2RAD - pushAngleTolerance) && angle < (90 * DEG2RAD + pushAngleTolerance))) {
					rotation.z -= HALF_PI;
					speed.z = 0.1f;
					dz      = 0;
					dx      = extent.x * (2.0f / sqrtf(3.0f)) / 2.0f;
				} else if ((angle > (270 * DEG2RAD - pushAngleTolerance) && angle < (270 * DEG2RAD + pushAngleTolerance))) {
					rotation.z += HALF_PI;
					speed.z = 0.1f;
					dz      = 0;
					dx      = -(extent.x * (2.0f / sqrtf(3.0f))) / 2.0f;
				} else if ((angle > (0 * DEG2RAD - pushAngleTolerance) && angle < (0 * DEG2RAD + pushAngleTolerance))) {
					rotation.x += HALF_PI;
					speed.x = 0.1f;
					dx      = 0;
					dz      = extent.z * (2.0f / sqrtf(3.0f)) / 2.0f;
				} else if ((angle > (180 * DEG2RAD - pushAngleTolerance) && angle < (180 * DEG2RAD + pushAngleTolerance))) {
					rotation.x -= HALF_PI;
					speed.x = 0.1f;
					dx      = 0;
					dz      = -(extent.z * (2.0f / sqrtf(3.0f))) / 2.0f;
				}
				target->mMoveRotate.mTarget.Set(rotation);
				target->mMoveRotate.mSpeed.Set(speed);
				target->mToppleOffset = CVec(dx, -(extent.y * (2.0f / sqrtf(3.0f))) / 2.0f, dz);
			}
			break;
		}
		}
	}
}

/**
 * @note Address: 0x800C5270
 * @note Size: 0x464
 */
void UpdatePushActionAnimation(XActPush* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x3E) {
			if (act->mTarget != NULL) {
				f32 angle = GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 1, 0, 0);
				if (fpclassify(angle) != FP_NAN) {
					act->mOwner->SetAnim(0x3E, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
					act->mOwner->SetRotationY(angle, 0);
				} else {
					Release(act);
					act->mState = 0;
				}
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(0x3F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState       = 2;
			act->mTargetState = 1;
		}
		break;
	case 2:
		if (act->mMoveDirection == 1) {
			if (anim == 0x4B) {
				if (aobj->flags & 0x44000000) {
					act->mOwner->SetAnim(0x40, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					act->mTargetState = 2;
				}
			} else if (anim != 0x40) {
				act->mOwner->SetAnim(0x4B, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTargetState = 0;
			}
		} else if (act->mMoveDirection == -1) {
			if (anim == 0x4C) {
				if (aobj->flags & 0x44000000) {
					act->mOwner->SetAnim(0x41, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					act->mTargetState = 2;
				}
			} else if (anim != 0x41) {
				act->mOwner->SetAnim(0x4C, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTargetState = 0;
			}
		} else if (anim != 0x3F) {
			act->mOwner->SetAnim(0x3F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->mTargetState = 0;
		}
		break;
	case 3:
		if (anim != 0x42) {
			act->mOwner->SetAnim(0x42, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else if (aobj->flags & 0x44000000) {
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	}
}

// lets go of what he's pushing once a is let go, japan uses b while the game flag is up
/**
 * @note Address: 0x800C56D4
 * @note Size: 0x248
 */
void HandlePushActionInput(XActPush* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
#if defined(VERSION_JP)
			if (gGame.mStatus.mFlags & 0x0000080000000000ULL) {
				if (pad->mTrigger & PAD_BUTTON_B) {
					Release(act);
					act->mState = 3;
					*weight     = 0.0f;
				}
			} else
#endif
			    if (!(pad->mButton & PAD_BUTTON_A)) {
				Release(act);
				act->mState = 3;
				*weight     = 0.0f;
			}
		}
	} else if (act->mState != 0) {
		if (act->mState != 3 && !(pad->mButton & PAD_BUTTON_A)
#if defined(VERSION_JP)
		    && !(gGame.mStatus.mFlags & 0x0000080000000000ULL)
#endif
		) {
			Release(act);
			XObjChr* chr = act->mOwner;
			chr->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x800C591C
 * @note Size: 0xE4
 */
void CancelPlayerPushAction(XActPush* act)
{
	if (act->mState != 0) {
		Release(act);
		act->mState  = 0;
		XObjChr* chr = act->mOwner;
		chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		chr->mIdleLoops = 3;
	}
}

/**
 * @note Address: 0x800C5A00
 * @note Size: 0x7C
 */
void ReleasePushAction(XActPush* act)
{
	Release(act);
}

void PositionPlayerAroundRopeTarget(XActRope*, f32);

// header accessors (obja_chr emits copies); a read through them is not merged with a direct act->mTarget read
static inline BOOL HasPlayerActionTarget(const XAction* action)
{
	return action->mTarget != NULL;
}
static inline XObj* GetPlayerActionTarget(const XAction* action)
{
	return action->mTarget;
}
static inline f32 GetFloorYawRadians(XObjFloor* const* floors, s32 index)
{
	return DEG2RAD * (*floors)[index].mYawDegrees;
}
static inline s32 GetActionTargetFloor(const XAction* action)
{
	return action->mTargetFloor;
}

static inline void RopeAnim(XActRope* act, s32 id, f32 loop, BOOL force = FALSE, f32 start = 0)
{
	act->mOwner->SetAnim(id, force, NULL, NULL, NULL, start, 1, loop, 10);
}
static inline void ReleaseRopePlug(XActRope* act, f32 frame, f32 threshold)
{
	if (CheckStageFlag(gGame.mStage, 1)) {
		XObjPlug* plug = &act->mOwner->mPlug;
		if (plug->mConnection && frame >= threshold) {
			ResetPlayerPlugState(plug, TRUE);
			PlayGameSound(&gGame.mSound, 0x37, 0xff);
		}
	}
}
static inline void SyncRopePosition(XObjChr* owner)
{
	if (owner->mMoveTranslate.y != owner->mLastPos.y)
		owner->mStatus.mFlags |= 0x0010000000000000ULL;
	owner->mLastPos = owner->mMoveTranslate;
}

// an inline: retail emits it right behind UpdatePlayerRopeAction, its first caller, and only the giants call it
/**
 * @note Address: 0x800C7810
 * @note Size: 0x2B4
 */
inline void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos)
{
	CJObj joint(GetJointByPath(obj->mJObj, path, depth));
	joint.SetTranslate(pos.x, pos.y, pos.z, TRUE);
}

/**
 * @note Address: 0x800C5B98
 * @note Size: 0x1C78 (US)
 */
// NONMATCHING: case 5 steps the yaw from mMoveRotate.y read again in each branch (US 0x800C7198, 0x800C7204 lfs 0xfc(r3)),
// here the value read for the test is reused; writing the field read in both branches compiles the same. The
// GetEventJointWorldPosition path pair at 0x800C6FF4 is two .sdata labels in retail's symbols, one literal here
void UpdatePlayerRopeAction(XActRope* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x21) {
			RopeAnim(act, 0x21, 0);
			if (act->mTarget) {
				act->mOwner->SetRotationY(WrapAngle(atan2(act->mTarget->mMoveTranslate.x - act->mOwner->mMoveTranslate.x,
				                                          act->mTarget->mMoveTranslate.z - act->mOwner->mMoveTranslate.z)),
				                          0);
				CVec player(act->mOwner->mMoveTranslate);
				player.y = 0;
				CVec rope(act->mTarget->mMoveTranslate);
				rope.y = 0;
				CVec offset(act->mTargetOffset);
				offset.y = 0;
				Mtx matrix;
				PSMTXRotRad(matrix, 'y', act->mOwner->mMoveRotate.y);
				PSMTXMultVec(matrix, &offset, &offset);
				SubVec(rope, offset);
				act->mPos   = rope;
				act->mDist  = PSVECDistance(&rope, &player);
				act->mAngle = atan2(player.x - rope.x, player.z - rope.z);
			}
		} else if (aobj->flags & 0x44000000) {
			RopeAnim(act, 0x8c, 0);
			act->mState = 5;
		} else {
			if (act->mTarget) {
				Vec delta = GetPlayerActionTarget(act)->mMoveTranslate - act->mTarget->mLastPos;
				act->mPos.x += delta.x;
				act->mPos.z += delta.z;
				if (act->mDist > 0 && frame > 20) {
					if ((act->mDist -= .2f * frames) < 0)
						act->mDist = 0;
					act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
					act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
				} else {
					act->mOwner->SetTranslateX(act->mPos.x, 0);
					act->mOwner->SetTranslateZ(act->mPos.z, 0);
				}
			}
			ReleaseRopePlug(act, frame, 28);
		}
		break;
	case 2:
		if (anim != 0x98) {
			RopeAnim(act, 0x98, 0);
			if (act->mTarget) {
				act->mOwner->SetRotationY(WrapAngle(act->mTarget->mMoveRotate.y), 0);
				CVec player(act->mOwner->mMoveTranslate);
				player.y = 0;
				CVec rope(act->mTarget->mMoveTranslate);
				rope.y = 0;
				CVec offset;
				offset.Set(0, 0, -.5f);
				Mtx matrix;
				PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(matrix, &offset, &offset);
				AddVec(rope, offset);
				act->mPos   = rope;
				act->mDist  = PSVECDistance(&rope, &player);
				act->mAngle = atan2(player.x - rope.x, player.z - rope.z);
			}
		} else if (aobj->flags & 0x44000000) {
			s32 path[] = { 0 };
			CVec pos   = GetEventJointWorldPosition(act->mOwner, 1, path);
			Vec offset = act->mTargetOffset;
			Mtx matrix;
			PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
			PSMTXMultVec(matrix, &offset, &offset);
			act->mOwner->SetPos(pos.x + offset.x, pos.y - offset.y, pos.z + offset.z, 0);
			SyncRopePosition(act->mOwner);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y, 0);
			s32 p2[] = { 0 };
			SetJointYawByPath(act->mOwner, 1, p2, 0);
			s32 p3[] = { 0 };
			SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, act->mTargetOffset);
			RopeAnim(act, 0x8c, 0);
			act->mState = 5;
		} else {
			if (act->mTarget) {
				CVec delta = GetPlayerActionTarget(act)->mMoveTranslate - act->mTarget->mLastPos;
				act->mPos.x += delta.x;
				act->mPos.z += delta.z;
				if (act->mDist > 0 && frame > 80) {
					if ((act->mDist -= .2f * frames) < 0)
						act->mDist = 0;
					act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
					act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
				}
			}
			ReleaseRopePlug(act, frame, 80);
		}
		break;
	case 3:
		if (anim != 0x85) {
			if (act->mTarget) {
				act->mOwner->SetRotationY(act->mTarget->mMoveRotate.y, 0);
				RopeAnim(act, 0x8a, 1, TRUE);
				act->mOwner->UpdateAnim(0);
				s32 p1[]   = { 0 };
				CVec local = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
				s32 p2[]   = { 0 };
				CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
				delta.y    = -delta.y;
				act->mOwner->SetTranslateY(act->mOwner->mMoveTranslate.y + delta.y, 0);
				RopeAnim(act, 0x85, 1);
				CVec player(act->mOwner->mMoveTranslate);
				player.y = 0;
				CVec rope(act->mTarget->mMoveTranslate);
				rope.y = 0;
				CVec offset;
				offset.Set(0, 0, -.5f);
				Mtx matrix;
				PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(matrix, &offset, &offset);
				AddVec(rope, offset);
				act->mPos   = rope;
				act->mDist  = PSVECDistance(&rope, &player);
				act->mAngle = atan2(player.x - rope.x, player.z - rope.z);
			}
			ReleaseRopePlug(act, 0, 0);
		} else if (aobj->flags & 0x44000000) {
			s32 p1[]        = { 0 };
			HSD_JObj* child = GetJointByPath(act->mOwner->mJObj, p1, 1);
			f32 turn        = HSD_JObjGetRotationY(child);
			act->mOwner->SetRotationY(HSD_JObjGetRotationY(act->mOwner->mJObj) + turn, 0);
			HSD_JObjSetRotationY(child, 0);
			s32 p2[]   = { 0 };
			CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
			delta.y    = 0;
			CVec offset(act->mTargetOffset);
			Mtx matrix;
			PSMTXRotRad(matrix, 'y', HSD_JObjGetRotationY(act->mOwner->mJObj));
			PSMTXMultVec(matrix, &offset, &offset);
			offset.y = 0;
			act->mOwner->SetPos(act->mOwner->mMoveTranslate - (delta + offset), 0);
			SyncRopePosition(act->mOwner);
			s32 p3[] = { 0 };
			SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, act->mTargetOffset);
			RopeAnim(act, 0x8c, 0);
			act->mState = 5;
		} else if (act->mTarget) {
			Vec delta = GetPlayerActionTarget(act)->mMoveTranslate - act->mTarget->mLastPos;
			act->mPos.x += delta.x;
			act->mPos.z += delta.z;
			if (act->mDist > 0) {
				if ((act->mDist -= .4f * frames) < 0)
					act->mDist = 0;
				act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
				act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
			} else {
				act->mOwner->SetTranslateX(act->mPos.x, 0);
				act->mOwner->SetTranslateZ(act->mPos.z, 0);
			}
		}
		break;
	case 4:
		RopeAnim(act, 0x8c, 1);
		act->mState = 5;
		break;
	case 5:
		if (anim == 0x8b) {
			if (aobj->flags & 0x44000000) {
				if (act->mMoveDirection == 1)
					RopeAnim(act, 0x8d, 1);
				else
					RopeAnim(act, 0x8c, 1);
			}
		} else if (anim == 0x8d) {
			if (aobj->flags & 0x44000000) {
				if (act->mMoveDirection == 1)
					RopeAnim(act, 0x8b, 1);
				else
					RopeAnim(act, 0x8a, 1);
			}
		} else if (act->mMoveDirection == 1 || act->mMoveDirection == 2) {
			if (anim == 0x8a)
				RopeAnim(act, 0x8b, 1);
			else if (anim == 0x8c)
				RopeAnim(act, 0x8d, 1);
#if defined(VERSION_PAL)
			else if (anim == 0x23)
				RopeAnim(act, 0x8c, 0);
#endif
		} else if (act->mMoveDirection == -1) {
			if (act->mOwner->GetHeight() > 0) {
				if (anim != 0x23)
					RopeAnim(act, 0x23, 0);
			} else {
				act->mState = 8;
				RopeAnim(act, 0x25, 0);
			}
		} else if (anim == 0x23)
			RopeAnim(act, 0x8c, 0);
		if (act->mMoveDirection == 1 || act->mMoveDirection == 2) {
			if (act->mTarget && GetEventJointWorldPosition(act->mOwner, 2, (s32[]) { 0, 0 }).y >= act->mTarget->mFloorHeights[0]) {
				act->mOwner->SetTranslateY(act->mTarget->mFloorHeights[0] - act->mTargetOffset.y, 0);
				RopeAnim(act, 0x24, 0);
				act->mState = 6;
				break;
			}
			PlayGameSound(&gGame.mSound, 0x11, 0xff);
		} else if (act->mMoveDirection == -1)
			PlayGameSound(&gGame.mSound, 0x15, 0xff);
		if (act->mTarget) {
			f32 target = WrapAngle(PI + act->mTarget->mMoveRotate.y);
			f32 angle  = act->mOwner->mMoveRotate.y;
			if (WrapAngle(target - angle) >= PI) {
				angle -= DEG2RAD * 2.f * frames;
				if (WrapAngle(target - angle) < PI)
					angle = target;
			} else {
				angle += DEG2RAD * 2.f * frames;
				if (WrapAngle(target - angle) > PI)
					angle = target;
			}
			if (anim == 0x34 || anim == 0x8b || anim == 0x8d || anim == 0x23)
				act->mOwner->SetRotationY(angle, 0);
			PositionPlayerAroundRopeTarget(act, angle);
		}
		break;
	case 6:
		if (anim != 0x24)
			RopeAnim(act, 0x24, 0);
		else if (aobj->flags & 0x44000000) {
			s32 p[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, p), 0);
			SyncRopePosition(act->mOwner);
			s32 p2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, p2, 0, 0, 0);
			XObjChr* owner = act->mOwner;
			owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			owner->mIdleLoops = 3;
			act->mState       = 0;
			act->mTarget      = NULL;
		}
		break;
	case 7:
		RopeAnim(act, 0x25, 1, TRUE, 20);
		act->mState = 8;
	// fall through
	case 8:
		if (anim != 0x25)
			RopeAnim(act, 0x25, 0);
		else if (aobj->flags & 0x44000000) {
			if (act->mOwner->GetAttachment() == 1) {
				act->mState                 = 0;
				act->mOwner->mCopter.mState = 7;
			} else {
				RopeAnim(act, 0x38, 0);
				act->mState = 9;
			}
			act->mTarget = NULL;
		}
		break;
	case 9:
		if (act->mOwner->GetHeight() <= 0) {
			if (act->mOwner->mFallDistance > 30) {
				RopeAnim(act, 0x91, 1);
				act->mState = 11;
				ApplyPlayerJumpStrength(&act->mOwner->mJump, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 255, 0, 0, 0);
				act->mOwner->SetColor(1, 0, 0, 0, .3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, .3f);
			} else {
				RopeAnim(act, 0x59, 1);
				act->mState = 10;
			}
		}
		break;
	case 10:
		if (aobj->flags & 0x44000000) {
			XObjChr* owner = act->mOwner;
			owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			owner->mIdleLoops = 3;
			act->mState       = 0;
		}
		break;
	case 11:
		if (aobj->flags & 0x44000000) {
			act->mState = 0;
			RopeAnim(act, 0x8f, 1);
			act->mOwner->mJump.mState = 6;
			gGame.mParticle.Create(0x10015, 0, act->mOwner->mJObj, TRUE);
			PlayGameSound(&gGame.mSound, 0x1a, 255);
			PlayGameSound(&gGame.mSound, 0x36, 255);
		}
		break;
	}
	if (act->mState == 5) {
		HSD_JObj* joint = GetJointByPath(act->mOwner->mJObj, (s32[]) { 0 }, 1);
		if (joint)
			act->mOwner->Push(joint, 2, act->mTargetOffset.y);
	}
}

// walks him around the target at the same distance
/**
 * @note Address: 0x800C5A7C
 * @note Size: 0x11C
 */
void PositionPlayerAroundRopeTarget(XActRope* act, f32 angle)
{
	if (act->mTarget != NULL) {
		CVec dir((Vec) { act->mTargetOffset.x, 0.0f, act->mTargetOffset.z });
		f32 dist = dir.Length();
		act->mOwner->SetTranslateX(act->mTarget->mMoveTranslate.x - dist * (f32)sin(angle), 0);
		act->mOwner->SetTranslateZ(act->mTarget->mMoveTranslate.z - dist * (f32)cos(angle), 0);
	}
}

/**
 * @note Address: 0x800C7AC4
 * @note Size: 0x100
 */
void HandleRopeActionInput(XActRope* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			// b lets go of the rope
			if (pad->mTrigger & PAD_BUTTON_B) {
				act->mState = 8;
			}
			if (act->mOwner->mAnimId == 0x8A || act->mOwner->mAnimId == 0x8C) {
				*weight = 0.0f;
			}
		}
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x800C7BC4
 * @note Size: 0x1FB0
 */
// NONMATCHING: retail calls GetObjectJointByPath, the header inline (weak copy 0x800B2764); this function expands it,
// being over 1200 units under retail's estimate. RecordObjectPositionAndHeightChange and HSD_JObjSetRotationY stand behind
// it in retail, so they were inlines that weighed in the estimate; here they are plain functions defined further down.
// Made inlines (the jobj.h one and a file inline above this function) they still leave this function about 1450 units
// short at limit 128, and RecordObjectPositionAndHeightChange (about 58 units) shares the 33..64 class with CVec's
// constructors, which retail expands here: retail's Record weighed 65 or more. Hang is about 1100 short the same way.
void UpdateLadderActionAnimation(XActLadder* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x66) {
			act->mOwner->SetAnim(0x66, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			if (act->mTarget != NULL) {
				act->mOwner->SetRotationY(WrapAngle(PI + act->mTarget->mMoveRotate.y), 0);
				CVec chrPos(act->mOwner->mMoveTranslate);
				chrPos.y = 0.0f;
				CVec ladPos(act->mTarget->mMoveTranslate);
				ladPos.y = 0.0f;
				CVec off(0.0f, 0.0f, 4.0f);
				Mtx m;
				PSMTXRotRad(m, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(m, &off, &off);
				AddVec(ladPos, off);
				act->mPos.x = ladPos.x;
				act->mPos.y = ladPos.y;
				act->mPos.z = ladPos.z;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 }), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			SetObjectJointTranslationByPath(act->mOwner, 1, (s32[]) { 0 }, 0.0f, 0.0f, 0.0f);
			act->mOwner->SetAnim(0x67, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState = 5;
		} else {
			if (act->mTarget != NULL) {
				Vec move = GetPlayerActionTarget(act)->mMoveTranslate - act->mTarget->mLastPos;
				act->mPos.x += move.x;
				act->mPos.z += move.z;
				if (act->mDist > 0.0f) {
					if ((act->mDist -= 0.2f * frames) < 0.0f) {
						act->mDist = 0.0f;
					}
					act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
					act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
				} else {
					act->mOwner->SetTranslateX(act->mPos.x, 0);
					act->mOwner->SetTranslateZ(act->mPos.z, 0);
				}
			}
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->mConnection != 0 && frame >= 25.0f) {
					ResetPlayerPlugState(plug, TRUE);
					PlayGameSound(&gGame.mSound, 0x37, 0xFF);
				}
			}
		}
		break;
	case 2:
		if (anim != 0x6D) {
			act->mOwner->SetAnim(0x6D, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			if (act->mTarget != NULL) {
				act->mOwner->SetRotationY(WrapAngle(act->mTarget->mMoveRotate.y), 0);
				CVec chrPos(act->mOwner->mMoveTranslate);
				chrPos.y = 0.0f;
				CVec ladPos(act->mTarget->mMoveTranslate);
				ladPos.y = 0.0f;
				CVec off(0.0f, 0.0f, -1.5f);
				Mtx m;
				PSMTXRotRad(m, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(m, &off, &off);
				AddVec(ladPos, off);
				act->mPos.x = ladPos.x;
				act->mPos.y = ladPos.y;
				act->mPos.z = ladPos.z;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 }), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			SetObjectJointTranslationByPath(act->mOwner, 1, (s32[]) { 0 }, 0.0f, 0.0f, 0.0f);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y, 0);
			SetJointYawByPath(act->mOwner, 1, (s32[]) { 0 }, 0.0f);
			act->mOwner->SetAnim(0x68, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState = 5;
		} else {
			if (act->mTarget != NULL) {
				Vec move = GetPlayerActionTarget(act)->mMoveTranslate - act->mTarget->mLastPos;
				act->mPos.x += move.x;
				act->mPos.z += move.z;
				if (act->mDist > 0.0f && frame > 20.0f) {
					if ((act->mDist -= 0.2f * frames) < 0.0f) {
						act->mDist = 0.0f;
					}
					act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
					act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
				}
			}
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->mConnection != 0 && frame >= 50.0f) {
					ResetPlayerPlugState(plug, TRUE);
					PlayGameSound(&gGame.mSound, 0x37, 0xFF);
				}
			}
		}
		break;
	case 3:
		if (anim != 0x87) {
			if (act->mTarget != NULL) {
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec chrPos(act->mOwner->mMoveTranslate);
					chrPos.y = 0.0f;
					CVec pt(GetTargetFloorPosition(act));
					pt.y = 0.0f;
					act->mOwner->SetRotationY(PI + (f32)atan2(pt.x - chrPos.x, pt.z - chrPos.z), 0);
				} else {
					act->mOwner->SetRotationY(act->mTarget->mMoveRotate.y, 0);
				}
				act->mOwner->SetAnim(0x87, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mOwner->SetAnim(0x87, TRUE, NULL, NULL, NULL, GetJointAnimationEndFrame(act->mOwner), 1.0f, 1.0f, 10.0f);
				act->mOwner->UpdateAnim(0);
				CVec d = GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 }) - act->mOwner->mMoveTranslate;
				act->mOwner->SetTranslateX(act->mOwner->mMoveTranslate.x - d.x, 0);
				act->mOwner->SetTranslateZ(act->mOwner->mMoveTranslate.z - d.z, 0);
				act->mOwner->SetTranslateY(act->mTarget->mMaxFloorHeight, 0);
				act->mOwner->SetAnim(0x87, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				CVec chrPos(act->mOwner->mMoveTranslate);
				chrPos.y = 0.0f;
				CVec ladPos(act->mTarget->mMoveTranslate);
				ladPos.y = 0.0f;
				CVec off(0.0f, 0.0f, -1.0f);
				Mtx m;
				PSMTXRotRad(m, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(m, &off, &off);
				AddVec(ladPos, off);
				act->mPos.x = ladPos.x;
				act->mPos.y = ladPos.y;
				act->mPos.z = ladPos.z;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->mConnection != 0) {
					ResetPlayerPlugState(plug, TRUE);
					PlayGameSound(&gGame.mSound, 0x37, 0xFF);
				}
			}
		} else if (aobj->flags & 0x44000000) {
			HSD_JObj* joint = GetObjectJointByPath(act->mOwner, 1, (s32[]) { 0 });
			f32 turn        = HSD_JObjGetRotationY(joint);
			f32 facing      = HSD_JObjGetRotationY(act->mOwner->mJObj);
			act->mOwner->SetRotationY(facing + turn, 0);
			HSD_JObjSetRotationY__FP9_HSD_JObjf(joint, 0.0f);
			CVec d    = GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 }) - act->mOwner->mMoveTranslate;
			f32 angle = HSD_JObjGetRotationY(act->mOwner->mJObj);
			d.x       = -d.x;
			d.z       = -d.z;
			act->mOwner->SetPos((act->mOwner->mMoveTranslate + d), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			SetObjectJointTranslationByPath(act->mOwner, 1, (s32[]) { 0 }, 0.0f, 0.0f, 0.0f);
			act->mOwner->SetAnim(0x68, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState         = 5;
			act->mMoveDirection = 0;
		} else if (act->mTarget != NULL) {
			Vec move = GetPlayerActionTarget(act)->mMoveTranslate - act->mTarget->mLastPos;
			act->mPos.x += move.x;
			act->mPos.z += move.z;
			if (act->mDist > 0.0f) {
				if ((act->mDist -= 0.4f * frames) < 0.0f) {
					act->mDist = 0.0f;
				}
				act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
				act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
			} else {
				act->mOwner->SetTranslateX(act->mPos.x, 0);
				act->mOwner->SetTranslateZ(act->mPos.z, 0);
			}
			act->mOwner->SetTranslateY(act->mTarget->mMaxFloorHeight, 0);
		}
		break;
	case 4:
		act->mOwner->SetAnim(0x68, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		act->mState = 5;
		break;
	case 5:
		if (anim == 0x69 || anim == 0x6A || anim == 0x6B || anim == 0x6C) {
			if (aobj->flags & 0x44000000) {
				CVec hand = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, (s32[]) { 0 });
				if (hand.y < 0.0f) {
					hand.y = (s32)(hand.y - 0.5f);
				}
				SetJointTranslationFromVectorByPath(act->mOwner, 1, (s32[]) { 0 }, hand);
				act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 }), 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
				SetObjectJointTranslationByPath(act->mOwner, 1, (s32[]) { 0 }, 0.0f, 0.0f, 0.0f);
				if (act->mMoveDirection == 1) {
					// his hands are up at the top of the ladder
					if (act->mTarget != NULL
					    && 14.0f + GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 }).y >= act->mTarget->mFloorHeights[0]) {
						act->mState = 6;
					} else if (anim == 0x69 || anim == 0x6B) {
						act->mOwner->SetAnim(0x6A, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					} else if (anim == 0x6A || anim == 0x6C) {
						act->mOwner->SetAnim(0x69, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					}
				} else if (act->mMoveDirection == -1) {
					if (act->mOwner->GetHeight() <= 2.0f) {
						act->mState = 7;
					} else if (anim == 0x69 || anim == 0x6B) {
						act->mOwner->SetAnim(0x6C, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					} else if (anim == 0x6A || anim == 0x6C) {
						act->mOwner->SetAnim(0x6B, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					}
				} else if (anim == 0x69 || anim == 0x6B) {
					act->mOwner->SetAnim(0x68, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				} else if (anim == 0x6A || anim == 0x6C) {
					act->mOwner->SetAnim(0x67, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				}
				if (act->mBPressed && act->mState == 5) {
					act->mState = 9;
				}
			}
		} else {
			if (act->mMoveDirection == 1) {
				// his hands are up at the top of the ladder
				if (act->mTarget != NULL
				    && 14.0f + GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 }).y >= act->mTarget->mFloorHeights[0]) {
					act->mState = 6;
				} else if (anim == 0x67) {
					act->mOwner->SetAnim(0x69, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				} else if (anim == 0x68) {
					act->mOwner->SetAnim(0x6A, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				}
			} else if (act->mMoveDirection == -1) {
				if (act->mOwner->GetHeight() <= 2.0f) {
					act->mState = 7;
				} else if (anim == 0x67) {
					act->mOwner->SetAnim(0x6B, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				} else if (anim == 0x68) {
					act->mOwner->SetAnim(0x6C, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				}
			}
			if (act->mBPressed && act->mState == 5) {
				act->mState = 9;
			}
		}
		break;
	case 6:
		if (!(anim == 0x6E || anim == 0x6F)) {
			if (anim == 0x69 || anim == 0x6B) {
				act->mOwner->SetAnim(0x6F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x6A || anim == 0x6C) {
				act->mOwner->SetAnim(0x6E, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x67) {
				act->mOwner->SetAnim(0x6E, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x68) {
				act->mOwner->SetAnim(0x6F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 }), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			SetObjectJointTranslationByPath(act->mOwner, 1, (s32[]) { 0 }, 0.0f, 0.0f, 0.0f);
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
			act->mTarget    = NULL;
		}
		break;
	case 7:
		if (!(anim == 0x70 || anim == 0x71)) {
			if (anim == 0x69 || anim == 0x6B) {
				act->mOwner->SetAnim(0x71, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x6A || anim == 0x6C) {
				act->mOwner->SetAnim(0x70, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x67) {
				act->mOwner->SetAnim(0x70, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x68) {
				act->mOwner->SetAnim(0x71, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 }), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			SetObjectJointTranslationByPath(act->mOwner, 1, (s32[]) { 0 }, 0.0f, 0.0f, 0.0f);
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
			act->mTarget    = NULL;
		}
		break;
	case 8:
		if (anim == 0x69 || anim == 0x6B) {
			act->mOwner->SetAnim(0x73, TRUE, NULL, NULL, NULL, 10.0f, 1.0f, 1.0f, 10.0f);
		} else if (anim == 0x6A || anim == 0x6C) {
			act->mOwner->SetAnim(0x72, TRUE, NULL, NULL, NULL, 10.0f, 1.0f, 1.0f, 10.0f);
		} else if (anim == 0x67) {
			act->mOwner->SetAnim(0x72, TRUE, NULL, NULL, NULL, 10.0f, 1.0f, 1.0f, 10.0f);
		} else if (anim == 0x68) {
			act->mOwner->SetAnim(0x73, TRUE, NULL, NULL, NULL, 10.0f, 1.0f, 1.0f, 10.0f);
		}
		act->mState = 9;
	case 9:
		if (!(anim == 0x72 || anim == 0x73)) {
			if (anim == 0x69 || anim == 0x6B) {
				act->mOwner->SetAnim(0x73, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x6A || anim == 0x6C) {
				act->mOwner->SetAnim(0x72, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x67) {
				act->mOwner->SetAnim(0x72, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x68) {
				act->mOwner->SetAnim(0x73, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 }), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			SetObjectJointTranslationByPath(act->mOwner, 1, (s32[]) { 0 }, 0.0f, 0.0f, 0.0f);
			if (act->mOwner->GetAttachment() == 1) {
				act->mState                 = 0;
				act->mOwner->mCopter.mState = 4;
			} else {
				act->mOwner->SetAnim(0x74, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mState = 10;
			}
			act->mTarget = NULL;
		}
		break;
	case 10:
		if (act->mOwner->GetHeight() <= 0.0f) {
			if (act->mOwner->mFallDistance > 30.0f) {
				act->mState = 0;
				act->mOwner->SetAnim(0x8E, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mOwner->mJump.mState = 5;
				ApplyPlayerJumpStrength(&act->mOwner->mJump, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 0xFF, 0, 0, 0.0f);
				act->mOwner->SetColor(1, 0, 0, 0, 0.3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 0xFF, 0, 0, 0, 100, 100, 200, 0, 0.3f);
			} else {
				act->mOwner->SetAnim(0x75, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mState = 11;
			}
		}
		break;
	case 11:
		if (aobj->flags & 0x44000000) {
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	}
}

const Vec* GetScaledTargetFloorExtent(const XAction*);
s32 FindPlayerSupportingFloor(XObjChr*, s32, s32);
f32 GetObjectEdgeOutwardYaw(const XObj*, Vec*, s32);
f32 GetJointAnimationCurrentFrame(CJObj*);

/**
 * @note Address: 0x800CBDCC
 * @note Size: 0x1CE8
 */
// NONMATCHING: retail copies the owner's position a second time for the sand-floor leave branch (US 0x800CCE24 reloads
// act->mOwner->mMoveTranslate for owner where this build reuses the loads made for pos; copy-initialising owner changes
// nothing), and it calls the header inline GetObjectJointByPath, which this build expands: its estimate is over 1000 units short
void UpdatePlayerHangAction(XActHang* act, s32 anim, HSD_AObj* aobj, u32, f32 frame, f32 end)
{
	s32 state = act->mState;
	switch (state) {
	case 1: {
		static CVec contact, normal, start;
		if (anim != 0x13) {
			if (act->mTarget) {
				f32 angle = GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 1, &contact, &normal);
				if (__fpclassifyf(angle) != FP_NAN) {
					StopPlayerMotion(act->mOwner);
					act->mOwner->SetAnim(0x13, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
					act->mOwner->SetRotationY(angle, 0);
					start     = act->mOwner->mMoveTranslate;
					contact.y = start.y;
				} else {
					act->mState  = 0;
					act->mTarget = NULL;
					return;
				}
			}
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->mConnection) {
					ResetPlayerPlugState(plug, TRUE);
					PlayGameSound(&gGame.mSound, 0x37, 255);
				}
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(0x14, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState         = 5;
			act->mMoveDirection = 0;
			if (act->mTarget) {
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec owner(act->mOwner->mMoveTranslate);
					owner.y = 0;
					CVec floor(GetTargetFloorPosition(act));
					floor.y           = 0;
					act->mFloorCenter = floor;
					act->mDistance    = PSVECDistance(&owner, &floor);
				} else
					act->mOwner->SetPos(contact - normal * 4.f, 0);
			}
		} else if (act->mTarget) {
			if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
				XObjChr* chr = act->mOwner;
				f32 radius   = chr->mMoveScale.x * GetScaledTargetFloorExtent(act)->x;
				CVec owner(act->mOwner->mMoveTranslate);
				owner.y = 0;
				CVec floor(GetTargetFloorPosition(act));
				floor.y = 0;
				if (PSVECDistance(&owner, &floor) < 4.f + radius) {
					f32 yaw  = act->mOwner->mMoveRotate.y;
					f32 sine = (f32)sin(yaw);
					act->mOwner->SetTranslateX(act->mOwner->mMoveTranslate.x - .2f * sine, 0);
					f32 cosine = (f32)cos(yaw);
					act->mOwner->SetTranslateZ(act->mOwner->mMoveTranslate.z - .2f * cosine, 0);
				}
			} else {
				CVec delta = (contact - normal * 4.f) - start;
				act->mOwner->SetPos((contact - normal * 4.f) - delta * (1.f - frame / end), 0);
			}
		}
		break;
	}
	case 4:
		if (anim != 0x7a && anim != 0xca) {
			if (act->mTarget) {
				s32 index = FindPlayerSupportingFloor(act->mOwner, 1, act->mTargetFloor);
				if (index >= 0)
					act->mTargetFloor = index;
				f32 yaw;
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec owner(act->mOwner->mMoveTranslate);
					owner.y = 0;
					CVec floor(GetTargetFloorPosition(act));
					floor.y = 0;
					yaw     = PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
				} else
					yaw = PI + GetObjectEdgeOutwardYaw(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor);
				yaw                   = WrapAngle(yaw);
				act->mHangStartOffset = act->mOwner->mMoveTranslate - act->mTarget->mMoveTranslate;
				Vec pos               = act->mOwner->mMoveTranslate;
				pos.x += 1.5f * (f32)sin(PI + yaw);
				pos.z += 1.5f * (f32)cos(PI + yaw);
				act->mOwner->SetPos(pos, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
			}
			act->mOwner->SetAnim(0x7a, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			return;
		}
		if (aobj->flags & 0x44000000) {
			act->mOwner->SetPos(act->mTarget->mMoveTranslate + act->mHangStartOffset, 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			act->mState               = 0;
			act->mTarget              = NULL;
			act->mOwner->mJump.mState = 3;
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->mConnection) {
					ResetPlayerPlugState(plug, TRUE);
					PlayGameSound(&gGame.mSound, 0x37, 255);
				}
			}
			return;
		}
		if (!act->mStickAway)
			return;
		{
			f32 last = GetJointAnimationEndFrame(act->mOwner);
			if (GetJointAnimationCurrentFrame(act->mOwner) / last <= .6f) {
				XObjChr* chr = act->mOwner;
				chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				chr->mIdleLoops = 3;
				act->mState     = 0;
				act->mTarget    = NULL;
				return;
			}
		}
		act->mOwner->SetPos(act->mTarget->mMoveTranslate + act->mHangStartOffset, 0);
		RecordObjectPositionAndHeightChange(act->mOwner);
		if (act->mOwner->GetAttachment() == 2 || act->mOwner->mLift.mState) {
			act->mOwner->mJump.mState = 2;
			act->mState               = 0;
			act->mTarget              = NULL;
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->mConnection) {
					ResetPlayerPlugState(plug, TRUE);
					PlayGameSound(&gGame.mSound, 0x37, 255);
				}
			}
		} else if (HasPlayerActionTarget(act) && GetPlayerActionTarget(act)->mStatus.mFlags & 0x800000000ULL) {
			act->mOwner->mLadder.SetState(3, GetPlayerActionTarget(act));
			act->mState  = 0;
			act->mTarget = NULL;
		} else if (HasPlayerActionTarget(act) && GetPlayerActionTarget(act)->mStatus.mFlags & 0x1000000000ULL) {
			act->mOwner->mRope.SetState(3, GetPlayerActionTarget(act));
			act->mState  = 0;
			act->mTarget = NULL;
			return;
		} else {
			act->mState = 3;
			return;
		}
		// The jump and ladder transitions also run the stage plug check below.
		// fall through
	case 3:
		if (anim != 0x76) {
			if (act->mTarget) {
				s32 index = FindPlayerSupportingFloor(act->mOwner, 1, act->mTargetFloor);
				if (index >= 0)
					act->mTargetFloor = index;
				CVec contact(0, 0, 0);
				f32 yaw;
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec owner(act->mOwner->mMoveTranslate);
					owner.y = 0;
					CVec floor(GetTargetFloorPosition(act));
					floor.y = 0;
					yaw     = PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
				} else {
					yaw = PI + GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 0, &contact, NULL);
					if (__fpclassifyf(yaw) == FP_NAN) {
						act->mOwner->mJump.mState = 2;
						act->mState               = 0;
						act->mTarget              = NULL;
						return;
					}
				}
				yaw = WrapAngle(yaw);
				act->mOwner->SetRotationY(yaw, 0);
				act->mOwner->SetAnim(0x13, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mOwner->SetAnim(0x13, TRUE, NULL, NULL, NULL, GetJointAnimationEndFrame(act->mOwner), 1.0f, 1.0f, 10.0f);
				act->mOwner->UpdateAnim(0);
				s32 p1[]   = { 0 };
				CVec local = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
				s32 p2[]   = { 0 };
				CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
				delta.y    = -delta.y;
				act->mOwner->SetPos(act->mOwner->mMoveTranslate + delta, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
				act->mOwner->SetAnim(0x76, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				local.x  = -local.x;
				local.z  = -local.z;
				s32 p3[] = { 0 };
				SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, local);
				Vec pos = act->mOwner->mMoveTranslate;
				f32 distance;
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec owner(act->mOwner->mMoveTranslate);
					owner.y = 0;
					CVec floor(GetTargetFloorPosition(act));
					floor.y    = 0;
					yaw        = PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
					XObjChr* p = act->mOwner;
					f32 span   = PSVECDistance(&owner, &floor);
					distance   = 1.f + (span - p->mMoveScale.x * GetScaledTargetFloorExtent(act)->x);
				} else {
					pos.x    = contact.x;
					pos.z    = contact.z;
					distance = 1;
				}
				yaw = WrapAngle(yaw);
				pos.x += distance * (f32)sin(PI + yaw);
				pos.z += distance * (f32)cos(PI + yaw);
				Vec offset = act->mTargetOffset;
				Mtx rotate;
				PSMTXRotRad(rotate, 'y', yaw);
				PSMTXMultVec(rotate, &offset, &offset);
				pos.x += offset.x;
				pos.z += offset.z;
				act->mOwner->SetPos(pos, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
			}
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->mConnection) {
					ResetPlayerPlugState(plug, TRUE);
					PlayGameSound(&gGame.mSound, 0x37, 255);
				}
			}
		} else if (aobj->flags & 0x44000000) {
			s32 childPath[] = { 0 };
			HSD_JObj* child = GetObjectJointByPath(act->mOwner, 1, childPath);
			f32 turn        = HSD_JObjGetRotationY(child);
			act->mOwner->SetRotationY(HSD_JObjGetRotationY(act->mOwner->mJObj) + turn, 0);
			HSD_JObjSetRotationY__FP9_HSD_JObjf(child, 0);
			act->mOwner->SetAnim(0x13, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mOwner->SetAnim(0x13, TRUE, NULL, NULL, NULL, GetJointAnimationEndFrame(act->mOwner), 1.0f, 1.0f, 10.0f);
			act->mOwner->UpdateAnim(0);
			s32 p1[]   = { 0 };
			CVec local = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
			s32 p2[]   = { 0 };
			CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
			s32 p3[]   = { 0 };
			SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, local);
			act->mOwner->SetAnim(0x14, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState         = 5;
			act->mMoveDirection = 0;
			if (act->mTarget && act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
				CVec owner(act->mOwner->mMoveTranslate);
				owner.y = 0;
				CVec floor(GetTargetFloorPosition(act));
				floor.y           = 0;
				act->mFloorCenter = floor;
				act->mDistance    = PSVECDistance(&owner, &floor);
			}
		}
		break;
	case 2:
		if (anim != 0x18) {
			act->mOwner->SetAnim(0x18, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			if (act->mTarget)
				act->mOwner->SetRotationY(GetObjectEdgeOutwardYaw(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor), 0);
		} else if (aobj->flags & 0x44000000) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 reset[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, reset, 0, 0, 0);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y, 0);
			s32 p[] = { 0 };
			SetJointYawByPath(act->mOwner, 1, p, 0);
			act->mOwner->SetAnim(0x14, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mOwner->UpdateAnim(0);
			s32 child[] = { 0 };
			act->mOwner->SetPos(
			    act->mOwner->mMoveTranslate + (act->mOwner->mMoveTranslate - GetEventJointWorldPosition(act->mOwner, 1, child)), 1);
			RecordObjectPositionAndHeightChange(act->mOwner);
			act->mState         = 5;
			act->mMoveDirection = 0;
		}
		break;
	case 5:
		if (act->mMoveDirection == -1) {
			if (anim != 0x16)
				act->mOwner->SetAnim(0x16, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		} else if (act->mMoveDirection == 1) {
			if (anim != 0x15)
				act->mOwner->SetAnim(0x15, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		} else if (anim != 0x14)
			act->mOwner->SetAnim(0x14, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		break;
	// climbing up, TryLeaveHang picks the other climb (7) one time in eight
	case 6:
	case 7: {
		int next = state == 7 ? 0x7d : 0x17;
		if (anim != next)
			act->mOwner->SetAnim(next, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		else if (aobj->flags & 0x44000000) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 reset[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, reset, 0, 0, 0);
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
			act->mTarget    = NULL;
		}
		break;
	}
	case 8:
		act->mOwner->SetAnim(0x19, TRUE, NULL, NULL, NULL, 15.0f, 1.0f, 1.0f, 10.0f);
		act->mState = 9;
	// fall through
	case 9:
		if (anim != 0x19) {
			act->mOwner->SetAnim(0x19, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			return;
		}
		if (!(aobj->flags & 0x44000000))
			return;
		act->mOwner->SetAnim(0x1a, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		act->mState  = 10;
		act->mTarget = NULL;
	// fall through
	case 10:
		if (act->mOwner->GetHeight() <= 0) {
			if (act->mOwner->mFallDistance > 30) {
				act->mState = 0;
				act->mOwner->SetAnim(0x8e, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mOwner->mJump.mState = 5;
				ApplyPlayerJumpStrength(&act->mOwner->mJump, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 255, 0, 0, 0);
				act->mOwner->SetColor(1, 0, 0, 0, .3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, .3f);
			} else {
				act->mOwner->SetAnim(0x1b, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mState = 11;
			}
		} else if (act->mOwner->GetAttachment() == 1) {
			act->mState                 = 0;
			act->mOwner->mCopter.mState = 8;
		}
		break;
	case 11:
		if (aobj->flags & 0x44000000) {
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	}
}

/**
 * @note Address: 0x800C9B74
 * @note Size: 0xC0
 */
void HSD_JObjSetRotationY__FP9_HSD_JObjf(HSD_JObj* jobj, f32 angle)
{
	HSD_JObjSetRotationY(jobj, angle);
}

/**
 * @note Address: 0x800C9C34
 * @note Size: 0x3C
 */
void RecordObjectPositionAndHeightChange(XObj* obj)
{
	if (obj->mMoveTranslate.y != obj->mLastPos.y) {
		obj->mStatus.mFlags |= 0x0010000000000000ULL;
	}
	obj->mLastPos = obj->mMoveTranslate;
}

/**
 * @note Address: 0x800C9C70
 * @note Size: 0xE4
 */
void HandleLadderActionInput(XActLadder* act, const CPadOne* pad, f32* weight)
{
	act->mBPressed = FALSE;
	if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && (pad->mTrigger & PAD_BUTTON_B)) {
			act->mBPressed = TRUE;
		}
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

extern Vec lbl_8037C334;

static inline Vec GetFloorOffset(XObjFloor* const* floors, s32 index)
{
	Vec value = lbl_8037C334;
	value.x   = (*floors)[index].mCenterOffset[0] / 100.0f;
	value.y   = (*floors)[index].mCenterOffset[1] / 100.0f;
	value.z   = (*floors)[index].mCenterOffset[2] / 100.0f;
	return value;
}

static inline void JumpAnim(XActJump* act, s32 id, f32 loop = 1)
{
	act->mOwner->SetAnim(id, FALSE, NULL, NULL, NULL, 0, 1, loop, 10);
}
static inline void EndJump(XActJump* act)
{
	XObjChr* owner = act->mOwner;
	owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
	owner->mIdleLoops = 3;
	act->mState       = 0;
}
static inline BOOL AdvanceJumpArc(XActJump* act, u32 frames)
{
	if (act->mArcAngle < HALF_PI) {
		if ((act->mArcAngle += act->mArcStep * frames) > HALF_PI)
			act->mArcAngle = HALF_PI;
		f32 sine = std::sinf(act->mArcAngle);
		f32 lift = act->mRise * sine;
		act->mOwner->SetTranslateY(act->mArcPosition.y + lift, 0);
		f32 angle = HSD_JObjGetRotationY(act->mOwner->mJObj);
		f32 dx    = act->mForward * (act->mArcStep * std::sinf(angle));
		f32 dz    = act->mForward * (act->mArcStep * std::cosf(angle));
		act->mOwner->SetTranslateX(act->mArcPosition.x += dx, 0);
		act->mOwner->SetTranslateZ(act->mArcPosition.z += dz, 0);
		return FALSE;
	}
	return TRUE;
}

/**
 * @note Address: 0x800CA0B4
 * @note Size: 0x1D00
 */
// NONMATCHING: retail keeps the cleared playing flag in r4 and loads its other zeros where they are stored; here that zero sits
// in r0 from the switch on and the state resets use it. Saved registers: retail has the frame count in r27 below the interrupt
// flag, the game pointer and next (r28 to r30), here it is in r30 above them. playing declared before next is
// flat, a bool playing drops to 98.2
void UpdatePlayerJumpAction(XActJump* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	BOOL interrupt = FALSE;
	if (!gGame.mEvt.mData.mScript) {
		CPad pad(0);
		if (StickLength(pad.mPad) > 0) {
			switch (act->mState) {
			case 19:
				if (frame > 1)
					interrupt = TRUE;
				break;
			case 4:
			case 18:
				if (frame > 3)
					interrupt = TRUE;
				break;
			case 10:
			case 13:
			case 16:
				act->mOwner->SetAnimRate(1.f + 2.f * ((frame / end) * StickLength(pad.mPad)), 0x5a0);
				break;
			}
		}
	}
	s32 next     = -1;
	BOOL playing = FALSE;
	switch (act->mState) {
	case 1:
		if (anim != 0x1c) {
			JumpAnim(act, 0x1c, 0);
			return;
		}
		if (!(aobj->flags & 0x44000000))
			return;
		act->mState = 2;
		// fall through
	case 2:
	case 3:
	case 17:
		if (act->mOwner->GetHeight() <= 0 || (act->mOwner->GetHeight() <= 1.f && GetPlayerFloorSlope(act->mOwner).Length() > .5f)) {
			if (act->mOwner->mLift.mState == 19) {
				act->mState = 0;
				return;
			}
			if (act->mOwner->mFallDistance > 30) {
				JumpAnim(act, 0x8e);
				act->mState = 5;
				ApplyPlayerJumpStrength(act, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 255, 0, 0, 0);
				act->mOwner->SetColor(1, 0, 0, 0, .3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, .3f);
			} else if (act->mOwner->mFallDistance > 11.5f) {
				JumpAnim(act, 0x1e);
				act->mState = 4;
				if (act->mOwner->mFallDistance > 12.5f)
					gGame.mParticle.Create(0x10006, 0, act->mOwner->mJObj, FALSE);
			} else if (act->mOwner->mFallDistance > 7.5f) {
				JumpAnim(act, 0x1f);
				act->mState = 18;
			} else {
				JumpAnim(act, 0x20);
				act->mState = 19;
			}
		} else if (act->mOwner->mLift.mState != 19) {
			if (act->mState == 3) {
				next    = 0x7c;
				playing = anim == 0x7c;
			} else {
				next    = 0x1d;
				playing = anim == 0x1d || anim == 0x93;
			}
			if (!playing)
				JumpAnim(act, next, 0);
		}
		break;
	case 4:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if ((aobj->flags & 0x44000000) || interrupt)
			EndJump(act);
		break;
	case 5:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if (aobj->flags & 0x44000000) {
			JumpAnim(act, 0x8f);
			act->mState = 6;
			gGame.mParticle.Create(0x10015, 0, act->mOwner->mJObj, TRUE);
			PlayGameSound(&gGame.mSound, 0x1a, 255);
			PlayGameSound(&gGame.mSound, 0x36, 255);
		}
		break;
	case 6:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if ((act->mStrength -= frames) <= 0) {
			s32 id;
			if (act->mTargetStrength >= 50)
				id = 0x9a;
			else {
				id = 0x90;
				if (act->mTargetStrength >= 45 && !gGame.mEvt.mData.mScript) {
					// a hard landing makes him put the mug (item 22) away
					XObjChr* chr = act->mOwner;
					if (chr->mLift.mState && IsCarrying(&chr->mLift, 22))
						PutAway(&chr->mLift);
				}
			}
			JumpAnim(act, id);
			act->mState = 7;
			gGame.mParticle.Kill(0x15);
			StopGameSfx(&gGame.mSound, 0x36);
		} else if (!IsGameSoundPlaying(&gGame.mSound, 0x36))
			PlayGameSound(&gGame.mSound, 0x36, 255);
		break;
	case 7:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if (aobj->flags & 0x44000000)
			EndJump(act);
		break;
	case 8:
		if (next < 0) {
			next    = 0x5a;
			playing = anim == next || anim == 0x10f || anim == 0x115;
		}
	// fall through
	case 11:
		if (next < 0) {
			next    = 0x60;
			playing = anim == next || anim == 0x112 || anim == 0x118;
		}
	// fall through
	case 14:
		if (next < 0) {
			next    = 0x63;
			playing = anim == next || anim == 0x11b || anim == 0x11e;
		}
		if (!playing) {
			act->mTargetHeight = act->mTarget->mSelectedFloorHeight;
			const Vec* pos     = &act->mOwner->mMoveTranslate;
			if (pos->y >= act->mTargetHeight) {
				act->mState = 0;
				return;
			}
			if (act->mTarget) {
				f32 yaw = GetFloorFacingAngle(act->mTarget, pos, act->mTargetFloor, 0, NULL, NULL);
				if (__fpclassifyf(yaw) != FP_NAN) {
					StopPlayerMotion(act->mOwner);
					JumpAnim(act, next);
					act->mOwner->SetRotationY(yaw, 0);
				} else {
					act->mState  = 0;
					act->mTarget = NULL;
				}
			}
		} else if (aobj->flags & 0x44000000) {
			s32 id = -1;
			switch (act->mState) {
			case 8:
				if (id < 0)
					id = 0x5b;
				break;
			case 11:
				if (id < 0)
					id = 0x61;
				break;
			case 14:
				if (id < 0)
					id = 0x64;
				break;
			}
			JumpAnim(act, id);
			++act->mState;
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->mConnection) {
					plug->mConnection = 0;
					s32 path[]        = { 0 };
					HSD_JObjSetRotationX(GetJointByPath(plug->mJObj, path, 1), 0);
					PlayGameSound(&gGame.mSound, 0x37, 255);
				}
			}
			if (act->mTarget) {
				act->mArcPosition = act->mOwner->mMoveTranslate;
				act->mRise        = act->mTargetHeight - act->mOwner->mMoveTranslate.y;
				act->mArcAngle    = 0;
				if (act->mRise > 6)
					act->mArcStep = .8f * (HALF_PI / act->mRise);
				else
					act->mArcStep = DEG2RAD * 12;
				act->mForward = 5;
				{
					f32 radius = act->mOwner->mMoveScale.x * GetFloorExtent(&act->mOwner->mFloors, 0).x;
					f32 height = .5f
					           * (act->mOwner->mMoveScale.x
					              * (GetFloorOffset(&act->mOwner->mFloors, 0).y + GetFloorExtent(&act->mOwner->mFloors, 0).y));
					Mtx rot;
					PSMTXRotRad(rot, 'y', act->mOwner->mMoveRotate.y);
					Vec offset = act->mOwner->mMoveScale * GetFloorOffset(&act->mOwner->mFloors, 0);
					PSMTXMultVec(rot, &offset, &offset);
					f32 yaw = act->mOwner->mMoveRotate.y;
					f32 x   = 3.f * std::sinf(yaw) + (offset.x + act->mOwner->mMoveTranslate.x);
					f32 z   = 3.f * std::cosf(yaw) + (offset.z + act->mOwner->mMoveTranslate.z);
					f32 y   = .5f + act->mTargetHeight;
					for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
						XObj* obj = gGame.GetObject(i);
						if (!(obj->mStatus.mFlags & 1) || !(obj->mStatus.mFlags & 0x10000) || obj->mStatus.mFlags & 0x100000)
							continue;
						if (obj->GetCategory() == 7 || obj->GetCategory() == 20 || (s32)act->mOwner->mId == (s32)obj->mId
						    || obj->GetKind() >= 10000)
							continue;
						f32 dx = obj->mMoveTranslate.x - x, dz = obj->mMoveTranslate.z - z;
						if (sqrtf(dx * dx + dz * dz) > 100.f + (radius + obj->mFloorXZExtent))
							continue;
						CVec push, contact;
						u32 touch = ProbeObjectFloor(obj, CVec(x, y, z), CVec(x, y, z), radius, height, &push, &contact, NULL) & 3;
						if (touch == 3) {
							act->mForward = 0;
							act->mRise    = .8f * act->mRise;
							break;
						}
					}
				}
				AdvanceJumpArc(act, frames);
			}
			if (!act->mTarget || act->mRise <= 0) {
				EndJump(act);
				act->mTarget = NULL;
			}
		}
		break;
	case 9:
		if (next < 0) {
			next    = 0x5b;
			playing = anim == next || anim == 0x110 || anim == 0x116;
		}
	// fall through
	case 12:
		if (next < 0) {
			next    = 0x61;
			playing = anim == next || anim == 0x113 || anim == 0x119;
		}
	// fall through
	case 15:
		if (next < 0) {
			next    = 0x64;
			playing = anim == next || anim == 0x11c || anim == 0x11f;
		}
		if (!playing)
			JumpAnim(act, next);
		if (AdvanceJumpArc(act, frames)) {
			if (act->mForward == 0) {
				JumpAnim(act, 0x1d, 0);
				act->mState = 17;
				return;
			}
			s32 id = -1;
			switch (act->mState) {
			case 9:
				if (id < 0)
					id = 0x5c;
			// fall through
			case 12:
				if (id < 0)
					id = 0x62;
			// fall through
			case 15:
				if (id < 0)
					id = 0x65;
			}
			JumpAnim(act, id);
			++act->mState;
		}
		break;
	case 10:
		if (next < 0) {
			next    = 0x5c;
			playing = anim == next || anim == 0x111 || anim == 0x117;
		}
	// fall through
	case 13:
		if (next < 0) {
			next    = 0x62;
			playing = anim == next || anim == 0x114 || anim == 0x11a;
		}
	// fall through
	case 16:
		if (next < 0) {
			next    = 0x65;
			playing = anim == next || anim == 0x11d || anim == 0x120;
		}
		if (!playing) {
			JumpAnim(act, next);
			return;
		}
		if (aobj->flags & 0x44000000) {
			EndJump(act);
			act->mTarget = NULL;
		} else if (interrupt)
			act->mOwner->SetAnimRate(4, 0x5a0);
		break;
	case 18:
		if ((aobj->flags & 0x44000000) || interrupt)
			EndJump(act);
		break;
	case 19:
		if ((aobj->flags & 0x44000000) || interrupt)
			EndJump(act);
		break;
	}
}

// the jump's fall: a long one makes him put the mug away, a longer one makes him tumble
/**
 * @note Address: 0x800C9D54
 * @note Size: 0x360
 */
void ApplyPlayerJumpStrength(XActJump* act, f32 strength)
{
	if (gGame.mEvt.mData.mScript != NULL) {
		act->mTargetStrength = 0.0f;
		act->mStrength       = 0.0f;
	} else {
		act->mTargetStrength = strength;
		act->mStrength       = strength;
		if (act->mTargetStrength >= 50.0f) {
			XObjChr* chr = act->mOwner;
			if (chr->mLift.mState != 0 && IsCarrying(&chr->mLift, 22)) {
				PutAway(&chr->mLift);
				if (act->mTargetStrength >= 60.0f) {
					act->mState                 = 0;
					act->mOwner->mTumble.mState = 1;
					act->mOwner->SetAnim(0x35, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				}
			}
		}
	}
}

/**
 * @note Address: 0x800CBDB4
 * @note Size: 0x18
 */
void SuppressMovementDuringJump(XActJump* act, const CPadOne* pad, f32* weight)
{
	if (act->mState != 0) {
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x800CE120
 * @note Size: 0x214
 */
void HandleHangActionInput(XActHang* act, const CPadOne* pad, f32* weight)
{
	act->mStickAway = FALSE;
	if (act->mState == 4) {
		f32 stick = -(f32)atan2(pad->mStickX, pad->mStickY);
		f32 angle = WrapAngle(stick + gGame.mMotionControlCamera.mCamera.mAngle.y - act->mOwner->mMoveRotate.y);
		// the stick points anywhere but within 30 degrees of where he faces
		if (!(angle < 30 * DEG2RAD) && !(angle > 330 * DEG2RAD)) {
			act->mStickAway = TRUE;
		}
		*weight = 0.0f;
	} else if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			// a climbs up if there is room, b lets go (9)
			if (pad->mTrigger & PAD_BUTTON_A) {
				if (act->mTarget == NULL || (act->mTarget->mStatus.mFlags & 0x400000000ULL)) {
					TryLeaveHang(act);
					act->mMoveDirection = 0;
				}
				*weight = 0.0f;
			} else if (pad->mTrigger & PAD_BUTTON_B) {
				act->mState         = 9;
				act->mMoveDirection = 0;
				*weight             = 0.0f;
			}
		}
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x800CE334
 * @note Size: 0x20
 */
void SetLiftPlugConnection(XActLift* act, u32 value, BOOL apply)
{
	act->_54 = value;
	if (apply) {
		act->mOwner->mPlug.mConnection = value | 0x10000;
	}
}

/**
 * @note Address: 0x800CE354
 * @note Size: 0x5B0
 */
BOOL CheckCarriedObjectContacts(XActLift* act, const Vec* pos, f32 radius, f32 height, BOOL checkItem, BOOL checkEvent, BOOL checkOthers,
                                Vec* result)
{
	BOOL found = FALSE;
	s32 item   = act->mTarget->GetItem();
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.GetObject(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000))
			continue;
		f32 dx = object->mMoveTranslate.x - pos->x;
		f32 dz = object->mMoveTranslate.z - pos->z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (radius + object->mFloorXZExtent))
			continue;
		CVec hit, contact;
		if (checkItem && (object->mStatus.mFlags & 0x40000000000ULL)) {
			u32 touch = ProbeObjectFloor(object, *pos, *pos, radius, height, &hit, &contact, NULL);
			if ((touch & 3) == 3 && object->OnItemUsed(pos, item)) {
				found = TRUE;
				if (result)
					*result = contact;
			}
		}
		if (checkEvent && (object->mStatus.mFlags & 0x80000000000ULL)) {
			u32 touch = ProbeObjectFloor(object, *pos, *pos, radius, height, &hit, &contact, NULL);
			if ((touch & 3) == 3) {
				object->OnSquirterHit(&contact, act->mOwner->mSquirterContent, act->mOwner->mItemTintWord);
				found = TRUE;
				if (result)
					*result = contact;
			}
		} else if (checkOthers) {
			if (object->mStatus.mFlags & 0x2000000000000ULL)
				continue;
			if (object->mStatus.mFlags & 0x100000)
				continue;
			if (object == act->mTarget)
				continue;
			if (object == act->mOwner)
				continue;
			u32 touch = ProbeObjectFloor(object, *pos, *pos, radius, height, &hit, &contact, NULL);
			if ((touch & 3) == 3) {
				object->OnSquirterHit(&contact, act->mOwner->mSquirterContent, act->mOwner->mItemTintWord);
				found = TRUE;
				if (result)
					*result = contact;
			}
		}
	}
	return found;
}

void UpdateCarriedObjectPosition(XActLift*, XObj*);
void HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(HSD_JObj*, MtxPtr);
void HSD_JObjClearMtxFlags__FP9_HSD_JObj(HSD_JObj*);
CVec GetStagePlacementScale(XObj*);
void GetJointScale(CJObj*, HSD_JObj*, Vec*);
f32 GetFirstFloorCollisionRadius(const XObj*);

/**
 * @note Address: 0x800CECFC
 * @note Size: 0x2040
 */
// NONMATCHING: the call set is retail's (inline limit 64: GetObjectJointByPath, ~CJObj and HSD_JObjCopyMtx are calls, the four
// HSD_JObjGetMtxPtr sites are expanded, US 0x800CF174, 0x800CF470, 0x800D0780, 0x800D0AC0). The two squirter SetAnim calls keep the
// item count in r0 and sign-extend it after loading the object (mr r0, r3; mr r3, r30; extsh r0, r0)
void UpdateHeldObject(XActLift* act, s32)
{
	if (!act->mTargetState)
		return;
	XObj* obj = act->mTarget;
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		f32 frame = GetJointAnimationCurrentFrame(act->mOwner), end = GetJointAnimationEndFrame(act->mOwner);
		switch (act->mTargetState) {
		case 1:
			obj->mStatus.mFlags |= 0x0400000000000000ULL;
			act->mLiftStartOffset = obj->mMoveTranslate - GetPlayerCarryJointWorldPosition(act->mOwner);
			act->mYawOffset       = obj->mMoveRotate.y - act->mOwner->mMoveRotate.y;
			obj->SetPos(GetPlayerCarryJointWorldPosition(act->mOwner) - act->mLiftStartOffset * (1.f - frame / end), 0);
			if (obj->GetCategory() == 24) {
				act->mYawOffset = 0;
				obj->SetRotateY(act->mYawOffset + act->mOwner->mMoveRotate.y, DEG2RAD * 6);
				obj->SetRotationX(0, 0);
				ResetPlayerPlugState((XObjPlug*)obj, TRUE);
			}
			if (obj->GetCategory() == 8 && obj->GetItem() == 22) {
				act->mYawOffset = 0;
				obj->SetRotateY(act->mYawOffset + act->mOwner->mMoveRotate.y, DEG2RAD * 6);
			}
			act->mTargetState = 2;
			// fall through
		case 2: {
			CVec offset(GetLiftTargetContactOffset(act, obj));
			f32 progress = 2.f * (frame / end);
			if (progress > 1)
				progress = 1;
			obj->SetPos((GetPlayerCarryJointWorldPosition(act->mOwner) + act->mLiftStartOffset * (1.f - progress)) - offset * progress, 0);
			break;
		}
		case 3:
			if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
				HSD_JObj* root = obj->mJObj;
				s32 path[]     = { 0, 0 };
				HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(root, HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)));
				if (IsCarryingSpoon(act))
					obj->SetAnim(4, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
				else if (IsCarryingSquirter(act))
					obj->SetAnim(sSquirterChargeLevels[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)] + 5, TRUE, NULL, NULL,
					             NULL, 0, 1, 0, 10);
			} else {
				UpdateCarriedObjectPosition(act, obj);
				{
					Vec scale;
					s32 path[] = { 0, 0 };
					GetJointScale(act->mOwner, GetObjectJointByPath(act->mOwner, 2, path), &scale);
					obj->SetScale(GetStagePlacementScale(obj) * scale, 0);
				}
				act->mYawOffset = 0;
				obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
				if (IsCarryingMug(act))
					obj->SetAnim(2, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			}
			break;
		case 4: {
			if (obj->mStatus.mFlags & 0x800000) {
				obj->mStatus.mFlags &= ~0x800000ULL;
				obj->SetDestinationAlpha(FALSE, 0);
			}
			if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
				HSD_JObj* root = obj->mJObj;
				s32 path[]     = { 0, 0 };
				HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(root, HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)));
				// one of the three is carried, so id is always set
				s32 id;
				switch (act->mOwner->mAnimId) {
				case 0x895:
					id = 1;
					break;
				case 0x8b7:
					id = 5;
					break;
				case 0x8ae:
					id = 2;
					break;
				case 0x8af:
					id = 3;
					break;
				case 0x8c5:
					id = 14;
					break;
				case 0x8c0:
					id = 9;
					break;
				case 0x8d2:
					id = 17;
					break;
				case 0x8d3:
					id = 18;
					break;
				case 0x8c1:
					id = 10;
					break;
				case 0x8c2:
					id = 11;
					break;
				case 0x8c3:
					id = 12;
					break;
				case 0x8c4:
					id = 13;
					break;
				case 0xe0:
					if (IsCarryingToothbrush(act))
						id = 0;
					else if (IsCarryingSpoon(act))
						id = 1;
					else if (IsCarryingSquirter(act))
						id = sSquirterChargeLevels[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)] + 1;
					break;
				case 0xca:
					if (IsCarryingToothbrush(act))
						id = 3;
					else if (IsCarryingSpoon(act))
						id = 4;
					else if (IsCarryingSquirter(act))
						id = sSquirterChargeLevels[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)] + 5;
					break;
				default:
					if (IsCarryingToothbrush(act))
						id = 0;
					else if (IsCarryingSpoon(act))
						id = 1;
					else if (IsCarryingSquirter(act))
						id = sSquirterChargeLevels[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)] + 1;
					break;
				}
				obj->SetAnim(id, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				break;
			}
			obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
			UpdateCarriedObjectPosition(act, obj);
			if (IsTargetPool(act, 24)) {
				s32 handPath[] = { 0, 0 };
				CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
				s32 path[] = { 0 };
				CJObj plug(GetObjectJointByPath(obj, 1, path));
				plug.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
				plug.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
			} else if (IsCarryingMug(act)) {
				s32 id;
				if (act->mState == 31) {
					id = 4;
				} else {
					switch (act->mOwner->mAnimId) {
					case 12:
						id = 8;
						break;
					case 0x88:
						id = 9;
						break;
					case 13:
						id = 10;
						break;
					default:
						id = 1;
					}
				}
				obj->SetAnim(id, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			}
			break;
		}
		case 5: {
			UpdateCarriedObjectPosition(act, obj);
			if (!(GetJointAnimationCurrentFrame(act->mOwner) >= 24))
				break;
			obj->SetTranslateY(act->mOwner->mMoveTranslate.y, 0);
			f32 radius = GetFirstFloorCollisionRadius(act->mOwner);
			f32 height = act->mOwner->GetCollisionHeight();
			CVec push, contact;
			if (obj->TestFloorCollision(&act->mOwner->mMoveTranslate, &act->mOwner->mMoveTranslate, radius, height, &push, &contact, 0, 0)
			    & 1) {
				f32 distance = PSVECDistance(&act->mOwner->mMoveTranslate, &contact);
				f32 s        = sin(act->mOwner->mMoveRotate.y);
				obj->SetTranslateX(distance * s + obj->mMoveTranslate.x, 0);
				f32 c = cos(act->mOwner->mMoveRotate.y);
				obj->SetTranslateZ(distance * c + obj->mMoveTranslate.z, 0);
			}
			if (obj->GetCategory() == 24)
				PlayGameSound(&gGame.mSound, 0x39, 255);
			else if (obj->GetCategory() != 8)
				PlayGameSound(&gGame.mSound, GetMaterialSoundId(obj->mFloors->mType >> 8), 255);
			obj->mStatus.mFlags &= ~0x0400000000000000ULL;
			act->mTarget = NULL;
			break;
		}
		case 6:
			UpdateCarriedObjectPosition(act, obj);
			if (obj->GetCategory() == 24) {
				s32 handPath[] = { 0, 0 };
				CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
				s32 endPath[] = { 0 };
				CJObj end(GetObjectJointByPath(obj, 1, endPath));
				end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
				HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
				if (anim->flags & 0x44000000)
					act->mTarget = NULL;
				else if (!((XObjPlug*)obj)->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 90) {
					((XObjPlug*)obj)->mConnection = act->_54 | 0x10000;
					obj->mStatus.mFlags &= ~0x0400000000000000ULL;
				} else if (!((XObjPlug*)obj)->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 34 && act->_54 == 0x200)
					end.SetRotationZ(-40.f * DEG2RAD, 0);
			}
			break;
		case 7:
			UpdateCarriedObjectPosition(act, obj);
			if (obj->GetCategory() == 24) {
				s32 handPath[] = { 0, 0 };
				CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
				s32 endPath[] = { 0 };
				CJObj end(GetObjectJointByPath(obj, 1, endPath));
				end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
				HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
				if (anim->flags & 0x44000000)
					act->mTargetState = 4;
				else if ((anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 80)
					end.SetRotationZ(0, 0);
				else if (((XObjPlug*)obj)->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 76) {
					ResetPlayerPlugState((XObjPlug*)obj, FALSE);
					obj->mStatus.mFlags |= 0x0400000000000000ULL;
				}
			}
			break;
		case 8:
			UpdateCarriedObjectPosition(act, obj);
			if (obj->GetCategory() == 24) {
				s32 handPath[] = { 0, 0 };
				CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
				s32 endPath[] = { 0 };
				CJObj end(GetObjectJointByPath(obj, 1, endPath));
				end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
				HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
				if (anim->flags & 0x44000000)
					act->mTargetState = 9;
				else if (!((XObjPlug*)obj)->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 24)
					((XObjPlug*)obj)->mConnection = -1;
			}
			break;
		case 10:
			UpdateCarriedObjectPosition(act, obj);
			if (obj->GetCategory() == 24) {
				s32 handPath[] = { 0, 0 };
				CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
				s32 endPath[] = { 0 };
				CJObj end(GetObjectJointByPath(obj, 1, endPath));
				end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
				HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
				if (anim->flags & 0x44000000)
					act->mTargetState = 4;
				else if (((XObjPlug*)obj)->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 0)
					ResetPlayerPlugState((XObjPlug*)obj, FALSE);
			}
			break;
		case 11:
			UpdateCarriedObjectPosition(act, obj);
			if (obj->GetCategory() == 24) {
				s32 handPath[] = { 0, 0 };
				CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
				s32 endPath[] = { 0 };
				CJObj end(GetObjectJointByPath(obj, 1, endPath));
				end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
				end.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
				HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
				if (anim->flags & 0x44000000)
					act->mTargetState = 12;
				else if (!((XObjPlug*)obj)->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 24)
					((XObjPlug*)obj)->mConnection = 0x10400;
			}
			break;
		case 12:
			UpdateCarriedObjectPosition(act, obj);
			if (obj->GetCategory() == 24) {
				s32 handPath[] = { 0, 0 };
				CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
				s32 endPath[] = { 0 };
				CJObj end(GetObjectJointByPath(obj, 1, endPath));
				end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
				end.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
			}
			break;
		case 13:
			UpdateCarriedObjectPosition(act, obj);
			if (obj->GetCategory() == 24) {
				s32 handPath[] = { 0, 0 };
				CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
				s32 endPath[] = { 0 };
				CJObj end(GetObjectJointByPath(obj, 1, endPath));
				end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
				end.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
				HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
				if (anim->flags & 0x44000000)
					act->mTargetState = 4;
				else if (((XObjPlug*)obj)->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 0)
					ResetPlayerPlugState((XObjPlug*)obj, FALSE);
			}
			break;
		case 14:
			if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
				HSD_JObj* root = obj->mJObj;
				s32 path[]     = { 0, 0 };
				HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(root, HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)));
				if (IsCarryingSpoon(act))
					obj->SetAnim(4, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
				else if (IsCarryingSquirter(act))
					obj->SetAnim(sSquirterChargeLevels[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)] + 5, TRUE, NULL, NULL,
					             NULL, 0, 1, 0, 10);
			} else {
				UpdateCarriedObjectPosition(act, obj);
				{
					Vec scale;
					s32 path[] = { 0, 0 };
					GetJointScale(act->mOwner, GetObjectJointByPath(act->mOwner, 2, path), &scale);
					obj->SetScale(GetStagePlacementScale(obj) * scale, 0);
				}

				if (IsCarryingMug(act))
					obj->SetAnim(3, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			}
			break;
		case 15:
			if (obj->GetCategory() == 8) {
				SetOrAddInventoryItemCount(&gGame.GetPlayer()->mParams.mInventory, obj->GetItem(), 1, TRUE);
				((XObjItem*)obj)->SetFlag();
			}
			obj->Release();
			act->mTarget = NULL;
			break;
		case 16:
		case 18: {
			CVec offset(GetLiftTargetContactOffset(act, obj));
			Mtx trans, world;
			PSMTXTrans(trans, offset.x, -offset.y, offset.z);
			s32 path[] = { 0, 0 };
			PSMTXConcat(HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)), trans, world);
			HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(obj->mJObj, world);
			Vec v;
			HSD_MtxGetTranslate(world, &v);
			obj->SetPos(v, 0);
			HSD_MtxGetScale(world, &v);
			obj->SetScale(v, 0);
			break;
		}
		case 17:
			if (obj->GetCategory() == 8) {
				((XObjItem*)obj)->SetFlag();
			}
			obj->Release();
			act->mTarget = NULL;
			break;
		case 19:
			HSD_JObjClearMtxFlags__FP9_HSD_JObj(obj->mJObj);
			act->mYawOffset = 0;
			obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
			UpdateCarriedObjectPosition(act, obj);
			{
				Vec scale;
				s32 path[] = { 0, 0 };
				GetJointScale(act->mOwner, GetObjectJointByPath(act->mOwner, 2, path), &scale);
				obj->SetScale(GetStagePlacementScale(obj) * scale, 0);
			}
			act->mTargetState = 4;
			break;
		case 20:
			if (GetAObj(obj->mJObj)->flags & 0x44000000) {
				obj->Release();
				act->mTarget = NULL;
			}
			break;
		}
	}
}

// puts the carried object where he holds it
/**
 * @note Address: 0x800CE904
 * @note Size: 0xA4
 */
void UpdateCarriedObjectPosition(XActLift* act, XObj* target)
{
	target->SetPos(GetPlayerCarryJointWorldPosition(act->mOwner) - GetLiftTargetContactOffset(act, target), 0);
}

/**
 * @note Address: 0x800CE9A8
 * @note Size: 0x354
 */
Vec GetLiftTargetContactOffset(const XActLift* act, const XObj* target)
{
	// a plug (24) is held by its end joint
	if (target->GetCategory() == 24) {
		s32 path[] = { 0 };
		CJObj joint(GetJointByPath(target->mJObj, path, 1));
		CVec pos(joint.GetPos());
		return pos - target->mMoveTranslate;
	}
	Vec offset = target->mMoveTranslate * -1.0f;
	return GetFloorContactPosition(target, act->mTargetFloor, &offset);
}

/**
 * @note Address: 0x800D0D3C
 * @note Size: 0x90
 */
void HSD_JObjClearMtxFlags__FP9_HSD_JObj(HSD_JObj* jobj)
{
	HSD_JObjClearMtxFlags(jobj);
}

// Both retail call sites read the result before the next call.
extern "C" const Vec* GetScaledTargetFloorExtent(const XAction* act)
{
	// the address of the product's temporary, as retail returns it (MWCC warns: not an lvalue)
	return &(GetPlayerActionTarget(act)->mMoveScale * GetFloorExtent(&GetPlayerActionTarget(act)->mFloors, GetActionTargetFloor(act)));
}

/**
 * @note Address: 0x800D0DCC
 * @note Size: 0xC0
 */
extern "C" f32 GetFirstFloorCollisionRadius(const XObj* object)
{
	return object->mMoveScale.x * GetFloorExtent(&object->mFloors, 0).x;
}

/**
 * @note Address: 0x800D0E8C
 * @note Size: 0xC0
 */
void HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(HSD_JObj* jobj, MtxPtr mtx)
{
	HSD_JObjCopyMtx(jobj, mtx);
}

/**
 * @note Address: 0x800D3A5C
 * @note Size: 0x94
 */
HSD_Generator* CreateParticleAtPosition(CGame* game, u32 bank, u32 id, s32 status, const Vec& pos, BOOL keep)
{
	return game->mParticle.CreateSRT(id | (bank << 16), status, pos, (Vec) { 1, 1, 1 }, (Vec) { 0, 0, 0 }, keep);
}

/**
 * @note Address: 0x800D3AF0
 * @note Size: 0xB4
 */
Vec GetJointMatrixScale(void* unused, HSD_JObj* jobj)
{
	Vec scale;
	HSD_MtxGetScale(HSD_JObjGetMtxPtr(jobj), &scale);
	return scale;
}

/**
 * @note Address: 0x800D3BA4
 * @note Size: 0x5C
 */
HSD_Generator* CreateScaledParticleAtPosition(CGame* game, u32 bank, u32 id, s32 status, const Vec& pos, const Vec& scale, BOOL keep)
{
	Vec rot = { 0.0f, 0.0f, 0.0f };
	return (&game->mParticle)->CreateSRT(id | (bank << 16), status, pos, scale, rot, keep);
}

/**
 * @note Address: 0x800D3C00
 * @note Size: 0xB8
 */
Vec GetObjectJointMatrixScale(const CJObj* obj)
{
	HSD_JObj* jobj = obj->mJObj;
	Vec scale;
	HSD_MtxGetScale(HSD_JObjGetMtxPtr(jobj), &scale);
	return scale;
}

/**
 * @note Address: 0x800D3CB8
 * @note Size: 0x24
 */
s32 RemovePlayerInventoryItemCount(Params* params, s16 id, s32 amount)
{
	return RemoveInventoryItemCount(&params->mInventory, id, amount);
}

/**
 * @note Address: 0x800D3CDC
 * @note Size: 0x30
 */
BOOL IsObjectLoaded(XObj* obj)
{
	return !(obj->mStatus.mFlags & 0x0040000000000000ULL);
}

/**
 * @note Address: 0x800D3D0C
 * @note Size: 0x64
 */
XObj* SpawnObjectFromPlacementIndex(CGame* game, s16 id)
{
	CoordInfo info(GetStageObjectPlacementByIndex(id));
	return SpawnObjectFromPlacement(&game->mXObjs, &info, 0, TRUE);
}

/**
 * @note Address: 0x800D3D70
 * @note Size: 0x44
 */
void SetClampedPlayerBattery(Params* params, f32 battery)
{
	params->SetBattery(battery);
}

/**
 * @note Address: 0x800D3DB4
 * @note Size: 0x20
 */
void EnableSubwindowWithFlags(CGame* game, u32 flags)
{
	game->mSubwin.mEnabled      = 1;
	game->mSubwin.mFlags.mFlags = 0;
	game->mSubwin.mFlags.mFlags |= flags;
}

/**
 * @note Address: 0x800D3DD4
 * @note Size: 0x24
 */
void SetSubwindowCamera(CGame* game, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov)
{
	ConfigureSubwindowCamera(&game->mSubwin, interest, angleY, angleX, distance, fov);
}

/**
 * @note Address: 0x800D3DF8
 * @note Size: 0xB8
 */
Vec GetObjectJointWorldPosition(const CJObj* obj)
{
	HSD_JObj* jobj = obj->mJObj;
	Vec pos;
	HSD_MtxGetTranslate(HSD_JObjGetMtxPtr(jobj), &pos);
	return pos;
}

/**
 * @note Address: 0x800D3EB0
 * @note Size: 0x14
 */
void SetSubwindowRect(CGame* game, f32 x, f32 y, f32 width, f32 height)
{
	game->mSubwin.mRectX      = x;
	game->mSubwin.mRectY      = y;
	game->mSubwin.mRectWidth  = width;
	game->mSubwin.mRectHeight = height;
}

/**
 * @note Address: 0x800D3EC4
 * @note Size: 0x50
 */
void ClearObjectAnimationQueue(XObj* obj)
{
	obj->mAnmQueue.Clear();
}

/**
 * @note Address: 0x800D3F14
 * @note Size: 0x1D8
 */
CXObj::CXObj(HSD_JObj* jobj)
{
	CJObj::Attach(jobj);
	SyncMoveFromJoint(this);
}

// picks the joint's srt up into the move values
/**
 * @note Address: 0x800D40EC
 * @note Size: 0x164
 */
void SyncMoveFromJoint(CXObj* obj)
{
	union {
		Vec vec;
		Quaternion quat;
	} value;
	HSD_JObjGetTranslation(obj->mJObj, &value.vec);
	obj->mMoveTranslate.Reset(value.vec);
	HSD_JObjGetRotation(obj->mJObj, (Quaternion*)&value.vec);
	obj->mMoveRotate.Reset(value.vec);
	HSD_JObjGetScale(obj->mJObj, &value.vec);
	obj->mMoveScale.Reset(value.vec);
}

/**
 * @note Address: 0x800D4250
 * @note Size: 0x44
 */
f32 GetObjectJointRotationY(CJObj* obj)
{
	return HSD_JObjGetRotationY(obj->mJObj);
}

/**
 * @note Address: 0x800D4294
 * @note Size: 0x60
 */
void SetPlayerStandAnim(XObjChr* chr, s32 anim, f32 blend)
{
	chr->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, blend, 10.0f);
	chr->mIdleLoops = 3;
}

/**
 * @note Address: 0x800D42F4
 * @note Size: 0x3C
 */
HSD_Generator* CreateParticleByBankAndId(CGame* game, s32 bank, s32 id, u8 type, HSD_JObj* jobj, BOOL keep)
{
	return (&game->mParticle)->Create(id | (bank << 16), type, jobj, keep);
}

/**
 * @note Address: 0x800D4330
 * @note Size: 0x10
 */
s16 GetInventoryCountByIndex(Params* params, u32 index)
{
	return params->mInventory.mItems[index].mCount;
}

/**
 * @note Address: 0x800D4340
 * @note Size: 0x28
 */
s16 GetObjectItemId(XObj* obj)
{
	return obj->GetItem();
}

/**
 * @note Address: 0x800D4368
 * @note Size: 0x48C
 */
void HandleLiftActionInput(XActLift* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 4) {
		XObjChr* chr = act->mOwner;
		if (chr->mJump.mState != 0) {
			return;
		}
		if (chr->mHang.mState != 0) {
			return;
		}
		if (!gGame.IsRunning()) {
			return;
		}
		if (gGame.mStatus.mFlags & 0x100) {
			return;
		}
		if (gGame.mEvt.mData.mScript != NULL) {
			return;
		}
		if (chr->mAnimId == 0x895 || chr->mAnimId == 0x8AE || chr->mAnimId == 0x8AF) {
			*weight = 0.0f;
			return;
		}
		// a sets it down (6), b puts an item away (19) and sets anything else down
		if (pad->mTrigger & PAD_BUTTON_A) {
			if (act->mTarget == NULL) {
				return;
			}
			if (act->mTarget->GetCategory() == 8) {
				return;
			}
			if (chr->mFloorContacts[4].objectIndex >= 0 && (gGame.GetObject(chr->mFloorContacts[4].objectIndex)->mStatus.mFlags & 0x20)) {
				*weight = 0.0f;
				return;
			}
			if (act->mTarget->GetCategory() == 0x18) {
				return;
			}
			act->mState = 6;
			*weight     = 0.0f;
		} else if (pad->mTrigger & PAD_BUTTON_B) {
			if (act->mTarget != NULL) {
				if (act->mTarget->GetCategory() == 8) {
					if (chr->GetAttachment() == 0) {
						act->mState = 19;
					}
				} else {
					act->mState = 6;
				}
			}
			*weight = 0.0f;
		}
	} else if (act->mState == 12 || act->mState == 13) {
		// a or b stops giving power and pulls the plug (14)
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && ((pad->mTrigger & PAD_BUTTON_A) || (pad->mTrigger & PAD_BUTTON_B))) {
			act->mState = 14;
			act->mTimer = 0.0f;
		}
		*weight = 0.0f;
	} else if (act->mState == 15) {
		if (gGame.IsRunning()) {
			if (pad->mStickX > 0.8f) {
				act->mState = 16;
				act->mTimer = 0.0f;
			} else if (pad->mTrigger & PAD_BUTTON_B) {
				act->mState = 17;
				act->mTimer = 0.0f;
			}
		}
		*weight = 0.0f;
	} else if (act->mState == 27) {
		// the first frames of a toothbrush stroke leave the stick alone
	} else if (act->mState == 28) {
	} else if (act->mState == 31 || act->mState == 32) {
		// he uses the mug for as long as a is held
		if (!(pad->mButton & PAD_BUTTON_A)) {
			act->mState = 33;
		}
		*weight = 0.0f;
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

// the item particles' color callback, they take his item tint
/**
 * @note Address: 0x800D47F4
 * @note Size: 0x38
 */
int ApplyPlayerItemParticleColor(HSD_Particle* particle)
{
	u32 color           = gGame.GetPlayer()->mItemTintWord;
	particle->primCol.r = color >> 24;
	particle->primCol.g = color >> 16;
	particle->primCol.b = color >> 8;
	return 0;
}

/**
 * @note Address: 0x800D482C
 * @note Size: 0x16C
 */
void ApplyPlayerItemTintToTarget(XAction* act)
{
	s32 path[]     = { 0, 1 };
	HSD_JObj* jobj = FindJointByPath(act->mTarget->mJObj, path, 2);
	for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
		HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
		if (mobj != NULL) {
			mobj->mat->diffuse = act->mOwner->mItemTint;
			for (HSD_TObj* tobj = HSD_MObjGetTObj(mobj); tobj != NULL; tobj = HSD_TObjGetNext(tobj)) {
				u32 flags   = tobj->flags & ~TEX_COLORMAP_MASK;
				tobj->flags = flags;
				tobj->flags = flags | TEX_COLORMAP_PASS;
			}
			HSD_MObjCompileTev(mobj);
		}
	}
}

/**
 * @note Address: 0x800D4998
 * @note Size: 0x1C8
 */
// NONMATCHING: identical instruction stream; retail binds act to r30 and the
// late item load to r31, ours binds act to r31 and item to r30 - a callee-save
// colouring swap. The same inlined PutAway body in ApplyPlayerJumpStrength
// binds act to r30 on both sides (100%), so the choice is context-dependent;
// named-local forms (item hoisted, item used for the target accesses, no chr
// local) all scored worse or identical.
void PutAwayLiftedObject(XActLift* act)
{
	PutAway(act);
}

/**
 * @note Address: 0x800D4B60
 * @note Size: 0x118
 */
BOOL IsUsingMug(XActLift* act)
{
	return IsCarryingStill(act, 0x16);
}

/**
 * @note Address: 0x800D4C78
 * @note Size: 0xDC
 */
BOOL IsCarryingSquirter(XActLift* act)
{
	return IsCarrying(act, 0x38);
}

/**
 * @note Address: 0x800D4D54
 * @note Size: 0xDC
 */
BOOL IsCarryingMug(XActLift* act)
{
	return IsCarrying(act, 0x16);
}

/**
 * @note Address: 0x800D4E30
 * @note Size: 0xDC
 */
BOOL IsCarryingSpoon(XActLift* act)
{
	return IsCarrying(act, 0x15);
}

/**
 * @note Address: 0x800D4F0C
 * @note Size: 0xDC
 */
BOOL IsCarryingToothbrush(XActLift* act)
{
	return IsCarrying(act, 0x10);
}

/**
 * @note Address: 0x800D4FE8
 * @note Size: 0x4C
 */
BOOL IsLiftTargetPlug(XActLift* act)
{
	return IsTargetPool(act, 0x18);
}

/**
 * @note Address: 0x800D5034
 * @note Size: 0xA0
 */
void ResetPlayerLiftAction(XActLift* act)
{
	if (act->mTarget != NULL) {
		act->mTarget->mStatus.mFlags &= ~0x10000ULL;
		act->mTarget->mStatus.mFlags &= ~0x0400000000000000ULL;
	}
	act->Reset();
	act->mContactEffect = NULL;
	act->mUseEffect     = NULL;
	act->mToolEffect    = NULL;
}

/**
 * @note Address: 0x800D50D4
 * @note Size: 0xBC
 */
void InitializePlayerLiftAction(XActLift* act, XObjChr* owner)
{
	act->Init(owner);
	act->mTimer         = 0.0f;
	act->mPendingItemId = -1;
	ResetPlayerLiftAction(act);
}

/**
 * @note Address: 0x800D5190
 * @note Size: 0x39C
 */
Vec GetTargetFloorPosition(XAction* act)
{
	if (act->mTarget != NULL) {
		CVec position = act->mTarget->mMoveTranslate;
		CVec offset
		    = GetPlayerActionTarget(act)->mMoveScale * GetFloorOffset(&GetPlayerActionTarget(act)->mFloors, GetActionTargetFloor(act));
		CVec extent
		    = GetPlayerActionTarget(act)->mMoveScale * GetFloorExtent(&GetPlayerActionTarget(act)->mFloors, GetActionTargetFloor(act));
		f32 yaw = act->mTarget->mMoveRotate.y + GetFloorYawRadians(&GetPlayerActionTarget(act)->mFloors, GetActionTargetFloor(act));
		Mtx translate, local, rotate, scale, result;
		PSMTXTrans(translate, position.x, position.y, position.z);
		PSMTXTrans(local, offset.x, offset.y, offset.z);
		PSMTXScale(scale, extent.x, extent.y, extent.z);
		PSMTXRotRad(rotate, 'y', yaw);
		PSMTXConcat(translate, rotate, result);
		PSMTXConcat(result, local, result);
		PSMTXConcat(result, scale, result);
		Vec point = { 0.0f, 0.0f, 0.0f };
		PSMTXMultVec(result, &point, &point);
		return point;
	}
	Vec zero = { 0.0f, 0.0f, 0.0f };
	return zero;
}

/**
 * @note Address: 0x800CDBA8
 * @note Size: 0x578
 */
static inline u32 CheckHangClearance(XObj* obj, const CVec& from, const CVec& to, f32 radius, f32 height, Vec& push, Vec& contact)
{
	return obj->TestFloorCollision(&from, &to, radius, height, &push, &contact, 0, 0);
}

void TryLeaveHang(XActHang* act)
{
	f32 radius = act->mOwner->mMoveScale.x * GetFloorExtent(&act->mOwner->mFloors, 0).x;
	f32 halfHeight
	    = 0.5f * (act->mOwner->mMoveScale.x * (GetFloorOffset(&act->mOwner->mFloors, 0).y + GetFloorExtent(&act->mOwner->mFloors, 0).y));
	Vec position = GetEventJointWorldPosition(act->mOwner, 1, (s32[]) { 0 });
	position.y += 0.5f;

	u32 blockingHit = 0;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; i++) {
		XObj* obj = gGame.GetObject(i);
		if (!(obj->mStatus.mFlags & 1) || !(obj->mStatus.mFlags & 0x10000) || (obj->mStatus.mFlags & 0x100000))
			continue;
		if (obj->GetCategory() == 7 || obj->GetCategory() == 20 || (s32)act->mOwner->mId == (s32)obj->mId || obj->GetKind() >= 10000)
			continue;
		f32 dx = obj->mMoveTranslate.x - position.x;
		f32 dz = obj->mMoveTranslate.z - position.z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (radius + obj->mFloorXZExtent))
			continue;

		CVec push, contact;
		u32 result = CheckHangClearance(obj, CVec(position), CVec(position), radius, halfHeight, push, contact);
		if ((result & 3) == 3) {
			blockingHit = result;
			break;
		}
	}
	if (blockingHit)
		return;
	if ((rand() & 7) == 0)
		act->mState = 7;
	else
		act->mState = 6;
}

} // extern "C"

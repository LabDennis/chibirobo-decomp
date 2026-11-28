#include "game/xchr.h"
#include "game/aimpitch.h"
#include <stdlib.h>
#include "game/video.h"
#include "game/cgame.h"
#include "game/cvec.h"
#include "game/objectfloorquery.h"
#include "game/xobjflags.h"
#include "game/stagedata.h"
#include "game/cjobjset.h"
#include "game/globals.h"
#include "game/hsd_dobj.h"
#include "Dolphin/pad.h"
#include "game/xact.h"
#include "game/gmath.h"
#include <math.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/aobj.h>
#include "game/cjobjinlineattach.h"
#include "game/cparticle.h"
#include "game/cscrn.h"
#include "game/csound.h"
#include "game/ccamera.h"
#include "game/xaction.h"
#include "game/xobjleticker.h"
#include "game/xobjs.h"
#include "game/cgs.h"
#include "game/fbtexeff.h"
#include <string.h>
#include <sysdolphin/mtx_transform.h>
#include <sysdolphin/particle.h>

// Chibi-Robo's radar action

/**
 * @note Address: 0x800BD320
 * @note Size: 0x4
 */
extern "C" void DrawPlayerRadarEffects(XActRadar*)
{
}

extern "C" CVec GetRadarDishWorldPosition(XObjRadar*);

enum {
	RadarBeamTurns        = 8,
	RadarBeamRingVertices = 16,
	RadarBeamVertexCount  = 2 * RadarBeamTurns * RadarBeamRingVertices,
};

static inline u32 PackRadarColor(const ScrnColorF& c)
{
	return ((u32)c.r << 24) | ((u32)c.g << 16) | ((u32)c.b << 8) | (u32)c.a;
}

static void ShadeRadarBeam(u32* colors, u32 base, f32 fade)
{
	for (int turn = 0; turn < RadarBeamTurns; ++turn) {
		for (int step = 0; step < RadarBeamRingVertices; ++step) {
			const int vertex    = 2 * (turn * RadarBeamRingVertices + step);
			const f32 intensity = turn < 2 ? (1.0f / 64) * vertex : 1.0f - (1.0f / 192) * ((turn - 2) * 32 + step);
			colors[vertex]      = base | (u8)(s32)(150.0f * intensity * fade);
			colors[vertex + 1]  = base;
		}
	}
}

/**
 * @note Address: 0x800BD324
 * @note Size: 0x1324
 */
extern "C" void DrawRadarTargetBeam(XActRadar* act)
{
	if (!(act->mFade > 0))
		return;
	CVec origin = GetRadarDishWorldPosition(&act->mOwner->mRadarObj);
	Vec offset  = { 0, 0, 0 };
	offset.y    = act->mTarget->GetCollisionHeight() * .5f;
	CVec target = act->mTarget->mMoveTranslate + offset;
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

	Vec vertices[RadarBeamVertexCount];
	u32 colors[RadarBeamVertexCount];
	int vertex          = 0;
	f32 z               = 2.0f;
	f32 phase           = act->mAngle[0];
	const f32 angleStep = TWO_PI / RadarBeamRingVertices;
	f32 ringSines[RadarBeamRingVertices];
	f32 ringCosines[RadarBeamRingVertices];
	for (int step = 0; step < RadarBeamRingVertices; ++step) {
		const f32 angle   = angleStep * step;
		ringSines[step]   = sin(angle);
		ringCosines[step] = cos(angle);
	}
	for (int turn = 0; turn < RadarBeamTurns; ++turn) {
		// Round the ring angle before adding the animated twist.
		const f32 ringAngle = angleStep * turn;
		const f32 twist     = static_cast<f64>(ringAngle) + act->mAngle[1];
		const f32 ringScale = 0.1f * (f32)sin(twist) + 1.0f;
		for (int step = 0; step < RadarBeamRingVertices; ++step) {
			const f32 sine   = ringSines[step];
			const f32 cosine = ringCosines[step];
			const f32 radius = 0.007f * vertex + 1.0f;
			Vec& outer       = vertices[vertex];
			outer.x          = radius * (ringScale * sine);
			outer.y          = radius * (cosine * ringScale);
			outer.z          = z;

			f32 wave = WrapAngle(phase);
			if (wave > PI)
				wave = 0.0f;
			const f32 height     = sin(wave);
			const f32 halfHeight = 0.5f * height;
			Vec& inner           = vertices[vertex + 1];
			// The paired offsets round once on each coordinate.
			inner.x = __fnmsubs(sine, halfHeight, outer.x);
			inner.y = __fnmsubs(sine, halfHeight, outer.y);
			inner.z = outer.z - 1.5f * height;
			vertex += 2;
			phase += DEG2RAD * 30;
			z += 0.1f * span;
		}
	}

	PSMTXMultVecArray(model, vertices, vertices, RadarBeamVertexCount);
	const u32 back  = PackRadarColor(act->mBeam.mColor[0].mColor);
	const u32 front = PackRadarColor(act->mBeam.mColor[1].mColor);
	CGs& gs         = gGame.mGs;
	SetGsCullMode(gs.mCurRenderMode, GX_CULL_BACK);
	ShadeRadarBeam(colors, back, act->mFade);
	DrawGsColoredTrianglePrimitive(&gs, GX_TRIANGLESTRIP, gs.mCamera->mViewMtx, vertices, colors, RadarBeamVertexCount);
	SetGsCullMode(gs.mCurRenderMode, GX_CULL_FRONT);
	ShadeRadarBeam(colors, front, act->mFade);
	DrawGsColoredTrianglePrimitive(&gs, GX_TRIANGLESTRIP, gs.mCamera->mViewMtx, vertices, colors, RadarBeamVertexCount);
	SetGsCullMode(gs.mCurRenderMode, GX_CULL_BACK);
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

/**
 * @note Address: 0x800BE930
 * @note Size: 0x12D4
 */
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
		if (act->mYaw.reached == 0) {
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
		if (act->mPitch.reached == 0) {
			StepRadarPitch(act, frames, 3 * DEG2RAD, false);
			if (act->mPitch.reached != 0) {
				if (act->mTarget) {
					CVec from               = GetRadarDishWorldPosition(&act->mOwner->mRadarObj);
					Vec offset              = { 0, 0, 0 };
					offset.y                = act->mTarget->GetCollisionHeight() * .5f;
					CVec to                 = act->mTarget->mMoveTranslate + offset;
					act->mPitch.targetAngle = TargetPitch(from, to);
				} else
					act->mPitch.targetAngle = 0;
			}
			SetRadarDishPitch(&act->mOwner->mRadarObj, act->mPitch.angle);
		}
		if (act->mYaw.reached != 0 && act->mPitch.reached != 0) {
			act->mState         = 5;
			act->mYaw.reached   = 0;
			act->mPitch.reached = 0;
		}
		break;
	case 5:
		if (act->mYaw.reached == 0) {
			act->mYaw.angle += (TWO_PI / FRAME_RATE) * frames;
			if (act->mYaw.angle >= act->mYaw.targetAngle) {
				act->mYaw.angle   = act->mYaw.targetAngle;
				act->mYaw.reached = 1;
			}
			SetRadarDishYaw(&act->mOwner->mRadarObj, act->mYaw.angle);
		}
		if (act->mPitch.reached == 0) {
			StepRadarPitch(act, frames, DEG2RAD, true);
			SetRadarDishPitch(&act->mOwner->mRadarObj, act->mPitch.angle);
		}
		if (act->mYaw.reached != 0 && act->mPitch.reached != 0) {
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
				Vec offset      = { 0, 0, 0 };
				offset.y        = act->mTarget->GetCollisionHeight() * .5f;
				CVec to         = act->mTarget->mMoveTranslate + offset;
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
		f32 radius           = act->mTarget->GetFloorXZExtent();
		act->mTargetDistance = dist - (act->mOwner->GetFloorXZExtent() + radius);
		if (act->mTargetDistance < 0)
			act->mTargetDistance = 0;
		if (act->mState == 0) {
			XSound& sound = gGame.mSound;
			// near, middle or far: each has its own tone and turns the beam red, yellow or blue
			if (act->mTargetDistance < (s32)GetRadarParameter(0) / 100.f) {
				StartRadarTone(act, 0x18e, 255, 150, 150, 255, 50, 50);
				sound.StopIfPlaying(0x18d);
				sound.StopIfPlaying(0x18c);
			} else if (act->mTargetDistance < (s32)GetRadarParameter(1) / 100.f) {
				sound.StopIfPlaying(0x18e);
				StartRadarTone(act, 0x18d, 255, 255, 150, 255, 255, 50);
				sound.StopIfPlaying(0x18c);
			} else {
				sound.StopIfPlaying(0x18e);
				sound.StopIfPlaying(0x18d);
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
	CVec pos        = act->mOwner->mMoveTranslate;
	XObj* nearest   = NULL;
	f32 nearestDist = 0.0f;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* obj = gGame.GetObject(i);
		u64 flags = obj->mStatus.mFlags;
		if (!(flags & 1) || !(flags & 0x10000) || (flags & 0x4000000000000000ULL) || !(flags & 0x80000) || act->mOwner->mId == obj->mId
		    || obj->GetKind() >= 10000) {
			continue;
		}
		f32 distance = PSVECDistance(&pos, &obj->mMoveTranslate);
		if ((nearest == NULL || distance < nearestDist) && fpclassify(distance) != FP_NAN) {
			nearest     = obj;
			nearestDist = distance;
		}
	}
	return nearest;
}

/**
 * @note Address: 0x800BFC04
 * @note Size: 0x68
 */
void XActRadar::HandleInput(const CPadOne&, f32& moveWeight)
{
	// with the radar out (attachment 3) he stands still while it comes out, goes away or scans
	if (mOwner->GetAttachment() == AttachmentRadar && mState >= 1 && mState <= 5) {
		moveWeight = 0.0f;
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

} // extern "C"

extern "C" {
void ProjectWorldPositionToScreen(CGame* game, const Vec& pos, f32* x, f32* y, f32* z, CCamera* camera); // where pos is on the screen
void CheckPopperShotImpact(XActPopperShot* shot);
}

static inline void KillGen(HSD_Generator*& gen)
{
	if (gen != NULL) {
		gGame.mParticle.Kill(gen);
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

static inline void PopperMuzzle(XObjChr* p, Vec& out)
{
	// through the eye camera the shot starts a little right of, below and ahead of the eye
	if (gGame.mStatus.mFlags & 0x100) {
		CCamera& camera = gGame.mMotionControlCamera.mCamera;
		Mtx rotY, rotX, rot;
		PSMTXRotRad(rotY, 'y', camera.mAngle.y);
		PSMTXRotRad(rotX, 'x', camera.mAngle.x);
		PSMTXConcat(rotY, rotX, rot);
		Vec offset = { 1.087597f, -1.056235f, 4.978912f };
		PSMTXMultVec(rot, &offset, &offset);
		const CVec& v = camera.mEye + offset;
		out.x         = v.x;
		out.y         = v.y;
		out.z         = v.z;
		return;
	}
	s32 path[]    = { 0, 1, 0, 0 };
	const CVec& v = CJObj_GetJointWorldPositionByPath(&p->mPopperObj, 4, path);
	out.x         = v.x;
	out.y         = v.y;
	out.z         = v.z;
}

static inline int ActivePopperShots(XActPopper* act)
{
	int count = 0;
	for (int i = 0; i < 10; ++i)
		if (act->mShots.mShot[i].mActive)
			++count;
	return count;
}

static inline void StartAvailablePopperShot(XActPopper* act, const Vec& origin, s32 kind, XObj* target, f32 yaw, f32 pitch, f32 power,
                                            f32 speed, f32 range)
{
	for (int i = 0; i < 10; ++i)
		if (!act->mShots.mShot[i].mActive) {
			StartPopperShot(&act->mShots.mShot[i], origin, kind, target, yaw, pitch, power, speed, range, act->mShotStrength);
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
void UpdatePlayerPopperAction(XActPopper* act, s32 anim, HSD_AObj* aobj, u32 frames, f32, f32)
{
	BOOL rangeUpgrade        = FindOwnedInventoryItemIndex(act->mOwner->mParams.mInventory.mItems, 0x36) >= 0;
	BOOL canCharge           = FindOwnedInventoryItemIndex(act->mOwner->mParams.mInventory.mItems, 0x35) >= 0;
	XActPopperCharge& charge = act->mCharge;
	// with the popper out (attachment 2), holding A charges a shot
	if (act->mOwner->GetAttachment() == AttachmentPopper) {
		CPad pad(0);
		XObjChr* p = act->mOwner;
		if ((pad.mPad->mButton & PAD_BUTTON_A) && canCharge && p->mPopperObj.mAnimId == 4 && !gGame.mEvt.mData.mScript
		    && !(gGame.mStatus.mFlags & 0x400) && !p->mLift.mState && !p->mHang.mState && !p->mJump.mState && !p->mLadder.mState
		    && !p->mRope.mState && !p->mPush.mState && !p->mTumble.mState) {
			if (!(gGame.mStatus.mFlags & 0x20)) {
				Vec pos;
				PopperMuzzle(p, pos);
				charge.mState = 1;
				charge.mPos   = pos;
				if (!charge.mGen)
					charge.mGen = gGame.mParticle.Create(0x10051, 0, charge.mJoint, 1);
				StartPopperTone(0x186);
				if (charge.mTimer >= .5f)
					act->mState = 4;
			} else {
				act->mState = 0;
				ClearPopperCharge(&charge);
			}
		} else {
			// letting go of A: a full charge fires, a partial one is dropped
			if (act->mState == 4)
				act->mState = charge.mTimer >= 1 ? 6 : 0;
			ClearPopperCharge(&charge);
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
					if (act->mState == 6)
						act->mShotStrength = (s32)GetPlayerActionParameter(31) / 100.f;
					else
						act->mShotStrength = (s32)GetPlayerActionParameter(30) / 100.f;
				}
				CVec origin;
				PopperMuzzle(act->mOwner, origin);
				f32 yaw, pitch;
				if (gGame.mStatus.mFlags & 0x100) {
					yaw   = gGame.mMotionControlCamera.mCamera.mAngle.y;
					pitch = gGame.mMotionControlCamera.mCamera.mAngle.x;
				} else {
					yaw   = act->mOwner->mMoveRotate.y;
					pitch = 0;
				}
				f32 power = act->mState == 6 ? 3 : 1;
				// in tenths: popper parameter 2 is the speed, 0 the range and 1 the range with the upgrade
				f32 speed = (s32)GetPopperParameter(2) / 10.f, range = (s32)GetPopperParameter(rangeUpgrade) / 10.f;
				XObj* target = NULL;
				for (int i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
					XObj* obj = gGame.GetObject(i);
					u64 flags = obj->mStatus.mFlags;
					if (!(flags & 1) || !(flags & 0x10000) || !(flags & 0x100) || (flags & 0x40000) || (flags & 0x100000))
						continue;
					Vec aim   = AddScalarToVectorComponents(obj->mMoveTranslate, obj->GetCollisionHeight() * .5f);
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
					p->mParams.SetBattery(p->mParams.mBattery - act->mShotStrength);
				}
				s32 kind = GetAttachmentCountById(&act->mOwner->mAttachments, AttachmentPopper);
				if (target != NULL) {
					StartAvailablePopperShot(act, origin, kind, target, yaw, pitch, power, speed, range);
				} else {
					StartAvailablePopperShot(act, origin, kind, NULL, yaw, pitch, power, speed, range);
				}
			}
			ClearPopperCharge(&charge);
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
	XActPopperShot* shot = act->mShots.mShot;
	for (int i = 0; i < 10; i++, shot++)
		UpdatePopperShotFlight(shot, frames);
	if (charge.mState) {
		if (charge.mTimer < 1 && (charge.mTimer += (1.f / FRAME_RATE) * frames) > 1)
			charge.mTimer = 1;
		if (charge.mGen || charge.mChargeGen)
			HSD_JObjSetTranslate(charge.mJoint, &charge.mPos);
		if (charge.mTimer >= 1) {
			if (!charge.mChargeGen)
				charge.mChargeGen = gGame.mParticle.Create(0x1005c, 0, charge.mJoint, 1);
			StartPopperTone(0x18b);
		}
	} else
		charge.mTimer = 0;
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
void XActPopper::HandleInput(const CPadOne&, f32& moveWeight)
{
	if (mState >= 4 && mState <= 8) {
		moveWeight = 1.0f;
	} else if (mState != 0) {
		moveWeight = 0.0f;
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
void UpdatePopperShotFlight(XActPopperShot* shot, u32 frames)
{
	if (shot->mActive) {
		XObj* target = shot->mHomingTarget;
		if (target != NULL) {
			Vec aim = AddScalarToVectorComponents(target->mMoveTranslate, target->GetCollisionHeight() * 0.5f);
			SteerPopperAngle(shot->mYaw, (f32)atan2(aim.x - shot->mPos.x, aim.z - shot->mPos.z), frames);
			SteerPopperAngle(shot->mPitch, TargetPitch(shot->mPos, aim), frames);
		}
		shot->mDistance += shot->mSpeed * frames;
		Mtx translation, yaw, pitch, transform;
		PSMTXTrans(translation, shot->mPos.x, shot->mPos.y, shot->mPos.z);
		PSMTXRotRad(yaw, 'y', shot->mYaw);
		PSMTXRotRad(pitch, 'x', shot->mPitch);
		PSMTXConcat(translation, yaw, transform);
		PSMTXConcat(transform, pitch, transform);
		Vec forward = { 0.0f, 0.0f, shot->mSpeed * frames };
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

/**
 * @note Address: 0x800C21A0
 * @note Size: 0x874
 */
void CheckPopperShotImpact(XActPopperShot* shot)
{
	Vec pos  = shot->mPos;
	BOOL hit = FALSE;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.GetObject(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000) || (object->mStatus.mFlags & 0x40000)
		    || (object->mStatus.mFlags & 0x0002000000000000ULL) || (object->mStatus.mFlags & 0x100000))
			continue;
		f32 dx = object->mMoveTranslate.x - pos.x, dz = object->mMoveTranslate.z - pos.z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (0.8f + object->mFloorXZExtent))
			continue;
		CVec push, contact;
		if ((ProbeObjectFloor(object, pos, pos, 0.8f, 0.8f, &push, &contact, NULL) & 3) == 3) {
			hit = TRUE;
			shot->mPos.Set(contact);
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
			hit = TRUE;
			shot->mPos.Set(CVec(pos));
			shot->mPos.y = ground.mHeight;
		}
	}
	if (!hit)
		return;
	for (s32 i = 0; i < 20; ++i) {
		f32 angle       = DEG2RAD * (rand() % 360);
		f32 sinAngle    = (f32)sin(angle);
		f32 vx          = (sinAngle * (rand() % 100)) / 200.0f;
		f32 cosAngle    = (f32)cos(angle);
		f32 vz          = (cosAngle * (rand() % 100)) / 200.0f;
		f32 vy          = (50 + rand() % 80) / 100.0f;
		f32 width       = (200 + rand() % 300) / 500.0f;
		f32 height      = (10 + rand() % 200) / 500.0f;
		Vec velocity    = { vx, vy, vz };
		Vec size        = { width, height, 1.0f };
		CPtclOne* spark = SpawnPoolParticle(&shot->mDust.mPtcl, &shot->mPos, &size, &velocity, 0xC8C8FFFF);
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
	shot->mActive          = FALSE;
	shot->mRing[1].mPos    = shot->mPos;
	shot->mRing[1].mAmount = 0.9f;
	shot->mRing[1].mRadius = 100.0f;
	shot->mRing[1].mColor  = 0x6464FF96;
	KillGen(shot->mGen);
	KillGen(shot->mChargeGen);
}

// fires a shot, kind 1 is the small one
/**
 * @note Address: 0x800C2A14
 * @note Size: 0x304
 */
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
	for (s32 i = 0; i < 10; ++i) {
		shot->mHistory[i] = shot->mPos;
	}
	shot->mRing[0].mPos    = pos;
	shot->mRing[0].mAmount = 0.9f;
	shot->mRing[0].mRadius = 100.0f;
	shot->mRing[0].mColor  = 0x00FF0096;
	shot->mStrength        = strength;
	shot->mKind            = kind;
	shot->mHomingTarget    = target;
	shot->mGen             = gGame.mParticle.Create((kind == 1 ? 0x4E : 0x4F) | 0x10000, 0, shot->mJoint, TRUE);
	const bool charged     = strength >= (s32)GetPlayerActionParameter(31) / 100.0f;
	if (charged) {
		shot->mChargeGen = gGame.mParticle.Create(0x10050, 0, shot->mJoint, TRUE);
	}
	s32 sound = charged ? (kind == 1 ? 0x193 : 0x196) : (kind == 1 ? 0xB : 0x195);
	PlayGameSound(&gGame.mSound, sound, 0xFF);
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

} // extern "C"

extern "C" {
void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z);
BOOL IsPlayerStopped(const XObjChr* chr);
void UpdateCopterBobbing(XActCopter* act, u32 frames, f32 speed);
CVec GetPlayerFloorSlope(XObjChr* chr);

// flies him while the copter is out
/**
 * @note Address: 0x800C2EB8
 * @note Size: 0x738
 */
void UpdatePlayerCopterFlight(XActCopter* act, s32 anim, HSD_AObj* aobj, u32 frames, f32, f32)
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
	} else if (act->mBob > 0.0f) {
		act->mBob -= 0.2f * frames;
		if (act->mBob <= 0.0f)
			act->mBob = 0.0f;
	}
	s32 path[] = { 0 };
	SetObjectJointTranslationByPath(act->mOwner, ARRAY_SIZE(path), path, 0.0f, act->mBob, 0.0f);
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
void XActCopter::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	switch (mState) {
	case 0:
		break;
	case 3:
		// Pressing B or running out of battery ends the flight.
		if ((pad.mTrigger & PAD_BUTTON_B) || IsPlayerStopped(mOwner)) {
			mState = 4;
			PlayCopterDescentAnimation(&mOwner->mCopterObj);
		}
		// Fall through to keep control during descent.
	case 4:
	case 7:
	case 8:
		moveWeight = 1.0f;
		break;
	default:
		moveWeight = 0.0f;
		break;
	}
}

} // extern "C"

extern "C" {

// the ring on the floor while he tumbles
/**
 * @note Address: 0x800C36E4
 * @note Size: 0x2F0
 */
void DrawTumbleRing(XActTumbleFade* fade, const Vec& pos, f32 angle)
{
	if (fade->mAmount > 0.0f) {
		CVec offset(fade->mOffset);
		Mtx m;
		PSMTXRotRad(m, 'y', angle);
		PSMTXMultVec(m, &offset, &offset);
		Vec center = { pos.x + offset.x, 0.01f + pos.y, pos.z + offset.z };
		f32 size   = 6.0f * fade->mAmount;
		u32 color  = (u32)(160.0f * fade->mAmount) | 0x641E00;
		DrawGsCircle(&gGame.mGs, 0x18, color, color, center.x, center.y, center.z, size, size, -HALF_PI, 0.0f, 0.0f);
		PrimVertex verts[50];
		f32 outerRadius = size * 0.5f + 1.0f;
		for (s32 i = 0; i <= 24; ++i) {
			f32 ringAngle     = (TWO_PI / 24) * i;
			f32 sine          = (f32)sin(ringAngle);
			f32 cosine        = (f32)cos(ringAngle);
			PrimVertex& inner = verts[i * 2];
			PrimVertex& outer = verts[i * 2 + 1];
			inner.pos.x       = center.x - size * sine * 0.5f;
			inner.pos.z       = center.z + size * cosine * 0.5f;
			inner.pos.y       = center.y;
			inner.color       = color;
			outer.pos.x       = center.x - outerRadius * sine;
			outer.pos.z       = center.z + outerRadius * cosine;
			outer.pos.y       = center.y;
			outer.color       = 0;
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

/**
 * @note Address: 0x800C3A80
 * @note Size: 0x9E8
 */
void UpdatePlayerTumbleAnimation(XActTumble* act, s32 anim, HSD_AObj* aobj, u32, f32, f32)
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
				if (gGame.mEvt.mData.mScript == NULL)
					StartOrQueueEventScript(&gGame.mEvt, GetStageBatteryDepletedScript(), 0, -1, -1);
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
		Vec offset = { 0.0f, 12.0f, 0.0f }, rotation = { 0.0f, 0.0f, 0.0f };
		gGame.mParticle.CreateSRT(0x10005, 1, act->mOwner->mMoveTranslate + offset, act->mOwner->mMoveScale, rotation, FALSE);
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
			if (!act->mIdleParticle && (act->mOwner->mMoveTranslate - (Vec)act->mOwner->mLastPos).Length() == 0.0f) {
				s32 effect = 0x28;
				if (rand() & 1)
					effect = 0x26;
				s32 height = act->mOwner->mLift.mState == 0 ? 2 : 4;
				s32 path[] = { 0, 1, 0, 1, 2, 0, 1 };
				CVec pos   = GetEventJointWorldPosition(act->mOwner, 7, path);
				Vec offset = { 0.0f, 0.0f, 0.0f }, rotation = { 0.0f, 0.0f, 0.0f };
				offset.y           = height;
				act->mIdleParticle = gGame.mParticle.CreateSRT(effect | 0x10000, 1, pos + offset, act->mOwner->mMoveScale, rotation, FALSE);
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
void XActTumble::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	if (mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if ((pad.mTrigger & PAD_BUTTON_A) || (pad.mTrigger & PAD_BUTTON_B)
			    || sqrtf(pad.mStickX * pad.mStickX + pad.mStickY * pad.mStickY) > 0.0f) {
				mState = 3;
			}
		}
		moveWeight = 0.0f;
	} else if (mState == 16) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if ((pad.mTrigger & PAD_BUTTON_A) || (pad.mTrigger & PAD_BUTTON_B)
			    || sqrtf(pad.mStickX * pad.mStickX + pad.mStickY * pad.mStickY) > 0.0f) {
				mState = 17;
			}
		}
		moveWeight = 0.0f;
	} else if (mState == 5) {
		// Recovering from an empty battery takes a button press.
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && ((pad.mTrigger & PAD_BUTTON_A) || (pad.mTrigger & PAD_BUTTON_B))) {
			mState = 6;
		}
		moveWeight = 0.0f;
	} else if (mState == 7) {
		moveWeight = 0.8f;
	} else if (mState != 0) {
		moveWeight = 0.0f;
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
/**
 * @note Address: 0x800C494C
 * @note Size: 0x924
 */
void UpdatePushedObject(XActPush* act)
{
	if (!act->mTargetState)
		return;
	XObj* target = act->mTarget;
	if (!target || !(target->mStatus.mFlags & 1))
		return;
	switch (act->mTargetState) {
	case 1: {
		target->mStatus.mFlags |= 0x0800000000000000ULL;
		Vec hand = GetPlayerCarryJointWorldPosition(act->mOwner);
		act->mTargetOffset.Set(target->mMoveTranslate - hand);
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
		if ((target->mStatus.mFlags & 0x4000000000ULL) && QueryObjectFloorHeight(target, 0.1f, 0.1f) < target->mMoveTranslate.y - 5.0f) {
			target->mStatus.mFlags &= ~0x0800000000000000ULL;
			ReleasePushAction(act);
			XObjChr* owner = act->mOwner;
			owner->SetAnim(17, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			owner->mIdleLoops = 3;
			act->mState       = 0;
			target->mStatus.mFlags |= 0x1000000000000000ULL;

			Vec rotation = target->mMoveRotate, speed = target->mMoveRotate.mSpeed;
			Vec extent = ReadObjectFloorExtent(target->mFloors[0]);
			// The cardinal cases supply an offset. An off-axis drop has no sideways offset.
			f32 dx = 0.0f, dz = 0.0f;
			if ((angle > (90 * DEG2RAD - pushAngleTolerance) && angle < (90 * DEG2RAD + pushAngleTolerance))) {
				rotation.z -= HALF_PI;
				speed.z = 0.1f;
				dz      = 0;
				dx      = extent.x * (2.0f / sqrtf(3.0f)) * 0.5f;
			} else if ((angle > (270 * DEG2RAD - pushAngleTolerance) && angle < (270 * DEG2RAD + pushAngleTolerance))) {
				rotation.z += HALF_PI;
				speed.z = 0.1f;
				dz      = 0;
				dx      = -(extent.x * (2.0f / sqrtf(3.0f))) * 0.5f;
			} else if ((angle > (0 * DEG2RAD - pushAngleTolerance) && angle < (0 * DEG2RAD + pushAngleTolerance))) {
				rotation.x += HALF_PI;
				speed.x = 0.1f;
				dx      = 0;
				dz      = extent.z * (2.0f / sqrtf(3.0f)) * 0.5f;
			} else if ((angle > (180 * DEG2RAD - pushAngleTolerance) && angle < (180 * DEG2RAD + pushAngleTolerance))) {
				rotation.x -= HALF_PI;
				speed.x = 0.1f;
				dx      = 0;
				dz      = -(extent.z * (2.0f / sqrtf(3.0f))) * 0.5f;
			}
			target->mMoveRotate.mTarget.Set(rotation);
			target->mMoveRotate.mSpeed.Set(speed);
			target->mToppleOffset = CVec(dx, -(extent.y * (2.0f / sqrtf(3.0f))) * 0.5f, dz);
		}
		break;
	}
	}
}

} // extern "C"

extern "C" {
}

extern "C" {
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
					ReleasePushAction(act);
					act->mState = 0;
				}
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			act->mOwner->SetAnim(0x3F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState       = 2;
			act->mTargetState = 1;
		}
		break;
	case 2:
		if (act->mMoveDirection == 1) {
			if (anim == 0x4B) {
				if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
					act->mOwner->SetAnim(0x40, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					act->mTargetState = 2;
				}
			} else if (anim != 0x40) {
				act->mOwner->SetAnim(0x4B, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTargetState = 0;
			}
		} else if (act->mMoveDirection == -1) {
			if (anim == 0x4C) {
				if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
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
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
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
void XActPush::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	if (mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
#if defined(VERSION_JP)
			if (gGame.mStatus.mFlags & 0x0000080000000000ULL) {
				if (pad.mTrigger & PAD_BUTTON_B) {
					ReleasePushAction(this);
					mState     = 3;
					moveWeight = 0.0f;
				}
			} else
#endif
			    if (!(pad.mButton & PAD_BUTTON_A)) {
				ReleasePushAction(this);
				mState     = 3;
				moveWeight = 0.0f;
			}
		}
	} else if (mState != 0) {
		if (mState != 3 && !(pad.mButton & PAD_BUTTON_A)
#if defined(VERSION_JP)
		    && !(gGame.mStatus.mFlags & 0x0000080000000000ULL)
#endif
		) {
			ReleasePushAction(this);
			XObjChr* chr = mOwner;
			chr->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->mIdleLoops = 3;
			mState          = 0;
		}
		moveWeight = 0.0f;
	}
}

/**
 * @note Address: 0x800C591C
 * @note Size: 0xE4
 */
// NONMATCHING: retail inlines ReleasePushAction, including the target flag clear and virtual Reset; this build emits a call.
void CancelPlayerPushAction(XActPush* act)
{
	if (act->mState != 0) {
		ReleasePushAction(act);
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
	XObj* target = act->mTarget;
	if (target != NULL && (target->mStatus.mFlags & 0x0800000000000000ULL)) {
		target->mStatus.mFlags &= ~0x0800000000000000ULL;
	}
	act->Reset();
}

} // extern "C"

extern "C" {
}

extern "C" {
void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z);
CVec GetPlayerJointLocalTranslationByPath(CJObj* obj, s32 depth, s32* path);
void SetJointYawByPath(CXObj* obj, s32 depth, s32* path, f32 angle);
void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos);
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
u32 CheckStageFlag(s32 stage, u32 mask);

void PositionPlayerAroundRopeTarget(XActRope*, f32);

static inline void RopeAnim(XActRope* act, s32 id, f32 loop, BOOL force = FALSE, f32 start = 0)
{
	act->mOwner->SetAnim(id, force, NULL, NULL, NULL, start, 1, loop, 10);
}
static inline void LocateRope(XActRope* act, bool grab)
{
	CVec player(act->mOwner->mMoveTranslate);
	player.y = 0;
	CVec rope(act->mTarget->mMoveTranslate);
	rope.y = 0;
	CVec offset;
	if (grab) {
		offset.Set(act->mTargetOffset);
		offset.y = 0;
	} else
		offset.Set(0, 0, -.5f);
	Mtx matrix;
	PSMTXRotRad(matrix, 'y', grab ? act->mOwner->mMoveRotate.y : act->mTarget->mMoveRotate.y);
	PSMTXMultVec(matrix, &offset, &offset);
	if (grab) {
		PSVECSubtract(&rope, &offset, &rope);
	} else
		rope += offset;
	act->mPos   = rope;
	act->mDist  = PSVECDistance(&rope, &player);
	act->mAngle = atan2(player.x - rope.x, player.z - rope.z);
}
static inline void ApproachRope(XActRope* act, u32 frames, bool move, bool snap, f32 speed)
{
	if (!act->mTarget)
		return;
	Vec delta = act->mTarget->mMoveTranslate - act->mTarget->mLastPos;
	act->mPos.x += delta.x;
	act->mPos.z += delta.z;
	if (act->mDist > 0 && move) {
		if ((act->mDist -= speed * frames) < 0)
			act->mDist = 0;
		act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
		act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
	} else if (snap) {
		act->mOwner->SetTranslateX(act->mPos.x, 0);
		act->mOwner->SetTranslateZ(act->mPos.z, 0);
	}
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
	owner->mLastPos.Set(owner->mMoveTranslate);
}

/**
 * @note Address: 0x800C5B98
 * @note Size: 0x1C78 (US)
 */
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
				LocateRope(act, true);
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			RopeAnim(act, 0x8c, 0);
			act->mState = 5;
		} else {
			ApproachRope(act, frames, frame > 20, true, .2f);
			ReleaseRopePlug(act, frame, 28);
		}
		break;
	case 2:
		if (anim != 0x98) {
			RopeAnim(act, 0x98, 0);
			if (act->mTarget) {
				act->mOwner->SetRotationY(WrapAngle(act->mTarget->mMoveRotate.y), 0);
				LocateRope(act, false);
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
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
			ApproachRope(act, frames, frame > 80, false, .2f);
			ReleaseRopePlug(act, frame, 80);
		}
		break;
	case 3:
		if (anim != 0x85) {
			if (act->mTarget) {
				act->mOwner->SetRotationY(act->mTarget->mMoveRotate.y, 0);
				RopeAnim(act, 0x8a, 1, TRUE);
				act->mOwner->UpdateAnim(0);
				s32 p1[] = { 0 };
				// Keep the local joint lookup's missing-joint assertion.
				GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
				s32 p2[]   = { 0 };
				CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
				act->mOwner->SetTranslateY(act->mOwner->mMoveTranslate.y - delta.y, 0);
				RopeAnim(act, 0x85, 1);
				LocateRope(act, false);
			}
			ReleaseRopePlug(act, 0, 0);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
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
		} else
			ApproachRope(act, frames, true, true, .4f);
		break;
	case 4:
		RopeAnim(act, 0x8c, 1);
		act->mState = 5;
		break;
	case 5:
		if (anim == 0x8b) {
			if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
				if (act->mMoveDirection == 1)
					RopeAnim(act, 0x8d, 1);
				else
					RopeAnim(act, 0x8c, 1);
			}
		} else if (anim == 0x8d) {
			if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
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
			s32 path[] = { 0, 0 };
			if (act->mTarget && GetEventJointWorldPosition(act->mOwner, 2, path).y >= act->mTarget->mFloorHeights[0]) {
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
		else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
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
		else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			if (act->mOwner->GetAttachment() == AttachmentCopter) {
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
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			XObjChr* owner = act->mOwner;
			owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			owner->mIdleLoops = 3;
			act->mState       = 0;
		}
		break;
	case 11:
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
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
		s32 p[]         = { 0 };
		HSD_JObj* joint = GetJointByPath(act->mOwner->mJObj, p, 1);
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
		const CVec dir(act->mTargetOffset.x, 0.0f, act->mTargetOffset.z);
		f32 dist = dir.Length();
		act->mOwner->SetTranslateX(act->mTarget->mMoveTranslate.x - dist * (f32)sin(angle), 0);
		act->mOwner->SetTranslateZ(act->mTarget->mMoveTranslate.z - dist * (f32)cos(angle), 0);
	}
}

/**
 * @note Address: 0x800C7AC4
 * @note Size: 0x100
 */
void XActRope::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	if (mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			// b lets go of the rope
			if (pad.mTrigger & PAD_BUTTON_B) {
				mState = 8;
			}
			if (mOwner->mAnimId == 0x8A || mOwner->mAnimId == 0x8C) {
				moveWeight = 0.0f;
			}
		}
	} else if (mState != 0) {
		moveWeight = 0.0f;
	}
}

/**
 * @note Address: 0x800C7810
 * @note Size: 0x2B4
 */
void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos)
{
	CJObj joint(GetJointByPath(obj->mJObj, path, depth));
	joint.SetTranslate(pos.x, pos.y, pos.z, TRUE);
}

} // extern "C"

extern "C" {
}

extern "C" {
void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z);
CVec GetPlayerJointLocalTranslationByPath(CJObj* obj, s32 depth, s32* path);
void SetJointYawByPath(CXObj* obj, s32 depth, s32* path, f32 angle);
void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos);
void RecordObjectPositionAndHeightChange(XObj* obj);
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
Vec GetTargetFloorPosition(XAction* act);
u32 CheckStageFlag(s32 stage, u32 mask);

// whether his hands are up at the top of the ladder
static inline bool AtLadderTop(XActLadder* act)
{
	if (act->mTarget != NULL) {
		s32 path[]      = { 0 };
		const f32 handY = GetEventJointWorldPosition(act->mOwner, 1, path).y;
		return 14.0f + handY >= act->mTarget->mFloorHeights[0];
	}
	return false;
}

/**
 * @note Address: 0x800C7BC4
 * @note Size: 0x1FB0
 */
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
				ladPos += off;
				act->mPos.x = ladPos.x;
				act->mPos.y = ladPos.y;
				act->mPos.z = ladPos.z;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path2, 0.0f, 0.0f, 0.0f);
			act->mOwner->SetAnim(0x67, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState = 5;
		} else {
			if (act->mTarget != NULL) {
				Vec move = act->mTarget->mMoveTranslate - act->mTarget->mLastPos;
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
				ladPos += off;
				act->mPos.x = ladPos.x;
				act->mPos.y = ladPos.y;
				act->mPos.z = ladPos.z;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path2, 0.0f, 0.0f, 0.0f);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y, 0);
			s32 path3[] = { 0 };
			SetJointYawByPath(act->mOwner, 1, path3, 0.0f);
			act->mOwner->SetAnim(0x68, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState = 5;
		} else {
			if (act->mTarget != NULL) {
				Vec move = act->mTarget->mMoveTranslate - act->mTarget->mLastPos;
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
				s32 path[] = { 0 };
				CVec d     = GetEventJointWorldPosition(act->mOwner, 1, path) - act->mOwner->mMoveTranslate;
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
				ladPos += off;
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
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 path[]      = { 0 };
			HSD_JObj* joint = GetObjectJointByPath(act->mOwner, 1, path);
			f32 turn        = HSD_JObjGetRotationY(joint);
			f32 facing      = HSD_JObjGetRotationY(act->mOwner->mJObj);
			act->mOwner->SetRotationY(facing + turn, 0);
			HSD_JObjSetRotationY(joint, 0.0f);
			s32 path2[] = { 0 };
			CVec d      = GetEventJointWorldPosition(act->mOwner, 1, path2) - act->mOwner->mMoveTranslate;
			// Check the owner joint again after changing its pose.
			HSD_JObjGetRotationY(act->mOwner->mJObj);
			d.x = -d.x;
			d.z = -d.z;
			act->mOwner->SetPos((act->mOwner->mMoveTranslate + d), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path3[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path3, 0.0f, 0.0f, 0.0f);
			act->mOwner->SetAnim(0x68, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState         = 5;
			act->mMoveDirection = 0;
		} else if (act->mTarget != NULL) {
			Vec move = act->mTarget->mMoveTranslate - act->mTarget->mLastPos;
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
			if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
				s32 path[] = { 0 };
				CVec hand  = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, path);
				if (hand.y < 0.0f) {
					hand.y = (s32)(hand.y - 0.5f);
				}
				s32 path2[] = { 0 };
				SetJointTranslationFromVectorByPath(act->mOwner, 1, path2, hand);
				s32 path3[] = { 0 };
				act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path3), 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
				s32 path4[] = { 0 };
				SetObjectJointTranslationByPath(act->mOwner, 1, path4, 0.0f, 0.0f, 0.0f);
				if (act->mMoveDirection == 1) {
					if (AtLadderTop(act)) {
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
				if (AtLadderTop(act)) {
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
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path2, 0.0f, 0.0f, 0.0f);
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
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path2, 0.0f, 0.0f, 0.0f);
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
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path2, 0.0f, 0.0f, 0.0f);
			if (act->mOwner->GetAttachment() == AttachmentCopter) {
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
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	}
}

} // extern "C"

extern "C" {
}

extern "C" {
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
u32 CheckStageFlag(s32 stage, u32 mask);
void PutAwayLiftedObject(XActLift*);

/**
 * @note Address: 0x800C9C34
 * @note Size: 0x3C
 */
void RecordObjectPositionAndHeightChange(XObj* obj)
{
	if (obj->mMoveTranslate.y != obj->mLastPos.y) {
		obj->mStatus.mFlags |= 0x0010000000000000ULL;
	}
	obj->mLastPos.x = obj->mMoveTranslate.x;
	obj->mLastPos.y = obj->mMoveTranslate.y;
	obj->mLastPos.z = obj->mMoveTranslate.z;
}

/**
 * @note Address: 0x800C9C70
 * @note Size: 0xE4
 */
void XActLadder::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	mBPressed = FALSE;
	if (mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && (pad.mTrigger & PAD_BUTTON_B)) {
			mBPressed = TRUE;
		}
	} else if (mState != 0) {
		moveWeight = 0.0f;
	}
}

CVec GetPlayerFloorSlope(XObjChr*);

static inline f32 JumpMotionLength(XObjChr* owner)
{
	CVec motion = GetPlayerFloorSlope(owner);
	return PSVECMag(&motion);
}

static inline f32 JumpStickLength(const CPadOne* pad)
{
	return sqrtf(pad->mStickX * pad->mStickX + pad->mStickY * pad->mStickY);
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
static inline bool AdvanceJumpArc(XActJump* act, u32 frames)
{
	if (act->mArcAngle < HALF_PI) {
		if ((act->mArcAngle += act->mArcStep * frames) > HALF_PI)
			act->mArcAngle = HALF_PI;
		f32 lift = act->mRise * (f32)sin(act->mArcAngle);
		act->mOwner->SetTranslateY(act->mArcPosition.y + lift, 0);
		f32 angle = HSD_JObjGetRotationY(act->mOwner->mJObj);
		f32 dx    = act->mForward * (act->mArcStep * (f32)sin(angle));
		f32 dz    = act->mForward * (act->mArcStep * (f32)cos(angle));
		act->mOwner->SetTranslateX(act->mArcPosition.x += dx, 0);
		act->mOwner->SetTranslateZ(act->mArcPosition.z += dz, 0);
		return false;
	}
	return true;
}

static inline bool IsJumpAnim(s32 anim, s32 base, s32 first, s32 second)
{
	return anim == base || anim == first || anim == second;
}

/**
 * @note Address: 0x800CA0B4
 * @note Size: 0x1D00
 */
void UpdatePlayerJumpAction(XActJump* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	bool interrupt = false;
	if (!gGame.mEvt.mData.mScript) {
		CPad pad(0);
		if (JumpStickLength(pad.mPad) > 0) {
			switch (act->mState) {
			case 19:
				if (frame > 1)
					interrupt = true;
				break;
			case 4:
			case 18:
				if (frame > 3)
					interrupt = true;
				break;
			case 10:
			case 13:
			case 16:
				act->mOwner->SetAnimRate(1.f + 2.f * ((frame / end) * JumpStickLength(pad.mPad)), 0x5a0);
				break;
			}
		}
	}
	s32 next     = -1;
	bool playing = false;
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
		if (act->mOwner->GetHeight() <= 0 || (act->mOwner->GetHeight() <= 1.f && JumpMotionLength(act->mOwner) > .5f)) {
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
				if (anim != 0x7c)
					JumpAnim(act, 0x7c, 0);
			} else if (anim != 0x1d && anim != 0x93)
				JumpAnim(act, 0x1d, 0);
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
			s32 id = 0x90;
			if (act->mTargetStrength >= 50)
				id = 0x9a;
			else if (act->mTargetStrength >= 45 && !gGame.mEvt.mData.mScript) {
				// a hard landing makes him put the mug (item 22) away
				XObjChr* chr = act->mOwner;
				if (chr->mLift.mState && IsCarryingMug(&chr->mLift))
					PutAwayLiftedObject(&chr->mLift);
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
		next    = 0x5a;
		playing = IsJumpAnim(anim, next, 0x10f, 0x115);
	// fall through
	case 11:
		if (next < 0) {
			next    = 0x60;
			playing = IsJumpAnim(anim, next, 0x112, 0x118);
		}
	// fall through
	case 14:
		if (next < 0) {
			next    = 0x63;
			playing = IsJumpAnim(anim, next, 0x11b, 0x11e);
		}
		if (!playing) {
			act->mTargetHeight = act->mTarget->mSelectedFloorHeight;
			if (act->mOwner->mMoveTranslate.y >= act->mTargetHeight) {
				act->mState = 0;
				return;
			}
			if (act->mTarget) {
				f32 yaw = GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 0, NULL, NULL);
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
			s32 id = act->mState == 8 ? 0x5b : act->mState == 11 ? 0x61 : 0x64;
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
				act->mArcStep     = act->mRise > 6 ? .8f * (HALF_PI / act->mRise) : DEG2RAD * 12;
				act->mForward     = 5;
				{
					XObjChr* p = act->mOwner;
					f32 radius = p->mMoveScale.x * ReadObjectFloorExtent(p->mFloors[0]).x;
					f32 height
					    = .5f * (p->mMoveScale.x * (ReadObjectFloorOffset(p->mFloors[0]).y + ReadObjectFloorExtent(p->mFloors[0]).y));
					Mtx rot;
					PSMTXRotRad(rot, 'y', p->mMoveRotate.y);
					Vec offset = act->mOwner->mMoveScale * ReadObjectFloorOffset(act->mOwner->mFloors[0]);
					PSMTXMultVec(rot, &offset, &offset);
					f32 yaw = act->mOwner->mMoveRotate.y;
					f32 x   = 3.f * (f32)sin(yaw) + (offset.x + act->mOwner->mMoveTranslate.x);
					f32 z   = 3.f * (f32)cos(yaw) + (offset.z + act->mOwner->mMoveTranslate.z);
					f32 y   = .5f + act->mTargetHeight;
					for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
						XObj* obj = gGame.GetObject(i);
						if (!(obj->mStatus.mFlags & 1) || !(obj->mStatus.mFlags & 0x10000) || obj->mStatus.mFlags & 0x100000)
							continue;
						if (obj->GetCategory() == 7 || obj->GetCategory() == 20 || act->mOwner->mId == obj->mId || obj->GetKind() >= 10000)
							continue;
						f32 dx = obj->mMoveTranslate.x - x, dz = obj->mMoveTranslate.z - z;
						if (sqrtf(dx * dx + dz * dz) > 100.f + (radius + obj->mFloorXZExtent))
							continue;
						CVec push, contact, from(x, y, z), to(x, y, z);
						if ((obj->TestFloorCollision(&from, &to, radius, height, &push, &contact, 0, 0) & 3) == 3) {
							act->mForward = 0;
							act->mRise *= .8f;
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
		next    = 0x5b;
		playing = IsJumpAnim(anim, next, 0x110, 0x116);
	// fall through
	case 12:
		if (next < 0) {
			next    = 0x61;
			playing = IsJumpAnim(anim, next, 0x113, 0x119);
		}
	// fall through
	case 15:
		if (next < 0) {
			next    = 0x64;
			playing = IsJumpAnim(anim, next, 0x11c, 0x11f);
		}
		if (!playing)
			JumpAnim(act, next);
		if (AdvanceJumpArc(act, frames)) {
			if (act->mForward == 0) {
				JumpAnim(act, 0x1d, 0);
				act->mState = 17;
				return;
			}
			s32 id = act->mState == 9 ? 0x5c : act->mState == 12 ? 0x62 : 0x65;
			JumpAnim(act, id);
			++act->mState;
		}
		break;
	case 10:
		next    = 0x5c;
		playing = IsJumpAnim(anim, next, 0x111, 0x117);
	// fall through
	case 13:
		if (next < 0) {
			next    = 0x62;
			playing = IsJumpAnim(anim, next, 0x114, 0x11a);
		}
	// fall through
	case 16:
		if (next < 0) {
			next    = 0x65;
			playing = IsJumpAnim(anim, next, 0x11d, 0x120);
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
			if (chr->mLift.mState != 0 && IsCarryingMug(&chr->mLift)) {
				PutAwayLiftedObject(&chr->mLift);
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
void XActJump::HandleInput(const CPadOne&, f32& moveWeight)
{
	if (mState != 0) {
		moveWeight = 0.0f;
	}
}

} // extern "C"

extern "C" {
}

extern "C" {
void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z);
CVec GetPlayerJointLocalTranslationByPath(CJObj* obj, s32 depth, s32* path);
void SetJointYawByPath(CXObj* obj, s32 depth, s32* path, f32 angle);
void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos);
void RecordObjectPositionAndHeightChange(XObj* obj);
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
void TryLeaveHang(XActHang* act);
Vec GetTargetFloorPosition(XAction* act);
u32 CheckStageFlag(s32 stage, u32 mask);

static f32 GetScaledTargetFloorExtent(const XAction* act);

s32 FindPlayerSupportingFloor(XObjChr*, s32, s32);
f32 GetObjectEdgeOutwardYaw(const XObj*, Vec*, s32);

static inline void HangAnim(XActHang* act, s32 id, f32 loop = 1, BOOL force = FALSE, f32 start = 0)
{
	act->mOwner->SetAnim(id, force, NULL, NULL, NULL, start, 1, loop, 10);
}
static inline void ReleaseHangPlug(XActHang* act)
{
	if (CheckStageFlag(gGame.mStage, 1)) {
		XObjPlug* plug = &act->mOwner->mPlug;
		if (plug->mConnection) {
			ResetPlayerPlugState(plug, TRUE);
			PlayGameSound(&gGame.mSound, 0x37, 255);
		}
	}
}
static inline bool RoundHangFloor(XActHang* act)
{
	return act->mTarget->mFloors[act->mTargetFloor].mId == 0;
}
static inline void SaveHangDistance(XActHang* act)
{
	CVec owner(act->mOwner->mMoveTranslate);
	owner.y = 0;
	CVec floor(GetTargetFloorPosition(act));
	floor.y           = 0;
	act->mFloorCenter = floor;
	act->mDistance    = PSVECDistance(&owner, &floor);
}
static inline void RefreshHangFloor(XActHang* act)
{
	s32 floor = FindPlayerSupportingFloor(act->mOwner, 1, act->mTargetFloor);
	if (floor >= 0)
		act->mTargetFloor = floor;
}
static inline f32 RoundHangAngle(XActHang* act)
{
	CVec owner(act->mOwner->mMoveTranslate);
	owner.y = 0;
	CVec floor(GetTargetFloorPosition(act));
	floor.y = 0;
	return PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
}
static inline void HangRootPosition(XActHang* act)
{
	s32 path[] = { 0 };
	act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
	RecordObjectPositionAndHeightChange(act->mOwner);
	s32 reset[] = { 0 };
	SetObjectJointTranslationByPath(act->mOwner, 1, reset, 0, 0, 0);
}
static inline void FinishHang(XActHang* act, f32 loop)
{
	XObjChr* owner = act->mOwner;
	owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, loop, 10);
	owner->mIdleLoops = 3;
	act->mState       = 0;
}

/**
 * @note Address: 0x800CBDCC
 * @note Size: 0x1CE8
 */
void UpdatePlayerHangAction(XActHang* act, s32 anim, HSD_AObj* aobj, u32, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1: {
		static CVec contact, normal, start;
		if (anim != 0x13) {
			if (act->mTarget) {
				f32 angle = GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 1, &contact, &normal);
				if (__fpclassifyf(angle) == FP_NAN) {
					act->mState  = 0;
					act->mTarget = NULL;
					return;
				}
				StopPlayerMotion(act->mOwner);
				HangAnim(act, 0x13, 0);
				act->mOwner->SetRotationY(angle, 0);
				start.Set(act->mOwner->mMoveTranslate);
				contact.y = start.y;
			}
			ReleaseHangPlug(act);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HangAnim(act, 0x14);
			act->mState         = 5;
			act->mMoveDirection = 0;
			if (act->mTarget) {
				if (RoundHangFloor(act))
					SaveHangDistance(act);
				else
					act->mOwner->SetPos(contact - normal * 4.f, 0);
			}
		} else if (act->mTarget) {
			if (RoundHangFloor(act)) {
				XObjChr* chr = act->mOwner;
				f32 radius   = chr->mMoveScale.x * GetScaledTargetFloorExtent(act);
				CVec owner(act->mOwner->mMoveTranslate);
				owner.y = 0;
				CVec floor(GetTargetFloorPosition(act));
				floor.y = 0;
				if (PSVECDistance(&owner, &floor) < 4.f + radius) {
					f32 yaw = act->mOwner->mMoveRotate.y;
					act->mOwner->SetTranslateX(act->mOwner->mMoveTranslate.x - .2f * (f32)sin(yaw), 0);
					act->mOwner->SetTranslateZ(act->mOwner->mMoveTranslate.z - .2f * (f32)cos(yaw), 0);
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
				RefreshHangFloor(act);
				f32 yaw               = RoundHangFloor(act) ? RoundHangAngle(act)
				                                            : PI + GetObjectEdgeOutwardYaw(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor);
				yaw                   = WrapAngle(yaw);
				act->mHangStartOffset = act->mOwner->mMoveTranslate - act->mTarget->mMoveTranslate;
				CVec pos(act->mOwner->mMoveTranslate);
				pos.x += 1.5f * (f32)sin(PI + yaw);
				pos.z += 1.5f * (f32)cos(PI + yaw);
				act->mOwner->SetPos(pos, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
			}
			HangAnim(act, 0x7a, 0);
			return;
		}
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			act->mOwner->SetPos(act->mTarget->mMoveTranslate + act->mHangStartOffset, 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			act->mState               = 0;
			act->mTarget              = NULL;
			act->mOwner->mJump.mState = 3;
			ReleaseHangPlug(act);
			return;
		}
		if (!act->mStickAway)
			return;
		{
			f32 last = GetJointAnimationEndFrame(act->mOwner);
			if (GetJointAnimationCurrentFrame(act->mOwner) / last <= .6f) {
				FinishHang(act, 0);
				act->mTarget = NULL;
				return;
			}
		}
		act->mOwner->SetPos(act->mTarget->mMoveTranslate + act->mHangStartOffset, 0);
		RecordObjectPositionAndHeightChange(act->mOwner);
		if (act->mOwner->GetAttachment() == AttachmentPopper || act->mOwner->mLift.mState) {
			act->mOwner->mJump.mState = 2;
			act->mState               = 0;
			act->mTarget              = NULL;
			ReleaseHangPlug(act);
		} else if (act->mTarget && act->mTarget->mStatus.mFlags & 0x800000000ULL) {
			act->mOwner->mLadder.SetState(3, act->mTarget);
			act->mState  = 0;
			act->mTarget = NULL;
		} else if (act->mTarget && act->mTarget->mStatus.mFlags & 0x1000000000ULL) {
			act->mOwner->mRope.SetState(3, act->mTarget);
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
				RefreshHangFloor(act);
				CVec contact(0, 0, 0);
				f32 yaw;
				if (RoundHangFloor(act))
					yaw = RoundHangAngle(act);
				else {
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
				HangAnim(act, 0x13, 1, TRUE);
				HangAnim(act, 0x13, 1, TRUE, GetJointAnimationEndFrame(act->mOwner));
				act->mOwner->UpdateAnim(0);
				s32 p1[]   = { 0 };
				CVec local = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
				s32 p2[]   = { 0 };
				CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
				delta.y    = -delta.y;
				act->mOwner->SetPos(act->mOwner->mMoveTranslate + delta, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
				HangAnim(act, 0x76);
				local.x  = -local.x;
				local.z  = -local.z;
				s32 p3[] = { 0 };
				SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, local);
				CVec pos(act->mOwner->mMoveTranslate);
				f32 distance;
				if (RoundHangFloor(act)) {
					CVec owner(act->mOwner->mMoveTranslate);
					owner.y = 0;
					CVec floor(GetTargetFloorPosition(act));
					floor.y    = 0;
					yaw        = PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
					f32 span   = PSVECDistance(&owner, &floor);
					XObjChr* p = act->mOwner;
					distance   = 1.f + (span - p->mMoveScale.x * GetScaledTargetFloorExtent(act));
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
			ReleaseHangPlug(act);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 childPath[] = { 0 };
			HSD_JObj* child = GetObjectJointByPath(act->mOwner, 1, childPath);
			f32 turn        = HSD_JObjGetRotationY(child);
			act->mOwner->SetRotationY(HSD_JObjGetRotationY(act->mOwner->mJObj) + turn, 0);
			HSD_JObjSetRotationY(child, 0);
			HangAnim(act, 0x13);
			HangAnim(act, 0x13, 1, TRUE, GetJointAnimationEndFrame(act->mOwner));
			act->mOwner->UpdateAnim(0);
			s32 p1[]   = { 0 };
			CVec local = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
			s32 p2[]   = { 0 };
			// Update the joint's world matrix before restoring the local translation.
			GetEventJointWorldPosition(act->mOwner, 1, p2);
			s32 p3[] = { 0 };
			SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, local);
			HangAnim(act, 0x14);
			act->mState         = 5;
			act->mMoveDirection = 0;
			if (act->mTarget && RoundHangFloor(act))
				SaveHangDistance(act);
		}
		break;
	case 2:
		if (anim != 0x18) {
			HangAnim(act, 0x18);
			if (act->mTarget)
				act->mOwner->SetRotationY(GetObjectEdgeOutwardYaw(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor), 0);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HangRootPosition(act);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y, 0);
			s32 p[] = { 0 };
			SetJointYawByPath(act->mOwner, 1, p, 0);
			HangAnim(act, 0x14);
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
				HangAnim(act, 0x16);
		} else if (act->mMoveDirection == 1) {
			if (anim != 0x15)
				HangAnim(act, 0x15);
		} else if (anim != 0x14)
			HangAnim(act, 0x14);
		break;
	// climbing up, TryLeaveHang picks the other climb (7) one time in eight
	case 6:
	case 7: {
		int next = act->mState == 7 ? 0x7d : 0x17;
		if (anim != next)
			HangAnim(act, next);
		else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HangRootPosition(act);
			FinishHang(act, 0);
			act->mTarget = NULL;
		}
		break;
	}
	case 8:
		HangAnim(act, 0x19, 1, TRUE, 15);
		act->mState = 9;
	// fall through
	case 9:
		if (anim != 0x19) {
			HangAnim(act, 0x19);
			return;
		}
		if (!(aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)))
			return;
		HangAnim(act, 0x1a);
		act->mState  = 10;
		act->mTarget = NULL;
	// fall through
	case 10:
		if (act->mOwner->GetHeight() <= 0) {
			if (act->mOwner->mFallDistance > 30) {
				act->mState = 0;
				HangAnim(act, 0x8e);
				act->mOwner->mJump.mState = 5;
				ApplyPlayerJumpStrength(&act->mOwner->mJump, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 255, 0, 0, 0);
				act->mOwner->SetColor(1, 0, 0, 0, .3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, .3f);
			} else {
				HangAnim(act, 0x1b);
				act->mState = 11;
			}
		} else if (act->mOwner->GetAttachment() == AttachmentCopter) {
			act->mState                 = 0;
			act->mOwner->mCopter.mState = 8;
		}
		break;
	case 11:
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM))
			FinishHang(act, 1);
		break;
	}
}

/**
 * @note Address: 0x800CDBA8
 * @note Size: 0x578
 */
void TryLeaveHang(XActHang* act)
{
	const XObjFloor& floor = act->mOwner->mFloors[0];
	f32 radius             = act->mOwner->mMoveScale.x * ReadObjectFloorExtent(floor).x;
	f32 halfHeight         = 0.5f * (act->mOwner->mMoveScale.x * (ReadObjectFloorOffset(floor).y + ReadObjectFloorExtent(floor).y));
	s32 path[]             = { 0 };
	Vec position           = GetEventJointWorldPosition(act->mOwner, 1, path);
	position.y += 0.5f;

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
		if ((obj->TestFloorCollision(&position, &position, radius, halfHeight, &push, &contact, 0, 0) & 3) == 3) {
			return;
		}
	}
	if ((rand() & 7) == 0)
		act->mState = 7;
	else
		act->mState = 6;
}

/**
 * @note Address: 0x800CE120
 * @note Size: 0x214
 */
void XActHang::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	mStickAway = FALSE;
	if (mState == 4) {
		f32 stick = -(f32)atan2(pad.mStickX, pad.mStickY);
		f32 angle = WrapAngle(stick + gGame.mMotionControlCamera.mCamera.mAngle.y - mOwner->mMoveRotate.y);
		// the stick points anywhere but within 30 degrees of where he faces
		if (!(angle < 30 * DEG2RAD) && !(angle > 330 * DEG2RAD)) {
			mStickAway = TRUE;
		}
		moveWeight = 0.0f;
	} else if (mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			// a climbs up if there is room, b lets go (9)
			if (pad.mTrigger & PAD_BUTTON_A) {
				if (mTarget == NULL || (mTarget->mStatus.mFlags & 0x400000000ULL)) {
					TryLeaveHang(this);
					mMoveDirection = 0;
				}
				moveWeight = 0.0f;
			} else if (pad.mTrigger & PAD_BUTTON_B) {
				mState         = 9;
				mMoveDirection = 0;
				moveWeight     = 0.0f;
			}
		}
	} else if (mState != 0) {
		moveWeight = 0.0f;
	}
}

static f32 GetScaledTargetFloorExtent(const XAction* act)
{
	const XObj* target = act->mTarget;
	return target->mMoveScale.x * ReadObjectFloorExtent(target->mFloors[act->mTargetFloor]).x;
}

} // extern "C"

extern "C" {

/**
 * @note Address: 0x800CE334
 * @note Size: 0x20
 */
void SetLiftPlugConnection(XActLift* act, u32 value, BOOL apply)
{
	act->mPlugConnection = value;
	if (apply) {
		act->mOwner->mPlug.mConnection = value | 0x10000;
	}
}

static inline bool CarriedObjectTouches(XObj* object, const Vec& pos, f32 radius, f32 height, Vec& push, Vec& contact)
{
	return (object->TestFloorCollision(&pos, &pos, radius, height, &push, &contact, NULL, 0) & 3) == 3;
}

/**
 * @note Address: 0x800CE354
 * @note Size: 0x5B0
 */
BOOL CheckCarriedObjectContacts(XActLift* act, const Vec* pos, f32 radius, f32 height, BOOL checkItem, BOOL checkEvent, BOOL checkOthers,
                                Vec* result)
{
	s16 item   = act->mTarget->GetItem();
	BOOL found = FALSE;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.GetObject(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000))
			continue;
		f32 dx = object->mMoveTranslate.x - pos->x;
		f32 dz = object->mMoveTranslate.z - pos->z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (radius + object->mFloorXZExtent))
			continue;
		if (checkItem && (object->mStatus.mFlags & 0x40000000000ULL)) {
			CVec push, contact;
			if (CarriedObjectTouches(object, *pos, radius, height, push, contact) && object->OnItemUsed(pos, item)) {
				found = TRUE;
				if (result)
					*result = contact;
			}
		}
		if (checkEvent && (object->mStatus.mFlags & 0x80000000000ULL)) {
			CVec push, contact;
			if (CarriedObjectTouches(object, *pos, radius, height, push, contact)) {
				object->OnSquirterHit(&contact, act->mOwner->mSquirterContent, act->mOwner->mItemTintWord);
				found = TRUE;
				if (result)
					*result = contact;
			}
		}
		if (checkOthers && !(object->mStatus.mFlags & 0x2000000000000ULL) && !(object->mStatus.mFlags & 0x100000) && object != act->mTarget
		    && object != act->mOwner) {
			CVec push, contact;
			if (CarriedObjectTouches(object, *pos, radius, height, push, contact)) {
				object->OnSquirterHit(&contact, act->mOwner->mSquirterContent, act->mOwner->mItemTintWord);
				found = TRUE;
				if (result)
					*result = contact;
			}
		}
	}
	return found;
}

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
// NONMATCHING: only register assignment and independent load/store scheduling differ; calls, field offsets and return copies match retail.
Vec GetLiftTargetContactOffset(XActLift* act, XObj* target)
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

} // extern "C"

extern "C" {
}

// chibi robo's actions, the XAct kinds of XAction, and the game helpers they use

extern "C" {
void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z);
CVec GetPlayerJointLocalTranslationByPath(CJObj* obj, s32 depth, s32* path);
void SetJointYawByPath(CXObj* obj, s32 depth, s32* path, f32 angle);
void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos);
void RecordObjectPositionAndHeightChange(XObj* obj);
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
void TryLeaveHang(XActHang* act);
Vec GetTargetFloorPosition(XAction* act); // where the held floor of the target is
u32 CheckStageFlag(s32 stage, u32 mask);
void PutAwayLiftedObject(XActLift*);
}

extern "C" {

void GetJointScale(CJObj*, HSD_JObj*, Vec*);
f32 GetFirstFloorCollisionRadius(const XObj*);
extern const s32 gSquirterChargeByCount[];

static inline s32 HeldItemVariant(XActLift* act)
{
	return gSquirterChargeByCount[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)];
}

/**
 * @note Address: 0x800CECFC
 * @note Size: 0x2040
 */
void UpdateHeldObject(XActLift* act, s32)
{
	if (!act->mTargetState)
		return;
	XObj* obj = act->mTarget;
	if (!obj || !(obj->mStatus.mFlags & 1))
		return;
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
			s32 path[]     = { 0, 0 };
			HSD_JObj* root = obj->mJObj;
			HSD_JObjCopyMtx(root, HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)));
			if (IsCarryingSpoon(act))
				obj->SetAnim(4, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			else if (IsCarryingSquirter(act))
				obj->SetAnim(HeldItemVariant(act) + 5, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
		} else {
			UpdateCarriedObjectPosition(act, obj);
			s32 path[] = { 0, 0 };
			Vec scale;
			GetJointScale(act->mOwner, GetObjectJointByPath(act->mOwner, 2, path), &scale);
			obj->SetScale(GetStagePlacementScale(obj) * scale, 0);
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
			s32 path[]     = { 0, 0 };
			HSD_JObj* root = obj->mJObj;
			HSD_JObjCopyMtx(root, HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)));
			s32 id = 0;
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
					id = HeldItemVariant(act) + 1;
				break;
			case 0xca:
				if (IsCarryingToothbrush(act))
					id = 3;
				else if (IsCarryingSpoon(act))
					id = 4;
				else if (IsCarryingSquirter(act))
					id = HeldItemVariant(act) + 5;
				break;
			default:
				if (IsCarryingToothbrush(act))
					id = 0;
				else if (IsCarryingSpoon(act))
					id = 1;
				else if (IsCarryingSquirter(act))
					id = HeldItemVariant(act) + 1;
				break;
			}
			obj->SetAnim(id, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			break;
		}
		obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
		UpdateCarriedObjectPosition(act, obj);
		if (act->mTarget != NULL && act->mTarget->GetCategory() == 24) {
			s32 handPath[] = { 0, 0 };
			CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
			s32 path[] = { 0 };
			CJObj plug(GetObjectJointByPath(obj, 1, path));
			plug.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			plug.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
		} else if (IsCarryingMug(act)) {
			s32 id = 4;
			if (act->mState != 31) {
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
		CVec push, contact;
		XObjChr* owner = act->mOwner;
		f32 radius     = GetFirstFloorCollisionRadius(owner);
		f32 height     = owner->GetCollisionHeight();
		if (obj->TestFloorCollision(&act->mOwner->mMoveTranslate, &act->mOwner->mMoveTranslate, radius, height, &push, &contact, 0, 0)
		    & 1) {
			f32 distance = PSVECDistance(&act->mOwner->mMoveTranslate, &contact);
			obj->SetTranslateX(distance * (f32)sin(act->mOwner->mMoveRotate.y) + obj->mMoveTranslate.x, 0);
			obj->SetTranslateZ(distance * (f32)cos(act->mOwner->mMoveRotate.y) + obj->mMoveTranslate.z, 0);
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
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTarget = NULL;
			else if (!plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 90) {
				plug->mConnection = act->mPlugConnection | 0x10000;
				obj->mStatus.mFlags &= ~0x0400000000000000ULL;
			} else if (!plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 34 && act->mPlugConnection == 0x200)
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
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTargetState = 4;
			else if ((anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 80)
				end.SetRotationZ(0, 0);
			else if (plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 76) {
				ResetPlayerPlugState(plug, FALSE);
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
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTargetState = 9;
			else if (!plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 24)
				plug->mConnection = -1;
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
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTargetState = 4;
			else if (plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 0)
				ResetPlayerPlugState(plug, FALSE);
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
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTargetState = 12;
			else if (!plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 24)
				plug->mConnection = 0x10400;
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
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTargetState = 4;
			else if (plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 0)
				ResetPlayerPlugState(plug, FALSE);
		}
		break;
	case 14:
		if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
			s32 path[]     = { 0, 0 };
			HSD_JObj* root = obj->mJObj;
			HSD_JObjCopyMtx(root, HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)));
			if (IsCarryingSpoon(act))
				obj->SetAnim(4, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			else if (IsCarryingSquirter(act))
				obj->SetAnim(HeldItemVariant(act) + 5, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
		} else {
			UpdateCarriedObjectPosition(act, obj);
			s32 path[] = { 0, 0 };
			Vec scale;
			GetJointScale(act->mOwner, GetObjectJointByPath(act->mOwner, 2, path), &scale);
			obj->SetScale(GetStagePlacementScale(obj) * scale, 0);

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
		HSD_JObjCopyMtx(obj->mJObj, world);
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
	case 19: {
		HSD_JObjClearMtxFlags(obj->mJObj);
		act->mYawOffset = 0;
		obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
		UpdateCarriedObjectPosition(act, obj);
		s32 path[] = { 0, 0 };
		Vec scale;
		GetJointScale(act->mOwner, GetObjectJointByPath(act->mOwner, 2, path), &scale);
		obj->SetScale(GetStagePlacementScale(obj) * scale, 0);
		act->mTargetState = 4;
		break;
	}
	case 20:
		if (GetAObj(obj->mJObj)->flags & 0x44000000) {
			obj->Release();
			act->mTarget = NULL;
		}
		break;
	}
}

/**
 * @note Address: 0x800D0DCC
 * @note Size: 0xC0
 */
extern "C" f32 GetFirstFloorCollisionRadius(const XObj* object)
{
	return object->mMoveScale.x * ReadObjectFloorExtent(object->mFloors[0]).x;
}

} // extern "C"

extern "C" {
}

extern "C" {
void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z);
void RecordObjectPositionAndHeightChange(XObj* obj);
s32 GetObjectPoolId(const XObj*);
s16 GetObjectItemId(XObj*);
s16 GetItemPickupHoldTime(s32);
f32 GetHeadingBetweenPositions(const Vec*, const Vec*);
void SetPlugCordSpeed(XObjPlug*, f32);
void SetLiftActionState(XActLift*, s32);
void SetLiftTargetState(XActLift*, s32);
XObj* GetGameObjectByIndex(const CGame*, s32);
BOOL IsEventScriptActive(const CGame*);
Params* GetPlayerParamsByIndex(CGame*, s32);
s32 GetInventoryCountByIndex(Params*, u32);
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
Vec GetJointMatrixScale(const CJObj*, HSD_JObj*);
HSD_Generator* CreateParticleAtPosition(CGame*, u32, u32, s32, const Vec&, BOOL);
extern HSD_PSUserFunc gPlayerItemParticleCallbacks;
void SetSubwindowRect(CGame*, f32, f32, f32, f32);
void SetSubwindowCamera(CGame*, const Vec&, f32, f32, f32, f32);
void EnableSubwindowWithFlags(CGame*, u32);
Vec GetObjectJointWorldPosition(const CJObj*);
BOOL IsLiftActionState(const XActLift*, s32);
f32 GetObjectJointRotationY(CJObj*);
void ClearObjectAnimationQueue(XObj*);
Vec GetObjectJointMatrixScale(const CJObj*);
HSD_Generator* CreateScaledParticleAtPosition(CGame*, u32, u32, s32, const Vec&, const Vec&, BOOL);
extern const s32 gSquirterChargeByCount[];
}

static inline void HeldAnim(XObj* obj, s32 id, BOOL force = FALSE, f32 blend = 0)
{
	obj->SetAnim(id, force, NULL, NULL, NULL, 0, 1, blend, 10);
}
static inline bool HasRoomForItem(XObj* item)
{
	s16 id   = GetObjectItemId(item);
	s32 slot = FindOwnedInventoryItemIndex(GetPlayerParamsByIndex(&gGame, 0)->mInventory.mItems, id);
	return slot < 0 || GetInventoryCountByIndex(GetPlayerParamsByIndex(&gGame, 0), slot) < 10;
}

static inline void LoadLiftItem(XActLift* act, s32 next)
{
	act->mPendingItem = act->mPendingItemId >= 0 ? SpawnObjectFromPlacementIndex(&gGame, act->mPendingItemId) : NULL;
	SetLiftActionState(act, next);
}
static inline void TakeLiftItem(XActLift* act, s32 anim, HSD_AObj* aobj, bool useCount)
{
	s32 wanted = useCount ? 11 : 0xec;
	if (anim != wanted) {
		HeldAnim(act->mOwner, wanted, TRUE, 1);
		SetLiftTargetState(act, useCount ? 3 : 18);
		if (IsLiftTargetPlug(act)) {
			ClearObjectStatusFlags(act->mTarget, 0x0400000000000000ULL);
			act->mTarget = NULL;
		}
		if (act->mPendingItemId >= 0 && act->mPendingItem) {
			act->mPendingItem->mStatus.mFlags |= 0x0400000000000000ULL;
			act->mPendingItem->SetScale(0, 0);
			SetPlayerActionTarget(act, act->mPendingItem);
			if (useCount) {
				s16 id         = GetObjectItemId(act->mTarget);
				s32 count      = RemovePlayerInventoryItemCount(GetPlayerParamsByIndex(&gGame, 0), id, 1);
				s32 difference = count - GetStageCollectionFlagBase(0);
				if (difference > 0)
					act->mPendingItem->mPlacementParam1 = difference;
			}
			act->mPendingItemId = -1;
			if (useCount && IsCarryingSquirter(act))
				ApplyPlayerItemTintToTarget(act);
		}
	} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
		HeldAnim(act->mOwner, 8);
		SetLiftActionState(act, 4);
		SetLiftTargetState(act, useCount ? 4 : 19);
	}
}
static inline void ReleasePlugTarget(XActLift* act, s32 anim, HSD_AObj* aobj, s32 next)
{
	if (anim != next) {
		HeldAnim(act->mOwner, next, FALSE, 1);
		act->mOwner->UpdateAnim(0);
		SetLiftTargetState(act, 13);
	} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
		HeldAnim(act->mOwner, 8);
		SetLiftActionState(act, 4);
		act->mPlugTarget = NULL;
	}
}
static inline void UseToolCharge(XActLift* act, s16 count, s32 level)
{
	if (gSquirterChargeByCount[count] == level)
		HeldAnim(act->mTarget, level + 1);
	else
		HeldAnim(act->mTarget, level);
	SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, count, FALSE);
}

/**
 * @note Address: 0x800D0F4C
 * @note Size: 0x2B10
 */
extern "C" void UpdateLiftActionAnimation(XActLift* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32)
{
	s32 placement     = act->mTarget ? act->mTarget->mCoord.mPlacement - GetStagePlacementByIndex(0) : -1;
	act->mBatteryCost = 0;
	switch (act->mState) {
	case 1:
		if (anim != 5) {
			HeldAnim(act->mOwner, 5);
			if (act->mTarget) {
				const CVec& point = GetFloorContactPosition(act->mTarget, act->mTargetFloor, NULL);
				act->mOwner->SetRotateY(GetHeadingBetweenPositions(&act->mOwner->mMoveTranslate, &point), 12 * DEG2RAD);
			}
			if (placement < 0 && PSVECDistance(&act->mOwner->mMoveTranslate, &act->mTarget->mMoveTranslate) >= 3.0f)
				SetPlugCordSpeed(&act->mOwner->mPlug, 0.2f);
			return;
		}
		if (!(aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)))
			return;
		HeldAnim(act->mOwner, 6, TRUE, 1);
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
		act->mTimer -= frames;
		if (!(act->mTimer <= 0))
			return;
		SetLiftActionState(act, 3);
		// fall through
	case 3:
		if (anim != 7) {
			if (!act->mTarget || (s16)GetObjectPoolId(act->mTarget) != 8 || HasRoomForItem(act->mTarget)) {
				SetLiftTargetState(act, 1);
				HeldAnim(act->mOwner, 7, TRUE, 1);
			} else {
				SetLiftActionState(act, 5);
			}
			if (act->mTarget && act->mTarget->GetEventScript() && (act->mTarget->mStatus.mFlags & 0x10000000000ULL)) {
				XObj* target = act->mTarget;
				TryStartObjectEvent(&gGame, GetGameObjectByIndex(&gGame, placement)->GetEventScript(), target, -1, -1);
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 8);
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
			if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
				HeldAnim(act->mOwner, 0x894);
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
			if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM))
				HeldAnim(act->mOwner, 0x894);
		}
		// the brush's effect lasts while it scrubs (0x895), the squirter's while it squirts (0x8c2 to 0x8c4)
		if (IsCarryingToothbrush(act)) {
			if (anim != 0x895 && act->mContactEffect) {
				gGame.mParticle.Kill(act->mContactEffectId);
				act->mContactEffect = NULL;
			}
		} else if (IsCarryingSquirter(act) && (anim < 0x8c2 || anim > 0x8c4) && act->mToolEffect) {
			gGame.mParticle.Kill(act->mToolEffectId);
			act->mToolEffect = NULL;
		}
		break;
	case 5:
		if (!IsEventScriptActive(&gGame)) {
			if ((s16)GetObjectPoolId(act->mTarget) == 8 && !(act->mTarget->mStatus.mFlags & 0x400000000000ULL)) {
				if (HasRoomForItem(act->mTarget))
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
			HeldAnim(act->mOwner, 9);
			SetLiftTargetState(act, 5);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			SetPlayerStandAnim(act->mOwner, 17, 0);
			SetLiftActionState(act, 0);
		}
		break;
	// event commands 102, 106 and 108 plug him into mPlugTarget: 7 only plugs in, 8 then gives it
	// power (12, 13), 9 then waits for the stick or b (15)
	case 7:
	case 8:
	case 9:
		if (anim != 0x9f && anim != 0xa0 && anim != 0xbc) {
			XObjPlug* plug = &act->mOwner->mPlug;
			SetPlayerActionTarget(act, plug);
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
		if (anim != 0x9f && anim != 0xa0 && anim != 0xbc) {
			ClearObjectAnimationQueue(act->mOwner);
			if (IsLiftActionState(act, 7)) {
				HeldAnim(act->mOwner, 0x9f);
				SetLiftTargetState(act, 6);
			} else if (IsLiftActionState(act, 8)) {
				HeldAnim(act->mOwner, 0xa0);
				SetLiftTargetState(act, 8);
			} else {
				HeldAnim(act->mOwner, 0xbc);
				SetLiftTargetState(act, 11);
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			if (IsLiftActionState(act, 7)) {
				SetPlayerStandAnim(act->mOwner, 17, 0);
				SetLiftActionState(act, 0);
				if (act->mPlugTarget)
					act->mPlugTarget->ReceivePlugPower(0);
			} else if (IsLiftActionState(act, 8)) {
				SetLiftActionState(act, 12);
				act->mTimer = 0.2f * FRAME_RATE;
			} else {
				HeldAnim(act->mOwner, 0xbe, FALSE, 1);
				SetLiftActionState(act, 15);
			}
		}
		break;
	case 11:
		if (anim != 0xad) {
			HeldAnim(act->mOwner, 0xad, FALSE, 1);
			act->mOwner->UpdateAnim(0);
			SetPlayerActionTarget(act, &act->mOwner->mPlug);
			SetLiftTargetState(act, 7);
			act->mYawOffset = 0;
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 root[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, root, 0, 0, 0);
			HeldAnim(act->mOwner, 8, FALSE, 1);
			SetLiftActionState(act, 4);
			act->mPlugTarget = NULL;
		}
		break;
	case 12:
		act->mTimer -= frames;
		if (act->mTimer <= 0) {
			act->mTimer = 0;
			if (act->mPlugTarget && !act->mPlugTarget->ReceivePlugPower(0)) {
				SetLiftActionState(act, 14);
				break;
			}
			HeldAnim(act->mOwner, 0xab);
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
				HeldAnim(act->mOwner, 0xab);
		} else if (anim != 0xae) {
			HeldAnim(act->mOwner, 0xae);
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
			HeldAnim(act->mOwner, 0xac);
			act->mOwner->UpdateAnim(0);
			SetLiftTargetState(act, 10);
			gGame.mSubwin.mEnabled = FALSE;
			if (act->mPlugTarget && GetObjData(&act->mPlugTarget->mCoord)->mCategory == 9)
				static_cast<XObjLeticker*>(act->mPlugTarget)->OnUnplugged();
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 8);
			SetLiftActionState(act, 4);
			act->mPlugTarget = NULL;
		}
		break;
	case 16: {
		XObjPlug* plug = static_cast<XObjPlug*>(act->mTarget);
		if (anim != 0xbf) {
			HeldAnim(act->mOwner, 0xbf, FALSE, 1);
			if (act->mPlugTarget)
				HeldAnim(act->mPlugTarget, 1, FALSE, 1);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 8);
			SetLiftActionState(act, 4);
			SetLiftTargetState(act, 4);
			act->mPlugTarget = NULL;
			ResetPlayerPlugState(plug, TRUE);
		} else if (frame > 142.0f) {
			plug->mConnection = 0;
		} else if (frame > 85.0f) {
			if (plug->mConnection != -1) {
				f32 remaining = act->mOwner->mParams.mBattery - 10.0f;
				if (remaining < 0)
					remaining = 0;
				SetClampedPlayerBattery(&act->mOwner->mParams, remaining);
			}
			plug->mConnection = -1;
		}
		break;
	}
	case 17:
		ReleasePlugTarget(act, anim, aobj, 0xbd);
		break;
	case 18:
		ReleasePlugTarget(act, anim, aobj, 0xac);
		break;
	case 19:
		if (anim != 10) {
			HeldAnim(act->mOwner, 10);
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
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			SetLiftActionState(act, 0);
			SetLiftTargetState(act, 15);
			SetPlayerStandAnim(act->mOwner, 17, 0);
			if (act->mTarget && (s16)GetObjectPoolId(act->mTarget) == 8) {
				s8 sound = (s8)GetItemSoundIndex(GetObjectItemId(act->mTarget), 1);
				if (sound >= 0)
					PlayGameSound(&gGame.mSound, GetItemReleaseSoundId(sound), 255);
			}
		}
		break;
	// 20 to 22 take an item out of his inventory, 24 to 26 the same without counting it off
	case 20:
		LoadLiftItem(act, 21);
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
		TakeLiftItem(act, anim, aobj, true);
		break;
	case 23:
		if (anim != 0xdd) {
			HeldAnim(act->mOwner, 0xdd);
			SetLiftTargetState(act, 16);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			SetPlayerStandAnim(act->mOwner, 17, 0);
			SetLiftActionState(act, 0);
			SetLiftTargetState(act, 17);
		}
		break;
	case 24:
		LoadLiftItem(act, 25);
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
		TakeLiftItem(act, anim, aobj, false);
		break;
	// a with a tool out: the toothbrush (27, 28), spoon (29, 30), mug (31 to 33) and squirter (35, 36)
	case 27:
		HeldAnim(act->mOwner, 0x895, TRUE);
		HeldAnim(act->mTarget, 1, TRUE);
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
		HeldAnim(act->mOwner, 0x8b7, TRUE);
		HeldAnim(act->mTarget, 5, TRUE);
		SetLiftActionState(act, 30);
		break;
	case 30:
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 0x8ae, TRUE);
			SetLiftActionState(act, 4);
			Vec pos = act->mOwner->mMoveTranslate;
			f32 yaw = act->mOwner->mMoveRotate.y;
			pos.x += 3.0f * (f32)sin(yaw);
			pos.z += 3.0f * (f32)cos(yaw);
			if (!CheckCarriedObjectContacts(act, &pos, 1, 1, TRUE, FALSE, FALSE, NULL)) {
				HeldAnim(act->mOwner, 0x8af, TRUE);
				PlayGameSound(&gGame.mSound, 0x110, 255);
				s32 path[]        = { 0, 0 };
				const CVec& point = CJObj_GetJointWorldPositionByPath(act->mTarget, 2, path);
				CreateScaledParticleAtPosition(&gGame, 1, 0x2e, 1, point, GetObjectJointMatrixScale(act->mTarget), FALSE);
			}
		}
		break;
	case 31:
		if (anim != 0x8b1) {
			HeldAnim(act->mOwner, 0x8b1);
			HeldAnim(act->mTarget, 4);
			act->mUseEffect = NULL;
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 0x8b2);
			HeldAnim(act->mTarget, 1);
			SetLiftActionState(act, 32);
		} else if (GetJointAnimationCurrentFrame(act->mOwner) > 10 && !act->mUseEffect) {
			act->mUseEffectId = 47;
			act->mUseEffect   = CreateParticleByBankAndId(&gGame, 1, 47, 1, act->mOwner->mJObj, FALSE);
		}
		break;
	case 33:
		if (anim != 0x8b3) {
			HeldAnim(act->mOwner, 0x8b3);
			HeldAnim(act->mTarget, 1);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 8);
			SetLiftActionState(act, 4);
		}
		break;
	case 34:
		PutAwayLiftedObject(act);
		break;
	// Refill at the low or high intake point. If neither touches a source, fire at the current charge level.
	case 35: {
		Vec point = act->mOwner->mMoveTranslate;
		f32 angle = act->mOwner->mMoveRotate.y;
		point.x += 2.0f * (f32)sin(angle);
		point.z += 2.0f * (f32)cos(angle);
		if (CheckCarriedObjectContacts(act, &point, 2, 1, TRUE, FALSE, FALSE, NULL)) {
			act->mOwner->mSquirterContent = 0;
			act->mOwner->mItemTintWord    = -1;
			HeldAnim(act->mOwner, 0x8d2);
			HeldAnim(act->mTarget, 17);
		} else {
			point.x += 2.0f * (f32)sin(angle);
			point.z += 2.0f * (f32)cos(angle);
			point.y += 7.0f;
			if (CheckCarriedObjectContacts(act, &point, 2, 3, TRUE, FALSE, FALSE, NULL)) {
				act->mOwner->mSquirterContent = 0;
				act->mOwner->mItemTintWord    = -1;
				HeldAnim(act->mOwner, 0x8c1);
				HeldAnim(act->mTarget, 10);
			}
		}
		if (act->mOwner->mAnimId != 0x8d2 && act->mOwner->mAnimId != 0x8c1) {
			s16 count       = (s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38);
			s32 chargeLevel = gSquirterChargeByCount[count];
			if (chargeLevel == 3) {
				HeldAnim(act->mOwner, 0x8c2);
				HeldAnim(act->mTarget, 11, TRUE, 1);
			} else if (chargeLevel == 2) {
				HeldAnim(act->mOwner, 0x8c3);
				HeldAnim(act->mTarget, 12, TRUE, 1);
			} else if (chargeLevel == 1) {
				HeldAnim(act->mOwner, 0x8c4);
				HeldAnim(act->mTarget, 13, TRUE, 1);
			} else if (count <= 1) {
				act->mOwner->mSquirterContent = 0;
				act->mOwner->mItemTintWord    = -1;
				HeldAnim(act->mOwner, 0x8c5);
				HeldAnim(act->mTarget, 14);
			}
		}
		act->mTarget->UpdateAnim(frames);
		SetLiftActionState(act, 36);
		act->mCheckToolHit = TRUE;
		break;
	}
	case 36:
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			bool finished = false;
			s16 count     = (s16)(GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38) - 1);
			if (anim == 0x8c2) {
				UseToolCharge(act, count, 3);
				finished = true;
			} else if (anim == 0x8c3) {
				UseToolCharge(act, count, 2);
				finished = true;
			} else if (anim == 0x8c4) {
				UseToolCharge(act, count, 1);
				finished = true;
			} else if (anim == 0x8c5) {
				HeldAnim(act->mTarget, 1);
				finished = true;
			} else if (anim == 0x8d2) {
				HeldAnim(act->mOwner, 0x8c0);
				HeldAnim(act->mTarget, 9);
			} else if (anim == 0x8c0) {
				HeldAnim(act->mOwner, 0x8d3);
				HeldAnim(act->mTarget, 18);
			} else if (anim == 0x8d3 || anim == 0x8c1) {
				HeldAnim(act->mTarget, 4);
				// filled up, the inventory keeps the count at 10 at most
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, 11, FALSE);
				finished = true;
			}
			act->mTarget->UpdateAnim(frames);
			if (finished) {
				HeldAnim(act->mOwner, 17);
				SetLiftActionState(act, 4);
				if (act->mToolEffect) {
					gGame.mParticle.Kill(act->mToolEffectId);
					act->mToolEffect = NULL;
				}
			}
		} else if (anim >= 0x8c2 && anim <= 0x8c5) {
			if (act->mCheckToolHit) {
				s32 first[]    = { 0, 0 };
				HSD_JObj* root = GetObjectJointByPath(act->mTarget, 2, first);
				s32 second[]   = { 0, 3 };
				HSD_JObj* tip  = GetObjectJointByPath(act->mTarget, 2, second);
				CVec pos(GetJointMatrixTranslation(act->mTarget, root));
				f32 distance;
				{
					CVec delta = pos - GetJointMatrixTranslation(act->mTarget, tip);
					distance   = PSVECMag(&delta);
				}
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
						if (anim != 0x8c5) {
							attachment         = root;
							act->mToolEffectId = 0x3c;
						} else if (frame > 10) {
							attachment         = tip;
							act->mToolEffectId = 0x42;
						}
						if (act->mToolEffectId >= 0)
							act->mToolEffect = CreateParticleByBankAndId(&gGame, 1, act->mToolEffectId, 0, attachment, TRUE);
						if (act->mToolEffect && anim != 0x8c5)
							psSetUserFunc(act->mToolEffect, &gPlayerItemParticleCallbacks);
					}
				}
			}
		} else {
			ApplyPlayerItemTintToTarget(act);
		}
		break;
	}
}

extern "C" {
}

extern "C" {
void ConfigureSubwindowCamera(FbSubwin* subwin, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov);

/**
 * @note Address: 0x800D3A5C
 * @note Size: 0x94
 */
HSD_Generator* CreateParticleAtPosition(CGame* game, u32 bank, u32 id, s32 status, const Vec& pos, BOOL keep)
{
	const Vec rotation = { 0.0f, 0.0f, 0.0f };
	const Vec scale    = { 1.0f, 1.0f, 1.0f };
	return game->mParticle.CreateSRT(id | (bank << 16), status, pos, scale, rotation, keep);
}

/**
 * @note Address: 0x800D3AF0
 * @note Size: 0xB4
 */
Vec GetJointMatrixScale(const CJObj*, HSD_JObj* jobj)
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
	const Vec rotation = { 0.0f, 0.0f, 0.0f };
	return game->mParticle.CreateSRT(id | (bank << 16), status, pos, scale, rotation, keep);
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
	game->mSubwin.mEnabled = TRUE;
	game->mSubwin.mFlags.Clear();
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

} // extern "C"

/**
 * @note Address: 0x800D3F14
 * @note Size: 0x1D8
 */
CXObj::CXObj(HSD_JObj* jobj)
    : CJObj(jobj)
{
	SyncMoveFromJoint(this);
}

extern "C" {

/**
 * @note Address: 0x800D40EC
 * @note Size: 0x164
 */
void SyncMoveFromJoint(CXObj* obj)
{
	Vec translate;
	Quaternion rotate;
	Vec scale;
	HSD_JObjGetTranslation(obj->mJObj, &translate);
	obj->mMoveTranslate.Reset(translate);
	HSD_JObjGetRotation(obj->mJObj, &rotate);
	obj->mMoveRotate.Reset(rotate.x, rotate.y, rotate.z);
	HSD_JObjGetScale(obj->mJObj, &scale);
	obj->mMoveScale.Reset(scale);
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
	const u32 particleId = static_cast<u32>(id) | (static_cast<u32>(bank) << 16);
	return game->mParticle.Create(particleId, type, jobj, keep);
}

/**
 * @note Address: 0x800D4330
 * @note Size: 0x10
 */
s32 GetInventoryCountByIndex(Params* params, u32 index)
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

} // extern "C"

extern "C" {
}

/**
 * @note Address: 0x800D4368
 * @note Size: 0x48C
 */
void XActLift::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	if (mState == 4) {
		XObjChr* chr = mOwner;
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
			moveWeight = 0.0f;
			return;
		}
		// a sets it down (6), b puts an item away (19) and sets anything else down
		if (pad.mTrigger & PAD_BUTTON_A) {
			if (mTarget == NULL) {
				return;
			}
			if (mTarget->GetCategory() == 8) {
				return;
			}
			if (chr->mFloorContacts[4].objectIndex >= 0 && (gGame.GetObject(chr->mFloorContacts[4].objectIndex)->mStatus.mFlags & 0x20)) {
				moveWeight = 0.0f;
				return;
			}
			if (mTarget->GetCategory() == 0x18) {
				return;
			}
			mState     = 6;
			moveWeight = 0.0f;
		} else if (pad.mTrigger & PAD_BUTTON_B) {
			if (mTarget != NULL) {
				if (mTarget->GetCategory() == 8) {
					if (chr->GetAttachment() == AttachmentNone) {
						mState = 19;
					}
				} else {
					mState = 6;
				}
			}
			moveWeight = 0.0f;
		}
	} else if (mState == 12 || mState == 13) {
		// a or b stops giving power and pulls the plug (14)
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && ((pad.mTrigger & PAD_BUTTON_A) || (pad.mTrigger & PAD_BUTTON_B))) {
			mState = 14;
			mTimer = 0.0f;
		}
		moveWeight = 0.0f;
	} else if (mState == 15) {
		if (gGame.IsRunning()) {
			if (pad.mStickX > 0.8f) {
				mState = 16;
				mTimer = 0.0f;
			} else if (pad.mTrigger & PAD_BUTTON_B) {
				mState = 17;
				mTimer = 0.0f;
			}
		}
		moveWeight = 0.0f;
	} else if (mState == 31 || mState == 32) {
		// he uses the mug for as long as a is held
		if (!(pad.mButton & PAD_BUTTON_A)) {
			mState = 33;
		}
		moveWeight = 0.0f;
	} else if (mState != 0 && mState != 27 && mState != 28) {
		moveWeight = 0.0f;
	}
}

// the item particles' color callback, they take his item tint
/**
 * @note Address: 0x800D47F4
 * @note Size: 0x38
 */
extern "C" {

int ApplyPlayerItemParticleColor(HSD_Particle* particle)
{
	const GXColor& color = gGame.GetPlayer()->mItemTint;
	particle->primCol.r  = color.r;
	particle->primCol.g  = color.g;
	particle->primCol.b  = color.b;
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
				tobj->flags = (tobj->flags & ~TEX_COLORMAP_MASK) | TEX_COLORMAP_PASS;
			}
			HSD_MObjCompileTev(mobj);
		}
	}
}

/**
 * @note Address: 0x800D4998
 * @note Size: 0x1C8
 */
void PutAwayLiftedObject(XActLift* act)
{
	XObjChr* chr = act->mOwner;
	if (chr->mAnimId == 0x8B1 || chr->mAnimId == 0x8B3) {
		chr->SetAnim(0x8B2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	} else if (chr->mAnimId == 11) {
		chr->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	}
	act->mState              = 0;
	act->mOwner->mAttachment = AttachmentNone;
	act->mTarget->mStatus.mFlags &= ~0x0200000000000000ULL;
	act->mTarget->SetAnim(7, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	act->mTargetState = 20;
	XObjItem* item    = static_cast<XObjItem*>(act->mTarget);
	SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, item->GetItem(), 1, TRUE);
	item->SetFlag();
	PlayGameSound(&gGame.mSound, 0x39, 0xFF);
}

static bool IsCarryingTool(const XActLift& action, s16 item)
{
	const XObj* target = action.mTarget;
	return target != NULL && (target->mStatus.mFlags & 0x400000000000ULL) && target->GetCategory() == 8 && target->GetItem() == item
	    && action.mTargetState != 15;
}

/**
 * @note Address: 0x800D4B60
 * @note Size: 0x118
 */
BOOL IsUsingMug(XActLift* act)
{
	return IsCarryingTool(*act, 0x16) && (act->mState >= 31 && act->mState <= 33);
}

/**
 * @note Address: 0x800D4C78
 * @note Size: 0xDC
 */
BOOL IsCarryingSquirter(XActLift* act)
{
	return IsCarryingTool(*act, 0x38);
}

/**
 * @note Address: 0x800D4D54
 * @note Size: 0xDC
 */
BOOL IsCarryingMug(XActLift* act)
{
	return IsCarryingTool(*act, 0x16);
}

/**
 * @note Address: 0x800D4E30
 * @note Size: 0xDC
 */
BOOL IsCarryingSpoon(XActLift* act)
{
	return IsCarryingTool(*act, 0x15);
}

/**
 * @note Address: 0x800D4F0C
 * @note Size: 0xDC
 */
BOOL IsCarryingToothbrush(XActLift* act)
{
	return IsCarryingTool(*act, 0x10);
}

/**
 * @note Address: 0x800D4FE8
 * @note Size: 0x4C
 */
// NONMATCHING: MWCC folds the two failure exits into beqlr/bnelr; retail branches to the shared clrlwi/blr return.
BOOL IsLiftTargetPlug(XActLift* act)
{
	bool result = false;
	if (act->mTarget != NULL && act->mTarget->GetCategory() == 24) {
		result = true;
	}
	return result;
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
	Vec position       = Vec();
	const XObj* target = act->mTarget;
	if (target != NULL) {
		const XObjFloor& floor = target->mFloors[act->mTargetFloor];
		const Vec offset       = target->mMoveScale * ReadObjectFloorOffset(floor);
		const Vec extent       = target->mMoveScale * ReadObjectFloorExtent(floor);
		const f32 yaw          = target->mMoveRotate.y + DEG2RAD * floor.mYawDegrees;
		Mtx transform;
		MakeFloorTransform(target, offset, extent, yaw, transform);
		PSMTXMultVec(transform, &position, &position);
	}
	return position;
}

} // extern "C"

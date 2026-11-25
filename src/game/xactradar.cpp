#include "game/xchr.h"
#include "game/aimpitch.h"
#include <stdlib.h>
#include "game/video.h"
#include "game/cgame.h"
#include "game/cvec.h"

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

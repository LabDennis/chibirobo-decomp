#include "game/xchr.h"
#include "game/objectfloorquery.h"
#include <stdlib.h>
#include "game/video.h"
#include "game/xobjflags.h"
#include "game/cgame.h"
#include "game/stagedata.h"
#include "game/cjobjset.h"
#include "game/cvec.h"
#include "game/globals.h"
#include "game/hsd_dobj.h"

// chibi robo's radar, popper, copter and tumble actions

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
	for (int turn = 0; turn < 2; ++turn) {
		for (int step = 0; step < 16; ++step) {
			colors[n]     = base | (u8)(s32)(150.f * ((1.f / 64) * n) * act->mFade);
			colors[n + 1] = base;
			n += 2;
		}
	}
	for (int turn = 0; turn < 6; ++turn) {
		for (int step = 0; step < 16; ++step) {
			colors[n]     = base | (u8)(s32)(150.f * (1.f - (1.f / 192) * (turn * 32 + step)) * act->mFade);
			colors[n + 1] = base;
			n += 2;
		}
	}
	return n;
}

extern "C" void DrawRadarTargetBeam(XActRadar* act)
{
	if (!(act->mFade > 0))
		return;
	CVec origin = GetRadarDishWorldPosition(&act->mOwner->mRadarObj);
	Vec offset  = { 0, 0, 0 };
	offset.y    = act->mTarget->GetCollisionHeight() * .5f;
	CVec target = act->mTarget->mMoveTranslate + offset;
	f32 yaw     = atan2(target.x - origin.x, target.z - origin.z);
	f32 dx = target.x - origin.x, dy = target.y - origin.y, dz = target.z - origin.z;
	f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
	pitch     = dy < 0 ? HALF_PI - pitch : pitch - HALF_PI;
	pitch -= DEG2RAD * 3;
	f32 span = PSVECDistance(&origin, &target) / 20;
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
			f32 angle = (TWO_PI / 16) * step;
			f32 twist = (TWO_PI / 16) * turn + act->mAngle[1];

			Vec& p   = vertices[n];
			p.x      = (.007f * n + 1) * ((.1f * (f32)sin(twist) + 1) * (f32)sin(angle));
			p.y      = (.007f * n + 1) * ((f32)cos(angle) * (.1f * (f32)sin((TWO_PI / 16) * turn + act->mAngle[1]) + 1));
			p.z      = z;
			f32 wave = WrapAngle(phase);
			if (wave > PI)
				wave = 0;
			Vec& edge = vertices[(int)n + 1];
			edge.x    = p.x - ((f32)sin(angle) * (.5f * (f32)sin(wave)));
			edge.y    = p.y - ((f32)sin(angle) * (.5f * (f32)sin(wave)));
			edge.z    = p.z - 1.5f * (f32)sin(wave);
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
		act->mAngle[0] -= 0.08726646f * frames;
		act->mAngle[1] -= 0.08726646f * frames;
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
	owner->mStandTimer = 3;
	act->mState        = 0;
	act->mTarget       = NULL;
	act->mYaw.angle = act->mYaw.targetAngle = act->mYaw.reached = 0;
	act->mPitch.angle = act->mPitch.targetAngle = act->mPitch.reached = 0;
	StopRadarTones();
}

static inline f32 TargetPitch(const Vec& from, const Vec& to)
{
	f32 dz = to.z - from.z, dx = to.x - from.x, dy = to.y - from.y;
	f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
	return dy < 0 ? HALF_PI - pitch : pitch - HALF_PI;
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
		owner->mStandTimer = 3;
		StartPlayerRadarScan(&act->mOwner->mRadarObj);
		act->mState  = 4;
		act->mTarget = FindNearestRadarTarget(act);
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

static inline void PopperMuzzle(XObjChr* p, Vec& out)
{
	if (gGame.mStatus.mFlags & 0x100) {
		CCamera& camera = gGame.mMotionControlCamera.mCamera;
		Mtx y, x, m;
		PSMTXRotRad(y, 'y', camera.mAngle.y);
		PSMTXRotRad(x, 'x', camera.mAngle.x);
		PSMTXConcat(y, x, m);
		Vec offset = { 1.087597f, -1.056235f, 4.978912f };
		PSMTXMultVec(m, &offset, &offset);
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
			StartPopperShot(&act->mShots.mShot[i], origin, kind, target, yaw, pitch, power, speed, range, act->_2C);
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
	if (act->mOwner->GetAttachment() == 2) {
		CPad pad(0);
		XObjChr* p = act->mOwner;
		if ((pad.mPad->mButton & 0x100) && canCharge && p->mPopperObj.mAnimId == 4 && !gGame.mEvt.mData.mScript
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
			if (act->mState == 4)
				act->mState = charge.mTimer >= 1 ? 6 : 0;
			ClearPopperCharge(&charge);
		}
	}
	act->_2C = 0;
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
			act->mState = 7;
			act->_2C    = (s32)GetPlayerActionParameter(30) / 100.f;
		}
	}
	// fall through
	case 6:
	case 7:
		if (anim != 0x823 || act->_2C != 0) {
			if (act->mState != 6)
				act->mState = 8;
			s32 limit = GetPopperParameter(3);
			if (ActivePopperShots(act) < limit) {
				PlayPopperFireAnimation(&act->mOwner->mPopperObj);
				if (act->_2C == 0) {
					if (act->mState == 6)
						act->_2C = (s32)GetPlayerActionParameter(31) / 100.f;
					else
						act->_2C = (s32)GetPlayerActionParameter(30) / 100.f;
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
				if (p->_158A4) {
					f32 battery = p->mParams.mBattery - act->_2C;
					if (battery < 0)
						battery = 0;
					p->mParams.mBattery = battery > 0 ? battery : 0;
					if (p->mParams.mBattery < 0)
						p->mParams.mBattery = 0;
					if (p->mParams.mBattery > p->mParams.mMaxBattery)
						p->mParams.mBattery = p->mParams.mMaxBattery;
				}
				s32 kind = GetAttachmentCountById(&act->mOwner->mAttachments, 2);
				if (target)
					StartAvailablePopperShot(act, origin, kind, target, yaw, pitch, power, speed, range);
				else
					StartAvailablePopperShot(act, origin, kind, NULL, yaw, pitch, power, speed, range);
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
			p->mStandTimer = 3;
			act->mState    = 0;
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
void UpdatePopperShotFlight(XActPopperShot* shot, u32 frames)
{
	if (shot->mActive) {
		XObj* target = shot->mHomingTarget;
		if (target != NULL) {
			Vec aim = AddScalarToVectorComponents(target->mMoveTranslate, target->GetCollisionHeight() * 0.5f);
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
		Vec step    = { 0.0f, 0.0f, 0.0f };
		step.z      = shot->mSpeed * frames;
		Vec forward = step;
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
		f32 angle    = DEG2RAD * (rand() % 360);
		f32 sinAngle = (f32)sin(angle);
		f32 vx       = (sinAngle * (rand() % 100)) / 200.0f;
		f32 cosAngle = (f32)cos(angle);
		f32 vz       = (cosAngle * (rand() % 100)) / 200.0f;
		f32 vy       = (50 + rand() % 80) / 100.0f;
		f32 width    = (200 + rand() % 300) / 500.0f;
		f32 height   = (10 + rand() % 200) / 500.0f;
		Vec velocity = { 0.0f, 0.0f, 0.0f }, size = { 0.0f, 0.0f, 1.0f };
		velocity.x      = vx;
		velocity.y      = vy;
		velocity.z      = vz;
		size.x          = width;
		size.y          = height;
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
// NONMATCHING: the position is read once too few
void StartPopperShot(XActPopperShot* shot, const Vec& pos, s32 kind, XObj* target, f32 a, f32 b, f32 c, f32 d, f32 e, f32 strength)
{
	shot->mActive    = TRUE;
	shot->mHitTarget = NULL;
	shot->mPos       = pos;
	shot->mYaw       = a;
	shot->mPitch     = b;
	shot->mSpeed     = d;
	shot->mDistance  = 0.0f;
	shot->mRange     = e;
	shot->_28        = c;
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
	if (act->mState == 0) {
		act->mTime = FRAME_RATE * ((s32)GetCopterParameter(0) / 100.0f);
	} else if (act->mState == 3) {
		if ((act->mTime -= frames) < 0.0f) {
			act->mTime  = 0.0f;
			act->mState = 4;
		}
	}
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
			chr->mStandTimer = 3;
			act->mState      = 0;
		}
		break;
	case 3:
		UpdateCopterBobbing(act, frames, 0.08726646f);
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
			chr->mStandTimer = 3;
			act->mState      = 0;
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
			chr->mStandTimer = 3;
			act->mState      = 0;
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
		act->mPhase = fmod(speed * frames + act->mPhase, 6.2831855f);
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
		f32 outer = size * 0.5f + 1.0f;
		for (int i = 0; i < 25; i++) {
			f32 a                  = 0.2617994f * i;
			verts[i * 2].pos.x     = center.x - size * (f32)sin(a) * 0.5f;
			verts[i * 2].pos.z     = center.z + size * (f32)cos(a) * 0.5f;
			verts[i * 2].pos.y     = center.y;
			verts[i * 2].color     = color;
			verts[i * 2 + 1].pos.x = center.x - outer * (f32)sin(a);
			verts[i * 2 + 1].pos.z = center.z + outer * (f32)cos(a);
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
	PlayTumbleAnimation(owner, 17, blend);
	owner->mStandTimer = 3;
	act->mState        = 0;
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
void UpdatePlayerTumbleAnimation(XActTumble* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
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
		if (act->mOwner->_158BC > 0) {
			if (!act->mIdleParticle && TumbleStationary(act->mOwner->mMoveTranslate - (Vec)act->mOwner->_264)) {
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
		if (act->mOwner->_158BC <= 0)
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
	if (act->mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if ((pad->mTrigger & 0x100) || (pad->mTrigger & 0x200) || StickLength(pad) > 0.0f) {
				act->mState = 3;
			}
		}
		*weight = 0.0f;
	} else if (act->mState == 0x10) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if ((pad->mTrigger & 0x100) || (pad->mTrigger & 0x200) || StickLength(pad) > 0.0f) {
				act->mState = 0x11;
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

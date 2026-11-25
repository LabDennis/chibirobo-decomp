#include "game/xchr.h"
#include "game/aimpitch.h"
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
				StartAvailablePopperShot(act, origin, kind, target, yaw, pitch, power, speed, range);
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

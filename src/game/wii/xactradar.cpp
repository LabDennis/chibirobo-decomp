// chibi robo's tumble fade and the ring on the floor around him, the GameCube's
// xactradar.cpp tail: retail 0x801078C8-0x80107D40, right behind the tumble actions in
// wii/xacttumble.cpp. The fade struct is the GameCube's, the draws go through the Wii
// CGs layer; HandleCopterActionInput reads the remote's B button through gWiiPointer
// where the GameCube read the pad trigger, and plays the descent animation on the
// copter object at _B580.
#include "arithmetic.h"
#include "game/wii/cgs.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/xact.h"
#include "game/wii/ceffect.h"
#include "game/globals.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>

extern "C" BOOL IsPlayerStopped(const WiiPlayerState* chr);
extern "C" void ShowAndDeployCopter(XObj* copter);
extern "C" void PlayCopterStowAnimation(XObj* copter);
extern "C" void PlayCopterDescentAnimation(XObj* copter);
extern "C" void PlayCopterLandingAnimation(XObj* copter);
extern "C" CVec GetPlayerFloorSlope(WiiPlayerState* chr);
extern "C" u32 GetCopterParameter(s32 index);
extern "C" s32 gRefreshRate;
extern "C" EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group); // takes a free slot; ids 0x4E-0x50 here
extern "C" void fn_80108BCC(XActPopperShot* shot);                                  // the shot's impact-detection pass (scan + floor probe)
extern "C" void CheckPopperShotImpact(XActPopperShot* shot, const Vec* pos, BOOL ended);
extern "C" BOOL fn_800C98E0(XObjMap* map, const Vec* pos, s32 start, XObjMapHit* hit); // the Wii's out-of-line FindMapFloor

// the tumble ring sits just above the floor
static inline Vec RingPos(const Vec& pos, const Vec& offset)
{
	Vec v = { pos.x + offset.x, 0.01f + pos.y, pos.z + offset.z };
	return v;
}

// retires the instance's effect when there is one
static inline void KillInstance(EffectInstance* inst)
{
	if (inst != NULL) {
		RetireEffect(&inst->handle, FALSE);
	}
}

// the GameCube's KillGen: on the Wii a generator is an event-effect instance, so the
// retail code shows the caller's own guard in front of the guarded call
static inline void KillGen(EffectInstance*& gen)
{
	if (gen != NULL) {
		KillInstance(gen);
		gen = NULL;
	}
}

// the GameCube objectfloorquery.h inline: the Wii keeps the reference signature and
// forwards to the pointer-taking virtual, so the CVec arguments materialize as
// temporaries on the caller's stack
static inline u32 ProbeObjectFloor(XObj* obj, const CVec& from, const CVec& to, f32 radius, f32 height, Vec* push, Vec* contact, s32* floor)
{
	return obj->TestFloorCollision(&from, &to, radius, height, push, contact, floor, 0);
}

// clears a shot and its effects, the GameCube's ResetShot
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

// turns an angle towards a target by up to 14 degrees per frame, the GameCube's
// homing steer; the short way round is taken when the wrapped delta passes pi
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

// fades a landing ring out over time, the GameCube's FadePopperRing
static inline void FadePopperRing(XActPopperRing& ring, u32 frames)
{
	if (ring.mAmount > 0.0f && (ring.mAmount -= 0.05f * frames) < 0.0f)
		ring.mAmount = 0.0f;
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
void UpdatePlayerCopterFlight(XActCopter* act, s32 anim, WiiPlayerState* owner, u32 frames, f32 frame, f32 end)
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
		if (act->mOwner->GetHeight() <= 0.0f || (act->mOwner->GetHeight() <= 1.0f && GetPlayerFloorSlope(act->mOwner).Length() > 0.5f)) {
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

// resets one shot and readies its pool and sound actors, the per-shot body of the
// GameCube's InitializePlayerPopperAction loop on a function of its own
/**
 * @note Address: 0x801085C8
 * @note Size: 0x140
 */
void InitPopperShot(XActPopperShot* shot)
{
	ResetShot(shot);
	InitParticlePool(&shot->mDust.mPtcl, shot->mDust.mPtcls, sizeof(CPtclDust2), 40);
	shot->mRing[0].mAmount = 0.0f;
	shot->mRing[1].mAmount = 0.0f;
	shot->mGen             = NULL;
	shot->mChargeGen       = NULL;
	for (s32 i = 0; i < 2; i++) {
		shot->mSfxActors[i].Reset();
	}
	for (s32 i = 0; i < 2; i++) {
		shot->mSfxActors[i].Create();
	}
}

/**
 * @note Address: 0x80108708
 * @note Size: 0x88
 */
void ResetPopperShot(XActPopperShot* shot)
{
	ResetShot(shot);
}

// fires one shot; kind 1 is the small one. Same field order as the GameCube version:
// on the Wii the joint-attached particles are event-effect instances (pos/rot zero,
// scale one, like the tumble effects) and the launch sound plays through both the
// shot's 3D actor and the flat PlayWiiSoundEffect while the archive mReady flag is 2
/**
 * @note Address: 0x80108790
 * @note Size: 0x43C
 */
// NONMATCHING: 68.27% - the semantics match retail field-for-field (same ids, same TRS
// {0,0,0}/{0,0,0}/{1,1,1} effect writes, same mReady=2/PlayWiiSoundEffect/mReady=1,
// same actor detail_StartSound + SetVolume, same trailing impact-detection call). The
// residue is stack-frame codegen: retail keeps a bytewise Vec stack copy of pos
// (stw-bit-copy + lfs/frsp reload into mPos) plus two separate TRS triples for the
// two effect calls - frame 0xC0 with f28-f31 + r28-r31 saved; this source compiles to
// a 0x70 frame with shared temporaries. Tested: explicit CVec(pos) temp and
// per-call-site Vec triples, both scored lower.
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
	Vec zero   = { 0.0f, 0.0f, 0.0f };
	Vec unit   = { 1.0f, 1.0f, 1.0f };
	shot->mGen = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, kind == 1 ? 0x4E : 0x4F, 1);
	if (shot->mGen != NULL) {
		shot->mGen->pos   = zero;
		shot->mGen->rot   = zero;
		shot->mGen->scale = unit;
		UpdateEffectMtx(shot->mGen);
	}
	if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f) {
		shot->mChargeGen = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x50, 1);
		if (shot->mChargeGen != NULL) {
			shot->mChargeGen->pos   = zero;
			shot->mChargeGen->rot   = zero;
			shot->mChargeGen->scale = unit;
			UpdateEffectMtx(shot->mChargeGen);
		}
	}
	f32 full             = (s32)GetPlayerActionParameter(31) / 100.0f;
	s32 sfx              = strength >= full ? (kind == 1 ? 0x200 : 0x203) : (kind == 1 ? 0x78 : 0x202);
	CSound3DActor* actor = shot->mSfxActors[0].mActor;
	if (actor != NULL && actor->detail_StartSound(&actor->mHandle, sfx, NULL) == nw4r::snd::SoundStartable::START_SUCCESS) {
		actor->mHandle.SetVolume(1.0f, 0);
	}
	gGame.mSound.mArchivePlayer.mReady = 2;
	PlayWiiSoundEffect(&gGame.mSound, sfx, 1.0f, 0.0f);
	gGame.mSound.mArchivePlayer.mReady = 1;
	fn_80108BCC(shot);
}

// NONMATCHING: 99.02% - every instruction and the control flow match; the only
// residue is register allocation order: retail binds i, ended, hit, the object-table
// walker and object to r24-r28 in that order while we get object, i, ended, hit,
// walker. Same rotation for f26/f27 on the GetPos loads. Declaring i outside the
// loop and swapping dx/dz were tried and moved nothing.

// the detection half of the GameCube's CheckPopperShotImpact, split off on the Wii:
// scans the live objects in reach and the stage map for the first thing the shot
// touches, snaps the position to the contact and leaves the sparks, the sound and the
// ring to CheckPopperShotImpact
/**
 * @note Address: 0x80108BCC
 * @note Size: 0x380
 */
void fn_80108BCC(XActPopperShot* shot)
{
	Vec pos  = shot->mPos;
	BOOL hit = FALSE, ended = FALSE;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.mXObjs.mObjects[i];
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000) || (object->mStatus.mFlags & 0x40000)
		    || (object->mStatus.mFlags & 0x0002000000000000ULL) || (object->mStatus.mFlags & 0x100000))
			continue;
		CVec* objPos = object->GetPos();
		f32 dz = objPos->z - pos.z, dx = objPos->x - pos.x;
		if (FSqrt(dx * dx + dz * dz) > 100.0f + (0.8f + object->mFloorXZExtent[0]))
			continue;
		CVec push, contact;
		u32 touch = ProbeObjectFloor(object, CVec(pos.x, pos.y, pos.z), CVec(pos.x, pos.y, pos.z), 0.8f, 0.8f, &push, &contact, NULL) & 3;
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
		XObjMap* map = (XObjMap*)gGame.mXObjs.mPools[3].mObjects;
		if (fn_800C98E0(map, &pos, -1, &ground) && pos.y <= ground.mHeight) {
			hit   = TRUE;
			ended = TRUE;
			shot->mPos.Set(Vec(pos));
			shot->mPos.y = ground.mHeight;
		}
	}
	if (!hit)
		return;
	CheckPopperShotImpact(shot, &shot->mPos, ended);
	KillGen(shot->mGen);
	KillGen(shot->mChargeGen);
}

// NONMATCHING: 98.74% - the whole spark loop, the stack Vec literals and the ended
// block match. The residue is one reload: retail reads mSfxActors[1].mActor twice
// (into r3 for the GetHandle test, into r21 for the start block), our compiler keeps
// it live in r21 through both; a named AnimSfxActor* local changed nothing.

// the resolution half of the GameCube's CheckPopperShotImpact, split off on the Wii:
// twenty sparks around the hit, the impact sound through the shot's second actor when
// it isn't playing already, and the landing ring when the shot is done. The GameCube
// version played the sound flat through PlayGameSound and retired the generators
// itself; the Wii leaves the KillGen pair to the caller, fn_80108BCC
/**
 * @note Address: 0x80108F4C
 * @note Size: 0x404
 */
void CheckPopperShotImpact(XActPopperShot* shot, const Vec* pos, BOOL ended)
{
	for (s32 i = 0; i < 20; ++i) {
		f32 angle       = DEG2RAD * (rand() % 360);
		f32 sinAngle    = std::sinf(angle);
		f32 vx          = (sinAngle * (rand() % 100)) / 200.0f;
		f32 cosAngle    = std::cosf(angle);
		f32 vz          = (cosAngle * (rand() % 100)) / 200.0f;
		f32 vy          = (50 + rand() % 80) / 100.0f;
		f32 width       = (200 + rand() % 300) / 500.0f;
		f32 height      = (10 + rand() % 200) / 500.0f;
		CPtclOne* spark = SpawnPoolParticle(&shot->mDust.mPtcl, pos, &(Vec) { width, height, 1.0f }, &(Vec) { vx, vy, vz }, 0xC8C8FFFF);
		if (!spark)
			break;
		spark->mAlpha      = (192 + (rand() & 63)) / 255.0f;
		spark->mAlphaStep  = 0.03f;
		spark->mAngleSpeed = DEG2RAD * ((rand() & 63) - 32);
	}
	u32 sfx = shot->mKind == 1 ? 0x201 : 0x204;
	if (shot->mSfxActors[1].GetHandle() == NULL) {
		CSound3DActor* actor = shot->mSfxActors[1].mActor;
		if (actor != NULL && actor->StartSound(&actor->mHandle, sfx)) {
			actor->mHandle.SetVolume(1.0f, 0);
		}
	}
	if (ended) {
		shot->mActive          = FALSE;
		shot->mRing[1].mPos    = *pos;
		shot->mRing[1].mAmount = 0.9f;
		shot->mRing[1].mRadius = 100.0f;
		shot->mRing[1].mColor  = 0x6464FF96;
	}
}

CVec AddScalarToVectorComponents(const CVec& v, f32 d);

// NONMATCHING: 96.9% - the call sequence (GetCollisionHeight, GetPos, atan2,
// PSMTX*, UpdateEffectMtx, RetireEffect, fn_80108BCC, UpdateParticlePool,
// SetPosition) and all branches match; the ~3% residue is in instruction
// scheduling and stack slot choice, undiagnosed this run.

// steps one shot through the air, the GameCube's UpdatePopperShotFlight: the homing
// steer on the target, the ballistic advance, the range check, the trail history
// and the dust pool. On the Wii the generators are event-effect instances dragged
// to the new position and rotation instead of a JObj joint, and the shot's two
// sound actors follow it while their fades step
/**
 * @note Address: 0x80109350
 * @note Size: 0x8C8
 */
void UpdatePopperShotFlight(XActPopperShot* shot, u32 frames)
{
	if (shot->mActive) {
		XObj* target = shot->mHomingTarget;
		if (target != NULL) {
			Vec aim = AddScalarToVectorComponents(*target->GetPos(), target->GetCollisionHeight() * 0.5f);
			SteerPopperAngle(shot->mYaw, (f32)atan2(aim.x - shot->mPos.x, aim.z - shot->mPos.z), frames);
			f32 dz    = aim.z - shot->mPos.z;
			f32 dx    = aim.x - shot->mPos.x;
			f32 dy    = aim.y - shot->mPos.y;
			f32 pitch = atan2(FSqrt(dx * dx + dz * dz), FSqrt(dy * dy));
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
		if (shot->mGen != NULL) {
			shot->mGen->pos = shot->mPos;
			UpdateEffectMtx(shot->mGen);
			Vec rot         = { 0.0f, 0.0f, 0.0f };
			rot.x           = shot->mPitch;
			rot.y           = shot->mYaw;
			shot->mGen->rot = rot;
			UpdateEffectMtx(shot->mGen);
		}
		if (shot->mChargeGen != NULL) {
			shot->mChargeGen->pos = shot->mPos;
			UpdateEffectMtx(shot->mChargeGen);
			Vec rot               = { 0.0f, 0.0f, 0.0f };
			rot.x                 = shot->mPitch;
			rot.y                 = shot->mYaw;
			shot->mChargeGen->rot = rot;
			UpdateEffectMtx(shot->mChargeGen);
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
			fn_80108BCC(shot);
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
	for (s32 i = 0; i < 2; ++i) {
		CSound3DActor* actor = shot->mSfxActors[i].mActor;
		if (actor != NULL && actor->mHandle.IsAttachedSound()) {
			actor->SetPosition(nw4r::math::VEC3(shot->mPos));
			actor->UpdateFade(frames);
		}
	}
}

// NONMATCHING: 58.4% - the GameCube's exact body, but GC/3.0a5 folds the local
// CVec into the sret slot while retail keeps it as a real stack temp (frame 0x20,
// word copies to the temp, float read-modify-writes on it, then a word copy plus
// vtable store to the out object). `t = v` assignment and `return CVec(t)` fold
// the same way; the older GC compiler emits retail's shape for this exact source.

// the GameCube's AddScalarToVectorComponents, the homing aim's lift over the
// target's center
/**
 * @note Address: 0x80109C18
 * @note Size: 0x70
 */
CVec AddScalarToVectorComponents(const CVec& v, f32 d)
{
	CVec t(v);
	t.x += d;
	t.y += d;
	t.z += d;
	return t;
}

} // extern "C"

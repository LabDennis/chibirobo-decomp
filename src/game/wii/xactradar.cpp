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
#include "game/xobjattachment.h"
#include "game/wii/ceffect.h"
#include "game/globals.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>

extern "C" BOOL IsPlayerStopped(const WiiPlayerState* chr);
extern "C" void ShowAndDeployCopter(XObjCopter* copter);
extern "C" void PlayCopterStowAnimation(XObjCopter* copter);
extern "C" void PlayCopterDescentAnimation(XObjCopter* copter);
extern "C" void PlayCopterLandingAnimation(XObjCopter* copter);
extern "C" CVec GetPlayerFloorSlope(WiiPlayerState* chr);
extern "C" u32 GetCopterParameter(s32 index);
extern "C" u32 gRefreshRate;
extern s32 gScreenLeft, gScreenRight, gScreenTop, gScreenBottom;
extern "C" EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group); // takes a free slot; ids 0x4E-0x50 here
extern "C" void fn_80108BCC(XActPopperShot* shot);                                  // the shot's impact-detection pass (scan + floor probe)
extern "C" void CheckPopperShotImpact(XActPopperShot* shot, const Vec* pos, BOOL ended);
extern "C" BOOL fn_800C98E0(XObjMap* map, const Vec* pos, s32 start, XObjMapHit* hit); // the Wii's out-of-line FindMapFloor
extern "C" void ShowAndDeployPopper(XObjPopper* popper);
extern "C" void PlayPopperStowAnimation(XObjPopper* popper);
extern "C" void PlayPopperRaiseAnimation(XObjPopper* popper);
extern "C" void PlayPopperFireAnimation(XObjPopper* popper);
extern "C" BOOL IsPopperFireAnimationSelected(XObjPopper* popper);
extern "C" void PlayPopperLowerAnimation(XObjPopper* popper);
extern "C" void ShowIdlePlayerPopper(XObjPopper* popper);
extern "C" void ActivatePlayerRadar(XObjRadar* radar);
extern "C" void DeactivatePlayerRadar(XObjRadar* radar);
extern "C" void StartPlayerRadarScan(XObjRadar* radar);
extern "C" void ShowIdlePlayerRadar(XObjRadar* radar);
extern "C" void ShowOXCircle(XObjOX* obj);
extern "C" void ShowOXCross(XObjOX* obj);
extern "C" CVec GetRadarDishWorldPosition(XObjRadar* obj);
extern "C" XObj* FindNearestRadarTarget(XActRadar* act);
extern "C" void fn_8010D094(XActRadar* act, s8 near, s8 mid, s8 far);
extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);
extern "C" void HSD_MtxGetScale(Mtx m, Vec* dst);
u32 GetRadarParameter(s32 index);
struct AttachmentValues;
extern "C" s32 GetAttachmentCountById(AttachmentValues* values, s32 id); // below 0 when there is none
extern "C" void PSMTX44MultVec(const Mtx44 m, const Vec* src, Vec* dst);
extern XObj* lbl_80453508; // the aim target CGame::Update locks while the pointer rests on it; its timer sits right behind

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
	f32 pitch = atan2(FSqrt(dx * dx + dz * dz), FSqrt(dy * dy));
	return dy < 0 ? HALF_PI - pitch : pitch - HALF_PI;
}

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

void ResetPlayerRadarAction(XActRadar* act);

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
			PlayCopterDescentAnimation((XObjCopter*)act->mOwner->mCopterObj);
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
			ShowAndDeployCopter((XObjCopter*)act->mOwner->mCopterObj);
		} else if (owner->IsAnimDone()) {
			act->mOwner->SetAnim(0x4A, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = 0;
		}
		break;
	case 2:
		if (anim != 0x817) {
			act->mOwner->SetAnim(0x817, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayCopterStowAnimation((XObjCopter*)act->mOwner->mCopterObj);
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
			PlayCopterDescentAnimation((XObjCopter*)act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x816, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObjCopter*)act->mOwner->mCopterObj);
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
			PlayCopterLandingAnimation((XObjCopter*)act->mOwner->mCopterObj);
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
			PlayCopterDescentAnimation((XObjCopter*)act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x59, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObjCopter*)act->mOwner->mCopterObj);
			act->mState = 9;
		}
		break;
	case 8:
		if (anim != 0x1A) {
			act->mOwner->SetAnim(0x1A, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayCopterDescentAnimation((XObjCopter*)act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x1B, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObjCopter*)act->mOwner->mCopterObj);
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

// NONMATCHING: 96.2% - same instruction count and frame; the residue is register
// allocation. Retail strength-reduces mRing[j] into three cursors (r29 = &mPos,
// r28 = &mAmount for the three body reads, r30 = shot for guard/radius/color) and
// so needs _savegpr_18; we emit two (pos + base) and land on _savegpr_19, shifting
// every callee colour by one. The mAmount*mRadius / (1-amount)*radius operand
// order is retail's (the GameCube source multiplies the other way round). A named
// XActPopperRing* local scored lower (one cursor, 96.08).

// one shot's two rings, drawn only while the projection lands inside the frame's
// bounds and a 0..1 depth. The GameCube's DrawPopperShotRings walked all ten shots
// itself; on the Wii the ten-step loop is a caller of its own at 0x8010BE94 and this
// draws a single shot, the ring math and colors unchanged
/**
 * @note Address: 0x80109C88
 * @note Size: 0x344
 */
void DrawPopperShotRings(XActPopperShot* shot)
{
	for (s32 j = 0; j < 2; j++) {
		if (shot->mRing[j].mAmount > 0.0f) {
			f32 x, y, z;
			gGame.ProjectOffsetToScreen(&shot->mRing[j].mPos, &x, &y, &z, NULL);
			if (x >= gScreenLeft && x < gScreenRight && y >= gScreenTop && y < gScreenBottom && z > 0.0f && z < 1.0f) {
				u32 color  = shot->mRing[j].mColor;
				u32 edge   = color & 0xFFFFFF00;
				u32 center = edge | (u8)((color & 0xFF) * shot->mRing[j].mAmount);
				f32 inner  = shot->mRing[j].mAmount * shot->mRing[j].mRadius;
				f32 outer  = (1.0f - shot->mRing[j].mAmount) * shot->mRing[j].mRadius;
				gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCCOL, GX_BL_SRCALPHA, GX_LO_SET);
				DrawGsCircle(&gGame.mGs, 16, (u32)(255.0f * shot->mRing[j].mAmount) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, 2.0f * inner,
				             2.0f * inner, 0.0f, 0.0f, 0.0f);
				DrawGsCircle(&gGame.mGs, 16, (u32)(128.0f * shot->mRing[j].mAmount) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, 1.5f * inner,
				             1.5f * inner, 0.0f, 0.0f, 0.0f);
				gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
				DrawGsCircle(&gGame.mGs, 16, edge, center, x, y, 0.0f, outer, outer, 0.0f, 0.0f, 0.0f);
			}
		}
	}
}

// the GameCube's ClearPopperCharge on the Wii's grown charge: retires the two effect
// instances, fades both sound actors out instantly (the retail math is
// gRefreshRate * 0.0f frames) and stops the two charge sounds by their Wii ids
static inline void ClearPopperCharge(XActPopperCharge* charge)
{
	charge->mState = 0;
	charge->mTimer = 0.0f;
	charge->mMtx   = NULL;
	KillGen(charge->mGen);
	KillGen(charge->mChargeGen);
	for (s32 i = 0; i < 2; ++i) {
		CSound3DActor* actor = charge->mSfxActors[i].mActor;
		if (actor != NULL) {
			actor->mHandle.Stop((s32)(gRefreshRate * 0.0f));
		}
	}
	StopWiiSfx(&gGame.mSound, 0x1F3, 0.0f);
	StopWiiSfx(&gGame.mSound, 0x1F8, 0.0f);
}

// NONMATCHING: retail re-evaluates (s32)(gRefreshRate * 0.0f) per actor instead of
// folding it to 0 the way this build does
/**
 * @note Address: 0x80109FCC
 * @note Size: 0x148
 */
void ResetPopperCharge(XActPopperCharge* charge)
{
	ClearPopperCharge(charge);
}

// steps the muzzle glow while a shot is charged: the timer fills at one per refresh
// frame, the two generators sit on the joint's matrix column or get dragged to mPos,
// the full-charge glow and its loop sound appear at 1.0, and both actors follow the
// muzzle while their fades step. The GameCube folded this into
// UpdatePlayerPopperAction's tail; on the Wii it's a function of its own
// NONMATCHING: same control flow and calls as retail, but the inlined helper set and
// the register schedule still diverge (effect-handle reads and the sfx-start block)
/**
 * @note Address: 0x8010A114
 * @note Size: 0x4B0
 */
void UpdatePopperCharge(XActPopperCharge* charge, u32 frames)
{
	if (charge->mState) {
		if (charge->mTimer < 1.0f) {
			if ((charge->mTimer += (1.0f / gRefreshRate) * frames) > 1.0f) {
				charge->mTimer = 1.0f;
			}
		}
		if (charge->mMtx != NULL) {
			Vec pos      = { charge->mMtx[0][3], charge->mMtx[1][3], charge->mMtx[2][3] };
			charge->mPos = pos;
		} else {
			if (charge->mGen != NULL) {
				charge->mGen->pos = charge->mPos;
				UpdateEffectMtx(charge->mGen);
			}
			if (charge->mChargeGen != NULL) {
				charge->mChargeGen->pos = charge->mPos;
				UpdateEffectMtx(charge->mChargeGen);
			}
		}
		if (charge->mTimer >= 1.0f) {
			if (charge->mChargeGen == NULL) {
				EffectInstance* inst;
				if (charge->mMtx != NULL) {
					inst = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x5C, 1);
					if (inst != NULL) {
						inst->mtx = charge->mMtx;
						UpdateEffectMtx(inst);
					}
				} else {
					inst = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x5C, 1);
					if (inst != NULL) {
						Vec zero    = { 0.0f, 0.0f, 0.0f };
						Vec unit    = { 1.0f, 1.0f, 1.0f };
						inst->pos   = zero;
						inst->rot   = zero;
						inst->scale = unit;
						UpdateEffectMtx(inst);
					}
				}
				charge->mChargeGen = inst;
			}
			if (!IsWiiSfxPlaying(&gGame.mSound, 0x1F8)) {
				if (charge->mSfxActors[1].GetHandle() == NULL) {
					CSound3DActor* actor = charge->mSfxActors[1].mActor;
					if (actor != NULL
					    && actor->detail_StartSound(&actor->mHandle, 0x1F8, NULL) == nw4r::snd::SoundStartable::START_SUCCESS) {
						actor->mHandle.SetVolume(1.0f, 0);
					}
				}
				gGame.mSound.mArchivePlayer.mReady = 2;
				PlayWiiSoundEffect(&gGame.mSound, 0x1F8, 1.0f, 0.0f);
				gGame.mSound.mArchivePlayer.mReady = 1;
			}
		}
	} else {
		charge->mTimer = 0.0f;
	}
	for (s32 i = 0; i < 2; ++i) {
		CSound3DActor* actor = charge->mSfxActors[i].mActor;
		if (actor != NULL && actor->mHandle.IsAttachedSound()) {
			actor->SetPosition(nw4r::math::VEC3(charge->mPos));
			actor->UpdateFade(frames);
		}
	}
}

// the GameCube's InitializePlayerPopperAction: base Init, ten shots, the charge
// cleared, its two actors reset and made on demand. On the Wii the charge carries a
// last word the GameCube's didn't have
/**
 * @note Address: 0x8010A5C4
 * @note Size: 0x108
 */
void InitializePlayerPopperAction(XActPopper* act, WiiPlayerState* owner)
{
	act->Init(owner);
	for (s32 i = 0; i < 10; ++i) {
		InitPopperShot(&act->mShots.mShot[i]);
	}
	act->mCharge.mState     = 0;
	act->mCharge.mChargeGen = NULL;
	act->mCharge.mGen       = NULL;
	for (s32 i = 0; i < 2; ++i) {
		act->mCharge.mSfxActors[i].Reset();
	}
	for (s32 i = 0; i < 2; ++i) {
		act->mCharge.mSfxActors[i].Create();
	}
	act->mCharge._38 = 0;
}

// the action's reset on the Wii: the base vcall, every shot cleared, then the same
// charge teardown ResetPopperCharge does on its own
// NONMATCHING: inherits the ResetPopperCharge residue through ClearPopperCharge
/**
 * @note Address: 0x8010A6CC
 * @note Size: 0x1FC
 */
void ResetPlayerPopperAction(XActPopper* act)
{
	act->Reset();
	for (s32 i = 0; i < 10; ++i) {
		ResetShot(&act->mShots.mShot[i]);
	}
	ClearPopperCharge(&act->mCharge);
}

// the GameCube's ApplyPopperMovementWeight, unchanged: only the aiming, flying and
// shot states pin the player in place
/**
 * @note Address: 0x8010A8C8
 * @note Size: 0x64
 */
void ApplyPopperMovementWeight(XActPopper* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 4) {
		*weight = 1.0f;
	} else if (act->mState == 6) {
		*weight = 1.0f;
	} else if (act->mState == 7 || act->mState == 8 || act->mState == 5) {
		*weight = 1.0f;
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

// how many of the ten shot slots are in the air, the GameCube's ActivePopperShots
static inline int ActivePopperShots(XActPopper* act)
{
	int count = 0;
	for (int i = 0; i < 10; ++i) {
		if (act->mShots.mShot[i].mActive) {
			++count;
		}
	}
	return count;
}

// the GameCube's UpdatePlayerPopperAction, rebuilt for the remote: holding the remote
// button while the popper is the active attachment charges (inventory 0x35 gates the
// charge, 0x36 stretches the range), letting go fires a full charge or drops a short
// one. States 1-3 deploy and ready the popper, 4 is the charge, 5-7 fire (6 the
// charged shot), 8 waits on the fire animation, 9 and 10 lower and stow it. Where the
// GameCube scanned the object list for a target the Wii reads the one CGame::Update
// locked, and without a target or the eye camera it aims through the remote's pointer
// NONMATCHING: same control flow and call set as retail; the inlined helpers
// (ClearPopperCharge's fade math, GetAttachment and the CVec temporaries around the
// pointer ray) still schedule differently
/**
 * @note Address: 0x8010A92C
 * @note Size: 0x14F0
 */
void UpdatePlayerPopperAction(XActPopper* act, s32 anim, WiiPlayerState* owner, u32 frames, f32, f32)
{
	BOOL rangeUpgrade = FindOwnedInventoryItemIndex(act->mOwner->mParams.mInventory.mItems, 0x36) >= 0;
	BOOL canCharge    = FindOwnedInventoryItemIndex(act->mOwner->mParams.mInventory.mItems, 0x35) >= 0;
	// with the popper out (attachment 2), holding the remote button charges a shot
	if (act->mOwner->GetAttachment() == 2) {
		if ((gWiiPads[0].mButton & 0x0001000000000010ULL) && canCharge && ((XObj*)act->mOwner->mPopperObj)->GetAnimId() == 4
		    && !gGame.mEvt.mData.mScript && !gGame.GetStatusFlags(0x400) && !act->mOwner->mLift.mState && !act->mOwner->mHang.mState
		    && !act->mOwner->mJump.mState && !act->mOwner->mLadder.mState && !act->mOwner->mRope.mState && !act->mOwner->mPush.mState
		    && !act->mOwner->mTumble.mState) {
			if (!gGame.GetStatusFlags(0x20)) {
				XActPopperCharge* charge = &act->mCharge;
				charge->mState           = 1;
				Vec pos                  = { 0.0f, 0.0f, 0.0f };
				charge->mPos             = pos;
				charge->mMtx             = ((XObj*)act->mOwner->mPopperObj)->GetWorldMtx("cannon_C");
				if (charge->mGen == NULL) {
					charge->mGen = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x51, 1);
					if (charge->mGen != NULL) {
						charge->mGen->mtx = charge->mMtx;
						UpdateEffectMtx(charge->mGen);
					}
				}
				if (!IsWiiSfxPlaying(&gGame.mSound, 0x1F3)) {
					if (charge->mSfxActors[0].GetHandle() == NULL) {
						CSound3DActor* actor = charge->mSfxActors[0].mActor;
						if (actor != NULL
						    && actor->detail_StartSound(&actor->mHandle, 0x1F3, NULL) == nw4r::snd::SoundStartable::START_SUCCESS) {
							actor->mHandle.SetVolume(1.0f, 0);
						}
					}
					gGame.mSound.mArchivePlayer.mReady = 2;
					PlayWiiSoundEffect(&gGame.mSound, 0x1F3, 1.0f, 0.0f);
					gGame.mSound.mArchivePlayer.mReady = 1;
				}
				if (charge->mTimer >= 0.5f) {
					act->mState = 4;
				}
			} else {
				act->mState = 0;
				ClearPopperCharge(&act->mCharge);
			}
		} else {
			// letting go of the button: a full charge fires, a short one is dropped
			if (act->mState == 4) {
				act->mState = act->mCharge.mTimer >= 1.0f ? 6 : 0;
			}
			ClearPopperCharge(&act->mCharge);
		}
	}
	act->mShotStrength = 0.0f;
	switch (act->mState) {
	case 1:
		if (anim != 0x81F) {
			act->mOwner->SetAnim(0x81F, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			ShowAndDeployPopper((XObjPopper*)act->mOwner->mPopperObj);
			break;
		}
		if (!owner->IsAnimDone()) {
			break;
		}
		act->mOwner->vf290(TRUE);
		act->mState = 2;
		// fall through
	case 2:
		if (anim != 0x821) {
			act->mOwner->SetAnim(0x821, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayPopperRaiseAnimation((XObjPopper*)act->mOwner->mPopperObj);
		} else if (owner->IsAnimDone()) {
			act->mOwner->SetAnim(0x822, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			ShowIdlePlayerPopper((XObjPopper*)act->mOwner->mPopperObj);
			act->mState = 0;
		}
		break;
	case 3:
		if (anim != 0x822) {
			act->mOwner->SetAnim(0x822, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			ShowIdlePlayerPopper((XObjPopper*)act->mOwner->mPopperObj);
		}
		act->mState = 0;
		break;
	case 5:
		if (ActivePopperShots(act) < (s32)GetPopperParameter(3)) {
			((XObj*)act->mOwner->mPopperObj)->SetAnim(4, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			((XObj*)act->mOwner->mPopperObj)->UpdateAnim(0.0f);
			act->mState        = 7;
			act->mShotStrength = (s32)GetPlayerActionParameter(0x1E) / 100.0f;
		}
		// fall through
	case 6:
	case 7:
		if (anim != 0x823 || act->mShotStrength != 0.0f) {
			if (ActivePopperShots(act) < (s32)GetPopperParameter(3)) {
				CVec pos = ((XObj*)act->mOwner->mPopperObj)->GetJointPos("cannon_C");
				f32 yaw, pitch;
				XObj* aimObj = lbl_80453508;
				if (aimObj != NULL) {
					// face the locked target and arc the shot over to it
					Vec aim = aimObj->mToppleOffset;
					yaw     = atan2(aim.x - pos.x, aim.z - pos.z);
					f32 dz  = aim.z - pos.z;
					f32 dx  = aim.x - pos.x;
					f32 dy  = aim.y - pos.y;
					f32 a   = atan2(FSqrt(dx * dx + dz * dz), FSqrt(dy * dy));
					pitch   = dy < 0.0f ? HALF_PI - a : a - HALF_PI;
					if (!gGame.GetStatusFlags(0x100)) {
						act->mOwner->SetRotationY(yaw);
					}
					if (act->mCharge._38 == 0) {
						act->mCharge._38 = 1;
						break;
					}
					act->mCharge._38 = 0;
				} else if (gGame.GetStatusFlags(0x100)) {
					// the eye camera keeps the shot angles on the player
					pitch = act->mOwner->mEyeAim.x - 0.02617994f;
					yaw   = act->mOwner->mEyeAim.y - 0.02617994f;
				} else if (gWiiPointer.mPointerValid) {
					CCamera* cam = &gGame.mMotionControlCamera;
					if (gGame.mStage == 0xB) {
						// no world ray here: the side of the projected position the pointer is on decides
						Vec pos0 = *act->mOwner->GetPos();
						f32 sx, sy, sz;
						GXProject(pos0.x, pos0.y, pos0.z, cam->mViewMtx, &cam->mIsOrtho, cam->mViewport, &sx, &sy, &sz);
						yaw   = gWiiPointer.mPointer.x < sx * cam->mScale ? 4.712389f : 1.5707964f;
						pitch = 0.0f;
						act->mOwner->SetRotationY(yaw);
					} else {
						// unproject the pointer to the near plane and stretch the eye's ray
						// through it to the muzzle's height
						Vec ndc = { gWiiPointer.mPointer.x / cam->mScale, gWiiPointer.mPointer.y, gWiiPointer.mPointer.z };
						CVec point;
						PSMTX44MultVec(cam->mScreenInv, &ndc, &point);
						CVec start = point;
						Vec eye    = cam->mView.mEye;
						CVec dir   = point - eye;
						CVec end   = point + dir;
						if (point.y > eye.y) {
							end.y = eye.y;
						}
						Vec plane[2] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };
						plane[0]     = pos;
						if (gGame.GetStatusFlags(0x100)) {
							plane[0].y = eye.y;
						}
						Vec hit;
						IntersectLineWithPlane(&start, &end, &plane[0], &plane[1], &hit);
						yaw    = atan2(hit.x - pos.x, hit.z - pos.z);
						f32 dz = hit.z - pos.z;
						f32 dx = hit.x - pos.x;
						f32 dy = hit.y - pos.y;
						f32 a  = atan2(FSqrt(dx * dx + dz * dz), FSqrt(dy * dy));
						pitch  = dy < 0.0f ? HALF_PI - a : a - HALF_PI;
						act->mOwner->SetRotationY(yaw);
					}
					// the aim runs one frame behind the state, so the first aiming
					// frame only turns and the second one fires
					if (act->mCharge._38 == 0) {
						act->mCharge._38 = 1;
						break;
					}
					act->mCharge._38 = 0;
				} else {
					yaw   = act->mOwner->mMoveRotate.y;
					pitch = 0.0f;
				}
				f32 power = act->mState == 6 ? 3 : 1;
				// in tenths: popper parameter 2 is the speed, 0 the range and 1 the range with the upgrade
				f32 speed = (s32)GetPopperParameter(2) / 10.0f;
				f32 range = (s32)GetPopperParameter(rangeUpgrade) / 10.0f;
				PlayPopperFireAnimation((XObjPopper*)act->mOwner->mPopperObj);
				if (act->mShotStrength == 0.0f) {
					act->mShotStrength
					    = act->mState == 6 ? (s32)GetPlayerActionParameter(0x1F) / 100.0f : (s32)GetPlayerActionParameter(0x1E) / 100.0f;
				}
				if (act->mOwner->mBatteryDrains) {
					act->mOwner->mParams.SetBattery(act->mOwner->mParams.mBattery - act->mShotStrength);
				}
				s32 kind = GetAttachmentCountById((AttachmentValues*)act->mOwner->mAttachments, 2);
				for (s32 i = 0; i < 10; ++i) {
					if (!act->mShots.mShot[i].mActive) {
						StartPopperShot(&act->mShots.mShot[i], pos, kind, NULL, yaw, pitch, power, speed, range, act->mShotStrength);
						break;
					}
				}
			}
			if (act->mState != 6) {
				act->mState = 8;
			}
			ClearPopperCharge(&act->mCharge);
			act->mState = 8;
		} else if (owner->IsAnimDone()) {
			act->mOwner->SetAnim(0x822, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			ShowIdlePlayerPopper((XObjPopper*)act->mOwner->mPopperObj);
			act->mState = 0;
		}
		break;
	case 8:
		if (!IsPopperFireAnimationSelected((XObjPopper*)act->mOwner->mPopperObj)) {
			ShowIdlePlayerPopper((XObjPopper*)act->mOwner->mPopperObj);
			act->mState = 0;
		}
		break;
	case 9:
		if (anim != 0x824) {
			act->mOwner->SetAnim(0x824, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayPopperLowerAnimation((XObjPopper*)act->mOwner->mPopperObj);
			break;
		}
		if (!owner->IsAnimDone()) {
			break;
		}
		act->mOwner->vf290(FALSE);
		act->mState = 10;
		// fall through
	case 10:
		if (anim != 0x820) {
			act->mOwner->SetAnim(0x820, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayPopperStowAnimation((XObjPopper*)act->mOwner->mPopperObj);
			act->mOwner->vf290(FALSE);
		} else if (owner->IsAnimDone()) {
			WiiPlayerState* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	}
	for (s32 i = 0; i < 10; ++i) {
		UpdatePopperShotFlight(&act->mShots.mShot[i], frames);
	}
	UpdatePopperCharge(&act->mCharge, frames);
}

// the GameCube's DrawPopperShotParticles: the dust pool behind every shot, with the
// pool wrapper's first virtual run before each draw the way DrawObjectEffects does
// it for the player's own CPtclSet
/**
 * @note Address: 0x8010BE1C
 * @note Size: 0x78
 */
void DrawPopperShotParticles(XActPopper* act)
{
	s32 i;
	XActPopperShot* shot;
	for (i = 0, shot = act->mShots.mShot; i < 10; i++, shot++) {
		shot->mDust.vf08(&gGame.mGs);
		DrawParticlePool(&shot->mDust.mPtcl, &gGame.mGs);
	}
}

// the GameCube's DrawPopperShotRings tail loop on its own: the overlay draw walks all
// ten shots here while DrawPopperShotRings itself stays a per-shot helper
/**
 * @note Address: 0x8010BE94
 * @note Size: 0x4C
 */
void fn_8010BE94(XActPopper* act)
{
	s32 i;
	XActPopperShot* shot;
	for (i = 0, shot = act->mShots.mShot; i < 10; i++, shot++) {
		DrawPopperShotRings(shot);
	}
}

// reads the popper muzzle's joint position out for the caller; CGame::Update uses it
// to aim shots from the cannon_C joint of the popper object
/**
 * @note Address: 0x8010BEE0
 * @note Size: 0x68
 */
// NONMATCHING: retail copies the returned vector with three word load/stores and
// schedules the mOwner load ahead of the string's address; this build does the copy
// as float member stores
void fn_8010BEE0(Vec* out, XActPopper* act)
{
	*out = ((XObj*)act->mOwner->mPopperObj)->GetJointPos("cannon_C");
}

// the GameCube's InitRadarAction: initialize the action, then its full reset
/**
 * @note Address: 0x8010BF48
 * @note Size: 0xE0
 */
void InitRadarAction(XActRadar* act, WiiPlayerState* owner)
{
	act->Init(owner);
	ResetPlayerRadarAction(act);
}

// the GameCube's ResetPlayerRadarAction: back to a cleared target and both beam
// colours fading in from white
/**
 * @note Address: 0x8010C028
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

// the Wii's own radar reset: the player reset body, then the aim angles are pushed
// into the radar attachment object's pair at _9B4/_9B8 and the radar loop sound is
// silenced. XObjC::UpdateObject and a spider state call it
/**
 * @note Address: 0x8010C0F4
 * @note Size: 0x114
 */
void fn_8010C0F4(XActRadar* act)
{
	act->Reset();
	act->mTarget    = NULL;
	act->mYaw.angle = act->mYaw.targetAngle = act->mYaw.reached = 0.0f;
	act->mPitch.angle = act->mPitch.targetAngle = act->mPitch.reached = 0.0f;
	act->mTargetDistance                                              = NAN;
	StartScreenFade(&act->mBeam.mColor[0], 0, 0, 0, 0, 0xFF, 0x19, 0xFF, 0, 0.0f);
	StartScreenFade(&act->mBeam.mColor[1], 0, 0, 0, 0, 0xFF, 0x19, 0xFF, 0, 0.0f);
	act->mFade                                   = 0.0f;
	*(f32*)((u8*)act->mOwner->mRadarObj + 0x9B4) = act->mYaw.angle;
	*(f32*)((u8*)act->mOwner->mRadarObj + 0x9B8) = act->mPitch.angle;
	if (IsWiiSfxPlaying(&gGame.mSound, 0xB1)) {
		StopWiiSfx(&gGame.mSound, 0xB1, 0.0f);
	}
}

// the GameCube's ApplyRadarMovementWeight: with the radar out (attachment 3) he
// stands still while it comes out, goes away or scans. the Wii checks the pending
// attachment pick first and falls back to the current one
/**
 * @note Address: 0x8010C20C
 * @note Size: 0x68
 */
// NONMATCHING: retail chains two compares per state group; this build folds the
// first group's 'state == 1 || state == 2' into an unsigned range test
void ApplyRadarMovementWeight(XActRadar* act, const CPadOne* pad, f32* weight)
{
	const WiiPlayerState* owner = act->mOwner;
	const s32 attachment        = owner->mAttachmentRequest >= 0 ? owner->mAttachmentRequest : owner->mAttachment;
	if (attachment == 3) {
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

// the GameCube's UpdateRadarTargeting: 1 takes the radar out, 2 puts it away, a scan starts at 3,
// 4 spins the dish a full turn and works out the aim, 5 turns it there and 0 keeps it on the target.
// The Wii hands the owner's anim id and an object to probe IsAnimDone on instead of the GameCube's
// AObj; finishing the draw anim falls back to 0x4a with no idle loops, stowing finishes on 0x11
/**
 * @note Address: 0x8010C274
 * @note Size: 0xC70
 */
// NONMATCHING: scheduling of the u32->f64 frames conversions and a few pointer reloads;
// semantics decoded: the action mirrors its aim into XObjRadar::mYaw/mPitch for the dish
void UpdateRadarTargeting(XActRadar* act, s32 anim, XObj* animObj, u32 frames)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x879) {
			act->mOwner->SetAnim(0x879, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			ActivatePlayerRadar((XObjRadar*)act->mOwner->mRadarObj);
		} else if (animObj->IsAnimDone()) {
			act->mOwner->SetAnim(0x4a, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState     = 0;
			act->mTarget    = NULL;
			act->mYaw.angle = act->mYaw.targetAngle = act->mYaw.reached = 0.0f;
			act->mPitch.angle = act->mPitch.targetAngle = act->mPitch.reached = 0.0f;
			fn_8010D094(act, 0, 0, 0);
		}
		break;
	case 2:
		if (anim != 0x87a) {
			act->mOwner->SetAnim(0x87a, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			DeactivatePlayerRadar((XObjRadar*)act->mOwner->mRadarObj);
		} else if (animObj->IsAnimDone()) {
			WiiPlayerState* owner = act->mOwner;
			owner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			owner->mIdleLoops = 3;
			act->mState       = 0;
			act->mTarget      = NULL;
			act->mYaw.angle = act->mYaw.targetAngle = act->mYaw.reached = 0.0f;
			act->mPitch.angle = act->mPitch.targetAngle = act->mPitch.reached = 0.0f;
			fn_8010D094(act, 0, 0, 0);
		}
		break;
	case 3: {
		WiiPlayerState* owner = act->mOwner;
		owner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		owner->mIdleLoops = 3;
		StartPlayerRadarScan((XObjRadar*)act->mOwner->mRadarObj);
		act->mState             = 4;
		act->mTarget            = FindNearestRadarTarget(act);
		act->mYaw.targetAngle   = TWO_PI + act->mYaw.angle;
		act->mYaw.reached       = 0;
		act->mPitch.targetAngle = 0;
		act->mPitch.reached     = 0;
		PlayWiiSoundEffect(&gGame.mSound, 0xB1, 1.0f, 0.0f);
		fn_8010D094(act, 0, 0, 0);
		break;
	}
	case 4:
		if (!act->mYaw.reached) {
			act->mYaw.angle += (TWO_PI / gRefreshRate) * frames;
			if (act->mYaw.angle >= act->mYaw.targetAngle) {
				act->mYaw.angle = WrapAngle(act->mYaw.angle);
				if (act->mTarget) {
					CVec* targetPos       = act->mTarget->GetPos();
					CVec* ownerPos        = act->mOwner->GetPos();
					f32 yaw               = (f32)atan2(targetPos->x - ownerPos->x, targetPos->z - ownerPos->z);
					act->mYaw.targetAngle = WrapAngle(yaw - act->mOwner->mMoveRotate.y);
				} else
					act->mYaw.targetAngle = TWO_PI;
				if (act->mYaw.targetAngle < act->mYaw.angle)
					act->mYaw.targetAngle += TWO_PI;
				act->mYaw.reached = 1;
			}
			((XObjRadar*)act->mOwner->mRadarObj)->mYaw = act->mYaw.angle;
		}
		if (!act->mPitch.reached) {
			RadarAimAxis& pitch = act->mPitch;
			if (pitch.angle < pitch.targetAngle) {
				pitch.angle += 3 * DEG2RAD * frames;
				if (pitch.angle >= pitch.targetAngle)
					pitch.reached = 1;
			} else if (pitch.angle > pitch.targetAngle) {
				pitch.angle -= 3 * DEG2RAD * frames;
				if (pitch.angle <= pitch.targetAngle)
					pitch.reached = 1;
			} else
				pitch.reached = 1;
			if (pitch.reached) {
				if (act->mTarget) {
					CVec from               = GetRadarDishWorldPosition((XObjRadar*)act->mOwner->mRadarObj);
					CVec to                 = *act->mTarget->GetPos() + (Vec) { 0, act->mTarget->GetCollisionHeight() / 2.f, 0 };
					act->mPitch.targetAngle = TargetPitch(from, to);
				} else
					act->mPitch.targetAngle = 0;
			}
			((XObjRadar*)act->mOwner->mRadarObj)->mPitch = act->mPitch.angle;
		}
		if (act->mYaw.reached && act->mPitch.reached) {
			act->mState         = 5;
			act->mYaw.reached   = 0;
			act->mPitch.reached = 0;
		}
		break;
	case 5:
		if (!act->mYaw.reached) {
			act->mYaw.angle += (TWO_PI / gRefreshRate) * frames;
			if (act->mYaw.angle >= act->mYaw.targetAngle) {
				act->mYaw.angle   = act->mYaw.targetAngle;
				act->mYaw.reached = 1;
			}
			((XObjRadar*)act->mOwner->mRadarObj)->mYaw = act->mYaw.angle;
		}
		if (!act->mPitch.reached) {
			RadarAimAxis& pitch = act->mPitch;
			if (pitch.angle < pitch.targetAngle) {
				pitch.angle += DEG2RAD * frames;
				if (pitch.angle >= pitch.targetAngle) {
					pitch.angle   = pitch.targetAngle;
					pitch.reached = 1;
				}
			} else if (pitch.angle > pitch.targetAngle) {
				pitch.angle -= DEG2RAD * frames;
				if (pitch.angle <= pitch.targetAngle) {
					pitch.angle   = pitch.targetAngle;
					pitch.reached = 1;
				}
			} else
				pitch.reached = 1;
			((XObjRadar*)act->mOwner->mRadarObj)->mPitch = act->mPitch.angle;
		}
		if (act->mYaw.reached && act->mPitch.reached) {
			act->mState = 0;
			if (act->mTarget)
				PlayWiiSoundEffect(&gGame.mSound, 0xB0, 1.0f, 0.0f);
		}
		break;
	case 0:
		if (act->mTarget) {
			u64 flags = act->mTarget->mStatus.mFlags;
			// the target is gone, hidden or being removed: drop it and turn the dish back to the front
			if (!(flags & 1) || !(flags & 0x10000) || (flags & 0x4000000000000000ULL)) {
				act->mTarget          = NULL;
				act->mYaw.targetAngle = TWO_PI;
				fn_8010D094(act, 0, 0, 0);
				act->mState = 5;
			} else {
				CVec from       = GetRadarDishWorldPosition((XObjRadar*)act->mOwner->mRadarObj);
				CVec to         = *act->mTarget->GetPos() + (Vec) { 0, act->mTarget->GetCollisionHeight() / 2.f, 0 };
				f32 yaw         = (f32)atan2(to.x - from.x, to.z - from.z);
				act->mYaw.angle = WrapAngle(yaw - act->mOwner->mMoveRotate.y);
				((XObjRadar*)act->mOwner->mRadarObj)->mYaw   = act->mYaw.angle;
				act->mPitch.angle                            = TargetPitch(from, to);
				((XObjRadar*)act->mOwner->mRadarObj)->mPitch = act->mPitch.angle;
			}
		}
		break;
	}
	if (act->mTarget) {
		CVec from = *act->mOwner->GetPos(), to = *act->mTarget->GetPos();
		f32 dist             = PSVECDistance(&from, &to);
		act->mTargetDistance = dist - (act->mTarget->GetFloorXZExtent() + act->mOwner->GetFloorXZExtent());
		if (act->mTargetDistance < 0)
			act->mTargetDistance = 0;
		if (act->mState == 0) {
			// near, middle or far: each has its own tone and turns the beam red, yellow or blue
			if (act->mTargetDistance < (s32)GetRadarParameter(0) / 100.f) {
				fn_8010D094(act, 1, 0, 0);
			} else if (act->mTargetDistance < (s32)GetRadarParameter(1) / 100.f) {
				fn_8010D094(act, 0, 1, 0);
			} else {
				fn_8010D094(act, 0, 0, 1);
			}
		}
		UpdateScreenFade(&act->mBeam.mColor[0], frames);
		UpdateScreenFade(&act->mBeam.mColor[1], frames);
	} else
		act->mTargetDistance = NAN;
}

// the GameCube's GetRadarDishWorldPosition: the dish joint's world position
/**
 * @note Address: 0x8010CEE4
 * @note Size: 0x14
 */
CVec GetRadarDishWorldPosition(XObjRadar* obj)
{
	return obj->GetJointPos("obj1");
}

// the radar's pick, the nearest drawn object that isn't chibi robo himself
/**
 * @note Address: 0x8010CEF8
 * @note Size: 0x19C
 */
extern "C" XObj* FindNearestRadarTarget(XActRadar* act)
{
	XObj* obj;
	CVec pos        = *act->mOwner->GetPos();
	XObj* nearest   = NULL;
	f32 nearestDist = NAN;
	for (int i = 0; i < gGame.mXObjs.mObjectCount; i++) {
		obj = gGame.mXObjs.mObjects[i];
		if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && !(obj->mStatus.mFlags & 0x4000000000000000ULL)
		    && (obj->mStatus.mFlags & 0x80000) && obj != (XObj*)act->mOwner && obj->GetKind() < 10000) {
			f32 dist;
			if (nearest == NULL) {
				dist = PSVECDistance(&pos, obj->GetPos());
			} else {
				dist = PSVECDistance(&pos, obj->GetPos());
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

// the Wii's StartRadarTone, all three tones in one helper: a set flag plays its tone
// (0x1fb near, 0x1fa middle, 0x1f9 far) and turns the beam that band's colour, a clear
// one silences it
/**
 * @note Address: 0x8010D094
 * @note Size: 0x228
 */
extern "C" void fn_8010D094(XActRadar* act, s8 near, s8 mid, s8 far)
{
	XSound& sound = gGame.mSound;
	if (near) {
		if (!IsWiiSfxPlaying(&sound, 0x1FB)) {
			PlayWiiSoundEffect(&sound, 0x1FB, 1.0f, 0.0f);
			SetScreenFadeTargetColor(&act->mBeam.mColor[0], 255, 150, 150, 0, 0.5f);
			SetScreenFadeTargetColor(&act->mBeam.mColor[1], 255, 50, 50, 0, 0.5f);
		}
	} else if (IsWiiSfxPlaying(&sound, 0x1FB)) {
		StopWiiSfx(&sound, 0x1FB, 0.0f);
	}
	if (mid) {
		if (!IsWiiSfxPlaying(&sound, 0x1FA)) {
			PlayWiiSoundEffect(&sound, 0x1FA, 1.0f, 0.0f);
			SetScreenFadeTargetColor(&act->mBeam.mColor[0], 255, 255, 150, 0, 0.5f);
			SetScreenFadeTargetColor(&act->mBeam.mColor[1], 255, 255, 50, 0, 0.5f);
		}
	} else if (IsWiiSfxPlaying(&sound, 0x1FA)) {
		StopWiiSfx(&sound, 0x1FA, 0.0f);
	}
	if (far) {
		if (!IsWiiSfxPlaying(&sound, 0x1F9)) {
			PlayWiiSoundEffect(&sound, 0x1F9, 1.0f, 0.0f);
			SetScreenFadeTargetColor(&act->mBeam.mColor[0], 150, 150, 255, 0, 0.5f);
			SetScreenFadeTargetColor(&act->mBeam.mColor[1], 50, 50, 255, 0, 0.5f);
		}
	} else if (IsWiiSfxPlaying(&sound, 0x1F9)) {
		StopWiiSfx(&sound, 0x1F9, 0.0f);
	}
}

// fades the radar's marker in while it has something, and spins it
/**
 * @note Address: 0x8010D2BC
 * @note Size: 0xEC
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

// NONMATCHING: retail lays out the vertex and colour loops differently, and the Wii
// renderer calls its own state and primitive entries where the GameCube used the GS helpers
/**
 * @note Address: 0x8010D3A8
 * @note Size: 0x11A0
 */
extern "C" void DrawRadarTargetBeam(XActRadar* act)
{
	if (!(act->mFade > 0))
		return;
	CVec origin = GetRadarDishWorldPosition((XObjRadar*)act->mOwner->mRadarObj);
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
	gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);
	n = ShadeRadarBeam(act, colors, back);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, colors, n);
	gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_FRONT);
	n = ShadeRadarBeam(act, colors, front);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, colors, n);
	gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);
}

} // extern "C"

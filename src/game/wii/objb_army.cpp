#include "game/wii/xobj.h" // first: its AnimSfx members need the real nw4r::math
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/gmath.h"
#include "game/globals.h"
#include "arithmetic.h"
#include <math.h>
#include <stdlib.h>
extern "C" {
#include <Dolphin/vec.h>
}

// the toy soldiers and the shots they fire. The GameCube's objb_army.cpp carries
// the same code; the Wii keeps the member layout shifted behind the bigger XObj,
// draws the shots through the CGs scene list and plays the sounds through
// PlayWiiSoundEffect/AnimSfx

extern "C" f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 which);
extern "C" void UpdateObjectFloorXZExtent(XObj* object);
extern "C" void UpdateObjectFloorHeights(XObj* object);
extern "C" void UpdateArmyShot(XObjArmySlot* shot);
extern "C" void DrawArmyShot(XObjArmySlot* shot);
extern "C" void FireNextArmyShot(XObjArmy* army);

// sets the object's floor boxes, they take effect at once
static inline void SetFloors(XObj* obj, XObjFloor* floors)
{
	obj->mFloors        = floors;
	obj->mFloorMasks[1] = 0;
	obj->mFloorMasks[0] = 0;
	UpdateObjectFloorXZExtent(obj);
	UpdateObjectFloorHeights(obj);
}

// the points the soldier stands on, upright, falling over and lying down
static XObjFloor sArmyStandingFloors[2] = {
	{ 0, 0, 3, { 0, 0x226, 0 }, 0, { 0x200, 0x200, 0x200 } },
	{ -1 },
};
static XObjFloor sArmyFallingFloors[2] = {
	{ 0, 0, 3, { 0, 0x100, -0xD4 }, 0, { 0x200, 0x100, 0x200 } },
	{ -1 },
};
static XObjFloor sArmyProneFloors[2] = {
	{ 0, 0, 3, { 0, 0x100, 0 }, 0, { 0x200, 0x100, 0x200 } },
	{ -1 },
};
extern "C" {
XObjFloor* sArmyFloorsByPose[4] = { sArmyStandingFloors, sArmyFallingFloors, sArmyProneFloors, NULL };

AnimSfxEvent sArmyF7SoundEvents[] = {
	{ 0x02, 18.0f, 36.0f, 1, 0, 0x512, 0xFF, 4, 0, 0 }, { 0x03, 14.0f, 27.0f, 1, 0, 0x512, 0xFF, 4, 0, 0 },
	{ 0x11, 11.0f, 22.0f, 1, 0, 0x512, 0xFF, 4, 0, 0 }, { 0x12, 5.0f, -1.0f, 1, 1, 0x511, 0xFF, 4, 0, 0 },
	{ 0x0F, 0.0f, -1.0f, 1, 2, 0x520, 0xFF, 0, 0, 0 },  { 0x07, 28.0f, -1.0f, 1, 3, 0x52C, 0xFF, 0, 0, 0 },
	{ 0x28, 18.0f, 36.0f, 1, 0, 0x512, 0xFF, 0, 0, 0 }, { -1 },
};
AnimSfxEvent sArmyF8SoundEvents[] = {
	{ 0x02, 18.0f, 36.0f, 1, 0, 0x512, 0xFF, 4, 0, 0 },
	{ 0x03, 14.0f, 27.0f, 1, 0, 0x512, 0xFF, 4, 0, 0 },
	{ 0x11, 11.0f, 22.0f, 1, 0, 0x512, 0xFF, 4, 0, 0 },
	{ 0x12, 5.0f, -1.0f, 1, 1, 0x511, 0xFF, 4, 0, 0 },
	{ 0x0F, 0.0f, -1.0f, 1, 2, 0x520, 0xFF, 0, 0, 0 },
	{ 0x07, 28.0f, -1.0f, 1, 3, 0x52C, 0xFF, 0, 0, 0 },
	{ -1 },
};

} // extern "C"

// the y angle that looks from to at from, 0 when they're on top of each other
static inline f32 GetAngleY(const Vec& from, const Vec& to)
{
	f32 dx = to.x - from.x;
	f32 dz = to.z - from.z;
	if (FSqrt(dx * dx + dz * dz) == 0.0f) {
		return 0.0f;
	}
	f32 angle = atan2(from.x - to.x, from.z - to.z);
	while (angle < 0.0f) {
		angle += TWO_PI;
	}
	while (TWO_PI <= angle) {
		angle -= TWO_PI;
	}
	return angle;
}

// the Wii rolls its own [0,1) out of rand()
static inline f32 ArmyRandf()
{
	return (rand() % 0x10000) / 65536.0f;
}

extern "C" {
Vec gArmyPatrolMin;
Vec gArmyPatrolMax;
XObj* gArmyShotModel;      // the leader's model draws the shots
s32 gArmyNextShotSound;    // the next of the four shot sounds
s32 gArmyKnockdownCount;   // soldiers knocked over
XObj* gArmyShotTarget;     // shot at instead of chibi robo
CStatus32 gArmyShotStatus; // 1 once a shot hits, 2 keeps them from shooting
}

// fires a shot from pos along dir
static inline void FireShot(XObjArmySlot* shot, const CVec& pos, const CVec& dir, u8 quiet)
{
	shot->mActive = 1;
	shot->mLife   = 120;
	shot->mPos    = pos;
	shot->mPos.y += 6.0f;
	shot->mVel = dir;
	PSVECNormalize(&shot->mVel, &shot->mVel);
	shot->mSpeed = 3.0f;
	shot->mPhase = 0;
	if (!quiet) {
		if (gArmyNextShotSound < 0 || gArmyNextShotSound >= 4) {
			gArmyNextShotSound = 0;
		}
		PlayWiiSoundEffect(&gGame.mSound, gArmyNextShotSound + 0x516, 1.0f, 0.0f);
		gArmyNextShotSound++;
	}
}

// starts what the soldier does between hits again
static inline void RestartMode(XObjArmy* army)
{
	army->mStep = 0;
	switch (army->mMode) {
	case 1:
		army->mStatus.mFlags |= 8;
		army->mFunc = &XObjArmy::StateMarch;
		break;
	case 2:
		army->mFunc = &XObjArmy::StateGuard;
		break;
	default:
		army->mFunc = NULL;
		break;
	}
}

static inline void Knock(XObjArmy* army)
{
	army->mStep = 0;
	army->mFunc = &XObjArmy::StateKnock;
}

static inline void SetMode(XObjArmy* army, s32 mode)
{
	army->mMode = mode;
	RestartMode(army);
}

/**
 * @note Address: 0x8011D988
 * @note Size: 0x34
 */
void XObjArmy::ResetForNewGame()
{
	mStep       = 0;
	mTimer      = 0;
	_9BC        = 0.0f;
	_9C0[0]     = 0;
	_9C0[1]     = 0;
	mFlags      = 0;
	mGuardPos.x = 0.0f;
	mGuardPos.y = 0.0f;
	mGuardPos.z = 0.0f;
	mMode       = 0;
}

/**
 * @note Address: 0x8011D9BC
 * @note Size: 0x138
 */
void XObjArmy::ResetObjectState()
{
	XObj::ResetObjectState();
	gArmyKnockdownCount    = 0;
	gArmyShotTarget        = NULL;
	gArmyShotStatus.mFlags = 0;
	mFunc                  = NULL;
	if (mPoolIndex == 0) { // the first soldier finds the model the shots draw with
		gArmyShotModel = NULL;
		for (s32 i = 0; gArmyShotModel == NULL && i < gGame.mXObjs.mObjectCount; i++) {
			XObj* obj = gGame.mXObjs.mObjects[i];
			if ((obj->mStatus.mFlags & 1) && obj->mCoord.mPlacement->mObjectId == 0x12A) {
				gArmyShotModel = obj;
			}
		}
	}
	switch (GetKind()) {
	case 0xF7:
		mSfx.Init((WiiPlayerState*)this, sArmyF7SoundEvents);
		break;
	case 0xF8:
		mSfx.Init((WiiPlayerState*)this, sArmyF8SoundEvents);
		break;
	}
	for (s32 i = 0; i < 5; i++) {
		mShots[i].mActive = 0;
	}
}

/**
 * @note Address: 0x8011DAF4
 * @note Size: 0x3A8
 */
void XObjArmy::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	if (mFunc) {
		(this->*mFunc)();
	}
	switch (GetAnimId()) {
	case 4:
		if (!(mFlags & 2) && 20.0f <= GetAnimFrame()) { // flag 2 marks the floors swapped
			SetFloors(this, sArmyFloorsByPose[2]);
			mFlags |= 2;
		}
		if (50.0f < GetAnimFrame()) {
			SetMoveDir(GetPos(), 1.0f);
		}
		break;
	case 5:
	case 6:
		if (!(mFlags & 2) && 15.0f <= GetAnimFrame()) {
			SetFloors(this, sArmyFloorsByPose[2]);
			mFlags |= 2;
		}
		if (50.0f < GetAnimFrame()) {
			SetMoveDir(GetPos(), 1.0f);
		}
		break;
	case 10: // the knock anim ends on the falling floors
		if (!(mFlags & 2) && GetAnimEndFrame() <= GetAnimFrame()) {
			SetFloors(this, sArmyFloorsByPose[1]);
			mFlags |= 2;
		}
		break;
	case 17:
		if (!(mFlags & 2)) {
			SetFloors(this, sArmyFloorsByPose[1]);
			mFlags |= 2;
		}
		break;
	case 18:
		if (!(mFlags & 2)) {
			SetFloors(this, sArmyFloorsByPose[1]);
			mFlags |= 2;
		}
		break;
	default:
		if (mFlags & 2) {
			SetFloors(this, sArmyFloorsByPose[0]);
			mFlags &= ~2;
		}
		break;
	}
	UpdateAnimationSounds();
	for (u32 i = 0; i < 5; i++) {
		UpdateArmyShot(&mShots[i]);
	}
}

/**
 * @note Address: 0x8011DE9C
 * @note Size: 0x4
 */
void XObjArmy::DrawObjectEffects()
{
}

/**
 * @note Address: 0x8011DEA0
 * @note Size: 0x4C
 */
void XObjArmy::DrawLate()
{
	for (u32 i = 0; i < 5; i++) {
		DrawArmyShot(&mShots[i]);
	}
}

static inline bool SoldierBlocksMarch(XObjArmy* self, CVec& next)
{
	for (s32 i = self->mPoolIndex + 1; i < 21; ++i) { // only the soldiers later in the pool
		XObjArmy* other = (XObjArmy*)(gGame.mXObjs.mPools[16].mObjects + i * gGame.mXObjs.mPools[16].mStride);
		if (!(other->mStatus.mFlags & 1) || (other->mFlags & 1)) // inactive or down
			continue;
		f32 height = self->GetPos()->y - other->GetPos()->y;
		if (height < 0.0f)
			height = -height;
		if (5.0f < height)
			continue;
		if (7.5f < PSVECDistance(&next, other->GetPos()))
			continue;
		return true;
	}
	return false;
}

/**
 * @note Address: 0x8011DEEC
 * @note Size: 0xD28
 */
void XObjArmy::StateMarch()
{
	f32 angle    = GetAngleY(*gGame.GetPlayer()->GetPos(), *GetPos());
	f32 distance = PSVECDistance(GetPos(), gGame.GetPlayer()->GetPos());
	f32 patrolY  = GetPos()->y;
	f32 height   = gGame.GetPlayer()->GetPos()->y - patrolY;
	CVec target;
	target.x = gGame.GetPlayer()->GetPos()->x;
	target.y = GetPos()->y;
	target.z = gGame.GetPlayer()->GetPos()->z;
	CVec next;
	switch (mStep) {
	case 0:
		next.x = gArmyPatrolMin.x + (gArmyPatrolMax.x - gArmyPatrolMin.x) * ArmyRandf();
		next.y = gArmyPatrolMin.y;
		next.z = gArmyPatrolMin.z + (gArmyPatrolMax.z - gArmyPatrolMin.z) * ArmyRandf();
		SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		SetMoveDir(&next, .4f);
		SetRotateY(GetAngleY(next, *GetPos()), .5f);
		++mStep;
		break;
	case 1: // walking to the patrol point
		if (PSVECDistance(GetPos(), &mMoveTarget0) < .1f) {
			if (target.x < gArmyPatrolMin.x || gArmyPatrolMax.x < target.x || target.z < gArmyPatrolMin.z || gArmyPatrolMax.z < target.z)
				mStep = 0; // the player left the patrol box; pick a new point
			else if (rand() % 100 < 75)
				++mStep;
			else
				mStep = 0;
		} else if (distance < 80 && Abs(height) < 10) {
			f32 facing = cos(angle - GetRotY());
			if ((f32)cos(20.f * DEG2RAD) < facing) { // the player inside a twenty-degree cone
				mStep = 10;
			} else if (distance < 40) // not facing him but close: step aside
				++mStep;
		}
		break;
	case 2: // a long step sideways
		SetAnim(5, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		next = *GetPos();
		next.x += 50 * (f32)sin(HALF_PI + GetRotY());
		next.z += 50 * (f32)cos(HALF_PI + GetRotY());
		SetMoveDir(&next, .4f);
		++mStep;
		break;
	case 3:
		next   = *GetPos();
		next.y = patrolY;
		next.x += 5 * (f32)sin(HALF_PI + GetRotY());
		next.z += 5 * (f32)cos(HALF_PI + GetRotY());
		if (SoldierBlocksMarch(this, next))
			SetMoveDir(GetPos(), 1.0f);
		if (GetAnimEndFrame() <= GetAnimFrame())
			mStep = 0;
		break;
	case 10: // turn to the player and close in
		SetAnim(4, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		SetRotateY(angle, .5f);
		SetMoveDir(&target, .5f);
		++mStep;
		break;
	case 11:
		next   = *GetPos();
		next.y = patrolY;
		next.x += 5 * (f32)sin(GetRotY());
		next.z += 5 * (f32)cos(GetRotY());
		if (SoldierBlocksMarch(this, next))
			SetMoveDir(GetPos(), 1.0f);
		if (GetAnimEndFrame() <= GetAnimFrame()) {
			if (gArmyShotStatus.mFlags & 2) {
				mStep = 0;
				break;
			}
			f32 facing = cos(angle - GetRotY());
			if ((f32)cos(20.) < facing) { // radians, not degrees: a wide cone
				if (rand() % 100 < 30) {
					SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					SetMoveDir(GetPos(), 1.0f);
					++mStep;
				} else
					mStep = 0;
				break;
			}
			mStep = 0;
		}
		break;
	case 12:
		if (GetAnimEndFrame() <= GetAnimFrame()) {
			SetRotateY(angle, .05f);
			SetAnim(9, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			++mStep;
		}
		break;
	case 13:
		SetRotateY(angle, .05f);
		if (20 <= GetAnimFrame()) { // fires at frame 20 of the aiming anim
			FireNextArmyShot(this);
			++mStep;
		}
		break;
	case 14: {
		if (IsAnimDone()) {
			if (distance < 40) {
				f32 facing = cos(angle - GetRotY());
				if ((f32)cos(45.) < facing) // radians again: a wide cone
					mStep = 10;
				else
					mStep = 0;
			} else
				mStep = 0;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8011EC14
 * @note Size: 0x17C
 */
void XObjArmy::StateWait()
{
	switch (mStep) {
	case 0:
		mTimer = 30;
		SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		SetMoveDir(GetPos(), 1.0f);
		SetRotateY(GetRotY(), 1.0f);
		mStep++;
		break;
	case 1:
		if (--mTimer == 0) {
			RestartMode(this);
		}
		break;
	}
}

/**
 * @note Address: 0x8011ED90
 * @note Size: 0x2BC
 */
void XObjArmy::StateKnock()
{
	CVec pos;
	switch (mStep) {
	case 0: {
		mFlags |= 1;
		gArmyKnockdownCount++;
		f32 angle = GetAngleY(*GetPos(), *((XObj*)gGame.mXObjs.mPools[2].mObjects)->GetPos());
		pos       = *GetPos();
		SetAnim(0xA, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		SetRotateY(angle - PI, 1.0f); // face the player
		SetMoveDir(&pos, 0.1f);
		mStep++;
		break;
	}
	case 1:
		if (GetAnimEndFrame() <= GetAnimFrame()) {
			SetMoveDir(GetPos(), 0.0f);
			mStep++;
		}
		break;
	}
}

/**
 * @note Address: 0x8011F04C
 * @note Size: 0x2BC
 */
void XObjArmy::StateFall()
{
	CVec pos;
	switch (mStep) {
	case 0: {
		mFlags |= 1;
		gArmyKnockdownCount++;
		f32 angle = GetAngleY(*GetPos(), *((XObj*)gGame.mXObjs.mPools[2].mObjects)->GetPos());
		pos       = *GetPos();
		SetAnim(0x13, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		SetRotateY(angle - PI, 1.0f); // face the player
		SetMoveDir(&pos, 0.5f);
		mStep++;
		break;
	}
	case 1:
		if (GetAnimEndFrame() <= GetAnimFrame()) {
			SetMoveDir(GetPos(), 1.0f);
			mStep++;
		}
		break;
	}
}

/**
 * @note Address: 0x8011F308
 * @note Size: 0x41C
 */
void XObjArmy::StateGuard()
{
	switch (mStep) {
	case 0:
		SetAnim(9, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		SetRotateY(GetAngleY(mGuardPos, *GetPos()), 1.0f);
		mStep++;
		break;
	case 1:
		if (20.0f <= GetAnimFrame()) { // fires at frame 20 of the aiming anim

			u32 i;
			for (i = 0; mShots[i].mActive && i < 5; i++) { }
			if (i < 5) {
				FireShot(&mShots[i], *GetPos(), CVec(mGuardPos) - *GetPos(), (mFlags & 0x80) != 0);
			}
			mStep++;
		}
		break;
	case 2: // then back to doing nothing
		if (GetAnimEndFrame() <= 2.0f + GetAnimFrame()) {
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			SetMode(this, 0);
		}
		break;
	}
}

extern "C" {

/**
 * @note Address: 0x8011F724
 * @note Size: 0x1F0
 */
void FireNextArmyShot(XObjArmy* army)
{
	CVec pos;
	CVec dir;
	pos        = *army->GetPos();
	Vec facing = { 0.0f, 0.0f, 0.0f };
	facing.x   = sin(army->GetRotY());
	facing.z   = cos(army->GetRotY());
	dir        = CVec(facing);
	u32 i;
	for (i = 0; army->mShots[i].mActive && i < 5; i++) { }
	if (i < 5) {
		FireShot(&army->mShots[i], pos, dir, (army->mFlags >> 7) & 1);
	}
}

} // extern "C"

/**
 * @note Address: 0x8011F914
 * @note Size: 0x594
 */
s32 XObjArmy::HandleEventCommand(s32 argc, s32* args)
{
	if (argc < 1) {
		return -1;
	}
	switch (args[0]) {
	case 0: // set the patrol box, restart the knockdown count
		gArmyKnockdownCount = 0;
		gArmyPatrolMin.x    = args[1] / 100.0f;
		gArmyPatrolMin.y    = args[2] / 100.0f;
		gArmyPatrolMin.z    = args[3] / 100.0f;
		gArmyPatrolMax.x    = args[4] / 100.0f;
		gArmyPatrolMax.y    = args[5] / 100.0f;
		gArmyPatrolMax.z    = args[6] / 100.0f;
		break;
	case 1: // queries
		switch (args[1]) {
		case 0:
			return gArmyKnockdownCount;
		case 5:
			return gArmyShotStatus.mFlags & 1; // whether a shot landed
		default:
			return gArmyKnockdownCount;
		}
	case 2: // commands
		switch (args[1]) {
		case 0: { // spawn inside the patrol box, marching
			Vec pos;
			pos.x = gArmyPatrolMin.x + (gArmyPatrolMax.x - gArmyPatrolMin.x) * ArmyRandf();
			pos.y = gArmyPatrolMin.y + (gArmyPatrolMax.y - gArmyPatrolMin.y) * ArmyRandf();
			pos.z = gArmyPatrolMin.z + (gArmyPatrolMax.z - gArmyPatrolMin.z) * ArmyRandf();
			SetPos(pos);
			mStatus.mFlags |= 0x10000;
			SetMode(this, 1);
			break;
		}
		case 1: // knock him over
			Knock(this);
			break;
		case 2: // stop
			mStep = 0;
			mFunc = NULL;
			break;
		case 3: // aim the shots at another object, or back at the player
			if (argc >= 2) {
				if (args[2] >= 0) {
					gArmyShotTarget = gGame.mXObjs.mObjects[args[2]];
				} else {
					gArmyShotTarget = NULL;
				}
			}
			break;
		case 4: // flag 0x80 silences the shots
			if (argc >= 3) {
				if (args[2]) {
					mFlags &= ~0x80;
				} else {
					mFlags |= 0x80;
				}
			} else {
				mFlags &= ~0x80;
			}
			break;
		case 5: // forget the landed shot
			gArmyShotStatus.mFlags &= ~1;
			break;
		case 6: // flag 2 of the shot status holds the fire
			if (argc >= 3) {
				if (args[2]) {
					gArmyShotStatus.mFlags &= ~2;
				} else {
					gArmyShotStatus.mFlags |= 2;
				}
			} else {
				gArmyShotStatus.mFlags |= 2;
			}
			break;
		}
		break;
	case 3: // fire once at a spot
		mGuardPos.x = args[1] / 100.0f;
		mGuardPos.y = args[2] / 100.0f;
		mGuardPos.z = args[3] / 100.0f;
		SetMode(this, 2);
		break;
	case 100:
		break;
	case 0x65: // flag 0x100 plays the sounds at full volume
		if (args[1] == 0) {
			mFlags &= ~0x100;
		} else {
			mFlags |= 0x100;
		}
		break;
	default:
		break;
	}
	return 0;
}

/**
 * @note Address: 0x8011FEA8
 * @note Size: 0x184
 */
void XObjArmy::OnHitFromPosition(const Vec& pos, f32 f)
{
	if (!(mFlags & 1)) { // flag 1 marks a soldier already down
		_9BC = 0.0f;
		PlayWiiSoundEffect(&gGame.mSound, 0xF0, 1.0f, 0.0f);
		if (_9BC <= 0.0f) { // always true; the StateWait branch below is dead
			if ((s32)GetPlayerActionParameter(0x1F) / 100.0f <= f || PSVECDistance(GetPos(), gGame.GetPlayer()->GetPos()) < 20.0f) {
				mStep = 0;
				mFunc = &XObjArmy::StateFall;
			} else {
				mStep = 0;
				mFunc = &XObjArmy::StateKnock;
			}
		} else {
			mStep = 0;
			mFunc = &XObjArmy::StateWait;
		}
	}
}

extern "C" {

/**
 * @note Address: 0x8012002C
 * @note Size: 0x2FC
 */
void UpdateArmyShot(XObjArmySlot* shot)
{
	if (!shot->mActive) {
		return;
	}
	CVec target;
	switch (shot->mPhase) {
	case 0: { // in flight; reaching the target bounces it back
		if (--shot->mLife == 0) {
			shot->mActive = 0;
		}
		shot->Move();
		f32 height;
		if (gArmyShotTarget != NULL) {
			target = *gArmyShotTarget->GetPos();
			height = 28.0f;
		} else {
			WiiPlayerState* player = gGame.GetPlayer();
			target                 = *player->GetPos();
			height                 = player->GetAttachment() != 6 ? 10.0f : 28.0f; // attachment 6 raises the hit height
		}
		f32 dy = target.y - shot->mPos.y;
		if (dy < 0.0f) {
			dy = -dy;
		}
		if (dy < height) {
			target.y = shot->mPos.y;
			if (PSVECDistance(&shot->mPos, &target) < 5.0f) {
				Vec hit = shot->mPos;
				PlayWiiSoundEffect(&gGame.mSound, 0x52D, 1.0f, 0.0f);
				gArmyShotStatus.mFlags |= 1;
				gGame.GetPlayer()->OnHitFromPosition(hit, 10.0f);
				shot->mVel.x = -1.0f * shot->mVel.x;
				shot->mVel.y = -1.0f;
				shot->mVel.z = -1.0f * shot->mVel.z;
				shot->mSpeed = 0.5f;
				shot->mPhase++;
			}
		}
		break;
	}
	case 1: // dropping to the floor after the hit
		shot->Move();
		shot->mVel.y += -0.15f;
		if (shot->mPos.y <= 1.5f) {
			shot->mVel.y = 0.0f;
			shot->mSpeed = 0.3f;
			shot->mLife  = 10;
			shot->mPhase++;
		}
		break;
	case 2: // slides along the floor for its last ten frames
		if (--shot->mLife == 0) {
			shot->mActive = 0;
		}
		shot->Move();
		break;
	}
}

/**
 * @note Address: 0x80120328
 * @note Size: 0x1D4
 */
void DrawArmyShot(XObjArmySlot* shot)
{
	if (!shot->mActive) {
		return;
	}
	CGs* gs = &gGame.mGs;
	if (gArmyShotModel == NULL) { // no model: a green blob
		gs->mBillboardEnabled = TRUE;
		Vec pos               = shot->mPos;
		gs->mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE, GX_LO_SET);
		Vec rot = { 0.0f, 0.0f, 0.0f };
		DrawGsCircle(gs, 16, 0x64FF64C8, 0xFF, pos.x, pos.y, pos.z, 3.0f, 3.0f, rot.x, rot.y, rot.z);
		gs->mBillboardEnabled = FALSE;
	} else { // the model joins the scene list, draws and leaves it again
		WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
		gArmyShotModel->SetTranslateX(shot->mPos.x);
		gArmyShotModel->SetTranslateY(shot->mPos.y);
		gArmyShotModel->SetTranslateZ(shot->mPos.z);
		scene->vf034(scene->mPending, gArmyShotModel->mObject.m_pScnMdl);
		((nw4r::g3d::ScnRoot*)scene)->UpdateFrame();
		((nw4r::g3d::ScnRoot*)scene)->CalcWorld();
		((nw4r::g3d::ScnRoot*)scene)->CalcView();
		((nw4r::g3d::ScnRoot*)scene)->GatherDrawScnObj();
		((nw4r::g3d::ScnRoot*)scene)->DrawOpa();
		((nw4r::g3d::ScnRoot*)scene)->DrawXlu();
		while (scene->mPending != 0) {
			scene->PopPending();
		}
	}
}

} // extern "C"

/**
 * @note Address: 0x801204FC
 * @note Size: 0xD4
 */
void XObjArmy::UpdateAnimationSounds()
{
	s32 anim;
	f32 volume;
	if (mFlags & 0x100) {
		volume = 1.0f;
	} else {
		volume = GetSoundVolumeAtPosition(&gGame.mSound, GetPos(), 0);
	}
	anim = GetAnimId();
	mSfx.Update(anim, GetAnimFrame(), volume);
}

/**
 * @note Address: 0x801205D0
 * @note Size: 0x14
 */
void XObjArmy::ResumeSounds(f32 fade)
{
	mSfx.Resume(fade);
}

/**
 * @note Address: 0x801205E4
 * @note Size: 0x14
 */
void XObjArmy::PauseSounds(f32 fade)
{
	mSfx.Pause(fade);
}

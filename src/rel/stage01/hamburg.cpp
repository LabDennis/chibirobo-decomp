#include "game/relana.h"
#include "game/relmodule.h"
#include "Version.h"
#include "game/hsd_jobj.h"
#include "game/xchr.h"
#include "game/ut.h"
#include "game/cparticle.h"
#include "game/cgame.h"
#include "game/gmath.h"
#include "game/placement.h"
#include "game/animframe.h"

extern "C" {
#include <sysdolphin/random.h>

double atan2(double y, double x);
double cos(double x);
double sin(double x);
}

struct HamDish;

struct HamObj {
	typedef void (HamObj::*StateFunc)();

	void StateStackLand();
	void StatePutDown();
	void StateSlide();
	void StateFlip();

	Vec* GetPos();
	XObj* GetActor();
	f32 GetStackHeight();

	void SetState(StateFunc func)
	{
		mState     = func;
		mStateTime = 0;
	}

	void StartSlide(f32 distance, f32 speed, f32 angle)
	{
		if (mState != &HamObj::StateFlip) {
			mMoveAngle     = angle;
			mSlideDistance = distance;
			mMoveSpeed     = speed;
			SetState(&HamObj::StateSlide);
		}
	}

	void SetRot(f32 rot)
	{
		mRotZ = rot;
		mActor->SetRotationZ(mRotZ, 0);
	}

	HamObj* mAbove;     // _00, the HamObj stacked on this one, NULL for none
	XObj* mActor;       // _04
	s32 mId;            // _08, script object id, the actor is gGame.GetObject(id); -1 when the slot is free
	s32 mType;          // _0C, 1 hamburger (kind 0x1F9), 2 kind 0x263, 3 kind 0x264, 0 other; only 1 cooks and plays the yaki sound
	f32 mRotZ;          // _10, z rotation, PI upside down; SetRot sets it and StateFlip turns the actor toward it
	f32 mFlipFrames;    // _14, frames left in the flip, set from the flip command
	f32 mMoveAngle;     // _18, yaw on the XZ plane the object slides or is put down along
	f32 mSlideDistance; // _1C, how far a slide goes, added along mMoveAngle to the move target (StartSlide's 'distance' parameter)
	f32 mMoveSpeed;     // _20, speed handed to SetMoveDir (vf_78) for slides and put-downs
	s32 mSide;          // _24, which side is cooking, 0 or 1, indexes mYakiTime; StateFlip toggles it
	s32 mYakiTime[2];   // _28, cook time per side, /600 + 1 is the doneness level; 9999 for kinds 0x263/0x264
	s32 mStackLevel;    // _30, how many objects it sits on (on a dish: the count already there); GetStackHeight adds 2 per level
	HamDish* mDish;     // _34, the HamDish it was put on, NULL for none
	u8 mSpawned; // _38, set by HamObj_Respawn, cleared once the actor is no longer drawn; while set it cooks, is slid back onto the gas
	             // plate and kept above y 77.5
	HSD_Generator* mYakiEffect; // _3C, particle 0x78 on joint path {0}, created while cooking, killed when put on a dish or at exit
	StateFunc mState;           // _40
	s32 mStateTime;             // _4C
};

struct HamDish {
	XObj* mActor;     // _00, object for the dish, NULL when the slot is free
	s32 mId;          // _04, script object id of the dish, -1 when free
	HamObj* mObjs[4]; // _08, the HamObjs put on the dish, filled in order, NULL past the last
};

static inline void AddToHamDish(HamDish* group, HamObj* slot)
{
	s32 i;
	for (i = 0; i < ARRAY_SIZE(group->mObjs); i++) {
		if (group->mObjs[i] == NULL) {
			group->mObjs[i] = slot;
			break;
		}
	}
}

#define STAGE01_EVT_DATA (&gGame.mEvt.mData)
#define STAGE01_PARTICLE (&gGame.mParticle)
#define STAGE01_SOUND    (&gGame.mSound)

extern "C" {
void AllowIdleHudCounters(InfoScrn* screen);
void SuppressIdleHudCounters(InfoScrn* screen);

s32 CmdPlaySoundAtActor(s32 cmd, s32 count, s32* args);
s32 CmdCheckYakiResult(s32 cmd, s32 count, s32* args);
s32 CmdPlayYakiSound(s32 cmd, s32 count, s32* args);
s32 CmdSetYakiRates(s32 cmd, s32 count, s32* args);
s32 CmdGetHamDishCount(s32 cmd, s32 count, s32* args);
s32 CmdExitHamburg(s32 cmd, s32 count, s32* args);
s32 CmdGetHeldHamObjId(s32 cmd, s32 count, s32* args);
s32 CmdGetYakiTime(s32 cmd, s32 count, s32* args);
s32 CmdPutHeldHamObjOnDish(s32 cmd, s32 count, s32* args);
s32 CmdPutDownHamObj(s32 cmd, s32 count, s32* args);
s32 CmdSetHamObjRot(s32 cmd, s32 count, s32* args);
s32 CmdGetFreeHamObjId(s32 cmd, s32 count, s32* args);
s32 CmdRespawnHamObj(s32 cmd, s32 count, s32* args);
s32 CmdPushHamObj(s32 cmd, s32 count, s32* args);
s32 CmdFlipHamburg(s32 cmd, s32 count, s32* args);
s32 CmdGetFacingHamburgId(s32 cmd, s32 count, s32* args);
s32 CmdRegisterHamObjs(s32 cmd, s32 count, s32* args);
s32 CmdRegisterHamDishes(s32 cmd, s32 count, s32* args);
s32 CmdGetHamObjKind(s32 cmd, s32 count, s32* args);
s32 CmdInitHamburg(s32 cmd, s32 count, s32* args);
s32 CmdHamburgNoOpSlot5(s32 cmd, s32 count, s32* args);
s32 CmdHamburgNoOpSlot4(s32 cmd, s32 count, s32* args);
s32 CmdHamburgNoOpSlot3(s32 cmd, s32 count, s32* args);
s32 CmdHamburgNoOpSlot2(s32 cmd, s32 count, s32* args);
s32 CmdHamburgNoOpSlot1(s32 cmd, s32 count, s32* args);
s32 CmdHamburgNoOpSlot0(s32 cmd, s32 count, s32* args);
void DrawStage(s32 arg);
void DrawStageHud(s32 arg);
void UpdateStage(s32 arg);
void InitStage(s32 arg);
s32 CheckYakiResult(void);
void AddTempActor(s32 id);
void ClearTempActors(void);
HamDish* FindHamDish(s32 id);
HamDish* RegisterHamDish(s32 id);
s32 CountHamDishObjs(HamDish* group);
void SetYakiRates(u8* data);
s32 PutHeldHamObjOnDish(s32 id);
bool HamObj_CheckSlideHit(HamObj* slot);
void UpdateHamPickupFlags(void);
HamObj* FindFreeHamObj(void);
HamObj* FindFacingHamburg(void);
HamObj* FindHamObjByActor(XObj* actor);
HamObj* FindHamObj(s32 id);
HamObj* RegisterHamObj(s32 id);
void ExitHamburg(void);
void InitHamburg(void);
void UpdateHamObjs(u32 arg);
void ResetYakiRates(void);
void PlayYakiSound(void);
void GetPos(HamObj* slot);
void GetSize(HamObj* slot);
void GetStackHeight(HamObj* slot);
void HamObj_SetRot(HamObj* slot, f32 angle);
void HamObj_StartPutDown(HamObj* slot);
void HamObj_Respawn(HamObj* slot);
void HamObj_StartSlide(HamObj* slot, f32 distance, f32 speed, f32 angle);
void HamObj_StartFlip(HamObj* slot, u32 frames);
void HamObj_Update(HamObj* slot, u32 step);
void HamObj_LowerStack(HamObj* slot);
}

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage, DrawStage, NULL, DrawStageHud, NULL, NULL };
RelCmdFunc _prolog_cmds[]    = {
    CmdHamburgNoOpSlot0,
    CmdHamburgNoOpSlot1,
    CmdHamburgNoOpSlot2,
    CmdHamburgNoOpSlot3,
    CmdHamburgNoOpSlot4,
    CmdHamburgNoOpSlot5,
    CmdInitHamburg,
    CmdGetHamObjKind,
    CmdRegisterHamDishes,
    CmdRegisterHamObjs,
    CmdGetFacingHamburgId,
    CmdFlipHamburg,
    CmdPushHamObj,
    CmdRespawnHamObj,
    CmdGetFreeHamObjId,
    CmdSetHamObjRot,
    CmdPutDownHamObj,
    CmdPutHeldHamObjOnDish,
    CmdGetYakiTime,
    CmdGetHeldHamObjId,
    CmdExitHamburg,
    CmdGetHamDishCount,
    CmdAddAna,
    CmdSetTutiAna,
    CmdFindNearestAna,
    CmdPlantTane,
    CmdSetAnaJObj,
    CmdSetYakiRates,
    CmdSetAnaRotY,
    CmdPlayYakiSound,
    CmdAttachAnaToRobo,
    CmdCheckYakiResult,
    CmdPlaySoundAtActor,
};
}

static f32 yakiRates[6];  // cook-speed multiplier per doneness level, [0] unused
static u32 yakiLoopLevel; // 0 none, 1 something cooks, 2 a side is well done
static u8 playerOffGasPlate;
static u8 yakiSoundPlayed;
static HamObj hamObjs[9];
static XObj* gasPlate;
static HamDish hamDishes[4];
static XObj* tempActors[4];
static CStatus yakiResultFlags; // bits of the results already reported once
static u8 putDownActive;        // a put-down runs: pickup flags stay off

static inline bool IsActive(XObj* actor)
{
	if (actor == NULL) {
		return false;
	}
	if (!(actor->mStatus.mFlags & 0x10000)) {
		return false;
	}
	return true;
}

static inline bool IsHeld(XObj* actor)
{
	XObjChr* player = gGame.GetPlayer();
	bool held       = false;
	if (player->mLift.mState != 0 && player->mLift.mTarget == actor) {
		held = true;
	}
	return held;
}

static inline bool IsOnGasPlate(XObj* actor)
{
	f32 x = actor->mMoveTranslate.x;
	f32 z = actor->mMoveTranslate.z;
	if (x < -289.0f || -239.0f < x || z < 21.4f || 62.6f < z) {
		return false;
	}
	return true;
}

static inline bool IsOnTopOf(HamObj* slot, HamObj* base)
{
	return slot->mAbove != NULL ? base->mId == slot->mAbove->mId : false;
}

static inline bool IsSamePos(const Vec& a, Vec b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

static inline HamObj* FindHamObjByActorInline(XObj* actor)
{
	s32 i;
	if (actor == NULL) {
		return NULL;
	}
	for (i = 0; i < 9; i++) {
		XObj* current = hamObjs[i].mActor;
		if (current != NULL && current->mPoolIndex == actor->mPoolIndex) {
			return &hamObjs[i];
		}
	}
	return NULL;
}

static inline bool IsLeavingGasPlate(HamObj* slot)
{
	f32 x = slot->mActor->mMoveTranslate.x;
	f32 z = slot->mActor->mMoveTranslate.z;
	if ((62.6f < z && 0.0f < (f32)cos(slot->mMoveAngle)) || (z < 21.4f && (f32)cos(slot->mMoveAngle) < 0.0f)
	    || (-239.0f < x && 0.0f < (f32)sin(slot->mMoveAngle)) || (x < -289.0f && (f32)sin(slot->mMoveAngle) < 0.0f)) {
		return true;
	}
	return false;
}

/**
 * @note Address: stage01 .text 0x3768
 * @note Size: 0xC
 */
inline Vec* HamObj::GetPos()
{
	return &mActor->mMoveTranslate;
}

/**
 * @note Address: stage01 .text 0x3838
 * @note Size: 0x8
 */
inline XObj* HamObj::GetActor()
{
	return mActor;
}

/**
 * @note Address: stage01 .text 0x3848
 * @note Size: 0xF0
 */
inline f32 HamObj::GetStackHeight()
{
	return PlacementPosition(GetActor()->mCoord).y + 2.0f * mStackLevel;
}

// yaw on the XZ plane
static inline f32 AngleFrom(const Vec* from, const Vec* to)
{
	return atan2(to->x - from->x, to->z - from->z);
}

static inline HamObj* FindStackBase(Vec pos, s32 id, s32 level, f32 maxDist)
{
	u32 i;
	HamObj* best = NULL;

	for (i = 0; i < ARRAY_SIZE(hamObjs); i++) {
		HamObj* other = &hamObjs[i];
		if (IsActive(other->mActor) && id != other->mId && level == other->mStackLevel && other->mDish == NULL && !IsHeld(other->mActor)) {
			pos.y    = other->mActor->mMoveTranslate.y;
			f32 dist = PSVECDistance(&other->mActor->mMoveTranslate, &pos);
			if (dist < maxDist) {
				best    = other;
				maxDist = dist;
			}
		}
	}
	return best;
}

/**
 * @note Address: stage01 .text 0x0
 * @note Size: 0x90
 */
extern "C" s32 CmdPlaySoundAtActor(s32 cmd, s32 count, s32* args)
{
	XObj* actor = gGame.GetObject(args[1]);
	Vec pos     = actor->mMoveTranslate;
	s32 id      = args[0];
	PlayGameSound(STAGE01_SOUND, id, GetSoundVolumeAtPosition(STAGE01_SOUND, &pos, 0));
	return 0;
}

/**
 * @note Address: stage01 .text 0x90
 * @note Size: 0x20
 */
extern "C" s32 CmdCheckYakiResult(s32 cmd, s32 count, s32* args)
{
	return CheckYakiResult();
}

/**
 * @note Address: stage01 .text 0xB0
 * @note Size: 0x24
 */
extern "C" s32 CmdPlayYakiSound(s32 cmd, s32 count, s32* args)
{
	PlayYakiSound();
	return 0;
}

/**
 * @note Address: stage01 .text 0xD4
 * @note Size: 0x28
 */
extern "C" s32 CmdSetYakiRates(s32 cmd, s32 count, s32* args)
{
	SetYakiRates(reinterpret_cast<u8*>(args[0]));
	return 0;
}

/**
 * @note Address: stage01 .text 0xFC
 * @note Size: 0x38
 */
extern "C" s32 CmdGetHamDishCount(s32 cmd, s32 count, s32* args)
{
	HamDish* group = FindHamDish(args[0]);
	if (group != NULL) {
		return CountHamDishObjs(group);
	}
	return 0;
}

/**
 * @note Address: stage01 .text 0x134
 * @note Size: 0x24
 */
extern "C" s32 CmdExitHamburg(s32 cmd, s32 count, s32* args)
{
	ExitHamburg();
	return 1;
}

/**
 * @note Address: stage01 .text 0x158
 * @note Size: 0x48
 */
extern "C" s32 CmdGetHeldHamObjId(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObjByActor(gGame.GetPlayer()->mLift.mTarget);
	if (slot != NULL) {
		return slot->mId;
	}
	return -1;
}

/**
 * @note Address: stage01 .text 0x1A0
 * @note Size: 0x50
 */
extern "C" s32 CmdGetYakiTime(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot != NULL) {
		return slot->mYakiTime[args[1]];
	}
	return -1;
}

/**
 * @note Address: stage01 .text 0x1F0
 * @note Size: 0x24
 */
extern "C" s32 CmdPutHeldHamObjOnDish(s32 cmd, s32 count, s32* args)
{
	return PutHeldHamObjOnDish(args[0]);
}

/**
 * @note Address: stage01 .text 0x214
 * @note Size: 0x90
 */
extern "C" s32 CmdPutDownHamObj(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	s32 i;
	if (slot == NULL) {
		return -1;
	}

	ClearTempActors();
	for (i = 1; i < count; i++) { // the remaining args are actors drawn during the put-down
		AddTempActor(args[i]);
	}
	HamObj_StartPutDown(slot);
	return 0;
}

/**
 * @note Address: stage01 .text 0x2A4
 * @note Size: 0x78
 */
extern "C" s32 CmdSetHamObjRot(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot == NULL) {
		return -1;
	}
	HamObj_SetRot(slot, DEG2RAD * args[1]);
	return 0;
}

/**
 * @note Address: stage01 .text 0x31C
 * @note Size: 0x34
 */
extern "C" s32 CmdGetFreeHamObjId(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindFreeHamObj();
	if (slot != NULL) {
		return slot->mId;
	}
	return -1;
}

/**
 * @note Address: stage01 .text 0x350
 * @note Size: 0x3C
 */
extern "C" s32 CmdRespawnHamObj(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot == NULL) {
		return 0;
	}
	HamObj_Respawn(slot);
	return 1;
}

/**
 * @note Address: stage01 .text 0x38C
 * @note Size: 0xB8
 */
extern "C" s32 CmdPushHamObj(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot == NULL) {
		return 0;
	}

	// slide it away from the player
	f32 angle = atan2(slot->mActor->mMoveTranslate.x - gGame.GetPlayer()->mMoveTranslate.x,
	                  slot->mActor->mMoveTranslate.z - gGame.GetPlayer()->mMoveTranslate.z);
	HamObj_StartSlide(slot, args[1], 0.2f, angle);
	return 1;
}

/**
 * @note Address: stage01 .text 0x444
 * @note Size: 0x44
 */
extern "C" s32 CmdFlipHamburg(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot != NULL) {
		HamObj_StartFlip(slot, args[1]);
	}
	return 1;
}

/**
 * @note Address: stage01 .text 0x488
 * @note Size: 0x34
 */
extern "C" s32 CmdGetFacingHamburgId(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindFacingHamburg();
	if (slot != NULL) {
		return slot->mId;
	}
	return -1;
}

/**
 * @note Address: stage01 .text 0x4BC
 * @note Size: 0x60
 */
extern "C" s32 CmdRegisterHamObjs(s32 cmd, s32 count, s32* args)
{
	s32 i;
	for (i = 0; i < count; i++) {
		RegisterHamObj(args[i]);
	}
	return 1;
}

/**
 * @note Address: stage01 .text 0x51C
 * @note Size: 0x60
 */
extern "C" s32 CmdRegisterHamDishes(s32 cmd, s32 count, s32* args)
{
	s32 i;
	for (i = 0; i < count; i++) {
		RegisterHamDish(args[i]);
	}
	return 1;
}

/**
 * @note Address: stage01 .text 0x57C
 * @note Size: 0x40
 */
extern "C" s32 CmdGetHamObjKind(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot != NULL) {
		return slot->mActor->mCoord.mPlacement->mObjectId;
	}
	return -1;
}

/**
 * @note Address: stage01 .text 0x5BC
 * @note Size: 0x24
 */
extern "C" s32 CmdInitHamburg(s32 cmd, s32 count, s32* args)
{
	InitHamburg();
	return 0;
}

/**
 * @note Address: stage01 .text 0x5E0
 * @note Size: 0x8
 */
extern "C" s32 CmdHamburgNoOpSlot5(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage01 .text 0x5E8
 * @note Size: 0x8
 */
extern "C" s32 CmdHamburgNoOpSlot4(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage01 .text 0x5F0
 * @note Size: 0x8
 */
extern "C" s32 CmdHamburgNoOpSlot3(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage01 .text 0x5F8
 * @note Size: 0x8
 */
extern "C" s32 CmdHamburgNoOpSlot2(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage01 .text 0x600
 * @note Size: 0x8
 */
extern "C" s32 CmdHamburgNoOpSlot1(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage01 .text 0x608
 * @note Size: 0x8
 */
extern "C" s32 CmdHamburgNoOpSlot0(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage01 .text 0x610
 * @note Size: 0x20
 */
extern "C" void DrawStage(s32 arg)
{
	DrawAnas();
}

/**
 * @note Address: stage01 .text 0x630
 * @note Size: 0x4
 */
extern "C" void DrawStageHud(s32 arg)
{
}

/**
 * @note Address: stage01 .text 0x634
 * @note Size: 0x218
 */
extern "C" void UpdateStage(s32 arg)
{
	Vec pos;

	UpdateHamObjs(arg);
	UpdateAnas(arg);

	// keep the running stage sounds' volume at their two emitter positions
	pos.x = -92.52f;
	pos.y = 75.87f;
	pos.z = -165.98f;
	if (IsGameSoundPlaying(STAGE01_SOUND, 0x40016)) {
		SetGameSfxVolume(STAGE01_SOUND, 0x40016, GetSoundVolumeAtPosition(STAGE01_SOUND, &pos, 3), 0);
	}
	if (IsGameSoundPlaying(STAGE01_SOUND, 0x40017)) {
		SetGameSfxVolume(STAGE01_SOUND, 0x40017, GetSoundVolumeAtPosition(STAGE01_SOUND, &pos, 3), 0);
	}
	if (IsGameSoundPlaying(STAGE01_SOUND, 0x40018)) {
		SetGameSfxVolume(STAGE01_SOUND, 0x40018, GetSoundVolumeAtPosition(STAGE01_SOUND, &pos, 3), 0);
	}
	if (IsGameSoundPlaying(STAGE01_SOUND, 0x40019)) {
		SetGameSfxVolume(STAGE01_SOUND, 0x40019, GetSoundVolumeAtPosition(STAGE01_SOUND, &pos, 3), 0);
	}

	pos.x = -281.1f;
	pos.y = 76.14f;
	pos.z = 110.23f;
	if (IsGameSoundPlaying(STAGE01_SOUND, 0x4000E)) {
		SetGameSfxVolume(STAGE01_SOUND, 0x4000E, GetSoundVolumeAtPosition(STAGE01_SOUND, &pos, 2), 0);
	}
	if (IsGameSoundPlaying(STAGE01_SOUND, 0x40010)) {
		SetGameSfxVolume(STAGE01_SOUND, 0x40010, GetSoundVolumeAtPosition(STAGE01_SOUND, &pos, 2), 0);
	}
}

/**
 * @note Address: stage01 .text 0x84C
 * @note Size: 0x24
 */
extern "C" void InitStage(s32 arg)
{
	ResetYakiRates();
	InitAnas();
}

/**
 * @note Address: stage01 .text 0x870
 * @note Size: 0x280
 */
extern "C" s32 CheckYakiResult(void)
{
	u32 i;
	s32 result;
	XObjChr* player;
	HamObj* slot;

	if (gGame.mEvt.mData.mScript != NULL) {
		return 0;
	}

	player = gGame.GetPlayer();
	// the player is away from the plate
	if (player->mMoveTranslate.x < -295.0f || -225.0 < player->mMoveTranslate.x || player->mMoveTranslate.z < 10.0f
	    || 74.0f < player->mMoveTranslate.z) {
		return 9;
	}

	// a hamburger done on both sides (doneness 3+) reports 1, once
	result = 0;
	if (result == 0 && !(yakiResultFlags.mFlags & 2)) {
		for (i = 0; i < ARRAY_SIZE(hamObjs) && result == 0; i++) {
			slot = &hamObjs[i];
			if (IsActive(slot->mActor) && slot->mActor->mCoord.mPlacement->mObjectId == 0x1F9 && slot->mYakiTime[0] / 600 + 1 >= 3
			    && slot->mYakiTime[1] / 600 + 1 >= 3) {
				result = 1;
			}
		}
		if (result != 0) {
			yakiResultFlags.mFlags |= 2;
		}
	}

	// a burnt hamburger side (doneness 5) reports 2, once
	if (result == 0 && !(yakiResultFlags.mFlags & 4)) {
		for (i = 0; i < ARRAY_SIZE(hamObjs) && result == 0; i++) {
			slot = &hamObjs[i];
			if (IsActive(slot->mActor) && slot->mActor->mCoord.mPlacement->mObjectId == 0x1F9
			    && (slot->mYakiTime[0] / 600 + 1 >= 5 || slot->mYakiTime[1] / 600 + 1 >= 5)) {
				result = 2;
			}
		}
		if (result != 0) {
			yakiResultFlags.mFlags |= 4;
		}
	}
	return result;
}

// CheckYakiResult

/**
 * @note Address: stage01 .text 0xAF0
 * @note Size: 0xA4
 */
extern "C" void AddTempActor(s32 id)
{
	s32 i;
	for (i = 0; i < ARRAY_SIZE(tempActors); i++) {
		if (tempActors[i] == NULL) {
			XObj* actor;
			if (gGame.mEvt.mData.mScript != NULL) {
				actor = ResolveEventObjectId(STAGE01_EVT_DATA, id);
			} else {
				actor = NULL;
			}
			tempActors[i] = actor;
			tempActors[i]->mStatus.mFlags |= 0x10000; // draw it; only valid while a script runs
			return;
		}
	}
}

/**
 * @note Address: stage01 .text 0xB94
 * @note Size: 0xBC
 */
extern "C" void ClearTempActors(void)
{
	s32 i;
	for (i = 0; i < ARRAY_SIZE(tempActors); i++) {
		if (tempActors[i] != NULL) {
			tempActors[i]->mStatus.mFlags &= ~0x10000ULL; // stop drawing it
			tempActors[i] = NULL;
		}
	}
}

/**
 * @note Address: stage01 .text 0xC50
 * @note Size: 0x54
 */
extern "C" HamDish* FindHamDish(s32 id)
{
	s32 i;
	for (i = 0; i < ARRAY_SIZE(hamDishes); i++) {
		if (hamDishes[i].mActor != NULL && id == hamDishes[i].mId) {
			return &hamDishes[i];
		}
	}
	return NULL;
}

/**
 * @note Address: stage01 .text 0xCA4
 * @note Size: 0xDC
 */
extern "C" HamDish* RegisterHamDish(s32 id)
{
	s32 i;
	HamDish* group = FindHamDish(id);
	if (group != NULL) {
		return group;
	}

	for (i = 0; i < ARRAY_SIZE(hamDishes); i++) {
		if (hamDishes[i].mActor == NULL) {
			group           = &hamDishes[i];
			group->mId      = id;
			group->mActor   = gGame.GetObject(id);
			group->mObjs[0] = NULL;
			group->mObjs[1] = NULL;
			group->mObjs[2] = NULL;
			group->mObjs[3] = NULL;
			return group;
		}
	}
	return NULL;
}

/**
 * @note Address: stage01 .text 0xD80
 * @note Size: 0x58
 */
extern "C" s32 CountHamDishObjs(HamDish* group)
{
	s32 i;
	for (i = 0; i < ARRAY_SIZE(group->mObjs); i++) {
		if (group->mObjs[i] == NULL) {
			return i;
		}
	}
	return ARRAY_SIZE(group->mObjs);
}

/**
 * @note Address: stage01 .text 0xDD8
 * @note Size: 0x180
 */
extern "C" void SetYakiRates(u8* data)
{
	s32 i;
	for (i = 0; i < 5; i++) {
		// a little-endian s32 per doneness level, /100; slot 0 keeps the reset 1.0
		yakiRates[i + 1] = (s32)(data[0] + data[1] * 0x100 + data[2] * 0x10000 + data[3] * 0x1000000) / 100.0f;
		data += 4;
	}
}

/**
 * @note Address: stage01 .text 0xF58
 * @note Size: 0x174
 */
extern "C" s32 PutHeldHamObjOnDish(s32 id)
{
	// NONMATCHING: regalloc
	XObjChr* player = gGame.GetPlayer();
	HamObj* slot;
	HamDish* group;

	if (IsLiftTargetPlug(&player->mLift)) {
		return -1;
	}

	slot = FindHamObjByActorInline(player->mLift.mTarget);
	if (slot == NULL) {
		return -1;
	}

	group       = FindHamDish(id);
	slot->mDish = group;
	AddToHamDish(group, slot);

	if (player->mLift.mState == 4) {
		player->mLift.mState = 6; // play the put-down animation
	}
	return slot->mId;
}

// PutHeldHamObjOnDish

/**
 * @note Address: stage01 .text 0x10CC
 * @note Size: 0x3B0
 */
extern "C" bool HamObj_CheckSlideHit(HamObj* slot)
{
	// NONMATCHING: regalloc
	HamObj* best = FindStackBase(slot->mActor->mMoveTranslate, slot->mId, slot->mStackLevel, 10.0f);

	if (best != NULL) {
		f32 angle = AngleFrom(&slot->mActor->mMoveTranslate, &best->mActor->mMoveTranslate);
		// inside a ~58-degree cone of the slide direction it gets pushed (the literal is 45 radians)
		f32 c = cos(angle - slot->mMoveAngle);
		if ((f32)cos(45.0) < c) {
			f32 distance;
			angle    = AngleFrom(&slot->mActor->mMoveTranslate, &best->mActor->mMoveTranslate);
			distance = slot->mSlideDistance * 0.5f; // it takes half the slide distance, up to 10
			if (10.0f < distance) {
				distance = 10.0f;
			}
			best->StartSlide(distance, slot->mMoveSpeed, angle);
			if (IsOnGasPlate(slot->mActor)) {
				return true;
			}
		}
	}

	if (IsLeavingGasPlate(slot)) { // sliding off the plate ends the move
		return true;
	}
	return false;
}

// HamObj_CheckSlideHit

/**
 * @note Address: stage01 .text 0x147C
 * @note Size: 0x2E4
 */
extern "C" void UpdateHamPickupFlags(void)
{
	// NONMATCHING: regalloc — retail keeps two zeroed locals (best and the inner
	// loop counter) in adjacent callee-saved slots so the inner counter is formed
	// by copying the pointer slot instead of a fresh li; the built code uses
	// independent li forms and different callee registers throughout
	u32 i;
	u32 j;

	putDownActive = 0;
	for (i = 0; i < ARRAY_SIZE(hamObjs); i++) {
		HamObj* slot = &hamObjs[i];
		if (!IsActive(slot->mActor)) {
			continue;
		}
		if (slot->mDish != NULL) {
			slot->mActor->mStatus.mFlags &= ~0x100000000ULL; // on a dish it can't be picked up
			slot->mActor->mStatus.mFlags &= ~0x10000000000ULL;
		} else {
			Vec pos      = slot->mActor->mMoveTranslate;
			HamObj* best = NULL;
			s32 id       = slot->mId;
			s32 level    = slot->mStackLevel + 1;
			f32 bestDist = 7.0f;
			for (j = 0; j < ARRAY_SIZE(hamObjs); j++) {
				HamObj* other = &hamObjs[j];
				if (IsActive(other->mActor) && id != other->mId && level == other->mStackLevel && other->mDish == NULL
				    && !IsHeld(other->mActor)) {
					pos.y    = other->mActor->mMoveTranslate.y;
					f32 dist = PSVECDistance(&other->mActor->mMoveTranslate, &pos);
					if (dist < bestDist) {
						best     = other;
						bestDist = dist;
					}
				}
			}
			if (best == NULL) {
				slot->mActor->mStatus.mFlags |= 0x100000000ULL; // nothing sits on it: it can be picked up
			}
		}
	}
	ClearTempActors();
}

// UpdateHamPickupFlags

/**
 * @note Address: stage01 .text 0x1760
 * @note Size: 0x8C
 */
extern "C" HamObj* FindFreeHamObj(void)
{
	s32 i;
	for (i = 0; i < ARRAY_SIZE(hamObjs); i++) {
		if (!IsActive(hamObjs[i].mActor)) {
			return &hamObjs[i];
		}
	}
	return NULL;
}

/**
 * @note Address: stage01 .text 0x17EC
 * @note Size: 0x1C8
 */
extern "C" HamObj* FindFacingHamburg(void)
{
	f32 dist;
	f32 c;
	HamObj* best = NULL;
	f32 bestDist = 0.0f;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(hamObjs); i++) {
		HamObj* slot = &hamObjs[i];
		XObj* actor  = slot->mActor;
		if (IsActive(actor) && (actor->mStatus.mFlags & 0x100000000ULL) && slot->mStackLevel <= 0
		    && actor->mCoord.mPlacement->mObjectId == 0x1F9) {
			dist      = PSVECDistance(&gGame.GetPlayer()->mMoveTranslate, &actor->mMoveTranslate);
			Vec* pos  = slot->GetPos();
			Vec* ppos = &gGame.GetPlayer()->mMoveTranslate;
			// the nearest pickable hamburger in a 45-degree cone within 15 units
			f32 angle = atan2(pos->x - ppos->x, pos->z - ppos->z);
			c         = cos(angle - gGame.GetPlayer()->mMoveRotate.y);
			if ((f32)cos(HALF_PI * 0.5f) < c && dist < 15.0f && (best == NULL || dist < bestDist)) {
				best     = slot;
				bestDist = dist;
			}
		}
	}
	return best;
}

// FindFacingHamburg

/**
 * @note Address: stage01 .text 0x19B4
 * @note Size: 0x68
 */
extern "C" HamObj* FindHamObjByActor(XObj* actor)
{
	s32 i;
	if (actor == NULL) {
		return NULL;
	}
	for (i = 0; i < ARRAY_SIZE(hamObjs); i++) {
		XObj* current = hamObjs[i].mActor;
		if (current != NULL && current->mPoolIndex == actor->mPoolIndex) {
			return &hamObjs[i];
		}
	}
	return NULL;
}

// FindHamObjByActor

/**
 * @note Address: stage01 .text 0x1A1C
 * @note Size: 0xD8
 */
extern "C" HamObj* FindHamObj(s32 id)
{
	// NONMATCHING: regalloc
	s32 i;
	if (id < 0) {
		return FindHamObjByActorInline(gGame.GetPlayer()->mLift.mTarget); // -1 asks for the held object
	}
	for (i = 0; i < ARRAY_SIZE(hamObjs); i++) {
		if (hamObjs[i].mActor != NULL && id == hamObjs[i].mId) {
			return &hamObjs[i];
		}
	}
	return NULL;
}

// FindHamObj

/**
 * @note Address: stage01 .text 0x1AF4
 * @note Size: 0x214
 */
extern "C" HamObj* RegisterHamObj(s32 id)
{
	// NONMATCHING: regalloc — retail forms the mActor field address once and
	// reloads the pointer through it for both flag updates; the built code uses
	// the field offset form and different callee registers
	s32 i;
	HamObj* slot = FindHamObj(id);
	if (slot != NULL) {
		return slot;
	}

	for (i = 0; i < ARRAY_SIZE(hamObjs); i++) {
		if (hamObjs[i].mActor == NULL) {
			slot         = &hamObjs[i];
			slot->mId    = id;
			slot->mActor = gGame.GetObject(id);
			slot->mSide  = 0;
			slot->mDish  = NULL;
			slot->mAbove = NULL;
			switch (slot->mActor->mCoord.mPlacement->mObjectId) {
			case 0x1F9:
				slot->mYakiTime[1] = 0;
				slot->mYakiTime[0] = 0;
				slot->mType        = 1;
				break;
			case 0x263:
				slot->mYakiTime[1] = 9999;
				slot->mYakiTime[0] = 9999;
				slot->mType        = 2;
				break;
			case 0x264:
				slot->mYakiTime[1] = 9999;
				slot->mYakiTime[0] = 9999;
				slot->mType        = 3;
				break;
			default:
				hamObjs[i].mType = 0;
				break;
			}
			if (slot->mActor != NULL) {
				slot->mActor->mStatus.mFlags |= 0x100000000ULL; // the ham-game flags
				slot->mActor->mStatus.mFlags |= 0x10000000000ULL;
			}
			return slot;
		}
	}
	return NULL;
}

// RegisterHamObj

/**
 * @note Address: stage01 .text 0x1D08
 * @note Size: 0x108
 */
extern "C" void ExitHamburg(void)
{
	s32 i;

	if (gasPlate != NULL) {
		gasPlate->mStatus.mFlags |= 0x200000000ULL;
	}

	for (i = 0; i < ARRAY_SIZE(hamObjs); i++) {
		HamObj* slot = &hamObjs[i];
		if (slot->mYakiEffect != 0) {
			STAGE01_PARTICLE->Kill(slot->mYakiEffect);
		}
		slot->mId         = -1;
		slot->mActor      = NULL;
		slot->mState      = NULL;
		slot->mYakiEffect = 0;
	}
	gasPlate = NULL;

	psKillAllParticle();
	StopGameSfx(STAGE01_SOUND, 0x4001C);
	StopGameSfx(STAGE01_SOUND, 0x4001D);
	AllowIdleHudCounters(&gGame.mInfoScrn);
}

/**
 * @note Address: stage01 .text 0x1E10
 * @note Size: 0x1B4
 */
extern "C" void InitHamburg(void)
{
	// NONMATCHING: regalloc/scheduling — the unrolled hamDishes init, the
	// zero/one constant slots and the gasPlate scan setup are interleaved
	// differently and land in different callee registers
	s32 i;
	s32 count;

	for (i = 0; i < ARRAY_SIZE(hamObjs); i++) {
		hamObjs[i].mId         = -1;
		hamObjs[i].mActor      = NULL;
		hamObjs[i].mState      = NULL;
		hamObjs[i].mYakiEffect = 0;
	}

	for (i = 0; i < ARRAY_SIZE(hamDishes); i++) {
		hamDishes[i].mId    = -1;
		hamDishes[i].mActor = NULL;
	}

	CGame* game = &gGame;
	count       = game->mXObjs.mObjectCount;
	gasPlate    = NULL;
	for (i = 0; gasPlate == NULL && i < count; i++) {
		XObj* actor = game->GetObject(i);
		if ((actor->mStatus.mFlags & 1) && actor->mCoord.mPlacement->mObjectId == 0x200) { // a live actor of the gas-plate kind
			gasPlate = actor;
		}
	}
	if (gasPlate != NULL) {
		gasPlate->mStatus.mFlags &= ~0x200000000ULL;
	}

	for (i = 0; i < ARRAY_SIZE(tempActors); i++) {
		tempActors[i] = NULL;
	}
	yakiResultFlags.mFlags = 0;
	putDownActive          = 0;
	SuppressIdleHudCounters(&gGame.mInfoScrn);
}

// InitHamburg

/**
 * @note Address: stage01 .text 0x1FC4
 * @note Size: 0x234
 */
extern "C" void UpdateHamObjs(u32 arg)
{
	s32 i;

	if (gasPlate == NULL) {
		return;
	}

	XObjChr* player = gGame.GetPlayer();
	f32 x           = player->mMoveTranslate.x;
	f32 z           = player->mMoveTranslate.z;
	if (x < -289.0f || -239.0f < x || z < 21.4f || 62.6f < z) {
		playerOffGasPlate = 1;
	} else {
		playerOffGasPlate = 0;
	}

	yakiLoopLevel = 0;
	for (i = 0; i < ARRAY_SIZE(hamObjs); i++) {
		if (IsActive(hamObjs[i].mActor)) {
			HamObj_Update(&hamObjs[i], arg);
		}
	}

	// the cooking loop: one sound while something cooks, another once a side is well done
	switch (yakiLoopLevel) {
	case 1:
		StopGameSfx(STAGE01_SOUND, 0x4001D);
		PlayGameSound(STAGE01_SOUND, 0x4001C, 0xFF);
		SetGameSfxVolume(STAGE01_SOUND, 0x4001C, 0xFF, 0);
		break;
	case 2:
		StopGameSfx(STAGE01_SOUND, 0x4001C);
		PlayGameSound(STAGE01_SOUND, 0x4001D, 0xFF);
		SetGameSfxVolume(STAGE01_SOUND, 0x4001D, 0xFF, 0);
		break;
	default:
		FadeOutGameSfx(STAGE01_SOUND, 0x4001C, 600);
		FadeOutGameSfx(STAGE01_SOUND, 0x4001D, 600);
		break;
	}
}

/**
 * @note Address: stage01 .text 0x21F8
 * @note Size: 0x28
 */
extern "C" void ResetYakiRates(void)
{
	s32 i;
	for (i = 0; i < 6; i++) {
		yakiRates[i] = 1.0f;
	}
}

/**
 * @note Address: stage01 .text 0x2220
 * @note Size: 0x48
 */
extern "C" void PlayYakiSound(void)
{
	PlayGameSound(STAGE01_SOUND, 0x4001B, 0xFF);
	yakiSoundPlayed = 1;
}

/**
 * @note Address: stage01 .text 0x2268
 * @note Size: 0xE8
 */
void HamObj::StateStackLand()
{
	f32 dist = PSVECDistance(&mActor->mMoveTranslate, &mActor->mMoveTranslate.mTarget);
	if (HamObj_CheckSlideHit(this)) {
		mActor->SetMoveDir(&mActor->mMoveTranslate, 0.0f);
		SetState(NULL);
	} else if (dist <= 0.001f) {
		SetState(NULL);
	}
}

/**
 * @note Address: stage01 .text 0x2350
 * @note Size: 0x9C4
 */
void HamObj::StatePutDown()
{
	// NONMATCHING: regalloc
	XObjChr* player = gGame.GetPlayer();
	s32 level;

	switch (mStateTime) {
	case 0: {
		// wait until the player's put-down anim passes frame 15
		if (player->mLift.mState != 6 || player->mAnimId != 9) {
			break;
		}
		if (GetAnimFrame(player->mJObj) < 15.0f) {
			break;
		}

		if (IsHeld(mActor)) {
			gGame.GetPlayer()->mLift.mTarget = NULL; // the player lets go
		}
		mActor->mStatus.mFlags &= ~0x400000000000000ULL; // drop the carried flag
		mMoveAngle = player->mMoveRotate.y;
		mMoveSpeed = 2.0f;
		Vec pos    = mActor->mMoveTranslate; // aim 5 units ahead in the player's facing
		pos.x += 5.0f * (f32)sin(mMoveAngle);
		pos.z += 5.0f * (f32)cos(mMoveAngle);

		if (mDish != NULL) {
			PlayGameSound(STAGE01_SOUND, 0x4001E, 0xFF);
			pos.x       = mDish->mActor->mMoveTranslate.x;
			pos.y       = mDish->mActor->mMoveTranslate.y;
			pos.z       = mDish->mActor->mMoveTranslate.z;
			mStackLevel = CountHamDishObjs(mDish);
			pos.y       = GetStackHeight();
			mActor->SetMoveDir(&pos, mMoveSpeed);
			SetState(NULL);
			STAGE01_PARTICLE->Kill(mYakiEffect);
			mYakiEffect = 0;
			UpdateHamPickupFlags();
			break;
		}

		if (mType == 1) {
			PlayYakiSound();
		}

		for (level = 4; level >= 0; level--) { // land on the tallest stack in reach
			HamObj* base = FindStackBase(pos, mId, level, 5.0f);
			if (base != NULL) {
				pos.y        = base->mActor->mMoveTranslate.y;
				base->mAbove = this;
				mStackLevel  = base->mStackLevel + 1;
				pos.y        = GetStackHeight();
				mActor->SetMoveDir(&pos, mMoveSpeed);
				SetState(&HamObj::StateStackLand);
				UpdateHamPickupFlags();
				return;
			}
		}

		UpdateHamPickupFlags();
		mStackLevel = 0;
		pos.y       = GetStackHeight();
		mActor->SetMoveDir(&pos, mMoveSpeed);
		mStateTime++;
		break;
	}
	case 1: // arrived: on the gas plate it slides on, off it the put-down ends
		if (PSVECDistance(&mActor->mMoveTranslate, &mActor->mMoveTranslate.mTarget) <= 0.001f) {
			UpdateHamPickupFlags();
			if (IsOnGasPlate(mActor)) {
				Vec pos = mActor->mMoveTranslate;
				pos.y   = PlacementPosition(mActor->mCoord).y;
				mActor->SetMoveDir(&pos, 10.0f);
				StartSlide(5.0f, 0.2f, mMoveAngle);
			} else {
				mStackLevel = 1;
				SetState(NULL);
			}
		}
		break;
	}
}

// StatePutDown

/**
 * @note Address: stage01 .text 0x2D14
 * @note Size: 0x30C
 */
void HamObj::StateSlide()
{
	Vec pos;

	switch (mStateTime) {
	case 0:
		pos.x = mActor->mMoveTranslate.x;
		pos.y = mActor->mMoveTranslate.y;
		pos.z = mActor->mMoveTranslate.z;
		pos.x += mSlideDistance * (f32)sin(mMoveAngle);
		pos.y = PlacementPosition(mActor->mCoord).y;
		pos.z += mSlideDistance * (f32)cos(mMoveAngle);
		mActor->SetMoveDir(&pos, mMoveSpeed);
		if (HamObj_CheckSlideHit(this)) {
			mActor->SetMoveDir(&mActor->mMoveTranslate, 0.0f);
			SetState(NULL);
		}
		if (mStateTime == 0) {
			mStateTime++;
		}
		break;
	case 1:
		if (HamObj_CheckSlideHit(this)) {
			mActor->SetMoveDir(&mActor->mMoveTranslate, 0.0f);
			SetState(NULL);
		} else if (IsSamePos(mActor->mMoveTranslate, mActor->mMoveTranslate.mTarget)) {
			SetState(NULL);
		}
		if (mAbove != NULL && 8.0f < PSVECDistance(&mActor->mMoveTranslate, &mAbove->mActor->mMoveTranslate)) {
			HamObj_LowerStack(mAbove); // it slid out from under its stack: drop what sits on it one level
			mAbove = NULL;
		}
		break;
	}
}

/**
 * @note Address: stage01 .text 0x3020
 * @note Size: 0x21C
 */
void HamObj::StateFlip()
{
	switch (mStateTime) {
	case 0: // flip to the opposite side: upside-down aims for 0, upright for PI
		if (0.0f < (f32)cos(HSD_JObjGetRotationZ(mActor->mJObj))) {
			mRotZ = PI;
		} else {
			mRotZ = 0.0f;
		}
		mStateTime++;
		break;
	case 1: // ease the rotation over the flip frames, then settle the new side
		mFlipFrames -= 1.0f;
		if (0.0f < mFlipFrames) {
			f32 rot = HSD_JObjGetRotationZ(mActor->mJObj);
			mActor->SetRotationZ(rot + (mRotZ - rot) / mFlipFrames, 0);
		} else {
			s32 level;
			PlayYakiSound();
			level = mYakiTime[mSide] / 600;
			if (level >= 5) {
				level = 4; // the doneness anim stops at 4
			}
			mActor->SetAnim(level, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mSide = 1 - mSide; // the other side now faces the plate
			SetState(NULL);
		}
		break;
	case 2:
		break;
	}
}

// StateFlip

/**
 * @note Address: stage01 .text 0x323C
 * @note Size: 0x52C
 */
extern "C" void HamObj_LowerStack(HamObj* slot)
{
	// drop a stacked object, and what sits on it, one level
	if (slot->mStackLevel > 0) {
		slot->mStackLevel--;
		Vec pos = *slot->GetPos();
		pos.y   = slot->GetStackHeight();
		slot->GetActor()->SetMoveDir(&pos, 0.1f);
		if (slot->mAbove != NULL) {
			HamObj_LowerStack(slot->mAbove);
		}
	}
}

/**
 * @note Address: stage01 .text 0x3938
 * @note Size: 0x40
 */
extern "C" void HamObj_SetRot(HamObj* slot, f32 rot)
{
	slot->SetRot(rot);
}

/**
 * @note Address: stage01 .text 0x3978
 * @note Size: 0x148
 */
extern "C" void HamObj_StartPutDown(HamObj* slot)
{
	s32 i;
	XObjChr* player = gGame.GetPlayer();

	if (IsHeld(slot->mActor)) {
		HamObj* other;
		putDownActive = 1; // while it runs, no pickup flags
		for (i = 0, other = hamObjs; i < ARRAY_SIZE(hamObjs); i++, other++) {
			if (IsActive(other->mActor)) {
				other->mActor->mStatus.mFlags &= ~0x100000000ULL;
				if (IsOnTopOf(other, slot)) {
					other->mAbove = NULL; // it was stacked under the held object
				}
			}
		}
		slot->SetState(&HamObj::StatePutDown);
	} else if (player->mLift.mState == 4) {
		player->mLift.mState = 6; // play the put-down animation
	}
}

/**
 * @note Address: stage01 .text 0x3AC0
 * @note Size: 0x270
 */
extern "C" void HamObj_Respawn(HamObj* slot)
{
	if (putDownActive) {
		slot->mActor->mStatus.mFlags &= ~0x100000000ULL; // a put-down is running: it can't be picked
	}

	if (slot->mType == 1) { // a hamburger respawns raw and sizzles; anything else just lands
		slot->mSide        = 0;
		slot->mYakiTime[1] = 0;
		slot->mYakiTime[0] = 0;
		PlayYakiSound();
	} else {
		PlayGameSound(STAGE01_SOUND, 0x4001E, 0xFF);
	}

	// put it back at the spawn point upright, at anim 0 (raw)
	slot->mActor->SetPos(-231.8f, 77.5f, 50.0f, 0);
	slot->mActor->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	slot->SetRot(0.0f);

	f32 distance;
	// toss it back in a random 240-300-degree direction
	f32 angle = DEG2RAD * (HSD_Randi(60) + 240);
	distance  = 30.0f + HSD_Randi(15);
	slot->StartSlide(distance, 0.5f, angle);
	slot->mSpawned    = 1;
	slot->mStackLevel = 0;
	slot->mAbove      = NULL;
}

/**
 * @note Address: stage01 .text 0x3D30
 * @note Size: 0x98
 */
extern "C" void HamObj_StartSlide(HamObj* slot, f32 distance, f32 speed, f32 angle)
{
	slot->StartSlide(distance, speed, angle);
}

/**
 * @note Address: stage01 .text 0x3DC8
 * @note Size: 0xA8
 */
extern "C" void HamObj_StartFlip(HamObj* slot, u32 frames)
{
	slot->mFlipFrames = frames;
	slot->mMoveAngle  = AngleFrom(&gGame.GetPlayer()->mMoveTranslate, &slot->mActor->mMoveTranslate);
	slot->SetState(&HamObj::StateFlip);
}

// HamObj_StartFlip

/**
 * @note Address: stage01 .text 0x3E70
 * @note Size: 0x86C
 */
extern "C" void HamObj_Update(HamObj* slot, u32 step)
{
	if (!IsActive(slot->mActor)) {
		slot->mSpawned = 0;
	}

	// only an unstacked spawned hamburger that isn't carried cooks
	if (slot->mType == 1 && slot->mSpawned && slot->mStackLevel == 0 && !IsHeld(slot->mActor)) {
		s32 oldLevel = slot->mYakiTime[slot->mSide] / 600 + 1;
		f32 rate;
		if (oldLevel < 1 || oldLevel >= ARRAY_SIZE(yakiRates)) {
			rate = 1.0f;
		} else {
			rate = yakiRates[oldLevel];
		}
		slot->mYakiTime[slot->mSide] += step * rate; // cook at the script's rate for the current doneness

		s32 level = slot->mYakiTime[slot->mSide] / 600 + 1;
		if (oldLevel > 5) { // doneness tops out at 5 (burnt)
			oldLevel = 5;
		}
		if (level > 5) {
			level = 5;
		}

		Vec scale;
		Vec pos;
		pos.x = slot->mActor->mMoveTranslate.x;
		pos.y = slot->mActor->mMoveTranslate.y;
		pos.z = slot->mActor->mMoveTranslate.z;
		pos.y += 2.5f;
		scale.x = scale.y = scale.z = 1.0f;

		if (level < 4 && oldLevel < level) { // crossing into a new doneness puffs once
			static const Vec rot = { 0.0f, 0.0f, 0.0f };
			STAGE01_PARTICLE->CreateSRT(0x3A, 1, pos, scale, rot, 1);
		}
		if (level == 4) { // well done: re-puff every 180 units of cook time
			if (slot->mYakiTime[slot->mSide] % 180 == 0) {
				static const Vec rot = { 0.0f, 0.0f, 0.0f };
				STAGE01_PARTICLE->Kill(0x3B);
				STAGE01_PARTICLE->CreateSRT(0x3B, 1, pos, scale, rot, 1);
			}
		} else if (level >= 5) { // burnt: every 120 units of cook time
			if (slot->mYakiTime[slot->mSide] % 120 == 0) {
				static const Vec rot = { 0.0f, 0.0f, 0.0f };
				STAGE01_PARTICLE->Kill(0x3B);
				STAGE01_PARTICLE->CreateSRT(0x3B, 1, pos, scale, rot, 1);
			}
		}

		if (yakiLoopLevel < 1) {
			yakiLoopLevel = 1;
		}
		if (level >= 4) {
			yakiLoopLevel = 2; // a side at doneness 4+ switches the cooking loop
		}

		if (slot->mYakiEffect == 0) { // start the cooking particles on joint 0 once
			s32 path[]        = { 0 };
			HSD_JObj* jobj    = GetJointByPath(slot->mActor->mJObj, path, 1);
			slot->mYakiEffect = STAGE01_PARTICLE->Create(0x78, 0, jobj, 1);
		}
	}

	if (slot->mState != NULL) {
		(slot->*(slot->mState))();
	} else if (slot->mSpawned && slot->mDish == NULL) {
		// slides go toward the plate centre (-263.9, 42.0)
		f32 angle = atan2(-263.9f - slot->mActor->mMoveTranslate.x, 42.0f - slot->mActor->mMoveTranslate.z);
		if (!IsOnGasPlate(slot->mActor)) { // off the plate: slide back onto it
			slot->StartSlide(5.0f, 0.2f, angle);
			slot->mStackLevel = 0;
		} else if (slot->mStackLevel == 0 && playerOffGasPlate) {
			// a player standing off the plate nudges it when closer than 8
			if (PSVECDistance(&gGame.GetPlayer()->mMoveTranslate, &slot->mActor->mMoveTranslate) < 8.0f) {
				slot->StartSlide(5.0f, 0.2f, angle);
			}
		} else if (PlacementPosition(slot->mActor->mCoord).y + 2.0f * slot->mStackLevel < slot->mActor->mMoveTranslate.y) {
			slot->StartSlide(0.0f, 0.5f, angle); // still in the air above its stack height: settle down
		}

		// restore the pickup flag once it is uncovered again
		if (!putDownActive && !(slot->mActor->mStatus.mFlags & 0x100000000ULL)
		    && (slot->mAbove == NULL || (slot->mAbove != NULL && slot->mStackLevel == slot->mAbove->mStackLevel))) {
			slot->mActor->mStatus.mFlags |= 0x100000000ULL;
		}
	}

	if (slot->mSpawned && slot->mActor->mMoveTranslate.y < 77.5f) { // keep spawned objects on the plate
		slot->mActor->SetTranslateY(78.0f, 0);
		slot->mActor->SetRotationZ(slot->mRotZ, 0);
	}
}

// HamObj_Update

/**
 * @note Address: stage01 .text 0x46DC
 * @note Size: 0x4C
 */
// fn_4_46DC

/**
 * @note Address: stage01 .text 0x4728
 * @note Size: 0x48
 */
// fn_4_4728

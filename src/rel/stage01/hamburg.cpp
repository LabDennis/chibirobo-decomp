#include "Version.h"
#include "game/hsd_jobj.h"
#include "game/ut.h"
#include "game/cparticle.h"
#include "game/cgame.h"
#include "game/gmath.h"

extern "C" {
double atan2(double y, double x);
double cos(double x);
double sin(double x);
}

struct Stage01Actor;

struct Stage01ActorKind {
	s32 mObjectId; // _00, UnkStageObj::mObjectId, the placement's object id (XObj::mCoord._00); 0x1F9, 0x200, 0x263 pick the ham object's
	               // behaviour
	s32 mPosX;     // _04, UnkStageObj::mPos[0], placement x in hundredths
	s32 mPosY;     // _08, UnkStageObj::mPos[1], placement y in hundredths
	s32 mPosZ;     // _0C, UnkStageObj::mPos[2], placement z in hundredths
};

struct Stage01KindRef {
	Vec GetSize() const;

	Stage01ActorKind* mKind; // _00
};

// non-virtual base, so the vtable pointer ends up at _34
struct Stage01ActorBase {
	HSD_JObj* mJObj;    // _00, CJObj::mJObj, the object's root joint
	u8 _04[0x34 - 0x4]; // _04
};

struct Stage01ActorVirt : public Stage01ActorBase {
	virtual void vf_08();                                // _08
	virtual void vf_0C();                                // _0C
	virtual void SetTranslateY(f32 y, s32 force);        // _10
	virtual void vf_14();                                // _14
	virtual void vf_18();                                // _18
	virtual void vf_1C();                                // _1C
	virtual void vf_20();                                // _20
	virtual void vf_24();                                // _24
	virtual void vf_28();                                // _28
	virtual void SetRotationZ(f32 z, s32 force);         // _2C
	virtual void vf_30();                                // _30
	virtual void vf_34();                                // _34
	virtual void vf_38();                                // _38
	virtual void vf_3C();                                // _3C
	virtual void vf_40();                                // _40
	virtual void vf_44();                                // _44
	virtual void vf_48();                                // _48
	virtual void vf_4C();                                // _4C
	virtual void vf_50();                                // _50
	virtual void vf_54();                                // _54
	virtual void vf_58();                                // _58
	virtual void vf_5C();                                // _5C
	virtual void vf_60();                                // _60
	virtual void vf_64();                                // _64
	virtual void SetPos(f32 x, f32 y, f32 z, s32 force); // _68
	virtual void vf_6C();                                // _6C
	virtual void vf_70();                                // _70
	virtual void vf_74();                                // _74
	virtual void SetMoveDir(const Vec* dir, f32 speed);  // _78
	virtual void vf_7C();                                // _7C
	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate); // _80
	                                     // _34 vtable
};

struct Stage01Actor : public Stage01ActorVirt {
	Stage01KindRef& GetKindRef();
	Vec* GetPos();

#if defined(VERSION_JP)
	u8 _38[0xC0 - 0x38]; // _38
#else
	u8 _38[0xC8 - 0x38]; // _38
#endif
	Vec mMoveTranslate;     // _C8 (JP: _C0)
	u8 _D4[0x4];            // _D4 (JP: _CC)
	Vec mMoveTarget;        // _D8 (JP: _D0)
	u8 _E4[0xFC - 0xE4];    // _E4 (JP: _DC)
	f32 mMoveRotateY;       // _FC (JP: _F4)
	u8 _100[0x158 - 0x100]; // _100 (JP: _F8)
	s32 mAnimId;            // _158 (JP: _150)
	u8 _15C[0x228 - 0x15C]; // _15C (JP: _154)
	u64 mStatus;            // _228 (JP: _220)
	u8 _230[0x238 - 0x230]; // _230 (JP: _228)
	Stage01KindRef mCoord;  // _238 (JP: _230)
	u8 _23C[0x258 - 0x23C]; // _23C (JP: _234)
	s32 mPoolIndex;         // _258 (JP: _250)
#if defined(VERSION_JP)
	u8 _254[0xA34 - 0x254]; // _254
#else
	u8 _25C[0xA44 - 0x25C]; // _25C
#endif
	u8 _A44[0x4];              // _A44 (JP: _A34)
	s32 mLiftState;            // _A48 (JP: _A38)
	u8 _A4C[0xA64 - 0xA4C];    // _A4C (JP: _A3C)
	Stage01Actor* mLiftTarget; // _A64 (JP: _A54)
};

struct HamDish;

struct HamObj {
	typedef void (HamObj::*StateFunc)();

	void StateStackLand();
	void StatePutDown();
	void StateSlide();
	void StateFlip();

	Vec* GetPos();
	Stage01Actor* GetActor();
	f32 GetStackHeight();

	void SetState(StateFunc func)
	{
		mState     = func;
		mStateTime = 0;
	}

	void StartSlide(f32 speed, f32 f, f32 angle)
	{
		if (mState != &HamObj::StateFlip) {
			mMoveAngle     = angle;
			mSlideDistance = speed;
			mMoveSpeed     = f;
			SetState(&HamObj::StateSlide);
		}
	}

	void SetRot(f32 rot)
	{
		mRotZ = rot;
		mActor->SetRotationZ(mRotZ, 0);
	}

	HamObj* mAbove;       // _00, the HamObj stacked on this one, NULL for none
	Stage01Actor* mActor; // _04
	s32 mId;              // _08, script object id, the actor is STAGE01_ACTORS[id]; -1 when the slot is free
	s32 mType;            // _0C, 1 hamburger (kind 0x1F9), 2 kind 0x263, 3 kind 0x264, 0 other; only 1 cooks and plays the yaki sound
	f32 mRotZ;            // _10, z rotation, PI upside down; SetRot sets it and StateFlip turns the actor toward it
	f32 mFlipFrames;      // _14, frames left in the flip, set from the flip command
	f32 mMoveAngle;       // _18, yaw on the XZ plane the object slides or is put down along
	f32 mSlideDistance;   // _1C, how far a slide goes, added along mMoveAngle to the move target (StartSlide's 'speed' parameter)
	f32 mMoveSpeed;       // _20, speed handed to SetMoveDir (vf_78) for slides and put-downs
	s32 mSide;            // _24, which side is cooking, 0 or 1, indexes mYakiTime; StateFlip toggles it
	s32 mYakiTime[2];     // _28, cook time per side, /600 + 1 is the doneness level; 9999 for kinds 0x263/0x264
	s32 mStackLevel;      // _30, how many objects it sits on (on a dish: the count already there); GetStackHeight adds 2 per level
	HamDish* mDish;       // _34, the HamDish it was put on, NULL for none
	u8 mSpawned; // _38, set by HamObj_Respawn, cleared once the actor is no longer drawn; while set it cooks, is slid back onto the gas
	             // plate and kept above y 77.5
	HSD_Generator* mYakiEffect; // _3C, particle 0x78 on joint path {0}, created while cooking, killed when put on a dish or at exit
	StateFunc mState;           // _40
	s32 mStateTime;             // _4C
};

struct HamDish {
	Stage01Actor* mActor; // _00, the dish's actor, STAGE01_ACTORS[id]; NULL when the slot is free
	s32 mId;              // _04, script object id of the dish, -1 when free
	HamObj* mObjs[4];     // _08, the HamObjs put on the dish, filled in order, NULL past the last
};

static inline void AddToHamDish(HamDish* group, HamObj* slot)
{
	s32 i;
	for (i = 0; i < 4; i++) {
		if (group->mObjs[i] == NULL) {
			group->mObjs[i] = slot;
			break;
		}
	}
}

#define STAGE01_PLAYER   ((Stage01Actor*)gGame.GetPlayer())
#define STAGE01_ACTORS   ((Stage01Actor**)gGame.mXObjs.mObjects)
#define STAGE01_EVT_DATA (&gGame.mEvt.mData)
#define STAGE01_PARTICLE (&gGame.mParticle)
#define STAGE01_OBJ_8F38 (&gGame.mInfoScrn)
#define STAGE01_SOUND    (&gGame.mSound)

extern "C" {
void AllowIdleHudCounters(void* obj);
void SuppressIdleHudCounters(void* obj);
s32 IsLiftTargetPlug(void* obj);
void psKillAllParticle(void);

// ana.cpp
void DrawAnas(void);
void UpdateAnas(u32 arg);
void InitAnas(void);
s32 CmdAttachAnaToRobo(s32 cmd, s32 count, s32* args);
s32 CmdSetAnaRotY(s32 cmd, s32 count, s32* args);
s32 CmdSetAnaJObj(s32 cmd, s32 count, s32* args);
s32 CmdPlantTane(s32 cmd, s32 count, s32* args);
s32 CmdFindNearestAna(s32 cmd, s32 count, s32* args);
s32 CmdSetTutiAna(s32 cmd, s32 count, s32* args);
s32 CmdAddAna(s32 cmd, s32 count, s32* args);

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
void DrawStage(u32 arg);
void DrawStageHud(u32 arg);
void UpdateStage(u32 arg);
void InitStage(u32 arg);
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
HamObj* FindHamObjByActor(Stage01Actor* actor);
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
void HamObj_StartSlide(HamObj* slot, f32 a, f32 b, f32 angle);
void HamObj_StartFlip(HamObj* slot, u32 time);
void HamObj_Update(HamObj* slot, u32 step);
void HamObj_LowerStack(HamObj* slot);
s32 HSD_Randi(s32 max);
}

typedef void (*RelSlotFunc)(u32 arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

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

// zero vec, only used to seed GetSize
static Vec sHamPlacementPosition;
static f32 yakiRates[6];
static u32 yakiLoopLevel;
static u8 playerOffGasPlate;
static u8 yakiSoundPlayed;
static HamObj hamObjs[9];
static Stage01Actor* gasPlate;
static HamDish hamDishes[4];
static Stage01Actor* tempActors[4];
static CStatus yakiResultFlags;
static u8 putDownActive;

static inline bool IsActive(Stage01Actor* actor)
{
	if (actor == NULL) {
		return false;
	}
	if (!(actor->mStatus & 0x10000)) {
		return false;
	}
	return true;
}

static inline bool IsHeld(Stage01Actor* actor)
{
	Stage01Actor* player = STAGE01_PLAYER;
	bool held            = false;
	if (player->mLiftState != 0 && player->mLiftTarget == actor) {
		held = true;
	}
	return held;
}

static inline bool IsOnGasPlate(Stage01Actor* actor)
{
	f32 x = actor->mMoveTranslate.x;
	f32 z = actor->mMoveTranslate.z;
	if (x < -289.0f || -239.0f < x || z < 21.4f || 62.6f < z) {
		return false;
	}
	return true;
}

// todo: copied inline, clean up
inline f32 HSD_AObjGetCurrFrame(HSD_AObj* aobj)
{
	((aobj) ? ((void)0) : __assert("aobj.h", 149, "aobj"));
	return aobj->curr_frame;
}

static inline bool IsOnTopOf(HamObj* slot, HamObj* base)
{
	return slot->mAbove != NULL ? base->mId == slot->mAbove->mId : false;
}

static inline bool IsSamePos(const Vec& a, Vec b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

static inline HamObj* FindHamObjByActorInline(Stage01Actor* actor)
{
	s32 i;
	if (actor == NULL) {
		return NULL;
	}
	for (i = 0; i < 9; i++) {
		Stage01Actor* current = hamObjs[i].mActor;
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

inline Vec* HamObj::GetPos()
{
	return mActor->GetPos();
}

inline Vec Stage01KindRef::GetSize() const
{
	Vec size = sHamPlacementPosition;
	size.x   = mKind->mPosX / 100.0f;
	size.y   = mKind->mPosY / 100.0f;
	size.z   = mKind->mPosZ / 100.0f;
	return size;
}

inline Stage01KindRef& Stage01Actor::GetKindRef()
{
	return mCoord;
}

inline Stage01Actor* HamObj::GetActor()
{
	return mActor;
}

inline Vec* Stage01Actor::GetPos()
{
	return &mMoveTranslate;
}

inline f32 HamObj::GetStackHeight()
{
	return GetActor()->GetKindRef().GetSize().y + 2.0f * mStackLevel;
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
	Stage01Actor* actor = STAGE01_ACTORS[args[1]];
	Vec pos             = actor->mMoveTranslate;
	s32 id              = args[0];
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
	SetYakiRates((u8*)args[0]);
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
	HamObj* slot = FindHamObjByActor(STAGE01_PLAYER->mLiftTarget);
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
	for (i = 1; i < count; i++) {
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

	f32 angle = atan2(slot->mActor->mMoveTranslate.x - STAGE01_PLAYER->mMoveTranslate.x,
	                  slot->mActor->mMoveTranslate.z - STAGE01_PLAYER->mMoveTranslate.z);
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
		return slot->mActor->mCoord.mKind->mObjectId;
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
extern "C" void DrawStage(u32 arg)
{
	DrawAnas();
}

/**
 * @note Address: stage01 .text 0x630
 * @note Size: 0x4
 */
extern "C" void DrawStageHud(u32 arg)
{
}

/**
 * @note Address: stage01 .text 0x634
 * @note Size: 0x218
 */
extern "C" void UpdateStage(u32 arg)
{
	Vec pos;

	UpdateHamObjs(arg);
	UpdateAnas(arg);

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
extern "C" void InitStage(u32 arg)
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
	Stage01Actor* player;
	HamObj* slot;

	if (gGame.mEvt.mData.mScript != NULL) {
		return 0;
	}

	player = STAGE01_PLAYER;
	if (player->mMoveTranslate.x < -295.0f || -225.0 < player->mMoveTranslate.x || player->mMoveTranslate.z < 10.0f
	    || 74.0f < player->mMoveTranslate.z) {
		return 9;
	}

	result = 0;
	if (result == 0 && !(yakiResultFlags.mFlags & 2)) {
		for (i = 0; i < 9 && result == 0; i++) {
			slot = &hamObjs[i];
			if (IsActive(slot->mActor) && slot->mActor->mCoord.mKind->mObjectId == 0x1F9 && slot->mYakiTime[0] / 600 + 1 >= 3
			    && slot->mYakiTime[1] / 600 + 1 >= 3) {
				result = 1;
			}
		}
		if (result != 0) {
			yakiResultFlags.mFlags |= 2;
		}
	}

	if (result == 0 && !(yakiResultFlags.mFlags & 4)) {
		for (i = 0; i < 9 && result == 0; i++) {
			slot = &hamObjs[i];
			if (IsActive(slot->mActor) && slot->mActor->mCoord.mKind->mObjectId == 0x1F9
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
/*
    lis      r3, gGame@ha
    addi     r4, r3, gGame@l
    addis    r3, r4, 0x6
    lwz      r0, -0x2f80(r3)
    cmplwi   r0, 0x0
    beq      .L_00000890
    li       r3, 0x0
    blr
.L_00000890:
    addis    r4, r4, 0x1
    lis      r3, sHamResultMinX@ha
    lwz      r4, -0x6944(r4)
    lfs      f0, sHamResultMinX@l(r3)
    lfs      f1, 0xc8(r4)
    fcmpo    cr0, f1, f0
    blt      .L_000008E0
    lis      r3, sHamResultMaxX@ha
    lfd      f0, sHamResultMaxX@l(r3)
    fcmpo    cr0, f0, f1
    blt      .L_000008E0
    lis      r3, sHamResultMinZ@ha
    lfs      f1, 0xd0(r4)
    lfs      f0, sHamResultMinZ@l(r3)
    fcmpo    cr0, f1, f0
    blt      .L_000008E0
    lis      r3, sHamResultMaxZ@ha
    lfs      f0, sHamResultMaxZ@l(r3)
    fcmpo    cr0, f0, f1
    bge      .L_000008E8
.L_000008E0:
    li       r3, 0x9
    blr
.L_000008E8:
    li       r3, 0x0
    cmpwi    r3, 0x0
    bne      .L_000009EC
    lis      r4, yakiResultFlags@ha
    lwz      r0, yakiResultFlags@l(r4)
    rlwinm.  r0, r0, 0, 30, 30
    bne      .L_000009EC
    lis      r4, hamObjs@ha
    li       r5, 0x0
    addi     r4, r4, hamObjs@l
    b        .L_000009C4
.L_00000914:
    lwz      r9, 0x4(r4)
    cmplwi   r9, 0x0
    bne      .L_00000928
    li       r8, 0x0
    b        .L_00000958
.L_00000928:
    lwz      r0, 0x228(r9)
    li       r8, 0x0
    lwz      r7, 0x22c(r9)
    lis      r6, 0x1
    and      r0, r0, r8
    and      r6, r7, r6
    xor      r6, r6, r8
    xor      r0, r0, r8
    or.      r0, r6, r0
    bne      .L_00000954
    b        .L_00000958
.L_00000954:
    li       r8, 0x1
.L_00000958:
    clrlwi.  r0, r8, 24
    beq      .L_000009BC
    lwz      r6, 0x238(r9)
    lwz      r0, 0x0(r6)
    cmpwi    r0, 0x1f9
    bne      .L_000009BC
    lis      r6, 0x1b4f
    lwz      r0, 0x28(r4)
    subi     r7, r6, 0x7e4b
    mulhw    r0, r7, r0
    srawi    r0, r0, 6
    srwi     r6, r0, 31
    add      r6, r0, r6
    addi     r0, r6, 0x1
    cmpwi    r0, 0x3
    blt      .L_000009BC
    lwz      r0, 0x2c(r4)
    mulhw    r0, r7, r0
    srawi    r0, r0, 6
    srwi     r6, r0, 31
    add      r6, r0, r6
    addi     r0, r6, 0x1
    cmpwi    r0, 0x3
    blt      .L_000009BC
    li       r3, 0x1
.L_000009BC:
    addi     r4, r4, 0x50
    addi     r5, r5, 0x1
.L_000009C4:
    cmplwi   r5, 0x9
    bge      .L_000009D4
    cmpwi    r3, 0x0
    beq      .L_00000914
.L_000009D4:
    cmpwi    r3, 0x0
    beq      .L_000009EC
    lis      r4, yakiResultFlags@ha
    lwzu     r0, yakiResultFlags@l(r4)
    ori      r0, r0, 0x2
    stw      r0, 0x0(r4)
.L_000009EC:
    cmpwi    r3, 0x0
    bnelr
    lis      r4, yakiResultFlags@ha
    lwz      r0, yakiResultFlags@l(r4)
    rlwinm.  r0, r0, 0, 29, 29
    bnelr
    lis      r4, hamObjs@ha
    li       r5, 0x0
    addi     r4, r4, hamObjs@l
    b        .L_00000AC4
.L_00000A14:
    lwz      r9, 0x4(r4)
    cmplwi   r9, 0x0
    bne      .L_00000A28
    li       r8, 0x0
    b        .L_00000A58
.L_00000A28:
    lwz      r0, 0x228(r9)
    li       r8, 0x0
    lwz      r7, 0x22c(r9)
    lis      r6, 0x1
    and      r0, r0, r8
    and      r6, r7, r6
    xor      r6, r6, r8
    xor      r0, r0, r8
    or.      r0, r6, r0
    bne      .L_00000A54
    b        .L_00000A58
.L_00000A54:
    li       r8, 0x1
.L_00000A58:
    clrlwi.  r0, r8, 24
    beq      .L_00000ABC
    lwz      r6, 0x238(r9)
    lwz      r0, 0x0(r6)
    cmpwi    r0, 0x1f9
    bne      .L_00000ABC
    lis      r6, 0x1b4f
    lwz      r0, 0x28(r4)
    subi     r7, r6, 0x7e4b
    mulhw    r0, r7, r0
    srawi    r0, r0, 6
    srwi     r6, r0, 31
    add      r6, r0, r6
    addi     r0, r6, 0x1
    cmpwi    r0, 0x5
    bge      .L_00000AB8
    lwz      r0, 0x2c(r4)
    mulhw    r0, r7, r0
    srawi    r0, r0, 6
    srwi     r6, r0, 31
    add      r6, r0, r6
    addi     r0, r6, 0x1
    cmpwi    r0, 0x5
    blt      .L_00000ABC
.L_00000AB8:
    li       r3, 0x2
.L_00000ABC:
    addi     r4, r4, 0x50
    addi     r5, r5, 0x1
.L_00000AC4:
    cmplwi   r5, 0x9
    bge      .L_00000AD4
    cmpwi    r3, 0x0
    beq      .L_00000A14
.L_00000AD4:
    cmpwi    r3, 0x0
    beqlr
    lis      r4, yakiResultFlags@ha
    lwzu     r0, yakiResultFlags@l(r4)
    ori      r0, r0, 0x4
    stw      r0, 0x0(r4)
    blr
*/

/**
 * @note Address: stage01 .text 0xAF0
 * @note Size: 0xA4
 */
extern "C" void AddTempActor(s32 id)
{
	s32 i;
	for (i = 0; i < 4; i++) {
		if (tempActors[i] == NULL) {
			Stage01Actor* actor;
			if (gGame.mEvt.mData.mScript != NULL) {
				actor = (Stage01Actor*)ResolveEventObjectId(STAGE01_EVT_DATA, id);
			} else {
				actor = NULL;
			}
			tempActors[i] = actor;
			tempActors[i]->mStatus |= 0x10000;
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
	for (i = 0; i < 4; i++) {
		if (tempActors[i] != NULL) {
			tempActors[i]->mStatus &= ~0x10000ULL;
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
	for (i = 0; i < 4; i++) {
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

	for (i = 0; i < 4; i++) {
		if (hamDishes[i].mActor == NULL) {
			group           = &hamDishes[i];
			group->mId      = id;
			group->mActor   = STAGE01_ACTORS[id];
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
	for (i = 0; i < 4; i++) {
		if (group->mObjs[i] == NULL) {
			return i;
		}
	}
	return 4;
}

/**
 * @note Address: stage01 .text 0xDD8
 * @note Size: 0x180
 */
extern "C" void SetYakiRates(u8* data)
{
	s32 i;
	for (i = 0; i < 5; i++) {
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
	Stage01Actor* player = STAGE01_PLAYER;
	HamObj* slot;
	HamDish* group;

	if (IsLiftTargetPlug(player->_A44)) {
		return -1;
	}

	slot = FindHamObjByActorInline(player->mLiftTarget);
	if (slot == NULL) {
		return -1;
	}

	group       = FindHamDish(id);
	slot->mDish = group;
	AddToHamDish(group, slot);

	if (player->mLiftState == 4) {
		player->mLiftState = 6;
	}
	return slot->mId;
}

// PutHeldHamObjOnDish
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lis      r4, gGame@ha
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    stw      r30, 0x8(r1)
    mr       r30, r3
    addi     r3, r4, gGame@l
    addis    r3, r3, 0x1
    lwz      r31, -0x6944(r3)
    addi     r3, r31, 0xa44
    bl       IsLiftTargetPlug
    cmpwi    r3, 0x0
    beq      .L_00000F98
    li       r3, -0x1
    b        .L_000010B4
.L_00000F98:
    lwz      r4, 0xa64(r31)
    cmplwi   r4, 0x0
    bne      .L_00000FAC
    li       r5, 0x0
    b        .L_00001000
.L_00000FAC:
    lis      r3, hamObjs@ha
    li       r0, 0x9
    addi     r6, r3, hamObjs@l
    li       r5, 0x0
    mtctr    r0
.L_00000FC0:
    lwz      r3, 0x4(r6)
    cmplwi   r3, 0x0
    beq      .L_00000FF0
    lwz      r3, 0x258(r3)
    lwz      r0, 0x258(r4)
    cmpw     r3, r0
    bne      .L_00000FF0
    mulli    r4, r5, 0x50
    lis      r3, hamObjs@ha
    addi     r0, r3, hamObjs@l
    add      r5, r0, r4
    b        .L_00001000
.L_00000FF0:
    addi     r6, r6, 0x50
    addi     r5, r5, 0x1
    bdnz     .L_00000FC0
    li       r5, 0x0
.L_00001000:
    cmplwi   r5, 0x0
    bne      .L_00001010
    li       r3, -0x1
    b        .L_000010B4
.L_00001010:
    lis      r3, hamDishes@ha
    li       r0, 0x4
    addi     r4, r3, hamDishes@l
    li       r3, 0x0
    mtctr    r0
.L_00001024:
    lwz      r0, 0x0(r4)
    cmplwi   r0, 0x0
    beq      .L_00001050
    lwz      r0, 0x4(r4)
    cmpw     r30, r0
    bne      .L_00001050
    mulli    r4, r3, 0x18
    lis      r3, hamDishes@ha
    addi     r0, r3, hamDishes@l
    add      r3, r0, r4
    b        .L_00001060
.L_00001050:
    addi     r4, r4, 0x18
    addi     r3, r3, 0x1
    bdnz     .L_00001024
    li       r3, 0x0
.L_00001060:
    li       r0, 0x4
    mr       r6, r3
    stw      r3, 0x34(r5)
    li       r4, 0x0
    mtctr    r0
.L_00001074:
    lwz      r0, 0x8(r6)
    cmplwi   r0, 0x0
    bne      .L_00001090
    slwi     r0, r4, 2
    add      r3, r3, r0
    stw      r5, 0x8(r3)
    b        .L_0000109C
.L_00001090:
    addi     r6, r6, 0x4
    addi     r4, r4, 0x1
    bdnz     .L_00001074
.L_0000109C:
    lwz      r0, 0xa48(r31)
    cmpwi    r0, 0x4
    bne      .L_000010B0
    li       r0, 0x6
    stw      r0, 0xa48(r31)
.L_000010B0:
    lwz      r3, 0x8(r5)
.L_000010B4:
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    lwz      r30, 0x8(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

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
		f32 c     = cos(angle - slot->mMoveAngle);
		if ((f32)cos(45.0) < c) {
			f32 speed;
			angle = AngleFrom(&slot->mActor->mMoveTranslate, &best->mActor->mMoveTranslate);
			speed = slot->mSlideDistance * 0.5f;
			if (10.0f < speed) {
				speed = 10.0f;
			}
			best->StartSlide(speed, slot->mMoveSpeed, angle);
			if (IsOnGasPlate(slot->mActor)) {
				return true;
			}
		}
	}

	if (IsLeavingGasPlate(slot)) {
		return true;
	}
	return false;
}

// HamObj_CheckSlideHit
/*
    stwu     r1, -0x70(r1)
    mflr     r0
    stw      r0, 0x74(r1)
    stfd     f31, 0x60(r1)
    psq_st   f31, 0x68(r1), 0, qr0
    stfd     f30, 0x50(r1)
    psq_st   f30, 0x58(r1), 0, qr0
    stfd     f29, 0x40(r1)
    psq_st   f29, 0x48(r1), 0, qr0
    stmw     r25, 0x24(r1)
    mr       r25, r3
    lis      r3, sHamResultMinZ@ha
    lwz      r7, 0x4(r25)
    addi     r5, r3, sHamResultMinZ@l
    lis      r3, gGame@ha
    lis      r4, hamObjs@ha
    lwz      r6, 0xc8(r7)
    addi     r3, r3, gGame@l
    lwz      r0, 0xcc(r7)
    li       r28, 0x0
    lfs      f31, 0x0(r5)
    addi     r27, r4, hamObjs@l
    stw      r6, 0x8(r1)
    mr       r29, r28
    addis    r26, r3, 0x1
    stw      r0, 0xc(r1)
    lwz      r0, 0xd0(r7)
    stw      r0, 0x10(r1)
    lwz      r31, 0x30(r25)
    lwz      r30, 0x8(r25)
.L_00001144:
    lwz      r6, 0x4(r27)
    cmplwi   r6, 0x0
    bne      .L_00001158
    li       r5, 0x0
    b        .L_00001188
.L_00001158:
    lwz      r0, 0x228(r6)
    li       r5, 0x0
    lwz      r4, 0x22c(r6)
    lis      r3, 0x1
    and      r0, r0, r5
    and      r3, r4, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    bne      .L_00001184
    b        .L_00001188
.L_00001184:
    li       r5, 0x1
.L_00001188:
    clrlwi.  r0, r5, 24
    beq      .L_00001208
    lwz      r0, 0x8(r27)
    cmpw     r30, r0
    beq      .L_00001208
    lwz      r0, 0x30(r27)
    cmpw     r31, r0
    bne      .L_00001208
    lwz      r0, 0x34(r27)
    cmplwi   r0, 0x0
    bne      .L_00001208
    lwz      r4, -0x6944(r26)
    li       r3, 0x0
    lwz      r0, 0xa48(r4)
    cmpwi    r0, 0x0
    beq      .L_000011D8
    lwz      r0, 0xa64(r4)
    cmplw    r0, r6
    bne      .L_000011D8
    li       r3, 0x1
.L_000011D8:
    clrlwi.  r0, r3, 24
    bne      .L_00001208
    lfs      f0, 0xcc(r6)
    addi     r4, r1, 0x8
    lwz      r3, 0x4(r27)
    stfs     f0, 0xc(r1)
    addi     r3, r3, 0xc8
    bl       PSVECDistance
    fcmpo    cr0, f1, f31
    bge      .L_00001208
    mr       r28, r27
    fmr      f31, f1
.L_00001208:
    addi     r29, r29, 0x1
    addi     r27, r27, 0x50
    cmplwi   r29, 0x9
    blt      .L_00001144
    cmplwi   r28, 0x0
    beq      .L_00001374
    lwz      r4, 0x4(r28)
    lwz      r3, 0x4(r25)
    lfs      f3, 0xc8(r4)
    lfs      f1, 0xc8(r3)
    lfs      f2, 0xd0(r4)
    lfs      f0, 0xd0(r3)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f1, f1
    lfs      f0, 0x18(r25)
    fsubs    f1, f1, f0
    bl       cos
    lis      r3, sHamSlideHitAngle@ha
    frsp     f30, f1
    lfd      f1, sHamSlideHitAngle@l(r3)
    bl       cos
    frsp     f0, f1
    fcmpo    cr0, f0, f30
    bge      .L_00001374
    lwz      r4, 0x4(r28)
    lwz      r3, 0x4(r25)
    lfs      f3, 0xc8(r4)
    lfs      f1, 0xc8(r3)
    lfs      f2, 0xd0(r4)
    lfs      f0, 0xd0(r3)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    lis      r4, sHamSlideHalf@ha
    lis      r3, sHamResultMinZ@ha
    lfs      f3, 0x1c(r25)
    frsp     f30, f1
    lfs      f2, sHamSlideHalf@l(r4)
    lfs      f0, sHamResultMinZ@l(r3)
    fmuls    f29, f3, f2
    fcmpo    cr0, f0, f29
    bge      .L_000012BC
    fmr      f29, f0
.L_000012BC:
    lis      r3, sHamFlipComparison@ha
    lfs      f31, 0x20(r25)
    addi     r4, r3, sHamFlipComparison@l
    addi     r3, r28, 0x40
    bl       __ptmf_cmpr
    cmpwi    r3, 0x0
    beq      .L_0000130C
    stfs     f30, 0x18(r28)
    lis      r3, sHamSlideState@ha
    addi     r5, r3, sHamSlideState@l
    li       r0, 0x0
    stfs     f29, 0x1c(r28)
    stfs     f31, 0x20(r28)
    lwz      r4, 0x0(r5)
    lwz      r3, 0x4(r5)
    stw      r4, 0x40(r28)
    stw      r3, 0x44(r28)
    lwz      r3, 0x8(r5)
    stw      r3, 0x48(r28)
    stw      r0, 0x4c(r28)
.L_0000130C:
    lwz      r4, 0x4(r25)
    lis      r3, sHamGasPlateMinX@ha
    lfs      f0, sHamGasPlateMinX@l(r3)
    lfs      f2, 0xc8(r4)
    lfs      f1, 0xd0(r4)
    fcmpo    cr0, f2, f0
    blt      .L_00001358
    lis      r3, sHamGasPlateMaxX@ha
    lfs      f0, sHamGasPlateMaxX@l(r3)
    fcmpo    cr0, f0, f2
    blt      .L_00001358
    lis      r3, sHamGasPlateMinZ@ha
    lfs      f0, sHamGasPlateMinZ@l(r3)
    fcmpo    cr0, f1, f0
    blt      .L_00001358
    lis      r3, sHamGasPlateMaxZ@ha
    lfs      f0, sHamGasPlateMaxZ@l(r3)
    fcmpo    cr0, f0, f1
    bge      .L_00001360
.L_00001358:
    li       r0, 0x0
    b        .L_00001364
.L_00001360:
    li       r0, 0x1
.L_00001364:
    clrlwi.  r0, r0, 24
    beq      .L_00001374
    li       r3, 0x1
    b        .L_00001450
.L_00001374:
    lwz      r4, 0x4(r25)
    lis      r3, sHamGasPlateMaxZ@ha
    lfs      f0, sHamGasPlateMaxZ@l(r3)
    lfs      f31, 0xd0(r4)
    lfs      f30, 0xc8(r4)
    fcmpo    cr0, f0, f31
    bge      .L_000013AC
    lfs      f1, 0x18(r25)
    bl       cos
    lis      r3, sHamZero@ha
    frsp     f1, f1
    lfs      f0, sHamZero@l(r3)
    fcmpo    cr0, f0, f1
    blt      .L_00001430
.L_000013AC:
    lis      r3, sHamGasPlateMinZ@ha
    lfs      f0, sHamGasPlateMinZ@l(r3)
    fcmpo    cr0, f31, f0
    bge      .L_000013D8
    lfs      f1, 0x18(r25)
    bl       cos
    lis      r3, sHamZero@ha
    frsp     f1, f1
    lfs      f0, sHamZero@l(r3)
    fcmpo    cr0, f1, f0
    blt      .L_00001430
.L_000013D8:
    lis      r3, sHamGasPlateMaxX@ha
    lfs      f0, sHamGasPlateMaxX@l(r3)
    fcmpo    cr0, f0, f30
    bge      .L_00001404
    lfs      f1, 0x18(r25)
    bl       sin
    lis      r3, sHamZero@ha
    frsp     f1, f1
    lfs      f0, sHamZero@l(r3)
    fcmpo    cr0, f0, f1
    blt      .L_00001430
.L_00001404:
    lis      r3, sHamGasPlateMinX@ha
    lfs      f0, sHamGasPlateMinX@l(r3)
    fcmpo    cr0, f30, f0
    bge      .L_00001438
    lfs      f1, 0x18(r25)
    bl       sin
    lis      r3, sHamZero@ha
    frsp     f1, f1
    lfs      f0, sHamZero@l(r3)
    fcmpo    cr0, f1, f0
    bge      .L_00001438
.L_00001430:
    li       r0, 0x1
    b        .L_0000143C
.L_00001438:
    li       r0, 0x0
.L_0000143C:
    clrlwi.  r0, r0, 24
    beq      .L_0000144C
    li       r3, 0x1
    b        .L_00001450
.L_0000144C:
    li       r3, 0x0
.L_00001450:
    psq_l    f31, 0x68(r1), 0, qr0
    lfd      f31, 0x60(r1)
    psq_l    f30, 0x58(r1), 0, qr0
    lfd      f30, 0x50(r1)
    psq_l    f29, 0x48(r1), 0, qr0
    lfd      f29, 0x40(r1)
    lmw      r25, 0x24(r1)
    lwz      r0, 0x74(r1)
    mtlr     r0
    addi     r1, r1, 0x70
    blr
*/

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
			slot->mActor->mStatus &= ~0x100000000ULL;
			slot->mActor->mStatus &= ~0x10000000000ULL;
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
				slot->mActor->mStatus |= 0x100000000ULL;
			}
		}
	}
	ClearTempActors();
}

// UpdateHamPickupFlags
/*
    stwu     r1, -0x50(r1)
    mflr     r0
    stw      r0, 0x54(r1)
    stfd     f31, 0x40(r1)
    psq_st   f31, 0x48(r1), 0, qr0
    stmw     r23, 0x1c(r1)
    lis      r4, hamObjs@ha
    lis      r3, gGame@ha
    addi     r24, r4, hamObjs@l
    lis      r5, putDownActive@ha
    li       r0, 0x0
    addi     r3, r3, gGame@l
    stb      r0, putDownActive@l(r5)
    mr       r26, r24
    addis    r25, r3, 0x1
    li       r23, 0x0
.L_000014BC:
    lwz      r3, 0x4(r26)
    cmplwi   r3, 0x0
    bne      .L_000014D0
    li       r6, 0x0
    b        .L_00001500
.L_000014D0:
    lwz      r0, 0x228(r3)
    li       r6, 0x0
    lwz      r5, 0x22c(r3)
    lis      r4, 0x1
    and      r0, r0, r6
    and      r4, r5, r4
    xor      r4, r4, r6
    xor      r0, r0, r6
    or.      r0, r4, r0
    bne      .L_000014FC
    b        .L_00001500
.L_000014FC:
    li       r6, 0x1
.L_00001500:
    clrlwi.  r0, r6, 24
    beq      .L_0000167C
    lwz      r0, 0x34(r26)
    cmplwi   r0, 0x0
    beq      .L_00001558
    lwz      r0, 0x22c(r3)
    li       r6, -0x1
    lwz      r7, 0x228(r3)
    li       r5, -0x2
    and      r0, r0, r6
    li       r4, -0x101
    stw      r0, 0x22c(r3)
    and      r0, r7, r5
    stw      r0, 0x228(r3)
    lwz      r5, 0x4(r26)
    lwz      r0, 0x22c(r5)
    lwz      r3, 0x228(r5)
    and      r0, r0, r6
    stw      r0, 0x22c(r5)
    and      r0, r3, r4
    stw      r0, 0x228(r5)
    b        .L_0000167C
.L_00001558:
    lwz      r5, 0xc8(r3)
    lis      r4, sHamPickupSearchRadius@ha
    lwz      r0, 0xcc(r3)
    li       r30, 0x0
    lwz      r6, 0x30(r26)
    mr       r31, r30
    stw      r5, 0x8(r1)
    mr       r27, r24
    lwz      r28, 0x8(r26)
    addi     r29, r6, 0x1
    stw      r0, 0xc(r1)
    lfs      f31, sHamPickupSearchRadius@l(r4)
    lwz      r0, 0xd0(r3)
    stw      r0, 0x10(r1)
.L_00001590:
    lwz      r3, 0x4(r27)
    cmplwi   r3, 0x0
    bne      .L_000015A4
    li       r6, 0x0
    b        .L_000015D4
.L_000015A4:
    lwz      r0, 0x228(r3)
    li       r6, 0x0
    lwz      r5, 0x22c(r3)
    lis      r4, 0x1
    and      r0, r0, r6
    and      r4, r5, r4
    xor      r4, r4, r6
    xor      r0, r0, r6
    or.      r0, r4, r0
    bne      .L_000015D0
    b        .L_000015D4
.L_000015D0:
    li       r6, 0x1
.L_000015D4:
    clrlwi.  r0, r6, 24
    beq      .L_00001654
    lwz      r0, 0x8(r27)
    cmpw     r28, r0
    beq      .L_00001654
    lwz      r0, 0x30(r27)
    cmpw     r29, r0
    bne      .L_00001654
    lwz      r0, 0x34(r27)
    cmplwi   r0, 0x0
    bne      .L_00001654
    lwz      r5, -0x6944(r25)
    li       r4, 0x0
    lwz      r0, 0xa48(r5)
    cmpwi    r0, 0x0
    beq      .L_00001624
    lwz      r0, 0xa64(r5)
    cmplw    r0, r3
    bne      .L_00001624
    li       r4, 0x1
.L_00001624:
    clrlwi.  r0, r4, 24
    bne      .L_00001654
    lfs      f0, 0xcc(r3)
    addi     r4, r1, 0x8
    lwz      r3, 0x4(r27)
    stfs     f0, 0xc(r1)
    addi     r3, r3, 0xc8
    bl       PSVECDistance
    fcmpo    cr0, f1, f31
    bge      .L_00001654
    mr       r30, r27
    fmr      f31, f1
.L_00001654:
    addi     r31, r31, 0x1
    addi     r27, r27, 0x50
    cmplwi   r31, 0x9
    blt      .L_00001590
    cmplwi   r30, 0x0
    bne      .L_0000167C
    lwz      r4, 0x4(r26)
    lwz      r0, 0x228(r4)
    ori      r0, r0, 0x1
    stw      r0, 0x228(r4)
.L_0000167C:
    addi     r23, r23, 0x1
    addi     r26, r26, 0x50
    cmplwi   r23, 0x9
    blt      .L_000014BC
    lis      r4, tempActors@ha
    lis      r3, 0xffff
    addi     r7, r4, tempActors@l
    li       r4, -0x1
    lwz      r8, 0x0(r7)
    subi     r5, r3, 0x1
    li       r0, 0x0
    cmplwi   r8, 0x0
    beq      .L_000016CC
    lwz      r3, 0x22c(r8)
    lwz      r6, 0x228(r8)
    and      r3, r3, r5
    stw      r3, 0x22c(r8)
    and      r3, r6, r4
    stw      r3, 0x228(r8)
    stw      r0, 0x0(r7)
.L_000016CC:
    lwzu     r8, 0x4(r7)
    cmplwi   r8, 0x0
    beq      .L_000016F4
    lwz      r3, 0x22c(r8)
    lwz      r6, 0x228(r8)
    and      r3, r3, r5
    stw      r3, 0x22c(r8)
    and      r3, r6, r4
    stw      r3, 0x228(r8)
    stw      r0, 0x0(r7)
.L_000016F4:
    lwzu     r8, 0x4(r7)
    cmplwi   r8, 0x0
    beq      .L_0000171C
    lwz      r3, 0x22c(r8)
    lwz      r6, 0x228(r8)
    and      r3, r3, r5
    stw      r3, 0x22c(r8)
    and      r3, r6, r4
    stw      r3, 0x228(r8)
    stw      r0, 0x0(r7)
.L_0000171C:
    lwzu     r8, 0x4(r7)
    cmplwi   r8, 0x0
    beq      .L_00001744
    lwz      r3, 0x22c(r8)
    lwz      r6, 0x228(r8)
    and      r3, r3, r5
    stw      r3, 0x22c(r8)
    and      r3, r6, r4
    stw      r3, 0x228(r8)
    stw      r0, 0x0(r7)
.L_00001744:
    psq_l    f31, 0x48(r1), 0, qr0
    lfd      f31, 0x40(r1)
    lmw      r23, 0x1c(r1)
    lwz      r0, 0x54(r1)
    mtlr     r0
    addi     r1, r1, 0x50
    blr
*/

/**
 * @note Address: stage01 .text 0x1760
 * @note Size: 0x8C
 */
extern "C" HamObj* FindFreeHamObj(void)
{
	s32 i;
	for (i = 0; i < 9; i++) {
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

	for (i = 0; i < 9; i++) {
		HamObj* slot        = &hamObjs[i];
		Stage01Actor* actor = slot->mActor;
		if (IsActive(actor) && (actor->mStatus & 0x100000000ULL) && slot->mStackLevel <= 0 && actor->mCoord.mKind->mObjectId == 0x1F9) {
			dist      = PSVECDistance(&STAGE01_PLAYER->mMoveTranslate, &actor->mMoveTranslate);
			Vec* pos  = slot->GetPos();
			Vec* ppos = &STAGE01_PLAYER->mMoveTranslate;
			f32 angle = atan2(pos->x - ppos->x, pos->z - ppos->z);
			c         = cos(angle - STAGE01_PLAYER->mMoveRotateY);
			if ((f32)cos(0.7853982f) < c && dist < 15.0f && (best == NULL || dist < bestDist)) {
				best     = slot;
				bestDist = dist;
			}
		}
	}
	return best;
}

// FindFacingHamburg
/*
    stwu     r1, -0x50(r1)
    mflr     r0
    stw      r0, 0x54(r1)
    stfd     f31, 0x40(r1)
    psq_st   f31, 0x48(r1), 0, qr0
    stfd     f30, 0x30(r1)
    psq_st   f30, 0x38(r1), 0, qr0
    stfd     f29, 0x20(r1)
    psq_st   f29, 0x28(r1), 0, qr0
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    stw      r28, 0x10(r1)
    lis      r3, sHamZero@ha
    lis      r4, hamObjs@ha
    addi     r5, r3, sHamZero@l
    li       r29, 0x0
    lis      r3, gGame@ha
    lfs      f29, 0x0(r5)
    addi     r3, r3, gGame@l
    addi     r31, r4, hamObjs@l
    li       r28, 0x0
    addis    r30, r3, 0x1
.L_00001848:
    lwz      r6, 0x4(r31)
    cmplwi   r6, 0x0
    bne      .L_0000185C
    li       r5, 0x0
    b        .L_0000188C
.L_0000185C:
    lwz      r0, 0x228(r6)
    li       r5, 0x0
    lwz      r4, 0x22c(r6)
    lis      r3, 0x1
    and      r0, r0, r5
    and      r3, r4, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    bne      .L_00001888
    b        .L_0000188C
.L_00001888:
    li       r5, 0x1
.L_0000188C:
    clrlwi.  r0, r5, 24
    beq      .L_00001968
    lwz      r3, 0x228(r6)
    li       r0, 0x1
    lwz      r5, 0x22c(r6)
    li       r4, 0x0
    and      r0, r3, r0
    and      r3, r5, r4
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_00001968
    lwz      r0, 0x30(r31)
    cmpwi    r0, 0x0
    bgt      .L_00001968
    lwz      r3, 0x238(r6)
    lwz      r0, 0x0(r3)
    cmpwi    r0, 0x1f9
    bne      .L_00001968
    lwz      r3, -0x6944(r30)
    addi     r4, r6, 0xc8
    addi     r3, r3, 0xc8
    bl       PSVECDistance
    lwz      r4, 0x4(r31)
    fmr      f31, f1
    lwz      r3, -0x6944(r30)
    lfs      f4, 0xc8(r4)
    lfs      f3, 0xc8(r3)
    lfs      f2, 0xd0(r4)
    lfs      f0, 0xd0(r3)
    fsubs    f1, f4, f3
    fsubs    f2, f2, f0
    bl       atan2
    lwz      r3, -0x6944(r30)
    frsp     f1, f1
    lfs      f0, 0xfc(r3)
    fsubs    f1, f1, f0
    bl       cos
    lis      r3, sHamFacingHalfAngle@ha
    frsp     f30, f1
    lfd      f1, sHamFacingHalfAngle@l(r3)
    bl       cos
    frsp     f0, f1
    fcmpo    cr0, f0, f30
    bge      .L_00001968
    lis      r3, sHamFacingDistance@ha
    lfs      f0, sHamFacingDistance@l(r3)
    fcmpo    cr0, f31, f0
    bge      .L_00001968
    cmplwi   r29, 0x0
    beq      .L_00001960
    fcmpo    cr0, f31, f29
    bge      .L_00001968
.L_00001960:
    mr       r29, r31
    fmr      f29, f31
.L_00001968:
    addi     r28, r28, 0x1
    addi     r31, r31, 0x50
    cmplwi   r28, 0x9
    blt      .L_00001848
    mr       r3, r29
    psq_l    f31, 0x48(r1), 0, qr0
    lfd      f31, 0x40(r1)
    psq_l    f30, 0x38(r1), 0, qr0
    lfd      f30, 0x30(r1)
    psq_l    f29, 0x28(r1), 0, qr0
    lfd      f29, 0x20(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    lwz      r0, 0x54(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x50
    blr
*/

/**
 * @note Address: stage01 .text 0x19B4
 * @note Size: 0x68
 */
extern "C" HamObj* FindHamObjByActor(Stage01Actor* actor)
{
	s32 i;
	if (actor == NULL) {
		return NULL;
	}
	for (i = 0; i < 9; i++) {
		Stage01Actor* current = hamObjs[i].mActor;
		if (current != NULL && current->mPoolIndex == actor->mPoolIndex) {
			return &hamObjs[i];
		}
	}
	return NULL;
}

// FindHamObjByActor
/*
    cmplwi   r3, 0x0
    bne      .L_000019C4
    li       r3, 0x0
    blr
.L_000019C4:
    lis      r4, hamObjs@ha
    li       r0, 0x9
    addi     r5, r4, hamObjs@l
    li       r6, 0x0
    mtctr    r0
.L_000019D8:
    lwz      r4, 0x4(r5)
    cmplwi   r4, 0x0
    beq      .L_00001A08
    lwz      r4, 0x258(r4)
    lwz      r0, 0x258(r3)
    cmpw     r4, r0
    bne      .L_00001A08
    mulli    r4, r6, 0x50
    lis      r3, hamObjs@ha
    addi     r0, r3, hamObjs@l
    add      r3, r0, r4
    blr
.L_00001A08:
    addi     r5, r5, 0x50
    addi     r6, r6, 0x1
    bdnz     .L_000019D8
    li       r3, 0x0
    blr
*/

/**
 * @note Address: stage01 .text 0x1A1C
 * @note Size: 0xD8
 */
extern "C" HamObj* FindHamObj(s32 id)
{
	// NONMATCHING: regalloc
	s32 i;
	if (id < 0) {
		return FindHamObjByActorInline(STAGE01_PLAYER->mLiftTarget);
	}
	for (i = 0; i < 9; i++) {
		if (hamObjs[i].mActor != NULL && id == hamObjs[i].mId) {
			return &hamObjs[i];
		}
	}
	return NULL;
}

// FindHamObj
/*
    cmpwi    r3, 0x0
    bge      .L_00001AA0
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    lwz      r4, 0xa64(r3)
    cmplwi   r4, 0x0
    bne      .L_00001A48
    li       r3, 0x0
    blr
.L_00001A48:
    lis      r3, hamObjs@ha
    li       r0, 0x9
    addi     r6, r3, hamObjs@l
    li       r5, 0x0
    mtctr    r0
.L_00001A5C:
    lwz      r3, 0x4(r6)
    cmplwi   r3, 0x0
    beq      .L_00001A8C
    lwz      r3, 0x258(r3)
    lwz      r0, 0x258(r4)
    cmpw     r3, r0
    bne      .L_00001A8C
    mulli    r4, r5, 0x50
    lis      r3, hamObjs@ha
    addi     r0, r3, hamObjs@l
    add      r3, r0, r4
    blr
.L_00001A8C:
    addi     r6, r6, 0x50
    addi     r5, r5, 0x1
    bdnz     .L_00001A5C
    li       r3, 0x0
    blr
.L_00001AA0:
    lis      r4, hamObjs@ha
    li       r0, 0x9
    addi     r4, r4, hamObjs@l
    li       r5, 0x0
    mtctr    r0
.L_00001AB4:
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    beq      .L_00001AE0
    lwz      r0, 0x8(r4)
    cmpw     r3, r0
    bne      .L_00001AE0
    mulli    r4, r5, 0x50
    lis      r3, hamObjs@ha
    addi     r0, r3, hamObjs@l
    add      r3, r0, r4
    blr
.L_00001AE0:
    addi     r4, r4, 0x50
    addi     r5, r5, 0x1
    bdnz     .L_00001AB4
    li       r3, 0x0
    blr
*/

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

	for (i = 0; i < 9; i++) {
		if (hamObjs[i].mActor == NULL) {
			slot         = &hamObjs[i];
			slot->mId    = id;
			slot->mActor = STAGE01_ACTORS[id];
			slot->mSide  = 0;
			slot->mDish  = NULL;
			slot->mAbove = NULL;
			switch (slot->mActor->mCoord.mKind->mObjectId) {
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
				slot->mActor->mStatus |= 0x100000000ULL;
				slot->mActor->mStatus |= 0x10000000000ULL;
			}
			return slot;
		}
	}
	return NULL;
}

// RegisterHamObj
/*
    cmpwi    r3, 0x0
    bge      .L_00001B78
    lis      r4, gGame@ha
    addi     r4, r4, gGame@l
    addis    r4, r4, 0x1
    lwz      r4, -0x6944(r4)
    lwz      r5, 0xa64(r4)
    cmplwi   r5, 0x0
    bne      .L_00001B20
    li       r0, 0x0
    b        .L_00001BC8
.L_00001B20:
    lis      r4, hamObjs@ha
    li       r0, 0x9
    addi     r7, r4, hamObjs@l
    li       r6, 0x0
    mtctr    r0
.L_00001B34:
    lwz      r4, 0x4(r7)
    cmplwi   r4, 0x0
    beq      .L_00001B64
    lwz      r4, 0x258(r4)
    lwz      r0, 0x258(r5)
    cmpw     r4, r0
    bne      .L_00001B64
    mulli    r5, r6, 0x50
    lis      r4, hamObjs@ha
    addi     r0, r4, hamObjs@l
    add      r0, r0, r5
    b        .L_00001BC8
.L_00001B64:
    addi     r7, r7, 0x50
    addi     r6, r6, 0x1
    bdnz     .L_00001B34
    li       r0, 0x0
    b        .L_00001BC8
.L_00001B78:
    lis      r4, hamObjs@ha
    li       r0, 0x9
    addi     r5, r4, hamObjs@l
    li       r4, 0x0
    mtctr    r0
.L_00001B8C:
    lwz      r0, 0x4(r5)
    cmplwi   r0, 0x0
    beq      .L_00001BB8
    lwz      r0, 0x8(r5)
    cmpw     r3, r0
    bne      .L_00001BB8
    mulli    r5, r4, 0x50
    lis      r4, hamObjs@ha
    addi     r0, r4, hamObjs@l
    add      r0, r0, r5
    b        .L_00001BC8
.L_00001BB8:
    addi     r5, r5, 0x50
    addi     r4, r4, 0x1
    bdnz     .L_00001B8C
    li       r0, 0x0
.L_00001BC8:
    cmplwi   r0, 0x0
    beq      .L_00001BD8
    mr       r3, r0
    blr
.L_00001BD8:
    lis      r4, hamObjs@ha
    li       r0, 0x9
    addi     r4, r4, hamObjs@l
    li       r7, 0x0
    mtctr    r0
.L_00001BEC:
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_00001CF4
    lis      r4, gGame@ha
    lis      r6, hamObjs@ha
    addi     r4, r4, gGame@l
    slwi     r0, r3, 2
    addis    r5, r4, 0x1
    addi     r6, r6, hamObjs@l
    mulli    r7, r7, 0x50
    li       r4, 0x0
    add      r5, r5, r0
    lwz      r0, -0x67a0(r5)
    add      r6, r6, r7
    stw      r3, 0x8(r6)
    addi     r5, r6, 0x4
    stw      r0, 0x4(r6)
    stw      r4, 0x24(r6)
    stw      r4, 0x34(r6)
    stw      r4, 0x0(r6)
    lwz      r3, 0x4(r6)
    lwz      r3, 0x238(r3)
    lwz      r0, 0x0(r3)
    cmpwi    r0, 0x263
    beq      .L_00001C80
    bge      .L_00001C60
    cmpwi    r0, 0x1f9
    beq      .L_00001C6C
    b        .L_00001CB0
.L_00001C60:
    cmpwi    r0, 0x265
    bge      .L_00001CB0
    b        .L_00001C98
.L_00001C6C:
    stw      r4, 0x2c(r6)
    li       r0, 0x1
    stw      r4, 0x28(r6)
    stw      r0, 0xc(r6)
    b        .L_00001CC4
.L_00001C80:
    li       r3, 0x270f
    li       r0, 0x2
    stw      r3, 0x2c(r6)
    stw      r3, 0x28(r6)
    stw      r0, 0xc(r6)
    b        .L_00001CC4
.L_00001C98:
    li       r3, 0x270f
    li       r0, 0x3
    stw      r3, 0x2c(r6)
    stw      r3, 0x28(r6)
    stw      r0, 0xc(r6)
    b        .L_00001CC4
.L_00001CB0:
    lis      r3, hamObjs@ha
    li       r4, 0x0
    addi     r0, r3, hamObjs@l
    add      r3, r0, r7
    stw      r4, 0xc(r3)
.L_00001CC4:
    lwz      r4, 0x0(r5)
    cmplwi   r4, 0x0
    beq      .L_00001CEC
    lwz      r0, 0x228(r4)
    ori      r0, r0, 0x1
    stw      r0, 0x228(r4)
    lwz      r4, 0x0(r5)
    lwz      r0, 0x228(r4)
    ori      r0, r0, 0x100
    stw      r0, 0x228(r4)
.L_00001CEC:
    mr       r3, r6
    blr
.L_00001CF4:
    addi     r4, r4, 0x50
    addi     r7, r7, 0x1
    bdnz     .L_00001BEC
    li       r3, 0x0
    blr
*/

/**
 * @note Address: stage01 .text 0x1D08
 * @note Size: 0x108
 */
extern "C" void ExitHamburg(void)
{
	s32 i;

	if (gasPlate != NULL) {
		gasPlate->mStatus |= 0x200000000ULL;
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
	AllowIdleHudCounters(STAGE01_OBJ_8F38);
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
		Stage01Actor* actor = (Stage01Actor*)game->mXObjs.mObjects[i];
		if ((actor->mStatus & 1) && actor->mCoord.mKind->mObjectId == 0x200) {
			gasPlate = actor;
		}
	}
	if (gasPlate != NULL) {
		gasPlate->mStatus &= ~0x200000000ULL;
	}

	for (i = 0; i < 4; i++) {
		tempActors[i] = NULL;
	}
	yakiResultFlags.mFlags = 0;
	putDownActive          = 0;
	SuppressIdleHudCounters(STAGE01_OBJ_8F38);
}

// InitHamburg
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    lis      r4, hamObjs@ha
    lis      r3, __ptmf_null@ha
    stw      r0, 0x24(r1)
    addi     r7, r4, hamObjs@l
    li       r0, 0x3
    lwzu     r6, __ptmf_null@l(r3)
    lwz      r4, 0x4(r3)
    lwz      r3, 0x8(r3)
    stw      r6, 0x8(r1)
    stw      r4, 0xc(r1)
    stw      r3, 0x10(r1)
    mtctr    r0
    li       r5, -0x1
    li       r0, 0x0
.L_00001E50:
    stw      r5, 0x8(r7)
    stw      r0, 0x4(r7)
    stw      r6, 0x40(r7)
    stw      r4, 0x44(r7)
    stw      r3, 0x48(r7)
    stw      r0, 0x3c(r7)
    stw      r5, 0x58(r7)
    stw      r0, 0x54(r7)
    stw      r6, 0x90(r7)
    stw      r4, 0x94(r7)
    stw      r3, 0x98(r7)
    stw      r0, 0x8c(r7)
    stw      r5, 0xa8(r7)
    stw      r0, 0xa4(r7)
    stw      r6, 0xe0(r7)
    stw      r4, 0xe4(r7)
    stw      r3, 0xe8(r7)
    stw      r0, 0xdc(r7)
    addi     r7, r7, 0xf0
    bdnz     .L_00001E50
    lis      r3, gGame@ha
    lis      r4, hamDishes@ha
    addi     r8, r3, gGame@l
    li       r10, 0x0
    addi     r4, r4, hamDishes@l
    lis      r3, gasPlate@ha
    addi     r6, r3, gasPlate@l
    stw      r5, 0x4(r4)
    addis    r3, r8, 0x1
    lwz      r9, -0x57a0(r3)
    stw      r0, 0x0(r4)
    stw      r5, 0x1c(r4)
    stw      r0, 0x18(r4)
    stw      r5, 0x34(r4)
    stw      r0, 0x30(r4)
    stw      r5, 0x4c(r4)
    stw      r0, 0x48(r4)
    stw      r0, 0x0(r6)
    b        .L_00001F3C
.L_00001EEC:
    addis    r4, r8, 0x1
    li       r3, 0x1
    lwz      r7, -0x67a0(r4)
    li       r4, 0x0
    lwz      r0, 0x228(r7)
    lwz      r5, 0x22c(r7)
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_00001F34
    lwz      r3, 0x238(r7)
    lwz      r0, 0x0(r3)
    cmpwi    r0, 0x200
    bne      .L_00001F34
    lis      r3, gasPlate@ha
    stw      r7, gasPlate@l(r3)
.L_00001F34:
    addi     r8, r8, 0x4
    addi     r10, r10, 0x1
.L_00001F3C:
    lwz      r7, 0x0(r6)
    cmplwi   r7, 0x0
    bne      .L_00001F50
    cmpw     r10, r9
    blt      .L_00001EEC
.L_00001F50:
    cmplwi   r7, 0x0
    beq      .L_00001F78
    lwz      r5, 0x22c(r7)
    li       r0, -0x1
    lwz      r4, 0x228(r7)
    li       r3, -0x3
    and      r0, r5, r0
    stw      r0, 0x22c(r7)
    and      r0, r4, r3
    stw      r0, 0x228(r7)
.L_00001F78:
    li       r0, 0x0
    lis      r6, tempActors@ha
    stwu     r0, tempActors@l(r6)
    lis      r3, gGame@ha
    lis      r5, yakiResultFlags@ha
    lis      r4, putDownActive@ha
    addi     r3, r3, gGame@l
    stw      r0, 0x4(r6)
    addis    r3, r3, 0x1
    stw      r0, 0x8(r6)
    subi     r3, r3, 0x70c8
    stw      r0, 0xc(r6)
    stw      r0, yakiResultFlags@l(r5)
    stb      r0, putDownActive@l(r4)
    bl       SuppressIdleHudCounters
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

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

	Stage01Actor* player = STAGE01_PLAYER;
	f32 x                = player->mMoveTranslate.x;
	f32 z                = player->mMoveTranslate.z;
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
	f32 dist = PSVECDistance(&mActor->mMoveTranslate, &mActor->mMoveTarget);
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
	Stage01Actor* player = STAGE01_PLAYER;
	s32 level;

	switch (mStateTime) {
	case 0: {
		if (player->mLiftState != 6 || player->mAnimId != 9) {
			break;
		}
		HSD_AObj* aobj = GetAObj(player->mJObj);
		f32 frame      = aobj != NULL ? HSD_AObjGetCurrFrame(aobj) : 0.0f;
		if (frame < 15.0f) {
			break;
		}

		if (IsHeld(mActor)) {
			STAGE01_PLAYER->mLiftTarget = NULL;
		}
		mActor->mStatus &= ~0x400000000000000ULL;
		mMoveAngle = player->mMoveRotateY;
		mMoveSpeed = 2.0f;
		Vec pos    = mActor->mMoveTranslate;
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

		for (level = 4; level >= 0; level--) {
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
	case 1:
		if (PSVECDistance(&mActor->mMoveTranslate, &mActor->mMoveTarget) <= 0.001f) {
			UpdateHamPickupFlags();
			if (IsOnGasPlate(mActor)) {
				Vec pos = mActor->mMoveTranslate;
				pos.y   = mActor->mCoord.GetSize().y;
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
/*
    stwu     r1, -0xf0(r1)
    mflr     r0
    stw      r0, 0xf4(r1)
    stfd     f31, 0xe0(r1)
    psq_st   f31, 0xe8(r1), 0, qr0
    stmw     r23, 0xbc(r1)
    mr       r30, r3
    lis      r3, gGame@ha
    lwz      r0, 0x4c(r30)
    addi     r3, r3, gGame@l
    addis    r31, r3, 0x1
    cmpwi    r0, 0x1
    lwz      r25, -0x6944(r31)
    beq      .L_00002ADC
    bge      .L_00002CF8
    cmpwi    r0, 0x0
    bge      .L_00002398
    b        .L_00002CF8
.L_00002398:
    lwz      r0, 0xa48(r25)
    cmpwi    r0, 0x6
    bne      .L_00002CF8
    lwz      r0, 0x158(r25)
    cmpwi    r0, 0x9
    bne      .L_00002CF8
    lwz      r23, 0x0(r25)
    cmplwi   r23, 0x0
    beq      .L_00002404
    mr       r3, r23
    bl       FindJointAnimationInTree
    cmplwi   r3, 0x0
    beq      .L_000023D4
    mr       r23, r3
    b        .L_00002408
.L_000023D4:
    mr       r3, r23
    bl       FindMaterialOrTextureAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_000023EC
    mr       r23, r3
    b        .L_00002408
.L_000023EC:
    mr       r3, r23
    bl       FindPolygonAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_00002404
    mr       r23, r3
    b        .L_00002408
.L_00002404:
    li       r23, 0x0
.L_00002408:
    cmplwi   r23, 0x0
    beq      .L_00002434
    bne      .L_0000242C
    lis      r3, sStage01PathAobjH@ha
    lis      r5, sHamAnimCondition@ha
    addi     r3, r3, sStage01PathAobjH@l
    li       r4, 0x95
    addi     r5, r5, sHamAnimCondition@l
    bl       __assert
.L_0000242C:
    lfs      f1, 0x4(r23)
    b        .L_0000243C
.L_00002434:
    lis      r3, sHamZero@ha
    lfs      f1, sHamZero@l(r3)
.L_0000243C:
    lis      r3, sHamFacingDistance@ha
    lfs      f0, sHamFacingDistance@l(r3)
    fcmpo    cr0, f1, f0
    blt      .L_00002CF8
    lwz      r0, 0xa48(r25)
    li       r3, 0x0
    lwz      r4, 0x4(r30)
    cmpwi    r0, 0x0
    beq      .L_00002470
    lwz      r0, 0xa64(r25)
    cmplw    r0, r4
    bne      .L_00002470
    li       r3, 0x1
.L_00002470:
    clrlwi.  r0, r3, 24
    beq      .L_00002484
    lwz      r3, -0x6944(r31)
    li       r0, 0x0
    stw      r0, 0xa64(r3)
.L_00002484:
    lwz      r7, 0x4(r30)
    lis      r4, 0xfc00
    lis      r3, sHamStackLevelHeight@ha
    li       r0, -0x1
    lwz      r6, 0x22c(r7)
    subi     r4, r4, 0x1
    lwz      r5, 0x228(r7)
    and      r0, r6, r0
    lfs      f0, sHamStackLevelHeight@l(r3)
    stw      r0, 0x22c(r7)
    and      r0, r5, r4
    stw      r0, 0x228(r7)
    lfs      f1, 0xfc(r25)
    stfs     f1, 0x18(r30)
    stfs     f0, 0x20(r30)
    lwz      r4, 0x4(r30)
    lwz      r3, 0xc8(r4)
    lwz      r0, 0xcc(r4)
    stw      r3, 0x80(r1)
    stw      r0, 0x84(r1)
    lwz      r0, 0xd0(r4)
    stw      r0, 0x88(r1)
    lfs      f1, 0x18(r30)
    bl       sin
    lis      r3, sHamReturnSlideSpeed@ha
    frsp     f2, f1
    lfs      f0, 0x80(r1)
    lfs      f1, sHamReturnSlideSpeed@l(r3)
    fmadds   f0, f1, f2, f0
    stfs     f0, 0x80(r1)
    lfs      f1, 0x18(r30)
    bl       cos
    lis      r3, sHamReturnSlideSpeed@ha
    frsp     f2, f1
    lfs      f0, 0x88(r1)
    lfs      f1, sHamReturnSlideSpeed@l(r3)
    fmadds   f0, f1, f2, f0
    stfs     f0, 0x88(r1)
    lwz      r0, 0x34(r30)
    cmplwi   r0, 0x0
    beq      .L_00002714
    lis      r3, gGame@ha
    lis      r4, 0x4
    addi     r3, r3, gGame@l
    li       r5, 0xff
    addis    r3, r3, 0xe
    addi     r4, r4, 0x1e
    addi     r3, r3, 0x7e24
    bl       PlayGameSound
    lwz      r4, 0x34(r30)
    lwz      r3, 0x0(r4)
    lfs      f0, 0xc8(r3)
    stfs     f0, 0x80(r1)
    lfs      f0, 0xcc(r3)
    stfs     f0, 0x84(r1)
    lfs      f0, 0xd0(r3)
    stfs     f0, 0x88(r1)
    lwz      r0, 0x8(r4)
    cmplwi   r0, 0x0
    bne      .L_0000257C
    li       r0, 0x0
    b        .L_000025BC
.L_0000257C:
    lwz      r0, 0xc(r4)
    cmplwi   r0, 0x0
    bne      .L_00002590
    li       r0, 0x1
    b        .L_000025BC
.L_00002590:
    lwz      r0, 0x10(r4)
    cmplwi   r0, 0x0
    bne      .L_000025A4
    li       r0, 0x2
    b        .L_000025BC
.L_000025A4:
    lwz      r0, 0x14(r4)
    cmplwi   r0, 0x0
    bne      .L_000025B8
    li       r0, 0x3
    b        .L_000025BC
.L_000025B8:
    li       r0, 0x4
.L_000025BC:
    stw      r0, 0x30(r30)
    lis      r8, 0x4330
    lis      r4, sHamRatePercent@ha
    lis      r5, sHamRateSignedBias@ha
    lwz      r3, 0x4(r30)
    addi     r7, r4, sHamRatePercent@l
    lis      r4, sHamPlacementPosition@ha
    addi     r10, r5, sHamRateSignedBias@l
    lwz      r9, 0x238(r3)
    addi     r12, r4, sHamPlacementPosition@l
    lis      r4, sHamStackLevelHeight@ha
    lfd      f5, 0x0(r10)
    lwz      r0, 0x8(r9)
    addi     r5, r4, sHamStackLevelHeight@l
    lfs      f3, 0x0(r7)
    xoris    r4, r0, 0x8000
    lwz      r11, 0x4(r12)
    stw      r4, 0x9c(r1)
    addi     r4, r1, 0x80
    lwz      r10, 0x0(r12)
    stw      r8, 0x98(r1)
    lwz      r7, 0x8(r12)
    lfd      f0, 0x98(r1)
    lwz      r6, 0x4(r9)
    fsubs    f0, f0, f5
    lwz      r0, 0xc(r9)
    xoris    r9, r6, 0x8000
    stw      r8, 0x90(r1)
    xoris    r6, r0, 0x8000
    lwz      r0, 0x30(r30)
    fdivs    f1, f0, f3
    stw      r9, 0x94(r1)
    xoris    r0, r0, 0x8000
    lfs      f2, 0x0(r5)
    lfd      f4, 0x90(r1)
    stw      r6, 0xa4(r1)
    stw      r8, 0xa0(r1)
    fsubs    f4, f4, f5
    lfd      f0, 0xa0(r1)
    stw      r11, 0x24(r1)
    fdivs    f4, f4, f3
    stfs     f1, 0x24(r1)
    lwz      r5, 0x24(r1)
    stw      r10, 0x20(r1)
    stw      r0, 0xac(r1)
    stw      r8, 0xa8(r1)
    fsubs    f0, f0, f5
    lfd      f1, 0xa8(r1)
    stfs     f4, 0x20(r1)
    fsubs    f1, f1, f5
    lwz      r0, 0x20(r1)
    fdivs    f3, f0, f3
    stw      r5, 0x54(r1)
    lfs      f0, 0x54(r1)
    stw      r7, 0x28(r1)
    stw      r0, 0x50(r1)
    fmadds   f0, f2, f1, f0
    stfs     f3, 0x28(r1)
    stfs     f0, 0x84(r1)
    lwz      r0, 0x28(r1)
    lwz      r12, 0x34(r3)
    stw      r0, 0x58(r1)
    lwz      r12, 0x78(r12)
    lfs      f1, 0x20(r30)
    mtctr    r12
    bctrl
    lis      r4, __ptmf_null@ha
    lis      r3, gGame@ha
    addi     r6, r4, __ptmf_null@l
    li       r0, 0x0
    lwz      r5, 0x0(r6)
    addi     r3, r3, gGame@l
    lwz      r4, 0x4(r6)
    addis    r3, r3, 0x1
    subi     r3, r3, 0x6eec
    stw      r5, 0x40(r30)
    stw      r4, 0x44(r30)
    lwz      r4, 0x8(r6)
    stw      r4, 0x48(r30)
    stw      r0, 0x4c(r30)
    lwz      r4, 0x3c(r30)
    bl       fn_801A89A0
    li       r0, 0x0
    stw      r0, 0x3c(r30)
    bl       UpdateHamPickupFlags
    b        .L_00002CF8
.L_00002714:
    lwz      r0, 0xc(r30)
    cmpwi    r0, 0x1
    bne      .L_0000274C
    lis      r3, gGame@ha
    lis      r4, 0x4
    addi     r3, r3, gGame@l
    li       r5, 0xff
    addis    r3, r3, 0xe
    addi     r4, r4, 0x1b
    addi     r3, r3, 0x7e24
    bl       PlayGameSound
    lis      r3, yakiSoundPlayed@ha
    li       r0, 0x1
    stb      r0, yakiSoundPlayed@l(r3)
.L_0000274C:
    lis      r4, sHamReturnSlideSpeed@ha
    lis      r3, hamObjs@ha
    addi     r28, r4, sHamReturnSlideSpeed@l
    li       r23, 0x4
    addi     r29, r3, hamObjs@l
.L_00002760:
    lwz      r4, 0x80(r1)
    li       r25, 0x0
    lwz      r3, 0x84(r1)
    mr       r26, r25
    lwz      r0, 0x88(r1)
    mr       r24, r29
    stw      r4, 0x68(r1)
    lfs      f31, 0x0(r28)
    stw      r3, 0x6c(r1)
    stw      r0, 0x70(r1)
    lwz      r27, 0x8(r30)
.L_0000278C:
    lwz      r3, 0x4(r24)
    cmplwi   r3, 0x0
    bne      .L_000027A0
    li       r6, 0x0
    b        .L_000027D0
.L_000027A0:
    lwz      r0, 0x228(r3)
    li       r6, 0x0
    lwz      r5, 0x22c(r3)
    lis      r4, 0x1
    and      r0, r0, r6
    and      r4, r5, r4
    xor      r4, r4, r6
    xor      r0, r0, r6
    or.      r0, r4, r0
    bne      .L_000027CC
    b        .L_000027D0
.L_000027CC:
    li       r6, 0x1
.L_000027D0:
    clrlwi.  r0, r6, 24
    beq      .L_00002850
    lwz      r0, 0x8(r24)
    cmpw     r27, r0
    beq      .L_00002850
    lwz      r0, 0x30(r24)
    cmpw     r23, r0
    bne      .L_00002850
    lwz      r0, 0x34(r24)
    cmplwi   r0, 0x0
    bne      .L_00002850
    lwz      r5, -0x6944(r31)
    li       r4, 0x0
    lwz      r0, 0xa48(r5)
    cmpwi    r0, 0x0
    beq      .L_00002820
    lwz      r0, 0xa64(r5)
    cmplw    r0, r3
    bne      .L_00002820
    li       r4, 0x1
.L_00002820:
    clrlwi.  r0, r4, 24
    bne      .L_00002850
    lfs      f0, 0xcc(r3)
    addi     r4, r1, 0x68
    lwz      r3, 0x4(r24)
    stfs     f0, 0x6c(r1)
    addi     r3, r3, 0xc8
    bl       PSVECDistance
    fcmpo    cr0, f1, f31
    bge      .L_00002850
    mr       r25, r24
    fmr      f31, f1
.L_00002850:
    addi     r26, r26, 0x1
    addi     r24, r24, 0x50
    cmplwi   r26, 0x9
    blt      .L_0000278C
    cmplwi   r25, 0x0
    beq      .L_000029B4
    lwz      r5, 0x4(r25)
    lis      r7, 0x4330
    lis      r6, sHamPlacementPosition@ha
    lis      r4, sHamRateSignedBias@ha
    lfs      f0, 0xcc(r5)
    addi     r5, r4, sHamRateSignedBias@l
    lis      r3, sHamRatePercent@ha
    lfd      f5, 0x0(r5)
    stfs     f0, 0x84(r1)
    addi     r4, r3, sHamRatePercent@l
    lis      r3, sHamStackLevelHeight@ha
    lfs      f4, 0x0(r4)
    stw      r30, 0x0(r25)
    addi     r11, r6, sHamPlacementPosition@l
    lfs      f2, sHamStackLevelHeight@l(r3)
    addi     r4, r1, 0x80
    lwz      r5, 0x30(r25)
    stw      r7, 0xa8(r1)
    addi     r0, r5, 0x1
    stw      r0, 0x30(r30)
    lwz      r3, 0x4(r30)
    lwz      r9, 0x4(r11)
    lwz      r10, 0x238(r3)
    lwz      r0, 0x30(r30)
    lwz      r5, 0x8(r10)
    lwz      r8, 0x4(r10)
    xoris    r0, r0, 0x8000
    xoris    r6, r5, 0x8000
    lwz      r5, 0xc(r10)
    stw      r6, 0xa4(r1)
    xoris    r6, r8, 0x8000
    xoris    r5, r5, 0x8000
    lwz      r10, 0x0(r11)
    stw      r6, 0xac(r1)
    lwz      r8, 0x8(r11)
    lfd      f3, 0xa8(r1)
    stw      r7, 0xa0(r1)
    fsubs    f3, f3, f5
    lfd      f0, 0xa0(r1)
    stw      r5, 0x9c(r1)
    fsubs    f0, f0, f5
    fdivs    f3, f3, f4
    stw      r10, 0x14(r1)
    stw      r7, 0x98(r1)
    stw      r9, 0x18(r1)
    stw      r0, 0x94(r1)
    stw      r7, 0x90(r1)
    fdivs    f1, f0, f4
    lfd      f0, 0x98(r1)
    stfs     f3, 0x14(r1)
    lwz      r0, 0x14(r1)
    stw      r8, 0x1c(r1)
    stw      r0, 0x44(r1)
    stfs     f1, 0x18(r1)
    fsubs    f0, f0, f5
    lfd      f1, 0x90(r1)
    lwz      r5, 0x18(r1)
    fdivs    f3, f0, f4
    stw      r5, 0x48(r1)
    lfs      f0, 0x48(r1)
    fsubs    f1, f1, f5
    stfs     f3, 0x1c(r1)
    lwz      r0, 0x1c(r1)
    fmadds   f0, f2, f1, f0
    stw      r0, 0x4c(r1)
    stfs     f0, 0x84(r1)
    lwz      r12, 0x34(r3)
    lfs      f1, 0x20(r30)
    lwz      r12, 0x78(r12)
    mtctr    r12
    bctrl
    lis      r3, sHamStackLandState@ha
    li       r0, 0x0
    addi     r5, r3, sHamStackLandState@l
    lwz      r4, 0x0(r5)
    lwz      r3, 0x4(r5)
    stw      r4, 0x40(r30)
    stw      r3, 0x44(r30)
    lwz      r3, 0x8(r5)
    stw      r3, 0x48(r30)
    stw      r0, 0x4c(r30)
    bl       UpdateHamPickupFlags
    b        .L_00002CF8
.L_000029B4:
    subic.   r23, r23, 0x1
    bge      .L_00002760
    bl       UpdateHamPickupFlags
    li       r0, 0x0
    lis      r8, 0x4330
    stw      r0, 0x30(r30)
    lis      r3, sHamRateSignedBias@ha
    addi     r10, r3, sHamRateSignedBias@l
    lis      r4, sHamRatePercent@ha
    lwz      r3, 0x4(r30)
    addi     r7, r4, sHamRatePercent@l
    lis      r4, sHamPlacementPosition@ha
    lwz      r0, 0x30(r30)
    lwz      r12, 0x238(r3)
    addi     r23, r4, sHamPlacementPosition@l
    lfs      f4, 0x0(r7)
    lis      r4, sHamStackLevelHeight@ha
    lwz      r9, 0x4(r12)
    addi     r5, r4, sHamStackLevelHeight@l
    lwz      r6, 0x8(r12)
    xoris    r0, r0, 0x8000
    xoris    r9, r9, 0x8000
    lwz      r4, 0xc(r12)
    stw      r9, 0xac(r1)
    xoris    r6, r6, 0x8000
    lwz      r12, 0x0(r23)
    stw      r8, 0xa8(r1)
    lfd      f5, 0x0(r10)
    lfd      f3, 0xa8(r1)
    stw      r6, 0xa4(r1)
    xoris    r6, r4, 0x8000
    fsubs    f3, f3, f5
    lwz      r11, 0x4(r23)
    stw      r8, 0xa0(r1)
    addi     r4, r1, 0x80
    lwz      r7, 0x8(r23)
    lfd      f0, 0xa0(r1)
    fdivs    f3, f3, f4
    stw      r12, 0x8(r1)
    lfs      f2, 0x0(r5)
    stw      r6, 0x9c(r1)
    stw      r8, 0x98(r1)
    stw      r11, 0xc(r1)
    fsubs    f0, f0, f5
    stfs     f3, 0x8(r1)
    stw      r0, 0x94(r1)
    fdivs    f1, f0, f4
    lfd      f0, 0x98(r1)
    stw      r8, 0x90(r1)
    lwz      r0, 0x8(r1)
    stw      r7, 0x10(r1)
    stw      r0, 0x38(r1)
    stfs     f1, 0xc(r1)
    fsubs    f0, f0, f5
    lfd      f1, 0x90(r1)
    lwz      r5, 0xc(r1)
    fdivs    f3, f0, f4
    stw      r5, 0x3c(r1)
    lfs      f0, 0x3c(r1)
    fsubs    f1, f1, f5
    stfs     f3, 0x10(r1)
    lwz      r0, 0x10(r1)
    fmadds   f0, f2, f1, f0
    stw      r0, 0x40(r1)
    stfs     f0, 0x84(r1)
    lwz      r12, 0x34(r3)
    lfs      f1, 0x20(r30)
    lwz      r12, 0x78(r12)
    mtctr    r12
    bctrl
    lwz      r3, 0x4c(r30)
    addi     r0, r3, 0x1
    stw      r0, 0x4c(r30)
    b        .L_00002CF8
.L_00002ADC:
    lwz      r4, 0x4(r30)
    addi     r3, r4, 0xc8
    addi     r4, r4, 0xd8
    bl       PSVECDistance
    lis      r3, sHamLandingTolerance@ha
    lfs      f0, sHamLandingTolerance@l(r3)
    fcmpo    cr0, f1, f0
    cror     eq, lt, eq
    bne      .L_00002CF8
    bl       UpdateHamPickupFlags
    lwz      r10, 0x4(r30)
    lis      r3, sHamGasPlateMinX@ha
    lfs      f0, sHamGasPlateMinX@l(r3)
    lfs      f2, 0xc8(r10)
    lfs      f1, 0xd0(r10)
    fcmpo    cr0, f2, f0
    blt      .L_00002B50
    lis      r3, sHamGasPlateMaxX@ha
    lfs      f0, sHamGasPlateMaxX@l(r3)
    fcmpo    cr0, f0, f2
    blt      .L_00002B50
    lis      r3, sHamGasPlateMinZ@ha
    lfs      f0, sHamGasPlateMinZ@l(r3)
    fcmpo    cr0, f1, f0
    blt      .L_00002B50
    lis      r3, sHamGasPlateMaxZ@ha
    lfs      f0, sHamGasPlateMaxZ@l(r3)
    fcmpo    cr0, f0, f1
    bge      .L_00002B58
.L_00002B50:
    li       r0, 0x0
    b        .L_00002B5C
.L_00002B58:
    li       r0, 0x1
.L_00002B5C:
    clrlwi.  r0, r0, 24
    beq      .L_00002CC8
    lwz      r4, 0xc8(r10)
    lis      r8, 0x4330
    lwz      r0, 0xcc(r10)
    lis      r3, sHamRateSignedBias@ha
    addi     r6, r3, sHamRateSignedBias@l
    stw      r8, 0xa0(r1)
    lis      r3, sHamRatePercent@ha
    lfd      f5, 0x0(r6)
    stw      r4, 0x74(r1)
    addi     r7, r3, sHamRatePercent@l
    lis      r3, sHamResultMinZ@ha
    lis      r4, sHamPlacementPosition@ha
    stw      r0, 0x78(r1)
    addi     r9, r4, sHamPlacementPosition@l
    addi     r5, r3, sHamResultMinZ@l
    lwz      r11, 0x4(r9)
    lwz      r0, 0xd0(r10)
    addi     r4, r1, 0x74
    lwz      r12, 0x0(r9)
    stw      r0, 0x7c(r1)
    lwz      r10, 0x8(r9)
    lwz      r3, 0x4(r30)
    lfs      f4, 0x0(r7)
    lwz      r23, 0x238(r3)
    stw      r11, 0x30(r1)
    lwz      r0, 0x8(r23)
    lwz      r9, 0x4(r23)
    xoris    r6, r0, 0x8000
    lwz      r0, 0xc(r23)
    stw      r6, 0xa4(r1)
    xoris    r6, r9, 0x8000
    xoris    r0, r0, 0x8000
    lfs      f1, 0x0(r5)
    lfd      f0, 0xa0(r1)
    stw      r6, 0xac(r1)
    fsubs    f0, f0, f5
    stw      r8, 0xa8(r1)
    fdivs    f2, f0, f4
    lfd      f3, 0xa8(r1)
    stw      r0, 0x9c(r1)
    stw      r8, 0x98(r1)
    lfd      f0, 0x98(r1)
    stw      r12, 0x2c(r1)
    stfs     f2, 0x30(r1)
    fsubs    f3, f3, f5
    fsubs    f2, f0, f5
    lwz      r0, 0x30(r1)
    fdivs    f3, f3, f4
    stw      r10, 0x34(r1)
    stw      r0, 0x60(r1)
    lfs      f0, 0x60(r1)
    stfs     f0, 0x78(r1)
    lwz      r12, 0x34(r3)
    fdivs    f0, f2, f4
    stfs     f3, 0x2c(r1)
    lwz      r12, 0x78(r12)
    lwz      r0, 0x2c(r1)
    stw      r0, 0x5c(r1)
    stfs     f0, 0x34(r1)
    lwz      r0, 0x34(r1)
    stw      r0, 0x64(r1)
    mtctr    r12
    bctrl
    lis      r3, sHamFlipComparison@ha
    lfs      f31, 0x18(r30)
    addi     r4, r3, sHamFlipComparison@l
    addi     r3, r30, 0x40
    bl       __ptmf_cmpr
    cmpwi    r3, 0x0
    beq      .L_00002CF8
    lis      r3, sHamReturnSlideSpeed@ha
    lis      r4, sHamReturnSlideStep@ha
    addi     r5, r3, sHamReturnSlideSpeed@l
    stfs     f31, 0x18(r30)
    lfs      f1, 0x0(r5)
    lis      r3, sHamSlideState@ha
    lfs      f0, sHamReturnSlideStep@l(r4)
    addi     r5, r3, sHamSlideState@l
    stfs     f1, 0x1c(r30)
    li       r0, 0x0
    stfs     f0, 0x20(r30)
    lwz      r4, 0x0(r5)
    lwz      r3, 0x4(r5)
    stw      r4, 0x40(r30)
    stw      r3, 0x44(r30)
    lwz      r3, 0x8(r5)
    stw      r3, 0x48(r30)
    stw      r0, 0x4c(r30)
    b        .L_00002CF8
.L_00002CC8:
    li       r0, 0x1
    lis      r3, __ptmf_null@ha
    stw      r0, 0x30(r30)
    addi     r5, r3, __ptmf_null@l
    li       r0, 0x0
    lwz      r4, 0x0(r5)
    lwz      r3, 0x4(r5)
    stw      r4, 0x40(r30)
    stw      r3, 0x44(r30)
    lwz      r3, 0x8(r5)
    stw      r3, 0x48(r30)
    stw      r0, 0x4c(r30)
.L_00002CF8:
    psq_l    f31, 0xe8(r1), 0, qr0
    lfd      f31, 0xe0(r1)
    lmw      r23, 0xbc(r1)
    lwz      r0, 0xf4(r1)
    mtlr     r0
    addi     r1, r1, 0xf0
    blr
*/

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
		pos.y = mActor->mCoord.GetSize().y;
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
		} else if (IsSamePos(mActor->mMoveTranslate, mActor->mMoveTarget)) {
			SetState(NULL);
		}
		if (mAbove != NULL && 8.0f < PSVECDistance(&mActor->mMoveTranslate, &mAbove->mActor->mMoveTranslate)) {
			HamObj_LowerStack(mAbove);
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
	case 0:
		if (0.0f < (f32)cos(HSD_JObjGetRotationZ(mActor->mJObj))) {
			mRotZ = PI;
		} else {
			mRotZ = 0.0f;
		}
		mStateTime++;
		break;
	case 1:
		mFlipFrames -= 1.0f;
		if (0.0f < mFlipFrames) {
			f32 rot = HSD_JObjGetRotationZ(mActor->mJObj);
			mActor->SetRotationZ(rot + (mRotZ - rot) / mFlipFrames, 0);
		} else {
			s32 level;
			PlayYakiSound();
			level = mYakiTime[mSide] / 600;
			if (level >= 5) {
				level = 4;
			}
			mActor->SetAnim(level, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mSide = 1 - mSide;
			SetState(NULL);
		}
		break;
	case 2:
		break;
	}
}

// StateFlip
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    mr       r31, r3
    stw      r30, 0x8(r1)
    lwz      r0, 0x4c(r3)
    cmpwi    r0, 0x1
    beq      .L_000030C0
    bge      .L_00003224
    cmpwi    r0, 0x0
    bge      .L_00003058
    b        .L_00003224
    b        .L_00003224
.L_00003058:
    lwz      r3, 0x4(r31)
    lwz      r30, 0x0(r3)
    cmplwi   r30, 0x0
    bne      .L_00003080
    lis      r3, sHamJointHeader@ha
    lis      r5, sHamJointCondition@ha
    addi     r3, r3, sHamJointHeader@l
    li       r4, 0x308
    addi     r5, r5, sHamJointCondition@l
    bl       __assert
.L_00003080:
    lfs      f1, 0x24(r30)
    bl       cos
    lis      r3, sHamZero@ha
    frsp     f1, f1
    lfs      f0, sHamZero@l(r3)
    fcmpo    cr0, f0, f1
    bge      .L_000030AC
    lis      r3, sHamHalfTurn@ha
    lfs      f0, sHamHalfTurn@l(r3)
    stfs     f0, 0x10(r31)
    b        .L_000030B0
.L_000030AC:
    stfs     f0, 0x10(r31)
.L_000030B0:
    lwz      r3, 0x4c(r31)
    addi     r0, r3, 0x1
    stw      r0, 0x4c(r31)
    b        .L_00003224
.L_000030C0:
    lis      r4, sHamDefaultRate@ha
    lis      r3, sHamZero@ha
    lfs      f2, 0x14(r31)
    lfs      f0, sHamDefaultRate@l(r4)
    lfs      f1, sHamZero@l(r3)
    fsubs    f0, f2, f0
    stfs     f0, 0x14(r31)
    lfs      f0, 0x14(r31)
    fcmpo    cr0, f1, f0
    bge      .L_00003144
    lwz      r3, 0x4(r31)
    lwz      r30, 0x0(r3)
    cmplwi   r30, 0x0
    bne      .L_00003110
    lis      r3, sHamJointHeader@ha
    lis      r5, sHamJointCondition@ha
    addi     r3, r3, sHamJointHeader@l
    li       r4, 0x308
    addi     r5, r5, sHamJointCondition@l
    bl       __assert
.L_00003110:
    lfs      f2, 0x24(r30)
    li       r4, 0x0
    lfs      f0, 0x10(r31)
    lwz      r3, 0x4(r31)
    fsubs    f1, f0, f2
    lfs      f0, 0x14(r31)
    lwz      r12, 0x34(r3)
    fdivs    f0, f1, f0
    lwz      r12, 0x2c(r12)
    fadds    f1, f2, f0
    mtctr    r12
    bctrl
    b        .L_00003224
.L_00003144:
    lis      r3, gGame@ha
    lis      r4, 0x4
    addi     r3, r3, gGame@l
    li       r5, 0xff
    addis    r3, r3, 0xe
    addi     r4, r4, 0x1b
    addi     r3, r3, 0x7e24
    bl       PlayGameSound
    lis      r4, yakiSoundPlayed@ha
    li       r0, 0x1
    stb      r0, yakiSoundPlayed@l(r4)
    lis      r3, 0x1b4f
    subi     r4, r3, 0x7e4b
    lwz      r0, 0x24(r31)
    slwi     r0, r0, 2
    add      r3, r31, r0
    lwz      r0, 0x28(r3)
    mulhw    r0, r4, r0
    srawi    r0, r0, 6
    srwi     r3, r0, 31
    add      r4, r0, r3
    cmpwi    r4, 0x5
    blt      .L_000031A4
    li       r4, 0x4
.L_000031A4:
    lwz      r3, 0x4(r31)
    lis      r5, sHamZero@ha
    addi     r7, r5, sHamZero@l
    lis      r6, sHamDefaultRate@ha
    lwz      r12, 0x34(r3)
    lis      r5, sHamResultMinZ@ha
    lfs      f1, 0x0(r7)
    addi     r7, r6, sHamDefaultRate@l
    lwz      r12, 0x80(r12)
    addi     r6, r5, sHamResultMinZ@l
    fmr      f3, f1
    lfs      f2, 0x0(r7)
    lfs      f4, 0x0(r6)
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    mtctr    r12
    bctrl
    lwz      r4, 0x24(r31)
    lis      r3, __ptmf_null@ha
    addi     r5, r3, __ptmf_null@l
    li       r0, 0x0
    subfic   r3, r4, 0x1
    stw      r3, 0x24(r31)
    lwz      r4, 0x0(r5)
    lwz      r3, 0x4(r5)
    stw      r4, 0x40(r31)
    stw      r3, 0x44(r31)
    lwz      r3, 0x8(r5)
    stw      r3, 0x48(r31)
    stw      r0, 0x4c(r31)
.L_00003224:
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    lwz      r30, 0x8(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage01 .text 0x323C
 * @note Size: 0x52C
 */
extern "C" void HamObj_LowerStack(HamObj* slot)
{
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
 * @note Address: stage01 .text 0x3768
 * @note Size: 0xC
 */

/**
 * @note Address: stage01 .text 0x3774
 * @note Size: 0xBC
 */

/**
 * @note Address: stage01 .text 0x3830
 * @note Size: 0x8
 */

/**
 * @note Address: stage01 .text 0x3838
 * @note Size: 0x8
 */

/**
 * @note Address: stage01 .text 0x3840
 * @note Size: 0x8
 */

/**
 * @note Address: stage01 .text 0x3848
 * @note Size: 0xF0
 */

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
	Stage01Actor* player = STAGE01_PLAYER;

	if (IsHeld(slot->mActor)) {
		HamObj* other;
		putDownActive = 1;
		for (i = 0, other = hamObjs; i < 9; i++, other++) {
			if (IsActive(other->mActor)) {
				other->mActor->mStatus &= ~0x100000000ULL;
				if (IsOnTopOf(other, slot)) {
					other->mAbove = NULL;
				}
			}
		}
		slot->SetState(&HamObj::StatePutDown);
	} else if (player->mLiftState == 4) {
		player->mLiftState = 6;
	}
}

/**
 * @note Address: stage01 .text 0x3AC0
 * @note Size: 0x270
 */
extern "C" void HamObj_Respawn(HamObj* slot)
{
	if (putDownActive) {
		slot->mActor->mStatus &= ~0x100000000ULL;
	}

	if (slot->mType == 1) {
		slot->mSide        = 0;
		slot->mYakiTime[1] = 0;
		slot->mYakiTime[0] = 0;
		PlayYakiSound();
	} else {
		PlayGameSound(STAGE01_SOUND, 0x4001E, 0xFF);
	}

	slot->mActor->SetPos(-231.8f, 77.5f, 50.0f, 0);
	slot->mActor->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	slot->SetRot(0.0f);

	f32 speed;
	f32 angle = DEG2RAD * (HSD_Randi(60) + 240);
	speed     = 30.0f + HSD_Randi(15);
	slot->StartSlide(speed, 0.5f, angle);
	slot->mSpawned    = 1;
	slot->mStackLevel = 0;
	slot->mAbove      = NULL;
}

/**
 * @note Address: stage01 .text 0x3D30
 * @note Size: 0x98
 */
extern "C" void HamObj_StartSlide(HamObj* slot, f32 speed, f32 f, f32 angle)
{
	slot->StartSlide(speed, f, angle);
}

/**
 * @note Address: stage01 .text 0x3DC8
 * @note Size: 0xA8
 */
extern "C" void HamObj_StartFlip(HamObj* slot, u32 time)
{
	slot->mFlipFrames = time;
	slot->mMoveAngle  = AngleFrom(&STAGE01_PLAYER->mMoveTranslate, &slot->mActor->mMoveTranslate);
	slot->SetState(&HamObj::StateFlip);
}

// HamObj_StartFlip
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    lis      r5, sHamFlipTimeUnsignedBias@ha
    stw      r0, 0x24(r1)
    lis      r0, 0x4330
    lfd      f1, sHamFlipTimeUnsignedBias@l(r5)
    stw      r4, 0xc(r1)
    lis      r4, gGame@ha
    addi     r4, r4, gGame@l
    stw      r0, 0x8(r1)
    lfd      f0, 0x8(r1)
    stw      r31, 0x1c(r1)
    mr       r31, r3
    fsubs    f0, f0, f1
    addis    r3, r4, 0x1
    stfs     f0, 0x14(r31)
    lwz      r4, 0x4(r31)
    lwz      r3, -0x6944(r3)
    lfs      f3, 0xc8(r4)
    lfs      f1, 0xc8(r3)
    lfs      f2, 0xd0(r4)
    lfs      f0, 0xd0(r3)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f0, f1
    lis      r3, sHamFlipState@ha
    addi     r5, r3, sHamFlipState@l
    li       r0, 0x0
    stfs     f0, 0x18(r31)
    lwz      r4, 0x0(r5)
    lwz      r3, 0x4(r5)
    stw      r4, 0x40(r31)
    stw      r3, 0x44(r31)
    lwz      r3, 0x8(r5)
    stw      r3, 0x48(r31)
    stw      r0, 0x4c(r31)
    lwz      r31, 0x1c(r1)
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: stage01 .text 0x3E70
 * @note Size: 0x86C
 */
extern "C" void HamObj_Update(HamObj* slot, u32 step)
{
	if (!IsActive(slot->mActor)) {
		slot->mSpawned = 0;
	}

	if (slot->mType == 1 && slot->mSpawned && slot->mStackLevel == 0 && !IsHeld(slot->mActor)) {
		s32 oldLevel = slot->mYakiTime[slot->mSide] / 600 + 1;
		f32 rate;
		if (oldLevel < 1 || oldLevel >= ARRAY_SIZE(yakiRates)) {
			rate = 1.0f;
		} else {
			rate = yakiRates[oldLevel];
		}
		slot->mYakiTime[slot->mSide] += step * rate;

		s32 level = slot->mYakiTime[slot->mSide] / 600 + 1;
		if (oldLevel > 5) {
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

		if (level < 4 && oldLevel < level) {
			static const Vec rot = { 0.0f, 0.0f, 0.0f };
			STAGE01_PARTICLE->CreateSRT(0x3A, 1, pos, scale, rot, 1);
		}
		if (level == 4) {
			if (slot->mYakiTime[slot->mSide] % 180 == 0) {
				static const Vec rot = { 0.0f, 0.0f, 0.0f };
				STAGE01_PARTICLE->Kill(0x3B);
				STAGE01_PARTICLE->CreateSRT(0x3B, 1, pos, scale, rot, 1);
			}
		} else if (level >= 5) {
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
			yakiLoopLevel = 2;
		}

		if (slot->mYakiEffect == 0) {
			s32 path[]        = { 0 };
			HSD_JObj* jobj    = GetJointByPath(slot->mActor->mJObj, path, 1);
			slot->mYakiEffect = STAGE01_PARTICLE->Create(0x78, 0, jobj, 1);
		}
	}

	if (slot->mState != NULL) {
		(slot->*(slot->mState))();
	} else if (slot->mSpawned && slot->mDish == NULL) {
		f32 angle = atan2(-263.9f - slot->mActor->mMoveTranslate.x, 42.0f - slot->mActor->mMoveTranslate.z);
		if (!IsOnGasPlate(slot->mActor)) {
			slot->StartSlide(5.0f, 0.2f, angle);
			slot->mStackLevel = 0;
		} else if (slot->mStackLevel == 0 && playerOffGasPlate) {
			if (PSVECDistance(&STAGE01_PLAYER->mMoveTranslate, &slot->mActor->mMoveTranslate) < 8.0f) {
				slot->StartSlide(5.0f, 0.2f, angle);
			}
		} else if (slot->mActor->mCoord.GetSize().y + 2.0f * slot->mStackLevel < slot->mActor->mMoveTranslate.y) {
			slot->StartSlide(0.0f, 0.5f, angle);
		}

		if (!putDownActive && !(slot->mActor->mStatus & 0x100000000ULL)
		    && (slot->mAbove == NULL || (slot->mAbove != NULL && slot->mStackLevel == slot->mAbove->mStackLevel))) {
			slot->mActor->mStatus |= 0x100000000ULL;
		}
	}

	if (slot->mSpawned && slot->mActor->mMoveTranslate.y < 77.5f) {
		slot->mActor->SetTranslateY(78.0f, 0);
		slot->mActor->SetRotationZ(slot->mRotZ, 0);
	}
}

// HamObj_Update
/*
    stwu     r1, -0xa0(r1)
    mflr     r0
    stw      r0, 0xa4(r1)
    stfd     f31, 0x90(r1)
    psq_st   f31, 0x98(r1), 0, qr0
    stw      r31, 0x8c(r1)
    stw      r30, 0x88(r1)
    stw      r29, 0x84(r1)
    mr       r30, r3
    lwz      r3, 0x4(r3)
    cmplwi   r3, 0x0
    bne      .L_00003EA8
    li       r6, 0x0
    b        .L_00003ED8
.L_00003EA8:
    lwz      r0, 0x228(r3)
    li       r6, 0x0
    lwz      r5, 0x22c(r3)
    lis      r3, 0x1
    and      r0, r0, r6
    and      r3, r5, r3
    xor      r3, r3, r6
    xor      r0, r0, r6
    or.      r0, r3, r0
    bne      .L_00003ED4
    b        .L_00003ED8
.L_00003ED4:
    li       r6, 0x1
.L_00003ED8:
    clrlwi.  r0, r6, 24
    bne      .L_00003EE8
    li       r0, 0x0
    stb      r0, 0x38(r30)
.L_00003EE8:
    lwz      r0, 0xc(r30)
    cmpwi    r0, 0x1
    bne      .L_000042C8
    lbz      r0, 0x38(r30)
    cmplwi   r0, 0x0
    beq      .L_000042C8
    lwz      r0, 0x30(r30)
    cmpwi    r0, 0x0
    bne      .L_000042C8
    lis      r3, gGame@ha
    lwz      r6, 0x4(r30)
    addi     r5, r3, gGame@l
    li       r3, 0x0
    addis    r5, r5, 0x1
    lwz      r5, -0x6944(r5)
    lwz      r0, 0xa48(r5)
    cmpwi    r0, 0x0
    beq      .L_00003F40
    lwz      r0, 0xa64(r5)
    cmplw    r0, r6
    bne      .L_00003F40
    li       r3, 0x1
.L_00003F40:
    clrlwi.  r0, r3, 24
    bne      .L_000042C8
    lwz      r0, 0x24(r30)
    lis      r3, 0x1b4f
    subi     r3, r3, 0x7e4b
    slwi     r0, r0, 2
    add      r9, r30, r0
    lwz      r0, 0x28(r9)
    mulhw    r0, r3, r0
    srawi    r0, r0, 6
    srwi     r3, r0, 31
    add      r3, r0, r3
    addi     r0, r3, 0x1
    cmpwi    r0, 0x1
    blt      .L_00003F84
    cmplwi   r0, 0x6
    blt      .L_00003F90
.L_00003F84:
    lis      r3, sHamDefaultRate@ha
    lfs      f3, sHamDefaultRate@l(r3)
    b        .L_00003FA0
.L_00003F90:
    lis      r3, yakiRates@ha
    slwi     r5, r0, 2
    addi     r3, r3, yakiRates@l
    lfsx     f3, r3, r5
.L_00003FA0:
    lis      r7, 0x4330
    lwz      r3, 0x28(r9)
    stw      r4, 0x64(r1)
    lis      r8, sHamFlipTimeUnsignedBias@ha
    xoris    r5, r3, 0x8000
    lis      r6, sHamRateSignedBias@ha
    stw      r7, 0x60(r1)
    lis      r3, 0x1b4f
    lfd      f2, sHamFlipTimeUnsignedBias@l(r8)
    subi     r4, r3, 0x7e4b
    lfd      f0, 0x60(r1)
    cmpwi    r0, 0x5
    stw      r5, 0x6c(r1)
    lfd      f1, sHamRateSignedBias@l(r6)
    fsubs    f2, f0, f2
    stw      r7, 0x68(r1)
    lfd      f0, 0x68(r1)
    fsubs    f0, f0, f1
    fmadds   f0, f2, f3, f0
    fctiwz   f0, f0
    stfd     f0, 0x70(r1)
    lwz      r3, 0x74(r1)
    stw      r3, 0x28(r9)
    lwz      r3, 0x24(r30)
    slwi     r3, r3, 2
    add      r3, r30, r3
    lwz      r3, 0x28(r3)
    mulhw    r3, r4, r3
    srawi    r3, r3, 6
    srwi     r4, r3, 31
    add      r3, r3, r4
    addi     r31, r3, 0x1
    ble      .L_00004028
    li       r0, 0x5
.L_00004028:
    cmpwi    r31, 0x5
    ble      .L_00004034
    li       r31, 0x5
.L_00004034:
    lwz      r5, 0x4(r30)
    lis      r3, sHamEffectHeight@ha
    addi     r4, r3, sHamEffectHeight@l
    cmpwi    r31, 0x4
    lfs      f0, 0xc8(r5)
    lis      r3, sHamDefaultRate@ha
    lfs      f1, 0x0(r4)
    stfs     f0, 0x44(r1)
    lfs      f0, sHamDefaultRate@l(r3)
    lfs      f2, 0xcc(r5)
    stfs     f2, 0x48(r1)
    fadds    f1, f2, f1
    lfs      f2, 0xd0(r5)
    stfs     f2, 0x4c(r1)
    stfs     f1, 0x48(r1)
    stfs     f0, 0x58(r1)
    stfs     f0, 0x54(r1)
    stfs     f0, 0x50(r1)
    bge      .L_000040D4
    cmpw     r0, r31
    bge      .L_000040D4
    lis      r4, sHamCookLevelRotation@ha
    lis      r3, gGame@ha
    addi     r7, r4, sHamCookLevelRotation@l
    addi     r6, r1, 0x44
    lwz      r4, 0x0(r7)
    addi     r3, r3, gGame@l
    lwz      r5, 0x4(r7)
    addis    r3, r3, 0x1
    lwz      r0, 0x8(r7)
    addi     r7, r1, 0x50
    stw      r4, 0x38(r1)
    addi     r8, r1, 0x38
    li       r4, 0x3a
    li       r9, 0x1
    stw      r5, 0x3c(r1)
    li       r5, 0x1
    subi     r3, r3, 0x6eec
    stw      r0, 0x40(r1)
    bl       fn_801A8BB0
.L_000040D4:
    cmpwi    r31, 0x4
    bne      .L_00004170
    lwz      r4, 0x24(r30)
    lis      r3, 0xb60b
    addi     r0, r3, 0x60b7
    slwi     r3, r4, 2
    add      r3, r30, r3
    lwz      r4, 0x28(r3)
    mulhw    r0, r0, r4
    add      r0, r0, r4
    srawi    r0, r0, 7
    srwi     r3, r0, 31
    add      r0, r0, r3
    mulli    r0, r0, 0xb4
    subf.    r0, r0, r4
    bne      .L_00004208
    lis      r3, gGame@ha
    li       r4, 0x3b
    addi     r3, r3, gGame@l
    addis    r29, r3, 0x1
    subi     r29, r29, 0x6eec
    mr       r3, r29
    bl       fn_801A8A10
    lis      r4, sHamOvercookRotation@ha
    lwzu     r5, sHamOvercookRotation@l(r4)
    mr       r3, r29
    addi     r6, r1, 0x44
    lwz      r10, 0x4(r4)
    addi     r7, r1, 0x50
    lwz      r0, 0x8(r4)
    addi     r8, r1, 0x2c
    stw      r5, 0x2c(r1)
    li       r4, 0x3b
    li       r5, 0x1
    li       r9, 0x1
    stw      r10, 0x30(r1)
    stw      r0, 0x34(r1)
    bl       fn_801A8BB0
    b        .L_00004208
.L_00004170:
    cmpwi    r31, 0x5
    blt      .L_00004208
    lwz      r4, 0x24(r30)
    lis      r3, 0x8889
    subi     r0, r3, 0x7777
    slwi     r3, r4, 2
    add      r3, r30, r3
    lwz      r4, 0x28(r3)
    mulhw    r0, r0, r4
    add      r0, r0, r4
    srawi    r0, r0, 6
    srwi     r3, r0, 31
    add      r0, r0, r3
    mulli    r0, r0, 0x78
    subf.    r0, r0, r4
    bne      .L_00004208
    lis      r3, gGame@ha
    li       r4, 0x3b
    addi     r3, r3, gGame@l
    addis    r29, r3, 0x1
    subi     r29, r29, 0x6eec
    mr       r3, r29
    bl       fn_801A8A10
    lis      r4, sHamBurnRotation@ha
    lwzu     r5, sHamBurnRotation@l(r4)
    mr       r3, r29
    addi     r6, r1, 0x44
    lwz      r10, 0x4(r4)
    addi     r7, r1, 0x50
    lwz      r0, 0x8(r4)
    addi     r8, r1, 0x20
    stw      r5, 0x20(r1)
    li       r4, 0x3b
    li       r5, 0x1
    li       r9, 0x1
    stw      r10, 0x24(r1)
    stw      r0, 0x28(r1)
    bl       fn_801A8BB0
.L_00004208:
    lis      r3, yakiLoopLevel@ha
    lwzu     r0, yakiLoopLevel@l(r3)
    cmplwi   r0, 0x1
    bge      .L_00004220
    li       r0, 0x1
    stw      r0, 0x0(r3)
.L_00004220:
    cmpwi    r31, 0x4
    blt      .L_00004234
    lis      r3, yakiLoopLevel@ha
    li       r0, 0x2
    stw      r0, yakiLoopLevel@l(r3)
.L_00004234:
    lwz      r0, 0x3c(r30)
    cmplwi   r0, 0x0
    bne      .L_000042C8
    lwz      r3, 0x4(r30)
    lis      r4, sHamCookingJoint@ha
    lwz      r0, sHamCookingJoint@l(r4)
    lwz      r6, 0x0(r3)
    cmplwi   r6, 0x0
    bne      .L_00004260
    li       r6, 0x0
    b        .L_000042A4
.L_00004260:
    bne      .L_0000426C
    li       r6, 0x0
    b        .L_00004270
.L_0000426C:
    lwz      r6, 0x10(r6)
.L_00004270:
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_000042A4
.L_0000427C:
    cmplwi   r6, 0x0
    bne      .L_0000428C
    li       r6, 0x0
    b        .L_00004290
.L_0000428C:
    lwz      r6, 0x8(r6)
.L_00004290:
    cmplwi   r6, 0x0
    bne      .L_000042A0
    li       r6, 0x0
    b        .L_000042A4
.L_000042A0:
    bdnz     .L_0000427C
.L_000042A4:
    lis      r3, gGame@ha
    li       r4, 0x78
    addi     r3, r3, gGame@l
    li       r5, 0x0
    addis    r3, r3, 0x1
    li       r7, 0x1
    subi     r3, r3, 0x6eec
    bl       fn_801A8A84
    stw      r3, 0x3c(r30)
.L_000042C8:
    addi     r3, r30, 0x40
    bl       __ptmf_test
    cmpwi    r3, 0x0
    beq      .L_000042EC
    mr       r3, r30
    addi     r12, r30, 0x40
    bl       __ptmf_scall
    nop
    b        .L_0000465C
.L_000042EC:
    lbz      r0, 0x38(r30)
    cmplwi   r0, 0x0
    beq      .L_0000465C
    lwz      r0, 0x34(r30)
    cmplwi   r0, 0x0
    bne      .L_0000465C
    lwz      r5, 0x4(r30)
    lis      r4, sHamGasPlateCenterX@ha
    lis      r3, sHamGasPlateCenterZ@ha
    lfs      f3, sHamGasPlateCenterX@l(r4)
    lfs      f1, 0xc8(r5)
    lfs      f2, sHamGasPlateCenterZ@l(r3)
    lfs      f0, 0xd0(r5)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    lwz      r3, 0x4(r30)
    lis      r4, sHamGasPlateMinX@ha
    frsp     f31, f1
    lfs      f0, sHamGasPlateMinX@l(r4)
    lfs      f2, 0xc8(r3)
    lfs      f1, 0xd0(r3)
    fcmpo    cr0, f2, f0
    blt      .L_0000437C
    lis      r4, sHamGasPlateMaxX@ha
    lfs      f0, sHamGasPlateMaxX@l(r4)
    fcmpo    cr0, f0, f2
    blt      .L_0000437C
    lis      r4, sHamGasPlateMinZ@ha
    lfs      f0, sHamGasPlateMinZ@l(r4)
    fcmpo    cr0, f1, f0
    blt      .L_0000437C
    lis      r4, sHamGasPlateMaxZ@ha
    lfs      f0, sHamGasPlateMaxZ@l(r4)
    fcmpo    cr0, f0, f1
    bge      .L_00004384
.L_0000437C:
    li       r0, 0x0
    b        .L_00004388
.L_00004384:
    li       r0, 0x1
.L_00004388:
    clrlwi.  r0, r0, 24
    bne      .L_000043FC
    lis      r4, sHamFlipComparison@ha
    addi     r3, r30, 0x40
    addi     r4, r4, sHamFlipComparison@l
    bl       __ptmf_cmpr
    cmpwi    r3, 0x0
    beq      .L_000043F0
    lis      r3, sHamReturnSlideSpeed@ha
    lis      r4, sHamReturnSlideStep@ha
    addi     r5, r3, sHamReturnSlideSpeed@l
    stfs     f31, 0x18(r30)
    lfs      f1, 0x0(r5)
    lis      r3, sHamSlideState@ha
    lfs      f0, sHamReturnSlideStep@l(r4)
    addi     r5, r3, sHamSlideState@l
    stfs     f1, 0x1c(r30)
    li       r0, 0x0
    stfs     f0, 0x20(r30)
    lwz      r4, 0x0(r5)
    lwz      r3, 0x4(r5)
    stw      r4, 0x40(r30)
    stw      r3, 0x44(r30)
    lwz      r3, 0x8(r5)
    stw      r3, 0x48(r30)
    stw      r0, 0x4c(r30)
.L_000043F0:
    li       r0, 0x0
    stw      r0, 0x30(r30)
    b        .L_000045F4
.L_000043FC:
    lwz      r0, 0x30(r30)
    cmpwi    r0, 0x0
    bne      .L_000044A8
    lis      r4, playerOffGasPlate@ha
    lbz      r0, playerOffGasPlate@l(r4)
    cmplwi   r0, 0x0
    beq      .L_000044A8
    lis      r5, gGame@ha
    addi     r4, r3, 0xc8
    addi     r3, r5, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addi     r3, r3, 0xc8
    bl       PSVECDistance
    lis      r3, sHamStackDetachDistance@ha
    lfs      f0, sHamStackDetachDistance@l(r3)
    fcmpo    cr0, f1, f0
    bge      .L_000045F4
    lis      r4, sHamFlipComparison@ha
    addi     r3, r30, 0x40
    addi     r4, r4, sHamFlipComparison@l
    bl       __ptmf_cmpr
    cmpwi    r3, 0x0
    beq      .L_000045F4
    lis      r3, sHamReturnSlideSpeed@ha
    lis      r4, sHamReturnSlideStep@ha
    addi     r5, r3, sHamReturnSlideSpeed@l
    stfs     f31, 0x18(r30)
    lfs      f1, 0x0(r5)
    lis      r3, sHamSlideState@ha
    lfs      f0, sHamReturnSlideStep@l(r4)
    addi     r5, r3, sHamSlideState@l
    stfs     f1, 0x1c(r30)
    li       r0, 0x0
    stfs     f0, 0x20(r30)
    lwz      r4, 0x0(r5)
    lwz      r3, 0x4(r5)
    stw      r4, 0x40(r30)
    stw      r3, 0x44(r30)
    lwz      r3, 0x8(r5)
    stw      r3, 0x48(r30)
    stw      r0, 0x4c(r30)
    b        .L_000045F4
.L_000044A8:
    lwz      r8, 0x238(r3)
    lis      r7, 0x4330
    lis      r4, sHamRatePercent@ha
    lis      r5, sHamRateSignedBias@ha
    lwz      r0, 0x8(r8)
    addi     r6, r4, sHamRatePercent@l
    addi     r9, r5, sHamRateSignedBias@l
    lwz      r5, 0x4(r8)
    xoris    r4, r0, 0x8000
    lwz      r0, 0xc(r8)
    stw      r4, 0x6c(r1)
    xoris    r8, r5, 0x8000
    xoris    r5, r0, 0x8000
    lis      r4, sHamPlacementPosition@ha
    stw      r7, 0x68(r1)
    addi     r10, r4, sHamPlacementPosition@l
    lfd      f5, 0x0(r9)
    lis      r4, sHamStackLevelHeight@ha
    lfd      f0, 0x68(r1)
    stw      r8, 0x74(r1)
    lfs      f4, 0x0(r6)
    fsubs    f0, f0, f5
    stw      r7, 0x70(r1)
    lwz      r6, 0x4(r10)
    lfd      f1, 0x70(r1)
    fdivs    f0, f0, f4
    stw      r5, 0x64(r1)
    lwz      r0, 0x30(r30)
    stw      r7, 0x60(r1)
    lwz      r8, 0x0(r10)
    xoris    r0, r0, 0x8000
    fsubs    f2, f1, f5
    lfd      f1, 0x60(r1)
    stw      r6, 0xc(r1)
    lwz      r5, 0x8(r10)
    fsubs    f1, f1, f5
    fdivs    f2, f2, f4
    stfs     f0, 0xc(r1)
    lfs      f0, 0xcc(r3)
    stw      r8, 0x8(r1)
    lwz      r3, 0xc(r1)
    stw      r0, 0x7c(r1)
    fdivs    f4, f1, f4
    stw      r3, 0x18(r1)
    lfs      f3, sHamStackLevelHeight@l(r4)
    stfs     f2, 0x8(r1)
    lfs      f1, 0x18(r1)
    stw      r7, 0x78(r1)
    lfd      f2, 0x78(r1)
    stw      r5, 0x10(r1)
    fsubs    f2, f2, f5
    lwz      r3, 0x8(r1)
    stfs     f4, 0x10(r1)
    fmadds   f1, f3, f2, f1
    lwz      r0, 0x10(r1)
    stw      r3, 0x14(r1)
    fcmpo    cr0, f1, f0
    stw      r0, 0x1c(r1)
    bge      .L_000045F4
    lis      r4, sHamFlipComparison@ha
    addi     r3, r30, 0x40
    addi     r4, r4, sHamFlipComparison@l
    bl       __ptmf_cmpr
    cmpwi    r3, 0x0
    beq      .L_000045F4
    lis      r3, sHamZero@ha
    lis      r4, sHamSlideHalf@ha
    addi     r5, r3, sHamZero@l
    stfs     f31, 0x18(r30)
    lfs      f1, 0x0(r5)
    lis      r3, sHamSlideState@ha
    lfs      f0, sHamSlideHalf@l(r4)
    addi     r5, r3, sHamSlideState@l
    stfs     f1, 0x1c(r30)
    li       r0, 0x0
    stfs     f0, 0x20(r30)
    lwz      r4, 0x0(r5)
    lwz      r3, 0x4(r5)
    stw      r4, 0x40(r30)
    stw      r3, 0x44(r30)
    lwz      r3, 0x8(r5)
    stw      r3, 0x48(r30)
    stw      r0, 0x4c(r30)
.L_000045F4:
    lis      r3, putDownActive@ha
    lbz      r0, putDownActive@l(r3)
    cmplwi   r0, 0x0
    bne      .L_0000465C
    lwz      r6, 0x4(r30)
    li       r4, 0x0
    li       r0, 0x1
    lwz      r3, 0x228(r6)
    lwz      r5, 0x22c(r6)
    and      r0, r3, r0
    and      r3, r5, r4
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    bne      .L_0000465C
    lwz      r4, 0x0(r30)
    cmplwi   r4, 0x0
    beq      .L_00004650
    beq      .L_0000465C
    lwz      r3, 0x30(r30)
    lwz      r0, 0x30(r4)
    cmpw     r3, r0
    bne      .L_0000465C
.L_00004650:
    lwz      r0, 0x228(r6)
    ori      r0, r0, 0x1
    stw      r0, 0x228(r6)
.L_0000465C:
    lbz      r0, 0x38(r30)
    cmplwi   r0, 0x0
    beq      .L_000046B8
    lwz      r3, 0x4(r30)
    lis      r4, sHamRespawnY@ha
    lfs      f0, sHamRespawnY@l(r4)
    lfs      f1, 0xcc(r3)
    fcmpo    cr0, f1, f0
    bge      .L_000046B8
    lwz      r12, 0x34(r3)
    lis      r4, sHamResetHeight@ha
    lfs      f1, sHamResetHeight@l(r4)
    li       r4, 0x0
    lwz      r12, 0x10(r12)
    mtctr    r12
    bctrl
    lwz      r3, 0x4(r30)
    li       r4, 0x0
    lfs      f1, 0x10(r30)
    lwz      r12, 0x34(r3)
    lwz      r12, 0x2c(r12)
    mtctr    r12
    bctrl
.L_000046B8:
    psq_l    f31, 0x98(r1), 0, qr0
    lwz      r0, 0xa4(r1)
    lfd      f31, 0x90(r1)
    lwz      r31, 0x8c(r1)
    lwz      r30, 0x88(r1)
    lwz      r29, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0xa0
    blr
*/

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

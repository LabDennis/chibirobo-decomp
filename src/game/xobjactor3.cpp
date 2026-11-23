#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/animframe.h"
#include <stdlib.h>
#include "game/globals.h"
#include "game/infoscrn.h"
#include "game/xobjspiderboss.h"
#include "game/xobjspider.h"
#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/cgameparticle.h"
#include "game/cgs.h"
#include "game/cpad.h"
#include "game/cscrn.h"
#include "game/stagedata.h"
#include "game/ctpl.h"
#include "game/ut.h"
#include "game/gmath.h"
#include "game/video.h"
#include "game/placement.h"
#include <sysdolphin/rumble.h>

// XObjActor3, the actor most of the stage's small moving things are: which one it is comes from the stage table,
// see GetKind and GetObjFlags. Its destructor, script commands, stage02 panels, update and per-kind functions
// NONMATCHING: besides the functions marked below, the data differs (US addresses). A trial link as Matching puts the code
// at retail's addresses but Kind9 is 0x30 shorter and Kind13 0x10 longer than retail. .rodata (0x8020B3D0) equals retail
// up to Kind11's literals; retail then has three Vecs no code reads ({1,1,1} and two {0,0,0}, 0x8020B4F0) in front of
// Kind13's and three more {0,0,0} among them (0x8020B544, 0x8020B550, 0x8020B580), and its strings add the "CPtclOne" type
// name and the jobj.h assert string. .sdata2 lacks the 40.0f, 0.0f and "scale" of probably a weak scale setter behind the
// type names (0x8065E1C8-0x8065E1E5); its leading 0x80 and 4 (fbeffect.h's colour setters) go, the linker drops them.
// After the jump tables (0x8022C6B0) .data and .sdata hold another set of weak vtables, base lists and type records than
// retail

// retail's .data for this file starts with these tables, then come the handler table's member pointer
// constants and the table
extern "C" {
// nothing in the game reads it, its meaning is unknown
s32 lbl_8022B758[26] = {
	0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 3, 0,
};

// the haisui nezumi's anim sounds, ResetActor3Fields hands them to the actor
AnimSfxEvent gHaisuiNezumiAnimSfxEvents[] = {
	{ 3, 11.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ 3, 53.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ 8, 11.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ 8, 53.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ 4, 1.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ 4, 11.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ -1 },
};

#if !defined(VERSION_JP)
// the four offsets the countdown label's outline is drawn at
f32 gStage02TimerOutlineX[4] = { -2.0f, 2.0f, 2.0f, -2.0f };
f32 gStage02TimerOutlineY[4] = { -2.0f, -2.0f, 2.0f, 2.0f };
#endif

Vec gActor3PlacementPosition;
Vec gActor3PlacementRotation;

s32 gSpiderBossVolleySlot;
s32 gActor3Kind15ActivationCounter;
}

// built at startup, member function pointers can't be plain data
XObjActor3Func gActor3UpdateHandlers[20] = {
	&XObjActor3::Kind0,
	&XObjActor3::UpdateObjectGauge,
	&XObjActor3::Kind2,
	&XObjActor3::Kind3,
	&XObjActor3::Kind4,
	&XObjActor3::Kind5,
	&XObjActor3::Kind6,
	&XObjActor3::Kind7,
	&XObjActor3::Kind8,
	&XObjActor3::Kind9,
	&XObjActor3::Kind10,
	&XObjActor3::Kind11,
	&XObjActor3::Kind12,
	&XObjActor3::Kind13,
	&XObjActor3::UpdateSpiderCounterPanel,
	&XObjActor3::Kind15,
	&XObjActor3::Kind16,
	&XObjActor3::Kind17,
	&XObjActor3::Kind18,
	&XObjActor3::Kind19,
};

extern "C" {
BOOL HSD_AudioSFXCheck(s32 id);
void HSD_AudioSFXKeyOff(s32 id);
CVec GetStagePlacementScale(XObj* obj);
void HSD_MtxGetScale(Mtx mat, Vec* vec);
void OSReport(const char* fmt, ...);
u16 GXGetTexObjWidth(const GXTexObj* obj);
u16 GXGetTexObjHeight(const GXTexObj* obj);
BOOL IsPlayerFlashingFromHit(XObj* obj);
void HideScrapCounter(void* mgr);
void ShowScrapCounter(void* mgr);
void HideMoolahCounter(void* mgr);
void ShowMoolahCounter(void* mgr);
void HideHappyPointCounter(void* mgr);
void ShowHappyPointCounter(void* mgr);
void PSMTXRotRad(Mtx m, char axis, f32 rad);
void InitParticlePool(CPtcl* pool, void* particles, s32 count, s32 stride);

// defined further down: the file is written in the reverse of retail's code order
void HandleKind9ActorCommand(XObjActor3* actor);
void HandleActorRiderCommand(XObjActor3* actor);
void DrawStage02TimerPanel(XObjActor3* actor);
void DrawStage02ActorCounterPanel(XObjActor3* actor);
void HandleSpiderCounterCommand(XObjActor3* actor);
void CarryRiderAtActorJoint(XObjActor3* actor, u32 frames);
void LaunchSpiderBossPart(XObjActor3* actor);
void ResetActor3Fields(XObjActor3* actor);
}

// hides the items of the kind, the 0x40B ones only
static inline void HideItems(XObjActor3* actor)
{
	s32 kind = -1;
	if (actor->mActorFlags.Check(0x100)) { // a 0x40B item is on show
		kind = 0x40B;
		actor->mActorFlags.mFlags &= ~0x100;
	}
	if (kind >= 0) {
		switch (kind) {
		case 0x40B:
			for (s32 i = 0; i < 0x120; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (kind == obj->GetKind()) {
					obj->mStatus.mFlags &= ~0x10000ULL;
				}
			}
			break;
		}
	}
}

static inline void UpdateActorAnimationSound(XObjActor3* actor, s32 id)
{
	f32 frame = GetAnimFrame(actor->mJObj);
	actor->mAnimSfx.Update(id, frame, GetSoundVolumeAtPosition(&gGame.mSound, &actor->mMoveTranslate, 0));
}

// a texture's size on screen at the scale
static inline f32 TexSize(f32 size, f32 scale)
{
	return scale * size;
}

#if !defined(VERSION_JP)
static inline void DrawTimerLabel(f32 scale)
{
	// retail keeps a second CTexObj scoped to the label drawing (vtable stored
	// before the label calls, destroyed right after); it is never read
	CTexObj labelTex;
	const f32 size = 20.0f;
	f32 width      = scale * (MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x5B), TRUE, size) - 4.0f);
	DrawGsQuad(&gGame.mGs, 0xFF, 0xFF, 0xFF, 0xFF, 320.0f, 40.0f, 0.0f, width, 12.0f, 0.0f, 0.0f, 0.0f);
	f32 left = 320.0f - MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x5B), TRUE, size) / 2.0f;
	for (int i = 0; i < 4; ++i)
		DrawGsProportionalTextFormat(&gGame.mGs, 0xFF, GetLocalizedInterfaceText(0x5B), left + gStage02TimerOutlineX[i],
		                             27.0f + gStage02TimerOutlineY[i], 0.0f, size, size);
	DrawGsProportionalTextFormat(&gGame.mGs, -1, GetLocalizedInterfaceText(0x5B), left, 27.0f, 0.0f, size, size);
}
#endif

// the robot is out of reach, past the front edge or in one of its climbing anims
static inline bool IsPlayerAway(XObjChr* player)
{
	Vec pos = player->mMoveTranslate;
	if (pos.z < -4.28f) {
		return true;
	}
	switch (player->mAnimId) {
	case 0x66:
	case 0x67:
	case 0x68:
	case 0x69:
	case 0x6A:
	case 0x6B:
	case 0x6C:
	case 0x6D:
	case 0x6E:
	case 0x6F:
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
		return true;
	}
	return false;
}

// the anims the robot reaches further in
static inline bool IsLongReachAnim(s32 anim)
{
	switch (anim) {
	case 0x4:
	case 0xD:
	case 0x35:
	case 0x79:
	case 0x89:
	case 0x10D:
	case 0x819:
	case 0x898:
		return true;
	}
	return false;
}

// the robot's anims it can't be talked to in
static inline bool IsBusyAnim(s32 anim)
{
	switch (anim) {
	case 0x5:
	case 0x6:
	case 0x7:
	case 0x8:
	case 0x9:
	case 0xA:
	case 0xB:
	case 0x27:
	case 0x28:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x43:
	case 0x97:
	case 0xA1:
	case 0xED:
	case 0xEE:
	case 0xF6:
	case 0x814:
	case 0x816:
	case 0x817:
	case 0x81F:
	case 0x820:
	case 0x824:
	case 0x879:
	case 0x87A:
	case 0x8AE:
	case 0x8AF:
	case 0x8B7:
	case 0x8C0:
	case 0x8C1:
	case 0x8C2:
	case 0x8C3:
	case 0x8C4:
	case 0x8C5:
		return true;
	}
	return false;
}

extern "C" bool IsPlayerWithinActorReach(XObjActor3*);
extern "C" BOOL IsUsingMug(XActLift*);
extern "C" f32 GetPlayerCollisionRadius(XObjChr*);

static inline void EmitKind9Dust(XObjActor3* actor);

static inline void StunKind9Actor(XObjActor3* actor)
{
	actor->SetAnim(11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
	PlayGameSound(&gGame.mSound, 0xb0004, 255);
	actor->mActorFlags.mFlags |= 0x200;
	actor->mKindStep = 20;
}

static inline BOOL Kind9AnimFinished(XObjActor3* actor, u32 frames)
{
	f32 end     = GetAnimEndFrame(actor->mJObj);
	f32 current = GetAnimFrame(actor->mJObj);
	return end - frames <= current;
}

// the spider boss, the only object in pool 19
static inline XObjSpiderBoss* GetSpiderBoss()
{
	return (XObjSpiderBoss*)gGame.mXObjs.mPools[19].mObjects;
}

// Each impact has its own entry in the original action table.
static inline void BounceBossPart(XObjActor3* part, f32 speed)
{
	part->mKindTimer = speed;
	++part->mFrameCount;
}

extern "C" bool IsSpiderBossProjectileHitSuppressed(XObjSpiderBoss* boss);

static inline void PlaceBossProjectile(XObjActor3* actor, XObjSpiderBoss* boss, int spread)
{
	actor->mMoveTranslate.x = HSD_JObjGetTranslationX(boss->mJObj);
	f32 base                = HSD_JObjGetTranslationY(boss->mJObj);
	actor->mMoveTranslate.y = 16 + base + rand() % spread;
	if (gSpiderBossVolleySlot == 0)
		actor->mMoveTranslate.y = HSD_JObjGetTranslationY(boss->mJObj);
	actor->mMoveTranslate.z = HSD_JObjGetTranslationZ(boss->mJObj);
	actor->SetPos(actor->mMoveTranslate, 0);
	actor->mStatus.mFlags |= 0x10000;
	actor->mStatus.mFlags |= 4;
}

static inline void AimBossVolley(XObjActor3* actor, const Vec& center, f32 radius)
{
	actor->mStatus.mFlags |= 0x10000;
	actor->mTimerFadeMode = gSpiderBossVolleySlot;
	if (gSpiderBossVolleySlot == 0) {
		actor->mOrbitX      = center.x;
		actor->mOrbitHeight = center.y;
		actor->mOrbitZ      = center.z;
	} else {
		actor->mOrbitX      = center.x + radius * (f32)sin(TWO_PI * (gSpiderBossVolleySlot - 1) / 9);
		actor->mOrbitHeight = center.y;
		actor->mOrbitZ      = center.z + radius * (f32)cos(TWO_PI * (gSpiderBossVolleySlot - 1) / 9);
	}
	++gSpiderBossVolleySlot;
	++actor->mKindStep;
}

static inline bool BossProjectileLanded(XObjActor3* actor, u32 frames)
{
	actor->mKindTimer -= .09f * frames;
	actor->mMoveTranslate.y += actor->mKindTimer;
	return actor->mMoveTranslate.y <= -1;
}

static inline void HideBossProjectile(XObjActor3* actor, bool resetPitch)
{
	actor->mStatus.mFlags &= ~0x10000ULL;
	actor->mStatus.mFlags &= ~4ULL;
	actor->mActorFlags.mFlags |= 4;
	actor->SetAnim(0, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
	if (resetPitch)
		actor->mMoveRotate.x = 0;
	actor->mKindStep = 0;
}

static inline void ResetBossAttachment(XObjActor3* obj)
{
	obj->mKindStep = 0;
	obj->SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	obj->mActorFlags.mFlags |= 4;
	obj->mActorFlags.mFlags |= 0x10;
	obj->SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	obj->mActorFlags.mFlags &= ~4;
	obj->mActorFlags.mFlags &= ~0x10;
	obj->SetScale(0.1f, 0.1f, 0.1f, 0);
	obj->mKindStep = 0;
}

/**
 * @note Address: 0x8011529C
 * @note Size: 0x4
 */
void XObjActor3::Kind0(u32 frames)
{
}

/**
 * @note Address: 0x801151E0
 * @note Size: 0xBC
 */
void XObjActor3::UpdateObjectGauge(u32 frames)
{
	switch (mEventCommand) {
	case 0:
		break;
	case 1:
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, gGame.mXObjs.mPools[5].Get(mPoolIndex), 1, 1);
		mEventCommand = 0;
		break;
	case 2:
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, gGame.mXObjs.mPools[5].Get(mPoolIndex), 0, 1);
		mEventCommand = 0;
		break;
	}
}

/**
 * @note Address: 0x80114CC8
 * @note Size: 0x518
 */
// a clock's minute hand, it turns twelve times as fast as Kind3's hour hand; kind 0x411 rides a joint of clock 0x2A1
// NONMATCHING: retail saves r25 too, the joint reads in Attach go r27, r26, r25, r27 and the hand's joint and matrix r25, r26; here
// r27, r26, r27, r26 and r26, r27
void XObjActor3::Kind2(u32)
{
	CJObj joint;
	f32 time = gGame.mTimeState.mTime;
	switch (GetKind()) {
	case 0x411: {
		for (s32 i = 0; i < 0x120; i++) {
			XObj* clock = gGame.mXObjs.mPools[22].Get(i);
			if (clock->GetKind() == 0x2A1) {
				s32 path[] = { 0, 0, 3, 0 };
				joint.Attach(GetJointFromRoot(clock->mJObj, 4, path));
				Mtx mount, translate, rotation, result;
				PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), mount);
				PSMTXRotRad(rotation, 'z', PI * (2.0f * -(12.0f * time)));
				PSMTXConcat(mount, rotation, result);
				PSMTXTrans(translate, 0.0f, 0.0f, 1.0f);
				PSMTXConcat(result, translate, result);
				HSD_JObj* jobj = mJObj;
				MtxPtr mp      = result;
				HSD_JObjCopyMtx(jobj, mp);
				return;
			}
		}
		break;
	}
	case 0x413:
		mMoveRotate.z = PI * (2.0f * -(12.0f * time));
		break;
	case 0x435:
		mMoveRotate.x = PI * (2.0f * (12.0f * time));
		break;
	default:
		mMoveRotate.z = PI * (2.0f * (12.0f * time));
	}
}

/**
 * @note Address: 0x801147C0
 * @note Size: 0x508
 */
// a clock's hour hand, 2.617994 and 3.6651917 are 150 and 210 degrees; kind 0x412 rides a joint of clock 0x2A1
// NONMATCHING: the same registers as in Kind2
void XObjActor3::Kind3(u32)
{
	CJObj joint;
	f32 time = gGame.mTimeState.mTime;
	switch (GetKind()) {
	case 0x412: {
		for (s32 i = 0; i < 0x120; i++) {
			XObj* clock = gGame.mXObjs.mPools[22].Get(i);
			if (clock->GetKind() == 0x2A1) {
				s32 path[] = { 0, 0, 3, 0 };
				joint.Attach(GetJointFromRoot(clock->mJObj, 4, path));
				Mtx mount, translate, rotation, result;
				PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), mount);
				PSMTXRotRad(rotation, 'z', PI * (2.0f * -time) + 2.617994f);
				PSMTXConcat(mount, rotation, result);
				PSMTXTrans(translate, 0.0f, 0.0f, 1.0f);
				PSMTXConcat(result, translate, result);
				HSD_JObj* jobj = mJObj;
				MtxPtr mp      = result;
				HSD_JObjCopyMtx(jobj, mp);
				return;
			}
		}
		break;
	}
	case 0x414:
		mMoveRotate.z = PI * (2.0f * -time) + 2.617994f;
		break;
	case 0x436:
		mMoveRotate.x = PI * (2.0f * time) + 3.6651917f;
		break;
	default:
		mMoveRotate.z = PI * (2.0f * time) + 3.6651917f;
	}
}

/**
 * @note Address: 0x801146F8
 * @note Size: 0xC8
 */
void XObjActor3::Kind4(u32 frames)
{
	mFrameCount++;
	f32 wave  = sin(DEG2RAD * ((mFrameCount * 6) % 360));
	f32 swing = 0.25f;
	if (GetKind() == 0x437) {
		mMoveRotate.x = wave * swing;
	} else {
		mMoveRotate.z = wave * swing;
	}
}

/**
 * @note Address: 0x801146F4
 * @note Size: 0x4
 */
void XObjActor3::Kind5(u32 frames)
{
}

/**
 * @note Address: 0x8011463C
 * @note Size: 0xB8
 */
// NONMATCHING: register allocation, retail holds the item id in r29 and the count in r28 (US 0x8011468C); the ids
// declared in the loop body read the same, an InventoryItem& for the slot scores lower; swapping the declarations or the
// two assignments changes nothing
void XObjActor3::Kind6(u32 frames)
{
	s16 id;
	s16 num;
	s32 i;
	s32 count = 0;
	switch (mEventCommand) {
	case 0:
		break;
	case 1:
		for (i = 0; i < 0x100; i++) {
			InventoryItem* items = gGame.GetPlayer()->mParams.mInventory.mItems;
			id                   = items[i].GetId();
			num                  = items[i].GetCount();
			if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
				count++;
			}
		}
		gGame.mEvt.mSelectResult = count;
		mEventCommand            = 0;
		break;
	}
}

/**
 * @note Address: 0x80114540
 * @note Size: 0xFC
 */
void XObjActor3::Kind7(u32 frames)
{
	switch (mEventCommand) {
	case 0:
		break;
	case 1:
		if (mEventArg1 != 0) {
			ShowScrapCounter(&gGame.mInfoScrn);
		} else {
			HideScrapCounter(&gGame.mInfoScrn);
		}
		mEventCommand = 0;
		break;
	case 2:
		if (mEventArg1 != 0) {
			ShowHappyPointCounter(&gGame.mInfoScrn);
			ShowMoolahCounter(&gGame.mInfoScrn);
			ShowScrapCounter(&gGame.mInfoScrn);
		} else {
			HideHappyPointCounter(&gGame.mInfoScrn);
			HideMoolahCounter(&gGame.mInfoScrn);
			HideScrapCounter(&gGame.mInfoScrn);
		}
		mEventCommand = 0;
		break;
	}
}

/**
 * @note Address: 0x80114060
 * @note Size: 0x4E0
 */
// holds the 0x40B item at its first joint while flag 0x100 is up
// NONMATCHING: the loop and the joint's vectors take each other's registers
void XObjActor3::Kind8(u32 frames)
{
	Mtx m;
	Mtx t;
	Mtx a;
	CJObj joint;
	if (mActorFlags.mFlags & 0x100) {
		CVec pos;
		s32 path[1] = { 0 };
		joint.Attach(GetJointFromRoot(mJObj, 1, path));
		pos      = GetJointPosition(joint.mJObj);
		s32 kind = -1;
		if (mActorFlags.mFlags & 0x100) {
			kind = 0x40B;
		}
		if (kind >= 0) {
			for (s32 i = 0; i < 0x120; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (kind == obj->GetKind()) {
					PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), m);
					PSMTXTrans(t, 0.0f, -14.0f, 0.0f);
					PSMTXConcat(m, t, a);
					HSD_JObj* jobj = obj->mJObj;
					MtxPtr mp      = a;
					HSD_JObjCopyMtx(jobj, mp);
					if (mStatus.mFlags & 0x10000) {
						obj->mStatus.mFlags |= 0x10000ULL;
					} else {
						obj->mStatus.mFlags &= ~0x10000ULL;
					}
				}
			}
		}
	}
}

/**
 * @note Address: 0x80111A78
 * @note Size: 0x25E8
 */
// Kind9 runs haisui_nezumi (GetKind 0x31B): it paces its window, charges the robot when it comes near and grabs it or ends
// up stunned
// NONMATCHING: retail builds each effect's rotation in one temporary and stores 0 to its y after the copy from .rodata, which a
// rotation passed by value cannot give (a y that is not constant makes a second temporary here); the dust's saved float
// registers come out in another order and step 6 needs r23 for the joint's third read
void XObjActor3::Kind9(u32 frames)
{
	XObjChr* player = gGame.GetPlayer();
	Vec pos         = player->mMoveTranslate;
	CJObj joint;
	Vec p, scale;
	// the lunge window step 5 opened runs out while it paces
	if (mActorFlags.mFlags & 0x20) {
		switch (mKindStep) {
		case 1:
		case 2:
		case 3:
		case 4:
			if ((s32)--mExtraEventArgs[6] < 0)
				mActorFlags.mFlags &= ~0x20;
			break;
		}
	}
	// the robot stands on top of it
	if (mKindStep != 0 && mKindStep < 20 && Abs(pos.x - mMoveTranslate.x) <= 10 && pos.y >= 4 + mMoveTranslate.y
	    && pos.y <= 8 + mMoveTranslate.y)
		StunKind9Actor(this);
	switch (mKindStep) {
	case 0:
		if (!(mActorFlags.mFlags & 4)) {
			mFrameCount = 30;
			++mKindStep;
		}
		break;
	case 1: // stands, then walks off to a random side
		SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		if (--mFrameCount < 0) {
			mFrameCount = rand() % 120 + 120;
			if (rand() % 2 == 0)
				mHeading = -HALF_PI;
			else
				mHeading = HALF_PI;
			mKindStep = 2;
		}
		if (IsPlayerWithinActorReach(this))
			mKindStep = 5;
		break;
	case 2: // walks along its window, turning at the edges; with the lunge window open it rests once its walk time is up
		SetAnim(3, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		if (mHeading == -HALF_PI)
			mMoveTranslate.x -= 0.1f * frames;
		else
			mMoveTranslate.x += 0.1f * frames;
		if (mMoveTranslate.x < mWindowLeft) {
			mMoveTranslate.x = mWindowLeft;
			_B70             = mHeading;
			mKindStep        = 3;
			SetAnim(8, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		} else if (mMoveTranslate.x > mWindowRight) {
			mMoveTranslate.x = mWindowRight;
			_B70             = mHeading;
			mKindStep        = 3;
			SetAnim(8, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		}
		if (--mFrameCount < 0 && (mActorFlags.mFlags & 0x20)) {
			mKindStep   = 4;
			mFrameCount = 90;
			p.x         = mMoveTranslate.x;
			p.y         = 12.0f + mMoveTranslate.y;
			p.z         = 12.0f + mMoveTranslate.z;
			scale.x     = 3.0f;
			scale.y     = 3.0f;
			scale.z     = 3.0f;
			mGens[0]    = (s32)gGame.mParticle.CreateSRT(0x28, 1, p, scale, (Vec) { 0, 0, 0 }, TRUE);
		}
		if (IsPlayerWithinActorReach(this))
			mKindStep = 5;
		break;
	case 3: // turns round to face away from the edge it reached
		if (_B70 == HALF_PI) {
			mHeading -= 0.04f * frames;
			if (mHeading <= -HALF_PI) {
				mHeading  = -HALF_PI;
				mKindStep = 2;
			}
		} else {
			mHeading += 0.04f * frames;
			if (mHeading >= HALF_PI) {
				mHeading  = HALF_PI;
				mKindStep = 2;
			}
		}
		break;
	case 4:
		SetAnim(2, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		if (--mFrameCount < 0) {
			mFrameCount = rand() % 120 + 30;
			mKindStep   = rand() % 2 + 2;
			gGame.mParticle.Kill((HSD_Generator*)mGens[0]);
		}
		if (IsPlayerWithinActorReach(this)) {
			mKindStep = 5;
			gGame.mParticle.Kill((HSD_Generator*)mGens[0]);
		}
		break;
	case 5: // the robot came near: opens the 10-second lunge window and turns to it
#ifdef VERSION_PAL
		mExtraEventArgs[6] = (10 * gVideoFramesPerSecond) / frames;
#else
		mExtraEventArgs[6] = 600 / frames;
#endif
		mActorFlags.mFlags |= 0x20;
		p.x         = mMoveTranslate.x;
		p.y         = 12.0f + mMoveTranslate.y;
		p.z         = 12.0f + mMoveTranslate.z;
		scale.x     = 3.0f;
		scale.y     = 3.0f;
		scale.z     = 3.0f;
		mGens[0]    = (s32)gGame.mParticle.CreateSRT(0x26, 1, p, scale, (Vec) { 0, 0, 0 }, TRUE);
		mFrameCount = 15;
		SetAnim(5, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		if (mMoveTranslate.x < pos.x)
			mHeading = HALF_PI;
		else
			mHeading = -HALF_PI;
		PlayGameSound(&gGame.mSound, 0xb0005, 255);
		mKindStep = 6;
		break;
	case 6:
		if (Kind9AnimFinished(this, frames)) {
			gGame.mParticle.Kill((HSD_Generator*)mGens[0]);
			joint.Attach(FindJointByPath(mJObj, (s32[4]) { 0, 0, 1, 1 }, 4));
			p = joint.GetPos();
			SetAnim(7, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			if (mHeading == -HALF_PI)
				p.x += 2;
			else {
				p.x -= 2;
				p.z += 4;
			}
			scale.x   = 1.0f;
			scale.y   = 1.0f;
			scale.z   = 1.0f;
			mGens[0]  = (s32)gGame.mParticle.CreateSRT(0x84, 1, p, scale, (Vec) { 0, 0, 0 }, TRUE);
			mKindStep = 7;
		}
		break;
	case 7:
		if (Kind9AnimFinished(this, frames)) {
			SetAnim(4, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			gGame.mParticle.Kill((HSD_Generator*)mGens[0]);
			mFrameCount = 2;
			mKindStep   = 8;
		}
		break;
	case 8: // runs at the robot kicking up dust until it is within 32, rests at the window's edge
		if (mHeading == -HALF_PI)
			mMoveTranslate.x -= 0.6f * frames;
		else
			mMoveTranslate.x += 0.6f * frames;
		if (mMoveTranslate.x < mWindowLeft) {
			mMoveTranslate.x = mWindowLeft;
			mKindStep        = 4;
			mFrameCount      = 90;
			p.x              = mMoveTranslate.x;
			p.y              = 12.0f + mMoveTranslate.y;
			p.z              = 12.0f + mMoveTranslate.z;
			scale.x          = 3.0f;
			scale.y          = 3.0f;
			scale.z          = 3.0f;
			mGens[0]         = (s32)gGame.mParticle.CreateSRT(0x28, 1, p, scale, (Vec) { 0, 0, 0 }, TRUE);
		} else if (mMoveTranslate.x > mWindowRight) {
			mMoveTranslate.x = mWindowRight;
			mKindStep        = 4;
			mFrameCount      = 90;
			p.x              = mMoveTranslate.x;
			p.y              = 12.0f + mMoveTranslate.y;
			p.z              = 12.0f + mMoveTranslate.z;
			scale.x          = 3.0f;
			scale.y          = 3.0f;
			scale.z          = 3.0f;
			mGens[0]         = (s32)gGame.mParticle.CreateSRT(0x28, 1, p, scale, (Vec) { 0, 0, 0 }, TRUE);
		} else {
			f32 dx = pos.x - mMoveTranslate.x, dy = pos.y - mMoveTranslate.y, dz = mMoveTranslate.z - mMoveTranslate.z;
			if (sqrtf(dz * dz + (dx * dx + dy * dy)) < 32 && !IsPlayerAway(gGame.GetPlayer())
			    && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
				SetAnim(9, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				mKindStep = 9;
			} else if (--mFrameCount < 0) {
				mFrameCount = 2;
				EmitKind9Dust(this);
			}
		}
		break;
	case 9:
		if (Kind9AnimFinished(this, frames)) {
			SetAnim(10, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			mKindStep = 10;
			PlayGameSound(&gGame.mSound, 0xb0002, 255);
			// both sides set up an effect that is never started
			if (mHeading == -HALF_PI) {
				p.x     = mMoveTranslate.x;
				p.y     = 12.0f + mMoveTranslate.y;
				p.z     = 12.0f + mMoveTranslate.z;
				scale.x = 0.2f;
				scale.y = 0.2f;
				scale.z = 0.2f;
			} else {
				p.x     = mMoveTranslate.x;
				p.y     = 12.0f + mMoveTranslate.y;
				p.z     = 12.0f + mMoveTranslate.z;
				scale.x = 0.2f;
				scale.y = 0.2f;
				scale.z = 0.2f;
			}
		}
		break;
	case 10: { // charges: stunned at the window's edge or by the robot's mug, grabs the robot if it is free
		f32 radius = GetPlayerCollisionRadius(gGame.GetPlayer());
		f32 offset;
		if (mHeading == -HALF_PI) {
			mMoveTranslate.x -= 1.4f * (0.6f * frames);
			offset = radius < 2 ? 1.5f : 5.5f;
		} else {
			mMoveTranslate.x += 1.4f * (0.6f * frames);
			offset = radius < 2 ? -1.5f : -5.5f;
		}
		if (mMoveTranslate.x < mWindowLeft) {
			mMoveTranslate.x = mWindowLeft;
			StunKind9Actor(this);
		} else if (mMoveTranslate.x > mWindowRight) {
			mMoveTranslate.x = mWindowRight;
			StunKind9Actor(this);
		} else {
			f32 dx = pos.x + offset - mMoveTranslate.x, dy = pos.y - mMoveTranslate.y, dz = mMoveTranslate.z - mMoveTranslate.z;
			if (sqrtf(dz * dz + (dx * dx + dy * dy)) < 10 && !IsPlayerFlashingFromHit(player) && !IsPlayerAway(gGame.GetPlayer())) {
				CPad pad(0);
				if (radius < 2) { // the robot isn't holding up its mug
					SetAnim(11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
					if (!IsBusyAnim(gGame.GetPlayer()->mAnimId) && gGame.GetPlayer()->mAttachmentRequest < 0
					    && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
						mActorFlags.mFlags |= 0x80;
						gGame.mStatus.mFlags &= ~0x800000000000ULL; // the robot can't open its equipment rings
					} else {
						PlayGameSound(&gGame.mSound, 0xb0004, 255);
						mActorFlags.mFlags |= 0x200;
					}
					mKindStep = 20;
				} else if (IsUsingMug(&gGame.GetPlayer()->mLift))
					StunKind9Actor(this);
			}
		}
		break;
	}
	case 20:
		if (--mFrameCount < 0) {
			mFrameCount = 2;
			EmitKind9Dust(this);
		}
		if (Kind9AnimFinished(this, frames)) {
			mStatus.mFlags |= 0x100000;
			SetAnim(13, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
#ifdef VERSION_PAL
			mFrameCount = (3 * gVideoFramesPerSecond) / frames;
#else
			mFrameCount = 180 / frames;
#endif
			mKindStep = 21;
			if (mActorFlags.Check(0x80))
				mActorFlags.mFlags &= ~0x80;
			if (mActorFlags.mFlags & 0x200) {
				p.x      = mMoveTranslate.x;
				p.y      = 12.0f + mMoveTranslate.y;
				p.z      = 12.0f + mMoveTranslate.z;
				scale.x  = 1.0f;
				scale.y  = 1.0f;
				scale.z  = 1.0f;
				mGens[0] = (s32)gGame.mParticle.CreateSRT(5, 1, p, scale, (Vec) { 0, 0, 0 }, TRUE);
				mActorFlags.mFlags &= ~0x200;
			}
		}
		break;
	case 21: { // waits until no script runs, its time is up and the robot is 12 away
		if (gGame.mEvt.mData.mScript)
			break;
		f32 distance = Abs(pos.x - mMoveTranslate.x);
		if (--mFrameCount < 0 && distance >= 12) {
			mFrameCount = 0;
			SetAnim(12, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			mKindStep = 22;
			gGame.mParticle.Kill((HSD_Generator*)mGens[0]);
			mStatus.mFlags &= ~0x100000ULL;
		}
		break;
	}
	case 22:
		if (Kind9AnimFinished(this, frames)) {
			SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			mFrameCount = 0;
			mKindStep   = 1;
		}
		break;
	case 100:
		if (mFrameCount & 1)
			mStatus.mFlags |= 0x10000;
		else
			mStatus.mFlags &= ~0x10000ULL;
		if (--mFrameCount < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			++mKindStep;
		}
		break;
	case 101:
		break;
	}
}

/**
 * @note Address: 0x801115FC
 * @note Size: 0x47C
 */
// falls from above the robot, knocks it down on a hit, blinks out on the floor
void XObjActor3::Kind10(u32 frames)
{
	XObjChr* player = gGame.GetPlayer();
	Vec pos         = player->mMoveTranslate;
	switch (mKindStep) {
	case 0:
		if (mActorFlags.mFlags & 4) {
			break;
		}
		mMoveTranslate.x = pos.x - (rand() % 3600) / 100.0f + 18.0f;
		mMoveTranslate.y = 64.0f;
		mMoveTranslate.z = pos.z - (rand() % 3600) / 100.0f + 18.0f;
		mStatus.mFlags |= 0x10000;
		mStatus.mFlags |= 4;
		_B70        = 4.0f;
		mFrameCount = rand() % 30 + 30;
		mKindStep++;
		break;
	case 1:
		if (--mFrameCount < 0) {
			mKindStep++;
		}
		break;
	case 2:
		f32 dx   = pos.x - mMoveTranslate.x;
		f32 dy   = pos.y - mMoveTranslate.y;
		f32 dz   = pos.z - mMoveTranslate.z;
		f32 dist = sqrtf(dx * dx + dy * dy + dz * dz);
		if (dist < 8.0f && !IsPlayerFlashingFromHit(player) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
			player->OnHitFromPosition(pos, 10.0f);
			mFrameCount = 30;
			mKindStep   = 3;
			break;
		}
		if (_B70 >= -2.3f) {
			_B70 -= 0.36f * frames;
		} else {
			_B70 = -2.3f;
		}
		mMoveTranslate.y += _B70;
		if (mMoveTranslate.y < 0.0f) {
			mMoveTranslate.y = 0.0f;
			mFrameCount      = 30;
			mKindStep        = 3;
		}
		break;
	case 3:
		if ((mFrameCount & 3) < 2) {
			mStatus.mFlags |= 0x10000;
		} else {
			mStatus.mFlags &= ~0x10000ULL;
		}
		if (--mFrameCount < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			mStatus.mFlags &= ~4ULL;
			mActorFlags.mFlags |= 4;
			mKindStep = 0;
		}
		break;
	}
}

/**
 * @note Address: 0x8011132C
 * @note Size: 0x2D0
 */
// whether the robot stands where the actor can reach it
extern "C" bool IsPlayerWithinActorReach(XObjActor3* actor)
{
	XObjChr* player = gGame.GetPlayer();
	Vec pos         = player->mMoveTranslate;
	if (!(actor->mActorFlags.mFlags & 4)) {
		if (IsPlayerAway(gGame.GetPlayer())) {
			return false;
		}
		if (pos.y >= actor->mMoveTranslate.y - 16.0f && pos.y <= actor->mMoveTranslate.y + 16.0f) {
			if (actor->mHeading == HALF_PI) {
				if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mWindowRight) {
					return true;
				}
				if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mWindowLeft) {
					if (IsLongReachAnim(player->mAnimId)) {
						if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mMoveTranslate.x - 32.0f) {
							return true;
						}
					} else {
						if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mMoveTranslate.x - 12.0f) {
							return true;
						}
					}
				}
			} else {
				if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mWindowLeft) {
					return true;
				}
				if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mWindowRight) {
					if (IsLongReachAnim(player->mAnimId)) {
						if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mMoveTranslate.x + 32.0f) {
							return true;
						}
					} else {
						if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mMoveTranslate.x + 12.0f) {
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

/**
 * @note Address: 0x8010FF94
 * @note Size: 0x1398
 */
// the spider boss's parts: each rides the boss at its orbit offset until launched, then bounces to a stop and blinks out
// the boss's rotation is read with its position but nothing uses it; its zero initializer still takes its place in .rodata
// NONMATCHING: only the .rodata offsets differ, the unit's constants and strings are laid out differently from retail
void XObjActor3::Kind11(u32 frames)
{
	CPad pad(0);
	switch (mKindStep) {
	case 0: {
		if (mActorFlags.mFlags & 4)
			break;
		Vec origin           = { 0, 0, 0 };
		Vec rotation         = { 0, 0, 0 };
		XObjSpiderBoss* boss = GetSpiderBoss();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			origin.x   = HSD_JObjGetTranslationX(boss->mJObj);
			origin.y   = HSD_JObjGetTranslationY(boss->mJObj);
			origin.z   = HSD_JObjGetTranslationZ(boss->mJObj);
			rotation.x = HSD_JObjGetRotationX(boss->mJObj);
			rotation.y = HSD_JObjGetRotationY(boss->mJObj);
			rotation.z = HSD_JObjGetRotationZ(boss->mJObj);
		}
		switch (GetKind()) {
		case 0x3cd:
			mOrbitDistance = 16;
			mOrbitAngle    = -PI / 4;
			mOrbitHeight   = 10;
			break;
		case 0x3ce:
			mOrbitDistance = 16;
			mOrbitAngle    = PI / 4;
			mOrbitHeight   = 10;
			break;
		case 0x3cf:
			mOrbitDistance = 10;
			mOrbitAngle    = PI;
			mOrbitHeight   = 16;
			break;
		case 0x3d0:
			mOrbitDistance = 0;
			mOrbitAngle    = HALF_PI;
			mOrbitHeight   = 30;
			break;
		case 0x3d1:
			mOrbitDistance = 30;
			mOrbitAngle    = PI;
			mOrbitHeight   = 10;
			break;
		case 0x3d2:
			mOrbitDistance = 40;
			mOrbitAngle    = 3 * HALF_PI;
			mOrbitHeight   = 24;
			break;
		case 0x3d3:
			mOrbitDistance = 30;
			mOrbitAngle    = 3 * PI / 4;
			mOrbitHeight   = 14;
			break;
		case 0x3d4:
			mOrbitDistance = 20;
			mOrbitAngle    = 0;
			mOrbitHeight   = 10;
			break;
		case 0x3d5:
			mOrbitDistance = 16;
			mOrbitAngle    = 3 * HALF_PI;
			mOrbitHeight   = 10;
			break;
		default:
			mOrbitDistance = 32;
			mOrbitAngle    = DEG2RAD * (rand() % 360);
			mOrbitHeight   = (48.f - (rand() % 1800) / 100) / 2;
			break;
		}
		mOrbitX  = mOrbitDistance * (f32)sin(mOrbitAngle);
		mOrbitZ  = mOrbitDistance * (f32)cos(mOrbitAngle);
		mHeading = atan2(origin.x - mMoveTranslate.x, origin.z - mMoveTranslate.z);
		mStatus.mFlags |= 0x100000;
		mStatus.mFlags |= 4;
		mFrameCount = 0;
		++mKindStep;
		break;
	}
	case 1: {
		XObjSpiderBoss* boss = GetSpiderBoss();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			f32 x = mOrbitX, y = mOrbitHeight, z = mOrbitZ;
			SetScale(1, 1, 1, 0);
			CJObj joint;
			s32 path[] = { 0 };
			joint.Attach(GetJointFromRoot(boss->mJObj, 1, path));
			Mtx scaleMtx, translation, result, mount;
			PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), mount);
			Vec scale;
			HSD_MtxGetScale(mount, &scale);
			scale.x = GetStagePlacementScale(this).x / scale.x;
			scale.y = GetStagePlacementScale(this).y / scale.y;
			scale.z = GetStagePlacementScale(this).z / scale.z;
			PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
			PSMTXConcat(mount, scaleMtx, result);
			PSMTXTrans(translation, x, y, z);
			PSMTXConcat(result, translation, result);
			HSD_JObj* jobj = mJObj;
			MtxPtr matrix  = result;
			HSD_JObjCopyMtx(jobj, matrix);
			if (boss->mBlinkTimer) {
				if (boss->mBlinkTimer % 30 < 15)
					SetColor(1, 128, 0, 0, 0);
				else
					SetColor(1, 0, 0, 0, 0);
			}
		}
		break;
	}
	case 2: { // launched: falls along its heading and bounces, it stays down at the tenth landing
		SetColor(1, 0, 0, 0, 0);
		CVec velocity;
		mKindTimer -= .09f * frames;
		mMoveTranslate.y += mKindTimer;
		f32 floor = 0;
		switch (GetKind()) {
		case 0x3d2:
			floor = 8;
			break;
		case 0x3d3:
			floor = 16;
			break;
		}
		if (mMoveTranslate.y <= floor) {
			mMoveTranslate.y = floor;
			switch (mFrameCount) {
			case 0: {
				++mFrameCount;
				mKindTimer = 8;
				PlayGameSound(&gGame.mSound, 0x160020, 255);
				Vec p;
				p.x      = mMoveTranslate.x;
				p.y      = mMoveTranslate.y - floor;
				p.z      = mMoveTranslate.z;
				mGens[0] = (s32)gGame.mParticle.CreateSRT(0xb5, 1, p, (Vec) { 1, 1, 1 }, (Vec) { 0, 0, 0 }, TRUE);
				break;
			}
			case 1:
				mKindTimer = 7;
				++mFrameCount;
				PlayGameSound(&gGame.mSound, 0x160020, 255);
				gGame.mParticle.Kill((HSD_Generator*)mGens[0]);
				break;
			case 2:
				BounceBossPart(this, 6);
				break;
			case 3:
				BounceBossPart(this, 6);
				break;
			case 4:
				BounceBossPart(this, 5);
				break;
			case 5:
				BounceBossPart(this, 5);
				break;
			case 6:
				BounceBossPart(this, 4);
				break;
			case 7:
				BounceBossPart(this, 4);
				break;
			case 8:
				BounceBossPart(this, 4);
				break;
			case 9:
				mKindTimer  = 0;
				mKindStep   = 3;
				mFrameCount = 30;
				break;
			}
		}
		if (mKindTimer >= -3.4499998f)
			mKindTimer -= .36f * frames;
		else
			mKindTimer = -3.4499998f;
		velocity.x = .6f * (f32)sin(mHeading);
		velocity.z = .6f * (f32)cos(mHeading);
		velocity.y = 0;
		mMoveTranslate.SetNow(mMoveTranslate + velocity * (f32)frames);
		SetPos(mMoveTranslate, 0);
		break;
	}
	case 3:
		if ((mFrameCount & 3) < 2)
			mStatus.mFlags |= 0x10000;
		else
			mStatus.mFlags &= ~0x10000ULL;
		if (--mFrameCount < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			mActorFlags.mFlags |= 4;
			mStatus.mFlags &= ~4ULL;
			mKindStep = 0;
		}
		break;
	case 5: {
		CVec velocity;
		Vec origin           = { 0, 0, 0 };
		Vec rotation         = { 0, 0, 0 };
		XObjSpiderBoss* boss = GetSpiderBoss();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			origin.x   = HSD_JObjGetTranslationX(boss->mJObj);
			origin.y   = HSD_JObjGetTranslationY(boss->mJObj);
			origin.z   = HSD_JObjGetTranslationZ(boss->mJObj);
			rotation.x = HSD_JObjGetRotationX(boss->mJObj);
			rotation.y = HSD_JObjGetRotationY(boss->mJObj);
			rotation.z = HSD_JObjGetRotationZ(boss->mJObj);
			mHeading   = atan2(boss->mMoveTranslate.x - mMoveTranslate.x, boss->mMoveTranslate.z - mMoveTranslate.z);
		}
		f32 dx = origin.x - mMoveTranslate.x, dy = origin.y - mMoveTranslate.y, dz = origin.z - mMoveTranslate.z;
		if (sqrtf(dx * dx + dy * dy + dz * dz) < 16)
			mKindStep = 6;
		velocity.x = .6f * (f32)sin(mHeading);
		velocity.z = .6f * (f32)cos(mHeading);
		velocity.y = 0;
		mMoveTranslate.SetNow(mMoveTranslate + velocity * (f32)frames);
		SetPos(mMoveTranslate, 0);
		break;
	}
	case 6: {
		Vec origin           = { 0, 0, 0 };
		Vec rotation         = { 0, 0, 0 };
		XObjSpiderBoss* boss = GetSpiderBoss();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			origin.x   = HSD_JObjGetTranslationX(boss->mJObj);
			origin.y   = HSD_JObjGetTranslationY(boss->mJObj);
			origin.z   = HSD_JObjGetTranslationZ(boss->mJObj);
			rotation.x = HSD_JObjGetRotationX(boss->mJObj);
			rotation.y = HSD_JObjGetRotationY(boss->mJObj);
			rotation.z = HSD_JObjGetRotationZ(boss->mJObj);
		}
		mOrbitAngle  = DEG2RAD * (rand() % 360);
		mOrbitX      = 32 * (f32)sin(mOrbitAngle);
		mOrbitHeight = (48.f - (rand() % 1800) / 100) / 2;
		mOrbitZ      = 32 * (f32)cos(mOrbitAngle);
		mHeading     = atan2(origin.x - mMoveTranslate.x, origin.z - mMoveTranslate.z);
		mStatus.mFlags |= 0x100000;
		mKindStep = 1;
		if (pad.mPad->mTrigger & 0x10) {
			mKindStep = 5;
			mStatus.mFlags |= 0x10000;
			mActorFlags.mFlags &= ~4;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8010FE68
 * @note Size: 0x12C
 */
extern "C" void EnableSpiderBossPartByIndex(s32 n)
{
	XObjActor3* obj;
	s32 offset;
	u8* data;
	s32 size;
	s32 count = 0;
	s32 i;
	size = gGame.mXObjs.mPools[5].mStride;
	data = gGame.mXObjs.mPools[5].mObjects;
	for (i = 0, offset = 0; i < 48; ++i, offset += size) {
		obj = (XObjActor3*)(data + offset);
		if (XObj::GetDataItem(obj->mAttr.mObjData) == 0xB) {
			if (count == n && (obj->mStatus.mFlags & 1)) {
				if (obj->mKindStep == 0) {
					obj->mActorFlags.mFlags &= ~4;
				}
				return;
			}
			count++;
		}
	}
}

/**
 * @note Address: 0x8010FDA0
 * @note Size: 0xC8
 */
// starts the n-th kind 0xB actor
// NONMATCHING: register allocation (retail loads the stride into r11 before the zero offset in r8, US 0x8010FDC0);
// a Get(i) loop and reading the pool through gGame at each use both score lower; `i * pool.mStride` is not strength-reduced
// (mullw, 91.1), a `s32 stride` local puts the loads in retail's order but the offset's li first (94.7)
extern "C" void LaunchSpiderBossPartByIndex(s32 n)
{
	XObjActor3* obj;
	s32 offset;
	XObjPool& pool = gGame.mXObjs.mPools[5];
	s32 count      = 0;
	s32 i;
	for (i = 0, offset = 0; i < 48; ++i, offset += pool.mStride) {
		obj = (XObjActor3*)(pool.mObjects + offset);
		if (XObj::GetDataItem(obj->mAttr.mObjData) == 0xB) {
			if (count == n && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
				LaunchSpiderBossPart(obj);
				return;
			}
			count++;
		}
	}
}

/**
 * @note Address: 0x8010FB1C
 * @note Size: 0x284
 */
// jumps the actor onto its holder, at its angle and distance around it
extern "C" void LaunchSpiderBossPart(XObjActor3* actor)
{
	XObj* holder = gGame.mXObjs.mPools[19].Get(0);
	if ((holder->mStatus.mFlags & 1) && (holder->mStatus.mFlags & 0x10000) && actor->mKindStep == 1) {
		f32 s             = sin(actor->mOrbitAngle + HSD_JObjGetRotationY(holder->mJObj));
		f32 x             = actor->mOrbitDistance * s;
		f32 y             = actor->mOrbitHeight;
		f32 c             = cos(actor->mOrbitAngle + HSD_JObjGetRotationY(holder->mJObj));
		f32 z             = actor->mOrbitDistance * c;
		actor->mKindStep  = 2;
		actor->mKindTimer = 6.0f;
		HSD_JObj* jobj    = actor->mJObj;
		HSD_JObjClearMtxFlags(jobj);
		actor->mMoveTranslate.x = x + HSD_JObjGetTranslationX(holder->mJObj);
		actor->mMoveTranslate.y = y + 8.0f + 10.0f;
		actor->mMoveTranslate.z = z + HSD_JObjGetTranslationZ(holder->mJObj);
		actor->SetPos(actor->mMoveTranslate, 0);
		actor->mHeading = (f32)atan2(holder->mMoveTranslate.x - actor->mMoveTranslate.x, holder->mMoveTranslate.z - actor->mMoveTranslate.z)
		                + PI; // facing away from the holder
		PlayGameSound(&gGame.mSound, 0x160021, 0xFF);
	}
}

/**
 * @note Address: 0x8010F318
 * @note Size: 0x804
 */
// NONMATCHING: retail keeps the joint reads in other saved registers and saves r24 too
void XObjActor3::Kind12(u32 frames)
{
	switch (mKindStep) {
	case 0:
		if (!(mActorFlags.mFlags & 4)) {
			mOrbitAngle  = 0.0f;
			mOrbitX      = 0.0f;
			mOrbitHeight = 0.0f;
			mOrbitZ      = 0.0f;
			mStatus.mFlags |= 0x100000;
			mStatus.mFlags |= 4;
			++mKindStep;
		}
		break;
	case 1: {
		XObjSpiderBoss* boss = GetSpiderBoss();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			mHeading = atan2(boss->mMoveTranslate.x - mMoveTranslate.x, boss->mMoveTranslate.z - mMoveTranslate.z);
			SetScale(1.0f, 1.0f, 1.0f, 0);
			CJObj joint;
			s32 path[] = { 0 };
			joint.Attach(GetJointFromRoot(boss->mJObj, 1, path));
			Mtx scaleMtx, translation, result, mount;
			PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), mount);
			Vec scale;
			HSD_MtxGetScale(mount, &scale);
			scale.x = GetStagePlacementScale(this).x / scale.x;
			scale.y = GetStagePlacementScale(this).y / scale.y;
			scale.z = GetStagePlacementScale(this).z / scale.z;
			PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
			PSMTXConcat(mount, scaleMtx, result);
			PSMTXTrans(translation, 0.0f, 0.0f, 0.0f);
			PSMTXConcat(result, translation, result);
			HSD_JObj* jobj = mJObj;
			MtxPtr matrix  = result;
			HSD_JObjCopyMtx(jobj, matrix);
			if (boss->mBlinkTimer) {
				if (boss->mBlinkTimer % 30 < 15)
					SetColor(1, 128, 0, 0, 0.0f);
				else
					SetColor(1, 0, 0, 0, 0.0f);
			}
		}
		break;
	}
	case 2: {
		SetColor(1, 0, 0, 0, 0.0f);
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			mStatus.mFlags &= ~0x10000ULL;
			mActorFlags.mFlags |= 4;
			PlayGameSound(&gGame.mSound, 0x160020, 0xFF);
			mKindStep = 0;
		}
		break;
	}
	case 3:
		if ((mFrameCount & 3) < 2)
			mStatus.mFlags |= 0x10000;
		else
			mStatus.mFlags &= ~0x10000ULL;
		if (--mFrameCount < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			mActorFlags.mFlags |= 4;
			mStatus.mFlags &= ~4ULL;
			mKindStep = 0;
			Release();
		}
		break;
	}
}

/**
 * @note Address: 0x8010F208
 * @note Size: 0x110
 */
extern "C" void EnableSpiderBossCenterPart(void)
{
	XObjActor3* obj;
	s32 offset;
	u8* data;
	s32 size;
	s32 i;
	size = gGame.mXObjs.mPools[5].mStride;
	data = gGame.mXObjs.mPools[5].mObjects;
	for (i = 0, offset = 0; i < 48; ++i, offset += size) {
		obj = (XObjActor3*)(data + offset);
		if (XObj::GetDataItem(obj->mAttr.mObjData) == 0xC && (obj->mStatus.mFlags & 1)) {
			if (obj->mKindStep == 0) {
				obj->mActorFlags.mFlags &= ~4;
			}
			return;
		}
	}
}

/**
 * @note Address: 0x8010EF70
 * @note Size: 0x298
 */
// jumps the first kind 0xC actor onto the holder's middle
// NONMATCHING: only the constants' and strings' relocations differ, the unit's data is not split out of retail
extern "C" void LaunchSpiderBossCenterPart(void)
{
	for (s32 i = 0; i < 48; i++) {
		XObjActor3* obj = (XObjActor3*)gGame.mXObjs.mPools[5].Get(i);
		if (XObj::GetDataItem(obj->mAttr.mObjData) == 0xC && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			XObj* holder = (XObj*)gGame.mXObjs.mPools[19].mObjects;
			if ((holder->mStatus.mFlags & 1) && (holder->mStatus.mFlags & 0x10000) && obj->mKindStep == 1) {
				obj->mKindStep  = 2;
				obj->mKindTimer = 6.0f;
				HSD_JObj* jobj  = obj->mJObj;
				HSD_JObjClearMtxFlags(jobj);
				f32 ofs               = 0.0f; // the holder's middle, where LaunchSpiderBossPart goes round it
				obj->mMoveTranslate.x = ofs + HSD_JObjGetTranslationX(holder->mJObj);
				obj->mMoveTranslate.y = 18.0f;
				obj->mMoveTranslate.z = ofs + HSD_JObjGetTranslationZ(holder->mJObj);
				obj->SetPos(obj->mMoveTranslate, 0);
				obj->mHeading
				    = (f32)atan2(holder->mMoveTranslate.x - obj->mMoveTranslate.x, holder->mMoveTranslate.z - obj->mMoveTranslate.z)
				    + PI; // facing away from the holder
				obj->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				PlayGameSound(&gGame.mSound, 0x160021, 0xFF);
			}
			return;
		}
	}
}

/**
 * @note Address: 0x8010D65C
 * @note Size: 0x1914
 */
// NONMATCHING: step 11's effect turns with the heading; retail stores it into the one temporary it passes, a rotation by value
// needs a second one here (12 bytes more frame), see Kind9. A named `Vec rotation = { 0, mHeading, 0 }` and a file inline
// returning the Vec (rodata image, y stored) both still copy into the argument slot (0x190 frame)
void XObjActor3::Kind13(u32 frames)
{
	XObjChr* player = gGame.GetPlayer();
	Vec pos         = player->mMoveTranslate;
	switch (mKindStep) {
	case 0:
		if (!(mActorFlags.mFlags & 4)) {
			XObjSpiderBoss* boss = GetSpiderBoss();
			if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
				// lobbed at the robot: the first lands on it, the rest in a ring of 24 round it
				PlaceBossProjectile(this, boss, 64);
				mKindTimer = 16;
				f32 dx = pos.x - mMoveTranslate.x, dy = 0, dz = pos.z - mMoveTranslate.z;
				mMoveSpeed = .01f * sqrtf(dx * dx + dy * dy + dz * dz);
				if (mMoveSpeed < 1.2f)
					mMoveSpeed = 1.2f;
				if (mMoveSpeed > 24)
					mMoveSpeed = 24;
				AimBossVolley(this, pos, 24);
			}
		}
		break;
	case 1: {
		XObjSpiderBoss* boss = GetSpiderBoss();
		bool blocked         = false;
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000) && IsSpiderBossProjectileHitSuppressed(boss))
			blocked = true;
		f32 dx = pos.x - mMoveTranslate.x, dy = pos.y - mMoveTranslate.y, dz = pos.z - mMoveTranslate.z;
		if (sqrtf(dx * dx + dy * dy + dz * dz) < 8 && !IsPlayerFlashingFromHit(player) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)
		    && !blocked) {
			player->OnHitFromPosition(pos, 6);
			mRumble     = 1;
			mFrameCount = 30;
			gGame.mParticle.Kill((HSD_Generator*)mGens[0]);
			mGens[0] = (s32)gGame.mParticle.CreateSRT(0xb1, 1, mMoveTranslate, (Vec) { 1, 1, 1 }, (Vec) { 0, 0, 0 }, TRUE);
			PlayGameSound(&gGame.mSound, 0x127, 255);
			SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			mKindStep = 2;
			break;
		}
		CVec velocity;
		if (BossProjectileLanded(this, frames)) {
			mKindTimer       = 0;
			mMoveTranslate.y = 0;
			mFrameCount      = 30;
			if (mTimerFadeMode == 0) {
				mGens[0] = (s32)gGame.mParticle.CreateSRT(0xb1, 1, mMoveTranslate, (Vec) { 1, 1, 1 }, (Vec) { 0, 0, 0 }, TRUE);
				PlayGameSound(&gGame.mSound, 0x127, 255);
			}
			SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			mKindStep = 2;
			break;
		}
		if (mKindTimer >= -3.4499998f)
			mKindTimer -= .36f * frames;
		else
			mKindTimer = -3.4499998f;
		mHeading   = atan2(mOrbitX - mMoveTranslate.x, mOrbitZ - mMoveTranslate.z);
		velocity.x = mMoveSpeed * (f32)sin(mHeading);
		velocity.z = mMoveSpeed * (f32)cos(mHeading);
		velocity.y = 0;
		mMoveTranslate.SetNow(mMoveTranslate + velocity * (f32)frames);
		SetPos(mMoveTranslate, 0);
		break;
	}
	case 2:
		if (GetAnimEndFrame(mJObj) - frames <= GetAnimFrame(mJObj))
			HideBossProjectile(this, false);
		break;
	case 10: // fired straight at the robot, gone after 120 frames
		if (!(mActorFlags.mFlags & 4)) {
			XObjSpiderBoss* boss = GetSpiderBoss();
			if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
				mMoveTranslate.x = HSD_JObjGetTranslationX(boss->mJObj);
				mMoveTranslate.y = 10 + HSD_JObjGetTranslationY(boss->mJObj);
				mMoveTranslate.z = HSD_JObjGetTranslationZ(boss->mJObj);
				SetPos(mMoveTranslate, 0);
				mStatus.mFlags |= 0x10000;
				mStatus.mFlags |= 4;
				mMoveSpeed    = 2;
				mHeading      = atan2(pos.x - mMoveTranslate.x, pos.z - mMoveTranslate.z);
				mMoveRotate.x = -HALF_PI;
				mStatus.mFlags |= 0x10000;
				mFrameCount = 120;
				++mKindStep;
			}
		}
		break;
	case 11: {
		f32 dx = pos.x - mMoveTranslate.x, dy = 0, dz = pos.z - mMoveTranslate.z;
		if (sqrtf(dx * dx + dy * dy + dz * dz) < 8 && !IsPlayerFlashingFromHit(player) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
			player->OnHitFromPosition(pos, 6);
			mRumble     = 1;
			mFrameCount = 30;
			mGens[0]    = (s32)gGame.mParticle.CreateSRT(0xb2, 1, mMoveTranslate, (Vec) { 1, 1, 1 }, (Vec) { 0, mHeading, 0 }, TRUE);
			PlayGameSound(&gGame.mSound, 0x127, 255);
			SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			mKindStep = 12;
		} else if (--mFrameCount < 0)
			HideBossProjectile(this, true);
		else {
			CVec velocity;
			velocity.x = mMoveSpeed * (f32)sin(mHeading);
			velocity.z = mMoveSpeed * (f32)cos(mHeading);
			velocity.y = 0;
			mMoveTranslate.SetNow(mMoveTranslate + velocity * (f32)frames);
			SetPos(mMoveTranslate, 0);
		}
		break;
	}
	case 12:
		if (GetAnimEndFrame(mJObj) - frames <= GetAnimFrame(mJObj))
			HideBossProjectile(this, true);
		break;
	case 20:
		if (!(mActorFlags.mFlags & 4)) {
			XObjSpiderBoss* boss = GetSpiderBoss();
			Vec center           = { 0, 0, 0 };
			if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
				// lobbed round the boss: the first onto its centre, the rest in a ring of 36
				PlaceBossProjectile(this, boss, 16);
				mKindTimer = 10;
				center.x   = HSD_JObjGetTranslationX(boss->mJObj);
				center.y   = HSD_JObjGetTranslationY(boss->mJObj);
				center.z   = HSD_JObjGetTranslationZ(boss->mJObj);
				// the distance to the centre is worked out as in step 0 and not used
				f32 dx = center.x - mMoveTranslate.x, dy = 0, dz = center.z - mMoveTranslate.z;
				f32 distance = sqrtf(dx * dx + dy * dy + dz * dz);
				mMoveSpeed   = 2.4f;
				AimBossVolley(this, center, 36);
			}
		}
		break;
	case 21: {
		CVec velocity;
		if (BossProjectileLanded(this, frames)) {
			mKindTimer       = 0;
			mMoveTranslate.y = 0;
			mFrameCount      = 30;
			if (mTimerFadeMode == 0) {
				mGens[0] = (s32)gGame.mParticle.CreateSRT(0xb1, 1, mMoveTranslate, (Vec) { 1, 1, 1 }, (Vec) { 0, 0, 0 }, TRUE);
				mRumble  = 1;
			}
			SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			mKindStep = 22;
			break;
		}
		if (mKindTimer >= -3.4499998f)
			mKindTimer -= .36f * frames;
		else
			mKindTimer = -3.4499998f;
		mHeading   = atan2(mOrbitX - mMoveTranslate.x, mOrbitZ - mMoveTranslate.z);
		velocity.x = mMoveSpeed * (f32)sin(mHeading);
		velocity.z = mMoveSpeed * (f32)cos(mHeading);
		velocity.y = 0;
		mMoveTranslate.SetNow(mMoveTranslate + velocity * (f32)frames);
		SetPos(mMoveTranslate, 0);
		break;
	}
	case 22:
		if (GetAnimEndFrame(mJObj) - frames <= GetAnimFrame(mJObj))
			HideBossProjectile(this, false);
		break;
	}
}

/**
 * @note Address: 0x8010D390
 * @note Size: 0x2CC
 */
// the script's orders to the boss's projectiles (kind 0x3E0, run by Kind13): 0 lobs all of them at the robot, 2 lobs the idle
// ones round the boss, anything else fires the next idle one straight at the robot
extern "C" void CommandSpiderBossProjectiles(s32 mode)
{
	if (mode == 0) {
		XObjActor3* obj;
		XObjPool& pool = gGame.mXObjs.mPools[5];
		for (s32 i = 0; i < 48; i++) {
			obj = (XObjActor3*)pool.Get(i);
			if (obj->GetKind() == 0x3E0 && (obj->mStatus.mFlags & 1)) {
				obj->mActorFlags.mFlags &= ~4;
				obj->mKindStep        = 0;
				gSpiderBossVolleySlot = 0;
			}
		}
	}
	if (mode == 2) {
		XObjActor3* obj;
		XObjPool& pool = gGame.mXObjs.mPools[5];
		for (s32 i = 0; i < 48; i++) {
			obj = (XObjActor3*)pool.Get(i);
			if (obj->GetKind() == 0x3E0 && (obj->mStatus.mFlags & 1) && obj->mActorFlags.Check(4)) {
				obj->mActorFlags.mFlags &= ~4;
				obj->mKindStep        = 20;
				gSpiderBossVolleySlot = 0;
			}
		}
	} else {
		XObjActor3* obj;
		s32 offset;
		u8* data;
		s32 size;
		s32 i;
		size = gGame.mXObjs.mPools[5].mStride;
		data = gGame.mXObjs.mPools[5].mObjects;
		for (i = 0, offset = 0; i < 48; ++i, offset += size) {
			obj = (XObjActor3*)(data + offset);
			if (obj->GetKind() == 0x3E0) {
				if (obj->mStatus.mFlags & 1) {
					if (obj->mActorFlags.Check(4)) {
						obj->mActorFlags.mFlags &= ~4;
						obj->mKindStep = 10;
						return;
					}
				} else {
					return;
				}
			}
		}
	}
}

/**
 * @note Address: 0x8010D114
 * @note Size: 0x27C
 */
void XObjActor3::UpdateSpiderCounterPanel(u32 frames)
{
	switch (mTimerFadeMode) {
	case 0:
		mTimerFadeMode   = 4;
		mTimerPanelAlpha = 0.0f;
		break;
	case 1:
		mTimerPanelAlpha = 1.0f;
		break;
	case 2:
		mTimerPanelAlpha = 0.0f;
		break;
	case 3:
		mTimerPanelAlpha += 0.08f * frames;
		if (mTimerPanelAlpha >= 1.0f) {
			mTimerPanelAlpha = 1.0f;
		}
		break;
	case 4:
		mTimerPanelAlpha -= 0.08f * frames;
		if (mTimerPanelAlpha < 0.0f) {
			mTimerPanelAlpha = 0.0f;
		}
		break;
	}
	if (mCountdownRunning) {
		mCountdownFrames -= frames;
	}
	switch (mCounterFadeMode) {
	case 0:
		mCounterFadeMode   = 4;
		mCounterPanelAlpha = 0.0f;
		break;
	case 1:
		mCounterPanelAlpha = 1.0f;
		break;
	case 2:
		mCounterPanelAlpha = 0.0f;
		break;
	case 3:
		mCounterPanelAlpha += 0.08f * frames;
		if (mCounterPanelAlpha >= 1.0f) {
			mCounterPanelAlpha = 1.0f;
		}
		break;
	case 4:
		mCounterPanelAlpha -= 0.08f * frames;
		if (mCounterPanelAlpha < 0.0f) {
			mCounterPanelAlpha = 0.0f;
		}
		break;
	}
	s32 count = 0;
	for (s32 i = 0; i < 16; i++) {
		XObj* obj = gGame.mXObjs.mPools[17].Get(i);
		if ((obj->mStatus.mFlags & 0x10000) && !HasSpiderFinishedDeathAnimation(static_cast<XObjSpider*>(obj))) {
			count++;
		}
	}
	mSpiderCount = count;
}

/**
 * @note Address: 0x8010CE80
 * @note Size: 0x294
 */
void XObjActor3::Kind15(u32 frames)
{
	switch (mKindStep) {
	case 0:
		if (mActorFlags.mFlags & 4) {
			break;
		}
		mKindStep++;
		SetAnim(1, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		if (++gActor3Kind15ActivationCounter >= 10) {
			gActor3Kind15ActivationCounter = 0;
		}
		if (!IsGameSoundPlaying(&gGame.mSound, 0x127)) {
			PlayGameSound(&gGame.mSound, 0x127, 0xFF);
		}
		break;
	case 1:
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			mKindStep++;
			SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
		break;
	case 2:
		mActorFlags.mFlags |= 4;
		mKindStep = 0;
		break;
	}
}

/**
 * @note Address: 0x8010CDDC
 * @note Size: 0xA4
 */
extern "C" void ActivateWaitingKind43AActorsAtPosition(f32 x, f32 y, f32 z)
{
	XObjActor3* obj;
	XObjPool& pool = gGame.mXObjs.mPools[5];
	for (s32 i = 0; i < 48; i++) {
		obj = (XObjActor3*)pool.Get(i);
		switch (obj->GetKind()) {
		case 0x43A:
			if ((obj->mStatus.mFlags & 1) && obj->mActorFlags.Check(4)) {
				obj->mActorFlags.mFlags &= ~4;
				obj->mMoveTranslate.x = x;
				obj->mMoveTranslate.y = y;
				obj->mMoveTranslate.z = z;
			}
			break;
		}
	}
}

/**
 * @note Address: 0x8010C304
 * @note Size: 0xAD8
 */
// NONMATCHING: retail keeps the boss in r30 ahead of the joint's three vectors (r29 to r27) and gets by with r24 to r31; here the
// vectors come first, the boss is in r25 and r23 is saved too
void XObjActor3::Kind16(u32 frames)
{
	CJObj joint;
	XObj* boss = gGame.mXObjs.mPools[19].First();
	if (mActorFlags.Check(0x10))
		return;
	switch (mKindStep) {
	case 0:
		mActorFlags.mFlags |= 4;
		mActorFlags.mFlags |= 0x10;
		SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		break;
	case 1: {
		s32 path[] = { 0, 7 };
		joint.Attach(GetJointFromRoot(boss->mJObj, 2, path));
		Vec pos = joint.GetPos();
		mMoveTranslate.Set(pos);
		mHeading = HSD_JObjGetRotationY(boss->mJObj);
		break;
	}
	case 2: {
		s32 path[] = { 0, 2, 0, 1, 1, 0, 0 };
		joint.Attach(GetJointFromRoot(boss->mJObj, 7, path));
		Vec pos          = joint.GetPos();
		f32 y            = pos.y;
		f32 z            = pos.z;
		mHeading         = HSD_JObjGetRotationY(boss->mJObj);
		mMoveTranslate.x = 2.0f * (f32)sin(mHeading) + pos.x;
		mMoveTranslate.y = y;
		mMoveTranslate.z = 2.0f * (f32)cos(mHeading) + z;
		break;
	}
	case 4: {
		s32 path[] = { 0, 7 };
		joint.Attach(GetJointFromRoot(boss->mJObj, 2, path));
		Vec pos = joint.GetPos();
		mMoveTranslate.Set(pos);
		mHeading = HSD_JObjGetRotationY(boss->mJObj);
		f32 end  = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			ResetBossAttachment(this);
			Release();
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8010C0F0
 * @note Size: 0x214
 */
// plays anim n on the first kind 0x10 actor and sizes it for it
extern "C" void PlaySpiderBossAttachmentAnimation(s32 n)
{
	u8* data;
	s32 size;
	size = gGame.mXObjs.mPools[5].mStride;
	data = gGame.mXObjs.mPools[5].mObjects;
	for (s32 i = 0; i < 48; i++) {
		XObjActor3* obj = (XObjActor3*)(data + i * size);
		if (XObj::GetDataItem(obj->mAttr.mObjData) == 0x10 && (obj->mStatus.mFlags & 1)) {
			obj->SetAnim(n, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			obj->mActorFlags.mFlags &= ~4;
			obj->mActorFlags.mFlags &= ~0x10;
			switch (n) {
			case 0:
				obj->SetScale(0.1f, 0.1f, 0.1f, 0);
				obj->mKindStep = 0;
				break;
			case 1:
				obj->SetScale(1.4f, 1.4f, 1.4f, 0);
				obj->mKindStep = 1;
				break;
			case 2:
				obj->SetScale(1.0f, 1.0f, 1.0f, 0);
				obj->mKindStep = 2;
				break;
			case 3:
				obj->SetScale(1.0f, 1.0f, 1.0f, 0);
				obj->mKindStep = 1;
				break;
			case 4:
				obj->SetScale(1.0f, 1.0f, 1.0f, 0);
				obj->mKindStep = 4;
				break;
			}
			return;
		}
	}
}

/**
 * @note Address: 0x8010BF80
 * @note Size: 0x170
 */
// puts the first kind 0x10 actor back to its rest pose, small
extern "C" void ResetSpiderBossAttachment(void)
{
	u8* data;
	s32 size;
	size = gGame.mXObjs.mPools[5].mStride;
	data = gGame.mXObjs.mPools[5].mObjects;
	for (s32 i = 0; i < 48; i++) {
		XObjActor3* obj = (XObjActor3*)(data + i * size);
		if (XObj::GetDataItem(obj->mAttr.mObjData) == 0x10 && (obj->mStatus.mFlags & 1)) {

			ResetBossAttachment(obj);
			return;
		}
	}
}

/**
 * @note Address: 0x8010BF7C
 * @note Size: 0x4
 */
void XObjActor3::Kind17(u32 frames)
{
}

/**
 * @note Address: 0x8010BC0C
 * @note Size: 0x370
 */
// slides out to the left from about ten to noon and back in the afternoon
void XObjActor3::Kind18(u32 frames)
{
	f32 time = gGame.mTimeState.mTime;
	switch (mKindStep) {
	case 0:
		if (time >= 0.42f && time < 0.5f) {
			mKindStep++;
		}
		break;
	case 1:
		mMoveTranslate.x -= 0.2f * frames;
		if (mMoveTranslate.x <= PlacementPosition(mCoord, gActor3PlacementPosition).x - 10.0f) {
			mMoveTranslate.x = PlacementPosition(mCoord, gActor3PlacementPosition).x - 10.0f;
			mKindStep++;
		}
		break;
	case 2:
		if (time >= 0.5f) {
			mKindStep++;
		}
		break;
	case 3:
		mMoveTranslate.x += 0.2f * frames;
		if (mMoveTranslate.x >= PlacementPosition(mCoord, gActor3PlacementPosition).x) {
			mMoveTranslate.x = PlacementPosition(mCoord, gActor3PlacementPosition).x;
			mKindStep        = 0;
		}
		break;
	}
}

/**
 * @note Address: 0x8010BC08
 * @note Size: 0x4
 */
void XObjActor3::Kind19(u32 frames)
{
}

/**
 * @note Address: 0x8010B7B4
 * @note Size: 0x454
 */
// carries the robot while it rides the actor, at the actor's first joint
// NONMATCHING: the robot and the joint's vectors take each other's registers
extern "C" void CarryRiderAtActorJoint(XObjActor3* actor, u32 frames)
{
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	if ((actor->mActorFlags.mFlags & 0x40) && (player->mPlayerStatus.mFlags & 0x40000000)) {
		Mtx scaleMtx;
		Mtx rotMtx;
		Mtx transMtx;
		Mtx mtx;
		Mtx m;
		s32 path[2] = { 0, 0 };
		joint.Attach(GetJointFromRoot(actor->mJObj, 2, path));
		PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), m);
		Vec scale;
		HSD_MtxGetScale(m, &scale);
		scale.x = GetStagePlacementScale(player).x / scale.x;
		scale.y = GetStagePlacementScale(player).y / scale.y;
		scale.z = GetStagePlacementScale(player).z / scale.z;
		PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
		PSMTXConcat(m, scaleMtx, mtx);
		PSMTXRotRad(rotMtx, 'y', 0.0f);
		PSMTXConcat(mtx, rotMtx, mtx);
		PSMTXTrans(transMtx, 0.0f, 3.7f, 30.0f);
		PSMTXConcat(mtx, transMtx, mtx);
		HSD_JObj* jobj = player->mJObj;
		MtxPtr mp      = mtx;
		HSD_JObjCopyMtx(jobj, mp);
	}
}

/**
 * @note Address: 0x8010B528
 * @note Size: 0x28C
 */
// the script's commands to haisui_nezumi: 1 starts it pacing between arg2 and arg3 (hundredths) or, with arg1 0, stops it;
// 2 sets event var arg1 if it holds the robot; 3 lets go of it; 4 sets event var arg1 if the robot is free to be grabbed
extern "C" void HandleKind9ActorCommand(XObjActor3* actor)
{
	switch (actor->mEventCommand) {
	case 1:
		if (actor->mEventArg1 != 0) {
			actor->mActorFlags.mFlags &= ~4;
			actor->mWindowLeft  = actor->mEventArg2 / 100.0f;
			actor->mWindowRight = actor->mEventArg3 / 100.0f;
		} else {
			actor->mActorFlags.mFlags |= 4;
			actor->mWindowLeft  = 0.0f;
			actor->mWindowRight = 0.0f;
			actor->mKindStep    = 0;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 2:
		if ((actor->mActorFlags.mFlags & 0x80) && !(actor->mActorFlags.mFlags & 4)) {
			gEventVars.mVars[actor->mEventArg1] = 1;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 3:
		actor->mActorFlags.mFlags &= ~0x80;
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 4:
		XObjChr* player = gGame.GetPlayer();
		if (!IsBusyAnim(player->mAnimId) && player->mAttachmentRequest < 0) {
			gEventVars.mVars[actor->mEventArg1] = 1;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	}
}

/**
 * @note Address: 0x8010B394
 * @note Size: 0x194
 */
// the script's commands to the actor, mEventCommand is the command and mEventArg1 its argument
extern "C" void HandleSpiderCounterCommand(XObjActor3* actor)
{
	switch (actor->mEventCommand) {
	case 0:
		break;
	case 1:
		if (actor->mEventArg1 != 0) {
			actor->mTimerFadeMode = 3;
		} else {
			actor->mTimerFadeMode = 4;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 2:
		if (actor->mEventArg1 != 0) {
			actor->mCountdownRunning = 1;
		} else {
			actor->mCountdownRunning = 0;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 3:
		actor->mCountdownFrames = (actor->mEventArg1 / 100.0f) * FRAME_RATE;
		actor->mEventCommand    = 0;
		actor->mEventArg1       = 0;
		break;
	case 4:
		gEventVars.mVars[actor->mEventArg1] = TimerFramesToHundredths(actor->mCountdownFrames);
		actor->mEventCommand                = 0;
		actor->mEventArg1                   = 0;
		break;
	case 5:
		if (actor->mEventArg1 != 0) {
			actor->mCounterFadeMode = 3;
		} else {
			actor->mCounterFadeMode = 4;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 6:
		actor->mSpiderCount  = actor->mEventArg1;
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 7:
		gEventVars.mVars[actor->mEventArg1] = actor->mSpiderCount;
		actor->mEventCommand                = 0;
		actor->mEventArg1                   = 0;
		break;
	}
}

/**
 * @note Address: 0x8010AF74
 * @note Size: 0x420
 */
// the robot gets on the actor at its first joint, or off it
extern "C" void HandleActorRiderCommand(XObjActor3* actor)
{
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	switch (actor->mEventCommand) {
	case 1:
		switch (actor->mEventArg1) {
		case 1:
			actor->mActorFlags.mFlags |= 0x40;
			player->mPlayerStatus.mFlags = 0;
			player->mPlayerStatus.mFlags |= 0x40000000;
			player->mPad.mReset = 0;
			break;
		default:
			actor->mActorFlags.mFlags &= ~0x40;
			s32 path[2] = { 0, 0 };
			joint.Attach(GetJointFromRoot(actor->mJObj, 2, path));
			CVec pos(GetJointPosition(joint.mJObj));
			player->SetPos(pos, 0);
			player->mPlayerStatus.mFlags = 0;
			player->mPlayerStatus.mFlags |= 0x20000000;
			player->mPlayerStatus.mFlags |= 7;
			player->mPad.mReset = 1;
			HSD_JObj* jobj      = player->mJObj;
			HSD_JObjClearMtxFlags(jobj);
			break;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	}
}

/**
 * @note Address: 0x8010AD88
 * @note Size: 0x1EC
 */
extern "C" void ResetActor3Fields(XObjActor3* actor)
{
	actor->_B70               = 0.0f;
	actor->mMoveSpeed         = 0.0f;
	actor->mKindTimer         = 0.0f;
	actor->mHeading           = 0.0f;
	actor->mKindStep          = 0;
	actor->mFrameCount        = 0;
	actor->mActorFlags.mFlags = 0;
	actor->mActorFlags.mFlags |= 4;
	actor->mEventCommand      = 0;
	actor->mEventArg1         = 0;
	actor->mEventArg2         = 0;
	actor->mEventArg3         = 0;
	actor->mExtraEventArgs[0] = 0;
	actor->mExtraEventArgs[1] = 0;
	actor->mExtraEventArgs[2] = 0;
	actor->mExtraEventArgs[3] = 0;
	actor->mExtraEventArgs[4] = 0;
	actor->mExtraEventArgs[5] = 0;
	actor->mExtraEventArgs[6] = 0;
	actor->mWindowLeft        = 0.0f;
	actor->mWindowRight       = 0.0f;
	actor->mOrbitAngle        = 0.0f;
	Vec zero                  = { 0.0f, 0.0f, 0.0f };
	actor->mOrbitX            = zero.x;
	actor->mOrbitHeight       = zero.y;
	actor->mOrbitZ            = zero.z;
	for (int i = 0; i < 8; i++) {
		actor->mParts[i].Clear();
	}
	if (actor->mPoolIndex == 0) {
		gSpiderBossVolleySlot          = 0;
		gActor3Kind15ActivationCounter = 0;
	}
	actor->SetColor(1, 0, 0, 0, 0.0f);
	actor->_BDC.x             = 0.0f;
	actor->_BDC.y             = 0.0f;
	actor->_BDC.z             = 0.0f;
	actor->mTimerFadeMode     = 0;
	actor->mTimerPanelAlpha   = 0.0f;
	actor->mCountdownFrames   = 0.0f;
	actor->mCountdownRunning  = 0;
	actor->mCounterFadeMode   = 0;
	actor->mCounterPanelAlpha = 0.0f;
	actor->mSpiderCount       = 0;
	actor->_C08               = 0;
	if (actor->GetKind() == 0x31B) {
		InitParticlePool(&actor->mDust.mPtcl, actor->mDust.mPtcls, 0x50, 0x32);
		actor->mAnimSfx.mEvents = gHaisuiNezumiAnimSfxEvents;
	}
	actor->mGens[0]     = 0;
	actor->mGens[1]     = 0;
	actor->mGens[2]     = 0;
	actor->mGens[3]     = 0;
	actor->mGens[4]     = 0;
	actor->mGens[5]     = 0;
	actor->mRumble      = 0;
	actor->mRumbleTimer = 0;
	actor->mHeld        = NULL;
	actor->_1DE4        = 0;
	actor->mSe          = -1;
}

/**
 * @note Address: 0x8010AD54
 * @note Size: 0x34
 */
void XObjActor3::InitializeObject(u32 id)
{
	XObj::InitializeObject(id);
	ResetActor3Fields(this);
}

/**
 * @note Address: 0x8010AD20
 * @note Size: 0x34
 */
void XObjActor3::ResetObjectState()
{
	XObj::ResetObjectState();
	ResetActor3Fields(this);
}

/**
 * @note Address: 0x8010AD00
 * @note Size: 0x20
 */
void XObjActor3::ResetForNewGame()
{
	ResetActor3Fields(this);
}

/**
 * @note Address: 0x8010A574
 * @note Size: 0x78C
 */
BOOL XObjActor3::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		bool ready = false;
		mMoveSpeed = 0.0f;
		mKindTimer = 0.0f;

		mKindStep          = 0;
		mFrameCount        = 0;
		mActorFlags.mFlags = 0;
		mActorFlags.mFlags |= 4;
		mEventCommand      = 0;
		mEventArg1         = 0;
		mEventArg2         = 0;
		mEventArg3         = 0;
		mExtraEventArgs[0] = 0;
		mExtraEventArgs[1] = 0;
		mExtraEventArgs[2] = 0;
		mExtraEventArgs[3] = 0;
		mExtraEventArgs[4] = 0;
		mExtraEventArgs[5] = 0;
		mExtraEventArgs[6] = 0;
		mWindowLeft        = 0.0f;
		mWindowRight       = 0.0f;
		mOrbitAngle        = 0.0f;
		Vec zero           = { 0.0f, 0.0f, 0.0f };
		mOrbitX            = zero.x;
		mOrbitHeight       = zero.y;
		mOrbitZ            = zero.z;

		mMoveTranslate.x = PlacementPosition(mCoord, gActor3PlacementPosition).x;
		mMoveTranslate.y = PlacementPosition(mCoord, gActor3PlacementPosition).y;
		mMoveTranslate.z = PlacementPosition(mCoord, gActor3PlacementPosition).z;
		mMoveRotate.x    = PlacementRotation(mCoord, gActor3PlacementRotation).x;
		mMoveRotate.y    = PlacementRotation(mCoord, gActor3PlacementRotation).y;
		mMoveRotate.z    = PlacementRotation(mCoord, gActor3PlacementRotation).z;
		mHeading         = PlacementRotation(mCoord, gActor3PlacementRotation).y;
		for (int i = 0; i < 8; ++i)
			mParts[i].Clear();
		if (mPoolIndex == 0) {
			gSpiderBossVolleySlot          = 0;
			gActor3Kind15ActivationCounter = 0;
		}
		switch (GetKind()) {
		case 0x42B:
		case 0x149:
			if (mPanelTpl.ReadEx("tpl/stage02.tpl", 0, 0))
				ready = true;
			return ready;
		case 0x31B: {

			mTex[0] = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[122 - StatusTextureOffset], 0);
			mTex[1] = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[123 - StatusTextureOffset], 0);
			return TRUE;
		}
		default:
			return TRUE;
		}
	}
	return FALSE;
}

// defined behind LoadFromPlacement, as retail's .rodata shows: the dust velocity's zero Vec (0x8020B5D8) follows
// LoadFromPlacement's literals
static inline void EmitKind9Dust(XObjActor3* actor)
{
	f32 x = actor->mMoveTranslate.x, y = actor->mMoveTranslate.y, z = actor->mMoveTranslate.z;
	f32 angle  = DEG2RAD * (rand() % 360);
	f32 sine   = std::sinf(angle);
	f32 vx     = sine * (rand() % 100) / 1000.0f;
	f32 cosine = std::cosf(angle);
	f32 vz     = cosine * (rand() % 100) / 1000.0f;
	f32 vy     = (rand() % 100) / 800.0f;
	Vec pos;
	pos.x = x + 16.0f * (vx * (rand() % 100) / 100.0f);
	pos.z = z + 16.0f * (vz * (rand() % 100) / 100.0f);
	pos.y = y;
	Vec scale;
	scale.x = scale.y = 1.0f + (rand() % 200) / 200.0f;
	scale.z           = 1.0f;
	Vec vel           = { vx, vy, vz };
	CPtclOne* dust    = SpawnPoolParticle(&actor->mDust.mPtcl, &pos, &scale, &vel, 0xffffffa0);
	if (dust) {
		dust->mAngle      = DEG2RAD * (rand() % 360);
		dust->mAngleSpeed = DEG2RAD * (20 - rand() % 40);
		dust->mColor      = 0xffffffa0;
		dust->mTex        = &actor->mTex[0];
	}
}

/**
 * @note Address: 0x8010A500
 * @note Size: 0x74
 */
void XObjActor3::Release()
{
	switch (GetKind()) {
	case 0x149:
	case 0x42B:
		mPanelTpl.Close();
		break;
	}
	XObj::Release();
}

/**
 * @note Address: 0x80109E80
 * @note Size: 0x680
 */
// the per-kind script commands, the kind's own function, the carried objects
void XObjActor3::UpdateObject(u32 frames)
{
	switch (GetItem()) {
	case 8:
		mActorFlags.mFlags |= 4; // flag 4 skips the heading/pos write below
		break;
	case 9:
		UpdateAnimationSounds();
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		break;
	case 10:
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		switch (mEventCommand) {
		case 1:
			mActorFlags.mFlags &= ~4;
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		}
		break;
	case 11:
	case 12:
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		switch (mEventCommand) {
		case 1:
			if (mEventArg1 != 0) {
				mActorFlags.mFlags &= ~4;
			} else {
				mActorFlags.mFlags |= 4;
				mKindStep = 0;
			}
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		case 2:
			mKindStep     = 2;
			mKindTimer    = mEventArg2 / 100.0f; // script units are 1/100
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		}
		break;
	case 13:
		switch (mRumble) {
		case 0:
			break;
		case 1: // start the rumble
			if (gGame.mStatus.mFlags & 0x800000000ULL) {
				CPadOne* rumble = &gGame.mPad.mPad[0];
				if (rumble->mConnected) {
					HSD_PadRumbleOn(rumble->mPort);
				}
				mRumbleTimer = 24 / frames;
				mRumble++;
			} else {
				mRumble = 0;
			}
			break;
		case 2:
			if (--mRumbleTimer < 0) {
				CPadOne* rumble = &gGame.mPad.mPad[0];
				if (rumble->mConnected) {
					HSD_PadRumbleOffN(rumble->mPort);
				}
				if (rumble->mConnected) {
					HSD_PadRumbleRemove(rumble->mPort);
				}
				mRumble = 0;
			}
			break;
		}
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		switch (mEventCommand) {
		case 1:
			mActorFlags.mFlags &= ~4;
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		}
		break;
	case 14:
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		HandleSpiderCounterCommand(this);
		break;
	}
	if (mActorFlags.mFlags & 0x10) { // flag 0x10 skips the update
		return;
	}
	(this->*gActor3UpdateHandlers[GetItem()])(frames);
	if (!(mActorFlags.mFlags & 4)) {
		SetRotateY(mHeading, 0.0f);
		SetPos(mMoveTranslate, 0);
	}
	UpdateObject__4XObjFUl(this, frames);
	if (GetKind() == 0x31B) { // 0x31B (haisui_nezumi) runs its dust particles
		UpdateParticlePool(&mDust.mPtcl, frames);
	}
	if (GetKind() == 0x2C3) { // carries a rider at a joint
		CarryRiderAtActorJoint(this, frames);
	}
	if (mActorFlags.mFlags & 2) { // flag 2: carried parts follow their joints
		for (u32 i = 0; i < 8; i++) {
			if (mParts[i].mJoint != NULL && mParts[i].mObj != NULL) {
				Mtx t;
				XObj* obj = mParts[i].mObj;
				PSMTXTrans(t, obj->mMoveTranslate.x, obj->mMoveTranslate.y, obj->mMoveTranslate.z);
				PSMTXCopy(HSD_JObjGetMtxPtr(mParts[i].mJoint), mParts[i].mMtx);
				Vec scale;
				HSD_MtxGetScale(mParts[i].mMtx, &scale);
				scale.x = GetStagePlacementScale(mParts[i].mObj).x / scale.x;
				scale.y = GetStagePlacementScale(mParts[i].mObj).y / scale.y;
				scale.z = GetStagePlacementScale(mParts[i].mObj).z / scale.z;
				Mtx s;
				PSMTXScale(s, scale.x, scale.y, scale.z);
				Mtx m;
				PSMTXConcat(mParts[i].mMtx, s, m);
				PSMTXConcat(m, t, m);
				HSD_JObj* joint = mParts[i].mObj->mJObj;
				MtxPtr mp       = m;
				HSD_JObjCopyMtx(joint, mp);
			}
		}
	}
}

/**
 * @note Address: 0x80109E44
 * @note Size: 0x3C
 */
void XObjActor3::DrawObject()
{
	if (!(mActorFlags.mFlags & 0x10)) {
		DrawWithView(NULL);
	}
}

/**
 * @note Address: 0x80109DD8
 * @note Size: 0x6C
 */
void XObjActor3::DrawScreenOverlay()
{
	UnkObjData* data = mAttr.mObjData;
	s16 type         = data != NULL ? (data != NULL ? data->mItemIndex : 0) : -1; // todo: cleanup, probably an inline
	switch (type) {
	case 14:
		DrawStage02TimerPanel(this);
		DrawStage02ActorCounterPanel(this);
		break;
	}
}

/**
 * @note Address: 0x801095E8
 * @note Size: 0x7F0
 */
// Draw the stage02 countdown above the spider panel.
// NONMATCHING: the locals and the quad arguments take other registers, and the
// literal-pool slots differ
extern "C" void DrawStage02TimerPanel(XObjActor3* actor)
{
	f32 alpha = actor->mTimerPanelAlpha;
	if (alpha <= 0.0f) {
		return;
	}
	if (!IsStage("stage02")) {
		return;
	}
	if (!actor->mPanelTpl.mBound) {
		return;
	}
	CTexObj tex;
	tex       = actor->mPanelTpl.mTex.Init(&((TPLPalette*)actor->mPanelTpl.mBuffer)->descriptorArray[4], 0);
	f32 scale = 1.0f;
#if defined(VERSION_JP)
	f32 w = scale * GXGetTexObjWidth(&tex);
#else
	u16 w = GXGetTexObjWidth(&tex);
#endif
	u16 h = GXGetTexObjHeight(&tex);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
#if defined(VERSION_JP)
	DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 320.0f, 40.0f, 0.0f, w, scale * h, 0.0f, 0.0f, 0.0f);
#else
	DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 320.0f, 40.0f, 0.0f, scale * w, scale * h, 0.0f, 0.0f, 0.0f);
#endif
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
#if !defined(VERSION_JP)
	DrawTimerLabel(scale);
#endif
	tex = actor->mPanelTpl.mTex.Init(&((TPLPalette*)actor->mPanelTpl.mBuffer)->descriptorArray[1], 0);
	f32 x;
	s32 tw = GXGetTexObjWidth(&tex) / 11;
	f32 dw = 1.4f * (scale * tw);
	u16 th = GXGetTexObjHeight(&tex);
	f32 dh = 1.4f * (scale * th);
	x      = 110.0f;
#if !defined(VERSION_JP)
	x += 11.0f;
#endif
	f32 count = TimerFramesToHundredths(actor->mCountdownFrames) / 100.0f;
	PSMTXScale(tex.mMtx[1], 0.09090909f, 1.0f, 1.0f);
	tex.mMtxDirty = TRUE;
	u32 c0;
	u32 c1;
	s32 i = 0;
	u32 a = 255.0f * alpha;
	c0    = a | 0xFFFFFF00;
	c1    = a | 0x1F33FF00;
	for (; i < 2; i++) {
		f32 p = 1.0f;
		if (i > 0) {
			for (s32 k = 0; k < i; k++) {
				p *= 10.0f;
			}
		}
		s32 d = count / p;
		if (i == 0 || d != 0) {
			d %= 10;
			PSMTXTrans(tex.mMtx[0], d * 0.09090909f, 0.0f, 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			DrawGsTexturedQuad(&gGame.mGs, &tex, c0, c0, c1, c1, 7.0f * dw + (x - i * (dw - 8.0f)) - 32.0f, 76.0f, 0.0f, dw, dh, 0.0f, 0.0f,
			                   0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
	}

	tw = GXGetTexObjWidth(&tex) / 11;
	dw = 1.4f * (scale * tw);
	th = GXGetTexObjHeight(&tex);
	dh = 1.4f * (scale * th);
	x  = 110.0f;
#if !defined(VERSION_JP)
	x += 11.0f;
#endif
	PSMTXTrans(tex.mMtx[0], 10.0f / 11.0f, 0.0f, 0.0f); // the eleventh glyph
	tex.mMtxDirty = TRUE;
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	s32 digits = 2; // the glyph slides with the digit count
	if (count < 10.0f) {
		digits = 1;
	}
	DrawGsTexturedQuad(&gGame.mGs, &tex, c0, c0, c1, c1, 7.0f * dw + (x - digits * (dw - 8.0f)) - 32.0f, 76.0f, 0.0f, dw, dh, 0.0f, 0.0f,
	                   0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

/**
 * @note Address: 0x8010904C
 * @note Size: 0x59C
 */
// Draw the stage02 spider count as two digits over the panel.
// NONMATCHING: the colours, the texture matrix pointer and the loop count take other
// saved regs. The instruction stream is identical to retail otherwise — the residual is
// allocation colouring plus the anonymous .sdata2 pool. Tried: `s32 i` inside the `for`
// (98.0 on PAL) and the height fetch inlined into TexSize (96.2): both keep the same
// stream but reseed the allocation and score lower. Retail numbers the width's conversion temps (f0, f1) before the
// scale literal (f2, 0x801091A8); a u16 width local, TexSize on it or on the call all sink the product (92.9..96.9).
extern "C" void DrawStage02ActorCounterPanel(XObjActor3* actor)
{
	f32 alpha = actor->mCounterPanelAlpha;
	if (alpha <= 0.0f) {
		return;
	}
	if (!IsStage("stage02")) {
		return;
	}
	if (!actor->mPanelTpl.mBound) {
		return;
	}
	CTexObj tex;
	tex       = actor->mPanelTpl.mTex.Init(&((TPLPalette*)actor->mPanelTpl.mBuffer)->descriptorArray[9], 0);
	f32 scale = 1.0f;
	f32 w     = scale * GXGetTexObjWidth(&tex);
	u16 h     = GXGetTexObjHeight(&tex);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 320.0f, 40.0f, 0.0f, w, TexSize(h, 1.0f), 0.0f, 0.0f, 0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	tex    = actor->mPanelTpl.mTex.Init(&((TPLPalette*)actor->mPanelTpl.mBuffer)->descriptorArray[1], 0); // the digit sheet
	f32 dw = 1.4f * TexSize(GXGetTexObjWidth(&tex) / 11, 1.0f);                                           // eleven glyphs in a row
	f32 x;
	f32 dh    = 1.4f * TexSize(GXGetTexObjHeight(&tex), 1.0f);
	x         = 110.0f - (1 - actor->mSpiderCount / 10) * 12;
	f32 count = actor->mSpiderCount;
	PSMTXScale(tex.mMtx[1], 0.09090909f, 1.0f, 1.0f); // one eleventh of the digit sheet per glyph
	tex.mMtxDirty = TRUE;
	s32 i         = 0;
	u32 a         = 255.0f * alpha;
	u32 c0;
	u32 c1 = a | 0x1F33FF00; // colored below
	c0     = a | 0xFFFFFF00; // white above
	for (; i < 2; i++) {
		f32 p = 1.0f;
		if (i > 0) {
			for (s32 k = 0; k < i; k++) {
				p *= 10.0f;
			}
		}
		s32 d = count / p;
		if (i == 0 || d != 0) {
			d %= 10;
			PSMTXTrans(tex.mMtx[0], d * 0.09090909f, 0.0f, 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			DrawGsTexturedQuad(&gGame.mGs, &tex, c0, c0, c1, c1, 7.0f * dw + (x - i * (dw - 8.0f)) - 32.0f, 76.0f, 0.0f, dw, dh, 0.0f, 0.0f,
			                   0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
	}
}

/**
 * @note Address: 0x80109008
 * @note Size: 0x44
 */
void XObjActor3::DrawObjectEffects()
{
	if (GetKind() == 0x31B) {
		DrawParticlePool(&mDust.mPtcl);
	}
}

/**
 * @note Address: 0x80109004
 * @note Size: 0x4
 */
void XObjActor3::DrawDebugStatus()
{
}

/**
 * @note Address: 0x80108794
 * @note Size: 0x870
 */
// the event commands: 100 carries another object at one of its joints, or lets go of all
// of them, anything else is kept for the kind's function to read
// NONMATCHING: argv[1] and the second 0x40B item loop's i and pool base take other registers, as in game/xobjspider. retail loads argv[1]
// into r5 at the top and the OSReport passes it as its second %d; written as xobjspider does (a const local
// passed to OSReport), the saved registers shift and the function drops to 98.4 %
s32 XObjActor3::HandleEventCommand(s32 argc, s32* argv)
{
	switch (argv[0]) {
	case 100:
		if (argv[1] == 0) {
			mActorFlags.mFlags &= ~2;
			for (u32 i = 0; i < 8; i++) {
				if (mParts[i].mObj != NULL) {
					HSD_JObjClearMtxFlags(mParts[i].mObj->mJObj);
					mParts[i].mObj   = NULL;
					mParts[i].mJoint = NULL;
				}
			}
		} else {
			u32 i;
			for (i = 0; mParts[i].mObj != NULL && i < 8; i++) { } // first free part slot
			if (i < 8) {
#if !defined(VERSION_PAL)
				OSReport("id = %d, argv[1]=%d\n", i);
#endif
				mActorFlags.mFlags |= 2;
				if (argv[1] >= 10000) { // a placement id vs a slot index
					mParts[i].mObj = FindObj(argv[1]);
				} else {
					mParts[i].mObj = gGame.GetObject((s16)argv[1]);
				}
#if !defined(VERSION_PAL)
				if (mParts[i].mObj == NULL) {
					OSReport("NULL===============\n");
				}
#endif
				s32 path[8];
				s32 depth = argv[2];
				for (s32 j = 0; j < depth; j++) {
					path[j] = argv[3 + j];
				}
				mParts[i].mJoint = GetJointByPath(mJObj, path, depth);
				Mtx t;
				XObj* obj = mParts[i].mObj;
				PSMTXTrans(t, obj->mMoveTranslate.x, obj->mMoveTranslate.y, obj->mMoveTranslate.z);
				PSMTXCopy(HSD_JObjGetMtxPtr(mParts[i].mJoint), mParts[i].mMtx);
				Vec scale;
				HSD_MtxGetScale(mParts[i].mMtx, &scale);
				scale.x = GetStagePlacementScale(mParts[i].mObj).x / scale.x;
				scale.y = GetStagePlacementScale(mParts[i].mObj).y / scale.y;
				scale.z = GetStagePlacementScale(mParts[i].mObj).z / scale.z;
				Mtx s;
				PSMTXScale(s, scale.x, scale.y, scale.z);
				Mtx m;
				PSMTXConcat(mParts[i].mMtx, s, m);
				PSMTXConcat(m, t, m);
				HSD_JObj* joint = mParts[i].mObj->mJObj;
				MtxPtr mp       = m;
				HSD_JObjCopyMtx(joint, mp);
			}
		}
		break;
	default:
		mEventCommand      = argv[0];
		mEventArg1         = argv[1];
		mEventArg2         = argv[2];
		mEventArg3         = argv[3];
		mExtraEventArgs[0] = argv[4];
		mExtraEventArgs[1] = argv[5];
		mExtraEventArgs[2] = argv[6];
		mExtraEventArgs[3] = argv[7];
		mExtraEventArgs[4] = argv[8];
		mExtraEventArgs[5] = argv[9];
		mExtraEventArgs[6] = 0;
		switch (GetItem()) {
		case 9:
			HandleKind9ActorCommand(this);
			break;
		case 0x13:
			HandleActorRiderCommand(this);
			break;
		case 8:
			switch (mEventCommand) {
			case 0:
				break;
			case 1:
				HideItems(this);
				switch (mEventArg1) {
				case 0x40B:
					for (s32 i = 0; i < 0x120; i++) {
						XObj* obj = gGame.mXObjs.mPools[22].Get(i);
						if (mEventArg1 == obj->GetKind()) {
							mHeld = obj;
							obj->mStatus.mFlags |= 0x10000ULL;
							switch (mEventArg1) {
							case 0x40B:
								mActorFlags.mFlags |= 0x100;
								obj->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
								break;
							}
						}
					}
					break;
				default:
					if (mHeld != NULL) {
						HSD_JObjClearMtxFlags(mHeld->mJObj);
						mHeld = NULL;
					}
					break;
				}
				mEventCommand = 0;
				break;
			}
			break;
		}
		break;
	}
	return 0;
}

/**
 * @note Address: 0x80108728
 * @note Size: 0x6C
 */
void XObjActor3::OnHitFromPosition(const Vec& position, f32 strength)
{
	if (GetItem() == 1) { // item 1's hits go to the base class
		XObj::OnHitFromPosition(position, strength);
	} else if (!mActorFlags.Check(1)) {
		mActorFlags.mFlags |= 1;
	}
}

/**
 * @note Address: 0x80108694
 * @note Size: 0x94
 */
void XObjActor3::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	XObj::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
	if (GetKind() == 0x31B) {
		if (mAnimId == 7) {
			mSe = PlayGameSound(&gGame.mSound, 0xB0000, 0xFF);
		} else if (HSD_AudioSFXCheck(mSe)) { // leaving anim 7: key the sound off
			HSD_AudioSFXKeyOff(mSe);
		}
	}
}

/**
 * @note Address: 0x80108540
 * @note Size: 0x154
 */

void XObjActor3::UpdateAnimationSounds()
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000) && mPoolIndex == 3) {
		UpdateActorAnimationSound(this, mAnimId);
	}
}

/**
 * @note Address: 0x80108424
 * @note Size: 0x11C
 */
XObjActor3::~XObjActor3()
{
}

#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/infoscrn.h"
#include "game/wii/cpad.h"
#include "game/wii/ceffect.h"
#include "game/wii/cpadact.h"
#include "game/wii/xact.h"
#include "game/gmath.h"
#include "arithmetic.h"
#include <math.h>
#include <stdlib.h>
#include "game/attachmentvalues.h"
#include "game/wii/spiderplacement.h"

// the Wii file of the GameCube's xobjspiderboss.cpp (retail 0x80132330-0x801370BC), its functions in
// the Wii's order

extern "C" {
void ResetSpiderBossState(XObjSpiderBoss* spider);
void ApplySpiderBossPlacement(XObjSpiderBoss* spider);

void ResetSpiderCoinBounces(void* unused);
void UpdateSpiderCoinBounces(void* unused, u32 frames);
void UpdateSpiderBossDamage(XObjSpiderBoss* spider, u32 frames);
void UpdateSpiderBossAttachments(XObjSpiderBoss* spider, u32 frames);
s32 GetAttachmentCountById(AttachmentValues* values, s32 id);
EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);
void fn_800A12F8(EffectInstance* effects, u32 id, BOOL retireParticles);
void fn_8013E788(s32 command);
void fn_8013F41C(s32 anim);
void fn_8013F558(void);
void ApproachHudValue(f32* value, f32* target, f32 step);
BOOL IsUsingMug(XAction* lift);
void fn_800FCD1C(XAction* act);
void fn_800FCDF0(XAction* act);
void fn_800FCE60(XAction* act);
void ResetPlayerPopperAction(XAction* act);
void fn_8010C0F4(XAction* act);
void FinishTumbleAction(XActTumble* act);
f32 GetPlayerCollisionRadius(WiiPlayerState* player);
BOOL IsPlayerFlashingFromHit(WiiPlayerState* player);
void ApplyPlayerHitReaction(WiiPlayerState* player, BOOL react, BOOL stop, f32 time);
u32 GetPlayerActionParameter(s32 index);
BOOL IsLiftTargetPlug(XAction* act);
BOOL IsCarryingMug(XAction* act);
BOOL IsCarryingToothbrush(XAction* act);
BOOL IsCarryingSpoon(XAction* act);
BOOL IsCarryingSquirter(XAction* act);
CVec GetStagePlacementScale(const XObj* obj);
void HSD_MtxGetScale(Mtx mat, Vec* vec);
void fn_8013C7EC(s32 index);
void fn_8013C6A4(s32 index);
void fn_8013CF58(void);
XObj* ResolveEventObjectId(const CEvtData* data, s16 id);
void fn_8013D0C8(void);
void StartScreenFade(CScrnColor* color, u8 r0, u8 g0, u8 b0, u8 a0, u8 r1, u8 g1, u8 b1, u8 a1, f32 time);

extern s32 gRefreshRate;
extern CEvtVars gEventVars;
extern AnimSfxEvent gSpiderBossAnimSfxEvents[];
extern u32 gSpiderBossCoinBounceContext;
extern u8 gSpiderDrainFlashClaimed;
extern s32 gSpiderDrainFlashTimers[2];
// the states, by number
extern void (XObjSpiderBoss::*gSpiderBossStateHandlers[5])(u32 frames); // the boss's coin bounce token, the callbacks never touch it
}

// puts the object at a place with nowhere to go
static inline void SetPosAndTarget(XObjSpiderBoss* spider, const Vec& value)
{
	spider->mMoveTranslate.Set(value);
	spider->mMoveTarget0.Set(value);
}

// back to the start: state 0, waiting to be placed
static inline void ResetState(XObjSpiderBoss* spider)
{
	ResetSpiderBossState(spider);
	spider->SetState(0);
	spider->mSpiderStatus.mFlags |= 0x4000000;
}

/**
 * @note Address: 0x80132330
 * @note Size: 0x58
 */
void XObjSpiderBoss::InitializeObject()
{
	XObj::InitializeObject();
	ResetState(this);
}

/**
 * @note Address: 0x80132388
 * @note Size: 0x50
 */
void XObjSpiderBoss::ResetForNewGame()
{
	ResetState(this);
}

/**
 * @note Address: 0x801323D8
 * @note Size: 0x2AC
 */
// NONMATCHING: retail reads mPoolIndex after the part stores, ours before them (as in ResetSpiderState)
extern "C" void ResetSpiderBossState(XObjSpiderBoss* spider)
{
	spider->mHeading             = 0.0f;
	spider->mSpeed               = 0.0f;
	spider->mTargetRotY          = 0.0f;
	spider->mRotSpeed            = 0.1f;
	spider->mSpeedY              = 0.0f;
	spider->_9C8                 = 0.0f;
	spider->_9CC                 = 0.0f;
	spider->mState               = 0;
	spider->mPrevState           = 0;
	spider->mStep                = 0;
	spider->mTimer               = 0;
	spider->mSpiderStatus.mFlags = 0;
	spider->mActionTimer         = 0;
	spider->_9EC                 = 1.0f;
	spider->_9F0                 = 1.0f;
	Vec hitPos                   = { 0.0f, 0.0f, 0.0f };
	spider->mHitPos              = hitPos;
	Vec basePos                  = { 0.0f, 0.0f, 0.0f };
	spider->mBasePos             = basePos;
	Vec patrolTarget             = { 0.0f, 0.0f, 0.0f };
	spider->mPatrolTarget        = patrolTarget;
	spider->mHitStrength         = 0.0f;
	spider->mMaxHits             = 23;
	spider->mHits                = spider->mMaxHits;
	spider->mCommand             = 0;
	spider->mArgs[0]             = 0;
	spider->_A6C                 = 0;
	spider->_9BC                 = 1.0f;
	spider->mStatus.mFlags |= 8ULL;
	spider->mRadius    = 28.0f;
	Vec areaPos        = { 0.0f, 0.0f, 0.0f };
	spider->mAreaPos   = areaPos;
	spider->mAreaWidth = 0.0f;
	spider->mAreaDepth = 0.0f;
	Vec startPos       = { 0.0f, 0.0f, 0.0f };
	spider->mStartPos  = startPos;
	spider->_A44       = 0.0f;
	spider->SetColor(1, 0, 0, 0, 0.0f);
	spider->_A70                 = 0.0f;
	spider->mEscapeInputStrength = 0.0f;
	spider->mDrainTimer          = 0;
	spider->mEscapeRemaining     = 0.0f;
	Vec vecA80                   = { 0.0f, 0.0f, 0.0f };
	spider->_A80                 = vecA80;
	spider->mGrabIntervalFrames  = 3 * gRefreshRate; // 3 seconds
	spider->ResetPatrolArea();
	spider->mDamageStep = 0;
	spider->mBlinkTimer = 0;
	for (u32 i = 0; i < 8; ++i) {
		spider->mParts[i].mObj = NULL;
	}
	if (spider->mPoolIndex == 0) {
		ResetSpiderCoinBounces(&gSpiderBossCoinBounceContext);
	}
	spider->mPatrolSelection = 0;
	spider->mAltAnimId       = 0;
	spider->mGrabCooldown    = 5;
	spider->mStatus.mFlags |= 4ULL;
	spider->mPatrolEffectActive = 0;
}

/**
 * @note Address: 0x80132684
 * @note Size: 0x440
 */
extern "C" void ApplySpiderBossPlacement(XObjSpiderBoss* spider)
{
	spider->SetColor(1, 0, 0, 0, 0.0f);
	spider->mMoveTranslate.x = PlacementPosition(spider->mCoord).x;
	spider->mMoveTranslate.y = PlacementPosition(spider->mCoord).y;
	spider->mMoveTranslate.z = PlacementPosition(spider->mCoord).z;
	spider->mBasePos.x       = PlacementPosition(spider->mCoord).x;
	spider->mBasePos.y       = PlacementPosition(spider->mCoord).y;
	spider->mBasePos.z       = PlacementPosition(spider->mCoord).z;
	spider->mHeading         = PlacementRotation(spider->mCoord).y;
	spider->mRadius          = 28.0f * PlacementScale(spider->mCoord).x;
	spider->mStep            = 0;
	spider->mTimer           = 0;
	spider->mPrevState       = spider->mState;
	spider->mState           = 0;
	spider->mSpiderStatus.mFlags |= 0x8000;
	spider->mSpiderStatus.mFlags |= 0x02000000;
}

/**
 * @note Address: 0x80132AC4
 * @note Size: 0x50
 */
BOOL XObjSpiderBoss::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		ResetSpiderBossState(this);
		ApplySpiderBossPlacement(this);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80132B14
 * @note Size: 0x40
 */
void XObjSpiderBoss::ResetObjectState()
{
	XObj::ResetObjectState();
	mAnimSfx.Init((WiiPlayerState*)this, gSpiderBossAnimSfxEvents);
}

/**
 * @note Address: 0x80132B54
 * @note Size: 0x254
 */
// NONMATCHING: only the CPad vtable reference differs, retail's copy (0x802E6DF8) has no name: naming it collides
// with cpaddtor.o's in the link
void XObjSpiderBoss::UpdateObject(u32 frames)
{
	CPad pad(0); // unused, retail builds one here
	if (gGame.GetStatusFlags(0x400)) {
		Vec& move = gGame.GetPlayer()->_1C9A8;
		move.x    = 0.0f;
		move.y    = 0.0f;
		move.z    = 0.0f;
		return;
	}
	if (mPoolIndex == 0) {
		UpdateSpiderCoinBounces(&gSpiderBossCoinBounceContext, frames);
	}
	if (!(mStatus.mFlags & 0x10000ULL) && mState != 1) {
		return;
	}
	UpdateAnimationSounds();
	UpdateSpiderBossDamage(this, frames);
	if (!(mSpiderStatus.mFlags & 0x02000000)) {
		(this->*gSpiderBossStateHandlers[mState])(frames);
		CVec step;
		if (mSpiderStatus.mFlags & 8) {
			mMoveTranslate.y += mSpeedY;
		} else {
			mMoveTranslate.y = mBasePos.y;
		}
		step.x = mSpeed * (f32)sin(mHeading);
		step.z = mSpeed * (f32)cos(mHeading);
		step.y = 0.0f;
		SetRotateY(mTargetRotY, mRotSpeed);
		SetPosAndTarget(this, mMoveTranslate + step * (f32)frames);
		SetPos(mMoveTranslate);
	}
	XObj::UpdateObject(frames);
	UpdateSpiderBossAttachments(this, frames);
}

/**
 * @note Address: 0x80132DA8
 * @note Size: 0x14
 */
void XObjSpiderBoss::DrawObject()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x80132DBC
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawGroundShadow(f32 opacity)
{
	XObj::DrawGroundShadow(opacity);
}

/**
 * @note Address: 0x80132DC0
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	XObj::DrawGroundShadowAt(origin, opacity);
}

/**
 * @note Address: 0x80132DC4
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawScreenOverlay()
{
}

/**
 * @note Address: 0x80132DC8
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawDebugStatus()
{
}

/**
 * @note Address: 0x80132DCC
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawObjectEffects()
{
}

/**
 * @note Address: 0x80132DD0
 * @note Size: 0x7CC
 */
// NONMATCHING: the GetPos evaluation schedules now match; command-switch register allocation and frame layout differ
s32 XObjSpiderBoss::HandleEventCommand(s32 count, s32* args)
{
	switch (args[0]) {
	case 100: {
		s32 id = args[1];
		if (id == 0) {
			mSpiderStatus.mFlags &= ~0x20000000;
			for (u32 i = 0; i < 8; i++) {
				if (mParts[i].mObj != NULL) {
					mParts[i].mObj->ClearBaseMtx();
					mParts[i].mObj = NULL;
				}
			}
		} else {
			u32 i;
			for (i = 0; mParts[i].mObj != NULL && i < 8; i++) { }
			if (i >= 8) {
				break;
			}
			mSpiderStatus.mFlags |= 0x20000000;
			if (args[1] >= 10000) {
				mParts[i].mObj = gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL;
			} else {
				mParts[i].mObj = gGame.mXObjs.mObjects[(s16)args[1]];
			}
			s32 path[8];
			s32 depth = args[2];
			for (s32 j = 0; j < depth; j++) {
				path[j] = args[3 + j];
			}
			mParts[i].mJoint = vf284(depth, path);
			Mtx mtx;
			f32 z = mParts[i].mObj->GetPos()->z;
			f32 y = mParts[i].mObj->GetPos()->y;
			f32 x = mParts[i].mObj->GetPos()->x;
			PSMTXTrans(mtx, x, y, z);
			PSMTXConcat(GetWorldMtx(mParts[i].mJoint), mtx, mParts[i].mMtx);
			mParts[i].mObj->SetBaseMtx(mParts[i].mMtx);
		}
		return 0;
	}
	default:
		if (count < 1) {
			return 0;
		}
		mCommand = args[0];
		if (count < 2) {
			mArgs[0] = 0;
		} else {
			mArgs[0] = args[1];
		}
		if (count < 3) {
			mArgs[1] = 0;
		} else {
			mArgs[1] = args[2];
		}
		if (count < 4) {
			mArgs[2] = 0;
		} else {
			mArgs[2] = args[3];
		}
		if (count < 5) {
			mArgs[3] = 0;
		} else {
			mArgs[3] = args[4];
		}
		if (count < 6) {
			mArgs[4] = 0;
		} else {
			mArgs[4] = args[5];
		}
		break;
	}
	switch (mCommand) {
	case 1:
		SetState(2);
		mSpiderStatus.mFlags &= ~0x02000000;
		mCommand = 0;
		break;
	case 2:
		if (mSpiderStatus.mFlags & 0x10000) {
			gGame.mEvt.mSelectResult = 1;
		} else {
			gGame.mEvt.mSelectResult = 0;
		}
		mCommand = 0;
		break;
	case 3:
		if (mSpiderStatus.mFlags & 0x80000) {
			gEventVars.mVars[mArgs[0]] = 1;
		} else {
			gEventVars.mVars[mArgs[0]] = 0;
		}
		mCommand = 0;
		break;
	case 4:
		mSpiderStatus.mFlags |= 0x200000;
		mCommand = 0;
		break;
	case 5:
		mSpiderStatus.mFlags |= 0x400000;
		mCommand = 0;
		break;
	case 6:
		mStatus.mFlags |= 0x100000;
		mCommand = 0;
		break;
	case 7:
		if (mArgs[0] == 1) {
			mSpiderStatus.mFlags &= ~0x02000000;
		} else {
			mSpiderStatus.mFlags |= 0x02000000;
		}
		mCommand = 0;
		mArgs[0] = 0;
		break;
	case 8:
		mCommand = 0;
		break;
	case 9:
		mBasePos.x       = mArgs[0] / 100.0f;
		mBasePos.y       = mArgs[1] / 100.0f;
		mBasePos.z       = mArgs[2] / 100.0f;
		mStartPos        = mBasePos;
		mMoveTranslate.x = mBasePos.x;
		mMoveTranslate.y = mBasePos.y;
		mMoveTranslate.z = mBasePos.z;
		mMoveTarget0.Set(mMoveTranslate);
		mCommand = 0;
		break;
	case 10:
		mAreaPos.x = mArgs[0] / 100.0f;
		mAreaPos.y = mArgs[1] / 100.0f;
		mAreaPos.z = mArgs[2] / 100.0f;
		mAreaWidth = mArgs[3];
		mAreaDepth = mArgs[4];
		mCommand   = 0;
		break;
	case 11:
		mSpeed   = 0.0f;
		mCommand = 0;
		break;
	case 12:
		mCommand = 0;
		break;
	case 16:
		for (int i = 0; i < 11; ++i) {
			fn_8013C6A4(i);
		}
		fn_8013CF58();
		mCommand = 0;
		break;
	case 17:
		if (mArgs[0] == 1) {
			mSpiderStatus.mFlags |= 0x20000;
			WiiPlayerState* player = gGame.GetPlayer();
			player->mPlayerFlags   = 0;
			player->mPlayerFlags |= 0x40000000;
			player->mPadReset = FALSE;
			player->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else {
			mSpiderStatus.mFlags &= ~0x20000;
			WiiPlayerState* player = gGame.GetPlayer();
			player->mPlayerFlags   = 0;
			player->mPlayerFlags |= 0x20000000;
			player->mPlayerFlags |= 7;
			player->mPadReset = TRUE;
		}
		mCommand = 0;
		break;
	case 18:
		if (mArgs[0] == 1) {
			InfoScrn_ApplyScanlineHudCommand(&gGame.mInfoScrn, TRUE, 1);
		} else {
			InfoScrn_ApplyScanlineHudCommand(&gGame.mInfoScrn, FALSE, 1);
		}
		mCommand = 0;
		break;
	case 20:
		fn_8013F41C(4);
		mCommand = 0;
		break;
	case 22:
		fn_8013E788(2);
		mCommand = 0;
		break;
	}
	return 0;
}

/**
 * @note Address: 0x8013359C
 * @note Size: 0x1B8
 */
// NONMATCHING: the frame has scale above the hit copy and rot below it, ours hit, scale, rot
// a hit from the robot's blaster at level 2 or up: keeps it for UpdateSpiderBossDamage and bursts where it landed
void XObjSpiderBoss::OnHitFromPosition(const Vec& pos, f32 strength)
{
	u32 flags = mSpiderStatus.mFlags;
	if (flags & 2) {
		return;
	}
	if (flags & 0x10) {
		return;
	}
	if (flags & 0x8000) {
		return;
	}
	if (GetAttachmentCountById(&gGame.GetPlayer()->mAttachments, 2) < 2) {
		return;
	}
	XObj::OnHitFromPosition(pos, strength);
	mSpiderStatus.mFlags |= 2;
	Vec hit      = { pos.x, pos.y, pos.z };
	mHitPos      = hit;
	mHitStrength = strength;
	fn_800A12F8((EffectInstance*)&gGame.mEvtPtcl, 0xB3, FALSE);
	Vec scale              = { 1.0f, 1.0f, 1.0f };
	Vec rot                = { 0.0f, mTargetRotY, 0.0f };
	EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0xB3, 0);
	if (effect != NULL) {
		effect->pos   = pos;
		effect->rot   = rot;
		effect->scale = scale;
		UpdateEffectMtx(effect);
	}
	if (!IsWiiSfxPlaying(&gGame.mSound, 0x194)) {
		PlayWiiSoundEffect(&gGame.mSound, 0x194, 1.0f, 0.0f); // npc_spider_bakuhatsu
	}
}

/**
 * @note Address: 0x80133754
 * @note Size: 0x18
 */
s32 XObjSpiderBoss::OnPlayerContact(s32 unused, s32 value)
{
	mSpiderStatus.mFlags |= 1;
	return value;
}

/**
 * @note Address: 0x80134710
 * @note Size: 0x8C
 */
void XObjSpiderBoss::State0(u32 frames)
{
	mSpiderStatus.mFlags &= ~0x80000U;
	u32 flags = mSpiderStatus.mFlags;
	if (flags & 0x02000000) {
		return;
	}
	if (flags & 0x40000000) {
		mStep      = 0;
		mTimer     = 0;
		mPrevState = mState;
		mState     = 4;
		ResetPatrolArea();
	} else {
		mStep      = 0;
		mTimer     = 0;
		mPrevState = mState;
		mState     = 3;
		mStep      = 100;
	}
	mSpiderStatus.mFlags &= ~0x8000U;
}

/**
 * @note Address: 0x8013479C
 * @note Size: 0x238
 */
// gets back up, blinking
void XObjSpiderBoss::State1(u32 frames)
{
	Vec zero = { 0.0f, 0.0f, 0.0f }; // retail builds it and never reads it
	switch (mStep) {
	case 0:
		mSpiderStatus.mFlags |= 0x10;
		SetAnim(11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		++mStep;
		mBlinkTimer = 0;
		mSpiderStatus.mFlags &= ~0x200;
		break;
	case 1: {
		if (mBlinkTimer % 30 < 15) {
			SetColor(1, 128, 0, 0, 0.0f);
		} else {
			SetColor(1, 0, 0, 0, 0.0f);
		}
		++mBlinkTimer;
		f32 frame = GetAnimFrame();
		if (GetAnimEndFrame() - frames <= frame) {
			SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mBlinkTimer = 0;
			SetColor(1, 0, 0, 0, 0.0f);
			mStep      = 0;
			mTimer     = 0;
			mPrevState = mState;
			mState     = 0;
			mSpiderStatus.mFlags |= 0x8000;
			mSpiderStatus.mFlags |= 0x02000000;
			mSpiderStatus.mFlags |= 0x40000000;
			mSpiderStatus.mFlags &= ~0x10;
			mSpiderStatus.mFlags |= 0x80000;
			mHits = 16;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x801349D4
 * @note Size: 0x58
 */
void XObjSpiderBoss::State2(u32 frames)
{
	Vec zero = { 0.0f, 0.0f, 0.0f }; // retail builds it and never reads it
	switch (mStep) {
	case 0:
		mSpiderStatus.mFlags |= 0x80000;
		mSpiderStatus.mFlags |= 0x10;
		mSpiderStatus.mFlags &= ~0x200U;
		mSpiderStatus.mFlags &= ~0x100000U;
		++mStep;
		break;
	case 1:
		break;
	}
}

static inline f32 SpiderContactDistance(const Vec* a, const Vec* b)
{
	f32 dx = a->x - b->x;
	f32 dy = a->y - b->y;
	f32 dz = a->z - b->z;
	return FSqrt(dx * dx + dy * dy + dz * dz);
}

static inline void DrainPlayerBattery(f32 amount)
{
	f32 battery = gGame.GetPlayer()->mParams.mBattery - amount;
	if (battery < 0.0f) {
		battery = 0.0f;
	}
	gGame.GetPlayer()->mParams.SetBattery(battery);
}

/**
 * @note Address: 0x8013376C
 * @note Size: 0x9E4
 */
// NONMATCHING: call set matches; vector temporary lifetimes leave a 0x10 smaller frame and different FPR schedules
extern "C" void UpdateSpiderBossDamage(XObjSpiderBoss* spider, u32 frames)
{
	CPad pad(0);
	switch (spider->mDamageStep) {
	case 0:
		if ((spider->mSpiderStatus.mFlags & 2) && !(spider->mSpiderStatus.mFlags & 0x10)) {
			StopWiiSfx(&gGame.mSound, 0x197, 0.0f);
			spider->mSpiderStatus.mFlags &= ~2;
			if (spider->mHitStrength >= (s32)GetPlayerActionParameter(31) / 100.0f) {
				spider->mHits -= 3;
				if (spider->mHits <= 0)
					spider->mHits = 0;
			} else if (--spider->mHits <= 0)
				spider->mHits = 0;
			if (gGame.GetPlayer()->mParams.mBattery <= 30.0f && (spider->mSpiderStatus.mFlags & 0x40000000))
				spider->mHits = 0;
			if (!(spider->mSpiderStatus.mFlags & 0x40000000)) {
				if (spider->mHits < 22)
					fn_8013C7EC(0);
				if (spider->mHits < 19)
					fn_8013C7EC(1);
				if (spider->mHits < 16) {
					fn_8013C7EC(2);
					fn_8013C7EC(8);
				}
				if (spider->mHits < 13)
					fn_8013C7EC(3);
				if (spider->mHits < 11) {
					fn_8013C7EC(4);
					fn_8013C7EC(9);
				}
				if (spider->mHits < 9)
					fn_8013C7EC(5);
				if (spider->mHits < 6) {
					fn_8013C7EC(6);
					fn_8013C7EC(10);
				}
				if (spider->mHits < 3)
					fn_8013C7EC(7);
				if (spider->mHits < 1)
					fn_8013D0C8();
			}
			if (spider->mHits <= 0) {
				fn_8013F558();
				StopWiiSfx(&gGame.mSound, 0x476, 0.0f);
				spider->mSpeed = 0;
				if (spider->mSpiderStatus.mFlags & 0x40000000) {
					spider->SetState(2);
					spider->mSpiderStatus.mFlags |= 0x10;
				} else {
					spider->SetState(1);
				}
			} else {
				spider->mDamageStep = 1;
				spider->mBlinkTimer = 0;
				spider->mSpiderStatus.mFlags |= 0x10;
			}
		}
		break;
	case 1:
		if (spider->mBlinkTimer % 30 < 15)
			spider->SetColor(1, 0x80, 0, 0, 0.0f);
		else
			spider->SetColor(1, 0, 0, 0, 0.0f);
		spider->mBlinkTimer++;
		if (spider->mBlinkTimer > 20) {
			spider->mSpiderStatus.mFlags &= ~0x10;
			spider->SetColor(1, 0, 0, 0, 0.0f);
			spider->mBlinkTimer = 0;
			spider->mDamageStep = 0;
		}
		break;
	}
	if (spider->mSpiderStatus.mFlags & 0x200) {
		f32 heading, speed, x, z;
		Vec p = *gGame.GetPlayer()->GetPos();
		CVec position(p);
		speed = 0.4f * (220.0f - SpiderContactDistance(&position, &spider->mMoveTranslate));
		if (speed > 4.0f)
			speed = 4.0f;
		heading = PI + (f32)atan2(p.x - spider->mMoveTranslate.x, p.z - spider->mMoveTranslate.z);
		x       = speed * (f32)sin(heading);
		z       = speed * (f32)cos(heading);
		if (IsUsingMug(&gGame.GetPlayer()->mLift)) {
			gGame.GetPlayer()->_1C9A8.x = 0.0f;
			gGame.GetPlayer()->_1C9A8.y = 0.0f;
			gGame.GetPlayer()->_1C9A8.z = 0.0f;
		} else {
			gGame.GetPlayer()->_1C9A8.x = x;
			gGame.GetPlayer()->_1C9A8.y = 0.0f;
			gGame.GetPlayer()->_1C9A8.z = z;
		}
	} else {
		gGame.GetPlayer()->_1C9A8.x = 0.0f;
		gGame.GetPlayer()->_1C9A8.y = 0.0f;
		gGame.GetPlayer()->_1C9A8.z = 0.0f;
	}
	WiiPlayerState* player = gGame.GetPlayer();
	Vec ppos               = *player->GetPos();
	CVec pos(ppos);
	if (SpiderContactDistance(&pos, &spider->mMoveTranslate) < 28.0f && !IsPlayerFlashingFromHit(player)
	    && (spider->mSpiderStatus.mFlags & 0x1000) && !(spider->mSpiderStatus.mFlags & 0x80000))
		player->OnHitFromPosition(ppos, 4.0f);
	if (GetPlayerCollisionRadius(gGame.GetPlayer()) < 2.0f && (spider->mSpiderStatus.mFlags & 0x20000)) {
		if (--spider->mDrainTimer <= 0) {
			DrainPlayerBattery(4.0f);
			spider->mDrainTimer = 30;
			if (!gSpiderDrainFlashClaimed) {
				spider->mSpiderStatus.mFlags |= 0x1000000;
				gSpiderDrainFlashClaimed = 1;
			}
			if (spider->mSpiderStatus.mFlags & 0x1000000) {
				gSpiderDrainFlashTimers[0] = gRefreshRate;
				gSpiderDrainFlashTimers[1] = 0;
			}
		}
	}
	if (spider->mSpiderStatus.mFlags & 0x1000000) {
		ApplyPlayerHitReaction(player, FALSE, TRUE, 0.0f);
		if (gSpiderDrainFlashTimers[0] > 0) {
			if ((gSpiderDrainFlashTimers[0] -= frames) < 0) {
				gSpiderDrainFlashTimers[0] = 0;
			}
			gSpiderDrainFlashTimers[1]++;
			if (gSpiderDrainFlashTimers[0] > 0) {
				f32 red = 128.0f * (gSpiderDrainFlashTimers[0] / (f32)gRefreshRate);
				player->SetColor(1, red, 0, 0, 0.0f);
				StartScreenFade(&gGame.mScrnTex.mColor, red, 0, 0, 0, red, 0, 0, 0, 0.0f);
			} else {
				player->SetColor(1, 0, 0, 0, 0.0f);
				StartScreenFade(&gGame.mScrnTex.mColor, 0, 0, 0, 0, 100, 100, 200, 0, 0.0f);
			}
		}
	}
	if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && (spider->mSpiderStatus.mFlags & 0x20000)) {
		InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 0, 1);
		spider->mSpiderStatus.mFlags &= ~0x20000;
		WiiPlayerState* chr = gGame.GetPlayer();
		chr->mPlayerFlags   = 0;
		chr->mPlayerFlags |= 0x20000000;
		chr->mPlayerFlags |= 7;
		chr->mPadReset = TRUE;
	}
}

/**
 * @note Address: 0x80134150
 * @note Size: 0x5C0
 */
// NONMATCHING: call set and size match; saved-FPR allocation around the heading and matrix temporaries differs
extern "C" void UpdateSpiderBossAttachments(XObjSpiderBoss* spider, u32 frames)
{
	WiiPlayerState* player = gGame.GetPlayer();
	CPad pad(0);
	if ((spider->mSpiderStatus.mFlags & 0x20000) && (player->mPlayerFlags & 0x80000000)) {
		CVec position = spider->GetJointPos(spider->FindResNode("cyl3"));
		f32 radius    = GetPlayerCollisionRadius(gGame.GetPlayer());
		position.y -= 6.0f;
		const Vec* spiderPos = spider->GetPos();
		const Vec* playerPos = player->GetPos();
		f32 heading          = PI + (f32)atan2(spiderPos->x - playerPos->x, spiderPos->z - playerPos->z);
		position.x += (radius + 0.5f) * (f32)sin(heading);
		position.z += (radius + 0.5f) * (f32)cos(heading);
		player->SetPos(position);
		if (gGame.IsRunning() && player->_1C9B8[2] >= 1.0f) {
			if (player->GetAttachment() == 2) {
				player->SetAnim(0x161, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (player->mLift.mState == 4 && (IsLiftTargetPlug(&player->mLift) || IsCarryingMug(&player->mLift))) {
				player->SetAnim(0x162, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (player->mLift.mState == 4
			           && (IsCarryingToothbrush(&player->mLift) || IsCarryingSpoon(&player->mLift) || IsCarryingSquirter(&player->mLift))) {
				player->SetAnim(0x163, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else {
				player->SetAnim(0x160, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
			heading = gGame.mMotionControlCamera.mView.mAngle.y + 2.0f * (PI * player->_1C9C4[0]);
		} else if (radius < 2.0f)
			player->SetAnim(0xE6, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		else
			player->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		player->SetRotateY(heading, 0.0f);
	}
	if (spider->mSpiderStatus.mFlags & 0x20000000) {
		for (u32 i = 0; i < 8; ++i) {
			SpiderTie& part = spider->mParts[i];
			if (part.mObj == NULL)
				continue;
			Mtx scaling, translation, result, mount;
			PSMTXCopy(spider->GetWorldMtx(part.mJoint), mount);
			Vec scale;
			HSD_MtxGetScale(mount, &scale);
			scale.x = GetStagePlacementScale(part.mObj).x / scale.x;
			scale.y = GetStagePlacementScale(part.mObj).y / scale.y;
			scale.z = GetStagePlacementScale(part.mObj).z / scale.z;
			PSMTXScale(scaling, scale.x, scale.y, scale.z);
			PSMTXConcat(mount, scaling, result);
			f32 y, z;
			z     = part.mObj->GetPos()->z;
			y     = part.mObj->GetPos()->y;
			f32 x = part.mObj->GetPos()->x;
			PSMTXTrans(translation, x, y, z);
			PSMTXConcat(result, translation, result);
			part.mObj->SetBaseMtx(result);
		}
	}
}

static inline void SetAttackAnim(XObjSpiderBoss* boss, s32 id, BOOL restart = FALSE)
{
	boss->SetAnim(id, restart, 0.0f, 1.0f, 0.0f, 15.0f);
}

static inline BOOL AttackAnimEnding(XObjSpiderBoss* boss, u32 frames)
{
	f32 now = boss->GetAnimFrame();
	return boss->GetAnimEndFrame() - frames <= now;
}

static inline void ShakeSpiderArena()
{
	if (!gGame.GetStatusFlags(0x100))
		StartScreenShake(&gGame.mQuake, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f);
}

static inline void PulseSpiderArena(XObjSpiderBoss* boss)
{
	if ((s32)--boss->mActionTimer < 0) {
		boss->mActionTimer = rand() % 60 + 60;
		ShakeSpiderArena();
		fn_8013E788(0);
	}
}

static inline bool IsGrabBlocked(s32 anim)
{
	switch (anim) {
	case 5:
	case 6:
	case 7:
	case 9:
	case 10:
	case 11:
	case 0x27:
	case 0x28:
	case 0x43:
	case 0xA1:
	case 0xED:
	case 0x158:
	case 0x159:
	case 0x814:
	case 0x816:
	case 0x817:
	case 0x81F:
	case 0x820:
	case 0x824:
	case 0x879:
	case 0x87A:
		return true;
	}
	return false;
}

/**
 * @note Address: 0x80134A2C
 * @note Size: 0x1880
 */
// NONMATCHING: call set and frame match; AttackAnimEnding materializes bool, some inline FPR schedules differ, and
// StartScreenShake gets lfd 0.0/3.0 where retail has lfs (the f64 prototype in game/cscrn.h, see xobjspider.cpp)
void XObjSpiderBoss::State3(u32 frames)
{
	CPad pad(0);
	Vec player = *gGame.GetPlayer()->GetPos();
	CVec position(player);
	switch (mStep) {
	case 100:
		mTimer = 3 * gRefreshRate / frames;
		SetAttackAnim(this, 7);
		mStep = 101;
		break;
	case 101: {
		mHeading  = atan2(position.x - mMoveTranslate.x, position.z - mMoveTranslate.z);
		f32 delta = mTargetRotY - mHeading;
		if (delta > PI)
			mHeading = TWO_PI + mHeading;
		else if (delta < -PI)
			mHeading -= TWO_PI;
		ApproachHudValue(&mTargetRotY, &mHeading, .018f);
		if (mHeading > mTargetRotY) {
			if (AttackAnimEnding(this, frames))
				SetAttackAnim(this, 9, TRUE);
			else
				SetAttackAnim(this, 9);
		} else if (mHeading < mTargetRotY) {
			if (AttackAnimEnding(this, frames))
				SetAttackAnim(this, 8, TRUE);
			else
				SetAttackAnim(this, 8);
		} else
			SetAttackAnim(this, 7);
	}
		if (--mTimer < 0)
			mStep = 0;
		break;
	case 0:
		mTimer       = rand() % 90 + 90;
		mActionTimer = rand() % 30 + 30;
		SetAttackAnim(this, 7);
		++mStep;
		break;
	case 1:
		mGrabIntervalFrames += frames;
		{
			mHeading  = atan2(position.x - mMoveTranslate.x, position.z - mMoveTranslate.z);
			f32 delta = mTargetRotY - mHeading;
			if (delta > PI)
				mHeading = TWO_PI + mHeading;
			else if (delta < -PI)
				mHeading -= TWO_PI;
			ApproachHudValue(&mTargetRotY, &mHeading, .018f);
			if (mHeading > mTargetRotY) {
				if (AttackAnimEnding(this, frames))
					SetAttackAnim(this, 9, TRUE);
				else
					SetAttackAnim(this, 9);
			} else if (mHeading < mTargetRotY) {
				if (AttackAnimEnding(this, frames))
					SetAttackAnim(this, 8, TRUE);
				else
					SetAttackAnim(this, 8);
			} else
				SetAttackAnim(this, 7);
		}
		if (SpiderContactDistance(&position, &mMoveTranslate) <= 220) {
			if (mGrabIntervalFrames >= (s32)(3 * gRefreshRate)) {
				mStep = 10;
				SetAttackAnim(this, 16);
				GetJointPos(FindResNode("cyl3"));
			}
			PulseSpiderArena(this);
		} else if (--mTimer < 0) {
			mTimer = 30;
			mStep  = 20;
			ShakeSpiderArena();
			fn_8013E788(0);
		}
		break;
	case 10:
		if (AttackAnimEnding(this, frames)) {
			fn_8013F41C(1);
			mTimer = gRefreshRate / frames;
			SetAttackAnim(this, 17);
			mGrabCooldown = 5;
			++mStep;
		}
		break;
	case 11: {
		mHeading  = atan2(position.x - mMoveTranslate.x, position.z - mMoveTranslate.z);
		f32 delta = mTargetRotY - mHeading;
		if (delta > PI)
			mHeading = TWO_PI + mHeading;
		else if (delta < -PI)
			mHeading -= TWO_PI;
		ApproachHudValue(&mTargetRotY, &mHeading, .018f);
		if (mHeading > mTargetRotY) {
			if (AttackAnimEnding(this, frames))
				SetAttackAnim(this, 18, TRUE);
			else
				SetAttackAnim(this, 18);
		} else if (mHeading < mTargetRotY) {
			if (AttackAnimEnding(this, frames))
				SetAttackAnim(this, 19, TRUE);
			else
				SetAttackAnim(this, 19);
		} else
			SetAttackAnim(this, 17);
	}
		if (--mTimer < 0) {
			mTimer       = 3 * gRefreshRate / frames + (gRefreshRate / frames) / 2;
			mActionTimer = rand() % 30 + 30;
			GetJointPos(FindResNode("cyl3"));
			fn_8013F41C(2);
			PlayWiiSoundEffect(&gGame.mSound, 0x476, 1.0f, 0.0f);
			++mStep;
		}
		break;
	case 12: {
		{
			mHeading  = atan2(position.x - mMoveTranslate.x, position.z - mMoveTranslate.z);
			f32 delta = mTargetRotY - mHeading;
			if (delta > PI)
				mHeading = TWO_PI + mHeading;
			else if (delta < -PI)
				mHeading -= TWO_PI;
			ApproachHudValue(&mTargetRotY, &mHeading, .018f);
			if (mHeading > mTargetRotY) {
				if (AttackAnimEnding(this, frames))
					SetAttackAnim(this, 18, TRUE);
				else
					SetAttackAnim(this, 18);
			} else if (mHeading < mTargetRotY) {
				if (AttackAnimEnding(this, frames))
					SetAttackAnim(this, 19, TRUE);
				else
					SetAttackAnim(this, 19);
			} else
				SetAttackAnim(this, 17);
		}
		if (--mGrabCooldown < 0)
			mGrabCooldown = 0;
		if (IsGrabBlocked(gGame.GetPlayer()->GetAnimId()))
			mGrabCooldown = 5;
		f32 distance = SpiderContactDistance(&position, &mMoveTranslate);
		if (distance <= 70 && !IsUsingMug(&gGame.GetPlayer()->mLift) && !IsGrabBlocked(gGame.GetPlayer()->GetAnimId())
		    && gGame.GetPlayer()->mAttachmentRequest < 0 && mGrabCooldown == 0) {
			mStatus.mFlags |= 0x100000;
			ShakeSpiderArena();
			mSpiderStatus.mFlags |= 0x20000;
			WiiPlayerState* chr = gGame.GetPlayer();
			chr->mPlayerFlags   = 0;
			chr->mPlayerFlags |= 0x80000000;
			chr->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			fn_800FCD1C(&chr->mLift);
			fn_800FCDF0(&chr->mLift);
			fn_800FCE60(&chr->mLift);
			ResetPlayerPopperAction(&chr->mPopper);
			fn_8010C0F4(&chr->mRadar);
			FinishTumbleAction(&chr->mTumble);
			mEscapeInputStrength = 1;
			mEscapeRemaining     = 450;
			++mStep;
			InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 1, 1);
			mSpiderStatus.mFlags &= ~0x200;
			PlayWiiSoundEffect(&gGame.mSound, 0x474, 1.0f, 0.0f);
			PlayWiiSoundEffect(&gGame.mSound, 0x475, 1.0f, 0.0f);
			break;
		}
		if (distance <= 220) {
			mSpiderStatus.mFlags |= 0x200;
			PulseSpiderArena(this);
		} else {
			mGrabIntervalFrames = 0;
			SetAttackAnim(this, 7);
			mSpiderStatus.mFlags &= ~0x200;
			StopWiiSfx(&gGame.mSound, 0x476, 0.0f);
			fn_8013F558();
			StopWiiSfx(&gGame.mSound, 0x476, 0.0f);
			mStep = 0;
		}
		if (--mTimer < 0) {
			mGrabIntervalFrames = 0;
			SetAttackAnim(this, 7);
			mSpiderStatus.mFlags &= ~0x200;
			mStep = 0;
			fn_8013F558();
			StopWiiSfx(&gGame.mSound, 0x476, 0.0f);
		}
		break;
	}
	case 13:
		if (gGame.IsRunning() && gGame.GetPlayer()->_1C9B8[2] >= 1.0f) {
			mEscapeRemaining -= mEscapeInputStrength;
			f32 strength = 0.0f;
			f32 remote   = nw4r::math::FAbs(gWiiPointer.mRemoteAcceleration[0].y - gWiiPointer.mRemoteAcceleration[1].y);
			if (remote > 1.0f) {
				strength += 3.0f * remote;
			}
			f32 nunchuk = nw4r::math::FAbs(gWiiPointer.mNunchukAcceleration[0].y - gWiiPointer.mNunchukAcceleration[1].y);
			if (nunchuk > 1.0f) {
				strength += 3.0f * nunchuk;
			}
			if (strength > 0.0f) {
				mEscapeInputStrength = strength;
			}
		}
		if (mEscapeInputStrength > 1)
			mEscapeInputStrength -= 1;
		if (mEscapeRemaining <= 0) {
			if (mSpiderStatus.Check(0x1000000)) {
				mSpiderStatus.mFlags &= ~0x1000000;
				gSpiderDrainFlashClaimed = 0;
				gGame.GetPlayer()->SetColor(1, 0, 0, 0, 0.0f);
			}
			mStatus.mFlags &= ~0x100000ULL;
			mSpiderStatus.mFlags &= ~0x20000;
			WiiPlayerState* chr = gGame.GetPlayer();
			chr->mPlayerFlags   = 0;
			chr->mPlayerFlags |= 0x20000000;
			chr->mPlayerFlags |= 7;
			chr->mPadReset      = TRUE;
			mGrabIntervalFrames = 0;
			SetAttackAnim(this, 7);
			mStep = 0;
			InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 0, 1);
			fn_8013F558();
			StopWiiSfx(&gGame.mSound, 0x476, 0.0f);
		}
		if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
			SetAttackAnim(this, 7);
			mStep = 30;
			fn_8013F558();
			StopWiiSfx(&gGame.mSound, 0x476, 0.0f);
		}
		break;
	case 20:
		if (--mTimer < 0)
			mStep = 0;
		break;
	case 30:
		if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && gGame.IsRunning() && !gGame.mEvt.mData.mScript)
			mStep = 0;
		break;
	}
}

static inline void PlayPatrolAnim(XObjSpiderBoss* boss, s32 id)
{
	boss->SetAnim(id, TRUE, 0.0f, 1.0f, 1.0f, 10.0f);
	boss->mObject.mDefaultTexPat = "sp3_wait3";
	boss->mObject.mTexPatName    = NULL;
	boss->mAltAnimId             = id;
}

static inline void FacePatrolPoint(XObjSpiderBoss* boss, const Vec& target, s32 turningStep, s32 readyStep)
{
	boss->mHeading = atan2(target.x - boss->mMoveTranslate.x, target.z - boss->mMoveTranslate.z);
	f32 delta      = boss->mHeading - boss->mTargetRotY;
	if (delta < 0) {
		delta += TWO_PI;
	}
	if (delta < PI) {
		PlayPatrolAnim(boss, 0x1A);
		boss->mStep = turningStep;
	} else if (delta > PI) {
		PlayPatrolAnim(boss, 0x19);
		boss->mStep = turningStep;
	} else {
		PlayPatrolAnim(boss, 0x17);
		boss->mStep = readyStep;
	}
}

static inline void TurnTowardPoint(XObjSpiderBoss* boss, s32 readyStep)
{
	f32 diff = boss->mTargetRotY - boss->mHeading;
	if (diff > PI) {
		boss->mHeading = TWO_PI + boss->mHeading;
	} else if (diff < -PI) {
		boss->mHeading -= TWO_PI;
	}
	ApproachHudValue(&boss->mTargetRotY, &boss->mHeading, 0.12f);
	if (boss->mTargetRotY == boss->mHeading) {
		PlayPatrolAnim(boss, 0x17);
		boss->mStep = readyStep;
	}
}

static const Vec sSpiderBossPatrolPoints[]
    = { { 0, 0, 0 },      { 80, 0, 0 },     { 160, 0, 0 },    { -80, 0, 0 },    { -160, 0, 0 },  { 0, 0, 80 },     { 80, 0, 80 },
	    { 160, 0, 80 },   { -80, 0, 80 },   { -160, 0, 80 },  { 0, 0, 160 },    { 80, 0, 160 },  { 120, 0, 120 },  { -80, 0, 160 },
	    { -120, 0, 120 }, { 0, 0, -80 },    { 80, 0, -80 },   { 160, 0, -80 },  { -80, 0, -80 }, { -160, 0, -80 }, { 0, 0, -160 },
	    { 80, 0, -160 },  { 120, 0, -120 }, { -80, 0, -160 }, { -120, 0, -120 } };

static inline void StartPatrolEffect(XObjSpiderBoss* boss)
{
	Vec rot                = { 0.0f, boss->mTargetRotY, 0.0f };
	Vec scale              = { 1.0f, 1.0f, 1.0f };
	EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0xB6, 0);
	if (effect != NULL) {
		effect->pos   = boss->mMoveTranslate;
		effect->rot   = rot;
		effect->scale = scale;
		UpdateEffectMtx(effect);
	}
	boss->mPatrolEffectActive = 1;
}

static inline void StopPatrolEffect(XObjSpiderBoss* boss)
{
	boss->mPatrolEffectActive = 0;
	fn_800A12F8((EffectInstance*)&gGame.mEvtPtcl, 0xB6, FALSE);
}

/**
 * @note Address: 0x801362AC
 * @note Size: 0xD18
 */
// NONMATCHING: patrol-table/effect temporaries use another register and stack schedule; the CPad vtable referent differs
void XObjSpiderBoss::State4(u32 frames)
{
	CPad pad(0);
	CVec player = *gGame.GetPlayer()->GetPos();
	if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && mStep != 30) {
		PlayPatrolAnim(this, 0x17);
		mSpeed = 0.0f;
		mStep  = 30;
	}
	switch (mStep) {
	case 0: {
		mTimer = (rand() % 30 + 30) / frames;
		mSpeed = 0.0f;
		PlayPatrolAnim(this, 0x17);
		mSpiderStatus.mFlags |= 0x1000;
		Vec candidates[25];
		Vec* next = candidates;
		for (s32 i = 0; i < 25; ++i) {
			if (mPatrolSelection != i) {
				*next++ = sSpiderBossPatrolPoints[i];
			}
		}
		s32 pick         = rand() % 24;
		mPatrolTarget    = candidates[pick];
		mPatrolSelection = pick;
		FacePatrolPoint(this, mPatrolTarget, 1, 2);
		break;
	}
	case 1:
		TurnTowardPoint(this, 2);
		break;
	case 2:
		if (--mTimer < 0) {
			mSpeed = 1.0f;
			PlayPatrolAnim(this, 0x18);
			StartPatrolEffect(this);
			mTimer = rand() % 30 + 30;
			++mStep;
		}
		break;
	case 3:
		if (mAltAnimId == 0x18) {
			if (!mPatrolEffectActive) {
				StartPatrolEffect(this);
			}
			if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
				StopPatrolEffect(this);
			}
		}
		mHeading    = atan2(mPatrolTarget.x - mMoveTranslate.x, mPatrolTarget.z - mMoveTranslate.z);
		mTargetRotY = mHeading;
		if (SpiderContactDistance(&mPatrolTarget, &mMoveTranslate) < 2.0f) {
			StopPatrolEffect(this);
			PlayPatrolAnim(this, 0x17);
			mSpeed       = 0.0f;
			mActionTimer = 5;
			mStep        = 4;
		}
		break;
	case 4:
		FacePatrolPoint(this, player, 5, 6);
		break;
	case 5:
		TurnTowardPoint(this, 6);
		break;
	case 6:
		if ((s32)--mActionTimer < 0) {
			mStep = 0;
		} else {
			PlayPatrolAnim(this, 0x1B);
			fn_8013E788(1);
			PlayWiiSoundEffect(&gGame.mSound, 0x472, 1.0f, 0.0f);
			++mStep;
		}
		break;
	case 7:
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			mStep = 4;
		}
		break;
	case 30:
		if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && gGame.IsRunning() && !gGame.mEvt.mData.mScript) {
			mStep = 0;
		}
		break;
	}
}

/**
 * @note Address: 0x80136FC4
 * @note Size: 0xC
 */
extern "C" bool IsSpiderBossProjectileHitSuppressed(XObjSpiderBoss* spider)
{
	return (spider->mSpiderStatus.mFlags & 0x200) != 0;
}

/**
 * @note Address: 0x80136FD0
 * @note Size: 0xBC
 */
void XObjSpiderBoss::UpdateAnimationSounds()
{
	if (mStatus.mFlags & 1ULL) {
		if (mStatus.mFlags & 0x10000ULL) {
			if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
				s32 id = (mSpiderStatus.mFlags & 0x40000000) ? mAltAnimId : GetAnimId();
				mAnimSfx.Update(id, GetAnimFrame(), 255.0f);
			}
		}
	}
}

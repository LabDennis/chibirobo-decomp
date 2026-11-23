#include "game/xobjspiderboss.h"
#include "game/cgameparticle.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/globals.h"
#include "game/video.h"
#include "game/gmath.h"
#include "game/spidercontact.h"
#include "game/infoscrn.h"
#include "game/hsd_cobj.h"
#include "game/cparticle.h"
#include "game/placement.h"
#include "game/animframe.h"
#include <stdlib.h>

extern "C" {
void LaunchSpiderBossPartByIndex(s32 index);
void LaunchSpiderBossCenterPart();
void ResetSpiderBossAttachment();
BOOL IsUsingMug(XActLift* lift);
BOOL IsPlayerFlashingFromHit(XObjChr* player);
f32 GetPlayerCollisionRadius(XObjChr* player);
void ApplyPlayerHitReaction(XObjChr* player, BOOL react, BOOL stop, f32 time);
void HSD_MtxGetScale(Mtx matrix, Vec* scale);
void OSReport(const char*, ...);
void EnableSpiderBossPartByIndex(s32);
void EnableSpiderBossCenterPart();
void PlaySpiderBossAttachmentAnimation(s32);
void CommandSpiderBossProjectiles(s32);
void ApproachHudValue(f32*, f32*, f32);
}

extern "C" {
s32 gSpiderBossStateData[26]           = { 0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 3, 0 };
static XObjFloor sSpiderBossLowFloor[] = {
	{ 0, 0, { 0, 135, 0 }, 0, { 377, 158, 377 } },
	{ -1 },
};
static XObjFloor sSpiderBossHighFloor[] = {
	{ 0, 0, { 0, 135, 0 }, 0, { 377, 632, 377 } },
	{ -1 },
};

AnimSfxEvent gSpiderBossAnimSfxEvents[] = {
	{ 2, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 8, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 9, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 15, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 14, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 24, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 26, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 25, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 2, 40.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ 8, 15.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ 9, 15.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ 15, 15.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ 14, 15.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ 24, 20.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ -1 },
};
}

XObjSpiderBoss::StateFunc gSpiderBossStateHandlers[5] = {
	&XObjSpiderBoss::State0, &XObjSpiderBoss::State1, &XObjSpiderBoss::State2, &XObjSpiderBoss::State3, &XObjSpiderBoss::State4,
};

u32 gSpiderBossCoinBounceContext; // the boss's coin bounce token, the callbacks never touch it; its real type is unknown

extern "C" {
f32 gSpiderBossSavedTargetYaw;
}

Vec gSpiderBossPosition, gSpiderBossScale, gSpiderBossRotation;

#if defined(VERSION_PAL)
#define BOSS_FPS ((u32)gVideoFramesPerSecond)
#else
#define BOSS_FPS 60U
#endif

// todo: cleanup
static inline void ResetState(XObjSpiderBoss* spider)
{
	ResetSpiderBossState(spider);
	spider->mStep      = 0;
	spider->mTimer     = 0;
	spider->mPrevState = spider->mState;
	spider->mState     = 0;
	spider->mSpiderStatus.mFlags |= 0x04000000;
}

/**
 * @note Address: 0x80106924
 * @note Size: 0x58
 */
void XObjSpiderBoss::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	ResetState(this);
}

/**
 * @note Address: 0x801068D4
 * @note Size: 0x50
 */
void XObjSpiderBoss::ResetForNewGame()
{
	ResetState(this);
}

/**
 * @note Address: 0x8010660C
 * @note Size: 0x2C8
 */
extern "C" void ResetSpiderBossState(XObj* object)
{
	XObjSpiderBoss* spider       = (XObjSpiderBoss*)object;
	spider->mHeading             = 0.0f;
	spider->mSpeed               = 0.0f;
	spider->mTargetRotY          = 0.0f;
	spider->mRotSpeed            = 0.1f;
	spider->mSpeedY              = 0.0f;
	spider->_9A0                 = 0.0f;
	spider->_9A4                 = 0.0f;
	spider->mState               = 0;
	spider->mPrevState           = 0;
	spider->mStep                = 0;
	spider->mTimer               = 0;
	spider->mSpiderStatus.mFlags = 0;
	spider->mActionTimer         = 0;
	spider->_9C4                 = 1.0f;
	spider->_9C8                 = 1.0f;
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
	spider->_A44                 = 0;
	spider->_994                 = 1.0f;
	spider->mStatus.mFlags |= 8ULL;
	spider->mRadius    = 28.0f;
	Vec areaPos        = { 0.0f, 0.0f, 0.0f };
	spider->mAreaPos   = areaPos;
	spider->mAreaWidth = 0.0f;
	spider->mAreaDepth = 0.0f;
	Vec startPos       = { 0.0f, 0.0f, 0.0f };
	spider->mStartPos  = startPos;
	spider->_A1C       = 0.0f;
	spider->SetColor(1, 0, 0, 0, 0.0f);
	spider->_A48                 = 0.0f;
	spider->mEscapeInputStrength = 0.0f;
	spider->mDrainTimer          = 0;
	spider->mEscapeRemaining     = 0.0f;
	Vec vecA58                   = { 0.0f, 0.0f, 0.0f };
	spider->_A58                 = vecA58;
#if defined(VERSION_PAL)
	spider->mGrabIntervalFrames = 3 * gVideoFramesPerSecond; // 3 seconds
#else
	spider->mGrabIntervalFrames = 180;
#endif
	spider->ResetPatrolArea();
	spider->mDamageStep = 0;
	spider->mBlinkTimer = 0;
	for (u32 i = 0; i < 8; ++i) {
		spider->mParts[i].mObj   = NULL;
		spider->mParts[i].mJoint = NULL;
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
 * @note Address: 0x80106088
 * @note Size: 0x584
 */
// NONMATCHING: retail reads the three start Vecs at +0xD4 of the file's .bss: the whole .bss (0x140, JP 0x134) is the
// header-made prefix every file starts with, which no header here makes yet, so every load has another offset. MWCC makes
// a zeroed 0xC .bss object for each Vec() it parses, also in an inline nobody calls, so the prefix is most likely the
// Vec() defaults of about 26 header inlines; the three placement ones (Vec pos = Vec(); pos.x = ...) are followed by six
// more (JP five)
extern "C" void ApplySpiderBossPlacement(XObjSpiderBoss* spider)
{
	spider->SetColor(1, 0, 0, 0, 0.0f);
	spider->mMoveTranslate.x = PlacementPosition(spider->mCoord, gSpiderBossPosition).x;
	spider->mMoveTranslate.y = PlacementPosition(spider->mCoord, gSpiderBossPosition).y;
	spider->mMoveTranslate.z = PlacementPosition(spider->mCoord, gSpiderBossPosition).z;
	spider->mBasePos.x       = PlacementPosition(spider->mCoord, gSpiderBossPosition).x;
	spider->mBasePos.y       = PlacementPosition(spider->mCoord, gSpiderBossPosition).y;
	spider->mBasePos.z       = PlacementPosition(spider->mCoord, gSpiderBossPosition).z;
	spider->mHeading         = PlacementRotation(spider->mCoord, gSpiderBossRotation).y;
	spider->mRadius          = 28.0f * PlacementScale(spider->mCoord, gSpiderBossScale).x;
	spider->mStep            = 0;
	spider->mTimer           = 0;
	spider->mPrevState       = spider->mState;
	spider->mState           = 0;
	spider->mSpiderStatus.mFlags |= 0x8000;
	spider->mSpiderStatus.mFlags |= 0x02000000;
}

/**
 * @note Address: 0x80106038
 * @note Size: 0x50
 */
BOOL XObjSpiderBoss::LoadFromPlacement(const CoordInfo& info)
{
	if (LoadFromPlacement__4XObjFRC9CoordInfo(this, &info)) {
		ResetSpiderBossState(this);
		ApplySpiderBossPlacement(this);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80106000
 * @note Size: 0x38
 */
void XObjSpiderBoss::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	mAnimSfx.mEvents = gSpiderBossAnimSfxEvents;
}

// todo: cleanup
static inline void SetPosAndTarget(CObjVec* vec, const Vec& value)
{
	vec->Set(value);
	vec->SetTarget(value);
}

/**
 * @note Address: 0x80105D58
 * @note Size: 0x2A8
 */
// UpdateObject, the update
void XObjSpiderBoss::UpdateObject(u32 frames)
{
	CPad pad(0); // retail does this
	if (gGame.mStatus.mFlags & 0x400ULL) {
		Vec& move = gGame.GetPlayer()->_158C4;
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
		SetPosAndTarget(&mMoveTranslate, mMoveTranslate + step * (f32)frames);
		SetPos(mMoveTranslate, 0);
	}
	UpdateObject__4XObjFUl(this, frames);
	UpdateSpiderBossAttachments(this, frames);
}

/**
 * @note Address: 0x80105D28
 * @note Size: 0x30
 */
void XObjSpiderBoss::DrawObject()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x80105D08
 * @note Size: 0x20
 */
void XObjSpiderBoss::DrawGroundShadow()
{
	DrawGroundShadow__4XObjFv(this);
}

/**
 * @note Address: 0x80105CE8
 * @note Size: 0x20
 */
void XObjSpiderBoss::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	DrawGroundShadowAt__4XObjFPC3Vecf(this, origin, opacity);
}

/**
 * @note Address: 0x80105CE4
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawScreenOverlay()
{
}

/**
 * @note Address: 0x80105CE0
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawDebugStatus()
{
}

/**
 * @note Address: 0x80105CDC
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawObjectEffects()
{
}

/**
 * @note Address: 0x8010535C
 * @note Size: 0x980
 */
s32 XObjSpiderBoss::HandleEventCommand(s32 count, s32* args)
{
	switch (args[0]) {
	case 100: { // 0 releases every carried part, else attach at a joint
		s32 id = args[1];
		if (id == 0) {
			mSpiderStatus.mFlags &= ~0x20000000;
			for (u32 i = 0; i < 8; i++) {
				if (mParts[i].mObj != NULL) {
					HSD_JObjClearMtxFlags(mParts[i].mObj->mJObj);
					mParts[i].mObj   = NULL;
					mParts[i].mJoint = NULL;
				}
			}
		} else {
			u32 i;
			for (i = 0; mParts[i].mObj != NULL && i < 8; i++) { }
			if (i >= 8)
				break;
#if !defined(VERSION_PAL)
			OSReport("id = %d, argv[1]=%d\n", i, id);
#endif
			mSpiderStatus.mFlags |= 0x20000000;
			if (args[1] >= 10000) { // a placement id vs a slot index
				mParts[i].mObj = FindObj(args[1]);
			} else {
				mParts[i].mObj = gGame.GetObject((s16)args[1]);
			}
#if !defined(VERSION_PAL)
			if (mParts[i].mObj == NULL) {
				OSReport("NULL===============\n");
			}
#endif
			s32 path[8];
			s32 depth = args[2];
			for (s32 j = 0; j < depth; j++) {
				path[j] = args[3 + j];
			}
			mParts[i].mJoint = GetJointByPath(mJObj, path, depth);
			XObjJointPart::UpdateMatrix(*this, i);
		}
		return 0;
	}
	default: // anything else is stored for the state machine
		if (count < 1)
			return 0;
		mCommand = args[0];
		if (count < 2)
			mArgs[0] = 0;
		else
			mArgs[0] = args[1];
		if (count < 3)
			mArgs[1] = 0;
		else
			mArgs[1] = args[2];
		if (count < 4)
			mArgs[2] = 0;
		else
			mArgs[2] = args[3];
		if (count < 5)
			mArgs[3] = 0;
		else
			mArgs[3] = args[4];
		if (count < 6)
			mArgs[4] = 0;
		else
			mArgs[4] = args[5];
		break;
	}
	switch (mCommand) {
	case 1:
		mStep      = 0;
		mTimer     = 0;
		mPrevState = mState;
		mState     = 2;
		mSpiderStatus.mFlags &= ~0x02000000;
		mCommand = 0;
		break;
	case 2:
		if (mSpiderStatus.mFlags & 0x10000)
			gGame.mEvt.mSelectResult = 1;
		else
			gGame.mEvt.mSelectResult = 0;
		mCommand = 0;
		break;
	case 3:
		if (mSpiderStatus.mFlags & 0x80000)
			gEventVars.mVars[mArgs[0]] = 1;
		else
			gEventVars.mVars[mArgs[0]] = 0;
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
		if (mArgs[0] == 1)
			mSpiderStatus.mFlags &= ~0x02000000;
		else
			mSpiderStatus.mFlags |= 0x02000000;
		mCommand = 0;
		mArgs[0] = 0;
		break;
	case 8:
		mCommand = 0;
		break;
	case 9: // set the base position, script units are 1/100
		mBasePos.x       = mArgs[0] / 100.0f;
		mBasePos.y       = mArgs[1] / 100.0f;
		mBasePos.z       = mArgs[2] / 100.0f;
		mStartPos        = mBasePos;
		mMoveTranslate.x = mBasePos.x;
		mMoveTranslate.y = mBasePos.y;
		mMoveTranslate.z = mBasePos.z;
		mMoveTranslate.SetTarget(mMoveTranslate);
		mCommand = 0;
		break;
	case 10: // the area it walks in: corner in 1/100 units, width and depth as given
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
	case 16: // re-enable the eleven item-0xB actors and the center (clears their flag 4)
		for (int i = 0; i < 11; ++i)
			EnableSpiderBossPartByIndex(i);
		EnableSpiderBossCenterPart();
		mCommand = 0;
		break;
	case 17: // 1 hands the player to the script, else gives control back
		if (mArgs[0] == 1) {
			mSpiderStatus.mFlags |= 0x20000;
			XObjChr* player = gGame.GetPlayer();
			player->LockControl();
			gGame.GetPlayer()->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else {
			mSpiderStatus.mFlags &= ~0x20000;
			XObjChr* player = gGame.GetPlayer();
			player->RestoreControl();
		}
		mCommand = 0;
		break;
	case 18:
		if (mArgs[0] == 1)
			InfoScrn_ApplyScanlineHudCommand(&gGame.mInfoScrn, TRUE, 1);
		else
			InfoScrn_ApplyScanlineHudCommand(&gGame.mInfoScrn, FALSE, 1);
		mCommand = 0;
		break;
	case 20:
		PlaySpiderBossAttachmentAnimation(4);
		mCommand = 0;
		break;
	case 22:
		CommandSpiderBossProjectiles(2);
		mCommand = 0;
		break;
	}
	return 0;
}

/**
 * @note Address: 0x801051B4
 * @note Size: 0x1A8
 */
// hit from pos, only once chibi robo has the item that does it
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
	XObjChr* player = gGame.GetPlayer();
	if (GetAttachmentCountById(&player->mAttachments, 2) < 2) {
		return;
	}
	OnHitFromPosition__4XObjFRC3Vecf(this, &pos, strength);
	mSpiderStatus.mFlags |= 2;
	Vec hit             = { pos.x, pos.y, pos.z };
	mHitPos             = hit;
	mHitStrength        = strength;
	CParticle* particle = &gGame.mParticle;
	particle->Kill(0xB3);
	// NONMATCHING: retail builds rot straight in CreateSRT's by-value argument slot, this compiler
	// builds the local and copies it (same in states34, mamahide, xobjactor3 and evt; the (Vec){...} and
	// Vec(...) argument forms do not change it); retail's .rodata also has { -PI/2, 0, 0 } in front of these
	// three Vecs, which no code reads, so they sit 0xC lower here
	Vec scale = { 1.0f, 1.0f, 1.0f };
	Vec rot   = { 0.0f, mTargetRotY, 0.0f };
	particle->CreateSRT(0xB3, 1, pos, scale, rot, TRUE);
	XSound* sound = &gGame.mSound;
	if (!IsGameSoundPlaying(sound, 0x127)) {
		PlayGameSound(sound, 0x127, 255); // npc_spider_bakuhatsu
	}
}

/**
 * @note Address: 0x801051A0
 * @note Size: 0x14
 */
s32 XObjSpiderBoss::OnPlayerContact(s32 unused, s32 value)
{
	mSpiderStatus.mFlags |= 1;
	return value;
}

static inline void DrainPlayerBattery(f32 amount)
{
	f32 battery = gGame.GetPlayer()->mParams.mBattery - amount;
	if (battery < 0.0f) {
		battery = 0.0f;
	}
	gGame.GetPlayer()->mParams.SetBattery(battery);
}

extern "C" void UpdateSpiderBossDamage(XObjSpiderBoss* spider, u32 frames)
{
	CPad pad(0);
	switch (spider->mDamageStep) {
	case 0: // flag 2 took a hit, 0x10 means already reacting
		if ((spider->mSpiderStatus.mFlags & 2) && !(spider->mSpiderStatus.mFlags & 0x10)) {
			StopGameSfx(&gGame.mSound, 0x12A);
			spider->mSpiderStatus.mFlags &= ~2;
			if (spider->mHitStrength >= (s32)GetPlayerActionParameter(31) / 100.0f) { // a strong hit costs 3
				spider->mHits -= 3;
				if (spider->mHits <= 0)
					spider->mHits = 0;
			} else if (--spider->mHits <= 0)
				spider->mHits = 0;
			if (gGame.GetPlayer()->mParams.mBattery <= 30.0f && (spider->mSpiderStatus.mFlags & 0x40000000)) // a low battery finishes it
				spider->mHits = 0;
			if (!(spider->mSpiderStatus.mFlags & 0x40000000)) { // each threshold sheds another part
				if (spider->mHits < 22) {
					LaunchSpiderBossPartByIndex(0);
				}
				if (spider->mHits < 19) {
					LaunchSpiderBossPartByIndex(1);
				}
				if (spider->mHits < 16) {
					LaunchSpiderBossPartByIndex(2);
					LaunchSpiderBossPartByIndex(8);
				}
				if (spider->mHits < 13) {
					LaunchSpiderBossPartByIndex(3);
				}
				if (spider->mHits < 11) {
					LaunchSpiderBossPartByIndex(4);
					LaunchSpiderBossPartByIndex(9);
				}
				if (spider->mHits < 9) {
					LaunchSpiderBossPartByIndex(5);
				}
				if (spider->mHits < 6) {
					LaunchSpiderBossPartByIndex(6);
					LaunchSpiderBossPartByIndex(10);
				}
				if (spider->mHits < 3) {
					LaunchSpiderBossPartByIndex(7);
				}
				if (spider->mHits < 1)
					LaunchSpiderBossCenterPart();
			}
			if (spider->mHits <= 0) { // beaten: state 2 under flag 0x40000000, else state 1
				ResetSpiderBossAttachment();
				StopGameSfx(&gGame.mSound, 0x16001F);
				spider->mSpeed = 0;
				if (spider->mSpiderStatus.mFlags & 0x40000000) {
					spider->mStep      = 0;
					spider->mTimer     = 0;
					spider->mPrevState = spider->mState;
					spider->mState     = 2;
					spider->mSpiderStatus.mFlags |= 0x10;
				} else {
					spider->mStep      = 0;
					spider->mTimer     = 0;
					spider->mPrevState = spider->mState;
					spider->mState     = 1;
				}
			} else {
				spider->mDamageStep = 1;
				spider->mBlinkTimer = 0;
				spider->mSpiderStatus.mFlags |= 0x10;
			}
		}
		break;
	case 1: // it blinks red for twenty frames while invulnerable
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
	if (spider->mSpiderStatus.mFlags & 0x200) { // it pulls the player in toward it
		f32 heading, speed, x, z;
		Vec p = gGame.GetPlayer()->mMoveTranslate;
		CVec position(p);
		speed = 0.4f * (220.0f - SpiderContactDistance(&position, &spider->mMoveTranslate)); // the pull weakens with distance
		if (speed > 4.0f)
			speed = 4.0f;
		heading = PI + (f32)atan2(p.x - spider->mMoveTranslate.x, p.z - spider->mMoveTranslate.z);
		x       = speed * (f32)sin(heading);
		z       = speed * (f32)cos(heading);
		if (IsUsingMug(&gGame.GetPlayer()->mLift)) { // the mug takes no push
			gGame.GetPlayer()->_158C4.Set(0, 0, 0);
		} else
			gGame.GetPlayer()->_158C4.Set(x, 0, z);
	} else
		gGame.GetPlayer()->_158C4.Set(0, 0, 0);
	XObjChr* player = gGame.GetPlayer();
	Vec ppos        = player->mMoveTranslate;
	CVec pos(ppos);
	if (SpiderContactDistance(&pos, &spider->mMoveTranslate) < 28.0f && !IsPlayerFlashingFromHit(player)
	    && (spider->mSpiderStatus.mFlags & 0x1000) && !(spider->mSpiderStatus.mFlags & 0x80000))
		player->OnHitFromPosition(ppos, 4.0f);
	if (GetPlayerCollisionRadius(gGame.GetPlayer()) < 2.0f && (spider->mSpiderStatus.mFlags & 0x20000)) { // a small rider gets drained
		if (--spider->mDrainTimer <= 0) { // every thirty frames, four battery
			DrainPlayerBattery(4.0f);
			spider->mDrainTimer = 30;
			if (!gSpiderDrainFlashClaimed) {
				spider->mSpiderStatus.mFlags |= 0x1000000;
				gSpiderDrainFlashClaimed = 1;
			}
			if (spider->mSpiderStatus.mFlags & 0x1000000) {
				gSpiderDrainFlashTimers[0] = FRAMES_PER_SECOND;
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
			if (gSpiderDrainFlashTimers[0] > 0) { // the red flash fades out over a second
				f32 red = 128.0f * (gSpiderDrainFlashTimers[0] / FRAME_RATE);
				player->SetColor(1, red, 0, 0, 0.0f);
				StartScreenFade(&gGame.mScrnTex.mColor, red, 0, 0, 0, red, 0, 0, 0, 0.0f);
			} else {
				player->SetColor(1, 0, 0, 0, 0.0f);
				StartScreenFade(&gGame.mScrnTex.mColor, 0, 0, 0, 0, 100, 100, 200, 0, 0.0f);
			}
		}
	}
	if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && (spider->mSpiderStatus.mFlags & 0x20000)) { // an empty battery drops the player
		InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 0, 1);
		spider->mSpiderStatus.mFlags &= ~0x20000;
		XObjChr* player = gGame.GetPlayer();
		player->RestoreControl();
	}
}

extern "C" void UpdateSpiderBossAttachments(XObjSpiderBoss* spider, u32)
{
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	CPad pad(0);
	if ((spider->mSpiderStatus.mFlags & 0x20000) && (player->mPlayerStatus.mFlags & 0x80000000)) { // it carries the player
		s32 path[] = { 0, 2, 0, 1, 1, 0, 0 };
		joint.Attach(FindJointByPath(spider->mJObj, path, 7));
		CVec position(GetJointPosition(joint.mJObj));
		f32 radius = GetPlayerCollisionRadius(gGame.GetPlayer());
		position.y -= 6.0f; // sits 6 below the joint
		f32 heading         // away from the spider's center
		    = PI + (f32)atan2(spider->mMoveTranslate.x - player->mMoveTranslate.x, spider->mMoveTranslate.z - player->mMoveTranslate.z);
		position.x += (radius + 0.5f) * (f32)sin(heading);
		position.z += (radius + 0.5f) * (f32)cos(heading);
		player->SetPos(position, 0);
		if (gGame.IsRunning() && (pad.mPad->mButton & 0xF0000)) { // stick pushed: face its direction
			player->SetAnim(4, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			heading = (f32)atan2(pad.mPad->mStickX, pad.mPad->mStickY);
			heading = -heading + gGame.mMotionControlCamera.mCamera.mAngle.y;
		} else if (radius < 2.0f) // a small rider gets another anim
			player->SetAnim(0xE6, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		else
			player->SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		player->SetRotateY(heading, 0);
	}
	if (spider->mSpiderStatus.mFlags & 0x20000000) { // re-fit the eight parts to their joints
		for (u32 i = 0; i < 8; ++i) {
			XObjJointPart& part = spider->mParts[i];
			if (!part.mJoint || !part.mObj)
				continue;
			Mtx scaling, translation, result, mount;
			PSMTXCopy(HSD_JObjGetMtxPtr(part.mJoint), mount);
			Vec scale;
			HSD_MtxGetScale(mount, &scale);
			scale.x = GetStagePlacementScale(part.mObj).x / scale.x; // undo the joint's scale, keep the placement's
			scale.y = GetStagePlacementScale(part.mObj).y / scale.y;
			scale.z = GetStagePlacementScale(part.mObj).z / scale.z;
			PSMTXScale(scaling, scale.x, scale.y, scale.z);
			PSMTXConcat(mount, scaling, result);
			const CVec& pos = part.mObj->mMoveTranslate;
			PSMTXTrans(translation, pos.x, pos.y, pos.z);
			PSMTXConcat(result, translation, result);
			HSD_JObj* obj = part.mObj->mJObj;
			MtxPtr matrix = result;
			HSD_JObjCopyMtx(obj, matrix);
		}
	}
}

/**
 * @note Address: 0x80103EC0
 * @note Size: 0x94
 */
void XObjSpiderBoss::State0(u32 frames)
{
	mSpiderStatus.mFlags &= ~0x80000U;
	u32 flags = mSpiderStatus.mFlags;
	if ((flags >> 25) & 1) {
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
 * @note Address: 0x80103B80
 * @note Size: 0x340
 */
// gets back up, blinking
void XObjSpiderBoss::State1(u32 frames)
{
	switch (mStep) {
	case 0:
		mSpiderStatus.mFlags |= 0x10;
		SetAnim(11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
		HSD_AObj* aobj = GetAObj(mJObj);
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		aobj           = GetAObj(mJObj);
		f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
		if (end - frames <= frame) {
			SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
 * @note Address: 0x80103B28
 * @note Size: 0x58
 */
void XObjSpiderBoss::State2(u32 frames)
{
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

static inline void SetAttackAnim(XObjSpiderBoss* boss, s32 id, BOOL restart = FALSE)
{
	boss->SetAnim(id, restart, NULL, NULL, NULL, 0, 1, 0, 10);
}

static inline bool AttackAnimEnding(XObjSpiderBoss* boss, u32 frames)
{
	return GetAnimEndFrame(boss->mJObj) - frames <= GetAnimFrame(boss->mJObj);
}

static inline void ShakeSpiderArena()
{
	if (!(gGame.mStatus.mFlags & 0x100))
		StartScreenShake(&gGame.mQuake, 0, 3, 0, 0, 0, 0, .4f);
}

static inline void PulseSpiderArena(XObjSpiderBoss* boss)
{
	if ((s32)--boss->mActionTimer < 0) {
		boss->mActionTimer = rand() % 60 + 60;
		ShakeSpiderArena();
		CommandSpiderBossProjectiles(0);
	}
}

static inline bool IsGrabBlocked(s32 anim)
{
	switch (anim) { // these player anims make him ungrabbable

	case 5:
	case 6:
	case 7:
	case 9:
	case 10:
	case 11:
	case 0x27:
	case 0x28:
	case 0xa1:
	case 0xf6:
	case 0x814:
	case 0x816:
	case 0x817:
	case 0x81f:
	case 0x820:
	case 0x824:
	case 0x879:
	case 0x87a:
	case 0x8c0:
	case 0x8c1:
	case 0x8c2:
	case 0x8c3:
	case 0x8c4:
	case 0x8c5:
		return true;
	}
	return false;
}

// NONMATCHING: same code; retail keeps the zero from the prologue in r6 (here r0), takes r4 for the second path
// pointer and has r29 and r30 the other way round in the whole function. The path arrays sit 0x24 lower in .rodata
// than retail's, which has an unread { -PI/2, 0, 0 } before OnHitFromPosition's Vecs and two unread zero Vecs
// behind UpdateSpiderBossAttachments' path.
void XObjSpiderBoss::State3(u32 frames)
{
	CPad pad(0);
	Vec player = gGame.GetPlayer()->mMoveTranslate;
	CVec position(player);
	CJObj joint;
	switch (mStep) {
	case 100:
		mTimer = 3 * BOSS_FPS / frames; // three seconds
		SetAttackAnim(this, 7);
		gSpiderBossSavedTargetYaw = mTargetRotY;
		mStep                     = 101;
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
		gSpiderBossSavedTargetYaw = mTargetRotY;
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
			// after three seconds spent approaching, counted across rounds until a grab ends
			if (mGrabIntervalFrames >= (s32)(3 * BOSS_FPS)) {

				mStep = 10;
				SetAttackAnim(this, 16);
				s32 path[] = { 0, 2, 0, 1, 1, 0, 0 };
				joint.Attach(FindJointByPath(mJObj, path, 7));
				// Retail updates the joint matrix here and discards the resulting position.
				joint.GetPos();
			}
			PulseSpiderArena(this);
		} else if (--mTimer < 0) {
			mTimer = 30;
			mStep  = 20; // step 20: a short pause, then back to the start
			ShakeSpiderArena();
			CommandSpiderBossProjectiles(0);
		}
		break;
	case 10:
		if (AttackAnimEnding(this, frames)) {
			PlaySpiderBossAttachmentAnimation(1);
			mTimer = BOSS_FPS / frames;
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
			mTimer       = 3 * BOSS_FPS / frames + (BOSS_FPS / frames) / 2; // three and a half seconds
			mActionTimer = rand() % 30 + 30;
			s32 path[]   = { 0, 2, 0, 1, 1, 0, 0 };
			joint.Attach(FindJointByPath(mJObj, path, 7));
			// Retail updates the joint matrix here and discards the resulting position.
			joint.GetPos();
			PlaySpiderBossAttachmentAnimation(2);
			PlayGameSound(&gGame.mSound, 0x16001f, 255);
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
		if (IsGrabBlocked(gGame.GetPlayer()->mAnimId))
			mGrabCooldown = 5;
		f32 distance = SpiderContactDistance(&position, &mMoveTranslate);
		// the grab: within 70, the mug not raised, a grab-able anim and no cooldown
		if (distance <= 70 && !IsUsingMug(&gGame.GetPlayer()->mLift) && !IsGrabBlocked(gGame.GetPlayer()->mAnimId)
		    && gGame.GetPlayer()->mAttachmentRequest < 0 && mGrabCooldown == 0) {
			mStatus.mFlags |= 0x100000; // flags 0x100000/0x20000 mark the grab
			ShakeSpiderArena();
			mSpiderStatus.mFlags |= 0x20000;
			XObjChr* chr              = gGame.GetPlayer();
			chr->mPlayerStatus.mFlags = 0;
			chr->mPlayerStatus.mFlags |= 0x80000000;
			gGame.GetPlayer()->SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			mEscapeInputStrength = 1;
			mEscapeRemaining     = 450; // the grab's escape meter
			++mStep;
			InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 1, 1);
			mSpiderStatus.mFlags &= ~0x200;
			PlayGameSound(&gGame.mSound, 0x16001d, 255);
			PlayGameSound(&gGame.mSound, 0x16001e, 255);
			break;
		}
		if (distance <= 220) {
			mSpiderStatus.mFlags |= 0x200; // flag 0x200 tracks the player being in range
			PulseSpiderArena(this);
		} else {
			mGrabIntervalFrames = 0;
			SetAttackAnim(this, 7);
			mSpiderStatus.mFlags &= ~0x200;
			StopGameSfx(&gGame.mSound, 0x16001f);
			ResetSpiderBossAttachment();
			StopGameSfx(&gGame.mSound, 0x16001f);
			mStep = 0;
		}
		if (--mTimer < 0) {
			mGrabIntervalFrames = 0;
			SetAttackAnim(this, 7);
			mSpiderStatus.mFlags &= ~0x200;
			mStep = 0;
			ResetSpiderBossAttachment();
			StopGameSfx(&gGame.mSound, 0x16001f);
		}
		break;
	}
	case 13: // the grab: holding the stick in any direction drains the escape meter
		if (gGame.IsRunning() && (pad.mPad->mButton & 0xf0000)) {
			mEscapeRemaining -= mEscapeInputStrength;
			mEscapeInputStrength = 8;
		}
		if (mEscapeInputStrength > 1)
			mEscapeInputStrength -= 1;
		if (mEscapeRemaining <= 0) {
			if (mSpiderStatus.Check(0x1000000)) { // clears the drain flash tint
				mSpiderStatus.mFlags &= ~0x1000000;
				gSpiderDrainFlashClaimed = 0;
				gGame.GetPlayer()->SetColor(1, 0, 0, 0, 0);
			}
			mStatus.mFlags &= ~0x100000ULL;
			mSpiderStatus.mFlags &= ~0x20000;
			XObjChr* chr = gGame.GetPlayer();
			chr->RestoreControl();
			mGrabIntervalFrames = 0;
			SetAttackAnim(this, 7);
			mStep = 0;
			InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 0, 1);
			ResetSpiderBossAttachment();
			StopGameSfx(&gGame.mSound, 0x16001f);
		}
		if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) { // a drained battery lets go too
			SetAttackAnim(this, 7);
			mStep = 30;
			ResetSpiderBossAttachment();
			StopGameSfx(&gGame.mSound, 0x16001f);
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

static const Vec sSpiderBossPatrolPoints[]
    = { { 0, 0, 0 },      { 80, 0, 0 },     { 160, 0, 0 },    { -80, 0, 0 },    { -160, 0, 0 },  { 0, 0, 80 },     { 80, 0, 80 },
	    { 160, 0, 80 },   { -80, 0, 80 },   { -160, 0, 80 },  { 0, 0, 160 },    { 80, 0, 160 },  { 120, 0, 120 },  { -80, 0, 160 },
	    { -120, 0, 120 }, { 0, 0, -80 },    { 80, 0, -80 },   { 160, 0, -80 },  { -80, 0, -80 }, { -160, 0, -80 }, { 0, 0, -160 },
	    { 80, 0, -160 },  { 120, 0, -120 }, { -80, 0, -160 }, { -120, 0, -120 } };

static inline void PlayPatrolAnim(XObjSpiderBoss* boss, s32 id)
{
	HSD_AnimJoint* joint      = boss->GetJointAnimation(id);
	HSD_MatAnimJoint* mat     = boss->GetMaterialAnimation(0x17);
	HSD_ShapeAnimJoint* shape = boss->GetShapeAnimation(id);
	boss->SetAnim(0, TRUE, joint, mat, shape, 0, 1, 1, 10);
	boss->mAltAnimId = id;
}

static inline void FacePatrolPoint(XObjSpiderBoss* boss, const Vec& target, s32 turningStep, s32 readyStep)
{
	boss->mHeading = atan2(target.x - boss->mMoveTranslate.x, target.z - boss->mMoveTranslate.z);
	f32 delta      = boss->mHeading - boss->mTargetRotY;
	if (delta < 0)
		delta += TWO_PI;
	if (delta < PI) { // one turn anim per side, an exact about-face needs none
		PlayPatrolAnim(boss, 0x1a);
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
	if (diff > PI)
		boss->mHeading = TWO_PI + boss->mHeading;
	else if (diff < -PI)
		boss->mHeading -= TWO_PI;
	ApproachHudValue(&boss->mTargetRotY, &boss->mHeading, .12f);
	if (boss->mTargetRotY == boss->mHeading) {
		PlayPatrolAnim(boss, 0x17);
		boss->mStep = readyStep;
	}
}

// NONMATCHING: retail's frame is 0x20 smaller, it builds each patrol effect's rotation straight in the copy it passes to
// CreateSRT where this build makes the local and copies it (same in OnHitFromPosition, no source form found). The patrol
// points and effect vectors also sit 0x24 lower in .rodata, for the unread Vecs named at State3. Case 0 forms the
// patrol-point pointer before the candidates' (US 0x8010064C addi r4, r29, 0x12c); copying by index (candidates[n++])
// instead of the walked pointer is worse (93.9 %), and (Vec) { 0, mTargetRotY, 0 } at the call keeps the copy.
void XObjSpiderBoss::State4(u32 frames)
{
	CPad pad(0);
	CVec player = gGame.GetPlayer()->mMoveTranslate;
	if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && mStep != 30) {
		PlayPatrolAnim(this, 0x17);
		mSpeed = 0;
		mStep  = 30; // step 30 waits out the drained battery
	}
	switch (mStep) {
	case 0: {
		mTimer = (rand() % 30 + 30) / frames;
		mSpeed = 0;
		PlayPatrolAnim(this, 0x17);
		mSpiderStatus.mFlags |= 0x1000;
		Vec candidates[25];
		Vec* next = candidates;
		// skips the point at the stored index (pick indexes the shortened list, so not always the last target)
		for (s32 i = 0; i < 25; ++i)
			if (mPatrolSelection != i)
				*next++ = sSpiderBossPatrolPoints[i];
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
			mSpeed = 1;
			PlayPatrolAnim(this, 0x18);
			{ // the move-start effect
				Vec scale    = { 1, 1, 1 };
				Vec rotation = {};
				rotation.y   = mTargetRotY;
				gGame.mParticle.CreateSRT(0xb6, 1, mMoveTranslate, scale, rotation, TRUE);
				mPatrolEffectActive = 1;
			}
			mTimer = rand() % 30 + 30;
			++mStep;
		}
		break;
	case 3:
		if (mAltAnimId == 0x18) {
			if (!mPatrolEffectActive) {
				Vec scale    = { 1, 1, 1 };
				Vec rotation = {};
				rotation.y   = mTargetRotY;
				gGame.mParticle.CreateSRT(0xb6, 1, mMoveTranslate, scale, rotation, TRUE);
				mPatrolEffectActive = 1;
			}
			if (GetAnimEndFrame(mJObj) - frames <= GetAnimFrame(mJObj)) {
				mPatrolEffectActive = 0;
				gGame.mParticle.Kill(0xb6);
			}
		}
		mHeading    = atan2(mPatrolTarget.x - mMoveTranslate.x, mPatrolTarget.z - mMoveTranslate.z);
		mTargetRotY = mHeading;
		if (SpiderContactDistance(&mPatrolTarget, &mMoveTranslate) < 2) {
			mPatrolEffectActive = 0;
			gGame.mParticle.Kill(0xb6);
			PlayPatrolAnim(this, 0x17);
			mSpeed       = 0;
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
		if ((s32)--mActionTimer < 0)
			mStep = 0;
		else { // the projectile shot
			PlayPatrolAnim(this, 0x1b);
			CommandSpiderBossProjectiles(1);
			PlayGameSound(&gGame.mSound, 0x16001b, 255);
			++mStep;
		}
		break;
	case 7:
		if (GetAnimEndFrame(mJObj) - frames <= GetAnimFrame(mJObj))
			mStep = 4;
		break;
	case 30: // waits until the battery is back and no script runs
		if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && gGame.IsRunning() && !gGame.mEvt.mData.mScript)
			mStep = 0;
		break;
	}
}

/**
 * @note Address: 0x80100424
 * @note Size: 0xC
 */
extern "C" bool IsSpiderBossProjectileHitSuppressed(XObjSpiderBoss* spider)
{
	return (spider->mSpiderStatus.mFlags & 0x200) != 0;
}

/**
 * @note Address: 0x801002DC
 * @note Size: 0x148
 */
void XObjSpiderBoss::UpdateAnimationSounds()
{
	if (mStatus.mFlags & 1ULL) {
		if (mStatus.mFlags & 0x10000ULL) {
			if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
				s32 id         = (mSpiderStatus.mFlags & 0x40000000) ? mAltAnimId : mAnimId;
				HSD_AObj* aobj = GetAObj(mJObj);
				f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
				mAnimSfx.Update(id, frame, 255);
			}
		}
	}
}

XObjSpiderBoss::~XObjSpiderBoss()
{
}

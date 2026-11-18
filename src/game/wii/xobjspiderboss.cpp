#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/infoscrn.h"
#include "game/wii/cpad.h"
#include "game/wii/ceffect.h"
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

extern s32 gRefreshRate;
extern AnimSfxEvent gSpiderBossAnimSfxEvents[];
extern u32 gSpiderBossCoinBounceContext;
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
	spider->mAreaPos.x           = -200.0f;
	spider->mAreaPos.y           = 0.0f;
	spider->mAreaPos.z           = -200.0f;
	spider->mAreaWidth           = 400.0f;
	spider->mAreaDepth           = 400.0f;
	spider->mDamageStep          = 0;
	spider->mBlinkTimer          = 0;
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
	if (GetAttachmentCountById((AttachmentValues*)gGame.GetPlayer()->mAttachments, 2) < 2) {
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
		mAreaPos.x = -200.0f;
		mAreaPos.y = 0.0f;
		mAreaPos.z = -200.0f;
		mAreaWidth = 400.0f;
		mAreaDepth = 400.0f;
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

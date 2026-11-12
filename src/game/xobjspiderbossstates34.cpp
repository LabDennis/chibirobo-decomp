#include "game/xobjspiderboss.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/animframe.h"
#include "game/spidercontact.h"
#include "game/video.h"
#include <stdlib.h>

extern "C" void ApproachHudValue(f32*, f32*, f32);
extern "C" void CommandSpiderBossProjectiles(s32);

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
	if (delta < PI) {
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

// NONMATCHING: retail's frame is 0x20 smaller, it sets the patrol effect's rotation in the copy it passes
void XObjSpiderBoss::State4(u32 frames)
{
	CPad pad(0);
	CVec player = gGame.GetPlayer()->mMoveTranslate;
	if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && mStep != 30) {
		PlayPatrolAnim(this, 0x17);
		mSpeed = 0;
		mStep  = 30;
	}
	switch (mStep) {
	case 0: {
		mTimer = (rand() % 30 + 30) / frames;
		mSpeed = 0;
		PlayPatrolAnim(this, 0x17);
		mSpiderStatus.mFlags |= 0x1000;
		Vec candidates[25];
		Vec* next = candidates;
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
			{
				Vec rotation = {};
				Vec scale    = { 1, 1, 1 };
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
				Vec rotation = {};
				Vec scale    = { 1, 1, 1 };
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
			mSpeed = 0;
			_9C0   = 5;
			mStep  = 4;
		}
		break;
	case 4:
		FacePatrolPoint(this, player, 5, 6);
		break;
	case 5:
		TurnTowardPoint(this, 6);
		break;
	case 6:
		if ((s32)--_9C0 < 0)
			mStep = 0;
		else {
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
	case 30:
		if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && gGame.IsRunning() && !gGame.mEvt.mData.mScript)
			mStep = 0;
		break;
	}
}

extern "C" {
f32 gSpiderBossSavedTargetYaw;
extern u8 gSpiderDrainFlashClaimed;
void PlaySpiderBossAttachmentAnimation(s32);
void ResetSpiderBossAttachment();
BOOL IsUsingMug(XActLift*);
}

#if defined(VERSION_PAL)
#define BOSS_FPS ((u32)gVideoFramesPerSecond)
#else
#define BOSS_FPS 60U
#endif

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
	if ((s32)--boss->_9C0 < 0) {
		boss->_9C0 = rand() % 60 + 60;
		ShakeSpiderArena();
		CommandSpiderBossProjectiles(0);
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
// pointer and reuses the player pointer for both _980 writes in case 12. The path arrays and the joint string sit
// at other .rodata offsets because this unit holds only part of the file.
void XObjSpiderBoss::State3(u32 frames)
{
	CPad pad(0);
	Vec player = gGame.GetPlayer()->mMoveTranslate;
	CVec position(player);
	CJObj joint;
	switch (mStep) {
	case 100:
		mTimer = 3 * BOSS_FPS / frames;
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
		mTimer = rand() % 90 + 90;
		_9C0   = rand() % 30 + 30;
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
			mStep  = 20;
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
			mTimer     = 3 * BOSS_FPS / frames + (BOSS_FPS / frames) / 2;
			_9C0       = rand() % 30 + 30;
			s32 path[] = { 0, 2, 0, 1, 1, 0, 0 };
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
		if (distance <= 70 && !IsUsingMug(&gGame.GetPlayer()->mLift) && !IsGrabBlocked(gGame.GetPlayer()->mAnimId)
		    && gGame.GetPlayer()->mAttachmentRequest < 0 && mGrabCooldown == 0) {
			mStatus.mFlags |= 0x100000;
			ShakeSpiderArena();
			mSpiderStatus.mFlags |= 0x20000;
			gGame.GetPlayer()->_980.mFlags = 0;
			gGame.GetPlayer()->_980.mFlags |= 0x80000000;
			gGame.GetPlayer()->SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			mEscapeInputStrength = 1;
			mEscapeRemaining     = 450;
			++mStep;
			InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 1, 1);
			mSpiderStatus.mFlags &= ~0x200;
			PlayGameSound(&gGame.mSound, 0x16001d, 255);
			PlayGameSound(&gGame.mSound, 0x16001e, 255);
			break;
		}
		if (distance <= 220) {
			mSpiderStatus.mFlags |= 0x200;
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
	case 13:
		if (gGame.IsRunning() && (pad.mPad->mButton & 0xf0000)) {
			mEscapeRemaining -= mEscapeInputStrength;
			mEscapeInputStrength = 8;
		}
		if (mEscapeInputStrength > 1)
			mEscapeInputStrength -= 1;
		if (mEscapeRemaining <= 0) {
			if (mSpiderStatus.Check(0x1000000)) {
				mSpiderStatus.mFlags &= ~0x1000000;
				gSpiderDrainFlashClaimed = 0;
				gGame.GetPlayer()->vf12C(1, 0, 0, 0, 0);
			}
			mStatus.mFlags &= ~0x100000ULL;
			mSpiderStatus.mFlags &= ~0x20000;
			XObjChr* chr     = gGame.GetPlayer();
			chr->_980.mFlags = 0;
			chr->_980.mFlags |= 0x20000000;
			chr->_980.mFlags |= 7;
			chr->mPad.mReset    = TRUE;
			mGrabIntervalFrames = 0;
			SetAttackAnim(this, 7);
			mStep = 0;
			InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 0, 1);
			ResetSpiderBossAttachment();
			StopGameSfx(&gGame.mSound, 0x16001f);
		}
		if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
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

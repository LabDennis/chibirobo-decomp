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
extern "C" void State4__14XObjSpiderBossFUl(XObjSpiderBoss* boss, u32 frames)
{
	CPad pad(0);
	CVec player = gGame.GetPlayer()->mMoveTranslate;
	if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && boss->mStep != 30) {
		PlayPatrolAnim(boss, 0x17);
		boss->mSpeed = 0;
		boss->mStep  = 30;
	}
	switch (boss->mStep) {
	case 0: {
		boss->mTimer = (rand() % 30 + 30) / frames;
		boss->mSpeed = 0;
		PlayPatrolAnim(boss, 0x17);
		boss->mSpiderStatus.mFlags |= 0x1000;
		Vec candidates[25];
		Vec* next = candidates;
		for (s32 i = 0; i < 25; ++i)
			if (boss->mPatrolSelection != i)
				*next++ = sSpiderBossPatrolPoints[i];
		s32 pick               = rand() % 24;
		boss->mPatrolTarget    = candidates[pick];
		boss->mPatrolSelection = pick;
		FacePatrolPoint(boss, boss->mPatrolTarget, 1, 2);
		break;
	}
	case 1:
		TurnTowardPoint(boss, 2);
		break;
	case 2:
		if (--boss->mTimer < 0) {
			boss->mSpeed = 1;
			PlayPatrolAnim(boss, 0x18);
			{
				Vec rotation = {};
				Vec scale    = { 1, 1, 1 };
				rotation.y   = boss->mTargetRotY;
				gGame.mParticle.CreateSRT(0xb6, 1, boss->mMoveTranslate, scale, rotation, TRUE);
				boss->mPatrolEffectActive = 1;
			}
			boss->mTimer = rand() % 30 + 30;
			++boss->mStep;
		}
		break;
	case 3:
		if (boss->mAltAnimId == 0x18) {
			if (!boss->mPatrolEffectActive) {
				Vec rotation = {};
				Vec scale    = { 1, 1, 1 };
				rotation.y   = boss->mTargetRotY;
				gGame.mParticle.CreateSRT(0xb6, 1, boss->mMoveTranslate, scale, rotation, TRUE);
				boss->mPatrolEffectActive = 1;
			}
			if (GetAnimEndFrame(boss->mJObj) - frames <= GetAnimFrame(boss->mJObj)) {
				boss->mPatrolEffectActive = 0;
				gGame.mParticle.Kill(0xb6);
			}
		}
		boss->mHeading    = atan2(boss->mPatrolTarget.x - boss->mMoveTranslate.x, boss->mPatrolTarget.z - boss->mMoveTranslate.z);
		boss->mTargetRotY = boss->mHeading;
		if (SpiderContactDistance(&boss->mPatrolTarget, &boss->mMoveTranslate) < 2) {
			boss->mPatrolEffectActive = 0;
			gGame.mParticle.Kill(0xb6);
			PlayPatrolAnim(boss, 0x17);
			boss->mSpeed = 0;
			boss->_9C0   = 5;
			boss->mStep  = 4;
		}
		break;
	case 4:
		FacePatrolPoint(boss, player, 5, 6);
		break;
	case 5:
		TurnTowardPoint(boss, 6);
		break;
	case 6:
		if ((s32)--boss->_9C0 < 0)
			boss->mStep = 0;
		else {
			PlayPatrolAnim(boss, 0x1b);
			CommandSpiderBossProjectiles(1);
			PlayGameSound(&gGame.mSound, 0x16001b, 255);
			++boss->mStep;
		}
		break;
	case 7:
		if (GetAnimEndFrame(boss->mJObj) - frames <= GetAnimFrame(boss->mJObj))
			boss->mStep = 4;
		break;
	case 30:
		if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && gGame.IsRunning() && !gGame.mEvt.mData.mScript)
			boss->mStep = 0;
		break;
	}
}

extern "C" {
extern f32 gSpiderBossSavedTargetYaw;
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

static inline void ReadGrabJoint(XObjSpiderBoss* boss, CJObj& joint)
{
	s32 path[] = { 0, 2, 0, 1, 1, 0, 0 };
	joint.Attach(FindJointByPath(boss->mJObj, path, 7));
	// Retail updates the joint matrix here and discards the resulting position.
	joint.GetPos();
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

// NONMATCHING: same code; retail orders the vector-constant loads (@2029) and a few lfs differently
extern "C" void State3__14XObjSpiderBossFUl(XObjSpiderBoss* boss, u32 frames)
{
	CPad pad(0);
	Vec player = gGame.GetPlayer()->mMoveTranslate;
	CVec position(player);
	CJObj joint;
	switch (boss->mStep) {
	case 100:
		boss->mTimer = 3 * BOSS_FPS / frames;
		SetAttackAnim(boss, 7);
		gSpiderBossSavedTargetYaw = boss->mTargetRotY;
		boss->mStep               = 101;
		break;
	case 101: {
		boss->mHeading = atan2(player.x - boss->mMoveTranslate.x, player.z - boss->mMoveTranslate.z);
		f32 delta      = boss->mTargetRotY - boss->mHeading;
		if (delta > PI)
			boss->mHeading = TWO_PI + boss->mHeading;
		else if (delta < -PI)
			boss->mHeading -= TWO_PI;
		ApproachHudValue(&boss->mTargetRotY, &boss->mHeading, .018f);
		if (boss->mHeading > boss->mTargetRotY) {
			if (AttackAnimEnding(boss, frames))
				SetAttackAnim(boss, 9, TRUE);
			else
				SetAttackAnim(boss, 9);
		} else if (boss->mHeading < boss->mTargetRotY) {
			if (AttackAnimEnding(boss, frames))
				SetAttackAnim(boss, 8, TRUE);
			else
				SetAttackAnim(boss, 8);
		} else
			SetAttackAnim(boss, 7);
	}
		if (--boss->mTimer < 0)
			boss->mStep = 0;
		break;
	case 0:
		boss->mTimer = rand() % 90 + 90;
		boss->_9C0   = rand() % 30 + 30;
		SetAttackAnim(boss, 7);
		++boss->mStep;
		gSpiderBossSavedTargetYaw = boss->mTargetRotY;
		break;
	case 1:
		boss->mGrabIntervalFrames += frames;
		{
			boss->mHeading = atan2(player.x - boss->mMoveTranslate.x, player.z - boss->mMoveTranslate.z);
			f32 delta      = boss->mTargetRotY - boss->mHeading;
			if (delta > PI)
				boss->mHeading = TWO_PI + boss->mHeading;
			else if (delta < -PI)
				boss->mHeading -= TWO_PI;
			ApproachHudValue(&boss->mTargetRotY, &boss->mHeading, .018f);
			if (boss->mHeading > boss->mTargetRotY) {
				if (AttackAnimEnding(boss, frames))
					SetAttackAnim(boss, 9, TRUE);
				else
					SetAttackAnim(boss, 9);
			} else if (boss->mHeading < boss->mTargetRotY) {
				if (AttackAnimEnding(boss, frames))
					SetAttackAnim(boss, 8, TRUE);
				else
					SetAttackAnim(boss, 8);
			} else
				SetAttackAnim(boss, 7);
		}
		if (SpiderContactDistance(&position, &boss->mMoveTranslate) <= 220) {
			if (boss->mGrabIntervalFrames >= (s32)(3 * BOSS_FPS)) {
				boss->mStep = 10;
				SetAttackAnim(boss, 16);
				ReadGrabJoint(boss, joint);
			}
			PulseSpiderArena(boss);
		} else if (--boss->mTimer < 0) {
			boss->mTimer = 30;
			boss->mStep  = 20;
			ShakeSpiderArena();
			CommandSpiderBossProjectiles(0);
		}
		break;
	case 10:
		if (AttackAnimEnding(boss, frames)) {
			PlaySpiderBossAttachmentAnimation(1);
			boss->mTimer = BOSS_FPS / frames;
			SetAttackAnim(boss, 17);
			boss->mGrabCooldown = 5;
			++boss->mStep;
		}
		break;
	case 11: {
		boss->mHeading = atan2(player.x - boss->mMoveTranslate.x, player.z - boss->mMoveTranslate.z);
		f32 delta      = boss->mTargetRotY - boss->mHeading;
		if (delta > PI)
			boss->mHeading = TWO_PI + boss->mHeading;
		else if (delta < -PI)
			boss->mHeading -= TWO_PI;
		ApproachHudValue(&boss->mTargetRotY, &boss->mHeading, .018f);
		if (boss->mHeading > boss->mTargetRotY) {
			if (AttackAnimEnding(boss, frames))
				SetAttackAnim(boss, 18, TRUE);
			else
				SetAttackAnim(boss, 18);
		} else if (boss->mHeading < boss->mTargetRotY) {
			if (AttackAnimEnding(boss, frames))
				SetAttackAnim(boss, 19, TRUE);
			else
				SetAttackAnim(boss, 19);
		} else
			SetAttackAnim(boss, 17);
	}
		if (--boss->mTimer < 0) {
			boss->mTimer = 3 * BOSS_FPS / frames + (BOSS_FPS / frames) / 2;
			boss->_9C0   = rand() % 30 + 30;
			ReadGrabJoint(boss, joint);
			PlaySpiderBossAttachmentAnimation(2);
			PlayGameSound(&gGame.mSound, 0x16001f, 255);
			++boss->mStep;
		}
		break;
	case 12: {
		{
			boss->mHeading = atan2(player.x - boss->mMoveTranslate.x, player.z - boss->mMoveTranslate.z);
			f32 delta      = boss->mTargetRotY - boss->mHeading;
			if (delta > PI)
				boss->mHeading = TWO_PI + boss->mHeading;
			else if (delta < -PI)
				boss->mHeading -= TWO_PI;
			ApproachHudValue(&boss->mTargetRotY, &boss->mHeading, .018f);
			if (boss->mHeading > boss->mTargetRotY) {
				if (AttackAnimEnding(boss, frames))
					SetAttackAnim(boss, 18, TRUE);
				else
					SetAttackAnim(boss, 18);
			} else if (boss->mHeading < boss->mTargetRotY) {
				if (AttackAnimEnding(boss, frames))
					SetAttackAnim(boss, 19, TRUE);
				else
					SetAttackAnim(boss, 19);
			} else
				SetAttackAnim(boss, 17);
		}
		if (--boss->mGrabCooldown < 0)
			boss->mGrabCooldown = 0;
		if (IsGrabBlocked(gGame.GetPlayer()->mAnimId))
			boss->mGrabCooldown = 5;
		f32 distance = SpiderContactDistance(&position, &boss->mMoveTranslate);
		if (distance <= 70 && !IsUsingMug(&gGame.GetPlayer()->mLift) && !IsGrabBlocked(gGame.GetPlayer()->mAnimId)
		    && gGame.GetPlayer()->mAttachmentRequest < 0 && boss->mGrabCooldown == 0) {
			boss->mStatus.mFlags |= 0x100000;
			ShakeSpiderArena();
			boss->mSpiderStatus.mFlags |= 0x20000;
			gGame.GetPlayer()->_980.mFlags = 0;
			gGame.GetPlayer()->_980.mFlags |= 0x80000000;
			gGame.GetPlayer()->SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			boss->mEscapeInputStrength = 1;
			boss->mEscapeRemaining     = 450;
			++boss->mStep;
			InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 1, 1);
			boss->mSpiderStatus.mFlags &= ~0x200;
			PlayGameSound(&gGame.mSound, 0x16001d, 255);
			PlayGameSound(&gGame.mSound, 0x16001e, 255);
			break;
		}
		if (distance <= 220) {
			boss->mSpiderStatus.mFlags |= 0x200;
			PulseSpiderArena(boss);
		} else {
			boss->mGrabIntervalFrames = 0;
			SetAttackAnim(boss, 7);
			boss->mSpiderStatus.mFlags &= ~0x200;
			StopGameSfx(&gGame.mSound, 0x16001f);
			ResetSpiderBossAttachment();
			StopGameSfx(&gGame.mSound, 0x16001f);
			boss->mStep = 0;
		}
		if (--boss->mTimer < 0) {
			boss->mGrabIntervalFrames = 0;
			SetAttackAnim(boss, 7);
			boss->mSpiderStatus.mFlags &= ~0x200;
			boss->mStep = 0;
			ResetSpiderBossAttachment();
			StopGameSfx(&gGame.mSound, 0x16001f);
		}
		break;
	}
	case 13:
		if (gGame.IsRunning() && (pad.mPad->mButton & 0xf0000)) {
			boss->mEscapeRemaining -= boss->mEscapeInputStrength;
			boss->mEscapeInputStrength = 8;
		}
		if (boss->mEscapeInputStrength > 1)
			boss->mEscapeInputStrength -= 1;
		if (boss->mEscapeRemaining <= 0) {
			if (boss->mSpiderStatus.mFlags & 0x1000000) {
				boss->mSpiderStatus.mFlags &= ~0x1000000;
				gSpiderDrainFlashClaimed = 0;
				gGame.GetPlayer()->vf12C(1, 0, 0, 0, 0);
			}
			boss->mStatus.mFlags &= ~0x100000ULL;
			boss->mSpiderStatus.mFlags &= ~0x20000;
			XObjChr* chr     = gGame.GetPlayer();
			chr->_980.mFlags = 0;
			chr->_980.mFlags |= 0x20000000;
			chr->_980.mFlags |= 7;
			chr->mPad.mReset          = TRUE;
			boss->mGrabIntervalFrames = 0;
			SetAttackAnim(boss, 7);
			boss->mStep = 0;
			InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 0, 1);
			ResetSpiderBossAttachment();
			StopGameSfx(&gGame.mSound, 0x16001f);
		}
		if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
			SetAttackAnim(boss, 7);
			boss->mStep = 30;
			ResetSpiderBossAttachment();
			StopGameSfx(&gGame.mSound, 0x16001f);
		}
		break;
	case 20:
		if (--boss->mTimer < 0)
			boss->mStep = 0;
		break;
	case 30:
		if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && gGame.IsRunning() && !gGame.mEvt.mData.mScript)
			boss->mStep = 0;
		break;
	}
}

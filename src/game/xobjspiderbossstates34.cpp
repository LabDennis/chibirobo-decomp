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

static void PlayPatrolAnim(XObjSpiderBoss* boss, s32 id)
{
	HSD_AnimJoint* joint      = boss->GetJointAnimation(id);
	HSD_MatAnimJoint* mat     = boss->GetMaterialAnimation(0x17);
	HSD_ShapeAnimJoint* shape = boss->GetShapeAnimation(id);
	boss->SetAnim(0, TRUE, joint, mat, shape, 0, 1, 1, 10);
	boss->mAltAnimId = id;
}

static void FacePatrolPoint(XObjSpiderBoss* boss, const Vec& target, s32 turningStep, s32 readyStep)
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

static void TurnTowardPoint(XObjSpiderBoss* boss, s32 readyStep)
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

static bool AnimationEnding(HSD_JObj* joint, u32 frames)
{
	HSD_AObj* anim    = GetAObj(joint);
	const f32 end     = anim != NULL ? HSD_AObjGetEndFrame(anim) : 0.0f;
	const f32 current = anim != NULL ? HSD_AObjGetCurrentFrame(anim) : 0.0f;
	return end - frames <= current;
}

static void StartPatrolEffect(XObjSpiderBoss* boss)
{
	const Vec scale    = { 1, 1, 1 };
	const Vec rotation = { 0, boss->mTargetRotY, 0 };
	gGame.mParticle.CreateSRT(0xb6, 1, boss->mMoveTranslate, scale, rotation, TRUE);
	boss->mPatrolEffectActive = 1;
}

void XObjSpiderBoss::State4(u32 frames)
{
	const Vec player = gGame.GetPlayer()->mMoveTranslate;
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
		const s32 count    = sizeof(sSpiderBossPatrolPoints) / sizeof(sSpiderBossPatrolPoints[0]);
		const s32 previous = mPatrolSelection;
		const s32 pick     = rand() % (count - 1);
		// The stored index is in the list with the previous selection skipped.
		const s32 point  = previous >= 0 && previous <= pick ? pick + 1 : pick;
		mPatrolTarget    = sSpiderBossPatrolPoints[point];
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
			StartPatrolEffect(this);
			mTimer = rand() % 30 + 30;
			++mStep;
		}
		break;
	case 3:
		if (mAltAnimId == 0x18) {
			if (!mPatrolEffectActive)
				StartPatrolEffect(this);
			if (AnimationEnding(mJObj, frames)) {
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
		if (AnimationEnding(mJObj, frames))
			mStep = 4;
		break;
	case 30: // waits until the battery is back and no script runs
		if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && gGame.IsRunning() && !gGame.mEvt.mData.mScript)
			mStep = 0;
		break;
	}
}

extern "C" {
extern f32 gSpiderBossSavedTargetYaw;
void PlaySpiderBossAttachmentAnimation(s32);
void ResetSpiderBossAttachment();
BOOL IsUsingMug(XActLift*);
}

#if defined(VERSION_PAL)
#define BOSS_FPS ((u32)gVideoFramesPerSecond)
#else
#define BOSS_FPS 60U
#endif

static void SetAttackAnim(XObjSpiderBoss* boss, s32 id, BOOL restart = FALSE)
{
	boss->SetAnim(id, restart, NULL, NULL, NULL, 0, 1, 0, 10);
}

static void FaceAttackTarget(XObjSpiderBoss* boss, const Vec& target, u32 frames, s32 idleAnim, s32 turnLeftAnim, s32 turnRightAnim)
{
	boss->mHeading  = atan2(target.x - boss->mMoveTranslate.x, target.z - boss->mMoveTranslate.z);
	const f32 delta = boss->mTargetRotY - boss->mHeading;
	if (delta > PI)
		boss->mHeading += TWO_PI;
	else if (delta < -PI)
		boss->mHeading -= TWO_PI;
	ApproachHudValue(&boss->mTargetRotY, &boss->mHeading, .018f);
	if (boss->mHeading > boss->mTargetRotY)
		SetAttackAnim(boss, turnLeftAnim, AnimationEnding(boss->mJObj, frames));
	else if (boss->mHeading < boss->mTargetRotY)
		SetAttackAnim(boss, turnRightAnim, AnimationEnding(boss->mJObj, frames));
	else
		SetAttackAnim(boss, idleAnim);
}

static void SetupGrabJoint(XObjSpiderBoss* boss)
{
	s32 path[] = { 0, 2, 0, 1, 1, 0, 0 };
	HSD_JObjSetupMatrix(FindJointByPath(boss->mJObj, path, sizeof(path) / sizeof(path[0])));
}

static void ShakeSpiderArena()
{
	if (!(gGame.mStatus.mFlags & 0x100))
		StartScreenShake(&gGame.mQuake, 0, 3, 0, 0, 0, 0, .4f);
}

static void PulseSpiderArena(XObjSpiderBoss* boss)
{
	if ((s32)--boss->mActionTimer < 0) {
		boss->mActionTimer = rand() % 60 + 60;
		ShakeSpiderArena();
		CommandSpiderBossProjectiles(0);
	}
}

static bool IsGrabBlocked(s32 anim)
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

void XObjSpiderBoss::State3(u32 frames)
{
	CPad pad(0);
	const Vec position = gGame.GetPlayer()->mMoveTranslate;
	switch (mStep) {
	case 100:
		mTimer = 3 * BOSS_FPS / frames; // three seconds
		SetAttackAnim(this, 7);
		gSpiderBossSavedTargetYaw = mTargetRotY;
		mStep                     = 101;
		break;
	case 101:
		FaceAttackTarget(this, position, frames, 7, 9, 8);
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
		FaceAttackTarget(this, position, frames, 7, 9, 8);
		if (SpiderContactDistance(&position, &mMoveTranslate) <= 220) {
			// after three seconds spent approaching, counted across rounds until a grab ends
			if (mGrabIntervalFrames >= (s32)(3 * BOSS_FPS)) {
				mStep = 10;
				SetAttackAnim(this, 16);
				SetupGrabJoint(this);
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
		if (AnimationEnding(mJObj, frames)) {
			PlaySpiderBossAttachmentAnimation(1);
			mTimer = BOSS_FPS / frames;
			SetAttackAnim(this, 17);
			mGrabCooldown = 5;
			++mStep;
		}
		break;
	case 11:
		FaceAttackTarget(this, position, frames, 17, 18, 19);
		if (--mTimer < 0) {
			mTimer       = 3 * BOSS_FPS / frames + (BOSS_FPS / frames) / 2; // three and a half seconds
			mActionTimer = rand() % 30 + 30;
			SetupGrabJoint(this);
			PlaySpiderBossAttachmentAnimation(2);
			PlayGameSound(&gGame.mSound, 0x16001f, 255);
			++mStep;
		}
		break;
	case 12: {
		FaceAttackTarget(this, position, frames, 17, 18, 19);
		if (--mGrabCooldown < 0)
			mGrabCooldown = 0;
		if (IsGrabBlocked(gGame.GetPlayer()->mAnimId))
			mGrabCooldown = 5;
		f32 distance = SpiderContactDistance(&position, &mMoveTranslate);
		if (distance <= 70 && !IsUsingMug(&gGame.GetPlayer()->mLift) && gGame.GetPlayer()->mAttachmentRequest < 0 && mGrabCooldown == 0) {
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
			mEscapeInputStrength = sEscapeInputStrength;
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

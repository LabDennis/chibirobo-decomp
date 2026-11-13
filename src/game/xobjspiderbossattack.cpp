#include "game/xobjspiderboss.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/globals.h"
#include "game/video.h"
#include "game/gmath.h"
#include "game/spidercontact.h"
extern "C" {
void LaunchSpiderBossPartByIndex(s32 index);
void LaunchSpiderBossCenterPart();
void ResetSpiderBossAttachment();
BOOL IsUsingMug(XActLift* lift);
BOOL IsPlayerFlashingFromHit(XObjChr* player);
f32 GetPlayerCollisionRadius(XObjChr* player);
void ApplyPlayerHitReaction(XObjChr* player, BOOL react, BOOL stop, f32 time);
}

extern "C" {
CVec GetStagePlacementScale(XObj* obj);
void HSD_MtxGetScale(Mtx matrix, Vec* scale);
}

extern "C" void UpdateSpiderBossAttachments(XObjSpiderBoss* spider, u32)
{
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	CPad pad(0);
	if ((spider->mSpiderStatus.mFlags & 0x20000) && (player->mPlayerStatus.mFlags & 0x80000000)) {
		s32 path[] = { 0, 2, 0, 1, 1, 0, 0 };
		joint.Attach(FindJointByPath(spider->mJObj, path, 7));
		CVec position(GetJointPosition(joint.mJObj));
		f32 radius = GetPlayerCollisionRadius(gGame.GetPlayer());
		position.y -= 6.0f;
		f32 heading
		    = PI + (f32)atan2(spider->mMoveTranslate.x - player->mMoveTranslate.x, spider->mMoveTranslate.z - player->mMoveTranslate.z);
		position.x += (radius + 0.5f) * (f32)sin(heading);
		position.z += (radius + 0.5f) * (f32)cos(heading);
		player->SetPos(position, 0);
		if (gGame.IsRunning() && (pad.mPad->mButton & 0xF0000)) {
			player->SetAnim(4, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			heading = (f32)atan2(pad.mPad->mStickX, pad.mPad->mStickY);
			heading = -heading + gGame.mMotionControlCamera.mCamera.mAngle.y;
		} else if (radius < 2.0f)
			player->SetAnim(0xE6, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		else
			player->SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		player->SetRotateY(heading, 0);
	}
	if (spider->mSpiderStatus.mFlags & 0x20000000) {
		for (u32 i = 0; i < 8; ++i) {
			XObjJointPart& part = spider->mParts[i];
			if (!part.mJoint || !part.mObj)
				continue;
			Mtx scaling, translation, result, mount;
			PSMTXCopy(HSD_JObjGetMtxPtr(part.mJoint), mount);
			Vec scale;
			HSD_MtxGetScale(mount, &scale);
			scale.x = GetStagePlacementScale(part.mObj).x / scale.x;
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
	case 0:
		if ((spider->mSpiderStatus.mFlags & 2) && !(spider->mSpiderStatus.mFlags & 0x10)) {
			StopGameSfx(&gGame.mSound, 0x12A);
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
			if (spider->mHits <= 0) {
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
	case 1:
		if (spider->mBlinkTimer % 30 < 15)
			spider->vf12C(1, 0x80, 0, 0, 0.0f);
		else
			spider->vf12C(1, 0, 0, 0, 0.0f);
		spider->mBlinkTimer++;
		if (spider->mBlinkTimer > 20) {
			spider->mSpiderStatus.mFlags &= ~0x10;
			spider->vf12C(1, 0, 0, 0, 0.0f);
			spider->mBlinkTimer = 0;
			spider->mDamageStep = 0;
		}
		break;
	}
	if (spider->mSpiderStatus.mFlags & 0x200) {
		f32 heading, speed, x, z;
		Vec p = gGame.GetPlayer()->mMoveTranslate;
		CVec position(p);
		speed = 0.4f * (220.0f - SpiderContactDistance(&position, &spider->mMoveTranslate));
		if (speed > 4.0f)
			speed = 4.0f;
		heading = PI + (f32)atan2(p.x - spider->mMoveTranslate.x, p.z - spider->mMoveTranslate.z);
		x       = speed * (f32)sin(heading);
		z       = speed * (f32)cos(heading);
		if (IsUsingMug(&gGame.GetPlayer()->mLift)) {
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
	if (GetPlayerCollisionRadius(gGame.GetPlayer()) < 2.0f && (spider->mSpiderStatus.mFlags & 0x20000)) {
		if (--spider->mDrainTimer <= 0) {
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
			if (gSpiderDrainFlashTimers[0] > 0) {
				f32 red = 128.0f * (gSpiderDrainFlashTimers[0] / FRAME_RATE);
				player->vf12C(1, red, 0, 0, 0.0f);
				StartScreenFade(&gGame.mScrnTex.mColor, red, 0, 0, 0, red, 0, 0, 0, 0.0f);
			} else {
				player->vf12C(1, 0, 0, 0, 0.0f);
				StartScreenFade(&gGame.mScrnTex.mColor, 0, 0, 0, 0, 100, 100, 200, 0, 0.0f);
			}
		}
	}
	if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && (spider->mSpiderStatus.mFlags & 0x20000)) {
		InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 0, 1);
		spider->mSpiderStatus.mFlags &= ~0x20000;
		XObjChr* player              = gGame.GetPlayer();
		player->mPlayerStatus.mFlags = 0;
		player->mPlayerStatus.mFlags |= 0x20000000;
		player->mPlayerStatus.mFlags |= 7;
		player->mPad.mReset = TRUE;
	}
}

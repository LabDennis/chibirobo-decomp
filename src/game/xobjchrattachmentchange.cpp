#include "game/cgame.h"
#include "game/xchr.h"
#include "game/xobjflags.h"

extern "C" {
void SetPlayerPopperMountHidden(XObj*, BOOL);
void SetObjectJointTranslationByPath(CJObj*, s32, s32*, f32, f32, f32);
void SetJointYawByPath(CXObj*, s32, s32*, f32);
void ResetPopperCharge(XActPopperCharge*);
void ResetTumbleAction(XActTumble*);
void ResetPlayerRadarAction(XActRadar*);
}

static inline void StopAttachmentSound(s32 id)
{
	if (IsGameSoundPlaying(&gGame.mSound, id))
		StopGameSfx(&gGame.mSound, id);
}

static inline void ResetAttachmentJoint(XObjChr* player)
{
	HSD_JObj* const& joint = player->mAttachmentJoint;
	s32 path[]             = { 0, 1, 0, 1, 2, 0, 0, 5 };
	HSD_AnimJoint* anim    = player->mArchive.GetAnimJoint(0x814, 8, path);
	HSD_JObjAddAnimAll(joint, anim, NULL, NULL);
	HSD_JObjReqAnimAll(joint, 0.0f);
	HSD_JObjClearFlagsAll(joint, 0x20000);
	HSD_JObjAnimAll(joint);
}

/**
 * @note Address: 0x800BAE70
 * @note Size: 0x560
 */
extern "C" void ResetPlayerAttachment(XObjChr* player, s32 nextAnimation)
{
	switch (player->GetAttachment()) {
	case 1:
		StopAttachmentSound(8);
		player->mCopterObj.Hide();
		ResetAttachmentJoint(player);
		break;
	case 2:
		StopAttachmentSound(0x186);
		StopAttachmentSound(0x18B);
		player->mPopperObj.Hide();
		SetPlayerPopperMountHidden(player, FALSE);
		ResetPopperCharge(&player->mPopper.mCharge);
		break;
	case 3:
		StopAttachmentSound(0x18C);
		StopAttachmentSound(0x18D);
		StopAttachmentSound(0x18E);
		player->mRadarObj.Hide();
		ResetAttachmentJoint(player);
		break;
	case 4:
	case 5:
	case 6:
	case 7:
	default:
		if (player->mLift.mTarget) {
			if ((s16)player->mLift.mTarget->GetPool() == 8) {
				Inventory* inventory = &gGame.GetPlayer()->mParams.mInventory;
				SetOrAddInventoryItemCount(inventory, player->mLift.mTarget->GetItem(), 1, TRUE);
				((XObjItem*)player->mLift.mTarget)->SetFlag();
			}
			if ((s16)player->mLift.mTarget->GetPool() != 24)
				player->mLift.mTarget->Release();
		}
	}
	ResetPlayerLiftAction(&player->mLift);
	player->mHang.Reset();
	player->mJump.Reset();
	player->mLadder.Reset();
	player->mRope.Reset();
	ReleasePushAction(&player->mPush);
	ResetTumbleAction(&player->mTumble);
	player->mCopter.Reset();
	player->mPopper.Reset();
	ResetPlayerRadarAction(&player->mRadar);
	if (player->mJObj) {
		s32 path[] = { 0 };
		SetObjectJointTranslationByPath(player, 1, path, 0.0f, 0.0f, 0.0f);
		s32 root[] = { 0 };
		SetJointYawByPath(player, 1, root, 0.0f);
	}
	player->mAttachment                   = 0;
	player->mAttachmentRequest            = -1;
	player->mAttachmentStep               = 0;
	player->mScriptAttach                 = 0;
	player->mNextClimbType                = 0;
	player->mClimbType                    = 0;
	player->mHitObjectIndex               = -1;
	player->mActionObjectIndex            = -1;
	player->mFloorContacts[2].objectIndex = -1;
	gGame.mParticle.Kill(0x15);
	if (nextAnimation >= 0)
		player->SetAnim(nextAnimation, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	player->mFallDistance = 0.0f;
}

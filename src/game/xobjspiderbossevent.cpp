#include "game/xobjspiderboss.h"
#include "game/cgame.h"
#include "game/xchr.h"

extern "C" {
void OSReport(const char*, ...);
void EnableSpiderBossPartByIndex(s32);
void EnableSpiderBossCenterPart();
void PlaySpiderBossAttachmentAnimation(s32);
void CommandSpiderBossProjectiles(s32);
}

/**
 * @note Address: 0x8010535C
 * @note Size: 0x980
 */
s32 XObjSpiderBoss::HandleEventCommand(s32 count, s32* args)
{
	switch (args[0]) {
	case 100: {
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
			if (args[1] >= 10000) {
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
	default:
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
			gGame.mEvt._10 = 1;
		else
			gGame.mEvt._10 = 0;
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
	case 9:
		mBasePos.x       = mArgs[0] / 100.0f;
		mBasePos.y       = mArgs[1] / 100.0f;
		mBasePos.z       = mArgs[2] / 100.0f;
		_A10             = mBasePos;
		mMoveTranslate.x = mBasePos.x;
		mMoveTranslate.y = mBasePos.y;
		mMoveTranslate.z = mBasePos.z;
		mMoveTranslate.SetTarget(mMoveTranslate);
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
		for (int i = 0; i < 11; ++i)
			EnableSpiderBossPartByIndex(i);
		EnableSpiderBossCenterPart();
		mCommand = 0;
		break;
	case 17:
		if (mArgs[0] == 1) {
			mSpiderStatus.mFlags |= 0x20000;
			XObjChr* player     = gGame.GetPlayer();
			player->_980.mFlags = 0;
			player->_980.mFlags |= 0x40000000;
			player->mPad.mReset = FALSE;
			gGame.GetPlayer()->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else {
			mSpiderStatus.mFlags &= ~0x20000;
			XObjChr* player     = gGame.GetPlayer();
			player->_980.mFlags = 0;
			player->_980.mFlags |= 0x20000000;
			player->_980.mFlags |= 7;
			player->mPad.mReset = TRUE;
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

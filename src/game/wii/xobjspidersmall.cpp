#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "nw4r/snd/snd_BasicSound.h"
#include "nw4r/snd/snd_SoundHandle.h"

// the Wii file of the GameCube's xobjspidersmall.cpp and xobjspidersmallstatus.cpp (retail
// 0x80128C84-0x80132330), its functions in the Wii's order; the web effect helpers it calls
// are defined in xobjspider.cpp

struct SmallSpiderStageStatus {
	CStatus mSpiders[16];
};
STATIC_ASSERT(sizeof(SmallSpiderStageStatus) == 0x80);

extern "C" {
void ResetSmallSpiderState(XObjSpiderSmall* spider);
void DrawSmallSpiderWebEffect(XObjSpiderSmallFx* fx, u8 index);

extern s32 gRefreshRate;
extern u32 gSpiderSmallSoundId;
extern AnimSfxEvent gSmallSpiderAnimSfxEvents[];
extern XObj* gSmallSpiderEffectObjects[12];
extern s32 gSmallSpiderEffectSlotOwners[12];
extern SmallSpiderStageStatus gSmallSpiderStatusByStage[39];
extern XObj* gSmallSpiderWebFound;
extern u8 gSmallSpiderWebFlag;
}

/**
 * @note Address: 0x80128C84
 * @note Size: 0x174
 */
void XObjSpiderSmall::InitializeObject()
{
	XObj::InitializeObject();
	mStage = -1;
	ResetSmallSpiderState(this);
	SetState(0);
	mSpiderStatus.mFlags |= 0x4000000;
	gSpiderSmallSoundId = 0;
	mPrevBgm            = -1;
	mLoadPoolIndex      = -1;
	if (mPoolIndex == 0) {
		for (s32 i = 0; i < 0x27; i++) {
			for (int j = 0; j < 16; j++) {
				gSmallSpiderStatusByStage[i].mSpiders[j].mFlags = 0;
			}
		}
	}
	mTimeMode = gGame.mTimeState.mMode;
}

/**
 * @note Address: 0x80128DF8
 * @note Size: 0x16C
 */
void XObjSpiderSmall::ResetForNewGame()
{
	mStage = -1;
	ResetSmallSpiderState(this);
	SetState(0);
	mSpiderStatus.mFlags |= 0x4000000;
	gSpiderSmallSoundId = 0;
	mPrevBgm            = -1;
	mLoadPoolIndex      = -1;
	if (mPoolIndex == 0) {
		for (s32 i = 0; i < 0x27; i++) {
			for (int j = 0; j < 16; j++) {
				gSmallSpiderStatusByStage[i].mSpiders[j].mFlags = 0;
			}
		}
	}
	mTimeMode = gGame.mTimeState.mMode;
}

/**
 * @note Address: 0x80129C00
 * @note Size: 0x78
 */
void XObjSpiderSmall::ResetObjectState()
{
	XObj::ResetObjectState();
	mAnimSfx.Init((WiiPlayerState*)this, gSmallSpiderAnimSfxEvents);
	mPrevBgm = gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0;
	if (mPoolIndex == 0) {
		FindWebs();
	}
}

/**
 * @note Address: 0x80129C78
 * @note Size: 0xE4
 */
// NONMATCHING: regalloc, retail keeps gGame in r29 and the fade frames in r31
// lets go of the battle track it started
void XObjSpiderSmall::Release()
{
	if (gSpiderSmallSoundId == 0xFF000C) {
		s32 frames = 0.2f * gRefreshRate;
		for (int i = 0; i < 0x40; i++) {
			if (gGame.mSound.mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()
			    && gGame.mSound.mArchivePlayer.mTracks.mHandles[i].GetId() == 0xC) {
				gGame.mSound.mArchivePlayer.mTracks.mHandles[i].Stop(frames);
			}
		}
		gSpiderSmallSoundId = 0;
	}
	XObj::Release();
}

/**
 * @note Address: 0x8012AA50
 * @note Size: 0x14
 */
void XObjSpiderSmall::DrawObject()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x8012AA64
 * @note Size: 0xC
 */
void XObjSpiderSmall::DrawLate()
{
	DrawSmallSpiderWebEffect(&mFx, mFxIndex);
}

/**
 * @note Address: 0x8012AA70
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawGroundShadow(f32 opacity)
{
}

/**
 * @note Address: 0x8012ABEC
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawScreenOverlay()
{
}

/**
 * @note Address: 0x8012ABF0
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawDebugStatus()
{
}

/**
 * @note Address: 0x8012ABF4
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawObjectEffects()
{
}

/**
 * @note Address: 0x8012ABF8
 * @note Size: 0xAC
 */
s32 XObjSpiderSmall::HandleEventCommand(s32 n, s32* args)
{
	if (n < 1) {
		return 0;
	}
	mEvtArgs[0] = args[0];
	if (n < 2) {
		mEvtArgs[1] = 0;
	} else {
		mEvtArgs[1] = args[1];
	}
	if (n < 3) {
		mEvtArgs[2] = 0;
	} else {
		mEvtArgs[2] = args[2];
	}
	if (n < 4) {
		mEvtArgs[3] = 0;
	} else {
		mEvtArgs[3] = args[3];
	}
	if (n < 5) {
		mEvtArgs[4] = 0;
	} else {
		mEvtArgs[4] = args[4];
	}
	if (n < 6) {
		mEvtArgs[5] = 0;
	} else {
		mEvtArgs[5] = args[5];
	}
	return 0;
}

/**
 * @note Address: 0x8012ACA4
 * @note Size: 0x9C
 */
// keeps the hit for UpdatePlayerCaughtInSpiderWeb, unless one is pending or it can't be hit (0x10, or 0x8000 in its web)
void XObjSpiderSmall::OnHitFromPosition(const Vec& pos, f32 strength)
{
	if (!(mSpiderStatus.mFlags & 2) && !(mSpiderStatus.mFlags & 0x10) && !(mSpiderStatus.mFlags & 0x8000)) {
		XObj::OnHitFromPosition(pos, strength);
		mSpiderStatus.mFlags |= 2;
		Vec hitPos   = { pos.x, pos.y, pos.z };
		mHitPos      = hitPos;
		mHitStrength = strength;
	}
}

/**
 * @note Address: 0x8012AD40
 * @note Size: 0x18
 */
s32 XObjSpiderSmall::OnPlayerContact(s32 unused, s32 value)
{
	mSpiderStatus.mFlags |= 1;
	return value;
}

/**
 * @note Address: 0x80131DF4
 * @note Size: 0x13C
 */
// finds the stage's webs, the objects of kind 0x1BC
void XObjSpiderSmall::FindWebs()
{
	for (int i = 0; i < 12; i++) {
		gSmallSpiderEffectObjects[i]    = NULL;
		gSmallSpiderEffectSlotOwners[i] = -1;
	}
	gSmallSpiderWebFlag  = 0;
	gSmallSpiderWebFound = NULL;
	s32 found            = 0;
	for (s32 i = 0; gSmallSpiderWebFound == NULL && i < gGame.mXObjs.mObjectCount; i++) {
		XObj* obj = gGame.mXObjs.mObjects[i];
		if ((obj->mStatus.mFlags & 1) && obj->mCoord.mPlacement->mObjectId == 0x1BC) {
			gSmallSpiderEffectObjects[found] = obj;
			gSmallSpiderEffectObjects[found]->SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			found++;
			if (found >= 12) {
				break;
			}
		}
	}
}

#include "game/video.h"
#include "game/stg.h"
#include "game/xobjmap.h"
#include "game/xobjread.h"
#include "game/cgameutil.h"
#include "game/xchr.h"

extern "C" {
void UpdateStageMicrophone(u32 frames);
}

/**
 * @note Address: 0x80010CD0
 * @note Size: 0x48
 */
// CMenu::Open
extern "C" void Open__5CMenuFv(CMenu* menu)
{
	menu->mState = 1;
	menu->Resume();
	menu->mWindow.mOpen = TRUE;
}

/**
 * @note Address: 0x80010D18
 * @note Size: 0x10
 */
// CMenu::Resume
extern "C" void Resume__5CMenuFv(CMenu* menu)
{
	menu->mSuspended   = FALSE;
	menu->mSuspendFade = FALSE;
}

extern "C" {
void SetMusicPlaybackPaused(XSound* sound, BOOL pause);
s32 HSD_DevComIsBusy(s32 id);
void PollMemoryCardWorker(void* card);
void CMsg_Update(CMsg* msg, u32 frames);
void UpdateOpenMenus(void* menu, u32 frames);
void UpdateHudTransitionsAndIndicators(void* info, u32 frames);
void BuildObjectDrawList(CGame* game);
}

/**
 * @note Address: 0x8000FACC
 * @note Size: 0x1204
 */
// NONMATCHING: the frame count and the inlined base update's pad pointer swap registers
// CGame::Update: the base's disc and reset checks, the reset itself, then everything that
// runs each frame. while the disc can't be read only the sound is paused
void CGame::Update(u32 frames)
{
	s32 i;
	BaseUpdate(this, frames);
	PollMemoryCardWorker(&mMemcard);
	if (mReset && !mMemcard.mCard.mBusy && HSD_DevComIsBusy(1) <= 0 && HSD_DevComIsBusy(2) <= 0) {
		if (!mDiskOk) {
			mResetHold = 0;
			mReset     = FALSE;
			HSD_PadReset();
			Reset(1);
		} else if (_3338 != 0x17 && _3338 != 0x1A && !HasPendingObjectReads()) {
			mResetHold = 0;
			mReset     = FALSE;
			HSD_PadReset();
			if (!mDiskOk) {
				Reset(1);
			} else if (_3338 <= 0x13) {
				Reset(0);
			} else {
				ChangeGameStage(this, 10000, FALSE);
			}
		}
	}

	if (IsDiscError()) {
		if (!(mStatus.mFlags & 8)) {
			mStatus.mFlags |= 8;
			PauseGameMusic(&gGame.mSound, 0xFFFF0000, 0);
		}
		return;
	}
	if (mStatus.mFlags & 8) {
		mStatus.mFlags &= ~8;
		ResumeGameSfx(&gGame.mSound, 0xFFFF0000);
	}
	UpdateStageMicrophone(frames);
	if (mStatus.mFlags & 0x1000) {
		mStatus.mFlags &= ~0x1000;
		BuildObjectDrawList(this);
	}
	if ((mStatus.mFlags & 1) && !(mStatus.mFlags & 2) && !(mStatus.mFlags & 0x20) && !mMenu.mActive && mEvt.mData.mScript == NULL
	    && (mPad.mPad[0].mTrigger & 0x1000) && !(mPad.mPad[0].mButton & 0x200)
#if !defined(VERSION_JP)
	    && mMenu.mMenus[1] != NULL
#endif
	) {
		mMenu.mMenus[1]->Open();
		mMenu.mActive = TRUE;
	}
	if ((mStatus.mFlags & 1) && !(mStatus.mFlags & 2)) {
		for (i = 0; i < mDrawLists[0].mCount; i++) {
			XObj* obj = mDrawLists[0].mObjects[i];
			if (obj->mStatus.mFlags & 1) {
				if (obj->mMoveTranslate.y != obj->_264.y) {
					obj->mStatus.mFlags |= 0x10000000000000;
				}
				obj->_264 = obj->mMoveTranslate;
				obj->UpdateObject(frames);
			}
		}
	}
	if (IsRunning() && !(mStatus.mFlags & 2)) {
#if defined(VERSION_PAL)
		mPlayTime += frames * (60.0f / FRAME_RATE);
#else
		mPlayTime += frames;
#endif
		if (mPlayTime >= 21600000) {
			mPlayTime = 21599999;
		}
	}
	if (mStatus.mFlags & 0x2000000000000) {
		if (IsRunning() && mEvt.mData.mScript == NULL && !mMenu.mActive && !(mStatus.mFlags & 0xC00) && mTimeState.mDuration > 0.0f) {
			if (!mTimeState.mRunning) {
				mTimeState.mTime = frames * (mTimeState.mDuration / (600.0f * FRAME_RATE)) + mTimeState.mTime;
				if (mTimeState.mTime >= 1.0f) {
					mTimeState.mRunning = TRUE;
				}
			}
			if (mTimeState.mRunning == TRUE) {
				bool idle       = false;
				XObjChr* player = GetPlayer();
				if ((player->mLift.mState == 0 || player->mLift.mState == 4) && player->mHang.mState == 0 && player->mJump.mState == 0
				    && player->mLadder.mState == 0 && player->mRope.mState == 0 && player->mPush.mState == 0
				    && player->mTumble.mState == 0) {
					idle = true;
				}
				if (idle) {
					mTimeState.mRunning = FALSE;
					u8* script          = GetStageTimerExpiredScript();
					if (script != NULL) {
						StartOrQueueEventScript(&mEvt, script, 0, -1, -1);
					}
				}
			}
		}
	}
	if (!(mStatus.mFlags & 2)) {
		CEvt_Update(&mEvt, frames);
		for (i = 0; i < 2; i++) {
			CMsg_Update(&mCMsgs[i], frames);
		}
	}
	if (IsRunning()) {
		CallStageModule(1, frames, NULL);
	}
	if (IsRunning() || (mStatus.mFlags & 0x20)) {
		mParticle.Update(frames);
	}
	if ((mStatus.mFlags & 1) && !(mStatus.mFlags & 0x200000000000) && !mMenu.mActive) {
		mMotionControlCamera.Update(frames);
	}
	UpdateGameSound(&mSound, frames);
	if (!(mStatus.mFlags & 2)) {
		UpdateScreenFade(&mFade, frames);
		mWipeCircle.Update(frames);
		mWipeTex.Update(frames);
		mWipeCustom.Update(frames);
		mWipeTile0.Update(frames);
		mWipeTile1.Update(frames);
		if (mStatus.mFlags & 1) {
			mRoboeyes.Update(frames);
			UpdateScreenShake(&mQuake, frames);
			mOutline.Update(frames);
			mScrnTex.mAngle = fmod(0.10471976f * frames + mScrnTex.mAngle, 6.2831854820251465);
			mOutline.mThick = 0.0f;
			u8 lineAlpha    = 100.0f * (f32)sin(mScrnTex.mAngle) + 150.0f;
			mOutline.SetLineColor(0xFF, 0xFF, 0xFF);
			mOutline.mColor0.a = lineAlpha;
			UpdateScreenFade(&mScrnTex.mColor, frames);
			mOutline.SetColor2(mScrnTex.mColor.mColor.r, mScrnTex.mColor.mColor.g, mScrnTex.mColor.mColor.b);
			UpdateScreenFade(&mColor0.mColor, frames);
			mOutline.SetColor3(mColor0.mColor.mColor.r, mColor0.mColor.mColor.g, mColor0.mColor.mColor.b, mColor0.mColor.mColor.a);
			mBlur.Update(frames);
		}
	} else if (mMotionControlCamera.IsAction(4)) {
		UpdateScreenFade(&mFade, frames);
		mWipeCircle.Update(frames);
		mWipeTex.Update(frames);
		mWipeCustom.Update(frames);
		mWipeTile0.Update(frames);
		mWipeTile1.Update(frames);
	}
	UpdateScreenFade(&mColor1.mColor, frames);
	mOutline.SetColor4(mColor1.mColor.mColor.r, mColor1.mColor.mColor.g, mColor1.mColor.mColor.b, mColor1.mColor.mColor.a);
	if (_4724 > 0) {
		if (mPad.mPad[0].mSubStickY > 0.0f) {
			if ((_4728 -= 0.01f * frames) < 0.04f) {
				_4728 = 0.04f;
			}
		} else if (mPad.mPad[0].mSubStickY < 0.0f) {
			if ((_4728 += 0.01f * frames) > 0.5f) {
				_4728 = 0.5f;
			}
		}
		mOutline.mZoom = sin(3.1415927f * _4728 / 2.0f);
	}
	UpdateBootLogos(&mLogo, frames);
	UpdateTitleScreen(&mTitle, frames);
	UpdateOpenMenus(&mMenu, frames);
	if ((mStatus.mFlags & 1) && !(mStatus.mFlags & 2)) {
		UpdateHudTransitionsAndIndicators(&mInfoScrn, frames);
		mSubwin.Update(frames);
		for (i = 0; i < 5; i++) {
			if (!IsMirrorOff(&gFramebufferMirrors[i])) {
				gFramebufferMirrors[i].FbMirror::Update(frames);
			}
		}
	}
	if (!(mStatus.mFlags & 2) && mStageName.mActive && mStageName.mActive > 0) {
		for (i = 0; i < mStageName.mCount; i++) {
			if (mStageName.mChars[i].mWait > 0.0f) {
				if (mStageName.mChars[i].mScale < 1.0f) {
					if ((mStageName.mChars[i].mScale += (1.0f / (0.3f * FRAME_RATE)) * frames) > 1.0f) {
						mStageName.mChars[i].mScale = 1.0f;
					}
				} else if (mStatus.mFlags & 1) {
					if ((mStageName.mChars[i].mWait -= frames) < 0.0f) {
						mStageName.mChars[i].mWait = 0.0f;
					}
				}
			} else if (mStageName.mChars[i].mScale > 0.0f) {
				if ((mStageName.mChars[i].mScale -= (1.0f / (0.3f * FRAME_RATE)) * frames) < 0.0f) {
					mStageName.mChars[i].mScale = 0.0f;
				}
			}
		}
		s32 moving = 0;
		f32 bob    = 0.08726646f * frames;
		f32 tilt   = 0.05235988f * frames;
		for (i = 0; i < mStageName.mCount; i++) {
			if (0.0f != mStageName.mChars[i].mScale) {
				moving++;
				mStageName.mChars[i].mBob += bob;
				mStageName.mChars[i].mTilt += tilt;
			}
		}
		if (moving == 0) {
			mStageName.mActive = FALSE;
		}
	}
	if (mStatus.mFlags & 2) {
		if (!(mStatus.mFlags & 4)) {
			mStatus.mFlags |= 4;
			SetMusicPlaybackPaused(&gGame.mSound, TRUE);
			CPadEx* pad = &mPad;
			for (i = 0; i < 4; i++) {
				if (pad->mPad[i].mConnected) {
					HSD_PadRumbleOffN(pad->mPad[i].mPort);
				}
			}
			HSD_PadRumblePauseAll();
		}
	} else if (mStatus.mFlags & 4) {
		mStatus.mFlags &= ~4;
		SetMusicPlaybackPaused(&gGame.mSound, FALSE);
		HSD_PadRumbleActiveAll();
	}
}

extern "C" {
void SelectSceneLights(CGame* game);
}

/**
 * @note Address: 0x80010D28
 * @note Size: 0x548
 */
// NONMATCHING: register allocation, the counters and loop pointers come out in other registers
// sorts the objects into the ones to draw and the ones mirrors show, those of pool 3 first,
// then the plain ones, then the 0xA00000 ones, then the 0x40 ones. objects out of view are only
// mirrored
extern "C" void BuildSceneDrawLists(CGame* game)
{
	s32 shown;
	XObj* obj;
	CMotionControlCamera* camera;
	s32 i;
	s32 mirrored;
	if (game->mStatus.mFlags & 0x8000000000) {
		camera = &game->mMotionControlCamera;
		for (i = 0; i < game->mXObjs.mObjectCount; i++) {
			obj = game->mXObjs.mObjects[i];
			if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
				if (!(game->mStatus.mFlags & 0x100) && (obj->mStatus.mFlags & 0x80000000) && obj->vf11C(&camera->mCamera)) {
					obj->mStatus.mFlags |= 0x1000000000000;
				} else {
					obj->mStatus.mFlags &= ~0x1000000000000;
				}
			}
		}
	}

	shown = mirrored = 0;
	for (i = 0; i < game->mXObjs.mPools[3].mUsed; i++) {
		obj = game->mXObjs.mPools[3].Get(i);
		if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && !(obj->mStatus.mFlags & 0xA00040)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000)) {
				game->mDrawLists[1].mObjects[shown++] = obj;
			}
			game->mDrawLists[2].mObjects[mirrored++] = obj;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = game->mXObjs.mObjects[i];
		if (obj->GetCategory() != 3 && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && !(obj->mStatus.mFlags & 0xA00040)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000)) {
				game->mDrawLists[1].mObjects[shown++] = obj;
			}
			game->mDrawLists[2].mObjects[mirrored++] = obj;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = game->mXObjs.mObjects[i];
		if (obj->GetCategory() != 3 && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && (obj->mStatus.mFlags & 0xA00000)
		    && !(obj->mStatus.mFlags & 0x40)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000)) {
				game->mDrawLists[1].mObjects[shown++] = obj;
			}
			game->mDrawLists[2].mObjects[mirrored++] = obj;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = game->mXObjs.mObjects[i];
		if (obj->GetCategory() != 3 && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && (obj->mStatus.mFlags & 0x40)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000)) {
				game->mDrawLists[1].mObjects[shown++] = obj;
			}
			game->mDrawLists[2].mObjects[mirrored++] = obj;
		}
	}
	game->mDrawLists[1].mCount = shown;
	game->mDrawLists[2].mCount = mirrored;
}

/**
 * @note Address: 0x80011270
 * @note Size: 0x1FC
 */
// NONMATCHING: the object and the loop pointers get other volatile registers
// lists the objects to draw: those with flag 0x4000 first, then the rest, then the two
// special ones
extern "C" void BuildObjectDrawList(CGame* game)
{
	s32 count = 0;
	s32 i;
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		if (game->mXObjs.mObjects[i] != game->mXObjs.mPools[2].First() && game->mXObjs.mObjects[i] != game->mXObjs.mPools[0].First()
		    && (game->mXObjs.mObjects[i]->mStatus.mFlags & 1) && (game->mXObjs.mObjects[i]->mStatus.mFlags & 0x4000)) {
			game->mDrawLists[0].mObjects[count] = game->mXObjs.mObjects[i];
			game->mDrawLists[0].mIndices[count] = i;
			count++;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		if (game->mXObjs.mObjects[i] != game->mXObjs.mPools[2].First() && game->mXObjs.mObjects[i] != game->mXObjs.mPools[0].First()
		    && (game->mXObjs.mObjects[i]->mStatus.mFlags & 1) && !(game->mXObjs.mObjects[i]->mStatus.mFlags & 0x4000)) {
			game->mDrawLists[0].mObjects[count] = game->mXObjs.mObjects[i];
			game->mDrawLists[0].mIndices[count] = i;
			count++;
		}
	}
	if (game->mXObjs.mPools[2].First()->mStatus.mFlags & 1) {
		game->mDrawLists[0].mObjects[count++] = game->mXObjs.mPools[2].First();
	}
	if (game->mXObjs.mPools[0].First()->mStatus.mFlags & 1) {
		game->mDrawLists[0].mObjects[count++] = game->mXObjs.mPools[0].First();
	}
	game->mDrawLists[0].mCount = count;
}

/**
 * @note Address: 0x8001146C
 * @note Size: 0x1A0
 */
// NONMATCHING: the light count isn't read again for the second GetLights
// picks the lights to draw with: an object's own when it has some, else the game's. only the
// object lights cast shadows then
extern "C" void SelectSceneLights(CGame* game)
{
	game->mLightList = NULL;
	XObjMap* map     = (XObjMap*)game->mXObjs.mPools[3].mObjects;
	if ((map->mStatus.mFlags & 1) && map->GetLights() != NULL) {
		game->mLightList = map->GetLights();
	} else {
		XObjLight* sceneLight = (XObjLight*)game->mXObjs.mPools[1].mObjects;
		if (sceneLight->mStatus.mFlags & 1) {
			game->mLightList = sceneLight->GetLights();
		}
	}

	if (game->mLightList != NULL) {
		CLight* light;
		for (light = game->mLightList; light != NULL; light = (CLight*)light->next) {
			if (light->mShadow.IsActive()) {
				HSD_CObjSetProjectionType(light->mShadow.mShadow->camera, 3);
			}
		}
		for (light = game->mLights; light != NULL; light = (CLight*)light->next) {
			if (light->mShadow.IsActive()) {
				light->mShadow.SetActive(FALSE);
			}
		}
	} else {
		game->mLightList = game->mLights;
	}
}

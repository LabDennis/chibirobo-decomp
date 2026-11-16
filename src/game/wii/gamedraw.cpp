#include "nw4r/math/math_types.h"
#include "game/wii/cgame.h"
#include "game/cthread.h"
#include "game/wii/xobj.h"
#include "Dolphin/os.h"
#include "Dolphin/vi.h"
#include <string.h>
#include "nw4r/snd/snd_BasicSound.h"
#include "nw4r/snd/snd_SoundHandle.h"
#include "game/wii/csound3dactor.h"
#include "game/wii/cpadact.h"
#include "game/wii/infoscrn.h"
#include "game/wii/msg.h"
#include "game/wii/fbeffect.h"
#include "game/fbtexeff.h"
#include "game/cmdline.h"
#include "game/mm.h"
#include <math.h>
#include <stdlib.h>

extern "C" {
void* UpdateMovie(void* arg);
char* FormatToStaticBuffer(const char* fmt, ...);
const char* GetLocalizedInterfaceText(s32 id);
extern CThread gMovieUpdateThread;
extern u8* gMovieUpdateThreadStack;
s32 CallStageModule(s32 function, s32 count, s32* args);
void fn_802AEDEC(WiiGsSceneObj** head, Mtx view); // sorts the queued scene draws under view, as title.cpp calls it
void ClearMainMenuExtraRows(void* menu);
void fn_80094904(void* evt, BOOL, BOOL, BOOL, BOOL); // resets the event state, the GameCube's ResetEventScriptState
void ResetMsgState(CMsg* msg);
void UnloadStageDataAndModule();
BOOL fn_80246BD8(void* handle);                     // the emitter handle is valid
void* fn_80246C04(void* handle);                    // the emitter behind it
void fn_802BEA5C(void* emitter, BOOL);              // kills the emitter
void fn_802BE3A8(void* particles, u32 group, BOOL); // kills a particle group
void ClearSoundEffects(XSound* sound, s32 frames);
s32 GetWiiSaveSlotStage(WiiSaveManager* manager, int index);
void ResetTitleScreenState(void* title);
void fn_800848C0(void* menus); // the GameCube's ResetMenuGroup
void ResetGameHudState(InfoScrn* scrn);
u8* GetStageDisplayName(s32 index);
extern GXColor gMessageFontDefaultColor;
s32 GetStageNum();
s32 GetDefaultStageIndex();
}

extern s32 gRefreshRate;
// a sound id XObjSpiderSmall keeps while it fights (0x80128CD4), cleared once its tracks are let go
extern u32 gSpiderSmallSoundId;
extern s32 gPointerScreenWidth;
extern s32 gPointerScreenHeight;
extern s32 gScreenLeft;
extern s32 gScreenTop;
extern s32 gScreenRight;
extern s32 gScreenBottom;
#include "revolution/mem.h"

// the expanded heap mm::InitMem1 creates over the MEM1 arena; every operator delete site reads it
extern MEMiHeapHead* gWiiMem1Heap;

// this file's deletes go straight to the MEM1 heap
inline void operator delete(void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}

// a lead byte of a two byte shift jis character
static inline bool IsSJISByte(char c)
{
	return ((u8)c >= 0x81 && (u8)c <= 0x9F) || ((u8)c >= 0xE0 && (u8)c <= 0xFC);
}

static inline bool IsSJISLead(char c)
{
	return IsSJISByte(c) ? true : false;
}

inline void CSoundTrackSlot::Stop(int fadeFrames)
{
	if (IsAttachedSound()) {
		((nw4r::snd::detail::BasicSound*)mTrack)->Stop(fadeFrames);
	}
}

// as gamemain.cpp defines it
inline void CSound::StopAllTracks(f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (int i = 0; i < 0x40; ++i) {
		if (mTracks[i].IsAttachedSound()) {
			mTracks[i].Stop(frames);
		}
	}
}

// the queued scene draws, sorted under the camera's view and popped one by one
static inline void DrawSceneQueue(CCamera* camera)
{
	fn_802AEDEC(&gGame.mGs.mSceneHead, camera->mViewMtx);
	WiiGsSceneObj* obj = gGame.mGs.mSceneHead;
	while (obj->mPending != 0) {
		if (obj->mPending != 0) {
			obj->vf038(obj->mPending - 1);
		}
	}
}

// shortcut: csound.h spells the 64 tracks as flat slots; drop the cast once they are CSoundHandle
static inline nw4r::snd::SoundHandle& TrackHandle(s32 i)
{
	return *(nw4r::snd::SoundHandle*)&gGame.mSound.mTracks[i].mTrack;
}

// the first track playing sound id, as retail expands it ahead of the jingle loops
static inline CSoundTrackSlot* FindTrack(u32 id)
{
	for (int i = 0; i < 0x40; i++) {
		if (TrackHandle(i).IsAttachedSound() && TrackHandle(i).GetId() == id) {
			return &gGame.mSound.mTracks[i];
		}
	}
	return NULL;
}

// whether the first track playing sound id is paused
static inline bool IsTrackPaused(u32 id)
{
	for (int i = 0; i < 0x40; i++) {
		if (TrackHandle(i).IsAttachedSound() && TrackHandle(i).GetId() == id) {
			return TrackHandle(i).IsPause();
		}
	}
	return false;
}

/**
 * @note Address: 0x8002CEF8
 * @note Size: 0xC0
 */
s32 CGame::FrameInterval()
{
	return (IsLogoActive() || IsTitleActive() || mMenuActive || mHomeMenu.IsOpen()) ? 1 : 2;
}

/**
 * @note Address: 0x8002CEEC
 * @note Size: 0xC
 */
CLight* CGame::GetLight()
{
	return mLightList;
}

/**
 * @note Address: 0x8002CED0
 * @note Size: 0x1C
 */
void CBase::ToggleStatusFlags(u64 mask)
{
	mStatus.mFlags ^= mask;
}

/**
 * @note Address: 0x8002CECC
 * @note Size: 0x4
 */
void CBase::vf70()
{
}

/**
 * @note Address: 0x8002CEB4
 * @note Size: 0x18
 */
void CCamera::SetInterest(f32 x, f32 y, f32 z)
{
	mView.mInterest.x = x;
	mView.mInterest.y = y;
	mView.mInterest.z = z;
	mView.mDirty      = TRUE;
}

/**
 * @note Address: 0x8002CE9C
 * @note Size: 0x18
 */
void CCamera::SetUp(f32 x, f32 y, f32 z)
{
	mView.mUp.x  = x;
	mView.mUp.y  = y;
	mView.mUp.z  = z;
	mView.mDirty = TRUE;
}

/**
 * @note Address: 0x8002CE84
 * @note Size: 0x18
 */
void CCamera::SetEye(f32 x, f32 y, f32 z)
{
	mView.mEye.x = x;
	mView.mEye.y = y;
	mView.mEye.z = z;
	mView.mDirty = TRUE;
}

/**
 * @note Address: 0x8002CE78
 * @note Size: 0xC
 */
extern "C" void StopGameMovie(CGame* game)
{
	game->mMovie.Stop();
}

/**
 * @note Address: 0x8002CD84
 * @note Size: 0xF4
 */
// NONMATCHING: only the path's address differs, retail addresses it from the whole file's string pool
// starts a movie; only the staff roll gets the thread that steps it
extern "C" void StartGameMovie(CGame* game, const char* path, BOOL loop, u32 flags, f32 volume)
{
	game->mMovieFlags = flags;
	game->mMovie.Start(path, volume, loop, flags & 1, TRUE);
	game->mMovieRect[0] = game->mMovieRect[1] = game->mMovieRect[2] = game->mMovieRect[3] = NAN;
	game->mMovieVolume                                                                    = volume;
	game->mMovieOnThread                                                                  = strcmp(path, "movie/staff_roll.thp") == 0;
	if (game->mMovieOnThread) {
		gMovieUpdateThread.Start(UpdateMovie, game, gMovieUpdateThreadStack, 4);
	}
}

/**
 * @note Address: 0x8002CBE8
 * @note Size: 0x19C
 */
// NONMATCHING: retail keeps the loop condition as a bool at both of its ends and hoists the HOME
// Menu's fields out of the loop; the strings are addressed from the whole file's pool
// the movie thread: steps the movie once a retrace until it's done, and holds it while the game is
// interrupted
extern "C" void* UpdateMovie(void* arg)
{
	CGame* game   = (CGame*)arg;
	u32 count     = VIGetRetraceCount();
	CMovie* movie = &game->mMovie;
	do {
		while (count == VIGetRetraceCount()) {
			VIWaitForRetrace();
			VIWaitForRetrace();
		}
		count = VIGetRetraceCount();
		if (game->GetStatusFlags(1) && game->mSystemMessage == GetLocalizedInterfaceText(3) && game->mSystemMessageAlpha > 0.0f) {
			continue;
		}
		if (movie->mState != 0 && !game->mHomeMenu.IsOpen() && !game->mResetRequested && !game->GetStatusFlags(0x0200000000000002)) {
			s32 result = movie->Update();
			if (result < 0) {
				movie->Stop();
				OSPanic("game.cpp", 5783, FormatToStaticBuffer("error on play movie! - %d\n", result));
			}
		}
	} while (movie->mState != 0);
	return NULL;
}

/**
 * @note Address: 0x8002C044
 * @note Size: 0x254
 */
// NONMATCHING: the second loop's counter and the count of moving letters swap registers
// steps the stage name's letters: each grows in, waits while the game runs, and shrinks away, bobbing
// and rocking all the while; the name is done once every letter is gone
extern "C" void UpdateStageNameBubbles(CGame* game, u32 frames)
{
	if (game->mStageName.mActive && game->mStageName.mActive > 0) {
		for (s32 i = 0; i < game->mStageName.mCount; i++) {
			if (game->mStageName.mChars[i].mWait > 0.0f) {
				if (game->mStageName.mChars[i].mScale < 1.0f) {
					if ((game->mStageName.mChars[i].mScale += (1.0f / (0.3f * gRefreshRate)) * frames) > 1.0f) {
						game->mStageName.mChars[i].mScale = 1.0f;
					}
				} else if (game->GetStatusFlags(1)) {
					if ((game->mStageName.mChars[i].mWait -= frames) < 0.0f) {
						game->mStageName.mChars[i].mWait = 0.0f;
					}
				}
			} else if (game->mStageName.mChars[i].mScale > 0.0f) {
				if ((game->mStageName.mChars[i].mScale -= (1.0f / (0.3f * gRefreshRate)) * frames) < 0.0f) {
					game->mStageName.mChars[i].mScale = 0.0f;
				}
			}
		}
		s32 moving = 0;
		for (s32 i = 0; i < game->mStageName.mCount; i++) {
			if (0.0f != game->mStageName.mChars[i].mScale) {
				moving++;
				game->mStageName.mChars[i].mBob += DEG2RAD * 5.0f * frames;  // bobs five degrees a frame
				game->mStageName.mChars[i].mTilt += DEG2RAD * 3.0f * frames; // tilts three
			}
		}
		if (moving == 0) {
			game->mStageName.mActive = FALSE;
		}
	}
}

/**
 * @note Address: 0x8002BC40
 * @note Size: 0x404
 */
// sets the stage's name up to pop in letter by letter in color, each letter waiting wait seconds
// before it shrinks away; the slots behind the name stay blank
extern "C" void StartStageNameBubbles(CGame* game, u32 color, f32 wait)
{
	game->mStageName.mActive = TRUE;
	game->mStageName.mCount  = 0;
	if (game->mStage < GetStageNum()) {
		char* name = (char*)GetStageDisplayName(game->mStage);
		if (name != NULL) {
			game->mStageName.mColor = color;
			for (s32 i = 0; i < 20 && *name != '\0'; i++, name++) {
				game->mStageName.mChars[i].mText  = name;
				game->mStageName.mChars[i].mScale = -0.2f * i; // later letters start hidden
				game->mStageName.mChars[i].mWait  = wait * gRefreshRate;
				game->mStageName.mChars[i].mBob   = DEG2RAD * (rand() % 360); // a random phase
				game->mStageName.mChars[i].mTilt  = DEG2RAD * (rand() % 360);
				game->mStageName.mChars[i]._14    = 0;
				game->mStageName.mChars[i]._18    = 0.0f;
				game->mStageName.mCount++;
				if (gGame.mLanguage == 0 && IsSJISLead(*name)) {
					name++;
				}
			}
			for (s32 i = game->mStageName.mCount; i < 20; i++) {
				game->mStageName.mChars[i].mText  = NULL;
				game->mStageName.mChars[i].mScale = 0.0f;
				game->mStageName.mChars[i].mWait  = 0.0f;
				game->mStageName.mChars[i]._14    = 0;
				game->mStageName.mChars[i]._18    = 0.0f;
			}
		}
	}
}

/**
 * @note Address: 0x8002BBE0
 * @note Size: 0x60
 */
bool CGame::IsSystemMessage3Shown()
{
	bool shown = false;
	if (mSystemMessage == GetLocalizedInterfaceText(3) && mSystemMessageAlpha > 0.0f) {
		shown = true;
	}
	return shown;
}

/**
 * @note Address: 0x8002B6D8
 * @note Size: 0x500
 */
// NONMATCHING: the tracks are reached through a cast of CSound's flat track table (see gamesound.cpp)
// undoes one PauseGameSounds. the last resume lets the tracks, the objects' sounds and the movie go
// again; the music pause underneath holds the jingle (sound 4) until its own count runs out
extern "C" void ResumeGameSounds(CGame* game, f32 fade)
{
	// _D9B5C counts the pauses, _D9B60 the music pauses under them
	if (game->_D9B5C > 0 && --game->_D9B5C == 0) {
		s32 frames = gRefreshRate * fade;
		for (int i = 0; i < 0x40; i++) {
			nw4r::snd::SoundHandle& handle = TrackHandle(i);
			if (handle.IsAttachedSound() && handle.IsPause()) {
				handle.Pause(false, frames);
			}
		}
		for (s32 i = 0; i < game->mDrawRefCount; i++) {
			XObj* object = (XObj*)game->mDrawRefs[i].mObj;
			if (object->mStatus.mFlags & 1) {
				object->ResumeSounds(fade);
			}
		}
		if (game->mMovie.mState != 0) {
			THPSimpleSetVolume(127.0f * game->mMovieVolume, 1000.0f * fade);
		}
	}
	if (game->_D9B60 > 0) {
		if (--game->_D9B60 == 0) {
			if (gSpiderSmallSoundId != 0) {
				u32 music = gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0;
				if (music != 0) {
					s32 frames = gRefreshRate * 0.2f;
					u32 id     = music & 0xFFFF;
					for (int i = 0; i < 0x40; i++) {
						nw4r::snd::SoundHandle& handle = TrackHandle(i);
						if (handle.IsAttachedSound() && handle.GetId() == id && !handle.IsPause()) {
							handle.Pause(true, frames);
						}
					}
				}
			}
		} else if (FindTrack(4) != NULL && IsTrackPaused(4)) {
			s32 frames = gRefreshRate * fade;
			for (int i = 0; i < 0x40; i++) {
				nw4r::snd::SoundHandle& handle = TrackHandle(i);
				if (handle.IsAttachedSound() && handle.GetId() == 4 && handle.IsPause()) {
					handle.Pause(false, frames);
				}
			}
		}
	}
}

/**
 * @note Address: 0x8002B2F0
 * @note Size: 0x3E8
 */
// NONMATCHING: the tracks are reached through a cast of CSound's flat track table (see gamesound.cpp)
// holds every playing track, the objects' own sounds and the movie's volume; pauses nest, only the
// first one acts. a nested pause holds the jingle (sound 4) too
extern "C" void PauseGameSounds(CGame* game, f32 fade)
{
	if (game->_D9B5C == 0) {
		s32 frames = gRefreshRate * fade;
		for (int i = 0; i < 0x40; i++) {
			nw4r::snd::SoundHandle& handle = TrackHandle(i);
			if (handle.IsAttachedSound() && !handle.IsPause()) {
				handle.Pause(true, frames);
			}
		}
		for (s32 i = 0; i < game->mDrawRefCount; i++) {
			XObj* object = (XObj*)game->mDrawRefs[i].mObj;
			if (object->mStatus.mFlags & 1) {
				object->PauseSounds(fade);
			}
		}
		if (game->mMovie.mState != 0) {
			THPSimpleSetVolume(0, 1000.0f * fade);
		}
	}
	game->_D9B5C++;
	if (game->_D9B60 > 0 && FindTrack(4) != NULL && !IsTrackPaused(4)) {
		s32 frames = gRefreshRate * fade;
		for (int i = 0; i < 0x40; i++) {
			nw4r::snd::SoundHandle& handle = TrackHandle(i);
			if (handle.IsAttachedSound() && handle.GetId() == 4 && !handle.IsPause()) {
				handle.Pause(true, frames);
			}
		}
	}
	game->_D9B60++;
}

/**
 * @note Address: 0x8002B2EC
 * @note Size: 0x4
 */
void XObj::ResumeSounds(f32)
{
}

/**
 * @note Address: 0x8002B2E8
 * @note Size: 0x4
 */
void XObj::PauseSounds(f32)
{
}

/**
 * @note Address: 0x8002A330
 * @note Size: 0xB98
 */
// NONMATCHING: FbSubwin::Reset is a virtual call in retail (fbtexeff.h's Wii class has no CFbTexEff base
// yet); the float literals are addressed from the whole file's pool
// changes to another stage: 10000 is the title, 20000 the stage from the save, below 0 the next
// one. everything that belongs to the old stage is let go of, and the new stage's name is set up to
// pop in when showName is set
extern "C" void ChangeGameStage(CGame* game, s32 stage, BOOL showName)
{
	StartScreenFade(&game->mFade, 0, 0, 0, 255, 0, 0, 0, 255, 0.0f);
	if (game->mMovie.mState != 0) { // cut any playing movie
		game->mMovie.Stop();
	}
	ClearMainMenuExtraRows(game->mMainMenu);
	fn_80094904(game->_4C8E0, TRUE, TRUE, TRUE, TRUE); // the event state in front of mEvtData
	CMsg* msgs = (CMsg*)game->mMsgs;
	for (s32 i = 0; i < 2; i++) {
		ResetMsgState(&msgs[i]);
		CMsg_ResetPos(&msgs[i]);
		msgs[i].SetColor(gMessageFontDefaultColor);
	}
	CGame::FbMirrorEx* mirrors = (CGame::FbMirrorEx*)game->mMirrors;
	for (s32 i = 0; i < 5; i++) { // let go of the mirrors' objects and textures
		FbMirror* mirror = &mirrors[i];
		if (mirror->IsUsed() && mirror->mJoint != NULL) {
			if (mirror->mJointAnm != NULL) { // the object's own material animation goes back
				memcpy(mirror->mJointAnm, mirror->mJointAnmCopy, 0x20);
				mirror->mJointAnm = NULL;
			}
			mirror->mJoint   = NULL;
			mirror->mVisible = FALSE;
			if (mirror->mTexture.mOwnsImage && mirror->mTexture.mImage != NULL) {
				mm::Free(mirror->mTexture.mImage);
				mirror->mTexture.mImage = NULL;
			}
		}
	}
	UnloadStageDataAndModule();
	for (s32 i = 0; i < 0x80; i++) { // kill every emitter still running
		void* emitter = game->mEmitters[i];
		s32 state     = fn_80246BD8(emitter) ? *(s32*)((u8*)fn_80246C04(emitter) + 0xC) : 0;
		if (!(state == 0 || state == 3)) {
			fn_802BEA5C(emitter, TRUE);
		}
	}
	for (u32 i = 0; i < 6; i++) {
		fn_802BE3A8(game->_B7A8, i, TRUE);
	}
	WiiPlayerState* player = game->mPlayer;
	for (s32 i = 0; i < 2; i++) {
		CSound3DActor* actor          = player->mSounds[i].mActor;
		nw4r::snd::SoundHandle* sound = actor != NULL && actor->mHandle.IsAttachedSound() ? &actor->mHandle : NULL;
		if (sound != NULL && actor != NULL) {
			actor->mHandle.Stop(gRefreshRate * 0.0f);
		}
	}
	gGame.mSound.StopAllTracks(0.0f);
	ClearSoundEffects(&gGame.mSound, 0);
	gGame.mSound.RestoreHeapLevel(1);
	Vec nan        = { NAN, NAN, NAN };
	game->_EE1F0.x = nan.x;
	game->_EE1F0.y = nan.y;
	game->_EE1F0.z = nan.z;
	gWiiPointer.ResetRumble(FALSE);
	game->ClearStatusFlags(0x020000000000002F);
	game->ClearStatusFlags(0xC00);
	game->ClearStatusFlags(0x8000);
	game->ClearStatusFlags(0x300);
	game->mRoboeyes.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	game->SetStatusFlags(0x0040000000000000);
	game->SetStatusFlags(0x0000800000000000);
	InfoScrn* scrn = (InfoScrn*)game->mInfoScrn;
	InfoScrn_ApplyScoreHudCommand(scrn, 0, 0);
	for (s32 i = 0; i < 4; i++) {
		InfoScrn_ApplyObjectGaugeCommand(scrn, NULL, 0, 0, i);
	}

	if (stage == 10000) {
		if (gCmdLineArgs.mStage < 0 || game->GetStatusFlags(0x0008000000000000)) {
			game->SetStage(GetDefaultStageIndex());
		} else {
			game->SetStage(gCmdLineArgs.mStage);
		}
	} else if (stage == 20000) {
		game->SetStage(GetWiiSaveSlotStage(&game->mSaveManager, 1));
		game->SetStage(GetWiiSaveSlotStage(&game->mSaveManager, 0));
		game->mMemcardLoaded = TRUE;
	} else if (stage >= 0) {
		game->SetStage(stage);
	} else {
		game->mPrevStage = game->mStage;
		if (++game->mStage >= game->mRoomCount) {
			game->mStage = 0;
		}
	}

	if (stage == 10000) {
		game->SetStatusFlags(0x20000000);
		game->SetStatusFlags(0x0001000000000000);
		game->ClearStatusFlags(0x80);
		ResetTitleScreenState(game->mTitle);
		fn_800848C0(&game->mMenuActive); // the menu group starts at its active word
		ResetGameHudState(scrn);
		((FbSubwin*)game->mSubwin)->Reset();
		StartScreenFade(&game->mFade, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f);
		game->mWipeCircle.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeTex.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeCustom.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeTile.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeCine.StartTransition(0, 0, 0, 0, 0, 0, 0, gPointerScreenWidth / 2, gPointerScreenHeight / 2, gPointerScreenWidth,
		                                gPointerScreenHeight, gPointerScreenWidth, gPointerScreenHeight, 0.0f, 0, TRUE);
		StartScreenShake(&game->mQuake, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		FbBlur* blur    = (FbBlur*)game->mBlur;
		blur->mAlpha    = 0.0f;
		blur->mTarget   = 0.0f;
		blur->mSpeed    = 0.0f;
		game->mLoadStep = 20;
	} else if (stage == 20000) {
		((FbSubwin*)game->mSubwin)->Reset();
		StartScreenFade(&game->mFade, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f);
		game->mWipeCircle.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeTex.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeCustom.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeTile.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeCine.StartTransition(0, 0, 0, 0, 0, 0, 0, gPointerScreenWidth / 2, gPointerScreenHeight / 2, gPointerScreenWidth,
		                                gPointerScreenHeight, gPointerScreenWidth, gPointerScreenHeight, 0.0f, 0, TRUE);
		StartScreenShake(&game->mQuake, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		FbBlur* blur    = (FbBlur*)game->mBlur;
		blur->mAlpha    = 0.0f;
		blur->mTarget   = 0.0f;
		blur->mSpeed    = 0.0f;
		game->mLoadStep = 21;
	} else {
		game->mLoadStep = 22;
	}
	game->_41F0 = 3;
	if (showName) {
		StartStageNameBubbles(game, 0x80FFFF00, 2.0f); // pale cyan for two seconds
	}
}

/**
 * @note Address: 0x8002AEC8
 * @note Size: 0xCC
 */
void CGame::ProjectOffsetToScreen(const Vec* pos, f32* x, f32* y, f32* z, void* camera)
{
	CCamera* cam = camera != NULL ? (CCamera*)camera : &gGame.mMotionControlCamera;
	f32 sx;
	Vec screen;
	GXProject(pos->x, pos->y, pos->z, cam->mViewMtx, &cam->mIsOrtho, cam->mViewport, &sx, &screen.y, &screen.z);
	f32 scale = cam->mScale;
	screen.x  = sx * scale;
	if (x != NULL) {
		*x = screen.x;
	}
	if (y != NULL) {
		*y = screen.y;
	}
	if (z != NULL) {
		*z = screen.z;
	}
}

/**
 * @note Address: 0x80026438
 * @note Size: 0x8
 */
CLight* CBase::GetScreenLight()
{
	return &mLights[8];
}

/**
 * @note Address: 0x80029DF4
 * @note Size: 0x53C
 */
// NONMATCHING: retail calls the Wii MSL's out-of-line __fpclassifyf, see XObj::GetCollisionHeight
// draws the movie, centred at its own size unless mMovieRect says otherwise. with alpha it's drawn
// three times over, without it over a black screen
extern "C" void DrawGameMovie(CGame* game)
{
	if (game->mHomeMenu.IsRunning()) {
		return;
	}
	if (game->mMovie.mState != 0) {
		f32 w   = fpclassify(game->mMovieRect[2]) == FP_NAN ? (f32)game->mMovie.mVideoInfo.xSize : game->mMovieRect[2];
		f32 h   = fpclassify(game->mMovieRect[3]) == FP_NAN ? (f32)game->mMovie.mVideoInfo.ySize : game->mMovieRect[3];
		f32 x   = fpclassify(game->mMovieRect[0]) == FP_NAN ? ((f32)gPointerScreenWidth - w) * 0.5f : game->mMovieRect[0];
		f32 y   = fpclassify(game->mMovieRect[1]) == FP_NAN ? ((f32)gPointerScreenHeight - h) * 0.5f : game->mMovieRect[1];
		CGs* gs = &gGame.mGs;
		gs->mCurRenderState->SetColorUpdate(GX_TRUE);
		gs->mCurRenderState->SetAlphaUpdate(GX_FALSE);
		gs->mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		// the Start flag's bit 0, which cmovie.h's Wii branch calls mVolumeTime: the movie carries alpha
		if (game->mMovie.mVolumeTime) {
			gs->mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gs->mCurRenderState->SetAlphaCompare(GX_GEQUAL, 0x20, GX_AOP_AND, GX_EQUAL, 255);
			if (gs->mCurRenderState != NULL) {
				gs->mCurRenderState->Apply();
			}
			if (gGame.mGs.mCurTevStage != NULL) {
				gGame.mGs.mCurTevStage->Apply();
			}
			if (gGame.mGs.mCurChannel != NULL) {
				gGame.mGs.mCurChannel->Apply();
			}
			if (game->mMovie.BeginDraw() >= 0) {
				for (s32 i = 0; i < 3; i++) {
					game->mMovie.Draw(x, y, 0.0f, w, h);
				}
				game->mMovie.EndDraw();
			}
		} else {
			gs->mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gs->mCurRenderState->SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 255);
			if (gs->mCurRenderState != NULL) {
				gs->mCurRenderState->Apply();
			}
			if (gGame.mGs.mCurTevStage != NULL) {
				gGame.mGs.mCurTevStage->Apply();
			}
			if (gGame.mGs.mCurChannel != NULL) {
				gGame.mGs.mCurChannel->Apply();
			}
			Vec quad[4] = { 0 };
			quad[0].x   = (f32)gScreenLeft;
			quad[0].y   = (f32)gScreenTop;
			quad[1].x   = (f32)gScreenRight;
			quad[1].y   = (f32)gScreenTop;
			quad[2].x   = (f32)gScreenRight;
			quad[2].y   = (f32)gScreenBottom;
			quad[3].x   = (f32)gScreenLeft;
			quad[3].y   = (f32)gScreenBottom;
			DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, 0xFF, quad[1].x, quad[1].y, quad[1].z, 0xFF, quad[2].x, quad[2].y,
			                 quad[2].z, 0xFF, quad[3].x, quad[3].y, quad[3].z, 0xFF);
			game->mMovie.DrawFrame(x, y, 0.0f, w, h);
		}
		gGame.mGs.mCurRenderState->SetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_EQUAL, 255);
	}
}

/**
 * @note Address: 0x800287F0
 * @note Size: 0x1A8
 */
// NONMATCHING: retail leaves through a branch pair after the menu test and keeps the scene object in
// r30
// every live object's DrawScreenOverlay, then the stage's function 4 and the queued scene draws;
// nothing while the HOME Menu or a menu covers the frame
void CGame::DrawHud()
{
	if (GetStatusFlags(1)) {
		if (mHomeMenu.IsRunning()) {
			return;
		}
		bool menuShown = mMenuActive && _AE34;
		if (menuShown) {
			return;
		}
		for (s32 i = 0; i < mObjectCount; i++) {
			XObj* object = (XObj*)mObjects[i];
			if (object->mStatus.mFlags & 1) {
				object->DrawScreenOverlay();
			}
		}
		if (mHudObject != NULL) {
			mHudObject->vf19C(0xFF0000C8, 0.0f, mHudObjectX, mHudObjectY);
		}
		CallStageModule(4, 0, NULL);
		fn_802AEDEC(&gGame.mGs.mSceneHead, GetCamera()->mViewMtx);
		WiiGsSceneObj* obj = gGame.mGs.mSceneHead;
		while (obj->mPending != 0) {
			if (obj->mPending != 0) { // retail tests the count again before it pops a draw
				obj->vf038(obj->mPending - 1);
			}
		}
	}
}

/**
 * @note Address: 0x80027D48
 * @note Size: 0x13C
 */
// the effects that stay over the screen color while it is up: the 0x200 objects, and the ones that
// carry both of the flags 0x0400000000000000 and 0x0000400000000000
extern "C" void DrawObjectEffectsOverFade(CGame* game)
{
	if (!game->GetStatusFlags(1)) {
		return;
	}
	if (game->mHomeMenu.IsRunning()) {
		return;
	}
	bool menuShown = game->mMenuActive && game->_AE34;
	if (menuShown) {
		return;
	}
	if (game->mColor0.mColor.mColor.a > 0.0f) {
		for (s32 i = 0; i < game->mEffectRefCount; i++) {
			XObj* obj = (XObj*)game->mEffectRefs[i].mObj;
			if (obj->mStatus.mFlags & 1) {
				if (obj->mStatus.mFlags & 0x200) {
					obj->DrawObjectEffects();
				} else if ((obj->mStatus.mFlags & 0x0400000000000000) && (obj->mStatus.mFlags & 0x0000400000000000)) {
					obj->DrawObjectEffects();
				}
			}
		}
	}
}

/**
 * @note Address: 0x80027D44
 * @note Size: 0x4
 */
void XObj::DrawObjectEffects()
{
}

/**
 * @note Address: 0x80027BF4
 * @note Size: 0x150
 */
// every live object's effects, then the stage's function 3; while the screen color is up the ones
// DrawObjectEffectsOverFade draws over it are left out
extern "C" void DrawObjectEffectsPass(CGame* game)
{
	if (!game->GetStatusFlags(1)) {
		return;
	}
	if (game->mHomeMenu.IsRunning()) {
		return;
	}
	bool menuShown = game->mMenuActive && game->_AE34;
	if (menuShown) {
		return;
	}
	for (s32 i = 0; i < game->mEffectRefCount; i++) {
		XObj* obj = (XObj*)game->mEffectRefs[i].mObj;
		if (obj->mStatus.mFlags & 1) {
			if (game->mColor0.mColor.mColor.a > 0.0f
			    && ((obj->mStatus.mFlags & 0x200)
			        || ((obj->mStatus.mFlags & 0x0400000000000000) && (obj->mStatus.mFlags & 0x0000400000000000)))) {
				continue;
			}
			obj->DrawObjectEffects();
		}
	}
	CallStageModule(3, 0, NULL);
}

/**
 * @note Address: 0x800276F8
 * @note Size: 0x4FC
 */
// draws the scene list through the main camera: kind 3 first, then the rest, then once more into the
// alpha only (the 0x820040 objects and what the player carries) for the outline, and last the late pass
// of the leticker, army and spider objects
void CGame::DrawScene()
{
	if (!GetStatusFlags(1)) {
		return;
	}
	if (mHomeMenu.IsRunning()) {
		return;
	}
	bool menuShown = mMenuActive && _AE34;
	if (menuShown) {
		return;
	}
	XObj* targets[2]       = { NULL, NULL };
	WiiPlayerState* player = mPlayer;
	targets[0]             = player->mLiftPlugTarget;
	if (targets[1] == NULL) {
		targets[1] = player->mLiftTarget;
	}
	gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
	for (s32 i = 0; i < mSceneRefCount; i++) {
		XObj* obj = (XObj*)mSceneRefs[i].mObj;
		if (obj->GetCategory() == 3) {
			obj->DrawObject();
		}
	}
	for (s32 i = 0; i < mSceneRefCount; i++) {
		XObj* obj = (XObj*)mSceneRefs[i].mObj;
		if (obj->GetCategory() != 3) {
			obj->DrawObject();
		}
	}
	DrawSceneQueue(GetCamera());

	gGame.mGs.mCurRenderState->SetColorUpdate(GX_FALSE);
	gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_TRUE);
	gGame.mGs.mCurRenderState->SetDstAlpha(GX_TRUE, 4);
	for (s32 i = 0; i < mSceneRefCount; i++) {
		XObj* obj = (XObj*)mSceneRefs[i].mObj;
		if (obj->GetCategory() != 3 && ((obj->mStatus.mFlags & 0x820040) || obj == targets[1])) {
			obj->DrawObject();
		}
	}
	DrawSceneQueue(GetCamera());

	gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
	for (s32 i = 0; i < mSceneRefCount; i++) {
		XObj* obj = (XObj*)mSceneRefs[i].mObj;
		if (obj->GetCategory() == 9) {
			((XObjLeticker*)obj)->DrawLate();
		} else if (obj->GetCategory() == 16) {
			((XObjArmy*)obj)->DrawLate();
		} else if (obj->GetCategory() == 17) {
			((XObjSpider*)obj)->DrawLate();
		} else if (obj->GetCategory() == 18) {
			((XObjSpiderSmall*)obj)->DrawLate();
		}
	}
	CallStageModule(2, 0, NULL);
	DrawSceneQueue(GetCamera());
}

/**
 * @note Address: 0x800270C8
 * @note Size: 0x4C
 */
// NONMATCHING: retail calls the Wii MSL's out-of-line __fpclassifyf (0x8022A450); the MSL header the
// game shares with the GameCube defines it inline
f32 XObj::GetCollisionHeight()
{
	if (__fpclassifyf(mMaxFloorHeight) == FP_NAN) {
		return 0.0f;
	}
	return mMaxFloorHeight - mMoveTranslate.y;
}

/**
 * @note Address: 0x800270C0
 * @note Size: 0x8
 */
f32 XObj::GetFloorXZExtent()
{
	return mFloorXZExtent;
}

/**
 * @note Address: 0x8002694C
 * @note Size: 0x10
 */
// the reflection pass draws it like any other view
void XObj::DrawReflection(Mtx view)
{
	DrawWithView(view);
}

/**
 * @note Address: 0x800263C0
 * @note Size: 0x78
 */
void CBase::SetScreenLight()
{
	SetLight(GetScreenLight(), GetScreenCamera()->mViewMtx, FALSE);
}

/**
 * @note Address: 0x800263AC
 * @note Size: 0x14
 */
void XObj::DrawObject()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x800263A4
 * @note Size: 0x8
 */
CCamera* CBase::GetScreenCamera()
{
	return (CCamera*)mCameras[1];
}

/**
 * @note Address: 0x80026354
 * @note Size: 0x50
 */
BOOL CBase::vf5C()
{
	return BeginCamera(GetScreenCamera());
}

/**
 * @note Address: 0x8002504C
 * @note Size: 0x4
 */
void CGame::Reset(u32 code)
{
	CBase::Reset(code);
}

/**
 * @note Address: 0x80024CA8
 * @note Size: 0x8
 */
CVec* XObj::GetPos()
{
	return &mMoveTranslate;
}

/**
 * @note Address: 0x80024CA0
 * @note Size: 0x8
 */
void CBase::vf54()
{
	EndGsCamera(&mGs);
}

/**
 * @note Address: 0x80024C94
 * @note Size: 0xC
 */
BOOL CBase::BeginCamera(CCamera* camera)
{
	mGs.mCamera = camera;
	return mGs.BeginCamera();
}

/**
 * @note Address: 0x80024C44
 * @note Size: 0x50
 */
BOOL CBase::BeginSceneCamera()
{
	return BeginCamera(GetCamera());
}

/**
 * @note Address: 0x80024BD4
 * @note Size: 0x4
 */
void XObj::vf16C()
{
}

/**
 * @note Address: 0x80024B4C
 * @note Size: 0x6C
 */
// NONMATCHING: retail builds the sum in a local and copies it out, this one builds it in the result
CVec CVec::operator+(Vec v) const
{
	CVec sum = *this;
	nw4r::math::VEC3Add((nw4r::math::VEC3*)&sum, (nw4r::math::VEC3*)&sum, (nw4r::math::VEC3*)&v);
	return sum;
}

/**
 * @note Address: 0x80024AE8
 * @note Size: 0x64
 */
// NONMATCHING: as operator+
CVec CVec::operator*(f32 s) const
{
	CVec product = *this;
	nw4r::math::VEC3Scale((nw4r::math::VEC3*)&product, (nw4r::math::VEC3*)&product, s);
	return product;
}

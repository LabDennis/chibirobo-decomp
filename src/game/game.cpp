#include "game/cgame.h"
#include "game/cjobjset.h"
#include "game/hsd_jobj.h"
#include "game/title.h"
#include "game/menuex.h"
#include "game/msg.h"
#include "game/cmotioncontrolcamera.h"
#include "game/mic.h"
#include "Dolphin/m2s.h"
#include "game/cgameutil.h"
#include "Dolphin/mic.h"
#include "game/cthread.h"
#include <Dolphin/vi.h>
#include "game/gmath.h"
#include "game/video.h"
#include "game/mm.h"
#include "game/stg.h"
#include "game/xobj.h"
#include "game/stagedata.h"
#include <math.h>
#include "game/xobjmap.h"
#include "game/hsd_dobj.h"
#include "game/xchr.h"
#include "game/texturecopy.h"
#include "game/fbtexeff.h"
#include "game/cshadow.h"
#include "game/clight.h"
#include "game/globals.h"
#include <types.h>
#include "Dolphin/OS/OSReset.h"
#include "game/csound.h"
#include "game/xobjread.h"
#include <hsd/sysdolphin/devcom.h>
#include "game/xobjlight.h"
#include "game/ccameraaction.h"
#include "game/newgamestate.h"
#include "game/arcache.h"
#include "game/ccamera.h"
#include "game/cparticle.h"
#include <Dolphin/ai.h>
#include <Dolphin/OS/OSReset.h>
#include "types.h"
#include "game/cobj.h"
#include "game/cscrn.h"
#include "game/fbeffect.h"
#include "game/logo.h"
#include "game/infoscrn.h"
#include "game/xsound.h"
#include "game/memcard.h"
#include "game/cmovie.h"
#include "Dolphin/OS/OSAlloc.h"
#include "game/cgs.h"

/**
 * @note Address: 0x80007880
 * @note Size: 0x6BC
 */
CGame::~CGame()
{
}

/**
 * @note Address: 0x80007F3C
 * @note Size: 0x114
 */
CBase::~CBase()
{
}

Title::~Title()
{
}

MenuCursor::~MenuCursor()
{
}

Cursor::~Cursor()
{
}

XObj::~XObj()
{
}

CXObjIntp::~CXObjIntp()
{
}

CXObj::~CXObj()
{
}

XObjDemo::~XObjDemo()
{
}

MenuEx::~MenuEx()
{
}

CMsg::~CMsg()
{
}

/**
 * @note Address: 0x8000902C
 * @note Size: 0x1A0
 */
CMotionControlCamera::~CMotionControlCamera()
{
}

/**
 * @note Address: 0x800091CC
 * @note Size: 0x3F4
 */
CCameraActions::~CCameraActions()
{
}

/**
 * @note Address: 0x800095C0
 * @note Size: 0x298
 */
CCameraActionArea::~CCameraActionArea()
{
}

/**
 * @note Address: 0x80009F20
 * @note Size: 0x340
 */
CCameraActionMap::~CCameraActionMap()
{
}

/**
 * @note Address: 0x8000A260
 * @note Size: 0x258
 */
CCameraActionReset::~CCameraActionReset()
{
}

/**
 * @note Address: 0x8000A4B8
 * @note Size: 0x68
 */
// CGame's FrameInterval, the logo and title run at 60, the game at 30
s32 CGame::FrameInterval()
{
	return IsIntroActive() ? 1 : 2;
}

/**
 * @note Address: 0x8000A520
 * @note Size: 0xC
 */
// CGame's GetCamera
CCamera* CGame::GetCamera()
{
	return &mMotionControlCamera.mCamera;
}

/**
 * @note Address: 0x8000A52C
 * @note Size: 0xC
 */
CLight* CGame::GetLight()
{
	return mLightList;
}

/**
 * @note Address: 0x8000A538
 * @note Size: 0x8
 */
u32 CBase::GetRetraceStep()
{
	return mRetraceStep;
}

/**
 * @note Address: 0x8000A540
 * @note Size: 0x8
 */
CCamera* CBase::GetPerspectiveCamera()
{
	return &mCameras[0];
}

/**
 * @note Address: 0x8000A548
 * @note Size: 0x8
 */
CCamera* CBase::GetScreenCamera()
{
	return &mCameras[1];
}

/**
 * @note Address: 0x8000A550
 * @note Size: 0x8
 */
CLight* CBase::GetDefaultLight()
{
	return &mLights[0];
}

/**
 * @note Address: 0x8000A558
 * @note Size: 0x8
 */
CLight* CBase::GetScreenLight()
{
	return &mLights[4];
}

extern "C" {
void* memset(void* dst, int val, u32 size);
}

/**
 * @note Address: 0x8000A560
 * @note Size: 0x118
 */
// pulls the next samples from the mic into the buffer, a faster step when more are waiting,
// and keeps the loudest ones
extern "C" void FillMicrophoneSampleBuffer(s16* buffer)
{
	s32 count;
	s32 step;
	u32 next;
	memset(buffer, 0, 0x280);
	u32* pos = &gMicSamplePhase[M2SGetActiveChannel()];
	s32 left = M2SGetSamplesLeft();
	if (left < 0) {
		*pos = 0;
	} else if (left >= sMicBufferThresholds[0]) {
		if (left < sMicBufferThresholds[1]) {
			step = sMicSamplePhaseSteps[0];
		} else if (left < sMicBufferThresholds[2]) {
			step = sMicSamplePhaseSteps[1];
		} else {
			step = sMicSamplePhaseSteps[2];
		}
		next = *pos;
		next += step * 159;
		count = next >> 16;
		s16* samples;
		M2SGetSamples(&samples, count + 4);
		s16* sample = samples;
		for (s32 i = 0; i < count + 4; i++) {
			s32 value = *sample;
			if (value > gMicMeterPeak) {
				gMicMeterPeak = value;
			}
			if (value > 1000 && value > gMicObjectPeak) {
				gMicObjectPeak = value;
			}
			sample++;
		}
		next += step;
		*pos = next & 0xFFFF;
		M2SAdvanceBuffer(next >> 16);
	}
}

/**
 * @note Address: 0x8000A678
 * @note Size: 0x60
 */
extern "C" void OnMicAudioDMA(void)
{
	gPreviousAudioDMACallback();
	FillMicrophoneSampleBuffer(gMicDMABuffers[gMicDMABufferIndex]);
	DCFlushRange(gMicDMABuffers[gMicDMABufferIndex], 0x280);
	gMicDMABufferIndex ^= 1;
}

extern "C" void OnMicrophoneDetached(s32 index, s32 result);

extern "C" {
// one per slot
MicChannel gMicrophoneChannels[2] = {
	{ MIC_RESULT_NOCARD, 0, 0, NULL },
	{ MIC_RESULT_NOCARD, 0, 0, NULL },
};
s32 sMicSamplePhaseSteps[3] = { 22557, 22559, 22561 };
s32 sMicBufferThresholds[3] = { 80, 160, 240 };
// the slot in use, -1 for none
s32 gActiveMicChannel = -1;
BOOL gMicRecording;
OSTime gMicButtonReleaseTime;
s32 gMicObjectPeak; // peak above 1000 delivered to stage objects
s32 gMicMeterPeak;  // largest positive sample since the level bar was drawn
s16* gMicDMABuffers[2];
u8 gMicDMABufferIndex;
u32 gMicSamplePhase[2];
void (*gPreviousAudioDMACallback)(void);
}

/**
 * @note Address: 0x8000A6D8
 * @note Size: 0x180
 */
// NONMATCHING: the work area isn't read back for the mount
// mounts the mic in slot B and keeps it mounted
extern "C" void UpdateMicrophoneMount(void)
{
	MicChannel* slot = &gMicrophoneChannels[1];
	s32 result;
	switch (slot->mState) {
	case 0:
		if (MICProbeEx(1) == 0) {
			slot->mState = 1;
		}
		break;
	case 1:
		slot->mBuffer = (s16*)OSAllocFromHeap(__OSCurrHeap, MIC_RINGBUFF_SIZE);
		switch (result = MICMountAsync(1, slot->mBuffer, MIC_RINGBUFF_SIZE, OnMicrophoneDetached, NULL)) {
		case 0:
			slot->mState = 2;
			break;
		case -1:
			break;
		default:
			goto failed;
		}
		break;
	case 2:
		switch (result = MICGetResultCode(1)) {
		case 0:
			MICGetDeviceID(1, &slot->mDeviceID);
			slot->mResult = 0;
			slot->mState  = 3;
			break;
		case -1:
			break;
		default:
			goto failed;
		}
		break;
	case 3:
		if (slot->mResult != 0) {
			MICUnmount(1);
			OSFreeToHeap(__OSCurrHeap, slot->mBuffer);
			slot->mBuffer = NULL;
			slot->mState  = 0;
		}
		break;
	}
	return;

failed:
	switch (result) {
	case -2:
		OSFreeToHeap(__OSCurrHeap, slot->mBuffer);
		slot->mBuffer = NULL;
		slot->mState  = 0;
		break;
	case -3:
		OSFreeToHeap(__OSCurrHeap, slot->mBuffer);
		slot->mBuffer = NULL;
		slot->mState  = 0;
		break;
	}
}

/**
 * @note Address: 0x8000A858
 * @note Size: 0x18
 */
extern "C" void OnMicrophoneDetached(s32 index, s32)
{
	gMicrophoneChannels[index].mResult = MIC_RESULT_NOCARD;
}

extern "C" void UpdateMicrophoneMount(void);

/**
 * @note Address: 0x8000A870
 * @note Size: 0x31C
 */
// the mic: opens it in whichever slot has one, records while its button is held and a little
// after, and hands the loudest sample to every object
extern "C" void UpdateStageMicrophone(u32)
{
	if (gGame.mStatus.mFlags & 1) {
		UpdateMicrophoneMount();
		if (gActiveMicChannel < 0) {
			if (gMicrophoneChannels[0].mResult == MIC_RESULT_READY) {
				gActiveMicChannel = 0;
			} else if (gMicrophoneChannels[1].mResult == MIC_RESULT_READY) {
				gActiveMicChannel = 1;
			}
			if (gActiveMicChannel >= 0 && gMicrophoneChannels[gActiveMicChannel].mDeviceID == 0) {
				M2SSetActiveChannel(gActiveMicChannel);
				M2SSetMode(M2S_MODE_PRERECORD | M2S_MODE_KEEP_MIC_RUNNING);
				M2SSetPrerecordSamples(400);
				M2SOpen();
				MICSetGain(gActiveMicChannel, 0);
				MICStart(gActiveMicChannel);
			}
		} else if (gMicrophoneChannels[gActiveMicChannel].mResult != MIC_RESULT_READY) {
			M2SClose();
			gActiveMicChannel = -1;
		}

		if (gActiveMicChannel >= 0 && gMicrophoneChannels[gActiveMicChannel].mDeviceID == 0) {
			u32 button = 0;
			MICGetButton(gActiveMicChannel, &button);
			if (button & MIC_BUTTON_TALK) {
				if (!gMicRecording && gMicButtonReleaseTime == 0) {
					M2SStart();
					gMicRecording = TRUE;
				}
			} else if (gMicRecording && gMicButtonReleaseTime == 0) {
				gMicButtonReleaseTime = OSGetTime();
			}
			if (OSGetTime() - gMicButtonReleaseTime > OSMillisecondsToTicks(100) && gMicButtonReleaseTime != 0 && gMicRecording) {
				M2SStop();
				gMicRecording         = FALSE;
				gMicButtonReleaseTime = 0;
			}
		}

		if (gMicObjectPeak != 0) {
			if ((gGame.mStatus.mFlags & 1) && !(gGame.mStatus.mFlags & 2)) {
				for (s32 i = 0; i < gGame.mDrawLists[0].mCount; i++) {
					if (gGame.mDrawLists[0].mObjects[i]->mStatus.mFlags & 1) {
						gGame.mDrawLists[0].mObjects[i]->OnMicrophonePeak(-1, gMicObjectPeak);
					}
				}
			}
			gMicObjectPeak = 0;
		}
	}
}

/**
 * @note Address: 0x8000AB8C
 * @note Size: 0x4
 */
void XObj::OnMicrophonePeak(s32, s32)
{
}

extern "C" {
void* UpdateMovie(void* arg);
}

#if defined(VERSION_JP)
enum { MovieErrorLine = 3780 };
#elif defined(VERSION_PAL)
enum { MovieErrorLine = 3917 };
#else
enum { MovieErrorLine = 3832 };
#endif

/**
 * @note Address: 0x8000AB90
 * @note Size: 0x28
 */
extern "C" void StopGameMovie(CGame* game)
{
	game->mMovie.mMovie.Stop();
}

/**
 * @note Address: 0x8000ABB8
 * @note Size: 0xAC
 */
// starts a movie and the thread that plays it, bit 0 of the flags plays it with alpha
extern "C" void StartGameMovie(CGame* game, const char* path, BOOL loop, u32 flags)
{
	game->mMovieFlags = flags;
	game->mMovie.mMovie.Start(path, loop, (flags & 1) != 0, TRUE);
	game->mMovieRect[0] = game->mMovieRect[1] = game->mMovieRect[2] = game->mMovieRect[3] = NAN;
	gMovieUpdateThread.Start(UpdateMovie, game, gMovieUpdateThreadStack, sizeof(gMovieUpdateThreadStack));
}

/**
 * @note Address: 0x8000AC64
 * @note Size: 0xD0
 */
// UpdateMovie, the movie thread: steps the movie once a retrace until it's done
extern "C" void* UpdateMovie(void* arg)
{
	s32 result;
	CMovie* movie;
	u32 count = VIGetRetraceCount();
	movie     = &((CGame*)arg)->mMovie.mMovie;
	do {
		while (count == VIGetRetraceCount()) {
			VIWaitForRetrace();
		}
		count = VIGetRetraceCount();
		if (movie->mState != 0) {
			result = movie->Update();
			if (result < 0) {
				movie->Stop();
				OSPanic("game.cpp", MovieErrorLine, "%s%s", FormatToStaticBuffer("error on play movie! - %d\n", result), __FUNCTION__);
			}
		}
	} while (movie->mState != 0);
	return NULL;
}

extern "C" {
int strncmp(const char* a, const char* b, unsigned long n);
}
// the stage name's four shadow copies, one pixel off to each corner
f32 sStageNameShadowX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 sStageNameShadowY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };

/**
 * @note Address: 0x8000AD34
 * @note Size: 0x600
 */
// NONMATCHING: the loop's float registers are assigned in another order
// draws the stage name: a bubble per letter, then the letter with a shadow on four sides
extern "C" void DrawStageNameBubbles(CGame* game)
{
#if defined(VERSION_JP)
	const f32 spacing   = 58.0f;
	const f32 glyphSize = 25.0f;
#else
	const f32 spacing   = 43.2f;
	const f32 glyphSize = 22.0f;
#endif
	if (game->mStageName.mActive && !(game->mStatus.mFlags & 2)) {
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		CTexObj bubble = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[138 - StatusTextureOffset], 0);
		f32 zero       = 0.0f;
		u32 color      = game->mStageName.mColor & 0xFFFFFF00; // rgb only; alpha comes from the scale
		for (s32 i = 0; i < game->mStageName.mCount; i++) {
			f32 scale = game->mStageName.mChars[i].mScale;
			if (scale <= zero) {
				continue;
			}
#if !defined(VERSION_JP)
			if (strncmp(game->mStageName.mChars[i].mText, " ", 1) == 0) {
				continue;
			}
#endif
			CFont font;
			s32 width;
			CTexObj letter = *GetFontGlyphTexture(&font, game->mStageName.mChars[i].mText, &width);
			f32 bob;
			f32 tilt;
			f32 swell;
			f32 pop;
			f32 x;
			f32 rot;
			f32 y;
			bob       = 4.0f * (f32)sin(game->mStageName.mChars[i].mBob);
			tilt      = sin(0.7f * game->mStageName.mChars[i].mTilt);
			swell     = sin(PI * scale);
			pop       = 0.2f * swell + (f32)sin(HALF_PI * scale);
			x         = spacing * 0.5f + (spacing * i + (320.0f - spacing * game->mStageName.mCount / 2.0f));
			u32 alpha = 255.0f * scale;
			rot       = -(DEG2RAD * (10.0f * tilt));
			y         = 150.0f + bob;
			DrawGsTexturedQuad(&gGame.mGs, &bubble, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, x, y,
			                   0.0f, glyphSize * (2.2f * pop), glyphSize * (2.2f * pop), zero, -zero, rot);
			f32 size   = glyphSize * (1.5f * pop);
			f32 shrink = width / 24.0f * size;
			f32 off    = size - shrink;
			for (s32 j = 0; j < 4; j++) {
				DrawGsTexturedQuad(&gGame.mGs, &letter, alpha, alpha, alpha, alpha, off / 2.0f + (x + sStageNameShadowX[j]),
				                   bob + (150.0f + sStageNameShadowY[j]), 0.0f, size, size, zero, -zero, rot);
			}
			DrawGsTexturedQuad(&gGame.mGs, &letter, color | alpha, color | alpha, color | alpha, color | alpha, off / 2.0f + x, y, 0.0f,
			                   size, size, zero, -zero, rot);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
}

extern "C" {
s32 GetDefaultStageIndex(void);
void ClearMainMenuExtraRows(MenuMain* menu);
void ResetMessageState(CMsg* msg);
void CMsg_ResetPos(CMsg* msg);
void UnloadStageDataAndModule(void);
void ReleaseStageObjectList(XOBJS* objs);
void psKillAllParticle(void);
void SetMusicPlaybackPaused(XSound* sound, BOOL pause);
void ClearStageSoundBank(XSound* sound);
void ResetMenuGroup(MenuEx* menu);
void ResetGameHudState(InfoScrn* info);
u8* GetStageDisplayName(s32 index);
int rand(void);
}
GXColor gMessageFontDefaultColor = { 0, 0, 0, 0x80 };

// a lead byte of a two byte shift jis character
static inline bool IsSJISLead(u8 c)
{
	return (c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC);
}

// shuts a wipe off where it is
static inline void ClearWipe(CScrnWipe* wipe)
{
	StartScreenWipe(wipe, 0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

/**
 * @note Address: 0x8000B394
 * @note Size: 0xAA4
 */
// NONMATCHING: the name loop's counter and letter pointer swap registers
// changes to another stage: 10000 is the title, 20000 the stage from the card, below 0 the
// next one. everything that belongs to the old stage is let go of, and the new stage's name
// is set up to pop in when showName is set
extern "C" void ChangeGameStage(CGame* game, s32 stage, BOOL showName)
{
	StartScreenFade(&game->mFade, 0, 0, 0, 255, 0, 0, 0, 255, 0.0f);
	if (stage == 10000) {
		s32 first = gCmdLineArgs.mStage;
		if (first < 0 || (game->mStatus.mFlags & 0x8000000000000)) {
			game->SetStage(GetDefaultStageIndex());
		} else {
			game->SetStage(first);
		}
	} else if (stage == 20000) {
		game->SetStage(GetSaveFileStage(&game->mMemcard.mCard));
		game->mMemcardLoaded = TRUE;
	} else if (stage >= 0) {
		game->SetStage(stage);
	} else {
		game->mPrevStage = game->mStage;
		if (++game->mStage >= game->mRoomCount) {
			game->mStage = 0;
		}
	}

	if (game->mMovie.mMovie.mState != 0) { // cut any playing movie
		game->mMovie.mMovie.Stop();
	}
	ClearMainMenuExtraRows(static_cast<MenuMain*>(game->mMenu.mMenus[0]));
	ResetEventScriptState(&game->mEvt, 1, 1);
	for (s32 i = 0; i < 2; i++) {
		ResetMessageState(&game->mCMsgs[i]);
		CMsg_ResetPos(&game->mCMsgs[i]);
		game->mCMsgs[i].SetColor(gMessageFontDefaultColor);
	}
	for (s32 i = 0; i < 5; i++) { // release the mirrors' joints and textures
		FbMirrorEx* mirror = &gFramebufferMirrors[i];
		if (mirror->IsUsed()) {
			if (mirror->mJoint != NULL) {
				mirror->mJoint   = NULL;
				mirror->mVisible = FALSE;
				if (mirror->mActive && mirror->mTexture.mImage != NULL) {
					mm::Free(mirror->mTexture.mImage, 1);
					mirror->mTexture.mImage = NULL;
				}
			}
			mirror->mOwner = NULL;
		}
	}
	UnloadStageDataAndModule();
	game->mLightList = game->mLights;
	ReleaseStageObjectList(&game->mXObjs);
	gActiveMicChannel = -1; // no mic channel survives the stage change
	game->mParticle.KillAll();
	psKillAllParticle();
	if (game->mStatus.mFlags & 4) { // flag 4 kept the music paused: resume it
		game->mStatus.mFlags &= ~4;
		SetMusicPlaybackPaused(&gGame.mSound, FALSE);
		HSD_PadRumbleActiveAll();
	}
	KeyOffGameSoundTrack(&gGame.mSound, -1);
	ClearStageSoundBank(&game->mSound);
	game->mStatus.mFlags &= ~0x20000000002F;
	game->mStatus.mFlags &= ~0xC00;
	game->mStatus.mFlags &= ~0x2000;
	game->mStatus.mFlags &= ~0x300;
	ClearWipe(&game->mRoboeyes);
	game->mStatus.mFlags |= 0x40000000000000;
	game->mStatus.mFlags |= 0x800000000000;
	InfoScrn_ApplyScoreHudCommand(&game->mInfoScrn, 0, 0);

	if (stage == 10000) {
		game->mStatus.mFlags |= 0x20000000;
		game->mStatus.mFlags |= 0x1000000000000;
		game->mStatus.mFlags &= ~0x80;
		ResetTitleScreenState(&game->mTitle);
		ResetMenuGroup(&game->mMenu);
		ResetGameHudState(&game->mInfoScrn);
		game->mSubwin.Reset();
		StartScreenFade(&game->mFade, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f);
		ClearWipe(&game->mWipeCircle);
		ClearWipe(&game->mWipeTex);
		ClearWipe(&game->mWipeCustom);
		ClearWipe(&game->mWipeTile0);
		StartScreenWipeColorTransition(&game->mWipeTile1, 0, 0, 0, 0, 0, 0, 0, 320.0f, 224.0f, 640.0f, 448.0f, 640.0f, 448.0f, 0.0f, 0,
		                               TRUE);
		StartScreenShake(&game->mQuake, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mBlur.mAlpha  = 0.0f;
		game->mBlur.mTarget = 0.0f;
		game->mLoadStep     = 19;
	} else if (stage == 20000) {
		game->mSubwin.Reset();
		StartScreenFade(&game->mFade, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f);
		ClearWipe(&game->mWipeCircle);
		ClearWipe(&game->mWipeTex);
		ClearWipe(&game->mWipeCustom);
		ClearWipe(&game->mWipeTile0);
		StartScreenWipeColorTransition(&game->mWipeTile1, 0, 0, 0, 0, 0, 0, 0, 320.0f, 224.0f, 640.0f, 448.0f, 640.0f, 448.0f, 0.0f, 0,
		                               TRUE);
		StartScreenShake(&game->mQuake, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mBlur.mAlpha  = 0.0f;
		game->mBlur.mTarget = 0.0f;
		game->mLoadStep     = 20;
	} else {
		game->mLoadStep = 21;
	}

	if (showName) {
		game->mStageName.mActive = TRUE;
		game->mStageName.mCount  = 0;
		s32 cur                  = game->mStage;
		if (cur < GetStageNum()) {
			const char* name = reinterpret_cast<const char*>(GetStageDisplayName(game->mStage));
			if (name != NULL) {
				game->mStageName.mColor = 0x80FFFF00; // pale cyan; the alpha byte is filled from each letter's scale
				for (s32 i = 0; i < 20 && *name != '\0'; i++, name++) {
					game->mStageName.mChars[i].mText  = name;
					game->mStageName.mChars[i].mScale = -0.2f * i; // later letters start hidden
					game->mStageName.mChars[i].mWait  = 2.0f * FRAME_RATE;
					game->mStageName.mChars[i].mBob   = DEG2RAD * (rand() % 360); // a random phase
					game->mStageName.mChars[i].mTilt  = DEG2RAD * (rand() % 360);
					game->mStageName.mCount++;
#if defined(VERSION_JP)
					if (IsSJISLead(*name)) {
						name++;
					}
#else
					if (gGame.mLanguage == 6 && IsSJISLead(*name)) {
						name++;
					}
#endif
				}
				for (s32 i = game->mStageName.mCount; i < 20; i++) { // the rest of the slots stay blank
					game->mStageName.mChars[i].mText  = NULL;
					game->mStageName.mChars[i].mScale = 0.0f;
					game->mStageName.mChars[i].mWait  = 0.0f;
				}
			}
		}
	}
}

/**
 * @note Address: 0x8000BE38
 * @note Size: 0x560
 */
// draws the movie, centred at its own size unless _FF460 says otherwise. with alpha it's drawn
// three times over
extern "C" void DrawGameMovie(CGame* game)
{
	if (game->mMovie.mMovie.mState != 0) {
		f32 w   = fpclassify(game->mMovieRect[2]) == FP_NAN ? (f32)game->mMovie.mMovie.mVideoInfo.xSize : game->mMovieRect[2];
		f32 h   = fpclassify(game->mMovieRect[3]) == FP_NAN ? (f32)game->mMovie.mMovie.mVideoInfo.ySize : game->mMovieRect[3];
		f32 x   = fpclassify(game->mMovieRect[0]) == FP_NAN ? (640.0f - w) / 2 : game->mMovieRect[0];
		f32 y   = fpclassify(game->mMovieRect[1]) == FP_NAN ? (448.0f - h) / 2 : game->mMovieRect[1];
		CGs* gs = &gGame.mGs;
		SetGsColorUpdate(gs->mCurRenderMode, 1);
		SetGsAlphaUpdate(gs->mCurRenderMode, 0);
		SetGsDepthMode(gs->mCurRenderMode, 1, 7, 0);
		if (game->mMovie.mMovie.mAlpha) {
			SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
			SetGsAlphaCompare(gs->mCurRenderMode, 4, 0x20, 0, 3, 255);
			SetupAll(FALSE);
			if (game->mMovie.mMovie.BeginDraw() >= 0) {
				for (s32 i = 0; i < 3; i++) {
					game->mMovie.mMovie.Draw(x, y, w, h);
				}
				game->mMovie.mMovie.EndDraw();
			}
		} else {
			SetGsBlendMode(gs->mCurRenderMode, 0, 4, 5, 15);
			SetGsAlphaCompare(gs->mCurRenderMode, 7, 0, 0, 7, 255);
			SetupAll(FALSE);
			game->mMovie.mMovie.DrawFrame(x, y, w, h);
		}
		SetGsAlphaCompare(gs->mCurRenderMode, 4, 0, 0, 3, 255);
		InvalidateAll();
	}
}

/**
 * @note Address: 0x8000C398
 * @note Size: 0xB8
 */
// CGame's DrawHud, every live object's DrawScreenOverlay, then the stage's function 4
void CGame::DrawHud()
{
	if (mStatus.mFlags & 1ULL) {
		for (s32 i = 0; i < mXObjs.mObjectCount; ++i) {
			XObj* object = mXObjs.mObjects[i];
			if (object->mStatus.mFlags & 1ULL)
				object->DrawScreenOverlay();
		}
		CallStageModule(4, 0, NULL);
	}
}

extern "C" void FindHighestJointBelowHeightLimit(HSD_JObj* jobj, void* context, s32);

// an object that is drawn into the shadow, or onto it with the mask
static inline bool IsShadowObj(XObj* obj, u64 mask)
{
	const u64 flags = obj->mStatus.mFlags;
	return (flags & 1) && (flags & 0x10000) && (flags & mask);
}

/**
 * @note Address: 0x8000C450
 * @note Size: 0x6FC
 */
// renders the shadow of the first light that casts one, from above the main camera's
// interest. the player's shadow object gets the height of its highest joint under the player
extern "C" void RenderSceneShadow(CGame* game)
{
	XObjChr* player;
	XObj* target;
	s32 i;
	XObj* obj;
	if (game->mStatus.mFlags & 1) {
		player        = gGame.GetPlayer();
		target        = player->mFloorContacts[3].object;
		f32 limits[2] = { 1.0f + player->mMoveTranslate.y, NAN }; // a unit over the player; nan = none found
		for (i = 0; i < game->mDrawLists[1].mCount; i++) {
			obj = game->mDrawLists[1].mObjects[i];
			if (obj->mStatus.mFlags & 2) {
				if (obj == target) {
					limits[1] = NAN;
					if (obj->mJObj != NULL) {
						HSD_JObjWalkTree(obj->mJObj, FindHighestJointBelowHeightLimit, limits);
					}
				} else if (obj->mJObj != NULL) { // every other object loses the mark
					HSD_JObjWalkTree(obj->mJObj, JObjClearMObjFlags, reinterpret_cast<void*>(0x04000000));
				}
			}
		}

		for (CLight* light = game->mLightList; light != NULL; light = (CLight*)light->next) {
			CShadow* shadow = &light->mShadow;
			if (shadow->IsActive()) {
				HSD_CObj* cobj = shadow->mShadow->camera;
				Vec interest   = game->GetCamera()->mInterest;
				Vec eye;
				eye.x = interest.x;
				eye.z = interest.z;
				eye.y = 200.0f + interest.y; // straight over the interest
				HSD_CObjSetInterest(cobj, &interest);
				HSD_CObjSetEyePosition(cobj, &eye);
				HSD_ShadowDeleteObject(shadow->mShadow, NULL);
				shadow->InitRect();
				for (s32 i = 0; i < game->mDrawLists[1].mCount; i++) {
					XObj* obj = game->mDrawLists[1].mObjects[i];
					if (IsShadowObj(obj, 4)) { // flag 4 casts into the shadow map
						obj->RegisterShadowObjects(shadow);
					}
					if ((GetStageInfo()->mFlags & 1) && obj->GetCategory() == 2) {
						obj->ExpandShadowBounds(shadow);
					}
				}
				Vec center = game->GetCamera()->mInterest;
				shadow->AddRect(&center, 100.0f, -100.0f, -100.0f, 100.0f);
				game->mGs.mCamera = shadow->StartRender();
				if (BeginGsCamera(game)) {
					SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
					SetGsChannelEnabled(&gGame.mGs, 0, 0);
					SetGsCullMode(gGame.mGs.mCurRenderMode, 2);
					SetupAll(TRUE);
					for (s32 i = 0; i < game->mDrawLists[1].mCount; i++) {
						XObj* obj = game->mDrawLists[1].mObjects[i];
						if (IsShadowObj(obj, 8)) { // flag 8 gets the blob shadow
							f32 alpha;
							if (GetStageInfo()->mFlags & 1) {
								alpha = 1.0f;
							} else { // else it fades over a hundred units

								CCamera* camera = game->GetCamera();
								f32 dz          = camera->mInterest.z - obj->mMoveTranslate.z;
								f32 dx          = camera->mInterest.x - obj->mMoveTranslate.x;
								alpha           = (100.0f - sqrtf(dx * dx + dz * dz)) / 100.0f;
								if (alpha < 0.0f) {
									alpha = 0.0f;
								}
								if (alpha > 0.8f) {
									alpha = 0.8f;
								}
							}
							obj->DrawGroundShadowAt(&obj->mMoveTranslate, sin(PI * alpha * 0.5f)); // a half-sine falloff
						}
					}
					EndGsCamera(&game->mGs);
				}
				shadow->EndRender();
				break;
			}
		}
		InvalidateAll();
	}
}

/**
 * @note Address: 0x8000CB4C
 * @note Size: 0x74
 */
void XObj::ExpandShadowBounds(CShadow* shadow)
{
	f32 size = GetShadowRadius();
	Vec pos  = mMoveTranslate;
	shadow->AddRect(&pos, size, -size, -size, size);
}

/**
 * @note Address: 0x8000CBC0
 * @note Size: 0x8
 */
f32 XObj::GetShadowRadius()
{
	return 40.0f;
}

/**
 * @note Address: 0x8000CBC8
 * @note Size: 0x2C
 */
void XObj::RegisterShadowObjects(CShadow* shadow)
{
	HSD_ShadowAddObject(shadow->mShadow, (HSD_Obj*)mJObj);
}

/**
 * @note Address: 0x8000CBF4
 * @note Size: 0x1A8
 */
// a joint walk: finds the highest joint at or under the limit in limits[0] and keeps it in
// limits[1], nan to start, and marks the materials of those joints
extern "C" void FindHighestJointBelowHeightLimit(HSD_JObj* jobj, void* context, s32)
{
	f32* limits = static_cast<f32*>(context);
	Vec pos;
	HSD_MtxGetTranslate(HSD_JObjGetMtxPtr(jobj), &pos);
	BOOL under = pos.y <= limits[0];
	if (under) {
		if (fpclassify(limits[1]) == FP_NAN) {
			limits[1] = pos.y;
		} else if (limits[1] < pos.y) {
			limits[1] = pos.y;
		}
	}
	for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
		HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
		if (mobj != NULL) {
			if (under) {
				HSD_MObjSetFlags(mobj, 0x4000000);
			} else {
				HSD_MObjClearFlags(mobj, 0x4000000);
			}
		}
	}
}

/**
 * @note Address: 0x8000CD9C
 * @note Size: 0x4
 */
void XObj::DrawObjectEffects()
{
}

/**
 * @note Address: 0x8000CDA0
 * @note Size: 0x28C
 */
// CGame::DrawScene, draws the objects in _C074 from the main camera, kind 3 ones first
void CGame::DrawScene()
{
	if (mStatus.mFlags & 1) {
		s32 i;
		mGs.mCamera = GetCamera();
		if (BeginGsCamera(this)) {
			for (i = 0; i < mDrawLists[1].mCount; i++) {
				if (mDrawLists[1].mObjects[i]->GetCategory() == 3) {
					mDrawLists[1].mObjects[i]->DrawObject();
				}
			}
			EndGsCamera(&mGs);
		}
		mGs.mCamera = GetCamera();
		if (BeginGsCamera(this)) {
			for (i = 0; i < mDrawLists[1].mCount; i++) {
				if (mDrawLists[1].mObjects[i]->GetCategory() != 3) {
					mDrawLists[1].mObjects[i]->DrawObject();
				}
			}
			CallStageModule(2, 0, NULL);
			EndGsCamera(&mGs);
		}
		InvalidateAll();
	}
}

/**
 * @note Address: 0x8000D02C
 * @note Size: 0x30
 */
void XObj::DrawObject()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x8000D05C
 * @note Size: 0x2C
 */
CCamera* CBase::GetCamera()
{
	return GetPerspectiveCamera();
}

/**
 * @note Address: 0x8000D088
 * @note Size: 0x88
 */
f32 XObj::GetCollisionHeight()
{
	if (fpclassify(mMaxFloorHeight) == FP_NAN) {
		return 0.0f;
	}
	return mMaxFloorHeight - mMoveTranslate.y;
}

/**
 * @note Address: 0x8000D110
 * @note Size: 0x8
 */
f32 XObj::GetFloorXZExtent()
{
	return mFloorXZExtent;
}

/**
 * @note Address: 0x8000D118
 * @note Size: 0x5EC
 */
// clears the screen's alpha from the second camera, draws the 0x40 objects and the two the
// player is busy with into it, then sets the 0x40 objects back up for the main pass
extern "C" void DrawObjectAlphaMask(CGame* game)
{
	XObj* obj;
	s32 i;
	XObjChr* player;
	if ((game->mStatus.mFlags & 1) && (game->mStatus.mFlags & 0x4000000000)) {
		game->mGs.mCamera = game->GetScreenCamera();
		if (BeginGsCamera(game)) {
			InvalidateAll();
			SetGsChannelEnabled(&gGame.mGs, 4, 0);
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 0, 4, 5, 15);
			SetGsColorUpdate(gGame.mGs.mCurRenderMode, 0);
			SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 1);
			SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
			SetGsDestinationAlpha(gGame.mGs.mCurRenderMode, 0, 0);
			SetupAll(TRUE);
			Vec quad[4]
			    = { { 0.0f, 0.0f, 0.0f }, { 640.0f, 0.0f, 0.0f }, { 640.0f, 448.0f, 0.0f }, { 0.0f, 448.0f, 0.0f } }; // a full-screen clear
			DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, 4, quad[1].x, quad[1].y, quad[1].z, 4, quad[2].x, quad[2].y,
			                 quad[2].z, 4, quad[3].x, quad[3].y, quad[3].z, 4);
			SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
			InvalidateAll();
			EndGsCamera(&game->mGs);
		}

		game->mGs.mCamera = game->GetCamera();
		if (BeginGsCamera(game)) {
			player           = static_cast<XObjChr*>(game->mXObjs.mPools[2].First());
			XObj* targets[2] = { player->mLift.mPlugTarget, player->mLift.mTarget };
			if (targets[1] == NULL) {
				targets[1] = player->mPush.mTarget;
			}
			if (targets[1] == NULL) {
				targets[1] = player->mLadder.mTarget;
			}
			if (targets[1] == NULL) { // or the rope's
				targets[1] = player->mRope.mTarget;
			}
			for (i = 0; i < game->mDrawLists[1].mCount; i++) {
				obj = game->mDrawLists[1].mObjects[i];
				if (obj->mStatus.mFlags & 0x40) {
					obj->SetDestinationAlpha(1, 0);
					obj->DrawObject();
				} else if ((targets[0] == obj || targets[1] == obj)
				           && !(obj->mStatus.mFlags & 0x800000)) { // flag 0x800000 stays out of the mask
					obj->DrawObject();
				}
			}
			EndGsCamera(&game->mGs);
		}

		for (i = 0; i < game->mDrawLists[1].mCount; i++) {
			obj = game->mDrawLists[1].mObjects[i];
			if (obj->mStatus.mFlags & 0x40) {
				if (obj->mStatus.mFlags & 0x800000) {
					obj->SetDestinationAlpha(1, 8);
				} else {
					obj->SetDestinationAlpha(1, 4);
				}
			}
		}
	}
}

// copies what was drawn into the effect's texture
static inline void CopyToTexture(FbMirrorEx* effect)
{
	SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
	SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 1);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 1);
	SetupAll(TRUE);
	if (effect->mTexture.mImage != NULL) {
		u16 width, height;
		if (effect->mTexture.mMipmap) {
			width  = effect->mTexture.mWidth << 1;
			height = effect->mTexture.mHeight << 1;
		} else {
			width  = effect->mTexture.mWidth;
			height = effect->mTexture.mHeight;
		}
		GXSetTexCopySrc(0, 0, width, height);
		GXSetTexCopyDst(effect->mTexture.mWidth, effect->mTexture.mHeight, static_cast<GXTexFmt>(effect->mTexture.mFormat),
		                effect->mTexture.mMipmap);
		GXCopyTex(effect->mTexture.mImage, GX_TRUE);
		GXPixModeSync();
		InvalidateAll();
	}
}

/**
 * @note Address: 0x8000D704
 * @note Size: 0x34C
 */
// draws what each mirror shows into its texture
extern "C" void RenderMirrorTextures(CGame* game)
{
	MtxPtr view;
	FbMirrorEx* mirror;
	s32 i;
	s32 j;
	XObj* obj;
	if ((game->mStatus.mFlags & 1) && (game->mStatus.mFlags & 0x100000000000)) {
		for (i = 0; i < 5; i++) {
			mirror = &gFramebufferMirrors[i];
			if (mirror->mJoint != NULL && mirror->mVisible && !IsMirrorOff(mirror)) { // each live mirror
				game->mGs.mCamera = &mirror->mCamera;
				if (BeginGsCamera(game)) {
					view = mirror->mView;
					for (j = 0; j < game->mDrawLists[2].mCount; j++) {
						obj = game->mDrawLists[2].mObjects[j];
						if (!(obj->mStatus.mFlags & 0x200000) && (obj->mStatus.mFlags & 0x200000000000)) { // only flagged objects reflect
							obj->DrawReflection(view);
						}
					}
					EndGsCamera(&game->mGs);
					if (mirror->mVisible) {
						CopyToTexture(mirror);
					}
				}
			}
		}
	}
}

/**
 * @note Address: 0x8000DA50
 * @note Size: 0x2C
 */
// XObj's DrawReflection, the reflection pass draws it like any other view
void XObj::DrawReflection(Mtx view)
{
	DrawWithView(view);
}

extern "C" {
void PSMTXReflect(Mtx m, const Vec* p, const Vec* n);
void PSMTXTranspose(const Mtx src, Mtx xPose);
}

/**
 * @note Address: 0x8000DA7C
 * @note Size: 0x4FC
 */
// NONMATCHING: the local initializers sit at their offset in the file's rodata pool, and the
// first object loop's registers swap
// draws the 0x20000000 objects mirrored in the floor with the lights flipped under it, then
// covers the floor with a quad
extern "C" void RenderFloorReflection(CGame* game)
{
	if ((game->mStatus.mFlags & 1) && (game->mStatus.mFlags & 0x2000000000)) {
		game->mGs.mCamera = game->GetCamera();
		if (BeginGsCamera(game)) {
			for (CLight* light = game->mLightList; light != NULL; light = (CLight*)light->next) {
				light->SetPosition(light->mPosition.x, -light->mPosition.y, light->mPosition.z);
			}
			game->mLightList->SetCurrent(game->GetCamera());

			Vec rot   = { 0.0f, 0.0f, 0.0f };
			Vec trans = { 0.0f, 0.0f, 0.0f };
			Mtx mtx;
			Mtx rotX;
			Mtx rotY;
			Mtx rotMtx;
			Mtx transMtx;
			PSMTXRotRad(rotX, 'X', DEG2RAD * rot.x);
			PSMTXRotRad(rotY, 'Y', DEG2RAD * rot.y);
			PSMTXConcat(rotY, rotX, rotMtx);
			PSMTXTrans(transMtx, trans.x, trans.y, trans.z);
			PSMTXConcat(transMtx, rotMtx, mtx);

			Vec point  = { 0.0f, 0.0f, 0.0f };
			Vec normal = { 0.0f, 1.0f, 0.0f };
			PSMTXMultVec(mtx, &point, &point);
			PSMTXInverse(mtx, mtx);
			PSMTXTranspose(mtx, mtx);
			PSMTXMultVec(mtx, &normal, &normal);
			PSVECNormalize(&normal, &normal);
			Mtx reflect;
			Mtx view;
			PSMTXReflect(reflect, &point, &normal);
			PSMTXConcat(game->GetCamera()->mViewMtx, reflect, view);
			for (s32 i = 0; i < game->mDrawLists[1].mCount; i++) {
				XObj* obj = game->mDrawLists[1].mObjects[i];
				if (!(obj->mStatus.mFlags & 0x200000) && (obj->mStatus.mFlags & 0x20000000)) {
					obj->SetCullMode(1);
					obj->DrawReflection(view);
				}
			}

			for (CLight* light = game->mLightList; light != NULL; light = (CLight*)light->next) {
				light->SetPosition(light->mPosition.x, -light->mPosition.y, light->mPosition.z);
			}
			game->mLightList->SetCurrent(game->GetCamera());
			EndGsCamera(&game->mGs);
			for (s32 i = 0; i < game->mDrawLists[1].mCount; i++) {
				XObj* obj = game->mDrawLists[1].mObjects[i];
				if (!(obj->mStatus.mFlags & 0x200000) && (obj->mStatus.mFlags & 0x20000000)) {
					obj->SetCullMode(2);
				}
			}
		}

		if (game->mStatus.mFlags & 0x20000002000000) {
			game->mGs.mCamera = game->GetCamera();
			if (BeginGsCamera(game)) {
				static Vec floor[4] = {
					{ -1000.0f, -0.1f, -1000.0f },
					{ 1000.0f, -0.1f, -1000.0f },
					{ 1000.0f, -0.1f, 1000.0f },
					{ -1000.0f, -0.1f, 1000.0f },
				};
				SetupAll(TRUE);
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
				SetGsColorUpdate(gGame.mGs.mCurRenderMode, 0);
				DrawGsVertexQuad(&gGame.mGs, floor[0].x, floor[0].y, floor[0].z, 0, floor[1].x, floor[1].y, floor[1].z, 0, floor[2].x,
				                 floor[2].y, floor[2].z, 0, floor[3].x, floor[3].y, floor[3].z, 0);
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
				SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
				EndGsCamera(&game->mGs);
			}
		}
	}
}

/**
 * @note Address: 0x8000DF78
 * @note Size: 0x30
 */
void CLight::SetPosition(f32 x, f32 y, f32 z)
{
	mPosition.x = x;
	mPosition.y = y;
	mPosition.z = z;
	HSD_LObjSetPosition(this, &mPosition);
}

extern "C" {
#include <dolphin/gx/gx.h>
#include <sysdolphin/initialize.h>
void RenderSceneShadow(CGame*);
void RenderMirrorTextures(CGame*);
void RenderFloorReflection(CGame*);
void DrawObjectAlphaMask(CGame*);
void DrawStageNameBubbles(CGame*);
void DrawTitleScreen(Title*);
void DrawGameMovie(CGame*);
void DrawPlayerEquipmentRings(XObjChrMenu*);
void DrawOXMenu(OXMenu*);
void DrawScanModePrompt();
void CMsg_Draw(CMsg*);
void CEvt_Draw();
void DrawObjectLoadingProgress(InfoScrn*);
void DrawHudWithTransitions(InfoScrn*);
void DrawHudPauseLabel(InfoScrn*);
void DrawOpenMenus(MenuEx*);
void ClearActiveObjectFloors(XObj*);
}

static inline void DrawFrameQuad(const Vec (&v)[4], u32 color)
{
	DrawGsVertexQuad(&gGame.mGs, v[0].x, v[0].y, v[0].z, color, v[1].x, v[1].y, v[1].z, color, v[2].x, v[2].y, v[2].z, color, v[3].x,
	                 v[3].y, v[3].z, color);
}

static inline void DrawSubwindowObjects(const XOBJS& objects, CCamera* camera)
{
	for (s32 i = 0; i < objects.mObjectCount; ++i) {
		XObj* object = objects.mObjects[i];
		BOOL draw    = FALSE;
		if ((object->mStatus.mFlags & 1) && (object->mStatus.mFlags & 0x10000)) {
			if (object->mStatus.mFlags & 0x02000000) // flag 0x02000000 draws sight-unseen
				draw = TRUE;
			else {
				f32 radius = object->GetFloorXZExtent() * .5f;
				f32 height = object->GetCollisionHeight();
				if (object->ProjectVisibleBounds(NULL, NULL, camera, radius, height))
					draw = TRUE;
			}
		}
		if (draw)
			object->DrawObject();
	}
}

/**
 * @note Address: 0x8000DFA8
 * @note Size: 0x194C (US, JP), 0x196C (PAL)
 */
void CGame::Draw()
{
	CVec interest = GetCamera()->mInterest;
	if (mQuake.mShake[0].x != 0 || mQuake.mShake[0].y != 0 || mQuake.mShake[0].z != 0) { // the shake nudges the interest
		GetCamera()->SetInterest(interest + mQuake.mShake[0] * (f32)mQuake.mSign);
		GetCamera()->CalcEye();
	}
	mGs.mCamera = GetCamera();
	if (BeginGsCamera(this)) {
		BuildSceneDrawLists(this);
		EndGsCamera(&mGs);
	}
	RenderSceneShadow(this);
	HSD_StartRender(HSD_RP_SCREEN);
	gGame.mGs.mLight = mLightList;
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000000000ULL) && mSubwin.mAmount > 0) { // the subwindow's own pass
		CCamera* camera = &mSubwin.mCamera;
		mGs.mCamera     = camera;
		if (BeginGsCamera(this)) {
#if defined(VERSION_PAL)
			f32 scale = gGame.mGs.mScreen.mHeightScale;
			GXSetScissor(0, 0, 320, static_cast<u32>(224.0f * scale));
#else
			GXSetScissor(0, 0, 320, 224);
#endif
			DrawSubwindowObjects(mXObjs, camera);
			EndGsCamera(&mGs);
		}
		mSubwin.Copy();
	}
	RenderMirrorTextures(this);
	RenderFloorReflection(this);
	if ((mStatus.mFlags & 0x4000000000ULL) && !(GetPlayer()->mStatus.mFlags & 0x200000))
		DrawObjectAlphaMask(this);
	DrawScene();
	mGs.mCamera = GetScreenCamera();
	if (BeginGsCamera(this)) {
		if (mStatus.mFlags & 1) {
			if (mStatus.mFlags & 0x4000000000ULL) { // the player's flag picks the outline
				if (GetPlayer()->mStatus.mFlags & 0x200000)
					mOutline.Draw(9);
				else
					mOutline.Draw(17);
			}
			if (mStatus.mFlags & 0x10000)
				mOutline.Draw(4);
		}
#if defined(VERSION_PAL)
		mBlur.Draw();
#else
		mBlur.Draw(TRUE);
#endif
		InvalidateAll();
		EndGsCamera(&mGs);
	}
	mGs.mCamera = GetCamera();
	if (BeginGsCamera(this)) {
		SetupAll(TRUE);
		if (mStatus.mFlags & 1) {
			for (s32 i = 0; i < mDrawLists[2].mCount; ++i) {
				XObj* object = mDrawLists[2].mObjects[i];
				if ((object->mStatus.mFlags & 1)
				    && !(mColor0.mColor.mColor.a > 0 // the overlay suppresses flagged effects
				         && ((object->mStatus.mFlags & 0x200)
				             || ((object->mStatus.mFlags & 0x0400000000000000ULL) && (object->mStatus.mFlags & 0x400000000000ULL)))))
					object->DrawObjectEffects();
			}
			CallStageModule(3, 0, NULL);
		}
		mParticle.Draw(1);
		SetupAll(TRUE);
		EndGsCamera(&mGs);
	}
	if (mColor0.mColor.mColor.a > 0 && (mStatus.mFlags & 1)) { // under it the flagged objects draw again
		mOutline.Draw(0x22);
		mGs.mCamera = GetCamera();
		if (BeginGsCamera(this)) {
			InvalidateAll();
			for (s32 i = 0; i < mDrawLists[1].mCount; ++i) {
				XObj* object = mDrawLists[1].mObjects[i];
				if (object->mStatus.mFlags & 0x200)
					object->DrawObject();
				else if ((object->mStatus.mFlags & 0x0400000000000000ULL) && (object->mStatus.mFlags & 0x400000000000ULL))
					object->DrawObject();
			}
			EndGsCamera(&mGs);
		}
	}
	mGs.mCamera = GetCamera();
	if (BeginGsCamera(this)) {
		SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetupAll(TRUE);
		if ((mStatus.mFlags & 1) && mColor0.mColor.mColor.a > 0) {
			for (s32 i = 0; i < mDrawLists[2].mCount; ++i) {
				XObj* object = mDrawLists[2].mObjects[i];
				if (!(object->mStatus.mFlags & 1))
					continue;
				if (object->mStatus.mFlags & 0x200)
					object->DrawObjectEffects();
				else if ((object->mStatus.mFlags & 0x0400000000000000ULL) && (object->mStatus.mFlags & 0x400000000000ULL))
					object->DrawObjectEffects();
			}
		}
		mParticle.Draw(2);
		EndGsCamera(&mGs);
	}
	gGame.mGs.mLight = GetScreenLight();
	DrawTitleScreen(&mTitle);
	mGs.mCamera = GetScreenCamera();
	if (BeginGsCamera(this)) {
		InvalidateAll();
		SetupAll(TRUE);
		DrawHud();
		Vec corners[4] = { (Vec) { 0, 0, -1 }, (Vec) { 640, 0, -1 }, (Vec) { 640, 448, -1 }, (Vec) { 0, 448, -1 } };
		SetGsColorUpdate(gGame.mGs.mCurRenderMode, 0);
		SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 0);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
		DrawFrameQuad(corners, 0); // an invisible quad writes the depth plane
		SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
		SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 0);
		if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000000000ULL) && mSubwin.mAmount > 0 && !(mSubwin.mFlags.mFlags & 4))
			mSubwin.Draw();
		mRoboeyes.Draw(&gGame.mGs, 0);
		DrawHudWithTransitions(&mInfoScrn);
		if (mStatus.mFlags & 1)
			mMotionControlCamera.Draw();
		if (mMovieFlags & 2) // flag 2 draws the movie under the wipes
			DrawGameMovie(this);
		DrawScreenColorOverlay(&mFade, &gGame.mGs);
		mWipeCircle.Draw(&gGame.mGs, 0);
		mWipeTex.Draw(&gGame.mGs, 1);
		mWipeCustom.Draw(&gGame.mGs, 1);
		mWipeTile0.Draw(&gGame.mGs, 1);
		mWipeTile1.Draw(&gGame.mGs, 1);
		if (!(mMovieFlags & 2))
			DrawGameMovie(this);
		DrawStageNameBubbles(this);
		if (mStatus.mFlags & 0x400) // flag 0x400: the equipment rings
			DrawPlayerEquipmentRings(&GetPlayer()->mMenu);
		if (mStatus.mFlags & 0x800) // flag 0x800: the ox menu
			DrawOXMenu(&GetPlayer()->mOXMenu);
		if (!gGame.mMotionControlCamera.IsAction(4)) {
			CEvt_Draw();
			for (s32 i = 0; i < 2; ++i)
				CMsg_Draw(&mCMsgs[i]);
		}
		DrawScanModePrompt();
		EndGsCamera(&mGs);
	}
	if (mColor1.mColor.mColor.a > 0) { // a blur copy, then texture 121 full-screen at up to 48 alpha scaled by the overlay's alpha

#if defined(VERSION_PAL)
		mBlur.Copy();
#else
		mBlur.Copy(TRUE);
#endif
		mOutline.Draw(0x42);
		mBlur.DrawTex();
		mOutline.Draw(0x80);
		InvalidateAll();
		mGs.mCamera = GetScreenCamera();
		if (BeginGsCamera(this)) {
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			CTexObj tex = mTpl.mTex.Init(&((TPLPalette*)mTpl.mBuffer)->descriptorArray[121 - StatusTextureOffset], 0);
			u16 width = GXGetTexObjWidth(&tex), height = GXGetTexObjHeight(&tex);
			PSMTXScale(tex.mMtx[1], 640 / width, 448 / height, 1);
			tex.mMtxDirty = TRUE;
			u32 c         = 0xffffff00 | (u32)(48.f * (mColor1.mColor.mColor.a / 255.f));
			DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, 320, 224, 0, 640, 448, 0, 0, 0);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
			EndGsCamera(&mGs);
		}
	}
	mGs.mCamera = GetScreenCamera();
	if (BeginGsCamera(this)) {
		SetupAll(TRUE);
		DrawObjectLoadingProgress(&mInfoScrn);
		DrawHudPauseLabel(&mInfoScrn);
		DrawCurrentLogo(&mLogo);
		DrawOpenMenus(&mMenu);
		if (gActiveMicChannel >= 0 && (gGame.mStatus.mFlags & 0x80000000) && gMicMeterPeak > 0) {
			gMicMeterPeak = (s32)(gMicMeterPeak * .0608f); // the mic meter's bar
			if (gMicMeterPeak > 608)
				gMicMeterPeak = 608;
			SetGsChannelEnabled(&gGame.mGs, 0, 0);
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
			SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
			Vec v[4] = { { 16, 414, 0 }, { 0, 414, 0 }, { 0, 419, 0 }, { 16, 419, 0 } };
			v[1].x   = 16 + gMicMeterPeak;
			v[2].x   = 16 + gMicMeterPeak;
			DrawFrameQuad(v, 0xff000080);
			gMicMeterPeak = 0;
		}
		if (IsDiscError()) {
#if defined(VERSION_JP)
			const int first = 141;
#else
			const int first = 84;
#endif
			s32 errors[][2] = {
				{ 5, first }, { 4, first + 1 }, { 6, first + 2 }, { 11, first + 3 }, { 1, first + 4 }, { 0, -1 }
			}; // dvd status -> text id
			const char* text = NULL;
			for (s32 i = 0; errors[i][1] >= 0; ++i)
				if (errors[i][0] == mDvdStatus)
					text = GetLocalizedInterfaceText(errors[i][1]);
			if (text) {
				f32 w = MeasureGameTextWidth(&gGame.mGs, text, TRUE, 24), h = MeasureGameTextHeight(&gGame.mGs, text, 24);
				f32 x = 320 - .5f * w, y = 224 - .5f * h;
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
				Vec v[4] = {
					{ x - 24, y - 24, 0 }, { 24 + (x + w), y - 24, 0 }, { 24 + (x + w), 24 + (y + h), 0 }, { x - 24, 24 + (y + h), 0 }
				};
				DrawFrameQuad(v, 200);
				DrawGsProportionalTextFormat(&gGame.mGs, 0xffffffff, text, x, y, 0, 24, 24);
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
			}
		}
		EndGsCamera(&mGs);
	}
	HSD_EndRender();
	for (s32 i = 0; i < mXObjs.mObjectCount; ++i) {
		XObj* object = mXObjs.mObjects[i];
		if ((object->mStatus.mFlags & 1) && (object->mStatus.mFlags & 0x10000))
			ClearActiveObjectFloors(object); // the frame's floor contacts clear at the end
	}
	GetCamera()->SetInterest(interest); // and the shake's nudge comes off
}

/**
 * @note Address: 0x8000F8F4
 * @note Size: 0x38
 */
void CCamera::SetInterest(const Vec& interest)
{
	SetInterest(interest.x, interest.y, interest.z);
}

/**
 * @note Address: 0x8000F92C
 * @note Size: 0x10
 */
void CCamera::SetInterest(f32 x, f32 y, f32 z)
{
	mInterest.x = x;
	mInterest.y = y;
	mInterest.z = z;
}

/**
 * @note Address: 0x8000F93C
 * @note Size: 0x84
 */
CVec CVec::operator+(Vec v) const
{
	CVec sum = *this;
	PSVECAdd(&sum, &v, &sum);
	return sum;
}

/**
 * @note Address: 0x8000F9C0
 * @note Size: 0x80
 */
CVec CVec::operator*(f32 s) const
{
	CVec product = *this;
	PSVECScale(&product, &product, s);
	return product;
}

extern "C" {
void AXQuit();
void AIReset();
void AIStopDMA();
void GXDrawDone();
void GXAbortFrame();
void VISetBlack(BOOL);
void VIFlush();
void VIWaitForRetrace();
}

/**
 * @note Address: 0x8000FA40
 * @note Size: 0x8C
 */
// CGame's _48, CBase's with the mic reset first
void CGame::Reset(u32 code)
{
	s32 channel = gActiveMicChannel;
	if (channel >= 0 && !gMicrophoneChannels[channel].mDeviceID)
		MICReset(channel);
#if defined(VERSION_PAL)
	AXDriverStopAllStreams();
	HSD_AudioSFXKeyOffAll();
#else
	AXQuit();
	AIReset();
#endif
	HSD_VISetUserGXDrawDoneCallback(NULL);
	GXDrawDone();
	GXAbortFrame();
	VISetBlack(TRUE);
	VIFlush();
	VIWaitForRetrace();
#if defined(VERSION_PAL)
	AXQuit();
	AIStopDMA();
#endif
	OSResetSystem(code, 0, FALSE);
}

extern "C" {
void UpdateStageMicrophone(u32 frames);
void SetMusicPlaybackPaused(XSound* sound, BOOL pause);
void CMsg_Update(CMsg* msg, u32 frames);
void UpdateOpenMenus(MenuEx* menu, u32 frames);
void UpdateHudTransitionsAndIndicators(InfoScrn* info, u32 frames);
}

// Retail checks the queue head as a signed word. Cached RAM addresses have
// the sign bit set, so an ordinary pending request does not block the reset.
static bool IsDeviceQueueBlockingReset(s32 priority)
{
	return reinterpret_cast<s32>(HSD_DevComIsBusy(priority)) > 0;
}

/**
 * @note Address: 0x8000FACC
 * @note Size: 0x1204
 */
// CGame::Update: the base's disc and reset checks, the reset itself, then everything that
// runs each frame. while the disc can't be read only the sound is paused
void CGame::Update(u32 frames)
{
	s32 i;
	BaseUpdate(this, frames);
	PollMemoryCardWorker(&mMemcard.mCard);
	if (mReset && !mMemcard.mCard.mBusy && !IsDeviceQueueBlockingReset(1) && !IsDeviceQueueBlockingReset(2)) {
		if (!mDiskOk) {
			mResetHold = 0;
			mReset     = FALSE;
			HSD_PadReset();
			Reset(1);
		} else if (mLoadStep != 0x17 && mLoadStep != 0x1A && !HasPendingObjectReads()) {
			mResetHold = 0;
			mReset     = FALSE;
			HSD_PadReset();
			if (!mDiskOk) {
				Reset(1);
			} else if (mLoadStep <= 0x13) { // early in the load a reset restarts the system (OSResetSystem code 0)
				Reset(0);
			} else { // later it goes back to stage 10000
				ChangeGameStage(this, 10000, FALSE);
			}
		}
	}

	if (IsDiscError()) { // flag 8: the music paused while the disc can't be read
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
	if (mStatus.mFlags & 0x1000) { // flag 0x1000 queues a draw-list rebuild
		mStatus.mFlags &= ~0x1000;
		BuildObjectDrawList(this);
	}
	if ((mStatus.mFlags & 1) && !(mStatus.mFlags & 2) && !(mStatus.mFlags & 0x20) && !mMenu.mActive && mEvt.mData.mScript == NULL
	    && (mPad.mPad[0].mTrigger & PAD_BUTTON_START) && !(mPad.mPad[0].mButton & PAD_BUTTON_B)
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
				if (obj->mMoveTranslate.y != obj->mLastPos.y) {
					obj->mStatus.mFlags |= 0x10000000000000; // the flag marks an object whose height changed
				}
				obj->mLastPos = obj->mMoveTranslate;
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
		if (mPlayTime >= 21600000) { // caps the play time at a hundred hours of frames
			mPlayTime = 21599999;
		}
	}
	if (mStatus.mFlags & 0x2000000000000) { // flag 0x2000000000000 arms the stage timer
		if (IsRunning() && mEvt.mData.mScript == NULL && !mMenu.mActive && !(mStatus.mFlags & 0xC00) && mTimeState.mDuration > 0.0f) {
			if (!mTimeState.mRunning) {
				mTimeState.mTime = frames * (mTimeState.mDuration / (600.0f * FRAME_RATE)) + mTimeState.mTime;
				if (mTimeState.mTime >= 1.0f) { // a full timer runs down
					mTimeState.mRunning = TRUE;
				}
			}
			if (mTimeState.mRunning == TRUE) { // then waits for an idle player
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
			mScrnTex.mAngle = fmod(DEG2RAD * 6.0f * frames + mScrnTex.mAngle, TWO_PI); // six degrees a frame
			mOutline.mThick = 0.0f;
			u8 lineAlpha    = 100.0f * (f32)sin(mScrnTex.mAngle) + 150.0f; // pulses the outline alpha

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
	if (mOutlineWidthControl > 0) { // the c-stick sets the outline width
		if (mPad.mPad[0].mSubStickY > 0.0f) {
			if ((mOutlineWidth -= 0.01f * frames) < 0.04f) {
				mOutlineWidth = 0.04f;
			}
		} else if (mPad.mPad[0].mSubStickY < 0.0f) {
			if ((mOutlineWidth += 0.01f * frames) > 0.5f) {
				mOutlineWidth = 0.5f;
			}
		}
		mOutline.mZoom = sin(PI * mOutlineWidth / 2.0f); // a half-sine over the width
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
	if (!(mStatus.mFlags & 2) && mStageName.mActive > 0) {
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
		f32 bob    = DEG2RAD * 5.0f * frames; // bobs five degrees a frame
		f32 tilt   = DEG2RAD * 3.0f * frames; // tilts three
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
	if (mStatus.mFlags & 2) { // flag 2 suspends: flag 4 marks the pause done once
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
	} else if (mStatus.mFlags & 4) { // and resumes both on the way back
		mStatus.mFlags &= ~4;
		SetMusicPlaybackPaused(&gGame.mSound, FALSE);
		HSD_PadRumbleActiveAll();
	}
}

/**
 * @note Address: 0x80010CD0
 * @note Size: 0x48
 */
void CMenu::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
}

extern "C" u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 maxLod);
extern "C" BOOL UpdateStageObjectLoading(XOBJS* objs);
extern "C" void BuildObjectDrawList(CGame* game);
extern "C" void SelectSceneLights(CGame* game);

static inline HSD_JObj* GetChild(HSD_JObj* jobj)
{
	return jobj == NULL ? NULL : jobj->child;
}

// the last joint down the first children
static inline HSD_JObj* GetLeaf(HSD_JObj* jobj)
{
	while (GetChild(jobj) != NULL) {
		jobj = GetChild(jobj);
	}
	return jobj;
}

/**
 * @note Address: 0x80011638
 * @note Size: 0x5A0
 */
// NONMATCHING: the stage's camera mode and the second loop's registers rotate
// the step after the objects are read: points the camera and its actions at them, lists them
// and sets up a mirror for each object that has one, at most five
extern "C" BOOL FinalizeStageObjectSetup(CGame* game)
{
	FbMirrorEx* mirror;
	s32 mirrors;
	XObj* obj;
	s32 mode;
	s32 i;
	HSD_JObj* jobj;
	if (UpdateStageObjectLoading(&game->mXObjs)) {
		mode                                                           = GetStageInfo()->mCameraMode;
		((CCameraActionScroll*)gGame.mCameraActions.mTable[11])->mZoom = 1;
		if (GetStageCameraAreaRecords() && GetStageCameraInfoRecords()) {
			game->mCameraActions.mTable[1]->Reset();
		}
		if (GetStageCameraAreaRecords() && GetStageCameraInfoRecords()) {
			((CCameraActionAreaExM*)game->mCameraActions.mTable[1])->LoadAreaState();
		}
		((CCameraActionAnim*)game->mCameraActions.mTable[6])->SetObj((XObjCamera*)game->mXObjs.mPools[0].First());
		game->mMotionControlCamera.Init();
		if (mode == 1 || mode == 7) { // both modes start on the map camera
			game->mMotionControlCamera.ChangeAction(CAMACTION_MAP);
			static_cast<CCameraActionMap*>(game->mCameraActions.mTable[CAMACTION_MAP])->Reset2();
			game->mMotionControlCamera.ApplyAction();
		} else {
			game->mMotionControlCamera.ChangeAction(mode);
		}
		for (i = 0; i < game->mXObjs.mObjectCount; i++) {
			game->mMotionControlCamera.UpdateTarget(game->mXObjs.mObjects[i]);
		}

		if (game->mStatus.mFlags & 0x100) { // flag 0x100: a fresh roboeyes wipe
			game->mStatus.mFlags &= ~0x300;
			StartScreenWipe(&game->mRoboeyes, 0, 0, 0, 0, 1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		SelectSceneLights(game);
		BuildObjectDrawList(game);

		game->mStatus.mFlags &= ~0x2000000000;
		mirror = gFramebufferMirrors;
		game->mStatus.mFlags &= ~0x8000000000;
		game->mStatus.mFlags &= ~0x100000000000;
		mirrors = 0;
		for (i = 0; i < game->mXObjs.mObjectCount; i++) {
			obj = game->mXObjs.mObjects[i];
			if (obj->mStatus.mFlags & 1) {
				if (!(game->mStatus.mFlags & 0x2000000000) && (obj->mStatus.mFlags & 0x20000000)
				    && !(obj->mStatus.mFlags & 0x400000000000000)) {
					game->mStatus.mFlags |= 0x2000000000; // the stage wants the mirror fade
				}
				if ((obj->mStatus.mFlags & 0x400) && mirrors < 5) { // flag 0x400 makes an object a mirror
					game->mStatus.mFlags |= 0x100000000000;
					jobj = obj->mJObj;
					if (jobj != NULL) {
						mirror->mOwner           = obj;
						jobj                     = GetLeaf(jobj);
						mirror->mJoint           = jobj;
						mirror->mVisible         = FALSE;
						mirror->mTexture.mWidth  = 256;
						mirror->mTexture.mHeight = 256;
						mirror->mTexture.mFormat = GX_TF_RGB565;
						mirror->mTexture._0C     = 0;
						mirror->mTexture.mX      = 0.0f;
						mirror->mTexture.mY      = 0.0f;
						mirror->mTexture.mMipmap = 0;
						mirror->mTexture.mImage  = mm::Alloc(GXGetTexBufferSize(256, 256, GX_TF_RGB565, GX_FALSE, 0), "(mirror)");
						mirror->mActive          = TRUE;
						mirror->mTexture.mTex.Init(mirror->mTexture.mImage, mirror->mTexture.mWidth, mirror->mTexture.mHeight,
						                           mirror->mTexture.mFormat, GX_CLAMP, GX_CLAMP, GX_FALSE);
						for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
							HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
							if (mobj != NULL) {
								SetMaterialAmbientColor(mobj, 255, 255, 255); // the surface draws full bright
								HSD_TObj* tobj = HSD_MObjGetTObj(mobj);
								if (tobj != NULL) {
									tobj->imagedesc = (HSD_ImageDesc*)mirror; // the mirror supplies its image
								}
							}
						}
						mirror++;
						mirrors++;
					}
				}
				if (!(game->mStatus.mFlags & 0x8000000000) && (obj->mStatus.mFlags & 0x80000000)) {
					game->mStatus.mFlags |= 0x8000000000; // an object needs the depth check
				}
			}
		}

		if (game->mStatus.mFlags & 0x2000000000) {
			f32 fade  = GetStageInfo()->mMirrorFade / 100.0f;
			XObj* obj = game->mXObjs.mPools[3].First();
			if ((obj->mStatus.mFlags & 1) && fade > 0.0f && fade < 1.0f) { // a partial fade translucents the map
				obj->SetTranslucent(fade);
			}
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80010D18
 * @note Size: 0x10
 */
void CMenu::Resume()
{
	mSuspended   = FALSE;
	mSuspendFade = FALSE;
}

namespace {

const u64 ObjectActive     = 0x1;
const u64 DrawLast         = 0x40;
const u64 UpdateFirst      = 0x4000;
const u64 InScene          = 0x10000;
const u64 DrawLate         = 0xA00000;
const u64 CheckObjectDepth = 0x80000000;
const u64 MirrorOnly       = 0x1000000000000;

const u64 SkipDepthChecks = 0x100;
const u64 CheckSceneDepth = 0x8000000000;

void AddToScene(CGame* game, XObj* obj, s32& sceneCount, s32& mirrorCount)
{
	if (!(obj->mStatus.mFlags & MirrorOnly)) {
		game->mDrawLists[1].mObjects[sceneCount++] = obj;
	}
	game->mDrawLists[2].mObjects[mirrorCount++] = obj;
}

bool IsInScene(const XObj* obj)
{
	return (obj->mStatus.mFlags & (ObjectActive | InScene)) == (ObjectActive | InScene);
}

void AddObjectsByDrawOrder(CGame* game, u64 required, u64 excluded, s32& sceneCount, s32& mirrorCount)
{
	for (s32 i = 0; i < game->mXObjs.mObjectCount; ++i) {
		XObj* obj = game->GetObject(i);
		u64 flags = obj->mStatus.mFlags;
		if (obj->GetCategory() != 3 && IsInScene(obj) && (required == 0 || (flags & required)) && !(flags & excluded)) {
			AddToScene(game, obj, sceneCount, mirrorCount);
		}
	}
}

void AddObjectsByUpdateOrder(CGame* game, const XObj* player, const XObj* camera, bool updateFirst, s32& count)
{
	XObjDrawList& frame = game->mDrawLists[0];
	for (s32 i = 0; i < game->mXObjs.mObjectCount; ++i) {
		XObj* obj = game->GetObject(i);
		if (obj != player && obj != camera && (obj->mStatus.mFlags & ObjectActive)
		    && ((obj->mStatus.mFlags & UpdateFirst) != 0) == updateFirst) {
			frame.mIndices[count]   = i;
			frame.mObjects[count++] = obj;
		}
	}
}

} // namespace

extern "C" void BuildSceneDrawLists(CGame* game)
{
	if (game->mStatus.mFlags & CheckSceneDepth) {
		for (s32 i = 0; i < game->mXObjs.mObjectCount; ++i) {
			XObj* obj = game->GetObject(i);
			if (!IsInScene(obj)) {
				continue;
			}
			if (!(game->mStatus.mFlags & SkipDepthChecks) && (obj->mStatus.mFlags & CheckObjectDepth)
			    && obj->IsOutsideCameraDepth(&game->mMotionControlCamera.mCamera)) {
				obj->mStatus.mFlags |= MirrorOnly;
			} else {
				obj->mStatus.mFlags &= ~MirrorOnly;
			}
		}
	}

	// Maps precede the three draw-priority groups of other objects.
	s32 sceneCount          = 0;
	s32 mirrorCount         = 0;
	const XObjPool& mapPool = game->mXObjs.mPools[3];
	for (s32 i = 0; i < mapPool.mUsed; ++i) {
		XObj* obj = mapPool.Get(i);
		if (IsInScene(obj) && !(obj->mStatus.mFlags & (DrawLate | DrawLast))) {
			AddToScene(game, obj, sceneCount, mirrorCount);
		}
	}
	AddObjectsByDrawOrder(game, 0, DrawLate | DrawLast, sceneCount, mirrorCount);
	AddObjectsByDrawOrder(game, DrawLate, DrawLast, sceneCount, mirrorCount);
	AddObjectsByDrawOrder(game, DrawLast, 0, sceneCount, mirrorCount);
	game->mDrawLists[1].mCount = sceneCount;
	game->mDrawLists[2].mCount = mirrorCount;
}

extern "C" void BuildObjectDrawList(CGame* game)
{
	XObjDrawList& frame = game->mDrawLists[0];
	XObj* player        = game->mXObjs.mPools[2].First();
	XObj* camera        = game->mXObjs.mPools[0].First();
	s32 count           = 0;
	AddObjectsByUpdateOrder(game, player, camera, true, count);
	AddObjectsByUpdateOrder(game, player, camera, false, count);
	// The player and camera run last. They have no draw-list index.
	if (player->mStatus.mFlags & ObjectActive) {
		frame.mObjects[count++] = player;
	}
	if (camera->mStatus.mFlags & ObjectActive) {
		frame.mObjects[count++] = camera;
	}
	frame.mCount = count;
}

extern "C" void SelectSceneLights(CGame* game)
{
	XObjMap* map   = static_cast<XObjMap*>(game->mXObjs.mPools[3].First());
	CLight* lights = (map->mStatus.mFlags & ObjectActive) ? map->GetLights() : NULL;
	if (lights == NULL) {
		XObjLight* sceneLight = static_cast<XObjLight*>(game->mXObjs.mPools[1].First());
		if (sceneLight->mStatus.mFlags & ObjectActive) {
			lights = sceneLight->GetLights();
		}
	}

	game->mLightList = lights != NULL ? lights : game->mLights;
	if (lights == NULL) {
		return;
	}

	for (CLight* light = lights; light != NULL; light = static_cast<CLight*>(light->next)) {
		if (light->mShadow.IsActive()) {
			HSD_CObjSetProjectionType(light->mShadow.mShadow->camera, PROJ_ORTHO);
		}
	}
	for (CLight* light = game->mLights; light != NULL; light = static_cast<CLight*>(light->next)) {
		if (light->mShadow.IsActive()) {
			light->mShadow.SetActive(FALSE);
		}
	}
}

/**
 * @note Address: 0x8001160C
 * @note Size: 0x28
 */
// CCameraActionAreaExM's LoadAreaState, resets the shared area camera state
void CCameraActionAreaExM::LoadAreaState()
{
	LoadStageCameraAreaState(&gCameraAreaState);
}

/**
 * @note Address: 0x80011634
 * @note Size: 0x4
 */
void CInterfaceCameraAction::Reset()
{
}

extern "C" {
u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 maxLod);
BOOL UpdateStageObjectLoading(XOBJS* objs);
}

extern "C" {
BOOL UpdateProgressiveScanPrompt(u32);
BOOL LoadGameSoundBank(XSound*, s32);
BOOL LoadMenuGroupResources(MenuEx*);
BOOL LoadEventResources(CEvt*);
BOOL LoadMessageCursorObject(CMsg*);
BOOL HideHudPointCounters(InfoScrn*);
BOOL LoadGlobalResourceData();
void SetAttachmentCountById(AttachmentValues*, s32, s32);
u32 CheckStageFlag(s32, u32);
BOOL LoadStageByIndex(s32);
void ApplyStageClearColor();
const s32* GetStageEventObjectPairs();
void RegisterEventObjectId(CEvt*, s16, s16);
void TakeNextCounterValue(int, void*, void*, BOOL);
u32 ARGetSize();
}

static inline BOOL PreloadStageArchives()
{
	ARPreCache& cache = gAramPrecache;
	switch (cache.mLoadStep) {
	case 0:
		if (cache.mFile0.ConvertPathToEntrynum("/qp/pc.idx") < 0 || cache.mFile1.ConvertPathToEntrynum("/qp/pc.bin") < 0)
			return TRUE;
		++cache.mLoadStep;
	case 1: {
		if (!cache.mFile0.ReadEx("/qp/pc.idx", 0, 0))
			break;
		++cache.mLoadStep;
		s32 entry   = DVDConvertPathToEntrynum("/qp/pc.bin");
		u32 address = gARAMCacheInfo.mBaseAddress;
		HSD_DevComRequest(entry, 0, address, ARGetSize() - gARAMCacheInfo.mBaseAddress, HSD_DEVCOM_DVD_TO_ARAM, 1, TakeNextCounterValue,
		                  &cache.mLoadStep);
		break;
	}
	case 3: {
		XObjSource* src = GetObjectSources();
		const u8* pair  = (const u8*)cache.mFile0.mBuffer;
		if (pair) {
			for (s32 id = 0;; ++id, ++src) { // offset/size pairs end at 0xffffffff
				if (*(const u32*)pair == 0xffffffff || src->mCache.address == 0xffffffff)
					break;
				if (src->mCache.size != 1)
					continue;
				u32 address = ReadResourceOffset(pair), size = ReadResourceOffset(pair + 4);
				if (address + size >= ARGetSize())
					break;
				src->mCache.address = address;
				src->mCache.size    = size;
				u32 padded          = (size + 31) & ~31u; // aram blocks align to 32

				gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].address  = gARAMCacheInfo.mBaseAddress;
				gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].size     = padded;
				gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].objectId = id;
				gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].name     = src->mName;
				++gARAMCacheInfo.mCount;
				gARAMCacheInfo.mBaseAddress += padded;
				pair += 8;
			}
		}
		cache.mFile0.Close();
		++cache.mLoadStep;
	}
	case 4:
		return TRUE;
	}
	return FALSE;
}

static inline BOOL LoadStartingAttachments(CGame* game)
{
	if (!LoadGlobalResourceData())
		return FALSE;
	if (!gCmdLineArgs.mCopterLevelOverride) { // attachment slot 1 takes its starting level from the global resource table
		u32 level = GetStartingAttachmentLevel(0);
		SetAttachmentCountById(&game->GetPlayer()->mAttachments, 1, level);
	}
	if (!gCmdLineArgs.mPopperLevelOverride) { // slot 2
		u32 level = GetStartingAttachmentLevel(1);
		SetAttachmentCountById(&game->GetPlayer()->mAttachments, 2, level);
	}
	if (!gCmdLineArgs.mRadarLevelOverride) { // slot 3
		u32 level = GetStartingAttachmentLevel(2);
		SetAttachmentCountById(&game->GetPlayer()->mAttachments, 3, level);
	}
	return TRUE;
}

BOOL CGame::BeforeUpdate()
{
	// Ready stages fall through. Pending reads resume at the same stage next frame.
	switch (mLoadStep) {
	case 0:
		return TRUE;
	case 1:
		if (!UpdateProgressiveScanPrompt(GetRetraceStep()))
			break;
#if defined(VERSION_PAL)
		mOutline.SetSize(gGame.mGs.mScreen.fbWidth, gGame.mGs.mScreen.efbHeight);
		mBlur.SetSize(gGame.mGs.mScreen.fbWidth / 2, gGame.mGs.mScreen.efbHeight / 2);
#endif
		++mLoadStep;
	case 2:
		++mLoadStep;
	case 3:
		if (!LoadGameSoundBank(&mSound, -1))
			break;
		++mLoadStep;
	case 4:
		if ((mStatus.mFlags & 0x200000000ULL) && !mArchive.ReadEx("/qp.bin", 0, 0))
			break;
		++mLoadStep;
	case 5:
		if (!mTpl.ReadEx("tpl/misc.tpl", 0, 0))
			break;
		++mLoadStep;
		mScrnTex.mTex = mTpl.mTex.Init(&((TPLPalette*)mTpl.mBuffer)->descriptorArray[121 - StatusTextureOffset], 0); // the outline texture
		mOutline.mPatternTex = &mScrnTex.mTex;
		mOutline.mFrame      = 0;
		mOutline.mThick2     = 0;
	case 6:
		if ((mStatus.mFlags & 0x400000000ULL) && mLogo.mState != 15) {
			if (!mLogo.IsActive())
				mLogo.mState = 1;
			if (mLogo.mState <= 2)
				break;
		}
		++mLoadStep;
	case 7:
		if ((mStatus.mFlags & 0x40000000000ULL) && !PreloadStageArchives())
			break;
		++mLoadStep;
	case 8:
		if ((mStatus.mFlags & 0x400000000ULL) && mLogo.mState != 15)
			break;
		++mLoadStep;
	case 9:
		if (!mParticle.Read("particle/chibi_robo"))
			break;
		++mLoadStep;
	case 10:
		if (!LoadMenuGroupResources(&mMenu))
			break;
		++mLoadStep;
	case 11:
		if (!LoadMemoryCardResources(&mMemcard.mCard))
			break;
		++mLoadStep;
	case 12:
		++mLoadStep;
	case 13:
		if (!LoadEventResources(&mEvt))
			break;
		++mLoadStep;
	case 14:
		if (!LoadMessageCursorObject(mCMsgs))
			break;
		++mLoadStep;
	case 15:
		if (!HideHudPointCounters(&mInfoScrn))
			break;
		++mLoadStep;
	case 16:
		++mLoadStep;
	case 17:
		if (!LoadStartingAttachments(this))
			break;
		++mLoadStep;
	case 18:
		if (!GetPlayer()->LoadFromPlacement(CoordInfo(GetSpawnData(14)))) // spawn placement 14
			break;
		++mLoadStep;
	case 19:
		if ((mStatus.mFlags & 0x20000000) && mTitle.mState != 23) {
			if (!(mTitle.mState >= 1 && mTitle.mState < 23)) {
				ResetNewGameState();
				mTitle.mState = 1;
			}
			break;
		}
		++mLoadStep;
	case 20:
		if (mMemcardLoaded) { // a save restores the battery too
			RestoreGameSaveData(&mMemcard.mCard);
			mMemcardLoaded = FALSE;
			mStatus.mFlags |= 0x2000;
			InitBatteryGauge(&mInfoScrn, GetPlayer()->mParams.mBattery);
		}
		++mLoadStep;
	case 21:
		++mLoadStep;
	case 22:
		++mLoadStep;
		break;
	case 23: {
		if (!LoadStageByIndex(mStage))
			break;
		ApplyStageClearColor();
		const s32* pair = GetStageEventObjectPairs();
		while (pair[0] >= 0) { // the pairs end at a -1
			RegisterEventObjectId(&mEvt, pair[1], pair[0]);
			pair += 2;
		}
		++mLoadStep;
	}
	case 24: {
		u8* script = GetStageSetupScript();
		if (script)
			StartOrQueueEventScript(&mEvt, script, 0, -1, -1);
		++mLoadStep;
	}
	case 25:
		if (mEvt.mData.mScript) // waits for the setup script
			break;
		if (CheckStageFlag(gGame.mStage, 4)) // stage flag 4 sets flag 0x40
			mStatus.mFlags |= 0x40;
		++mLoadStep;
	case 26:
		if (!FinalizeStageObjectSetup(this))
			break;
		mStatus.mFlags &= ~0x40ULL;
		if (mStatus.mFlags & 0x2000)
			RestoreSavedPlayerAndCamera(&mMemcard.mCard);
		++mLoadStep;
	case 27:
		CallStageModule(0, 0, NULL);
		++mLoadStep;
	case 28: {
		if (mFade.mSpeed.r != 0 || mFade.mSpeed.g != 0 || mFade.mSpeed.b != 0 || mFade.mSpeed.a != 0)
			break;
		StartScreenFade(&mFade, 0, 0, 0, 255, 0, 0, 0, 0, 0.5f);
		StartOrQueueEventScript(&mEvt, GetStageEntryScript(), 0, -1, -1);
		u32 minutes = GetStageEntryTimeComponent(1), hours = GetStageEntryTimeComponent(0);
		_FF204 = (s32)(hours * 216000 + (minutes * 3600 + GetStageEntryTimeComponent(2) * 60)); // the clock, in frames
		mStatus.mFlags |= 1;
		++mLoadStep;
	}
	}
	return !(mLoadStep > 0 && mLoadStep < 29);
}

/**
 * @note Address: 0x80012A7C
 * @note Size: 0x10
 */
extern "C" void TakeNextCounterValue(int, void* context, void*, BOOL)
{
	++*static_cast<u32*>(context);
}

extern "C" {
#include <dolphin/os.h>
}
#include "game/cgame.h"
#include "game/newgamestate.h"
#include "game/arcache.h"
#include "game/mic.h"
#include "game/xchr.h"
#include "game/xobjs.h"
#include "game/gmath.h"
#include <Dolphin/ai.h>
#include <Dolphin/ar.h>
#include <Dolphin/mic.h>
#include <Dolphin/m2s.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

extern "C" {
void InitGameSound(XSound*);
s32 GetDefaultStageIndex();
s32 GetStageNum();
void LoadBootConfig();
void ResetProgressiveScanPrompt();
void InitGameMenus(MenuEx*);
void InitInfoScreen(InfoScrn*);
HSD_Generator* LinkJointSoundGenerator(int, int, int, HSD_JObj*);
HSD_Generator* LinkJointParticleGenerator(int, int, int, HSD_JObj*);
void InitMessageWindow(CMsg*);
}

#include "game/cgameutil.h"
#include <Dolphin/vi.h>
#include <Dolphin/dvd.h>

/**
 * @note Address: 0x80012A8C
 * @note Size: 0xAB0
 */
// NONMATCHING: retail has CBase::Start's body inline here, the two are in one file there
void CGame::Start()
{
	CBase::Start();
	mMotionControlCamera.InitCamera();
	mCameraActions.Init();
	mLightList = mLights;
	InitGameSound(&mSound);
	InitMemoryCardManager(&mMemcard.mCard);
	mMemcardLoaded = FALSE;

	MICInit();
	M2SInit();
	M2SSetBuffer((s16*)OSAllocFromHeap(__OSCurrHeap, 256)); // the mic sample buffer
	gMicSamplePhase[0]        = 0;
	gMicSamplePhase[1]        = 0;
	gMicDMABuffers[0]         = (s16*)OSAllocFromHeap(__OSCurrHeap, 640);
	gMicDMABuffers[1]         = (s16*)OSAllocFromHeap(__OSCurrHeap, 640);
	gPreviousAudioDMACallback = AIRegisterDMACallback(OnMicAudioDMA);
	memset(gMicDMABuffers[0], 0, 640);
	DCFlushRange(gMicDMABuffers[0], 640);
	gMicDMABufferIndex = 1;
	AIStartDMA();

	gARAMCacheInfo.mBaseAddress = ARAlloc(GetFreeARAMSize()); // claim all free aram for the cache
	ARGetSize();
	mLoadStep  = 1; // the staged load starts
	mStage     = GetDefaultStageIndex();
	mRoomCount = GetStageNum();
	LoadBootConfig();
	mPrevStage = -1;
	ResetProgressiveScanPrompt();

	InitToonTexture(&mToonTexture, 15, 31, 2);
	SetToonTObj(&mToonTexture.mImageDesc);
	mFade.Reset();
	mWipeCircle.Reset();
	mWipeTex.Reset();
	mWipeCustom.Reset();
	mWipeTile0.Reset();
	mWipeTile1.Reset();
	mRoboeyes.Reset();
#if defined(VERSION_PAL)
	mOutline.Init(gGame.mGs.mScreen.fbWidth, gGame.mGs.mScreen.efbHeight);
	mBlur.Init(gGame.mGs.mScreen.fbWidth / 2, gGame.mGs.mScreen.efbHeight / 2);
#else
	mOutline.Init(640, 448);
	mBlur.Init(320, 224);
#endif
	mOutline.SetLineColor(34, 9, 25);
	mOutline.mThick = 2;
	mOutline.SetColor2(100, 100, 200);
	mOutline.mThick2 = -2;

	mSubwin.Init();
	for (int i = 0; i < 5; ++i) {
		FbMirrorEx& mirror = gFramebufferMirrors[i];
		mirror.mJoint      = NULL;
		mirror.mCamera.Init(0.1f, 32768.0f, 0.5f, -0.5f, -0.5f, 0.5f, &gGame.mGs.mScreen);
		mirror.mCamera.SetViewport(0, 256, 0, 256); // a 256x256 mirror view
		mirror.mCamera.SetScissor(0, 256, 0, 256);
		mirror.Reset();
		mirror.mOwner = NULL;
	}

	ResetLogoFadeState(&mLogo);
	InitTitleScreen(&mTitle);
	InitGameMenus(&mMenu);
	InitInfoScreen(&mInfoScrn);
	mParticle.Init();
	HSD_JObjSetSPtclCallback(LinkJointSoundGenerator);
	HSD_JObjSetDPtclCallback(LinkJointParticleGenerator);
	ResetScreenShake(&mQuake);

	InitializeObjectPools(&mXObjs);
	mDrawLists[0].ClearObjects();
	mDrawLists[1].ClearObjects();
	mDrawLists[2].ClearObjects();

	InitializeEventScriptContext(&mEvt, &mCMsgs[0], NULL);
	for (int i = 0; i < 2; ++i) // two message windows
		InitMessageWindow(&mCMsgs[i]);
	ResetNewGameState();

	StartScreenFade(&mFade, 0, 0, 0, 255, 0, 0, 0, 255, 0.0f); // open on a fade and a tile wipe
	StartScreenWipeColorTransition(&mWipeTile1, 0, 0, 0, 0, 0, 0, 0, 320, 224, 640, 448, 640, 448, 0, 0, TRUE);

	mMovie.mMovie.Init();
	mMovieRect[0] = mMovieRect[1] = mMovieRect[2] = mMovieRect[3] = NAN; // no movie rect
}

/**
 * @note Address: 0x8001353C
 * @note Size: 0x20
 */
void CCamera::SetScissor(s32 left, u16 right, s32 top, u16 bottom)
{
	HSD_CObjSetScissorx4(this, left, right, top, bottom);
}

/**
 * @note Address: 0x8001355C
 * @note Size: 0x20
 */
void CCamera::SetViewport(f32 left, f32 right, f32 top, f32 bottom)
{
	HSD_CObjSetViewportfx4(this, left, right, top, bottom);
}

extern "C" {
char* strtok(char* str, const char* delim);
int strcmp(const char* a, const char* b);
}

CmdLineArgs gCmdLineArgs = { -1, -1, 3, 0, 0, 0, { 0, 0, 0 }, 3, 2, 1, 0, 0, -1 };

/**
 * @note Address: 0x80013580
 * @note Size: 0xCC
 */
// reads /stage:name and /slot:letter off the command line
void CGame::Initialize(int argc, char** argv)
{
	for (s32 i = 1; i < argc; i++) {
		switch (argv[i][0]) {
		case '/':
			char* name = strtok(argv[i] + 1, ":");
			if (strcmp(name, "stage") == 0) {
				gCmdLineArgs.mStage = FindStageIndexByName(strtok(NULL, " "));
			} else if (strcmp(name, "slot") == 0) {
				gCmdLineArgs.mSlot = *strtok(NULL, " ") - 'A';
			}
			break;
		}
	}
}

/**
 * @note Address: 0x8001357C
 * @note Size: 0x4
 */
void CScrnFade::Reset()
{
}

/**
 * @note Address: 0x8001364C
 * @note Size: 0x34
 */
// the particle jobj link callback, kind 0x48 on link 0 goes to link 1
extern "C" HSD_Generator* LinkJointParticleGenerator(int linkNo, int bank, int kind, HSD_JObj* jobj)
{
	if (linkNo == 0 && kind == 0x48)
		linkNo = 1;
	return psCreateJointGenerator(linkNo, bank, kind, jobj);
}

/**
 * @note Address: 0x80013680
 * @note Size: 0x34
 */
// the same for the sound link
extern "C" HSD_Generator* LinkJointSoundGenerator(int linkNo, int bank, int kind, HSD_JObj* jobj)
{
	if (linkNo == 0 && kind == 0x48)
		linkNo = 1;
	return psCreateQueuedJointGenerator(linkNo, bank, kind, jobj);
}

/**
 * @note Address: 0x800136B4
 * @note Size: 0xD0
 */
extern "C" void ConfigureSubwindowCamera(FbSubwin* subwin, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov)
{
	subwin->mCamera.SetInterest(interest);
	subwin->mCamera.SetAngleY(angleY);
	subwin->mCamera.SetAngleX(angleX);
	subwin->mCamera.SetDistance(distance);
	subwin->mCamera.SetFov(fov);
	subwin->mCamera.CalcEye();
}

/**
 * @note Address: 0x80013784
 * @note Size: 0x24
 */
void CCamera::SetFov(f32 fov)
{
	mFov = fov;
	HSD_CObjSetFov(this, fov);
}

/**
 * @note Address: 0x800137A8
 * @note Size: 0x8
 */
void CCamera::SetDistance(f32 distance)
{
	mDistance = distance;
}

/**
 * @note Address: 0x800137B0
 * @note Size: 0x8
 */
void CCamera::SetAngleX(f32 x)
{
	mAngle.x = x;
}

/**
 * @note Address: 0x800137B8
 * @note Size: 0x8
 */
void CCamera::SetAngleY(f32 y)
{
	mAngle.y = y;
}

namespace {

const u32 WindowFill = 0xff000000;

struct WindowEdges {
	f32 left;
	f32 top;
	f32 right;
	f32 bottom;
};

struct FrameTile {
	f32 texS;
	f32 texT;
	f32 texScaleS;
	f32 texScaleT;
	f32 x;
	f32 y;
	f32 width;
	f32 height;
};

CTexObj& WindowFrameTexture()
{
	CTpl& tpl           = gGame.mTpl;
	TPLPalette* palette = static_cast<TPLPalette*>(tpl.mBuffer);
	return tpl.mTex.Init(&palette->descriptorArray[158 - StatusTextureOffset], 0);
}

void DrawQuad(CGs& gs, CTexObj& tex, u32 color, f32 x, f32 y, f32 width, f32 height)
{
	DrawGsTexturedQuad(&gs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
}

void DrawCorner(CGs& gs, u32 color, f32 x, f32 y, f32 depth, f32 width, f32 height)
{
	DrawGsCircle(&gs, 16, color, color, x, y, depth, width, height, 0.0f, 0.0f, 0.0f);
}

WindowEdges GetWindowEdges(f32 x, f32 y, f32 halfWidth, f32 halfHeight)
{
	WindowEdges edges = { x - halfWidth, y - halfHeight, x + halfWidth, y + halfHeight };
	return edges;
}

WindowEdges GetCornerCenters(const WindowEdges& edges, f32 inset, f32 corner)
{
	WindowEdges centers = {
		corner + (inset + edges.left),
		corner + (inset + edges.top),
		(edges.right - inset) - corner,
		(edges.bottom - inset) - corner,
	};
	return centers;
}

void DrawCorners(CGs& gs, const WindowEdges& centers, u32 color, f32 depth, f32 diameter)
{
	DrawCorner(gs, color, centers.left, centers.top, depth, diameter, diameter);
	DrawCorner(gs, color, centers.right, centers.top, depth, diameter, diameter);
	DrawCorner(gs, color, centers.left, centers.bottom, depth, diameter, diameter);
	DrawCorner(gs, color, centers.right, centers.bottom, depth, diameter, diameter);
}

void DrawWindowImage(CGs& gs, FbTexture& texture, u32 alpha, f32 x, f32 y, f32 width, f32 height)
{
	SetGsDepthMode(gs.mCurRenderMode, 1, 2, 0);
	SetGsTevMode(gs.mCurTevStage, 0);
	if (texture.mImage != NULL) {
		DrawQuad(gs, texture.mTex, 0xffffff00 | alpha, x, y, width, height);
	}
	SetGsTevMode(gs.mCurTevStage, 4);
	SetGsDepthMode(gs.mCurRenderMode, 1, 3, 0);
}

void DrawCircularWindow(CGs& gs, FbSubwin& window, f32 fade, u32 alpha)
{
	CTexObj frame = WindowFrameTexture();
	f32 width     = fade * GXGetTexObjWidth(&frame);
	f32 height    = fade * GXGetTexObjHeight(&frame);
	f32 aspect    = static_cast<f32>(window.mTexture.mWidth) / window.mTexture.mHeight;
	const f32 x   = 160.0f;
	const f32 y   = 80.0f;
	DrawCorner(gs, WindowFill, x, y, 0.0f, width, height);
	DrawWindowImage(gs, window.mTexture, alpha, x, y, width * aspect, height);
	SetGsTevMode(gs.mCurTevStage, 0);
	DrawQuad(gs, frame, 0xc8c8c800 | alpha, x, y, 1.1f * width, 1.1f * height);
	SetGsTevMode(gs.mCurTevStage, 4);
}

void DrawRectangularWindow(CGs& gs, FbSubwin& window, f32 fade, f32 amount, u32 alpha)
{
	f32 width           = window.mRectWidth * fade;
	f32 height          = window.mRectHeight * fade;
	f32 corner          = 24.0f * fade;
	f32 diameter        = 2.0f * corner;
	f32 x               = window.mRectX;
	f32 y               = window.mRectY;
	f32 halfWidth       = width * 0.5f;
	f32 halfHeight      = height * 0.5f;
	WindowEdges edges   = GetWindowEdges(x, y, halfWidth, halfHeight);
	WindowEdges centers = GetCornerCenters(edges, fade, corner);
	f32 left            = fade + edges.left;
	f32 top             = fade + edges.top;
	f32 right           = edges.right - fade;
	f32 bottom          = edges.bottom - fade;
	Vec points[]        = {
        { x, y, 0.0f },
        { left, centers.top, 0.0f },
        { centers.left, top, 0.0f },
        { centers.right, top, 0.0f },
        { right, centers.top, 0.0f },
        { right, centers.bottom, 0.0f },
        { centers.right, bottom, 0.0f },
        { centers.left, bottom, 0.0f },
        { left, centers.bottom, 0.0f },
        { left, centers.top, 0.0f },
	};
	DrawGsTrianglePrimitive(&gs, GX_TRIANGLEFAN, gs.mCamera->mViewMtx, points, WindowFill, ARRAY_SIZE(points));
	SetGsDepthMode(gs.mCurRenderMode, 1, 1, 1);
	DrawCorners(gs, centers, WindowFill, 0.0f, diameter);

	const f32 shadowX     = 6.0f;
	const f32 shadowY     = 4.0f;
	const f32 shadowDepth = -0.1f;
	u32 shadow            = static_cast<u32>(96.0f * amount);
	for (u32 i = 0; i < ARRAY_SIZE(points); ++i) {
		points[i].x += shadowX;
		points[i].y += shadowY;
		points[i].z = shadowDepth;
	}
	DrawGsTrianglePrimitive(&gs, GX_TRIANGLEFAN, gs.mCamera->mViewMtx, points, shadow, ARRAY_SIZE(points));
	WindowEdges shadowEdges = GetWindowEdges(x + shadowX, y + shadowY, halfWidth, halfHeight);
	DrawCorners(gs, GetCornerCenters(shadowEdges, fade, corner), shadow, shadowDepth, diameter);
	DrawWindowImage(gs, window.mTexture, alpha, x, y, width, height);

	CTexObj frame           = WindowFrameTexture();
	u16 texWidth            = GXGetTexObjWidth(&frame);
	u16 texHeight           = GXGetTexObjHeight(&frame);
	f32 texelWidth          = 1.0f / texWidth;
	f32 texelHeight         = 1.0f / texHeight;
	f32 halfCorner          = corner * 0.5f;
	f32 leftTile            = edges.left + halfCorner;
	f32 rightTile           = edges.right - halfCorner;
	f32 topTile             = edges.top + halfCorner;
	f32 bottomTile          = edges.bottom - halfCorner;
	f32 horizontalSpan      = width - diameter;
	f32 verticalSpan        = height - diameter;
	const FrameTile tiles[] = {
		{ 0.0f, 0.0f, 0.5f, 0.5f, leftTile, topTile, corner, corner },
		{ 0.5f, 0.0f, texelWidth, 0.5f, x, topTile, horizontalSpan, corner },
		{ 0.5f, 0.0f, 0.5f, 0.5f, rightTile, topTile, corner, corner },
		{ 0.0f, 0.5f, 0.5f, texelHeight, leftTile, y, corner, verticalSpan },
		{ 0.5f, 0.5f, 0.5f, texelHeight, rightTile, y, corner, verticalSpan },
		{ 0.0f, 0.5f, 0.5f, 0.5f, leftTile, bottomTile, corner, corner },
		{ 0.5f, 0.5f, texelWidth, 0.5f, x, bottomTile, horizontalSpan, corner },
		{ 0.5f, 0.5f, 0.5f, 0.5f, rightTile, bottomTile, corner, corner },
	};
	SetGsTevMode(gs.mCurTevStage, 0);
	for (u32 i = 0; i < ARRAY_SIZE(tiles); ++i) {
		const FrameTile& tile = tiles[i];
		PSMTXTrans(frame.mMtx[0], tile.texS, tile.texT, 0.0f);
		PSMTXScale(frame.mMtx[1], tile.texScaleS, tile.texScaleT, 1.0f);
		frame.mMtxDirty = TRUE;
		DrawQuad(gs, frame, 0xffffff00 | alpha, tile.x, tile.y, tile.width, tile.height);
	}
	SetGsTevMode(gs.mCurTevStage, 4);
}

} // namespace

/**
 * @note Address: 0x800137C0
 * @note Size: 0xF10
 */
void FbSubwin::Draw()
{
	if ((mFlags.mFlags & 4) || mAmount == 0.0f) {
		return;
	}

	CGs& gs = gGame.mGs;
	SetGsChannelEnabled(&gs, 4, 0);
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsAlphaCompare(gs.mCurRenderMode, 7, 0, 0, 7, 255);
	SetGsDepthMode(gs.mCurRenderMode, 1, 7, 1);
	SetGsColorUpdate(gs.mCurRenderMode, 1);
	SetGsAlphaUpdate(gs.mCurRenderMode, 0);
	SetupAll(TRUE);

	const f32 amount = mAmount;
	f32 fade         = sin(0.5f * (PI * amount));
	u32 alpha        = static_cast<u32>(255.0f * amount);
	if (fpclassify(mRectWidth) == FP_NAN || fpclassify(mRectHeight) == FP_NAN) {
		DrawCircularWindow(gs, *this, fade, alpha);
	} else {
		DrawRectangularWindow(gs, *this, fade, amount, alpha);
	}

	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsAlphaCompare(gs.mCurRenderMode, 4, 0, 0, 3, 255);
	SetGsDepthMode(gs.mCurRenderMode, 1, 3, 0);
}

/**
 * @note Address: 0x800146D0
 * @note Size: 0x1F0
 */
// FbSubwin's Copy
void FbSubwin::Copy()
{
	CGs* graphics = CaptureGraphics();
	SetGsColorUpdate(graphics->mCurRenderMode, 1);
	SetGsAlphaUpdate(graphics->mCurRenderMode, 1);
	SetGsDepthMode(graphics->mCurRenderMode, 0, 7, 1);
	FlushCaptureGraphics(graphics);
	CopyCaptureTexture(&mTexture, graphics);
}

extern "C" {
u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 maxLod);
}

/**
 * @note Address: 0x80014978
 * @note Size: 0xEC
 */
void FbSubwin::Init()
{
#if defined(VERSION_PAL)
	GXRenderModeObj* mode = &gGame.mGs.mScreen;
	u16 width             = mode->fbWidth / 2;
	u16 height            = mode->efbHeight / 2;
#else
	u16 width  = 320;
	u16 height = 224;
#endif
	FbTexture* texture = &mTexture;
	texture->mWidth    = width;
	texture->mHeight   = height;
	texture->mFormat   = GX_TF_RGB565;
	texture->_0C       = 0;
	texture->mX        = 0.0f;
	texture->mY        = 0.0f;
	texture->mMipmap   = FALSE;
	texture->mImage    = mm::Alloc(GXGetTexBufferSize(width, height, GX_TF_RGB565, FALSE, 0), "(subwin)");
	mActive            = TRUE;
	texture->mTex.Init(texture->mImage, texture->mWidth, texture->mHeight, texture->mFormat, GX_CLAMP, GX_CLAMP, FALSE);
	InitGamePerspectiveCamera(&mCamera, &gGame.mGs.mScreen);
#if defined(VERSION_PAL)
	HSD_RectS16 viewport = { 0, 0, 0, 0 };
	viewport.xmax        = width;
	viewport.ymax        = height;
#else
	HSD_RectS16 viewport = { 0, 320, 0, 224 };
#endif
	HSD_CObjSetViewport(&mCamera, &viewport);
	Reset();
}

/**
 * @note Address: 0x80014964
 * @note Size: 0x14
 */
void FbSubwin::Reset()
{
	mEnabled = FALSE;
	mAmount  = 0.0f;
}

/**
 * @note Address: 0x800148C0
 * @note Size: 0xA4
 */
void FbSubwin::Update(u32 frames)
{
	if (mEnabled) {
		if (mAmount < 1.0f) {
			if ((mAmount += (1.0f / (0.3f * FRAME_RATE)) * frames) >= 1.0f) {
				mAmount = 1.0f;
			}
		}
	} else {
		if (mAmount > 0.0f) {
			if ((mAmount -= (1.0f / (0.3f * FRAME_RATE)) * frames) <= 0.0f) {
				mAmount = 0.0f;
			}
		}
	}
}

/**
 * @note Address: 0x80014A64
 * @note Size: 0x144
 */
// FbMirror's Draw, sets up gx for drawing the mirror's texture
void FbMirror::Draw()
{
	if (mJoint) {
		CGs* graphics = CaptureGraphics();
		if (graphics->mCurRenderMode)
			ApplyGsRenderMode(graphics->mCurRenderMode, TRUE);
		if (graphics->mCurTevStage)
			ApplyGsTevStage(graphics->mCurTevStage, TRUE);
		if (graphics->mCurChannel)
			ApplyGsChannel(graphics->mCurChannel, TRUE);
		SetGsChannelEnabled(&gGame.mGs, 4, 0);
		SetGsBlendMode(graphics->mCurRenderMode, 0, 4, 5, 15);
		SetGsAlphaCompare(graphics->mCurRenderMode, 7, 0, 0, 7, 255);
		SetGsDepthMode(graphics->mCurRenderMode, 1, 3, 1);
		SetGsColorUpdate(graphics->mCurRenderMode, 1);
		SetGsAlphaUpdate(graphics->mCurRenderMode, 0);
		CGs* global = CaptureGraphics();
		SetGsTevMode(global->mCurTevStage, 0);
		SetGsCullMode(graphics->mCurRenderMode, 1);
		SetGsCullMode(graphics->mCurRenderMode, 2);
		SetGsTevMode(global->mCurTevStage, 4);
		SetGsAlphaCompare(graphics->mCurRenderMode, 4, 0, 0, 3, 255);
	}
}

/**
 * @note Address: 0x80014BA8
 * @note Size: 0x1FC
 */
// FbMirror's Copy
void FbMirror::Copy()
{
	if (mVisible) {
		CGs* graphics = CaptureGraphics();
		SetGsColorUpdate(graphics->mCurRenderMode, 1);
		SetGsAlphaUpdate(graphics->mCurRenderMode, 1);
		SetGsDepthMode(graphics->mCurRenderMode, 0, 7, 1);
		FlushCaptureGraphics(graphics);
		CopyCaptureTexture(&mTexture, graphics);
	}
}

extern "C" {
#include <sysdolphin/video.h>

CGame gGame;
ARCacheInfo gARAMCacheInfo;
FbMirrorEx gFramebufferMirrors[5];
ARPreCache gAramPrecache;
CThread gMovieUpdateThread;

void AXQuit(void);
void InitGameRuntime();
void InitGameRenderState(CBase* base);
void InitGameARAM();
void InitGameDVD();
#if defined(VERSION_PAL)
u8 OSGetLanguage();
// os language and game language pairs
extern u8 gGameLanguageMap[12];
#endif
}

/**
 * @note Address: 0x8001552C
 * @note Size: 0x8
 */
BOOL CBase::BeforeUpdate()
{
	return TRUE;
}

/**
 * @note Address: 0x80015534
 * @note Size: 0x100
 */
// NONMATCHING: pool symbol names (sGameVectorDefaults) and regalloc in the
// language-map loop
// CBase's Start
void CBase::Start()
{
	InitGameRuntime();
	InitGameRenderState(this);
	InitGameARAM();
	InitGameDVD();
#if defined(VERSION_PAL)
	u8 language = OSGetLanguage();
	for (s32 i = 0; i < 6; ++i) {
		if (language == gGameLanguageMap[i * 2]) {
			mLanguage = gGameLanguageMap[i * 2 + 1];
			break;
		}
	}
#elif !defined(VERSION_JP)
	mLanguage = 0;
#endif
#if defined(VERSION_JP)
	mFont.Init();
#else
	mFont.Init(gGame.mLanguage != 6);
#endif
	mPad.Init();
	mLastRetrace   = 0;
	mRetraceStep   = 0;
	mStatus.mFlags = 0;
	InitGamePerspectiveCamera(&mCameras[0], &mGs.mScreen);
	InitGameOrthoCamera(&mCameras[1], &mGs.mScreen);
	InitFourLightSet(&mLights[0]);
	InitTwoLightSet(&mLights[4]);
	OSInitStopwatch(&mStopwatch[0], "__$dsync$ ");
	OSInitStopwatch(&mStopwatch[1], "__$update$");
	OSInitStopwatch(&mStopwatch[2], "__$draw$  ");
	mPrevDvdStatus = 0;
	mDvdStatus     = 0;
	mDiskOk        = 1;
	mDvdError      = 0;
	mResetHold     = 0;
	mReset         = 0;
}

/**
 * @note Address: 0x800153B4
 * @note Size: 0x4
 */
void CBase::DrawHud()
{
}

/**
 * @note Address: 0x800153B8
 * @note Size: 0x4
 */
void CBase::DrawScene()
{
}

/**
 * @note Address: 0x80015634
 * @note Size: 0x4
 */
void CBase::Initialize(int argc, char** argv)
{
}

/**
 * @note Address: 0x80015638
 * @note Size: 0x8
 */
s32 CBase::FrameInterval()
{
	return 1;
}

/**
 * @note Address: 0x80015640
 * @note Size: 0x4
 */
void CBase::Draw()
{
}

extern "C" {
void GXDrawDone(void);
void GXAbortFrame(void);
}

/**
 * @note Address: 0x800153BC
 * @note Size: 0x170
 */
void CBase::Update(u32 frames)
{
	BaseUpdate(this, frames);
}

// stops the sound and the video and resets
inline void ResetSystem(u32 code)
{
#if defined(VERSION_PAL)
	AXDriverStopAllStreams();
	HSD_AudioSFXKeyOffAll();
#else
	AXQuit();
	AIReset();
#endif
	HSD_VISetUserGXDrawDoneCallback(NULL);
	GXDrawDone();
	GXAbortFrame();
	VISetBlack(TRUE);
	VIFlush();
	VIWaitForRetrace();
#if defined(VERSION_PAL)
	AXQuit();
	AIStopDMA();
#endif
	OSResetSystem(code, 0, 0);
}

/**
 * @note Address: 0x800152A0
 * @note Size: 0x60
 */
// CBase::Reset
void CBase::Reset(u32 code)
{
	ResetSystem(code);
}

extern "C" void GXProject(f32 x, f32 y, f32 z, const Mtx mtx, const f32* pm, const f32* vp, f32* sx, f32* sy, f32* sz);

/**
 * @note Address: 0x80015300
 * @note Size: 0xB4
 */
// where a point ends up on screen through a camera, the main one by default
extern "C" void ProjectWorldPositionToScreen(CGame* game, const Vec& pos, f32* x, f32* y, f32* z, CCamera* camera)
{
	camera = camera != NULL ? camera : &gGame.mMotionControlCamera.mCamera;
	f32 sx, sy, sz;
	GXProject(pos.x, pos.y, pos.z, camera->mViewMtx, camera->mProjection, camera->mViewport, &sx, &sy, &sz);
	if (x != NULL) {
		*x = sx;
	}
	if (y != NULL) {
#if defined(VERSION_PAL)
		*y = 448.0f * sy / gGame.mGs.mScreen.xfbHeight;
#else
		*y = sy;
#endif
	}
	if (z != NULL) {
		*z = sz;
	}
}

/**
 * @note Address: 0x80015644
 * @note Size: 0x1C8
 */
void main(int argc, char** argv)
{
	((CBase*)&gGame)->Initialize(argc, argv);
	((CBase*)&gGame)->Start();
	HSD_VISetBlack(false);
	while (true) {
		VIWaitForRetrace();
		gGame.mRetraceStep = VIGetRetraceCount() - gGame.mLastRetrace;
		gGame.mLastRetrace = VIGetRetraceCount();
		if (gGame.mRetraceStep > 4) { // a hitch only counts four frames
			gGame.mRetraceStep = 4;
		}
		OSResetStopwatch(&gGame.mStopwatch[0]);
		OSStartStopwatch(&gGame.mStopwatch[0]);
		OSResetStopwatch(&gGame.mStopwatch[1]);
		OSStartStopwatch(&gGame.mStopwatch[1]);
		OSResetStopwatch(&gGame.mStopwatch[2]);
		OSStartStopwatch(&gGame.mStopwatch[2]);
		HSD_VICopyXFBAsync(HSD_RP_SCREEN);
		gGame.mStopwatch[0].mElapsed = OSCheckStopwatch(&gGame.mStopwatch[0]);
		OSStopStopwatch(&gGame.mStopwatch[0]);
		gGame.mPad.Update(gGame.mRetraceStep);
		((CBase*)&gGame)->BeforeUpdate();
		((CBase*)&gGame)->Update(gGame.mRetraceStep);
		gGame.mStopwatch[1].mElapsed = OSCheckStopwatch(&gGame.mStopwatch[1]);
		OSStopStopwatch(&gGame.mStopwatch[1]);
		((CBase*)&gGame)->Draw();
		gGame.mStopwatch[2].mElapsed = OSCheckStopwatch(&gGame.mStopwatch[2]);
		OSStopStopwatch(&gGame.mStopwatch[2]);
#if defined(VERSION_PAL)
		s32 interval = ((CBase*)&gGame)->FrameInterval() * 60;
		while (VIGetRetraceCount() - gGame.mLastRetrace < (u32)(interval / gVideoFramesPerSecond - 1)) { }
#else
		s32 interval = ((CBase*)&gGame)->FrameInterval();
		u32 wait     = interval * 60 / 60 - 1;
		while (VIGetRetraceCount() - gGame.mLastRetrace < wait) { } // pads the frame to the interval
#endif
	}
}

/**
 * @note Address: 0x800151DC
 * @note Size: 0x4
 */
void FbMirror::Reset()
{
}

/**
 * @note Address: 0x80014DA4
 * @note Size: 0x2BC
 */
// FbMirror::Update, puts the mirror camera at the main camera's eye reflected in the surface,
// looking at the surface through a frustum that just fits it
void FbMirror::Update(u32 frames)
{
	if (IsUsed()) {
		HSD_JObj* jobj = mJoint;
		Mtx mtx;
		Mtx view;
		Vec eye;
		Vec pos;
		Vec diff;
		Vec x;
		Vec y;
		Vec normal;

		PSMTXCopy(HSD_JObjGetMtxPtr(jobj), mtx);
		x.x = mtx[0][0];
		x.y = mtx[1][0];
		x.z = mtx[2][0];
		y.x = mtx[0][1];
		y.y = mtx[1][1];
		y.z = mtx[2][1];
		PSVECScale(&x, &x, -1.0f); // the surface normal is -x cross y
		PSVECCrossProduct(&x, &y, &normal);
		PSVECNormalize(&normal, &normal);
		mtx[0][0] = x.x;
		mtx[1][0] = x.y;
		mtx[2][0] = x.z;
		mtx[0][2] = normal.x;
		mtx[1][2] = normal.y;
		mtx[2][2] = normal.z;

		CMotionControlCamera* main = &gGame.mMotionControlCamera;
		eye.x                      = main->mCamera.mEye.x;
		eye.y                      = main->mCamera.mEye.y;
		eye.z                      = main->mCamera.mEye.z;
		HSD_MtxGetTranslate(mtx, &pos);
		PSVECSubtract(&pos, &eye, &diff);
		f32 dist = PSVECDotProduct(&normal, &diff);
		if (dist <= 0.0f) { // behind the surface, the mirror hides
			mVisible = FALSE;
		} else {
			PSVECScale(&normal, &diff, 2.0f * dist); // the eye mirrored across the plane
			PSVECAdd(&diff, &eye, &diff);
			mtx[0][3] = diff.x;
			mtx[1][3] = diff.y;
			mtx[2][3] = diff.z;
			PSMTXInverse(mtx, view);
			PSMTXMultVec(view, &pos, &pos);
			mCamera.SetNear(0.1f - pos.z); // near starts at the surface's depth, far 32768 beyond it
			mCamera.SetFar(32768.0f + (0.1f - pos.z));
			mCamera.SetFrustum(0.5f + pos.y, -0.5f + pos.y, -0.5f + pos.x, 0.5f + pos.x);
			mCamera.SetEye(diff);
			mCamera.SetMtx(view);
			PSMTXCopy(view, mView);
			mVisible = TRUE;
		}
	}
}

/**
 * @note Address: 0x800151E0
 * @note Size: 0xC0
 */
// FbMirror::Init
void FbMirror::Init()
{
	mJoint = NULL;
	mCamera.Init(0.1f, 32768.0f, 0.5f, -0.5f, -0.5f, 0.5f, &gGame.mGs.mScreen);
	mCamera.SetViewport(0.0f, 256.0f, 0.0f, 256.0f); // the mirror renders a 256 square
	mCamera.SetScissor(0, 256, 0, 256);
	Reset();
}

/**
 * @note Address: 0x8001580C
 * @note Size: 0x50
 */
// the retail build compiles debug output out
extern "C" void OSReport(const char* format, ...)
{
}

/**
 * @note Address: 0x8001585C
 * @note Size: 0x1C
 */
bool CInterpolationNone::Start(f32 from, f32 to, f32 time)
{
	mTime  = 1.0f;
	mValue = to;
	mSpeed = 0.0f;
	return true;
}

/**
 * @note Address: 0x80015878
 * @note Size: 0x24
 */
bool CInterpolationNone::Step(f32 dt)
{
	mTime = 1.0f;
	return mTime <= 0.0f;
}

/**
 * @note Address: 0x8001589C
 * @note Size: 0x2C
 */
bool CInterpolationNone::Update(u32 step, f32 dt)
{
	return Step(dt);
}

/**
 * @note Address: 0x800158C8
 * @note Size: 0x8
 */
f32 CInterpolationNone::Get()
{
	return mValue;
}

/**
 * @note Address: 0x800158D0
 * @note Size: 0xCC
 */
bool CVectorNoneInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time < 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	return true;
}

/**
 * @note Address: 0x8001599C
 * @note Size: 0xB4
 */
bool CVectorNoneInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x80015A50
 * @note Size: 0x2C
 */
bool CVectorNoneInterpolation::Update(u32 step, f32 dt)
{
	return Step(dt);
}

/**
 * @note Address: 0x80015A7C
 * @note Size: 0xBC
 */
Vec CVectorNoneInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x80015BBC
 * @note Size: 0x98
 */
CCameraActionAreaExM::~CCameraActionAreaExM()
{
}

// CCameraActionAreaExM's _68 to _74, they all go to the shared area state

/**
 * @note Address: 0x80015C54
 * @note Size: 0x10
 */
f32 CCameraActionAreaExM::GetBaseFov()
{
	return gCameraAreaState.mBaseFov;
}

/**
 * @note Address: 0x80015C64
 * @note Size: 0x10
 */
s32 CCameraActionAreaExM::GetZoomLevel()
{
	return gCameraAreaState.mZoomLevel;
}

/**
 * @note Address: 0x80015C74
 * @note Size: 0x10
 */
void CCameraActionAreaExM::SetZoomLevel(s32 mode)
{
	gCameraAreaState.mZoomLevel = mode;
}

/**
 * @note Address: 0x80015C84
 * @note Size: 0x44
 */
s32 CCameraActionAreaExM::FindAreaContainingPoint(Vec pos)
{
	return FindFirstCameraAreaContainingPoint(&gCameraAreaState, pos);
}

/**
 * @note Address: 0x80015EF0
 * @note Size: 0x8
 */
f32 CInterfaceCameraAction::GetFollowDivisor()
{
	return 12.0f;
}

/**
 * @note Address: 0x80015F08
 * @note Size: 0xCC
 */
bool CVectorLinearInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time < 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	return true;
}

/**
 * @note Address: 0x80015FD4
 * @note Size: 0x30
 */
bool CInterpolationLinear::Start(f32 from, f32 to, f32 time)
{
	if (time < 0.0f) { // a negative time means no interp
		return false;
	}
	mTime  = time;
	mValue = from;
	mSpeed = (to - from) / time;
	return true;
}

/**
 * @note Address: 0x80016004
 * @note Size: 0xB4
 */
bool CVectorLinearInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x800160B8
 * @note Size: 0x38
 */
bool CInterpolationLinear::Step(f32 dt)
{
	mTime -= dt;
	mValue += mSpeed * dt;
	return mTime <= 0.0f;
}

/**
 * @note Address: 0x800160F0
 * @note Size: 0x2C
 */
bool CVectorLinearInterpolation::Update(u32 step, f32 dt)
{
	return Step(dt);
}

/**
 * @note Address: 0x8001611C
 * @note Size: 0xBC
 */
Vec CVectorLinearInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x800161D8
 * @note Size: 0x8
 */
f32 CInterpolationLinear::Get()
{
	return mValue;
}

/**
 * @note Address: 0x80016264
 * @note Size: 0xD8
 */
bool CVectorEaseInOutInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	mXZDone = FALSE;
	return true;
}

/**
 * @note Address: 0x8001633C
 * @note Size: 0xE0
 */
bool CVectorEaseInOutInterpolation::Update(u32 step, f32 dt)
{
	bool x = mAxes[0].Update(step, dt);
	bool y = mAxes[1].Update(step, dt);
	bool z = mAxes[2].Update(step, dt);
	if (x && z) { // x and z have both arrived
		mXZDone = TRUE;
	}
	return x && y && z;
}

/**
 * @note Address: 0x8001641C
 * @note Size: 0xB4
 */
bool CVectorEaseInOutInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x800164D0
 * @note Size: 0xBC
 */
Vec CVectorEaseInOutInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x80016610
 * @note Size: 0xD0
 */
bool CVectorEaseOutInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	return true;
}

/**
 * @note Address: 0x800166E0
 * @note Size: 0xC8
 */
bool CVectorEaseOutInterpolation::Update(u32 step, f32 dt)
{
	bool x = mAxes[0].Update(step, dt);
	bool y = mAxes[1].Update(step, dt);
	bool z = mAxes[2].Update(step, dt);
	return x && y && z;
}

/**
 * @note Address: 0x800167A8
 * @note Size: 0xB4
 */
bool CVectorEaseOutInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x8001685C
 * @note Size: 0xBC
 */
Vec CVectorEaseOutInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x80015CC8
 * @note Size: 0x64
 */
BOOL CCameraActionAreaExM::IsCameraArea(s32 index)
{
	if (CamAreaInfo(&gCameraAreaState.mAreaRecords[index]).IsCameraArea()) {
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80015D74
 * @note Size: 0x64
 */
BOOL CCameraActionAreaExM::IsAreaEnabled(s32 index)
{
	if (CamAreaInfo(&gCameraAreaState.mAreaExRecords[index]).IsDisabled()) {
		return FALSE;
	}
	return TRUE;
}

/**
 * @note Address: 0x800169E4
 * @note Size: 0xD8
 */
bool CVectorEaseOutDivideInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	mXZDone = FALSE;
	return true;
}

/**
 * @note Address: 0x80016ABC
 * @note Size: 0x2C
 */
bool CInterpolationEaseOutDivide::Start(f32 from, f32 to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	mValue   = from;
	mTarget  = to;
	mDivisor = time;
	return true;
}

/**
 * @note Address: 0x80016AE8
 * @note Size: 0xE0
 */
bool CVectorEaseOutDivideInterpolation::Update(u32 step, f32 dt)
{
	bool x = mAxes[0].Update(step, dt);
	bool y = mAxes[1].Update(step, dt);
	bool z = mAxes[2].Update(step, dt);
	if (x && z) {
		mXZDone = TRUE;
	}
	return x && y && z;
}

/**
 * @note Address: 0x80016BC8
 * @note Size: 0x84
 */
bool CInterpolationEaseOutDivide::Update(u32 step, f32 dt)
{
	bool done = false;
	for (u32 i = 0; i < step; i++) {
		done = Step(dt);
		if (done == true) {
			break;
		}
	}
	return done;
}

/**
 * @note Address: 0x80016C4C
 * @note Size: 0x58
 */
bool CInterpolationEaseOutDivide::Step(f32 dt)
{
	f32 last = mValue;
	if (mDivisor > 0.0f) {
		mValue = (last * (mDivisor - 1.0f) + mTarget) / mDivisor;
	}
	if ((f32)fabs(mValue - last) < 0.001f) {
		return true;
	}
	return false;
}

/**
 * @note Address: 0x80016CA4
 * @note Size: 0xB4
 */
bool CVectorEaseOutDivideInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x80016D58
 * @note Size: 0xBC
 */
Vec CVectorEaseOutDivideInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x80016E14
 * @note Size: 0x8
 */
f32 CInterpolationEaseOutDivide::Get()
{
	return mValue;
}

/**
 * @note Address: 0x80016EA0
 * @note Size: 0x2C
 */
bool CInterpolationLinear::Update(u32 step, f32 dt)
{
	return Step(dt);
}

/**
 * @note Address: 0x80016ECC
 * @note Size: 0x88
 */
CInterfaceCameraAction::~CInterfaceCameraAction()
{
}

/**
 * @note Address: 0x80016F54
 * @note Size: 0x48
 */
CInterfaceInterpolation::~CInterfaceInterpolation()
{
}

/**
 * @note Address: 0x80016F9C
 * @note Size: 0x88
 */
CCameraMemento::~CCameraMemento()
{
}

/**
 * @note Address: 0x80017024
 * @note Size: 0x48
 */
CMsgData::~CMsgData()
{
}

/**
 * @note Address: 0x8001706C
 * @note Size: 0xC8
 */
CMsgFont::~CMsgFont()
{
}

/**
 * @note Address: 0x80017134
 * @note Size: 0x88
 */
CObjVec::~CObjVec()
{
}

/**
 * @note Address: 0x800171BC
 * @note Size: 0x88
 */
CJObj::~CJObj()
{
}

/**
 * @note Address: 0x80018BD0
 * @note Size: 0x6C
 */
CScrnWipeCircle::~CScrnWipeCircle()
{
}

/**
 * @note Address: 0x80018C3C
 * @note Size: 0x48
 */
CFbOutline::~CFbOutline()
{
}

/**
 * @note Address: 0x80018C84
 * @note Size: 0x48
 */
CFbBlur::~CFbBlur()
{
}

/**
 * @note Address: 0x80018CCC
 * @note Size: 0x4
 */
void CFbTexEff::Init()
{
}

/**
 * @note Address: 0x80018CD0
 * @note Size: 0x4
 */
void CFbTexEff::Reset()
{
}

/**
 * @note Address: 0x80018CD4
 * @note Size: 0x4
 */
void CFbTexEff::Copy()
{
}

/**
 * @note Address: 0x80018CD8
 * @note Size: 0x4
 */
void CFbTexEff::Update(u32 frames)
{
}

/**
 * @note Address: 0x80018CDC
 * @note Size: 0x4
 */
void CFbTexEff::Draw()
{
}

/**
 * @note Address: 0x80018D28
 * @note Size: 0x90
 */
Logo::~Logo()
{
}

/**
 * @note Address: 0x80018E14
 * @note Size: 0x48
 */
CScrnColor::~CScrnColor()
{
}

/**
 * @note Address: 0x80018DB8
 * @note Size: 0x5C
 */
CScrnFade::~CScrnFade()
{
}

/**
 * @note Address: 0x80018E5C
 * @note Size: 0x98
 */
InfoScrn::~InfoScrn()
{
}

/**
 * @note Address: 0x80018EF4
 * @note Size: 0x88
 */
CParticle::~CParticle()
{
}

/**
 * @note Address: 0x80018F7C
 * @note Size: 0x6C
 */
CScrnQuake::~CScrnQuake()
{
}

/**
 * @note Address: 0x80018FE8
 * @note Size: 0x48
 */
XOBJS::~XOBJS()
{
}

/**
 * @note Address: 0x800191A4
 * @note Size: 0x74
 */
XSound::~XSound()
{
}

/**
 * @note Address: 0x80019264
 * @note Size: 0x64
 */
Memcard::~Memcard()
{
}

/**
 * @note Address: 0x800192C8
 * @note Size: 0x48
 */
CMovie::~CMovie()
{
}

/**
 * @note Address: 0x80019310
 * @note Size: 0x2C
 */
CLight* CBase::GetLight()
{
	return GetDefaultLight();
}

u8 gMovieUpdateThreadStack[0x1000];

/**
 * @note Address: 0x80019480
 * @note Size: 0x8C
 */
BOOL CThread::Create(OSThreadStartFunction func, void* arg, u32 stackSize, OSPriority priority, u16 attr)
{
	mStack     = static_cast<u8*>(OSAllocFromHeap(__OSCurrHeap, stackSize));
	mStackSize = stackSize;
	if (OSCreateThread(&mThread, func, arg, mStack + mStackSize, mStackSize, priority, attr)) {
		mStackSize = stackSize;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8001950C
 * @note Size: 0x48
 */
CThread::~CThread()
{
}

/**
 * @note Address: 0x80019554
 * @note Size: 0x74
 */
ARPreCache::~ARPreCache()
{
}

/**
 * @note Address: 0x80019600
 * @note Size: 0x100
 */
FbMirrorEx::~FbMirrorEx()
{
}

/**
 * @note Address: 0x80019700
 * @note Size: 0x5C
 */
FbMirrorEx::FbMirrorEx()
{
}

/**
 * @note Address: 0x8001975C
 * @note Size: 0x48
 */
ARCacheInfo::~ARCacheInfo()
{
}

// CGame's constructor and the constructors and destructors the compiler writes for the
// objects it owns

/**
 * @note Address: 0x800197A4
 * @note Size: 0x6B8
 */
CGame::CGame()
{
}

/**
 * @note Address: 0x8001A45C
 * @note Size: 0x190
 */
CCameraActionMap::CCameraActionMap()
{
}

/**
 * @note Address: 0x80019E5C
 * @note Size: 0x488
 */
CCameraActions::CCameraActions()
{
}

/**
 * @note Address: 0x8001AF40
 * @note Size: 0x30
 */
MsgCursor::MsgCursor()
{
}

/**
 * @note Address: 0x8001AF70
 * @note Size: 0x3C
 */
Cursor::Cursor()
{
}

/**
 * @note Address: 0x8001B0AC
 * @note Size: 0x5C
 */
CJAnim::~CJAnim()
{
}

/**
 * @note Address: 0x8001B108
 * @note Size: 0x48
 */
CJArchive::~CJArchive()
{
}

/**
 * @note Address: 0x8001B2EC
 * @note Size: 0x40
 */
CObjArchive::CObjArchive()
{
	mRefCount = 0;
}

/**
 * @note Address: 0x8001B33C
 * @note Size: 0x20
 */
CGenerator::CGenerator()
{
	mId  = -1;
	mGen = NULL;
}

MenuEx::MenuEx()
{
}

MenuCursor::MenuCursor()
{
}

Title::Title()
{
}

XObjDemo::XObjDemo()
{
}

/**
 * @note Address: 0x8001B5A4
 * @note Size: 0x6C
 */
CScrnRoboeyes::~CScrnRoboeyes()
{
}

/**
 * @note Address: 0x8001B610
 * @note Size: 0x6C
 */
CScrnWipeTile::~CScrnWipeTile()
{
}

/**
 * @note Address: 0x8001B67C
 * @note Size: 0x6C
 */
CScrnWipeCustom::~CScrnWipeCustom()
{
}

/**
 * @note Address: 0x8001B6E8
 * @note Size: 0x84
 */
CScrnWipeTex::~CScrnWipeTex()
{
}

/**
 * @note Address: 0x8001B76C
 * @note Size: 0x5C
 */
CScrnWipe::~CScrnWipe()
{
}

/**
 * @note Address: 0x8001B8A0
 * @note Size: 0xDC
 */
CBase::CBase()
{
}

/**
 * @note Address: 0x8001B9B8
 * @note Size: 0x1A4
 */
CGs::CGs()
{
}

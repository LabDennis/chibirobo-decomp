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
void InitMemoryCardManager(Memcard*);
s32 GetDefaultStageIndex();
s32 GetStageNum();
void LoadBootConfig();
void ResetProgressiveScanPrompt();
void InitGameMenus(MenuEx*);
void InitInfoScreen(InfoScrn*);
void HSD_JObjSetSPtclCallback(HSD_Generator* (*func)(int, int, int, HSD_JObj*));
void HSD_JObjSetDPtclCallback(HSD_Generator* (*func)(int, int, int, HSD_JObj*));
HSD_Generator* LinkJointSoundGenerator(int, int, int, HSD_JObj*);
HSD_Generator* LinkJointParticleGenerator(int, int, int, HSD_JObj*);
void InitializeEventScriptContext(CEvt*, u8*, void*);
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

	InitializeEventScriptContext(&mEvt, (u8*)&mCMsgs[0], NULL);
	for (int i = 0; i < 2; ++i) // two message windows
		InitMessageWindow(&mCMsgs[i]);
	ResetNewGameState();

	StartScreenFade(&mFade, 0, 0, 0, 255, 0, 0, 0, 255, 0.0f); // open on a fade and a tile wipe
	StartScreenWipeColorTransition(&mWipeTile1, 0, 0, 0, 0, 0, 0, 0, 320, 224, 640, 448, 640, 448, 0, 0, TRUE);

	mMovie.mMovie.Init();
	mMovieRect[0] = mMovieRect[1] = mMovieRect[2] = mMovieRect[3] = NAN; // no movie rect
}

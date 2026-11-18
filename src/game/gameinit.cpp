#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/cmdline.h"
#include "game/ctoon.h"
#include "game/mm.h"
#include "game/stagedata.h"
#include "revolution/mem.h"
#include <math.h>
#else
#include "game/cgame.h"
#include "game/xobjmap.h"
#include "game/cgameutil.h"
#endif

extern "C" {
char* strtok(char* str, const char* delim);
int strcmp(const char* a, const char* b);
}

#if defined(VERSION_R24J01)
extern "C" {
u32 strlen(const char* str);
void LoadBootConfig();
void ResetLogoFadeState(Logo* logo);
void InitTitleScreen(Title* title);
void InitGameMenus(MenuEx* ex);
void InitInfoScreen(InfoScrn* scrn);
void InitMessageWindow(CMsg* msg, s32 mode);
s32 GetDefaultStageIndex();
s32 GetStageNum();
void fn_80092EC8(void);
// allocates each draw list's object table and clears it, the Wii's XObjDrawList::Init
void fn_800C5CD4(XObjDrawList* list);
}
// sorts the queued scene draws under camera, in game/wii/cgs.cpp
void fn_802AEF34(WiiGsSceneObj** head, CCamera* camera, BOOL flag);

extern MEMiHeapHead* gWiiGameHeap;
extern u8* gMovieUpdateThreadStack;
extern s32 gPointerScreenWidth, gPointerScreenHeight;
extern f32 gScreenWidthScale; // against the GameCube's 640
#endif

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

#if defined(VERSION_R24J01)
/**
 * @note Address: 0x8001BC60
 * @note Size: 0x674
 */
void CGame::Start()
{
	CBase::Start();
	mMotionControlCamera.InitCamera();
	mCameraActions.Init();
	mLightList = mLights;
	InitializeWiiSaveManager(&mSaveManager);
	mMemcardLoaded = FALSE;
	mEffects.Init();
	mLoadStep  = 1;
	mSetupStep = 0;
	_41F0      = 0;
	fn_80092EC8();
	mStage     = GetDefaultStageIndex();
	mRoomCount = GetStageNum();
	LoadBootConfig();
	mPrevStage = -1;
	// the toon ramp lives at _4308+0x18 inside the unnamed block, ahead of mTpl
	InitToonTexture((ToonTexture*)&_4308[0x18], 15, 31, 2);
	// the Wii command line keeps the font path at +0x14, where the GameCube has the crosshair
	if (strlen((const char*)&gCmdLineArgs + 0x14) != 0) {
		InitFont((const char*)&gCmdLineArgs + 0x14);
	} else {
		InitFont(NULL);
	}
	gWiiPointer.Init();
	mFade.Reset();
	mWipeCircle.Reset();
	mWipeTex.Reset();
	mWipeCustom.Reset();
	mWipeTile.Reset();
	mWipeCine.Reset();
	mRoboeyes.Reset();
	mOutline.Init(&gGame.mGs.mScreen, NULL, gScreenWidthScale);
	mBlur.Init(&gGame.mGs.mScreen, NULL, gScreenWidthScale);
	mOutline.SetLineColor(0x22, 0x09, 0x19);
	mOutline.SetColor2(0x64, 0x64, 0xC8);
	mOutline.mThick  = 2.0f;
	mOutline.mThick2 = -2.0f;
	mSubwin.Init();
	for (s32 i = 0; i < 5; i++) {
		mMirrors[i].Init();
	}
	mShadow.mTexWidth = mShadow.mTexHeight = 0x200;
	mShadow.mStrength                      = 1.0f;
	mShadow.mCamera.Init(-100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 500.0f, 1.0f, &gGame.mGs.mScreen);
	Vec interest = { 0.0f, 0.0f, 0.0f };
	mShadow.mCamera.SetEye(interest.x, 400.0f + interest.y, interest.z);
	mShadow.mCamera.SetInterest(interest);
	mShadow.mCamera.SetUp(0.0f, 0.0f, 1.0f);
	mShadow.mCamera.vf88(1.0f, 1.0f, mShadow.mTexWidth - 2, mShadow.mTexHeight - 2, 0.0f, 1.0f);
	fn_802AEF34(&gGame.mGs.mSceneHead, &mShadow.mCamera, TRUE);
	mShadow.mImage = mm::Alloc(GXGetTexBufferSize(mShadow.mTexWidth, mShadow.mTexHeight, GX_TF_I8, GX_FALSE, 0), "(shadow)");
	mShadow.mTex.Init(mShadow.mImage, mShadow.mTexWidth, mShadow.mTexHeight, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&mShadow.mTex, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
	ResetLogoFadeState(&mLogo);
	InitTitleScreen(&mTitle);
	InitGameMenus(&mMenu);
	InitInfoScreen(&mInfoScrn);
	InitWiiHomeMenu(&mHomeMenu);
	ResetScreenShake(&mQuake);
	InitializeObjectPools(&mXObjs);
	for (s32 i = 0; i < 4; i++) {
		fn_800C5CD4(&mDrawLists[i]);
	}
	InitializeEventScriptContext(&mEvt, mMsgs, NULL);
	for (s32 i = 0; i < 2; i++) {
		InitMessageWindow(&mMsgs[i], i);
	}
	((XObj*)mPointer.mCursor)->InitializeObject();
	ResetNewGameState();
	StartScreenFade(&mFade, 0, 0, 0, 0xff, 0, 0, 0, 0xff, 0.0f);
	mWipeCine.StartTransition(0, 0, 0, 0, 0, 0, 0, gPointerScreenWidth / 2, gPointerScreenHeight / 2, gPointerScreenWidth,
	                          gPointerScreenHeight, gPointerScreenWidth, gPointerScreenHeight, 0.0f, 0, TRUE);
	mMovie.Init();
	mMovieRect[0] = mMovieRect[1] = mMovieRect[2] = mMovieRect[3] = NAN;
	mMovieOnThread                                                = FALSE;
	gMovieUpdateThreadStack                                       = (u8*)MEMAllocFromExpHeapEx(gWiiGameHeap, 0x1000, 0x20);
	mMenuControls                                                 = (mMenuControls | 1) & ~2;
}
#endif

/**
 * @note Address: 0x8001357C
 * @note Size: 0x4
 */
void CScrnFade::Reset()
{
}

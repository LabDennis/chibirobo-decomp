#include "game/wii/cgame.h"
#include "game/cfile.h"
#include "game/cmdline.h"
#include "game/mm.h"
#include <Dolphin/gx.h>
#include <Dolphin/vi.h>
#include "Dolphin/OS/OSFastCast.h"

// The Wii start-up file, init.cpp by the file name its OSPanic call carries: the system, heap,
// render state and video setup, the cameras and light sets the scenes start from, and the boot
// configuration. On the GameCube the same functions are InitGameRuntime through InitTwoLightSet
// (game/gxa.cpp) and LoadBootConfig (game/progressive.cpp), linked in the reverse order.
//
// Retail's .rodata also keeps the 32 key names of a boot configuration table ("ENABLE_ATC_COPTER"
// through "EVENT_LOG", 0x802C53D4-0x802C55EC) between InitTwoLightSet's vector and the type names.
// The table itself was stripped at link time, so its entries are not known and it is not written
// here; the unit's .rodata is that much shorter than retail's.

extern "C" {
u32 SCCheckStatus();
enum {
	SC_ASPECT_RATIO_4x3,
	SC_ASPECT_RATIO_16x9,
};
u8 SCGetAspectRatio();
u8 SCGetProgressiveMode();
u32 VIGetScanMode();
void VIInit();
void DVDInit();
void DVDSetAutoFatalMessaging(BOOL enable);
void* OSGetArenaLo();
void* OSGetArenaHi();
void* OSGetMEM2ArenaLo();
void* OSGetMEM2ArenaHi();
void OSPanic(const char* file, int line, const char* msg, ...);
void KPADSetPosParam(s32, f32, f32);
void KPADSetHoriParam(s32, f32, f32);
void KPADSetDistParam(s32, f32, f32);
u32 OSGetTick();
void srand(u32);
s32 GetStageNum();
s32 GetDefaultStageIndex();

void InitGameSound(XSound* sound, u32 heapSize);

void InitGameHeaps(CBase* base);
void InitGameRenderState(CBase* base);
void InitGameVideo(CBase* base);
void InitGameScreenMetrics(CBase* base);

// the screen the game lays itself out on, in pixels
extern s32 gRingScreenWidth;
extern s32 gRingScreenHeight;
extern s32 gPointerScreenWidth;
extern s32 gPointerScreenHeight;
extern s32 gScreenLeft;
extern s32 gScreenRight;
extern s32 gScreenTop;
extern s32 gScreenBottom;
extern s32 gScreenSafeMarginX;
extern s32 gScreenSafeMarginY;
extern s32 gScreenSafeLeft;
extern s32 gScreenSafeRight;
extern s32 gScreenSafeTop;
extern s32 gScreenSafeBottom;
extern s32 gScreenSafeWidth;
extern s32 gScreenSafeHeight;
extern f32 gScreenWidthScale; // against the GameCube's 640
extern s32 gRefreshRate;
extern f32 gFrameRateScale; // 60 / gRefreshRate
}

/**
 * @note Address: 0x800354DC
 * @note Size: 0x118
 */
// what CBase::Start (0x80017D4C) runs first
extern "C" void InitGameRuntime(CBase* base)
{
	while (SCCheckStatus() != 0) { }
	OSInitFastCast();
	DVDInit();
	DVDSetAutoFatalMessaging(TRUE);
	DVDChangeDir("/cb/"); // the game's data root
	VIInit();
	InitGameHeaps(base);
	InitGameRenderState(base);
	InitGameVideo(base);
	gGame.mGs.InitFrameBuffers();
	base->mLanguage = 0;
	base->mPad.Init(1);
	KPADSetPosParam(gWiiPads[0].mPort, 0.07f, 0.8f);
	KPADSetHoriParam(gWiiPads[0].mPort, 0.05f, 1.0f);
	KPADSetDistParam(gWiiPads[0].mPort, 0.0f, 1.0f);
	InitGameSound(&base->mSound, 0xA00000);
	srand(OSGetTick());
	GXSetMisc(GX_MT_XF_FLUSH, 8);
}

/**
 * @note Address: 0x800355F4
 * @note Size: 0x98
 */
// one heap over all of each arena
extern "C" void InitGameHeaps(CBase* base)
{
	base->mMem1Base  = (void*)0x80000000;
	base->mMem1Start = OSGetArenaLo();

	u8* mem1Lo         = (u8*)OSGetArenaLo();
	u8* mem1Hi         = (u8*)OSGetArenaHi();
	base->mMem1ArenaLo = mem1Lo;
	base->mMem1ArenaHi = mem1Hi;

	u8* heapStart = (u8*)OSGetArenaLo();
	u8* heapEnd   = (u8*)OSGetArenaHi();
	mm::InitMem1(heapStart, heapEnd - heapStart);

	u8* mem2Lo         = (u8*)OSGetMEM2ArenaLo();
	u8* mem2Hi         = (u8*)OSGetMEM2ArenaHi();
	base->mMem2ArenaLo = mem2Lo;
	base->mMem2ArenaHi = mem2Hi;
	mm::InitMem2(mem2Lo, mem2Hi - mem2Lo);
	mm::Init();
}

/**
 * @note Address: 0x8003568C
 * @note Size: 0x444
 */
// NONMATCHING: retail loads the second SetChannel call's channel id, GX_ALPHA0, from the literal pool
// (0x805A2B7C, between the two calls' colors) where this build has li r5, 2. GameCube retail reads
// both channel ids from small data in the same place. The unit's later literals sit 4 bytes lower
// for it.
extern "C" void InitGameRenderState(CBase* base)
{
	gGame.mGs.Init(0x200, 0x200, 8, 8);
	gGame.mGs.SetRenderState(NULL, GSBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET),
	                         GSZMode(GX_TRUE, GX_LEQUAL, GX_TRUE), GSAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 255),
	                         GSColorUpdate(GX_TRUE), GSAlphaUpdate(GX_FALSE), GSDstAlpha(GX_FALSE, 0), GSZCompLoc(GX_TRUE),
	                         GSDither(GX_TRUE), GSLineWidth(12, GX_TO_ONE), GSPointSize(24, GX_TO_ONE), GSCullMode(GX_CULL_BACK),
	                         GSClipMode(GX_CLIP_ENABLE));
	gGame.mGs.SetTevStage(NULL, 0, GSTevOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0),
	                      GSTevColorOp(GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV),
	                      GSTevColorIn(GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO),
	                      GSTevAlphaOp(GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV),
	                      GSTevAlphaIn(GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO));
	gGame.mGs.SetChannel(NULL, GX_COLOR0, GSChanCtrl(GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_SPOT),
	                     (GXColor) { 0, 0, 0, 255 }, (GXColor) { 255, 255, 255, 255 });
	gGame.mGs.SetChannel(NULL, GX_ALPHA0, GSChanCtrl(GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE),
	                     (GXColor) { 0, 0, 0, 255 }, (GXColor) { 255, 255, 255, 255 });
	if (gGame.mGs.mCurRenderState)
		gGame.mGs.mCurRenderState->Apply();
	if (gGame.mGs.mCurTevStage)
		gGame.mGs.mCurTevStage->Apply();
	if (gGame.mGs.mCurChannel)
		gGame.mGs.mCurChannel->Apply();
	gGame.mGs._1B84 = TRUE;
}

/**
 * @note Address: 0x80035AD0
 * @note Size: 0x240
 */
// picks the screen mode from the console's TV settings
extern "C" void InitGameVideo(CBase* base)
{
	base->mAspectRatio = SCGetAspectRatio();
	GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
	switch (VIGetTvFormat()) {
	case VI_NTSC:
		base->mTvFormat = 0;
		break;
	case VI_PAL:
		base->mTvFormat = 1;
		break;
	case VI_MPAL:
		base->mTvFormat = 2;
		break;
	case VI_EURGB60:
		base->mTvFormat = 3;
		break;
	default:
		OSPanic(__FILE__, 219, "Unknown return value(%d) from VIGetTvFormat()", VIGetTvFormat());
	}
	if (SCGetProgressiveMode() == 1 && VIGetDTVStatus()) {
		base->mProgressive = TRUE;
	} else {
		base->mProgressive = FALSE;
	}

	s32 mode; // stays unset for a TV format out of range, as in retail
	switch (base->mTvFormat) {
	case 0:
		mode = base->mProgressive ? 7 : 4;
		break;
	case 1:
		mode = 27;
		break;
	case 2:
		mode = base->mProgressive ? 17 : 14;
		break;
	case 3:
		mode = base->mProgressive ? 36 : 33;
		break;
	}

	// a switch to progressive scan gets a hundred retraces to settle
	BOOL wait = FALSE;
	if (base->mProgressive && VIGetScanMode() != 2) {
		wait = TRUE;
	}
	s32 frames = wait ? 100 : 0;

	GXRenderModeObj* rmode = gGame.mGs.mScreen.GetRenderMode(mode);
	rmode->viWidth         = 686;
	rmode->viXOrigin       = 17;
	gGame.mGs.SetScreenMode(mode, 0, 12);
	InitGameScreenMetrics(base);
	gRefreshRate    = base->mTvFormat == 1 ? 50 : 60;
	gFrameRateScale = 60.0f / gRefreshRate;
	while (frames-- != 0) {
		VIWaitForRetrace();
	}
}

/**
 * @note Address: 0x80035D10
 * @note Size: 0xB8
 */
extern "C" void InitGameScreenMetrics(CBase* base)
{
	gRingScreenWidth     = base->mAspectRatio == SC_ASPECT_RATIO_4x3 ? 640 : 854;
	gRingScreenHeight    = gGame.mGs.mScreen.efbHeight + 24;
	gPointerScreenWidth  = gRingScreenWidth;
	gPointerScreenHeight = gRingScreenHeight - 24;
	gScreenLeft          = 0;
	gScreenRight         = gPointerScreenWidth;
	gScreenTop           = 0;
	gScreenBottom        = gPointerScreenHeight;
	gScreenSafeMarginX   = 39;
	gScreenSafeMarginY   = 34;
	gScreenSafeLeft      = gScreenLeft + gScreenSafeMarginX;
	gScreenSafeRight     = gScreenRight - gScreenSafeMarginX;
	gScreenSafeTop       = gScreenTop + gScreenSafeMarginY;
	gScreenSafeBottom    = gScreenBottom - gScreenSafeMarginY;
	gScreenSafeWidth     = gScreenSafeRight - gScreenSafeLeft;
	gScreenSafeHeight    = gScreenSafeBottom - gScreenSafeTop;
	gScreenWidthScale    = gRingScreenWidth / 640.0f;
}

/**
 * @note Address: 0x80035DC8
 * @note Size: 0xD0
 */
extern "C" CCamera* InitGamePerspectiveCamera(CCamera* camera, const GXRenderModeObj* rmode)
{
	camera->Init(1.0f, 80000.0f, 30.0f, gScreenWidthScale, rmode);
	camera->SetInterest(-4.2f, 0.0f, -64.0f);
	camera->SetAngle(0.41887903f, -3.1415927f, 0.0f); // pitched 24 degrees, yawed -180
	camera->SetDistance(400.0f);
	camera->SetFov(18.0f);
	camera->CalcEye();
	return camera;
}

/**
 * @note Address: 0x80035EB0
 * @note Size: 0xDC
 */
extern "C" CCamera* InitMotionControlCameraDefaults(CCamera* camera)
{
	camera->Init(1.0f, 80000.0f, 30.0f, gScreenWidthScale, &gGame.mGs.mScreen);
	camera->SetInterest(-4.2f, 0.0f, -64.0f);
	camera->SetAngle(0.41887903f, -3.1415927f, 0.0f);
	camera->SetDistance(400.0f);
	camera->SetFov(18.0f);
	camera->CalcEye();
	return camera;
}

/**
 * @note Address: 0x80035F8C
 * @note Size: 0xF0
 */
// the screen-space camera: a pixel is a unit, the origin is the top left corner
extern "C" CCamera* InitGameOrthoCamera(CCamera* camera, const GXRenderModeObj* rmode)
{
	s32 height = rmode->efbHeight;
	s32 width  = rmode->fbWidth;
	f32 scale  = gScreenWidthScale;
	camera->Init(0.0f, height, 0.0f, width, -200.0f, 200.0f, scale, rmode);
	camera->SetEye(0.0f, 0.0f, 0.0f);
	camera->SetInterest(0.0f, 0.0f, -100.0f);
	camera->CalcAngle();
	camera->CalcDistance();
	return camera;
}

/**
 * @note Address: 0x8003608C
 * @note Size: 0x1C8
 */
extern "C" CLight* InitFourLightSet(CLight* lights)
{
	lights[0].InitAmbient(200, 200, 200, 255);
	lights[1].InitInfinite((Vec) { -1.0f, 10.0f, 1.0f }, 255, 255, 255, 255);

	CLight* light = &lights[2];
	light->InitPoint((Vec) { 0.0f, 20.0f, -20.0f }, 0, 255, 255, 255);
	light->SetPosition(0.0f, 20.0f, -20.0f);
	light->vf30();

	light = &lights[3];
	light->InitSpot((Vec) { 0.0f, 20.0f, 0.0f }, (Vec) { 0.0f, -1.0f, 1.0f }, 255, 255, 0, 255);
	light->vf30();

	lights[0].mNext = &lights[1];
	lights[1].mNext = &lights[2];
	lights[2].mNext = &lights[3];
	lights[3].mNext = NULL;
	return lights;
}

/**
 * @note Address: 0x80036274
 * @note Size: 0xA0
 */
extern "C" CLight* InitTwoLightSet(CLight* lights)
{
	lights[0].InitAmbient(200, 200, 200, 255);
	lights[1].InitInfinite((Vec) { 1.0f, -1.0f, 1.0f }, 255, 255, 255, 255);

	lights[0].mNext = &lights[1];
	lights[1].mNext = NULL;
	return lights;
}

/**
 * @note Address: 0x80036314
 * @note Size: 0x50
 */
// no fog
extern "C" CFog* InitGameFog(CFog* fog)
{
	GXColor color = { 0, 0, 0, 0 };
	fog->Init(GX_FOG_NONE, color, 0.0f, 0.0f, 0.0f, 0.0f);
	return fog;
}

/**
 * @note Address: 0x80036364
 * @note Size: 0x234
 */
// the GameCube version read these from /qp.ini; here only the defaults and the stage are left
extern "C" void LoadBootConfig()
{
	gGame.SetStatusFlags(1ULL << 34); // logo, title, game over, rel, archiver
	gGame.SetStatusFlags(1ULL << 29);
	gGame.SetStatusFlags(1ULL << 30);
	gGame.SetStatusFlags(1ULL << 32);
	gGame.SetStatusFlags(1ULL << 33);
	gGame.SetStatusFlags(1ULL << 35);
	gGame.SetStatusFlags(1ULL << 14);
	gGame.SetStatusFlags(1ULL << 16);
	gGame.SetStatusFlags(1ULL << 38);
	gGame.SetStatusFlags(1ULL << 40);
	gGame.SetStatusFlags(1ULL << 47);
	gGame.SetStatusFlags(1ULL << 48);
	gGame.SetStatusFlags(1ULL << 49);
	gGame.SetStatusFlags(1ULL << 54);

	CFileText file;
	file.mPos = 0;
	s32 stage = gCmdLineArgs.mStage >= 0 ? gCmdLineArgs.mStage : GetDefaultStageIndex();
	if (stage >= 100) { // 100+ indexes the rel stages
		stage += GetStageNum() - 99;
	}
	gCmdLineArgs.mStage = stage;
	gGame.SetStage(stage);
}

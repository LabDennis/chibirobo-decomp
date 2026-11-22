#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/csound.h"
#include "game/mm.h"
#include "game/progressive.h"
#include "game/video.h"
#include "game/cgsdefaults.h"
#include "game/vistate.h"

#include "Dolphin/OS/OSFastCast.h"

extern "C" {
#include <dolphin/gx/gx.h>
#include <sysdolphin/initialize.h>
extern GXRenderModeObj GXPal528IntDf;
void HSD_SetPixelFormat(s32, s32);
void HSD_SetZSortMode(s32, s32);
void GXSetClipMode(s32);
}

// the game core's start-up pieces: the light sets and cameras the scenes start from, the
// system setup and the anim sfx

// the light sets and cameras the scenes start from. each one sets up the objects it gets and
// hands them back

/**
 * @note Address: 0x8001CEFC
 * @note Size: 0xD4
 */
extern "C" CLight* InitTwoLightSet(CLight* lights)
{
	CLight* light;

	lights[0].Init(LOBJ_AMBIENT | LOBJ_DIFFUSE, FALSE);
	lights[0].mColor.r = 0x80;
	lights[0].mColor.g = 0x80;
	lights[0].mColor.b = 0x80;
	lights[0].mColor.a = 0xFF;
	HSD_LObjSetColor(&lights[0], lights[0].mColor);

	lights[0].next = &lights[1];
	light          = &lights[1];
	light->Init(LOBJ_INFINITE | LOBJ_DIFFUSE, FALSE);
	light->SetPosition(1.0f, -1.0f, 1.0f);
	light->mColor.r = 0xFF;
	light->mColor.g = 0xFF;
	light->mColor.b = 0xFF;
	light->mColor.a = 0xFF;
	HSD_LObjSetColor(light, light->mColor);
	light->next = NULL;
	return lights;
}

/**
 * @note Address: 0x8001CFD0
 * @note Size: 0x1D0
 */
extern "C" CLight* InitFourLightSet(CLight* lights)
{
	CLight* light;

	lights[0].Init(LOBJ_AMBIENT | LOBJ_DIFFUSE, FALSE);
	lights[0].mColor.r = 200;
	lights[0].mColor.g = 200;
	lights[0].mColor.b = 200;
	lights[0].mColor.a = 0xFF;
	HSD_LObjSetColor(&lights[0], lights[0].mColor);

	lights[0].next = &lights[1];
	light          = &lights[1];
	light->Init(LOBJ_INFINITE | LOBJ_DIFFUSE, TRUE);
	light->SetPosition(-2.0f, 20.0f, 2.0f);
	light->mColor.r = 200;
	light->mColor.g = 200;
	light->mColor.b = 200;
	light->mColor.a = 0xFF;
	HSD_LObjSetColor(light, light->mColor);

	light->next = NULL;

	light = &lights[2];
	light->Init(LOBJ_POINT | LOBJ_DIFFUSE, FALSE);
	light->SetPosition(0.0f, 20.0f, -20.0f);
	light->mColor.r = 0;
	light->mColor.g = 0xFF;
	light->mColor.b = 0xFF;
	light->mColor.a = 0xFF;
	HSD_LObjSetColor(light, light->mColor);

	light->next = NULL;

	light = &lights[3];
	light->Init(LOBJ_SPOT | LOBJ_DIFFUSE, FALSE);
	light->SetPosition(0.0f, 20.0f, 0.0f);
	light->SetInterest(0.0f, 0.0f, 20.0f);
	light->mColor.r = 0xFF;
	light->mColor.g = 0;
	light->mColor.b = 0;
	light->mColor.a = 0xFF;
	HSD_LObjSetColor(light, light->mColor);
	light->next = NULL;
	return lights;
}

/**
 * @note Address: 0x8001D1D0
 * @note Size: 0xCC
 */
extern "C" CCamera* InitGameOrthoCamera(CCamera* camera, GXRenderModeObj* rmode)
{
	camera->Init(0.0f, 10000.0f, rmode);
#if defined(VERSION_PAL)
	camera->SetFrustum(0.0f, 448.0f, 0.0f, 640.0f);
#endif
	camera->SetScale(1.0f, 1.0f, 1.0f);
	camera->SetEye(0.0f, 0.0f, 5000.0f);
	camera->SetInterest(0.0f, 0.0f, 0.0f);
	camera->CalcAngle();
	camera->CalcDistance();
	return camera;
}

/**
 * @note Address: 0x8001D2D0
 * @note Size: 0xF4
 */
extern "C" CCamera* InitMotionControlCameraDefaults(CCamera* camera)
{
	camera->Init(1.0f, 80000.0f, 30.0f, &gGame.mGs.mScreen);
	camera->SetScale(1.0f, 1.0f, 1.0f);
	camera->SetInterest(-4.2f, 0.0f, -64.0f);
	camera->SetAngle(0.41887903f, -3.1415927f, 0.0f); // pitched 24 degrees, yawed -180
	camera->SetDistance(400.0f);
	camera->SetFov(18.0f);
	camera->CalcEye();
	return camera;
}

/**
 * @note Address: 0x8001D3D4
 * @note Size: 0xEC
 */
extern "C" CCamera* InitGamePerspectiveCamera(CCamera* camera, GXRenderModeObj* rmode)
{
	camera->Init(1.0f, 80000.0f, 30.0f, rmode);
	camera->SetScale(1.0f, 1.0f, 1.0f);
	camera->SetInterest(-4.2f, 0.0f, -64.0f);
	camera->SetAngle(0.41887903f, -3.1415927f, 0.0f);
	camera->SetDistance(400.0f);
	camera->SetFov(18.0f);
	camera->CalcEye();
	return camera;
}

// the system setup the game runs before anything else

extern "C" {
extern GXRenderModeObj GXPal528IntDf;
void srand(u32 seed);
u32 OSGetTick(void);
void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps);
u32 OSGetResetCode(void);
void OSSetSaveRegion(void* start, void* end);
void DVDSetAutoFatalMessaging(BOOL enable);
BOOL DVDChangeDir(char* dirName);
u32 ARInit(u32* stackIndexAddr, u32 numEntries);
void ARQInit(void);
void HSD_VIDrawDoneXFB(s32 idx);
}

/**
 * @note Address: 0x8001D4C0
 * @note Size: 0x2C
 */
extern "C" void InitGameDVD(void)
{
	DVDSetAutoFatalMessaging(TRUE);
	DVDChangeDir((char*)"/qp/"); // the game's data root
}

// ARInit's allocation stack
u32 gAramBlockLengths[8];

/**
 * @note Address: 0x8001D4EC
 * @note Size: 0x30
 */
extern "C" void InitGameARAM(void)
{
	ARInit(gAramBlockLengths, 8);
	ARQInit();
}

extern "C" void GameXFBDrawDoneCallback(int idx);

/**
 * @note Address: 0x8001D51C
 * @note Size: 0x384
 */
// NONMATCHING: retail loads the scalar render-mode arguments (color and alpha update, z before
// texture, dither, cull) from one-object .sdata2/.sbss2 constants where this build has li, and its
// struct arguments' .rodata lies in another order behind the unit's 0x48-byte header
extern "C" void InitGameRenderState(CBase* base)
{
	CGs* gs = &base->mGs;
#if defined(VERSION_PAL)
	SetGsScreenModeAndRenderDefaults(gs, 21, 0, 0, 0);
	gs->mScreen.mHeightScale = gGame.mGs.mScreen.xfbHeight / 448.0f;
	gs->mScreen.viHeight     = 480;
#else
	SetGsScreenModeAndRenderDefaults(gs, 4, 0, 16, 0);
#endif
#if !defined(VERSION_JP)
	SetGsFixedGlyphWidthScale(gs, 1.2f);
#endif
	HSD_SetPixelFormat(1, 0);
	GXSetPixelFmt(1, 0);
	GXSetCopyFilter(0, NULL, 0, NULL);

	InitializeVideoClearState((GXColor) { 0, 0, 0, 0 });
	GXSetClipMode(0);
	HSD_SetZSortMode(1, 1);

	ConfigureGsRenderMode(gs, NULL, GX_TRUE, GX_FALSE, (GsBlendMode) { 1, 4, 5, 15 }, (GsAlphaCompare) { 4, 0, 0, 3, 255 },
	                      (GsDstAlpha) { 0, 0 }, (GsZMode) { 1, 7, 1 }, GX_TRUE, GX_TRUE, 2, (GsLineWidth) { 12, 5 },
	                      (GsLineWidth) { 24, 5 });
	ConfigureGsPrimaryTevStage(gs);
	ConfigureGsChannel(gs, NULL, GX_COLOR0, (GsChanCtrl) { GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, 2, 2 }, (GXColor) { 0, 0, 0, 255 },
	                   (GXColor) { 255, 255, 255, 255 });
	ConfigureGsChannel(gs, NULL, GX_ALPHA0, (GsChanCtrl) { GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, 0, 2 }, (GXColor) { 0, 0, 0, 255 },
	                   (GXColor) { 255, 255, 255, 255 });
	if (gs->mCurRenderMode)
		ApplyGsRenderMode(gs->mCurRenderMode, TRUE);
	if (gs->mCurTevStage)
		ApplyGsTevStage(gs->mCurTevStage, TRUE);
	if (gs->mCurChannel)
		ApplyGsChannel(gs->mCurChannel, TRUE);
	HSD_VISetUserGXDrawDoneCallback(GameXFBDrawDoneCallback);
}

/**
 * @note Address: 0x8001D8A0
 * @note Size: 0x20
 */
extern "C" void GameXFBDrawDoneCallback(int idx)
{
	HSD_VIDrawDoneXFB(idx);
}

// the two words the game keeps over a reset
extern "C" {
SavedVideoMode* gVideoModeResetState;
}

/**
 * @note Address: 0x8001D8C0
 * @note Size: 0xE8
 */
extern "C" void InitGameRuntime(void)
{
	OSInitFastCast();
	srand(OSGetTick());
	void* lo = OSGetArenaLo();
	void* hi = OSGetArenaHi();
	OSSetArenaLo(OSInitAlloc(lo, hi, 1));
	HSD_SetInitParameter(HSD_INIT_FIFO_SIZE, 0x40000);
	HSD_SetInitParameter(HSD_INIT_XFB_MAX_NUM, 3);
#if defined(VERSION_PAL)
	HSD_SetInitParameter(HSD_INIT_RENDER_MODE_OBJ, &GXPal528IntDf);
#endif
	HSD_Init();
	mm::Init();
	gVideoModeResetState = (SavedVideoMode*)OSAllocFromHeap(__OSCurrHeap, sizeof(SavedVideoMode));
	if (!(OSGetResetCode() >> 31)) { // a set sign bit keeps what was saved

		gVideoModeResetState->hasSelection    = 0;
		gVideoModeResetState->useStandardMode = 1;
	}
	OSSetSaveRegion(gVideoModeResetState, gVideoModeResetState + 1);
}

#if defined(VERSION_PAL)
f32 gProgressiveFadeFrames = 0.15f * FRAME_RATE;
#endif

/**
 * @note Address: 0x8001D9A8
 * @note Size: 0x8
 */
// CCameraActionMap's GetTarget
XObj* CCameraActionMap::GetTarget()
{
	return mTarget;
}

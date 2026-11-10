#include "game/cgame.h"
#include <Dolphin/vi.h>
#include "game/cgameutil.h"

extern "C" {
void HSD_VISetBlack(u8 black);
void HSD_VICopyXFBAsync(s32 pri);
void OSInitStopwatch(void* sw, char* name);
void OSResetStopwatch(void* sw);
void OSStartStopwatch(void* sw);
void OSStopStopwatch(void* sw);
s64 OSCheckStopwatch(void* sw);
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
extern "C" BOOL BeforeUpdate__5CBaseFv(CBase* base)
{
	return TRUE;
}

/**
 * @note Address: 0x80015534
 * @note Size: 0x100
 */
// CBase's Start
extern "C" void Start__5CBaseFv(CBase* base)
{
	InitGameRuntime();
	InitGameRenderState(base);
	InitGameARAM();
	InitGameDVD();
#if defined(VERSION_PAL)
	u8 language = OSGetLanguage();
	for (s32 i = 0; i < 6; ++i) {
		if (language == gGameLanguageMap[i * 2]) {
			base->mLanguage = gGameLanguageMap[i * 2 + 1];
			break;
		}
	}
#elif !defined(VERSION_JP)
	base->mLanguage = 0;
#endif
#if defined(VERSION_JP)
	Init__5CFontFv(&base->mFont);
#else
	Init__5CFontFi(&base->mFont, gGame.mLanguage != 6);
#endif
	base->mPad.Init();
	base->mLastRetrace   = 0;
	base->mRetraceStep   = 0;
	base->mStatus.mFlags = 0;
	InitGamePerspectiveCamera(&base->mCameras[0], &base->mGs.mScreen);
	InitGameOrthoCamera(&base->mCameras[1], &base->mGs.mScreen);
	InitFourLightSet(&base->mLights[0]);
	InitTwoLightSet(&base->mLights[4]);
	OSInitStopwatch(&base->mStopwatch[0], "__$dsync$ ");
	OSInitStopwatch(&base->mStopwatch[1], "__$update$");
	OSInitStopwatch(&base->mStopwatch[2], "__$draw$  ");
	base->mPrevDvdStatus = 0;
	base->mDvdStatus     = 0;
	base->mDiskOk        = 1;
	base->mDvdError      = 0;
	base->mResetHold     = 0;
	base->mReset         = 0;
}

/**
 * @note Address: 0x800153B4
 * @note Size: 0x4
 */
extern "C" void vf24__5CBaseFv(CBase* base)
{
}

/**
 * @note Address: 0x800153B8
 * @note Size: 0x4
 */
extern "C" void vf20__5CBaseFv(CBase* base)
{
}

/**
 * @note Address: 0x80015634
 * @note Size: 0x4
 */
extern "C" void Initialize__5CBaseFiPPc(CBase* base, int argc, char** argv)
{
}

/**
 * @note Address: 0x80015638
 * @note Size: 0x8
 */
extern "C" s32 FrameInterval__5CBaseFv(CBase* base)
{
	return 1;
}

/**
 * @note Address: 0x80015640
 * @note Size: 0x4
 */
extern "C" void Draw__5CBaseFv(CBase* base)
{
}

extern "C" {
void AXQuit(void);
void AIReset(void);
void AIStopDMA(void);
void* HSD_VISetUserGXDrawDoneCallback(void* cb);
void GXDrawDone(void);
void GXAbortFrame(void);
void OSResetSystem(int reset, u32 code, BOOL forceMenu);
}

/**
 * @note Address: 0x800153BC
 * @note Size: 0x170
 */
extern "C" void Update__5CBaseFUl(CBase* base, u32 frames)
{
	BaseUpdate(base, frames);
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
extern "C" void Reset__5CBaseFUl(CBase* base, u32 code)
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
	HSD_VISetBlack(0);
	while (true) {
		VIWaitForRetrace();
		gGame.mRetraceStep = VIGetRetraceCount() - gGame.mLastRetrace;
		gGame.mLastRetrace = VIGetRetraceCount();
		if (gGame.mRetraceStep > 4) {
			gGame.mRetraceStep = 4;
		}
		OSResetStopwatch(&gGame.mStopwatch[0]);
		OSStartStopwatch(&gGame.mStopwatch[0]);
		OSResetStopwatch(&gGame.mStopwatch[1]);
		OSStartStopwatch(&gGame.mStopwatch[1]);
		OSResetStopwatch(&gGame.mStopwatch[2]);
		OSStartStopwatch(&gGame.mStopwatch[2]);
		HSD_VICopyXFBAsync(0);
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
		while (VIGetRetraceCount() - gGame.mLastRetrace < wait) { }
#endif
	}
}

/**
 * @note Address: 0x800151DC
 * @note Size: 0x4
 */
extern "C" void Reset__8FbMirrorFv(void)
{
}

/**
 * @note Address: 0x80014DA4
 * @note Size: 0x2BC
 */
// NONMATCHING: the surface check is a compare where the original tests a bool
// FbMirror::Update, puts the mirror camera at the main camera's eye reflected in the surface,
// looking at the surface through a frustum that just fits it
extern "C" void Update__8FbMirrorFUl(FbMirror* mirror, u32 frames)
{
	HSD_JObj* jobj = mirror->mJoint;
	if (jobj != NULL) {
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
		PSVECScale(&x, &x, -1.0f);
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
		if (dist <= 0.0f) {
			mirror->mVisible = FALSE;
		} else {
			PSVECScale(&normal, &diff, 2.0f * dist);
			PSVECAdd(&diff, &eye, &diff);
			mtx[0][3] = diff.x;
			mtx[1][3] = diff.y;
			mtx[2][3] = diff.z;
			PSMTXInverse(mtx, view);
			PSMTXMultVec(view, &pos, &pos);
			mirror->mCamera.SetNear(0.1f - pos.z);
			mirror->mCamera.SetFar(32768.0f + (0.1f - pos.z));
			mirror->mCamera.SetFrustum(0.5f + pos.y, -0.5f + pos.y, -0.5f + pos.x, 0.5f + pos.x);
			mirror->mCamera.SetEye(diff);
			mirror->mCamera.SetMtx(view);
			PSMTXCopy(view, mirror->mView);
			mirror->mVisible = TRUE;
		}
	}
}

/**
 * @note Address: 0x800151E0
 * @note Size: 0xC0
 */
// FbMirror::Init
extern "C" void Init__8FbMirrorFv(FbMirror* mirror)
{
	mirror->mJoint = NULL;
	mirror->mCamera.Init(0.1f, 32768.0f, 0.5f, -0.5f, -0.5f, 0.5f, &gGame.mGs.mScreen);
	mirror->mCamera.SetViewport(0.0f, 256.0f, 0.0f, 256.0f);
	mirror->mCamera.SetScissor(0, 256, 0, 256);
	mirror->Reset();
}

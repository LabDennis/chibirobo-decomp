#include "game/wii/cgame.h"
#include "game/cthread.h"
#include "game/wii/xobj.h"
#include "Dolphin/os.h"
#include "Dolphin/vi.h"
#include <string.h>

extern "C" {
void* UpdateMovie(void* arg);
char* FormatToStaticBuffer(const char* fmt, ...);
const char* GetLocalizedInterfaceText(s32 id);
extern CThread gMovieUpdateThread;
extern u8* gMovieUpdateThreadStack;
s32 CallStageModule(s32 function, s32 count, s32* args);
void fn_802AEDEC(WiiGsSceneObj** head, Mtx view); // sorts the queued scene draws under view, as title.cpp calls it
}

extern s32 gRefreshRate;
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
 * @note Address: 0x8002B2EC
 * @note Size: 0x4
 */
void XObj::vf210()
{
}

/**
 * @note Address: 0x8002B2E8
 * @note Size: 0x4
 */
void XObj::OnMicrophonePeak(s32, s32)
{
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
	GXProject(pos->x, pos->y, pos->z, cam->mViewMtx, cam->mProjectionv, cam->mViewport, &sx, &screen.y, &screen.z);
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
	return (CLight*)mLights[8];
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
 * @note Address: 0x80027D44
 * @note Size: 0x4
 */
void XObj::DrawObjectEffects()
{
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
const Vec* XObj::GetPosition()
{
	return &mMoveTranslate;
}

/**
 * @note Address: 0x80024CA0
 * @note Size: 0x8
 */
void CBase::vf54()
{
	mGs.EndCamera();
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
// NONMATCHING: retail has nw4r::math::VEC3Add's paired-single code inline here; the nw4r math headers
// do not compile under the game's include paths, so this calls the SDK function as the GameCube did
CVec CVec::operator+(Vec v) const
{
	CVec sum = *this;
	PSVECAdd(&sum, &v, &sum);
	return sum;
}

/**
 * @note Address: 0x80024AE8
 * @note Size: 0x64
 */
// NONMATCHING: nw4r::math::VEC3Scale inline in retail, see operator+
CVec CVec::operator*(f32 s) const
{
	CVec product = *this;
	PSVECScale(&product, &product, s);
	return product;
}

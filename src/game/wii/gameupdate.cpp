#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h"
#include "nw4r/g3d/g3d_camera.h"
#include "game/wii/xobjflags.h"
#include "game/wii/homemenu.h"
#include "game/wii/saveslots.h"
#include "game/wii/xchrmenu.h"
#include "game/globals.h"
#include "arithmetic.h"
#include "game/attachmentvalues.h"
#include "game/cscrn.h"
#include "game/evt.h"
#include "game/gmath.h"
#include "revolution/sc.h"
#include "nw4r/snd/snd_AxManager.h"
#include "nw4r/math/math_types.h"
#include "game/wii/cgame.h"
#include "game/cthread.h"
#include "game/wii/xobj.h"
#include "game/wii/xobjleticker.h"
#include "Dolphin/os.h"
#include "Dolphin/vi.h"
#include <string.h>
#include "nw4r/snd/snd_BasicSound.h"
#include "nw4r/snd/snd_SoundHandle.h"
#include "game/wii/csound3dactor.h"
#include "game/wii/animsfx.h"
#include "game/wii/cpadact.h"
#include "game/wii/cursor.h"
#include "game/wii/infoscrn.h"
#include "game/wii/msg.h"
#include "game/wii/fbeffect.h"
#include "game/fbtexeff.h"
#include "game/cmdline.h"
#include "game/cvec.h"
#include "game/mm.h"
#include "game/cfont.h"
#include "game/stagetable.h"
#include "game/stagedata.h"
#include <math.h>
#include <stdlib.h>

extern "C" {
void ChangeGameStage(CGame* game, s32 stage, BOOL showName);
void* UpdateMovie(void* arg);
char* FormatToStaticBuffer(const char* fmt, ...);
const char* GetLocalizedInterfaceText(s32 id);
extern CThread gMovieUpdateThread;
extern u8* gMovieUpdateThreadStack;
s32 CallStageModule(s32 function, s32 count, s32* args);
void fn_802AEDEC(WiiGsSceneObj** head, Mtx view); // sorts the queued scene draws under view, as title.cpp calls it
void ClearMainMenuExtraRows(void* menu);
void ResetMsgState(CMsg* msg);
void UnloadStageDataAndModule();
void ClearSoundEffects(XSound* sound, s32 frames);
s32 GetWiiSaveSlotStage(CSaveFile* manager, int index);
void ResetTitleScreenState(Title* title);
void ResetMenuGroup(void* menus); // the GameCube's ResetMenuGroup
void fn_80084BF0(void* menus, s32 menu);
BOOL HasPendingObjectReads();
void fn_802AEF34(WiiGsSceneObj** head, CCamera* camera, BOOL); // sorts the queued scene draws under camera
void BuildSceneDrawLists(CGame* game);
// draws one group of the effects
void DrawTitleScreen(Title* title);
void DrawGameHud(InfoScrn* scrn);
void DrawScreenColorOverlay(CScrnColor* color, CGs* gs);
void DrawPlayerEquipmentRings(XObjChrMenu* rings);
void DrawOXMenu(WiiOXMenu* menu);
void CMsg_Draw(CMsg* msg);
void fn_80084C84(void* menus);
void DrawObjectLoadingProgress(InfoScrn* scrn);
void DrawOpenMenus(void* menus);
void fn_80028998(CGame* game);
void ClearActiveObjectFloors(XObj* obj);
void ResetGameHudState(InfoScrn* scrn);
u8* GetStageDisplayName(s32 index);
extern GXColor gMessageFontDefaultColor;
s32 GetStageNum();
s32 GetDefaultStageIndex();
}

BOOL fn_80169658(CMotionControlCamera* camera);
CCameraActionPointer* fn_80169674();
extern s32 lbl_802E5F18[];

extern s32 gRefreshRate;
// the stage name's four shadow copies, one pixel off to each corner
f32 sStageNameShadowX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 sStageNameShadowY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
extern f32 gScreenWidthScale;
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

inline void CSound::StopAllTracks(f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (int i = 0; i < 0x40; ++i) {
		if (mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()) {
			mArchivePlayer.mTracks.mHandles[i].Stop(frames);
		}
	}
}

inline void CSound::PauseAllTracks(f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (int i = 0; i < 0x40; ++i) {
		if (mArchivePlayer.mTracks.mHandles[i].IsAttachedSound() && !mArchivePlayer.mTracks.mHandles[i].IsPause()) {
			mArchivePlayer.mTracks.mHandles[i].nw4r::snd::SoundHandle::Pause(true, frames);
		}
	}
}

inline void CSound::ResumeAllTracks(f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (int i = 0; i < 0x40; ++i) {
		if (mArchivePlayer.mTracks.mHandles[i].IsAttachedSound() && mArchivePlayer.mTracks.mHandles[i].IsPause()) {
			mArchivePlayer.mTracks.mHandles[i].nw4r::snd::SoundHandle::Pause(false, frames);
		}
	}
}

// the head of the stage's info record, as far as this file reads it: the GameCube's UnkStageInfo
// (game/cgameutil.h, which the Wii cannot include). todo: one definition with gameupdate.cpp's
struct WiiStageInfo {
	u8 _00[0x6];
	s16 mCameraMode; // _06
	u8 mMirrorFade;  // _08
	u8 _09[0x7];
	u8 mFlags; // _10, bit 0 keeps the shadows at full strength
};

static inline WiiStageInfo* GetStageInfo()
{
	return *(WiiStageInfo**)GetStageEnvironment();
}

// the view the effects draw under: the scene camera's animated camera when its scene has one, else the
// scene camera's own view
static inline MtxP GetEffectView(CGame* game)
{
	XObjScnCam* camera = (XObjScnCam*)game->mXObjs.mPools[0].mObjects;
	bool scene         = false;
	if (camera != NULL && (camera->mStatus.mFlags & 1) == 1 && camera->mScene != NULL && camera->mScene->_2888 != NULL) {
		scene = true;
	}
	if (scene) {
		MtxP view = ((nw4r::g3d::ScnRoot*)camera->mScene)->GetCurrentCamera();
		if (view == NULL) {
			nw4r::db::Panic("g3d_rescommon_ac.h", 140, "NW4R:Pointer must not be NULL (mpData)");
		}
		return view;
	}
	return game->GetCamera()->mViewMtx;
}

// the queued scene draws, sorted under view and popped one by one
static inline void DrawSceneQueue(MtxP view)
{
	fn_802AEDEC(&gGame.mGs.mScene.mHead, view);
	WiiGsSceneObj* obj = gGame.mGs.mScene.mHead;
	while (obj->mPending != 0) {
		if (obj->mPending != 0) {
			obj->vf038(obj->mPending - 1);
		}
	}
}

// the first track playing sound id, as retail expands it ahead of the jingle loops
static inline CSoundHandle* FindTrack(u32 id)
{
	for (int i = 0; i < 0x40; i++) {
		if (gGame.mSound.mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()
		    && gGame.mSound.mArchivePlayer.mTracks.mHandles[i].GetId() == id) {
			return &gGame.mSound.mArchivePlayer.mTracks.mHandles[i];
		}
	}
	return NULL;
}

// whether the first track playing sound id is paused
static inline bool IsTrackPaused(u32 id)
{
	for (int i = 0; i < 0x40; i++) {
		if (gGame.mSound.mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()
		    && gGame.mSound.mArchivePlayer.mTracks.mHandles[i].GetId() == id) {
			return gGame.mSound.mArchivePlayer.mTracks.mHandles[i].IsPause();
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
	return (IsLogoActive() || IsTitleActive() || mMenu.mActive || mHomeMenu.IsOpen()) ? 1 : 2;
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

// NONMATCHING: the Wii pointer and scope draw has retail's texture copies and five cases;
// texture-copy scheduling and register/temporary placement account for the remaining diff.
/**
 * @note Address: 0x80028998
 * @note Size: 0x145C
 */
extern "C" void fn_80028998(CGame* game)
{
	if (!game->mTpl.mBound) {
		return;
	}
	if (game->IsDiscError()) {
		return;
	}
	if (game->mHomeMenu.IsActive()) {
		return;
	}
	if (game->GetStatusFlags(0x400000000ULL) && game->mLogo.mState != 12) {
		return;
	}
	if (game->mLoadStep < 2) {
		return;
	}
	if (!gWiiPads[0].IsRemoteActive() && game->mPointer._120 < 0.0f) {
		return;
	}

	f32 pulse = 1.0f;
	if (game->mPointer.mScopeTimer > 0.0f) {
		if (game->mPointer.mScopeTimer >= 5.0f) {
			pulse = 0.0f;
		} else {
			pulse = sin(HALF_PI * (1.0f - (f32)fmod(game->mPointer.mScopeTimer, 1.0)));
		}
	}
	f32 alpha = pulse * game->mPointer.mAlpha;
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);

	f32 x            = gWiiPointer.mPointer.x;
	f32 y            = gWiiPointer.mPointer.y;
	f32 pointerAngle = gWiiPointer.mPointerAngle;
	f32 scale        = 1.0f;
	alpha *= gWiiPointer.mPointerOpacity;
	u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
	if (game->mPointer._120 >= 0.0f) {
		x += (game->mPointer._114 - x) * (f32)sin(HALF_PI * game->mPointer._120);
		y += (game->mPointer._118 - y) * (f32)sin(HALF_PI * game->mPointer._120);
		pointerAngle = 0.0f;
		if (alpha < game->mPointer._120) {
			alpha = 1.0f;
			color = (u32)(255.0f * alpha) | 0xFFFFFF00;
		}
	}

	s32 mode       = -1;
	Cursor* cursor = (Cursor*)game->mPointer.mCursor;
	if (game->IsLoadStepRunning() && game->mLoadStep > 20) {
		if (cursor->mStatus.mFlags & 1) {
			cursor->DrawObject();
		}
	} else if (game->GetStatusFlags(0x0200000000000002ULL) && !game->mMotionControlCamera.IsAction(CAMACTION_SCROLL2)) {
		mode = 0;
	} else if (!game->GetStatusFlags(1) || game->mEvt.mData.mScript != NULL || game->mMenu.mActive || game->GetStatusFlags(0x400)
	           || game->GetStatusFlags(0x800) || (game->mStage == 3 && CallStageModule(0x10, 0, NULL))) {
		mode = 0;
	} else if (game->mMotionControlCamera.IsAction(CAMACTION_SCROLL2)) {
		mode = 3;
	} else {
		WiiPlayerState* player = game->GetPlayer();
		s32 attachment         = player->mAttachmentRequest >= 0 ? player->mAttachmentRequest : player->mAttachment;
		if (attachment == 2) {
			mode = 4;
		} else if (game->mMotionControlCamera.IsAction(CAMACTION_EYE)
		           || (game->mMotionControlCamera.IsAction(CAMACTION_AREA)
		               && ((CCameraActionArea*)game->mCameraActions.mTable[CAMACTION_AREA])->_2D0
		               && ((CCameraActionArea*)game->mCameraActions.mTable[CAMACTION_AREA])->mControlEnabled != 0)
		           || (game->mMotionControlCamera.IsAction(CAMACTION_MAP)
		               && ((CCameraActionMap*)game->mCameraActions.mTable[CAMACTION_MAP])->mPointerHeld)
		           || (fn_80169658(&game->mMotionControlCamera) && fn_80169674()->mPointerHeld)) {
			mode = 2;
		} else {
			mode = 0;
		}
	}

	CTexObj tex;
	f32 width;
	f32 height;
	switch (mode) {
	case 0:
		if (game->mPointer._110 != NULL) {
			tex    = *game->mPointer._110;
			width  = GXGetTexObjWidth(&tex);
			height = GXGetTexObjHeight(&tex);
		} else {
			game->mTpl.mTex.Init(&game->mTpl.mPalette->descriptorArray[256], 0);
			tex    = game->mTpl.mTex;
			width  = GXGetTexObjWidth(&tex) / 2;
			height = GXGetTexObjHeight(&tex);
			PSMTXScale(tex.mTexMtx.mMtx[2], 0.5f, 1.0f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			if (game->mPointer.mScopeTarget != NULL) {
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.5f, 0.0f, 0.0f);
				tex.mTexMtx.mDirty = TRUE;
			}
		}
		Mtx rotation;
		PSMTXRotRad(rotation, 'z', pointerAngle);
		CVec offset;
		offset.x = 0.4f * width;
		offset.y = 0.4f * height;
		offset.z = 0.0f;
		PSMTXMultVec(rotation, &offset, &offset);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x + offset.x, y + offset.y, offset.z, width * scale,
		                   height * scale, 0.0f, 0.0f, -pointerAngle);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);

		alpha = game->mPointer.mScopeFade;
		if (alpha <= 0.0f) {
			break;
		}
		if (game->mPointer.mScopeTarget != NULL && game->mPointer.mScopeMarker >= 0) {
			game->mTpl.mTex.Init(&game->mTpl.mPalette->descriptorArray[260], 0);
			tex    = game->mTpl.mTex;
			width  = GXGetTexObjWidth(&tex) / 10;
			height = GXGetTexObjHeight(&tex) / 5;
			PSMTXScale(tex.mTexMtx.mMtx[2], 0.1f, 0.2f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			s32 marker         = game->mPointer.mScopeMarker;
			if (marker == 0) {
				marker = lbl_802E5F18[game->GetPlayer()->mSuits.mCurrent];
			}
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.1f * (marker % 10), 0.2f * (marker / 10), 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00,
			                   (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00, x + offset.x, y + offset.y, 0.0f,
			                   width * scale, height * scale, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		} else if (game->mPointer.mScopeItem >= 0) {
			game->mTpl.mTex.Init(&game->mTpl.mPalette->descriptorArray[260], 0);
			tex    = game->mTpl.mTex;
			width  = GXGetTexObjWidth(&tex) / 10;
			height = GXGetTexObjHeight(&tex) / 5;
			PSMTXScale(tex.mTexMtx.mMtx[2], 0.1f, 0.2f, 1.0f);
			tex.mTexMtx.mDirty = TRUE;
			if (game->mPointer.mScopeRareItem) {
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.8f, 0.0f);
			} else {
				PSMTXTrans(tex.mTexMtx.mMtx[0], 0.1f, 0.8f, 0.0f);
			}
			tex.mTexMtx.mDirty = TRUE;
			if (game->mPointer.mScopeTarget == NULL) {
				width *= 0.8f;
				height *= 0.8f;
				offset.x = 0.4f * width;
				offset.y = 0.4f * height;
				PSMTXMultVec(rotation, &offset, &offset);
			}
			gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_NONE);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00,
			                   (u32)(255.0f * alpha) | 0xFFFFFF00, (u32)(255.0f * alpha) | 0xFFFFFF00, x + offset.x, y + offset.y, 0.0f,
			                   width * scale, height * scale, 0.0f, game->mPointer.mScopeAngle, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);
		}
		break;
	case 2:
		game->mTpl.mTex.Init(&game->mTpl.mPalette->descriptorArray[258], 0);
		tex    = game->mTpl.mTex;
		width  = GXGetTexObjWidth(&tex);
		height = GXGetTexObjHeight(&tex);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width * scale, height * scale, 0.0f, 0.0f,
		                   -pointerAngle);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		break;
	case 3: {
		game->mTpl.mTex.Init(&game->mTpl.mPalette->descriptorArray[259], 0);
		tex       = game->mTpl.mTex;
		width     = GXGetTexObjWidth(&tex) / 2;
		height    = GXGetTexObjHeight(&tex);
		BOOL held = (gWiiPointer.mButton & 0x0001000000000010ULL) != 0;
		PSMTXTrans(tex.mTexMtx.mMtx[0], 0.5f * held, 0.0f, 0.0f);
		tex.mTexMtx.mDirty = TRUE;
		PSMTXScale(tex.mTexMtx.mMtx[2], 0.5f, 1.0f, 1.0f);
		tex.mTexMtx.mDirty = TRUE;
		if (!((CCameraActionScroll2*)game->mCameraActions.mTable[CAMACTION_SCROLL2])->IsFovNotMax()) {
			color = (u32)(0.5f * (255.0f * alpha)) | 0xFFFFFF00;
		}
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width * scale, height * scale, 0.0f, 0.0f,
		                   -pointerAngle);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		break;
	}
	case 1: {
		// This retail case has no mode-1 producer and dereferences its null target.
		XObj* target = NULL;
		Vec pos      = *target->GetPos();
		pos.x += 5.0f * (f32)sin(PI + target->mMoveRotate.y);
		pos.z += 5.0f * (f32)cos(PI + target->mMoveRotate.y);
		f32 depth = HALF_PI - game->GetCamera()->mView.mAngle.x;
		if (depth > 1.2217305f) {
			depth = 1.2217305f;
		}
		const CVec* playerPos = game->GetPlayer()->GetPos();
		f32 angle             = (f32)atan2(pos.x - playerPos->x, pos.z - playerPos->z);
		angle -= game->GetCamera()->mView.mAngle.y;
		game->mTpl.mTex.Init(&game->mTpl.mPalette->descriptorArray[257], 0);
		tex = game->mTpl.mTex;
		gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_NONE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		gGame.mGs.mRotOrder = 1;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, 50.0f * scale, 50.0f * scale, depth, 0.0f, angle);
		gGame.mGs.mRotOrder = 4;
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);
		break;
	}
	case 4:
		if (!game->GetStatusFlags(0x100)) {
			game->mRoboeyes.mRotation = game->GetPlayer()->mMoveRotate.y;
			game->mRoboeyes.DrawLenses(1, -1, 0xFF, x, y, 430.0f, 400.0f);
		}
		break;
	}
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
}

/**
 * @note Address: 0x8002C298
 * @note Size: 0x8E4
 */
// NONMATCHING: written from the GameCube's and the retail code, not tuned: the float registers and the
// literal pool differ; the GameCube's tev and blend calls are the CGs inlines here
// draws the stage name: a bubble per letter, then the letter with a shadow on four sides. while a stage
// loads the pointer pops a bubble: it jumps, the loading object plays one of two animations and the
// stage's sound for that letter plays
extern "C" void DrawStageNameBubbles(CGame* game)
{
	const f32 spacing   = 58.0f;
	const f32 glyphSize = 25.0f;
	if (game->mStageName.mActive && !game->GetStatusFlags(2)) {
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		f32 center = gPointerScreenWidth / 2;
		game->mTpl.mTex.Init(&game->mTpl.mPalette->descriptorArray[139], 0);
		CTexObj bubble = game->mTpl.mTex;
		u32 color      = game->mStageName.mColor & 0xFFFFFF00; // rgb only; alpha comes from the scale
		for (s32 i = 0; i < game->mStageName.mCount; i++) {
			f32 scale = game->mStageName.mChars[i].mScale;
			if (scale <= 0.0f) {
				continue;
			}
			CFont font;
			s32 width;
			CTexObj letter = *GetFontGlyphTexture(&font, game->mStageName.mChars[i].mText, 1, &width);
			f32 bob        = 4.0f * std::sinf(game->mStageName.mChars[i].mBob);
			f32 rot        = DEG2RAD * (10.0f * std::sinf(0.7f * game->mStageName.mChars[i].mTilt));
			f32 swell      = std::sinf(3.1415927f * scale);
			f32 pop        = 0.2f * swell + std::sinf(1.5707964f * scale);
			f32 x          = spacing * 0.5f + (spacing * i + (center - spacing * game->mStageName.mCount * 0.5f));
			f32 y          = 150.0f;
			if (gWiiPointer.mPointerValid && game->IsLoadStepRunning() && game->mLoadStep > 20) {
				f32 half = glyphSize * (2.2f * pop) * 0.5f;
				if (gWiiPointer.mPointer.x >= x - half && gWiiPointer.mPointer.x <= x + half && gWiiPointer.mPointer.y >= 150.0f - half
				    && gWiiPointer.mPointer.y <= 150.0f + half) {
					if (!game->mStageName.mChars[i]._14) {
						static const s32 anims[2]      = { 1011, 1007 };
						game->mStageName.mChars[i]._14 = TRUE;
						game->mStageName.mChars[i]._18 = 1.0f;
						((CObj*)game->mPointer.mCursor)->SetAnim(anims[rand() % 2], TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
						s32 first = GetStageFirstSoundId(game->mStage);
						if (first >= 0) {
							for (s32 j = 0; j < game->mStageName.mCount; j++) {
								if (IsWiiSfxPlaying(&gGame.mSound, first + j)) {
									StopWiiSfx(&gGame.mSound, first + j, 0.1f);
								}
							}
							PlayWiiSoundEffect(&gGame.mSound, first + i, 1.0f, 0.0f);
						}
					}
				} else {
					game->mStageName.mChars[i]._14 = FALSE;
				}
			} else {
				game->mStageName.mChars[i]._14 = FALSE;
			}
			if (game->mStageName.mChars[i]._18 > 0.0f) { // the jump
				if ((game->mStageName.mChars[i]._18 -= 0.035f * game->mRetraceStep) <= 0.0f) {
					game->mStageName.mChars[i]._18 = 0.0f;
				}
				y = 150.0f - 30.0f * std::sinf(0.9f * (3.1415927f * game->mStageName.mChars[i]._18));
			}
			u32 alpha = 255.0f * scale;
			DrawGsTexturedQuad(&gGame.mGs, &bubble, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, x,
			                   y + bob, 0.0f, glyphSize * (2.2f * pop), glyphSize * (2.2f * pop), 0.0f, -0.0f, -rot);
			f32 size   = glyphSize * (1.5f * pop);
			f32 shrink = width / 24.0f * size;
			alpha      = 255.0f * scale;
			for (s32 j = 0; j < 4; j++) {
				DrawGsTexturedQuad(&gGame.mGs, &letter, alpha, alpha, alpha, alpha, (size - shrink) * 0.5f + (x + sStageNameShadowX[j]),
				                   bob + (y + sStageNameShadowY[j]), 0.0f, size, size, 0.0f, -0.0f, -rot);
			}
			alpha = 255.0f * scale;
			DrawGsTexturedQuad(&gGame.mGs, &letter, color | alpha, color | alpha, color | alpha, color | alpha, (size - shrink) * 0.5f + x,
			                   y + bob, 0.0f, size, size, 0.0f, -0.0f, -rot);
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
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

extern "C" void fn_8002BBD8(CGs* gs, f32 x, f32 y)
{
}

extern "C" void fn_8002BBDC(CGs* gs)
{
}

/**
 * @note Address: 0x8002B6D8
 * @note Size: 0x500
 */
// NONMATCHING: the all-tracks loops match through ResumeAllTracks; the frame count and loop counter still swap
// registers, and the jingle-specific track lookup has further register differences
inline void CSound::PauseTrack(u32 id, f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (int i = 0; i < 0x40; i++) {
		nw4r::snd::SoundHandle& handle = mArchivePlayer.mTracks.mHandles[i];
		if (handle.IsAttachedSound() && handle.GetId() == id && !handle.IsPause()) {
			handle.Pause(true, frames);
		}
	}
}

inline void CSound::ResumeTrack(u32 id, f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (int i = 0; i < 0x40; i++) {
		nw4r::snd::SoundHandle& handle = mArchivePlayer.mTracks.mHandles[i];
		if (handle.IsAttachedSound() && handle.GetId() == id && handle.IsPause()) {
			handle.Pause(false, frames);
		}
	}
}

inline void XSound::PauseCurrentMusic(f32 fade)
{
	u32 music = mMusicFadeOutFrames <= 0 ? mCurrentMusicId : 0;
	if (music != 0) {
		PauseTrack(music & 0xFFFF, fade);
	}
}

inline void XObjDrawList::PauseSounds(f32 fade)
{
	for (s32 i = 0; i < mCount; i++) {
		XObj* object = (XObj*)mRefs[i].mObj;
		if (object->mStatus.mFlags & 1) {
			object->PauseSounds(fade);
		}
	}
}

inline void XObjDrawList::ResumeSounds(f32 fade)
{
	for (s32 i = 0; i < mCount; i++) {
		XObj* object = (XObj*)mRefs[i].mObj;
		if (object->mStatus.mFlags & 1) {
			object->ResumeSounds(fade);
		}
	}
}

// undoes one PauseGameSounds. the last resume lets the tracks, the objects' sounds and the movie go
// again; the music pause underneath holds the jingle (sound 4) until its own count runs out
extern "C" void ResumeGameSounds(CGame* game, f32 fade)
{
	// _D9B5C counts the pauses, _D9B60 the music pauses under them
	if (game->_D9B5C > 0 && --game->_D9B5C == 0) {
		gGame.mSound.ResumeAllTracks(fade);
		game->mDrawLists[0].ResumeSounds(fade);
		if (game->mMovie.mState != 0) {
			THPSimpleSetVolume(127.0f * game->mMovieVolume, 1000.0f * fade);
		}
	}
	if (game->_D9B60 > 0) {
		if (--game->_D9B60 == 0) {
			if (gSpiderSmallSoundId != 0) {
				gGame.mSound.PauseCurrentMusic(0.2f);
			}
		} else if (FindTrack(4) != NULL && IsTrackPaused(4)) {
			gGame.mSound.ResumeTrack(4, fade);
		}
	}
}

/**
 * @note Address: 0x8002B2F0
 * @note Size: 0x3E8
 */
// NONMATCHING: the all-tracks loop matches through PauseAllTracks; the frame count and loop counter still swap
// registers, and the jingle-specific track lookup has further register differences
// holds every playing track, the objects' own sounds and the movie's volume; pauses nest, only the
// first one acts. a nested pause holds the jingle (sound 4) too
extern "C" void PauseGameSounds(CGame* game, f32 fade)
{
	if (game->_D9B5C == 0) {
		gGame.mSound.PauseAllTracks(fade);
		game->mDrawLists[0].PauseSounds(fade);
		if (game->mMovie.mState != 0) {
			THPSimpleSetVolume(0, 1000.0f * fade);
		}
	}
	game->_D9B5C++;
	if (game->_D9B60 > 0 && FindTrack(4) != NULL && !IsTrackPaused(4)) {
		gGame.mSound.PauseTrack(4, fade);
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
// changes to another stage: 10001 is the title, 10002 the stage from the save, below 0 the next
// one. everything that belongs to the old stage is let go of, and the new stage's name is set up to
// pop in when showName is set
extern "C" void ChangeGameStage(CGame* game, s32 stage, BOOL showName)
{
	StartScreenFade(&game->mFade, 0, 0, 0, 255, 0, 0, 0, 255, 0.0f);
	if (game->mMovie.mState != 0) { // cut any playing movie
		game->mMovie.Stop();
	}
	ClearMainMenuExtraRows(game->mMenu.mMenus[0]);
	ResetEventScriptState(&game->mEvt, TRUE, TRUE, TRUE, TRUE); // the event state in front of mEvtData
	CMsg* msgs = game->mMsgs;
	for (s32 i = 0; i < 2; i++) {
		ResetMsgState(&msgs[i]);
		CMsg_ResetPos(&msgs[i]);
		msgs[i].SetColor(gMessageFontDefaultColor);
	}
	CGame::FbMirrorEx* mirrors = game->mMirrors;
	for (s32 i = 0; i < 5; i++) { // let go of the mirrors' objects and textures
		FbMirror* mirror = &mirrors[i];
		if (mirror->IsUsed() && mirror->mJoint != NULL) {
			if (mirror->mJointAnm != NULL) { // the object's own material animation goes back
				memcpy(mirror->mJointAnm, &mirror->mJointAnmCopy, sizeof(GXTexObj));
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
		if (!game->mEvtPtcl.mEffects[i].IsFinished()) {
			RetireEffect(&game->mEvtPtcl.mEffects[i].handle, TRUE);
		}
	}
	for (u32 i = 0; i < 6; i++) {
		game->mEffects.RetireAll(i, TRUE);
	}
	WiiPlayerState* player = game->GetPlayer();
	for (s32 i = 0; i < 2; i++) {
		if (((AnimSfx*)player->mSfx)->mActors[i].GetHandle() != NULL) {
			((AnimSfx*)player->mSfx)->mActors[i].Stop(0.0f);
		}
	}
	gGame.mSound.StopAllTracks(0.0f);
	ClearSoundEffects(&gGame.mSound, 0);
	gGame.mSound.RestoreHeapLevel(1);
	Vec nan                  = { NAN, NAN, NAN };
	game->mListener.mPoint.x = nan.x;
	game->mListener.mPoint.y = nan.y;
	game->mListener.mPoint.z = nan.z;
	gWiiPointer.ResetRumble(FALSE);
	game->ClearStatusFlags(0x020000000000002F);
	game->ClearStatusFlags(0xC00);
	game->ClearStatusFlags(0x8000);
	game->ClearStatusFlags(0x300);
	game->mRoboeyes.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	game->SetStatusFlags(0x0040000000000000);
	game->SetStatusFlags(0x0000800000000000);
	InfoScrn* scrn = &game->mInfoScrn;
	InfoScrn_ApplyScoreHudCommand(scrn, 0, 0);
	InfoScrn_ApplyObjectGaugeCommand(scrn, NULL, 0, 0, 0);
	InfoScrn_ApplyObjectGaugeCommand(scrn, NULL, 0, 0, 1);
	InfoScrn_ApplyObjectGaugeCommand(scrn, NULL, 0, 0, 2);
	InfoScrn_ApplyObjectGaugeCommand(scrn, NULL, 0, 0, 3);

	if (stage == 10001) {
		if (gCmdLineArgs.mStage < 0 || game->GetStatusFlags(0x0008000000000000)) {
			game->SetStage(GetDefaultStageIndex());
		} else {
			game->SetStage(gCmdLineArgs.mStage);
		}
	} else if (stage == 10002) {
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

	if (stage == 10001) {
		game->SetStatusFlags(0x20000000);
		game->SetStatusFlags(0x0001000000000000);
		game->ClearStatusFlags(0x80);
		ResetTitleScreenState(&game->mTitle);
		ResetMenuGroup(&game->mMenu.mActive); // the menu group starts at its active word
		ResetGameHudState(scrn);
		game->mSubwin.Reset();
		StartScreenFade(&game->mFade, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f);
		game->mWipeCircle.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeTex.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeCustom.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeTile.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeCine.StartTransition(0, 0, 0, 0, 0, 0, 0, gPointerScreenWidth / 2, gPointerScreenHeight / 2, gPointerScreenWidth,
		                                gPointerScreenHeight, gPointerScreenWidth, gPointerScreenHeight, 0.0f, 0, TRUE);
		StartScreenShake(&game->mQuake, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		FbBlur* blur    = &game->mBlur;
		blur->mAlpha    = 0.0f;
		blur->mTarget   = 0.0f;
		blur->mSpeed    = 0.0f;
		game->mLoadStep = 20;
	} else if (stage == 10002) {
		game->mSubwin.Reset();
		StartScreenFade(&game->mFade, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f);
		game->mWipeCircle.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeTex.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeCustom.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeTile.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		game->mWipeCine.StartTransition(0, 0, 0, 0, 0, 0, 0, gPointerScreenWidth / 2, gPointerScreenHeight / 2, gPointerScreenWidth,
		                                gPointerScreenHeight, gPointerScreenWidth, gPointerScreenHeight, 0.0f, 0, TRUE);
		StartScreenShake(&game->mQuake, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		FbBlur* blur    = &game->mBlur;
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
 * @note Address: 0x8002B1F4
 * @note Size: 0xF4
 */
// NONMATCHING: the scene-resource pointer and several loads use different registers
// the scene animation is at its last frame, or will be within this step
bool CObj::IsAnmScnEnd()
{
	f64 frame = mObject.m_resources.scn != NULL ? mObject.m_resources.scn->GetFrame() : 0.0f;
	f32 end   = mObject.m_resources.scn != NULL ? mObject.m_resources.scn->mRes.GetNumFrame() - 1 : 0.0f;
	return (f32)frame + mAnimRate[1] >= end;
}

/**
 * @note Address: 0x8002B18C
 * @note Size: 0x68
 */
// NONMATCHING: retail tests the live bit as a negated bool and keeps each check's result in a flag of its own,
// the trace of inlines whose names are not known
// whether the scene camera's animation is done; without a scene of its own it always is
extern "C" BOOL IsSceneCameraAnmEnd(CGame* game)
{
	XObjScnCam* camera = (XObjScnCam*)game->mXObjs.mPools[0].mObjects;
	if (camera != NULL && (camera->mStatus.mFlags & 1) == 1) {
		BOOL hasScene = FALSE;
		if (camera->mScene != NULL && camera->mScene->_2888 != NULL) {
			hasScene = TRUE;
		}
		if (hasScene) {
			return camera->IsAnmScnEnd();
		}
	}
	return TRUE;
}

/**
 * @note Address: 0x8002B088
 * @note Size: 0x104
 */
// NONMATCHING: retail tests the live bit as a negated bool and keeps each check's result in a flag of its own,
// the trace of inlines whose names are not known
// lets go of the scene camera's animation and puts the perspective camera back to its defaults
extern "C" void StopSceneCameraAnm(CGame* game)
{
	XObjScnCam* camera = (XObjScnCam*)game->mXObjs.mPools[0].mObjects;
	if (camera != NULL && (camera->mStatus.mFlags & 1) == 1) {
		BOOL hasAnimation = FALSE;
		BOOL hasEntries   = FALSE;
		if (camera->mObject.m_pScnMdl != NULL && camera->mObject.m_resFile.HasResAnmScn()) {
			hasAnimation = TRUE;
		}
		if (hasAnimation && camera->mObject.m_resFile.GetResAnmScnNumEntries() != 0) {
			hasEntries = TRUE;
		}
		if (hasEntries) {
			camera->ReleaseModelWork();
			game->GetPerspectiveCamera()->Init(1.0f, 80000.0f, 30.0f, gScreenWidthScale, &gGame.mGs.mScreen);
		}
	}
}

/**
 * @note Address: 0x8002AF94
 * @note Size: 0xF4
 */
// NONMATCHING: retail tests the live bit as a negated bool and keeps each check's result in a flag of its own,
// the trace of inlines whose names are not known
// plays scene animation id on the scene camera from frame at rate; the camera, or NULL when it has no
// scene animations
extern "C" XObj* StartSceneCameraAnm(CGame* game, u32 id, f32 frame, f32 rate)
{
	XObjScnCam* camera = (XObjScnCam*)game->mXObjs.mPools[0].mObjects;
	if (camera != NULL && (camera->mStatus.mFlags & 1) == 1) {
		BOOL hasAnimation = FALSE;
		BOOL hasEntries   = FALSE;
		if (camera->mObject.m_pScnMdl != NULL && camera->mObject.m_resFile.HasResAnmScn()) {
			hasAnimation = TRUE;
		}
		if (hasAnimation && camera->mObject.m_resFile.GetResAnmScnNumEntries() != 0) {
			hasEntries = TRUE;
		}
		if (hasEntries) {
			camera->SetAnmScn(id, frame, rate);
			return camera;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x8002AEC8
 * @note Size: 0xCC
 */
void CGame::ProjectOffsetToScreen(const Vec* pos, f32* x, f32* y, f32* z, void* camera)
{
	CCamera* cam = camera != NULL ? (CCamera*)camera : &gGame.mMotionControlCamera.mCamera;
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
// NONMATCHING: integer/float register allocation and centring multiply operand order differ.
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
			gs->mCurRenderState->SetAlphaCompare(GX_GREATER, 0x20, GX_AOP_AND, GX_LEQUAL, 255);
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
		gGame.mGs.mCurRenderState->SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 255);
	}
}

/**
 * @note Address: 0x80027E84
 * @note Size: 0x96C
 */
// NONMATCHING: written from the retail code, not tuned; retail's FSqrt is nw4r's with the domain warning
// (WiiReportMathError), the header's has none
// renders the shadow from straight above the focus object (or the scene camera's interest) into a
// texture: the 0x4 objects cast it into the alpha, the 0x8 ones get their ground shadow drawn, both
// fading over a hundred units unless the stage keeps them at full strength
extern "C" void RenderSceneShadow(CGame* game)
{
	if (!game->GetStatusFlags(1)) {
		return;
	}
	if (game->mHomeMenu.IsOpen()) {
		return;
	}
	bool menuShown = game->mMenu.mActive && game->mMenu._34;
	if (menuShown) {
		return;
	}
	CVec focus;
	CCamera* camera = &game->mShadow.mCamera;
	if (game->mShadowFocus != NULL && (game->mShadowFocus->mStatus.mFlags & 1)) {
		CVec* pos = game->mShadowFocus->GetPos();
		focus.x   = pos->x;
		focus.y   = pos->y;
		focus.z   = pos->z;
	} else {
		CCamera* scene = game->GetCamera();
		focus.x        = scene->mView.mInterest.x;
		focus.y        = scene->mView.mInterest.y;
		focus.z        = scene->mView.mInterest.z;
	}
	f32 width  = game->mShadowWidth > 0.0f ? game->mShadowWidth : 100.0f;
	f32 height = game->mShadowHeight > 0.0f ? game->mShadowHeight : 100.0f;
	f32 alpha  = game->mShadowAlpha > 0.0f ? game->mShadowAlpha : 1.0f;
	camera->Init(height, -height, -width, width, camera->mNear, camera->mFar, 1.0f, &gGame.mGs.mScreen);
	camera->SetEye(focus.x, 400.0f + focus.y, focus.z);
	camera->SetInterest(focus);
	camera->SetUp(0.0f, 0.0f, 1.0f);
	camera->vf88(1.0f, 1.0f, game->mShadow.mTexWidth - 2, game->mShadow.mTexHeight - 2, 0.0f, 1.0f);
	game->mShadow.mStrength = alpha;
	game->BeginCamera(camera);
	fn_802AEF34(&gGame.mGs.mScene.mHead, camera, TRUE);
	gGame.mGs.mCurRenderState->SetColorUpdate(GX_FALSE);
	gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_TRUE);
	gGame.mGs.mCurRenderState->SetDstAlpha(GX_FALSE, 0);

	f32 interestY = game->GetCamera()->mView.mInterest.y;
	f32 strength  = game->mShadow.mStrength;
	for (s32 i = 0; i < game->mDrawLists[3].mCount; i++) {
		XObj* obj = (XObj*)game->mDrawLists[3].mRefs[i].mObj;
		if (!(obj->mStatus.mFlags & 4)) {
			continue;
		}
		if (interestY - obj->GetPos()->y >= 40.0f) {
			continue;
		}
		CVec* pos = obj->GetPos();
		f32 dz    = focus.z - pos->z;
		f32 dx    = focus.x - pos->x;
		f32 dist  = nw4r::math::FSqrt(dx * dx + dz * dz);
		f32 fade;
		if (GetStageInfo()->mFlags & 1) {
			fade = 1.0f;
		} else { // else it fades over a hundred units
			fade = (100.0f - dist) / 100.0f;
			if (fade < 0.0f) {
				fade = 0.0f;
			}
			if (fade > 0.8f) {
				fade = 0.8f;
			}
		}
		strength = sin(3.1415927f * fade * 0.5f); // a half-sine falloff
		obj->DrawGroundShadow(strength);
	}

	gGame.mGs.mScene.GetRoot()->SetForceResMdlDrawMode(true, nw4r::g3d::RESMDL_DRAWMODE_IGNORE_MATERIAL);
	GXColor color = { 0, 0, 0, 0 };
	color.a       = 100.0f * strength;
	GXSetNumTexGens(0);
	GXSetNumChans(1);
	GXSetNumTevStages(1);
	GXSetNumIndStages(0);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
	GXSetChanAmbColor(GX_COLOR0A0, color);
	GXSetChanMatColor(GX_COLOR0A0, color);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	GXSetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, GX_LO_COPY);
	gGame.mGs.mScene.GetRoot()->CalcView();
	gGame.mGs.mScene.GetRoot()->GatherDrawScnObj();
	gGame.mGs.mScene.GetRoot()->ZSort();
	gGame.mGs.mScene.GetRoot()->DrawXlu();
	gGame.mGs.mScene.GetRoot()->DrawOpa();
	WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
	while (scene->mPending != 0) {
		if (scene->mPending != 0) {
			scene->vf038(scene->mPending - 1);
		}
	}
	gGame.mGs.mScene.GetRoot()->SetForceResMdlDrawMode(false, nw4r::g3d::RESMDL_DRAWMODE_DEFAULT);
	gGame.mGs.mCurRenderState->SetColorUpdate(GX_FALSE);
	gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_TRUE);
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	if (gGame.mGs.mCurRenderState != NULL) {
		gGame.mGs.mCurRenderState->Apply();
	}
	if (gGame.mGs.mCurTevStage != NULL) {
		gGame.mGs.mCurTevStage->Apply();
	}
	if (gGame.mGs.mCurChannel != NULL) {
		gGame.mGs.mCurChannel->Apply();
	}

	for (s32 i = 0; i < game->mDrawLists[3].mCount; i++) {
		XObj* obj = (XObj*)game->mDrawLists[3].mRefs[i].mObj;
		if (obj->GetCategory() == 3 || !(obj->mStatus.mFlags & 8)) {
			continue;
		}
		if (interestY - obj->GetPos()->y >= 40.0f) {
			continue;
		}
		CVec* pos = obj->GetPos();
		f32 dz    = focus.z - pos->z;
		f32 dx    = focus.x - pos->x;
		f32 dist  = nw4r::math::FSqrt(dx * dx + dz * dz);
		f32 fade;
		if (GetStageInfo()->mFlags & 1) {
			fade = 1.0f;
		} else {
			fade = (100.0f - dist) / 100.0f;
			if (fade < 0.0f) {
				fade = 0.0f;
			}
			if (fade > 0.8f) {
				fade = 0.8f;
			}
		}
		f32 opacity = sin(3.1415927f * fade * 0.5f);
		obj->DrawGroundShadowAt(obj->GetPos(), opacity);
	}

	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GXSetTexCopySrc(0, 0, game->mShadow.mTexWidth, game->mShadow.mTexHeight);
	GXSetTexCopyDst(game->mShadow.mTexWidth, game->mShadow.mTexHeight, GX_CTF_A8, GX_FALSE);
	GXCopyTex(game->mShadow.mImage, GX_TRUE);
	GXPixModeSync();
	GXInvalidateTexAll();
	gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
	gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	gGame.mGs.mScene.GetRoot()->SetCurrentCamera(0);
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
		bool menuShown = mMenu.mActive && mMenu._34;
		if (menuShown) {
			return;
		}
		for (s32 i = 0; i < mXObjs.mObjectCount; i++) {
			XObj* object = (XObj*)mXObjs.mObjects[i];
			if (object->mStatus.mFlags & 1) {
				object->DrawScreenOverlay();
			}
		}
		if (mPointer.mHudObject != NULL) {
			mPointer.mHudObject->vf19C(0xFF0000C8, 0.0f, mPointer.mHudObjectX, mPointer.mHudObjectY);
		}
		CallStageModule(4, 0, NULL);
		fn_802AEDEC(&gGame.mGs.mScene.mHead, GetCamera()->mViewMtx);
		WiiGsSceneObj* obj = gGame.mGs.mScene.mHead;
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
	bool menuShown = game->mMenu.mActive && game->mMenu._34;
	if (menuShown) {
		return;
	}
	if (game->mColor0.mColor.mColor.a > 0.0f) {
		for (s32 i = 0; i < game->mDrawLists[3].mCount; i++) {
			XObj* obj = (XObj*)game->mDrawLists[3].mRefs[i].mObj;
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
	bool menuShown = game->mMenu.mActive && game->mMenu._34;
	if (menuShown) {
		return;
	}
	for (s32 i = 0; i < game->mDrawLists[3].mCount; i++) {
		XObj* obj = (XObj*)game->mDrawLists[3].mRefs[i].mObj;
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
	bool menuShown = mMenu.mActive && mMenu._34;
	if (menuShown) {
		return;
	}
	XObj* targets[2]       = { NULL, NULL };
	WiiPlayerState* player = GetPlayer();
	targets[0]             = player->mLiftPlugTarget;
	if (targets[1] == NULL) {
		targets[1] = player->mLift.mTarget;
	}
	gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
	for (s32 i = 0; i < mDrawLists[2].mCount; i++) {
		XObj* obj = (XObj*)mDrawLists[2].mRefs[i].mObj;
		if (obj->GetCategory() == 3) {
			obj->DrawObject();
		}
	}
	for (s32 i = 0; i < mDrawLists[2].mCount; i++) {
		XObj* obj = (XObj*)mDrawLists[2].mRefs[i].mObj;
		if (obj->GetCategory() != 3) {
			obj->DrawObject();
		}
	}
	DrawSceneQueue(GetCamera()->mViewMtx);

	gGame.mGs.mCurRenderState->SetColorUpdate(GX_FALSE);
	gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_TRUE);
	gGame.mGs.mCurRenderState->SetDstAlpha(GX_TRUE, 4);
	for (s32 i = 0; i < mDrawLists[2].mCount; i++) {
		XObj* obj = (XObj*)mDrawLists[2].mRefs[i].mObj;
		if (obj->GetCategory() != 3 && ((obj->mStatus.mFlags & 0x820040) || obj == targets[1])) {
			obj->DrawObject();
		}
	}
	DrawSceneQueue(GetCamera()->mViewMtx);

	gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
	for (s32 i = 0; i < mDrawLists[2].mCount; i++) {
		XObj* obj = (XObj*)mDrawLists[2].mRefs[i].mObj;
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
	DrawSceneQueue(GetCamera()->mViewMtx);
}

/**
 * @note Address: 0x8002695C
 * @note Size: 0x4CC
 */
// NONMATCHING: not tuned; the camera copy and the loop's registers differ
// draws what each live mirror shows into its texture: the 0x2000 objects through the mirror's camera
extern "C" void RenderMirrorTextures(CGame* game)
{
	if (!game->GetStatusFlags(1) || !game->GetStatusFlags(0x1000)) {
		return;
	}
	if (game->mHomeMenu.IsOpen()) {
		return;
	}
	bool menuShown = game->mMenu.mActive && game->mMenu._34;
	if (menuShown) {
		return;
	}
	CGame::FbMirrorEx* mirrors = game->mMirrors;
	for (s32 i = 0; i < 5; i++) {
		FbMirror* mirror = &mirrors[i];
		if (mirror->mJoint == NULL || !mirror->mVisible) {
			continue;
		}
		bool off = mirror->mJoint == NULL || (mirror->mJoint->mStatus & 0x0001000000000000);
		if (off) {
			continue;
		}
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		CCamera camera = mirror->mCamera;
		Mtx view;
		PSMTXCopy(mirror->mView, view);
		camera.vf64(view);
		if (game->BeginCamera(&camera)) {
			for (s32 j = 0; j < game->mDrawLists[2].mCount; j++) {
				XObj* obj = (XObj*)game->mDrawLists[2].mRefs[j].mObj;
				if (obj->mStatus.mFlags & 0x2000) {
					obj->DrawReflection(view);
				}
			}
			DrawSceneQueue(view);
			game->vf54();
			mirror->Copy();
		}
	}
}

/**
 * @note Address: 0x80026E28
 * @note Size: 0x298
 */
// NONMATCHING: FbSubwin::Copy is a virtual call in retail (see ChangeGameStage)
// draws the subwindow's view into its quarter of the screen and copies it: the 0x2000000 objects always,
// the rest when they are in the subwindow camera's view
extern "C" void RenderSubwindow(CGame* game)
{
	if (!game->GetStatusFlags(1) || !game->GetStatusFlags(0x100)) {
		return;
	}
	if (game->mHomeMenu.IsRunning()) {
		return;
	}
	bool menuShown = game->mMenu.mActive && game->mMenu._34;
	if (menuShown) {
		return;
	}
	FbSubwin* subwin = &game->mSubwin;
	if (!(subwin->mAmount > 0.0f)) {
		return;
	}
	if (game->BeginCamera(&subwin->mCamera)) {
		GXSetScissor(0, 0, gPointerScreenWidth / 2, gPointerScreenHeight / 2);
		for (s32 i = 0; i < game->mXObjs.mObjectCount; i++) {
			XObj* obj = (XObj*)game->mXObjs.mObjects[i];
			bool draw = false;
			if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
				if (obj->mStatus.mFlags & 0x2000000) {
					draw = true;
				} else if (obj->ProjectVisibleBounds(NULL, NULL, &subwin->mCamera, obj->GetFloorXZExtent() * 0.5f,
				                                     obj->GetCollisionHeight())) {
					draw = true;
				}
			}
			if (draw) {
				obj->DrawObject();
			}
		}
		DrawSceneQueue(NULL);
		game->vf54();
	}
	subwin->Copy();
}

/**
 * @note Address: 0x80027114
 * @note Size: 0x5E4
 */
// NONMATCHING: not tuned; the clear quad's corners are built in another order
// clears the screen's alpha through the screen camera, then draws the 0x40 objects and what the player
// carries (unless he is plugging it in) into the alpha only, for the outline pass
extern "C" void DrawObjectAlphaMask(CGame* game)
{
	if (!game->GetStatusFlags(1) || !game->GetStatusFlags(0x40)) {
		return;
	}
	if (game->mHomeMenu.IsRunning()) {
		return;
	}
	bool menuShown = game->mMenu.mActive && game->mMenu._34;
	if (menuShown) {
		return;
	}
	if (game->vf5C()) {
		SetGsChannelEnabled(&gGame.mGs, 4, 0);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetColorUpdate(GX_FALSE);
		gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_TRUE);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurRenderState->SetDstAlpha(GX_FALSE, 0);
		if (gGame.mGs.mCurRenderState != NULL) {
			gGame.mGs.mCurRenderState->Apply();
		}
		if (gGame.mGs.mCurTevStage != NULL) {
			gGame.mGs.mCurTevStage->Apply();
		}
		if (gGame.mGs.mCurChannel != NULL) {
			gGame.mGs.mCurChannel->Apply();
		}
		Vec quad[4] = { 0 }; // a full-screen clear
		quad[0].x   = (f32)gScreenLeft;
		quad[0].y   = (f32)gScreenTop;
		quad[1].x   = (f32)gScreenRight;
		quad[1].y   = (f32)gScreenTop;
		quad[2].x   = (f32)gScreenRight;
		quad[2].y   = (f32)gScreenBottom;
		quad[3].x   = (f32)gScreenLeft;
		quad[3].y   = (f32)gScreenBottom;
		DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, 4, quad[1].x, quad[1].y, quad[1].z, 4, quad[2].x, quad[2].y,
		                 quad[2].z, 4, quad[3].x, quad[3].y, quad[3].z, 4);
		gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
		game->vf54();
	}
	if (game->BeginSceneCamera()) {
		XObj* targets[2]       = { NULL, NULL };
		WiiPlayerState* player = game->GetPlayer();
		targets[0]             = player->mLiftPlugTarget;
		if (targets[1] == NULL) {
			targets[1] = player->mLift.mTarget;
		}
		gGame.mGs.mScene.GetRoot()->SetForceResMdlDrawMode(true, nw4r::g3d::RESMDL_DRAWMODE_IGNORE_MATERIAL);
		GXSetNumTexGens(0);
		GXSetNumChans(1);
		GXSetNumTevStages(1);
		GXSetNumIndStages(0);
		GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
		GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
		GXColor white = { 0xFF, 0xFF, 0xFF, 0xFF };
		GXSetChanAmbColor(GX_COLOR0A0, white);
		GXColor mat = { 0xFF, 0xFF, 0xFF, 0xFF };
		GXSetChanMatColor(GX_COLOR0A0, mat);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_TRUE);
		gGame.mGs.mCurRenderState->SetDstAlpha(GX_TRUE, 0);
		for (s32 i = 0; i < game->mDrawLists[2].mCount; i++) {
			XObj* obj = (XObj*)game->mDrawLists[2].mRefs[i].mObj;
			if (obj->mStatus.mFlags & 0x40) {
				obj->DrawObject();
			} else if (obj == targets[1] && player->mLift.mState != 24) { // not while he plugs it in
				obj->DrawObject();
			}
		}
		DrawSceneQueue(NULL);
		gGame.mGs.mScene.GetRoot()->SetForceResMdlDrawMode(false, nw4r::g3d::RESMDL_DRAWMODE_DEFAULT);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		gGame.mGs.mCurRenderState->SetDstAlpha(GX_FALSE, 0);
		game->vf54();
	}
}

/**
 * @note Address: 0x800270C8
 * @note Size: 0x4C
 */
// NONMATCHING: retail schedules the first height load before the saved-register stores.
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
	return mFloorXZExtent[0];
}

/**
 * @note Address: 0x80026440
 * @note Size: 0x50C
 */
// NONMATCHING: not tuned; the camera copy and the loop's registers differ
// draws the 0x20000000 objects mirrored in the floor through a copy of the main camera under the
// floor's reflection
extern "C" void RenderFloorReflection(CGame* game)
{
	if (!game->GetStatusFlags(1) || !game->GetStatusFlags(0x20)) {
		return;
	}
	if (game->mHomeMenu.IsRunning()) {
		return;
	}
	bool menuShown = game->mMenu.mActive && game->mMenu._34;
	if (menuShown) {
		return;
	}
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	CCamera camera = *game->GetCamera();
	Mtx view;
	PSMTXConcat(camera.mViewMtx, game->mPointer.mFloorReflectMtx, view);
	camera.vf64(view);
	if (game->BeginCamera(&camera)) {
		for (s32 i = 0; i < game->mDrawLists[3].mCount; i++) {
			XObj* obj = (XObj*)game->mDrawLists[3].mRefs[i].mObj;
			if ((obj->mStatus.mFlags & 0x20000000) && (obj->mBaseMtxPtr == NULL || (obj->mStatus.mFlags & 0x0000400000000000))) {
				obj->SetCullMode(1);
				obj->DrawReflection(view);
			}
		}
		DrawSceneQueue(view);
		game->vf54();
	}
	for (s32 i = 0; i < game->mDrawLists[3].mCount; i++) {
		XObj* obj = (XObj*)game->mDrawLists[3].mRefs[i].mObj;
		if ((obj->mStatus.mFlags & 0x20000000) && (obj->mBaseMtxPtr == NULL || (obj->mStatus.mFlags & 0x0000400000000000))) {
			obj->ResetCullMode();
		}
	}
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
	return &mCameras[1];
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
 * @note Address: 0x80025050
 * @note Size: 0x1304
 */
// NONMATCHING: written from the retail code, not tuned; FbSubwin's Draw is a virtual call in retail (see
// ChangeGameStage) and the effect view's two inlines are guesses
// the frame: the shadow, subwindow, mirror and floor passes, the scene, the outline and blur, the effects,
// then the screen layer (hud, wipes, movie, stage name, messages), the menus and the HOME Menu on top.
// the quake shakes the camera's interest for the frame only
void CGame::Draw()
{
	if (mResetRequested) {
		if (mHomeMenu.IsActive() && vf5C()) {
			DrawWiiHomeMenu(&mHomeMenu);
			vf54();
		}
		return;
	}
	CVec interest = GetCamera()->mView.mInterest;
	bool shaking  = mQuake.mShake[0].x != 0.0f || mQuake.mShake[0].y != 0.0f || mQuake.mShake[0].z != 0.0f;
	if (shaking) {
		CVec offset = mQuake.mShake[0] * (f32)mQuake.mSign;
		CVec moved  = interest + offset;
		GetCamera()->SetInterest(moved);
		GetCamera()->CalcEye();
	}
	BuildSceneDrawLists(this);
	if (gGame.mGs.mCurRenderState != NULL) {
		gGame.mGs.mCurRenderState->Apply();
	}
	if (gGame.mGs.mCurTevStage != NULL) {
		gGame.mGs.mCurTevStage->Apply();
	}
	if (gGame.mGs.mCurChannel != NULL) {
		gGame.mGs.mCurChannel->Apply();
	}
	RenderSceneShadow(this);
	gGame.mGs.fn_802AB860();
	SetCameraLight();
	if (gGame.mGs.mScene.mHead->_2888 != NULL) {
		mEffects.UpdateLights(gGame.mGs.mLight);
		nw4r::g3d::Fog fog = gGame.mGs.mScene.GetRoot()->GetFog(0);
		GXFogType type;
		f32 startZ;
		f32 endZ;
		f32 nearZ;
		f32 farZ;
		GXColor color;
		fog.GetFog(&type, &startZ, &endZ, &nearZ, &farZ, &color);
		GXColor fogColor = color;
		mEffects.SetFog(type, &fogColor, startZ, endZ, nearZ, farZ);
	}
	RenderSubwindow(this);
	RenderMirrorTextures(this);
	RenderFloorReflection(this);
	if (GetStatusFlags(0x40) && !(GetPlayer()->mStatus.mFlags & 0x200000)) {
		DrawObjectAlphaMask(this);
	}
	if (BeginSceneCamera()) {
		DrawScene();
		vf54();
	}
	FbEffect* outline = &mOutline;
	if (vf5C()) {
		if (GetStatusFlags(1)) {
			if (GetStatusFlags(0x40)) {
				outline->Draw(FBEFF_COPY_A8 | 0x10);
			}
			if (GetStatusFlags(0x10000)) {
				outline->Draw(FBEFF_OUTLINE);
			}
		}
		mBlur.Draw();
		vf54();
	}
	if (BeginSceneCamera()) {
		if (gGame.mGs.mCurRenderState != NULL) {
			gGame.mGs.mCurRenderState->Apply();
		}
		if (gGame.mGs.mCurTevStage != NULL) {
			gGame.mGs.mCurTevStage->Apply();
		}
		if (gGame.mGs.mCurChannel != NULL) {
			gGame.mGs.mCurChannel->Apply();
		}
		DrawObjectEffectsPass(this);
		if (!mHomeMenu.IsRunning() && !(mMenu.mActive && mMenu._34)) {
			MtxP view = GetEffectView(this);
			for (u32 i = 0; i < 6; i++) {
				if (i != 1 && i != 4) {
					mEffects.Draw(view, i);
				}
			}
		}
		if (gGame.mGs.mCurRenderState != NULL) {
			gGame.mGs.mCurRenderState->Apply();
		}
		if (gGame.mGs.mCurTevStage != NULL) {
			gGame.mGs.mCurTevStage->Apply();
		}
		if (gGame.mGs.mCurChannel != NULL) {
			gGame.mGs.mCurChannel->Apply();
		}
		vf54();
	}
	if (mColor0.mColor.mColor.a > 0.0f && GetStatusFlags(1)) { // what stays over the screen color
		outline->Draw(FBEFF_COPY_I8 | FBEFF_OVERLAY);
		if (BeginSceneCamera()) {
			for (s32 i = 0; i < mDrawLists[2].mCount; i++) {
				XObj* obj = (XObj*)mDrawLists[2].mRefs[i].mObj;
				if (obj->mStatus.mFlags & 0x200) {
					obj->DrawObject();
				} else if ((obj->mStatus.mFlags & 0x0400000000000000) && (obj->mStatus.mFlags & 0x0000400000000000)) {
					obj->DrawObject();
				}
			}
			DrawSceneQueue(GetCamera()->mViewMtx);
			vf54();
		}
	}
	if (BeginSceneCamera()) {
		gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		if (gGame.mGs.mCurRenderState != NULL) {
			gGame.mGs.mCurRenderState->Apply();
		}
		if (gGame.mGs.mCurTevStage != NULL) {
			gGame.mGs.mCurTevStage->Apply();
		}
		if (gGame.mGs.mCurChannel != NULL) {
			gGame.mGs.mCurChannel->Apply();
		}
		DrawObjectEffectsOverFade(this);
		if (!mHomeMenu.IsRunning() && !(mMenu.mActive && mMenu._34)) {
			MtxP view = GetEffectView(this);
			for (u32 i = 0; i < 6; i++) {
				if (i == 1) {
					mEffects.Draw(view, i);
				}
			}
		}
		vf54();
	}

	SetScreenLight();
	if (gGame.mGs.mScene.mHead->_2888 != NULL) {
		mEffects.UpdateLights(gGame.mGs.mLight);
	}
	DrawTitleScreen(&mTitle);
	if (vf5C()) {
		if (gGame.mGs.mCurRenderState != NULL) {
			gGame.mGs.mCurRenderState->Apply();
		}
		if (gGame.mGs.mCurTevStage != NULL) {
			gGame.mGs.mCurTevStage->Apply();
		}
		if (gGame.mGs.mCurChannel != NULL) {
			gGame.mGs.mCurChannel->Apply();
		}
		DrawHud();
		mEffects.Draw(GetScreenCamera()->mViewMtx, 4);
		Vec v0      = { (f32)gScreenLeft, (f32)gScreenTop, -1.0f };
		Vec v1      = { (f32)gScreenRight, (f32)gScreenTop, -1.0f };
		Vec v2      = { (f32)gScreenRight, (f32)gScreenBottom, -1.0f };
		Vec v3      = { (f32)gScreenLeft, (f32)gScreenBottom, -1.0f };
		Vec quad[4] = { v0, v1, v2, v3 }; // the depth goes back
		gGame.mGs.mCurRenderState->SetColorUpdate(GX_FALSE);
		gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, 0, quad[1].x, quad[1].y, quad[1].z, 0, quad[2].x, quad[2].y,
		                 quad[2].z, 0, quad[3].x, quad[3].y, quad[3].z, 0);
		gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
		gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
		if (!mHomeMenu.IsRunning() && !(mMenu.mActive && mMenu._34)) {
			FbSubwin* subwin = &mSubwin;
			if (GetStatusFlags(1) && GetStatusFlags(0x100) && !mHomeMenu.IsRunning() && !(mMenu.mActive && mMenu._34)
			    && subwin->mAmount > 0.0f && !(subwin->mFlags.mFlags & 4)) {
				subwin->Draw();
			}
			mRoboeyes.Draw(&gGame.mGs, 0);
			DrawGameHud(&mInfoScrn);
			if (GetStatusFlags(1)) {
				mMotionControlCamera.Draw();
			}
			if (mMovieFlags & 2) { // a movie under the wipes
				DrawGameMovie(this);
			}
			DrawScreenColorOverlay(&mFade, &gGame.mGs);
			mWipeCircle.Draw(&gGame.mGs, 0);
			mWipeTex.Draw(&gGame.mGs, 1);
			mWipeCustom.Draw(&gGame.mGs, 1);
			mWipeTile.Draw(&gGame.mGs, 1);
			mWipeCine.Draw(&gGame.mGs, 1);
			if (GetStatusFlags(1) && !mMenu.mActive) {
				bool faded = mFade.mColor.r != 0.0f || mFade.mColor.g != 0.0f || mFade.mColor.b != 0.0f || mFade.mColor.a != 0.0f;
				if (!faded
				    || ((mFade.mColor.r != 0.0f || mFade.mColor.g != 0.0f || mFade.mColor.b != 0.0f || mFade.mColor.a != 0.0f)
				        && mFade.mColor.a < 1.0f)) {
					mMotionControlCamera.fn_80169528();
				}
			}
			if (!(mMovieFlags & 2)) {
				DrawGameMovie(this);
			}
			DrawStageNameBubbles(this);
			if (GetStatusFlags(0x400)) {
				DrawPlayerEquipmentRings(&GetPlayer()->mMenu);
			}
			if (GetStatusFlags(0x800)) {
				DrawOXMenu(&GetPlayer()->mOXMenu);
			}
			if (!mMotionControlCamera.IsAction(4)) {
				CEvt_Draw(&mEvt);
				CMsg* msgs = mMsgs;
				for (s32 i = 0; i < 2; i++) {
					CMsg_Draw(&msgs[i]);
				}
			}
			DrawSceneQueue(GetScreenCamera()->mViewMtx);
		}
		vf54();
	}
	if (!mHomeMenu.IsRunning()) {
		DrawCurrentLogo(&mLogo);
	}
	if (vf5C()) {
		if (gGame.mGs.mCurRenderState != NULL) {
			gGame.mGs.mCurRenderState->Apply();
		}
		if (gGame.mGs.mCurTevStage != NULL) {
			gGame.mGs.mCurTevStage->Apply();
		}
		if (gGame.mGs.mCurChannel != NULL) {
			gGame.mGs.mCurChannel->Apply();
		}
		if (!mHomeMenu.IsRunning()) {
			fn_80084C84(&mMenu.mActive);
			DrawObjectLoadingProgress(&mInfoScrn);
			DrawOpenMenus(&mMenu.mActive);
			vf7C();
		}
		fn_80028998(this);
		DrawWiiHomeMenu(&mHomeMenu);
		DrawDiscError();
		DrawSceneQueue(GetScreenCamera()->mViewMtx);
		vf54();
	}
	gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
	gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_TRUE);
	gGame.mGs.ResetZMode();
	for (s32 i = 0; i < mXObjs.mObjectCount; i++) {
		XObj* obj = (XObj*)mXObjs.mObjects[i];
		if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			ClearActiveObjectFloors(obj);
		}
	}
	GetCamera()->SetInterest(interest); // the shake was for this frame only
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
 * @note Address: 0x80024CC0
 * @note Size: 0x38C
 */
// NONMATCHING: the effect manager is still bytes, so its two counts are read through casts; the strings
// are addressed from the whole file's pool
// carries out a requested reset once nothing is busy: without a disc it returns to the menu at once,
// otherwise it waits for the HOME Menu, a suit change, the effects and the sound to settle. a reset to
// the title from a running game changes the stage instead of restarting
extern "C" void UpdateResetRequest(CGame* game)
{
	if (!game->mResetRequested || game->mSaveManager.mJobStarted) {
		return;
	}
	if (!game->mDiskOk) {
		if (game->mResetType == 2) {
			game->Reset(2);
		} else {
			game->mResetRequested = false;
			game->mPad.Clear();
			game->Reset(4);
		}
		return;
	}
	if (game->GetStatusFlags(0x40)) {
		game->ClearStatusFlags(0x40);
	}
	if (!game->mStageName.mActive) {
		game->mStageName.mActive = FALSE;
		for (s32 i = 0; i < 20; i++) {
			game->mStageName.mChars[i].mScale = 0.0f;
			game->mStageName.mChars[i].mWait  = 0.0f;
			game->mStageName.mChars[i].mBob   = DEG2RAD * (rand() % 360);
			game->mStageName.mChars[i].mTilt  = DEG2RAD * (rand() % 360);
		}
	}
	if (game->mHomeMenu.IsOpen()) {
		return;
	}
	if (HasPendingObjectReads()) {
		return;
	}
	if (game->GetPlayer()->mSuits.mLoadStep != 0) {
		return;
	}
	// the effect manager's two live counts
	if (gGame.mEffects.mResources[0].mState != 0 || gGame.mEffects.mResources[1].mState != 0) {
		return;
	}
	if (gGame.mSound.mLoadPending) {
		return;
	}
	game->mResetRequested = false;
	game->mPad.Clear();
	switch (game->mResetType) {
	case 2:
		game->Reset(2);
		break;
	case 4:
		game->Reset(4);
		break;
	case 5:
		game->Reset(5);
		break;
	case 3:
		if (!game->mDiskOk) {
			game->Reset(4);
		} else if (game->mLoadStep <= 20) {
			game->Reset(3);
		} else {
			fn_80084BF0(&game->mMenu.mActive, 6);
			ChangeGameStage(game, 10001, TRUE);
		}
		break;
	default:
		OSPanic("game.cpp", 2979, "Unknown reset type - %d\n", game->mResetType);
		break;
	}
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
void XObj::vf16C(u32 frames)
{
}

/**
 * @note Address: 0x80024BB8
 * @note Size: 0x1C
 */
// moves the object there at once: the target goes along, so nothing steps
void CObj::SetPos(f32 x, f32 y, f32 z)
{
	mMoveTranslate.x = x;
	mMoveTranslate.y = y;
	mMoveTranslate.z = z;
	mMoveTarget0.x   = x;
	mMoveTarget0.y   = y;
	mMoveTarget0.z   = z;
}

/**
 * @note Address: 0x80024B4C
 * @note Size: 0x6C
 */
CVec CVec::operator+(Vec v) const
{
	CVec sum = *this;
	return sum += v;
}

/**
 * @note Address: 0x80024AE8
 * @note Size: 0x64
 */
CVec CVec::operator*(f32 s) const
{
	CVec product = *this;
	return product *= s;
}

// --- gameupdate (0x8001C2D8-0x80024AE8, same retail game.cpp unit) ---

extern "C" {
void ResetPooledObjects(XObjPool* pools); // 0x800C5424
void InitBatteryGauge(InfoScrn* scrn, f32 battery);
void PSMTXReflect(Mtx m, const Vec* p, const Vec* n);
void PSMTX44MultVec(const Mtx44 m, const Vec* src, Vec* dst);
BOOL FinalizeStageObjectSetup(CGame* game);
void BuildObjectDrawList(CGame* game);
void LoadStageCameraAreaState(CCameraActionAreaEx* state);
// only the addresses are retail's, the signatures are read off the calls in this file
void fn_800C7F70(void* map);              // takes the map object, the first of pool 3
BOOL fn_800CDC08(WiiPlayerState* player); // three steps of the player's own setup, each true when done
BOOL RestorePlayerHeldItem(WiiPlayerState* player);
BOOL LoadPlayerAttachmentAnimation(WiiPlayerState* player);
BOOL HideHudPointCounters(InfoScrn* scrn);
BOOL LoadWiiHomeMenu(WiiHomeMenu* menu);
s32 LoadMessageCursorObject(CMsg* msg);
void ApplyWiiSaveSlot(CSaveFile* manager);
void RestoreSavedPlayerAndCamera(CSaveFile* manager);
void ApplyStageSoundEffects(XSound* sound);
BOOL LoadGlobalResourceData();
u32 GetStartingAttachmentLevel(s32 index);
u32 GetStageEntryTimeComponent(s32 index);
u32 CheckStageFlag(s32 stage, u32 flag);
BOOL LoadStageByIndex(s32 stage);
void ApplyStageClearColor();
BOOL LoadMenuGroupResources(void* menu);      // the menus' resources, the GameCube's LoadMenuGroupResources
void UpdateOpenMenus(void* menu, s32 frames); // steps the open menus
}

extern s32 gRefreshRate;
extern u8 gPowerButtonPressed;

class Logo;
class Title;

extern "C" {
BOOL OSGetResetButtonState();
void OSPanic(const char* file, int line, const char* msg, ...);
char* FormatToStaticBuffer(const char* fmt, ...);
void PauseGameSounds(CGame* game, f32 fadeSeconds);
void ResumeGameSounds(CGame* game, f32 fadeSeconds);
void UpdateResetRequest(CGame* game);               // the frame's work at the start of gamedraw.cpp's range
void fn_802AEDA0(WiiGsSceneObj** head, u32 frames); // steps the queued scene draws
void fn_80084B90(void* menu, s32 index);            // opens one of the menus
void CMsg_Update(CMsg* msg, u32 step);
void UpdateBootLogos(Logo* logo, u32 frames);
void UpdateTitleScreen(Title* title, u32 frames);
void UpdateHudTransitionsAndIndicators(InfoScrn* scrn, u32 frames);
void UpdateStageNameBubbles(CGame* game, u32 frames);
u8* GetStageTimerExpiredScript(void);
void fn_800C7764(XObj* camera, CCamera* view);        // the stage camera object follows the game camera
void fn_800B4988(XObj* obj);                          // one entry of the object reference list
void fn_800D1A8C(WiiPlayerState* player, u32 frames); // four steps of the player after the objects ran
void fn_800D1AF8(WiiPlayerState* player, u32 frames);
void fn_800D1BF0(WiiPlayerState* player, u32 frames);
void fn_800D1B64(WiiPlayerState* player, u32 frames);
void fn_800A13D0(void* effects, CCamera* camera);

extern u8 lbl_8044EE4C[];
// the pointer's position in the last frame, and each stage's scope sounds (0x802C49C8)
extern Vec lbl_80444E98;
Vec fn_8010BEE0(XAction* popper);                             // the popper's muzzle, the GameCube's joint path 0 1 0 0
void fn_800E8140(WiiPlayerState* player, f32 yaw, f32 pitch); // where the eye camera's popper aims
}

BOOL fn_80169658(CMotionControlCamera* camera);
CCameraActionPointer* fn_80169674();

// steps the scene list and pops the draws it still holds; Update does it after each group of objects.
// Retail expands it every time, the name is ours
static inline void StepSceneDraws(u32 frames)
{
	fn_802AEDA0(&gGame.mGs.mScene.mHead, frames);
	WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
	while (scene->mPending != 0) {
		if (scene->mPending != 0) {
			scene->vf038(scene->mPending - 1);
		}
	}
}

/**
 * @note Address: 0x8001C2D8
 * @note Size: 0x34
 */
void CScrnWipeCine::Reset()
{
	CScrnWipe::Reset();
	mScaleToWidescreen = FALSE;
}

/**
 * @note Address: 0x8001C30C
 * @note Size: 0x20
 */
// the screen's size out of the render mode
void FbEffect::Init(const GXRenderModeObj* rmode, void* buf, f32 aspect)
{
	Init(rmode->fbWidth, rmode->efbHeight, buf, aspect);
}

/**
 * @note Address: 0x8001C32C
 * @note Size: 0x20
 */
void FbBlur::Init(const GXRenderModeObj* rmode, void* buf, f32 aspect)
{
	Init(rmode->fbWidth, rmode->efbHeight, buf, aspect);
}

/**
 * @note Address: 0x8001C34C
 * @note Size: 0x30
 */
void CScrnWipeTile::StartTransition(u8 r, u8 g, u8 b, u8 a, u8 targetR, u8 targetG, u8 targetB, f32 x, f32 y, f32 width, f32 height,
                                    f32 targetWidth, f32 targetHeight, f32 time, u8 targetA, BOOL hideWhenDone)
{
	CScrnWipe::StartTransition(r, g, b, a, targetR, targetG, targetB, x, y, width, height, targetWidth, targetHeight, time, targetA,
	                           hideWhenDone);
}

/**
 * @note Address: 0x8001C37C
 * @note Size: 0x410
 */
// NONMATCHING: retail hands both vectors to their setter by value (a second copy of each on the stack);
// CVec has no such setter in ut.h and its name is not known
// what a new game starts from, the GameCube's inline of the same name made a virtual
void CGame::ResetNewGameState()
{
	ClearStatusFlags(0x40);
	gWiiPointer.ResetRumble(TRUE);
	for (int i = 0; i < 128; ++i)
		gEventFlags.mFlags[i] = 0;
	for (int i = 0; i < 2048; ++i)
		gEventVars.mVars[i] = 0;

	mMemcardLoaded       = FALSE;
	mTimeState.mMode     = 0;
	mTimeState.mTime     = 0.0f;
	mTimeState.mDuration = 1.0f;
	mTimeState.mRunning  = FALSE;
	mPlayTime            = 0.0f;
	_EC1A4               = 0.0f;
	mPointer.mAlpha      = 1.0f;
	mPointer.mScopeTimer = 0.0f;
	mPointer._10C        = NAN;
	mPointer._120        = -1.0f;
	ResetPooledObjects(mXObjs.mPools);
	InitBatteryGauge(&mInfoScrn, GetPlayer()->mParams.mBattery);

	mScrnTex.mAngle = 0.0f;
	StartScreenFade(&mScrnTex.mColor, 0, 0, 0, 0, 100, 100, 200, 0, 0.0f);
	StartScreenFade(&mColor0.mColor, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f);
	StartScreenFade(&mColor1.mColor, 0, 0, 0, 0, 255, 255, 255, 0, 0.0f);
	mOutlineWidthControl = 0;
	mOutlineWidth        = 0.5f;
	Vec zero             = { 0.0f, 0.0f, 0.0f };
	mListener.mPos.Set(zero);
	Vec nan = { NAN, NAN, NAN };
	mListener.mPoint.Set(nan);
	_D9B60             = 0;
	_D9B5C             = 0;
	mStageName.mActive = FALSE;
	for (int i = 0; i < 20; ++i) {
		mStageName.mChars[i].mScale = 0.0f;
		mStageName.mChars[i].mWait  = 0.0f;
		mStageName.mChars[i].mBob   = DEG2RAD * (rand() % 360);
		mStageName.mChars[i].mTilt  = DEG2RAD * (rand() % 360);
	}
	mMotionControlCamera.fn_80167728();
}

/**
 * @note Address: 0x8001C78C
 * @note Size: 0x24
 */
void CBase::ClearStatusFlags(u64 mask)
{
	mStatus.mFlags &= ~mask;
}

// a menu still open or the objects not read in again yet: steps 20 to 22 of BeforeUpdate wait for both.
// Retail expands the same code three times; the name is ours
static inline BOOL IsStageBusy(CGame* game)
{
	if (game->mMenu.mActive) {
		UpdateOpenMenus(&game->mMenu.mActive, 10);
		if (game->mMenu.mActive) {
			return TRUE;
		}
	}
	if (game->_41F0 > 0) {
		if (--game->_41F0 <= 0) {
			game->mLightList = game->mLights;
			ReleaseStageObjectList(&game->mXObjs);
		} else {
			return TRUE;
		}
	}
	return FALSE;
}

static inline BOOL LoadStartingAttachments(CGame* game)
{
	if (LoadGlobalResourceData()) {
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
	return FALSE;
}

static inline BOOL LoadStageAndEventObjects(CGame* game)
{
	if (LoadStageByIndex(game->mStage)) {
		ApplyStageClearColor();
		for (const StageEventObject* entry = GetStageEventObjectPairs(); entry->objectIndex >= 0; ++entry) { // the pairs end at a -1
			RegisterEventObjectId(&game->mEvt, entry->eventId, entry->objectIndex);
		}
		Vec origin = { 0.0f, 0.0f, 0.0f };
		((CSound3D*)&gGame.mSound.mSound3DManager)->SetListener(0.0f, &origin);
		((CSound3D*)&gGame.mSound.mSound3DManager)->SetVolumePanPitch(10.0f, 10.0f, 50.0f);
		return TRUE;
	}
	return FALSE;
}

static inline bool FadeStopped(const CScrnColor& fade)
{
	return 0.0f == fade.mSpeed.r && 0.0f == fade.mSpeed.g && 0.0f == fade.mSpeed.b && 0.0f == fade.mSpeed.a;
}

/**
 * @note Address: 0x8001C7B0
 * @note Size: 0xD50
 */
// NONMATCHING: CSound3D::Init takes no argument in its declaration, retail passes one (it seems, whatever is
// left in r4); and the logo and title states are loaded once for their two tests, as if they sat in objects
// of their own (CGame destroys one at _5978 and one at _5C70)
// the GameCube function with the Wii's resources: one step a frame until the stage runs, true when done
BOOL CGame::BeforeUpdate()
{
	// Ready steps fall through. Pending reads resume at the same step next frame.
	switch (mLoadStep) {
	case 0:
		return TRUE;
	case 1:
		++mLoadStep;
	case 2:
		if (GetStatusFlags(0x400000000ULL) && mLogo.mState != 12) {
			if (!IsLogoActive())
				mLogo.mState = 1;
			if (mLogo.mState <= 3)
				break;
			++mLoadStep;
		} else {
			++mLoadStep;
		}
	case 3:
		if (!mGs.mFont.LoadResource())
			break;
		++mLoadStep;
	case 4:
		if (!gGame.mSound.OpenArchiveAsync("snd/cb_robo", 0))
			break;
		++mLoadStep;
	case 5:
		if (gGame.mSound.mLoadPending)
			break;
		switch (SCGetSoundMode()) {
		case SC_SOUND_MODE_MONO:
			nw4r::snd::detail::AxManager::GetInstance().SetOutputMode(nw4r::snd::OUTPUT_MODE_MONO);
			break;
		case SC_SOUND_MODE_STEREO:
			nw4r::snd::detail::AxManager::GetInstance().SetOutputMode(nw4r::snd::OUTPUT_MODE_STEREO);
			break;
		case SC_SOUND_MODE_SURROUND:
			nw4r::snd::detail::AxManager::GetInstance().SetOutputMode(nw4r::snd::OUTPUT_MODE_DPL2);
			break;
		}
		((CSound3D*)&gGame.mSound.mSound3DManager)->Init(NULL);
		++mLoadStep;
	case 6:
		if (GetStatusFlags(0x200000000ULL)) {
			if (!mArchive.ReadEx("/cb.bin", 0, 0))
				break;
			++mLoadStep;
		} else {
			++mLoadStep;
		}
	case 7: {
		CoordInfo spawn(GetSpawnData(12)); // spawn placement 12
		if (!((XObj*)mPointer.mCursor)->LoadFromPlacement(spawn))
			break;
		((XObj*)mPointer.mCursor)->SetScale(6.0f);
		++mLoadStep;
	}
	case 8:
		if (!mTpl.ReadEx("tpl/misc.tpl", 0, 0))
			break;
		++mLoadStep;
		mTpl.mTex.Init(&mTpl.mPalette->descriptorArray[261], 0);
		mPointer.mTex = mTpl.mTex;
		_41E0         = &mPointer.mTex;
		mTpl.mTex.Init(&mTpl.mPalette->descriptorArray[122], 0); // the outline texture
		mScrnTex.mTex        = mTpl.mTex;
		mOutline.mPatternTex = &mScrnTex.mTex;
		mOutline.mFrame      = 0;
		mOutline.mThick2     = 0.0f;
	case 9:
		if (!mEffects.Load("eff/cb_robo", 0))
			break;
		++mLoadStep;
	case 10:
		if (!mEffects.UpdateLoad(0))
			break;
		++mLoadStep;
	case 11:
		if (!LoadMenuGroupResources(&mMenu.mActive))
			break;
		++mLoadStep;
	case 12:
		if (!LoadWiiHomeMenu(&mHomeMenu))
			break;
		++mLoadStep;
	case 13:
		++mLoadStep;
	case 14:
		if (!LoadStartingAttachments(this))
			break;
		++mLoadStep;
	case 15:
		if (!HideHudPointCounters(&mInfoScrn))
			break;
		++mLoadStep;
	case 16:
		if (!LoadEventResources(&mEvt))
			break;
		++mLoadStep;
	case 17:
		if (!LoadMessageCursorObject(&mMsgs[0]))
			break;
		LoadMessageCursorObject(&mMsgs[1]);
		++mLoadStep;
	case 18: {
		CoordInfo spawn(GetSpawnData(14)); // spawn placement 14
		if (!((XObj*)GetPlayer())->LoadFromPlacement(spawn))
			break;
		++mLoadStep;
	}
	case 19:
		if (GetStatusFlags(0x400000000ULL)) {
			if (mLogo.mState != 12)
				break;
			++mLoadStep;
		} else {
			++mLoadStep;
		}
		break;
	case 20:
		if (IsStageBusy(this))
			break;
		if (GetStatusFlags(0x20000000) && mTitle.mState != 23) {
			if (!IsTitleActive()) {
				ResetNewGameState();
				mTitle.mState = 1;
			}
			break;
		}
		++mLoadStep;
	case 21:
		if (IsStageBusy(this))
			break;
		if (mMemcardLoaded) { // a save restores the battery too
			ApplyWiiSaveSlot(&mSaveManager);
			mMemcardLoaded = FALSE;
			SetStatusFlags(0x8000);
			InitBatteryGauge(&mInfoScrn, GetPlayer()->mParams.mBattery);
		}
		++mLoadStep;
	case 22:
		if (IsStageBusy(this))
			break;
		mPointer._120 = -1.0f;
		++mLoadStep;
	case 23:
		if (!LoadStageAndEventObjects(this))
			break;
		++mLoadStep;
	case 24: {
		u8* script = GetStageSetupScript();
		if (script)
			StartOrQueueEventScript(&mEvt, script, 0, -1, -1);
		++mLoadStep;
	}
	case 25:
		if (mEvt.mData.mScript) // waits for the setup script
			break;
		if (CheckStageFlag(mStage, 4)) // stage flag 4 sets flag 0x40
			SetStatusFlags(0x40);
		mSetupStep = 0;
		++mLoadStep;
	case 26:
		if (!FinalizeStageObjectSetup(this))
			break;
		++mLoadStep;
		if (GetStatusFlags(0x40))
			break;
	case 27:
		ClearStatusFlags(0x40);
		mShadowFocus  = NULL;
		mShadowWidth  = 0.0f;
		mShadowHeight = 0.0f;
		mShadowAlpha  = 1.0f;
		if (GetStatusFlags(0x8000))
			RestoreSavedPlayerAndCamera(&mSaveManager);
		++mLoadStep;
	case 28:
		if (!CallStageModule(0, 0, NULL))
			break;
		ApplyStageSoundEffects(&gGame.mSound);
		++mLoadStep;
	case 29: {
		if (!FadeStopped(mFade))
			break;
		StartScreenFade(&mFade, 0, 0, 0, 255, 0, 0, 0, 0, 0.5f);
		StartOrQueueEventScript(&mEvt, GetStageEntryScript(), 0, -1, -1);
		u32 minutes = GetStageEntryTimeComponent(1), hours = GetStageEntryTimeComponent(0);
		_EC1A4 = (s32)(hours * 216000 + minutes * 3600 + GetStageEntryTimeComponent(2) * 60); // the clock, in frames
		SetStatusFlags(1);
		++mLoadStep;
	}
	}
	if (IsLoadStepRunning()) {
		// past the first steps, and the logos done or never shown: the loading screen may come up
		if (mLoadStep > 2 && (!GetStatusFlags(0x400000000ULL) || mLogo.mState == 12)) {
			if (!GetStatusFlags(0x100000000000000ULL))
				SetStatusFlags(0x100000000000000ULL);
			if (!(_41B0 & 1)) {
				_41B0 |= 7;
				_41C1 = 1;
				_41C0 = 0;
				_41C3 = 1;
			}
		}
		return FALSE;
	}
	return TRUE;
}

/**
 * @note Address: 0x8001D500
 * @note Size: 0x14
 */
u64 CBase::GetStatusFlags(u64 mask) const
{
	return mask & mStatus.mFlags;
}

/**
 * @note Address: 0x8001D5B4
 * @note Size: 0x1C
 */
void CBase::SetStatusFlags(u64 mask)
{
	mStatus.mFlags |= mask;
}

/**
 * @note Address: 0x8001D5D0
 * @note Size: 0x6B8
 */
// NONMATCHING: retail copies rot, trans, point and normal once more on the stack, so the matrix is
// built by inlines that take them by value and whose split is not known; the registers follow from that
// the step after the objects are read, the GameCube function of the same name cut into six steps
// (mSetupStep): lists the mirrors and the floor reflection, sets the player up, then points the camera,
// the lights and the fog at the stage
extern "C" BOOL FinalizeStageObjectSetup(CGame* game)
{
	FbMirror* mirror;
	s32 mirrors;
	CMirrorObj* obj;
	s32 i;
	switch (game->mSetupStep) {
	case 0:
		if (!UpdateStageObjectLoading(&game->mXObjs)) {
			break;
		}
		if (game->mResetRequested) {
			return TRUE;
		}
		game->mSetupStep++;
	case 1: {
		fn_800C7F70((void*)game->mXObjs.mPools[3].mObjects);
		game->ClearStatusFlags(0x2000000000ULL);
		game->ClearStatusFlags(0x8000000000ULL);
		game->ClearStatusFlags(0x100000000000ULL);
		game->mPointer.mFloorReflect = FALSE;
		mirror                       = game->mMirrors;
		mirrors                      = 0;
		for (i = 0; i < game->mXObjs.mObjectCount; i++) {
			obj = (CMirrorObj*)game->mXObjs.mObjects[i];
			if (obj->mStatus & 1) {
				if (!game->GetStatusFlags(0x2000000000ULL) && (obj->mStatus & 0x20000000)) {
					game->SetStatusFlags(0x2000000000ULL); // the stage wants the floor reflection
					game->mPointer.mFloorReflect = TRUE;

					Vec rot   = { 0.0f, 0.0f, 0.0f };
					Vec trans = { 0.0f, 0.0f, 0.0f };
					trans.y   = 0.0f;
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
					PSMTXReflect(game->mPointer.mFloorReflectMtx, &point, &normal);
				}
				if ((obj->mStatus & 0x400) && mirrors < 5) { // flag 0x400 makes an object a mirror
					game->SetStatusFlags(0x100000000000ULL);
					if (obj->mF0 != NULL) {
						mirror->SetMirror(obj);
						mirror++;
						mirrors++;
					}
				}
				if (!game->GetStatusFlags(0x8000000000ULL) && (obj->mStatus & 0x80000000)) {
					game->SetStatusFlags(0x8000000000ULL); // an object needs the depth check
				}
			}
		}
		game->mSetupStep++;
	}
	case 2:
		if (!fn_800CDC08(game->GetPlayer())) {
			break;
		}
		game->mSetupStep++;
	case 3:
		if (!RestorePlayerHeldItem(game->GetPlayer())) {
			break;
		}
		game->mSetupStep++;
	case 4:
		if (!LoadPlayerAttachmentAnimation(game->GetPlayer())) {
			break;
		}
		game->mSetupStep++;
	case 5: {
		game->SetupStageCamera(GetStageInfo()->mCameraMode);
		game->mLightList = NULL;
		if (gGame.mGs.mScene.mHead->_2888 == NULL) {
			game->mLightList = game->mLights;
		}
		for (i = 0; i < 9; i++) {
			gGame.mGs.mScene.mSlots[i]._18       = FALSE;
			gGame.mGs.mScene.mSlots[i].mPosSet   = FALSE;
			gGame.mGs.mScene.mSlots[i].mColorSet = FALSE;
		}
		game->SetCameraLight();
		game->mEffects.UpdateLights(gGame.mGs.mLight);
		if (gGame.mGs.mScene.mHead->_2888 != NULL) {
			nw4r::g3d::Fog fog = gGame.mGs.mScene.GetRoot()->GetFog(0);
			GXFogType type;
			f32 startZ;
			f32 endZ;
			f32 nearZ;
			f32 farZ;
			GXColor color;
			fog.GetFog(&type, &startZ, &endZ, &nearZ, &farZ, &color);
			GXColor fogColor = color;
			game->mEffects.SetFog(type, &fogColor, startZ, endZ, nearZ, farZ);
		} else {
			GXColor none = { 0, 0, 0, 0 };
			game->mFog.Init(GX_FOG_NONE, none, 0.0f, 0.0f, 0.0f, 0.0f);
			GXColor black = { 0, 0, 0, 0 };
			game->mEffects.SetFog(GX_FOG_NONE, &black, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		BuildObjectDrawList(game);
		if (game->GetStatusFlags(0x2000000000ULL)) {
			f32 fade        = GetStageInfo()->mMirrorFade / 100.0f;
			CMirrorObj* map = (CMirrorObj*)game->mXObjs.mPools[3].mObjects;
			if ((map->mStatus & 1) && fade > 0.0f && fade < 1.0f) {
				// the GameCube made the map translucent here, nothing is left of that
			}
		}
		// three reads the code does nothing with
		game->GetStatusFlags(0x2000000000ULL);
		game->GetStatusFlags(0x100000000000ULL);
		game->GetStatusFlags(0x8000000000ULL);
		game->mPointer.mHudObject       = NULL;
		game->mPointer.mHudObjectX      = 0.0f;
		game->mPointer.mHudObjectY      = 0.0f;
		game->mPointer.mScopeTarget     = NULL;
		game->mPointer.mScopeMarker     = -1;
		game->mPointer.mScopeItem       = -1;
		game->mPointer.mScopeTicks      = 0;
		game->mPointer.mScopeRareItem   = 0;
		game->mPointer.mScopeLevel      = 0;
		game->mPointer.mScopeSoundIndex = 0;
		game->mPointer.mScopeSound      = -1;
		game->mPointer.mScopeZoom       = 0.0f;
		game->mPointer.mScopeAngle      = 0.0f;
		game->mPointer.mScopeFade       = 0.0f;
		game->mPointer.mScopeFade2      = 0.0f;
	}
	}
	return game->mSetupStep == 5;
}

/**
 * @note Address: 0x8001DC88
 * @note Size: 0x1F0
 */
// points the camera and its actions at the stage's objects; mode is the stage's camera action
void CGame::SetupStageCamera(s32 mode)
{
	((CCameraActionScroll*)mCameraActions.mTable[CAMACTION_SCROLL])->mZoom = 1;
	if (GetStageCameraAreaRecords() && GetStageCameraInfoRecords()) {
		mCameraActions.mTable[CAMACTION_AREA]->Reset();
	}
	if (GetStageCameraAreaRecords() && GetStageCameraInfoRecords()) {
		((CCameraActionAreaExM*)mCameraActions.mTable[CAMACTION_AREA])->LoadAreaState();
	}
	((CCameraActionAnim*)mCameraActions.mTable[CAMACTION_ANIM])->SetObj((XObjCamera*)mXObjs.mPools[0].mObjects);
	mMotionControlCamera.Init();
	if (mode == 1 || mode == 7) { // both modes start on the map camera
		mMotionControlCamera.ChangeAction(7);
		((CCameraActionMap*)mCameraActions.mTable[CAMACTION_MAP])->Reset2();
		mMotionControlCamera.ApplyAction();
	} else {
		mMotionControlCamera.ChangeAction(mode);
	}
	for (s32 i = 0; i < mXObjs.mObjectCount; i++) {
		mMotionControlCamera.UpdateTarget((XObj*)mXObjs.mObjects[i]);
	}
	if (GetStatusFlags(0x100)) { // flag 0x100: a fresh roboeyes wipe
		ClearStatusFlags(0x300);
		mRoboeyes.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	gGame.mGs.mScene.GetRoot()->SetCurrentCamera(0);
}

/**
 * @note Address: 0x8001DE88
 * @note Size: 0x78
 */
void CBase::SetCameraLight()
{
	CCamera* camera = GetCamera();
	SetLight(GetLight(), camera->mViewMtx, TRUE);
}

/**
 * @note Address: 0x8001DF00
 * @note Size: 0x18
 */
void CBase::SetLight(CLight* light, MtxP view, BOOL flag)
{
	mGs.mLight = light;
	mGs.fn_802AB81C(view, flag);
}

/**
 * @note Address: 0x8001DF18
 * @note Size: 0x10
 */
CLight* CBase::GetLight()
{
	return GetDefaultLight();
}

/**
 * @note Address: 0x8001DF28
 * @note Size: 0x8
 */
// the member behind the two cameras; cgame.h does not type that block yet
CLight* CBase::GetDefaultLight()
{
	return mLights;
}

/**
 * @note Address: 0x8001DF30
 * @note Size: 0x10
 */
CCamera* CBase::GetCamera()
{
	return GetPerspectiveCamera();
}

/**
 * @note Address: 0x8001DF40
 * @note Size: 0x8
 */
CCamera* CBase::GetPerspectiveCamera()
{
	return &mCameras[0];
}

/**
 * @note Address: 0x8001DF48
 * @note Size: 0x280
 */
// NONMATCHING: the object and the loop's table pointer swap registers in both loops (r6/r7)
// lists the objects in draw order: the ones with flag 0x4000 first, then the rest, then the player and
// last the first object of pool 0. Each entry keeps the object's slot in mXObjs.mObjects
extern "C" void BuildObjectDrawList(CGame* game)
{
	s32 i;
	for (i = 0; i < 0x400; i++) {
		game->mDrawLists[0].mRefs[i].mObj = NULL;
	}
	game->mDrawLists[0].mCount = 0;
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		if (game->mXObjs.mObjects[i] != (XObj*)game->GetPlayer() && game->mXObjs.mObjects[i] != (XObj*)game->mXObjs.mPools[0].mObjects
		    && (game->mXObjs.mObjects[i]->mStatus.mFlags & 1) && (game->mXObjs.mObjects[i]->mStatus.mFlags & 0x4000)) {
			game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mObj   = game->mXObjs.mObjects[i];
			game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mIndex = i;
			game->mDrawLists[0].mCount++;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		if (game->mXObjs.mObjects[i] != (XObj*)game->GetPlayer() && game->mXObjs.mObjects[i] != (XObj*)game->mXObjs.mPools[0].mObjects
		    && (game->mXObjs.mObjects[i]->mStatus.mFlags & 1) && !(game->mXObjs.mObjects[i]->mStatus.mFlags & 0x4000)) {
			game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mObj   = game->mXObjs.mObjects[i];
			game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mIndex = i;
			game->mDrawLists[0].mCount++;
		}
	}
	if (((XObj*)game->GetPlayer())->mStatus.mFlags & 1) {
		game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mObj   = game->GetPlayer();
		game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mIndex = 0;
		game->mDrawLists[0].mCount++;
	}
	if (((XObj*)game->mXObjs.mPools[0].mObjects)->mStatus.mFlags & 1) {
		game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mObj   = game->mXObjs.mPools[0].mObjects;
		game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mIndex = 0;
		game->mDrawLists[0].mCount++;
	}
}

/**
 * @note Address: 0x8001E1C8
 * @note Size: 0x668
 */
// sorts the objects into the ones to draw and the ones mirrors show, those of pool 3 first, then the plain
// ones, then the 0xA00000 ones, then the 0x40 ones. Objects out of view are only mirrored
extern "C" void BuildSceneDrawLists(CGame* game)
{
	XObj* obj;
	s32 i;
	if (game->GetStatusFlags(0x8000000000ULL)) { // only while flag 0x8000000000 stands
		CMotionControlCamera* camera = &game->mMotionControlCamera;
		for (i = 0; i < game->mXObjs.mObjectCount; i++) {
			obj = (XObj*)game->mXObjs.mObjects[i];
			if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
				if (!game->GetStatusFlags(0x100) && (obj->mStatus.mFlags & 0x80000000) && obj->IsOutsideCameraDepth(&camera->mCamera)) {
					obj->mStatus.mFlags |= 0x1000000000000ULL; // out of depth: mirror-only
				} else {
					obj->mStatus.mFlags &= ~0x1000000000000ULL;
				}
			}
		}
	}

	for (i = 0; i < 0x400; i++) {
		game->mDrawLists[2].mRefs[i].mObj = NULL;
	}
	game->mDrawLists[2].mCount = 0;
	for (i = 0; i < 0x400; i++) {
		game->mDrawLists[3].mRefs[i].mObj = NULL;
	}
	game->mDrawLists[3].mCount = 0;

	for (i = 0; i < game->mXObjs.mPools[3].mUsed; i++) {
		obj = game->mXObjs.mPools[3].Get(i);
		if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && !(obj->mStatus.mFlags & 0xA00040)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000ULL)) {
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mObj   = obj;
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mIndex = 0;
				game->mDrawLists[2].mCount++;
			}
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mObj   = obj;
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mIndex = 0;
			game->mDrawLists[3].mCount++;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = (XObj*)game->mXObjs.mObjects[i];
		if (obj->GetPool() != 3 && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && !(obj->mStatus.mFlags & 0xA00040)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000ULL)) {
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mObj   = obj;
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mIndex = 0;
				game->mDrawLists[2].mCount++;
			}
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mObj   = obj;
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mIndex = 0;
			game->mDrawLists[3].mCount++;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = (XObj*)game->mXObjs.mObjects[i];
		if (obj->GetPool() != 3 && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && (obj->mStatus.mFlags & 0xA00000)
		    && !(obj->mStatus.mFlags & 0x40)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000ULL)) {
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mObj   = obj;
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mIndex = 0;
				game->mDrawLists[2].mCount++;
			}
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mObj   = obj;
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mIndex = 0;
			game->mDrawLists[3].mCount++;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = (XObj*)game->mXObjs.mObjects[i];
		if (obj->GetPool() != 3 && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && (obj->mStatus.mFlags & 0x40)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000ULL)) {
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mObj   = obj;
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mIndex = 0;
				game->mDrawLists[2].mCount++;
			}
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mObj   = obj;
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mIndex = 0;
			game->mDrawLists[3].mCount++;
		}
	}
}

// a select() is up in the first message window
static inline bool IsMessageSelecting(CMsg* msg)
{
	bool selecting = false;
	if (msg->mData.mCursor != NULL && msg->mFont.mSelectedLine >= 0) {
		selecting = true;
	}
	return selecting;
}

static inline BOOL IsMenuOpen(CMenu* menu)
{
	return menu != NULL ? menu->IsOpen() : FALSE;
}

// chibi robo stands still enough for the timer's script: no action running, or only a lift that
// is done or holds one of a few poses
static inline bool IsPlayerIdle(WiiPlayerState* player)
{
	s32 lift  = player->mLift.mState;
	bool free = true;
	if (!(lift == 0 || lift == 4 || lift == 0x1D || lift == 0x21 || lift == 0x23)) {
		bool posing = false;
		if (lift == 2 && ((XObj*)player)->GetAnimId() == 0x159) {
			posing = true;
		}
		if (!posing) {
			free = false;
		}
	}
	return free && player->mHang.mState == 0 && player->mJump.mState == 0 && player->mLadder.mState == 0 && player->mRope.mState == 0
	    && player->mPush.mState == 0 && player->mTumble.mState == 0;
}

// what the pointer is on and within the popper's reach, the HUD shows it
struct PointerPick {
	f32 mDist;
	XObj* mObj;
};

// what the scope picks: the object nearest to the eye camera among those the pointer is on
struct ScopePick {
	f32 mDist;
	XObj* mObj;
	f32 mSpeed; // how fast the pointer moved
	BOOL mOn;   // the pointer is on it, not only swept across it
	s32 mLevel; // 1 to 3 by the speed
};

// the pointer as Update hands it to the objects
struct PointerMotion {
	const Vec* mPos;
	Vec mMove;  // its move on the screen this frame, in screen widths and heights
	f32 mSpeed; // the move's length in screen widths
};

// the sounds a stage's items play under the scope, mCount for each of the three levels
struct ScopeStageSounds {
	const s32* mSounds;
	s32 mCount;
};
extern const ScopeStageSounds lbl_802C49C8[];

extern s32 gScreenLeft, gScreenRight, gScreenTop, gScreenBottom;
extern s32 gPointerScreenWidth, gPointerScreenHeight;

// the scope takes obj if nothing keeps it from looking and it is nearer to the eye than its pick so far.
// Retail has these lines twice, the name is ours
static inline void PickScopeTarget(CGame* game, ScopePick* scope, XObj* obj, XObj* held, const Vec* eye, const PointerMotion& motion,
                                   BOOL on)
{
	if (!game->GetStatusFlags(0x100) && game->GetPlayer()->GetAttachment() != 2
	    && !(game->mMotionControlCamera.IsAction(CAMACTION_AREA) && ((CCameraActionArea*)game->mCameraActions.mTable[CAMACTION_AREA])->_2D0
	         && ((CCameraActionArea*)game->mCameraActions.mTable[CAMACTION_AREA])->mControlEnabled != 0)
	    && !(game->mMotionControlCamera.IsAction(CAMACTION_MAP)
	         && ((CCameraActionMap*)game->mCameraActions.mTable[CAMACTION_MAP])->mPointerHeld)
	    && (obj->_18C >= 0 || (obj->GetPool() == 8 && obj != held))) {
		f32 dist = PSVECDistance(obj->GetPos(), eye) - obj->mFloorXZExtent[0];
		if (scope->mObj == NULL || dist < scope->mDist) {
			scope->mDist  = dist;
			scope->mObj   = obj;
			scope->mSpeed = motion.mSpeed;
			scope->mOn    = on;
		}
	}
}

// the eye camera's popper aims along the ray through the pointer. Retail has these lines twice, the name
// is ours
static inline void AimAtPointer(CGame* game)
{
	CCamera* camera = game->GetCamera();
	Vec pointer     = gWiiPointer.mPointer;
	CVec screen(pointer);
	CVec world;
	Vec clip;
	clip.x = screen.x / camera->mScale;
	clip.y = screen.y;
	clip.z = screen.z;
	PSMTX44MultVec(camera->mScreenInv, &clip, &world);
	CVec from = world;
	CVec to   = world + (world - camera->mView.mEye);
	f32 yaw   = atan2(to.x - from.x, to.z - from.z);
	f32 dz    = to.z - from.z;
	f32 dx    = to.x - from.x;
	f32 dy    = to.y - from.y;
	f32 pitch = atan2(FSqrt(dx * dx + dz * dz), FSqrt(dy * dy));
	fn_800E8140(game->GetPlayer(), yaw, dy < 0.0f ? HALF_PI - pitch : pitch - HALF_PI);
}

// two sets of items the scope marks, each a bit mask over a run of item indices; the names are ours
static inline bool IsScopeRareItemA(s32 item)
{
	bool rare = false;
	if ((u32)(item - 0x12) <= 0x22 && (0x780000003ULL & (1ULL << (item - 0x12)))) {
		rare = true;
	}
	return rare;
}

static inline bool IsScopeRareItemB(s32 item)
{
	bool rare = false;
	if ((u32)(item - 0x5B) <= 0x2F && (0xF00400000001ULL & (1ULL << (item - 0x5B)))) {
		rare = true;
	}
	return rare;
}

static inline bool IsScopeRareItem(s32 item)
{
	return IsScopeRareItemA(item) || IsScopeRareItemB(item);
}

/**
 * @note Address: 0x8001E830
 * @note Size: 0x57C4
 */
// NONMATCHING: retail also expands fn_80114564; the remaining differences are register and stack placement
// the GameCube function with the Wii's pointer, home menu, movie and sound pauses
void CGame::Update(u32 frames)
{
	s32 i;
	gWiiPointer.Update(frames);
	if (!mMovieOnThread && !(GetStatusFlags(1) && IsSystemMessage3Shown())) {
		if (mMovie.mState != 0 && !mHomeMenu.IsOpen() && !mResetRequested && !GetStatusFlags(0x200000000000002ULL)) {
			s32 result = mMovie.Update();
			if (result < 0) {
				mMovie.Stop();
				OSPanic("game.cpp", 0x1697, FormatToStaticBuffer("error on play movie! - %d\n", result));
			}
		}
	}
	UpdateSystemMessage();
	UpdateDvdStatus();
	if (!mResetRequested) {
		BOOL pressed = OSGetResetButtonState();
		if (gPowerButtonPressed) {
			mResetRequested = true;
			mResetType      = 2;
		} else if (pressed) {
			mResetRequested = true;
			mResetType      = 3;
		}
	}

	if (gWiiPointer.mPointerValid) {
		mPointer.mScopeTimer = 0.0f;
	} else if (mPointer._120 >= 0.0f) {
		if (mPointer._120 < 1.0f) {
			if ((mPointer._120 += 0.1f * frames) > 1.0f) {
				mPointer._120 = 1.0f;
			}
		}
	} else if (mPointer.mScopeTimer < 5.0f) {
		if ((mPointer.mScopeTimer += 2.0f * (1.0f / gRefreshRate) * frames) > 5.0f) {
			mPointer.mScopeTimer = 5.0f;
		}
	}

	// _EC1A8 fades the pointer's cursor: out while the eye camera aims the radar, half way while nothing
	// else wants the pointer, in while something does
	if (mMotionControlCamera.IsAction(CAMACTION_EYE) && GetPlayer()->GetAttachment() == 2 && !mMenu.mActive && !GetStatusFlags(0x800)) {
		if (mPointer.mAlpha > 0.0f) {
			if ((mPointer.mAlpha -= 0.05f * frames) < 0.0f) {
				mPointer.mAlpha = 0.0f;
			}
		}
	} else if (mEvt.mData.mScript == NULL && !IsMessageSelecting(GetMsg(0)) && !GetStatusFlags(0x800) && !IsMenuOpen(mMenu.mMenus[1])
	           && !IsMenuOpen(mMenu.mMenus[2]) && !IsMenuOpen(mMenu.mMenus[4])
	           && !(mMotionControlCamera.IsAction(CAMACTION_AREA) && ((CCameraActionArea*)mCameraActions.mTable[CAMACTION_AREA])->_2D0
	                && ((CCameraActionArea*)mCameraActions.mTable[CAMACTION_AREA])->mControlEnabled != 0)
	           && !(mMotionControlCamera.IsAction(CAMACTION_MAP) && ((CCameraActionMap*)mCameraActions.mTable[CAMACTION_MAP])->mPointerHeld)
	           && !(fn_80169658(&mMotionControlCamera) && fn_80169674()->mPointerHeld)
	           && !(mStage == 3 && CallStageModule(0x10, 0, NULL))) {
		if (mPointer.mAlpha > 0.5f) {
			if ((mPointer.mAlpha -= 0.1f * frames) < 0.5f) {
				mPointer.mAlpha = 0.5f;
			}
		}
	} else if (mPointer.mAlpha < 1.0f) {
		if ((mPointer.mAlpha += 0.1f * frames) > 1.0f) {
			mPointer.mAlpha = 1.0f;
		}
	}

	// _EC2B4 counts down how long the pointer may stay off the screen before its cursor hides (flag 8 of
	// _41B0); NAN while nothing on screen wants the pointer
	bool wanted = false;
	if (GetStatusFlags(1)) {
		if ((!(mMenuControls & 2) && GetStatusFlags(0x400) && mEvt.mData.mScript == NULL) || (!(mMenuControls & 2) && GetStatusFlags(0x800))
		    || (!(mMenuControls & 2) && (IsMenuOpen(mMenu.mMenus[1]) || IsMenuOpen(mMenu.mMenus[2]) || IsMenuOpen(mMenu.mMenus[4])))
		    || (mMotionControlCamera.IsAction(CAMACTION_EYE) && mEvt.mData.mScript == NULL && (!(mMenuControls & 2) || !mMenu.mActive))
		    || mMotionControlCamera.IsAction(CAMACTION_SCROLL2)
		    || (mMotionControlCamera.IsAction(CAMACTION_AREA) && ((CCameraActionArea*)mCameraActions.mTable[CAMACTION_AREA])->_2D0
		        && ((CCameraActionArea*)mCameraActions.mTable[CAMACTION_AREA])->mControlEnabled != 0)
		    || (mMotionControlCamera.IsAction(CAMACTION_MAP) && ((CCameraActionMap*)mCameraActions.mTable[CAMACTION_MAP])->mPointerHeld)
		    || (fn_80169658(&mMotionControlCamera) && fn_80169674()->mPointerHeld) || (mStage == 3 && CallStageModule(0x10, 0, NULL))) {
			wanted = true;
		}
	}
	if (wanted) {
		if (gWiiPads[0].mSamples[0].pointerValid >= 2) {
			if (_41B0 & 8) {
				_41B0 &= ~8;
			}
			if (__fpclassifyf(mPointer._10C) == FP_NAN || mPointer._10C < gRefreshRate * 5) {
				mPointer._10C = gRefreshRate * 5;
			}
		} else if (__fpclassifyf(mPointer._10C) == FP_NAN) {
			mPointer._10C = 0.0f;
		}
		if (!(_41B0 & 8)) {
			if (mPointer._10C > 0.0f) {
				if ((mPointer._10C -= frames) < 0.0f) {
					mPointer._10C = 0.0f;
				}
			}
			if (0.0f == mPointer._10C) {
				_41B0 |= 8;
			}
		}
	} else {
		if (_41B0 & 8) {
			_41B0 &= ~8;
		}
		if (__fpclassifyf(mPointer._10C) != FP_NAN) {
			mPointer._10C = NAN;
		}
	}

	UpdateWiiSaveJob(&mSaveManager);
	UpdateResetRequest(this);
	if (IsDiscError()) { // flag 8: the sounds pause while the disc can't be read
		if (!GetStatusFlags(8)) {
			SetStatusFlags(8);
			PauseGameSounds(this, 0.0f);
			UpdateGameSound(&gGame.mSound, frames);
		}
		if (gGame.mGs.mScene._04) {
			WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
			scene->vf034(scene->mPending, mXObjs.mPools[3].Get(0)->mObject.m_pScnMdl);
			StepSceneDraws(frames);
		}
		UpdateGameSound(&gGame.mSound, frames);
		return;
	}
	if (GetStatusFlags(8)) {
		ClearStatusFlags(8);
		ResumeGameSounds(this, 0.0f);
	}
	if (GetStatusFlags(1) || (mTitle.mState >= 11 && mTitle.mState < 23)) {
		if (IsSystemMessage3Shown()) { // the message that the remote is gone stops the game
			if (!GetStatusFlags(0x800000000000000ULL)) {
				SetStatusFlags(0x800000000000000ULL);
				PauseGameSounds(this, 0.0f);
				UpdateGameSound(&gGame.mSound, frames);
			}
			if (gGame.mGs.mScene._04) {
				WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
				scene->vf034(scene->mPending, mXObjs.mPools[3].Get(0)->mObject.m_pScnMdl);
				StepSceneDraws(frames);
			}
			UpdateWiiHomeMenu(&mHomeMenu, frames);
			UpdateGameSound(&gGame.mSound, frames);
			return;
		}
		if (GetStatusFlags(0x800000000000000ULL)) {
			ClearStatusFlags(0x800000000000000ULL);
			ResumeGameSounds(this, 0.0f);
		}
	}

	if (GetStatusFlags(0x1000)) { // flag 0x1000 queues a draw-list rebuild
		ClearStatusFlags(0x1000);
		BuildObjectDrawList(this);
	}
	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL)) {
		if (!GetStatusFlags(0x20) && !mMenu.mActive && mEvt.mData.mScript == NULL) {
			if (gWiiPointer.mFlags & 0x400) {
				fn_80084B90(&mMenu.mActive, 1);
			} else if (gWiiPointer.mFlags & 0x80) {
				fn_80084B90(&mMenu.mActive, 1);
			}
		}
		GetStatusFlags(0x20);
	}
	if (!mHomeMenu.IsOpen()) {
		for (i = 0; i < 4; i++) {
			if (gWiiPads[i].mTrigger & 0x800000400ULL) { // HOME on the remote or the classic controller
				OpenWiiHomeMenu(&mHomeMenu);
				break;
			}
		}
	}

	for (i = 0; i < 0x400; i++) {
		mDrawLists[1].mRefs[i].mObj = NULL;
	}
	mDrawLists[1].mCount = 0;
	if (!IsLoadStepRunning()) {
		if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
			for (i = 0; i < mDrawLists[0].mCount; i++) {
				XObj* obj = (XObj*)mDrawLists[0].mRefs[i].mObj;
				if ((obj->mStatus.mFlags & 1) && obj->_188 && obj->mBaseMtxPtr == NULL) {
					obj->vf15C();
					obj->UpdateObject(frames);
					obj->vf160();
				}
			}
		} else if (GetStatusFlags(1) && GetStatusFlags(2)) { // the game paused: his looping sounds stop
			if (((XObj*)GetPlayer())->GetAnimId() == 0x12 || ((XObj*)GetPlayer())->GetAnimId() == 0xCFA
			    || ((XObj*)GetPlayer())->GetAnimId() == 0xCFD) {
				if (IsWiiSfxPlaying(&gGame.mSound, 0x79)) {
					StopWiiSfx(&gGame.mSound, 0x79, 0.0f);
				}
			}
			if (IsWiiSfxPlaying(&gGame.mSound, 0x11D)) {
				StopWiiSfx(&gGame.mSound, 0x11D, 0.0f);
			}
			if (IsWiiSfxPlaying(&gGame.mSound, 0x75)) {
				StopWiiSfx(&gGame.mSound, 0x75, 0.0f);
			}
			if (IsWiiSfxPlaying(&gGame.mSound, 0x1F3)) {
				StopWiiSfx(&gGame.mSound, 0x1F3, 0.0f);
			}
			if (IsWiiSfxPlaying(&gGame.mSound, 0x1F8)) {
				StopWiiSfx(&gGame.mSound, 0x1F8, 0.0f);
			}
			for (i = 0; i < 2; i++) {
				if (((AnimSfx*)GetPlayer()->mSfx)->mActors[i].GetHandle() != NULL) {
					((AnimSfx*)GetPlayer()->mSfx)->mActors[i].Stop(0.0f);
				}
			}
		}
	}
	if (IsRunning() && !GetStatusFlags(0x200000000000002ULL)) {
		mPlayTime += frames * (60.0f / gRefreshRate);
		if (mPlayTime >= 21600000.0f) { // caps the play time at a hundred hours of frames
			mPlayTime = 21599999.0f;
		}
	}
	if (GetStatusFlags(0x2000000000000ULL)) { // flag 0x2000000000000 arms the stage timer
		if (IsRunning() && mEvt.mData.mScript == NULL && !mMenu.mActive && !GetStatusFlags(0xC00) && mTimeState.mDuration > 0.0f) {
			if (!mTimeState.mRunning) {
				mTimeState.mTime = frames * (mTimeState.mDuration / (600.0f * gRefreshRate)) + mTimeState.mTime;
				if (mTimeState.mTime >= 1.0f) { // a full timer runs down
					mTimeState.mRunning = TRUE;
				}
			}
			if (mTimeState.mRunning == TRUE && IsPlayerIdle(GetPlayer())) { // then waits for an idle player
				mTimeState.mRunning = FALSE;
				u8* script          = GetStageTimerExpiredScript();
				if (script != NULL) {
					StartOrQueueEventScript(&mEvt, script, 0, -1, -1);
				}
			}
		}
	}
	if (!GetStatusFlags(0x200000000000002ULL)) {
		CEvt_Update(&mEvt, frames);
		for (i = 0; i < 2; i++) {
			CMsg_Update(&mMsgs[i], frames);
		}
	}
	if (IsRunning()) {
		CallStageModule(1, frames, NULL);
	}
	UpdateWiiHomeMenu(&mHomeMenu, frames);
	if (!GetStatusFlags(0x200000000000002ULL)) {
		UpdateScreenFade(&mFade, frames);
		mWipeCircle.Update(frames);
		mWipeTex.Update(frames);
		mWipeCustom.Update(frames);
		mWipeTile.Update(frames);
		mWipeCine.Update(frames);
		if (GetStatusFlags(1)) {
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
	} else if (mMotionControlCamera.IsAction(CAMACTION_SCROLL2)) {
		UpdateScreenFade(&mFade, frames);
		mWipeCircle.Update(frames);
		mWipeTex.Update(frames);
		mWipeCustom.Update(frames);
		mWipeTile.Update(frames);
		mWipeCine.Update(frames);
	}
	UpdateScreenFade(&mColor1.mColor, frames);
	if (mOutlineWidthControl > 0) { // the c-stick sets the outline width
		if (gWiiPads[0].mSubStick.y > 0.0f) {
			if ((mOutlineWidth -= 0.01f * frames) < 0.04f) {
				mOutlineWidth = 0.04f;
			}
		} else if (gWiiPads[0].mSubStick.y < 0.0f) {
			if ((mOutlineWidth += 0.01f * frames) > 0.5f) {
				mOutlineWidth = 0.5f;
			}
		}
	}
	if (!mHomeMenu.IsOpen()) {
		UpdateBootLogos(&mLogo, frames);
		UpdateTitleScreen(&mTitle, frames);
		if (!GetStatusFlags(0x200000000000000ULL)) {
			UpdateOpenMenus(&mMenu.mActive, frames);
		}
	}
	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL)) {
		UpdateHudTransitionsAndIndicators(&mInfoScrn, frames);
		((CFbTexEff*)&mSubwin)->Update(frames);
		for (i = 0; i < 5; i++) {
			FbMirror* mirror = &mMirrors[i];
			bool off         = true;
			if (mirror->mJoint != NULL && !(mirror->mJoint->mStatus & 0x1000000000000ULL)) {
				off = false;
			}
			if (!off) {
				mirror->Update(frames);
			}
		}
	}
	if (!GetStatusFlags(0x200000000000002ULL)) {
		UpdateStageNameBubbles(this, frames);
	}

	// the 3D sound listens from a little in front of the camera, or of the point mListener.mPoint names
	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
		CCamera* camera = GetCamera();
		if (__fpclassifyf(mListener.mPoint.x) != FP_NAN) {
			CVec dir = camera->mView.mEye - mListener.mPoint;
			PSVECNormalize(&dir, &dir);
			mListener.mPos = mListener.mPoint + dir * 50.0f;
		} else {
			CVec dir = camera->mView.mInterest - camera->mView.mEye;
			PSVECNormalize(&dir, &dir);
			mListener.mPos = camera->mView.mEye + dir * 50.0f;
		}
		((CSound3D*)&gGame.mSound.mSound3DManager)->SetListener(GetCamera()->mView.mAngle.y, &mListener.mPos);
	}
	UpdateGameSound(&gGame.mSound, frames);
	for (i = 0; i < mDrawLists[1].mCount; i++) {
		XObj* obj = (XObj*)mDrawLists[1].mRefs[i].mObj;
		if (obj->mStatus.mFlags & 1) {
			gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, obj->mObject.m_pScnMdl);
		}
	}
	if (IsLoadStepRunning() && mLoadStep > 20 && (((XObj*)mPointer.mCursor)->mStatus.mFlags & 1)) { // the loading screen's robot
		XObj* robot = (XObj*)mPointer.mCursor;
		if (robot->GetAnimId() != 1 && robot->IsAnimDone()) {
			robot->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		robot->SetPos(gWiiPointer.mPointer.x, gWiiPointer.mPointer.y, 50.0f);
		robot->UpdateObject(frames);
		gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, robot->mObject.m_pScnMdl);
	}
	StepSceneDraws(frames);

	s32 count = mDrawLists[1].mCount;
	if (!IsLoadStepRunning() && GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
		for (i = 0; i < mDrawLists[0].mCount; i++) {
			XObj* obj = (XObj*)mDrawLists[0].mRefs[i].mObj;
			if (obj->mStatus.mFlags & 1) {
				if (obj->_188 && obj->mBaseMtxPtr == NULL) {
					obj->vf16C(frames);
				} else {
					obj->vf15C();
					obj->UpdateObject(frames);
					obj->vf160();
				}
			}
		}
		fn_800D1A8C(GetPlayer(), frames);
		fn_800D1AF8(GetPlayer(), frames);
	}
	for (i = count; i < mDrawLists[1].mCount; i++) { // the ones the objects listed while they ran
		XObj* obj = (XObj*)mDrawLists[1].mRefs[i].mObj;
		if ((obj->mStatus.mFlags & 1) && !(obj->_188 && obj->mBaseMtxPtr == NULL)) {
			gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, obj->mObject.m_pScnMdl);
		}
	}
	StepSceneDraws(frames);
	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
		fn_800D1BF0(GetPlayer(), frames);
	}
	StepSceneDraws(frames);
	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
		fn_800D1B64(GetPlayer(), frames);
	}
	StepSceneDraws(frames);
	if (!GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
		fn_800A13D0(&lbl_8044EE4C, GetCamera());
		fn_800A13D0(mEvtPtcl.mEffects, GetCamera());
	}

	// the effects follow the camera: the stage camera object's when its scene brings one, else the game's
	if (IsRunning() || (GetStatusFlags(0x20) && !GetStatusFlags(0x200000000000002ULL))) {
		Vec eye;
		MtxP viewMtx;
		MtxP projection;
		f32 near;
		f32 far;
		XObj* camera = mXObjs.mPools[0].Get(0);
		if (camera != NULL && (camera->mStatus.mFlags & 1) && ((XObjScnCam*)camera)->mScene != NULL
		    && ((XObjScnCam*)camera)->mScene->_2888 != NULL) {
			nw4r::g3d::ScnRoot* scene      = (nw4r::g3d::ScnRoot*)((XObjScnCam*)camera)->mScene;
			MtxP handle                    = scene->GetCurrentCamera();
			nw4r::g3d::Camera& sceneCamera = *(nw4r::g3d::Camera*)&handle;
			sceneCamera.GetPosition((nw4r::math::VEC3*)&eye);
			handle = scene->GetCurrentCamera();
			if (sceneCamera.ptr() == NULL) {
				nw4r::db::Panic("g3d_rescommon_ac.h", 0x8C, "NW4R:Pointer must not be NULL (mpData)");
			}
			viewMtx = sceneCamera.ref().cameraMtx;
			handle  = scene->GetCurrentCamera();
			if (sceneCamera.ptr() == NULL) {
				nw4r::db::Panic("g3d_rescommon_ac.h", 0x8C, "NW4R:Pointer must not be NULL (mpData)");
			}
			projection = sceneCamera.ref().projMtx;
			handle     = scene->GetCurrentCamera();
			if (sceneCamera.ptr() == NULL) {
				nw4r::db::Panic("g3d_rescommon_ac.h", 0x8C, "NW4R:Pointer must not be NULL (mpData)");
			}
			near   = sceneCamera.ref().projNear;
			handle = scene->GetCurrentCamera();
			if (sceneCamera.ptr() == NULL) {
				nw4r::db::Panic("g3d_rescommon_ac.h", 0x8C, "NW4R:Pointer must not be NULL (mpData)");
			}
			far = sceneCamera.ref().projFar;
		} else {
			CCamera camera = *GetCamera();
			eye            = camera.mView.mEye;
			viewMtx        = camera.mViewMtx;
			projection     = camera.mProjection;
			near           = camera.mNear;
			far            = camera.mFar;
		}
		for (u32 j = 0; j < 6; j++) {
			if (j != 4) {
				mEffects.Update(&eye, viewMtx, projection, frames, j, near, far);
			}
		}
		CCamera screen = *GetCamera();
		eye            = screen.mView.mEye;
		mEffects.Update(&eye, screen.mViewMtx, screen.mProjection, frames, 4, screen.mNear, screen.mFar);
	} else if (mResetRequested) {
		mEffects.UpdateLoad(0);
		mEffects.UpdateLoad(1);
	}

	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000000ULL) && !mMenu.mActive && !mHomeMenu.IsOpen()) {
		mMotionControlCamera.Update(frames);
	}
	if (!mHomeMenu.IsOpen()) {
		XObj* camera = mXObjs.mPools[0].Get(0);
		if (camera != NULL && (camera->mStatus.mFlags & 1) && ((XObjScnCam*)camera)->mScene != NULL
		    && ((XObjScnCam*)camera)->mScene->_2888 != NULL) {
			fn_800C7764(camera, GetCamera());
		}
	}
	if (BeginSceneCamera()) {
		vf54();
	}
	for (i = 0; i < mDrawLists[1].mCount; i++) {
		XObj* obj = (XObj*)mDrawLists[1].mRefs[i].mObj;
		if (obj->mStatus.mFlags & 1) {
			fn_800B4988(obj);
		}
	}

	// the pointer picks what it is on: the nearest such object within the popper's reach for the HUD,
	// and the nearest to the eye camera for the scope's sounds
	PointerPick pick = { NAN, NULL };
	ScopePick scope  = { NAN, NULL, 0.0f, FALSE, 0 };
	if (gWiiPointer.mPointerValid && GetStatusFlags(1) && (GetPlayer()->mPlayerFlags & 0x20000000) && !GetPlayer()->mMenu.IsOpen()
	    && mEvt.mData.mScript == NULL && !IsLoadStepRunning()) {
		// in tenths: popper parameter 0 is the range, 1 the range with the upgrade
		f32 range = (s32)GetPopperParameter(FindOwnedInventoryItemIndex(GetPlayer()->mParams.mInventory.mItems, 0x36) >= 0) / 10.0f;
		const Vec* pointer   = &gWiiPointer.mPointer;
		PointerMotion motion = { pointer };
		if (gWiiPads[0].IsRemoteActive()) {
			motion.mMove.x = gWiiPads[0].mSamples[0].pointer.velocity.x;
			motion.mMove.y = gWiiPads[0].mSamples[0].pointer.velocity.y;
			motion.mSpeed  = gWiiPads[0].mSamples[0].pointer.speed;
			if (motion.mPos->x >= gScreenLeft && motion.mPos->x <= gScreenRight && motion.mPos->y >= gScreenTop
			    && motion.mPos->y <= gScreenBottom) {
				Vec zero = { 0.0f, 0.0f, 0.0f };
				CVec center(zero);
				CVec from(lbl_80444E98);
				Vec toPos = { motion.mPos->x, motion.mPos->y, 0.0f };
				CVec to(toPos);
				CVec eye = GetCamera()->mView.mEye;
				CVec muzzle(fn_8010BEE0(&GetPlayer()->mPopper));
				XObj* held = GetPlayer()->mLift.mState == 0 ? NULL : GetPlayer()->mLift.mTarget;
				for (i = 0; i < mDrawLists[1].mCount; i++) {
					XObj* obj = (XObj*)mDrawLists[1].mRefs[i].mObj;
					if (obj == NULL) {
						continue;
					}
					if ((obj->mStatus.mFlags & 0x10001) != 0x10001 || !(obj->mStatus.mFlags & 0x100)
					    || (obj->mStatus.mFlags & 0x1000000000800ULL)) {
						continue;
					}
					bool on = false;
					f32 y   = motion.mPos->y;
					f32 x   = motion.mPos->x;
					if (obj->mScreenBoundsValid && __fpclassifyf(obj->mScreenLeft) != FP_NAN && __fpclassifyf(obj->mScreenTop) != FP_NAN
					    && __fpclassifyf(obj->mScreenRight) != FP_NAN && __fpclassifyf(obj->mScreenBottom) != FP_NAN
					    && x >= obj->mScreenLeft && x <= obj->mScreenRight && y >= obj->mScreenTop && y <= obj->mScreenBottom) {
						on = true;
					}
					if (on) {
						obj->vf208(motion.mPos, &motion.mMove, TRUE, motion.mSpeed);
						if (GetPlayer()->GetAttachment() == 2 && GetPlayer()->mHang.mState == 0 && GetPlayer()->mJump.mState == 0
						    && GetPlayer()->mLadder.mState == 0 && GetPlayer()->mRope.mState == 0 && GetPlayer()->mPush.mState == 0
						    && GetPlayer()->mTumble.mState == 0 && !(obj->mStatus.mFlags & 0x40000) && obj->_18C != 1 && obj->_18C != 3
						    && obj->_18C != 4 && obj->_18C != 10 && obj->_18C != 0x15 && obj->GetKind() != 0x324) {
							f32 dist = PSVECDistance(&muzzle, &obj->mToppleOffset) - obj->mFloorXZExtent[0];
							// a 0x46F before a 0x6B wins regardless of the order, a 0x6B never displaces a 0x46F
							if (obj->GetKind() == 0x46F && pick.mObj != NULL && pick.mObj->GetKind() == 0x6B) {
								if (dist < range) {
									pick.mDist = dist;
									pick.mObj  = obj;
								}
							} else if (!(obj->GetKind() == 0x6B && pick.mObj != NULL && pick.mObj->GetKind() == 0x46F)) {
								if ((pick.mObj == NULL || dist < pick.mDist) && dist < range) {
									pick.mDist = dist;
									pick.mObj  = obj;
								}
							}
						}
						PickScopeTarget(this, &scope, obj, held, &eye, motion, TRUE);
					} else {
						// off it: the pointer may still have crossed its rectangle since the last frame
						bool crossed = false;
						if (obj->mScreenBoundsValid) {
							f32 left   = obj->mScreenLeft;
							f32 right  = obj->mScreenRight;
							f32 top    = obj->mScreenTop;
							f32 bottom = obj->mScreenBottom;
							if (__fpclassifyf(left) != FP_NAN && __fpclassifyf(top) != FP_NAN && __fpclassifyf(right) != FP_NAN
							    && __fpclassifyf(bottom) != FP_NAN) {
								f32 width  = right - left;
								f32 height = bottom - top;
								center.x   = width * 0.5f + left;
								center.y   = height * 0.5f + top;
								if ((center.x - from.x) * (center.x - to.x) < 0.0f) {
									Vec path, toCenter, normal, side, offset;
									PSVECSubtract(&from, &to, &path);
									PSVECSubtract(&from, &center, &toCenter);
									PSVECCrossProduct(&toCenter, &path, &normal);
									PSVECCrossProduct(&path, &normal, &side);
									PSVECNormalize(&side, &side);
									PSVECSubtract(&from, &center, &offset);
									if (PSVECDotProduct(&side, &offset) < ((height - width) * 0.5f + width) * 0.5f
									    && !(from.y > top && to.y > top) && !(from.y < bottom && to.y < bottom)) {
										crossed        = true;
										CVec step      = to - from;
										motion.mMove.x = step.x / gPointerScreenWidth;
										motion.mMove.y = step.y / gPointerScreenHeight;
										motion.mSpeed  = PSVECMag(&step) / gPointerScreenWidth;
									}
								}
							}
						}
						if (crossed) {
							obj->_6DC = TRUE;
							obj->vf208(motion.mPos, &motion.mMove, TRUE, motion.mSpeed);
							PickScopeTarget(this, &scope, obj, held, &eye, motion, FALSE);
						} else {
							obj->vf208(motion.mPos, &motion.mMove, FALSE, motion.mSpeed);
						}
					}
				}
			}
		}
		lbl_80444E98 = *motion.mPos;
		if (GetStatusFlags(0x100) && GetPlayer()->GetAttachment() == 2) {
			AimAtPointer(this);
		} else {
			fn_800E8140(GetPlayer(), GetPlayer()->mMoveRotate.y, 0.0f);
		}
	} else if (GetStatusFlags(0x100) && GetPlayer()->GetAttachment() == 2) {
		// the eye camera keeps the last aim while the pointer is briefly off the screen
		if (mPointer._10C > 0.0f) {
			AimAtPointer(this);
		} else {
			fn_800E8140(GetPlayer(), GetPlayer()->mMoveRotate.y, 0.0f);
		}
	}

	// the HUD's object fades in and its marker turns while the pointer stays on it
	if (pick.mObj != mPointer.mHudObject) {
		mPointer.mHudObject  = pick.mObj;
		mPointer.mHudObjectX = 0.0f;
	}
	if (mPointer.mHudObject != NULL) {
		if (mPointer.mHudObjectX < 1.0f) {
			if ((mPointer.mHudObjectX += 0.1f * frames) > 1.0f) {
				mPointer.mHudObjectX = 1.0f;
			}
		}
		mPointer.mHudObjectY += DEG2RAD * 3.0f * frames;
	} else if (mPointer.mHudObjectX > 0.0f) {
		if ((mPointer.mHudObjectX -= 0.1f * frames) < 0.0f) {
			mPointer.mHudObjectX = 0.0f;
		}
	}

	// a new scope target: its marker clicks, every tenth time a longer sound; an item plays one of its
	// stage's sounds for the speed the pointer came at, at night a random one of twenty
	if (scope.mObj != mPointer.mScopeTarget) {
		mPointer.mScopeTarget = scope.mObj;
		if (scope.mObj != NULL) {
			if (scope.mObj->_18C >= 0) {
				mPointer.mScopeMarker = scope.mObj->_18C;
				mPointer.mScopeItem   = -1;
			} else {
				s32 item                = scope.mObj->GetObjFlags();
				mPointer.mScopeItem     = item;
				mPointer.mScopeMarker   = -1;
				mPointer.mScopeRareItem = IsScopeRareItem(item);
				((XObjItem*)mPointer.mScopeTarget)->SetUseAnim();
			}
			if (scope.mSpeed < 0.007f) {
				scope.mLevel = 1;
			} else if (scope.mSpeed < 0.02f) {
				scope.mLevel = 2;
			} else {
				scope.mLevel = 3;
			}
			if (mPointer.mScopeZoom > scope.mLevel) {
				if (mPointer.mScopeZoom > 2.0f) {
					mPointer.mScopeLevel = 3;
				} else if (mPointer.mScopeZoom > 1.0f) {
					mPointer.mScopeLevel = 2;
				} else {
					mPointer.mScopeLevel = 1;
				}
			} else {
				mPointer.mScopeLevel = scope.mLevel;
				mPointer.mScopeSpeed = scope.mSpeed;
				mPointer.mScopeZoom  = scope.mLevel;
			}
			if (mPointer.mScopeMarker >= 0) {
				if (IsWiiSfxPlaying(&gGame.mSound, 0x262)) {
					StopWiiSfx(&gGame.mSound, 0x262, 0.1f);
				}
				if (++mPointer.mScopeTicks >= 10) {
					PlayWiiSoundEffect(&gGame.mSound, 0x263, 1.0f, 0.0f);
					mPointer.mScopeTicks = 0;
				} else {
					PlayWiiSoundEffect(&gGame.mSound, 0x262, 1.0f, 0.0f);
				}
			} else if (mPointer.mScopeItem >= 0) {
				if (mTimeState.mMode == 1 && mTimeState.mTime > 0.5f
				    && (gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0) == 0
				    && gGame.mSound.FindTrack(0x10) == NULL) {
					if (mPointer.mScopeSound >= 0 && IsWiiSfxPlaying(&gGame.mSound, mPointer.mScopeSound)) {
						StopWiiSfx(&gGame.mSound, mPointer.mScopeSound, 0.1f);
					}
					s32 index = rand() % 20;
					if (index == mPointer.mScopeSound) {
						mPointer.mScopeSound = (index + 1) % 20;
					} else {
						mPointer.mScopeSound = index;
					}
					PlayWiiSoundEffect(&gGame.mSound, mPointer.mScopeSound + 0x264, 1.0f, 0.0f);
				} else if (lbl_802C49C8[mStage].mSounds != NULL) {
					if (mPointer.mScopeSound >= 0 && IsWiiSfxPlaying(&gGame.mSound, mPointer.mScopeSound)) {
						StopWiiSfx(&gGame.mSound, mPointer.mScopeSound, 0.2f);
					}
					const ScopeStageSounds& sounds = lbl_802C49C8[mStage];
					mPointer.mScopeSoundIndex      = (mPointer.mScopeSoundIndex + 1) % sounds.mCount;
					mPointer.mScopeSound           = sounds.mSounds[mPointer.mScopeSoundIndex + (mPointer.mScopeLevel - 1) * sounds.mCount];
					PlayWiiSoundEffect(&gGame.mSound, mPointer.mScopeSound, 1.0f, 0.0f);
				} else if (mStage == 11) {
					switch (mPointer.mScopeItem) {
					case 0:
						PlayWiiSoundEffect(&gGame.mSound, 0x431, 1.0f, 0.0f);
						break;
					case 0x7C:
						PlayWiiSoundEffect(&gGame.mSound, 0x432, 1.0f, 0.0f);
						break;
					}
				} else if (mStage == 18) {
					switch (mPointer.mScopeItem) {
					case 0xE:
						PlayWiiSoundEffect(&gGame.mSound, 0x455, 1.0f, 0.0f);
						break;
					case 0x6E:
						PlayWiiSoundEffect(&gGame.mSound, 0x456, 1.0f, 0.0f);
						break;
					}
				} else if (mStage == 22) {
					switch (mPointer.mScopeItem) {
					case 0:
						PlayWiiSoundEffect(&gGame.mSound, 0x47D, 1.0f, 0.0f);
						break;
					case 0x6D:
						PlayWiiSoundEffect(&gGame.mSound, 0x47E, 1.0f, 0.0f);
						break;
					}
				}
			}
		}
	}
	if (mPointer.mScopeTarget != NULL) {
		if (mPointer.mScopeFade < 1.0f) {
			mPointer.mScopeFade = 1.0f;
		}
		if (mPointer.mScopeFade2 < 1.0f) {
			mPointer.mScopeFade2 = 1.0f;
		}
	} else {
		if (mPointer.mScopeFade > 0.0f) {
			if ((mPointer.mScopeFade -= 0.05f * frames) <= 0.0f) {
				mPointer.mScopeFade = 0.0f;
				mPointer.mScopeZoom = 1.0f;
			}
		}
		if (mPointer.mScopeFade2 > 0.0f) {
			if ((mPointer.mScopeFade2 -= 0.02f * frames) <= 0.0f) {
				mPointer.mScopeFade2 = 0.0f;
			}
		}
	}
	if (mPointer.mScopeZoom > 1.0f) {
		if ((mPointer.mScopeZoom -= 0.01f * frames) < 1.0f) {
			mPointer.mScopeZoom = 1.0f;
		}
	}
	if (mPointer.mScopeZoom > 0.0f) {
		mPointer.mScopeAngle += frames * (DEG2RAD * 3.0f * (mPointer.mScopeZoom * mPointer.mScopeZoom));
	}
}

/**
 * @note Address: 0x80024008
 * @note Size: 0x258
 */
// NONMATCHING: both queue loops and the call match; the selected slot's values use other registers and stack slots
void XObj::vf15C()
{
	if (mMoveTranslate.y != mLastPos.y) {
		mStatus.mFlags |= 0x10000000000000ULL;
	}
	mLastPos = mMoveTranslate;
	mLastRot = mMoveRotate;
	if (IsAnimDone() && HasQueuedAnim()) {
		s32 id;
		u16 frame;
		s16 rate;
		PopAnim(id, frame, rate);
		SetAnim(id, FALSE, frame, rate / 100.0f, 0.0f, 15.0f);
	}
}

// NONMATCHING: all six retail animation blocks and resource checks are present;
// temporary lifetimes and register allocation remain open.
/**
 * @note Address: 0x800243CC
 * @note Size: 0x600
 */
f32 CObj::GetAnimEndFrame()
{
	f32 end = 0.0f;
	if (mObject.m_pScnMdl != NULL) {
		const nw4r::g3d::AnmObj* anm
		    = ((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_CHR);
		if (anm != NULL && mObject.GetAnmObjChrRes() != NULL) {
			nw4r::g3d::ResAnmChr res = mObject.GetAnmObjChrRes()->GetResAnm();
			if (res.GetNumFrame() > end) {
				nw4r::g3d::ResAnmChr res2 = mObject.GetAnmObjChrRes()->GetResAnm();
				end                       = res2.GetNumFrame();
			}
		}
		anm = ((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_VIS);
		if (anm != NULL) {
			nw4r::g3d::ResAnmVis res = ((nw4r::g3d::AnmObjVisRes*)((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)
			                                ->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_VIS))
			                               ->GetResAnm();
			if (res.GetNumFrame() > end) {
				nw4r::g3d::ResAnmVis res2 = ((nw4r::g3d::AnmObjVisRes*)((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)
				                                 ->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_VIS))
				                                ->GetResAnm();
				end = res2.GetNumFrame();
			}
		}
		anm = ((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_MATCLR);
		if (anm != NULL) {
			nw4r::g3d::ResAnmClr res = ((nw4r::g3d::AnmObjMatClrRes*)((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)
			                                ->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_MATCLR))
			                               ->GetResAnm();
			if (res.GetNumFrame() > end) {
				nw4r::g3d::ResAnmClr res2 = ((nw4r::g3d::AnmObjMatClrRes*)((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)
				                                 ->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_MATCLR))
				                                ->GetResAnm();
				end = res2.GetNumFrame();
			}
		}
		anm = ((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_TEXPAT);
		if (anm != NULL && mObject.mTexPatName != mObject.mDefaultTexPat) {
			nw4r::g3d::ResAnmTexPat res = ((nw4r::g3d::AnmObjTexPatRes*)((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)
			                                   ->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_TEXPAT))
			                                  ->GetResAnm();
			if (res.GetNumFrame() > end) {
				nw4r::g3d::ResAnmTexPat res2 = ((nw4r::g3d::AnmObjTexPatRes*)((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)
				                                    ->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_TEXPAT))
				                                   ->GetResAnm();
				end = res2.GetNumFrame();
			}
		}
		anm = ((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_TEXSRT);
		if (anm != NULL) {
			nw4r::g3d::ResAnmTexSrt res = ((nw4r::g3d::AnmObjTexSrtRes*)((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)
			                                   ->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_TEXSRT))
			                                  ->GetResAnm();
			if (res.GetNumFrame() > end) {
				nw4r::g3d::ResAnmTexSrt res2 = ((nw4r::g3d::AnmObjTexSrtRes*)((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)
				                                    ->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_TEXSRT))
				                                   ->GetResAnm();
				end = res2.GetNumFrame();
			}
		}
		anm = ((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_SHP);
		if (anm != NULL) {
			nw4r::g3d::ResAnmShp res = ((nw4r::g3d::AnmObjShpRes*)((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)
			                                ->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_SHP))
			                               ->GetResAnm();
			if (res.GetNumFrame() > end) {
				nw4r::g3d::ResAnmShp res2 = ((nw4r::g3d::AnmObjShpRes*)((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)
				                                 ->GetAnmObj(nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_SHP))
				                                ->GetResAnm();
				end = res2.GetNumFrame();
			}
		}
	}
	return end;
}

// the greatest current frame among the model's animation objects; the default
// texture-pattern animation does not participate
// NONMATCHING: the loop and calls match; retail assigns the prologue's object and
// index registers in the opposite order
f32 CObj::GetAnimFrame()
{
	f32 frame = 0.0f;
	if (mObject.m_pScnMdl != NULL) {
		for (s32 i = 0; i < 6; ++i) {
			const nw4r::g3d::AnmObj* anm
			    = ((const nw4r::g3d::ScnMdlSimple*)mObject.m_pScnMdl)->GetAnmObj((nw4r::g3d::ScnMdlSimple::AnmObjType)i);
			if (anm != NULL && anm->GetFrame() > frame) {
				if (i == 3) {
					if (mObject.mTexPatName != mObject.mDefaultTexPat) {
						frame = anm->GetFrame();
					}
				} else {
					frame = anm->GetFrame();
				}
			}
		}
	}
	return frame;
}

/**
 * @note Address: 0x80024A10
 * @note Size: 0x6C
 */
CVec CVec::operator-(Vec other) const
{
	CVec result = *this;
	return result -= other;
}

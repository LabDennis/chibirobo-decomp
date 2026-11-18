#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h"
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
#include "game/wii/animsfx.h"
#include "game/wii/cpadact.h"
#include "game/wii/infoscrn.h"
#include "game/wii/msg.h"
#include "game/wii/fbeffect.h"
#include "game/fbtexeff.h"
#include "game/cmdline.h"
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
BOOL fn_80246BD8(void* handle);                     // the emitter handle is valid
void* fn_80246C04(void* handle);                    // the emitter behind it
void fn_802BEA5C(void* emitter, BOOL);              // kills the emitter
void fn_802BE3A8(void* particles, u32 group, BOOL); // kills a particle group
void ClearSoundEffects(XSound* sound, s32 frames);
s32 GetWiiSaveSlotStage(CSaveFile* manager, int index);
void ResetTitleScreenState(Title* title);
void fn_800848C0(void* menus); // the GameCube's ResetMenuGroup
void fn_80084BF0(void* menus, s32 menu);
BOOL fn_800C61C8();
void fn_8027B6B8(WiiGsSceneObj* scene, BOOL, s32);             // switches the scene's draws to and from the alpha pass
void fn_802AEF34(WiiGsSceneObj** head, CCamera* camera, BOOL); // sorts the queued scene draws under camera
void fn_8027B40C(WiiGsSceneObj* scene);                        // the shadow pass' five scene draws
void fn_8027B484(WiiGsSceneObj* scene);
void fn_8027B570(WiiGsSceneObj* scene);
void fn_8027B644(WiiGsSceneObj* scene);
void fn_8027B5D0(WiiGsSceneObj* scene);
void fn_8027AE04(WiiGsSceneObj* scene, s32 index); // as gameupdate.cpp declares it
void fn_8001E1C8(CGame* game);
void fn_802AB860(CGs* gs);
void fn_802BE414(void* obj, CLight* light); // as gameupdate.cpp declares them
void fn_802BE4D4(void* obj, GXFogType type, const GXColor& color, f32 startZ, f32 endZ, f32 nearZ, f32 farZ);
void* fn_8027AE0C(WiiGsSceneObj* scene, s32 index); // the scene's fog
void fn_8028175C(void** fog, GXFogType* type, f32* startZ, f32* endZ, f32* nearZ, f32* farZ, GXColor* color);
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

// the head of the stage's info record, as far as this file reads it: the GameCube's UnkStageInfo
// (game/cgameutil.h, which the Wii cannot include). todo: one definition with gameupdate.cpp's
struct WiiStageInfo {
	u8 _00[0x10];
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
	if (camera != NULL && (camera->mStatus.mFlags & 1) && camera->mScene != NULL && camera->mScene->_2888 != NULL) {
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
		game->mTpl.mTex.Init(&game->mTpl.mPalette->descriptorArray[139], 0);
		CTexObj bubble = game->mTpl.mTex;
		f32 center     = gPointerScreenWidth / 2;
		u32 color      = game->mStageName.mColor & 0xFFFFFF00; // rgb only; alpha comes from the scale
		for (s32 i = 0; i < game->mStageName.mCount; i++) {
			f32 scale = game->mStageName.mChars[i].mScale;
			if (scale <= 0.0f) {
				continue;
			}
			CFont font;
			s32 width;
			CTexObj letter = *GetFontGlyphTexture(&font, game->mStageName.mChars[i].mText, 1, &width);
			f32 bob        = 4.0f * (f32)sin(game->mStageName.mChars[i].mBob);
			f32 rot        = DEG2RAD * (10.0f * (f32)sin(0.7f * game->mStageName.mChars[i].mTilt));
			f32 swell      = sin(3.1415927f * scale);
			f32 pop        = 0.2f * swell + (f32)sin(1.5707964f * scale);
			f32 x          = spacing * 0.5f + (spacing * i + (center - spacing * game->mStageName.mCount * 0.5f));
			f32 y          = 150.0f;
			if (gWiiPointer.mPointerValid && game->IsLoadStepRunning() && game->mLoadStep > 20) {
				f32 half = glyphSize * (2.2f * pop) * 0.5f;
				if (gWiiPointer.mPointer.x >= x - half && gWiiPointer.mPointer.x <= x + half && gWiiPointer.mPointer.y >= 150.0f - half
				    && gWiiPointer.mPointer.y <= 150.0f + half) {
					if (!game->mStageName.mChars[i]._14) {
						const s32 anims[2]             = { 1011, 1007 };
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
				y = 150.0f - 30.0f * (f32)sin(0.9f * (3.1415927f * game->mStageName.mChars[i]._18));
			}
			DrawGsTexturedQuad(&gGame.mGs, &bubble, (u32)(255.0f * scale) | 0xFFFFFF00, (u32)(255.0f * scale) | 0xFFFFFF00,
			                   (u32)(255.0f * scale) | 0xFFFFFF00, (u32)(255.0f * scale) | 0xFFFFFF00, x, y + bob, 0.0f,
			                   glyphSize * (2.2f * pop), glyphSize * (2.2f * pop), 0.0f, -0.0f, -rot);
			f32 size   = glyphSize * (1.5f * pop);
			f32 shrink = width / 24.0f * size;
			u32 alpha  = 255.0f * scale;
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
			nw4r::snd::SoundHandle& handle = gGame.mSound.mArchivePlayer.mTracks.mHandles[i];
			if (handle.IsAttachedSound() && handle.IsPause()) {
				handle.Pause(false, frames);
			}
		}
		for (s32 i = 0; i < game->mDrawLists[0].mCount; i++) {
			XObj* object = (XObj*)game->mDrawLists[0].mRefs[i].mObj;
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
						nw4r::snd::SoundHandle& handle = gGame.mSound.mArchivePlayer.mTracks.mHandles[i];
						if (handle.IsAttachedSound() && handle.GetId() == id && !handle.IsPause()) {
							handle.Pause(true, frames);
						}
					}
				}
			}
		} else if (FindTrack(4) != NULL && IsTrackPaused(4)) {
			s32 frames = gRefreshRate * fade;
			for (int i = 0; i < 0x40; i++) {
				nw4r::snd::SoundHandle& handle = gGame.mSound.mArchivePlayer.mTracks.mHandles[i];
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
			nw4r::snd::SoundHandle& handle = gGame.mSound.mArchivePlayer.mTracks.mHandles[i];
			if (handle.IsAttachedSound() && !handle.IsPause()) {
				handle.Pause(true, frames);
			}
		}
		for (s32 i = 0; i < game->mDrawLists[0].mCount; i++) {
			XObj* object = (XObj*)game->mDrawLists[0].mRefs[i].mObj;
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
			nw4r::snd::SoundHandle& handle = gGame.mSound.mArchivePlayer.mTracks.mHandles[i];
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
		void* emitter = game->mEvtPtcl.mEmitters[i];
		s32 state     = fn_80246BD8(emitter) ? *(s32*)((u8*)fn_80246C04(emitter) + 0xC) : 0;
		if (!(state == 0 || state == 3)) {
			fn_802BEA5C(emitter, TRUE);
		}
	}
	for (u32 i = 0; i < 6; i++) {
		fn_802BE3A8(&game->mEffects, i, TRUE);
	}
	WiiPlayerState* player = game->GetPlayer();
	for (s32 i = 0; i < 2; i++) {
		CSound3DActor* actor          = ((AnimSfx*)player->mSfx)->mActors[i].mActor;
		nw4r::snd::SoundHandle* sound = actor != NULL && actor->mHandle.IsAttachedSound() ? &actor->mHandle : NULL;
		if (sound != NULL && actor != NULL) {
			actor->mHandle.Stop(gRefreshRate * 0.0f);
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
		ResetTitleScreenState(&game->mTitle);
		fn_800848C0(&game->mMenu.mActive); // the menu group starts at its active word
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
	} else if (stage == 20000) {
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
// NONMATCHING: retail rounds the frame through a double before it adds the rate
// the scene animation is at its last frame, or will be within this step
bool CObj::IsAnmScnEnd()
{
	f32 frame = mObject.m_resources.scn != NULL ? mObject.m_resources.scn->GetFrame() : 0.0f;
	f32 end   = mObject.m_resources.scn != NULL ? mObject.m_resources.scn->mRes.GetNumFrame() - 1 : 0.0f;
	return frame + mAnimRate[1] >= end;
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
	if (camera != NULL && (camera->mStatus.mFlags & 1) && camera->mScene != NULL && camera->mScene->_2888 != NULL) {
		return camera->IsAnmScnEnd();
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
	if (camera != NULL && (camera->mStatus.mFlags & 1)) {
		nw4r::g3d::ResFile* file = &camera->mObject.m_resFile;
		if (camera->mObject.m_pScnMdl != NULL && file->HasResAnmScn() && file->GetResAnmScnNumEntries() != 0) {
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
	if (camera != NULL && (camera->mStatus.mFlags & 1)) {
		nw4r::g3d::ResFile* file = &camera->mObject.m_resFile;
		if (camera->mObject.m_pScnMdl != NULL && file->HasResAnmScn() && file->GetResAnmScnNumEntries() != 0) {
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

	fn_8027B6B8(gGame.mGs.mScene.mHead, TRUE, 4);
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
	fn_8027B40C(gGame.mGs.mScene.mHead);
	fn_8027B484(gGame.mGs.mScene.mHead);
	fn_8027B570(gGame.mGs.mScene.mHead);
	fn_8027B644(gGame.mGs.mScene.mHead);
	fn_8027B5D0(gGame.mGs.mScene.mHead);
	WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
	while (scene->mPending != 0) {
		if (scene->mPending != 0) {
			scene->vf038(scene->mPending - 1);
		}
	}
	fn_8027B6B8(gGame.mGs.mScene.mHead, FALSE, 2);
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
	fn_8027AE04(gGame.mGs.mScene.mHead, 0);
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
		fn_8027B6B8(gGame.mGs.mScene.mHead, TRUE, 4);
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
		fn_8027B6B8(gGame.mGs.mScene.mHead, FALSE, 2);
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
	fn_8001E1C8(this);
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
	fn_802AB860(&gGame.mGs);
	SetCameraLight();
	if (gGame.mGs.mScene.mHead->_2888 != NULL) {
		fn_802BE414(&mEffects, gGame.mGs.mLight);
		void* fog = fn_8027AE0C(gGame.mGs.mScene.mHead, 0);
		GXFogType type;
		f32 startZ;
		f32 endZ;
		f32 nearZ;
		f32 farZ;
		GXColor color;
		fn_8028175C(&fog, &type, &startZ, &endZ, &nearZ, &farZ, &color);
		GXColor fogColor = color;
		fn_802BE4D4(&mEffects, type, fogColor, startZ, endZ, nearZ, farZ);
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
		fn_802BE414(&mEffects, gGame.mGs.mLight);
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
		Vec quad[4] = { { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f } }; // the depth goes back
		quad[0].x   = (f32)gScreenLeft;
		quad[0].y   = (f32)gScreenTop;
		quad[1].x   = (f32)gScreenRight;
		quad[1].y   = (f32)gScreenTop;
		quad[2].x   = (f32)gScreenRight;
		quad[2].y   = (f32)gScreenBottom;
		quad[3].x   = (f32)gScreenLeft;
		quad[3].y   = (f32)gScreenBottom;
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
				if (!faded || mFade.mColor.a < 1.0f) {
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
	if (fn_800C61C8()) {
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
			ChangeGameStage(game, 10000, TRUE);
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
void XObj::vf16C()
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

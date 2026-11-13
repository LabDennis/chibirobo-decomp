#include "game/cgame.h"
#include "game/mic.h"
#include "game/video.h"
#include "game/mm.h"
#include <Dolphin/vi.h>
#include "game/cgameutil.h"

extern "C" {
int strncmp(const char* a, const char* b, unsigned long n);
}
extern "C" f32 sStageNameShadowX[4];
extern "C" f32 sStageNameShadowY[4];

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
		u32 color      = game->mStageName.mColor & 0xFFFFFF00;
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
			swell     = sin(3.1415927f * scale);
			pop       = 0.2f * swell + (f32)sin(1.5707964f * scale);
			x         = spacing * 0.5f + (spacing * i + (320.0f - spacing * game->mStageName.mCount / 2.0f));
			u32 alpha = 255.0f * scale;
			rot       = -(0.017453292f * (10.0f * tilt));
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
void ClearMainMenuExtraRows(void* menu);
void ResetMessageState(CMsg* msg);
void CMsg_ResetPos(CMsg* msg);
void UnloadStageDataAndModule(void);
void ReleaseStageObjectList(XOBJS* objs);
void psKillAllParticle(void);
void SetMusicPlaybackPaused(XSound* sound, BOOL pause);
void ClearStageSoundBank(XSound* sound);
void ResetMenuGroup(void* menu);
void ResetGameHudState(void* info);
u8* GetStageDisplayName(s32 index);
int rand(void);
}
extern "C" GXColor gMessageFontDefaultColor;

// a lead byte of a two byte shift jis character
static inline bool IsSJISByte(char c)
{
	return ((u8)c >= 0x81 && (u8)c <= 0x9F) || ((u8)c >= 0xE0 && (u8)c <= 0xFC);
}

static inline bool IsSJISLead(char c)
{
	return IsSJISByte(c) ? true : false;
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
		if (++game->mStage >= game->_344C) {
			game->mStage = 0;
		}
	}

	if (game->mMovie.mMovie.mState != 0) {
		game->mMovie.mMovie.Stop();
	}
	ClearMainMenuExtraRows(game->mMenu.mMenus[0]);
	ResetEventScriptState(&game->mEvt, 1, 1);
	for (s32 i = 0; i < 2; i++) {
		ResetMessageState(&game->mCMsgs[i]);
		CMsg_ResetPos(&game->mCMsgs[i]);
		game->mCMsgs[i].SetColor(gMessageFontDefaultColor);
	}
	for (s32 i = 0; i < 5; i++) {
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
	gActiveMicChannel = -1;
	game->mParticle.KillAll();
	psKillAllParticle();
	if (game->mStatus.mFlags & 4) {
		if (game->mStatus.mFlags & 4) {
			game->mStatus.mFlags &= ~4;
			SetMusicPlaybackPaused(&gGame.mSound, FALSE);
			HSD_PadRumbleActiveAll();
		}
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
		game->_3338         = 19;
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
		game->_3338         = 20;
	} else {
		game->_3338 = 21;
	}

	if (showName) {
		game->mStageName.mActive = TRUE;
		game->mStageName.mCount  = 0;
		s32 cur                  = game->mStage;
		if (cur < GetStageNum()) {
			char* name = (char*)GetStageDisplayName(game->mStage);
			if (name != NULL) {
				game->mStageName.mColor = 0x80FFFF00;
				for (s32 i = 0; i < 20 && *name != '\0'; i++, name++) {
					game->mStageName.mChars[i].mText  = name;
					game->mStageName.mChars[i].mScale = -0.2f * i;
					game->mStageName.mChars[i].mWait  = 2.0f * FRAME_RATE;
					game->mStageName.mChars[i].mBob   = DEG2RAD * (rand() % 360);
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
				for (s32 i = game->mStageName.mCount; i < 20; i++) {
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

extern "C" {
s32 MICProbeEx(s32 chan);
s32 MICMountAsync(s32 chan, void* work, s32 size, void (*detach)(s32), void (*attach)(s32, s32));
s32 MICGetResultCode(s32 chan);
s32 MICGetDeviceID(s32 chan, u32* id);
s32 MICUnmount(s32 chan);
s32 M2SGetActiveChannel(void);
s32 M2SGetSamplesLeft(void);
void M2SGetSamples(s16** samples, s32 count);
void M2SAdvanceBuffer(s32 count);
void* memset(void* dst, int val, u32 size);
}

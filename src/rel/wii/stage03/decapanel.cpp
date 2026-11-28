#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/ctpl.h"

struct DecaPanelKey {
	f32 x;
	f32 y;
	u8 mChar;
};

DecaPanelKey decaPanelKeys[12] = {
	{ -58.0f, -40.0f, '7' }, { -1.0f, -40.0f, '8' },  { 56.0f, -40.0f, '9' },  { -58.0f, 17.0f, '4' },
	{ -1.0f, 17.0f, '5' },   { 56.0f, 17.0f, '6' },   { -58.0f, 74.0f, '1' },  { -1.0f, 74.0f, '2' },
	{ 56.0f, 74.0f, '3' },   { -29.0f, 131.0f, 'C' }, { -29.0f, 131.0f, 'C' }, { 56.0f, 131.0f, '0' },
};

class DecaPanel {
public:
	void ResetInput()
	{
		mDigits[0]  = ' ';
		mDigits[1]  = ' ';
		mDigits[2]  = ' ';
		mDigits[3]  = ' ';
		mDigits[4]  = ' ';
		mDigits[5]  = ' ';
		mDigitCount = 0;
		mCode       = 0;
		mCursorX    = 0;
		mCursorY    = 3;
		mLocked     = 0;
		mDigitNum   = 0;
		s32 idx     = mCursorX + mCursorY * 3;
		mCurX = mTargetX = decaPanelKeys[idx].x;
		mCurY = mTargetY = decaPanelKeys[idx].y;
	}

	void LoadTextures()
	{
		if (mState == 1) {
			if (mTpl.ReadEx("tpl/stage03.tpl", 0, 0)) {
				for (s32 i = 0; i < 5; i++) {
					mTpl.mTex.Init(&mTpl.mPalette->descriptorArray[i], 0);
					mTexs[i] = mTpl.mTex;
				}
				mState              = 2;
				mActive             = 0;
				gGame.mPointer._110 = NULL;
				mFade               = 0.0f;
			}
		}
	}

	s32 mState;
	CTpl mTpl;
	u32 mActive;
	f32 mFade;
	CTexObj mTexs[5];
	u8 mDigits[6];
	u8 mDigitCount;
	u8 _71F;
	s32 mCode;
	s8 mCursorX;
	s8 mCursorY;
	s8 mLocked;
	s8 mDigitNum;
	f32 mCurX;
	f32 mCurY;
	f32 mTargetX;
	f32 mTargetY;
};

DecaPanel decaPanel;

extern "C" XObj* ResolveEventObjectId(const CEvtData* data, s16 id);
extern "C" void DecaPanel_Update(DecaPanel* work);
extern "C" s32 gRefreshRate;
extern "C" s32 gPointerScreenWidth;

inline XObj* GetObjById(s16 id)
{
	return gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, id) : NULL;
}

struct SavedObjFlag {
	s32 mId;
	u8 mFlags;
};

struct SavedObjFlagList {
	void Apply(s32 mode);
	void Clear();

	SavedObjFlag mEntries[300];
	s32 mNum;
};
STATIC_ASSERT(sizeof(SavedObjFlagList) == 0x964);

SavedObjFlagList savedObjFlags;

extern "C" s32 Stage03_Init(u32 arg)
{
	decaPanel.mActive   = 0;
	decaPanel.mState    = 1;
	gGame.mPointer._110 = NULL;
	decaPanel.mFade     = 0.0f;
	decaPanel.ResetInput();
	return 1;
}

extern "C" s32 Stage03_Update(s32 frames)
{
	static s32 sTimer = 0;
	sTimer += frames;
	if (IsWiiSfxPlaying(&gGame.mSound, 0xFF000E)) {
		SetGameSfxPitch(&gGame.mSound, 0xFF000E, (s16)(5.0f * (f32)sin(0.017453292f * sTimer)), 0.0f);
	}
	if (IsWiiSfxPlaying(&gGame.mSound, 0xFF000F)) {
		SetGameSfxPitch(&gGame.mSound, 0xFF000F, (s16)(5.0f * (f32)sin(0.017453292f * sTimer)), 0.0f);
	}
	if (gGame.mEvt.mData.mScript == NULL && rand() % 5000 < 10) {
		switch (rand() % 4) {
		case 0:
			PlayWiiSoundEffect(&gGame.mSound, 0x2E5, 1.0f, 0.0f);
			break;
		case 1:
			PlayWiiSoundEffect(&gGame.mSound, 0x2E6, 1.0f, 0.0f);
			break;
		case 2:
			PlayWiiSoundEffect(&gGame.mSound, 0x2E7, 1.0f, 0.0f);
			break;
		case 3:
			PlayWiiSoundEffect(&gGame.mSound, 0x2E8, 1.0f, 0.0f);
			break;
		}
	}
	decaPanel.LoadTextures();
	return 1;
}

extern "C" s32 Stage03_DrawHud(u32 arg)
{
	DecaPanel_Update(&decaPanel);
	return 1;
}

extern "C" s32 Stage03_Exit(u32 arg)
{
	decaPanel.mState = 0;
	decaPanel.mTpl.Close();
	return 1;
}

extern "C" s32 Stage03_CmdNop(s32 cmd, s32 count, s32* args)
{
	return 0;
}

extern "C" s32 Stage03_CmdAddObjFlagList(s32 cmd, s32 count, s32* args)
{
	for (s32 i = 0; i < count; i++) {
		((CCameraActionScroll2*)gGame.mCameraActions.mTable[4])->AddHideObj(args[i]);
	}
	return 0;
}

extern "C" s32 Stage03_CmdClearObjFlagList(s32 cmd, s32 count, s32* args)
{
	((CCameraActionScroll2*)gGame.mCameraActions.mTable[4])->ClearHideList();
	return 0;
}

void SavedObjFlagList::Apply(s32 mode)
{
	if (mNum <= 0) {
		return;
	}
	for (s32 i = 0; i < mNum; i++) {
		if (mEntries[i].mId == -1) {
			break;
		}
		XObj* obj = GetObjById(mEntries[i].mId);
		if (obj != NULL) {
			if (mode != 0) {
				if (mEntries[i].mFlags & 1) {
					obj->mStatus.mFlags |= 0x10000ULL;
				}
			} else {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
	}
}

inline void SavedObjFlagList::Clear()
{
	mNum = 0;
	for (s32 i = 0; i < 300; i++) {
		mEntries[i].mId    = -1;
		mEntries[i].mFlags = 0;
	}
}

extern "C" s32 CmdClearSavedObjFlags(s32 cmd, s32 count, s32* args)
{
	savedObjFlags.Clear();
	return 0;
}

extern "C" s32 CmdSaveObjFlag(s32 cmd, s32 count, s32* args)
{
	if (savedObjFlags.mNum < 300) {
		savedObjFlags.mEntries[savedObjFlags.mNum].mId = args[0];
		XObj* obj                                      = GetObjById(savedObjFlags.mEntries[savedObjFlags.mNum].mId);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mStatus.mFlags & 0x10000ULL) {
				savedObjFlags.mEntries[savedObjFlags.mNum].mFlags |= 1;
			}
			savedObjFlags.mNum++;
		}
	}
	return 0;
}

extern "C" s32 CmdApplySavedObjFlags(s32 cmd, s32 count, s32* args)
{
	if (args[0] != 0) {
		savedObjFlags.Apply(1);
	} else {
		savedObjFlags.Apply(0);
	}
	return 0;
}

extern "C" s32 CmdOpenDecaPanel(s32 cmd, s32 count, s32* args)
{
	decaPanel.ResetInput();
	decaPanel.mActive = 1;
	gGame.SetStatusFlags(2);
	StopWiiSfx(&gGame.mSound, 0x2F6, 0.0f);
	gGame.mPointer._110 = &decaPanel.mTexs[3];
	return 0;
}

extern "C" s32 CmdCloseDecaPanel(s32 cmd, s32 count, s32* args)
{
	decaPanel.mActive = 0;
	gGame.ClearStatusFlags(2);
	gGame.mPointer._110 = NULL;
	return 0;
}

extern "C" s32 CmdIsDecaPanelOpen(s32 cmd, s32 count, s32* args)
{
	return decaPanel.mActive;
}

extern "C" s32 CmdGetDecaPassword(s32 cmd, s32 count, s32* args)
{
	return decaPanel.mCode;
}

static s32 decaPanelKeySounds[12] = {
	0x2EF, 0x2F0, 0x2F1, 0x2EC, 0x2ED, 0x2EE, 0x2E9, 0x2EA, 0x2EB, 0x2F3, 0x2F3, 0x2F2,
};

static f32 decaPanelDigitPos[6][2] = {
	{ -68.0f, -115.0f }, { -41.0f, -115.0f }, { -14.0f, -115.0f }, { 13.0f, -115.0f }, { 40.0f, -115.0f }, { 67.0f, -115.0f },
};

static s32 decaPanelDigitCells[10] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8 };

struct DecaPanelKeySprite {
	s32 mTex;
	f32 mScale;
	f32 mTrans;
	s32 mKey;
	f32 mW;
	f32 mH;
};

static inline u32 FadeColor(f32 fade)
{
	return (u32)(255.0f * fade) | 0xFFFFFF00;
}

// NONMATCHING: pointer hit-testing, digit input and all draw calls are present; register allocation around
// the key sprites and the packed colour construction remains open, and retail tests mState unsigned here (cmplwi)
// though Stage03_Update's switch on it is signed.
extern "C" void DecaPanel_Update(DecaPanel* work)
{
	CTexObj* tex;
	if (work->mState != 2) {
		return;
	}
	if ((work->mActive & 1) && !(work->mActive & 2)) {
		work->mActive |= 2;
		PlayWiiSoundEffect(&gGame.mSound, 0x2F6, 1.0f, 0.0f);
	}
	u32 frames = 2;
	if (work->mActive) {
		if (work->mFade < 1.0f && (work->mFade += (1.0f / (0.2f * gRefreshRate)) * frames) > 1.0f) {
			work->mFade = 1.0f;
		}
	} else if (work->mFade > 0.0f && (work->mFade -= (1.0f / (0.2f * gRefreshRate)) * frames) < 0.0f) {
		work->mFade = 0.0f;
	}

	if (work->mFade >= 1.0f) {
		if (work->mLocked) {
			if (!(gWiiPads[0].mButton & 0x0001000000000010ULL)) {
				work->mLocked = 0;
			}
		} else {
			work->mLocked = 0;
			if (gWiiPointer.mPointerValid) {
				work->mDigitNum = 0;
				for (s32 i = 0; i < 12; i++) {
					f32 height = 44.28f;
					f32 half   = (u32)(i - 9) <= 1 ? 88.56f : 44.28f;
					f32 x      = 0.9f * decaPanelKeys[i].x + gPointerScreenWidth / 2;
					f32 y      = 0.9f * decaPanelKeys[i].y + 200.0f;
					f32 minX   = x - half * 0.5f;
					f32 maxX   = x + half * 0.5f;
					f32 minY   = y - height * 0.5f;
					f32 maxY   = y + height * 0.5f;
					if (gWiiPointer.mPointer.x >= minX && gWiiPointer.mPointer.x <= maxX && gWiiPointer.mPointer.y >= minY
					    && gWiiPointer.mPointer.y <= maxY) {
						work->mDigitNum = 1;
						work->mCursorY  = i / 3;
						work->mCursorX  = i % 3;
						break;
					}
				}
			}
		}
		if (gWiiPointer.Check(1) && work->mDigitNum) {
			work->mLocked = 1;
			u8 ch         = decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar;
			PlayWiiSoundEffect(&gGame.mSound, decaPanelKeySounds[work->mCursorX + work->mCursorY * 3], 1.0f, 0.0f);
			if (ch == 'C') {
				work->mDigits[0]  = ' ';
				work->mDigits[1]  = ' ';
				work->mDigits[2]  = ' ';
				work->mDigits[3]  = ' ';
				work->mDigits[4]  = ' ';
				work->mDigits[5]  = ' ';
				work->mDigitCount = 0;
				work->mCode       = 0;
			} else {
				if (work->mDigitCount < 6) {
					if (work->mDigitCount != 0) {
						work->mDigits[0] = work->mDigits[1];
						work->mDigits[1] = work->mDigits[2];
						work->mDigits[2] = work->mDigits[3];
						work->mDigits[3] = work->mDigits[4];
						work->mDigits[4] = work->mDigits[5];
					}
					work->mDigits[5] = ch;
					work->mDigitCount++;
				}
				if (work->mDigitCount >= 6) {
					work->mCode = 0;
					for (s32 i = 0; i < 6; i++) {
						if (work->mDigits[i] != ' ') {
							work->mCode = work->mCode * 10 + (work->mDigits[i] - '0');
						}
					}
					work->mActive = 0;
					gGame.ClearStatusFlags(2);
					gGame.mPointer._110 = NULL;
				}
			}
		}
		if (gWiiPointer.Check(2)) {
			work->mCode   = -1;
			work->mActive = 0;
			gGame.ClearStatusFlags(2);
			gGame.mPointer._110 = NULL;
		}
	}

	if (work->mFade > 0.0f) {
		gGame.mGs.mCurRenderState->SetBlendMode((GXBlendMode)1, (GXBlendFactor)4, (GXBlendFactor)5, (GXLogicOp)15);
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
		s32 idx        = work->mCursorX + work->mCursorY * 3;
		work->mTargetX = decaPanelKeys[idx].x;
		work->mTargetY = decaPanelKeys[idx].y;
		work->mCurX += 0.8f * (work->mTargetX - work->mCurX);
		work->mCurY += 0.8f * (work->mTargetY - work->mCurY);

		tex       = &work->mTexs[0];
		f32 cx    = gPointerScreenWidth / 2;
		f32 w     = 0.9f * GXGetTexObjWidth(tex);
		f32 h     = 0.9f * GXGetTexObjHeight(tex);
		u32 color = FadeColor(work->mFade);
		DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, cx, 200.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);

		f32 w1                       = 0.9f * (GXGetTexObjWidth(&work->mTexs[1]) / 10);
		f32 h1                       = 0.9f * GXGetTexObjHeight(&work->mTexs[1]);
		f32 w2                       = 0.9f * GXGetTexObjWidth(&work->mTexs[2]);
		f32 h2                       = 0.9f * GXGetTexObjHeight(&work->mTexs[2]);
		DecaPanelKeySprite draws[12] = {
			{ 1, 0.1f, 0.6f, 0, w1, h1 }, { 1, 0.1f, 0.7f, 1, w1, h1 }, { 1, 0.1f, 0.8f, 2, w1, h1 },  { 1, 0.1f, 0.3f, 3, w1, h1 },
			{ 1, 0.1f, 0.4f, 4, w1, h1 }, { 1, 0.1f, 0.5f, 5, w1, h1 }, { 1, 0.1f, 0.0f, 6, w1, h1 },  { 1, 0.1f, 0.1f, 7, w1, h1 },
			{ 1, 0.1f, 0.2f, 8, w1, h1 }, { 2, 1.0f, 0.0f, 9, w2, h2 }, { 2, 1.0f, 0.0f, 10, w2, h2 }, { 1, 0.1f, 0.1f * 9, 11, w1, h1 },
		};
		for (s32 i = 0; i < 12; i++) {
			if (work->mLocked && decaPanelKeys[idx].mChar == decaPanelKeys[i].mChar) {
				continue;
			}
			tex = &work->mTexs[draws[i].mTex];
			PSMTXScale(tex->mTexMtx.mMtx[2], draws[i].mScale, 1.0f, 1.0f);
			tex->mTexMtx.mDirty = TRUE;
			PSMTXTrans(tex->mTexMtx.mMtx[0], draws[i].mTrans, 0.0f, 0.0f);
			tex->mTexMtx.mDirty = TRUE;
			f32 x               = 0.9f * decaPanelKeys[draws[i].mKey].x;
			u8 intensity        = decaPanelKeys[idx].mChar == decaPanelKeys[i].mChar && work->mDigitNum ? 0xFF : 0xC8;
			u32 color           = (u32)(255.0f * work->mFade) | (intensity << 24) | (intensity << 16) | (intensity << 8);
			f32 y               = 0.9f * decaPanelKeys[draws[i].mKey].y;
			DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, cx + x, 200.0f + y, 0.0f, draws[i].mW, draws[i].mH, 0.0f, 0.0f,
			                   0.0f);
		}

		tex = &work->mTexs[4];
		PSMTXScale(tex->mTexMtx.mMtx[2], 0.1f, 1.0f, 1.0f);
		tex->mTexMtx.mDirty = TRUE;
		w2                  = 0.9f * (GXGetTexObjWidth(tex) / 10);
		h2                  = 0.9f * GXGetTexObjHeight(tex);
		for (s32 i = 0; i < 6; i++) {
			u8 ch = work->mDigits[i];
			if (ch != ' ') {
				PSMTXTrans(tex->mTexMtx.mMtx[0], 0.1f * decaPanelDigitCells[ch - '0'], 0.0f, 0.0f);
				tex->mTexMtx.mDirty = TRUE;
				f32 x               = 0.9f * decaPanelDigitPos[i][0];
				f32 y               = 0.9f * decaPanelDigitPos[i][1];
				u32 color           = FadeColor(work->mFade);
				DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, cx + x, 200.0f + y, 0.0f, w2, h2, 0.0f, 0.0f, 0.0f);
			}
		}

		gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
	}
}
#else
#include "game/relmodule.h"
#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/csound.h"
#include "game/gmath.h"
#include "game/video.h"
#include <math.h>
#include <sysdolphin/axdriver.h>

extern "C" {
#include <sysdolphin/random.h>
void AXDriverSetAllStreamPitches(s32 pitch);
}

static inline CCameraActionScroll2* GetScrollCamera()
{
	return static_cast<CCameraActionScroll2*>(gGame.mCameraActions.mTable[CAMACTION_SCROLL2]);
}

struct DecaPanelKey {
	f32 mX;
	f32 mY;
	f32 mTextureOffset;
	s32 mSound;
	u8 mChar;
};

class DecaPanel {
public:
	u32 mState;       // _000
	CTpl mTpl;        // _004
	s32 mActive;      // _1F4
	f32 mFade;        // _1F8
	CTexObj mTexs[5]; // _1FC
	u8 mDigits[6];    // _6FC
	u8 mDigitNum;     // _702
	s32 mCode;        // _704
	s8 mCursorX;      // _708
	s8 mCursorY;      // _709
	s8 mLocked;       // _70A
	f32 mCurX;        // _70C
	f32 mCurY;        // _710
	f32 mTargetX;     // _714
	f32 mTargetY;     // _718

	void ClearDigits();
	void ResetInput();
	void LoadTextures();
};

struct SavedObjFlag {
	s32 mId;   // _00
	u8 mFlags; // _04
};

struct SavedObjFlagList {
	void Clear();
	void Apply(s32 mode);

	SavedObjFlag mEntries[300]; // _000
	s32 mNum;                   // _960
};

extern "C" {
s32 CmdGetDecaPassword(s32 cmd, s32 count, s32* args);
s32 CmdIsDecaPanelOpen(s32 cmd, s32 count, s32* args);
s32 CmdCloseDecaPanel(s32 cmd, s32 count, s32* args);
s32 CmdOpenDecaPanel(s32 cmd, s32 count, s32* args);
s32 CmdApplySavedObjFlags(s32 cmd, s32 count, s32* args);
s32 CmdSaveObjFlag(s32 cmd, s32 count, s32* args);
s32 CmdClearSavedObjFlags(s32 cmd, s32 count, s32* args);
s32 CmdClearObjFlagList(s32 cmd, s32 count, s32* args);
s32 CmdAddObjFlagList(s32 cmd, s32 count, s32* args);
s32 CmdNop(s32 cmd, s32 count, s32* args);
void ExitStage(s32 arg);
void DrawStageHud(s32 arg);
void UpdateStage(s32 frames);
void InitStage(s32 arg);
void DecaPanel_Update(DecaPanel* work);
}

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage, NULL, NULL, DrawStageHud, NULL, ExitStage };
RelCmdFunc _prolog_cmds[]
    = { CmdAddObjFlagList, CmdClearObjFlagList, CmdClearSavedObjFlags, CmdSaveObjFlag,    CmdApplySavedObjFlags, CmdNop, CmdNop,
	    CmdOpenDecaPanel,  CmdCloseDecaPanel,   CmdIsDecaPanelOpen,    CmdGetDecaPassword };
}

// Both cells of the wide clear key share a position, sound and glyph.
static const DecaPanelKey decaPanelKeys[12] = {
	{ -58.0f, -40.0f, 0.6f, 0x5000E, '7' }, { -1.0f, -40.0f, 0.7f, 0x5000F, '8' },  { 56.0f, -40.0f, 0.8f, 0x50010, '9' },
	{ -58.0f, 17.0f, 0.3f, 0x5000B, '4' },  { -1.0f, 17.0f, 0.4f, 0x5000C, '5' },   { 56.0f, 17.0f, 0.5f, 0x5000D, '6' },
	{ -58.0f, 74.0f, 0.0f, 0x50008, '1' },  { -1.0f, 74.0f, 0.1f, 0x50009, '2' },   { 56.0f, 74.0f, 0.2f, 0x5000A, '3' },
	{ -29.0f, 131.0f, 0.0f, 0x50012, 'C' }, { -29.0f, 131.0f, 0.0f, 0x50012, 'C' }, { 56.0f, 131.0f, 0.1f * 9.0f, 0x50011, '0' },
};

static f32 decaPanelDigitPos[6][2] = {
	{ -68.0f, -115.0f }, { -41.0f, -115.0f }, { -14.0f, -115.0f }, { 13.0f, -115.0f }, { 40.0f, -115.0f }, { 67.0f, -115.0f },
};

// which cell of the digit strip each of '0' to '9' uses
static s32 decaPanelDigitCells[10] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8 };

static DecaPanel decaPanel;
static SavedObjFlagList savedObjFlags;

inline void DecaPanel::ClearDigits()
{
	for (u32 i = 0; i < ARRAY_SIZE(mDigits); ++i) {
		mDigits[i] = ' ';
	}
	mDigitNum = 0;
	mCode     = 0;
}

inline void DecaPanel::ResetInput()
{
	ClearDigits();
	mCursorX                  = 0;
	mCursorY                  = 3;
	mLocked                   = 0;
	const DecaPanelKey& clear = decaPanelKeys[mCursorX + mCursorY * 3];
	mCurX = mTargetX = clear.mX;
	mCurY = mTargetY = clear.mY;
}

// white at fade alpha
inline u32 FadeColor(f32 fade)
{
	return (u32)(255.0f * fade) | 0xFFFFFF00;
}

inline void DecaPanel::LoadTextures()
{
	if (mState != 1 || !mTpl.ReadEx("tpl/stage03.tpl", 0, 0)) {
		return;
	}

	const TPLPalette* palette = static_cast<const TPLPalette*>(mTpl.mBuffer);
	for (u32 i = 0; i < ARRAY_SIZE(mTexs); ++i) {
		mTexs[i] = mTpl.mTex.Init(&palette->descriptorArray[i], 0);
	}
	mState  = 2;
	mActive = 0;
	mFade   = 0.0f;
}

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void SavedObjFlagList::Clear()
{
	int i;
	mNum = 0;
	for (i = 0; i < 300; i++) {
		mEntries[i].mId    = -1;
		mEntries[i].mFlags = 0;
	}
}

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void SavedObjFlagList::Apply(s32 mode)
{
	s32 i;

	if (mode != 0) {
		s32 j;
		if (mNum <= 0) {
			return;
		}
		for (j = 0; j < mNum; j++) {
			if (mEntries[j].mId == -1) {
				break;
			}
			XObj* obj = FindObj(mEntries[j].mId);
			if (mEntries[j].mFlags & 1) { // only entries saved with the flag get it back
				obj->mStatus.mFlags |= 0x10000ULL;
			}
		}
	} else {
		if (mNum <= 0) {
			return;
		}
		for (i = 0; i < mNum; i++) {
			if (mEntries[i].mId == -1) {
				break;
			}
			XObj* obj = FindObj(mEntries[i].mId);
			obj->mStatus.mFlags &= ~0x10000ULL;
		}
	}
}

/**
 * @note Address: stage03 .text 0x0
 * @note Size: 0x10
 */
extern "C" s32 CmdGetDecaPassword(s32 cmd, s32 count, s32* args)
{
	return decaPanel.mCode;
}

/**
 * @note Address: stage03 .text 0x10
 * @note Size: 0x10
 */
extern "C" s32 CmdIsDecaPanelOpen(s32 cmd, s32 count, s32* args)
{
	return decaPanel.mActive;
}

/**
 * @note Address: stage03 .text 0x20
 * @note Size: 0x40
 */
extern "C" s32 CmdCloseDecaPanel(s32 cmd, s32 count, s32* args)
{
	decaPanel.mActive = 0;
	gGame.mStatus.mFlags &= ~2ULL; // the panel's game flag
	return 0;
}

/**
 * @note Address: stage03 .text 0x60
 * @note Size: 0x94
 */
// NONMATCHING: retail's key table stores only x/y/character (12-byte entries), while our combined
// DecaPanelKey also carries sound and texture data. Fixing the remaining index arithmetic needs those data
// paths split too.
extern "C" s32 CmdOpenDecaPanel(s32 cmd, s32 count, s32* args)
{
	decaPanel.ResetInput();
	decaPanel.mActive = 1;
	gGame.mStatus.mFlags |= 2ULL;
	return 0;
}

/**
 * @note Address: stage03 .text 0xF4
 * @note Size: 0xBC8
 */
extern "C" void DecaPanel_Update(DecaPanel* work)
{
	if (work->mState != 2) {
		return;
	}

	if (work->mActive) {
		if (work->mFade < 1.0f && (work->mFade += (1.0f / (0.2f * FRAME_RATE)) * 2.0f) > 1.0f) {
			work->mFade = 1.0f;
		}
	} else if (work->mFade > 0.0f && (work->mFade -= (1.0f / (0.2f * FRAME_RATE)) * 2.0f) < 0.0f) {
		work->mFade = 0.0f;
	}

	if (work->mFade >= 1.0f) {
		const CPadOne& pad = gGame.mPad.mPad[0];
		u8 cur             = decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar;
		if (pad.mButton & PAD_BUTTON_A) { // A
			work->mLocked = 1;
		} else {
			work->mLocked = 0;
			if (pad.mDirRepeat & 0x0001) { // left
				do {                       // skip cells that show the same glyph
					if (--work->mCursorX < 0) {
						work->mCursorX = 2;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			} else if (pad.mDirRepeat & 0x0002) { // right
				do {
					if (++work->mCursorX > 2) {
						work->mCursorX = 0;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			}
			if (pad.mDirRepeat & 0x0008) { // up
				do {
					if (--work->mCursorY < 0) {
						work->mCursorY = 3;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			} else if (pad.mDirRepeat & 0x0004) { // down
				do {
					if (++work->mCursorY > 3) {
						work->mCursorY = 0;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			}
		}

		if (pad.mTrigger & PAD_BUTTON_A) { // A confirms the key
			const DecaPanelKey& key = decaPanelKeys[work->mCursorX + work->mCursorY * 3];
			PlayGameSound(&gGame.mSound, key.mSound, 0xFF);
			if (key.mChar == 'C') {
				work->ClearDigits();
			} else {
				if (work->mDigitNum < ARRAY_SIZE(work->mDigits)) {
					if (work->mDigitNum != 0) {
						for (u32 i = 1; i < ARRAY_SIZE(work->mDigits); ++i) {
							work->mDigits[i - 1] = work->mDigits[i];
						}
					}
					work->mDigits[ARRAY_SIZE(work->mDigits) - 1] = key.mChar;
					work->mDigitNum++;
				}
				if (work->mDigitNum >= ARRAY_SIZE(work->mDigits)) {
					s32 code = 0;
					for (u32 i = 0; i < ARRAY_SIZE(work->mDigits); ++i) {
						u8 digit = work->mDigits[i];
						if (digit != ' ') {
							code = 10 * code + (digit - '0');
						}
					}
					work->mCode   = code;
					work->mActive = 0;
					gGame.mStatus.mFlags &= ~2ULL;
				}
			}
		}
		if (pad.mTrigger & PAD_BUTTON_B) { // B cancels
			work->mCode   = -1;
			work->mActive = 0;
			gGame.mStatus.mFlags &= ~2ULL;
		}
	}

	if (work->mFade > 0.0f) {
		CTexObj* tex;
		f32 keyW, keyH, wideW, wideH;
		s32 i;

		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);

		const DecaPanelKey& selected = decaPanelKeys[work->mCursorX + work->mCursorY * 3];
		work->mTargetX               = selected.mX;
		work->mTargetY               = selected.mY;
		// the cursor sprite chases the current key
		work->mCurX += 0.8f * (work->mTargetX - work->mCurX);
		work->mCurY += 0.8f * (work->mTargetY - work->mCurY);

		tex = &work->mTexs[0];
		{
			u16 w     = GXGetTexObjWidth(tex);
			u16 h     = GXGetTexObjHeight(tex);
			u32 color = FadeColor(work->mFade);
			DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, 320.0f, 200.0f, 0.0f, 0.9f * w, 0.9f * h, 0.0f, 0.0f, 0.0f);
		}

		// tex1 is a 10-cell digit strip, one cell per key
		keyW  = 0.9f * (GXGetTexObjWidth(&work->mTexs[1]) / 10);
		keyH  = 0.9f * GXGetTexObjHeight(&work->mTexs[1]);
		wideW = 0.9f * GXGetTexObjWidth(&work->mTexs[2]);
		wideH = 0.9f * GXGetTexObjHeight(&work->mTexs[2]);

		for (i = 0; i < ARRAY_SIZE(decaPanelKeys); i++) {
			const DecaPanelKey& key = decaPanelKeys[i];
			if (work->mLocked && selected.mChar == key.mChar) {
				continue;
			}

			bool clear = key.mChar == 'C';
			tex        = &work->mTexs[clear ? 2 : 1];
			PSMTXScale(tex->mMtx[1], clear ? 1.0f : 0.1f, 1.0f, 1.0f);
			PSMTXTrans(tex->mMtx[0], key.mTextureOffset, 0.0f, 0.0f);
			tex->mMtxDirty = TRUE;

			f32 x     = 0.9f * key.mX;
			f32 y     = 0.9f * key.mY;
			u32 color = FadeColor(work->mFade);
			DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, 320.0f + x, 200.0f + y, 0.0f, clear ? wideW : keyW,
			                   clear ? wideH : keyH, 0.0f, 0.0f, 0.0f);
		}

		// tex4 is the digit strip for the typed entry
		tex = &work->mTexs[4];
		PSMTXScale(tex->mMtx[1], 0.1f, 1.0f, 1.0f);
		tex->mMtxDirty = TRUE;
		keyW           = 0.9f * (GXGetTexObjWidth(tex) / 10);
		keyH           = 0.9f * GXGetTexObjHeight(tex);
		for (i = 0; i < 6; i++) {
			u8 ch = work->mDigits[i];
			if (ch != ' ') {
				PSMTXTrans(tex->mMtx[0], 0.1f * decaPanelDigitCells[ch - '0'], 0.0f, 0.0f);
				tex->mMtxDirty = TRUE;
				u32 color      = FadeColor(work->mFade);
				DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, 320.0f + 0.9f * decaPanelDigitPos[i][0],
				                   200.0f + 0.9f * decaPanelDigitPos[i][1], 0.0f, keyW, keyH, 0.0f, 0.0f, 0.0f);
			}
		}

		// tex3 is the cursor
		tex  = &work->mTexs[3];
		keyW = 0.9f * GXGetTexObjWidth(tex);
		keyH = 0.9f * GXGetTexObjHeight(tex);
		{
			u32 color = FadeColor(work->mFade);
			DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, keyW / 3.0f + (320.0f + 0.9f * work->mCurX),
			                   keyH / 3.0f + (200.0f + 0.9f * work->mCurY), 0.0f, keyW, keyH, 0.0f, 0.0f, 0.0f);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
}

/**
 * @note Address: stage03 .text 0xCBC
 * @note Size: 0x158
 */
extern "C" s32 CmdApplySavedObjFlags(s32 cmd, s32 count, s32* args)
{
	savedObjFlags.Apply(args[0]);
	return 0;
}

/**
 * @note Address: stage03 .text 0xE14
 * @note Size: 0xF0
 */
extern "C" s32 CmdSaveObjFlag(s32 cmd, s32 count, s32* args)
{
	s32 id = args[0];
	if (savedObjFlags.mNum < 300) {
		savedObjFlags.mEntries[savedObjFlags.mNum].mId = id;
		XObj* obj                                      = FindObj(savedObjFlags.mEntries[savedObjFlags.mNum].mId);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) { // flag 1 gates whether it saves
			if (obj->mStatus.mFlags & 0x10000) {
				savedObjFlags.mEntries[savedObjFlags.mNum].mFlags |= 1;
			}
			savedObjFlags.mNum++;
		}
	}
	return 0;
}

/**
 * @note Address: stage03 .text 0xF04
 * @note Size: 0xA4
 */
extern "C" s32 CmdClearSavedObjFlags(s32 cmd, s32 count, s32* args)
{
	savedObjFlags.Clear();
	return 0;
}

/**
 * @note Address: stage03 .text 0xFA8
 * @note Size: 0x34
 */
extern "C" s32 CmdClearObjFlagList(s32 cmd, s32 count, s32* args)
{
	GetScrollCamera()->ClearHideList();
	return 0;
}

/**
 * @note Address: stage03 .text 0xFDC
 * @note Size: 0x78
 */
extern "C" s32 CmdAddObjFlagList(s32 cmd, s32 count, s32* args)
{
	s32 i;
	for (i = 0; i < count; i++) {
		GetScrollCamera()->AddHideObj(args[i]);
	}
	return 0;
}

/**
 * @note Address: stage03 .text 0x1054
 * @note Size: 0x8
 */
extern "C" s32 CmdNop(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage03 .text 0x105C
 * @note Size: 0x3C
 */
extern "C" void ExitStage(s32 arg)
{
	decaPanel.mState = 0;
	decaPanel.mTpl.Close();
}

/**
 * @note Address: stage03 .text 0x1098
 * @note Size: 0x28
 */
extern "C" void DrawStageHud(s32 arg)
{
	DecaPanel_Update(&decaPanel);
}

/**
 * @note Address: stage03 .text 0x10C0
 * @note Size: 0x35C
 */
extern "C" void UpdateStage(s32 frames)
{
	static s32 sTimer = 0;

	sTimer += frames;
	if (IsGameSoundPlaying(&gGame.mSound, 0xFFFF000E)) {
		AXDriverSetAllStreamPitches(5.0f * (f32)sin(DEG2RAD * sTimer)); // wobble the stream's pitch
	}
	if (IsGameSoundPlaying(&gGame.mSound, 0xFFFF000F)) {
		AXDriverSetAllStreamPitches(5.0f * (f32)sin(DEG2RAD * sTimer));
	}
	// a rare ambient sound, only while no event script is loaded
	if (gGame.mEvt.mData.mScript == NULL && HSD_Randi(5000) < 10) {
		switch (HSD_Randi(4)) {
		case 0:
			PlayGameSound(&gGame.mSound, 0x50004, 0xFF);
			break;
		case 1:
			PlayGameSound(&gGame.mSound, 0x50005, 0xFF);
			break;
		case 2:
			PlayGameSound(&gGame.mSound, 0x50006, 0xFF);
			break;
		case 3:
			PlayGameSound(&gGame.mSound, 0x50007, 0xFF);
			break;
		}
	}

	decaPanel.LoadTextures();
}

/**
 * @note Address: stage03 .text 0x141C
 * @note Size: 0x84
 */
extern "C" void InitStage(s32 arg)
{
	decaPanel.mActive = 0;
	decaPanel.mState  = 1;
	decaPanel.mFade   = 0.0f;
	decaPanel.ResetInput();
}

/**
 * @note Address: stage03 .text 0x14A0
 * @note Size: 0xAC
 */
// static initializer for decaPanel

/**
 * @note Address: stage03 .text 0x154C
 * @note Size: 0xA8
 */
// DecaPanel::~DecaPanel, compiler made

/**
 * @note Address: stage03 .text 0x15F4
 * @note Size: 0x10
 */
// CTexObj::CTexObj, inline in the class above

/**
 * @note Address: stage03 .text 0x1604
 * @note Size: 0x48
 */
// CTexObj::~CTexObj, inline in the class above
#endif

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

// CmdOpenDecaPanel

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
			if (pad.mDirRepeat & PAD_BUTTON_LEFT) { // left
				do {                                // skip cells that show the same glyph
					if (--work->mCursorX < 0) {
						work->mCursorX = 2;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			} else if (pad.mDirRepeat & PAD_BUTTON_RIGHT) { // right
				do {
					if (++work->mCursorX > 2) {
						work->mCursorX = 0;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			}
			if (pad.mDirRepeat & PAD_BUTTON_UP) { // up
				do {
					if (--work->mCursorY < 0) {
						work->mCursorY = 3;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			} else if (pad.mDirRepeat & PAD_BUTTON_DOWN) { // down
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

// DecaPanel_Update

/**
 * @note Address: stage03 .text 0xCBC
 * @note Size: 0x158
 */
extern "C" s32 CmdApplySavedObjFlags(s32 cmd, s32 count, s32* args)
{
	savedObjFlags.Apply(args[0]);
	return 0;
}

// CmdApplySavedObjFlags

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

// InitStage

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

#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/ctpl.h"
#include "game/evt.h"
#include "game/gmath.h"
#include "game/video.h"
#include "Version.h"

extern "C" {
f64 sin(f64 x);
s32 HSD_Randi(s32 max);
void PSMTXScale(Mtx m, f32 x, f32 y, f32 z);
void PSMTXTrans(Mtx m, f32 x, f32 y, f32 z);
}

extern "C" void AXDriverSetAllStreamPitches(s32 pitch);

struct DecaPanelKey {
	f32 x;    // _00
	f32 y;    // _04
	u8 mChar; // _08
};

struct DecaPanelKeySprite {
	s32 mTex;   // _00
	f32 mScale; // _04
	f32 mTrans; // _08
	s32 mKey;   // _0C
	f32 mW;     // _10
	f32 mH;     // _14
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

	void ResetInput();
	void LoadTextures();
	void Close()
	{
		mState = 0;
		mTpl.Close();
	}
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
void ExitStage(void* arg);
void DrawStageHud(void* arg);
void UpdateStage(s32 frames);
void InitStage(void* arg);
void DecaPanel_Update(DecaPanel* work);
}

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage, (RelSlotFunc)UpdateStage, NULL, NULL, DrawStageHud, NULL, ExitStage };
RelCmdFunc _prolog_cmds[]
    = { CmdAddObjFlagList, CmdClearObjFlagList, CmdClearSavedObjFlags, CmdSaveObjFlag,    CmdApplySavedObjFlags, CmdNop, CmdNop,
	    CmdOpenDecaPanel,  CmdCloseDecaPanel,   CmdIsDecaPanelOpen,    CmdGetDecaPassword };
}

// 4 rows of 3 keys
static DecaPanelKey decaPanelKeys[12] = {
	{ -58.0f, -40.0f, '7' }, { -1.0f, -40.0f, '8' },  { 56.0f, -40.0f, '9' },  { -58.0f, 17.0f, '4' },
	{ -1.0f, 17.0f, '5' },   { 56.0f, 17.0f, '6' },   { -58.0f, 74.0f, '1' },  { -1.0f, 74.0f, '2' },
	{ 56.0f, 74.0f, '3' },   { -29.0f, 131.0f, 'C' }, { -29.0f, 131.0f, 'C' }, { 56.0f, 131.0f, '0' },
};

static s32 decaPanelKeySounds[12] = {
	0x5000E, 0x5000F, 0x50010, 0x5000B, 0x5000C, 0x5000D, 0x50008, 0x50009, 0x5000A, 0x50012, 0x50012, 0x50011,
};

static f32 decaPanelDigitPos[6][2] = {
	{ -68.0f, -115.0f }, { -41.0f, -115.0f }, { -14.0f, -115.0f }, { 13.0f, -115.0f }, { 40.0f, -115.0f }, { 67.0f, -115.0f },
};

// which cell of the digit strip each of '0' to '9' uses
static s32 decaPanelDigitCells[10] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8 };

static DecaPanel decaPanel;
static SavedObjFlagList savedObjFlags;

inline void DecaPanel::ResetInput()
{
	mDigits[0] = ' ';
	mDigits[1] = ' ';
	mDigits[2] = ' ';
	mDigits[3] = ' ';
	mDigits[4] = ' ';
	mDigits[5] = ' ';
	mDigitNum  = 0;
	mCode      = 0;
	mCursorX   = 0;
	mCursorY   = 3;
	mLocked    = 0;
	mCurX = mTargetX = decaPanelKeys[mCursorX + mCursorY * 3].x;
	mCurY = mTargetY = decaPanelKeys[mCursorX + mCursorY * 3].y;
}

inline u32 FadeColor(f32 fade)
{
	return (u32)(255.0f * fade) | 0xFFFFFF00;
}

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void DecaPanel::LoadTextures()
{
	s32 i;

	switch (mState) {
	case 1:
		if (mTpl.ReadEx("tpl/stage03.tpl", 0, 0)) {
			for (i = 0; i < 5; i++) {
				mTexs[i] = mTpl.mTex.Init(&((TPLPalette*)mTpl.mBuffer)->descriptorArray[i], 0);
			}
			mState  = 2;
			mActive = 0;
			mFade   = 0.0f;
		}
		break;
	case 2:
		break;
	}
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
			if (mEntries[j].mFlags & 1) {
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
	gGame.mStatus.mFlags &= ~2ULL;
	return 0;
}

/**
 * @note Address: stage03 .text 0x60
 * @note Size: 0x94
 */
// NONMATCHING: regalloc, the key-index offset and the decaPanel
// base swap r6/r8; instruction sequence is identical.
extern "C" s32 CmdOpenDecaPanel(s32 cmd, s32 count, s32* args)
{
	decaPanel.ResetInput();
	decaPanel.mActive = 1;
	gGame.mStatus.mFlags |= 2ULL;
	return 0;
}

// CmdOpenDecaPanel
/*
    li       r7, 0x3
    lis      r3, gGame@ha
    mulli    r8, r7, 0x24
    lis      r6, decaPanel@ha
    lis      r4, decaPanelKeys@ha
    addi     r5, r3, gGame@l
    addi     r3, r4, decaPanelKeys@l
    lwz      r0, 0x1b14(r5)
    lfsx     f1, r3, r8
    add      r3, r3, r8
    addi     r9, r6, decaPanel@l
    li       r10, 0x20
    li       r8, 0x0
    lfs      f0, 0x4(r3)
    lwz      r4, 0x1b10(r5)
    ori      r0, r0, 0x2
    li       r6, 0x1
    stb      r10, 0x6fc(r9)
    li       r3, 0x0
    stb      r10, 0x6fd(r9)
    stb      r10, 0x6fe(r9)
    stb      r10, 0x6ff(r9)
    stb      r10, 0x700(r9)
    stb      r10, 0x701(r9)
    stb      r8, 0x702(r9)
    stw      r8, 0x704(r9)
    stb      r8, 0x708(r9)
    stb      r7, 0x709(r9)
    stb      r8, 0x70a(r9)
    stfs     f1, 0x714(r9)
    stfs     f1, 0x70c(r9)
    stfs     f0, 0x718(r9)
    stfs     f0, 0x710(r9)
    stw      r6, 0x1f4(r9)
    stw      r0, 0x1b14(r5)
    stw      r4, 0x1b10(r5)
    blr
*/

/**
 * @note Address: stage03 .text 0xF4
 * @note Size: 0xBC8
 */
// NONMATCHING: regalloc/scheduling through the draw section. Retail keeps one fewer fp callee
// save live, associates the digit-code accumulation differently, does not fuse the
// 0.9f * pos->y multiply into an fmadds, and addresses key->mChar through a dedicated pointer.
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
		if (pad.mButton & 0x100) {
			work->mLocked = 1;
		} else {
			work->mLocked = 0;
			if (pad.mDirRepeat & 1) {
				do {
					if (--work->mCursorX < 0) {
						work->mCursorX = 2;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			} else if (pad.mDirRepeat & 2) {
				do {
					if (++work->mCursorX > 2) {
						work->mCursorX = 0;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			}
			if (pad.mDirRepeat & 8) {
				do {
					if (--work->mCursorY < 0) {
						work->mCursorY = 3;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			} else if (pad.mDirRepeat & 4) {
				do {
					if (++work->mCursorY > 3) {
						work->mCursorY = 0;
					}
				} while (cur == decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar);
			}
		}

		if (pad.mTrigger & 0x100) {
			u8 ch = decaPanelKeys[work->mCursorX + work->mCursorY * 3].mChar;
			PlayGameSound(&gGame.mSound, decaPanelKeySounds[work->mCursorX + work->mCursorY * 3], 0xFF);
			if (ch == 'C') {
				work->mDigits[0] = ' ';
				work->mDigits[1] = ' ';
				work->mDigits[2] = ' ';
				work->mDigits[3] = ' ';
				work->mDigits[4] = ' ';
				work->mDigits[5] = ' ';
				work->mDigitNum  = 0;
				work->mCode      = 0;
			} else {
				if (work->mDigitNum < 6) {
					if (work->mDigitNum != 0) {
						work->mDigits[0] = work->mDigits[1];
						work->mDigits[1] = work->mDigits[2];
						work->mDigits[2] = work->mDigits[3];
						work->mDigits[3] = work->mDigits[4];
						work->mDigits[4] = work->mDigits[5];
					}
					work->mDigits[5] = ch;
					work->mDigitNum++;
				}
				if (work->mDigitNum >= 6) {
					work->mCode = 0;
					if (work->mDigits[0] != ' ') {
						work->mCode = work->mCode * 10 + work->mDigits[0] - '0';
					}
					if (work->mDigits[1] != ' ') {
						work->mCode = work->mCode * 10 + work->mDigits[1] - '0';
					}
					if (work->mDigits[2] != ' ') {
						work->mCode = work->mCode * 10 + work->mDigits[2] - '0';
					}
					if (work->mDigits[3] != ' ') {
						work->mCode = work->mCode * 10 + work->mDigits[3] - '0';
					}
					if (work->mDigits[4] != ' ') {
						work->mCode = work->mCode * 10 + work->mDigits[4] - '0';
					}
					if (work->mDigits[5] != ' ') {
						work->mCode = work->mCode * 10 + work->mDigits[5] - '0';
					}
					work->mActive = 0;
					gGame.mStatus.mFlags &= ~2ULL;
				}
			}
		}
		if (pad.mTrigger & 0x200) {
			work->mCode   = -1;
			work->mActive = 0;
			gGame.mStatus.mFlags &= ~2ULL;
		}
	}

	if (work->mFade > 0.0f) {
		DecaPanelKey* key;
		CTexObj* tex;
		f32 keyW, keyH, wideW, wideH;
		s32 i;

		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);

		key            = &decaPanelKeys[work->mCursorX + work->mCursorY * 3];
		work->mTargetX = key->x;
		work->mTargetY = key->y;
		work->mCurX += 0.8f * (work->mTargetX - work->mCurX);
		work->mCurY += 0.8f * (work->mTargetY - work->mCurY);

		tex = &work->mTexs[0];
		{
			u16 w     = GXGetTexObjWidth(tex);
			u16 h     = GXGetTexObjHeight(tex);
			u32 color = FadeColor(work->mFade);
			DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, 320.0f, 200.0f, 0.0f, 0.9f * w, 0.9f * h, 0.0f, 0.0f, 0.0f);
		}

		keyW  = 0.9f * (GXGetTexObjWidth(&work->mTexs[1]) / 10);
		keyH  = 0.9f * GXGetTexObjHeight(&work->mTexs[1]);
		wideW = 0.9f * GXGetTexObjWidth(&work->mTexs[2]);
		wideH = 0.9f * GXGetTexObjHeight(&work->mTexs[2]);

		DecaPanelKeySprite draws[12] = {
			{ 1, 0.1f, 0.1f * 6, 0, 0.0f, 0.0f }, { 1, 0.1f, 0.1f * 7, 1, 0.0f, 0.0f }, { 1, 0.1f, 0.1f * 8, 2, 0.0f, 0.0f },
			{ 1, 0.1f, 0.1f * 3, 3, 0.0f, 0.0f }, { 1, 0.1f, 0.1f * 4, 4, 0.0f, 0.0f }, { 1, 0.1f, 0.1f * 5, 5, 0.0f, 0.0f },
			{ 1, 0.1f, 0.1f * 0, 6, 0.0f, 0.0f }, { 1, 0.1f, 0.1f * 1, 7, 0.0f, 0.0f }, { 1, 0.1f, 0.1f * 2, 8, 0.0f, 0.0f },
			{ 2, 1.0f, 0.0f, 9, 0.0f, 0.0f },     { 2, 1.0f, 0.0f, 10, 0.0f, 0.0f },    { 1, 0.1f, 0.1f * 9, 11, 0.0f, 0.0f },
		};
		draws[0].mW  = keyW;
		draws[0].mH  = keyH;
		draws[1].mW  = keyW;
		draws[1].mH  = keyH;
		draws[2].mW  = keyW;
		draws[2].mH  = keyH;
		draws[3].mW  = keyW;
		draws[3].mH  = keyH;
		draws[4].mW  = keyW;
		draws[4].mH  = keyH;
		draws[5].mW  = keyW;
		draws[5].mH  = keyH;
		draws[6].mW  = keyW;
		draws[6].mH  = keyH;
		draws[7].mW  = keyW;
		draws[7].mH  = keyH;
		draws[8].mW  = keyW;
		draws[8].mH  = keyH;
		draws[9].mW  = wideW;
		draws[9].mH  = wideH;
		draws[10].mW = wideW;
		draws[10].mH = wideH;
		draws[11].mW = keyW;
		draws[11].mH = keyH;

		for (i = 0; i < 12; i++) {
			if (work->mLocked && key->mChar == decaPanelKeys[i].mChar) {
				continue;
			}
			tex = &work->mTexs[draws[i].mTex];
			PSMTXScale(tex->mMtx[1], draws[i].mScale, 1.0f, 1.0f);
			tex->mMtxDirty = 1;
			PSMTXTrans(tex->mMtx[0], draws[i].mTrans, 0.0f, 0.0f);
			tex->mMtxDirty    = 1;
			DecaPanelKey* pos = &decaPanelKeys[draws[i].mKey];
			f32 x             = 0.9f * pos->x;
			u32 color         = FadeColor(work->mFade);
			DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, 320.0f + x, 200.0f + 0.9f * pos->y, 0.0f, draws[i].mW,
			                   draws[i].mH, 0.0f, 0.0f, 0.0f);
		}

		tex = &work->mTexs[4];
		PSMTXScale(tex->mMtx[1], 0.1f, 1.0f, 1.0f);
		tex->mMtxDirty = 1;
		keyW           = 0.9f * (GXGetTexObjWidth(tex) / 10);
		keyH           = 0.9f * GXGetTexObjHeight(tex);
		for (i = 0; i < 6; i++) {
			u8 ch = work->mDigits[i];
			if (ch != ' ') {
				PSMTXTrans(tex->mMtx[0], 0.1f * decaPanelDigitCells[ch - '0'], 0.0f, 0.0f);
				tex->mMtxDirty = 1;
				u32 color      = FadeColor(work->mFade);
				DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, 320.0f + 0.9f * decaPanelDigitPos[i][0],
				                   200.0f + 0.9f * decaPanelDigitPos[i][1], 0.0f, keyW, keyH, 0.0f, 0.0f, 0.0f);
			}
		}

		tex  = &work->mTexs[3];
		keyW = 0.9f * GXGetTexObjWidth(tex);
		keyH = 0.9f * GXGetTexObjHeight(tex);
		{
			f32 x = 0.9f * work->mCurX;
			f32 y = 0.9f * work->mCurY;
			x += 320.0f;
			y += 200.0f;
			u32 color = FadeColor(work->mFade);
			DrawGsTexturedQuad(&gGame.mGs, tex, color, color, color, color, keyW / 3.0f + x, keyH / 3.0f + y, 0.0f, keyW, keyH, 0.0f, 0.0f,
			                   0.0f);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
}

// DecaPanel_Update
/*
    stwu     r1, -0x1b0(r1)
    mflr     r0
    stw      r0, 0x1b4(r1)
    stfd     f31, 0x1a0(r1)
    psq_st   f31, 0x1a8(r1), 0, qr0
    stfd     f30, 0x190(r1)
    psq_st   f30, 0x198(r1), 0, qr0
    stfd     f29, 0x180(r1)
    psq_st   f29, 0x188(r1), 0, qr0
    stmw     r24, 0x160(r1)
    mr       r31, r3
    lwz      r0, 0x0(r3)
    cmplwi   r0, 0x2
    bne      .L_00000C90
    lwz      r0, 0x1f4(r31)
    cmpwi    r0, 0x0
    beq      .L_0000016C
    lis      r3, lbl_6_rodata_120@ha
    lfs      f2, 0x1f8(r31)
    lfs      f1, lbl_6_rodata_120@l(r3)
    fcmpo    cr0, f2, f1
    bge      .L_0000019C
    lis      r3, lbl_6_rodata_124@ha
    lfs      f0, lbl_6_rodata_124@l(r3)
    fadds    f0, f2, f0
    fcmpo    cr0, f0, f1
    stfs     f0, 0x1f8(r31)
    ble      .L_0000019C
    stfs     f1, 0x1f8(r31)
    b        .L_0000019C
.L_0000016C:
    lis      r3, sDecaZero@ha
    lfs      f2, 0x1f8(r31)
    lfs      f1, sDecaZero@l(r3)
    fcmpo    cr0, f2, f1
    ble      .L_0000019C
    lis      r3, lbl_6_rodata_124@ha
    lfs      f0, lbl_6_rodata_124@l(r3)
    fsubs    f0, f2, f0
    fcmpo    cr0, f0, f1
    stfs     f0, 0x1f8(r31)
    bge      .L_0000019C
    stfs     f1, 0x1f8(r31)
.L_0000019C:
    lis      r3, lbl_6_rodata_120@ha
    lfs      f1, 0x1f8(r31)
    lfs      f0, lbl_6_rodata_120@l(r3)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_00000584
    lbz      r0, 0x709(r31)
    lis      r3, gGame@ha
    addi     r30, r3, gGame@l
    lbz      r4, 0x708(r31)
    extsb    r3, r0
    lwz      r0, 0x19b8(r30)
    mulli    r3, r3, 0x3
    extsb    r4, r4
    lis      r5, decaPanelKeys@ha
    rlwinm.  r0, r0, 0, 23, 23
    add      r4, r4, r3
    mulli    r4, r4, 0xc
    addi     r3, r5, decaPanelKeys@l
    addi     r6, r3, 0x8
    lbzx     r0, r6, r4
    beq      .L_00000200
    li       r0, 0x1
    stb      r0, 0x70a(r31)
    b        .L_0000036C
.L_00000200:
    li       r7, 0x0
    stb      r7, 0x70a(r31)
    lwz      r5, 0x19d4(r30)
    clrlwi.  r4, r5, 31
    beq      .L_00000260
    li       r7, 0x2
.L_00000218:
    lbz      r4, 0x708(r31)
    subi     r5, r4, 0x1
    extsb.   r4, r5
    stb      r5, 0x708(r31)
    bge      .L_00000230
    stb      r7, 0x708(r31)
.L_00000230:
    lbz      r4, 0x709(r31)
    lbz      r5, 0x708(r31)
    extsb    r4, r4
    mulli    r4, r4, 0x3
    extsb    r5, r5
    add      r4, r5, r4
    mulli    r4, r4, 0xc
    add      r4, r3, r4
    lbz      r4, 0x8(r4)
    cmplw    r0, r4
    beq      .L_00000218
    b        .L_000002B0
.L_00000260:
    rlwinm.  r4, r5, 0, 30, 30
    beq      .L_000002B0
.L_00000268:
    lbz      r4, 0x708(r31)
    addi     r5, r4, 0x1
    extsb    r4, r5
    stb      r5, 0x708(r31)
    cmpwi    r4, 0x2
    ble      .L_00000284
    stb      r7, 0x708(r31)
.L_00000284:
    lbz      r4, 0x709(r31)
    lbz      r5, 0x708(r31)
    extsb    r4, r4
    mulli    r4, r4, 0x3
    extsb    r5, r5
    add      r4, r5, r4
    mulli    r4, r4, 0xc
    add      r4, r3, r4
    lbz      r4, 0x8(r4)
    cmplw    r0, r4
    beq      .L_00000268
.L_000002B0:
    lwz      r4, 0x19d4(r30)
    rlwinm.  r3, r4, 0, 28, 28
    beq      .L_00000310
    lis      r3, decaPanelKeys@ha
    li       r7, 0x3
    addi     r4, r3, decaPanelKeys@l
.L_000002C8:
    lbz      r3, 0x709(r31)
    subi     r5, r3, 0x1
    extsb.   r3, r5
    stb      r5, 0x709(r31)
    bge      .L_000002E0
    stb      r7, 0x709(r31)
.L_000002E0:
    lbz      r3, 0x709(r31)
    lbz      r5, 0x708(r31)
    extsb    r3, r3
    mulli    r3, r3, 0x3
    extsb    r5, r5
    add      r3, r5, r3
    mulli    r3, r3, 0xc
    add      r3, r4, r3
    lbz      r3, 0x8(r3)
    cmplw    r0, r3
    beq      .L_000002C8
    b        .L_0000036C
.L_00000310:
    rlwinm.  r3, r4, 0, 29, 29
    beq      .L_0000036C
    lis      r3, decaPanelKeys@ha
    li       r7, 0x0
    addi     r4, r3, decaPanelKeys@l
.L_00000324:
    lbz      r3, 0x709(r31)
    addi     r5, r3, 0x1
    extsb    r3, r5
    stb      r5, 0x709(r31)
    cmpwi    r3, 0x3
    ble      .L_00000340
    stb      r7, 0x709(r31)
.L_00000340:
    lbz      r3, 0x709(r31)
    lbz      r5, 0x708(r31)
    extsb    r3, r3
    mulli    r3, r3, 0x3
    extsb    r5, r5
    add      r3, r5, r3
    mulli    r3, r3, 0xc
    add      r3, r4, r3
    lbz      r3, 0x8(r3)
    cmplw    r0, r3
    beq      .L_00000324
.L_0000036C:
    lwz      r0, 0x19c8(r30)
    rlwinm.  r0, r0, 0, 23, 23
    beq      .L_00000544
    lbz      r0, 0x709(r31)
    lis      r3, gGame@ha
    lbz      r5, 0x708(r31)
    lis      r4, decaPanelKeySounds@ha
    extsb    r0, r0
    addi     r3, r3, gGame@l
    mulli    r0, r0, 0x3
    extsb    r5, r5
    addis    r3, r3, 0xe
    addi     r4, r4, decaPanelKeySounds@l
    add      r0, r5, r0
    li       r5, 0xff
    mulli    r7, r0, 0xc
    addi     r3, r3, 0x7e24
    slwi     r0, r0, 2
    lwzx     r4, r4, r0
    lbzx     r24, r6, r7
    bl       PlayGameSound
    cmpwi    r24, 0x43
    bne      .L_000003F4
    li       r3, 0x20
    li       r0, 0x0
    stb      r3, 0x6fc(r31)
    stb      r3, 0x6fd(r31)
    stb      r3, 0x6fe(r31)
    stb      r3, 0x6ff(r31)
    stb      r3, 0x700(r31)
    stb      r3, 0x701(r31)
    stb      r0, 0x702(r31)
    stw      r0, 0x704(r31)
    b        .L_00000544
.L_000003F4:
    lbz      r0, 0x702(r31)
    cmplwi   r0, 0x6
    bge      .L_00000440
    cmplwi   r0, 0x0
    beq      .L_00000430
    lbz      r0, 0x6fd(r31)
    stb      r0, 0x6fc(r31)
    lbz      r0, 0x6fe(r31)
    stb      r0, 0x6fd(r31)
    lbz      r0, 0x6ff(r31)
    stb      r0, 0x6fe(r31)
    lbz      r0, 0x700(r31)
    stb      r0, 0x6ff(r31)
    lbz      r0, 0x701(r31)
    stb      r0, 0x700(r31)
.L_00000430:
    stb      r24, 0x701(r31)
    lbz      r3, 0x702(r31)
    addi     r0, r3, 0x1
    stb      r0, 0x702(r31)
.L_00000440:
    lbz      r0, 0x702(r31)
    cmplwi   r0, 0x6
    blt      .L_00000544
    li       r0, 0x0
    stw      r0, 0x704(r31)
    lbz      r3, 0x6fc(r31)
    cmpwi    r3, 0x20
    beq      .L_00000474
    lwz      r0, 0x704(r31)
    mulli    r0, r0, 0xa
    add      r3, r3, r0
    subi     r0, r3, 0x30
    stw      r0, 0x704(r31)
.L_00000474:
    lbz      r3, 0x6fd(r31)
    cmpwi    r3, 0x20
    beq      .L_00000494
    lwz      r0, 0x704(r31)
    mulli    r0, r0, 0xa
    add      r3, r3, r0
    subi     r0, r3, 0x30
    stw      r0, 0x704(r31)
.L_00000494:
    lbz      r3, 0x6fe(r31)
    cmpwi    r3, 0x20
    beq      .L_000004B4
    lwz      r0, 0x704(r31)
    mulli    r0, r0, 0xa
    add      r3, r3, r0
    subi     r0, r3, 0x30
    stw      r0, 0x704(r31)
.L_000004B4:
    lbz      r3, 0x6ff(r31)
    cmpwi    r3, 0x20
    beq      .L_000004D4
    lwz      r0, 0x704(r31)
    mulli    r0, r0, 0xa
    add      r3, r3, r0
    subi     r0, r3, 0x30
    stw      r0, 0x704(r31)
.L_000004D4:
    lbz      r3, 0x700(r31)
    cmpwi    r3, 0x20
    beq      .L_000004F4
    lwz      r0, 0x704(r31)
    mulli    r0, r0, 0xa
    add      r3, r3, r0
    subi     r0, r3, 0x30
    stw      r0, 0x704(r31)
.L_000004F4:
    lbz      r3, 0x701(r31)
    cmpwi    r3, 0x20
    beq      .L_00000514
    lwz      r0, 0x704(r31)
    mulli    r0, r0, 0xa
    add      r3, r3, r0
    subi     r0, r3, 0x30
    stw      r0, 0x704(r31)
.L_00000514:
    li       r0, 0x0
    lis      r3, gGame@ha
    stw      r0, 0x1f4(r31)
    addi     r6, r3, gGame@l
    li       r3, -0x3
    li       r0, -0x1
    lwz      r4, 0x1b10(r6)
    lwz      r5, 0x1b14(r6)
    and      r0, r4, r0
    and      r3, r5, r3
    stw      r0, 0x1b10(r6)
    stw      r3, 0x1b14(r6)
.L_00000544:
    lwz      r0, 0x19c8(r30)
    rlwinm.  r0, r0, 0, 22, 22
    beq      .L_00000584
    li       r6, -0x1
    lis      r3, gGame@ha
    stw      r6, 0x704(r31)
    li       r0, 0x0
    addi     r5, r3, gGame@l
    li       r3, -0x3
    stw      r0, 0x1f4(r31)
    lwz      r0, 0x1b10(r5)
    lwz      r4, 0x1b14(r5)
    and      r0, r0, r6
    and      r3, r4, r3
    stw      r0, 0x1b10(r5)
    stw      r3, 0x1b14(r5)
.L_00000584:
    lis      r3, sDecaZero@ha
    lfs      f1, 0x1f8(r31)
    lfs      f0, sDecaZero@l(r3)
    fcmpo    cr0, f1, f0
    ble      .L_00000C90
    lis      r3, gGame@ha
    li       r4, 0x1
    addi     r3, r3, gGame@l
    li       r5, 0x4
    lwz      r3, 0x290(r3)
    li       r6, 0x5
    li       r7, 0xf
    bl       SetGsBlendMode
    lis      r3, gGame@ha
    li       r4, 0x0
    addi     r30, r3, gGame@l
    lwz      r3, 0x294(r30)
    bl       SetGsTevMode
    lbz      r0, 0x709(r31)
    lis      r3, sDecaZero@ha
    lbz      r6, 0x708(r31)
    lis      r4, decaPanelKeys@ha
    extsb    r0, r0
    lfs      f2, sDecaZero@l(r3)
    mulli    r5, r0, 0x3
    extsb    r6, r6
    addi     r0, r4, decaPanelKeys@l
    addi     r26, r31, 0x1fc
    add      r3, r6, r5
    mulli    r4, r3, 0xc
    mr       r3, r26
    add      r27, r0, r4
    lfs      f0, 0x0(r27)
    stfs     f0, 0x714(r31)
    lfs      f0, 0x4(r27)
    stfs     f0, 0x718(r31)
    lfs      f0, 0x714(r31)
    lfs      f1, 0x70c(r31)
    fsubs    f0, f0, f1
    fmadds   f0, f2, f0, f1
    stfs     f0, 0x70c(r31)
    lfs      f0, 0x718(r31)
    lfs      f1, 0x710(r31)
    fsubs    f0, f0, f1
    fmadds   f0, f2, f0, f1
    stfs     f0, 0x710(r31)
    bl       GXGetTexObjWidth
    mr       r28, r3
    mr       r3, r26
    bl       GXGetTexObjHeight
    lis      r4, lbl_6_rodata_130@ha
    lfs      f0, 0x1f8(r31)
    lfs      f1, lbl_6_rodata_130@l(r4)
    mr       r25, r3
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    lis      r4, 0x4330
    clrlwi   r6, r28, 16
    stw      r6, 0x12c(r1)
    clrlwi   r0, r25, 16
    lis      r5, lbl_6_rodata_148@ha
    lis      r6, sDecaZero@ha
    stw      r4, 0x128(r1)
    lis      r8, lbl_6_rodata_134@ha
    lfd      f2, lbl_6_rodata_148@l(r5)
    lis      r5, lbl_6_rodata_13C@ha
    lfd      f0, 0x128(r1)
    lis      r7, lbl_6_rodata_138@ha
    stw      r4, 0x130(r1)
    mr       r4, r26
    lfs      f3, sDecaZero@l(r6)
    addi     r6, r5, lbl_6_rodata_13C@l
    stw      r0, 0x134(r1)
    fsubs    f4, f0, f2
    lfs      f5, 0x0(r6)
    li       r5, -0x100
    lfd      f0, 0x130(r1)
    fmr      f6, f3
    or       r5, r3, r5
    fsubs    f0, f0, f2
    lis      r3, gGame@ha
    fmuls    f4, f5, f4
    lfs      f1, lbl_6_rodata_134@l(r8)
    lfs      f2, lbl_6_rodata_138@l(r7)
    fmr      f7, f3
    fmr      f8, f3
    addi     r3, r3, gGame@l
    fmuls    f5, f5, f0
    mr       r6, r5
    mr       r7, r5
    mr       r8, r5
    bl       DrawGsTexturedQuad
    addi     r3, r31, 0x2fc
    bl       GXGetTexObjWidth
    lis      r4, 0x6666
    lis      r0, 0x4330
    clrlwi   r3, r3, 16
    lis      r5, lbl_6_rodata_13C@ha
    addi     r4, r4, 0x6667
    stw      r0, 0x138(r1)
    mulhw    r4, r4, r3
    lis      r3, sDecaSignedConversionBias@ha
    lfd      f1, sDecaSignedConversionBias@l(r3)
    lfs      f2, lbl_6_rodata_13C@l(r5)
    srawi    r0, r4, 2
    srwi     r3, r0, 31
    add      r0, r0, r3
    xoris    r0, r0, 0x8000
    addi     r3, r31, 0x2fc
    stw      r0, 0x13c(r1)
    lfd      f0, 0x138(r1)
    fsubs    f0, f0, f1
    fmuls    f30, f2, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x144(r1)
    lis      r3, lbl_6_rodata_148@ha
    lfd      f1, lbl_6_rodata_148@l(r3)
    lis      r4, lbl_6_rodata_13C@ha
    stw      r0, 0x140(r1)
    addi     r3, r31, 0x3fc
    lfs      f2, lbl_6_rodata_13C@l(r4)
    lfd      f0, 0x140(r1)
    fsubs    f0, f0, f1
    fmuls    f31, f2, f0
    bl       GXGetTexObjWidth
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x14c(r1)
    lis      r3, lbl_6_rodata_148@ha
    lfd      f1, lbl_6_rodata_148@l(r3)
    lis      r4, lbl_6_rodata_13C@ha
    stw      r0, 0x148(r1)
    addi     r3, r31, 0x3fc
    lfs      f2, lbl_6_rodata_13C@l(r4)
    lfd      f0, 0x148(r1)
    fsubs    f0, f0, f1
    fmuls    f29, f2, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x154(r1)
    lis      r3, lbl_6_rodata_148@ha
    lfd      f1, lbl_6_rodata_148@l(r3)
    lis      r4, lbl_6_rodata_13C@ha
    stw      r0, 0x150(r1)
    lis      r3, sDecaKeySpriteLayout@ha
    lfs      f2, lbl_6_rodata_13C@l(r4)
    addi     r3, r3, sDecaKeySpriteLayout@l
    lfd      f0, 0x150(r1)
    li       r0, 0x24
    addi     r5, r1, 0x4
    subi     r4, r3, 0x4
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    mtctr    r0
.L_000007F8:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_000007F8
    lis      r3, decaPanelKeys@ha
    stfs     f30, 0x18(r1)
    addi     r29, r3, decaPanelKeys@l
    addi     r28, r1, 0x8
    stfs     f31, 0x1c(r1)
    addi     r27, r27, 0x8
    li       r25, 0x0
    stfs     f30, 0x30(r1)
    stfs     f31, 0x34(r1)
    stfs     f30, 0x48(r1)
    stfs     f31, 0x4c(r1)
    stfs     f30, 0x60(r1)
    stfs     f31, 0x64(r1)
    stfs     f30, 0x78(r1)
    stfs     f31, 0x7c(r1)
    stfs     f30, 0x90(r1)
    stfs     f31, 0x94(r1)
    stfs     f30, 0xa8(r1)
    stfs     f31, 0xac(r1)
    stfs     f30, 0xc0(r1)
    stfs     f31, 0xc4(r1)
    stfs     f30, 0xd8(r1)
    stfs     f31, 0xdc(r1)
    stfs     f29, 0xf0(r1)
    stfs     f0, 0xf4(r1)
    stfs     f29, 0x108(r1)
    stfs     f0, 0x10c(r1)
    stfs     f30, 0x120(r1)
    stfs     f31, 0x124(r1)
.L_00000880:
    lbz      r0, 0x70a(r31)
    extsb.   r0, r0
    beq      .L_0000089C
    lbz      r3, 0x0(r27)
    lbz      r0, 0x8(r29)
    cmplw    r3, r0
    beq      .L_0000098C
.L_0000089C:
    lis      r3, lbl_6_rodata_120@ha
    lwz      r0, 0x0(r28)
    lfs      f2, lbl_6_rodata_120@l(r3)
    slwi     r3, r0, 8
    lfs      f1, 0x4(r28)
    addi     r26, r3, 0x1fc
    fmr      f3, f2
    add      r26, r31, r26
    addi     r3, r26, 0x6c
    bl       PSMTXScale
    lis      r3, sDecaZero@ha
    li       r0, 0x1
    lfs      f2, sDecaZero@l(r3)
    addi     r3, r26, 0x3c
    stw      r0, 0x38(r26)
    fmr      f3, f2
    lfs      f1, 0x8(r28)
    bl       PSMTXTrans
    li       r0, 0x1
    lis      r3, lbl_6_rodata_130@ha
    stw      r0, 0x38(r26)
    lis      r4, decaPanelKeys@ha
    addi     r0, r4, decaPanelKeys@l
    lis      r5, lbl_6_rodata_13C@ha
    lwz      r4, 0xc(r28)
    lfs      f1, lbl_6_rodata_130@l(r3)
    mulli    r4, r4, 0xc
    lfs      f0, 0x1f8(r31)
    lfs      f2, lbl_6_rodata_13C@l(r5)
    fmuls    f1, f1, f0
    add      r24, r0, r4
    lfs      f0, 0x0(r24)
    fmuls    f30, f2, f0
    bl       __cvt_fp2unsigned
    lis      r4, lbl_6_rodata_13C@ha
    lis      r5, lbl_6_rodata_138@ha
    lfs      f1, lbl_6_rodata_13C@l(r4)
    lis      r4, sDecaZero@ha
    lfs      f0, 0x4(r24)
    lis      r6, lbl_6_rodata_134@ha
    lfs      f3, sDecaZero@l(r4)
    li       r0, -0x100
    fmuls    f0, f1, f0
    lfs      f2, lbl_6_rodata_138@l(r5)
    lfs      f1, lbl_6_rodata_134@l(r6)
    or       r5, r3, r0
    fmr      f6, f3
    lis      r4, gGame@ha
    addi     r3, r4, gGame@l
    fmr      f7, f3
    fmr      f8, f3
    lfs      f4, 0x10(r28)
    fadds    f1, f1, f30
    lfs      f5, 0x14(r28)
    fadds    f2, f2, f0
    mr       r4, r26
    mr       r6, r5
    mr       r7, r5
    mr       r8, r5
    bl       DrawGsTexturedQuad
.L_0000098C:
    addi     r25, r25, 0x1
    addi     r28, r28, 0x18
    cmpwi    r25, 0xc
    addi     r29, r29, 0xc
    blt      .L_00000880
    lis      r3, lbl_6_rodata_120@ha
    lis      r4, lbl_6_rodata_140@ha
    lfs      f2, lbl_6_rodata_120@l(r3)
    addi     r28, r31, 0x5fc
    lfs      f1, lbl_6_rodata_140@l(r4)
    addi     r3, r28, 0x6c
    fmr      f3, f2
    bl       PSMTXScale
    li       r0, 0x1
    mr       r3, r28
    stw      r0, 0x38(r28)
    bl       GXGetTexObjWidth
    lis      r4, 0x6666
    lis      r0, 0x4330
    clrlwi   r3, r3, 16
    lis      r5, lbl_6_rodata_13C@ha
    addi     r4, r4, 0x6667
    stw      r0, 0x150(r1)
    mulhw    r4, r4, r3
    lis      r3, sDecaSignedConversionBias@ha
    lfd      f1, sDecaSignedConversionBias@l(r3)
    lfs      f2, lbl_6_rodata_13C@l(r5)
    srawi    r0, r4, 2
    srwi     r3, r0, 31
    add      r0, r0, r3
    xoris    r0, r0, 0x8000
    mr       r3, r28
    stw      r0, 0x154(r1)
    lfd      f0, 0x150(r1)
    fsubs    f0, f0, f1
    fmuls    f30, f2, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x14c(r1)
    lis      r3, lbl_6_rodata_148@ha
    lfd      f1, lbl_6_rodata_148@l(r3)
    lis      r4, lbl_6_rodata_13C@ha
    stw      r0, 0x148(r1)
    lis      r3, decaPanelDigitPos@ha
    lfs      f2, lbl_6_rodata_13C@l(r4)
    addi     r27, r3, decaPanelDigitPos@l
    lfd      f0, 0x148(r1)
    li       r25, 0x0
    fsubs    f0, f0, f1
    fmuls    f31, f2, f0
.L_00000A58:
    addi     r0, r25, 0x6fc
    lbzx     r3, r31, r0
    cmpwi    r3, 0x20
    beq      .L_00000B58
    lis      r4, decaPanelDigitCells@ha
    slwi     r5, r3, 2
    addi     r4, r4, decaPanelDigitCells@l
    lis      r0, 0x4330
    add      r4, r4, r5
    lis      r3, sDecaSignedConversionBias@ha
    lwz      r4, -0xc0(r4)
    addi     r5, r3, sDecaSignedConversionBias@l
    stw      r0, 0x150(r1)
    lis      r3, sDecaZero@ha
    xoris    r0, r4, 0x8000
    lfd      f1, 0x0(r5)
    stw      r0, 0x154(r1)
    addi     r4, r3, sDecaZero@l
    lfs      f2, 0x0(r4)
    lis      r6, lbl_6_rodata_140@ha
    lfd      f0, 0x150(r1)
    addi     r3, r28, 0x3c
    fmr      f3, f2
    fsubs    f0, f0, f1
    lfs      f1, lbl_6_rodata_140@l(r6)
    fmuls    f1, f1, f0
    bl       PSMTXTrans
    li       r0, 0x1
    lis      r3, lbl_6_rodata_130@ha
    stw      r0, 0x38(r28)
    lfs      f1, lbl_6_rodata_130@l(r3)
    lfs      f0, 0x1f8(r31)
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    lis      r4, lbl_6_rodata_13C@ha
    lis      r6, lbl_6_rodata_134@ha
    lfs      f2, lbl_6_rodata_13C@l(r4)
    lis      r4, sDecaZero@ha
    lfs      f1, 0x0(r27)
    lis      r5, lbl_6_rodata_138@ha
    addi     r7, r6, lbl_6_rodata_134@l
    lfs      f0, 0x4(r27)
    lfs      f3, sDecaZero@l(r4)
    addi     r6, r5, lbl_6_rodata_138@l
    fmuls    f1, f2, f1
    lfs      f9, 0x0(r7)
    fmuls    f0, f2, f0
    lfs      f2, 0x0(r6)
    li       r0, -0x100
    fmr      f4, f30
    or       r5, r3, r0
    lis      r3, gGame@ha
    fmr      f5, f31
    addi     r3, r3, gGame@l
    fmr      f6, f3
    mr       r4, r28
    fmr      f7, f3
    mr       r6, r5
    fmr      f8, f3
    mr       r7, r5
    fadds    f1, f9, f1
    mr       r8, r5
    fadds    f2, f2, f0
    bl       DrawGsTexturedQuad
.L_00000B58:
    addi     r25, r25, 0x1
    addi     r27, r27, 0x8
    cmpwi    r25, 0x6
    blt      .L_00000A58
    addi     r27, r31, 0x4fc
    mr       r3, r27
    bl       GXGetTexObjWidth
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x154(r1)
    lis      r3, lbl_6_rodata_148@ha
    lfd      f1, lbl_6_rodata_148@l(r3)
    lis      r4, lbl_6_rodata_13C@ha
    stw      r0, 0x150(r1)
    mr       r3, r27
    lfs      f2, lbl_6_rodata_13C@l(r4)
    lfd      f0, 0x150(r1)
    fsubs    f0, f0, f1
    fmuls    f31, f2, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    lis      r3, lbl_6_rodata_148@ha
    stw      r4, 0x14c(r1)
    addi     r4, r3, lbl_6_rodata_148@l
    lis      r5, lbl_6_rodata_13C@ha
    stw      r0, 0x148(r1)
    lis      r3, lbl_6_rodata_130@ha
    lfd      f1, 0x0(r4)
    lfd      f0, 0x148(r1)
    lfs      f3, lbl_6_rodata_13C@l(r5)
    fsubs    f2, f0, f1
    lfs      f1, lbl_6_rodata_130@l(r3)
    lfs      f0, 0x1f8(r31)
    fmuls    f30, f3, f2
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    lis      r5, lbl_6_rodata_144@ha
    lis      r4, lbl_6_rodata_13C@ha
    addi     r6, r5, lbl_6_rodata_144@l
    lfs      f1, 0x70c(r31)
    lfs      f0, 0x0(r6)
    addi     r7, r4, lbl_6_rodata_13C@l
    lfs      f4, 0x0(r7)
    lis      r5, lbl_6_rodata_134@ha
    fdivs    f10, f31, f0
    lis      r4, lbl_6_rodata_138@ha
    addi     r8, r5, lbl_6_rodata_134@l
    li       r0, -0x100
    or       r5, r3, r0
    addi     r6, r4, lbl_6_rodata_138@l
    fdivs    f2, f30, f0
    lfs      f0, 0x710(r31)
    lfs      f6, 0x0(r8)
    lis      r4, sDecaZero@ha
    lfs      f3, sDecaZero@l(r4)
    lis      r3, gGame@ha
    fmuls    f5, f4, f1
    lfs      f1, 0x0(r6)
    fmuls    f0, f4, f0
    addi     r3, r3, gGame@l
    fmr      f4, f31
    mr       r4, r27
    fadds    f9, f6, f5
    mr       r6, r5
    fadds    f0, f1, f0
    mr       r7, r5
    fmr      f5, f30
    mr       r8, r5
    fmr      f6, f3
    fmr      f7, f3
    fmr      f8, f3
    fadds    f1, f10, f9
    fadds    f2, f2, f0
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r30)
    li       r4, 0x4
    bl       SetGsTevMode
.L_00000C90:
    psq_l    f31, 0x1a8(r1), 0, qr0
    lfd      f31, 0x1a0(r1)
    psq_l    f30, 0x198(r1), 0, qr0
    lfd      f30, 0x190(r1)
    psq_l    f29, 0x188(r1), 0, qr0
    lfd      f29, 0x180(r1)
    lmw      r24, 0x160(r1)
    lwz      r0, 0x1b4(r1)
    mtlr     r0
    addi     r1, r1, 0x1b0
    blr
*/

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
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stmw     r27, 0xc(r1)
    lwz      r0, 0x0(r5)
    cmpwi    r0, 0x0
    beq      .L_00000D68
    lis      r3, savedObjFlags@ha
    addi     r30, r3, savedObjFlags@l
    lwz      r0, 0x960(r30)
    cmpwi    r0, 0x0
    ble      .L_00000DFC
    lis      r3, gGame@ha
    mr       r29, r30
    addi     r3, r3, gGame@l
    li       r31, 0x0
    addis    r28, r3, 0x6
    subi     r28, r28, 0x2f80
    b        .L_00000D58
.L_00000D08:
    lwz      r3, 0x0(r29)
    cmpwi    r3, -0x1
    beq      .L_00000DFC
    lwz      r0, 0x0(r28)
    extsh    r4, r3
    cmplwi   r0, 0x0
    beq      .L_00000D30
    mr       r3, r28
    bl       ResolveEventObjectId
    b        .L_00000D34
.L_00000D30:
    li       r3, 0x0
.L_00000D34:
    lbz      r0, 0x4(r29)
    clrlwi.  r0, r0, 31
    beq      .L_00000D50
    lwz      r5, 0x22c(r3)
    lis      r0, 0x1
    or       r0, r5, r0
    stw      r0, 0x22c(r3)
.L_00000D50:
    addi     r29, r29, 0x8
    addi     r31, r31, 0x1
.L_00000D58:
    lwz      r0, 0x960(r30)
    cmpw     r31, r0
    blt      .L_00000D08
    b        .L_00000DFC
.L_00000D68:
    lis      r3, savedObjFlags@ha
    addi     r30, r3, savedObjFlags@l
    lwz      r0, 0x960(r30)
    cmpwi    r0, 0x0
    ble      .L_00000DFC
    lis      r4, gGame@ha
    lis      r3, 0xffff
    addi     r4, r4, gGame@l
    mr       r28, r30
    addis    r27, r4, 0x6
    subi     r31, r3, 0x1
    li       r29, 0x0
    subi     r27, r27, 0x2f80
    b        .L_00000DF0
.L_00000DA0:
    lwz      r3, 0x0(r28)
    cmpwi    r3, -0x1
    beq      .L_00000DFC
    lwz      r0, 0x0(r27)
    extsh    r4, r3
    cmplwi   r0, 0x0
    beq      .L_00000DC8
    mr       r3, r27
    bl       ResolveEventObjectId
    b        .L_00000DCC
.L_00000DC8:
    li       r3, 0x0
.L_00000DCC:
    lwz      r0, 0x22c(r3)
    li       r4, -0x1
    lwz      r5, 0x228(r3)
    addi     r28, r28, 0x8
    and      r0, r0, r31
    addi     r29, r29, 0x1
    stw      r0, 0x22c(r3)
    and      r0, r5, r4
    stw      r0, 0x228(r3)
.L_00000DF0:
    lwz      r0, 0x960(r30)
    cmpw     r29, r0
    blt      .L_00000DA0
.L_00000DFC:
    lmw      r27, 0xc(r1)
    li       r3, 0x0
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

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
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
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
	static_cast<CCameraActionScroll2*>(gGame.mCameraActions.mTable[CAMACTION_SCROLL2])->ClearHideList();
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
		static_cast<CCameraActionScroll2*>(gGame.mCameraActions.mTable[CAMACTION_SCROLL2])->AddHideObj(args[i]);
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
extern "C" void ExitStage(void* arg)
{
	decaPanel.Close();
}

/**
 * @note Address: stage03 .text 0x1098
 * @note Size: 0x28
 */
extern "C" void DrawStageHud(void* arg)
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
		AXDriverSetAllStreamPitches(5.0f * (f32)sin(DEG2RAD * sTimer));
	}
	if (IsGameSoundPlaying(&gGame.mSound, 0xFFFF000F)) {
		AXDriverSetAllStreamPitches(5.0f * (f32)sin(DEG2RAD * sTimer));
	}
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
// NONMATCHING: regalloc, the key-index offset and the ' '/1
// constants land in different scratch regs; sequence is identical.
extern "C" void InitStage(void* arg)
{
	decaPanel.mActive = 0;
	decaPanel.mState  = 1;
	decaPanel.mFade   = 0.0f;
	decaPanel.ResetInput();
}

// InitStage
/*
    li       r4, 0x3
    lis      r3, decaPanelKeys@ha
    mulli    r7, r4, 0x24
    lis      r5, sDecaZero@ha
    addi     r3, r3, decaPanelKeys@l
    lfs      f2, sDecaZero@l(r5)
    lis      r6, decaPanel@ha
    lfsx     f1, r3, r7
    addi     r6, r6, decaPanel@l
    add      r3, r3, r7
    li       r0, 0x20
    lfs      f0, 0x4(r3)
    stb      r4, 0x709(r6)
    li       r4, 0x0
    li       r3, 0x1
    stw      r4, 0x1f4(r6)
    stw      r3, 0x0(r6)
    stfs     f2, 0x1f8(r6)
    stb      r0, 0x6fc(r6)
    stb      r0, 0x6fd(r6)
    stb      r0, 0x6fe(r6)
    stb      r0, 0x6ff(r6)
    stb      r0, 0x700(r6)
    stb      r0, 0x701(r6)
    stb      r4, 0x702(r6)
    stw      r4, 0x704(r6)
    stb      r4, 0x708(r6)
    stb      r4, 0x70a(r6)
    stfs     f1, 0x714(r6)
    stfs     f1, 0x70c(r6)
    stfs     f0, 0x718(r6)
    stfs     f0, 0x710(r6)
    blr
*/

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

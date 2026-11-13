#include "game/cgame.h"
#include "game/video.h"
#include "game/xchr.h"

struct SavedObjFlag {
	s32 mId;  // _00
	u8 mFlag; // _04
};

struct SavedObjFlagList {
	void Apply(s32 mode);

	SavedObjFlag mEntries[0x100]; // _00
	s32 mCount;                   // _800
};

extern "C" void SetJunkUnkoFlag(u8 on);
extern "C" void SetFafaPartVisibility(s32 mode);

struct OnOffWork {
	s32 _00;      // _00
	u8 _04[0x4];  // _04
	s32 _08;      // _08
	u8 _0C[0x2C]; // _0C
	CVec _38[3];  // _38
	s32 _68;      // _68
	u8 _6C[0xC];  // _6C
	s32 _78;      // _78
	s32 _7C;      // _7C
	u8 _80[0x4];  // _80
	s32 _84;      // _84
	CStatus _88;  // _88
};

static SavedObjFlagList savedObjFlags;
static OnOffWork onOffWork;

extern "C" {
s32 CmdSetJunkUnkoFlag(s32 cmd, s32 count, s32* args);
s32 CmdApplySavedObjFlags(s32 cmd, s32 count, s32* args);
s32 CmdSaveObjFlag(s32 cmd, s32 count, s32* args);
s32 CmdClearSavedObjFlags(s32 cmd, s32 count, s32* args);
s32 CmdSetOrGetTimer(s32 cmd, s32 count, s32* args);
s32 CmdSetTimerRunning(s32 cmd, s32 count, s32* args);
s32 CmdShowTimer(s32 cmd, s32 count, s32* args);
s32 CmdSetFafaPartVisibility(s32 cmd, s32 count, s32* args);
s32 CmdCheckOnOffFlag(s32 cmd, s32 count, s32* args);
s32 CmdStartOnOff2(s32 cmd, s32 count, s32* args);
s32 CmdStartOnOff1(s32 cmd, s32 count, s32* args);
s32 CmdStartOnOff0(s32 cmd, s32 count, s32* args);
s32 CmdResetOnOff(s32 cmd, s32 count, s32* args);
s32 CmdPrintArgs(s32 cmd, s32 count, s32* args);
void ShunNoOpSlot5(void* arg);
void DrawStageHud(void* arg);
void UpdateStage(void* arg);
void InitStage(void* arg);

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage, NULL, NULL, DrawStageHud, ShunNoOpSlot5, NULL };
RelCmdFunc _prolog_cmds[]    = {
    CmdPrintArgs,          CmdResetOnOff,      CmdStartOnOff0,           CmdStartOnOff1,
    CmdStartOnOff2,        CmdCheckOnOffFlag,  CmdSetFafaPartVisibility, CmdShowTimer,
    CmdSetTimerRunning,    CmdSetOrGetTimer,   CmdClearSavedObjFlags,    CmdSaveObjFlag,
    CmdApplySavedObjFlags, CmdSetJunkUnkoFlag,
};
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void SavedObjFlagList::Apply(s32 mode)
{
	for (s32 i = 0; i < mCount; ++i) {
		const SavedObjFlag& entry = mEntries[i];
		if (entry.mId == -1) {
			break;
		}
		XObj* obj = FindObj(entry.mId);
		if (mode == 0) {
			obj->mStatus.mFlags &= ~0x10000ULL;
		} else if (entry.mFlag & 1) {
			obj->mStatus.mFlags |= 0x10000;
		}
	}
}

/**
 * @note Address: shun .text 0x0
 * @note Size: 0x40
 */
extern "C" s32 CmdSetJunkUnkoFlag(s32 cmd, s32 count, s32* args)
{
	SetJunkUnkoFlag(args[0] != 0);
	return 0;
}

/**
 * @note Address: shun .text 0x40
 * @note Size: 0x158
 */
extern "C" s32 CmdApplySavedObjFlags(s32 cmd, s32 count, s32* args)
{
	savedObjFlags.Apply(args[0]);
	return 0;
}

/**
 * @note Address: shun .text 0x198
 * @note Size: 0xF0
 */
extern "C" s32 CmdSaveObjFlag(s32 cmd, s32 count, s32* args)
{
	if (savedObjFlags.mCount < (s32)ARRAY_SIZE(savedObjFlags.mEntries)) {
		SavedObjFlag& entry = savedObjFlags.mEntries[savedObjFlags.mCount];
		entry.mId           = args[0];
		XObj* obj           = FindObj(entry.mId);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mStatus.mFlags & 0x10000) {
				entry.mFlag |= 1;
			}
			++savedObjFlags.mCount;
		}
	}
	return 0;
}

/**
 * @note Address: shun .text 0x288
 * @note Size: 0xB0
 */
extern "C" s32 CmdClearSavedObjFlags(s32 cmd, s32 count, s32* args)
{
	s32 i;

	savedObjFlags.mCount = 0;
	for (i = 0; i < ARRAY_SIZE(savedObjFlags.mEntries); i++) {
		savedObjFlags.mEntries[i].mId   = -1;
		savedObjFlags.mEntries[i].mFlag = 0;
	}
	return 0;
}

/**
 * @note Address: shun .text 0x408
 * @note Size: 0x6C
 */
extern "C" s32 CmdSetOrGetTimer(s32 cmd, s32 count, s32* args)
{
	s32 ret = 0;

	if (args[0] != 0) {
		InfoScrn_SetTimerHundredths(&gGame.mInfoScrn, args[1]);
	} else {
		ret = InfoScrn_GetTimerHundredths(&gGame.mInfoScrn);
	}
	return ret;
}

/**
 * @note Address: shun .text 0x474
 * @note Size: 0x58
 */
extern "C" s32 CmdSetTimerRunning(s32 cmd, s32 count, s32* args)
{
	if (args[0] != 0) {
		InfoScrn_ShowTimer(&gGame.mInfoScrn);
	} else {
		InfoScrn_HideTimer(&gGame.mInfoScrn);
	}
	return 0;
}

/**
 * @note Address: shun .text 0x4CC
 * @note Size: 0x68
 */
extern "C" s32 CmdShowTimer(s32 cmd, s32 count, s32* args)
{
	InfoScrn_ApplyTimerHudCommand(&gGame.mInfoScrn, args[0] != 0, 1);
	return 0;
}

/**
 * @note Address: shun .text 0x534
 * @note Size: 0x28
 */
extern "C" s32 CmdSetFafaPartVisibility(s32 cmd, s32 count, s32* args)
{
	SetFafaPartVisibility(args[0]);
	return 0;
}

/**
 * @note Address: shun .text 0x55C
 * @note Size: 0x24
 */
extern "C" s32 CmdCheckOnOffFlag(s32 cmd, s32 count, s32* args)
{
	return onOffWork._88.Check(1);
}

static inline void StartOnOff(s32 mode)
{
	XObjChr* player     = gGame.GetPlayer();
	player->_980.mFlags = 0x40000000;
	player->mPad.mReset = FALSE;
	player->SetAnim(0x3E, FALSE, NULL, NULL, NULL, 0.0f, 2.0f, 0.0f, 10.0f);
	onOffWork._68 = 0;
	onOffWork._84 = mode;
}

/**
 * @note Address: shun .text 0x580
 * @note Size: 0xA8
 */
extern "C" s32 CmdStartOnOff2(s32 cmd, s32 count, s32* args)
{
	StartOnOff(2);
	return 0;
}

/**
 * @note Address: shun .text 0x628
 * @note Size: 0xA8
 */
extern "C" s32 CmdStartOnOff1(s32 cmd, s32 count, s32* args)
{
	StartOnOff(1);
	return 0;
}

/**
 * @note Address: shun .text 0x6D0
 * @note Size: 0xA4
 */
extern "C" s32 CmdStartOnOff0(s32 cmd, s32 count, s32* args)
{
	StartOnOff(0);
	return 0;
}

/**
 * @note Address: shun .text 0x774
 * @note Size: 0x2C
 */
extern "C" s32 CmdResetOnOff(s32 cmd, s32 count, s32* args)
{
	onOffWork._00        = (FRAMES_PER_SECOND * 60) / 2; // half a minute
	onOffWork._08        = 0;
	onOffWork._78        = 0;
	onOffWork._7C        = 0;
	onOffWork._88.mFlags = 0;
	return 0;
}

/**
 * @note Address: shun .text 0x7A0
 * @note Size: 0x50
 */
extern "C" s32 CmdPrintArgs(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: shun .text 0x7F0
 * @note Size: 0x4
 */
extern "C" void ShunNoOpSlot5(void* arg)
{
}

/**
 * @note Address: shun .text 0x7F4
 * @note Size: 0x4
 */
extern "C" void DrawStageHud(void* arg)
{
}

/**
 * @note Address: shun .text 0x7F8
 * @note Size: 0x4
 */
extern "C" void UpdateStage(void* arg)
{
}

/**
 * @note Address: shun .text 0x7FC
 * @note Size: 0x4
 */
extern "C" void InitStage(void* arg)
{
}

/**
 * @note Address: shun .text 0x1C48
 * @note Size: 0x74
 */
// the static initializer for onOffWork

/**
 * @note Address: shun .text 0x1CBC
 * @note Size: 0x78
 */
// OnOffWork's implicit destructor

/**
 * @note Address: shun .text 0x1D34
 * @note Size: 0x48
 */
// CStatus::~CStatus, emitted from the class

/**
 * @note Address: shun .text 0x1D7C
 * @note Size: 0x10
 */
// CVec::CVec, emitted from the class

#include "game/relmodule.h"
#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/cjobjset.h"
#include "game/video.h"
#include "game/xchr.h"
#include "Version.h"
extern "C" {
#include <sysdolphin/aobj.h>
#include <sysdolphin/mtx.h>
}

struct SavedObjFlag {
	s32 mId;  // _00
	u8 mFlag; // _04, bit 0: the object's drawn flag (0x10000) was set
};

struct SavedObjFlagList {
	void Apply(s32 mode);

	SavedObjFlag mEntries[0x100]; // _00
	s32 mCount;                   // _800
};

extern "C" void SetJunkUnkoFlag(u8);
extern "C" void InfoScrn_SetTimerHundredths(InfoScrn* screen, s32 arg);
extern "C" s32 InfoScrn_GetTimerHundredths(InfoScrn* screen);
extern "C" void InfoScrn_ShowTimer(InfoScrn* screen);
extern "C" void InfoScrn_HideTimer(InfoScrn* screen);
extern "C" void InfoScrn_ApplyTimerHudCommand(InfoScrn* screen, u8 show, s32 mode);
extern "C" void SetFafaPartVisibility(s32 arg);

struct OnOffWork {
	s32 _00;         // _00, half a minute in frames, written by CmdResetOnOff and never read
	u8 _04[0x4];     // _04
	s32 _08;         // _08, zeroed by CmdResetOnOff, never read
	u8 _0C[0x2C];    // _0C
	CVec _38[3];     // _38
	s32 _68;         // _68, zeroed by each CmdStartOnOff0/1/2, never read
	u8 _6C[0xC];     // _6C
	s32 _78;         // _78, zeroed by CmdResetOnOff, never read
	s32 _7C;         // _7C, zeroed by CmdResetOnOff, never read
	u8 _80[0x4];     // _80
	s32 _84;         // _84, which start command ran: 0, 1 or 2 from CmdStartOnOff0/1/2; never read
	CStatus mStatus; // _88, CStatus; bit 1 is what CmdCheckOnOffFlag reports, cleared by CmdResetOnOff
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
void ShunNoOpSlot5(s32 arg);
void DrawStageHud(s32 arg);
void UpdateStage(s32 arg);
void InitStage(s32 arg);

RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage, NULL, NULL, DrawStageHud, ShunNoOpSlot5, NULL };
RelCmdFunc _prolog_cmds[]    = {
    CmdPrintArgs,          CmdResetOnOff,      CmdStartOnOff0,           CmdStartOnOff1,
    CmdStartOnOff2,        CmdCheckOnOffFlag,  CmdSetFafaPartVisibility, CmdShowTimer,
    CmdSetTimerRunning,    CmdSetOrGetTimer,   CmdClearSavedObjFlags,    CmdSaveObjFlag,
    CmdApplySavedObjFlags, CmdSetJunkUnkoFlag,
};
}

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void SavedObjFlagList::Apply(s32 mode)
{
	s32 i;
	XObj* obj;

	// mode 0 clears the drawn bit, anything else restores the bits CmdSaveObjFlag saw
	if (mode != 0) {
		if (mCount > 0) {
			s32 j;
			for (j = 0; j < mCount; j++) {
				if (mEntries[j].mId == -1) { // -1 ends the list
					break;
				}
				obj = FindObj(mEntries[j].mId);
				if (mEntries[j].mFlag & 1) { // its drawn bit was saved
					obj->mStatus.mFlags |= 0x10000;
				}
			}
		}
	} else {
		if (mCount > 0) {
			for (i = 0; i < mCount; i++) {
				if (mEntries[i].mId == -1) { // -1 ends the list
					break;
				}
				obj = FindObj(mEntries[i].mId);
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
	}
}

/**
 * @note Address: shun .text 0x0
 * @note Size: 0x40
 */
extern "C" s32 CmdSetJunkUnkoFlag(s32 cmd, s32 count, s32* args)
{
	if (args[0] != 0) {
		SetJunkUnkoFlag(1);
	} else {
		SetJunkUnkoFlag(0);
	}
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
// CmdApplySavedObjFlags

/**
 * @note Address: shun .text 0x198
 * @note Size: 0xF0
 */
extern "C" s32 CmdSaveObjFlag(s32 cmd, s32 count, s32* args)
{
	s32 id = args[0];
	XObj* obj;

	if (savedObjFlags.mCount < 0x100) {
		savedObjFlags.mEntries[savedObjFlags.mCount].mId = id;
		obj                                              = FindObj(savedObjFlags.mEntries[savedObjFlags.mCount].mId);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) { // objects without flag 1 are skipped
			if (obj->mStatus.mFlags & 0x10000) {        // drawn now: remember it
				savedObjFlags.mEntries[savedObjFlags.mCount].mFlag |= 1;
			}
			savedObjFlags.mCount++;
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
 * @note Address: shun .text 0x3C0
 * @note Size: 0x48
 */
// CVec::~CVec, emitted from the class

/**
 * @note Address: shun .text 0x408
 * @note Size: 0x6C
 */
extern "C" s32 CmdSetOrGetTimer(s32 cmd, s32 count, s32* args)
{
	s32 ret = 0;

	// args[0] != 0 writes args[1] hundredths, 0 reads them
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
	// starts or stops the timer counting (InfoScrn::mTimerShown gates mTimer += frames)
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
	// args[0] becomes the hud command's first arg
	if (args[0] != 0) {
		InfoScrn_ApplyTimerHudCommand(&gGame.mInfoScrn, 1, 1);
	} else {
		InfoScrn_ApplyTimerHudCommand(&gGame.mInfoScrn, 0, 1);
	}
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
	if (onOffWork.mStatus.Check(1)) {
		return 1;
	}
	return 0;
}

/**
 * @note Address: shun .text 0x580
 * @note Size: 0xA8
 */
extern "C" s32 CmdStartOnOff2(s32 cmd, s32 count, s32* args)
{
	XObjChr* player = gGame.GetPlayer();

	player->LockControl();
	player->SetAnim(0x3E, FALSE, NULL, NULL, NULL, 0.0f, 2.0f, 0.0f, 10.0f); // anim 0x3E at double rate
	onOffWork._68 = 0;
	onOffWork._84 = 2;
	return 0;
}

/**
 * @note Address: shun .text 0x628
 * @note Size: 0xA8
 */
extern "C" s32 CmdStartOnOff1(s32 cmd, s32 count, s32* args)
{
	XObjChr* player = gGame.GetPlayer();

	player->LockControl();
	player->SetAnim(0x3E, FALSE, NULL, NULL, NULL, 0.0f, 2.0f, 0.0f, 10.0f); // anim 0x3E at double rate
	onOffWork._68 = 0;
	onOffWork._84 = 1;
	return 0;
}

/**
 * @note Address: shun .text 0x6D0
 * @note Size: 0xA4
 */
extern "C" s32 CmdStartOnOff0(s32 cmd, s32 count, s32* args)
{
	XObjChr* player = gGame.GetPlayer();

	player->LockControl();
	player->SetAnim(0x3E, FALSE, NULL, NULL, NULL, 0.0f, 2.0f, 0.0f, 10.0f); // anim 0x3E at double rate
	onOffWork._68 = 0;
	onOffWork._84 = 0;
	return 0;
}

/**
 * @note Address: shun .text 0x774
 * @note Size: 0x2C
 */
extern "C" s32 CmdResetOnOff(s32 cmd, s32 count, s32* args)
{
	onOffWork._00            = (FRAMES_PER_SECOND * 60) / 2; // half a minute
	onOffWork._08            = 0;
	onOffWork._78            = 0;
	onOffWork._7C            = 0;
	onOffWork.mStatus.mFlags = 0;
	return 0;
}

/**
 * @note Address: shun .text 0x7A0
 * @note Size: 0x50
 */
extern "C" s32 CmdPrintArgs(s32, s32, s32*)
{
	return 0;
}

/**
 * @note Address: shun .text 0x7F0
 * @note Size: 0x4
 */
extern "C" void ShunNoOpSlot5(s32 arg)
{
}

/**
 * @note Address: shun .text 0x7F4
 * @note Size: 0x4
 */
extern "C" void DrawStageHud(s32 arg)
{
}

/**
 * @note Address: shun .text 0x7F8
 * @note Size: 0x4
 */
extern "C" void UpdateStage(s32 arg)
{
}

/**
 * @note Address: shun .text 0x7FC
 * @note Size: 0x4
 */
extern "C" void InitStage(s32 arg)
{
}

/**
 * @note Address: shun .text 0x800
 * @note Size: 0xD4
 */
// CJObj::SetTranslateX, inline in game/cjobj.h
// SetTranslateX__5CJObjFfl

/**
 * @note Address: shun .text 0x8D4
 * @note Size: 0xD4
 */
// CJObj::SetTranslateY, inline in game/cjobj.h
// SetTranslateY__5CJObjFfl

/**
 * @note Address: shun .text 0x9A8
 * @note Size: 0xD4
 */
// CJObj::SetTranslateZ, inline in game/cjobj.h
// SetTranslateZ__5CJObjFfl

/**
 * @note Address: shun .text 0xA7C
 * @note Size: 0xD4
 */
// CJObj::SetScaleX, inline in game/cjobj.h
// SetScaleX__5CJObjFfl

/**
 * @note Address: shun .text 0xB50
 * @note Size: 0xD4
 */
// CJObj::SetScaleY, inline in game/cjobj.h
// SetScaleY__5CJObjFfl

/**
 * @note Address: shun .text 0xC24
 * @note Size: 0xD4
 */
// CJObj::SetScaleZ, inline in game/cjobj.h
// SetScaleZ__5CJObjFfl

/**
 * @note Address: shun .text 0xCF8
 * @note Size: 0x1A8
 */
// CJObj::SetRotationX, inline in game/cjobj.h
// SetRotationX__5CJObjFfl

/**
 * @note Address: shun .text 0xEA0
 * @note Size: 0x1A8
 */
// CJObj::SetRotationY, inline in game/cjobj.h
// SetRotationY__5CJObjFfl

/**
 * @note Address: shun .text 0x1048
 * @note Size: 0x1A8
 */
// CJObj::SetRotationZ, inline in game/cjobj.h
// SetRotationZ__5CJObjFfl

/**
 * @note Address: shun .text 0x11F0
 * @note Size: 0x178
 */
// CJObj::SetTranslate, inline in game/cjobj.h
// SetTranslate__5CJObjFfffl

/**
 * @note Address: shun .text 0x1368
 * @note Size: 0x158
 */
// CJObj::SetTranslate, inline in game/cjobj.h
// SetTranslate__5CJObjFRC3Vecl

/**
 * @note Address: shun .text 0x14C0
 * @note Size: 0x178
 */
// CJObj::SetScale, inline in game/cjobj.h
// SetScale__5CJObjFfffl

/**
 * @note Address: shun .text 0x1638
 * @note Size: 0x158
 */
// CJObj::SetScale, inline in game/cjobj.h
// SetScale__5CJObjFRC3Vecl

/**
 * @note Address: shun .text 0x1790
 * @note Size: 0x22C
 */
// CJObj::SetRotation, inline in game/cjobj.h
// SetRotation__5CJObjFfffl

/**
 * @note Address: shun .text 0x19BC
 * @note Size: 0x20C
 */
// CJObj::SetRotation, inline in game/cjobj.h

/**
 * @note Address: shun .text 0x1BC8
 * @note Size: 0x80
 */
// CJObj::SetAnimRate, inline in game/cjobj.h

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

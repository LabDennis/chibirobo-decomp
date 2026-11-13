#include "game/cjobj.h"
#include "game/cjobjset.h"
#include "game/video.h"
#include "Version.h"
extern "C" {
#include <sysdolphin/aobj.h>
#include <sysdolphin/mtx.h>
}

// CGame
extern "C" u8 gGame[];

struct ShunObj {
#if defined(VERSION_GGTJ01_01)
	u8 _00[0x220]; // _00, set to half a minute in frames (FRAMES_PER_SECOND * 30) by CmdResetOnOff; never read, shun's UpdateStage is empty
#else
	u8 _00[0x228]; // _00
#endif
	u64 mFlags; // _228, JP _220
};
#if defined(VERSION_GGTJ01_01)
typedef char ShunObjViewSizeCheck[sizeof(ShunObj) == 0x228 ? 1 : -1];
#else
typedef char ShunObjViewSizeCheck[sizeof(ShunObj) == 0x230 ? 1 : -1];
#endif

struct ShunObjList {
	void* mScript; // _00, CEvtData::mScript of gGame.mEvt (CGame + 0x5D080 = mEvt _5D07C + mData _04); objects are looked up only while a
	               // script is loaded
};

struct SavedObjFlag {
	s32 mId;  // _00
	u8 mFlag; // _04, bit 0: the object's drawn flag (0x10000) was set
};

struct SavedObjFlagList {
	void Apply(s32 mode);

	SavedObjFlag mEntries[0x100]; // _00
	s32 mCount;                   // _800
};

extern "C" void SetJunkUnkoFlag(s32);
extern "C" ShunObj* ResolveEventObjectId(ShunObjList* list, s16 id);
extern "C" void InfoScrn_SetTimerHundredths(void* obj, s32 arg);
extern "C" s32 InfoScrn_GetTimerHundredths(void* obj);
extern "C" void InfoScrn_ShowTimer(void* obj);
extern "C" void InfoScrn_HideTimer(void* obj);
extern "C" void InfoScrn_ApplyTimerHudCommand(void* obj, s32, s32);
extern "C" void SetFafaPartVisibility(s32 arg);

// an offset into CGame: japan lays it out differently from the others
#if defined(VERSION_JP)
#define GAME_OFS(other, japan) (japan)
#else
#define GAME_OFS(other, japan) (other)
#endif
#define SHUN_OBJ_LIST()  ((ShunObjList*)(gGame + GAME_OFS(0x5D080, 0x5D038)))
#define SHUN_TIMER_OBJ() ((void*)(gGame + GAME_OFS(0x8F38, 0x8EF0))) // the InfoScrn timer object
#define SHUN_PLAYER()    (*(RoboView**)(gGame + GAME_OFS(0x96BC, 0x9674)))

inline ShunObj* ShunFindObj(s16 id)
{
	ShunObjList* list = SHUN_OBJ_LIST();
	if (list->mScript != NULL) {
		return ResolveEventObjectId(list, id);
	}
	return NULL;
}

// object at CGame + 0x96BC, a CJObj subclass
class RoboView : public CJObj {
public:
	virtual void vf_4C();
	virtual void vf_50();
	virtual void vf_54();
	virtual void vf_58();
	virtual void vf_5C();
	virtual void vf_60();
	virtual void vf_64();
	virtual void SetPos(f32 x, f32 y, f32 z, s32 force);
	virtual void vf_6C();
	virtual void vf_70();
	virtual void vf_74();
	virtual void SetMoveDir(const Vec* dir, f32 speed);
	virtual void vf_7C();
	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate);

#if defined(VERSION_JP)
	u8 _38[0x970 - 0x38];     // _38, three CVecs, no access in any edition
	u32 mPlayerStatus;        // _970
	u8 _984[0x12CF4 - 0x974]; // _974
	s32 mPadReset;            // _12CF4
#else
	u8 _38[0x980 - 0x38]; // _38
	u32 mPlayerStatus; // _980, XObjChr::mPlayerStatus.mFlags (JP _970); 0x40000000 hands him to a script, 0x20000000 | 7 gives control back
	u8 _984[0x12DF4 - 0x984]; // _984
	s32 mPadReset; // _12DF4, XObjChr::mPad.mReset (XPad at _12DA0, mReset at +0x54; JP _12CF4); 1 with the give-back flags, 0 with the
	               // script flags
#endif
};

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

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void SavedObjFlagList::Apply(s32 mode)
{
	s32 i;
	ShunObj* obj;

	// mode 0 clears the drawn bit, anything else restores the bits CmdSaveObjFlag saw
	if (mode != 0) {
		if (mCount > 0) {
			s32 j;
			for (j = 0; j < mCount; j++) {
				if (mEntries[j].mId == -1) { // -1 ends the list
					break;
				}
				obj = ShunFindObj(mEntries[j].mId);
				if (mEntries[j].mFlag & 1) { // its drawn bit was saved
					obj->mFlags |= 0x10000;
				}
			}
		}
	} else {
		if (mCount > 0) {
			for (i = 0; i < mCount; i++) {
				if (mEntries[i].mId == -1) { // -1 ends the list
					break;
				}
				obj = ShunFindObj(mEntries[i].mId);
				obj->mFlags &= ~0x10000ULL;
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
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stmw     r27, 0xc(r1)
    lwz      r0, 0x0(r5)
    cmpwi    r0, 0x0
    beq      .L_000000EC
    lis      r3, savedObjFlags@ha
    addi     r30, r3, savedObjFlags@l
    lwz      r0, 0x800(r30)
    cmpwi    r0, 0x0
    ble      .L_00000180
    lis      r3, gGame@ha
    mr       r29, r30
    addi     r3, r3, gGame@l
    li       r31, 0x0
    addis    r28, r3, 0x6
    subi     r28, r28, 0x2f80
    b        .L_000000DC
.L_0000008C:
    lwz      r3, 0x0(r29)
    cmpwi    r3, -0x1
    beq      .L_00000180
    lwz      r0, 0x0(r28)
    extsh    r4, r3
    cmplwi   r0, 0x0
    beq      .L_000000B4
    mr       r3, r28
    bl       ResolveEventObjectId
    b        .L_000000B8
.L_000000B4:
    li       r3, 0x0
.L_000000B8:
    lbz      r0, 0x4(r29)
    clrlwi.  r0, r0, 31
    beq      .L_000000D4
    lwz      r5, 0x22c(r3)
    lis      r0, 0x1
    or       r0, r5, r0
    stw      r0, 0x22c(r3)
.L_000000D4:
    addi     r29, r29, 0x8
    addi     r31, r31, 0x1
.L_000000DC:
    lwz      r0, 0x800(r30)
    cmpw     r31, r0
    blt      .L_0000008C
    b        .L_00000180
.L_000000EC:
    lis      r3, savedObjFlags@ha
    addi     r30, r3, savedObjFlags@l
    lwz      r0, 0x800(r30)
    cmpwi    r0, 0x0
    ble      .L_00000180
    lis      r4, gGame@ha
    lis      r3, 0xffff
    addi     r4, r4, gGame@l
    mr       r28, r30
    addis    r27, r4, 0x6
    subi     r31, r3, 0x1
    li       r29, 0x0
    subi     r27, r27, 0x2f80
    b        .L_00000174
.L_00000124:
    lwz      r3, 0x0(r28)
    cmpwi    r3, -0x1
    beq      .L_00000180
    lwz      r0, 0x0(r27)
    extsh    r4, r3
    cmplwi   r0, 0x0
    beq      .L_0000014C
    mr       r3, r27
    bl       ResolveEventObjectId
    b        .L_00000150
.L_0000014C:
    li       r3, 0x0
.L_00000150:
    lwz      r0, 0x22c(r3)
    li       r4, -0x1
    lwz      r5, 0x228(r3)
    addi     r28, r28, 0x8
    and      r0, r0, r31
    addi     r29, r29, 0x1
    stw      r0, 0x22c(r3)
    and      r0, r5, r4
    stw      r0, 0x228(r3)
.L_00000174:
    lwz      r0, 0x800(r30)
    cmpw     r29, r0
    blt      .L_00000124
.L_00000180:
    lmw      r27, 0xc(r1)
    li       r3, 0x0
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: shun .text 0x198
 * @note Size: 0xF0
 */
extern "C" s32 CmdSaveObjFlag(s32 cmd, s32 count, s32* args)
{
	s32 id = args[0];
	ShunObj* obj;

	if (savedObjFlags.mCount < 0x100) {
		savedObjFlags.mEntries[savedObjFlags.mCount].mId = id;
		obj                                              = ShunFindObj(savedObjFlags.mEntries[savedObjFlags.mCount].mId);
		if (obj != NULL && (obj->mFlags & 1)) { // objects without flag 1 are skipped
			if (obj->mFlags & 0x10000) {        // drawn now: remember it
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
 * @note Address: N/A
 * @note Size: unknown
 */
// UNUSED FUNCTION
// stripped function that used a CJObj. mwcc puts header inlines after their first
// user, that's why the CJObj dtor follows CmdClearSavedObjFlags. same in stage07
extern "C" void UnusedCJObjUser(void)
{
	CJObj jobj;
}

/**
 * @note Address: shun .text 0x338
 * @note Size: 0x88
 */
// CJObj::~CJObj, inline in game/cjobj.h

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
		InfoScrn_SetTimerHundredths(SHUN_TIMER_OBJ(), args[1]);
	} else {
		ret = InfoScrn_GetTimerHundredths(SHUN_TIMER_OBJ());
	}
	return ret;
}

/**
 * @note Address: shun .text 0x474
 * @note Size: 0x58
 */
extern "C" s32 CmdSetTimerRunning(s32 cmd, s32 count, s32* args)
{
	// shows or hides the timer, despite the name
	if (args[0] != 0) {
		InfoScrn_ShowTimer(SHUN_TIMER_OBJ());
	} else {
		InfoScrn_HideTimer(SHUN_TIMER_OBJ());
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
		InfoScrn_ApplyTimerHudCommand(SHUN_TIMER_OBJ(), 1, 1);
	} else {
		InfoScrn_ApplyTimerHudCommand(SHUN_TIMER_OBJ(), 0, 1);
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
	RoboView* player = SHUN_PLAYER();

	player->mPlayerStatus = 0;
	player->mPlayerStatus |= 0x40000000; // hand him to the script
	player->mPadReset = 0;
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
	RoboView* player = SHUN_PLAYER();

	player->mPlayerStatus = 0;
	player->mPlayerStatus |= 0x40000000; // hand him to the script
	player->mPadReset = 0;
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
	RoboView* player = SHUN_PLAYER();

	player->mPlayerStatus = 0;
	player->mPlayerStatus |= 0x40000000; // hand him to the script
	player->mPadReset = 0;
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
extern "C" s32 CmdPrintArgs(s32 cmd, s32 count, s32* args)
{
	int i;

	// body compiled out
	for (i = 0; i < count; i++) { }
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
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stfd     f31, 0x30(r1)
    psq_st   f31, 0x38(r1), 0, qr0
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    fmr      f31, f1
    cmpwi    r4, 0x0
    lis      r4, sShunVectorTemplates@ha
    mr       r29, r3
    addi     r31, r4, sShunVectorTemplates@l
    bne      .L_00000D3C
    lfs      f0, 0x14(r29)
    fcmpu    cr0, f31, f0
    beq      .L_00000E7C
.L_00000D3C:
    stfs     f31, 0x14(r29)
    lwz      r30, 0x0(r29)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_00000DF0
    addi     r3, r29, 0x14
    addi     r4, r1, 0x8
    bl       EulerToQuat
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00000D78
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x290
    bl       __assert
.L_00000D78:
    lfs      f0, 0x8(r1)
    stfs     f0, 0x1c(r30)
    lfs      f0, 0xc(r1)
    stfs     f0, 0x20(r30)
    lfs      f0, 0x10(r1)
    stfs     f0, 0x24(r30)
    lfs      f0, 0x14(r1)
    stfs     f0, 0x28(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00000E7C
    cmplwi   r30, 0x0
    beq      .L_00000E7C
    bne      .L_00000DC0
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_00000DC0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00000DDC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00000DDC
    li       r3, 0x1
.L_00000DDC:
    clrlwi.  r0, r3, 24
    bne      .L_00000E7C
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_00000E7C
.L_00000DF0:
    cmplwi   r30, 0x0
    bne      .L_00000E08
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x2a4
    bl       __assert
.L_00000E08:
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_00000E24
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x7b8
    li       r4, 0x2a5
    bl       __assert
.L_00000E24:
    stfs     f31, 0x1c(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00000E7C
    cmplwi   r30, 0x0
    beq      .L_00000E7C
    bne      .L_00000E50
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_00000E50:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00000E6C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00000E6C
    li       r3, 0x1
.L_00000E6C:
    clrlwi.  r0, r3, 24
    bne      .L_00000E7C
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00000E7C:
    psq_l    f31, 0x38(r1), 0, qr0
    lwz      r0, 0x44(r1)
    lfd      f31, 0x30(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: shun .text 0xEA0
 * @note Size: 0x1A8
 */
// CJObj::SetRotationY, inline in game/cjobj.h
// SetRotationY__5CJObjFfl
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stfd     f31, 0x30(r1)
    psq_st   f31, 0x38(r1), 0, qr0
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    fmr      f31, f1
    cmpwi    r4, 0x0
    lis      r4, sShunVectorTemplates@ha
    mr       r29, r3
    addi     r31, r4, sShunVectorTemplates@l
    bne      .L_00000EE4
    lfs      f0, 0x18(r29)
    fcmpu    cr0, f31, f0
    beq      .L_00001024
.L_00000EE4:
    stfs     f31, 0x18(r29)
    lwz      r30, 0x0(r29)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_00000F98
    addi     r3, r29, 0x14
    addi     r4, r1, 0x8
    bl       EulerToQuat
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00000F20
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x290
    bl       __assert
.L_00000F20:
    lfs      f0, 0x8(r1)
    stfs     f0, 0x1c(r30)
    lfs      f0, 0xc(r1)
    stfs     f0, 0x20(r30)
    lfs      f0, 0x10(r1)
    stfs     f0, 0x24(r30)
    lfs      f0, 0x14(r1)
    stfs     f0, 0x28(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001024
    cmplwi   r30, 0x0
    beq      .L_00001024
    bne      .L_00000F68
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_00000F68:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00000F84
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00000F84
    li       r3, 0x1
.L_00000F84:
    clrlwi.  r0, r3, 24
    bne      .L_00001024
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_00001024
.L_00000F98:
    cmplwi   r30, 0x0
    bne      .L_00000FB0
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x2b8
    bl       __assert
.L_00000FB0:
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_00000FCC
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x7b8
    li       r4, 0x2b9
    bl       __assert
.L_00000FCC:
    stfs     f31, 0x20(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001024
    cmplwi   r30, 0x0
    beq      .L_00001024
    bne      .L_00000FF8
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_00000FF8:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001014
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001014
    li       r3, 0x1
.L_00001014:
    clrlwi.  r0, r3, 24
    bne      .L_00001024
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00001024:
    psq_l    f31, 0x38(r1), 0, qr0
    lwz      r0, 0x44(r1)
    lfd      f31, 0x30(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: shun .text 0x1048
 * @note Size: 0x1A8
 */
// CJObj::SetRotationZ, inline in game/cjobj.h
// SetRotationZ__5CJObjFfl
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stfd     f31, 0x30(r1)
    psq_st   f31, 0x38(r1), 0, qr0
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    fmr      f31, f1
    cmpwi    r4, 0x0
    lis      r4, sShunVectorTemplates@ha
    mr       r29, r3
    addi     r31, r4, sShunVectorTemplates@l
    bne      .L_0000108C
    lfs      f0, 0x1c(r29)
    fcmpu    cr0, f31, f0
    beq      .L_000011CC
.L_0000108C:
    stfs     f31, 0x1c(r29)
    lwz      r30, 0x0(r29)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_00001140
    addi     r3, r29, 0x14
    addi     r4, r1, 0x8
    bl       EulerToQuat
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_000010C8
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x290
    bl       __assert
.L_000010C8:
    lfs      f0, 0x8(r1)
    stfs     f0, 0x1c(r30)
    lfs      f0, 0xc(r1)
    stfs     f0, 0x20(r30)
    lfs      f0, 0x10(r1)
    stfs     f0, 0x24(r30)
    lfs      f0, 0x14(r1)
    stfs     f0, 0x28(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_000011CC
    cmplwi   r30, 0x0
    beq      .L_000011CC
    bne      .L_00001110
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_00001110:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_0000112C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_0000112C
    li       r3, 0x1
.L_0000112C:
    clrlwi.  r0, r3, 24
    bne      .L_000011CC
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_000011CC
.L_00001140:
    cmplwi   r30, 0x0
    bne      .L_00001158
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x2cc
    bl       __assert
.L_00001158:
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_00001174
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x7b8
    li       r4, 0x2cd
    bl       __assert
.L_00001174:
    stfs     f31, 0x24(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_000011CC
    cmplwi   r30, 0x0
    beq      .L_000011CC
    bne      .L_000011A0
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_000011A0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_000011BC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_000011BC
    li       r3, 0x1
.L_000011BC:
    clrlwi.  r0, r3, 24
    bne      .L_000011CC
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_000011CC:
    psq_l    f31, 0x38(r1), 0, qr0
    lwz      r0, 0x44(r1)
    lfd      f31, 0x30(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: shun .text 0x11F0
 * @note Size: 0x178
 */
// CJObj::SetTranslate, inline in game/cjobj.h
// SetTranslate__5CJObjFfffl
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    cmpwi    r4, 0x0
    lis      r4, sShunVectorTemplates@ha
    stw      r0, 0x34(r1)
    stw      r31, 0x2c(r1)
    addi     r31, r4, sShunVectorTemplates@l
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    mr       r29, r3
    bne      .L_000012A0
    lis      r3, gJointTranslateZero@ha
    lfs      f4, 0x4(r29)
    addi     r4, r3, gJointTranslateZero@l
    li       r0, 0x0
    lwz      r3, 0x0(r4)
    lwz      r5, 0x4(r4)
    stw      r3, 0x8(r1)
    lwz      r4, 0x8(r4)
    stfs     f1, 0x8(r1)
    lwz      r3, 0x8(r1)
    stw      r5, 0xc(r1)
    stw      r4, 0x10(r1)
    stw      r3, 0x14(r1)
    stfs     f2, 0xc(r1)
    lfs      f0, 0x14(r1)
    stfs     f3, 0x10(r1)
    lwz      r4, 0xc(r1)
    fcmpu    cr0, f4, f0
    lwz      r3, 0x10(r1)
    stw      r4, 0x18(r1)
    stw      r3, 0x1c(r1)
    bne      .L_00001294
    lfs      f4, 0x8(r29)
    lfs      f0, 0x18(r1)
    fcmpu    cr0, f4, f0
    bne      .L_00001294
    lfs      f4, 0xc(r29)
    lfs      f0, 0x1c(r1)
    fcmpu    cr0, f4, f0
    beq      .L_00001298
.L_00001294:
    li       r0, 0x1
.L_00001298:
    clrlwi.  r0, r0, 24
    beq      .L_0000134C
.L_000012A0:
    stfs     f1, 0x4(r29)
    stfs     f2, 0x8(r29)
    stfs     f3, 0xc(r29)
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_000012C8
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x3a9
    bl       __assert
.L_000012C8:
    addic.   r0, r29, 0x4
    bne      .L_000012E0
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6c0
    li       r4, 0x3aa
    bl       __assert
.L_000012E0:
    lfs      f0, 0x4(r29)
    stfs     f0, 0x38(r30)
    lfs      f0, 0x8(r29)
    stfs     f0, 0x3c(r30)
    lfs      f0, 0xc(r29)
    stfs     f0, 0x40(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_0000134C
    cmplwi   r30, 0x0
    beq      .L_0000134C
    bne      .L_00001320
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_00001320:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_0000133C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_0000133C
    li       r3, 0x1
.L_0000133C:
    clrlwi.  r0, r3, 24
    bne      .L_0000134C
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_0000134C:
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: shun .text 0x1368
 * @note Size: 0x158
 */
// CJObj::SetTranslate, inline in game/cjobj.h
// SetTranslate__5CJObjFRC3Vecl
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    cmpwi    r5, 0x0
    lis      r5, sShunVectorTemplates@ha
    stw      r0, 0x34(r1)
    stw      r31, 0x2c(r1)
    addi     r31, r5, sShunVectorTemplates@l
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    mr       r29, r3
    bne      .L_000013EC
    lwz      r3, 0x0(r4)
    li       r0, 0x0
    lwz      r5, 0x4(r4)
    stw      r3, 0x8(r1)
    lwz      r3, 0x8(r4)
    lfs      f1, 0x4(r29)
    lfs      f0, 0x8(r1)
    stw      r5, 0xc(r1)
    fcmpu    cr0, f1, f0
    stw      r3, 0x10(r1)
    bne      .L_000013E0
    lfs      f1, 0x8(r29)
    lfs      f0, 0xc(r1)
    fcmpu    cr0, f1, f0
    bne      .L_000013E0
    lfs      f1, 0xc(r29)
    lfs      f0, 0x10(r1)
    fcmpu    cr0, f1, f0
    beq      .L_000013E4
.L_000013E0:
    li       r0, 0x1
.L_000013E4:
    clrlwi.  r0, r0, 24
    beq      .L_000014A4
.L_000013EC:
    lfs      f0, 0x0(r4)
    lfs      f1, 0x4(r4)
    stfs     f0, 0x4(r29)
    lfs      f0, 0x8(r4)
    stfs     f1, 0x8(r29)
    stfs     f0, 0xc(r29)
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00001420
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x3a9
    bl       __assert
.L_00001420:
    addic.   r0, r29, 0x4
    bne      .L_00001438
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6c0
    li       r4, 0x3aa
    bl       __assert
.L_00001438:
    lfs      f0, 0x4(r29)
    stfs     f0, 0x38(r30)
    lfs      f0, 0x8(r29)
    stfs     f0, 0x3c(r30)
    lfs      f0, 0xc(r29)
    stfs     f0, 0x40(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_000014A4
    cmplwi   r30, 0x0
    beq      .L_000014A4
    bne      .L_00001478
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_00001478:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001494
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001494
    li       r3, 0x1
.L_00001494:
    clrlwi.  r0, r3, 24
    bne      .L_000014A4
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_000014A4:
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: shun .text 0x14C0
 * @note Size: 0x178
 */
// CJObj::SetScale, inline in game/cjobj.h
// SetScale__5CJObjFfffl
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    cmpwi    r4, 0x0
    lis      r4, sShunVectorTemplates@ha
    stw      r0, 0x34(r1)
    stw      r31, 0x2c(r1)
    addi     r31, r4, sShunVectorTemplates@l
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    mr       r29, r3
    bne      .L_00001570
    lis      r3, gJointScaleZero@ha
    lfs      f4, 0x24(r29)
    addi     r4, r3, gJointScaleZero@l
    li       r0, 0x0
    lwz      r3, 0x0(r4)
    lwz      r5, 0x4(r4)
    stw      r3, 0x8(r1)
    lwz      r4, 0x8(r4)
    stfs     f1, 0x8(r1)
    lwz      r3, 0x8(r1)
    stw      r5, 0xc(r1)
    stw      r4, 0x10(r1)
    stw      r3, 0x14(r1)
    stfs     f2, 0xc(r1)
    lfs      f0, 0x14(r1)
    stfs     f3, 0x10(r1)
    lwz      r4, 0xc(r1)
    fcmpu    cr0, f4, f0
    lwz      r3, 0x10(r1)
    stw      r4, 0x18(r1)
    stw      r3, 0x1c(r1)
    bne      .L_00001564
    lfs      f4, 0x28(r29)
    lfs      f0, 0x18(r1)
    fcmpu    cr0, f4, f0
    bne      .L_00001564
    lfs      f4, 0x2c(r29)
    lfs      f0, 0x1c(r1)
    fcmpu    cr0, f4, f0
    beq      .L_00001568
.L_00001564:
    li       r0, 0x1
.L_00001568:
    clrlwi.  r0, r0, 24
    beq      .L_0000161C
.L_00001570:
    stfs     f1, 0x24(r29)
    stfs     f2, 0x28(r29)
    stfs     f3, 0x2c(r29)
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00001598
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x316
    bl       __assert
.L_00001598:
    addic.   r0, r29, 0x24
    bne      .L_000015B0
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6d4
    li       r4, 0x317
    bl       __assert
.L_000015B0:
    lfs      f0, 0x24(r29)
    stfs     f0, 0x2c(r30)
    lfs      f0, 0x28(r29)
    stfs     f0, 0x30(r30)
    lfs      f0, 0x2c(r29)
    stfs     f0, 0x34(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_0000161C
    cmplwi   r30, 0x0
    beq      .L_0000161C
    bne      .L_000015F0
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_000015F0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_0000160C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_0000160C
    li       r3, 0x1
.L_0000160C:
    clrlwi.  r0, r3, 24
    bne      .L_0000161C
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_0000161C:
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: shun .text 0x1638
 * @note Size: 0x158
 */
// CJObj::SetScale, inline in game/cjobj.h
// SetScale__5CJObjFRC3Vecl
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    cmpwi    r5, 0x0
    lis      r5, sShunVectorTemplates@ha
    stw      r0, 0x34(r1)
    stw      r31, 0x2c(r1)
    addi     r31, r5, sShunVectorTemplates@l
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    mr       r29, r3
    bne      .L_000016BC
    lwz      r3, 0x0(r4)
    li       r0, 0x0
    lwz      r5, 0x4(r4)
    stw      r3, 0x8(r1)
    lwz      r3, 0x8(r4)
    lfs      f1, 0x24(r29)
    lfs      f0, 0x8(r1)
    stw      r5, 0xc(r1)
    fcmpu    cr0, f1, f0
    stw      r3, 0x10(r1)
    bne      .L_000016B0
    lfs      f1, 0x28(r29)
    lfs      f0, 0xc(r1)
    fcmpu    cr0, f1, f0
    bne      .L_000016B0
    lfs      f1, 0x2c(r29)
    lfs      f0, 0x10(r1)
    fcmpu    cr0, f1, f0
    beq      .L_000016B4
.L_000016B0:
    li       r0, 0x1
.L_000016B4:
    clrlwi.  r0, r0, 24
    beq      .L_00001774
.L_000016BC:
    lfs      f0, 0x0(r4)
    lfs      f1, 0x4(r4)
    stfs     f0, 0x24(r29)
    lfs      f0, 0x8(r4)
    stfs     f1, 0x28(r29)
    stfs     f0, 0x2c(r29)
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_000016F0
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x316
    bl       __assert
.L_000016F0:
    addic.   r0, r29, 0x24
    bne      .L_00001708
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6d4
    li       r4, 0x317
    bl       __assert
.L_00001708:
    lfs      f0, 0x24(r29)
    stfs     f0, 0x2c(r30)
    lfs      f0, 0x28(r29)
    stfs     f0, 0x30(r30)
    lfs      f0, 0x2c(r29)
    stfs     f0, 0x34(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001774
    cmplwi   r30, 0x0
    beq      .L_00001774
    bne      .L_00001748
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_00001748:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001764
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001764
    li       r3, 0x1
.L_00001764:
    clrlwi.  r0, r3, 24
    bne      .L_00001774
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00001774:
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: shun .text 0x1790
 * @note Size: 0x22C
 */
// CJObj::SetRotation, inline in game/cjobj.h
// SetRotation__5CJObjFfffl
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    cmpwi    r4, 0x0
    lis      r4, sShunVectorTemplates@ha
    stw      r0, 0x44(r1)
    stw      r31, 0x3c(r1)
    addi     r31, r4, sShunVectorTemplates@l
    stw      r30, 0x38(r1)
    stw      r29, 0x34(r1)
    mr       r29, r3
    bne      .L_00001840
    lis      r3, gJointRotationZero@ha
    lfs      f4, 0x4(r29)
    addi     r4, r3, gJointRotationZero@l
    li       r0, 0x0
    lwz      r3, 0x0(r4)
    lwz      r5, 0x4(r4)
    stw      r3, 0x8(r1)
    lwz      r4, 0x8(r4)
    stfs     f1, 0x8(r1)
    lwz      r3, 0x8(r1)
    stw      r5, 0xc(r1)
    stw      r4, 0x10(r1)
    stw      r3, 0x14(r1)
    stfs     f2, 0xc(r1)
    lfs      f0, 0x14(r1)
    stfs     f3, 0x10(r1)
    lwz      r4, 0xc(r1)
    fcmpu    cr0, f4, f0
    lwz      r3, 0x10(r1)
    stw      r4, 0x18(r1)
    stw      r3, 0x1c(r1)
    bne      .L_00001834
    lfs      f4, 0x8(r29)
    lfs      f0, 0x18(r1)
    fcmpu    cr0, f4, f0
    bne      .L_00001834
    lfs      f4, 0xc(r29)
    lfs      f0, 0x1c(r1)
    fcmpu    cr0, f4, f0
    beq      .L_00001838
.L_00001834:
    li       r0, 0x1
.L_00001838:
    clrlwi.  r0, r0, 24
    beq      .L_000019A0
.L_00001840:
    stfs     f1, 0x14(r29)
    stfs     f2, 0x18(r29)
    stfs     f3, 0x1c(r29)
    lwz      r30, 0x0(r29)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_000018FC
    addi     r3, r29, 0x14
    addi     r4, r1, 0x20
    bl       EulerToQuat
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00001884
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x290
    bl       __assert
.L_00001884:
    lfs      f0, 0x20(r1)
    stfs     f0, 0x1c(r30)
    lfs      f0, 0x24(r1)
    stfs     f0, 0x20(r30)
    lfs      f0, 0x28(r1)
    stfs     f0, 0x24(r30)
    lfs      f0, 0x2c(r1)
    stfs     f0, 0x28(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_000019A0
    cmplwi   r30, 0x0
    beq      .L_000019A0
    bne      .L_000018CC
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_000018CC:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_000018E8
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_000018E8
    li       r3, 0x1
.L_000018E8:
    clrlwi.  r0, r3, 24
    bne      .L_000019A0
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_000019A0
.L_000018FC:
    cmplwi   r30, 0x0
    bne      .L_00001914
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x290
    bl       __assert
.L_00001914:
    addic.   r0, r29, 0x14
    bne      .L_0000192C
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6cc
    li       r4, 0x291
    bl       __assert
.L_0000192C:
    lfs      f0, 0x14(r29)
    stfs     f0, 0x1c(r30)
    lfs      f0, 0x18(r29)
    stfs     f0, 0x20(r30)
    lfs      f0, 0x1c(r29)
    stfs     f0, 0x24(r30)
    lfs      f0, 0x20(r29)
    stfs     f0, 0x28(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_000019A0
    cmplwi   r30, 0x0
    beq      .L_000019A0
    bne      .L_00001974
    addi     r3, r31, 0x6b0
    addi     r5, r31, 0x6b8
    li       r4, 0x25d
    bl       __assert
.L_00001974:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001990
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001990
    li       r3, 0x1
.L_00001990:
    clrlwi.  r0, r3, 24
    bne      .L_000019A0
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_000019A0:
    lwz      r0, 0x44(r1)
    lwz      r31, 0x3c(r1)
    lwz      r30, 0x38(r1)
    lwz      r29, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

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

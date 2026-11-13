#include "Version.h"

// the hallway stage: its entry points and its script commands. the cars are in car.cpp. the
// functions are in the order they were written, retail's code runs the other way (-inline deferred)

#include "game/hsd_jobj.h"
#include "game/ut.h"
#include "game/cgame.h"
#include "game/caminfo.h"
#include "game/infoscrn.h"
#include "rel/stage02/fighter.h"

namespace Car {

// prints "Car::Init"
void Init(void);

} // namespace Car

static XObj* takiObj;

extern "C" {
void SetJunkUnkoFlag(u8 on);
BOOL HSD_AudioSFXSetPanByID(s32 id, u8 pan);

s32 CmdGetTimer(s32 cmd, s32 count, s32* args);
s32 CmdSetTimer(s32 cmd, s32 count, s32* args);
s32 CmdSetTimerRunning(s32 cmd, s32 count, s32* args);
s32 CmdShowTimer(s32 cmd, s32 count, s32* args);
s32 CmdSetCarData(s32 cmd, s32 count, s32* args);
s32 CmdTerminateCars(s32 cmd, s32 count, s32* args);
s32 CmdGetCarState(s32 cmd, s32 count, s32* args);
s32 CmdStartCars(s32 cmd, s32 count, s32* args);
s32 CmdRegisterCar(s32 cmd, s32 count, s32* args);
s32 CmdClearCamInfo(s32 cmd, s32 count, s32* args);
s32 CmdSetCamInfo(s32 cmd, s32 count, s32* args);
s32 CmdClearObjFlagList(s32 cmd, s32 count, s32* args);
s32 CmdAddObjFlagList(s32 cmd, s32 count, s32* args);
s32 CmdSetJunkUnkoFlag(s32 cmd, s32 count, s32* args);
s32 CmdShowFighterCenterPanel(s32 cmd, s32 count, s32* args);
s32 CmdShowFighterLowerPanel(s32 cmd, s32 count, s32* args);
s32 CmdRegisterFighterLink(s32 cmd, s32 count, s32* args);
s32 CmdGetFighterEvent(s32 cmd, s32 count, s32* args);
s32 CmdQueryFighter(s32 cmd, s32 count, s32* args);
s32 CmdSetFighterData(s32 cmd, s32 count, s32* args);
s32 CmdEndFighter(s32 cmd, s32 count, s32* args);
s32 CmdAddMato(s32 cmd, s32 count, s32* args);
s32 CmdStartFighter(s32 cmd, s32 count, s32* args);
void ExitStage(void* arg);
void DrawStageHud(void* arg);
void DrawStageEffects(void* arg);
void DrawStage(void* arg);
void UpdateStage(void* arg);
void InitStage(void* arg);
void RestoreDefaultCamera(void);
void Terminate(void);
s32 GetPlayerCarFrame(void);
s32 GetPlayerCarResult(void);
void SetCarSoundCues(u8* data);
void SetCarParams(u8* data);
void DrawCarHud(void* arg);
void UpdateCars(u32 arg);
void StartCars(void);
void StartCarCamera(void);
s32 RegisterCar(XObj* a, XObj* b, XObj* c);
}

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern "C" {
RelSlotFunc _prolog_slots[7] = {
	InitStage, UpdateStage, DrawStage, DrawStageEffects, DrawStageHud, NULL, ExitStage,
};
RelCmdFunc _prolog_cmds[] = {
	CmdStartFighter,
	CmdAddMato,
	CmdEndFighter,
	CmdSetFighterData,
	CmdQueryFighter,
	CmdAddObjFlagList,
	CmdClearObjFlagList,
	CmdSetCamInfo,
	CmdClearCamInfo,
	CmdGetFighterEvent,
	CmdRegisterFighterLink,
	CmdRegisterCar,
	CmdStartCars,
	CmdGetCarState,
	CmdTerminateCars,
	CmdShowTimer,
	CmdSetTimerRunning,
	CmdSetTimer,
	CmdGetTimer,
	CmdSetCarData,
	CmdShowFighterLowerPanel,
	CmdShowFighterCenterPanel,
	CmdSetJunkUnkoFlag,
};
}

/**
 * @note Address: stage02 .text 0x6DAC
 * @note Size: 0xB0
 */
extern "C" void InitStage(void* arg)
{
	XObj* obj;
	s32 count;
	s32 i;

	ClearFighterObjects();
	Car::Init();
	takiObj = NULL;
	count   = gGame.mXObjs.mObjectCount;
	for (i = 0; takiObj == NULL && i < count; i++) {
		obj = gGame.GetObject(i);
		if ((obj->mStatus.mFlags & 1) != 0 && obj->mCoord.mPlacement->mObjectId == 0x254) {
			takiObj = obj;
		}
	}
}

// InitStage
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    bl       ClearFighterObjects
    bl       Init__3CarFv
    lis      r3, gGame@ha
    lis      r4, takiObj@ha
    addi     r8, r3, gGame@l
    li       r0, 0x0
    addi     r6, r4, takiObj@l
    li       r10, 0x0
    addis    r3, r8, 0x1
    stw      r0, 0x0(r6)
    lwz      r9, -0x57a0(r3)
    b        .L_00006E38
.L_00006DE8:
    addis    r4, r8, 0x1
    li       r3, 0x1
    lwz      r7, -0x67a0(r4)
    li       r4, 0x0
    lwz      r0, 0x228(r7)
    lwz      r5, 0x22c(r7)
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_00006E30
    lwz      r3, 0x238(r7)
    lwz      r0, 0x0(r3)
    cmpwi    r0, 0x254
    bne      .L_00006E30
    lis      r3, takiObj@ha
    stw      r7, takiObj@l(r3)
.L_00006E30:
    addi     r8, r8, 0x4
    addi     r10, r10, 0x1
.L_00006E38:
    lwz      r0, 0x0(r6)
    cmplwi   r0, 0x0
    bne      .L_00006E4C
    cmpw     r10, r9
    blt      .L_00006DE8
.L_00006E4C:
    lwz      r0, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage02 .text 0x6C34
 * @note Size: 0x178
 */
extern "C" void UpdateStage(void* arg)
{
	UpdateFighter((u32)arg);
	UpdateCars((u32)arg);
	if (takiObj != NULL) {
		if (takiObj->mAnimId == 0) {
			XSound* sound = &gGame.mSound;
			if (IsGameSoundPlaying(sound, 0x20019) == 0) {
				PlayGameSound(sound, 0x20019, 0xFF);
				HSD_AudioSFXSetPanByID(0x20019, 0);
			}
			if (IsGameSoundPlaying(sound, 0x2001A) == 0) {
				PlayGameSound(sound, 0x2001A, 0xFF);
				HSD_AudioSFXSetPanByID(0x2001A, 0xFE);
			}
			SetGameSfxVolume(sound, 0x20019, GetSoundVolumeAtPosition(sound, &takiObj->mMoveTranslate, 1), 0);
			SetGameSfxVolume(sound, 0x2001A, GetSoundVolumeAtPosition(sound, &takiObj->mMoveTranslate, 1), 0);
		} else {
			XSound* sound = &gGame.mSound;
			StopGameSfx(sound, 0x20019);
			StopGameSfx(sound, 0x2001A);
		}
	}
}

/**
 * @note Address: stage02 .text 0x6C14
 * @note Size: 0x20
 */
extern "C" void DrawStage(void* arg)
{
	DrawFighterStage();
}

/**
 * @note Address: stage02 .text 0x6BF4
 * @note Size: 0x20
 */
extern "C" void DrawStageEffects(void* arg)
{
	DrawShots();
}

/**
 * @note Address: stage02 .text 0x6BC0
 * @note Size: 0x34
 */
extern "C" void DrawStageHud(void* arg)
{
	DrawFighterHud();
	DrawCarHud(arg);
}

/**
 * @note Address: stage02 .text 0x6B9C
 * @note Size: 0x24
 */
extern "C" void ExitStage(void* arg)
{
	EndFighter();
	RestoreDefaultCamera();
}

/**
 * @note Address: stage02 .text 0x6AA0
 * @note Size: 0xFC
 */
extern "C" s32 CmdStartFighter(s32 cmd, s32 count, s32* args)
{
	XObj* objs[3] = {};
	u32 j;

#if !defined(VERSION_PAL)
	OSReport("args = ");
	for (s32 i = 0; i < count; i++) {
		OSReport("%d ", args[i]);
	}
	OSReport("\n");
#endif
	for (j = 0; j < 3 && (s32)j - 1 < count; j++) {
		objs[j] = FindObj(args[j]);
	}
	StartFighter(objs[0], objs[1], objs[2]);
	return 0;
}

/**
 * @note Address: stage02 .text 0x6A20
 * @note Size: 0x80
 */
extern "C" s32 CmdAddMato(s32 cmd, s32 count, s32* args)
{
	s32 i;
	for (i = 0; i < count; i++) {
		AddMato(gGame.GetObject(args[i]), args[i]);
	}
	return 0;
}

// CmdAddMato
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    lis      r3, gGame@ha
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    addi     r31, r3, gGame@l
    stw      r30, 0x18(r1)
    mr       r30, r5
    stw      r29, 0x14(r1)
    li       r29, 0x0
    stw      r28, 0x10(r1)
    mr       r28, r4
    b        .L_00006A74
.L_00006A54:
    lwz      r4, 0x0(r30)
    slwi     r0, r4, 2
    add      r3, r31, r0
    addis    r3, r3, 0x1
    lwz      r3, -0x67a0(r3)
    bl       AddMato
    addi     r30, r30, 0x4
    addi     r29, r29, 0x1
.L_00006A74:
    cmpw     r29, r28
    blt      .L_00006A54
    lwz      r0, 0x24(r1)
    li       r3, 0x0
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: stage02 .text 0x69FC
 * @note Size: 0x24
 */
extern "C" s32 CmdEndFighter(s32 cmd, s32 count, s32* args)
{
	EndFighter();
	return 0;
}

/**
 * @note Address: stage02 .text 0x6994
 * @note Size: 0x68
 */
extern "C" s32 CmdSetFighterData(s32 cmd, s32 count, s32* args)
{
	switch (args[0]) {
	case 0:
		SetSpeedRateTable((u8*)args[1]);
		SetFighterParams((u8*)args[2]);
		if (count >= 4) {
			SetSoundCueTable((u8*)args[3]);
		}
		break;
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x68A8
 * @note Size: 0xEC
 */
extern "C" s32 CmdQueryFighter(s32 cmd, s32 count, s32* args)
{
	s32 percent = GetFighterProgress() * 100.0f;
	u32 flag    = GetTimeLeft();

	switch (args[0]) {
	case 0:
		if (percent < 100 && flag != 0) {
			return 1;
		}
		return 0;
	case 1:
		return GetScore();
	case 2:
		return flag;
	case 3:
		return percent * 100;
	case 4:
		return TakeDestroyedMatoId();
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6888
 * @note Size: 0x20
 */
extern "C" s32 CmdGetFighterEvent(s32 cmd, s32 count, s32* args)
{
	return GetShotEvent();
}

/**
 * @note Address: stage02 .text 0x67CC
 * @note Size: 0xBC
 */
extern "C" s32 CmdRegisterFighterLink(s32 cmd, s32 count, s32* args)
{
	XObj* objs[4];
	u32 i;

	for (i = 0; i < 4; i++) {
		objs[i] = NULL;
	}
	for (i = 0; i < 4 && (s32)i - 1 < count; i++) {
		objs[i] = FindObj(args[i]);
	}
	InitPilotNwing(objs[0], objs[1], objs[2], objs[3]);
	return 0;
}

// CmdRegisterFighterLink
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    lis      r3, gGame@ha
    stw      r0, 0x34(r1)
    li       r0, 0x0
    addi     r3, r3, gGame@l
    stmw     r27, 0x1c(r1)
    addis    r30, r3, 0x6
    addi     r29, r1, 0x8
    mr       r27, r4
    stw      r0, 0x8(r1)
    mr       r31, r5
    li       r28, 0x0
    subi     r30, r30, 0x2f80
    stw      r0, 0xc(r1)
    stw      r0, 0x10(r1)
    stw      r0, 0x14(r1)
    b        .L_00006848
.L_00006814:
    lwz      r0, 0x0(r30)
    lwz      r3, 0x0(r31)
    cmplwi   r0, 0x0
    extsh    r4, r3
    beq      .L_00006834
    mr       r3, r30
    bl       ResolveEventObjectId
    b        .L_00006838
.L_00006834:
    li       r3, 0x0
.L_00006838:
    stw      r3, 0x0(r29)
    addi     r31, r31, 0x4
    addi     r29, r29, 0x4
    addi     r28, r28, 0x1
.L_00006848:
    cmplwi   r28, 0x4
    bge      .L_0000685C
    subi     r0, r28, 0x1
    cmpw     r0, r27
    blt      .L_00006814
.L_0000685C:
    lwz      r3, 0x8(r1)
    lwz      r4, 0xc(r1)
    lwz      r5, 0x10(r1)
    lwz      r6, 0x14(r1)
    bl       InitPilotNwing
    lmw      r27, 0x1c(r1)
    li       r3, 0x0
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: stage02 .text 0x678C
 * @note Size: 0x40
 */
extern "C" s32 CmdShowFighterLowerPanel(s32 cmd, s32 count, s32* args)
{
	if (args[0] != 0) {
		ShowLowerPanel(true);
	} else {
		ShowLowerPanel(false);
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6744
 * @note Size: 0x48
 */
extern "C" s32 CmdShowFighterCenterPanel(s32 cmd, s32 count, s32* args)
{
	if (args[0] != 0) {
		ShowCenterPanel(true, args[1]);
	} else {
		ShowCenterPanel(false, 0);
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6704
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
 * @note Address: stage02 .text 0x668C
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
 * @note Address: stage02 .text 0x6658
 * @note Size: 0x34
 */
extern "C" s32 CmdClearObjFlagList(s32 cmd, s32 count, s32* args)
{
	static_cast<CCameraActionScroll2*>(gGame.mCameraActions.mTable[CAMACTION_SCROLL2])->ClearHideList();
	return 0;
}

/**
 * @note Address: stage02 .text 0x6610
 * @note Size: 0x48
 */
// fn_5_6610

/**
 * @note Address: stage02 .text 0x64BC
 * @note Size: 0x154
 */
extern "C" s32 CmdSetCamInfo(s32 cmd, s32 count, s32* args)
{
	CamInfoData info = { 0, { 0, 79300, 0 }, { 1, 10000, 0 }, 6000 };
	static_cast<CCameraActionScroll2*>(gGame.mCameraActions.mTable[CAMACTION_SCROLL2])->SaveCamInfo(&info);
	return 0;
}

/**
 * @note Address: stage02 .text 0x6484
 * @note Size: 0x38
 */
extern "C" s32 CmdClearCamInfo(s32 cmd, s32 count, s32* args)
{
	static_cast<CCameraActionScroll2*>(gGame.mCameraActions.mTable[CAMACTION_SCROLL2])->SaveCamInfo(NULL);
	return 0;
}

/**
 * @note Address: stage02 .text 0x63A4
 * @note Size: 0xE0
 */
extern "C" s32 CmdRegisterCar(s32 cmd, s32 count, s32* args)
{
	XObj* c = NULL;
	if (count >= 3) {
		c = FindObj(args[2]);
	}
	return RegisterCar(FindObj(args[0]), FindObj(args[1]), c);
}

/**
 * @note Address: stage02 .text 0x635C
 * @note Size: 0x48
 */
extern "C" s32 CmdStartCars(s32 cmd, s32 count, s32* args)
{
	if (count == 0) {
		StartCars();
		return 0;
	}
	switch (args[0]) {
	case 1:
		StartCarCamera();
		break;
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6314
 * @note Size: 0x48
 */
extern "C" s32 CmdGetCarState(s32 cmd, s32 count, s32* args)
{
	if (count == 0) {
		return GetPlayerCarResult();
	}
	switch (args[0]) {
	case 1:
		return GetPlayerCarFrame();
	}
	return -1;
}

/**
 * @note Address: stage02 .text 0x62F0
 * @note Size: 0x24
 */
extern "C" s32 CmdTerminateCars(s32 cmd, s32 count, s32* args)
{
	Terminate();
	return 0;
}

/**
 * @note Address: stage02 .text 0x6298
 * @note Size: 0x58
 */
extern "C" s32 CmdSetCarData(s32 cmd, s32 count, s32* args)
{
	SetCarParams((u8*)args[0]);
	if (count < 2) {
		return 0;
	}
	SetCarSoundCues((u8*)args[1]);
	return 0;
}

/**
 * @note Address: stage02 .text 0x6230
 * @note Size: 0x68
 */
extern "C" s32 CmdShowTimer(s32 cmd, s32 count, s32* args)
{
	if (args[0] != 0) {
		InfoScrn_ApplyTimerHudCommand(&gGame.mInfoScrn, 1, 1);
	} else {
		InfoScrn_ApplyTimerHudCommand(&gGame.mInfoScrn, 0, 1);
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x61D8
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
 * @note Address: stage02 .text 0x61A0
 * @note Size: 0x38
 */
extern "C" s32 CmdSetTimer(s32 cmd, s32 count, s32* args)
{
	InfoScrn_SetTimerHundredths(&gGame.mInfoScrn, args[0]);
	return 0;
}

/**
 * @note Address: stage02 .text 0x6170
 * @note Size: 0x30
 */
extern "C" s32 CmdGetTimer(s32 cmd, s32 count, s32* args)
{
	return InfoScrn_GetTimerHundredths(&gGame.mInfoScrn);
}

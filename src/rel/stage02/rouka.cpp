#include "game/relmodule.h"
#include "Version.h"

// the hallway stage: its entry points and its script commands. the cars are in car.cpp. the
// functions are in the order they were written, retail's code runs the other way (-inline deferred)

#include "game/hsd_jobj.h"
#include "game/caminfo.h"
#include "game/cgame.h"
#include "game/csound.h"
#include "game/fighter.h"

namespace Car {

// prints "Car::Init"
void Init(void);

} // namespace Car

static XObj* takiObj;

static inline CCameraActionScroll2* GetScrollCamera()
{
	return static_cast<CCameraActionScroll2*>(gGame.mCameraActions.mTable[CAMACTION_SCROLL2]);
}

extern "C" {
s32 InfoScrn_GetTimerHundredths(InfoScrn* screen);
void InfoScrn_SetTimerHundredths(InfoScrn* screen, s32 hundredths);
void InfoScrn_HideTimer(InfoScrn* screen);
void InfoScrn_ShowTimer(InfoScrn* screen);
void InfoScrn_ApplyTimerHudCommand(InfoScrn* screen, u8 show, s32 mode);
void SetJunkUnkoFlag(u8 on);

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
void ExitStage(s32 arg);
void DrawStageHud(s32 arg);
void DrawStageEffects(s32 arg);
void DrawStage(s32 arg);
void UpdateStage(s32 arg);
void InitStage(s32 arg);
void RestoreDefaultCamera(void);
void Terminate(void);
s32 GetPlayerCarFrame(void);
s32 GetPlayerCarResult(void);
void SetCarSoundCues(u8* data);
void SetCarParams(u8* data);
void DrawCarHud(s32 frames);
void UpdateCars(u32 arg);
void StartCars(void);
void StartCarCamera(void);
s32 RegisterCar(XObj* a, XObj* b, XObj* c);
}

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
extern "C" void InitStage(s32 arg)
{
	XObj* obj;
	s32 count;
	s32 i;

	ClearFighterObjects();
	Car::Init();
	takiObj = NULL;
	count   = gGame.mXObjs.mObjectCount;
	for (i = 0; takiObj == NULL && i < count; i++) { // find the waterfall
		obj = gGame.GetObject(i);
		if ((obj->mStatus.mFlags & 1) != 0 && obj->mCoord.mPlacement->mObjectId == 0x254) {
			takiObj = obj;
		}
	}
}

// InitStage

/**
 * @note Address: stage02 .text 0x6C34
 * @note Size: 0x178
 */
extern "C" void UpdateStage(s32 arg)
{
	UpdateFighter(arg);
	UpdateCars(arg);
	if (takiObj != NULL) {
		if (takiObj->mAnimId == 0) { // its ambient water sounds follow it
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
		} else { // another anim: silence it
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
extern "C" void DrawStage(s32 arg)
{
	DrawFighterStage();
}

/**
 * @note Address: stage02 .text 0x6BF4
 * @note Size: 0x20
 */
extern "C" void DrawStageEffects(s32 arg)
{
	DrawShots();
}

/**
 * @note Address: stage02 .text 0x6BC0
 * @note Size: 0x34
 */
extern "C" void DrawStageHud(s32 arg)
{
	DrawFighterHud();
	DrawCarHud(arg);
}

/**
 * @note Address: stage02 .text 0x6B9C
 * @note Size: 0x24
 */
extern "C" void ExitStage(s32 arg)
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
	XObj* objs[3];
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
	case 0: // speed rates, params, optional sound cues
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
	case 0: // still flying
		if (percent < 100 && flag != 0) {
			return 1;
		}
		return 0;
	case 1: // the score
		return GetScore();
		break;
	case 2: // the time left
		return flag;
	case 3: // the progress in hundredths of a percent
		return percent * 100;
	case 4: // the last destroyed target's id
		return TakeDestroyedMatoId();
	}
	if (args[0] == 0) {
		return GetFighterProgress() * 100.0f;
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
		GetScrollCamera()->AddHideObj(args[i]); // objects the scroll camera hides
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6658
 * @note Size: 0x34
 */
extern "C" s32 CmdClearObjFlagList(s32 cmd, s32 count, s32* args)
{
	GetScrollCamera()->ClearHideList();
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
	const CamInfoData info = { 0, { 0, 79300, 0 }, { 1, 10000, 0 }, 6000 };
	GetScrollCamera()->SaveCamInfo(&info);
	return 0;
}

/**
 * @note Address: stage02 .text 0x6484
 * @note Size: 0x38
 */
extern "C" s32 CmdClearCamInfo(s32 cmd, s32 count, s32* args)
{
	GetScrollCamera()->SaveCamInfo(NULL);
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
	case 1: // just the camera
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
		return GetPlayerCarResult(); // no arg: the race result
	}
	switch (args[0]) {
	case 1: // the player's frame
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

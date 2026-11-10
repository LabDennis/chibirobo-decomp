#include "types.h"

extern "C" {
s32 CmdGetTimer(s32 cmd, s32 argc, s32* argv);
s32 CmdSetTimer(s32 cmd, s32 argc, s32* argv);
s32 CmdSetTimerRunning(s32 cmd, s32 argc, s32* argv);
s32 CmdShowTimer(s32 cmd, s32 argc, s32* argv);
s32 CmdSetCarData(s32 cmd, s32 argc, s32* argv);
s32 CmdTerminateCars(s32 cmd, s32 argc, s32* argv);
s32 CmdGetCarState(s32 cmd, s32 argc, s32* argv);
s32 CmdStartCars(s32 cmd, s32 argc, s32* argv);
s32 CmdRegisterCar(s32 cmd, s32 argc, s32* argv);
s32 CmdClearCamInfo(s32 cmd, s32 argc, s32* argv);
s32 CmdSetCamInfo(s32 cmd, s32 argc, s32* argv);
s32 CmdClearObjFlagList(s32 cmd, s32 argc, s32* argv);
s32 CmdAddObjFlagList(s32 cmd, s32 argc, s32* argv);
s32 CmdSetJunkUnkoFlag(s32 cmd, s32 argc, s32* argv);
s32 CmdShowFighterCenterPanel(s32 cmd, s32 argc, s32* argv);
s32 CmdShowFighterLowerPanel(s32 cmd, s32 argc, s32* argv);
s32 CmdRegisterFighterLink(s32 cmd, s32 argc, s32* argv);
s32 CmdGetFighterEvent(s32 cmd, s32 argc, s32* argv);
s32 CmdQueryFighter(s32 cmd, s32 argc, s32* argv);
s32 CmdSetFighterData(s32 cmd, s32 argc, s32* argv);
s32 CmdEndFighter(s32 cmd, s32 argc, s32* argv);
s32 CmdAddMato(s32 cmd, s32 argc, s32* argv);
s32 CmdStartFighter(s32 cmd, s32 argc, s32* argv);
void ExitStage(void* arg);
void DrawStageHud(void* arg);
void DrawStageEffects(void* arg);
void DrawStage(void* arg);
void UpdateStage(void* arg);
void InitStage(void* arg);
}

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 argc, s32* argv);

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

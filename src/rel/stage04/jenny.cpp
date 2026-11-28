#if defined(VERSION_R24J01)
#include "types.h"

extern "C" s32 InitAnas(void);
extern "C" void DrawAnas(void);
extern "C" void UpdateAnas(s32 dt);
extern "C" void ExitAnas(void);
extern "C" void InitTrash(void);
extern "C" void UpdateTrash(u32 frames);
extern "C" void DrawTrashStack(void);
extern "C" void ExitTrash(void);

extern "C" s32 Stage04_Init(void* arg)
{
	InitAnas();
	InitTrash();
	return 1;
}

extern "C" s32 Stage04_Update(void* arg)
{
	UpdateAnas((s32)arg);
	UpdateTrash((u32)arg);
	return 1;
}

extern "C" s32 Stage04_Draw(void* arg)
{
	DrawAnas();
	DrawTrashStack();
	return 1;
}

extern "C" s32 Stage04_Exit(void* arg)
{
	ExitAnas();
	ExitTrash();
	return 1;
}
#else
#include "game/reltrash.h"
#include "game/relana.h"
#include "game/relmodule.h"
#include "types.h"

extern "C" void DrawStage(s32 arg);
extern "C" void UpdateStage(s32 arg);
extern "C" void InitStage(s32 arg);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage, DrawStage };
RelCmdFunc _prolog_cmds[]    = {
    CmdAddAna,          CmdSetTutiAna, CmdFindNearestAna, CmdPlantTane, CmdSetAnaJObj,  CmdSetAnaRotY,     CmdDropHeldObj,
    CmdAttachAnaToRobo, CmdStartTrash, CmdGetTrashResult, CmdEndTrash,  CmdCancelTrash, CmdIsNearTrashbox,
};
}

/**
 * @note Address: stage04 .text 0x0
 * @note Size: 0x24
 */
extern "C" void DrawStage(s32 arg)
{
	DrawAnas();
	DrawTrashStack();
}

/**
 * @note Address: stage04 .text 0x24
 * @note Size: 0x34
 */
extern "C" void UpdateStage(s32 arg)
{
	UpdateAnas(arg);
	UpdateTrash(arg);
}

/**
 * @note Address: stage04 .text 0x58
 * @note Size: 0x24
 */
extern "C" void InitStage(s32 arg)
{
	InitAnas();
	InitTrash();
}
#endif

#include "rel/trash.h"

// rel/ana.cpp
extern "C" void DrawAnas(void);
extern "C" void UpdateAnas(void);
extern "C" void InitAnas(void);
extern "C" s32 CmdAttachAnaToRobo(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdSetAnaRotY(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdSetAnaJObj(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdPlantTane(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdFindNearestAna(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdSetTutiAna(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdAddAna(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdDropHeldObj(s32 cmd, s32 count, s32* args);

extern "C" void DrawStage(void* arg);
extern "C" void UpdateStage(void* arg);
extern "C" void InitStage(void* arg);

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

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
extern "C" void DrawStage(void* arg)
{
	DrawAnas();
	DrawTrashStack();
}

/**
 * @note Address: stage04 .text 0x24
 * @note Size: 0x34
 */
extern "C" void UpdateStage(void* arg)
{
	UpdateAnas();
	UpdateTrash(reinterpret_cast<u32>(arg));
}

/**
 * @note Address: stage04 .text 0x58
 * @note Size: 0x24
 */
extern "C" void InitStage(void* arg)
{
	InitAnas();
	InitTrash();
}

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

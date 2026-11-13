#include "types.h"

extern "C" s32 CmdNop(s32 cmd, s32 count, s32* args);

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern "C" {
RelSlotFunc _prolog_slots[7] = { 0 };
RelCmdFunc _prolog_cmds[]    = { CmdNop };
}

/**
 * @note Address: stage08 .text 0x0
 * @note Size: 0x8
 */
extern "C" s32 CmdNop(s32 cmd, s32 count, s32* args)
{
	return 0;
}

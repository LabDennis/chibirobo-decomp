#ifndef _GAME_RELMODULE_H
#define _GAME_RELMODULE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*RelSlotFunc)(s32 frames);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern RelSlotFunc _prolog_slots[7];
extern RelCmdFunc _prolog_cmds[];

s32 _prolog(s32 cmd, s32 count, s32* args);

#ifdef __cplusplus
}
#endif

#endif

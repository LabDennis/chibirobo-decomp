#ifndef _GAME_RELTRASH_H
#define _GAME_RELTRASH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void InitTrash(void);
void DrawTrashStack(void);
void UpdateTrash(u32 frames);

s32 CmdStartTrash(s32 cmd, s32 count, s32* args);
s32 CmdGetTrashResult(s32 cmd, s32 count, s32* args);
s32 CmdEndTrash(s32 cmd, s32 count, s32* args);
s32 CmdCancelTrash(s32 cmd, s32 count, s32* args);
s32 CmdIsNearTrashbox(s32 cmd, s32 count, s32* args);

#ifdef __cplusplus
}
#endif

#endif

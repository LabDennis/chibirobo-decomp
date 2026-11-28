#ifndef _REL_TRASH_H
#define _REL_TRASH_H

#include "types.h"

extern "C" {
void InitTrash(void);
void UpdateTrash(u32 frames);
void DrawTrashStack(void);
s32 CmdStartTrash(s32 cmd, s32 count, s32* args);
s32 CmdGetTrashResult(s32 cmd, s32 count, s32* args);
s32 CmdEndTrash(s32 cmd, s32 count, s32* args);
s32 CmdCancelTrash(s32 cmd, s32 count, s32* args);
s32 CmdIsNearTrashbox(s32 cmd, s32 count, s32* args);
}

#endif

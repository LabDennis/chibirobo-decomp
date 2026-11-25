#ifndef _GAME_RELANA_H
#define _GAME_RELANA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void InitAnas(void);
void DrawAnas(void);
void UpdateAnas(s32 frames);

s32 CmdAddAna(s32 cmd, s32 count, s32* args);
s32 CmdSetTutiAna(s32 cmd, s32 count, s32* args);
s32 CmdFindNearestAna(s32 cmd, s32 count, s32* args);
s32 CmdPlantTane(s32 cmd, s32 count, s32* args);
s32 CmdSetAnaJObj(s32 cmd, s32 count, s32* args);
s32 CmdSetAnaRotY(s32 cmd, s32 count, s32* args);
s32 CmdDropHeldObj(s32 cmd, s32 count, s32* args);
s32 CmdAttachAnaToRobo(s32 cmd, s32 count, s32* args);

#ifdef __cplusplus
}
#endif

#endif

#ifndef _REL_STAGE02_FIGHTER_H
#define _REL_STAGE02_FIGHTER_H

#include "types.h"

class XObj;

extern "C" {
void StartFighter(XObj* body, XObj* path, XObj* attachment);
void InitPilotNwing(XObj* carriedObj, XObj* path, XObj* attachment0, XObj* attachment1);
void AddMato(XObj* obj, s32 id);
void ClearFighterObjects(void);
void EndFighter(void);
void UpdateFighter(u32 step);
void DrawFighterHud(void);
void DrawFighterStage(void);
void DrawShots(void);
void ShowCenterPanel(bool on, s32 variant);
void ShowLowerPanel(bool on);
void SetSoundCueTable(u8* data);
void SetFighterParams(u8* data);
void SetSpeedRateTable(u8* rates);
s32 TakeDestroyedMatoId(void);
s32 GetShotEvent(void);
s32 GetScore(void);
u32 GetTimeLeft(void);
f32 GetFighterProgress(void);
}

#endif

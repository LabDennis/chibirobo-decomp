#ifndef _STAGE02_FIGHTER_H
#define _STAGE02_FIGHTER_H

#include "types.h"

class XObj;

extern "C" {
void ShowCenterPanel(bool on, s32 variant);
void ShowLowerPanel(bool on);
void InitPilotNwing(XObj* carried, XObj* path, XObj* attached0, XObj* attached1);
s32 GetShotEvent(void);
void EndFighter(void);
s32 TakeDestroyedMatoId(void);
s32 GetScore(void);
u32 GetTimeLeft(void);
f32 GetFighterProgress(void);
void SetSoundCueTable(u8* data);
void SetFighterParams(u8* data);
void SetSpeedRateTable(u8* rates);
void AddMato(XObj* obj, s32 id);
void StartFighter(XObj* body, XObj* path, XObj* extra);
void DrawFighterHud(void);
void DrawShots(void);
void DrawFighterStage(void);
void UpdateFighter(u32 frames);
void ClearFighterObjects(void);
}

#endif

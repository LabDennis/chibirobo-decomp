#ifndef _GAME_STAGEDATA_H
#define _GAME_STAGEDATA_H

#include "types.h"

// what stage.cpp hands out of the loaded stage file. apart from stage.h, which drags in
// the sdk os headers through CModule

class EnvInfo;
struct UnkStageObj;
struct CamInfoData;
struct CamAreaRecord;

struct StageEventObject {
	s32 objectIndex; // A negative index ends the list.
	s32 eventId;
};
STATIC_ASSERT(sizeof(StageEventObject) == 8);

extern "C" {
UnkStageObj* GetSpawnData(s32 index);
UnkStageObj* GetStageObjectPlacementByIndex(s32 index);
s32 CallStageModule(s32 function, s32 count, s32* args);
s32 GetStageCollectionFlagBase(s32 index);
const StageEventObject* GetStageEventObjectPairs(void);
CamAreaRecord* GetStageCameraAreaExRecords(void);
CamInfoData* GetStageCameraInfoRecords(void);
CamAreaRecord* GetStageCameraAreaRecords(void);
u8* GetStageSetupScript(void);
u8* GetStageEntryScript(void);
u8* GetStageBatteryDepletedScript(void);
u8* GetStageZButtonScript(void);
u8* GetStageTimerExpiredScript(void);
u8* GetStageDataBuffer(void);
EnvInfo* GetStageEnvironment(void);
s32 FindStageIndexByName(const char* name);
}

#endif

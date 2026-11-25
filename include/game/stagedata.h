#ifndef _GAME_STAGEDATA_H
#define _GAME_STAGEDATA_H

#include "types.h"

// Views into resources owned by the loaded stage.
class EnvInfo {
public:
	u8* mData; // _00
	// _04 vtable

	EnvInfo(u8* data)
	    : mData(data)
	{
	}
	virtual ~EnvInfo() { } // _08
};
STATIC_ASSERT(sizeof(EnvInfo) == 8);

struct StagePlacement;
struct CamInfoData;
struct CamAreaRecord;

extern "C" {
StagePlacement* GetSpawnData(s32 index);
StagePlacement* GetStageObjectPlacementByIndex(s32 index);
s32 CallStageModule(s32 function, s32 count, s32* args);
s32 GetStageCollectionFlagBase(s32 index);
CamAreaRecord* GetStageCameraAreaExRecords(void);
CamInfoData* GetStageCameraInfoRecords(void);
CamAreaRecord* GetStageCameraAreaRecords(void);
u8* GetStageSetupScript(void);
u8* GetStageEntryScript(void);
u8* GetStageBatteryDepletedScript(void);
u8* GetStageZButtonScript(void);
u8* GetStageTimerExpiredScript(void);
u8* GetStageScriptByIndex(s32 index);
u8* GetStageDataBuffer(void);
EnvInfo* GetStageEnvironment(void);
u8* GetGlobalTextEntry(s32 index);
BOOL IsStageDataLoaded(void);
s32 FindStageIndexByName(const char* name);
}

inline const u8* GetEnvData()
{
	return GetStageEnvironment()->mData;
}

#endif

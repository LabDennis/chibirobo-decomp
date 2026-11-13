#ifndef _GAME_STAGETABLE_H
#define _GAME_STAGETABLE_H

#include "types.h"

struct StageDesc {
	const char* fileName;
#if defined(VERSION_PAL) || defined(VERSION_R24J01)
	const char* moduleName;
#endif
#if defined(VERSION_R24J01)
	const char* soundBank;
	s32 firstSoundId;
#endif
	s32 message;
	u32 flags;
};
extern "C" StageDesc stageTable[];

#if defined(VERSION_R24J01)
STATIC_ASSERT(sizeof(StageDesc) == 0x18);
extern "C" s32 GetStageFirstSoundId(s32 index);
extern "C" const char* GetStageDisplayName(s32 index);
#endif

#endif

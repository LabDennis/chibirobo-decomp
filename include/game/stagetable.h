#ifndef _GAME_STAGETABLE_H
#define _GAME_STAGETABLE_H

#include "types.h"

struct StageDesc {
	const char* fileName;
#if defined(VERSION_PAL)
	const char* moduleName;
#endif
	s32 message;
	u32 flags;
};
extern "C" StageDesc stageTable[];

#endif

#ifndef _GAME_SOUNDTRACKS_H
#define _GAME_SOUNDTRACKS_H

#include "types.h"

struct MusicEntry {
	const char* path;
	u8 volume;
};
STATIC_ASSERT(sizeof(MusicEntry) == 8);

extern "C" const char sSoundRegisterFormat[];
extern "C" const char sSoundLoadQueueFull[];
extern "C" const char sSoundBankLoadFormat[];
extern "C" const char sSoundSampleSemPath[];
extern "C" const char sSoundSourceFile[];
extern "C" const char sSoundSampleSsmPath[];

extern "C" MusicEntry musicTable[];
extern "C" const char* stageSfxTable[];

#endif

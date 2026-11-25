#ifndef _GAME_SOUNDTRACKS_H
#define _GAME_SOUNDTRACKS_H

#include "types.h"

struct MusicEntry {
	const char* path;
	u8 volume;
};
STATIC_ASSERT(sizeof(MusicEntry) == 8);

extern "C" MusicEntry musicTable[];
extern "C" const char* stageSfxTable[];

#endif

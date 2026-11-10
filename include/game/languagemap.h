#ifndef _GAME_LANGUAGEMAP_H
#define _GAME_LANGUAGEMAP_H

#include "types.h"

struct GameLanguageMapEntry {
	u8 osLanguage;
	u8 gameLanguage;
};

extern "C" GameLanguageMapEntry gGameLanguageMap[6];

#endif

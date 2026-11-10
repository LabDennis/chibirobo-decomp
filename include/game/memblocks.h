#ifndef _GAME_MEMBLOCKS_H
#define _GAME_MEMBLOCKS_H

#include "types.h"

// one entry per block, used or freed. a freed block keeps its address in mFree so Alloc
// can hand it out again, the name is the tag the caller passed. MEMINFO is from the wii
// build's "no space for MEMINFO array" message
struct MEMINFO {
	u8* mAddr;        // _00
	u8* mFree;        // _04
	u32 mSize;        // _08
	char mName[0x80]; // _0C
	s32 mStatic;      // _8C
};
STATIC_ASSERT(sizeof(MEMINFO) == 0x90);

enum { MEMINFO_MAX = 0x400 };

extern "C" MEMINFO gMemoryBlockRecords[MEMINFO_MAX];

#endif

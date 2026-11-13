#ifndef _GAME_ARCACHE_H
#define _GAME_ARCACHE_H

#include "game/cfile.h"

// aram file caching, class names from RTTI, layouts are partial. one of each is a global
// in CGame's file

struct ARCacheEntry {
	u32 address;
	u32 size;
	s32 objectId;
	const char* name;
};

class ARCacheInfo {
public:
	u32 mBaseAddress;            // _00
	u32 mCount;                  // _04
	ARCacheEntry mEntries[1024]; // _08
	// _4008 vtable

	ARCacheInfo()
	    : mBaseAddress(0)
	    , mCount(0)
	{
	}
	virtual ~ARCacheInfo() { } // _08
};
STATIC_ASSERT(sizeof(ARCacheInfo) == 0x400C);

extern "C" ARCacheInfo gARAMCacheInfo;

class ARPreCache {
public:
	u32 _00;                // _00
	CFile mFile0;           // _04
	CFile mFile1;           // _F0
	u8 _1DC[0x1E4 - 0x1DC]; // _1DC
	// _1E4 vtable

	ARPreCache() { _00 = 0; }
	virtual ~ARPreCache() { } // _08
};
STATIC_ASSERT(sizeof(ARPreCache) == 0x1E8);

#endif

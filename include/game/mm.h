#ifndef _GAME_MM_H
#define _GAME_MM_H

#include "types.h"

// the game's memory manager, in mem.cpp. it hands out blocks from the top of the arena
// down and keeps up to 1024 of them in a table with a name each, for the heap dump. mm,
// Alloc and Dump are the game's own spellings from its messages
namespace mm {

// size must be a multiple of 32 or it complains, the name shows up in Dump
void* Alloc(u32 size, const char* name);
#if defined(VERSION_R24J01)
void Free(void* ptr);
#else
// every caller passes 1, it clears the block's static flag
void Free(void* ptr, s32 flag);
#endif
void Dump();
void Init();
// the name a block was allocated with, and setting a new one
void SetName(void* ptr, const char* name);
char* GetName(void* ptr);
// glue freed neighbours together and give freed blocks below the lowest used one back
void Compact(BOOL dump);

} // namespace mm

#endif

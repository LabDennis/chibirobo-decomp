#include "Dolphin/os.h"

#define ROUND_UP(x, align) (((u32)(x) + (align) - 1) & ~((align) - 1))

static void* __OSArenaHi = NULL;
static void* __OSArenaLo = (void*)0xffffffff;

/**
 * @note Address: 0x80162C88
 * @note Size: 0x8
 */
void* OSGetArenaHi(void)
{
	return __OSArenaHi;
}

/**
 * @note Address: 0x80162C90
 * @note Size: 0x8
 */
void* OSGetArenaLo(void)
{
	return __OSArenaLo;
}

/**
 * @note Address: 0x80162C98
 * @note Size: 0x8
 */
void OSSetArenaHi(void* arena_hi)
{
	__OSArenaHi = arena_hi;
}

/**
 * @note Address: 0x80162CA0
 * @note Size: 0x8
 */
void OSSetArenaLo(void* arena_lo)
{
	__OSArenaLo = arena_lo;
}

/**
 * @note Address: 0x80162CA8
 * @note Size: 0x2C
 */
void* OSAllocFromArenaLo(u32 size, u32 align)
{
	void* ptr;
	u8* arenaLo;

	ptr     = (void*)ROUND_UP(__OSArenaLo, align);
	arenaLo = (u8*)ptr + size;
	arenaLo = (u8*)ROUND_UP(arenaLo, align);

	__OSArenaLo = arenaLo;
	return ptr;
}

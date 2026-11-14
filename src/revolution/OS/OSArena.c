#include "types.h"

static void* __OSArenaLo = (void*)-1;
static void* s_mem2ArenaLo = (void*)-1;
static void* __OSArenaHi = NULL;
static void* s_mem2ArenaHi = NULL;

void* OSGetMEM1ArenaHi(void)
{
	return __OSArenaHi;
}

void* OSGetMEM2ArenaHi(void)
{
	return s_mem2ArenaHi;
}

void* OSGetArenaHi(void)
{
	return OSGetMEM1ArenaHi();
}

void* OSGetMEM1ArenaLo(void)
{
	return __OSArenaLo;
}

void* OSGetMEM2ArenaLo(void)
{
	return s_mem2ArenaLo;
}

void* OSGetArenaLo(void)
{
	return OSGetMEM1ArenaLo();
}

void OSSetMEM1ArenaHi(void* hi)
{
	__OSArenaHi = hi;
}

void OSSetMEM2ArenaHi(void* hi)
{
	s_mem2ArenaHi = hi;
}

void OSSetArenaHi(void* hi)
{
	OSSetMEM1ArenaHi(hi);
}

void OSSetMEM1ArenaLo(void* lo)
{
	__OSArenaLo = lo;
}

void OSSetMEM2ArenaLo(void* lo)
{
	s_mem2ArenaLo = lo;
}

void OSSetArenaLo(void* lo)
{
	OSSetMEM1ArenaLo(lo);
}

void* OSAllocFromMEM1ArenaLo(u32 size, u32 align)
{
	void* ptr;
	u8* arenaLo;

	ptr     = (void*)ALIGN_NEXT((u32)OSGetMEM1ArenaLo(), align);
	arenaLo = (u8*)ptr + size;
	arenaLo = (u8*)ALIGN_NEXT((u32)arenaLo, align);

	OSSetMEM1ArenaLo(arenaLo);
	return ptr;
}

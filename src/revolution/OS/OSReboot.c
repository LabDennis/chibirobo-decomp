#include "revolution/hw_regs.h"
#include "revolution/os/OSExec.h"

#include "Dolphin/os.h"

// OSExec.c: partition type handed to the next program. Descriptive name, not from the SDK.
extern u32 __OSNextPartitionType;

static void* SaveEnd;
static void* SaveStart;

void __OSReboot(u32 resetCode, u32 bootDol)
{
	const char* argvToPass[1];

	OSDisableInterrupts();
	OSSetArenaLo((void*)0x81280000);
	OSSetArenaHi((void*)0x812F0000);
	argvToPass[0]         = NULL;
	__OSNextPartitionType = *(u32*)0x80003194;
	__OSBootDol(bootDol, resetCode | 0x80000000, argvToPass);
}

void OSGetSaveRegion(void** start, void** end)
{
	*start = SaveStart;
	*end   = SaveEnd;
}

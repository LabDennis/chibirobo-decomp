#include "game/wii/saveslots.h"
#include "game/cthread.h"
#include "revolution/mem.h"

extern "C" {
extern MEMiHeapHead* gWiiGameHeap;
extern CThread gWiiSaveThread;
extern u8* gWiiSaveThreadStack;
extern void* gWiiSaveNandWorkBuffer;
}

extern "C" void InitializeWiiSaveManager(WiiSaveManager* manager)
{
	InitializeNandGameFile(&manager->mFile);
	gWiiSaveNandWorkBuffer = MEMAllocFromExpHeapEx(gWiiGameHeap, 0x4000, 32);
	gWiiSaveThreadStack    = static_cast<u8*>(MEMAllocFromExpHeapEx(gWiiGameHeap, 0x2000, 32));
	GetNandGameHomeDir(&manager->mFile, manager->mHomeDir);
	u32 blocks, inodes;
	GetNandGameFreeSpace(&manager->mFile, &blocks, &inodes);
}

extern "C" void UpdateWiiSaveJob(WiiSaveManager* manager)
{
	if (manager->mJobStarted && OSIsThreadTerminated(&gWiiSaveThread.mThread))
		manager->mJobStarted = 0;
}

// The save slots: one file from ScanWiiSaveSlots (0x800A9834) to its static initializer (0x800AACEC), which
// constructs gWiiSaveThread. The banner functions before it are a file of their own: WriteWiiSaveSlot calls
// CheckWiiSaveBanner instead of inlining it, and "CB_SKIP(%d)" starts a new, aligned string pool after "banner.bin".
// NONMATCHING: five functions have no source yet: ScanWiiSaveSlots, CaptureWiiSaveSlot and the three after it
// (0x800AA65C and 0x800AA6A4, which read the saved stages, and 0x800AAB18), so the file cannot be linked.
// The static initializer stores the vtable and mStack like retail's but does not register CThread's destructor
// through __register_global_object as retail's does.
#include "game/wii/saveslots.h"
#include "game/cthread.h"
#include "stl/stdio.h"
#include "stl/mem.h"

struct WiiSaveJob {
	WiiSaveManager* mManager;
	s32 mSlot;
	s32 mNewGame;
};

extern "C" {
CThread gWiiSaveThread;
WiiSaveJob gWiiSaveJob;
extern u8* gWiiSaveThreadStack;
extern void* gWiiSaveNandWorkBuffer;

void CaptureWiiSaveSlot(WiiSaveManager* manager, bool newGame);
s32 GetDefaultStageIndex();
void ScanWiiSaveSlots(WiiSaveManager* manager, int count);

void* RunWiiSaveSlotScan(void* param);
void* RunWiiSaveSlotWrite(void* param);
void* RunWiiSaveSlotRead(void* param);
void* RunWiiSaveSlotDelete(void* param);
}

// ScanWiiSaveSlots (0x800A9834) comes here

static void StartWiiSaveJob(WiiSaveManager* manager, OSThreadStartFunction entry)
{
	const u32 stackSize       = 0x2000;
	gWiiSaveThread.mStack     = gWiiSaveThreadStack;
	gWiiSaveThread.mStackSize = stackSize;
	OSCreateThread(&gWiiSaveThread.mThread, entry, &gWiiSaveJob, gWiiSaveThreadStack + stackSize, stackSize, 17, OS_THREAD_ATTR_DETACH);
	OSResumeThread(&gWiiSaveThread.mThread);
	manager->mJobStarted = 1;
}

static u32 GetWiiSaveSlotChecksum(WiiSaveSlot& slot)
{
	slot.mHeader.mChecksum = 0;
	const u8* bytes        = reinterpret_cast<const u8*>(&slot);
	u32 sum                = 0;
	for (u32 offset = 0; offset < sizeof(slot); offset += 2)
		sum += (static_cast<u32>(bytes[offset]) << 8) | bytes[offset + 1];
	return sum ^ 0x50494b53;
}

extern "C" void StartWiiSaveSlotScan(WiiSaveManager* manager, int count)
{
	gWiiSaveJob.mManager = manager;
	gWiiSaveJob.mSlot    = count;
	StartWiiSaveJob(manager, RunWiiSaveSlotScan);
}

extern "C" void* RunWiiSaveSlotScan(void* param)
{
	WiiSaveJob* job = static_cast<WiiSaveJob*>(param);
	ScanWiiSaveSlots(job->mManager, job->mSlot);
	return NULL;
}

extern "C" s32 WriteWiiSaveSlot(WiiSaveManager* manager, int slot, bool newGame)
{
	sprintf(manager->mSlotPath, "CB_SKIP(%d)", slot);
	memset(&manager->mSlot, 0, sizeof(manager->mSlot));
	CaptureWiiSaveSlot(manager, newGame);
	ChangeNandGameDir(&manager->mFile, manager->mHomeDir);
	bool hasBanner = CheckWiiSaveBanner(manager, true) != 0;
	s32 result     = OpenNandGameFile(&manager->mFile, manager->mSlotPath, 2, gWiiSaveNandWorkBuffer, 0x4000);
	if (result != 0) {
		u32 answer;
		result = CheckNandGameSpace(&manager->mFile, hasBanner ? 1 : 5, hasBanner ? 1 : 2, &answer);
		if (result != 0)
			return result;
		if (answer & 5)
			return manager->mFile.mResult = NAND_RESULT_MAXBLOCKS;
		if (answer & 10)
			return manager->mFile.mResult = NAND_RESULT_MAXFILES;
		result = CreateNandGameFile(&manager->mFile, manager->mSlotPath, 0x30, 0);
		if (result != 0)
			return result;
		result = OpenNandGameFile(&manager->mFile, manager->mSlotPath, 2, gWiiSaveNandWorkBuffer, 0x4000);
		if (result != 0)
			return result;
	}
	manager->mSlot.mHeader.mChecksum = GetWiiSaveSlotChecksum(manager->mSlot);
	result                           = WriteNandGameFile(&manager->mFile, &manager->mSlot, sizeof(manager->mSlot));
	if (result < 0) {
		CloseNandGameFile(&manager->mFile);
		return result;
	}
	result = CloseNandGameFile(&manager->mFile);
	if (result != 0)
		return result;
	WriteWiiSaveBanner(manager);
	return 0;
}

extern "C" void StartWiiSaveSlotWrite(WiiSaveManager* manager, int slot, int newGame)
{
	gWiiSaveJob.mManager = manager;
	gWiiSaveJob.mNewGame = newGame;
	gWiiSaveJob.mSlot    = slot;
	StartWiiSaveJob(manager, RunWiiSaveSlotWrite);
}

extern "C" void* RunWiiSaveSlotWrite(void* param)
{
	WiiSaveJob* job = static_cast<WiiSaveJob*>(param);
	WriteWiiSaveSlot(job->mManager, job->mSlot, job->mNewGame != 0);
	return NULL;
}

extern "C" s32 ReadWiiSaveSlot(WiiSaveManager* manager, int slot)
{
	sprintf(manager->mSlotPath, "CB_SKIP(%d)", slot);
	memset(&manager->mSlot, 0, sizeof(manager->mSlot));
	ChangeNandGameDir(&manager->mFile, manager->mHomeDir);
	s32 result = OpenNandGameFile(&manager->mFile, manager->mSlotPath, 1, gWiiSaveNandWorkBuffer, 0x4000);
	if (result != 0)
		return result;
	result = ReadNandGameFile(&manager->mFile, &manager->mSlot, sizeof(manager->mSlot));
	if (result < 0) {
		CloseNandGameFile(&manager->mFile);
		return result;
	}
	u32 storedChecksum = manager->mSlot.mHeader.mChecksum;
	if (storedChecksum != GetWiiSaveSlotChecksum(manager->mSlot))
		return NAND_RESULT_UNKNOWN;
	if (manager->mSlot.mHeader.mStages[0] < 0)
		manager->mSlot.mHeader.mStages[0] = manager->mSlot.mHeader.mStages[1] = GetDefaultStageIndex();
	return CloseNandGameFile(&manager->mFile);
}

extern "C" void StartWiiSaveSlotRead(WiiSaveManager* manager, int slot)
{
	gWiiSaveJob.mManager = manager;
	gWiiSaveJob.mSlot    = slot;
	StartWiiSaveJob(manager, RunWiiSaveSlotRead);
}

extern "C" void* RunWiiSaveSlotRead(void* param)
{
	WiiSaveJob* job = static_cast<WiiSaveJob*>(param);
	ReadWiiSaveSlot(job->mManager, job->mSlot);
	return NULL;
}

extern "C" void StartWiiSaveSlotDelete(WiiSaveManager* manager, int slot)
{
	gWiiSaveJob.mManager = manager;
	gWiiSaveJob.mSlot    = slot;
	StartWiiSaveJob(manager, RunWiiSaveSlotDelete);
}

extern "C" void* RunWiiSaveSlotDelete(void* param)
{
	WiiSaveJob* job         = static_cast<WiiSaveJob*>(param);
	WiiSaveManager* manager = job->mManager;
	sprintf(manager->mSlotPath, "CB_SKIP(%d)", job->mSlot);
	DeleteNandGameFile(&manager->mFile, manager->mSlotPath);
	return NULL;
}

// CaptureWiiSaveSlot (0x800AA2F8) and the functions at 0x800AA65C, 0x800AA6A4 and 0x800AAB18 come here

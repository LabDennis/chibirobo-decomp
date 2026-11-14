// The Wii save manager, one file from InitializeWiiSaveManager (0x800A9320) to its static initializer
// (0x800AACEC), which constructs gWiiSaveThread. Retail's strings show the banner and slot code are one file:
// "banner.bin" is its own object, and the string pool that starts with "CB_SKIP(%d)" follows it 4-aligned,
// where the next file's pool would start 8-aligned.
// NONMATCHING: five functions have no source yet: ScanWiiSaveSlots, CaptureWiiSaveSlot and the three after it
// (0x800AA65C and 0x800AA6A4 read the saved stages, then 0x800AAB18), so the file cannot be linked. Here
// "banner.bin" goes into the string pool, so "CB_SKIP(%d)" sits at +0xB (RunWiiSaveSlotDelete). This
// CheckWiiSaveBanner is smaller than retail's (0xBC against 0xE4), so it is inlined into WriteWiiSaveSlot, and
// BuildWiiSaveBanner into WriteWiiSaveBanner, where retail calls both. ReadWiiSaveSlot does not line up with
// retail yet, StartWiiSaveJob and GetWiiSaveSlotChecksum are also emitted out of line (retail only inlines them),
// and the static initializer does not register CThread's destructor as retail's does. The file's data is not
// assigned to the unit until the rest has source.
#include "game/wii/saveslots.h"
#include "game/wii/cgame.h"
#include "game/cthread.h"
#include "game/globals.h"
#include "revolution/mem.h"
#include "stl/stdio.h"
#include "stl/mem.h"

struct WiiSaveJob {
	WiiSaveManager* mManager;
	s32 mSlot;
	s32 mNewGame;
};

extern "C" {
extern MEMiHeapHead* gWiiGameHeap;

CThread gWiiSaveThread;
WiiSaveJob gWiiSaveJob;
const char* gWiiSaveBannerPath = "banner.bin";
void* gWiiSaveNandWorkBuffer;
u8* gWiiSaveThreadStack;

void ScanWiiSaveSlots(WiiSaveManager* manager, int count);
void CaptureWiiSaveSlot(WiiSaveManager* manager, bool newGame);
s32 GetDefaultStageIndex();

void* RunWiiSaveSlotScan(void* param);
void* RunWiiSaveSlotWrite(void* param);
void* RunWiiSaveSlotRead(void* param);
void* RunWiiSaveSlotDelete(void* param);
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

extern "C" int CheckWiiSaveBanner(WiiSaveManager* manager, bool removeInvalid)
{
	WiiGameSaveFile* file = &manager->mFile;
	if (OpenNandGameFile(file, gWiiSaveBannerPath, 1, gWiiSaveNandWorkBuffer, 0x4000) != 0)
		return 0;
	u32 length;
	bool valid = GetNandGameFileLength(file, &length) == 0 && length == sizeof(manager->mBanner);
	if (CloseNandGameFile(file) != 0)
		valid = false;
	if (!valid && removeInvalid)
		DeleteNandGameFile(file, gWiiSaveBannerPath);
	return valid;
}

extern "C" void BuildWiiSaveBanner(WiiSaveManager* manager)
{
	NANDBanner& banner  = manager->mBanner;
	const u16* subtitle = reinterpret_cast<const u16*>(GetLocalizedInterfaceText(2));
	const u16* title    = reinterpret_cast<const u16*>(GetLocalizedInterfaceText(1));
	NANDInitBanner(&banner, 0, title, subtitle);
	CTpl& textures = gGame.mTpl;
	textures.mTex.Init(&textures.mPalette->descriptorArray[247], 0);
	memcpy(banner.bannerTexture, textures.mTex.mImageData, sizeof(banner.bannerTexture));
	for (int i = 0; i < ARRAY_SIZE(banner.iconTexture); ++i) {
		textures.mTex.Init(&textures.mPalette->descriptorArray[248 + i], 0);
		memcpy(banner.iconTexture[i], textures.mTex.mImageData, sizeof(banner.iconTexture[i]));
		banner.iconSpeed = (banner.iconSpeed & ~(3 << (2 * i))) | (1 << (2 * i));
	}
}

extern "C" s32 WriteWiiSaveBanner(WiiSaveManager* manager)
{
	BuildWiiSaveBanner(manager);
	WiiGameSaveFile* file = &manager->mFile;
	for (int attempt = 0; attempt < 3; ++attempt) {
		s32 result = CreateNandGameFile(file, gWiiSaveBannerPath, 0x30, 0);
		if (result == 0)
			break;
		if (result != NAND_RESULT_EXISTS)
			return result;
		result = DeleteNandGameFile(file, gWiiSaveBannerPath);
		if (result != 0)
			return result;
	}
	s32 result = OpenNandGameFile(file, gWiiSaveBannerPath, 2, gWiiSaveNandWorkBuffer, 0x4000);
	if (result != 0)
		return result;
	result = WriteNandGameFile(file, &manager->mBanner, sizeof(manager->mBanner));
	if (result < 0)
		return result;
	return CloseNandGameFile(file);
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

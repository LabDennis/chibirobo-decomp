// The Wii save manager, one file from InitializeWiiSaveManager (0x800A9320) to its static initializer
// (0x800AACEC), which constructs gWiiSaveThread. Retail's strings show the banner and slot code are one file:
// "banner.bin" is its own object, and the string pool that starts with "CB_SKIP(%d)" follows it 4-aligned,
// where the next file's pool would start 8-aligned.
// NONMATCHING: four functions have no source yet: CaptureWiiSaveSlot and the three after it
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
	u32 flags                 = 0;
	if (OpenNandGameFile(file, gWiiSaveBannerPath, 1, gWiiSaveNandWorkBuffer, 0x4000) == 0) {
		u32 length;
		if (GetNandGameFileLength(file, &length) == 0) {
			if (length == sizeof(manager->mBanner)) {
				if (CloseNandGameFile(file) == 0)
					flags |= 1;
				else
					flags |= 4;
			} else
				flags |= 6;
		} else
			flags |= 6;
	}
	if (!(flags & 1)) {
		if (flags & 2)
			CloseNandGameFile(file);
		if ((flags & 4) && removeInvalid)
			DeleteNandGameFile(file, gWiiSaveBannerPath);
		flags = 0;
	}
	return flags;
}

extern "C" void BuildWiiSaveBanner(WiiSaveManager* manager)
{
	memset(manager->mBanner.comment[0], 0x30, 0x20);
	memset(manager->mBanner.comment[1], 0x30, 0x20);
	const u16* subtitle = reinterpret_cast<const u16*>(GetLocalizedInterfaceText(2));
	const u16* title    = reinterpret_cast<const u16*>(GetLocalizedInterfaceText(1));
	NANDInitBanner(&manager->mBanner, 0, title, subtitle);
	CTpl& textures = gGame.mTpl;
	textures.mTex.Init(&textures.mPalette->descriptorArray[247], 0);
	CTexObj tex = textures.mTex;
	memcpy(manager->mBanner.bannerTexture, tex.mImageData, sizeof(manager->mBanner.bannerTexture));
	for (int i = 0; i < 8; ++i) {
		textures.mTex.Init(&textures.mPalette->descriptorArray[248 + i], 0);
		CTexObj icon = textures.mTex;
		memcpy(manager->mBanner.iconTexture[i], icon.mImageData, sizeof(manager->mBanner.iconTexture[i]));
		manager->mBanner.iconSpeed = (manager->mBanner.iconSpeed & ~(3 << (2 * i))) | (1 << (2 * i));
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
		if (result == NAND_RESULT_EXISTS) {
			result = DeleteNandGameFile(file, gWiiSaveBannerPath);
			if (result == 0)
				continue;
			return result;
		}
		return result;
	}
	s32 result = OpenNandGameFile(file, gWiiSaveBannerPath, 2, gWiiSaveNandWorkBuffer, 0x4000);
	if (result != 0)
		return result;
	result = WriteNandGameFile(file, &manager->mBanner, sizeof(manager->mBanner));
	if (result < 0)
		return result;
	result = CloseNandGameFile(file);
	return result == 0 ? 0 : result;
}

static inline u32 GetWiiSaveSlotChecksum(WiiSaveSlot& slot)
{
	slot.mHeader.mChecksum = 0;
	const u16* p           = reinterpret_cast<const u16*>(&slot);
	u32 sum                = 0;
	u32 i                  = sizeof(slot) / 2;
	while (i-- != 0)
		sum += *p++;
	return sum;
}

void ScanWiiSaveSlots(WiiSaveManager* manager, int count)
{
	static WiiSaveSlot sScanSlot;
	char path[0x40];
	u32 answer;
	int usable = 0;
	int empty  = 0;
	int i;
	s32 result;

	ChangeNandGameDir(&manager->mFile, manager->mHomeDir);
	int hasBanner = CheckWiiSaveBanner(manager, false);

	for (i = 0; i < count; ++i) {
		sprintf(path, "CB_SKIP(%d)", i);
		manager->mScanEntries[i].mStatus = 0;
		memset(manager->mScanEntries[i].mData, 0, sizeof(manager->mScanEntries[i].mData));
		result = OpenNandGameFile(&manager->mFile, path, 1, gWiiSaveNandWorkBuffer, 0x4000);
		GetNandGameResultName(&manager->mFile, result);
		switch (result) {
		case NAND_RESULT_OK: {
			u32 length;
			result = GetNandGameFileLength(&manager->mFile, &length);
			if (result == 0 && length == sizeof(sScanSlot)) {
				result = ReadNandGameFile(&manager->mFile, &sScanSlot, sizeof(sScanSlot));
				if (result >= 0) {
					u32 readResult     = result;
					u32 storedChecksum = sScanSlot.mHeader.mChecksum;
					result             = (storedChecksum ^ 0x50494b53) == GetWiiSaveSlotChecksum(sScanSlot);
					if (result != 0) {
						memcpy(manager->mScanEntries[i].mData, &sScanSlot, sizeof(sScanSlot));
						manager->mScanEntries[i].mStatus = 1;
					} else {
						manager->mScanEntries[i].mStatus = 3;
						GetNandGameResultName(&manager->mFile, readResult);
					}
					CloseNandGameFile(&manager->mFile);
				} else {
					switch (result) {
					case NAND_RESULT_ECC_CRIT:
					case NAND_RESULT_AUTHENTICATION:
					default:
						manager->mScanEntries[i].mStatus = 3;
						CloseNandGameFile(&manager->mFile);
						break;
					case NAND_RESULT_CORRUPT:
						i = count;
						break;
					}
					GetNandGameResultName(&manager->mFile, result);
				}
			} else {
				manager->mScanEntries[i].mStatus = 3;
				CloseNandGameFile(&manager->mFile);
				GetNandGameResultName(&manager->mFile, result);
			}
			++usable;
			break;
		}
		case NAND_RESULT_AUTHENTICATION:
		case NAND_RESULT_ECC_CRIT:
			manager->mScanEntries[i].mStatus = 3;
			++usable;
			break;
		case NAND_RESULT_CORRUPT:
			i = count;
			break;
		case NAND_RESULT_NOEXISTS:
		default:
			manager->mScanEntries[i].mStatus = 4;
			++empty;
			manager->mFile.mResult           = 0;
			break;
		}
	}
	GetNandGameResultName(&manager->mFile, manager->mFile.mResult);
	if (manager->mFile.mResult == 0 && empty > 0) {
		int needed = 0;
		for (i = 0; i < count; ++i) {
			if (!(manager->mScanEntries[i].mStatus & 4))
				continue;
			++needed;
			u32 blocks = needed;
			u32 files  = needed;
			if (!hasBanner) {
				blocks = needed + 4;
				files  = needed + 1;
			}
			result = CheckNandGameSpace(&manager->mFile, blocks, files, &answer);
			GetNandGameResultName(&manager->mFile, result);
			if (result != 0)
				manager->mFile.mResult = 0;
			else if (answer == 0)
				++usable;
			else if (answer & 5)
				manager->mScanEntries[i].mStatus = 8;
			else if (answer & 0xa)
				manager->mScanEntries[i].mStatus = 0x10;
		}
		if (usable == 0) {
			for (i = 0; i < count; ++i) {
				if (manager->mScanEntries[i].mStatus & 8) {
					manager->mFile.mResult = NAND_RESULT_MAXBLOCKS;
					return;
				}
				if (manager->mScanEntries[i].mStatus & 0x10) {
					manager->mFile.mResult = NAND_RESULT_MAXFILES;
					return;
				}
			}
		}
	}
}

static inline void StartWiiSaveJob(WiiSaveManager* manager, OSThreadStartFunction entry)
{
	gWiiSaveThread.mStack     = gWiiSaveThreadStack;
	gWiiSaveThread.mStackSize = 0x2000;
	OSCreateThread(&gWiiSaveThread.mThread, entry, &gWiiSaveJob, gWiiSaveThreadStack + 0x2000, 0x2000, 17, OS_THREAD_ATTR_DETACH);
	OSResumeThread(&gWiiSaveThread.mThread);
	manager->mJobStarted = 1;
}

static inline void MakeWiiSaveSlotPath(WiiSaveManager* manager, int slot)
{
	sprintf(manager->mSlotPath, "CB_SKIP(%d)", slot);
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
	MakeWiiSaveSlotPath(manager, slot);
	memset(&manager->mSlot, 0, sizeof(manager->mSlot));
	CaptureWiiSaveSlot(manager, newGame);
	ChangeNandGameDir(&manager->mFile, manager->mHomeDir);
	int hasBanner  = CheckWiiSaveBanner(manager, true);
	s32 result     = OpenNandGameFile(&manager->mFile, manager->mSlotPath, 2, gWiiSaveNandWorkBuffer, 0x4000);
	if (result != 0) {
		u32 answer;
		u32 blocks = 1;
		u32 files  = 1;
		if (!hasBanner) {
			blocks = 5;
			files  = 2;
		}
		result = CheckNandGameSpace(&manager->mFile, blocks, files, &answer);
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
	manager->mSlot.mHeader.mChecksum = GetWiiSaveSlotChecksum(manager->mSlot) ^ 0x50494b53;
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
	MakeWiiSaveSlotPath(manager, slot);
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
	result             = (storedChecksum ^ 0x50494b53) == GetWiiSaveSlotChecksum(manager->mSlot);
	if (result == 0)
		return NAND_RESULT_UNKNOWN;
	if (manager->mSlot.mHeader.mStages[0] < 0)
		manager->mSlot.mHeader.mStages[0] = manager->mSlot.mHeader.mStages[1] = GetDefaultStageIndex();
	result = CloseNandGameFile(&manager->mFile);
	if (result != 0)
		return result;
	return 0;
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
	MakeWiiSaveSlotPath(manager, job->mSlot);
	DeleteNandGameFile(&manager->mFile, manager->mSlotPath);
	return NULL;
}

// CaptureWiiSaveSlot (0x800AA2F8) and the functions at 0x800AA65C, 0x800AA6A4 and 0x800AAB18 come here

#include "__dvd.h"

#include <cstring>

enum {
	FST_READ_DISK_ID,
	FST_READ_BOOT_BLOCK,
	FST_READ_TABLE,
};

#define DVD_DMA_ALIGNMENT 32

static u32 loadState;
static DVDBB2* bootBlock;
static DVDDiskID* diskID;
static u8 bootBlockBuffer[sizeof(DVDBB2) + DVD_DMA_ALIGNMENT - 1];

/**
 * @note Address: 0x80170998
 * @note Size: 0xD8
 */
static void FstLoadCallback(s32 result, DVDCommandBlock* block)
{
	if (result > 0) {
		switch (loadState) {
		case FST_READ_DISK_ID:
			loadState = FST_READ_BOOT_BLOCK;
			DVDReadAbsAsyncForBS(block, bootBlock, sizeof(*bootBlock), DVD_BOOT_BLOCK_OFFSET, FstLoadCallback);
			break;
		case FST_READ_BOOT_BLOCK:
			loadState = FST_READ_TABLE;
			DVDReadAbsAsyncForBS(block, bootBlock->FSTAddress, OSRoundUp32B(bootBlock->FSTLength), bootBlock->FSTPosition,
			                     FstLoadCallback);
			break;
		}
	} else if (result == DVD_RESULT_FATAL_ERROR) {
		return;
	} else if (result == DVD_RESULT_COVER_CLOSED) {
		loadState = FST_READ_DISK_ID;
		DVDReset();
		DVDReadDiskID(block, diskID, FstLoadCallback);
	}
}

/**
 * @note Address: 0x80170A70
 * @note Size: 0x168
 */
void __fstLoad(void)
{
	static DVDCommandBlock block;
	u8 diskIDBuffer[sizeof(DVDDiskID) + DVD_DMA_ALIGNMENT - 1];
	OSBootInfo* bootInfo;
	const char* streaming;
	s32 driveStatus;
	void* arenaHi;

	// The boot loader queries the arena before starting the asynchronous reads.
	arenaHi = OSGetArenaHi();
	diskID = (DVDDiskID*)OSRoundUp32B(diskIDBuffer);
	bootBlock = (DVDBB2*)OSRoundUp32B(bootBlockBuffer);
	DVDReset();
	DVDReadDiskID(&block, diskID, FstLoadCallback);
	do {
		driveStatus = DVDGetDriveStatus();
	} while (driveStatus != DVD_STATE_END);

	bootInfo = OSPhysicalToCached(0);
	bootInfo->FSTLocation = bootBlock->FSTAddress;
	bootInfo->FSTMaxLength = bootBlock->FSTMaxLength;
	memcpy(&bootInfo->DVDDiskID, diskID, sizeof(*diskID));

	OSReport("\n");
	OSReport("  Game Name ... %c%c%c%c\n", bootInfo->DVDDiskID.gameName[0], bootInfo->DVDDiskID.gameName[1],
	         bootInfo->DVDDiskID.gameName[2], bootInfo->DVDDiskID.gameName[3]);
	OSReport("  Company ..... %c%c\n", bootInfo->DVDDiskID.company[0], bootInfo->DVDDiskID.company[1]);
	OSReport("  Disk # ...... %d\n", bootInfo->DVDDiskID.diskNumber);
	OSReport("  Game ver .... %d\n", bootInfo->DVDDiskID.gameVersion);
	if (bootInfo->DVDDiskID.streaming == 0) {
		streaming = "OFF";
	} else {
		streaming = "ON";
	}
	OSReport("  Streaming ... %s\n", streaming);
	OSReport("\n");
	OSSetArenaHi(bootBlock->FSTAddress);
}

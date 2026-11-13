#include "revolution/nand.h"
#include "revolution/fs.h"
#include "revolution/nand_internal.h"

static const char* USER_DIR_LIST[] = { "/meta",
	                                   "/ticket",
	                                   "/title/00010000",
	                                   "/title/00010001",
	                                   "/title/00010003",
	                                   "/title/00010004",
	                                   "/title/00010005",
	                                   "/title/00010006",
	                                   "/title/00010007",
	                                   "/shared2/title",
	                                   NULL,
	                                   NULL };

static s32 nandCalcUsage(s32* blockCountOut, s32* fileCountOut, const char** dirList)
{
	s32 result = -117;

	*blockCountOut = 0;
	*fileCountOut  = 0;

	for (; *dirList != NULL; dirList++) {
		s32 blockCount = 0;
		s32 fileCount  = 0;

		result = ISFS_GetUsage(*dirList, &blockCount, &fileCount);
		if (result == IPC_RESULT_OK) {
			*blockCountOut += blockCount;
			*fileCountOut += fileCount;
		} else if (result == IPC_RESULT_NOEXISTS) {
			result = IPC_RESULT_OK;
		} else {
			break;
		}
	}

	return result;
}

static s32 nandCalcUserUsage(s32* blockCountOut, s32* fileCountOut)
{
	return nandCalcUsage(blockCountOut, fileCountOut, USER_DIR_LIST);
}

static u32 nandCheck(u32 neededBlocks, u32 neededFiles, u32 homeDirBlocks, u32 homeDirFiles, u32 userBlocks, u32 userFiles)
{
	u32 answer = 0;

	if (homeDirBlocks + neededBlocks > 0x400) {
		answer |= NAND_CHECK_TOO_MANY_APP_BLOCKS;
	}

	if (homeDirFiles + neededFiles > 0x21) {
		answer |= NAND_CHECK_TOO_MANY_APP_FILES;
	}

	if (userBlocks + neededBlocks > 0x4400) {
		answer |= NAND_CHECK_TOO_MANY_USER_BLOCKS;
	}

	if (userFiles + neededFiles > 0xFA0) {
		answer |= NAND_CHECK_TOO_MANY_USER_FILES;
	}

	return answer;
}

s32 NANDCheck(u32 neededBlocks, u32 neededFiles, u32* answer)
{
	s32 result;

	s32 homeDirBlocks = -1;
	s32 homeDirFiles  = -1;
	s32 userBlocks    = -1;
	s32 userFiles     = -1;

	if (!nandIsInitialized()) {
		return NAND_RESULT_FATAL_ERROR;
	}

	result = ISFS_GetUsage(nandGetHomeDir(), &homeDirBlocks, &homeDirFiles);
	if (result != IPC_RESULT_OK) {
		return nandConvertErrorCode(result);
	}

	result = nandCalcUserUsage(&userBlocks, &userFiles);
	if (result != IPC_RESULT_OK) {
		return nandConvertErrorCode(result);
	}

	*answer = nandCheck(neededBlocks, neededFiles, homeDirBlocks, homeDirFiles, userBlocks, userFiles);

	return NAND_RESULT_OK;
}

static u32 remainingQuota(u32 limit, u32 usage)
{
	return usage > limit ? 0 : limit - usage;
}

s32 fn_801B6050(u32* freeBlocks, u32* freeFiles)
{
	u32 userFreeBlocks, userFreeFiles, homeFreeBlocks, homeFreeFiles;
	s32 userBlocks = -1;
	s32 userFiles  = -1;
	s32 homeBlocks = -1;
	s32 homeFiles  = -1;
	s32 result     = ISFS_GetUsage(nandGetHomeDir(), &homeBlocks, &homeFiles);
	if (result != IPC_RESULT_OK)
		return nandConvertErrorCode(result);

	// The title's data directory itself is included in the IOS inode count.
	homeFiles--;
	result = nandCalcUserUsage(&userBlocks, &userFiles);
	if (result != IPC_RESULT_OK)
		return nandConvertErrorCode(result);

	userFreeBlocks = remainingQuota(0x4400, userBlocks);
	userFreeFiles  = remainingQuota(0xfa0, userFiles);
	homeFreeBlocks = remainingQuota(0x400, homeBlocks);
	homeFreeFiles  = remainingQuota(0x20, homeFiles);
	*freeBlocks    = userFreeBlocks < homeFreeBlocks ? userFreeBlocks : homeFreeBlocks;
	*freeFiles     = userFreeFiles < homeFreeFiles ? userFreeFiles : homeFreeFiles;
	return NAND_RESULT_OK;
}

#include "revolution/nand.h"
#include "revolution/fs.h"
#include "revolution/nand_internal.h"

s32 NANDSecretGetUsage(const char* path, s32* blocks, s32* inodes)
{
	if (!nandIsInitialized()) {
		return NAND_RESULT_FATAL_ERROR;
	} else {
		char absolute[64] = { 0 };
		nandGenerateAbsPath(absolute, path);
		return nandConvertErrorCode(ISFS_GetUsage(absolute, blocks, inodes));
	}
}

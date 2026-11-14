#include "game/wii/nandfile.h"

extern "C" s32 InitializeNandGameFile(WiiGameSaveFile* file)
{
	file->mResult = NAND_RESULT_OK;
	file->mSafe   = false;
	return NAND_RESULT_OK;
}

extern "C" s32 CreateNandGameFile(WiiGameSaveFile* file, const char* path, u8 permission, u8 attribute)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDCreate(path, permission, attribute);
		switch (file->mResult) {
		case NAND_RESULT_BUSY:
		case NAND_RESULT_ALLOC_FAILED:
			continue;
		}
		break;
	}
	return file->mResult;
}

extern "C" s32 ReadNandGameFile(WiiGameSaveFile* file, void* buffer, u32 length)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDRead(&file->mFileInfo, buffer, length);
		if (file->mResult == NAND_RESULT_BUSY || file->mResult == NAND_RESULT_ALLOC_FAILED)
			continue;
		if ((file->mResult == NAND_RESULT_AUTHENTICATION || file->mResult == NAND_RESULT_ECC_CRIT) && file->mSafe)
			CancelNandGameFile(file);
		break;
	}
	return file->mResult;
}

extern "C" s32 WriteNandGameFile(WiiGameSaveFile* file, const void* buffer, u32 length)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDWrite(&file->mFileInfo, buffer, length);
		if (file->mResult == NAND_RESULT_BUSY || file->mResult == NAND_RESULT_ALLOC_FAILED)
			continue;
		if ((file->mResult == NAND_RESULT_AUTHENTICATION || file->mResult == NAND_RESULT_ECC_CRIT) && file->mSafe)
			CancelNandGameFile(file);
		break;
	}
	return file->mResult;
}

extern "C" s32 GetNandGameFileLength(WiiGameSaveFile* file, u32* length)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDGetLength(&file->mFileInfo, length);
		switch (file->mResult) {
		case NAND_RESULT_BUSY:
		case NAND_RESULT_ALLOC_FAILED:
			continue;
		}
		break;
	}
	return file->mResult;
}

extern "C" s32 OpenNandGameFile(WiiGameSaveFile* file, const char* path, u8 access, void* buffer, u32 length)
{
	file->mSafe = true;
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDSimpleSafeOpen(path, &file->mFileInfo, access, buffer, length);
		switch (file->mResult) {
		case NAND_RESULT_BUSY:
		case NAND_RESULT_ALLOC_FAILED:
			continue;
		}
		break;
	}
	return file->mResult;
}

extern "C" s32 CloseNandGameFile(WiiGameSaveFile* file)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDSimpleSafeClose(&file->mFileInfo);
		switch (file->mResult) {
		case NAND_RESULT_BUSY:
		case NAND_RESULT_ALLOC_FAILED:
			continue;
		}
		break;
	}
	return file->mResult;
}

extern "C" s32 CancelNandGameFile(WiiGameSaveFile* file)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDSimpleSafeCancel(&file->mFileInfo);
		switch (file->mResult) {
		case NAND_RESULT_BUSY:
		case NAND_RESULT_ALLOC_FAILED:
			continue;
		}
		break;
	}
	return file->mResult;
}

extern "C" s32 ChangeNandGameDir(WiiGameSaveFile* file, const char* path)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDChangeDir(path);
		switch (file->mResult) {
		case NAND_RESULT_BUSY:
		case NAND_RESULT_ALLOC_FAILED:
			continue;
		}
		break;
	}
	return file->mResult;
}

extern "C" s32 GetNandGameHomeDir(WiiGameSaveFile* file, char* path)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDGetHomeDir(path);
		switch (file->mResult) {
		case NAND_RESULT_BUSY:
		case NAND_RESULT_ALLOC_FAILED:
			continue;
		}
		break;
	}
	return file->mResult;
}

extern "C" s32 DeleteNandGameFile(WiiGameSaveFile* file, const char* path)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDDelete(path);
		switch (file->mResult) {
		case NAND_RESULT_BUSY:
		case NAND_RESULT_ALLOC_FAILED:
			continue;
		}
		break;
	}
	return file->mResult;
}

extern "C" s32 GetNandGameFreeSpace(WiiGameSaveFile* file, u32* blocks, u32* inodes)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = fn_801B6050(blocks, inodes);
		switch (file->mResult) {
		case NAND_RESULT_BUSY:
		case NAND_RESULT_ALLOC_FAILED:
			continue;
		}
		break;
	}
	return file->mResult;
}

extern "C" s32 CheckNandGameSpace(WiiGameSaveFile* file, u32 blocks, u32 inodes, u32* answer)
{
	for (int attempt = 0; attempt < 3; ++attempt) {
		file->mResult = NANDCheck(blocks, inodes, answer);
		switch (file->mResult) {
		case NAND_RESULT_BUSY:
		case NAND_RESULT_ALLOC_FAILED:
			continue;
		}
		break;
	}
	return file->mResult;
}

extern "C" const char* GetNandGameResultName(WiiGameSaveFile*, s32 result)
{
	switch (result) {
	case NAND_RESULT_OK:
		return "NAND_RESULT_OK";
	case NAND_RESULT_ACCESS:
		return "NAND_RESULT_ACCESS";
	case NAND_RESULT_ALLOC_FAILED:
		return "NAND_RESULT_ALLOC_FAILED";
	case NAND_RESULT_BUSY:
		return "NAND_RESULT_BUSY";
	case NAND_RESULT_CORRUPT:
		return "NAND_RESULT_CORRUPT";
	case NAND_RESULT_ECC_CRIT:
		return "NAND_RESULT_ECC_CRIT";
	case NAND_RESULT_EXISTS:
		return "NAND_RESULT_EXISTS";
	case NAND_RESULT_INVALID:
		return "NAND_RESULT_INVALID";
	case NAND_RESULT_MAXBLOCKS:
		return "NAND_RESULT_MAXBLOCKS";
	case NAND_RESULT_MAXFD:
		return "NAND_RESULT_MAXFD";
	case NAND_RESULT_MAXFILES:
		return "NAND_RESULT_MAXFILES";
	case NAND_RESULT_NOEXISTS:
		return "NAND_RESULT_NOEXISTS";
	case NAND_RESULT_NOTEMPTY:
		return "NAND_RESULT_NOTEMPTY";
	case NAND_RESULT_OPENFD:
		return "NAND_RESULT_OPENFD";
	case NAND_RESULT_AUTHENTICATION:
		return "NAND_RESULT_AUTHENTICATION";
	case NAND_RESULT_UNKNOWN:
		return "NAND_RESULT_UNKNOWN";
	case NAND_RESULT_FATAL_ERROR:
		return "NAND_RESULT_FATAL_ERROR";
	default:
		return "Unknown";
	}
}

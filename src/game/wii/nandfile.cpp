#include "game/wii/nandfile.h"

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

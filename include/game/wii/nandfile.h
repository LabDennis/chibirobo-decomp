#ifndef GAME_WII_NANDFILE_H
#define GAME_WII_NANDFILE_H

#include "revolution/nand.h"

struct WiiGameSaveFile {
	NANDFileInfo mFileInfo;
	s32 mResult;
	bool mSafe;
};
STATIC_ASSERT(sizeof(WiiGameSaveFile) == 0x94);

extern "C" {
const char* GetNandGameResultName(WiiGameSaveFile* file, s32 result);
s32 InitializeNandGameFile(WiiGameSaveFile* file);
s32 CreateNandGameFile(WiiGameSaveFile* file, const char* path, u8 permission, u8 attribute);
s32 ChangeNandGameDir(WiiGameSaveFile* file, const char* path);
s32 GetNandGameHomeDir(WiiGameSaveFile* file, char* path);
s32 DeleteNandGameFile(WiiGameSaveFile* file, const char* path);
s32 GetNandGameFreeSpace(WiiGameSaveFile* file, u32* blocks, u32* inodes);
s32 CheckNandGameSpace(WiiGameSaveFile* file, u32 blocks, u32 inodes, u32* answer);
s32 ReadNandGameFile(WiiGameSaveFile* file, void* buffer, u32 length);
s32 WriteNandGameFile(WiiGameSaveFile* file, const void* buffer, u32 length);
s32 GetNandGameFileLength(WiiGameSaveFile* file, u32* length);
s32 OpenNandGameFile(WiiGameSaveFile* file, const char* path, u8 access, void* buffer, u32 length);
s32 CloseNandGameFile(WiiGameSaveFile* file);
s32 CancelNandGameFile(WiiGameSaveFile* file);
}

#endif

#ifndef REVOLUTION_NAND_H
#define REVOLUTION_NAND_H

#include "types.h"

enum NANDResult {
	NAND_RESULT_FATAL_ERROR    = -128,
	NAND_RESULT_UNKNOWN        = -64,
	NAND_RESULT_AUTHENTICATION = -15,
	NAND_RESULT_OPENFD         = -14,
	NAND_RESULT_NOTEMPTY       = -13,
	NAND_RESULT_NOEXISTS       = -12,
	NAND_RESULT_MAXFILES       = -11,
	NAND_RESULT_MAXFD          = -10,
	NAND_RESULT_MAXBLOCKS      = -9,
	NAND_RESULT_INVALID        = -8,
	NAND_RESULT_EXISTS         = -6,
	NAND_RESULT_ECC_CRIT       = -5,
	NAND_RESULT_CORRUPT        = -4,
	NAND_RESULT_BUSY           = -3,
	NAND_RESULT_ALLOC_FAILED   = -2,
	NAND_RESULT_ACCESS         = -1,
	NAND_RESULT_OK             = 0,
};

enum {
	NAND_CHECK_TOO_MANY_APP_BLOCKS  = 1,
	NAND_CHECK_TOO_MANY_APP_FILES   = 2,
	NAND_CHECK_TOO_MANY_USER_BLOCKS = 4,
	NAND_CHECK_TOO_MANY_USER_FILES  = 8,
};

typedef struct NANDFileInfo {
	s32 fd;
	s32 tempFd;
	char openPath[64];
	char tempPath[64];
	u8 access;
	u8 stage;
	u8 mark;
} NANDFileInfo;
STATIC_ASSERT(sizeof(NANDFileInfo) == 0x8c);

typedef struct NANDBanner {
	u32 signature;
	u32 flag;
	u16 iconSpeed;
	u8 reserved[22];
	u16 comment[2][32];
	u8 bannerTexture[192 * 64 * 2];
	u8 iconTexture[8][48 * 48 * 2];
} NANDBanner;
STATIC_ASSERT(sizeof(NANDBanner) == 0xf0a0);

BEGIN_SCOPE_EXTERN_C
void NANDInitBanner(NANDBanner* banner, u32 flags, const u16* title, const u16* subtitle);
s32 NANDCreate(const char* path, u8 permission, u8 attribute);
s32 NANDChangeDir(const char* path);
s32 NANDGetHomeDir(char* path);
s32 NANDDelete(const char* path);
s32 NANDCheck(u32 blocks, u32 inodes, u32* answer);
s32 fn_801B6050(u32* freeBlocks, u32* freeFiles);
s32 NANDSecretGetUsage(const char* path, s32* blocks, s32* inodes);
s32 NANDRead(NANDFileInfo* file, void* buffer, u32 length);
s32 NANDWrite(NANDFileInfo* file, const void* buffer, u32 length);
s32 NANDGetLength(NANDFileInfo* file, u32* length);
s32 NANDSimpleSafeOpen(const char* path, NANDFileInfo* file, u8 access, void* buffer, u32 length);
s32 NANDSimpleSafeClose(NANDFileInfo* file);
s32 NANDSimpleSafeCancel(NANDFileInfo* file);
END_SCOPE_EXTERN_C

#endif

#ifndef REVOLUTION_NAND_H
#define REVOLUTION_NAND_H

#include "types.h"

enum NANDResult {
	NAND_RESULT_AUTHENTICATION = -15,
	NAND_RESULT_ECC_CRIT       = -5,
	NAND_RESULT_BUSY           = -3,
	NAND_RESULT_ALLOC_FAILED   = -2,
	NAND_RESULT_OK             = 0
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

BEGIN_SCOPE_EXTERN_C
s32 NANDRead(NANDFileInfo* file, void* buffer, u32 length);
s32 NANDWrite(NANDFileInfo* file, const void* buffer, u32 length);
s32 NANDGetLength(NANDFileInfo* file, u32* length);
s32 NANDSimpleSafeOpen(const char* path, NANDFileInfo* file, u8 access, void* buffer, u32 length);
s32 NANDSimpleSafeClose(NANDFileInfo* file);
s32 NANDSimpleSafeCancel(NANDFileInfo* file);
END_SCOPE_EXTERN_C

#endif

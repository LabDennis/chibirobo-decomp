#ifndef REVOLUTION_FS_H
#define REVOLUTION_FS_H
#include "types.h"
enum {
	IPC_RESULT_NOEXISTS = -106,
	IPC_RESULT_OK       = 0,
};
BEGIN_SCOPE_EXTERN_C
s32 ISFS_GetUsage(const char* path, s32* blockCount, s32* fileCount);
END_SCOPE_EXTERN_C
#endif

#ifndef REVOLUTION_FS_H
#define REVOLUTION_FS_H
#include "types.h"

#include "revolution/ipc.h"

BEGIN_SCOPE_EXTERN_C

#define FS_MAX_PATH     64
#define FS_MAX_DIR_PATH 12

enum {
	IPC_RESULT_NOEXISTS = -106,
	IPC_RESULT_OK       = 0,
};

#define ISFS_ERROR_OK             0
#define ISFS_ERROR_INVALID        -101
#define ISFS_ERROR_ACCESS         -102
#define ISFS_ERROR_CORRUPT        -103
#define ISFS_ERROR_NOTREADY       -104
#define ISFS_ERROR_EXISTS         -105
#define ISFS_ERROR_NOEXISTS       -106
#define ISFS_ERROR_MAXFILES       -107
#define ISFS_ERROR_MAXBLOCKS      -108
#define ISFS_ERROR_MAXFD          -109
#define ISFS_ERROR_MAXDEPTH       -110
#define ISFS_ERROR_OPENFD         -111
#define ISFS_ERROR_BADBLOCK       -112
#define ISFS_ERROR_ECC            -113
#define ISFS_ERROR_ECC_CRIT       -114
#define ISFS_ERROR_NOTEMPTY       -115
#define ISFS_ERROR_AUTHENTICATION -116
#define ISFS_ERROR_UNKNOWN        -117
#define ISFS_ERROR_BUSY           -118
#define ISFS_ERROR_SHUTDOWN       -119

enum {
	ISFS_ACCESS_NONE  = 0,
	ISFS_ACCESS_READ  = (1 << 0),
	ISFS_ACCESS_WRITE = (1 << 1),
	ISFS_ACCESS_RW    = ISFS_ACCESS_READ | ISFS_ACCESS_WRITE
};

enum {
	ISFS_SEEK_BEG,
	ISFS_SEEK_CUR,
	ISFS_SEEK_END,
};

typedef s32 ISFSError;
typedef void (*ISFSCallback)(ISFSError result, void* ctxt);

typedef struct {
	u32 blockSize;
	u32 freeBlocks;
	u32 occupiedBlocks;
	u32 badBlocks;
	u32 reservedBlocks;
	u32 freeInodes;
	u32 occupiedInodes;
} ISFSStats;

typedef struct {
	u32 size;
	u32 offset;
} ISFSFileStats;

typedef struct {
	IOSUid ownerId;
	IOSGid groupId;
	u8 path[64];
	u8 ownerAccess;
	u8 groupAccess;
	u8 othersAccess;
	u8 attr;
} ISFSPathAttrArgs;

typedef struct {
	u8 path1[64];
	u8 path2[64];
} ISFSPathsArgs;

ISFSError ISFS_OpenLib(void);
s32 ISFS_CreateDir(const u8* dname, u32 dirAttr, u32 ownerAcc, u32 groupAcc, u32 othersAcc);
s32 ISFS_CreateDirAsync(const u8* dname, u32 dirAttr, u32 ownerAcc, u32 groupAcc, u32 othersAcc, ISFSCallback cb, void* fsCtxt);
s32 ISFS_ReadDir(const u8* dname, u8* nameList, u32* num);
s32 ISFS_ReadDirAsync(const u8* dname, u8* nameList, u32* num, ISFSCallback cb, void* fsCtxt);
s32 ISFS_GetAttr(const u8* name, IOSUid* ownerId, IOSGid* groupId, u32* attr, u32* ownerAcc, u32* groupAcc, u32* othersAcc);
s32 ISFS_GetAttrAsync(const u8* name, IOSUid* ownerId, IOSGid* groupId, u32* attr, u32* ownerAcc, u32* groupAcc, u32* othersAcc,
                      ISFSCallback cb, void* fsCtxt);
s32 ISFS_Delete(const u8* name);
s32 ISFS_DeleteAsync(const u8* name, ISFSCallback cb, void* fsCtxt);
s32 ISFS_Rename(const u8* oldName, const u8* newName);
s32 ISFS_RenameAsync(const u8* oldName, const u8* newName, ISFSCallback cb, void* fsCtxt);
s32 ISFS_GetUsage(const u8* dname, s32* nblocks, s32* ninodes);
s32 ISFS_CreateFile(const u8* fname, u32 fileAttr, u32 ownerAcc, u32 groupAcc, u32 othersAcc);
s32 ISFS_CreateFileAsync(const u8* fname, u32 fileAttr, u32 ownerAcc, u32 groupAcc, u32 othersAcc, ISFSCallback cb, void* fsCtxt);
IOSFd ISFS_Open(const u8* fname, u32 access);
IOSFd ISFS_OpenAsync(const u8* fname, u32 access, ISFSCallback cb, void* fsCtxt);
s32 ISFS_GetFileStats(IOSFd fd, ISFSFileStats* stats);
s32 ISFS_GetFileStatsAsync(IOSFd fd, ISFSFileStats* stats, ISFSCallback cb, void* fsCtxt);
s32 ISFS_Seek(IOSFd fd, s32 offset, u32 whence);
s32 ISFS_SeekAsync(IOSFd fd, s32 offset, u32 whence, ISFSCallback cb, void* fsCtxt);
s32 ISFS_Read(IOSFd fd, u8* buf, u32 size);
s32 ISFS_ReadAsync(IOSFd fd, u8* buf, u32 size, ISFSCallback cb, void* fsCtxt);
s32 ISFS_Write(IOSFd fd, const u8* buf, u32 size);
s32 ISFS_WriteAsync(IOSFd fd, const u8* buf, u32 size, ISFSCallback cb, void* fsCtxt);
s32 ISFS_Close(IOSFd fd);
s32 ISFS_CloseAsync(IOSFd fd, ISFSCallback cb, void* fsCtxt);
s32 ISFS_ShutdownAsync(ISFSCallback cb, void* fsCtxt);

END_SCOPE_EXTERN_C

#endif

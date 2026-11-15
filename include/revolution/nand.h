#ifndef REVOLUTION_NAND_H
#define REVOLUTION_NAND_H

#include "types.h"

BEGIN_SCOPE_EXTERN_C

#define NAND_MAX_PATH     64
#define NAND_MAX_DIR_PATH 12
#define NAND_BLOCK_SIZE   (16 * 1024)

typedef enum NANDAccessType { NAND_ACCESS_NONE, NAND_ACCESS_READ, NAND_ACCESS_WRITE, NAND_ACCESS_RW } NANDAccessType;

enum NANDResult {
	NAND_RESULT_FATAL_ERROR    = -128,
	NAND_RESULT_UNKNOWN        = -64,
	NAND_RESULT_MAXDEPTH       = -16,
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
	NAND_SEEK_BEG,
	NAND_SEEK_CUR,
	NAND_SEEK_END,
};

enum {
	NAND_TYPE_NONE,
	NAND_TYPE_FILE,
	NAND_TYPE_DIR,
};

enum {
	NAND_PERM_MASK_BOTH  = 0,
	NAND_PERM_MASK_GROUP = 2,
	NAND_PERM_MASK_USER  = 4,
};

enum {
	NAND_PERM_USER_READ  = (NAND_ACCESS_READ << NAND_PERM_MASK_USER),
	NAND_PERM_USER_WRITE = (NAND_ACCESS_WRITE << NAND_PERM_MASK_USER),
	NAND_PERM_USER       = NAND_PERM_USER_READ | NAND_PERM_USER_WRITE,

	NAND_PERM_GROUP_READ  = (NAND_ACCESS_READ << NAND_PERM_MASK_GROUP),
	NAND_PERM_GROUP_WRITE = (NAND_ACCESS_WRITE << NAND_PERM_MASK_GROUP),
	NAND_PERM_GROUP       = NAND_PERM_GROUP_READ | NAND_PERM_GROUP_WRITE,

	NAND_PERM_BOTH_READ  = (NAND_ACCESS_READ << NAND_PERM_MASK_BOTH),
	NAND_PERM_BOTH_WRITE = (NAND_ACCESS_WRITE << NAND_PERM_MASK_BOTH),
	NAND_PERM_BOTH       = NAND_PERM_BOTH_READ | NAND_PERM_BOTH_WRITE,

	NAND_PERM_ALL_READ  = NAND_PERM_USER_READ | NAND_PERM_GROUP_READ | NAND_PERM_BOTH_READ,
	NAND_PERM_ALL_WRITE = NAND_PERM_USER_WRITE | NAND_PERM_GROUP_WRITE | NAND_PERM_BOTH_WRITE,
	NAND_PERM_ALL_RW    = NAND_PERM_ALL_READ | NAND_PERM_ALL_WRITE
};

enum {
	NAND_CHECK_TOO_MANY_APP_BLOCKS  = 1,
	NAND_CHECK_TOO_MANY_APP_FILES   = 2,
	NAND_CHECK_TOO_MANY_USER_BLOCKS = 4,
	NAND_CHECK_TOO_MANY_USER_FILES  = 8,
};

typedef struct NANDFileInfo {
	s32 fileDescriptor;
	s32 origFd;
	char origPath[NAND_MAX_PATH];
	char tmpPath[NAND_MAX_PATH];
	u8 accType;
	u8 stage;
	u8 mark;
} NANDFileInfo;
STATIC_ASSERT(sizeof(NANDFileInfo) == 0x8c);

typedef struct NANDStatus {
	u32 ownerId;
	u16 groupId;
	u8 attribute;
	u8 permission;
} NANDStatus;

typedef struct NANDCommandBlock NANDCommandBlock;
typedef void (*NANDCallback)(s32 result, NANDCommandBlock* block);

struct NANDCommandBlock {
	void* userData;
	NANDCallback callback;
	void* fileInfo;
	void* bytes;
	void* inodes;
	void* status;
	u32 ownerId;
	u16 groupId;
	u8 nextStage;
	u32 attr;
	u32 ownerAcc;
	u32 groupAcc;
	u32 othersAcc;
	u32 num;
	char absPath[NAND_MAX_PATH];
	u32* length;
	u32* pos;
	int state;
	void* copyBuf;
	u32 bufLength;
	u8* type;
	u32 uniqNo;
	u32 reqBlocks;
	u32 reqInodes;
	u32* answer;
	u32 homeBlocks;
	u32 homeInodes;
	u32 userBlocks;
	u32 userInodes;
	u32 workBlocks;
	u32 workInodes;
	const char** dir;
	BOOL simpleFlag;
};
STATIC_ASSERT(sizeof(NANDCommandBlock) == 0xbc);

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

#define NAND_BANNER_SIGNATURE 0x5749424E

s32 NANDInit(void);
void NANDInitBanner(NANDBanner* banner, u32 flags, const u16* title, const u16* subtitle);

s32 NANDCreate(const char* path, u8 perm, u8 attr);
s32 NANDPrivateCreate(const char* path, u8 perm, u8 attr);
s32 NANDPrivateCreateAsync(const char* path, u8 perm, u8 attr, NANDCallback cb, NANDCommandBlock* block);
s32 NANDDelete(const char* path);
s32 NANDPrivateDelete(const char* path);
s32 NANDPrivateDeleteAsync(const char* path, NANDCallback cb, NANDCommandBlock* block);
s32 NANDRead(NANDFileInfo* info, void* buf, u32 length);
s32 NANDReadAsync(NANDFileInfo* info, void* buf, u32 length, NANDCallback cb, NANDCommandBlock* block);
s32 NANDWrite(NANDFileInfo* info, const void* buf, u32 length);
s32 NANDWriteAsync(NANDFileInfo* info, const void* buf, u32 length, NANDCallback cb, NANDCommandBlock* block);
s32 NANDSeek(NANDFileInfo* info, s32 offset, s32 whence);
s32 NANDSeekAsync(NANDFileInfo* info, s32 offset, s32 whence, NANDCallback cb, NANDCommandBlock* block);
s32 NANDPrivateCreateDir(const char* path, u8 perm, u8 attr);
s32 NANDPrivateCreateDirAsync(const char* path, u8 perm, u8 attr, NANDCallback cb, NANDCommandBlock* block);
s32 NANDMove(const char* path, const char* destDir);
s32 NANDGetLength(NANDFileInfo* info, u32* length);
s32 NANDGetLengthAsync(NANDFileInfo* info, u32* length, NANDCallback cb, NANDCommandBlock* block);
s32 NANDGetStatus(const char* path, NANDStatus* stat);
s32 NANDPrivateGetStatus(const char* path, NANDStatus* stat);
s32 NANDPrivateGetStatusAsync(const char* path, NANDStatus* stat, NANDCallback cb, NANDCommandBlock* block);
void NANDSetUserData(NANDCommandBlock* block, void* data);
void* NANDGetUserData(NANDCommandBlock* block);

s32 NANDOpen(const char* path, NANDFileInfo* info, u8 accType);
s32 NANDPrivateOpen(const char* path, NANDFileInfo* info, u8 accType);
s32 NANDOpenAsync(const char* path, NANDFileInfo* info, u8 accType, NANDCallback cb, NANDCommandBlock* block);
s32 NANDPrivateOpenAsync(const char* path, NANDFileInfo* info, u8 accType, NANDCallback cb, NANDCommandBlock* block);
s32 NANDClose(NANDFileInfo* info);
s32 NANDCloseAsync(NANDFileInfo* info, NANDCallback cb, NANDCommandBlock* block);
s32 NANDSafeOpen(const char* path, NANDFileInfo* info, u8 accType, void* buf, u32 length);
s32 NANDSafeClose(NANDFileInfo* info);
s32 NANDPrivateSafeOpenAsync(const char* path, NANDFileInfo* info, u8 accType, void* buf, u32 length, NANDCallback cb,
                             NANDCommandBlock* block);
s32 NANDSafeCloseAsync(NANDFileInfo* info, NANDCallback cb, NANDCommandBlock* block);
s32 NANDSimpleSafeOpen(const char* path, NANDFileInfo* info, u8 accType, void* buf, u32 length);
s32 NANDSimpleSafeClose(NANDFileInfo* info);
s32 NANDSimpleSafeCancel(NANDFileInfo* info);

s32 NANDChangeDir(const char* path);
s32 NANDGetHomeDir(char* path);
s32 NANDPrivateGetTypeAsync(const char* path, u8* type, NANDCallback cb, NANDCommandBlock* block);

s32 NANDCheck(u32 blocks, u32 inodes, u32* answer);
s32 fn_801B6050(u32* freeBlocks, u32* freeFiles);
s32 NANDSecretGetUsage(const char* path, s32* blocks, s32* inodes);

END_SCOPE_EXTERN_C

#endif

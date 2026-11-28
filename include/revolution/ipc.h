#ifndef REVOLUTION_IPC_H
#define REVOLUTION_IPC_H
#include "types.h"

BEGIN_SCOPE_EXTERN_C

// result codes of the IOS calls that the DVD driver tells apart
enum {
	IPC_RESULT_ACCESS   = -1,
	IPC_RESULT_CONN_MAX = -5,
	IPC_RESULT_NOENT    = -6,
};

#define IOS_ERROR_OK              0
#define IOS_ERROR_ACCESS          -1
#define IOS_ERROR_EXISTS          -2
#define IOS_ERROR_INTR            -3
#define IOS_ERROR_INVALID         -4
#define IOS_ERROR_MAX             -5
#define IOS_ERROR_NOEXISTS        -6
#define IOS_ERROR_QEMPTY          -7
#define IOS_ERROR_QFULL           -8
#define IOS_ERROR_UNKNOWN         -9
#define IOS_ERROR_NOTREADY        -10
#define IOS_ERROR_ECC             -11
#define IOS_ERROR_ECC_CRIT        -12
#define IOS_ERROR_BADBLOCK        -13
#define IOS_ERROR_INVALID_OBJTYPE -14
#define IOS_ERROR_INVALID_RNG     -15
#define IOS_ERROR_INVALID_FLAG    -16
#define IOS_ERROR_INVALID_FORMAT  -17
#define IOS_ERROR_INVALID_VERSION -18
#define IOS_ERROR_INVALID_SIGNER  -19
#define IOS_ERROR_FAIL_CHECKVALUE -20
#define IOS_ERROR_FAIL_INTERNAL   -21
#define IOS_ERROR_FAIL_ALLOC      -22
#define IOS_ERROR_INVALID_SIZE    -23

enum {
	IPC_SEEK_BEG,
	IPC_SEEK_CUR,
	IPC_SEEK_END,
};

typedef s32 IOSError;
typedef s32 IOSFd;
typedef s32 IOSHeapId;
typedef u32 IOSUid;
typedef u16 IOSGid;
typedef u32 IOSResourceHandle;

typedef struct {
	u8* path;
	u32 flags;
	IOSUid uid;
	IOSGid gid;
} IOSResourceOpen;

typedef struct {
	u8* outPtr;
	u32 outLen;
} IOSResourceRead;

typedef struct {
	u8* inPtr;
	u32 inLen;
} IOSResourceWrite;

typedef struct {
	s32 offset;
	u32 whence;
} IOSResourceSeek;

typedef struct {
	u32 cmd;
	u8* inPtr;
	u32 inLen;
	u8* outPtr;
	u32 outLen;
} IOSResourceIoctl;

typedef struct IOSIoVector {
	void* base; // _00
	u32 length; // _04
} IOSIoVector;

typedef struct {
	u32 cmd;
	u32 readCount;
	u32 writeCount;
	IOSIoVector* vector;
} IOSResourceIoctlv;

typedef struct {
	u32 cmd;
	s32 status;
	IOSResourceHandle handle;
	union {
		IOSResourceOpen open;
		IOSResourceRead read;
		IOSResourceWrite write;
		IOSResourceSeek seek;
		IOSResourceIoctl ioctl;
		IOSResourceIoctlv ioctlv;
	} args;
} IOSResourceRequest;

typedef IOSError (*IOSIpcCb)(IOSError result, void* usrData);

// ipcMain.c
void IPCInit(void);
void IPCReInit(void);
u32 IPCReadReg(u32 regIdx);
void IPCWriteReg(u32 regIdx, u32 data);
void* IPCGetBufferHi(void);
void* IPCGetBufferLo(void);
void IPCSetBufferLo(void* lo);

// ipcclt.c
IOSError IPCCltInit(void);
IOSError IPCCltReInit(void);

IOSError IOS_OpenAsync(const char* path, u32 flags, IOSIpcCb cb, void* usrData);
IOSError IOS_Open(const char* path, u32 flags);
IOSError IOS_CloseAsync(IOSFd fd, IOSIpcCb cb, void* usrData);
IOSError IOS_Close(IOSFd fd);
IOSError IOS_ReadAsync(IOSFd fd, void* buf, u32 len, IOSIpcCb cb, void* usrData);
IOSError IOS_Read(IOSFd fd, void* buf, u32 len);
IOSError IOS_WriteAsync(IOSFd fd, void* buf, u32 len, IOSIpcCb cb, void* usrData);
IOSError IOS_Write(IOSFd fd, void* buf, u32 len);
IOSError IOS_SeekAsync(IOSFd fd, s32 offset, u32 whence, IOSIpcCb cb, void* usrData);
IOSError IOS_Seek(IOSFd fd, s32 offset, u32 whence);
IOSError IOS_IoctlAsync(IOSFd fd, s32 cmd, void* input, u32 inputLen, void* output, u32 outputLen, IOSIpcCb cb, void* usrData);
IOSError IOS_Ioctl(IOSFd fd, s32 cmd, void* input, u32 inputLen, void* output, u32 outputLen);
IOSError IOS_IoctlvAsync(IOSFd fd, s32 cmd, u32 readCount, u32 writeCount, IOSIoVector* vect, IOSIpcCb cb, void* usrData);
IOSError IOS_Ioctlv(IOSFd fd, s32 cmd, u32 readCount, u32 writeCount, IOSIoVector* vect);
IOSError IOS_IoctlvReboot(IOSFd fd, s32 cmd, u32 readCount, u32 writeCount, IOSIoVector* vect);

// memory.c
IOSHeapId iosCreateHeap(void* ptr, u32 size);
void* iosAllocAligned(IOSHeapId id, u32 size, u32 alignment);
IOSError iosFree(IOSHeapId id, void* ptr);

// ipcProfile.c
void IPCiProfInit(void);
void IPCiProfQueueReq(void* req, s32 fd);
void IPCiProfAck(void);
void IPCiProfReply(void* req, s32 fd);

END_SCOPE_EXTERN_C

#endif

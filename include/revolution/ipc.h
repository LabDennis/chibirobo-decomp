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

typedef struct IOSIoVector {
	void* base; // _00
	u32 length; // _04
} IOSIoVector;

typedef s32 (*IOSIpcCb)(s32 result, void* usrData);

s32 IPCCltInit(void);
void* IPCGetBufferLo(void);
void* IPCGetBufferHi(void);
void IPCSetBufferLo(void* lo);

s32 IOS_Open(const char* path, u32 flags);
s32 IOS_Close(s32 fd);
s32 IOS_Ioctl(s32 fd, s32 cmd, void* input, u32 inputLen, void* output, u32 outputLen);
s32 IOS_IoctlAsync(s32 fd, s32 cmd, void* input, u32 inputLen, void* output, u32 outputLen, IOSIpcCb cb, void* usrData);
s32 IOS_IoctlvAsync(s32 fd, s32 cmd, u32 readCount, u32 writeCount, IOSIoVector* vect, IOSIpcCb cb, void* usrData);

END_SCOPE_EXTERN_C

#endif

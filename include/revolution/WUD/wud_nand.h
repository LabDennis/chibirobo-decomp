/* The asynchronous part of NAND that WUD uses. Stand-in until revolution/nand.h declares it. */
#ifndef REVOLUTION_WUD_NAND_H
#define REVOLUTION_WUD_NAND_H

#include "revolution/nand.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NANDCommandBlock NANDCommandBlock;
typedef void (*NANDCallback)(s32 result, NANDCommandBlock* block);

struct NANDCommandBlock {
	u8 unk_0x00[0xBC];
};

s32 NANDInit(void);
s32 NANDOpenAsync(const char* path, NANDFileInfo* info, u8 accType, NANDCallback cb, NANDCommandBlock* block);
s32 NANDCloseAsync(NANDFileInfo* info, NANDCallback cb, NANDCommandBlock* block);
s32 NANDReadAsync(NANDFileInfo* info, void* buf, u32 length, NANDCallback cb, NANDCommandBlock* block);
s32 NANDWriteAsync(NANDFileInfo* info, const void* buf, u32 length, NANDCallback cb, NANDCommandBlock* block);
s32 NANDSeekAsync(NANDFileInfo* info, s32 offset, s32 whence, NANDCallback cb, NANDCommandBlock* block);

#ifdef __cplusplus
}
#endif

#endif // REVOLUTION_WUD_NAND_H

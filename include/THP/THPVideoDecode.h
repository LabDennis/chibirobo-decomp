#ifndef _THP_THPVIDEODECODE_H
#define _THP_THPVIDEODECODE_H

#include "Dolphin/os.h"
#include <sdk/dolphin/thp_api.h>
#include "THP/THPRead.h"
#include "types.h"

static void* VideoDecoder(void*);
static void* VideoDecoderForOnMemory(void*);
static void VideoDecode(THPReadBuffer*);

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus
BOOL CreateVideoDecodeThread(OSPriority priority, void* task);
void VideoDecodeThreadStart();
void VideoDecodeThreadCancel();
OSMessage PopFreeTextureSet();
BOOL PushFreeTextureSet(OSMessage*);
OSMessage PopDecodedTextureSet(s32 flags);
BOOL PushDecodedTextureSet(OSMessage*);

extern BOOL VideoDecodeThreadCreated;
extern OSMessageQueue FreeTextureSetQueue;
extern OSMessageQueue DecodedTextureSetQueue;
extern OSThread VideoDecodeThread;

#ifdef __cplusplus
};
#endif

#endif

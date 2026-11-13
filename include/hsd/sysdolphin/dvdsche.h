#ifndef SYSDOLPHIN_DVDSCHE_H
#define SYSDOLPHIN_DVDSCHE_H

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void SetDvdStreamingLimits(s32 max, s32 key);
s32 AcquireDvdStreamingSlot(void (*callback)(s32), s32 param);
void ReleaseDvdStreamingSlot(s32 id);
void SetDvdStreamingSlotKey(s32 id, s32 key);
void SetDvdStreamingSlotCallback(s32 id, void (*callback)(s32));
void ServiceNextDvdStreamingSlot(void);

#ifdef __cplusplus
}
#endif

#endif

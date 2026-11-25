#ifndef _dvdsche_h_
#define _dvdsche_h_

#include <dolphin/types.h>

typedef void (*DvdStreamingSlotCallback)(s32 param);

void SetDvdStreamingLimits(s32 max, s32 key);
s32 AcquireDvdStreamingSlot(DvdStreamingSlotCallback callback, s32 param);
void ReleaseDvdStreamingSlot(s32 id);
void SetDvdStreamingSlotKey(s32 id, s32 key);
void SetDvdStreamingSlotCallback(s32 id, DvdStreamingSlotCallback callback);
void ServiceNextDvdStreamingSlot(void);

#endif

#include <global.h>
#include <dolphin/os.h>
#include <sysdolphin/dvdsche.h>

#define DVDSCHE_ASSERT(line, cond)                 \
	if (!(cond)) {                                 \
		OSReport("assertion %s failed.\n", #cond); \
		OSPanic(__FILE__, line, "");               \
	}

typedef struct DVDScheSlot {
	s32 key;               // _00
	DvdStreamingSlotCallback callback; // _04
	s32 param;             // _08
} DVDScheSlot;

static DVDScheSlot sStreamingSlots[4];
static s32 sStreamingOrder[4];

enum {
	DVD_STREAM_WAITING = -1,
	DVD_STREAM_IDLE = 0,
	DVD_STREAM_ACTIVE = 1,
};

static s32 sStreamingOrderIndex;
static u32 hsd_DVDScheStreamingFlag;
static s32 sStreamingState;
static s32 sStreamingBudget;
static s32 hsd_DVDScheNStreaming;
static s32 sStreamingKeyLimit;
static s32 hsd_DVDScheNStreamingMax;

/**
 * @note Address: 0x801EB614
 * @note Size: 0x44
 */
void SetDvdStreamingLimits(s32 max, s32 key)
{
	u32 enabled = OSDisableInterrupts();
	hsd_DVDScheNStreamingMax = max;
	sStreamingKeyLimit = key;
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EB658
 * @note Size: 0x26C
 */
s32 AcquireDvdStreamingSlot(DvdStreamingSlotCallback callback, s32 param)
{
	s32 newid;
	u32 enabled;
	s32 i;
	DVDScheSlot* slot;

	newid = -1;
	if (hsd_DVDScheNStreaming >= hsd_DVDScheNStreamingMax) {
		return -1;
	}
	enabled = OSDisableInterrupts();
	hsd_DVDScheNStreaming += 1;
	for (i = 0; i < hsd_DVDScheNStreamingMax; i++) {
		if (!(hsd_DVDScheStreamingFlag & (1 << i))) {
			hsd_DVDScheStreamingFlag |= 1 << i;
			newid = i;
			slot  = &sStreamingSlots[i];
			break;
		}
	}
	DVDSCHE_ASSERT(0x42, newid != -1);
	slot->key      = 0;
	slot->callback = callback;
	slot->param    = param;
	if (sStreamingState == DVD_STREAM_ACTIVE) {
		for (i = hsd_DVDScheNStreaming; i > sStreamingOrderIndex; i--) {
			sStreamingOrder[i] = sStreamingOrder[i - 1];
		}
		sStreamingOrder[i] = newid;
		sStreamingOrderIndex += 1;
	} else {
		for (i = hsd_DVDScheNStreaming; i > 0; i--) {
			sStreamingOrder[i] = sStreamingOrder[i - 1];
		}
		sStreamingOrder[0] = newid;
		ServiceNextDvdStreamingSlot();
	}
	OSRestoreInterrupts(enabled);
	return newid;
}

// stripped helper inlined into ReleaseDvdStreamingSlot, its assert string lands before the caller's in .data
static void remove_order(s32 id)
{
	s32 i;
	s32 orderix;
	s32 n;

	orderix = -1;
	n       = --hsd_DVDScheNStreaming;
	for (i = 0; i <= n; i++) {
		if (sStreamingOrder[i] == id) {
			orderix = i;
			for (; i < n; i++) {
				sStreamingOrder[i] = sStreamingOrder[i + 1];
			}
			break;
		}
	}
	DVDSCHE_ASSERT(0x6A, orderix != -1);
	if (sStreamingOrderIndex == orderix) {
		ServiceNextDvdStreamingSlot();
	} else if (sStreamingOrderIndex > orderix) {
		sStreamingOrderIndex--;
	}
}

/**
 * @note Address: 0x801EB8C4
 * @note Size: 0x1EC
 */
void ReleaseDvdStreamingSlot(s32 id)
{
	u32 enabled;

	DVDSCHE_ASSERT(0x7A, hsd_DVDScheStreamingFlag & (1 << id));
	enabled = OSDisableInterrupts();
	hsd_DVDScheStreamingFlag &= ~(u32)(1 << id);
	DVDSCHE_ASSERT(0x80, hsd_DVDScheNStreaming);
	remove_order(id);
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EBAB0
 * @note Size: 0xE0
 */
void SetDvdStreamingSlotKey(s32 id, s32 key)
{
	u32 enabled;

	DVDSCHE_ASSERT(0x8E, id >= 0 && id < hsd_DVDScheNStreamingMax);
	DVDSCHE_ASSERT(0x90, hsd_DVDScheStreamingFlag & (1 << id));
	enabled = OSDisableInterrupts();
	sStreamingSlots[id].key = key;
	if (sStreamingState == DVD_STREAM_WAITING) {
		ServiceNextDvdStreamingSlot();
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EBB90
 * @note Size: 0xCC
 */
void SetDvdStreamingSlotCallback(s32 id, DvdStreamingSlotCallback callback)
{
	u32 enabled;

	DVDSCHE_ASSERT(0x9F, id >= 0 && id < hsd_DVDScheNStreamingMax);
	DVDSCHE_ASSERT(0xA1, hsd_DVDScheStreamingFlag & (1 << id));
	enabled = OSDisableInterrupts();
	sStreamingSlots[id].callback = callback;
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EBC5C
 * @note Size: 0x650
 */
void ServiceNextDvdStreamingSlot(void)
{
	s32 i;
	s32 found;
	DVDScheSlot* slot;

	if (sStreamingOrderIndex < hsd_DVDScheNStreaming && sStreamingBudget) {
		slot = &sStreamingSlots[sStreamingOrder[sStreamingOrderIndex]];
		if (slot->key <= sStreamingKeyLimit) {
			sStreamingState = DVD_STREAM_ACTIVE;
			slot->callback(slot->param);
			sStreamingBudget -= 1;
			return;
		}
	}

	sStreamingBudget = sStreamingKeyLimit;
	sStreamingOrderIndex++;
	if (sStreamingOrderIndex < hsd_DVDScheNStreaming) {
		ServiceNextDvdStreamingSlot();
	} else {
		sStreamingOrderIndex = 0;
		if (!hsd_DVDScheNStreaming) {
			sStreamingState = DVD_STREAM_IDLE;
		} else {
			found = 0;
			for (i = 0; i < hsd_DVDScheNStreaming; i++) {
				if (sStreamingSlots[sStreamingOrder[i]].key <= sStreamingKeyLimit) {
					found = 1;
				}
			}
			if (found) {
				ServiceNextDvdStreamingSlot();
			} else {
				sStreamingState = DVD_STREAM_WAITING;
			}
		}
	}
}

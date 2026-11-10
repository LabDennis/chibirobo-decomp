#include <global.h>
#include <dolphin/types.h>

u32 OSDisableInterrupts(void);
u32 OSRestoreInterrupts(u32 level);
void OSReport(const char*, ...);
void OSPanic(const char*, s32, const char*, ...);

#define DVDSCHE_ASSERT(line, cond)                 \
	if (!(cond)) {                                 \
		OSReport("assertion %s failed.\n", #cond); \
		OSPanic(__FILE__, line, "");               \
	}

typedef struct DVDScheSlot {
	s32 key;               // _00
	void (*callback)(s32); // _04
	s32 param;             // _08
} DVDScheSlot;

static DVDScheSlot lbl_80655F00[4];
static s32 lbl_80655F30[4];

static s32 lbl_8065BB70;
static u32 hsd_DVDScheStreamingFlag;
static s32 lbl_8065BB68;
static s32 lbl_8065BB64;
static s32 hsd_DVDScheNStreaming;
static s32 lbl_8065BB5C;
static s32 hsd_DVDScheNStreamingMax;

void fn_801EBC5C(void);

/**
 * @note Address: 0x801EB614
 * @note Size: 0x44
 */
void fn_801EB614(s32 max, s32 key)
{
	u32 enabled              = OSDisableInterrupts();
	hsd_DVDScheNStreamingMax = max;
	lbl_8065BB5C             = key;
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EB658
 * @note Size: 0x26C
 */
s32 AcquireDvdStreamingSlot(void (*callback)(s32), s32 param)
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
			slot  = &lbl_80655F00[i];
			break;
		}
	}
	DVDSCHE_ASSERT(0x42, newid != -1);
	slot->key      = 0;
	slot->callback = callback;
	slot->param    = param;
	if (lbl_8065BB68 == 1) {
		for (i = hsd_DVDScheNStreaming; i > lbl_8065BB70; i--) {
			lbl_80655F30[i] = lbl_80655F30[i - 1];
		}
		lbl_80655F30[i] = newid;
		lbl_8065BB70 += 1;
	} else {
		for (i = hsd_DVDScheNStreaming; i > 0; i--) {
			lbl_80655F30[i] = lbl_80655F30[i - 1];
		}
		lbl_80655F30[0] = newid;
		fn_801EBC5C();
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
		if (lbl_80655F30[i] == id) {
			orderix = i;
			for (; i < n; i++) {
				lbl_80655F30[i] = lbl_80655F30[i + 1];
			}
			break;
		}
	}
	DVDSCHE_ASSERT(0x6A, orderix != -1);
	if (lbl_8065BB70 == orderix) {
		fn_801EBC5C();
	} else if (lbl_8065BB70 > orderix) {
		lbl_8065BB70--;
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
	enabled              = OSDisableInterrupts();
	lbl_80655F00[id].key = key;
	if (lbl_8065BB68 == -1) {
		fn_801EBC5C();
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EBB90
 * @note Size: 0xCC
 */
void SetDvdStreamingSlotCallback(s32 id, void (*callback)(s32))
{
	u32 enabled;

	DVDSCHE_ASSERT(0x9F, id >= 0 && id < hsd_DVDScheNStreamingMax);
	DVDSCHE_ASSERT(0xA1, hsd_DVDScheStreamingFlag & (1 << id));
	enabled                   = OSDisableInterrupts();
	lbl_80655F00[id].callback = callback;
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EBC5C
 * @note Size: 0x650
 */
void fn_801EBC5C(void)
{
	s32 i;
	s32 found;
	DVDScheSlot* slot;

	if (lbl_8065BB70 < hsd_DVDScheNStreaming && lbl_8065BB64) {
		slot = &lbl_80655F00[lbl_80655F30[lbl_8065BB70]];
		if (slot->key <= lbl_8065BB5C) {
			lbl_8065BB68 = 1;
			slot->callback(slot->param);
			lbl_8065BB64 -= 1;
			return;
		}
	}

	lbl_8065BB64 = lbl_8065BB5C;
	lbl_8065BB70++;
	if (lbl_8065BB70 < hsd_DVDScheNStreaming) {
		fn_801EBC5C();
	} else {
		lbl_8065BB70 = 0;
		if (!hsd_DVDScheNStreaming) {
			lbl_8065BB68 = 0;
		} else {
			found = 0;
			for (i = 0; i < hsd_DVDScheNStreaming; i++) {
				if (lbl_80655F00[lbl_80655F30[i]].key <= lbl_8065BB5C) {
					found = 1;
				}
			}
			if (found) {
				fn_801EBC5C();
			} else {
				lbl_8065BB68 = -1;
			}
		}
	}
}

#include <global.h>

#include <sysdolphin/forward.h>

#define DEVCOMDEST_SBUF 2
#define DEVCOM_BUF_SIZE 0x4000
#define INIT_N_DEVCOMS  16

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define HSD_ASSERT(line, cond) ((cond) ? ((void)0) : __assert(__FILE__, line, #cond))

struct HSD_DevCom {
	struct HSD_DevCom* next;     // _00
	int dcReq;                   // _04
	int file;                    // _08
	u32 src;                     // _0C
	u32 dest;                    // _10
	u32 size;                    // _14
	u16 type;                    // _18
	u16 cancelflag;              // _1A
	HSD_DevComCallback callback; // _1C
	int args;                    // _20
};

typedef struct ARQRequest ARQRequest;
typedef void (*ARQCallback)(ARQRequest*);
struct ARQRequest {
	u8 _00[0x20]; // _00
};

typedef struct DVDFileInfo DVDFileInfo;
typedef void (*DVDCallback)(s32, DVDFileInfo*);
struct DVDFileInfo {
	u8 _00[0x40]; // _00
};

void __assert(const char*, unsigned long, const char*);
u32 OSDisableInterrupts(void);
void OSRestoreInterrupts(u32 level);
void* HSD_AudioMalloc(u32 size);
void ARQPostRequest(ARQRequest* request, u32 owner, u32 type, u32 priority, u32 source, u32 dest, u32 length, ARQCallback callback);
void DCInvalidateRange(void* addr, u32 nBytes);
void DCStoreRange(void* addr, u32 nBytes);
BOOL DVDFastOpen(s32 entrynum, DVDFileInfo* fileInfo);
BOOL DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset, DVDCallback callback, s32 prio);
void fn_801EBC5C(void);

void HSD_DevComARAMWakeUp(void);
void HSD_DevComDVDWakeUp(void);

static HSD_DevCom* sDevComQueueHeads[4];
static HSD_DevCom* sDevComQueueTails[4];
static ARQRequest lbl_8064DE40[2][2];
static int sDevComTransferBuffers[2][DEVCOM_BUF_SIZE / sizeof(int)] __attribute__((aligned(32)));
static DVDFileInfo sDevComDvdFileInfo;

static u8 lbl_8065BB54[2];
static HSD_DevCom* sDevComActiveRequest;
static HSD_DevCom* dvdDC;
static HSD_DevCom* lbl_8065BB44[2];
static s32 lbl_8065BB40;
static u8 lbl_8065BB3F;
static u8 lbl_8065BB3E;
static u8 lbl_8065BB3D;
static u8 lbl_8065BB3C;
static HSD_DevCom* sDevComFreeList;

static int lbl_8065B0D0 = 4;

/**
 * @note Address: 0x801EA398
 * @note Size: 0x14
 */
HSD_DevCom* HSD_DevComIsBusy(int idx)
{
	return sDevComQueueHeads[idx];
}

// plain static, compiled right here even though every caller inlines it,
// that puts the tail table second in .bss
static void HSD_DevComUnlink(HSD_DevCom* dc)
{
	HSD_DevCom* curr;
	u32 enabled = OSDisableInterrupts();
	int i       = dc->dcReq & 3;

	if (sDevComQueueHeads[i] == dc) {
		sDevComQueueHeads[i] = dc->next;
		if (sDevComQueueTails[i] == dc) {
			sDevComQueueTails[i] = 0;
		}
	} else {
		for (curr = sDevComQueueHeads[i]; curr->next != 0; curr = curr->next) {
			if (curr->next == dc) {
				curr->next = dc->next;
				if (sDevComQueueTails[i] == dc) {
					sDevComQueueTails[i] = curr;
				}
				OSRestoreInterrupts(enabled);
				return;
			}
		}
		HSD_ASSERT(0x79, 0);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EA3AC
 * @note Size: 0x7C
 */
void HSD_DevComStdCallback(ARQRequest* request)
{
	int i;

	if (request == &lbl_8064DE40[0][0]) {
		i = 0;
	} else if (request == &lbl_8064DE40[1][0]) {
		i = 1;
	} else {
		HSD_ASSERT(0xB0, 0);
	}
	lbl_8065BB3C    = 0;
	lbl_8065BB54[i] = 0;
	HSD_DevComDVDWakeUp();
	HSD_DevComARAMWakeUp();
}

static inline void HSD_DevComFree(HSD_DevCom* devcom)
{
	u32 enabled  = OSDisableInterrupts();
	devcom->next = sDevComFreeList;
	sDevComFreeList = devcom;
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EA428
 * @note Size: 0x1A4
 */
void HSD_DevComARAMCallback(ARQRequest* request)
{
	int i;
	void* buf;

	if (sDevComActiveRequest->type == 0x1A) {
		if (request == &lbl_8064DE40[0][0]) {
			i = 0;
		} else {
			i = 1;
		}
		buf = sDevComTransferBuffers[i];
	} else {
		buf = 0;
	}

	if (sDevComActiveRequest->callback != 0) {
		sDevComActiveRequest->callback(sDevComActiveRequest->dcReq, sDevComActiveRequest->args, buf, sDevComActiveRequest->cancelflag);
	}

	HSD_DevComUnlink(sDevComActiveRequest);
	HSD_DevComFree(sDevComActiveRequest);
	HSD_DevComStdCallback(request);
}

static inline int getRelayBufIdx(void)
{
	int i;
	for (i = 0; i < 2; i++) {
		if (!lbl_8065BB54[i]) {
			lbl_8065BB54[i] = 1;
			return i;
		}
	}
	return -1;
}

/**
 * @note Address: 0x801EA5CC
 * @note Size: 0x4D8
 */
void HSD_DevComARAMWakeUp(void)
{
	u32 enabled;
	int req_idx;
	int i;
	u32 xfer_size2;
	ARQCallback arq_callback;
	ARQCallback arq_callback2;

	enabled = OSDisableInterrupts();
	if (lbl_8065BB3C != 0) {
		OSRestoreInterrupts(enabled);
		return;
	}
	sDevComActiveRequest = sDevComQueueHeads[3];
	if (sDevComQueueHeads[3] != 0) {
		if (sDevComActiveRequest->cancelflag) {
			if (sDevComActiveRequest->callback != 0) {
				sDevComActiveRequest->callback(sDevComActiveRequest->dcReq, sDevComActiveRequest->args, 0, 1);
			}
			HSD_DevComUnlink(sDevComActiveRequest);
			OSRestoreInterrupts(enabled);
			HSD_DevComARAMWakeUp();
			return;
		}
		req_idx = getRelayBufIdx();
		if (req_idx >= 0) {
			if (sDevComActiveRequest->type == 3) {
				u32 xfer_size;
				if (sDevComActiveRequest->size > DEVCOM_BUF_SIZE) {
					arq_callback = HSD_DevComStdCallback;
					xfer_size    = DEVCOM_BUF_SIZE;
				} else {
					arq_callback = HSD_DevComARAMCallback;
					xfer_size    = sDevComActiveRequest->size;
				}
				for (i = 0; i < DEVCOM_BUF_SIZE / sizeof(int); i++) {
					sDevComTransferBuffers[req_idx][i] = 0;
				}
				DCStoreRange(sDevComTransferBuffers[req_idx], DEVCOM_BUF_SIZE);
				ARQPostRequest(lbl_8064DE40[req_idx], 0, 0, 1, (u32)sDevComTransferBuffers[req_idx], sDevComActiveRequest->dest, xfer_size, arq_callback);
				sDevComActiveRequest->dest += xfer_size;
				sDevComActiveRequest->size -= xfer_size;
				lbl_8065BB3C = 1;
			} else if (sDevComActiveRequest->type == 0xB) {
				DCStoreRange((void*)sDevComActiveRequest->src, sDevComActiveRequest->size);
				ARQPostRequest(lbl_8064DE40[req_idx], 0, 0, 1, sDevComActiveRequest->src, sDevComActiveRequest->dest, sDevComActiveRequest->size,
				               HSD_DevComARAMCallback);
				lbl_8065BB3C = 1;
			} else if (sDevComActiveRequest->type == 0x19) {
				DCInvalidateRange((void*)sDevComActiveRequest->dest, sDevComActiveRequest->size);
				ARQPostRequest(lbl_8064DE40[req_idx], 0, 1, 1, sDevComActiveRequest->src, sDevComActiveRequest->dest, sDevComActiveRequest->size,
				               HSD_DevComARAMCallback);
				lbl_8065BB3C = 1;
			} else if (sDevComActiveRequest->type == 0x1A) {
				DCInvalidateRange(sDevComTransferBuffers[req_idx], DEVCOM_BUF_SIZE);
				ARQPostRequest(lbl_8064DE40[req_idx], 0, 1, 1, sDevComActiveRequest->src, (u32)sDevComTransferBuffers[req_idx], sDevComActiveRequest->size,
				               HSD_DevComARAMCallback);
				lbl_8065BB3C = 1;
			} else if (sDevComActiveRequest->type == 0x1B) {
				DCInvalidateRange(sDevComTransferBuffers[req_idx], DEVCOM_BUF_SIZE);
				if (sDevComActiveRequest->size > DEVCOM_BUF_SIZE) {
					arq_callback2 = HSD_DevComStdCallback;
					xfer_size2    = DEVCOM_BUF_SIZE;
				} else {
					arq_callback2 = HSD_DevComARAMCallback;
					xfer_size2    = sDevComActiveRequest->size;
				}
				ARQPostRequest(&lbl_8064DE40[req_idx][1], 0, 1, 1, sDevComActiveRequest->src, (u32)sDevComTransferBuffers[req_idx], xfer_size2, 0);
				ARQPostRequest(&lbl_8064DE40[req_idx][0], 0, 0, 1, (u32)sDevComTransferBuffers[req_idx], sDevComActiveRequest->dest, xfer_size2,
				               arq_callback2);
				sDevComActiveRequest->src += xfer_size2;
				sDevComActiveRequest->dest += xfer_size2;
				sDevComActiveRequest->size -= xfer_size2;
				lbl_8065BB3C = 1;
			}
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EAAA4
 * @note Size: 0x78
 */
void HSD_DevComDVDStdCallback(ARQRequest* request)
{
	int i;

	if (request == &lbl_8064DE40[0][0]) {
		i = 0;
	} else if (request == &lbl_8064DE40[1][0]) {
		i = 1;
	} else {
		HSD_ASSERT(0x163, 0);
	}
	lbl_8065BB54[i] = 0;
	HSD_DevComDVDWakeUp();
	HSD_DevComARAMWakeUp();
}

/**
 * @note Address: 0x801EAB1C
 * @note Size: 0x1A0
 */
void HSD_DevComDVDARAMEndCallback(ARQRequest* request)
{
	int i;
	ARQRequest* arq = &lbl_8064DE40[0][0];

	HSD_DevComDVDStdCallback(request);

	if (request == arq) {
		i = 0;
	} else {
		i = 1;
	}

	if (lbl_8065BB44[i]->callback != 0 && lbl_8065BB40 == 0) {
		lbl_8065BB44[i]->callback(lbl_8065BB44[i]->dcReq, lbl_8065BB44[i]->args, 0, lbl_8065BB44[i]->cancelflag);
	}
	HSD_DevComUnlink(dvdDC);
	HSD_DevComFree(lbl_8065BB44[i]);
	lbl_8065BB3D    = 0;
	lbl_8065BB44[i] = 0;
	HSD_DevComDVDWakeUp();
}

/**
 * @note Address: 0x801EACBC
 * @note Size: 0x198
 */
void HSD_DevComDVDMemCallback(s32 result, DVDFileInfo* fileInfo)
{
	HSD_DevCom* dc;
	u32 enabled;

	if (result == -1) {
		lbl_8065BB40 = 1;
		return;
	}
	if (dvdDC->size > 0x80000) {
		dvdDC->src += 0x80000;
		dvdDC->dest += 0x80000;
		dvdDC->size -= 0x80000;
		lbl_8065BB3D = 0;
		HSD_DevComDVDWakeUp();
		return;
	}
	if (dvdDC->callback != 0 && lbl_8065BB40 == 0) {
		dvdDC->callback(dvdDC->dcReq, dvdDC->args, 0, dvdDC->cancelflag);
	}
	HSD_DevComUnlink(dvdDC);
	dc           = dvdDC;
	enabled      = OSDisableInterrupts();
	dc->next     = sDevComFreeList;
	sDevComFreeList = dc;
	OSRestoreInterrupts(enabled);
	lbl_8065BB3D = 0;
	HSD_DevComDVDWakeUp();
}

/**
 * @note Address: 0x801EAE54
 * @note Size: 0x2A4
 */
void HSD_DevComDVDCallback(s32 result, DVDFileInfo* fileInfo)
{
	HSD_DevCom* dc;
	u32 enabled;
	u16 type;

	if (result == -1) {
		lbl_8065BB40 = 1;
		return;
	}
	type = dvdDC->type;
	if (type == 0x22) {
		HSD_ASSERT(0x19E, dvdDC->size <= DEVCOM_BUF_SIZE);
		HSD_ASSERT(0x19F, dvdDC->callback);
		if (lbl_8065BB40 == 0) {
			dvdDC->callback(dvdDC->dcReq, dvdDC->args, sDevComTransferBuffers[lbl_8065BB3E], dvdDC->cancelflag);
		}
		HSD_DevComUnlink(dvdDC);
		dc           = dvdDC;
		enabled      = OSDisableInterrupts();
		dc->next     = sDevComFreeList;
		sDevComFreeList = dc;
		OSRestoreInterrupts(enabled);
		lbl_8065BB3D               = 0;
		lbl_8065BB54[lbl_8065BB3E] = 0;
		HSD_DevComDVDWakeUp();
		HSD_DevComARAMWakeUp();
	} else if (type == 0x23) {
		lbl_8065BB3F = lbl_8065BB3E;
		if (dvdDC->size > DEVCOM_BUF_SIZE) {
			ARQPostRequest(lbl_8064DE40[lbl_8065BB3F], 0, 0, 1, (u32)sDevComTransferBuffers[lbl_8065BB3F], dvdDC->dest, DEVCOM_BUF_SIZE,
			               HSD_DevComDVDStdCallback);
			dvdDC->src += DEVCOM_BUF_SIZE;
			dvdDC->dest += DEVCOM_BUF_SIZE;
			dvdDC->size -= DEVCOM_BUF_SIZE;
			lbl_8065BB3D = 0;
			HSD_DevComDVDWakeUp();
		} else {
			lbl_8065BB44[lbl_8065BB3F] = dvdDC;
			ARQPostRequest(lbl_8064DE40[lbl_8065BB3F], 0, 0, 1, (u32)sDevComTransferBuffers[lbl_8065BB3F], dvdDC->dest, dvdDC->size,
			               HSD_DevComDVDARAMEndCallback);
		}
	}
}

/**
 * @note Address: 0x801EB0F8
 * @note Size: 0x2AC
 */
void HSD_DevComDVDWakeUp(void)
{
	u32 enabled;
	int i;
	int buf_idx;

	if (lbl_8065BB40 != 0) {
		return;
	}
	enabled = OSDisableInterrupts();
	if (lbl_8065BB3D != 0) {
		OSRestoreInterrupts(enabled);
		return;
	}
	lbl_8065BB3D = 1;
	if (sDevComQueueHeads[0] == 0) {
		fn_801EBC5C();
	}
	lbl_8065BB3D = 0;
	for (i = 0; i < 3; i++) {
		if ((dvdDC = sDevComQueueHeads[i])) {
			if (dvdDC->cancelflag) {
				if (dvdDC->callback != 0) {
					dvdDC->callback(dvdDC->dcReq, dvdDC->args, 0, 1);
				}
				HSD_DevComUnlink(dvdDC);
				OSRestoreInterrupts(enabled);
				HSD_DevComDVDWakeUp();
				return;
			}
			DVDFastOpen(dvdDC->file, &sDevComDvdFileInfo);
			if (dvdDC->type == 0x21) {
				DVDReadAsyncPrio(&sDevComDvdFileInfo, (void*)dvdDC->dest, MIN(dvdDC->size, 0x80000), dvdDC->src, HSD_DevComDVDMemCallback, 2);
				lbl_8065BB3D = 1;
				OSRestoreInterrupts(enabled);
				return;
			}
			buf_idx = getRelayBufIdx();
			if (buf_idx >= 0) {
				lbl_8065BB3E = buf_idx;
				DVDReadAsyncPrio(&sDevComDvdFileInfo, sDevComTransferBuffers[buf_idx], MIN(dvdDC->size, DEVCOM_BUF_SIZE), dvdDC->src, HSD_DevComDVDCallback,
				                 2);
				lbl_8065BB3D = 1;
				OSRestoreInterrupts(enabled);
				return;
			}
		}
	}
	OSRestoreInterrupts(enabled);
}

static inline int HSD_DevComGetDestType(int type)
{
	return type & 7;
}

static inline void DevComLinkNext(HSD_DevCom* dc)
{
	int i;
	for (i = 1; i < INIT_N_DEVCOMS - 1; i++) {
		dc[i].next = &dc[i] + 1;
	}
	dc[i].next = 0;
}

static inline HSD_DevCom* HSD_DevComAlloc(void)
{
	HSD_DevCom* dc;
	u32 enabled = OSDisableInterrupts();

	if ((dc = sDevComFreeList)) {
		sDevComFreeList = dc->next;
		OSRestoreInterrupts(enabled);
	} else {
		dc = HSD_AudioMalloc(sizeof(HSD_DevCom) * INIT_N_DEVCOMS);
		DevComLinkNext(dc);

		sDevComFreeList = &dc[1];
		OSRestoreInterrupts(enabled);
	}
	return dc;
}

/**
 * @note Address: 0x801EB3A4
 * @note Size: 0x270
 */
int HSD_DevComRequest(int file, u32 src, u32 dest, u32 size, int type, int pri, HSD_DevComCallback cb, int args)
{
	u32 enabled;
	HSD_DevCom* dc;
	int result;

	dc = HSD_DevComAlloc();

	HSD_ASSERT(0x22B, dc);
	HSD_ASSERT(0x22C, !(HSD_DevComGetDestType(type) == DEVCOMDEST_SBUF && size > DEVCOM_BUF_SIZE));
	HSD_ASSERT(0x22D, src % 32 == 0);
	HSD_ASSERT(0x22E, dest % 32 == 0);
	HSD_ASSERT(0x22F, size % 32 == 0);
	HSD_ASSERT(0x230, size != 0);

	pri = (type & 0x38) == 0x20 ? pri : 3;

	dc->file       = file;
	dc->src        = src;
	dc->dest       = dest;
	dc->size       = size;
	dc->type       = type;
	dc->cancelflag = 0;
	dc->callback   = cb;
	dc->args       = args;

	enabled = OSDisableInterrupts();
	result = dc->dcReq = lbl_8065B0D0 + pri;
	lbl_8065B0D0 += 4;
	if (sDevComQueueTails[pri] != 0) {
		sDevComQueueTails[pri]->next = dc;
	}
	sDevComQueueTails[pri] = dc;
	dc->next          = 0;
	if (sDevComQueueHeads[pri] == 0) {
		sDevComQueueHeads[pri] = dc;
		HSD_DevComDVDWakeUp();
		HSD_DevComARAMWakeUp();
	}
	OSRestoreInterrupts(enabled);

	return result;
}

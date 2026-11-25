#include <global.h>
#include <dolphin/os.h>
#include <sdk/dolphin/ar.h>
#include <sdk/dolphin/dvd.h>
#include <sdk/dolphin/os/OSCache.h>

#include <sysdolphin/devcom.h>
#include <sysdolphin/dvdsche.h>
#include <sysdolphin/synth.h>

#define DEVCOM_DEST_MASK 7
#define DEVCOMDEST_SBUF 2
#define DEVCOM_SOURCE_MASK 0x38
#define DEVCOM_SOURCE_DVD 0x20

/* Request IDs use the low two bits for the queue index. */
#define DEVCOM_DVD_QUEUE_COUNT 3
#define DEVCOM_ARAM_QUEUE DEVCOM_DVD_QUEUE_COUNT
#define DEVCOM_QUEUE_COUNT (DEVCOM_DVD_QUEUE_COUNT + 1)

#define DEVCOM_BUFFER_COUNT 2
#define DEVCOM_BUF_SIZE 0x4000
#define INIT_N_DEVCOMS  16

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
	void* context;               // _20
};

void HSD_DevComARAMWakeUp(void);
void HSD_DevComDVDWakeUp(void);

typedef struct HSD_DevComQueue {
	HSD_DevCom* head;
	HSD_DevCom* tail;
} HSD_DevComQueue;

static HSD_DevComQueue sDevComQueues[DEVCOM_QUEUE_COUNT];
static ARQRequest sDevComArqRequests[DEVCOM_BUFFER_COUNT][2];
static u32 sDevComTransferBuffers[DEVCOM_BUFFER_COUNT][DEVCOM_BUF_SIZE / sizeof(u32)] ATTRIBUTE_ALIGN(32);
static DVDFileInfo sDevComDvdFileInfo;

static u8 sDevComBufferBusy[DEVCOM_BUFFER_COUNT];
static HSD_DevCom* sDevComActiveRequest;
static HSD_DevCom* dvdDC;
static HSD_DevCom* sDevComDvdAramRequests[DEVCOM_BUFFER_COUNT];
static s32 sDevComDvdFatalError;
static u8 sDevComDvdAramBuffer;
static u8 sDevComDvdBuffer;
static u8 sDevComDvdBusy;
static u8 sDevComAramBusy;
static HSD_DevCom* sDevComFreeList;

static int sDevComNextRequestID = DEVCOM_QUEUE_COUNT;

/**
 * @note Address: 0x801EA398
 * @note Size: 0x14
 */
HSD_DevCom* HSD_DevComIsBusy(int priority)
{
	return sDevComQueues[priority].head;
}

/* Unlink a completed or canceled request with interrupts disabled. */
static void HSD_DevComUnlink(HSD_DevCom* dc)
{
	HSD_DevCom* curr;
	BOOL enabled = OSDisableInterrupts();
	HSD_DevComQueue* queue = &sDevComQueues[dc->dcReq & (DEVCOM_QUEUE_COUNT - 1)];

	if (queue->head == dc) {
		queue->head = dc->next;
		if (queue->tail == dc) {
			queue->tail = 0;
		}
	} else {
		for (curr = queue->head; curr->next != 0; curr = curr->next) {
			if (curr->next == dc) {
				curr->next = dc->next;
				if (queue->tail == dc) {
					queue->tail = curr;
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
void HSD_DevComStdCallback(u32 requestAddress)
{
	int i;

	if (requestAddress == (u32)&sDevComArqRequests[0][0]) {
		i = 0;
	} else if (requestAddress == (u32)&sDevComArqRequests[1][0]) {
		i = 1;
	} else {
		HSD_ASSERT(0xB0, 0);
	}
	sDevComAramBusy    = 0;
	sDevComBufferBusy[i] = 0;
	HSD_DevComDVDWakeUp();
	HSD_DevComARAMWakeUp();
}

static inline void HSD_DevComFree(HSD_DevCom* devcom)
{
	BOOL enabled  = OSDisableInterrupts();
	devcom->next = sDevComFreeList;
	sDevComFreeList = devcom;
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EA428
 * @note Size: 0x1A4
 */
void HSD_DevComARAMCallback(u32 requestAddress)
{
	int i;
	void* buf;

	if (sDevComActiveRequest->type == HSD_DEVCOM_ARAM_TO_BUFFER) {
		if (requestAddress == (u32)&sDevComArqRequests[0][0]) {
			i = 0;
		} else {
			i = 1;
		}
		buf = sDevComTransferBuffers[i];
	} else {
		buf = 0;
	}

	if (sDevComActiveRequest->callback != 0) {
		sDevComActiveRequest->callback(sDevComActiveRequest->dcReq, sDevComActiveRequest->context, buf, sDevComActiveRequest->cancelflag);
	}

	HSD_DevComUnlink(sDevComActiveRequest);
	HSD_DevComFree(sDevComActiveRequest);
	HSD_DevComStdCallback(requestAddress);
}

static inline int getRelayBufIdx(void)
{
	int i;
	for (i = 0; i < DEVCOM_BUFFER_COUNT; i++) {
		if (!sDevComBufferBusy[i]) {
			sDevComBufferBusy[i] = 1;
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
	BOOL enabled;
	int req_idx;
	int i;
	u32 xfer_size2;
	ARQCallback arq_callback;
	ARQCallback arq_callback2;

	enabled = OSDisableInterrupts();
	if (sDevComAramBusy != 0) {
		OSRestoreInterrupts(enabled);
		return;
	}
	sDevComActiveRequest = sDevComQueues[DEVCOM_ARAM_QUEUE].head;
	if (sDevComQueues[DEVCOM_ARAM_QUEUE].head != 0) {
		if (sDevComActiveRequest->cancelflag) {
			if (sDevComActiveRequest->callback != 0) {
				sDevComActiveRequest->callback(sDevComActiveRequest->dcReq, sDevComActiveRequest->context, 0, 1);
			}
			HSD_DevComUnlink(sDevComActiveRequest);
			OSRestoreInterrupts(enabled);
			HSD_DevComARAMWakeUp();
			return;
		}
		req_idx = getRelayBufIdx();
		if (req_idx >= 0) {
			if (sDevComActiveRequest->type == HSD_DEVCOM_CLEAR_ARAM) {
				u32 xfer_size;
				if (sDevComActiveRequest->size > DEVCOM_BUF_SIZE) {
					arq_callback = HSD_DevComStdCallback;
					xfer_size    = DEVCOM_BUF_SIZE;
				} else {
					arq_callback = HSD_DevComARAMCallback;
					xfer_size    = sDevComActiveRequest->size;
				}
				for (i = 0; i < DEVCOM_BUF_SIZE / sizeof(u32); i++) {
					sDevComTransferBuffers[req_idx][i] = 0;
				}
				DCStoreRange(sDevComTransferBuffers[req_idx], DEVCOM_BUF_SIZE);
				ARQPostRequest(sDevComArqRequests[req_idx], 0, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH,
				               (u32)sDevComTransferBuffers[req_idx], sDevComActiveRequest->dest,
				               xfer_size, arq_callback);
				sDevComActiveRequest->dest += xfer_size;
				sDevComActiveRequest->size -= xfer_size;
				sDevComAramBusy = 1;
			} else if (sDevComActiveRequest->type == HSD_DEVCOM_MRAM_TO_ARAM) {
				DCStoreRange((void*)sDevComActiveRequest->src, sDevComActiveRequest->size);
				ARQPostRequest(sDevComArqRequests[req_idx], 0, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH,
				               sDevComActiveRequest->src, sDevComActiveRequest->dest,
				               sDevComActiveRequest->size, HSD_DevComARAMCallback);
				sDevComAramBusy = 1;
			} else if (sDevComActiveRequest->type == HSD_DEVCOM_ARAM_TO_MRAM) {
				DCInvalidateRange((void*)sDevComActiveRequest->dest, sDevComActiveRequest->size);
				ARQPostRequest(sDevComArqRequests[req_idx], 0, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH,
				               sDevComActiveRequest->src, sDevComActiveRequest->dest,
				               sDevComActiveRequest->size, HSD_DevComARAMCallback);
				sDevComAramBusy = 1;
			} else if (sDevComActiveRequest->type == HSD_DEVCOM_ARAM_TO_BUFFER) {
				DCInvalidateRange(sDevComTransferBuffers[req_idx], DEVCOM_BUF_SIZE);
				ARQPostRequest(sDevComArqRequests[req_idx], 0, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH,
				               sDevComActiveRequest->src, (u32)sDevComTransferBuffers[req_idx],
				               sDevComActiveRequest->size, HSD_DevComARAMCallback);
				sDevComAramBusy = 1;
			} else if (sDevComActiveRequest->type == HSD_DEVCOM_ARAM_TO_ARAM) {
				DCInvalidateRange(sDevComTransferBuffers[req_idx], DEVCOM_BUF_SIZE);
				if (sDevComActiveRequest->size > DEVCOM_BUF_SIZE) {
					arq_callback2 = HSD_DevComStdCallback;
					xfer_size2    = DEVCOM_BUF_SIZE;
				} else {
					arq_callback2 = HSD_DevComARAMCallback;
					xfer_size2    = sDevComActiveRequest->size;
				}
				ARQPostRequest(&sDevComArqRequests[req_idx][1], 0, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH,
				               sDevComActiveRequest->src, (u32)sDevComTransferBuffers[req_idx],
				               xfer_size2, 0);
				ARQPostRequest(&sDevComArqRequests[req_idx][0], 0, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH,
				               (u32)sDevComTransferBuffers[req_idx], sDevComActiveRequest->dest,
				               xfer_size2, arq_callback2);
				sDevComActiveRequest->src += xfer_size2;
				sDevComActiveRequest->dest += xfer_size2;
				sDevComActiveRequest->size -= xfer_size2;
				sDevComAramBusy = 1;
			}
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EAAA4
 * @note Size: 0x78
 */
void HSD_DevComDVDStdCallback(u32 requestAddress)
{
	int i;

	if (requestAddress == (u32)&sDevComArqRequests[0][0]) {
		i = 0;
	} else if (requestAddress == (u32)&sDevComArqRequests[1][0]) {
		i = 1;
	} else {
		HSD_ASSERT(0x163, 0);
	}
	sDevComBufferBusy[i] = 0;
	HSD_DevComDVDWakeUp();
	HSD_DevComARAMWakeUp();
}

/**
 * @note Address: 0x801EAB1C
 * @note Size: 0x1A0
 */
void HSD_DevComDVDARAMEndCallback(u32 requestAddress)
{
	int i;
	ARQRequest* arq = &sDevComArqRequests[0][0];

	HSD_DevComDVDStdCallback(requestAddress);

	if (requestAddress == (u32)arq) {
		i = 0;
	} else {
		i = 1;
	}

	if (sDevComDvdAramRequests[i]->callback != 0 && sDevComDvdFatalError == 0) {
		sDevComDvdAramRequests[i]->callback(sDevComDvdAramRequests[i]->dcReq, sDevComDvdAramRequests[i]->context, 0, sDevComDvdAramRequests[i]->cancelflag);
	}
	HSD_DevComUnlink(dvdDC);
	HSD_DevComFree(sDevComDvdAramRequests[i]);
	sDevComDvdBusy    = 0;
	sDevComDvdAramRequests[i] = 0;
	HSD_DevComDVDWakeUp();
}

/**
 * @note Address: 0x801EACBC
 * @note Size: 0x198
 */
void HSD_DevComDVDMemCallback(s32 result, DVDFileInfo* fileInfo)
{
	HSD_DevCom* dc;
	BOOL enabled;

	if (result == DVD_RESULT_FATAL_ERROR) {
		sDevComDvdFatalError = 1;
		return;
	}
	if (dvdDC->size > 0x80000) {
		dvdDC->src += 0x80000;
		dvdDC->dest += 0x80000;
		dvdDC->size -= 0x80000;
		sDevComDvdBusy = 0;
		HSD_DevComDVDWakeUp();
		return;
	}
	if (dvdDC->callback != 0 && sDevComDvdFatalError == 0) {
		dvdDC->callback(dvdDC->dcReq, dvdDC->context, 0, dvdDC->cancelflag);
	}
	HSD_DevComUnlink(dvdDC);
	dc           = dvdDC;
	enabled      = OSDisableInterrupts();
	dc->next     = sDevComFreeList;
	sDevComFreeList = dc;
	OSRestoreInterrupts(enabled);
	sDevComDvdBusy = 0;
	HSD_DevComDVDWakeUp();
}

/**
 * @note Address: 0x801EAE54
 * @note Size: 0x2A4
 */
void HSD_DevComDVDCallback(s32 result, DVDFileInfo* fileInfo)
{
	HSD_DevCom* dc;
	BOOL enabled;
	u16 type;

	if (result == DVD_RESULT_FATAL_ERROR) {
		sDevComDvdFatalError = 1;
		return;
	}
	type = dvdDC->type;
	if (type == HSD_DEVCOM_DVD_TO_BUFFER) {
		HSD_ASSERT(0x19E, dvdDC->size <= DEVCOM_BUF_SIZE);
		HSD_ASSERT(0x19F, dvdDC->callback);
		if (sDevComDvdFatalError == 0) {
			dvdDC->callback(dvdDC->dcReq, dvdDC->context, sDevComTransferBuffers[sDevComDvdBuffer], dvdDC->cancelflag);
		}
		HSD_DevComUnlink(dvdDC);
		dc           = dvdDC;
		enabled      = OSDisableInterrupts();
		dc->next     = sDevComFreeList;
		sDevComFreeList = dc;
		OSRestoreInterrupts(enabled);
		sDevComDvdBusy               = 0;
		sDevComBufferBusy[sDevComDvdBuffer] = 0;
		HSD_DevComDVDWakeUp();
		HSD_DevComARAMWakeUp();
	} else if (type == HSD_DEVCOM_DVD_TO_ARAM) {
		sDevComDvdAramBuffer = sDevComDvdBuffer;
		if (dvdDC->size > DEVCOM_BUF_SIZE) {
			ARQPostRequest(sDevComArqRequests[sDevComDvdAramBuffer], 0, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH,
			               (u32)sDevComTransferBuffers[sDevComDvdAramBuffer], dvdDC->dest,
			               DEVCOM_BUF_SIZE, HSD_DevComDVDStdCallback);
			dvdDC->src += DEVCOM_BUF_SIZE;
			dvdDC->dest += DEVCOM_BUF_SIZE;
			dvdDC->size -= DEVCOM_BUF_SIZE;
			sDevComDvdBusy = 0;
			HSD_DevComDVDWakeUp();
		} else {
			sDevComDvdAramRequests[sDevComDvdAramBuffer] = dvdDC;
			ARQPostRequest(sDevComArqRequests[sDevComDvdAramBuffer], 0, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH,
			               (u32)sDevComTransferBuffers[sDevComDvdAramBuffer], dvdDC->dest,
			               dvdDC->size, HSD_DevComDVDARAMEndCallback);
		}
	}
}

/**
 * @note Address: 0x801EB0F8
 * @note Size: 0x2AC
 */
void HSD_DevComDVDWakeUp(void)
{
	BOOL enabled;
	int i;
	int buf_idx;

	if (sDevComDvdFatalError != 0) {
		return;
	}
	enabled = OSDisableInterrupts();
	if (sDevComDvdBusy != 0) {
		OSRestoreInterrupts(enabled);
		return;
	}
	sDevComDvdBusy = 1;
	if (sDevComQueues[0].head == 0) {
		ServiceNextDvdStreamingSlot();
	}
	sDevComDvdBusy = 0;
	for (i = 0; i < DEVCOM_DVD_QUEUE_COUNT; i++) {
		if ((dvdDC = sDevComQueues[i].head)) {
			if (dvdDC->cancelflag) {
				if (dvdDC->callback != 0) {
					dvdDC->callback(dvdDC->dcReq, dvdDC->context, 0, 1);
				}
				HSD_DevComUnlink(dvdDC);
				OSRestoreInterrupts(enabled);
				HSD_DevComDVDWakeUp();
				return;
			}
			DVDFastOpen(dvdDC->file, &sDevComDvdFileInfo);
			if (dvdDC->type == HSD_DEVCOM_DVD_TO_MRAM) {
				DVDReadAsyncPrio(&sDevComDvdFileInfo, (void*)dvdDC->dest, MIN(dvdDC->size, 0x80000), dvdDC->src, HSD_DevComDVDMemCallback, 2);
				sDevComDvdBusy = 1;
				OSRestoreInterrupts(enabled);
				return;
			}
			buf_idx = getRelayBufIdx();
			if (buf_idx >= 0) {
				sDevComDvdBuffer = buf_idx;
				DVDReadAsyncPrio(&sDevComDvdFileInfo, sDevComTransferBuffers[buf_idx], MIN(dvdDC->size, DEVCOM_BUF_SIZE), dvdDC->src, HSD_DevComDVDCallback,
				                 2);
				sDevComDvdBusy = 1;
				OSRestoreInterrupts(enabled);
				return;
			}
		}
	}
	OSRestoreInterrupts(enabled);
}

static inline int HSD_DevComGetDestType(int type)
{
	return type & DEVCOM_DEST_MASK;
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
	BOOL enabled = OSDisableInterrupts();

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
int HSD_DevComRequest(int file, u32 src, u32 dest, u32 size, int type, int pri, HSD_DevComCallback callback, void* context)
{
	BOOL enabled;
	HSD_DevCom* dc;
	int result;
	HSD_DevComQueue* queue;

	dc = HSD_DevComAlloc();

	HSD_ASSERT(0x22B, dc);
	HSD_ASSERT(0x22C, !(HSD_DevComGetDestType(type) == DEVCOMDEST_SBUF && size > DEVCOM_BUF_SIZE));
	HSD_ASSERT(0x22D, src % 32 == 0);
	HSD_ASSERT(0x22E, dest % 32 == 0);
	HSD_ASSERT(0x22F, size % 32 == 0);
	HSD_ASSERT(0x230, size != 0);

	pri = (type & DEVCOM_SOURCE_MASK) == DEVCOM_SOURCE_DVD ? pri : DEVCOM_ARAM_QUEUE;

	dc->file       = file;
	dc->src        = src;
	dc->dest       = dest;
	dc->size       = size;
	dc->type       = type;
	dc->cancelflag = 0;
	dc->callback   = callback;
	dc->context    = context;

	enabled = OSDisableInterrupts();
	queue = &sDevComQueues[pri];
	result = dc->dcReq = sDevComNextRequestID + pri;
	sDevComNextRequestID += DEVCOM_QUEUE_COUNT;
	if (queue->tail != 0) {
		queue->tail->next = dc;
	}
	queue->tail = dc;
	dc->next = NULL;
	if (queue->head == 0) {
		queue->head = dc;
		HSD_DevComDVDWakeUp();
		HSD_DevComARAMWakeUp();
	}
	OSRestoreInterrupts(enabled);

	return result;
}

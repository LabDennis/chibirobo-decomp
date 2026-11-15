#include "types.h"
#include "Dolphin/dvd.h"

struct DVDQueue WaitingQueue[4];

/**
 * @note Address: 0x801703E4
 * @note Size: 0x38
 */
void __DVDClearWaitingQueue()
{
	u32 i;

	for (i = 0; i < 4; i++) {
		DVDCommandBlock* q = (DVDCommandBlock*)&WaitingQueue[i];
		q->next            = q;
		q->prev            = q;
	}
}

/**
 * @note Address: 0x8017041C
 * @note Size: 0x68
 */
BOOL __DVDPushWaitingQueue(int idx, struct DVDQueue* newTail)
{
	BOOL intrEnabled = OSDisableInterrupts();

	struct DVDQueue* waitingQueue = &WaitingQueue[idx];

	waitingQueue->mTail->mHead = newTail;
	newTail->mTail             = waitingQueue->mTail;
	newTail->mHead             = waitingQueue;
	waitingQueue->mTail        = newTail;

	OSRestoreInterrupts(intrEnabled);
	return TRUE;
}

/**
 * @note Address: 0x80170484
 * @note Size: 0xA0
 */
struct DVDQueue* __DVDPopWaitingQueue()
{
	BOOL intrEnabled = OSDisableInterrupts();
	int i;

	for (i = 0; i < 4; i++) {
		if (WaitingQueue[i].mHead != &WaitingQueue[i]) {
			struct DVDQueue* tempQueue;
			struct DVDQueue* outQueue;

			OSRestoreInterrupts(intrEnabled);

			intrEnabled            = OSDisableInterrupts();
			tempQueue              = &WaitingQueue[i];
			outQueue               = tempQueue->mHead;
			tempQueue->mHead       = outQueue->mHead;
			outQueue->mHead->mTail = tempQueue;
			OSRestoreInterrupts(intrEnabled);

			outQueue->mHead = nullptr;
			outQueue->mTail = nullptr;
			return outQueue;
		}
	}
	OSRestoreInterrupts(intrEnabled);
	return NULL;
}

/**
 * @note Address: 0x80170524
 * @note Size: 0x58
 */
BOOL __DVDCheckWaitingQueue()
{
	BOOL intrEnabled = OSDisableInterrupts();
	int i;

	for (i = 0; i < 4; i++) {
		if (WaitingQueue[i].mHead != &WaitingQueue[i]) {
			OSRestoreInterrupts(intrEnabled);
			return TRUE;
		}
	}
	OSRestoreInterrupts(intrEnabled);
	return FALSE;
}

/**
 * @note Address: 0x80195824
 * @note Size: 0x60
 */
struct DVDQueue* __DVDGetNextWaitingQueue()
{
	BOOL intrEnabled = OSDisableInterrupts();
	int i;

	for (i = 0; i < 4; i++) {
		struct DVDQueue* queue = WaitingQueue[i].mHead;

		if (queue != &WaitingQueue[i]) {
			OSRestoreInterrupts(intrEnabled);
			return queue;
		}
	}
	OSRestoreInterrupts(intrEnabled);
	return NULL;
}

/**
 * @note Address: 0x8017057C
 * @note Size: 0x60
 */
BOOL __DVDDequeueWaitingQueue(struct DVDQueue* queue)
{
	BOOL intrEnabled      = OSDisableInterrupts();
	struct DVDQueue* tail = queue->mTail;
	struct DVDQueue* head = queue->mHead;

	if (tail == nullptr || head == nullptr) {
		OSRestoreInterrupts(intrEnabled);
		return FALSE;
	}
	tail->mHead = head;
	head->mTail = tail;
	OSRestoreInterrupts(intrEnabled);
	return TRUE;
}

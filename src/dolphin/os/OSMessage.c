#include "Dolphin/os.h"

/**
 * @note Address: 0x801680A8
 * @note Size: 0x60
 */
void OSInitMessageQueue(OSMessageQueue* queue, OSMessage* msgArray, s32 msgCount)
{
	OSInitThreadQueue(&queue->queueSend);
	OSInitThreadQueue(&queue->queueReceive);
	queue->msgArray   = msgArray;
	queue->msgCount   = msgCount;
	queue->firstIndex = 0;
	queue->usedCount  = 0;
}

/**
 * Appends a message. OS_MESSAGE_BLOCK waits for space in a full queue.
 *
 * @note Address: 0x80168108
 * @note Size: 0xC8
 */
BOOL OSSendMessage(OSMessageQueue* queue, OSMessage msg, s32 flags)
{
	s32 index;
	BOOL enabled;

	enabled = OSDisableInterrupts();

	while (queue->msgCount <= queue->usedCount) {
		if (!(flags & OS_MESSAGE_BLOCK)) {
			OSRestoreInterrupts(enabled);
			return FALSE;
		}

		OSSleepThread(&queue->queueSend);
	}

	index                  = (queue->firstIndex + queue->usedCount) % queue->msgCount;
	queue->msgArray[index] = msg;
	queue->usedCount++;

	OSWakeupThread(&queue->queueReceive);
	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * Removes the first message. A null msgPtr discards it.
 *
 * @note Address: 0x801681D0
 * @note Size: 0xDC
 */
BOOL OSReceiveMessage(OSMessageQueue* queue, OSMessage* msgPtr, s32 flags)
{
	BOOL enabled;

	enabled = OSDisableInterrupts();

	while (queue->usedCount == 0) {
		if (!(flags & OS_MESSAGE_BLOCK)) {
			OSRestoreInterrupts(enabled);
			return FALSE;
		}

		OSSleepThread(&queue->queueReceive);
	}

	if (msgPtr) {
		*msgPtr = queue->msgArray[queue->firstIndex];
	}

	queue->firstIndex = (queue->firstIndex + 1) % queue->msgCount;
	queue->usedCount--;

	OSWakeupThread(&queue->queueSend);
	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * Prepends a message. OS_MESSAGE_BLOCK waits for space in a full queue.
 *
 * @note Address: N/A
 * @note Size: 0xD4
 */
BOOL OSJamMessage(OSMessageQueue* queue, OSMessage msg, s32 flags)
{
	s32 index;
	BOOL enabled;

	enabled = OSDisableInterrupts();

	while (queue->msgCount <= queue->usedCount) {
		if (!(flags & OS_MESSAGE_BLOCK)) {
			OSRestoreInterrupts(enabled);
			return FALSE;
		}

		OSSleepThread(&queue->queueSend);
	}

	// Move the head backwards by one slot.
	index                             = (queue->firstIndex + queue->msgCount - 1) % queue->msgCount;
	queue->firstIndex                 = index;
	queue->msgArray[queue->firstIndex] = msg;
	queue->usedCount++;

	OSWakeupThread(&queue->queueReceive);
	OSRestoreInterrupts(enabled);
	return TRUE;
}

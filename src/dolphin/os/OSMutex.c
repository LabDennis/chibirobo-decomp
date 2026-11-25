#include "Dolphin/os.h"

// Append a mutex to its owner's queue.
#define AddTailMutex(queue, mutex)     \
	do {                               \
		OSMutex* prev;                 \
                                       \
		prev = (queue)->tail;          \
		if (prev == nullptr)           \
			(queue)->head = (mutex);   \
		else                           \
			prev->link.next = (mutex); \
		(mutex)->link.prev = prev;     \
		(mutex)->link.next = nullptr;  \
		(queue)->tail      = (mutex);  \
	} while (0)

// Remove the first mutex from a nonempty owner queue.
#define RemoveHeadMutex(queue, mutex)  \
	do {                               \
		OSMutex* next;                 \
                                       \
		(mutex) = (queue)->head;       \
		next    = (mutex)->link.next;  \
		if (next == nullptr)           \
			(queue)->tail = nullptr;   \
		else                           \
			next->link.prev = nullptr; \
		(queue)->head = next;          \
	} while (0)

// Remove a mutex from its owner's queue.
#define RemoveItemMutex(queue, mutex) \
	do {                              \
		OSMutex* next;                \
		OSMutex* prev;                \
                                      \
		next = (mutex)->link.next;    \
		prev = (mutex)->link.prev;    \
                                      \
		if (next == nullptr)          \
			(queue)->tail = prev;     \
		else                          \
			next->link.prev = prev;   \
                                      \
		if (prev == nullptr)          \
			(queue)->head = next;     \
		else                          \
			prev->link.next = next;   \
	} while (0)

/**
 * @note Address: N/A
 * @note Size: 0x38
 */
void OSInitMutex(OSMutex* mutex)
{
	OSInitThreadQueue(&mutex->queue);
	mutex->thread = nullptr;
	mutex->count  = 0;
}

/**
 * @note Address: N/A
 * @note Size: 0xDC
 */
void OSLockMutex(OSMutex* mutex)
{
	BOOL enabled            = OSDisableInterrupts();
	OSThread* currentThread = OSGetCurrentThread();
	OSThread* ownerThread;

	while (TRUE) {
		ownerThread = mutex->thread;
		if (ownerThread == 0) {
			mutex->thread = currentThread;
			mutex->count++;
			AddTailMutex(&currentThread->queueMutex, mutex);
			break;
		} else if (ownerThread == currentThread) {
			mutex->count++;
			break;
		} else {
			currentThread->mutex = mutex;
			__OSPromoteThread(mutex->thread, currentThread->priority);
			OSSleepThread(&mutex->queue);
			currentThread->mutex = nullptr;
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: 0xC8
 */
void OSUnlockMutex(OSMutex* mutex)
{
	BOOL enabled            = OSDisableInterrupts();
	OSThread* currentThread = OSGetCurrentThread();

	if (mutex->thread == currentThread && --mutex->count == 0) {
		RemoveItemMutex(&currentThread->queueMutex, mutex);
		mutex->thread = nullptr;
		if (currentThread->priority < currentThread->base) {
			currentThread->priority = __OSGetEffectivePriority(currentThread);
		}

		OSWakeupThread(&mutex->queue);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x80168590
 * @note Size: 0x70
 */
void __OSUnlockAllMutex(OSThread* thread)
{
	OSMutex* mutex;

	while (thread->queueMutex.head) {
		RemoveHeadMutex(&thread->queueMutex, mutex);
		mutex->count  = 0;
		mutex->thread = nullptr;
		OSWakeupThread(&mutex->queue);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xBC
 */
BOOL OSTryLockMutex(OSMutex* mutex)
{
	BOOL enabled            = OSDisableInterrupts();
	OSThread* currentThread = OSGetCurrentThread();
	BOOL locked;

	if (mutex->thread == nullptr) {
		mutex->thread = currentThread;
		mutex->count++;
		AddTailMutex(&currentThread->queueMutex, mutex);
		locked = TRUE;

	} else if (mutex->thread == currentThread) {
		mutex->count++;
		locked = TRUE;

	} else {
		locked = FALSE;
	}

	OSRestoreInterrupts(enabled);
	return locked;
}

/**
 * @note Address: N/A
 * @note Size: 0x20
 */
void OSInitCond(OSCond* cond)
{
	OSInitThreadQueue(&cond->queue);
}

/**
 * @note Address: N/A
 * @note Size: 0xD4
 */
void OSWaitCond(OSCond* cond, OSMutex* mutex)
{
	BOOL enabled            = OSDisableInterrupts();
	OSThread* currentThread = OSGetCurrentThread();
	s32 count;

	if (mutex->thread == currentThread) {
		count        = mutex->count;
		mutex->count = 0;
		RemoveItemMutex(&currentThread->queueMutex, mutex);
		mutex->thread = nullptr;

		if (currentThread->priority < currentThread->base) {
			currentThread->priority = __OSGetEffectivePriority(currentThread);
		}

		OSDisableScheduler();
		OSWakeupThread(&mutex->queue);
		OSEnableScheduler();
		OSSleepThread(&cond->queue);
		OSLockMutex(mutex);
		mutex->count = count;
	}

	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: 0x20
 */
void OSSignalCond(OSCond* cond)
{
	OSWakeupThread(&cond->queue);
}

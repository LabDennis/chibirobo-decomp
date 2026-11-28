#ifndef _DOLPHIN_OS_OSTHREAD_H
#define _DOLPHIN_OS_OSTHREAD_H

#include "Dolphin/OS/OSUtil.h"
#include <sdk/dolphin/os/OSThread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Thread global addresses.
volatile OSContext* __OSCurrentContext AT_ADDRESS((u32)OSPhysicalToCached(0x00D4));
volatile OSContext* __OSFPUContext AT_ADDRESS((u32)OSPhysicalToCached(0x00D8));
OSThreadQueue __OSActiveThreadQueue AT_ADDRESS((u32)OSPhysicalToCached(0x00DC));
OSThread* __OSCurrentThread AT_ADDRESS((u32)OSPhysicalToCached(0x00E4));

///////// THREAD MACROS //////////
// The Revolution SDK keeps the three-argument forms here;
// the GameCube SDK file's OSThread.c defines its own two-argument set.
#ifdef VERSION_R24J01
// Add link to queue at tail.
#define AddTail(queue, thread, link)    \
	do {                                \
		OSThread* prev;                 \
                                        \
		prev = (queue)->tail;           \
		if (prev == nullptr)            \
			(queue)->head = (thread);   \
		else                            \
			prev->link.next = (thread); \
		(thread)->link.prev = prev;     \
		(thread)->link.next = nullptr;  \
		(queue)->tail       = (thread); \
	} while (0)

// Add link to queue in priority order.
#define AddPrio(queue, thread, link)                                                                   \
	do {                                                                                               \
		OSThread *prev, *next;                                                                         \
                                                                                                       \
		for (next = (queue)->head; next && next->priority <= thread->priority; next = next->link.next) \
			;                                                                                          \
		if (next == nullptr)                                                                           \
			AddTail(queue, thread, link);                                                              \
		else {                                                                                         \
			(thread)->link.next = next;                                                                \
			prev                = next->link.prev;                                                     \
			next->link.prev     = (thread);                                                            \
			(thread)->link.prev = prev;                                                                \
			if (prev == nullptr)                                                                       \
				(queue)->head = (thread);                                                              \
			else                                                                                       \
				prev->link.next = (thread);                                                            \
		}                                                                                              \
	} while (0)

// Remove link from queue.
#define RemoveItem(queue, thread, link) \
	do {                                \
		OSThread *next, *prev;          \
		next = (thread)->link.next;     \
		prev = (thread)->link.prev;     \
		if (next == nullptr)            \
			(queue)->tail = prev;       \
		else                            \
			next->link.prev = prev;     \
		if (prev == nullptr)            \
			(queue)->head = next;       \
		else                            \
			prev->link.next = next;     \
	} while (0)

// Remove head link from queue.
#define RemoveHead(queue, thread, link)  \
	do {                                 \
		OSThread* __next;                \
		(thread) = (queue)->head;        \
		__next   = (thread)->link.next;  \
		if (__next == nullptr)           \
			(queue)->tail = nullptr;     \
		else                             \
			__next->link.prev = nullptr; \
		(queue)->head = __next;          \
	} while (0)
#endif // VERSION_R24J01

//////////////////////////////////

////////// MUTEX MACROS //////////
// The Revolution SDK keeps the three-argument forms here (pik1wii/pik2wii OSThread.h);
// the GameCube SDK file's OSMutex.c defines its own two-argument set.
#ifdef VERSION_R24J01
// Add link to queue at tail.
#define AddTailMutex(queue, mutex, link) \
	do {                                 \
		OSMutex* prev;                   \
                                         \
		prev = (queue)->tail;            \
		if (prev == nullptr)             \
			(queue)->head = (mutex);     \
		else                             \
			prev->link.next = (mutex);   \
		(mutex)->link.prev = prev;       \
		(mutex)->link.next = nullptr;    \
		(queue)->tail      = (mutex);    \
	} while (0)

// Remove head link.
#define RemoveHeadMutex(queue, mutex, link) \
	do {                                    \
		OSMutex* next;                      \
                                            \
		(mutex) = (queue)->head;            \
		next    = (mutex)->link.next;       \
		if (next == nullptr)                \
			(queue)->tail = nullptr;        \
			else                            \
				next->link.prev = nullptr;  \
		(queue)->head = next;               \
	} while (0)

// Remove item.
#define RemoveItemMutex(queue, mutex, link) \
	do {                                    \
		OSMutex* next;                      \
		OSMutex* prev;                      \
                                            \
		next = (mutex)->link.next;          \
		prev = (mutex)->link.prev;          \
                                            \
		if (next == nullptr)                \
			(queue)->tail = prev;           \
		else                                \
			next->link.prev = prev;         \
                                            \
		if (prev == nullptr)                \
			(queue)->head = next;           \
		else                                \
			prev->link.next = next;         \
	} while (0)
#endif // VERSION_R24J01


#ifdef __cplusplus
}
#endif

#endif

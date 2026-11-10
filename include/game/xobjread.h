#ifndef _GAME_XOBJREAD_H
#define _GAME_XOBJREAD_H

#include "game/cthread.h"
extern "C" {
#include <dolphin/os.h>
}
#include "Dolphin/OS/OSMessage.h"

// the threads that read objects in the background, class names from RTTI. xobjs.cpp keeps
// one of each kind, the methods are there too

// an OSMessageQueue with its message buffer
class CMsgQueue {
public:
	OSMessageQueue mQueue; // _00
	OSMessage* mMsgs;      // _20
	// _24 vtable

	virtual ~CMsgQueue();               // _08
	virtual void Initialize(s32 count); // _0C, allocates mMsgs and inits mQueue
};
typedef char CMsgQueue_size_check[sizeof(CMsgQueue) == 0x28 ? 1 : -1];

inline CMsgQueue::~CMsgQueue()
{
}

// one read waiting in a reader
struct XObjReadReq {
	void* mObj;  // _00, null when the slot is free
	void* mInfo; // _04
};

class XObjRead : public CThread {
public:
	// _00-_328 = CThread
	CMsgQueue mQueue;     // _328
	XObjReadReq mReqs[8]; // _350

	BOOL ReadStart(void* obj, void* info);

	void StartReader(OSThreadStartFunction func, void* param, u32 stackSize, OSPriority priority, u16 attr)
	{
		mQueue.Initialize(8);
		for (s32 i = 0; i < 8; i++) {
			mReqs[i].mObj = NULL;
		}
		mStack     = (u8*)OSAllocFromHeap(__OSCurrHeap, stackSize);
		mStackSize = stackSize;
		if (OSCreateThread(&mThread, func, param, mStack + mStackSize, mStackSize, priority, attr)) {
			mStackSize = stackSize;
		}
		OSResumeThread(&mThread);
	}

	virtual ~XObjRead(); // _08
	virtual BOOL Create(OSThreadStartFunction func, void* param, u32 stackSize, OSPriority priority,
	                    u16 attr);                    // _0C, makes the thread and its queue
	virtual BOOL QueueRead(void* object, void* info); // _10
	virtual void RunRequestLoop();                    // _14
	virtual void ProcessRequest(XObjReadReq* req);    // _18, runs one read
	virtual BOOL HasPendingReads();                   // _1C
};
typedef char XObjRead_size_check[sizeof(XObjRead) == 0x390 ? 1 : -1];

// reads a whole object list
class XObjReadAll : public XObjRead {
public:
	virtual ~XObjReadAll();                        // _08
	virtual void ProcessRequest(XObjReadReq* req); // _18
};

// reads one object
class XObjReadOne : public XObjRead {
public:
	virtual ~XObjReadOne();                           // _08
	virtual BOOL QueueRead(void* object, void* info); // _10
	virtual void ProcessRequest(XObjReadReq* req);    // _18
};

// reads the anim archives
class XObjReadAnm : public XObjRead {
public:
	virtual ~XObjReadAnm();                           // _08
	virtual BOOL QueueRead(void* object, void* info); // _10
	virtual void ProcessRequest(XObjReadReq* req);    // _18
};

STATIC_ASSERT(sizeof(XObjReadAnm) == 0x390);

extern "C" {
BOOL HasPendingObjectReads();
void* ReadObjectListThread(void* arg);
void* ReadSingleObjectThread(void* arg);
void* ReadObjectAnimationsThread(void* arg);
extern XObjReadAll gAllObjectsReader;
extern XObjReadOne gSingleObjectReader;
extern XObjReadAnm gObjectAnimationReader;
}

#endif

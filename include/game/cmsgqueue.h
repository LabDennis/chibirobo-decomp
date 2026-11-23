#ifndef _GAME_CMSGQUEUE_H
#define _GAME_CMSGQUEUE_H

#include "Version.h"
#include "Dolphin/OS/OSMessage.h"

// an OSMessageQueue with its message buffer
#if defined(VERSION_R24J01)
class CMsgQueue : public OSMessageQueue {
public:
	// _00-_1F OSMessageQueue
	OSMessage* mMsgs; // _20
	                  // _24 vtable
#else
class CMsgQueue {
public:
	OSMessageQueue mQueue; // _00
	OSMessage* mMsgs;      // _20
	                       // _24 vtable
#endif
	virtual ~CMsgQueue();               // _08
	virtual void Initialize(s32 count); // _0C
};
STATIC_ASSERT(sizeof(CMsgQueue) == 0x28);

inline CMsgQueue::~CMsgQueue()
{
}

#endif

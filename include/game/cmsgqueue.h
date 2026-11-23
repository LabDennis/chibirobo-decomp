#ifndef _GAME_CMSGQUEUE_H
#define _GAME_CMSGQUEUE_H

#include "Version.h"
#include "Dolphin/OS/OSMessage.h"

// an OSMessageQueue with its message buffer. the Wii's RTTI shows it derives from
// OSMessageQueue, the GameCube's wraps it as a member, same layout
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
	virtual void Initialize(s32 count); // _0C, allocates mMsgs and inits the queue
};
STATIC_ASSERT(sizeof(CMsgQueue) == 0x28);

// the Wii's retail emits the out-of-line deleting copy in xobjs.cpp; the inline body
// keeps every derived dtor folding it to nothing, like retail's
inline CMsgQueue::~CMsgQueue()
{
}

#endif

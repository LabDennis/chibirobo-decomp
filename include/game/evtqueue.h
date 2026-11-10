#ifndef _GAME_EVTQUEUE_H
#define _GAME_EVTQUEUE_H

#include "game/evt.h"

class XObj;

// maps object ids from 10000 up to an index in the game's object table
struct EvtObjIdEntry {
	s16 mId;    // _00
	s16 mIndex; // _02
};

struct EvtObjIdTable {
	s32 mCount;                // _00
	EvtObjIdEntry mEntry[128]; // _04
};
STATIC_ASSERT(sizeof(EvtObjIdTable) == 0x204);

extern "C" {
extern CEvtQueueEx gEventScriptQueue;
extern CEvtTimer gEventTimers;
// The event system constructs its own XObj at startup.
extern XObj gEventScriptObject;
// script names for objects
extern EvtObjIdTable gEventObjectIds;
}

#endif

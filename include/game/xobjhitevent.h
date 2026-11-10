#ifndef _GAME_XOBJHITEVENT_H
#define _GAME_XOBJHITEVENT_H

#include "game/xobjflags.h"
#include "game/cgame.h"
#include "game/evt.h"
#include "game/xobjchr.h"

static inline void StartObjectHitEvent(XObj* object, u32 first, u32 second)
{
	const void* script = object->vf128();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer()))
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, (u32)object, first, second);
}

#endif

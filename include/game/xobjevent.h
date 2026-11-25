#ifndef _GAME_XOBJEVENT_H
#define _GAME_XOBJEVENT_H

#include "game/cgame.h"
#include "game/xobjchr.h"

inline void XObj::TriggerEvent(s16 useValue, s16 hitValue)
{
	u8* script = GetEventScript();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer()))
		StartOrQueueEventScript(&gGame.mEvt, script, (u32)this, useValue, hitValue);
}

#endif

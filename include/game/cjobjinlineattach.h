#ifndef _GAME_CJOBJINLINEATTACH_H
#define _GAME_CJOBJINLINEATTACH_H

#include "game/cjobj.h"

// msg.cpp and a few others inline the jobj ctor
inline CJObj::CJObj(HSD_JObj* jobj)
{
	Attach(jobj);
}

#endif

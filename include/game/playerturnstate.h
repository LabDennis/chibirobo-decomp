#ifndef _GAME_PLAYERTURNSTATE_H
#define _GAME_PLAYERTURNSTATE_H

#include "types.h"

struct PlayerTurnState {
	f32 heading;         // _00
	f32 previousHeading; // _04
	f32 change;          // _08
	f32 previousChange;  // _0C
	f32 total;           // _10
};
STATIC_ASSERT(sizeof(PlayerTurnState) == 0x14);

#endif

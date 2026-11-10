#ifndef GAME_EVENTHUDCOMMANDS_H
#define GAME_EVENTHUDCOMMANDS_H

#include "types.h"

struct EvtHudCommand {
	u8 enabled; // _00
	s32 mode;   // _04
};

template <int N>
struct EvtHudTable {
	EvtHudCommand entries[N];
};

#define EVT_HUD_BATTERY_COMMANDS                                              \
	{                                                                         \
		{ 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }, { 0, 2 }, { 1, 2 }, { 0, 3 }, \
		{                                                                     \
			1, 3                                                              \
		}                                                                     \
	}
#define EVT_HUD_CLOCK_COMMANDS                                                \
	{                                                                         \
		{ 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 }, { 1, 3 }, { 0, 4 }, \
		{                                                                     \
			1, 4                                                              \
		}                                                                     \
	}
#define EVT_HUD_SCORE_TIMER_COMMANDS  \
	{                                 \
		{ 0, 0 }, { 1, 0 }, { 0, 1 }, \
		{                             \
			1, 1                      \
		}                             \
	}

#endif

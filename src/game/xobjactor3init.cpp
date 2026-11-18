#include "game/xobjactor.h"

// retail's .data for this file starts with these two tables, the handler
// table's member pointer constants come right after them
extern "C" {
// nothing in the game reads it, its meaning is unknown
s32 lbl_8022B758[26] = {
	0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 3, 0,
};

// the haisui nezumi's anim sounds, scrc hands them to the actor
AnimSfxEvent gHaisuiNezumiAnimSfxEvents[] = {
	{ 3, 11.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ 3, 53.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ 8, 11.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ 8, 53.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ 4, 1.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ 4, 11.0f, 0.0f, 1, 0xB0003, 0xFF, 0, 0, 0 },
	{ -1 },
};
}

/**
 * @note Address: 0x801152A0
 * @note Size: 0x298
 */
// built at startup, member function pointers can't be plain data
XObjActor3Func gActor3UpdateHandlers[20] = {
	&XObjActor3::Kind0,
	&XObjActor3::UpdateObjectGauge,
	&XObjActor3::Kind2,
	&XObjActor3::Kind3,
	&XObjActor3::Kind4,
	&XObjActor3::Kind5,
	&XObjActor3::Kind6,
	&XObjActor3::Kind7,
	&XObjActor3::Kind8,
	&XObjActor3::Kind9,
	&XObjActor3::Kind10,
	&XObjActor3::Kind11,
	&XObjActor3::Kind12,
	&XObjActor3::Kind13,
	&XObjActor3::UpdateSpiderCounterPanel,
	&XObjActor3::Kind15,
	&XObjActor3::Kind16,
	&XObjActor3::Kind17,
	&XObjActor3::Kind18,
	&XObjActor3::Kind19,
};

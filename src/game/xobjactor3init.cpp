#include "game/xobjactor.h"

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

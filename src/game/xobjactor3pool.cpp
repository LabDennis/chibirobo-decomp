#include "game/xobjactor.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

extern "C" {
extern u8 gActor3Pool[sizeof(XObjActor3) * 48];
XObjActor3* __dt__10XObjActor3Fv(XObjActor3* object, s32 flags);
}

/**
 * @note Address: 0x80134500
 * @note Size: 0x38
 */
// todo: weak copy
extern "C" void DestroyXObjActor3Array()
{
	__destroy_arr(gActor3Pool, (ConstructorDestructor*)__dt__10XObjActor3Fv, sizeof(XObjActor3), sizeof(gActor3Pool) / sizeof(XObjActor3));
}

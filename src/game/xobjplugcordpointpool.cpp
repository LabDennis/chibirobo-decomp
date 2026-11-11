#include "game/xobjattachment.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

extern "C" {
extern XObjPlugCordPoint gPlugCordPoints[128];
XObjPlugCordPoint* __dt__17XObjPlugCordPointFv(XObjPlugCordPoint* point, s32 flags);
}

// todo: weak copy
// the array dtor for fn_800DBA7C's points
/**
 * @note Address: 0x800DBBCC
 * @note Size: 0x38
 */
extern "C" void DestroyXObjPlugCordPointArray()
{
	__destroy_arr(gPlugCordPoints, (ConstructorDestructor*)__dt__17XObjPlugCordPointFv, sizeof(XObjPlugCordPoint), 128);
}

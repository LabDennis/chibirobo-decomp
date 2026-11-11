#include "game/xobjspidersmall.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"
#include <string.h>
#include "PowerPC_EABI_Support/Runtime/global_destructor_chain.h"

extern "C" {
extern SmallSpiderStageStatus gSmallSpiderStatusByStage[39];
extern u32 gSmallSpiderStatusDestructorChain[3];
extern void (XObjSpiderSmall::*sSmallSpiderState0Descriptor)(u32);
}

/**
 * @note Address: 0x800FFFA0
 * @note Size: 0x17C
 */
// the tu's __sinit, the statics it sets up are other units'
extern "C" void __sinit_xobjspidersmall_cpp()
{
	__construct_array(gSmallSpiderStatusByStage, (ConstructorDestructor)__ct__22SmallSpiderStageStatusFv,
	                  (ConstructorDestructor)__dt__22SmallSpiderStageStatusFv, sizeof(gSmallSpiderStatusByStage[0]), 39);
	__register_global_object(NULL, (void*)DestroySmallSpiderStatusTable, gSmallSpiderStatusDestructorChain);
	memcpy(gSmallSpiderStateHandlers, &sSmallSpiderState0Descriptor, sizeof(gSmallSpiderStateHandlers));
}

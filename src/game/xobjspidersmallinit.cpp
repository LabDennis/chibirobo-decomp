#include "game/xobjspidersmall.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"
#include "PowerPC_EABI_Support/Runtime/global_destructor_chain.h"

struct XObjSpiderSmallParams {
	u32 _00, _04, _08, _0C, _10, _14; // _00
	u32 _18, _1C, _20, _24, _28, _2C; // _18
	u32 _30, _34, _38, _3C, _40, _44; // _30
	u32 _48, _4C, _50, _54, _58, _5C; // _48
	u32 _60, _64, _68, _6C, _70, _74; // _60
};
typedef char XObjSpiderSmallParams_size_check[sizeof(XObjSpiderSmallParams) == 0x78 ? 1 : -1];

// the tu's data block, the code reaches it from sSmallSpiderStateData
struct XObjSpiderSmallData {
	u8 _000[0x130];                  // _000
	XObjSpiderSmallParams mDefaults; // _130
	XObjSpiderSmallParams mParams;   // _1A8, gSmallSpiderStateHandlers
};

extern "C" {
extern XObjSpiderSmallData sSmallSpiderStateData;
}

/**
 * @note Address: 0x800FFFA0
 * @note Size: 0x17C
 */
// todo: the tu's __sinit, the statics it sets up are other units'
extern "C" void __sinit_xobjspidersmall_cpp()
{
	__construct_array(gSmallSpiderStatusByStage, (ConstructorDestructor)__ct__22SmallSpiderStageStatusFv,
	                  (ConstructorDestructor)__dt__22SmallSpiderStageStatusFv, sizeof(gSmallSpiderStatusByStage[0]), 39);
	__register_global_object(NULL, (void*)DestroySmallSpiderStatusTable, gSmallSpiderStatusDestructorChain);
	sSmallSpiderStateData.mParams = sSmallSpiderStateData.mDefaults;
}

__declspec(section ".ctors") extern void (*const spiderSmallInit)() = __sinit_xobjspidersmall_cpp;

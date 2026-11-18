#include "game/xobjflags.h"

void operator delete(void* object);

extern "C" {
extern void* __vt__8XObjDust[];
extern void* __vt__9XObjFlags[];
XObjFlags* __dt__9XObjFlagsFv(XObjFlags* flags, s32 del);
}

/**
 * @note Address: 0x800E2240
 * @note Size: 0x60
 */
// todo: weak copy
extern "C" XObjDust* __dt__8XObjDustFv(XObjDust* dust, s32 del)
{
	if (dust != NULL) {
		*(void***)((u8*)dust + 0x34) = __vt__8XObjDust;
		__dt__9XObjFlagsFv(dust, 0);
		if ((s16)del > 0)
			::operator delete(dust);
	}
	return dust;
}

/**
 * @note Address: 0x800E22A0
 * @note Size: 0x60
 */
// todo: weak copy
extern "C" XObjFlags* __dt__9XObjFlagsFv(XObjFlags* flags, s32 del)
{
	if (flags != NULL) {
		*(void***)((u8*)flags + 0x34) = __vt__9XObjFlags;
		__dt__4XObjFv((XObj*)flags, 0);
		if ((s16)del > 0)
			::operator delete(flags);
	}
	return flags;
}

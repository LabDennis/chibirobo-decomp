#include "game/hsd_jobj.h"
#include "game/cprimitive.h"

void operator delete(void* object);
extern "C" {
extern void* __vt__7CSprite[];
extern void* __vt__10CPrimitive[];

// todo: weak copy
/**
 * @note Address: 0x8001AE9C
 * @note Size: 0x5C
 */
CSprite* __dt__7CSpriteFv(CSprite* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__7CSprite; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}

// todo: weak copy
/**
 * @note Address: 0x8001AEF8
 * @note Size: 0x48
 */
CPrimitive* __dt__10CPrimitiveFv(CPrimitive* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__10CPrimitive; // _164 vtable
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}
}

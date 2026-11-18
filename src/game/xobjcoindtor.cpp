#include "game/xobjflags.h"
#include "game/xobjhbox.h"

void operator delete(void* object);

extern "C" {
extern void* __vt__8XObjCoin[];
extern void* __vt__8XObjItem[];
XObjFlags* __dt__9XObjFlagsFv(XObjFlags* flags, s32 del);
}

/**
 * @note Address: 0x800E240C
 * @note Size: 0x54
 */
// todo: weak copy
extern "C" XObjHBoxPart* __dt__12XObjHBoxPartFv(XObjHBoxPart* part, s32 del)
{
	if (part != NULL) {
		__dt__4XObjFv((XObj*)&part->mObj, -1);
		if ((s16)del > 0)
			::operator delete(part);
	}
	return part;
}

/**
 * @note Address: 0x800E2460
 * @note Size: 0x60
 */
// todo: weak copy
extern "C" XObjCoin* __dt__8XObjCoinFv(XObjCoin* coin, s32 del)
{
	if (coin != NULL) {
		*(void***)((u8*)coin + 0x34) = __vt__8XObjCoin;
		__dt__9XObjFlagsFv(coin, 0);
		if ((s16)del > 0)
			::operator delete(coin);
	}
	return coin;
}

/**
 * @note Address: 0x800E24C0
 * @note Size: 0x60
 */
// todo: weak copy
extern "C" XObjItem* __dt__8XObjItemFv(XObjItem* item, s32 del)
{
	if (item != NULL) {
		*(void***)((u8*)item + 0x34) = __vt__8XObjItem;
		__dt__9XObjFlagsFv(item, 0);
		if ((s16)del > 0)
			::operator delete(item);
	}
	return item;
}

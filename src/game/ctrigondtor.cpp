#include "Version.h"
#include "game/ctrigon.h"

#if defined(VERSION_R24J01)
// the weak dtor copy the GameCube file spells out by hand; retail 0x800BEB0C is the
// minimal body (no member/vtable work)
CTrigon::~CTrigon()
{
}
#else
void operator delete(void* object);
extern "C" {
extern void* __vt__7CTrigon[];
}

// todo: weak copy
/**
 * @note Address: 0x800931C8
 * @note Size: 0x48
 */
extern "C" CTrigon* __dt__7CTrigonFv(CTrigon* object, s32 flags)
{
	if (object != NULL) {
		*(void***)((u8*)object + 0x18) = __vt__7CTrigon;
		if ((s16)flags > 0)
			::operator delete(object);
	}
	return object;
}
#endif

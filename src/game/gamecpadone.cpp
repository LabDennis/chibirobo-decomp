#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/wii/cpad.h"

// the weak dtor copy the GameCube file spells out by hand; on Wii the emitted body
// is the minimal one (retail 0x800C0EF0, no member/vtable work)
CPadOne::~CPadOne()
{
}
#else
#include "game/cpad.h"

void operator delete(void* object);

extern "C" void* __vt__7CPadOne[];

/**
 * @note Address: 0x800083D4
 * @note Size: 0x48
 */
// todo: weak copy
extern "C" CPadOne* __dt__7CPadOneFv(CPadOne* pad, s32 flags)
{
	if (pad != NULL) {
		*(void***)((u8*)pad + 0x50) = __vt__7CPadOne;
		if ((s16)flags > 0)
			::operator delete(pad);
	}
	return pad;
}
#endif

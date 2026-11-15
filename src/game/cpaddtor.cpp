#include "Version.h"
#include "game/cpad.h"

#if defined(VERSION_R24J01)
// the weak dtor copy the GameCube file spells out by hand; on Wii the emitted body
// is the minimal one (retail 0x800365D8, past game/wii/init.cpp's range)
CPad::~CPad()
{
}
#else
extern "C" void* __vt__4CPad[];

/**
 * @note Address: 0x8001C854
 * @note Size: 0x48
 */
// todo: weak copy
extern "C" CPad* __dt__4CPadFv(CPad* pad, s32 flags)
{
	if (pad != NULL) {
		*(void***)((u8*)pad + 0x4) = __vt__4CPad;
		if ((s16)flags > 0) {
			::operator delete(pad);
		}
	}
	return pad;
}
#endif

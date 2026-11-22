#include "game/ctrigon.h"

// Wii only: the GameCube copy is the weak ~CTrigon obja_map.cpp emits
void operator delete(void* object) throw();

// todo: weak copy
// the compiler's weak dtor spelled out by hand like below; unlike the GameCube
// body the retail copy (0x800BEB0C) doesn't restore the vtable
extern "C" CTrigon* __dt__7CTrigonFv(CTrigon* object, s32 flags)
{
	if (object != NULL) {
		if (flags > 0)
			::operator delete(object);
	}
	return object;
}

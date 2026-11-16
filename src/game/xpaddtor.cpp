#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/xpad.h"
#include "game/wii/xchrmenu.h"

// the weak dtor copies the GameCube file spells out by hand; on Wii the emitted body is
// the minimal one (retail 0x800C0F30, no vtable store and no base call), so ~CPadOne is
// defined inline here to be inlined away without re-emitting CPadOne's vtable
inline CPadOne::~CPadOne()
{
}
XPad::~XPad()
{
}
XObjChrMenu::XObjChrMenu()
{
}
// NONMATCHING: both functions match; the unit cannot link byte-exact because the
// XObjChrMenu ctor's AtcMenu/DrsMenu vtable stores resolve to compiled-out vtables
// (atcmenudtor.o) whose placement shifts .data, while naming them in symbols.txt
// collides with those same strong emissions
#else
#include "game/xpad.h"

void operator delete(void* object);
extern "C" {
extern void* __vt__9CJointRot[];
extern void* __vt__6Params[];
extern void* __vt__4XPad[];

/**
 * @note Address: 0x800BD17C
 * @note Size: 0x48
 */
// CJointRot::~CJointRot, cobj.h can't come in with cpad.h
// todo: weak copy
void* __dt__9CJointRotFv(void* object, s32 flags)
{
	if (object != NULL) {
		*(void***)((u8*)object + 0x1C) = __vt__9CJointRot;
		if ((s16)flags > 0)
			::operator delete(object);
	}
	return object;
}

/**
 * @note Address: 0x800BD1C4
 * @note Size: 0x48
 */
// Params::~Params, its vtable is its last word
// todo: weak copy
void* __dt__6ParamsFv(void* object, s32 flags)
{
	if (object != NULL) {
		*(void***)((u8*)object + 0x440) = __vt__6Params;
		if ((s16)flags > 0)
			::operator delete(object);
	}
	return object;
}

/**
 * @note Address: 0x800BD20C
 * @note Size: 0x5C
 */
// todo: weak copy
XPad* __dt__4XPadFv(XPad* pad, s32 flags)
{
	if (pad != NULL) {
		*(void***)((u8*)pad + sizeof(CPadOne) - 4) = __vt__4XPad;
		pad->CPadOne::~CPadOne();
		if ((s16)flags > 0)
			::operator delete(pad);
	}
	return pad;
}
}
#endif

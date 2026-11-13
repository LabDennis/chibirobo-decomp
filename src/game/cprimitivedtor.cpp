#include "game/hsd_jobj.h"
#include "game/cprimitive.h"

void operator delete(void* object);
extern "C" {
extern void* __vt__5CLine[];
extern void* __vt__9CTriangle[];
extern void* __vt__5CCube[];
extern void* __vt__11COctahedron[];
extern void* __vt__7CDodeca[];
extern void* __vt__12CIcosahedron[];
extern void* __vt__7CSphere[];
extern void* __vt__9CCylinder[];
extern void* __vt__6CTorus[];

// the file holds the compiler's deleting destructors for the primitives: each stores the
// child vtable over the slot at _164, runs the base dtor and frees when flags > 0
// todo: weak copy
/**
 * @note Address: 0x800187DC
 * @note Size: 0x5C
 */
CLine* __dt__5CLineFv(CLine* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__5CLine; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}

// todo: weak copy
/**
 * @note Address: 0x80018838
 * @note Size: 0x5C
 */
CTriangle* __dt__9CTriangleFv(CTriangle* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__9CTriangle; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}

// todo: weak copy
/**
 * @note Address: 0x80018894
 * @note Size: 0x5C
 */
CCube* __dt__5CCubeFv(CCube* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__5CCube; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}

// todo: weak copy
/**
 * @note Address: 0x800188F0
 * @note Size: 0x5C
 */
COctahedron* __dt__11COctahedronFv(COctahedron* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__11COctahedron; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}

// todo: weak copy
/**
 * @note Address: 0x8001894C
 * @note Size: 0x5C
 */
CDodeca* __dt__7CDodecaFv(CDodeca* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__7CDodeca; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}

// todo: weak copy
/**
 * @note Address: 0x800189A8
 * @note Size: 0x5C
 */
CIcosahedron* __dt__12CIcosahedronFv(CIcosahedron* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__12CIcosahedron; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}

// todo: weak copy
/**
 * @note Address: 0x80018A04
 * @note Size: 0x5C
 */
CSphere* __dt__7CSphereFv(CSphere* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__7CSphere; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}

// todo: weak copy
/**
 * @note Address: 0x80018A60
 * @note Size: 0x5C
 */
CCylinder* __dt__9CCylinderFv(CCylinder* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__9CCylinder; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}

// todo: weak copy
/**
 * @note Address: 0x80018ABC
 * @note Size: 0x5C
 */
CTorus* __dt__6CTorusFv(CTorus* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__6CTorus; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}
}

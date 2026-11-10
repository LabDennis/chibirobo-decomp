#include "game/xobjattachment.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

void operator delete(void* object);
extern "C" {
extern void* __vt__8XObjPlug[];
void* __dt__11XObjWireEffFv(XObjWireEff* wire, s32 flags);
void* __dt__16XObjPlugCordNodeFv(void* object, s32 flags);
}

// todo: cleanup, stand-ins for the cord's own dtors
static inline void DestroyNodes(XObjPlugCordNodes* nodes)
{
	if (nodes != NULL)
		__destroy_arr(nodes, (ConstructorDestructor*)__dt__16XObjPlugCordNodeFv, sizeof(XObjPlugCordNode), 10);
}

static inline void DestroyCord(XObjPlugCord* cord)
{
	if (cord != NULL) {
		DestroyNodes(&cord->mNodes);
		cord->_6C.CVec::~CVec();
	}
}

/**
 * @note Address: 0x800BCC4C
 * @note Size: 0xA8
 */
// todo: weak copy
extern "C" XObjPlug* __dt__8XObjPlugFv(XObjPlug* plug, s32 flags)
{
	if (plug != NULL) {
		*(void***)((u8*)plug + 0x34) = __vt__8XObjPlug;
		__dt__11XObjWireEffFv(&plug->mWire, -1);
		DestroyCord(&plug->mCord);
		__dt__4XObjFv(plug, 0);
		if ((s16)flags > 0)
			::operator delete(plug);
	}
	return plug;
}

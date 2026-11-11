#include "game/xobjactor.h"
#include "game/xobjfree.h"
#include "game/xobjflags.h"
#include "game/xobjspidersmall.h"
#include "game/xobjsound.h"
#include "game/xchr.h"
#include "game/xobjmap.h"
#include "game/xobjread.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

// the objects XOBJS hands out, their constructors and the weak destructors their arrays are torn
// down with when the game ends

extern "C" {
// The two plain objects in pool 4.
extern u8 gActorsPool[sizeof(XObj) * 2];
}

/**
 * @note Address: 0x80134538
 * @note Size: 0xF4
 */
XObjActor3::XObjActor3()
{
}

/**
 * @note Address: 0x8013465C
 * @note Size: 0x38
 */
// 0x8013462C, CPtclDust3's ctor, the compiler emits it here

extern "C" void DestroyXObjArray()
{
	__destroy_arr(gActorsPool, (ConstructorDestructor*)__dt__4XObjFv, sizeof(XObj), sizeof(gActorsPool) / sizeof(XObj));
}

/**
 * @note Address: 0x80134694
 * @note Size: 0x354
 */
XObjChr::XObjChr()
{
}

#if !defined(VERSION_JP)
/**
 * @note Address: 0x801349E8
 * @note Size: 0x34
 */
OXMenuMark::OXMenuMark()
{
}
#endif

/**
 * @note Address: 0x80134A1C
 * @note Size: 0x3C
 */
XObjOX::XObjOX()
{
}

/**
 * @note Address: 0x80134A58
 * @note Size: 0x3C
 */
XObjAttachment::XObjAttachment()
{
}

/**
 * @note Address: 0x80134A94
 * @note Size: 0x3C
 */
XObjRadar::XObjRadar()
{
}

/**
 * @note Address: 0x80134AD0
 * @note Size: 0x3C
 */
XObjPopper::XObjPopper()
{
}

/**
 * @note Address: 0x80134B0C
 * @note Size: 0x3C
 */
XObjCopter::XObjCopter()
{
}

/**
 * @note Address: 0x80134B48
 * @note Size: 0x70
 */
XObjPlug::XObjPlug()
{
}

/**
 * @note Address: 0x80134BB8
 * @note Size: 0x3C
 */
XObjWireEff::XObjWireEff()
{
}

/**
 * @note Address: 0x80134BF4
 * @note Size: 0x2C
 */
XObjPlugCordNode::XObjPlugCordNode()
{
}

/**
 * @note Address: 0x80134C64
 * @note Size: 0x64
 */
XActPopperShot::XActPopperShot()
{
}

/**
 * @note Address: 0x80134CF8
 * @note Size: 0x7C
 */
// 0x80134CC8, CPtclDust2's ctor, the compiler emits it here

XObjMap::XObjMap()
{
}

/**
 * @note Address: 0x80134D8C
 * @note Size: 0x5C
 */
// 0x80134D74, CTrigon's ctor, the compiler emits it here

XObjLight::XObjLight()
{
}

/**
 * @note Address: 0x80134DE8
 * @note Size: 0x64
 */
XObjCamera::XObjCamera()
{
}

/**
 * @note Address: 0x80134E4C
 * @note Size: 0x48
 */
// todo: weak copy
// NONMATCHING: the vtable store is scheduled later
extern "C" CMsgQueue* __dt__9CMsgQueueFv(CMsgQueue* queue, s32 flags)
{
	if (queue != NULL) {
		queue->CMsgQueue::~CMsgQueue();
		if ((s16)flags > 0) {
			operator delete(queue);
		}
	}
	return queue;
}

/**
 * @note Address: 0x80134E94
 * @note Size: 0x84
 */
XObjReadOne::~XObjReadOne()
{
}

/**
 * @note Address: 0x80134F18
 * @note Size: 0x84
 */
XObjReadAll::~XObjReadAll()
{
}

/**
 * @note Address: 0x80134F9C
 * @note Size: 0x74
 */
// todo: weak copy
// NONMATCHING: the inlined dtors
extern "C" XObjRead* __dt__8XObjReadFv(XObjRead* reader, s32 flags)
{
	if (reader != NULL) {
		reader->XObjRead::~XObjRead();
		if ((s16)flags > 0) {
			operator delete(reader);
		}
	}
	return reader;
}

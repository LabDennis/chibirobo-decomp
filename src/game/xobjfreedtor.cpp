#include "game/xobjfree.h"

// todo: weak copy
/**
 * @note Address: 0x800DD5A0
 * @note Size: 0x60
 */
extern "C" XObjFree* __dt__8XObjFreeFv(XObjFree* object, s32 flags)
{
	if (object != NULL) {
		object->XObjFree::~XObjFree();
		if ((s16)flags > 0)
			::operator delete(object);
	}
	return object;
}

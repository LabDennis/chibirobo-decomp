#include "game/xobjactor.h"

/**
 * @note Address: 0x80115F18
 * @note Size: 0xBC
 */
extern "C" s32 HandleEventCommand__12XObjActorToyFlPl(XObjActorToy* object, s32 count, s32* command)
{
	HandleEventCommand__9XObjActorFlPl(object, count, command);
	if (count < 1)
		return 0;
	object->mCmd[0] = command[0];
	if (count < 2)
		object->mCmd[1] = 0;
	else
		object->mCmd[1] = command[1];
	object->mCmd[2] = command[2];
	object->mCmd[3] = command[3];
	object->mCmd[4] = command[4];
	object->mCmd[5] = command[5];
	object->mCmd[6] = command[6];
	object->mCmd[7] = command[7];
	object->mCmd[8] = command[8];
	object->mCmd[9] = command[9];
	return 0;
}

#include "game/xobjactor.h"

/**
 * @note Address: 0x80115F18
 * @note Size: 0xBC
 */
s32 XObjActorToy::HandleEventCommand(s32 count, s32* command)
{
	HandleEventCommand__9XObjActorFlPl(this, count, command);
	if (count < 1)
		return 0;
	mCmd[0] = command[0];
	if (count < 2)
		mCmd[1] = 0;
	else
		mCmd[1] = command[1];
	mCmd[2] = command[2];
	mCmd[3] = command[3];
	mCmd[4] = command[4];
	mCmd[5] = command[5];
	mCmd[6] = command[6];
	mCmd[7] = command[7];
	mCmd[8] = command[8];
	mCmd[9] = command[9];
	return 0;
}

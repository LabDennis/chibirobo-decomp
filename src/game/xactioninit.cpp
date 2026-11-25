#include "game/xact.h"

/**
 * @note Address: 0x800BBD7C
 * @note Size: 0x30
 */
void XAction::Init(XObjChr* owner)
{
	mOwner = owner;
	Reset();
}

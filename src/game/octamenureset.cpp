#include "game/octamenu.h"

// todo: weak copy
/**
 * @note Address: 0x80093BD8
 * @note Size: 0x30
 */
void OctaMenu::OpenActive()
{
	Open(0);
}

// todo: weak copy
/**
 * @note Address: 0x80093C08
 * @note Size: 0xC
 */
void OctaMenu::FinishClose()
{
	mOpen = 0;
}

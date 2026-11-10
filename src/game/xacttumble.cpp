#include "game/xact.h"

/**
 * @note Address: 0x800C490C
 * @note Size: 0x40
 */
extern "C" void ResetTumbleAction(XActTumble* act)
{
	act->Reset();
	act->mIdleParticle = NULL;
}

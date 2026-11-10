#ifndef GAME_ACTORTOYANIMATION_H
#define GAME_ACTORTOYANIMATION_H
#include "game/csound.h"
#include <dolphin/mtx/mtxtypes.h>
struct ActorToyAnimationData {
	s32 jointPathIndices[26];
	AnimSfxEvent sounds110[19];
	f32 movementSpeeds[60];
	Vec positions[7];
	AnimSfxEvent sounds6F[5];
	AnimSfxEvent sounds1FC[8];
	AnimSfxEvent sounds26CAnd485[2];
	AnimSfxEvent sounds159[2];
	AnimSfxEvent sounds117[12];
	AnimSfxEvent sounds1FB[4];
	AnimSfxEvent sounds317[8];
	AnimSfxEvent sounds25E[5];
};
STATIC_ASSERT(sizeof(ActorToyAnimationData) == 0x7C4);
extern "C" ActorToyAnimationData gActorToyAnimationData;
#endif

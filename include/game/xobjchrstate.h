#ifndef _GAME_XOBJCHRSTATE_H
#define _GAME_XOBJCHRSTATE_H
#include "game/xchr.h"
#include "game/cgame.h"
#include "game/pointptcl.h"

struct SuitParticle {
	Vec position;
	f32 scale;
	u32 color;
	f32 angle;
};

struct SuitChangeEffect {
	BOOL active;
	SuitParticle rings[2][10];

	void Reset()
	{
		active = FALSE;
		for (u32 ring = 0; ring < ARRAY_SIZE(rings); ++ring) {
			for (u32 particle = 0; particle < ARRAY_SIZE(rings[ring]); ++particle) {
				rings[ring][particle].scale = 0.0f;
				rings[ring][particle].angle = 0.0f;
			}
		}
	}
};

// joints that follow an item's own anim
struct PlayerTurnState {
	f32 heading, previousHeading;
	f32 change, previousChange;
	f32 total;
};

extern "C" {
f32 UpdatePlayerActions(XObjChr* player, u32 frames);
void DrawSpiralTrail(SuitChangeEffect*, const CVec&);
extern SuitChangeEffect gPlayerSuitChangeEffect;
extern PointPtcl gPointParticlePool[10];
extern PointPtcl* gPointParticleSlots[10];
void GetHudPointParticleOrigin(InfoScrn*, f32*, f32*);
}

#endif

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
};

extern "C" {
f32 UpdatePlayerActions(XObjChr* player, u32 frames);
void DrawSpiralTrail(SuitChangeEffect*, const CVec&);
extern SuitChangeEffect gPlayerSuitChangeEffect;
extern PointPtcl gPointParticlePool[10];
extern PointPtcl* gPointParticleSlots[10];
void GetHudPointParticleOrigin(InfoScrn*, f32*, f32*);
}

static inline void ResetPlayerEffects(bool clearPool)
{
	for (s32 i = 0; i < 10; ++i) {
		PointPtcl& p = gPointParticlePool[i];
		GetHudPointParticleOrigin(&gGame.mInfoScrn, &p.mStartY, &p.mStartX);
		p.display.mState = p.burst.mState = 0;
		if (clearPool)
			gPointParticleSlots[i] = NULL;
	}
	gPlayerSuitChangeEffect.active = FALSE;
	for (s32 i = 0; i < 2; ++i)
		for (s32 j = 0; j < 10; ++j) {
			gPlayerSuitChangeEffect.rings[i][j].scale = 0.0f;
			gPlayerSuitChangeEffect.rings[i][j].angle = 0.0f;
		}
}

static inline void ResetPlayerLook(XObjChrLook& look, XObj* target)
{
	look.mActive = FALSE;
	look.mMode   = -1;
	look.mObject = NULL;
	Vec zero     = { 0.0f, 0.0f, 0.0f };
	look.mOffset = zero;
	look.mFade = look.mPhase = look.mPulse = 0.0f;
	look.mObject                           = target;
}
#endif

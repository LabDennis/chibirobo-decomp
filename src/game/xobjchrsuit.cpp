#include "game/xobjchritem.h"
#include "game/xobjchrstate.h"
#include "game/cgame.h"
#include "game/gmath.h"
#include "game/video.h"
#include <stdlib.h>

extern "C" {
extern f32 gPlayerSuitChangeTimer;
}

static inline bool SuitFadeFinished(const XObjChr* player)
{
	const Vec& speed = player->mColorStep[1].mSpeed;
	return speed.x == 0.0f && speed.y == 0.0f && speed.z == 0.0f;
}

/**
 * @note Address: 0x800B9134
 * @note Size: 0x624
 */
extern "C" void UpdatePlayerSuitChange(XObjChr* player)
{
	PlayerSuits& suits = player->mSuits;
	switch (suits.mChangeStep) {
	case 1:
		player->SetColor(2, 255, 255, 255, 0.5f);
		gPlayerSuitChangeEffect.active = TRUE;
		for (u32 i = 0; i < ARRAY_SIZE(gPlayerSuitChangeEffect.rings); ++i) {
			for (u32 j = 0; j < ARRAY_SIZE(gPlayerSuitChangeEffect.rings[i]); ++j) {
				SuitParticle& particle = gPlayerSuitChangeEffect.rings[i][j];
				Vec position           = { 0.0f, 0.5f, 0.0f };
				particle.position      = position;
				particle.scale         = 0.0f;
				u32 green              = rand() % 156 + 100;
				u32 red                = rand() % 156 + 100;
				u32 blue               = rand() % 156 + 100;
				particle.color         = (red << 24) | (green << 16) | (blue << 8);
				particle.angle         = 0.0f;
			}
		}
		gPlayerSuitChangeEffect.rings[0][0].scale = 1.0f;
		gPlayerSuitChangeEffect.rings[1][0].scale = 1.0f;
		gPlayerSuitChangeEffect.rings[0][0].angle = DEG2RAD * (rand() % 360);
		gPlayerSuitChangeEffect.rings[1][0].angle = PI + gPlayerSuitChangeEffect.rings[0][0].angle;
		PlayGameSound(&gGame.mSound, 0x45, 255);
		++suits.mChangeStep;
		break;
	case 2:
		if (SuitFadeFinished(player))
			++suits.mChangeStep;
		break;
	case 3:
		if (suits.mCurrent != 0) {
			RemovePlayerSuit(player);
			player->SetColor(2, 255, 255, 255, 0.0f);
		}
		suits.mCurrent  = suits.mRequested;
		suits.mLoadStep = 1;
		++suits.mChangeStep;
		gPlayerSuitChangeTimer = 0.5f * FRAME_RATE;
		// fall through
	case 4:
		gPlayerSuitChangeTimer -= 2.0f;
		if (gPlayerSuitChangeTimer <= 0.0f)
			gPlayerSuitChangeTimer = 0.0f;
		if (!LoadPlayerSuitItem(player) || gPlayerSuitChangeTimer != 0.0f)
			break;
		++suits.mChangeStep;
		// fall through
	case 5: {
		ApplyPlayerSuit(player);
		player->SetColor(2, 255, 255, 255, 0.0f);
		player->SetColor(2, 0, 0, 0, 0.5f);
		const Vec& tint = player->mColorStep[0].mValue;
		u32 color       = ((u32)tint.y << 16) | ((u32)tint.x << 24) | ((u32)tint.z << 8);
		player->SetColor(0, color >> 24, color >> 16, color >> 8, 0.0f);
		++suits.mChangeStep;
		break;
	}
	case 6:
		if (SuitFadeFinished(player))
			suits.mChangeStep = 0;
		break;
	}
}

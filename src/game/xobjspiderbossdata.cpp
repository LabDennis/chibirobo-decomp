#include "game/xobjspiderboss.h"

extern "C" {
static XObjFloor sSpiderBossLowFloor[] = {
	{ 0, 0, { 0, 135, 0 }, 0, { 377, 158, 377 } },
	{ -1 },
};
static XObjFloor sSpiderBossHighFloor[] = {
	{ 0, 0, { 0, 135, 0 }, 0, { 377, 632, 377 } },
	{ -1 },
};

AnimSfxEvent gSpiderBossAnimSfxEvents[] = {
	{ 2, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 8, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 9, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 15, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 14, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 24, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 26, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 25, 1.0f, 0.0f, 1, 0x160019, 0xFF, 0, 0, 0 },
	{ 2, 40.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ 8, 15.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ 9, 15.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ 15, 15.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ 14, 15.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ 24, 20.0f, 0.0f, 1, 0x16001A, 0xFF, 0, 0, 0 },
	{ -1 },
};
}

XObjSpiderBoss::StateFunc gSpiderBossStateHandlers[5] = {
	&XObjSpiderBoss::State0, &XObjSpiderBoss::State1, &XObjSpiderBoss::State2, &XObjSpiderBoss::State3, &XObjSpiderBoss::State4,
};

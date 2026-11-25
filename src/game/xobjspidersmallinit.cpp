#include "game/xobjspidersmall.h"

SmallSpiderStageStatus gSmallSpiderStatusByStage[39];

extern "C" XObjFloor sSmallSpiderLowFloor[] = {
	{ 0, 0, { 0, 135, 0 }, 0, { 377, 158, 377 } },
	{ -1 },
};
extern "C" XObjFloor sSmallSpiderHighFloor[] = {
	{ 0, 0, { 0, 135, 0 }, 0, { 377, 632, 377 } },
	{ -1 },
};
extern "C" XObjFloor* sSmallSpiderFloors[2] = { sSmallSpiderLowFloor, sSmallSpiderHighFloor };
extern "C" {
AnimSfxEvent gSmallSpiderAnimSfxEvents[] = {
	{ 2, 1.0f, 0.0f, 1, 0x137, 0xFF, 0, 0, 0 },
	{ 7, 1.0f, 0.0f, 1, 0x137, 0xFF, 0, 0, 0 },
	{ -1 },
};
}

void (XObjSpiderSmall::*gSmallSpiderStateHandlers[10])(u32) = {
	&XObjSpiderSmall::State0, &XObjSpiderSmall::State1, &XObjSpiderSmall::State2, &XObjSpiderSmall::State3, &XObjSpiderSmall::State4,
	&XObjSpiderSmall::State5, &XObjSpiderSmall::State6, &XObjSpiderSmall::State7, &XObjSpiderSmall::State8, &XObjSpiderSmall::State9,
};

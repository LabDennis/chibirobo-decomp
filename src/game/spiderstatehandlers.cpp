#include "game/xobjspider.h"

static s32 sSpiderStateData[26] = { 0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 3, 0 };
static XObjFloor sSpiderFloor[] = {
	{ 0, 0, { 0, 135, 0 }, 0, { 754, 632, 377 } },
	{ -1 },
};
static XObjFloor sSpiderWideFloor[] = {
	{ 0, 0, { 0, 135, 0 }, 0, { 3016, 632, 377 } },
	{ -1 },
};
static XObjFloor* sSpiderFloors[8] = {
	sSpiderFloor, sSpiderFloor, sSpiderFloor, sSpiderFloor, sSpiderFloor, sSpiderFloor, sSpiderFloor, sSpiderFloor,
};
static f32 sSpiderWeights[2][15] = {
	{ 0, 0.25f, 0.5f, 0.65f, 0.75f, 0.85f, 0.85f, 0.85f, 0.9f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.2f },
	{ 0, 0.4f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.7f, 0.5f, 0 },
};
extern "C" {
AnimSfxEvent gSpiderAnimSfxEvents[] = {
	{ 2, 1.0f, 0.0f, 1, 0x137, 0xFF, 0, 0, 0 },
	{ 12, 1.0f, 0.0f, 1, 0x137, 0xFF, 0, 0, 0 },
	{ -1 },
};
}

// the state handlers by number, Update calls them
XObjSpider::Func gSpiderStateHandlers[10] = {
	&XObjSpider::State0, &XObjSpider::State1, &XObjSpider::State2, &XObjSpider::State3, &XObjSpider::State4,
	&XObjSpider::State5, &XObjSpider::State6, &XObjSpider::State7, &XObjSpider::State8, &XObjSpider::State9,
};

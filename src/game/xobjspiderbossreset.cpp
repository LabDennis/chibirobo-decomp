#include "game/xobjspiderboss.h"

// todo: cleanup
static inline void ResetState(XObjSpiderBoss* spider)
{
	ResetSpiderBossState(spider);
	spider->mStep      = 0;
	spider->mTimer     = 0;
	spider->mPrevState = spider->mState;
	spider->mState     = 0;
	spider->mSpiderStatus.mFlags |= 0x04000000;
}

/**
 * @note Address: 0x801068D4
 * @note Size: 0x50
 */
extern "C" void vf94__14XObjSpiderBossFv(XObjSpiderBoss* spider)
{
	ResetState(spider);
}

/**
 * @note Address: 0x80106924
 * @note Size: 0x58
 */
extern "C" void InitializeObject__14XObjSpiderBossFUl(XObjSpiderBoss* spider, u32 id)
{
	InitializeObject__4XObjFUl(spider, id);
	ResetState(spider);
}

extern "C" {
s32 gSpiderBossStateData[26]           = { 0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 3, 0 };
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

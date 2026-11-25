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
void XObjSpiderBoss::ResetForNewGame()
{
	ResetState(this);
}

/**
 * @note Address: 0x80106924
 * @note Size: 0x58
 */
void XObjSpiderBoss::InitializeObject(u32 id)
{
	XObj::InitializeObject(id);
	ResetState(this);
}

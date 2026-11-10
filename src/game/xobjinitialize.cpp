#include "game/xobj.h"
#include "game/cgame.h"

extern "C" {
void InitializeEventScriptContext(CEvt* event, u8* context, void* owner);
}

/**
 * @note Address: 0x800929C4
 * @note Size: 0xE0
 */
void XObj::InitializeObject(u32 id)
{
	CXObj::Init(id);
	mStatus.mFlags = 0;
	mCoord._00     = NULL;
	mAttr._00      = NULL;
	mAttr._04      = NULL;
	mFloors        = NULL;
	mPoolIndex     = 0;
	ResetObjectState();
	InitializeEventScriptContext(&mEvt, (u8*)gGame.GetMsg(1), this);
	mAnmQueue.Clear();
	mAnim.Clear();
}

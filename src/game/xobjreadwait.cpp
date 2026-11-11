#include "game/xobj.h"
#include "game/xobjread.h"
#include "Dolphin/os.h"

/**
 * @note Address: 0x80131C6C
 * @note Size: 0xC0
 */
// any read still waiting
BOOL XObjRead::HasPendingReads()
{
	for (s32 i = 0; i < 8; ++i) {
		if (mReqs[i].mObj != NULL) {
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x80131D2C
 * @note Size: 0x280
 */
extern "C" BOOL HasPendingObjectReads()
{
	return gAllObjectsReader.HasPendingReads() || gSingleObjectReader.HasPendingReads() || gObjectAnimationReader.HasPendingReads();
}

/**
 * @note Address: 0x80131FAC
 * @note Size: 0x94
 */
// XObjReadAnm::ProcessRequest, reads the object's anim archive
void XObjReadAnm::ProcessRequest(XObjReadReq* req)
{
	XObj* object     = (XObj*)req->mObj;
	const char* path = (const char*)req->mInfo;
	while (!object->LoadAnimationArchiveAsync(path)) {
		OSYieldThread();
	}
	req->mObj = NULL;
	object->mStatus.mFlags &= ~0x0080000000000000ULL;
}

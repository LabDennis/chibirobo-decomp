#include "game/ccameraaction.h"
#include "game/cgame.h"
#include "game/evtqueue.h"

// what ResolveEventObjectId gets as its context
struct EvtContext {
	u32 _00;       // _00
	XObj* mObject; // _04, the one running the script
};

static inline XObj* NamedObject(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; ++i) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			XObj* object = gGame.GetObject(gEventObjectIds.mEntry[i].mIndex);
			if (object != NULL && (object->mStatus.mFlags & 1ULL))
				return object;
			return NULL;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x8007F570
 * @note Size: 0xF4
 */
extern "C" XObj* ResolveEventObjectId(void* context, s16 id)
{
	if (id >= 10000) {
		XObj* object = NamedObject(id);
		if (object != NULL)
			return object;
		return NULL;
	}
	if (id >= 0)
		return gGame.GetObject(id);
	if (id == -1)
		return ((EvtContext*)context)->mObject;
	return (XObj*)(s32)id;
}

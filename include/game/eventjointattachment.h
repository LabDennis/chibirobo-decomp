#ifndef _GAME_EVENTJOINTATTACHMENT_H
#define _GAME_EVENTJOINTATTACHMENT_H

#include "game/xobjjointpart.h"
#include "game/cgame.h"

extern "C" void OSReport(const char*, ...);

typedef s32 EventJointPath[8];

template <class Owner>
static inline void SetEventJointAttachment(Owner& owner, CStatus& status, u32 mask, s32* args, EventJointPath* path = NULL)
{
	const s32 objectId = args[1];
	if (objectId == 0) {
		status.mFlags &= ~mask;
		for (u32 i = 0; i < 8; ++i) {
			XObjJointPart& part = owner.mParts[i];
			if (part.mObj != NULL) {
				HSD_JObjClearMtxFlags(part.mObj->mJObj);
				part.mObj   = NULL;
				part.mJoint = NULL;
			}
		}
	} else {
		u32 i;
		for (i = 0; i < 8 && owner.mParts[i].mObj != NULL; ++i) { }
		if (i < 8) {
#if !defined(VERSION_PAL)
			OSReport("id = %d, argv[1]=%d\n", i, objectId);
#endif
			status.mFlags |= mask;
			if (args[1] >= 10000)
				owner.mParts[i].mObj = FindObj(args[1]);
			else
				owner.mParts[i].mObj = gGame.GetObject((s16)args[1]);
#if !defined(VERSION_PAL)
			if (owner.mParts[i].mObj == NULL) {
				OSReport("NULL===============\n");
			}
#endif

			s32 depth    = args[2];
			s32* indices = args + 3;
			if (path && depth <= (s32)ARRAY_SIZE(*path)) {
				for (s32 j = 0; j < depth; ++j)
					(*path)[j] = args[j + 3];
				indices = *path;
			}
			owner.mParts[i].mJoint = GetJointByPath(owner.mJObj, indices, depth);
			XObjJointPart::UpdateMatrix(owner, i);
		}
	}
}

#endif

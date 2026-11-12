#include "game/xobjactor.h"

#include "game/cgame.h"

extern "C" void OSReport(const char*, ...);

/**
 * @note Address: 0x801070A4
 * @note Size: 0x498
 */
s32 XObjActor::HandleEventCommand(s32 count, s32* args)
{
	switch (args[0]) {
	case 100:
		if (args[1] == 0) {
			_998.mFlags &= ~0x1000;
			for (u32 i = 0; i < 8; i++) {
				if (mParts[i].mObj != NULL) {
					HSD_JObjClearMtxFlags(mParts[i].mObj->mJObj);
					mParts[i].mObj   = NULL;
					mParts[i].mJoint = NULL;
				}
			}
		} else {
			u32 i;
			for (i = 0; mParts[i].mObj != NULL && i < 8; i++) { }
			if (i < 8) {
#if !defined(VERSION_PAL)
				OSReport("id = %d, argv[1]=%d\n", i);
#endif
				_998.mFlags |= 0x1000;
				if (args[1] >= 10000) {
					mParts[i].mObj = FindObj(args[1]);
				} else {
					mParts[i].mObj = gGame.GetObject((s16)args[1]);
				}
#if !defined(VERSION_PAL)
				if (mParts[i].mObj == NULL) {
					OSReport("NULL===============\n");
				}
#endif
				s32 path[8];
				s32 depth = args[2];
				for (s32 j = 0; j < depth; j++) {
					path[j] = args[3 + j];
				}
				mParts[i].mJoint = GetJointByPath(mJObj, path, depth);
				XObjJointPart::UpdateMatrix(*this, i);
			}
		}
		break;
	case 101:
		if (args[1] == 0) {
			_998.mFlags &= ~0x800;
		} else {
			_998.mFlags |= 0x800;
		}
		break;
	}
	return 0;
}

#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"

extern "C" void SetFafaPartVisibility(s32 mode)
{
	for (s32 i = 0; i < 15; i++) {
		XObj* item = gGame.mXObjs.mPools[10].Get(i);
		if (item->GetKind() == 0x117) {
			return;
		}
	}
}
#else
#include "Version.h"
#include "game/cgame.h"
#include "game/cjobj.h"

// linked into both shun and stage07

inline HSD_JObj* FafaFindPart(HSD_JObj* root, s32* path)
{
	return FindJointByPath(root, path, 3);
}

/**
 * @note Address: shun .text 0x1FB8
 * @note Size: 0x164C
 */
extern "C" void SetFafaPartVisibility(s32 mode)
{
	CJObj obj;
	s32 i;

	for (i = 0; i < 15; i++) {
		XObj* item = gGame.mXObjs.mPools[10].Get(i);
		if (item->GetKind() != 0x117) {
			continue;
		}
		switch (mode) { // mode 4, 7 or 6 shows part 2, 3 or 4; any other mode hides all three
		case 4: {
			s32 path[3] = { 0, 0, 2 };
			obj.Attach(FafaFindPart(item->mJObj, path));
			HSD_JObjClearFlags(obj.mJObj, HIDDEN);
		}
			{
				s32 path[3] = { 0, 0, 3 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, HIDDEN);
			}
			{
				s32 path[3] = { 0, 0, 4 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, HIDDEN);
			}
			break;
		case 7: {
			s32 path[3] = { 0, 0, 2 };
			obj.Attach(FafaFindPart(item->mJObj, path));
			HSD_JObjSetFlags(obj.mJObj, HIDDEN);
		}
			{
				s32 path[3] = { 0, 0, 3 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjClearFlags(obj.mJObj, HIDDEN);
			}
			{
				s32 path[3] = { 0, 0, 4 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, HIDDEN);
			}
			break;
		case 6: {
			s32 path[3] = { 0, 0, 2 };
			obj.Attach(FafaFindPart(item->mJObj, path));
			HSD_JObjSetFlags(obj.mJObj, HIDDEN);
		}
			{
				s32 path[3] = { 0, 0, 3 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, HIDDEN);
			}
			{
				s32 path[3] = { 0, 0, 4 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjClearFlags(obj.mJObj, HIDDEN);
			}
			break;
		default: {
			s32 path[3] = { 0, 0, 2 };
			obj.Attach(FafaFindPart(item->mJObj, path));
			HSD_JObjSetFlags(obj.mJObj, HIDDEN);
		}
			{
				s32 path[3] = { 0, 0, 3 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, HIDDEN);
			}
			{
				s32 path[3] = { 0, 0, 4 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, HIDDEN);
			}
			break;
		}
		break; // only the first 0x117 item
	}
}
#endif

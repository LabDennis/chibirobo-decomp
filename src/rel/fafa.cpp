#include "game/hsd_jobj.h"
#include "game/cgame.h"

// linked into both shun and stage07

// three levels, child then N siblings at each
// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline HSD_JObj* FafaFindPart(HSD_JObj* root, s32* path)
{
	HSD_JObj* jobj = root;
	s32 i;
	s32 j;

	if (jobj == NULL) {
		return NULL;
	}
	for (i = 0; i < 3; i++, path++) {
		jobj = HSD_JObjGetChild(jobj);
		for (j = 0; j < *path; j++) {
			jobj = HSD_JObjGetNext(jobj);
			if (jobj == NULL) {
				return NULL;
			}
		}
	}
	return jobj;
}

/**
 * @note Address: shun .text 0x1FB8
 * @note Size: 0x164C
 */
extern "C" void SetFafaPartVisibility(s32 mode)
{
	CJObj obj;
	const XObjPool& items = gGame.mXObjs.mPools[10];
	s32 i;

	for (i = 0; i < 15; i++) {
		XObj* item = items.Get(i);
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

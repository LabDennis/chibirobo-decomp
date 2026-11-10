#include "game/hsd_jobj.h"
#include "game/ut.h"

/**
 * @note Address: 0x801908F8
 * @note Size: 0x1B8
 */
extern "C" HSD_AObj* FindJointAnimationInTree(HSD_JObj* jobj)
{
	HSD_AObj* aobj = NULL;
	for (; jobj != NULL; jobj = jobj->child) {
		if ((aobj = jobj->aobj) != NULL) {
			break;
		}
		if ((aobj = FindJointAnimationInTree(jobj->next)) != NULL) {
			break;
		}
	}
	return aobj;
}

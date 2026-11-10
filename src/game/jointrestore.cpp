#include "game/jointposeassert.h"
#include "game/ut.h"

extern "C" {
// puts the joint's rotation, scale and position back on the jobj
void RestoreJointLocalPose(HSD_Joint* joint, HSD_JObj* jobj)
{
	if (jobj == NULL || joint == NULL) {
		return;
	}
	jobj->rotate.x  = joint->rotation.x;
	jobj->rotate.y  = joint->rotation.y;
	jobj->rotate.z  = joint->rotation.z;
	jobj->scale     = joint->scale;
	jobj->translate = joint->position;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
	jobj->flags &= ~USE_QUATERNION;
}
}

/**
 * @note Address: 0x8018F9B4
 * @note Size: 0x27C
 */
// back to the joint's own pose, the whole tree
void RestoreJointTreePose(HSD_Joint* joint, HSD_JObj* jobj)
{
	HSD_JObj* child;
	HSD_Joint* cjoint;

	if (jobj == NULL || joint == NULL) {
		return;
	}
	RestoreJointLocalPose(joint, jobj);
	if (jobj->flags & INSTANCE) {
		return;
	}
	for (child = jobj->child, cjoint = joint->child; child != NULL && cjoint != NULL;
	     child = child->next, cjoint = cjoint != NULL ? cjoint->next : NULL) {
		if (child == NULL || cjoint == NULL) {
			break;
		}
		RestoreJointTreePose(cjoint, child);
	}
}

#include "game/jointposeassert.h"
#include "game/ut.h"
extern "C" {
#include <sysdolphin/mtx.h>
}

extern "C" {
// turns the euler rotation into a quaternion
void ConvertJointRotationToQuaternion(HSD_JObj* jobj)
{
	if (!(jobj->flags & USE_QUATERNION)) {
		Vec euler;
		Quaternion q;
		// reads four floats into a three float vec, w lands past it. retail does this too
		HSD_JObjGetRotation(jobj, (Quaternion*)&euler);
		EulerToQuat(&euler, &q);
		HSD_JObjSetRotation(jobj, &q);
		jobj->flags |= USE_QUATERNION;
	}
}
}

/**
 * @note Address: 0x8018FD80
 * @note Size: 0x2A4
 */
// every jobj in the tree to quaternion rotation
void ConvertJointTreeRotationToQuaternion(HSD_JObj* jobj)
{
	HSD_JObj* child;

	ConvertJointRotationToQuaternion(jobj);
	if (jobj->flags & INSTANCE) {
		return;
	}
	for (child = jobj->child; child != NULL; child = child->next) {
		ConvertJointTreeRotationToQuaternion(child);
	}
}

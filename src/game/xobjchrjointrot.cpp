#include "game/cobj.h"
#include "game/cjobjinlineattach.h"

/**
 * @note Address: 0x800BB3D0
 * @note Size: 0x338
 */
extern "C" void SetJointYawByPath(CXObj* object, s32 count, s32* path, f32 angle)
{
	CJObj joint(GetJointByPath(object->mJObj, path, count));
	joint.SetRotationY(angle, 1);
}

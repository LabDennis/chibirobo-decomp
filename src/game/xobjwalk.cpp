#include "game/xobj.h"

// XObj::vfD8, the flags go to ApplyJointMaterialPEPresets on every joint
/**
 * @note Address: 0x80091C44
 * @note Size: 0x3C
 */
extern "C" void vfD8__4XObjFPUl(XObj* object, u32* flags)
{
	if (object->mJObj != NULL)
		HSD_JObjWalkTree(object->mJObj, ApplyJointMaterialPEPresets, (f32**)flags);
}

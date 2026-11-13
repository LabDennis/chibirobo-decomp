#include "game/xobj.h"

// XObj::vfD8, the flags go to ApplyJointMaterialPEPresets on every joint
/**
 * @note Address: 0x80091C44
 * @note Size: 0x3C
 */
void XObj::vfD8(u32* flags)
{
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, ApplyJointMaterialPEPresets, (f32**)flags);
}

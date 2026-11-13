#include "game/xobj.h"

// XObj::ApplyMaterialPresets, the flags go to ApplyJointMaterialPEPresets on every joint
/**
 * @note Address: 0x80091C44
 * @note Size: 0x3C
 */
void XObj::ApplyMaterialPresets(u32* flags)
{
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, ApplyJointMaterialPEPresets, (f32**)flags);
}

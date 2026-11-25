#include "game/xobjattachment.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/cvec.h"

/**
 * @note Address: 0x800DC43C
 * @note Size: 0x1F0
 */
extern "C" void StartPlugCordNode(XObjPlugCordNode* node, XObjPlugCordPoint* points, s32 count, f32 time, s32 mode)
{
	node->mMode = mode;
	if (mode != 1 && mode != -1)
		return;

	const s32 index = mode == 1 ? count - 2 : 0;
	const s32 next  = index - mode;
	node->mIndex    = index;
	node->mPos      = points[index].mPos;
	node->mStep.Start(0.5f, 0.2f, time);
	node->mDir = points[next].mPos - points[index].mPos;
	node->mDir.Normalize();
	node->mState    = 1;
	node->mDistance = 0.0f;
}

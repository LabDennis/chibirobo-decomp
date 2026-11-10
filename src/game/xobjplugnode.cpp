#include "game/xobjattachment.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/cvec.h"

extern "C" {
// Keep the shared constant pool at its original addresses.
extern const f32 lbl_8065D968;
}

// todo: cleanup
static inline void CopyPosition(XObjPlugCordNode* node, XObjPlugCordPoint* point)
{
	node->mPos.x = point->mPos.x;
	node->mPos.y = point->mPos.y;
	node->mPos.z = point->mPos.z;
}

/**
 * @note Address: 0x800DC43C
 * @note Size: 0x1F0
 */
extern "C" void StartPlugCordNode(XObjPlugCordNode* node, XObjPlugCordPoint* points, s32 count, f32 time, s32 mode)
{
	node->mMode = mode;
	switch (node->mMode) {
	case 1:
		node->mIndex = (s32)((u32)count - 2);
		CopyPosition(node, &points[node->mIndex]);
		node->mStep.Start(lbl_8065D968, sPlugCordDefaultWidth, time);
		node->mDir = points[node->mIndex - 1].mPos - points[node->mIndex].mPos;
		node->mDir.Normalize();
		node->mState    = 1;
		node->mDistance = sPlugCordZero;
		break;
	case -1:
		node->mIndex = 0;
		CopyPosition(node, &points[node->mIndex]);
		node->mStep.Start(lbl_8065D968, sPlugCordDefaultWidth, time);
		node->mDir = points[node->mIndex + 1].mPos - points[node->mIndex].mPos;
		node->mDir.Normalize();
		node->mState    = 1;
		node->mDistance = sPlugCordZero;
		break;
	}
}

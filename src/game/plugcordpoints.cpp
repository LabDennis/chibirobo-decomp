#include "game/xobjattachment.h"
#include "game/cvec.h"

extern "C" void UpdatePlugCordSpark(XObjPlugCordNode* node, XObjPlugCordPoint* points, s32 count, f32)
{
	if (!node->mState || (node->mMode != 1 && node->mMode != -1))
		return;

	const bool backward = node->mMode == 1;
	const s32 direction = backward ? -1 : 1;
	const s32 next      = node->mIndex + direction;
	node->mDir          = points[next].mPos - points[node->mIndex].mPos;
	node->mDir.Normalize();
	node->mPos         = points[node->mIndex].mPos + node->mDir * node->mDistance;
	const f32 distance = PSVECDistance(&node->mPos, &points[next].mPos);
	const f32 step     = node->mStep.Get();
	if (distance > step) {
		node->mPos += node->mDir * step;
		node->mDistance += step;
	} else {
		node->mIndex = next;
		if (backward && next == 0) {
			node->mState    = 0;
			node->mPos      = points[0].mPos;
			node->mDistance = 0.0f;
			node->mStep.Step(1.0f);
			return;
		}
		if (!backward && next == count - 2)
			node->mState = 0;
		node->mDir = points[next + direction].mPos - points[next].mPos;
		node->mDir.Normalize();
		node->mDistance = step - distance;
		node->mPos      = points[next].mPos + node->mDir * node->mDistance;
		node->mStep.Step(1.0f);
	}
#if !defined(VERSION_JP)
	if (backward && node->mIndex == 1 && PSVECDistance(&node->mPos, &points[0].mPos) > PSVECDistance(&points[1].mPos, &points[0].mPos))
		node->mState = 0;
#endif
}

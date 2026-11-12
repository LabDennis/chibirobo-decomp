#include "game/xobjattachment.h"
#include "game/cvec.h"

static inline void AddStep(CVec& position, Vec step)
{
	position += step;
}

extern "C" void UpdatePlugCordSpark(XObjPlugCordNode* node, XObjPlugCordPoint* points, s32 count, f32)
{
	if (!node->mState)
		return;
	switch (node->mMode) {
	case 1: {
		{
			node->mDir = points[node->mIndex - 1].mPos - points[node->mIndex].mPos;
			node->mDir.Normalize();
			node->mPos    = points[node->mIndex].mPos + node->mDir * node->mDistance;
			f32 remaining = PSVECDistance(&node->mPos, &points[node->mIndex - 1].mPos);
			if (remaining > node->mStep.Get()) {
				AddStep(node->mPos, node->mDir * node->mStep.Get());
				node->mDistance += node->mStep.Get();
			} else {
				if (node->mIndex - 1 == 0)
					node->mState = 0;
				node->mIndex -= 1;
				node->mDir = points[node->mIndex - 1].mPos - points[node->mIndex].mPos;
				node->mDir.Normalize();
				node->mPos      = points[node->mIndex].mPos + node->mDir * (node->mStep.Get() - remaining);
				node->mDistance = node->mStep.Get() - remaining;
				node->mStep.Step(1.0f);
			}
		}
#if !defined(VERSION_JP)
		f32 remaining = PSVECDistance(&node->mPos, &points[node->mIndex - 1].mPos);
		if (remaining > PSVECDistance(&points[node->mIndex].mPos, &points[node->mIndex - 1].mPos) && node->mIndex - 1 == 0)
			node->mState = 0;
#endif
		break;
	}
	case -1: {
		node->mDir = points[node->mIndex + 1].mPos - points[node->mIndex].mPos;
		node->mDir.Normalize();
		node->mPos    = points[node->mIndex].mPos + node->mDir * node->mDistance;
		f32 remaining = PSVECDistance(&node->mPos, &points[node->mIndex + 1].mPos);
		if (remaining > node->mStep.Get()) {
			AddStep(node->mPos, node->mDir * node->mStep.Get());
			node->mDistance += node->mStep.Get();
		} else {
			if (node->mIndex + 1 == count - 2)
				node->mState = 0;
			node->mIndex += 1;
			node->mDir = points[node->mIndex + 1].mPos - points[node->mIndex].mPos;
			node->mDir.Normalize();
			node->mPos      = points[node->mIndex].mPos + node->mDir * (node->mStep.Get() - remaining);
			node->mDistance = node->mStep.Get() - remaining;
			node->mStep.Step(1.0f);
		}
	} break;
	}
}

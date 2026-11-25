#include "game/xobjattachment.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/cvec.h"
#include <math.h>

// coils the cord up around a point
/**
 * @note Address: 0x800DB810
 * @note Size: 0x26C
 */
extern "C" void ResetPlugCordPoints(XObjPlugCord* cord, f32 x, f32 y, f32 z)
{
	Vec center = { x, y, z };
	for (s32 i = 0; i < cord->mCount; ++i) {
		f32 angle                = (3.1415f * i) / 8.0f;
		f32 sine                 = (f32)sin(angle);
		f32 cosine               = (f32)cos(angle);
		XObjPlugCordPoint& point = cord->mPoints[i];
		point.mPos.Set(5.0f * cosine, -10.0f, 5.0f * sine);
		point.mPos += center;
		point.mTarget = point.mPos;
		point.mSpeed.Set(0.0f, 0.0f, 0.0f);
	}
	cord->mEnd[0]       = center;
	cord->mEnd[1]       = cord->mPoints[cord->mCount - 1].mPos;
	cord->mMode         = 0;
	cord->mNodes.mColor = 0;
	cord->mNodes.mMode  = 0;
	cord->mNodes.mCount = 0;
	for (u32 i = 0; i < ARRAY_SIZE(cord->mNodes.mNode); ++i)
		cord->mNodes.mNode[i].mState = 0;
	cord->mNodes.mEnabled = 0;
}

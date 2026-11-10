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
	Vec center = CVec(x, y, z);
	for (s32 i = 0; i < cord->mCount; ++i) {
		f32 angle  = (3.1415f * i) / 8.0f;
		f32 sine   = (f32)sin(angle);
		f32 cosine = (f32)cos(angle);
		cord->mPoints[i].mPos.Set(CVec(5.0f * cosine, -10.0f, 5.0f * sine));
		Vec offset = center;
		cord->mPoints[i].mPos += offset;
		XObjPlugCordPoint& point = cord->mPoints[i];
		point.mTarget.x          = point.mPos.x;
		point.mTarget.y          = point.mPos.y;
		point.mTarget.z          = point.mPos.z;
		cord->mPoints[i].mSpeed  = CVec(0.0f, 0.0f, 0.0f);
	}
	cord->mEnd[0]                = center;
	cord->mEnd[1]                = cord->mPoints[cord->mCount - 1].mPos;
	cord->mMode                  = 0;
	cord->mNodes.mColor          = 0;
	cord->mNodes.mMode           = 0;
	cord->mNodes.mCount          = 0;
	cord->mNodes.mNode[0].mState = 0;
	cord->mNodes.mNode[1].mState = 0;
	cord->mNodes.mNode[2].mState = 0;
	cord->mNodes.mNode[3].mState = 0;
	cord->mNodes.mNode[4].mState = 0;
	cord->mNodes.mNode[5].mState = 0;
	cord->mNodes.mNode[6].mState = 0;
	cord->mNodes.mNode[7].mState = 0;
	cord->mNodes.mNode[8].mState = 0;
	cord->mNodes.mNode[9].mState = 0;
	cord->mNodes.mEnabled        = 0;
}

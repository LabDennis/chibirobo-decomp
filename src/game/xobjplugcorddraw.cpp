#include "game/xobjattachment.h"
#include "game/texturecopy.h"
#include "game/ccamera.h"
extern "C" {
#include <sysdolphin/mtx_transform.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
}
#include <math.h>

// draws the cord a segment at a time
/**
 * @note Address: 0x800DA68C
 * @note Size: 0x354
 */
extern "C" void DrawPlugCordSegments(XObjPlugCord* cord, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ)
{
	CGs* graphics = CaptureGraphics();
	SetGsColorUpdate(graphics->mCurRenderMode, cord->mFlags & 1);
	SetGsAlphaUpdate(graphics->mCurRenderMode, (cord->mFlags >> 1) & 1);
	SetGsDestinationAlpha(graphics->mCurRenderMode, (cord->mFlags >> 2) & 1, cord->mAlpha);
	SetGsDepthMode(graphics->mCurRenderMode, 1, 3, 1);
	SetGsBlendMode(graphics->mCurRenderMode, 1, 4, 5, 15);
	FlushCaptureGraphics(graphics);
	f32(*selectedView)[4];
	if (view == NULL)
		selectedView = gGame.mGs.mCamera->mViewMtx;
	else
		selectedView = view;
	Mtx combined, local;
	Vec scale, rotation, position;
	s32 i   = 0;
	scale.x = scale.y = cord->mWidth;
	for (; i < cord->mCount - 2; i++) {
		CVec* current = &cord->mPoints[i].mPos;
		CVec* next    = &cord->mPoints[i + 1].mPos;
		f32 x         = next->x - current->x;
		f32 y         = next->y - current->y;
		f32 z         = next->z - current->z;
		rotation.x    = atan2(-y, sqrtf(x * x + z * z));
		rotation.y    = atan2(x, z);
		rotation.z    = 0.0f;
		position.x    = 0.5f * (current->x + next->x);
		position.y    = 0.5f * (current->y + next->y);
		position.z    = 0.5f * (current->z + next->z);
		scale.z       = 0.5f * PSVECDistance(current, next);
		HSD_MtxSRT(local, &scale, &rotation, &position, NULL);
		PSMTXConcat(selectedView, local, combined);
		DrawGsCylinderWithMatrix(&gGame.mGs, combined, cord->mColor, 6);
	}
	scale.z = 0.9f * cord->mWidth;
}

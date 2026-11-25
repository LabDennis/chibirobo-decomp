#include "game/xobjattachment.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/ccamera.h"
extern "C" {
#include <sysdolphin/mtx_transform.h>
#include <dolphin/mtx/mtx.h>
}

/**
 * @note Address: 0x800DA4D0
 * @note Size: 0x1BC
 */
extern "C" void DrawPlugCordNodes(XObjPlugCord* cord, Mtx view, f32, f32, f32)
{
	MtxPtr selectedView = view == NULL ? gGame.mGs.mCamera->mViewMtx : view;
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);

	XObjPlugCordNodes& nodes = cord->mNodes;
	if (nodes.mEnabled != TRUE)
		return;

	u32 count = nodes.mCount;
	if (count > ARRAY_SIZE(nodes.mNode))
		count = ARRAY_SIZE(nodes.mNode);

	Vec scale    = { 0.7f, 0.7f, 0.7f };
	Vec rotation = { 0.0f, 0.0f, 0.0f };
	for (u32 i = 0; i < count; ++i) {
		XObjPlugCordNode& node = nodes.mNode[i];
		if (node.mState == 0)
			continue;

		Mtx combined;
		Mtx local;
		HSD_MtxSRT(local, &scale, &rotation, &node.mPos, NULL);
		PSMTXConcat(selectedView, local, combined);

		// Each successive spark uses half the intensity of the previous one.
		u32 color = (nodes.mColor & 0xFFFFFF00) | (300U >> i);
		DrawGsSphereWithMatrix(&gGame.mGs, combined, color, 8, 8);
	}
}

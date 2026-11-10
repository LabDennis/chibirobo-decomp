#include "game/xobjattachment.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/ccamera.h"
extern "C" {
#include <sysdolphin/mtx_transform.h>
#include <dolphin/mtx/mtx.h>
}
#include <math.h>

// draws the nodes, each one half as bright as the one before
/**
 * @note Address: 0x800DA4D0
 * @note Size: 0x1BC
 */
extern "C" void DrawPlugCordNodes(XObjPlugCord* cord, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ)
{
	Mtx combined;
	Mtx local;
	MtxPtr selectedView = view == NULL ? gGame.mGs.mCamera->mViewMtx : view;
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
	if (cord->mNodes.mEnabled == 1) {
		XObjPlugCordNode* node = cord->mNodes.mNode;
		for (s32 i = 0; i < cord->mNodes.mCount; i++, node++) {
			if (cord->mNodes.mNode[i].mState != 0) {
				Vec scale    = CVec(0.7f, 0.7f, 0.7f);
				Vec rotation = CVec(0.0f, 0.0f, 0.0f);
				HSD_MtxSRT(local, &scale, &rotation, &node->mPos, NULL);
				PSMTXConcat(selectedView, local, combined);
				DrawGsSphereWithMatrix(&gGame.mGs, combined, (cord->mNodes.mColor & 0xFFFFFF00) | (s32)(300.0 / pow(2.0, i)), 8, 8);
			}
		}
	}
}

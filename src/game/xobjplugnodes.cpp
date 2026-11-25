#include "game/xobjattachment.h"
#include "game/xchr.h"
#include "game/cgame.h"

/**
 * @note Address: 0x800DBC98
 * @note Size: 0x144
 */
extern "C" void UpdatePlugCordNodes(XObjPlugCordNodes* nodes, XObjPlugCordPoint* points, s32 count, u32)
{
	if (nodes->mEnabled) {
		if (nodes->mCount > 0 && nodes->mCount <= ARRAY_SIZE(nodes->mNode) && nodes->mNode[nodes->mCount - 1].mState == 0) {
			for (u32 i = 0; i < nodes->mCount; ++i) {
				if (nodes->mNode[i].mState == 0) {
					StartPlugCordNode(&nodes->mNode[i], points, count, 20.0f, nodes->mMode);
					break;
				}
			}
		}
	} else {
		for (u32 i = 0; i < ARRAY_SIZE(nodes->mNode); ++i)
			nodes->mNode[i].mState = 0;
	}

	for (u32 i = 0; i < ARRAY_SIZE(nodes->mNode); ++i) {
		if (nodes->mNode[i].mState == 1) {
			UpdatePlugCordSpark(&nodes->mNode[i], points, count, 0.0f);
		}
	}
	if (nodes->mNode[0].mState == 1) {
		gGame.GetPlayer()->mPlug.mWire.SetPos(nodes->mNode[0].mPos, 0);
	}
}

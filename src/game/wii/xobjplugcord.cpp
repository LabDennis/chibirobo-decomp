#include "game/xobjattachment.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/cvec.h"
#include <math.h>

// the Wii build keeps the whole cord implementation in one unit: the nodes and the cord
// share one literal pool (InitPlugCord loads its constants from the same .sdata2 block
// as StartPlugCordNode, retail 0x805A5390 on)

extern "C" {
// the Wii build's SRT-matrix helper, it takes no parent scale where the GameCube's
// HSD_MtxSRT takes a fifth
void fn_802A5A84(Mtx m, Vec* scale, Vec* rotation, Vec* translation);
}

// todo: cleanup
static inline void CopyPosition(XObjPlugCordNode* node, XObjPlugCordPoint* point)
{
	node->mPos.x = point->mPos.x;
	node->mPos.y = point->mPos.y;
	node->mPos.z = point->mPos.z;
}

/**
 * @note Address: 0x80110C2C
 * @note Size: 0x1C4
 */
extern "C" void StartPlugCordNode(XObjPlugCordNode* node, XObjPlugCordPoint* points, s32 count, f32 time, s32 mode)
{
	node->mMode = mode;
	switch (node->mMode) {
	case 1:
		node->mIndex = (s32)((u32)count - 2);
		CopyPosition(node, &points[node->mIndex]);
		node->mStep.Start(0.5f, 0.2f, time);
		node->mDir = points[node->mIndex - 1].mPos - points[node->mIndex].mPos;
		node->mDir.Normalize();
		node->mState    = 1;
		node->mDistance = 0.0f;
		break;
	case -1:
		node->mIndex = 0;
		CopyPosition(node, &points[node->mIndex]);
		node->mStep.Start(0.5f, 0.2f, time);
		node->mDir = points[node->mIndex + 1].mPos - points[node->mIndex].mPos;
		node->mDir.Normalize();
		node->mState    = 1;
		node->mDistance = 0.0f;
		break;
	}
}

static inline void AddStep(CVec& position, Vec step)
{
	position += step;
}

/**
 * @note Address: 0x80110DF0
 * @note Size: 0x600
 */
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
		f32 remaining = PSVECDistance(&node->mPos, &points[node->mIndex - 1].mPos);
		if (remaining > PSVECDistance(&points[node->mIndex].mPos, &points[node->mIndex - 1].mPos) && node->mIndex - 1 == 0)
			node->mState = 0;
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

/**
 * @note Address: 0x801113F0
 * @note Size: 0x144
 */
extern "C" void UpdatePlugCordNodes(XObjPlugCordNodes* nodes, XObjPlugCordPoint* points, s32 count, u32)
{
	if (nodes->mEnabled) {
		if (nodes->mNode[nodes->mCount - 1].mState == 0) {
			for (u32 i = 0; i < nodes->mCount; ++i) {
				if (nodes->mNode[i].mState == 0) {
					StartPlugCordNode(&nodes->mNode[i], points, count, 20.0f, (s32)nodes->mMode);
					break;
				}
			}
		}
	} else {
		nodes->mNode[0].mState = 0;
		nodes->mNode[1].mState = 0;
		nodes->mNode[2].mState = 0;
		nodes->mNode[3].mState = 0;
		nodes->mNode[4].mState = 0;
		nodes->mNode[5].mState = 0;
		nodes->mNode[6].mState = 0;
		nodes->mNode[7].mState = 0;
		nodes->mNode[8].mState = 0;
		nodes->mNode[9].mState = 0;
	}

	for (s32 i = 0; i < 10; ++i) {
		if (nodes->mNode[i].mState == 1) {
			UpdatePlugCordSpark(&nodes->mNode[i], points, count, 0.0f);
		}
	}
	// the GameCube calls the plug's XObjWireEff through XObj::SetPos, the Wii the wire
	// object embedded in the player
	if (nodes->mNode[0].mState == 1) {
		gGame.GetPlayer()->mCordWire.SetPosition(&nodes->mNode[0].mPos);
	}
}

// draws the nodes, each one half as bright as the one before; the Wii caller sets the
// GX state up itself, so this takes the node block where the GameCube took the cord
/**
 * @note Address: 0x80111534
 * @note Size: 0x190
 */
extern "C" void DrawPlugCordNodes(XObjPlugCordNodes* nodes, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ)
{
	Mtx combined;
	Mtx local;
	if (nodes->mEnabled == 1) {
		XObjPlugCordNode* node = nodes->mNode;
		for (s32 i = 0; i < nodes->mCount; i++, node++) {
			if (node->mState != 0) {
				Vec rotation = CVec(0.0f, 0.0f, 0.0f);
				Vec scale    = CVec(0.7f, 0.7f, 0.7f);
				fn_802A5A84(local, &scale, &rotation, &node->mPos);
				PSMTXConcat(view, local, combined);
				DrawGsSphereWithMatrix(&gGame.mGs, combined, (nodes->mColor & 0xFFFFFF00) | (s32)(300.0 / pow(2.0, i)), 8, 8);
			}
		}
	}
}

/**
 * @note Address: 0x801116C4
 * @note Size: 0x13C
 */
extern "C" void InitPlugCord(XObjPlugCord* cord, s32 count, f32 x, f32 y, f32 z, f32 speed)
{
	cord->mCount = count;
	static XObjPlugCordPoint gPlugCordPoints[128];
	cord->mPoints = gPlugCordPoints;
	ResetPlugCordPoints(cord, x, y, z);
	cord->mSpeed        = speed;
	cord->mEndSource[0] = NULL;
	cord->mEndSource[1] = NULL;
	cord->mEndSource[2] = NULL;
	cord->_3C           = 0.0f;
	cord->_40           = 0.0f;
	cord->mEndHeight[0] = 0;
	cord->mEndHeight[1] = 0;
	cord->_54           = 6;
	cord->mWidth        = 0.2f;
	cord->mColor = cord->_60 = 255;
	cord->mAlpha             = 0;
	cord->mFlags             = 0;
	cord->mFlags |= 1;
	cord->mFlags &= ~8;
}

XObjPlugCordPoint::XObjPlugCordPoint()
{
}

XObjPlugCordPoint::~XObjPlugCordPoint()
{
}

// coils the cord up around a point
/**
 * @note Address: 0x80111874
 * @note Size: 0x268
 */
extern "C" void ResetPlugCordPoints(XObjPlugCord* cord, f32 x, f32 y, f32 z)
{
	Vec center = CVec(x, y, z);
	for (s32 i = 0; i < cord->mCount; ++i) {
		f32 angle = (3.1415f * i) / 8.0f;
		cord->mPoints[i].mPos.Set(CVec(5.0f * (f32)cos(angle), -10.0f, 5.0f * (f32)sin(angle)));
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

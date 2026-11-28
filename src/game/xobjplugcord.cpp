#include "game/xobjattachment.h"
#include "game/xchr.h"
#include "game/cgame.h"
#include "game/gmath.h"
#include "game/cvec.h"
#include "game/texturecopy.h"
#include "game/ccamera.h"
#include "game/cgs.h"
#include "game/objectfloorquery.h"

/**
 * @note Address: 0x800DBA7C
 * @note Size: 0x150
 */
extern "C" void InitPlugCord(XObjPlugCord* cord, s32 count, f32 x, f32 y, f32 z, f32 speed)
{
	cord->mCount = count;
	static XObjPlugCordPoint points[128];
	cord->mPoints = points;
	ResetPlugCordPoints(cord, x, y, z);
	cord->mSpeed        = speed;
	cord->mEndSource[0] = NULL;
	cord->mEndSource[1] = NULL;
	cord->mEndSource[2] = NULL;
	cord->_3C           = 0.0f;
	cord->_40           = 0.0f;
	cord->mEndHeight[0] = NULL;
	cord->mEndHeight[1] = NULL;
	cord->_54           = 6;
	cord->mWidth        = 0.2f;
	cord->mColor        = 255;
	cord->_60           = 255;
	cord->mAlpha        = 0;
	cord->mFlags        = XObjPlugCord::UpdateColor;
}

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

/**
 * @note Address: 0x800DB7A8
 * @note Size: 0x30
 */
extern "C" void SetPlugCordAlphaMode(XObjPlugCord* cord, BOOL enabled, u8 alpha)
{
	if (enabled) {
		cord->mFlags |= XObjPlugCord::AlphaOutput;
		cord->mAlpha = alpha;
	} else {
		cord->mFlags &= ~XObjPlugCord::AlphaOutput;
		cord->mAlpha = alpha;
	}
}

// ties one cord end to a position
/**
 * @note Address: 0x800DB7D8
 * @note Size: 0x38
 */
extern "C" void SetPlugCordEndpointSource(XObjPlugCord* cord, u32 index, const Vec* source)
{
	cord->mEndSource[index] = source;
	if (source != NULL) {
		cord->mEnd[index].x = source->x;
		cord->mEnd[index].y = source->y;
		cord->mEnd[index].z = source->z;
	}
}

extern "C" {
void ClearWireEffect(XObjWireEff* wire);
void ConfigurePlugWireEffect(XObjWireEff* wire, s32 anim, s32 effect, f32 rate, u32 color);
}

static inline void PinTail(XObjPlugCord* cord, XObjPlugCordPoint* points, s32 end)
{
	XObjPlugCordPoint& point = points[cord->mCount - 3 + end];
	if (cord->mEndSource[end])
		point.mPos.Set(*cord->mEndSource[end]);
	else if (cord->mEndHeight[end])
		point.mPos.y = *cord->mEndHeight[end];
}
static inline void PinCordEnds(XObjPlugCord* cord, XObjPlugCordPoint* points)
{
	if (cord->mEndSource[0]) {
		points[0].mPos.Set(*cord->mEndSource[0]);
		points[0].mTarget = points[0].mPos;
	} else if (cord->mEndHeight[0]) {
		points[0].mPos.y = points[0].mTarget.y = *cord->mEndHeight[0];
	}
	PinTail(cord, points, 1);
	PinTail(cord, points, 2);
}

static inline void ResetSparks(XObjPlugCordNodes& nodes, u32 color, s32 mode, s32 count, s32 enabled)
{
	nodes.mColor = color;
	nodes.mMode  = mode;
	nodes.mCount = count;
	for (u32 i = 0; i < ARRAY_SIZE(nodes.mNode); ++i)
		nodes.mNode[i].mState = 0;
	nodes.mEnabled = enabled;
}

extern "C" void UpdatePlugCordSimulation(XObjPlugCord* cord, u32 frames)
{
	XObjPlugCordPoint* points = cord->mPoints;
	Vec probe                 = GetPlugCordPointPosition(cord, 18);
	probe.y                   = gGame.GetPlayer()->mMoveTranslate.y;
	f32 floorHeight           = FindPlugCordFloor(cord, &probe, -1, 0);
	Vec anchor;
	GetPlayerPlugCordAnchorTransform(&gGame.GetPlayer()->mPlug, &anchor, NULL, NULL);
	if (floorHeight > anchor.y)
		floorHeight = gGame.GetPlayer()->mFloorHit.mHeight;
	const f32 bounceHeight = 0.2f + floorHeight;
	for (u32 frame = 0; frame < frames; ++frame) {
		const f32 gravity = 0.25f;
		for (s32 i = 0; i < cord->mCount; ++i) {
			if (i < 4)
				points[i].mPos.y += 0.0049f; // the near ends drift up
			else if (i < 10)
				points[i].mPos.y += 0.98f * -gravity * 0.5f; // half gravity on the middle
			else
				points[i].mPos.y += 0.98f * -gravity;
		}
		for (s32 pass = 0; pass < 10; ++pass) { // ten relaxation passes
			PinCordEnds(cord, points);
			for (s32 i = 0; i < cord->mCount - 1; ++i) {
				CVec& a        = points[i].mPos;
				CVec& b        = points[i + 1].mPos;
				CVec delta     = b - a;
				f32 distance   = PSVECDistance(&b, &a);
				bool pinned    = i == 0 && cord->mEndSource[0];
				f32 restLength = !pinned && i == cord->mCount - 2 ? 5.0f : cord->mSpeed;
				CVec step      = delta / distance * (0.7f * (restLength - distance));
				if (pinned) {
					a.Set(*cord->mEndSource[0]);
					cord->mPull = step * -1.0f;
				} else {
					PSVECSubtract(&a, &step, &a);
				}
				b += step;
			}
			PinCordEnds(cord, points);
		}
		for (s32 i = 0; i < cord->mCount; ++i) {
			if (points[i].mPos.y < bounceHeight)
				points[i].mPos.y = bounceHeight - 0.8f * points[i].mSpeed.y;
		}
		if (cord->mFlags & XObjPlugCord::CollideWithObjects) {
			for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
				XObj* object = gGame.GetObject(i);
				if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000) || object->mStatus.mFlags & 0x100000
				    || object == gGame.GetPlayer())
					continue;
				Vec pos = gGame.GetPlayer()->mMoveTranslate;
				f32 dx = object->mMoveTranslate.x - pos.x, dz = object->mMoveTranslate.z - pos.z;
				if (sqrtf(dx * dx + dz * dz) > 150.0f) // only nearby objects
					continue;
				for (s32 j = 0; j < cord->mCount; ++j)
					PushPlugCordPointFromObject(&cord->mPoints[j].mPos, object, 0);
			}
		}
		for (s32 i = 0; i < cord->mCount; ++i)
			points[i].mTarget = points[i].mPos;
	}
	u32 old   = cord->mMode;
	s32 state = gGame.GetPlayer()->mPlug.mConnection;
	if (state < 0)
		cord->mMode = 3;
	else {
		s32 kind = (state >> 8) & 255;
		if (kind == 0)
			cord->mMode = 0;
		else if (kind == 1)
			cord->mMode = 1;
		else if (kind == 2)
			cord->mMode = 2;
		else if (kind == 3)
			cord->mMode = 4;
	}
	if (cord->mMode != old) {
		switch (cord->mMode) {
		case 1:
			ResetSparks(cord->mNodes, 0xff00ff, 1, 8, 1);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mPlug.mWire, 0, -1, 0.4f, 0xff00ff);
			break;
		case 2:
			ResetSparks(cord->mNodes, 0x6400ffff, 1, 8, 1);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mPlug.mWire, 0, -1, 1, 0x6400ffff);
			break;
		case 3:
			ResetSparks(cord->mNodes, 0xff0064ff, -1, 8, 1);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mPlug.mWire, 0, -1, 1, 0xff0064ff);
			break;
		case 4:
			ResetSparks(cord->mNodes, 0xff, -1, 1, 2);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mPlug.mWire, -1, 0x52, 1, 0xff);
			break;
		default:
			ResetSparks(cord->mNodes, 0, 0, 0, 0);
			ClearWireEffect(&gGame.GetPlayer()->mPlug.mWire);
			break;
		}
	}
	UpdatePlugCordNodes(&cord->mNodes, cord->mPoints, cord->mCount, frames);
}

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

extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/cvec.h"

/**
 * @note Address: 0x800DC43C
 * @note Size: 0x1F0
 */
extern "C" void StartPlugCordNode(XObjPlugCordNode* node, XObjPlugCordPoint* points, s32 count, f32 time, s32 mode)
{
	node->mMode = mode;
	if (mode != 1 && mode != -1)
		return;

	const s32 index = mode == 1 ? count - 2 : 0;
	const s32 next  = index - mode;
	node->mIndex    = index;
	node->mPos      = points[index].mPos;
	node->mStep.Start(0.5f, 0.2f, time);
	node->mDir = points[next].mPos - points[index].mPos;
	node->mDir.Normalize();
	node->mState    = 1;
	node->mDistance = 0.0f;
}

/**
 * @note Address: 0x800DBC80
 * @note Size: 0x18
 */
XObjPlugCordPoint::XObjPlugCordPoint()
{
}

/**
 * @note Address: 0x800DBC04
 * @note Size: 0x7C
 */
XObjPlugCordPoint::~XObjPlugCordPoint()
{
}

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
	SetGsColorUpdate(graphics->mCurRenderMode, (cord->mFlags & XObjPlugCord::UpdateColor) != 0);
	SetGsAlphaUpdate(graphics->mCurRenderMode, (cord->mFlags & XObjPlugCord::UpdateAlpha) != 0);
	SetGsDestinationAlpha(graphics->mCurRenderMode, (cord->mFlags & XObjPlugCord::SetDestinationAlpha) != 0, cord->mAlpha);
	SetGsDepthMode(graphics->mCurRenderMode, 1, 3, 1);
	SetGsBlendMode(graphics->mCurRenderMode, 1, 4, 5, 15);
	FlushCaptureGraphics(graphics);
	MtxPtr selectedView = view != NULL ? view : gGame.mGs.mCamera->mViewMtx;
	Mtx combined, local;
	Vec scale, rotation, position;
	scale.x = scale.y = cord->mWidth;
	for (s32 i = 0; i < cord->mCount - 2; ++i) {
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
}

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

/**
 * @note Address: 0x800D9F5C
 * @note Size: 0x574
 */
// Retail rounds the floor rotation before adding the object rotation.
extern "C" BOOL PushPlugCordPointFromObject(CVec* pos, XObj* object, f32 radius)
{
	const f32 margin = 1.0f;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return FALSE;
	if (object->mStatus.mFlags & 0x0c00000000000000ULL)
		return FALSE;
	if (object->mStatus.mFlags & 0x4000000000000000ULL)
		return FALSE;
	CVec point(pos->x, 0.0f, pos->z);
	BOOL pushed = FALSE;
	for (int i = 0; object->mFloors[i].mId != -1; ++i) {
		const XObjFloor& floor = object->mFloors[i];
		Vec centerOffset       = object->mMoveScale * ReadObjectFloorOffset(floor);
		Vec size               = object->mMoveScale * ReadObjectFloorExtent(floor);
		f32 yaw                = DEG2RAD * floor.mYawDegrees;
		f32 angle              = object->mMoveRotate.y + yaw;
		switch (floor.mId) {
		case 0:
		case 1: {
			Mtx world;
			CVec center(0, 0, 0);
			MakeFloorTransform(object, centerOffset, size, angle, world);
			PSMTXMultVec(world, &center, &center);
			f32 centerY = center.y;
			center.y    = 0.0f;
			f32 reach   = radius + size.x;
			if (PSVECDistance(&center, &point) < reach) {
				CVec direction = (point - center).Normalize();
				CVec edge      = center + direction * reach;
				if (pos->y <= centerY + size.y + margin && pos->y >= centerY - size.y - margin) {
					pos->x = edge.x;
					pos->z = edge.z;
					pushed = TRUE;
				}
			}
			break;
		}
		}
	}
	return pushed;
}

/**
 * @note Address: 0x800D9B10
 * @note Size: 0x44C
 */
extern "C" f32 FindPlugCordFloor(XObjPlugCord* cord, Vec* pos, f32 radius, f32 height)
{
	XObjMap* map = static_cast<XObjMap*>(gGame.mXObjs.mPools[3].First());
	if (!FindMapFloor(map, pos, &cord->mFloorHit, NULL)) {
		cord->mFloorHit.mTriangle = -1;
		cord->mFloorHit.mTrigon   = -1;
		cord->mFloorHit.mHeight   = pos->y;
	}
	f32 highest = cord->mFloorHit.mHeight;
	CVec contact, push;
	for (int i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.GetObject(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000) || (object->mStatus.mFlags & 0x100000))
			continue;
		f32 dx = object->mMoveTranslate.x - pos->x;
		f32 dz = object->mMoveTranslate.z - pos->z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (radius + object->mFloorXZExtent))
			continue;
		s32 floor = 0;
		while (floor != -1) {
			if (object->TestFloorCollision(pos, pos, radius, height, &push, &contact, &floor, 0) & 1) {
				f32 floorHeight = object->mFloorHeights[object->mHitFloorIndex];
				if (__fpclassifyf(floorHeight) != FP_NAN && pos->y + 1.0f >= floorHeight && floorHeight >= highest)
					highest = floorHeight;
			}
		}
	}
	if (highest > cord->mFloorHit.mHeight)
		cord->mFloorHit.mHeight = highest;
	return cord->mFloorHit.mHeight;
}

#include "game/xobjattachment.h"
#include "game/objectfloorquery.h"
#include "game/xchr.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/ccamera.h"
#include "game/gmath.h"
#include "game/texturecopy.h"
#include "game/cvec.h"
extern "C" {
#include <sysdolphin/mtx_transform.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
}
#include <math.h>

// the GameCube's plug cord in one unit (retail 0x800DB8A8-0x800DE370): the floor probe and
// object push, both draws, the simulation, the cord setup with its function-scope static
// point array, and the sparks that run along it. The Wii build has the same unit as
// game/wii/xobjplugcord.cpp.
// NONMATCHING: besides the per-function notes, retail lays the .bss/.sbss statics out
// InitPlugCord's first, then UpdatePlugCordSimulation's, then PushPlugCordPointFromObject's
// (the reverse of .text), and this build emits UpdatePlugCordNodes ahead of InitPlugCord.

extern "C" {
void ClearWireEffect(XObjWireEff* wire);
void ConfigurePlugWireEffect(XObjWireEff* wire, s32 anim, s32 effect, f32 rate, u32 color);
}

/**
 * @note Address: 0x800D9B10
 * @note Size: 0x44C
 */
extern "C" f32 FindPlugCordFloor(XObjPlugCord* cord, Vec* pos, f32 radius, f32 height)
{
	XObjMap* map = (XObjMap*)gGame.mXObjs.mPools[3].mObjects;
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
			f32 z = pos->z;
			f32 y = pos->y;
			f32 x = pos->x;
			if (ProbeObjectFloor(object, CVec(x, y, z), CVec(x, y, z), radius, height, &push, &contact, &floor) & 1) {
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

extern "C" {
Vec gPlugFloorOffsetTemplate, gPlugFloorExtentTemplate;
}
static inline Vec FloorOffset(XObj* obj, int i)
{
	Vec v = gPlugFloorOffsetTemplate;
	v.x   = obj->mFloors[i].mCenterOffset[0] / 100.0f;
	v.y   = obj->mFloors[i].mCenterOffset[1] / 100.0f;
	v.z   = obj->mFloors[i].mCenterOffset[2] / 100.0f;
	return v;
}
static inline Vec FloorExtent(XObj* obj, int i)
{
	Vec v = gPlugFloorExtentTemplate;
	v.x   = obj->mFloors[i].mExtent[0] / 100.0f;
	v.y   = obj->mFloors[i].mExtent[1] / 100.0f;
	v.z   = obj->mFloors[i].mExtent[2] / 100.0f;
	return v;
}
/**
 * @note Address: 0x800D9F5C
 * @note Size: 0x574
 */
// Retail rounds the floor rotation before adding the object rotation.
extern "C" BOOL PushPlugCordPointFromObject(CVec* pos, XObj* object, f32 radius)
{
	static f32 margin = 1.0f;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return FALSE;
	if (object->mStatus.mFlags & 0x0c00000000000000ULL)
		return FALSE;
	if (object->mStatus.mFlags & 0x4000000000000000ULL)
		return FALSE;
	CVec point, edge;
	point.x     = pos->x;
	point.y     = pos->y;
	point.z     = pos->z;
	point.y     = 0.0f;
	BOOL pushed = FALSE;
	for (int i = 0; object->mFloors[i].mId != -1; ++i) {
		CVec origin(object->mMoveTranslate);
		Vec centerOffset = object->mMoveScale * FloorOffset(object, i);
		Vec size         = object->mMoveScale * FloorExtent(object, i);
		f32 yaw          = DEG2RAD * object->mFloors[i].mYawDegrees;
		f32 angle        = object->mMoveRotate.y + yaw;
		switch (object->mFloors[i].mId) {
		case 0:
		case 1: {
			Mtx translate, local, rotate, scale, world;
			CVec center(0, 0, 0);
			PSMTXTrans(translate, origin.x, origin.y, origin.z);
			PSMTXTrans(local, centerOffset.x, centerOffset.y, centerOffset.z);
			PSMTXScale(scale, size.x, size.y, size.z);
			PSMTXRotRad(rotate, 'y', angle);
			PSMTXConcat(translate, rotate, world);
			PSMTXConcat(world, local, world);
			PSMTXConcat(world, scale, world);
			PSMTXMultVec(world, &center, &center);
			f32 centerY = center.y;
			center.y    = 0.0f;
			f32 reach   = radius + size.x;
			if (PSVECDistance(&center, &point) < reach) {
				CVec direction = (point - center).Normalize();
				edge.Set(center + direction * reach);
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

static inline void CopyCordPos(Vec& dst, const Vec& src)
{
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}
static inline void PinTail(XObjPlugCord* cord, XObjPlugCordPoint* points, s32 end)
{
	if (cord->mEndSource[end]) {
		Vec& dst = points[cord->mCount - 3 + end].mPos;
		CopyCordPos(dst, *cord->mEndSource[end]);
		CopyCordPos(points[cord->mCount - 3 + end].mPos, dst);
	} else if (cord->mEndHeight[end]) {
		points[cord->mCount - 3 + end].mPos.y = *cord->mEndHeight[end];
	}
}
static inline void PinCordEnds(XObjPlugCord* cord, XObjPlugCordPoint* points)
{
	if (cord->mEndSource[0]) {
		CopyCordPos(points[0].mPos, *cord->mEndSource[0]);
		CopyCordPos(points[0].mTarget, points[0].mPos);
	} else if (cord->mEndHeight[0]) {
		points[0].mPos.y = points[0].mTarget.y = *cord->mEndHeight[0];
	}
	PinTail(cord, points, 1);
	PinTail(cord, points, 2);
}

static inline void AddStep(CVec& position, Vec step)
{
	PSVECAdd(&position, &step, &position);
}
static inline void SubStep(CVec& position, Vec step)
{
	PSVECSubtract(&position, &step, &position);
}

static inline void ResetSparks(XObjPlugCordNodes& nodes, u32 color, s32 mode, s32 count, s32 enabled)
{
	nodes.mColor = color;
	nodes.mMode  = mode;
	nodes.mCount = count;
	for (s32 i = 0; i < 10; i++)
		nodes.mNode[i].mState = 0;
	nodes.mEnabled = enabled;
}

// NONMATCHING: retail emits an extra compare/clamp sequence (cmpw/bge/cmpwi/subi) in the point loop
// that the current source doesn't produce; the rest is scheduling
extern "C" void UpdatePlugCordSimulation(XObjPlugCord* cord, u32 frames)
{
	static f32 floorHeight;
	XObjPlugCordPoint* points = cord->mPoints;
	Vec tip                   = GetPlugCordPointPosition(cord, 18);
	tip.y                     = gGame.GetPlayer()->mMoveTranslate.y;
	Vec probe                 = tip;
	floorHeight               = FindPlugCordFloor(cord, &probe, -1, 0); // the floor under cord point 18, probed from the player's height
	Vec anchor;
	GetPlayerPlugCordAnchorTransform(&gGame.GetPlayer()->mPlug, &anchor, NULL, NULL);
	if (floorHeight > anchor.y) // above the plug anchor: use the player's own floor
		floorHeight = gGame.GetPlayer()->mFloorHit.mHeight;
	for (u32 frame = 0; frame < frames; ++frame) {
		static f32 gravity = 0.25f;
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
				CVec& a = points[i].mPos;
				CVec& b = points[i + 1].mPos;
				static CVec delta;
				delta          = b - a;
				f32 distance   = PSVECDistance(&b, &a);
				f32 correction = 0.7f * (cord->mSpeed - distance); // relax toward the rest length
				if (i == 0 && cord->mEndSource[0]) {               // the pinned end pulls instead of moving
					CopyCordPos(a, *cord->mEndSource[0]);
					AddStep(b, delta / distance * correction);
					cord->mPull = delta / distance * correction * -1.0f;
				} else if (i == cord->mCount - 2) { // the last segment relaxes to 5.0
					correction = 0.7f * (5.0f - distance);
					SubStep(a, delta / distance * correction);
					AddStep(b, delta / distance * correction);
				} else {
					SubStep(a, delta / distance * correction);
					AddStep(b, delta / distance * correction);
				}
			}
			PinCordEnds(cord, points);
		}
		for (s32 i = 0; i < cord->mCount; ++i) {
			f32 floor = 0.2f + floorHeight;
			if (points[i].mPos.y < floor) // bounce, damped
				points[i].mPos.y = floor - 0.8f * points[i].mSpeed.y;
		}
		if (cord->mFlags & 8) { // flag 8: objects push the cord aside
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
			CopyCordPos(points[i].mTarget, points[i].mPos);
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
 * @note Address: 0x800DB7A8
 * @note Size: 0x30
 */
extern "C" void SetPlugCordAlphaMode(XObjPlugCord* cord, BOOL enabled, u8 alpha)
{
	if (enabled) {
		cord->mFlags |= 6;
		cord->mAlpha = alpha;
	} else {
		cord->mFlags &= ~6;
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

// the static point array's destructor is registered on first use; the compiler generates
// its array-destroy stub and XObjPlugCordPoint's implicit dtor and ctor for it
/**
 * @note Address: 0x800DBA7C
 * @note Size: 0x150
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

/**
 * @note Address: 0x800DBC98
 * @note Size: 0x144
 */
// NONMATCHING: retail issues the wrapped game-pointer subtract one insn later.
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
	if (nodes->mNode[0].mState == 1) {
		XObj* object = &gGame.GetPlayer()->mPlug.mWire;
		object->SetPos(nodes->mNode[0].mPos, 0);
	}
}

static inline void AdvanceSpark(CVec& position, Vec step)
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
				AdvanceSpark(node->mPos, node->mDir * node->mStep.Get());
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
			AdvanceSpark(node->mPos, node->mDir * node->mStep.Get());
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

// todo: cleanup
static inline void CopyPosition(XObjPlugCordNode* node, XObjPlugCordPoint* point)
{
	node->mPos.x = point->mPos.x;
	node->mPos.y = point->mPos.y;
	node->mPos.z = point->mPos.z;
}

/**
 * @note Address: 0x800DC43C
 * @note Size: 0x1F0
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

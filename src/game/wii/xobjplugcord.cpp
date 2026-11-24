#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf
#include "game/xobjattachment.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/wii/title.h"
#include "game/wii/xobj.h"
#include "game/mapfloorhit.h"
#include "game/gmath.h"
#include "game/cvec.h"
#include "arithmetic.h"
#include <math.h>

// the Wii build keeps the whole cord implementation in one unit: the nodes, the cord
// and the collision scans share one literal pool (retail 0x805A5390 on)

extern "C" {
// the Wii build's SRT-matrix helper, it takes no parent scale where the GameCube's
// HSD_MtxSRT takes a fifth
void HSD_MtxSRT(Mtx m, Vec* scale, Vec* rotation, Vec* translation);
// the Wii map-floor probe: writes the hit record, returns nonzero when it found a floor
s32 fn_800C98E0(void* map, const Vec* pos, s32 triangle, XObjMapHit* hit);
// the Wii wire effect, the GameCube's XObjWireEff calls
void ConfigurePlugWireEffect(XObjWireEff* wire, s32 anim, s32 effect, f32 rate, u32 color);
void ClearWireEffect(XObjWireEff* wire);
// the Wii's stand-alone object-push pass, the GameCube had it inside UpdatePlugCordSimulation
void PushPlugCordPointsFromObjects(XObjPlugCord* cord);
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
		gGame.GetPlayer()->mCordWire.SetPos(nodes->mNode[0].mPos);
	}
}

// draws the spark nodes, each one half as bright as the one before; the Wii caller sets the
// GX state up itself, so this takes the node block where the GameCube took the cord
/**
 * @note Address: 0x80111534
 * @note Size: 0x190
 */
extern "C" void DrawPlugCordSparks(XObjPlugCordNodes* nodes, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ)
{
	Mtx combined;
	Mtx local;
	if (nodes->mEnabled == 1) {
		XObjPlugCordNode* node = nodes->mNode;
		for (s32 i = 0; i < nodes->mCount; i++, node++) {
			if (node->mState != 0) {
				Vec rotation = CVec(0.0f, 0.0f, 0.0f);
				Vec scale    = CVec(0.7f, 0.7f, 0.7f);
				HSD_MtxSRT(local, &scale, &rotation, &node->mPos);
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

// ties one cord end to a position
/**
 * @note Address: 0x80111ADC
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

/**
 * @note Address: 0x80111B14
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

static inline void SubStep(CVec& position, Vec step)
{
	position -= step;
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

// NONMATCHING: retail hoists `frames` into a callee-saved register and guards the outer loop
// with an upfront `if (frames == 0)` before the bottom `cmplw` check, and PinCordEnds' tail
// indices are recomputed as (mCount - n) * 0x30 with mulli each time; the compiler folds them
// into a pointer chain here, which shifts register allocation across the function
/**
 * @note Address: 0x80111B44
 * @note Size: 0xBBC
 */
extern "C" void UpdatePlugCordSimulation(XObjPlugCord* cord, u32 frames)
{
	XObjPlugCordPoint* points = cord->mPoints;
	Vec tip                   = GetPlugCordPointPosition(cord, 18);
	tip.y                     = gGame.GetPlayer()->GetPos()->y;
	Vec probe                 = tip;
	static f32 sFloorY;
	sFloorY = FindPlugCordFloor(cord, &probe, -1, 0); // the floor under cord point 18, probed from the player's height
	Vec anchor;
	GetPlayerPlugCordAnchorTransform(&gGame.GetPlayer()->mPlug, &anchor, NULL, NULL);
	if (sFloorY > anchor.y) // above the plug anchor: use the player's own floor
		sFloorY = gGame.GetPlayer()->mFloorHit.mHeight;
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
			f32 floor = 0.2f + sFloorY;
			if (points[i].mPos.y < floor) // bounce, damped
				points[i].mPos.y = floor - 0.8f * points[i].mSpeed.y;
		}
		// flag 8: objects push the cord aside; the GameCube had the scan inline here
		if (cord->mFlags & 8)
			PushPlugCordPointsFromObjects(cord);
		for (s32 i = 0; i < cord->mCount; ++i)
			CopyCordPos(points[i].mTarget, points[i].mPos);
	}
	s32 old   = cord->mMode;
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
		else
			cord->mMode = 0;
	}
	if (cord->mMode != old) {
		switch (cord->mMode) {
		case 1:
			ResetSparks(cord->mNodes, 0xff00ff, 1, 8, 1);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mCordWire, 0, -1, 0.4f, 0xff00ff);
			break;
		case 2:
			ResetSparks(cord->mNodes, 0x6400ffff, 1, 8, 1);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mCordWire, 0, -1, 1, 0x6400ffff);
			break;
		case 3:
			ResetSparks(cord->mNodes, 0xff0064ff, -1, 8, 1);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mCordWire, 0, -1, 1, 0xff0064ff);
			break;
		case 4:
			ResetSparks(cord->mNodes, 0xff, -1, 1, 2);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mCordWire, -1, 0x52, 1, 0xff);
			break;
		case 0:
		default:
			ResetSparks(cord->mNodes, 0, 0, 0, 0);
			ClearWireEffect(&gGame.GetPlayer()->mCordWire);
			break;
		}
	}
	UpdatePlugCordNodes(&cord->mNodes, cord->mPoints, cord->mCount, frames);
}

// draws the cord a segment at a time; the Wii moves the GX setup to the callers and keeps
// this body, with its own SRT-matrix and cylinder helpers
/**
 * @note Address: 0x80112700
 * @note Size: 0x218
 */
extern "C" void DrawPlugCordSegments(XObjPlugCord* cord, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ)
{
	f32(*selectedView)[4];
	if (view == NULL)
		selectedView = gGame.mGs.mCamera->mViewMtx;
	else
		selectedView = view;
	Mtx combined, local;
	Vec diff, scale, rotation, position;
	s32 i   = 0;
	scale.x = scale.y = cord->mWidth;
	for (; i < cord->mCount - 2; i++) {
		CVec* current = &cord->mPoints[i].mPos;
		CVec* next    = &cord->mPoints[i + 1].mPos;
		diff.x        = next->x - current->x;
		diff.y        = next->y - current->y;
		diff.z        = next->z - current->z;
		rotation.x    = atan2(-diff.y, FSqrt(diff.x * diff.x + diff.z * diff.z));
		rotation.y    = atan2(diff.x, diff.z);
		rotation.z    = 0.0f;
		position.x    = 0.5f * (current->x + next->x);
		position.y    = 0.5f * (current->y + next->y);
		position.z    = 0.5f * (current->z + next->z);
		// NONMATCHING: retail recomputes &mPoints[i + 1] as (i + 1) * 0x30 every iteration
		// and keeps the view pointer in r25; the compiler folds the address into the
		// point pointer chain instead, which shifts every callee-saved register choice.
		scale.z = 0.5f * PSVECDistance(current, next);
		HSD_MtxSRT(local, &scale, &rotation, &position);
		PSMTXConcat(selectedView, local, combined);
		DrawGsCylinderWithMatrix(&gGame.mGs, combined, cord->mColor, 6);
	}
	scale.z = 0.9f * cord->mWidth;
}

// the cord's spark-node draw entry: sets the GS state the GameCube's DrawPlugCordNodes set
// itself, then hands the embedded node block to the split-off draw
/**
 * @note Address: 0x80112918
 * @note Size: 0x114
 */
extern "C" void DrawPlugCordNodes(XObjPlugCord* cord, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ)
{
	f32(*selectedView)[4];
	if (view == NULL)
		selectedView = gGame.mGs.mCamera->mViewMtx;
	else
		selectedView = view;
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	gGame.mGs.mCurRenderState->SetBlendMode((GXBlendMode)1, (GXBlendFactor)4, (GXBlendFactor)5, (GXLogicOp)15);
	gGame.mGs.mCurRenderState->SetZMode((GXBool)1, (GXCompare)3, (GXBool)0);
	DrawPlugCordSparks(&cord->mNodes, selectedView, scaleX, scaleY, scaleZ);
}

static inline Vec FloorOffset(WiiTitleObject* obj, int i)
{
	Vec v;
	v.x = obj->mFloors[i].mCenterOffset[0] / 100.0f;
	v.y = obj->mFloors[i].mCenterOffset[1] / 100.0f;
	v.z = obj->mFloors[i].mCenterOffset[2] / 100.0f;
	return v;
}
static inline Vec FloorExtent(WiiTitleObject* obj, int i)
{
	Vec v;
	v.x = obj->mFloors[i].mExtent[0] / 100.0f;
	v.y = obj->mFloors[i].mExtent[1] / 100.0f;
	v.z = obj->mFloors[i].mExtent[2] / 100.0f;
	return v;
}
// the Wii object keeps the floor list at _174 and answers position/scale through virtuals;
// its flags word at _150 replaces the GameCube's mStatus.mFlags
/**
 * @note Address: 0x80112A2C
 * @note Size: 0x4C4
 */
extern "C" BOOL PushPlugCordPointFromObject(CVec* pos, WiiTitleObject* object, f32 radius)
{
	static f32 margin = 1.0f;
	if (!(object->mStatus & 0x0200000000000000ULL))
		return FALSE;
	if (object->mStatus & 0x0c00000000000000ULL)
		return FALSE;
	if (object->mStatus & 0x4000000000000000ULL)
		return FALSE;
	CVec point, edge;
	point.x     = pos->x;
	point.y     = pos->y;
	point.z     = pos->z;
	point.y     = 0.0f;
	BOOL pushed = FALSE;
	for (int i = 0; object->mFloors[i].mId != -1; ++i) {
		if (!(object->mFloors[i].mFlags & 1))
			continue;
		CVec origin(*object->GetPos());
		Vec centerOffset = *object->GetScale() * FloorOffset(object, i);
		Vec size         = *object->GetScale() * FloorExtent(object, i);
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

// the object-push pass the GameCube kept inside UpdatePlugCordSimulation, on the Wii a
// function of its own: walk the object table, skip far ones, push every cord point
/**
 * @note Address: 0x80112EF0
 * @note Size: 0x1C4
 */
extern "C" void PushPlugCordPointsFromObjects(XObjPlugCord* cord)
{
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		WiiTitleObject* object = (WiiTitleObject*)gGame.mXObjs.mObjects[i];
		if (!(object->mStatus & 1) || !(object->mStatus & 0x10000) || object->mStatus & 0x100000
		    || object == (WiiTitleObject*)gGame.GetPlayer())
			continue;
		Vec pos         = *gGame.GetPlayer()->GetPos();
		CVec* objectPos = object->GetPos();
		f32 dx = objectPos->x - pos.x, dz = objectPos->z - pos.z;
		if (FSqrt(dx * dx + dz * dz) > 150.0f) // only nearby objects
			continue;
		for (s32 j = 0; j < cord->mCount; ++j)
			PushPlugCordPointFromObject(&cord->mPoints[j].mPos, object, 0);
	}
}

// the highest floor under the probe point: the map first, then every object's floor
// records through the _1C8 virtual, the GameCube's plugcordcollision.cpp
/**
 * @note Address: 0x801130B4
 * @note Size: 0x2E0
 */
extern "C" f32 FindPlugCordFloor(XObjPlugCord* cord, Vec* pos, f32 radius, f32 height)
{
	if (!fn_800C98E0(gGame.mXObjs.mPools[3].mObjects, pos, cord->mFloorHit.mTriangle, &cord->mFloorHit)) {
		cord->mFloorHit.mTriangle = -1;
		cord->mFloorHit.mTrigon   = -1;
		cord->mFloorHit.mHeight   = pos->y;
	}
	f32 highest = cord->mFloorHit.mHeight;
	CVec contact, push;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		WiiTitleObject* object = (WiiTitleObject*)gGame.mXObjs.mObjects[i];
		if (!(object->mStatus & 1) || !(object->mStatus & 0x10000) || object->mStatus & 0x100000)
			continue;
		CVec* objectPos = object->GetPos();
		f32 dx = objectPos->x - pos->x, dz = objectPos->z - pos->z;
		if (FSqrt(dx * dx + dz * dz) > 100.0f + (radius + object->mFloorXZExtent))
			continue;
		s32 floor = 0;
		while (floor != -1) {
			CVec from(pos->x, pos->y, pos->z), to(pos->x, pos->y, pos->z);
			if (object->TestFloorCollision(&from, &to, &push, &contact, &floor, 0, radius, height, pos->y, pos->z) & 1) {
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

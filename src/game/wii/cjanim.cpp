// The unit between gxb.cpp and xobj_ut.cpp, .text 0x800B1558-0x800BD66C (wii_units.md
// Nr. 46): the object's collision floors, its screen projection virtuals and the
// event virtuals. The Wii source merges the GameCube's cjanim.cpp with the floor
// helpers obja.cpp and xobjfloor.cpp carried; the floor coordinate readers appear
// open-coded here, the array-and-index pair's inline half.
// NONMATCHING: the per-frame animation stepper (0x800B4988), the ExecCallback
// family and the kind-callback table (0x800B6F00-0x800BCBC4) plus
// AssignAnimEventCallback and __sinit still come from the retail asm.
#include "Version.h"
#if defined(VERSION_R24J01)
#include "arithmetic.h"
#include "game/wii/cgame.h"
#include "game/wii/ceffect.h"
#include "game/wii/xobj.h"
#include "game/gmath.h"
#include "game/globals.h"
#include "game/ut.h"
#include "game/evt.h"
#include "game/xobjchr.h"
#include "Dolphin/mtx.h"
#include <math.h>

extern "C" {
void C_VECReflect(const Vec* source, const Vec* normal, Vec* result);
void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);
f64 fmax(f64 x, f64 y);
f64 fmin(f64 x, f64 y);
// the map XObj's floor query in an unsplit unit: fills hit with the triangle
// under pos, the previous triangle as a hint; FALSE when there is none
s32 fn_800C98E0(XObj* map, const Vec* pos, s32 tri, XObjMapHit* hit);
}

// the floor coordinate readers this unit expands inline (retail shows the loads and
// the hundredths conversion open-coded; xobj_chr.cpp keeps its own out-of-line copies)
static inline Vec GetFloorCenterOffset(XObjFloor* const* floors, s32 index)
{
	Vec offset = { (*floors)[index].mCenterOffset[0] / 100.0f, (*floors)[index].mCenterOffset[1] / 100.0f,
		           (*floors)[index].mCenterOffset[2] / 100.0f };
	return offset;
}

static inline Vec GetFloorExtent(XObjFloor* const* floors, s32 index)
{
	Vec extent = { (*floors)[index].mExtent[0] / 100.0f, (*floors)[index].mExtent[1] / 100.0f, (*floors)[index].mExtent[2] / 100.0f };
	return extent;
}

static inline f32 GetFloorYawRadians(XObjFloor* const* floors, s32 index)
{
	return DEG2RAD * (*floors)[index].mYawDegrees;
}

extern s32 gScreenLeft, gScreenRight, gScreenTop, gScreenBottom;

static inline void TriggerObjectEvent(XObj* object, u32 first, u32 second)
{
	const void* script = object->GetEventScript();
	if (!IsPlayerBlockingEvents((XObjChr*)gGame.GetPlayer())) {
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, object, first, second);
	}
}

static inline void MakeFloorTransform(const XObj* object, const Vec& center, const Vec& extent, f32 yaw, Mtx result)
{
	Mtx world, local, rotate, scale;
	PSMTXTrans(world, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
	PSMTXTrans(local, center.x, center.y, center.z);
	PSMTXScale(scale, extent.x, extent.y, extent.z);
	PSMTXRotRad(rotate, 'y', yaw);
	PSMTXConcat(world, rotate, result);
	PSMTXConcat(result, local, result);
	PSMTXConcat(result, scale, result);
}

// CVec::operator* per component
CVec CVec::operator*(Vec other) const
{
	CVec result = *this;
	result.x *= other.x;
	result.y *= other.y;
	result.z *= other.z;
	return result;
}

extern "C" CVec ReflectVectorAboutNormal(const Vec* source, const Vec* normal)
{
	CVec result;
	C_VECReflect(source, normal, &result);
	return result;
}

static inline s32 IntersectFloorRay(const Vec& start, const Vec& end, const Vec& a, const Vec& b, CVec& hit, s32 segment)
{
	return IntersectLinesXZ(&start, &end, &a, &b, &hit, segment);
}

static inline void FloorNormalAngle(const CVec& normal, Vec* out, f32& angle)
{
	angle = WrapAngle(atan2(normal.x, normal.z));
	if (out)
		*out = normal;
}

extern "C" f32 GetFloorFacingAngle(XObj* object, const Vec* pos, s32 index, s32 requireSegment, Vec* contact, Vec* normalOut)
{
	CVec from(*pos);
	from.y = 0.0f;
	CVec middle;
	f32 angle = NAN;

	Vec center   = object->mMoveScale * (object->mVec714 + GetFloorCenterOffset(&object->mFloors, index));
	Vec extent   = object->mMoveScale * GetFloorExtent(&object->mFloors, index);
	f32 baseYaw  = object->mMoveRotate.y;
	f32 floorYaw = GetFloorYawRadians(&object->mFloors, index);
	f32 yaw      = baseYaw + floorYaw;
	switch (object->mFloors[index].mId) {
	case 0:
	case 1: { // a disc: face its center
		Mtx world, local, rotate, scale, transform;
		PSMTXTrans(world, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
		PSMTXTrans(local, center.x, center.y, center.z);
		PSMTXScale(scale, extent.x, extent.y, extent.z);
		PSMTXRotRad(rotate, 'y', yaw);
		PSMTXConcat(world, rotate, transform);
		PSMTXConcat(transform, local, transform);
		PSMTXConcat(transform, scale, transform);
		static Vec origin = { 0.0f, 0.0f, 0.0f };
		PSMTXMultVec(transform, &origin, &middle);
		middle.y = 0.0f;
		angle    = atan2(middle.x - from.x, middle.z - from.z);
		break;
	}
	case 2: { // a box: face the wall edge the ray crosses
		Mtx world, local, rotate, scale, transform;
		PSMTXTrans(world, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
		PSMTXTrans(local, center.x, center.y, center.z);
		PSMTXScale(scale, extent.x, extent.y, extent.z);
		PSMTXRotRad(rotate, 'y', yaw);
		PSMTXConcat(world, rotate, transform);
		PSMTXConcat(transform, local, transform);
		PSMTXConcat(transform, scale, transform);
		static Vec origin = { 0.0f, 0.0f, 0.0f };
		PSMTXMultVec(transform, &origin, &middle);
		middle.y = 0.0f;
		f32 h    = (2.0f / (3.0f * nw4r::math::FrSqrt(3.0f))) * 0.5f;
		static Vec corners[8]
		    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
		Vec points[8];
		PSMTXMultVecArray(transform, corners, points, 8);
		for (s32 i = 0; i < 4; ++i) {
			Vec& a = points[2 * i];
			Vec& b = points[2 * i + 1];
			CVec hit, normal;
			if (IntersectLinesXZ(&from, &middle, &a, &b, &hit, 1)) {
				a.y = b.y = 0.0f;
				GetOutwardPerpendicularToEdge(&from, &a, &b, &normal);
				PSVECNormalize(&normal, &normal);
				if (contact && IntersectFloorRay(from - normal * 100.0f, from + normal * 100.0f, a, b, hit, 0))
					*contact = hit;
				if (requireSegment) {
					if (IntersectFloorRay(from - normal * 100.0f, from + normal * 100.0f, a, b, hit, 1)) {
						FloorNormalAngle(normal, normalOut, angle);
						break;
					}
				} else {
					FloorNormalAngle(normal, normalOut, angle);
					break;
				}
			}
		}
		break;
	}
	case 3:
		break;
	case 5: { // a quad: face the edge the ray crosses
		Mtx translate, rotate, scaling, transform;
		Vec corners[4];
		for (s32 j = 0; j < 4; ++j)
			corners[j] = object->mVec714 + GetFloorCenterOffset(&object->mFloors, index - 3 + j);
		Vec scale     = object->mMoveScale * GetFloorExtent(&object->mFloors, index);
		f32 baseAngle = object->mMoveRotate.y;
		f32 turnAngle = GetFloorYawRadians(&object->mFloors, index);
		f32 angleY    = baseAngle + turnAngle;
		PSMTXTrans(translate, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
		PSMTXScale(scaling, scale.x, scale.y, scale.z);
		PSMTXRotRad(rotate, 'y', angleY);
		PSMTXConcat(translate, rotate, transform);
		PSMTXConcat(transform, scaling, transform);
		PSMTXMultVecArray(transform, corners, corners, 4);
		IntersectLinesXZ(&corners[0], &corners[2], &corners[1], &corners[3], &middle, 1);
		for (s32 i = 0; i < 4; ++i) {
			Vec& a = corners[i];
			Vec& b = corners[(i + 1) % 4];
			CVec hit, normal;
			if (IntersectLinesXZ(&from, &middle, &a, &b, &hit, 1)) {
				if (contact)
					*contact = hit;
				a.y = b.y = 0.0f;
				GetOutwardPerpendicularToEdge((Vec*)pos, &a, &b, &normal);
				PSVECNormalize(&normal, &normal);
				if (contact && IntersectFloorRay(from - normal * 100.0f, from + normal * 100.0f, a, b, hit, 0))
					*contact = hit;
				if (requireSegment) {
					if (IntersectFloorRay(from - normal * 100.0f, from + normal * 100.0f, a, b, hit, 1)) {
						FloorNormalAngle(normal, normalOut, angle);
						break;
					}
				} else {
					FloorNormalAngle(normal, normalOut, angle);
					break;
				}
			}
		}
		break;
	}
	}
	return angle;
}

extern "C" void ClearActiveObjectFloors(XObj* object)
{
	if (object->mStatus.mFlags & 0x0200000000000000ULL) {
		object->mFloorMasks[1] = object->mFloorMasks[0];
		for (s32 i = 0; object->mFloors[i].mId != -1; i++) {
			if (object->IsFloorOn(i)) {
				object->ClearFloor(i);
			}
		}
	}
}

static inline void ExpandRadius(f32 x, f32 z, f32& maxX, f32& maxZ)
{
	if (ABS(x) > maxX)
		maxX = ABS(x);
	if (ABS(z) > maxZ)
		maxZ = ABS(z);
}

extern "C" void UpdateObjectFloorXZExtent(XObj* object)
{
	object->mFloorXZExtent[0] = object->mFloorXZExtent[1] = NAN;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return;
	f32 maxX = 0.0f, maxZ = 0.0f;
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) {
		CVec origin((Vec) { 0, 0, 0 });
		Vec center     = object->mMoveScale * (object->mVec714 + GetFloorCenterOffset(&object->mFloors, i));
		Vec extent     = object->mMoveScale * GetFloorExtent(&object->mFloors, i);
		f32 baseYaw    = object->mMoveRotate.y;
		f32 floorAngle = GetFloorYawRadians(&object->mFloors, i);
		f32 yaw        = baseYaw + floorAngle;
		switch (object->mFloors[i].mId) {
		case 0:
		case 1: { // a disc
			CVec point(0, 0, 0);
			Mtx world, local, rotate, scale, result;
			PSMTXTrans(world, origin.x, origin.y, origin.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, result);
			PSMTXConcat(result, local, result);
			PSMTXConcat(result, scale, result);
			PSMTXMultVec(result, &point, &point);
			f32 x = point.x + extent.x;
			f32 z = point.z + extent.z;
			ExpandRadius(x, z, maxX, maxZ);
			break;
		}
		case 2: { // a box
			Mtx world, local, rotate, scale, result;
			PSMTXTrans(world, origin.x, origin.y, origin.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, result);
			PSMTXConcat(result, local, result);
			PSMTXConcat(result, scale, result);
			f32 h = (2.0f / (3.0f * nw4r::math::FrSqrt(3.0f))) * 0.5f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec points[8];
			PSMTXMultVecArray(result, corners, points, 8);
			for (s32 j = 0; j < 4; ++j)
				ExpandRadius(points[j].x, points[j].z, maxX, maxZ);
			break;
		}
		case 3: { // a wall segment
			Mtx world, local, rotate, turn, scale, result;
			PSMTXTrans(world, origin.x, origin.y, origin.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', baseYaw);
			PSMTXRotRad(turn, 'y', floorAngle);
			PSMTXConcat(world, rotate, result);
			PSMTXConcat(result, local, result);
			PSMTXConcat(result, turn, result);
			PSMTXConcat(result, scale, result);
			static Vec ends[2] = { { -.5f, 0, 0 }, { .5f, 0, 0 } };
			Vec line[2];
			PSMTXMultVecArray(result, ends, line, 2);
			// retail scans four points after transforming two
			for (s32 j = 0; j < 4; ++j)
				ExpandRadius(line[j].x, line[j].z, maxX, maxZ);
			break;
		}
		case 5: { // a quad from this and the next three records
			Mtx world, rotate, scale, result;
			Vec corners[4];
			for (s32 j = 0; j < 4; ++j)
				corners[j] = object->mVec714 + GetFloorCenterOffset(&object->mFloors, i + j);
			Vec extent    = object->mMoveScale * GetFloorExtent(&object->mFloors, i);
			f32 baseAngle = object->mMoveRotate.y;
			f32 turnAngle = GetFloorYawRadians(&object->mFloors, i);
			f32 angle     = baseAngle + turnAngle;
			PSMTXTrans(world, origin.x, origin.y, origin.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', angle);
			PSMTXConcat(world, rotate, result);
			PSMTXConcat(result, scale, result);
			PSMTXMultVecArray(result, corners, corners, 4);
			for (s32 j = 0; j < 4; ++j)
				ExpandRadius(corners[j].x, corners[j].z, maxX, maxZ);
			break;
		}
		}
	}
	object->mFloorXZExtent[0] = fmax(maxX, maxZ);
	object->mFloorXZExtent[1] = fmin(maxX, maxZ);
}

extern "C" void UpdateObjectFloorHeights(XObj* object)
{
	for (u32 i = 0; i < ARRAY_SIZE(object->mFloorHeights); ++i)
		object->mFloorHeights[i] = NAN;
	object->mMaxFloorHeight = NAN;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return;
	object->mStatus.mFlags &= ~0x0010000000000000ULL;
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) { // the list ends at id -1
		Vec center  = object->mMoveScale * (object->mVec714 + GetFloorCenterOffset(&object->mFloors, i));
		Vec extent  = object->mMoveScale * GetFloorExtent(&object->mFloors, i);
		f32 baseYaw = object->mMoveRotate.y;
		f32 turn    = GetFloorYawRadians(&object->mFloors, i);
		f32 yaw     = baseYaw + turn;
		switch (object->mFloors[i].mId) {
		case 0:
		case 1: { // a disc: the top of its column
			CVec point(0.0f, 0.0f, 0.0f);
			Mtx world, local, rotate, scale, transform;
			PSMTXTrans(world, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			PSMTXMultVec(transform, &point, &point);
			object->mFloorHeights[i] = point.y + extent.y;
			break;
		}
		case 2: { // a box: the top of its cube
			Mtx world, local, rotate, scale, transform;
			PSMTXTrans(world, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			f32 h = (2.0f / (3.0f * nw4r::math::FrSqrt(3.0f))) * 0.5f; // cube corners at unit radius
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec points[8];
			PSMTXMultVecArray(transform, corners, points, 8);
			object->mFloorHeights[i] = points[0].y - 0.01f;
			break;
		}
		case 3: // a wall has no floor height
			break;
		case 5: { // a quad: it and the three records after it share one height
			Mtx translate, rotate, scaling, transform;
			Vec corners[4];
			for (s32 j = 0; j < 4; ++j)
				corners[j] = object->mVec714 + GetFloorCenterOffset(&object->mFloors, i + j);
			Vec scale     = object->mMoveScale * GetFloorExtent(&object->mFloors, i);
			f32 baseAngle = object->mMoveRotate.y;
			f32 turnAngle = GetFloorYawRadians(&object->mFloors, i);
			f32 angle     = baseAngle + turnAngle;
			PSMTXTrans(translate, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
			PSMTXScale(scaling, scale.x, scale.y, scale.z);
			PSMTXRotRad(rotate, 'y', angle);
			PSMTXConcat(translate, rotate, transform);
			PSMTXConcat(transform, scaling, transform);
			PSMTXMultVecArray(transform, corners, corners, 4);
			object->mFloorHeights[i] = object->mFloorHeights[i + 1] = object->mFloorHeights[i + 2] = object->mFloorHeights[i + 3]
			    = extent.y / 2.0f + corners[0].y - 0.01f;
			i += 3;
			break;
		}
		}
		if (__fpclassifyf(object->mMaxFloorHeight) == FP_NAN || object->mFloorHeights[i] > object->mMaxFloorHeight)
			object->mMaxFloorHeight = object->mFloorHeights[i];
	}
}

extern "C" void SelectHighestActiveFloor(XObj* object)
{
	object->mSelectedFloorHeight = NAN;
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) {
		if (object->mFloorMasks[0] & (1U << i)) {
			if (__fpclassifyf(object->mSelectedFloorHeight) == FP_NAN || object->mFloorHeights[i] > object->mSelectedFloorHeight) {
				object->mSelectedFloorIndex  = i;
				object->mSelectedFloorHeight = object->mFloorHeights[i];
			}
		}
	}
}

// NONMATCHING: retail keeps index in the frame and reads it again for each of the floor's values,
// as if its address were taken; the GameCube version has the same wall
extern "C" CVec GetFloorContactPosition(XObj* object, s32 index, const Vec* offset)
{
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return object->mMoveTranslate; // that flag off: no floor contact to compute

	Vec displacement       = offset ? *offset : (Vec) { 0.0f, 0.0f, 0.0f };
	Vec position           = object->mMoveTranslate + displacement;
	Vec center             = object->mMoveScale * (object->mVec714 + GetFloorCenterOffset(&object->mFloors, index));
	Vec extent             = object->mMoveScale * GetFloorExtent(&object->mFloors, index);
	const XObjFloor& floor = object->mFloors[index];
	f32 yaw                = object->mMoveRotate.y + GetFloorYawRadians(&object->mFloors, index);
	f32 height;
	switch (floor.mId) {
	case 0:
	case 1: // a disc touches at its bottom, one unit down
		height = -1.0f;
		break;
	case 2: { // a box touches at its underside, half an edge down
		height = -(2.0f / (3.0f * nw4r::math::FrSqrt(3.0f))) * 0.5f;
		break;
	}
	default:
		height = NAN; // other shapes return the object's translation, offset ignored
	}
	if (__fpclassifyf(height) != FP_NAN) {
		Mtx translate, local, rotate, scale, result;
		PSMTXTrans(translate, position.x, position.y, position.z);
		PSMTXTrans(local, center.x, center.y, center.z);
		PSMTXScale(scale, extent.x, extent.y, extent.z);
		PSMTXRotRad(rotate, 'y', yaw);
		PSMTXConcat(translate, rotate, result);
		PSMTXConcat(result, local, result);
		PSMTXConcat(result, scale, result);
		CVec point(0.0f, height, 0.0f);
		PSMTXMultVec(result, &point, &point);
		return point;
	}
	return object->mMoveTranslate;
}

static inline void CountCrossedEdge(Vec& from, Vec& to, Vec* a, Vec* b, s32& outside, s32& crossed)
{
	f32 first = SignedTriangleArea2XZ(&from, a, b);
	f32 last  = SignedTriangleArea2XZ(&to, a, b);
	if (first < 0.0f) {
		++outside;
		if (last > 0.0f)
			++crossed;
	}
}

BOOL XObj::TestFloorCrossing(const Vec& start, const Vec& end)
{
	XObjFloor* floors = mFloors;
	if (!floors)
		return FALSE;
	if (mStatus.mFlags & 0x0400000000000000ULL)
		return FALSE;
	if (mStatus.mFlags & 0x4000000000000000ULL)
		return FALSE;
	CVec from(start);
	CVec extent;
	from.y = 0;
	Vec to = end;
	to.y   = 0; // floors are tested flat on the ground plane

	f32 rotation = mMoveRotate.y;
	for (s32 i = 0; floors[i].mId != -1; ++i) {
		CVec center  = mMoveScale * (mVec714 + GetFloorCenterOffset(&mFloors, i));
		extent       = mMoveScale * GetFloorExtent(&mFloors, i);
		f32 localYaw = GetFloorYawRadians(&mFloors, i);
		f32 yaw      = rotation + localYaw;
		switch (floors[i].mId) {
		case 0:
		case 1: { // a point: passed if the bearing to it swings through
			CVec point(0, 0, 0);
			Mtx transform;
			MakeFloorTransform(this, center, extent, yaw, transform);
			PSMTXMultVec(transform, &point, &point);
			point.y   = 0;
			f32 first = atan2(from.x - point.x, from.z - point.z);
			f32 last  = atan2(to.x - point.x, to.z - point.z);
			f32 diff  = WrapAngle(first - last);
			if (diff > 2 * THIRD_PI && diff < 4 * THIRD_PI)
				return TRUE;
			break;
		}
		case 2: { // a box, tested against its four side edges
			Mtx transform;
			MakeFloorTransform(this, center, extent, yaw, transform);
			f32 h = (2.0f / (3.0f * nw4r::math::FrSqrt(3.0f))) * 0.5f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec world[8];
			PSMTXMultVecArray(transform, corners, world, 8);
			s32 outside = 0, crossed = 0;
			for (s32 i = 0; i < 4; ++i) {
				Vec* a = &world[2 * i];
				Vec* b = &world[2 * i + 1];
				CountCrossedEdge(from, to, a, b, outside, crossed);
			}
			if (outside == crossed)
				return TRUE;
			break;
		}
		case 3: { // a line segment: the path crosses it when the sides swap
			Mtx world, offset, turn, local, scale, transform;
			PSMTXTrans(world, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXTrans(offset, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(turn, 'y', rotation);
			PSMTXRotRad(local, 'y', localYaw);
			PSMTXConcat(world, turn, transform);
			PSMTXConcat(transform, offset, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			static Vec ends[2] = { { -.5f, 0, 0 }, { .5f, 0, 0 } };
			Vec edge[2];
			PSMTXMultVecArray(transform, ends, edge, 2);
			f32 first = SignedTriangleArea2XZ(&from, &edge[0], &edge[1]);
			if (first * SignedTriangleArea2XZ(&to, &edge[0], &edge[1]) < 0)
				return TRUE;
			break;
		}
		case 5: { // a four-point polygon built from this and the next three floors
			Vec corners[4];
			for (s32 j = 0; j < 4; ++j)
				corners[j] = mVec714 + GetFloorCenterOffset(&mFloors, i + j);
			Vec size  = mMoveScale * GetFloorExtent(&mFloors, i);
			f32 angle = mMoveRotate.y + GetFloorYawRadians(&mFloors, i);
			Mtx translate, rotate, scale, transform;
			PSMTXTrans(translate, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXScale(scale, size.x, size.y, size.z);
			PSMTXRotRad(rotate, 'y', angle);
			PSMTXConcat(translate, rotate, transform);
			PSMTXConcat(transform, scale, transform);
			PSMTXMultVecArray(transform, corners, corners, 4);
			s32 outside = 0, crossed = 0;
			for (s32 i = 0; i < 4; ++i) {
				Vec* a = &corners[i];
				Vec* b = &corners[(i + 1) % 4];
				CountCrossedEdge(from, to, a, b, outside, crossed);
			}
			if (outside == crossed)
				return TRUE;
			i += 3; // its four points were this and the next three entries
			break;
		}
		}
	}
	return FALSE;
}

void XObj::ProjectOffsetToScreen(const Vec* offset, f32* x, f32* y, f32* z, CCamera* camera)
{
	camera        = camera != NULL ? camera : &gGame.mMotionControlCamera;
	CVec position = mMoveTranslate + *offset;
	f32 screenX, screenY, screenZ;
	GXProject(position.x, position.y, position.z, camera->mViewMtx, &camera->mIsOrtho, camera->mViewport, &screenX, &screenY, &screenZ);
	screenX *= camera->mScale;
	if (x != NULL)
		*x = screenX;
	if (y != NULL)
		*y = screenY;
	if (z != NULL)
		*z = screenZ;
}

// NONMATCHING: retail keeps the four projected points in a Vec screen[4] (index 3
// down to 0 in the calls' order: 3*pi/2, pi/2, pi, heading) and each raw screen x in
// an f32 sx[4], then writes screen[i].x = sx[i] * mScale. Whether the source used
// arrays or four named vectors is not recoverable yet.
BOOL XObj::ProjectVisibleBounds(f32* width, f32* height, CCamera* camera, f32 radius, f32 objectHeight)
{
	camera          = camera != NULL ? camera : &gGame.mMotionControlCamera;
	f32 heading     = camera->mView.mAngle.y;
	f32* projection = &camera->mIsOrtho;
	f32* viewport   = camera->mViewport;
	Vec screen[4];
	f32 sx[4];
	// three points on the circle and one raised by the height give its screen extent
	f32 angle  = (PI + HALF_PI) + heading;
	f32 sine   = (f32)sin(angle);
	f32 cosine = (f32)cos(angle);
	GXProject(radius * sine + mMoveTranslate.x, mMoveTranslate.y, radius * cosine + mMoveTranslate.z, camera->mViewMtx, projection,
	          viewport, &sx[3], &screen[3].y, &screen[3].z);
	screen[3].x = sx[3] * camera->mScale;
	angle       = HALF_PI + heading;
	sine        = (f32)sin(angle);
	cosine      = (f32)cos(angle);
	GXProject(radius * sine + mMoveTranslate.x, mMoveTranslate.y, radius * cosine + mMoveTranslate.z, camera->mViewMtx, projection,
	          viewport, &sx[2], &screen[2].y, &screen[2].z);
	screen[2].x = sx[2] * camera->mScale;
	angle       = PI + heading;
	sine        = (f32)sin(angle);
	cosine      = (f32)cos(angle);
	GXProject(radius * sine + mMoveTranslate.x, mMoveTranslate.y, radius * cosine + mMoveTranslate.z, camera->mViewMtx, projection,
	          viewport, &sx[1], &screen[1].y, &screen[1].z);
	screen[1].x = sx[1] * camera->mScale;
	sine        = (f32)sin(heading);
	cosine      = (f32)cos(heading);
	GXProject(radius * sine + mMoveTranslate.x, mMoveTranslate.y + objectHeight, radius * cosine + mMoveTranslate.z, camera->mViewMtx,
	          projection, viewport, &sx[0], &screen[0].y, &screen[0].z);
	screen[0].x = sx[0] * camera->mScale;
	if (screen[3].x >= gScreenLeft && screen[2].x < gScreenRight && screen[1].y >= gScreenTop && screen[0].y < gScreenBottom
	    && screen[0].z > 0.0f && screen[0].z < 1.0f) {
		if (width != NULL)
			*width = __fabsf(screen[2].x - screen[3].x);
		if (height != NULL)
			*height = __fabsf(screen[0].y - screen[1].y);
		return TRUE;
	}
	return FALSE;
}

BOOL XObj::IsOutsideCameraDepth(CCamera* camera)
{
	if (mObject.m_pScnMdl == NULL)
		return TRUE;
	nw4r::g3d::ResNode node = GetRootResNode();
	CVec position           = GetWorldPos(node);
	f32 screenX, screenY, screenZ;
	GXProject(position.x, position.y, position.z, camera->mViewMtx, &camera->mIsOrtho, camera->mViewport, &screenX, &screenY, &screenZ);
	screenX *= camera->mScale;
	if (screenZ > 0.0f && screenZ < 1.0f)
		return FALSE; // inside the depth range: visible
	return TRUE;
}

// the GameCube's cjanim.cpp function, the Wii keeps it as a free function too;
// the anchor and limits it reads moved into the object's CAnmQueue on the Wii
extern "C" BOOL SetClampedPushPositionXZ(XObj* object, const Vec* direction, u8 axes, f32 x, f32 z)
{
	BOOL changed = FALSE;
	if (__fpclassifyf(object->mAnmQueue.mAnchorX) == FP_NAN || __fpclassifyf(object->mAnmQueue.mAnchorZ) == FP_NAN) {
		object->SetTranslateX(x);
		object->SetTranslateZ(z);
	} else if (object->mAnmQueue.mAnchorX == x && object->mAnmQueue.mAnchorZ == z) {
		object->SetTranslateX(x);
		object->SetTranslateZ(z);
	} else {
		Mtx rotation;
		Vec displacement;
		PSMTXRotRad(rotation, 'y', -object->mMoveRotate.y);
		displacement.x = object->mAnmQueue.mAnchorX - x;
		displacement.z = object->mAnmQueue.mAnchorZ - z;
		displacement.y = 0.0f;
		PSMTXMultVec(rotation, &displacement, &displacement);
		if (axes & 1) {
			if (direction->x > 0.0f && displacement.x < object->mAnmQueue.mLimitPosX) {
				displacement.x = object->mAnmQueue.mLimitPosX;
				changed        = TRUE;
			} else if (direction->x < 0.0f && displacement.x > object->mAnmQueue.mLimitNegX) {
				displacement.x = object->mAnmQueue.mLimitNegX;
				changed        = TRUE;
			}
		}
		if (axes & 2) {
			if (direction->z > 0.0f && displacement.z < object->mAnmQueue.mLimitPosZ) {
				displacement.z = object->mAnmQueue.mLimitPosZ;
				changed        = TRUE;
			} else if (direction->z < 0.0f && displacement.z > object->mAnmQueue.mLimitNegZ) {
				displacement.z = object->mAnmQueue.mLimitNegZ;
				changed        = TRUE;
			}
		}
		PSMTXRotRad(rotation, 'y', object->mMoveRotate.y);
		PSMTXMultVec(rotation, &displacement, &displacement);
		object->SetTranslateX(object->mAnmQueue.mAnchorX - displacement.x);
		object->SetTranslateZ(object->mAnmQueue.mAnchorZ - displacement.z);
	}
	return changed;
}

// probes the stage map under the object's position and refreshes its mFloorHit,
// the height clamped up to minHeight; TRUE when the map has a triangle there
extern "C" BOOL QueryMapFloorHeight(XObj* object, f32 minHeight)
{
	XObj* map = (XObj*)gGame.mXObjs.mPools[3].mObjects;
	if (!fn_800C98E0(map, object->GetPos(), -1, &object->mFloorHit))
		return FALSE;
	if (object->mFloorHit.mHeight < minHeight)
		object->mFloorHit.mHeight = minHeight;
	return TRUE;
}

// fn_800B650C, the GameCube cjanim.cpp's QueryObjectFloorHeight: the highest floor
// under the position across the map and every live object
extern "C" f32 QueryObjectFloorHeight(XObj* self, f32 radius, f32 height)
{
	Vec* pos  = &self->mMoveTranslate;
	XObj* map = (XObj*)gGame.mXObjs.mPools[3].mObjects;
	if (!fn_800C98E0(map, pos, self->mFloorHit.mTriangle, &self->mFloorHit)) {
		self->mFloorHit.mTriangle = -1;
		self->mFloorHit.mTrigon   = -1;
		self->mFloorHit.mHeight   = pos->y;
	}
	f32 best = self->mFloorHit.mHeight;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.mXObjs.mObjects[i];
		if (!(object->mStatus.mFlags & 0x1) || !(object->mStatus.mFlags & 0x10000) || object->mStatus.mFlags & 0x100000 || object == self) {
			continue;
		}
		CVec* otherPos = object->GetPos();
		f32 dx         = otherPos->x - pos->x;
		f32 dz         = otherPos->z - pos->z;
		f32 dist       = FSqrt(dx * dx + dz * dz);
		if (dist > 100.0f + radius + object->mFloorXZExtent[0])
			continue;
		s32 cursor = 0;
		do {
			CVec from(*pos), to(*pos);
			Vec push, contact;
			u32 hit = object->TestFloorCollision(&from, &to, radius, height, &push, &contact, &cursor, 0);
			if (hit & 1) {
				f32 floorHeight = object->mFloorHeights[object->mHitFloorIndex];
				if (__fpclassifyf(floorHeight) != FP_NAN && pos->y + 1.0f >= floorHeight && floorHeight >= best)
					best = floorHeight;
			}
		} while (cursor != -1);
	}
	if (best > self->mFloorHit.mHeight)
		self->mFloorHit.mHeight = best;
	return self->mFloorHit.mHeight;
}

void XObj::SetColor(s32 index, u8 red, u8 green, u8 blue, f32 time)
{
	mAnmQueue.mColorStep[index].mTarget.x = red;
	mAnmQueue.mColorStep[index].mTarget.y = green;
	mAnmQueue.mColorStep[index].mTarget.z = blue;
	if (time == 0.0f) {
		mAnmQueue.mColorStep[index].mValue.x = red;
		mAnmQueue.mColorStep[index].mValue.y = green;
		mAnmQueue.mColorStep[index].mValue.z = blue;
		mAnmQueue.mColorStep[index].mSpeed.x = mAnmQueue.mColorStep[index].mSpeed.y = mAnmQueue.mColorStep[index].mSpeed.z = 0.0f;
	} else {
		mAnmQueue.mColorStep[index].mSpeed.x = ((f32)red - mAnmQueue.mColorStep[index].mValue.x) / (60.0f * time);
		mAnmQueue.mColorStep[index].mSpeed.y = ((f32)green - mAnmQueue.mColorStep[index].mValue.y) / (60.0f * time);
		mAnmQueue.mColorStep[index].mSpeed.z = ((f32)blue - mAnmQueue.mColorStep[index].mValue.z) / (60.0f * time);
	}
}

BOOL XObj::ReceivePlugPower(f32 amount)
{
	return TRUE;
}

void XObj::OnHitFromPosition(const Vec& position, f32 strength)
{
	if (mStatus.mFlags & 0x0000080000000000ULL) {
		// a hit of the required force raises event 2001, less raises 2000
		if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f) {
			TriggerObjectEvent(this, -1, 2001);
		} else {
			TriggerObjectEvent(this, -1, 2000);
		}
	}
}

void XObj::OnSquirterHit(const Vec* contact, s32 value, s32 tint)
{
	if (mStatus.mFlags & 0x0000080000000000ULL) { // flag means: has a hit event
		const void* script = this->GetEventScript();
		if (!IsPlayerBlockingEvents((XObjChr*)gGame.GetPlayer())) {
			StartOrQueueEventScript(&gGame.mEvt, (u8*)script, this, -1,
			                        (s16)((u32)value + 3000)); // squirts reach the script as 3000 + value
		}
	}
}

BOOL XObj::OnItemUsed(const Vec* position, s32 value)
{
	if (mStatus.mFlags & 0x0000040000000000ULL) { // flag means: has an item-use event
		if (value == 56)
			return FALSE; // item 56 never triggers it
		const void* script = this->GetEventScript();
		if (!IsPlayerBlockingEvents((XObjChr*)gGame.GetPlayer())) {
			StartOrQueueEventScript(&gGame.mEvt, (u8*)script, this, (s16)value, -1);
		}
		return TRUE;
	}
	return FALSE;
}

// Update reports the pointer sweep; XObj only notes the over flag
s32 XObj::vf208(const Vec* pointer, const Vec* motion, u8 over, f32 speed)
{
	_6D8 = over;
	return 0;
}

// the per-kind event node tables in the data unit at 0x802FF340 (lbl_80302300):
// a record's key packs the event args four bits each behind a seed bit and names
// the model node they select; a kind without a match resolves to node 0
struct AnimNodeRecord {
	u64 mKey;          // _00
	const char* mName; // _08, NULL ends the records
	u32 _C;
};
struct AnimNodeTable {
	s32 mKind;                      // _00, -1 ends the table
	const AnimNodeRecord* mRecords; // _04
};
extern "C" const AnimNodeTable lbl_80302300[];

// fn_800B6BD8: the node name for the record matching the packed args
static const char* FindAnimNodeName(XObj* object, s32 count, s32* args)
{
	u64 key = 1;
	for (s32 i = 0; i < count; ++i)
		key = key << 4 | (args[i] & 0xF);
	const AnimNodeRecord* records = NULL;
	for (s32 i = 0; lbl_80302300[i].mKind >= 0; ++i) {
		if (lbl_80302300[i].mKind == object->GetKind()) {
			records = lbl_80302300[i].mRecords;
			break;
		}
	}
	if (records != NULL) {
		for (s32 i = 0; records[i].mName != NULL; ++i) {
			if (records[i].mKey == key)
				return records[i].mName;
		}
	}
	return NULL;
}

nw4r::g3d::ResNode XObj::vf280(s32 count, s32* args)
{
	const char* name = FindAnimNodeName(this, count, args);
	if (name != NULL)
		return GetResNode(name);
	return GetResNode((u32)0);
}

CVec XObj::vf288(s32 count, s32* args)
{
	const char* name = FindAnimNodeName(this, count, args);
	if (name != NULL)
		return GetJointWorldPos(name);
	return GetJointWorldPos((u32)0);
}

// the per-kind animation event callbacks (retail 0x800B6F00-0x800BCBC4):
// AssignAnimEventCallback puts the one for the object's kind on it; the
// animation stepper calls it while an animation with event frames plays.
// Each starts the kind's effect on the named model node when the frame
// condition holds; kind is the event's kind, frame the animation's current
// frame and target the event's frame (0 marks an event without one)
extern "C" EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);

static inline void StartNodeEffect(XObj* object, const char* node, u32 effect, s32 group)
{
	MtxP mtx             = object->GetWorldMtx(node);
	EffectInstance* inst = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, effect, group);
	if (inst != NULL) {
		inst->mtx = mtx;
		UpdateEffectMtx(inst);
	}
}

// kind 302: the cylinder node's loop effect
static void AnimEventCyl295(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 3)
		return;
	if (frame == 0.0f || prev > frame)
		StartNodeEffect(object, "cyl295", 0x83, 0);
}

// kind 405: the kettle's steam
static void AnimEventKettle(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (prev < 4.0f && frame >= 4.0f)
		StartNodeEffect(object, "kettle_ptcl01", 0x43, 0);
}

// kind 406: the teacup's steam
static void AnimEventTeacup(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f))
		StartNodeEffect(object, "ptcl_teacup", 0x44, 0);
}

// kind 424: one effect per event kind on the same node
static void AnimEventPtcl(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1) {
		if (frame == 0.0f || prev > frame)
			StartNodeEffect(object, "ptcl", 0x30, 0);
	} else if (kind == 2) {
		if (frame == 0.0f || prev > frame)
			StartNodeEffect(object, "ptcl", 0x31, 0);
	}
}

// kind 626
static void AnimEventZactGicco(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (prev < 76.0f && frame >= 76.0f)
		StartNodeEffect(object, "fx_zact_gicco1", 0x24, 1);
}

// kind 832: the ghost's effect
static void AnimEventGhost(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f))
		StartNodeEffect(object, "fx_ghost_ptcl", 0x47, 1);
}

// kind 842
static void AnimEventCbDoor(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 2)
		return;
	if (prev < 2.0f && frame >= 2.0f)
		StartNodeEffect(object, "fx_cbdoor_ptcl", 0x49, 0);
}

// kind 856: the welding tools' sparks
static void AnimEventWeldingTools(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (frame == 0.0f || prev > frame)
		StartNodeEffect(object, "welding_tools", 0x80, 0);
}

// kind 870: two effects on the same node
static void AnimEventObj30(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 2)
		return;
	if (frame == 0.0f || prev > frame) {
		StartNodeEffect(object, "obj30", 0x66, 0);
		StartNodeEffect(object, "obj30", 0x67, 0);
	}
}

// kind 934: the rain effect
static void AnimEventRain(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f))
		StartNodeEffect(object, "rain_ptcl1", 0x56, 0);
}

// kind 995: the clock's second hand, one effect per event kind
static void AnimEventSecond(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1) {
		if (frame == 0.0f || prev > frame)
			StartNodeEffect(object, "second", 0x61, 0);
	} else if (kind == 2) {
		if (frame == 0.0f || prev > frame)
			StartNodeEffect(object, "second", 0x62, 0);
	}
}

// kind 1034: two effects on two nodes
static void AnimEventPtcl01(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (frame == 0.0f || prev > frame) {
		StartNodeEffect(object, "ptcl01", 0x6b, 0);
		StartNodeEffect(object, "ptcl2", 0x6c, 0);
	}
}

// fn_800BD064: sets the object's animation event callback for its kind
extern "C" void AssignAnimEventCallback(XObj* object)
{
	switch (object->GetKind()) {
	case 302: object->mAnimEventCallback = AnimEventCyl295; break;
	case 405: object->mAnimEventCallback = AnimEventKettle; break;
	case 406: object->mAnimEventCallback = AnimEventTeacup; break;
	case 424: object->mAnimEventCallback = AnimEventPtcl; break;
	case 626: object->mAnimEventCallback = AnimEventZactGicco; break;
	case 832: object->mAnimEventCallback = AnimEventGhost; break;
	case 842: object->mAnimEventCallback = AnimEventCbDoor; break;
	case 856: object->mAnimEventCallback = AnimEventWeldingTools; break;
	case 870: object->mAnimEventCallback = AnimEventObj30; break;
	case 934: object->mAnimEventCallback = AnimEventRain; break;
	case 995: object->mAnimEventCallback = AnimEventSecond; break;
	case 1034: object->mAnimEventCallback = AnimEventPtcl01; break;
	}
}
#endif

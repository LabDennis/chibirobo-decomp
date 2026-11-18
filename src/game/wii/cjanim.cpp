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
#include "game/wii/xobj.h"
#include "game/wii/ceffect.h"
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
void HSD_MtxGetScale(Mtx m, Vec* out);
f64 fmax(f64 x, f64 y);
f64 fmin(f64 x, f64 y);
// the map XObj's floor query in an unsplit unit: fills hit with the triangle
// under pos, the previous triangle as a hint; FALSE when there is none
s32 fn_800C98E0(XObj* map, const Vec* pos, s32 tri, XObjMapHit* hit);
// evt.cpp's effect slot allocator/retirer (0x800A11A4/0x800A12F8)
EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);
void fn_800A12F8(EffectInstance* effects, u32 id, BOOL retireParticles);
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

// the object-world-callback's second pass: copy the base matrix into the node's
// result slot, and with the flag set move the object's transform to the matrix's
void XObj::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	if (pFuncObj->GetCallbackNodeID() == 0 && mBaseMtxPtr != NULL) {
		Mtx mtx;
		PSMTXCopy(mBaseMtx, mtx);
		PSMTXCopy(mtx, pResult->mpMtx);
		if (mStatus.mFlags & 0x0000400000000000ULL) {
			mMoveTranslate.x = mtx[0][3];
			mMoveTranslate.y = mtx[1][3];
			mMoveTranslate.z = mtx[2][3];
			HSD_MtxGetScale(mtx, &mMoveScale);
		}
	}
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
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B6F00(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 3) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("cyl295");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x83, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B6F8C(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 149.0f && f2 >= 149.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl6");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl7");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl8");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl9");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl10");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl12");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl13");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl16");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl17");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl18");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x36, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B7348(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 4.0f && f2 >= 4.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("kettle_ptcl01");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x43, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B73D8(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_teacup");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x44, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B7480(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x30, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 2) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x31, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B7578(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_l");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_l1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_l2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r6");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r7");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r8");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r9");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r10");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r11");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r12");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r13");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r14");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r15");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_r16");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x33, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B7BD0(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("bb_nikki");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x46, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 2) {
		if (f1 < 51.0f && f2 >= 51.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("bb_nikki");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x46, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B7CCC(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_tomp_scare3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x38, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("fx_tomp_scare4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x38, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B7DD4(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("migi1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x85, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("migi2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x85, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("migi3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x85, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("migi4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x85, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("migi5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x85, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 2) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("hidari1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x86, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 7.0f && f2 >= 7.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("hidari2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x86, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 14.0f && f2 >= 14.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("hidari3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x86, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 21.0f && f2 >= 21.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("hidari4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x86, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 28.0f && f2 >= 28.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("hidari5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x86, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("center");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x89, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 4) {
		if (f1 < 39.0f && f2 >= 39.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("hidari1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8A, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("hidari1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 20.0f && f2 >= 20.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("hidari2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8A, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("hidari2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("hidari3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8A, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("hidari3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("hidari4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8A, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("hidari4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("hidari5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8A, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("hidari5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B85B8(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 2) {
		if (f1 < 3.0f && f2 >= 3.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_leticwarp_in_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x3D, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f1 < 3.0f && f2 >= 3.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_leticwarp_out_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x3E, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B86B8(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 76.0f && f2 >= 76.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_zact_gicco1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x24, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B8748(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 15.0f && f2 >= 15.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_zact_frog_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x40, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 29.0f && f2 >= 29.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_zact_frog_ptcl2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x41, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B8840(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 18.0f && f2 >= 18.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_zact_injured_shock");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x40, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 14.0f && f2 >= 14.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_zact_injured_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x40, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B8938(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_pajamas_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x16, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 22.0f && f2 >= 22.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_pajamas_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x16, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 42.0f && f2 >= 42.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_pajamas_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x16, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B8A98(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x0, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 2) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x12, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x13, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 4) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4A, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B8C78(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_warpzone_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x48, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAB, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAB, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAB, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 4) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAC, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAC, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAC, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 41.0f && f2 >= 41.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAC, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAC, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAC, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 5) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAC, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAC, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("chrgeOK3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAC, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B91C0(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_ghost_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x47, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B9268(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 2) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_cbdoor_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x49, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B92F8(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl6");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl7");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl8");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl9");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl10");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 2) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x79, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 160.0f && f2 >= 160.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x79, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x79, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 59.0f && f2 >= 59.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4C, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 4) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x79, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 59.0f && f2 >= 59.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4C, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 5) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x79, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 59.0f && f2 >= 59.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4C, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 6) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x79, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 7) {
		if (f1 < 289.0f && f2 >= 289.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x53, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 49.0f && f2 >= 49.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl11");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x54, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B9C50(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4C, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4C, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 4) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("torch_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B9E6C(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("welding_tools");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x80, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B9EF8(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 2) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("obj30");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x66, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("obj30");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x67, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800B9FE4(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 0.0f && f2 >= 0.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("elec_ptcl2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xF, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("air_ptcl1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x5D, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 4) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_deca_battery2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x1A, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 5) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_deca_battery2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x5E, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 6) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_short1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x91, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_short3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x91, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 3.0f && f2 >= 3.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_short2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x91, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_short5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x91, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 7) {
		if (f1 < 3.0f && f2 >= 3.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_short1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x92, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_short2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x92, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_short3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x92, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_short4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x92, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_short5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x92, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 8) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_short1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x92, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_short2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x92, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_short3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x92, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_short4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x92, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_short6");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x93, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("eye_short7");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x93, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 9) {
		if (f1 < 34.0f && f2 >= 34.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_short1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x94, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 10.0f && f2 >= 10.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_short4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x94, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("eye_short5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x94, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 49.0f && f2 >= 49.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_deca_battery2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x96, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("fx_deca_battery2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x95, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BA990(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_gicco_twinb01_ptcl1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x9D, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("first_ptcl2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xA0, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 2) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("first_ptcl01");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x57, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f1 < 0.0f && f2 >= 0.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("first_ptcl2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xA1, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BAB88(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 0.0f && f2 >= 0.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("tb01_ptcl01");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xA2, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("tb01_ptcl01");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x1, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("tb01_ptcl2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xA2, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("tb01_ptcl2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x1, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("tb01_ptcl3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xA2, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("tb01_ptcl3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x1, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BADC0(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 199.0f && f2 >= 199.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_gicco_twinb03_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x9E, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 19.0f && f2 >= 19.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x9F, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BAEB8(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("rain_ptcl1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x56, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BAF60(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("catch_a_cold_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x4D, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("beam_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x58, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 4) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("beam_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x58, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 5) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("beam_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x58, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 6) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("beam_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x59, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 7) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("beam_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x59, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BB208(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("second");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x61, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 2) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("second");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x62, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BB300(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl01");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x6B, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x6C, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BB3EC(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_pirateboat_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x64, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("fx_pirateboat_ptcl1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x64, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BB4F4(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 2) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("energy_ptcl_root");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x6F, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("energy_ptcl_root");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x6F, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 12.0f && f2 >= 12.0f || f1 < 24.0f && f2 >= 24.0f || f1 < 36.0f && f2 >= 36.0f || f1 < 48.0f && f2 >= 48.0f
		           || f1 < 60.0f && f2 >= 60.0f || f1 < 72.0f && f2 >= 72.0f || f1 < 84.0f && f2 >= 84.0f || f1 < 96.0f && f2 >= 96.0f
		           || f1 < 108.0f && f2 >= 108.0f || f1 < 120.0f && f2 >= 120.0f || f1 < 132.0f && f2 >= 132.0f
		           || f1 < 144.0f && f2 >= 144.0f || f1 < 156.0f && f2 >= 156.0f || f1 < 168.0f && f2 >= 168.0f
		           || f1 < 180.0f && f2 >= 180.0f || f1 < 192.0f && f2 >= 192.0f || f1 < 204.0f && f2 >= 204.0f
		           || f1 < 216.0f && f2 >= 216.0f || f1 < 228.0f && f2 >= 228.0f || f1 < 240.0f && f2 >= 240.0f
		           || f1 < 252.0f && f2 >= 252.0f || f1 < 264.0f && f2 >= 264.0f || f1 < 276.0f && f2 >= 276.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("energy_ptcl_root");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x6F, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BB864(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_matrix1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x6E, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_matrix2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x6E, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 2) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_electric");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x75, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x76, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 4) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x77, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x76, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 10.0f && f2 >= 10.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x77, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x76, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 20.0f && f2 >= 20.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x77, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x76, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 30.0f && f2 >= 30.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x77, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x76, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 40.0f && f2 >= 40.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x77, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x76, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 50.0f && f2 >= 50.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x77, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x76, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BBF0C(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("red_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x71, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 2) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("red_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x72, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("red_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x73, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 4) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("red_ptcl");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x74, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BC0DC(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp02");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x9, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp02");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x7, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("ptcl_exp02");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x8, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BC218(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_tm_takeoff1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x97, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("fx_tm_takeoff2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x97, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("fx_tm_takeoff3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x97, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("fx_tm_takeoff4");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x97, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("fx_tm_takeoff5");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x97, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("fx_tm_takeoff6");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x97, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("fx_tm_takeoff7");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x97, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BC494(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 1) {
		if (f1 < 13.0f && f2 >= 13.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl1_laser_shock_smoke1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAA, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 179.0f && f2 >= 179.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl1_laser_shock_smoke1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAA, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 199.0f && f2 >= 199.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl1_laser_shock_smoke1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAA, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 219.0f && f2 >= 219.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl1_laser_shock_smoke1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAA, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 79.0f && f2 >= 79.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("ptcl1_laser_energyfall1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x58, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 2) {
		if (f1 < 17.0f && f2 >= 17.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_chibirobo2");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xB8, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("fx_chibirobo3");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xB8, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_chibirobo_bright1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xB9, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 4) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_chibirobo_bright1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xBA, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 144.0f && f2 >= 144.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_chibirobo_spark1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xBB, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 5) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_chibirobo_bright1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xB7, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BC964(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 0) {
		fn_800A12F8((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xBD, TRUE);
	} else if (event == 1) {
		if (f2 == 0.0f || f1 > f2) {
			{
				MtxP mtx          = object->GetWorldMtx("sphere10");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xBD, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("sphere11");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xBD, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("sphere12");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xBD, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("sphere13");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xBD, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("sphere14");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xBD, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("sphere15");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xBD, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the object kind's animation event callback (registered by AssignAnimEventCallback)
static void fn_800BCBC4(XObj* object, s32 event, f32 f1, f32 f2)
{
	if (event == 2) {
		if (f1 < 1.0f && f2 >= 1.0f || f1 < 1.0f && f2 >= 1.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_deca_neck1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAE, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("deca_socket1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAF, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("deca_btrybox1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAF, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	} else if (event == 3) {
		if (f1 < 2.0f && f2 >= 2.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_deca_neck1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAE, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("deca_btrybox1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAF, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 31.0f && f2 >= 31.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_deca_neck1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAE, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("deca_socket1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAF, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("deca_btrybox1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAF, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 61.0f && f2 >= 61.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_deca_neck1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAE, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("deca_socket1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAF, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		} else if (f1 < 91.0f && f2 >= 91.0f) {
			{
				MtxP mtx          = object->GetWorldMtx("fx_deca_neck1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAE, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
			{
				MtxP mtx          = object->GetWorldMtx("deca_socket1");
				EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0xAF, 0);
				if (e != NULL) {
					e->mtx = mtx;
					UpdateEffectMtx(e);
				}
			}
		}
	}
}
// the kind's anim keyframe hook picker (name invented): switches on GetKind()
void XObj::AssignAnimEventCallback()
{
	switch (GetKind()) {
	case 302:
		mAnimEventCallback = fn_800B6F00;
		break;
	case 397:
		mAnimEventCallback = fn_800B6F8C;
		break;
	case 405:
		mAnimEventCallback = fn_800B7348;
		break;
	case 406:
		mAnimEventCallback = fn_800B73D8;
		break;
	case 424:
		mAnimEventCallback = fn_800B7480;
		break;
	case 428:
		mAnimEventCallback = fn_800B7578;
		break;
	case 431:
		mAnimEventCallback = fn_800B7BD0;
		break;
	case 435:
		mAnimEventCallback = fn_800B7CCC;
		break;
	case 486:
		mAnimEventCallback = fn_800B7DD4;
		break;
	case 614:
		mAnimEventCallback = fn_800B85B8;
		break;
	case 626:
		mAnimEventCallback = fn_800B86B8;
		break;
	case 628:
		mAnimEventCallback = fn_800B8748;
		break;
	case 629:
		mAnimEventCallback = fn_800B8840;
		break;
	case 630:
		mAnimEventCallback = fn_800B8938;
		break;
	case 786:
		mAnimEventCallback = fn_800B8A98;
		break;
	case 787:
		mAnimEventCallback = fn_800B8C78;
		break;
	case 832:
		mAnimEventCallback = fn_800B91C0;
		break;
	case 842:
		mAnimEventCallback = fn_800B9268;
		break;
	case 849:
		mAnimEventCallback = fn_800B92F8;
		break;
	case 854:
		mAnimEventCallback = fn_800B9C50;
		break;
	case 856:
		mAnimEventCallback = fn_800B9E6C;
		break;
	case 870:
		mAnimEventCallback = fn_800B9EF8;
		break;
	case 881:
		mAnimEventCallback = fn_800B9FE4;
		break;
	case 930:
		mAnimEventCallback = fn_800BA990;
		break;
	case 931:
		mAnimEventCallback = fn_800BAB88;
		break;
	case 932:
		mAnimEventCallback = fn_800BADC0;
		break;
	case 934:
		mAnimEventCallback = fn_800BAEB8;
		break;
	case 939:
		mAnimEventCallback = fn_800BAF60;
		break;
	case 995:
		mAnimEventCallback = fn_800BB208;
		break;
	case 1034:
		mAnimEventCallback = fn_800BB300;
		break;
	case 1035:
		mAnimEventCallback = fn_800BB3EC;
		break;
	case 1036:
		mAnimEventCallback = fn_800BB4F4;
		break;
	case 1037:
		mAnimEventCallback = fn_800BB864;
		break;
	case 1038:
		mAnimEventCallback = fn_800BBF0C;
		break;
	case 1082:
		mAnimEventCallback = fn_800BC0DC;
		break;
	case 1122:
		mAnimEventCallback = fn_800BC218;
		break;
	case 1155:
		mAnimEventCallback = fn_800BC494;
		break;
	case 1156:
		mAnimEventCallback = fn_800BCBC4;
		break;
	case 1161:
		mAnimEventCallback = fn_800BC964;
		break;
	case 855:
	default:
		mAnimEventCallback = NULL;
	}
}

#endif

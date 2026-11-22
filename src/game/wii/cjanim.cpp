// The unit between gxb.cpp and xobj_ut.cpp, .text 0x800B1558-0x800BD66C (wii_units.md
// Nr. 46): the object's collision floors, its screen projection virtuals and the
// event virtuals. The Wii source merges the GameCube's cjanim.cpp with the floor
// helpers obja.cpp and xobjfloor.cpp carried; the floor coordinate readers appear
// open-coded here, the array-and-index pair's inline half.
// NONMATCHING: the per-frame animation stepper fn_800B4988 (79.7 %), ExecCallback_CALC_MAT
// (97.0 %) and XObj2D::ExecCallbackA (97.3 %) still differ from retail; the last two only
// in register assignment (ExecCallbackA's mirror products).
#include "Version.h"
#if defined(VERSION_R24J01)
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf
#include "arithmetic.h"
#include "game/wii/cgame.h"
#include "game/wii/cursor.h"
#include "game/wii/xobj.h"
#include "game/wii/ceffect.h"
#include "JSystem/JMath/Inline.h"
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
// set from the TV format in init.cpp (50 or 60)
extern s32 gRefreshRate;

// the unit's one static registration: the shadow helper xobj_chr queries
// (retail bss 0x80456A5C, .ctors 0x802C45A8)
CustomShadow gCustomShadow;

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

// NONMATCHING: retail builds the result in a stack CVec and copies it to the sret slot;
// MWCC elides it into the sret object here (same source as the GameCube unit)
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
			if ((object->mFloors[i].mFlags & 1) && object->IsFloorOn(i)) {
				object->ClearFloor(i);
			}
		}
	}
}

static inline void ExpandRadius(f32 x, f32 z, f64& maxX, f64& maxZ)
{
	if (nw4r::math::FAbs(x) > maxX)
		maxX = nw4r::math::FAbs(x);
	if (nw4r::math::FAbs(z) > maxZ)
		maxZ = nw4r::math::FAbs(z);
}

extern "C" void UpdateObjectFloorXZExtent(XObj* object)
{
	object->mFloorXZExtent[0] = object->mFloorXZExtent[1] = NAN;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return;
	f64 maxX = 0.0, maxZ = 0.0;
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) {
		if (!(object->mFloors[i].mFlags & 1))
			continue;
		CVec origin((Vec) { 0, 0, 0 });
		Vec center  = object->mMoveScale * (object->mVec714 + GetFloorCenterOffset(&object->mFloors, i));
		Vec extent  = object->mMoveScale * GetFloorExtent(&object->mFloors, i);
		f32 baseYaw = object->mMoveRotate.y;
		f32 yaw     = baseYaw + GetFloorYawRadians(&object->mFloors, i);
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
			PSMTXRotRad(turn, 'y', GetFloorYawRadians(&object->mFloors, i));
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
		if (!(object->mFloors[i].mFlags & 1))
			continue;
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
		if (!(object->mFloors[i].mFlags & 1))
			continue;
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

// NONMATCHING: retail lays out the four f32 locals as x, scaledX, y, z in that order
// (an extra dead store of the scaled x survives); the rest matches
void XObj::ProjectOffsetToScreen(const Vec* offset, f32* x, f32* y, f32* z, CCamera* camera)
{
	camera       = camera != NULL ? camera : &gGame.mMotionControlCamera;
	Vec position = mMoveTranslate + *offset;
	f32 screenX, scaledX, screenY, screenZ;
	GXProject(position.x, position.y, position.z, camera->mViewMtx, &camera->mIsOrtho, camera->mViewport, &screenX, &screenY, &screenZ);
	scaledX = screenX * camera->mScale;
	if (x != NULL)
		*x = scaledX;
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

// NONMATCHING: retail assigns the locals different stack slots (node before the screen floats,
// a separate slot for the scaled screen x and a 0x50 frame instead of 0x40); control flow and
// instruction sequence already match
BOOL XObj::IsOutsideCameraDepth(CCamera* camera)
{
	if (mObject.m_pScnMdl != NULL) {
		nw4r::g3d::ResNode node = GetRootResNode();
		Vec position            = GetWorldPos(node);
		f32 screenX, screenY, screenZ;
		GXProject(position.x, position.y, position.z, camera->mViewMtx, &camera->mIsOrtho, camera->mViewport, &screenX, &screenY, &screenZ);
		screenX *= camera->mScale;
		if (screenZ > 0.0f && screenZ < 1.0f)
			return FALSE; // inside the depth range: visible
		return TRUE;
	}
	return TRUE;
}

static inline void ExpandScreenBounds(f32 x, f32 y, f32& minX, f32& minY, f32& maxX, f32& maxY)
{
	if (x < minX)
		minX = x;
	if (y < minY)
		minY = y;
	if (x > maxX)
		maxX = x;
	if (y > maxY)
		maxY = y;
}

// the on-screen bounds of every floor that takes part in pointer picking
extern "C" void fn_800B4988(XObj* object)
{
	if (object->mFloors == NULL || !(object->mStatus.mFlags & 0x100) || !object->mScreenBoundsValid)
		return;

	CCamera camera        = gGame.mMotionControlCamera;
	object->_6DC          = object->_6D8;
	object->_6D8          = 0;
	object->mScreenBottom = NAN;
	object->mScreenRight  = NAN;
	object->mScreenTop    = NAN;
	object->mScreenLeft   = NAN;
	object->mFloorTop     = NAN;
	object->mToppleOffset.Set(0.0f, 0.0f, 0.0f);

	s32 count = 0;
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) {
		XObjFloor* floor = &object->mFloors[i];
		if (!(floor->mFlags & 2) || floor->mId >= 3 || floor->mId < 0)
			continue;

		CVec pos   = object->mMoveTranslate;
		Vec center = object->mMoveScale * (object->mVec714 + GetFloorCenterOffset(&object->mFloors, i));
		Vec extent = object->mMoveScale * GetFloorExtent(&object->mFloors, i);
		f32 yaw    = object->mMoveRotate.y + GetFloorYawRadians(&object->mFloors, i);
		if (object->GetKind() == 0x2A0)
			pos = object->GetJointWorldPos(7);

		if (gGame.mStage == 14) {
			switch (object->_18C) {
			case 1:
			case 4:
				pos.y += 30.0f;
				break;
			case 3:
				extent.y *= 0.5f;
				break;
			case 10:
				extent.y *= 0.45f;
				pos.y -= 12.0f;
				break;
			}
		}

		Mtx transform, work, translate, scale;
		if (object->mBaseMtxPtr != NULL) {
			PSMTXTrans(translate, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXConcat(object->mBaseMtxPtr, translate, work);
			PSMTXConcat(work, scale, transform);
		} else {
			Mtx position, rotate;
			PSMTXTrans(position, pos.x, pos.y, pos.z);
			PSMTXTrans(translate, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(position, rotate, work);
			PSMTXConcat(work, translate, work);
			PSMTXConcat(work, scale, transform);
		}

		CVec origin(0.0f, 0.0f, 0.0f);
		PSMTXMultVec(transform, &origin, &origin);
		JMathInlineVEC::PSVECAdd(&object->mToppleOffset, &origin, &object->mToppleOffset);
		++count;

		f32 x, y, z;
		GXProject(origin.x, origin.y, origin.z, camera.mViewMtx, &camera.mIsOrtho, camera.mViewport, &x, &y, &z);
		x *= camera.mScale;
		if (z <= 0.0f || z >= 1.0f)
			continue;
		f32 minX = x, minY = y, maxX = x, maxY = y;
		f32 height = NAN;

		switch (floor->mId) {
		case 0:
		case 1: {
			f32 angle     = camera.mView.mAngle.y;
			f32 angles[4] = { angle, HALF_PI + angle, PI + angle, (PI + HALF_PI) + angle };
			f32 xs[4]     = { extent.x * (f32)sin(angles[0]), extent.x * (f32)sin(angles[1]), extent.x * (f32)sin(angles[2]),
				              extent.x * (f32)sin(angles[3]) };
			f32 zs[4]     = { extent.z * (f32)cos(angles[0]), extent.z * (f32)cos(angles[1]), extent.z * (f32)cos(angles[2]),
				              extent.z * (f32)cos(angles[3]) };
			Vec points[8] = {};
			points[0]     = origin + (Vec) { xs[0], extent.y, zs[0] };
			points[1]     = origin + (Vec) { xs[1], extent.y, zs[1] };
			points[2]     = origin + (Vec) { xs[2], extent.y, zs[2] };
			points[3]     = origin + (Vec) { xs[3], extent.y, zs[3] };
			points[4]     = origin + (Vec) { xs[0], -extent.y, zs[0] };
			points[5]     = origin + (Vec) { xs[1], -extent.y, zs[1] };
			points[6]     = origin + (Vec) { xs[2], -extent.y, zs[2] };
			points[7]     = origin + (Vec) { xs[3], -extent.y, zs[3] };
			for (s32 j = 0; j < 8; ++j) {
				GXProject(points[j].x, points[j].y, points[j].z, camera.mViewMtx, &camera.mIsOrtho, camera.mViewport, &x, &y, &z);
				x *= camera.mScale;
				if (z <= 0.0f || z >= 1.0f)
					goto nextFloor;
				ExpandScreenBounds(x, y, minX, minY, maxX, maxY);
			}
			height = origin.y + extent.y;
			break;
		}
		case 2: {
			f32 h = (2.0f / (3.0f * nw4r::math::FrSqrt(3.0f))) * 0.5f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, h }, { -h, h, h }, { -h, -h, -h }, { h, -h, -h }, { h, -h, h }, { -h, -h, h } };
			Vec points[8];
			PSMTXMultVecArray(transform, corners, points, 8);
			for (s32 j = 0; j < 8; ++j) {
				GXProject(points[j].x, points[j].y, points[j].z, camera.mViewMtx, &camera.mIsOrtho, camera.mViewport, &x, &y, &z);
				x *= camera.mScale;
				if (z <= 0.0f || z >= 1.0f)
					goto nextFloor;
				ExpandScreenBounds(x, y, minX, minY, maxX, maxY);
			}
			height = points[0].y - 0.01f;
			break;
		}
		}

		if (count > 0) {
			object->mToppleOffset.x /= count;
			object->mToppleOffset.y /= count;
			object->mToppleOffset.z /= count;
		}
		if (__fpclassifyf(minX) != FP_NAN && (__fpclassifyf(object->mScreenLeft) == FP_NAN || minX < object->mScreenLeft))
			object->mScreenLeft = minX;
		if (__fpclassifyf(minY) != FP_NAN && (__fpclassifyf(object->mScreenTop) == FP_NAN || minY < object->mScreenTop))
			object->mScreenTop = minY;
		if (__fpclassifyf(maxX) != FP_NAN && (__fpclassifyf(object->mScreenRight) == FP_NAN || maxX > object->mScreenRight))
			object->mScreenRight = maxX;
		if (__fpclassifyf(maxY) != FP_NAN && (__fpclassifyf(object->mScreenBottom) == FP_NAN || maxY > object->mScreenBottom))
			object->mScreenBottom = maxY;
		if (__fpclassifyf(object->mFloorTop) == FP_NAN || height > object->mFloorTop)
			object->mFloorTop = height;

	nextFloor:;
	}
}

// the object-world-callback's first pass: with the 0x800 flag the model draws mirrored - the
// move scale's X and Z negate, Y flips with the screen camera's orientation, and the yaw gains pi
void XObj::ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	if (mStatus.mFlags & 0x800 && pFuncObj->GetCallbackNodeID() == 0) {
		CCamera* camera = gGame.GetScreenCamera();
		f32 bottom      = camera->mBottom;
		f32 top         = camera->mTop;
		s32 sign        = top < bottom ? -1 : 1;
		f32 scaleY      = mMoveScale.y;
		f32 scaleX      = mMoveScale.x;
		f32 scaleZ      = mMoveScale.z;
		f32 rotateY     = mMoveRotate.y;
		mMoveScale.x *= -1.0f;
		mMoveScale.y *= sign;
		mMoveScale.z *= -1.0f;
		mMoveRotate.y += PI;
		CObj::ExecCallbackA(pResult, mdl, pFuncObj);
		mMoveScale.y  = scaleY;
		mMoveScale.x  = scaleX;
		mMoveScale.z  = scaleZ;
		mMoveRotate.y = rotateY;
	} else {
		CObj::ExecCallbackA(pResult, mdl, pFuncObj);
	}
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

// the tint stage the game appends to every material (NW_Obj::InitColorTev): on the map and
// the tintable object kinds ExecCallback_CALC_MAT rewrites its last stage's inputs, on the
// rest it passes the previous color through. The padding byte the marker lives in is the
// original struct's dummy[6]; the asserts name the original files, xobj.cpp for the game's
// own checks and g3d_resmat_ac.h for the resource accessors'.
#define UPDATED_TEV_MAGIC_NUMBER    0xC3
#define XOBJ_ASSERT_LINE(line, exp) ((void)((exp) || (nw4r::db::Panic("xobj.cpp", line, "NW4R:Failed assertion " #exp), 0)))

void XObj::ExecCallback_CALC_MAT(int timing, nw4r::g3d::ScnObj* obj, u32 param, void* info)
{
	if (timing == 4 && (mStatus.mFlags & 0x2)) {
		u32 numMats = mObject.m_resMdl.GetResMatNumEntries();
		if (numMats != 0) {
			for (u32 i = 0; i < numMats; i++) {
				nw4r::g3d::ResMat mat = mObject.m_resMdl.GetResMat(i);
				nw4r::g3d::ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
				nw4r::g3d::ResTev tev(NULL);
				tev = access.GetResTev(false);
				if (!tev.IsValid()) {
					tev = mat.GetResTev();
				}
				XOBJ_ASSERT_LINE(0xF0E, tev.IsValid());
				if (tev.ptr()->PADDING_0x18[6] == UPDATED_TEV_MAGIC_NUMBER) {
					nw4r::g3d::ResGenMode genMode = access.GetResGenMode(false);
					if (!genMode.IsValid()) {
						genMode = mat.GetResGenMode();
					}
					XOBJ_ASSERT_LINE(0xF15, genMode.IsValid());
					nw4r::g3d::ResTexSrt texSrt = access.GetResTexSrt(false);
					if (!texSrt.IsValid()) {
						texSrt = mat.GetResTexSrt();
					}
					XOBJ_ASSERT_LINE(0xF1B, texSrt.IsValid());
					// the last tex coord's flag nibble, TexSrt::FLAGSET_IDENTITY
					texSrt.SetTexSrtFlag(genMode.GXGetNumTexGens() - 1, 0xF);
					GXTevStageID stage = (GXTevStageID)(genMode.GXGetNumTevStages() - 1);
					s32 floorIdx       = gGame.GetPlayer()->mFloorContactObjects[0];
					if ((mAttr.mObjData != NULL ? mAttr.mObjData->GetCategory() : -1) == 3) {
						// the map: blend while the object the player stands on is a
						// tintable kind, or while he is within a meter above the map
						if (floorIdx >= 0) {
							XObj* stood = gGame.mXObjs.mObjects[floorIdx];
							switch (stood->GetKind()) {
							case 0x20:
							case 0x54:
							case 0x10C:
								tev.GXSetTevColorIn(stage, GX_CC_CPREV, GX_CC_KONST, GX_CC_TEXA, GX_CC_ZERO);
								break;
							default:
								if (gGame.GetPlayer()->GetPos()->y - GetPos()->y < 1.0f) {
									tev.GXSetTevColorIn(stage, GX_CC_CPREV, GX_CC_KONST, GX_CC_TEXA, GX_CC_ZERO);
								} else {
									tev.GXSetTevColorIn(stage, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
								}
							}
						} else {
							tev.GXSetTevColorIn(stage, GX_CC_CPREV, GX_CC_KONST, GX_CC_TEXA, GX_CC_ZERO);
						}
						tev.DCStore(false);
					} else {
						switch (GetKind()) {
						case 0x20:
						case 0x54:
						case 0x10C:
							if (floorIdx < 0 || floorIdx == GetStagePlacementIndex(mCoord.mPlacement)) {
								tev.GXSetTevColorIn(stage, GX_CC_CPREV, GX_CC_KONST, GX_CC_TEXA, GX_CC_ZERO);
							} else {
								tev.GXSetTevColorIn(stage, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
							}
							tev.DCStore(false);
						}
					}
				}
			}
		}
	}
	CObj::ExecCallback_CALC_MAT(timing, obj, param, info);
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
	if (fn_800C98E0(map, object->GetPos(), -1, &object->mFloorHit)) {
		if (object->mFloorHit.mHeight < minHeight)
			object->mFloorHit.mHeight = minHeight;
		return TRUE;
	}
	return FALSE;
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
		self->mFloorHit.mHeight   = self->mMoveTranslate.y;
	}
	f32 best = self->mFloorHit.mHeight;
	CVec push, contact;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.mXObjs.mObjects[i];
		if (!(object->mStatus.mFlags & 0x1) || !(object->mStatus.mFlags & 0x10000) || object->mStatus.mFlags & 0x100000 || object == self) {
			continue;
		}
		CVec* otherPos = object->GetPos();
		f32 dx         = otherPos->x - pos->x;
		f32 dz         = otherPos->z - pos->z;
		f32 dist       = FSqrt(dx * dx + dz * dz);
		if (dist > 100.0f + (radius + object->mFloorXZExtent[0]))
			continue;
		s32 cursor = 0;
		while (cursor != -1) {
			CVec from(*pos), to(*pos);
			u32 hit = object->TestFloorCollision(&from, &to, radius, height, &push, &contact, &cursor, 0);
			if (hit & 1) {
				f32 floorHeight = object->mFloorHeights[object->mHitFloorIndex];
				if (__fpclassifyf(floorHeight) != FP_NAN && self->mMoveTranslate.y + 1.0f >= floorHeight && floorHeight >= best)
					best = floorHeight;
			}
		}
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
		mAnmQueue.mColorStep[index].mSpeed.x = ((f32)red - mAnmQueue.mColorStep[index].mValue.x) / (gRefreshRate * time);
		mAnmQueue.mColorStep[index].mSpeed.y = ((f32)green - mAnmQueue.mColorStep[index].mValue.y) / (gRefreshRate * time);
		mAnmQueue.mColorStep[index].mSpeed.z = ((f32)blue - mAnmQueue.mColorStep[index].mValue.z) / (gRefreshRate * time);
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
// NONMATCHING (96 %): retail leaves the record loop with the name in a local and tests it
// once more before it returns; that shape is not recovered
static const char* FindAnimNodeName(XObj* object, s32 count, s32* args)
{
	u64 key = 1;
	for (s32 i = 0; i < count; ++i)
		key = key << 4 | (args[i] & 0xFLL);
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

// kind 397: the particle burst on the ptcl5..ptcl18 nodes
static void AnimEventPtcl397(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (prev < 149.0f && frame >= 149.0f) {
		StartNodeEffect(object, "ptcl5", 0x36, 0);
		StartNodeEffect(object, "ptcl6", 0x36, 0);
		StartNodeEffect(object, "ptcl7", 0x36, 0);
		StartNodeEffect(object, "ptcl8", 0x36, 0);
		StartNodeEffect(object, "ptcl9", 0x36, 0);
		StartNodeEffect(object, "ptcl10", 0x36, 0);
		StartNodeEffect(object, "ptcl12", 0x36, 0);
		StartNodeEffect(object, "ptcl13", 0x36, 0);
		StartNodeEffect(object, "ptcl16", 0x36, 0);
		StartNodeEffect(object, "ptcl17", 0x36, 0);
		StartNodeEffect(object, "ptcl18", 0x36, 0);
	}
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

// kind 428: the eyes' sparkle on every eye node
static void AnimEventEyes(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f)) {
		StartNodeEffect(object, "eye_l", 0x33, 1);
		StartNodeEffect(object, "eye_l1", 0x33, 1);
		StartNodeEffect(object, "eye_l2", 0x33, 1);
		StartNodeEffect(object, "eye_r1", 0x33, 1);
		StartNodeEffect(object, "eye_r2", 0x33, 1);
		StartNodeEffect(object, "eye_r3", 0x33, 1);
		StartNodeEffect(object, "eye_r4", 0x33, 1);
		StartNodeEffect(object, "eye_r5", 0x33, 1);
		StartNodeEffect(object, "eye_r6", 0x33, 1);
		StartNodeEffect(object, "eye_r7", 0x33, 1);
		StartNodeEffect(object, "eye_r8", 0x33, 1);
		StartNodeEffect(object, "eye_r9", 0x33, 1);
		StartNodeEffect(object, "eye_r10", 0x33, 1);
		StartNodeEffect(object, "eye_r11", 0x33, 1);
		StartNodeEffect(object, "eye_r12", 0x33, 1);
		StartNodeEffect(object, "eye_r13", 0x33, 1);
		StartNodeEffect(object, "eye_r14", 0x33, 1);
		StartNodeEffect(object, "eye_r15", 0x33, 1);
		StartNodeEffect(object, "eye_r16", 0x33, 1);
	}
}

// kind 431: the same diary effect on two different events
static void AnimEventBbNikki(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "bb_nikki", 0x46, 0);
	} else if (kind == 2 && prev < 51.0f && frame >= 51.0f) {
		StartNodeEffect(object, "bb_nikki", 0x46, 0);
	}
}

// kind 435: both scare effects at once
static void AnimEventTompScare(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f)) {
		StartNodeEffect(object, "fx_tomp_scare3", 0x38, 1);
		StartNodeEffect(object, "fx_tomp_scare4", 0x38, 1);
	}
}

// kind 486: the fan's right, left and center blades
static void AnimEventFan(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "migi1", 0x85, 0);
		StartNodeEffect(object, "migi2", 0x85, 0);
		StartNodeEffect(object, "migi3", 0x85, 0);
		StartNodeEffect(object, "migi4", 0x85, 0);
		StartNodeEffect(object, "migi5", 0x85, 0);
	} else if (kind == 2) {
		if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "hidari1", 0x86, 0);
		} else if (prev < 7.0f && frame >= 7.0f) {
			StartNodeEffect(object, "hidari2", 0x86, 0);
		} else if (prev < 14.0f && frame >= 14.0f) {
			StartNodeEffect(object, "hidari3", 0x86, 0);
		} else if (prev < 21.0f && frame >= 21.0f) {
			StartNodeEffect(object, "hidari4", 0x86, 0);
		} else if (prev < 28.0f && frame >= 28.0f) {
			StartNodeEffect(object, "hidari5", 0x86, 0);
		}
	} else if (kind == 3 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "center", 0x89, 0);
	} else if (kind == 4) {
		if (prev < 39.0f && frame >= 39.0f) {
			StartNodeEffect(object, "hidari1", 0x8a, 0);
			StartNodeEffect(object, "hidari1", 0x8b, 0);
		} else if (prev < 20.0f && frame >= 20.0f) {
			StartNodeEffect(object, "hidari2", 0x8a, 0);
			StartNodeEffect(object, "hidari2", 0x8b, 0);
			StartNodeEffect(object, "hidari3", 0x8a, 0);
			StartNodeEffect(object, "hidari3", 0x8b, 0);
		} else if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "hidari4", 0x8a, 0);
			StartNodeEffect(object, "hidari4", 0x8b, 0);
			StartNodeEffect(object, "hidari5", 0x8a, 0);
			StartNodeEffect(object, "hidari5", 0x8b, 0);
		}
	}
}

// kind 614: warp-in on event 2, warp-out on event 3
static void AnimEventLeticWarp(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 2 && prev < 3.0f && frame >= 3.0f) {
		StartNodeEffect(object, "fx_leticwarp_in_ptcl", 0x3d, 0);
	} else if (kind == 3 && prev < 3.0f && frame >= 3.0f) {
		StartNodeEffect(object, "fx_leticwarp_out_ptcl", 0x3e, 0);
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

// kind 628: the frog's two jumps
static void AnimEventZactFrog(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (prev < 15.0f && frame >= 15.0f) {
		StartNodeEffect(object, "fx_zact_frog_ptcl", 0x40, 1);
	} else if (prev < 29.0f && frame >= 29.0f) {
		StartNodeEffect(object, "fx_zact_frog_ptcl2", 0x41, 1);
	}
}

// kind 629: the injury shock first, the lasting effect after
static void AnimEventZactInjured(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (prev < 18.0f && frame >= 18.0f) {
		StartNodeEffect(object, "fx_zact_injured_shock", 0x40, 1);
	} else if (prev < 14.0f && frame >= 14.0f) {
		StartNodeEffect(object, "fx_zact_injured_ptcl", 0x40, 1);
	}
}

// kind 630: the pajamas' effect at three frames
static void AnimEventPajamas(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (prev < 2.0f && frame >= 2.0f) {
		StartNodeEffect(object, "fx_pajamas_ptcl", 0x16, 1);
	} else if (prev < 22.0f && frame >= 22.0f) {
		StartNodeEffect(object, "fx_pajamas_ptcl", 0x16, 1);
	} else if (prev < 42.0f && frame >= 42.0f) {
		StartNodeEffect(object, "fx_pajamas_ptcl", 0x16, 1);
	}
}

// kind 786: one effect per event kind on the same node
static void AnimEventFxPtcl(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1 && prev < 2.0f && frame >= 2.0f) {
		StartNodeEffect(object, "fx_ptcl", 0x0, 0);
	} else if (kind == 2 && prev < 2.0f && frame >= 2.0f) {
		StartNodeEffect(object, "fx_ptcl", 0x12, 0);
	} else if (kind == 3 && prev < 2.0f && frame >= 2.0f) {
		StartNodeEffect(object, "fx_ptcl", 0x13, 0);
	} else if (kind == 4 && prev < 2.0f && frame >= 2.0f) {
		StartNodeEffect(object, "fx_ptcl", 0x4a, 0);
	}
}

// kind 787: the warp zone and its charge-ok nodes
static void AnimEventWarpzone(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1 && ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f))) {
		StartNodeEffect(object, "fx_warpzone_ptcl", 0x48, 1);
	} else if (kind == 3 && ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f))) {
		StartNodeEffect(object, "chrgeOK1", 0xab, 0);
		StartNodeEffect(object, "chrgeOK2", 0xab, 0);
		StartNodeEffect(object, "chrgeOK3", 0xab, 0);
	} else if (kind == 4) {
		if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f)) {
			StartNodeEffect(object, "chrgeOK1", 0xac, 0);
			StartNodeEffect(object, "chrgeOK2", 0xac, 0);
			StartNodeEffect(object, "chrgeOK3", 0xac, 0);
		} else if (prev < 41.0f && frame >= 41.0f) {
			StartNodeEffect(object, "chrgeOK1", 0xac, 0);
			StartNodeEffect(object, "chrgeOK2", 0xac, 0);
			StartNodeEffect(object, "chrgeOK3", 0xac, 0);
		}
	} else if (kind == 5 && ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f))) {
		StartNodeEffect(object, "chrgeOK1", 0xac, 0);
		StartNodeEffect(object, "chrgeOK2", 0xac, 0);
		StartNodeEffect(object, "chrgeOK3", 0xac, 0);
	}
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

// kind 849: the torch stand's particle nodes
static void AnimEventTorch849(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1) {
		if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "ptcl1", 0x53, 0);
			StartNodeEffect(object, "ptcl2", 0x53, 0);
			StartNodeEffect(object, "ptcl3", 0x53, 0);
			StartNodeEffect(object, "ptcl4", 0x53, 0);
			StartNodeEffect(object, "ptcl5", 0x53, 0);
			StartNodeEffect(object, "ptcl6", 0x53, 0);
			StartNodeEffect(object, "ptcl7", 0x53, 0);
			StartNodeEffect(object, "ptcl8", 0x53, 0);
			StartNodeEffect(object, "ptcl9", 0x53, 0);
			StartNodeEffect(object, "ptcl10", 0x53, 0);
		}
	} else if (kind == 2) {
		if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "torch_ptcl", 0x79, 0);
		} else if (prev < 160.0f && frame >= 160.0f) {
			StartNodeEffect(object, "torch_ptcl", 0x79, 0);
		}
	} else if (kind == 3) {
		if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "torch_ptcl", 0x79, 0);
		} else if (prev < 59.0f && frame >= 59.0f) {
			StartNodeEffect(object, "torch_ptcl", 0x4b, 0);
			StartNodeEffect(object, "torch_ptcl", 0x4c, 0);
		}
	} else if (kind == 4) {
		if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "torch_ptcl", 0x79, 0);
		} else if (prev < 59.0f && frame >= 59.0f) {
			StartNodeEffect(object, "torch_ptcl", 0x4b, 0);
			StartNodeEffect(object, "torch_ptcl", 0x4c, 0);
		}
	} else if (kind == 5) {
		if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "torch_ptcl", 0x79, 0);
		} else if (prev < 59.0f && frame >= 59.0f) {
			StartNodeEffect(object, "torch_ptcl", 0x4b, 0);
			StartNodeEffect(object, "torch_ptcl", 0x4c, 0);
		}
	} else if (kind == 6) {
		if (frame == 0.0f || prev > frame)
			StartNodeEffect(object, "torch_ptcl", 0x79, 0);
	} else if (kind == 7) {
		if (prev < 289.0f && frame >= 289.0f) {
			StartNodeEffect(object, "ptcl1", 0x53, 0);
			StartNodeEffect(object, "ptcl2", 0x53, 0);
		} else if (prev < 49.0f && frame >= 49.0f) {
			StartNodeEffect(object, "ptcl11", 0x54, 0);
		}
	}
}

// kind 854: the torch, lit and burning
static void AnimEventTorchPtcl(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1) {
		if (prev < 2.0f && frame >= 2.0f) {
			StartNodeEffect(object, "torch_ptcl", 0x4b, 0);
			StartNodeEffect(object, "torch_ptcl", 0x4c, 0);
		}
	} else if (kind == 3) {
		if (prev < 2.0f && frame >= 2.0f) {
			StartNodeEffect(object, "torch_ptcl", 0x4b, 0);
			StartNodeEffect(object, "torch_ptcl", 0x4c, 0);
		}
	} else if (kind == 4) {
		if (prev < 2.0f && frame >= 2.0f) {
			StartNodeEffect(object, "torch_ptcl", 0x4b, 0);
		}
	}
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

// kind 881: the eye short circuit and battery effects
static void AnimEventEyeShort(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1 && ((prev < 0.0f && frame >= 0.0f) || prev > frame)) {
		StartNodeEffect(object, "elec_ptcl2", 0xf, 0);
	} else if (kind == 3 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "air_ptcl1", 0x5d, 0);
	} else if (kind == 4 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "fx_deca_battery2", 0x1a, 0);
	} else if (kind == 5 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "fx_deca_battery2", 0x5e, 0);
	} else if (kind == 6) {
		if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f)) {
			StartNodeEffect(object, "eye_short1", 0x91, 0);
			StartNodeEffect(object, "eye_short3", 0x91, 0);
		} else if (prev < 3.0f && frame >= 3.0f) {
			StartNodeEffect(object, "eye_short2", 0x91, 0);
		} else if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "eye_short5", 0x91, 0);
		}
	} else if (kind == 7) {
		if (prev < 3.0f && frame >= 3.0f) {
			StartNodeEffect(object, "eye_short1", 0x92, 0);
			StartNodeEffect(object, "eye_short2", 0x92, 0);
		} else if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f)) {
			StartNodeEffect(object, "eye_short3", 0x92, 0);
			StartNodeEffect(object, "eye_short4", 0x92, 0);
		} else if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "eye_short5", 0x92, 0);
		}
	} else if (kind == 8 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "eye_short1", 0x92, 0);
		StartNodeEffect(object, "eye_short2", 0x92, 0);
		StartNodeEffect(object, "eye_short3", 0x92, 0);
		StartNodeEffect(object, "eye_short4", 0x92, 0);
		StartNodeEffect(object, "eye_short6", 0x93, 0);
		StartNodeEffect(object, "eye_short7", 0x93, 0);
	} else if (kind == 9) {
		if (prev < 34.0f && frame >= 34.0f) {
			StartNodeEffect(object, "eye_short1", 0x94, 0);
		} else if (prev < 10.0f && frame >= 10.0f) {
			StartNodeEffect(object, "eye_short4", 0x94, 0);
		} else if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "eye_short5", 0x94, 0);
		} else if (prev < 49.0f && frame >= 49.0f) {
			StartNodeEffect(object, "fx_deca_battery2", 0x96, 0);
			StartNodeEffect(object, "fx_deca_battery2", 0x95, 0);
		}
	}
}

// kind 930: the twin's start and first effects
static void AnimEventGiccoTwinB01(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1) {
		if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f)) {
			StartNodeEffect(object, "fx_gicco_twinb01_ptcl1", 0x9d, 0);
		} else if (prev < 2.0f && frame >= 2.0f) {
			StartNodeEffect(object, "first_ptcl2", 0xa0, 0);
		}
	} else if (kind == 2 && prev < 2.0f && frame >= 2.0f) {
		StartNodeEffect(object, "first_ptcl01", 0x57, 0);
	} else if (kind == 3 && ((prev < 0.0f && frame >= 0.0f) || prev > frame)) {
		StartNodeEffect(object, "first_ptcl2", 0xa1, 0);
	}
}

// kind 931: the toolbox's three nodes, two effects each
static void AnimEventTb01(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if ((prev < 0.0f && frame >= 0.0f) || prev > frame) {
		StartNodeEffect(object, "tb01_ptcl01", 0xa2, 0);
		StartNodeEffect(object, "tb01_ptcl01", 0x1, 0);
		StartNodeEffect(object, "tb01_ptcl2", 0xa2, 0);
		StartNodeEffect(object, "tb01_ptcl2", 0x1, 0);
		StartNodeEffect(object, "tb01_ptcl3", 0xa2, 0);
		StartNodeEffect(object, "tb01_ptcl3", 0x1, 0);
	}
}

// kind 932: the twin's two effects, far apart in the animation
static void AnimEventGiccoTwinB03(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (prev < 199.0f && frame >= 199.0f) {
		StartNodeEffect(object, "fx_gicco_twinb03_ptcl", 0x9e, 0);
	} else if (prev < 19.0f && frame >= 19.0f) {
		StartNodeEffect(object, "ptcl1", 0x9f, 0);
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

// kind 939: catching a cold, then the beam's stages
static void AnimEventCatchACold(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "catch_a_cold_ptcl", 0x4d, 0);
	} else if (kind == 3 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "beam_ptcl", 0x58, 0);
	} else if (kind == 4 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "beam_ptcl", 0x58, 0);
	} else if (kind == 5 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "beam_ptcl", 0x58, 0);
	} else if (kind == 6 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "beam_ptcl", 0x59, 0);
	} else if (kind == 7 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "beam_ptcl", 0x59, 0);
	}
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

// kind 1035: the pirate boat's two effects at once
static void AnimEventPirateBoat(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f)) {
		StartNodeEffect(object, "fx_pirateboat_ptcl", 0x64, 0);
		StartNodeEffect(object, "fx_pirateboat_ptcl1", 0x64, 0);
	}
}

// kind 1036: the energy node's charge-up sparks
static void AnimEventEnergyPtcl(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 2 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "energy_ptcl_root", 0x6f, 0);
	} else if (kind == 3) {
		if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "energy_ptcl_root", 0x6f, 0);
		} else if ((prev < 12.0f && frame >= 12.0f) || (prev < 24.0f && frame >= 24.0f) || (prev < 36.0f && frame >= 36.0f)
		           || (prev < 48.0f && frame >= 48.0f) || (prev < 60.0f && frame >= 60.0f) || (prev < 72.0f && frame >= 72.0f)
		           || (prev < 84.0f && frame >= 84.0f) || (prev < 96.0f && frame >= 96.0f) || (prev < 108.0f && frame >= 108.0f)
		           || (prev < 120.0f && frame >= 120.0f) || (prev < 132.0f && frame >= 132.0f) || (prev < 144.0f && frame >= 144.0f)
		           || (prev < 156.0f && frame >= 156.0f) || (prev < 168.0f && frame >= 168.0f) || (prev < 180.0f && frame >= 180.0f)
		           || (prev < 192.0f && frame >= 192.0f) || (prev < 204.0f && frame >= 204.0f) || (prev < 216.0f && frame >= 216.0f)
		           || (prev < 228.0f && frame >= 228.0f) || (prev < 240.0f && frame >= 240.0f) || (prev < 252.0f && frame >= 252.0f)
		           || (prev < 264.0f && frame >= 264.0f) || (prev < 276.0f && frame >= 276.0f)) {
			StartNodeEffect(object, "energy_ptcl_root", 0x6f, 0);
		}
	}
}

// kind 1037: the matrix and explosion particles
static void AnimEventPtclExp1(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1 && ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f))) {
		StartNodeEffect(object, "ptcl_matrix1", 0x6e, 0);
		StartNodeEffect(object, "ptcl_matrix2", 0x6e, 0);
	} else if (kind == 2 && ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f))) {
		StartNodeEffect(object, "ptcl_electric", 0x75, 0);
	} else if (kind == 3 && ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f))) {
		StartNodeEffect(object, "ptcl_exp1", 0x76, 0);
	} else if (kind == 4) {
		if ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f)) {
			StartNodeEffect(object, "ptcl_exp1", 0x77, 0);
			StartNodeEffect(object, "ptcl_exp1", 0x76, 0);
		} else if (prev < 10.0f && frame >= 10.0f) {
			StartNodeEffect(object, "ptcl_exp1", 0x77, 0);
			StartNodeEffect(object, "ptcl_exp1", 0x76, 0);
		} else if (prev < 20.0f && frame >= 20.0f) {
			StartNodeEffect(object, "ptcl_exp1", 0x77, 0);
			StartNodeEffect(object, "ptcl_exp1", 0x76, 0);
		} else if (prev < 30.0f && frame >= 30.0f) {
			StartNodeEffect(object, "ptcl_exp1", 0x77, 0);
			StartNodeEffect(object, "ptcl_exp1", 0x76, 0);
		} else if (prev < 40.0f && frame >= 40.0f) {
			StartNodeEffect(object, "ptcl_exp1", 0x77, 0);
			StartNodeEffect(object, "ptcl_exp1", 0x76, 0);
		} else if (prev < 50.0f && frame >= 50.0f) {
			StartNodeEffect(object, "ptcl_exp1", 0x77, 0);
			StartNodeEffect(object, "ptcl_exp1", 0x76, 0);
		}
	}
}

// kind 1038: the red effect, one per event kind
static void AnimEventRedPtcl(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "red_ptcl", 0x71, 0);
	} else if (kind == 2 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "red_ptcl", 0x72, 0);
	} else if (kind == 3 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "red_ptcl", 0x73, 0);
	} else if (kind == 4 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "red_ptcl", 0x74, 0);
	}
}

// kind 1082: the explosion's three phases on one node
static void AnimEventPtclExp02(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (frame == 0.0f || prev > frame) {
		StartNodeEffect(object, "ptcl_exp02", 0x9, 0);
		StartNodeEffect(object, "ptcl_exp02", 0x7, 0);
		StartNodeEffect(object, "ptcl_exp02", 0x8, 0);
	}
}

// kind 1122: the takeoff's seven stages
static void AnimEventTmTakeoff(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind != 1)
		return;
	if (frame == 0.0f || prev > frame) {
		StartNodeEffect(object, "fx_tm_takeoff1", 0x97, 0);
		StartNodeEffect(object, "fx_tm_takeoff2", 0x97, 0);
		StartNodeEffect(object, "fx_tm_takeoff3", 0x97, 0);
		StartNodeEffect(object, "fx_tm_takeoff4", 0x97, 0);
		StartNodeEffect(object, "fx_tm_takeoff5", 0x97, 0);
		StartNodeEffect(object, "fx_tm_takeoff6", 0x97, 0);
		StartNodeEffect(object, "fx_tm_takeoff7", 0x97, 0);
	}
}

// kind 1155: the laser shock's smoke and the robot's reaction effects
static void AnimEventLaserShock(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 1) {
		if (prev < 13.0f && frame >= 13.0f) {
			StartNodeEffect(object, "ptcl1_laser_shock_smoke1", 0xaa, 0);
		} else if (prev < 179.0f && frame >= 179.0f) {
			StartNodeEffect(object, "ptcl1_laser_shock_smoke1", 0xaa, 0);
		} else if (prev < 199.0f && frame >= 199.0f) {
			StartNodeEffect(object, "ptcl1_laser_shock_smoke1", 0xaa, 0);
		} else if (prev < 219.0f && frame >= 219.0f) {
			StartNodeEffect(object, "ptcl1_laser_shock_smoke1", 0xaa, 0);
		} else if (prev < 79.0f && frame >= 79.0f) {
			StartNodeEffect(object, "ptcl1_laser_energyfall1", 0x58, 0);
		}
	} else if (kind == 2 && prev < 17.0f && frame >= 17.0f) {
		StartNodeEffect(object, "fx_chibirobo2", 0xb8, 0);
		StartNodeEffect(object, "fx_chibirobo3", 0xb8, 0);
	} else if (kind == 3 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "fx_chibirobo_bright1", 0xb9, 0);
	} else if (kind == 4) {
		if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "fx_chibirobo_bright1", 0xba, 0);
		} else if ((prev < 144.0f && frame >= 144.0f) || prev > frame) {
			StartNodeEffect(object, "fx_chibirobo_spark1", 0xbb, 0);
		}
	} else if (kind == 5 && (frame == 0.0f || prev > frame)) {
		StartNodeEffect(object, "fx_chibirobo_bright1", 0xb7, 0);
	}
}

// kind 1161: the spheres' effect, stopped while the object is off
extern "C" void fn_800A12F8(EffectInstance* effects, u32 id, BOOL retireParticles);
static void AnimEventSphere(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (!(object->mStatus.mFlags & 0x10000)) {
		if (kind == 0)
			fn_800A12F8((EffectInstance*)&gGame.mEvtPtcl, 0xbd, TRUE);
	} else if (kind == 1) {
		if (frame == 0.0f || prev > frame) {
			StartNodeEffect(object, "sphere10", 0xbd, 0);
			StartNodeEffect(object, "sphere11", 0xbd, 0);
			StartNodeEffect(object, "sphere12", 0xbd, 0);
			StartNodeEffect(object, "sphere13", 0xbd, 0);
			StartNodeEffect(object, "sphere14", 0xbd, 0);
			StartNodeEffect(object, "sphere15", 0xbd, 0);
		}
	}
}

// kind 1156: the deca robot's neck, socket and battery box effects
static void AnimEventDeca(XObj* object, s32 kind, f32 prev, f32 frame)
{
	if (kind == 2 && ((prev < 1.0f && frame >= 1.0f) || (prev > frame && frame >= 1.0f))) {
		StartNodeEffect(object, "fx_deca_neck1", 0xae, 0);
		StartNodeEffect(object, "deca_socket1", 0xaf, 0);
		StartNodeEffect(object, "deca_btrybox1", 0xaf, 0);
	} else if (kind == 3) {
		if (prev < 2.0f && frame >= 2.0f) {
			StartNodeEffect(object, "fx_deca_neck1", 0xae, 0);
			StartNodeEffect(object, "deca_btrybox1", 0xaf, 0);
		} else if (prev < 31.0f && frame >= 31.0f) {
			StartNodeEffect(object, "fx_deca_neck1", 0xae, 0);
			StartNodeEffect(object, "deca_socket1", 0xaf, 0);
			StartNodeEffect(object, "deca_btrybox1", 0xaf, 0);
		} else if (prev < 61.0f && frame >= 61.0f) {
			StartNodeEffect(object, "fx_deca_neck1", 0xae, 0);
			StartNodeEffect(object, "deca_socket1", 0xaf, 0);
		} else if (prev < 91.0f && frame >= 91.0f) {
			StartNodeEffect(object, "fx_deca_neck1", 0xae, 0);
			StartNodeEffect(object, "deca_socket1", 0xaf, 0);
		}
	}
}

// the kind's anim keyframe hook picker (name invented): switches on GetKind()
void XObj::AssignAnimEventCallback()
{
	switch (GetKind()) {
	case 302:
		mAnimEventCallback = AnimEventCyl295;
		break;
	case 397:
		mAnimEventCallback = AnimEventPtcl397;
		break;
	case 405:
		mAnimEventCallback = AnimEventKettle;
		break;
	case 406:
		mAnimEventCallback = AnimEventTeacup;
		break;
	case 424:
		mAnimEventCallback = AnimEventPtcl;
		break;
	case 428:
		mAnimEventCallback = AnimEventEyes;
		break;
	case 431:
		mAnimEventCallback = AnimEventBbNikki;
		break;
	case 435:
		mAnimEventCallback = AnimEventTompScare;
		break;
	case 486:
		mAnimEventCallback = AnimEventFan;
		break;
	case 614:
		mAnimEventCallback = AnimEventLeticWarp;
		break;
	case 626:
		mAnimEventCallback = AnimEventZactGicco;
		break;
	case 628:
		mAnimEventCallback = AnimEventZactFrog;
		break;
	case 629:
		mAnimEventCallback = AnimEventZactInjured;
		break;
	case 630:
		mAnimEventCallback = AnimEventPajamas;
		break;
	case 786:
		mAnimEventCallback = AnimEventFxPtcl;
		break;
	case 787:
		mAnimEventCallback = AnimEventWarpzone;
		break;
	case 832:
		mAnimEventCallback = AnimEventGhost;
		break;
	case 842:
		mAnimEventCallback = AnimEventCbDoor;
		break;
	case 849:
		mAnimEventCallback = AnimEventTorch849;
		break;
	case 854:
		mAnimEventCallback = AnimEventTorchPtcl;
		break;
	case 856:
		mAnimEventCallback = AnimEventWeldingTools;
		break;
	case 870:
		mAnimEventCallback = AnimEventObj30;
		break;
	case 881:
		mAnimEventCallback = AnimEventEyeShort;
		break;
	case 930:
		mAnimEventCallback = AnimEventGiccoTwinB01;
		break;
	case 931:
		mAnimEventCallback = AnimEventTb01;
		break;
	case 932:
		mAnimEventCallback = AnimEventGiccoTwinB03;
		break;
	case 934:
		mAnimEventCallback = AnimEventRain;
		break;
	case 939:
		mAnimEventCallback = AnimEventCatchACold;
		break;
	case 995:
		mAnimEventCallback = AnimEventSecond;
		break;
	case 1034:
		mAnimEventCallback = AnimEventPtcl01;
		break;
	case 1035:
		mAnimEventCallback = AnimEventPirateBoat;
		break;
	case 1036:
		mAnimEventCallback = AnimEventEnergyPtcl;
		break;
	case 1037:
		mAnimEventCallback = AnimEventPtclExp1;
		break;
	case 1038:
		mAnimEventCallback = AnimEventRedPtcl;
		break;
	case 1082:
		mAnimEventCallback = AnimEventPtclExp02;
		break;
	case 1122:
		mAnimEventCallback = AnimEventTmTakeoff;
		break;
	case 1155:
		mAnimEventCallback = AnimEventLaserShock;
		break;
	case 1156:
		mAnimEventCallback = AnimEventDeca;
		break;
	case 1161:
		mAnimEventCallback = AnimEventSphere;
		break;
	case 855:
	default:
		mAnimEventCallback = NULL;
	}
}

// the screen objects' pass (XObj2D, shared by Cursor and XObjDemo): they mirror
// unconditionally, so the flag only picks where the jitter happens - inside the
// base's own pass when it is set, here when it is clear
void XObj2D::ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	if (pFuncObj->GetCallbackNodeID() == 0) {
		if (mStatus.mFlags & 0x800) {
			XObj::ExecCallbackA(pResult, mdl, pFuncObj);
		} else {
			CCamera* camera = gGame.GetScreenCamera();
			f32 bottom      = camera->mBottom;
			f32 top         = camera->mTop;
			s32 sign        = top < bottom ? -1 : 1;
			f32 scaleY      = mMoveScale.y;
			f32 scaleX      = mMoveScale.x;
			f32 scaleZ      = mMoveScale.z;
			f32 rotateY     = mMoveRotate.y;
			mMoveScale.x *= -1.0f;
			mMoveScale.y *= sign;
			mMoveScale.z *= -1.0f;
			mMoveRotate.y += PI;
			XObj::ExecCallbackA(pResult, mdl, pFuncObj);
			mMoveScale.y  = scaleY;
			mMoveScale.x  = scaleX;
			mMoveScale.z  = scaleZ;
			mMoveRotate.y = rotateY;
		}
	}
}

#endif

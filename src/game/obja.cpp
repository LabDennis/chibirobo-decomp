#include "game/xobj.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/objectfloorquery.h"
#include "game/cjobjset.h"
#include "game/cgame.h"
#include "game/hsd_dobj.h"
#include "game/globals.h"
#include "game/xchr.h"
#include "game/cvec.h"
extern "C" {
#include <sysdolphin/class_alloc.h>
}

// XObj, the CXObjIntp every placed object in the house is built on

extern "C" {
void C_VECReflect(const Vec* source, const Vec* normal, Vec* result);
}

#if defined(VERSION_PAL)
#define ANIMATION_RATE(rate) ((rate) * (60.0f / (f32)gVideoFramesPerSecond))
#else
#define ANIMATION_RATE(rate) (rate)
#endif

// the pe presets sit 0x5C into this unit's rodata, which isn't split out yet
struct XObjPEPresets {
	u8 _00[0x5C];          // _00
	HSD_PEDesc mReset;     // _5C
	HSD_PEDesc mPreset[3]; // _68
};

extern "C" {
extern const XObjPEPresets gObjectPixelEnginePresets;
}

static inline u32 ShadowSize(XObj* object)
{
	return object->mAttr._00 != NULL ? object->mAttr._00->_0C : 0;
}

static inline void StepColor(f32& current, f32& target, f32& speed, u32 frames)
{
	if (current != target) {
		current += speed * frames;
		if (speed > 0.0f && current >= target) {
			current = target;
			speed   = 0.0f;
		}
		if (speed < 0.0f && current <= target) {
			current = target;
			speed   = 0.0f;
		}
	}
}

extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);

extern "C" void UpdateObjectFloorHeights(XObj* object)
{
	for (u32 i = 0; i < ARRAY_SIZE(object->mFloorHeights); ++i)
		object->mFloorHeights[i] = NAN;
	object->mMaxFloorHeight = NAN;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return;
	object->mStatus.mFlags &= ~0x0010000000000000ULL;
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) {
		Vec center
		    = object->mMoveScale * (object->_794 + ReadFloorCoordinates(object->mFloors[i].mCenterOffset, gObjectFloorOffsetTemplate));
		Vec extent = object->mMoveScale * ReadFloorCoordinates(object->mFloors[i].mExtent, lbl_80378148);
		f32 yaw    = FloorYaw(object, i);
		switch (object->mFloors[i].mId) {
		case 0:
		case 1: {
			CVec point(0.0f, 0.0f, 0.0f);
			Mtx transform;
			MakeFloorTransform(object, center, extent, yaw, transform);
			PSMTXMultVec(transform, &point, &point);
			object->mFloorHeights[i] = point.y + extent.y;
			break;
		}
		case 2: {
			Mtx transform;
			MakeFloorTransform(object, center, extent, yaw, transform);
			f32 h = (2.0f / sqrtf(3.0f)) * 0.5f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec world[8];
			PSMTXMultVecArray(transform, corners, world, 8);
			object->mFloorHeights[i] = world[0].y - 0.01f;
			break;
		}
		case 3:
			break;
		case 5: {
			Vec corners[4];
			for (s32 j = 0; j < 4; ++j)
				corners[j] = object->_794 + ReadFloorCoordinates(object->mFloors[i + j].mCenterOffset, gObjectFloorOffsetTemplate);
			Vec scale = object->mMoveScale * ReadFloorCoordinates(object->mFloors[i].mExtent, lbl_80378148);
			f32 angle = FloorYaw(object, i);
			Mtx translate, rotate, scaling, transform;
			PSMTXTrans(translate, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
			PSMTXScale(scaling, scale.x, scale.y, scale.z);
			PSMTXRotRad(rotate, 'y', angle);
			PSMTXConcat(translate, rotate, transform);
			PSMTXConcat(transform, scaling, transform);
			PSMTXMultVecArray(transform, corners, corners, 4);
			object->mFloorHeights[i] = object->mFloorHeights[i + 1] = object->mFloorHeights[i + 2] = object->mFloorHeights[i + 3]
			    = extent.y * 0.5f + corners[0].y - 0.01f;
			i += 3;
			break;
		}
		}
		if (__fpclassifyf(object->mMaxFloorHeight) == FP_NAN || object->mFloorHeights[i] > object->mMaxFloorHeight)
			object->mMaxFloorHeight = object->mFloorHeights[i];
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
	object->mFloorXZExtent = NAN;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return;
	f32 maxX = 0.0f, maxZ = 0.0f;
	// Type 3 starts two vectors before the four type-5 corners in the original frame.
	struct Geometry {
		Vec samples[6];
		Mtx polygon;
		Mtx polygonScale;
		Mtx polygonRotate;
		Mtx polygonWorld;
		Mtx segment;
		Mtx segmentScale;
		Mtx segmentTurn;
		Mtx segmentRotate;
		Mtx segmentLocal;
		Mtx segmentWorld;
		Mtx prismResult;
		Mtx prismScale;
		Mtx prismRotate;
		Mtx prismLocal;
		Mtx prismWorld;
		Mtx roundResult;
		Mtx roundScale;
		Mtx roundRotate;
		Mtx roundLocal;
		Mtx roundWorld;
		Vec prismPoints[8];
	} geometry;

	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) {
		Vec zero = { 0, 0, 0 };
		CVec origin(zero);
		Vec center     = object->mMoveScale * (object->_794 + ReadObjectFloorOffset(object, i));
		Vec extent     = object->mMoveScale * ReadObjectFloorExtent(object, i);
		f32 baseYaw    = object->mMoveRotate.y;
		f32 floorAngle = (f32)((f64)DEG2RAD * (f32)object->mFloors[i].mYawDegrees);
		f32 yaw        = baseYaw + floorAngle;
		switch (object->mFloors[i].mId) {
		case 0:
		case 1: {
			CVec point(0, 0, 0);
			PSMTXTrans(geometry.roundWorld, origin.x, origin.y, origin.z);
			PSMTXTrans(geometry.roundLocal, center.x, center.y, center.z);
			PSMTXScale(geometry.roundScale, extent.x, extent.y, extent.z);
			PSMTXRotRad(geometry.roundRotate, 'y', yaw);
			PSMTXConcat(geometry.roundWorld, geometry.roundRotate, geometry.roundResult);
			PSMTXConcat(geometry.roundResult, geometry.roundLocal, geometry.roundResult);
			PSMTXConcat(geometry.roundResult, geometry.roundScale, geometry.roundResult);
			PSMTXMultVec(geometry.roundResult, &point, &point);
			ExpandRadius(point.x + extent.x, point.z + extent.z, maxX, maxZ);
			break;
		}
		case 2: {
			PSMTXTrans(geometry.prismWorld, origin.x, origin.y, origin.z);
			PSMTXTrans(geometry.prismLocal, center.x, center.y, center.z);
			PSMTXScale(geometry.prismScale, extent.x, extent.y, extent.z);
			PSMTXRotRad(geometry.prismRotate, 'y', yaw);
			PSMTXConcat(geometry.prismWorld, geometry.prismRotate, geometry.prismResult);
			PSMTXConcat(geometry.prismResult, geometry.prismLocal, geometry.prismResult);
			PSMTXConcat(geometry.prismResult, geometry.prismScale, geometry.prismResult);
			f32 h = (2.0f / sqrtf(3.0f)) * 0.5f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec* world = geometry.prismPoints;
			PSMTXMultVecArray(geometry.prismResult, corners, world, 8);
			for (int j = 0; j < 4; ++j)
				ExpandRadius(world[j].x, world[j].z, maxX, maxZ);
			break;
		}
		case 3: {

			PSMTXTrans(geometry.segmentWorld, origin.x, origin.y, origin.z);
			PSMTXTrans(geometry.segmentLocal, center.x, center.y, center.z);
			PSMTXScale(geometry.segmentScale, extent.x, extent.y, extent.z);
			PSMTXRotRad(geometry.segmentRotate, 'y', baseYaw);
			PSMTXRotRad(geometry.segmentTurn, 'y', floorAngle);
			PSMTXConcat(geometry.segmentWorld, geometry.segmentRotate, geometry.segment);
			PSMTXConcat(geometry.segment, geometry.segmentLocal, geometry.segment);
			PSMTXConcat(geometry.segment, geometry.segmentTurn, geometry.segment);
			PSMTXConcat(geometry.segment, geometry.segmentScale, geometry.segment);
			static Vec ends[2] = { { -.5f, 0, 0 }, { .5f, 0, 0 } };
			PSMTXMultVecArray(geometry.segment, ends, geometry.samples, 2);
			// Retail scans four samples after transforming two; the last two are the type-5 corners.
			for (int j = 0; j < 4; ++j)
				ExpandRadius(geometry.samples[j].x, geometry.samples[j].z, maxX, maxZ);
			break;
		}
		case 5: {
			Vec* corners = geometry.samples + 2;
			for (int j = 0; j < 4; ++j)
				corners[j] = object->_794 + ReadObjectFloorOffset(object, i + j);
			Vec extent = object->mMoveScale * ReadObjectFloorExtent(object, i);
			f32 angle  = FloorYaw(object, i);

			PSMTXTrans(geometry.polygonWorld, origin.x, origin.y, origin.z);
			PSMTXScale(geometry.polygonScale, extent.x, extent.y, extent.z);
			PSMTXRotRad(geometry.polygonRotate, 'y', angle);
			PSMTXConcat(geometry.polygonWorld, geometry.polygonRotate, geometry.polygon);
			PSMTXConcat(geometry.polygon, geometry.polygonScale, geometry.polygon);
			PSMTXMultVecArray(geometry.polygon, corners, corners, 4);
			for (int j = 0; j < 4; ++j)
				ExpandRadius(corners[j].x, corners[j].z, maxX, maxZ);
			break;
		}
		}
	}
	object->mFloorXZExtent = maxX > maxZ ? maxX : maxZ;
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
	const XObjFloor& floorRecord = object->mFloors[index];
	CVec from(*pos);
	from.y = 0.0f;
	CVec middle;
	static Vec origin = { 0.0f, 0.0f, 0.0f };
	f32 angle         = NAN;

	Vec center = object->mMoveScale * (object->_794 + ReadObjectFloorOffset(floorRecord));
	Vec extent = object->mMoveScale * ReadObjectFloorExtent(floorRecord);
	f32 yaw    = (object->mMoveRotate.y + (f32)((f64)DEG2RAD * (f32)floorRecord.mYawDegrees));
	switch (floorRecord.mId) {
	case 0:
	case 1: {
		Mtx transform;
		MakeFloorTransform(object, center, extent, yaw, transform);

		PSMTXMultVec(transform, &origin, &middle);
		middle.y = 0.0f;
		angle    = atan2(middle.x - from.x, middle.z - from.z);
		break;
	}
	case 2: {
		Mtx transform;
		MakeFloorTransform(object, center, extent, yaw, transform);

		PSMTXMultVec(transform, &origin, &middle);
		middle.y = 0.0f;
		f32 h    = (2.0f / sqrtf(3.0f)) * 0.5f;
		static Vec corners[8]
		    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
		Vec world[8];
		PSMTXMultVecArray(transform, corners, world, 8);
		for (s32 i = 0; i < 4; ++i) {
			Vec& a = world[2 * i];
			Vec& b = world[2 * i + 1];
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
	case 5: {
		Mtx translate, rotate, scaling, transform;
		Vec corners[4];
		for (s32 j = 0; j < 4; ++j)
			corners[j] = object->_794 + ReadObjectFloorOffset(object->mFloors[index - 3 + j]);
		Vec scale = object->mMoveScale * ReadObjectFloorExtent(object->mFloors[index]);

		f32 angleY = FloorYaw(object, index);
		PSMTXTrans(translate, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
		PSMTXScale(scaling, scale.x, scale.y, scale.z);
		PSMTXRotRad(rotate, 'y', angleY);
		PSMTXConcat(translate, rotate, transform);
		PSMTXConcat(transform, scaling, transform);
		PSMTXMultVecArray(transform, corners, corners, 4);
		// A degenerate polygon has no center from which to select a facing edge.
		if (!IntersectLinesXZ(&corners[0], &corners[2], &corners[1], &corners[3], &middle, 1))
			break;
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

/**
 * @note Address: 0x8008DEC0
 * @note Size: 0x74
 */
extern "C" void ClearActiveObjectFloors(XObj* object)
{
	if (object->mStatus.mFlags & 0x0200000000000000ULL) {
		object->_250[1] = object->_250[0];
		for (int i = 0; object->mFloors[i].mId != -1; i++) {
			if (object->IsFloorOn(i)) {
				object->ClearFloor(i);
			}
		}
	}
}

extern "C" CVec ReflectVectorAboutNormal(const Vec* source, const Vec* normal);
extern "C" BOOL IsPlayerStopped(const XObjChr* player);

static inline void StoreFloorEdge(XObjEdge& edge, const Vec& origin, const Vec& a, const Vec& b)
{
	edge.mContactOffset.x -= origin.x;
	edge.mContactOffset.y -= origin.y;
	edge.mContactOffset.z -= origin.z;
	edge.mFrom.x = a.x - origin.x;
	edge.mFrom.y = a.y - origin.y;
	edge.mFrom.z = a.z - origin.z;
	edge.mTo.x   = b.x - origin.x;
	edge.mTo.y   = b.y - origin.y;
	edge.mTo.z   = b.z - origin.z;
}

static inline void StoreSweepContact(const Vec* from, const Vec* to, Vec* contact, Vec& dest, const Vec& hit, f32 top)
{
	if (to->y == top) {
		contact->x = to->x;
		// The original copies the destination height into Z on an exact top hit.
		contact->z = to->y;
		contact->y = from->y;
	} else {
		contact->x = hit.x;
		contact->z = hit.z;
		contact->y = from->y;
		dest.x     = hit.x;
		dest.z     = hit.z;
	}
}

/**
 * @note Address: 0x8008ED50
 * @note Size: 0x1498
 */
u32 XObj::vfFC(const Vec* from, const Vec* to, f32 radius, f32 height, Vec* push, Vec* contact, s32 floorCursor, s32 flags)
{
	s32* cursor = (s32*)floorCursor;
	if (!(mStatus.mFlags & 0x0200000000000000ULL)) {
		if (cursor)
			*cursor = -1;
		return 0;
	}
	if (mStatus.mFlags & 0x0C00000000000000ULL) {
		if (cursor)
			*cursor = -1;
		return 0;
	}
	if (mStatus.mFlags & 0x4000000000000000ULL) {
		if (cursor)
			*cursor = -1;
		return 0;
	}
	u32 result = 0;
	CVec start(*from);
	start.y = 0;
	CVec dest(*to);
	dest.y = 0;
	CVec hit;
	s32 i = cursor ? *cursor : 0;
	for (; mFloors[i].mId != -1; ++i) {
		u32 collision          = 0;
		const XObjFloor& floor = mFloors[i];
		Vec center             = mMoveScale * (_794 + ReadObjectFloorOffset(floor));
		Vec extent             = mMoveScale * ReadObjectFloorExtent(floor);
		f32 objYaw             = mMoveRotate.y;
		f32 angle              = DEG2RAD * floor.mYawDegrees;
		f32 yaw                = objYaw + angle;
		switch (floor.mId) {
		case 0:
		case 1: {
			CVec middle(0, 0, 0);
			Mtx transform;
			MakeFloorTransform(this, center, extent, yaw, transform);
			PSMTXMultVec(transform, &middle, &middle);
			const f32 y = middle.y;
			middle.y    = 0;
			f32 reach   = radius + extent.x;
			if (PSVECDistance(&middle, &dest) < reach) {
				CVec travel              = (dest - start).Normalize();
				CVec normal              = (dest - middle).Normalize();
				CVec reflect             = ReflectVectorAboutNormal(&travel, &normal);
				*push                    = reflect * (dest - start).Length() * 0.0f;
				hit                      = middle + normal * reach;
				mEdges[i].mContactOffset = (middle + normal * extent.x) - mMoveTranslate;
				if (to->y <= mFloorHeights[i] && to->y >= ((y - extent.y) - height)) {
					StoreSweepContact(from, to, contact, dest, hit, mFloorHeights[i]);
					collision = 3;
				} else
					collision = to->y < ((y - extent.y) - height) ? 0 : 1;
			}
			break;
		}
		case 2: {
			Mtx transform;
			MakeFloorTransform(this, center, extent, yaw, transform);
			f32 h = (2.f / sqrtf(3.f)) * .5f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec world[8];
			PSMTXMultVecArray(transform, corners, world, 8);
			for (s32 side = 0; side < 4; ++side) {
				collision = ResolveCircleWallCollisionXZ(&start, &dest, &world[side * 2], &world[side * 2 + 1], push, &hit,
				                                         &mEdges[i].mContactOffset, NULL, radius, 0, 0);
				if (collision) {
					collision = 8;
					StoreFloorEdge(mEdges[i], mMoveTranslate, world[side * 2], world[side * 2 + 1]);
					break;
				}
			}
			if (!collision && IsPointInsideQuadXZ(&dest, &world[0], &world[2], &world[4], &world[6])) {
				collision = 4;
				hit.x     = to->x;
				hit.z     = to->z;
			}
			if (collision) {
				if (to->y <= mFloorHeights[i] && to->y >= (world[0].y - 2.f * (extent.y * h) - height)) {
					StoreSweepContact(from, to, contact, dest, hit, mFloorHeights[i]);
					collision = 3;
				} else
					collision = to->y < (world[0].y - 2.f * (extent.y * h) - height) ? 0 : 1;
			}
			break;
		}
		case 3: {
			Mtx base, local, rot, extra, scale, transform;
			PSMTXTrans(base, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rot, 'y', objYaw);
			PSMTXRotRad(extra, 'y', angle);
			PSMTXConcat(base, rot, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, extra, transform);
			PSMTXConcat(transform, scale, transform);
			Vec wall[2];
			static Vec ends[2] = { { -.5f, 0, 0 }, { .5f, 0, 0 } };
			PSMTXMultVecArray(transform, ends, wall, 2);
			if (ResolveCircleWallCollisionXZ(&start, &dest, &wall[0], &wall[1], push, contact, &mEdges[i].mContactOffset, NULL, radius, 0,
			                                 1)) {
				collision  = 3;
				contact->y = from->y;
				StoreFloorEdge(mEdges[i], mMoveTranslate, wall[0], wall[1]);
			}
			break;
		}
		case 5: {
			Vec quad[4];
			for (s32 j = 0; j < 4; ++j)
				quad[j] = _794 + ReadObjectFloorOffset(mFloors[i + j]);
			Vec size       = mMoveScale * ReadObjectFloorExtent(floor);
			f32 floorAngle = DEG2RAD * floor.mYawDegrees;
			f32 rotY       = mMoveRotate.y + floorAngle;
			Mtx translate, rotate, scale, transform;
			PSMTXTrans(translate, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXScale(scale, size.x, size.y, size.z);
			PSMTXRotRad(rotate, 'y', rotY);
			PSMTXConcat(translate, rotate, transform);
			PSMTXConcat(transform, scale, transform);
			PSMTXMultVecArray(transform, quad, quad, 4);
			for (s32 j = 0; j < 4; ++j) {
				collision = ResolveCircleWallCollisionXZ(&start, &dest, &quad[j], &quad[(j + 1) % 4], push, &hit,
				                                         &mEdges[i + 3].mContactOffset, NULL, radius, 0, 1);
				if (collision) {
					collision = 8;
					StoreFloorEdge(mEdges[i + 3], mMoveTranslate, quad[j], quad[(j + 1) % 4]);
					break;
				}
			}
			if (!collision && IsPointInsideQuadXZ(&dest, &quad[0], &quad[1], &quad[2], &quad[3])) {
				collision = 4;
				hit.x     = to->x;
				hit.z     = to->z;
			}
			if (collision) {
				if (to->y <= mFloorHeights[i] && to->y >= (quad[0].y - extent.y * .5f - height)) {
					StoreSweepContact(from, to, contact, dest, hit, mFloorHeights[i]);
					collision = 3;
				} else
					collision = to->y < (quad[0].y - extent.y * .5f - height) ? 0 : 1;
			}
			i += 3;
			break;
		}
		}
		if (collision) {
			_250[0] |= 1u << i;
			result |= collision;
			mHitFloorIndex = i;
			if (cursor) {
				++i;
				break;
			}
		}
	}
	if (cursor) {
		*cursor = i;
		if (mFloors[i].mId == -1)
			*cursor = -1;
	}
	if (result) {
		mStatus.mFlags |= 0x0100000000000000ULL;
		if ((flags & 2) && (result & 3) == 3) {
			XObjChr* player = gGame.GetPlayer();
			if ((mStatus.mFlags & 0x20000000000ULL) && !(mStatus.mFlags & 0x8000000000000000ULL) && !(gGame.mStatus.mFlags & 0x400)
			    && player->mAttachmentRequest < 0
			    && (!(gGame.mStatus.mFlags & 0x40000000) || !IsPlayerStopped(player) || !player->_158A4 || (mStatus.mFlags & 0x8000))) {
				mStatus.mFlags |= 0x8000000000000000ULL;
				StartOrQueueEventScript(&gGame.mEvt, (u8*)vf128(), (u32)this, -1, -1);
			}
		}
		if (flags & 1) {
			const CVec& delta = CVec(*contact) - *from;
			PSVECMag(push);
			result = vf100((s32)&delta, result);
		}
	} else
		mStatus.mFlags &= ~0x0100000000000000ULL;
	return result;
}

/**
 * @note Address: 0x800901E8
 * @note Size: 0x6C
 */
extern "C" CVec ReflectVectorAboutNormal(const Vec* source, const Vec* normal)
{
	CVec result;
	C_VECReflect(source, normal, &result);
	return result;
}

/**
 * @note Address: 0x80090254
 * @note Size: 0x50
 */
extern "C" void ClearAnim__4XObjFv(XObj* object)
{
	object->CXObj::ClearAnim();
	object->mStatus.mFlags &= ~0x0008000000000000ULL;
}

/**
 * @note Address: 0x800902A4
 * @note Size: 0x1D8
 */
extern "C" void SetAnim__4XObjFliPvPvPvffff(XObj* object, s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame,
                                            f32 rate, f32 blend, f32 blendRate)
{
	if (object->mStatus.mFlags & 0x0020000000000000ULL)
		blend = 1.0f;
	if (object->GetAnimSet(id) != 0) {
		s32 index = id % 1000;
		jointAnim = jointAnim != NULL ? jointAnim : object->mAnim.GetAnimJoint(index);
		matAnim   = matAnim != NULL ? matAnim : object->mAnim.GetMatAnimJoint(index);
		shapeAnim = shapeAnim != NULL ? shapeAnim : object->mAnim.GetShapeAnimJoint(index);
		object->CXObjIntp::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, ANIMATION_RATE(rate), blend, blendRate);
		object->mStatus.mFlags |= 0x0008000000000000ULL;
	} else {
		object->CXObjIntp::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, ANIMATION_RATE(rate), blend, blendRate);
		object->mStatus.mFlags &= ~0x0008000000000000ULL;
	}
}

/**
 * @note Address: 0x8009047C
 * @note Size: 0xEC
 */
extern "C" void vfC8__4XObjFv(XObj* object)
{
	if ((object->mStatus.mFlags & 0x800ULL) && (object->mStatus.mFlags & 0x10000ULL)) {
		HSD_JObjSetFlags(object->mJObj, CLASSICAL_SCALE);
		Vec scale = object->mMoveScale;
		object->SetScale(scale.x, -scale.y, scale.z, 0);
		if (object->mJObj != NULL)
			HSD_JObjDispAll(object->mJObj, NULL, 7, 0);
		object->SetScale(scale, 0);
	}
}

/**
 * @note Address: 0x80090568
 * @note Size: 0x4
 */
extern "C" void vfC4__4XObjFv(XObj*)
{
}

/**
 * @note Address: 0x8009056C
 * @note Size: 0x2F8
 */
extern "C" void DrawGroundShadowAt__4XObjFPC3Vecf(XObj* object, const Vec* origin, f32 opacity)
{
	if (!(object->mStatus.mFlags & 8ULL))
		return;
	if (ShadowSize(object) == 0)
		return;
	Vec position = *origin;
	Vec rotation = { -HALF_PI, 0.0f, 0.0f };
	Vec size     = object->mMoveScale * (f32)ShadowSize(object);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	u32 edge   = (u32)(128.0f * opacity);
	u32 center = (u32)(160.0f * opacity);
	DrawGsCircle(&gGame.mGs, 12, center, edge, position.x, position.y, position.z, size.x, size.z, rotation.x, rotation.y, rotation.z);
	PrimVertex vertices[26];
	f32 halfWidth  = size.x * 0.5f;
	f32 halfHeight = size.z * 0.5f;
	for (s32 i = 0; i < 13; i++) {
		f32 angle                 = (PI / 6.0f) * i;
		vertices[i * 2].pos.x     = position.x - halfWidth * (f32)sin(angle);
		vertices[i * 2].pos.z     = position.z + halfHeight * (f32)cos(angle);
		vertices[i * 2].pos.y     = position.y;
		vertices[i * 2].color     = edge;
		vertices[i * 2 + 1].pos.x = position.x - (1.5f * object->mMoveScale.x + halfWidth) * (f32)sin(angle);
		vertices[i * 2 + 1].pos.z = position.z + (1.5f * object->mMoveScale.z + halfHeight) * (f32)cos(angle);
		vertices[i * 2 + 1].pos.y = position.y;
		vertices[i * 2 + 1].color = 0;
	}
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, 26);
}

/**
 * @note Address: 0x80090864
 * @note Size: 0x5C
 */
extern "C" void DrawGroundShadow__4XObjFv(XObj* object)
{
	if (object->mStatus.mFlags & 8ULL) {
		object->DrawGroundShadowAt(&object->mMoveTranslate, 1.0f);
	}
}

/**
 * @note Address: 0x800908C0
 * @note Size: 0x3C4
 */
extern "C" void DrawWithView__4XObjFPA4_f(XObj* object, Mtx view)
{
	HSD_AObj* animation = GetAObj(object->mJObj);
	if (animation != NULL && (animation->flags & (AOBJ_NO_ANIM | AOBJ_REWINDED)) && object->HasQueuedAnim()) {
		s32 id;
		u16 frame;
		s16 rate;
		object->PopAnim(id, frame, rate);
		if (id & 0xFFFF0000) {
			s32 jointId             = id & 0xFFFF;
			s32 materialId          = id >> 16;
			void* jointAnimation    = object->GetJointAnimation(jointId);
			void* materialAnimation = object->GetMaterialAnimation(materialId);
			void* shapeAnimation    = object->GetShapeAnimation(jointId);
			object->SetAnim(0, TRUE, jointAnimation, materialAnimation, shapeAnimation, (f32)frame, rate / 100.0f, 0.0f, 10.0f);
			if (jointId >= 1000 || materialId >= 1000)
				object->mStatus.mFlags |= 0x0008000000000000ULL;
		} else {
			object->SetAnim(id, FALSE, NULL, NULL, NULL, (f32)frame, rate / 100.0f, 0.0f, 10.0f);
		}
		object->UpdateAnim(1);
	}
	if (!(object->mStatus.mFlags & 0x800ULL)) {
		if (object->mJObj != NULL)
			HSD_JObjDispAll(object->mJObj, view, 7, 0);
	}
}

/**
 * @note Address: 0x80090C84
 * @note Size: 0x250
 */
extern "C" void vfA8__4XObjFUl(XObj* object, u32 frames)
{
	if (object->mJObj != NULL) {
		for (s32 i = 0; i < 2; ++i) {
			XObjColorStep& color = object->mColorStep[i];
			StepColor(color.mValue.x, color.mTarget.x, color.mSpeed.x, frames);
			StepColor(color.mValue.y, color.mTarget.y, color.mSpeed.y, frames);
			StepColor(color.mValue.z, color.mTarget.z, color.mSpeed.z, frames);
		}
		object->mColors[1] = ((u32)object->mColorStep[0].mValue.x << 24) | ((u32)object->mColorStep[0].mValue.y << 16)
		                   | ((u32)object->mColorStep[0].mValue.z << 8);
		object->mColors[0] = ((u32)object->mColorStep[1].mValue.x << 24) | ((u32)object->mColorStep[1].mValue.y << 16)
		                   | ((u32)object->mColorStep[1].mValue.z << 8);
	}
}

/**
 * @note Address: 0x80090ED4
 * @note Size: 0x2EC
 */
extern "C" void UpdateObject__4XObjFUl(XObj* object, u32 frames)
{
	if (object->mJObj != NULL) {
		object->vfA8(frames);
		if (object->mStatus.mFlags & 0x1000000000000000ULL) {
			if (object->mMoveTranslate.y + object->_794.y > object->mFloorHit.mHeight) {
				object->mMoveTranslate.mTarget.y = object->mFloorHit.mHeight;
				f32 speed                        = 0.15f + object->mMoveTranslate.mSpeed.y;
				if (speed > 10.0f)
					speed = 10.0f;
				object->mMoveTranslate.mSpeed.y = speed;
			} else {
				object->mMoveTranslate.mSpeed.y = 0.0f;
				object->mMoveTranslate.x += object->_794.x;
				object->mMoveTranslate.z += object->_794.z;
				object->mMoveTranslate.y = object->mFloorHit.mHeight;
				object->_794.x           = 0.0f;
				object->_794.y           = 0.0f;
				object->_794.z           = 0.0f;
				object->SetRotationX(0.0f, 0);
				object->SetRotationZ(0.0f, 0);
				object->mStatus.mFlags &= ~0x1000000000000000ULL;
				if ((s16)object->GetPool() == 21) {
					object->mStatus.mFlags |= 0x2000000000000000ULL;
				} else if ((s16)object->GetPool() == 22) {
					if (object->mStatus.mFlags & 0x04000000ULL)
						object->mStatus.mFlags |= 0x2000000000000000ULL;
				}
			}
		}
		object->CXObjIntp::StepRotate(frames);
		object->CXObjIntp::StepTranslate(frames);
		if (object->CXObjIntp::StepScale(frames)) {
			UpdateObjectFloorHeights((XObj*)object);
			UpdateObjectFloorXZExtent((XObj*)object);
		}
		object->UpdateAnim(frames);
		HSD_JObjSetupMatrix(object->mJObj);
	}
	if ((object->mStatus.mFlags & 0x0010000000000000ULL) || object->mMoveTranslate.y != object->_264.y) {
		UpdateObjectFloorHeights((XObj*)object);
	}
	if (object->mEvt.mData.mScript != NULL)
		CEvt_Update(&object->mEvt, frames);
}

/**
 * @note Address: 0x800911C0
 * @note Size: 0xF4
 */
extern "C" void Release__4XObjFv(XObj* object)
{
	CJArchive* archive = &object->mAnim;
	archive->Release();
	if (object->mEvt.mData.mScript != NULL)
		ResetEventScriptState(&object->mEvt, TRUE, FALSE);
	if (object->mStatus.mFlags & 1) {
		object->CXObjIntp::Release();
		XObjSource** slot = (XObjSource**)object->mAttr._00;
		if (slot != NULL && *slot != NULL) {
			XObjSource* source = *slot;
			if (source->mArchive != NULL && source->mArchive->mRefCount <= 0)
				source->mArchive = NULL;
		}
	}
	object->mAttr._00 = NULL;
	object->mAttr._04 = NULL;
	object->mFloors   = NULL;
	object->mStatus.mFlags &= ~0x10001ULL;
}

/**
 * @note Address: 0x800912B4
 * @note Size: 0xB8
 */
extern "C" void fn_800912B4(HSD_JObj* joint, f32** arguments, s32 type)
{
	HSD_JObjModifyFlags(joint, 0x40000, 0x1C0000);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_DObjModifyFlags(draw, 2, 0xE);
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjClearFlags(material, 0x68000000);
			HSD_MObjSetFlags(material, 0x2000);
			HSD_MObjCompileTev(material);
		}
	}
}

/**
 * @note Address: 0x8009136C
 * @note Size: 0x100
 */
extern "C" void fn_8009136C(HSD_JObj* joint, f32** arguments, s32 type)
{
	CXObj* object = (CXObj*)arguments[0];
	f32 alpha     = *arguments[1];
	HSD_JObjModifyFlags(joint, 0x80000, 0x1C0000);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_DObjModifyFlags(draw, 4, 0xE);
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjSetFlags(material, 0x60000000);
			HSD_MObjSetFlags(material, 0x2000);
			HSD_MObjCompileTev(material);
			if (material->mat != NULL)
				HSD_MObjSetAlpha(material, alpha * object->GetMObjAlpha(material));
		}
	}
}

/**
 * @note Address: 0x8009146C
 * @note Size: 0x60
 */
extern "C" void vfE8__4XObjFv(CXObj* object)
{
	void* arguments[1] = { NULL };
	arguments[0]       = object;
	if (object->mJObj != NULL)
		HSD_JObjWalkTree(object->mJObj, fn_800912B4, (f32**)arguments);
	HSD_JObjModifyFlags(object->mJObj, 0x10000000, 0x70000000);
}

/**
 * @note Address: 0x800914CC
 * @note Size: 0x74
 */
extern "C" void vfE4__4XObjFf(CXObj* object, f32 alpha)
{
	void* arguments[2] = { NULL, NULL };
	arguments[0]       = object;
	arguments[1]       = &alpha;
	if (object->mJObj != NULL)
		HSD_JObjWalkTree(object->mJObj, fn_8009136C, (f32**)arguments);
	HSD_JObjModifyFlags(object->mJObj, 0x20000000, 0x70000000);
}

/**
 * @note Address: 0x80091540
 * @note Size: 0x1EC
 */
extern "C" void ScaleJointMaterialDiffuse(HSD_JObj* joint, f32** arguments, s32 type)
{
	f32 factor = *arguments[1];
	HSD_JObjSetFlags(joint, 0x80);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjSetFlags(material, 4);
			if (material->mat != NULL && __fpclassifyf(factor) != FP_NAN) {
				material->mat->diffuse.r = (u8)(material->mat->diffuse.r * factor);
				material->mat->diffuse.g = (u8)(material->mat->diffuse.g * factor);
				material->mat->diffuse.b = (u8)(material->mat->diffuse.b * factor);
			}
			for (HSD_TObj* texture = HSD_MObjGetTObj(material); texture != NULL; texture = HSD_TObjGetNext(texture)) {
				u32 value      = texture->flags & ~0x1F0;
				texture->flags = value;
				texture->flags = value | 0x50;
				value          = texture->flags & ~0xF0000;
				texture->flags = value;
				texture->flags = value | 0x70000;
			}
			HSD_MObjCompileTev(material);
		}
	}
}

/**
 * @note Address: 0x8009172C
 * @note Size: 0x58
 */
extern "C" void vfE0__4XObjFf(CXObj* object, f32 factor)
{
	void* arguments[2] = { NULL, NULL };
	arguments[0]       = object;
	arguments[1]       = &factor;
	if (object->mJObj != NULL)
		HSD_JObjWalkTree(object->mJObj, ScaleJointMaterialDiffuse, (f32**)arguments);
}

/**
 * @note Address: 0x80091784
 * @note Size: 0xB0
 */
extern "C" void fn_80091784(HSD_JObj* joint, f32** arguments, s32 type)
{
	BOOL enabled = (BOOL)arguments[0];
	u8 alpha     = (u32)arguments[1];
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL && material->pe != NULL) {
			if (enabled)
				material->pe->flags |= 6;
			else
				material->pe->flags &= ~6;
			material->pe->dst_alpha = alpha;
		}
	}
}

/**
 * @note Address: 0x80091834
 * @note Size: 0x54
 */
extern "C" void vfDC__4XObjFiUc(CXObj* object, BOOL enabled, u8 value)
{
	void* arguments[2] = { NULL, NULL };
	arguments[0]       = (void*)enabled;
	arguments[1]       = (void*)value;
	if (object->mJObj != NULL)
		HSD_JObjWalkTree(object->mJObj, fn_80091784, (f32**)arguments);
}

// joint walk callback for XObj::vfD8, the args are five on/off flags
/**
 * @note Address: 0x80091888
 * @note Size: 0x3BC
 */
extern "C" void fn_80091888(HSD_JObj* joint, f32** arguments, s32 type)
{
	const XObjPEPresets* presets = &gObjectPixelEnginePresets;
	HSD_DObj* draw;
	u32 reset        = (u32)arguments[0];
	u32 firstPreset  = (u32)arguments[1];
	u32 secondPreset = (u32)arguments[2];
	u32 thirdPreset  = (u32)arguments[3];
	u32 toon         = (u32)arguments[4];
	for (draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			if (reset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				HSD_PEDesc settings = presets->mReset;
				*material->pe       = settings;
				if (material->rendermode & RENDER_XLU)
					material->pe->type = 1;
				if (material->rendermode & RENDER_NO_ZUPDATE)
					material->pe->flags &= ~0x20;
				if (material->rendermode & RENDER_ZMODE_ALWAYS)
					material->pe->flags &= ~0x10;
			}
			if (firstPreset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				HSD_PEDesc settings = presets->mPreset[0];
				*material->pe       = settings;
			} else if (secondPreset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				HSD_PEDesc settings = presets->mPreset[1];
				*material->pe       = settings;
			} else if (thirdPreset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				HSD_PEDesc settings = presets->mPreset[2];
				*material->pe       = settings;
				if (material->rendermode & RENDER_XLU)
					material->pe->type = 1;
			}
			if (toon) {
				if (!(GetMaterialRenderModeFlags(material) & RENDER_TOON)) {
					HSD_MObjSetFlags(material, RENDER_TOON);
					HSD_MObjCompileTev(material);
				}
			} else {
				if (GetMaterialRenderModeFlags(material) & RENDER_TOON) {
					HSD_MObjClearFlags(material, RENDER_TOON);
					HSD_MObjCompileTev(material);
				}
			}
		}
	}
}

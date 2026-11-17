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
	return object->mAttr.mObjData != NULL ? object->mAttr.mObjData->mShadowSize : 0;
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
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) { // the list ends at id -1
		Vec center  = object->mMoveScale * (object->mToppleOffset + ReadObjectFloorOffset(object, i));
		Vec extent  = object->mMoveScale * ReadObjectFloorExtent(object, i);
		f32 baseYaw = object->mMoveRotate.y;
		f32 turn    = DEG2RAD * object->mFloors[i].mYawDegrees;
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
			f32 h = (2.0f / sqrtf(3.0f)) / 2.0f; // cube corners at unit radius
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
				corners[j] = object->mToppleOffset + ReadObjectFloorOffset(object, i + j);
			Vec scale     = object->mMoveScale * ReadObjectFloorExtent(object, i);
			f32 baseAngle = object->mMoveRotate.y;
			f32 turnAngle = DEG2RAD * object->mFloors[i].mYawDegrees;
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
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) {
		CVec origin((Vec) { 0, 0, 0 });
		Vec center     = object->mMoveScale * (object->mToppleOffset + ReadObjectFloorOffset(object, i));
		Vec extent     = object->mMoveScale * ReadObjectFloorExtent(object, i);
		f32 baseYaw    = object->mMoveRotate.y;
		f32 floorAngle = DEG2RAD * object->mFloors[i].mYawDegrees;
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
			f32 h = (2.0f / sqrtf(3.0f)) / 2.0f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec points[8];
			PSMTXMultVecArray(result, corners, points, 8);
			for (int j = 0; j < 4; ++j)
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
			for (int j = 0; j < 4; ++j)
				ExpandRadius(line[j].x, line[j].z, maxX, maxZ);
			break;
		}
		case 5: { // a quad from this and the next three records
			Mtx world, rotate, scale, result;
			Vec corners[4];
			for (int j = 0; j < 4; ++j)
				corners[j] = object->mToppleOffset + ReadObjectFloorOffset(object, i + j);
			Vec extent    = object->mMoveScale * ReadObjectFloorExtent(object, i);
			f32 baseAngle = object->mMoveRotate.y;
			f32 turnAngle = DEG2RAD * object->mFloors[i].mYawDegrees;
			f32 angle     = baseAngle + turnAngle;

			PSMTXTrans(world, origin.x, origin.y, origin.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', angle);
			PSMTXConcat(world, rotate, result);
			PSMTXConcat(result, scale, result);
			PSMTXMultVecArray(result, corners, corners, 4);
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

// NONMATCHING: retail reads the quad corners through floors (lwzx off the held pointer) but the quad's yaw through
// object->mFloors, keeps index * 32 apart from the pointer, and allocates registers differently
extern "C" f32 GetFloorFacingAngle(XObj* object, const Vec* pos, s32 index, s32 requireSegment, Vec* contact, Vec* normalOut)
{
	CVec from(*pos);
	from.y = 0.0f;
	CVec middle;
	f32 angle = NAN;

	Vec center        = object->mMoveScale * (object->mToppleOffset + ReadObjectFloorOffset(object, index));
	Vec extent        = object->mMoveScale * ReadObjectFloorExtent(object, index);
	XObjFloor* floors = object->mFloors;
	f32 baseYaw       = object->mMoveRotate.y;
	f32 floorYaw      = DEG2RAD * floors[index].mYawDegrees;
	f32 yaw           = baseYaw + floorYaw;
	switch (floors[index].mId) {
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
		f32 h    = (2.0f / sqrtf(3.0f)) / 2.0f;
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
			corners[j] = object->mToppleOffset + ReadObjectFloorOffset(object, index - 3 + j);
		Vec scale = object->mMoveScale * ReadObjectFloorExtent(object, index);

		f32 baseAngle = object->mMoveRotate.y;
		f32 turnAngle = DEG2RAD * floors[index].mYawDegrees;
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

/**
 * @note Address: 0x8008DEC0
 * @note Size: 0x74
 */
extern "C" void ClearActiveObjectFloors(XObj* object)
{
	if (object->mStatus.mFlags & 0x0200000000000000ULL) {
		object->mFloorMasks[1] = object->mFloorMasks[0];
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
u32 XObj::TestFloorCollision(const Vec* from, const Vec* to, f32 radius, f32 height, Vec* push, Vec* contact, s32* cursor, s32 flags)
{
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
	CVec start(*(const CVec*)from); // the callers pass CVecs
	start.y = 0;
	CVec dest(*(const CVec*)to);
	dest.y = 0;
	CVec hit;
	s32 i = cursor ? *cursor : 0; // a cursor lets a caller resume after a hit
	for (; mFloors[i].mId != -1; ++i) {
		u32 collision = 0;
		Vec center    = mMoveScale * (mToppleOffset + ReadObjectFloorOffset(this, i));
		Vec extent    = mMoveScale * ReadObjectFloorExtent(this, i);
		f32 objYaw    = mMoveRotate.y;
		f32 angle     = DEG2RAD * mFloors[i].mYawDegrees;
		f32 yaw       = objYaw + angle;
		switch (mFloors[i].mId) {
		case 0:
		case 1: { // a disc
			CVec middle(0, 0, 0);
			Mtx world, local, rotate, scale, transform;
			PSMTXTrans(world, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			PSMTXMultVec(transform, &middle, &middle);
			const f32 y = middle.y;
			middle.y    = 0;
			f32 reach   = radius + extent.x;
			if (PSVECDistance(&middle, &dest) < reach) { // inside the disc's radius
				CVec travel              = (dest - start).Normalize();
				CVec normal              = (dest - middle).Normalize();
				CVec reflect             = ReflectVectorAboutNormal(&travel, &normal);
				*push                    = reflect * (dest - start).Length() * 0.0f;
				hit                      = middle + normal * reach;
				mEdges[i].mContactOffset = (middle + normal * extent.x) - mMoveTranslate;
				if (to->y <= mFloorHeights[i] && to->y >= ((y - extent.y) - height)) {
					StoreSweepContact(from, to, contact, dest, hit, mFloorHeights[i]);
					collision = 3; // touching it within its height span
				} else
					collision = to->y < ((y - extent.y) - height) ? 0 : 1; // above it
			}
			break;
		}
		case 2: { // a box
			Mtx base, local, rotate, scale, transform;
			PSMTXTrans(base, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(base, rotate, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			f32 h = (2.f / sqrtf(3.f)) / 2.f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec world[8];
			PSMTXMultVecArray(transform, corners, world, 8);
			for (s32 side = 0; side < 4; ++side) {
				collision = ResolveCircleWallCollisionXZ(&start, &dest, &world[side * 2], &world[side * 2 + 1], push, &hit,
				                                         &mEdges[i].mContactOffset, NULL, radius, 0, 0);
				if (collision) {
					collision = 8; // a wall hit
					StoreFloorEdge(mEdges[i], mMoveTranslate, world[side * 2], world[side * 2 + 1]);
					break;
				}
			}
			if (!collision && IsPointInsideQuadXZ(&dest, &world[0], &world[2], &world[4], &world[6])) {
				collision = 4; // inside it
				hit.x     = to->x;
				hit.z     = to->z;
			}
			if (collision) {
				if (to->y <= mFloorHeights[i] && to->y >= (world[0].y - 2.f * (extent.y * h) - height)) {
					StoreSweepContact(from, to, contact, dest, hit, mFloorHeights[i]);
					collision = 3;
				} else
					collision = to->y < (world[0].y - 2.f * (extent.y * h) - height) ? 0 : 1; // above it
			}
			break;
		}
		case 3: { // a wall segment
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
			u32 touched = ResolveCircleWallCollisionXZ(&start, &dest, &wall[0], &wall[1], push, contact, &mEdges[i].mContactOffset, NULL,
			                                           radius, 0, 1);
			if (touched) {
				collision  = 3; // the wall stops the sweep
				contact->y = from->y;
				StoreFloorEdge(mEdges[i], mMoveTranslate, wall[0], wall[1]);
			}
			break;
		}
		case 5: { // a quad from this and the next three records
			Mtx translate, rotate, scale, transform;
			Vec quad[4];
			for (s32 j = 0; j < 4; ++j)
				quad[j] = mToppleOffset + ReadObjectFloorOffset(this, i + j);
			Vec size       = mMoveScale * ReadObjectFloorExtent(this, i);
			f32 floorAngle = DEG2RAD * mFloors[i].mYawDegrees;
			f32 rotY       = mMoveRotate.y + floorAngle;
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
				if (to->y <= mFloorHeights[i] && to->y >= (quad[0].y - extent.y / 2.f - height)) {
					StoreSweepContact(from, to, contact, dest, hit, mFloorHeights[i]);
					collision = 3;
				} else
					collision = to->y < (quad[0].y - extent.y / 2.f - height) ? 0 : 1; // above it
			}
			i += 3;
			break;
		}
		}
		if (collision) {
			mFloorMasks[0] |= 1u << i; // mark the floor touched
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
		if ((flags & 2) && (result & 3) == 3) { // any contact (3) can start its event script
			XObjChr* player = gGame.GetPlayer();
			if ((mStatus.mFlags & 0x20000000000ULL) && !(mStatus.mFlags & 0x8000000000000000ULL) && !(gGame.mStatus.mFlags & 0x400)
			    && player->mAttachmentRequest < 0
			    && (!(gGame.mStatus.mFlags & 0x40000000) || !IsPlayerStopped(player) || !player->mBatteryDrains
			        || (mStatus.mFlags & 0x8000))) {
				mStatus.mFlags |= 0x8000000000000000ULL;
				u8* script = (u8*)GetEventScript();
				StartOrQueueEventScript(&gGame.mEvt, script, this, -1, -1);
			}
		}
		if (flags & 1) { // let the object's own handler react and adjust the result
			const CVec& delta = *(CVec*)contact - *from;
			PSVECMag(push);
			result = OnPlayerContact((s32)&delta, result);
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
void XObj::ClearAnim()
{
	CXObj::ClearAnim();
	mStatus.mFlags &= ~0x0008000000000000ULL;
}

/**
 * @note Address: 0x800902A4
 * @note Size: 0x1D8
 */
void XObj::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (mStatus.mFlags & 0x0020000000000000ULL)
		blend = 1.0f;
	if (GetAnimSet(id) != 0) {
		s32 index = id % 1000;
		jointAnim = jointAnim != NULL ? jointAnim : mAnim.GetAnimJoint(index);
		matAnim   = matAnim != NULL ? matAnim : mAnim.GetMatAnimJoint(index);
		shapeAnim = shapeAnim != NULL ? shapeAnim : mAnim.GetShapeAnimJoint(index);
		CXObjIntp::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, ANIMATION_RATE(rate), blend, blendRate);
		mStatus.mFlags |= 0x0008000000000000ULL; // the anim has a matching set
	} else {
		CXObjIntp::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, ANIMATION_RATE(rate), blend, blendRate);
		mStatus.mFlags &= ~0x0008000000000000ULL;
	}
}

/**
 * @note Address: 0x8009047C
 * @note Size: 0xEC
 */
void XObj::DrawScreenOverlay()
{
	if ((mStatus.mFlags & 0x800ULL) && (mStatus.mFlags & 0x10000ULL)) {
		HSD_JObjSetFlags(mJObj, CLASSICAL_SCALE);
		Vec scale = mMoveScale;
		SetScale(scale.x, -scale.y, scale.z, 0); // drawn flipped
		if (mJObj != NULL)
			HSD_JObjDispAll(mJObj, NULL, 7, 0);
		SetScale(scale, 0);
	}
}

/**
 * @note Address: 0x80090568
 * @note Size: 0x4
 */
void XObj::DrawCollisionShapes()
{
}

/**
 * @note Address: 0x8009056C
 * @note Size: 0x2F8
 */
void XObj::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	if (!(mStatus.mFlags & 8ULL))
		return;
	if (ShadowSize(this) == 0)
		return;
	Vec position = *origin;
	Vec rotation = { -HALF_PI, 0.0f, 0.0f };
	Vec size     = mMoveScale * (f32)ShadowSize(this);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	u32 edge   = (u32)(128.0f * opacity);
	u32 center = (u32)(160.0f * opacity);
	DrawGsCircle(&gGame.mGs, 12, center, edge, position.x, position.y, position.z, size.x, size.z, rotation.x, rotation.y, rotation.z);
	PrimVertex vertices[26]; // a rim of transparent vertices fades the edge out
	f32 halfWidth  = size.x / 2.0f;
	f32 halfHeight = size.z / 2.0f;
	for (s32 i = 0; i < 13; i++) {
		f32 angle                 = (PI / 6.0f) * i;
		vertices[i * 2].pos.x     = position.x - halfWidth * (f32)sin(angle);
		vertices[i * 2].pos.z     = position.z + halfHeight * (f32)cos(angle);
		vertices[i * 2].pos.y     = position.y;
		vertices[i * 2].color     = edge;
		vertices[i * 2 + 1].pos.x = position.x - (1.5f * mMoveScale.x + halfWidth) * (f32)sin(angle);
		vertices[i * 2 + 1].pos.z = position.z + (1.5f * mMoveScale.z + halfHeight) * (f32)cos(angle);
		vertices[i * 2 + 1].pos.y = position.y;
		vertices[i * 2 + 1].color = 0;
	}
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, 26);
}

/**
 * @note Address: 0x80090864
 * @note Size: 0x5C
 */
void XObj::DrawGroundShadow()
{
	if (mStatus.mFlags & 8ULL) {
		DrawGroundShadowAt(&mMoveTranslate, 1.0f);
	}
}

/**
 * @note Address: 0x800908C0
 * @note Size: 0x3C4
 */
void XObj::DrawWithView(Mtx view)
{
	HSD_AObj* animation = GetAObj(mJObj);
	if (animation != NULL && (animation->flags & (AOBJ_NO_ANIM | AOBJ_REWINDED)) && HasQueuedAnim()) {
		s32 id;
		u16 frame;
		s16 rate;
		PopAnim(id, frame, rate);
		if (id & 0xFFFF0000) { // packed ids: material in the high half, joint in the low
			s32 jointId             = id & 0xFFFF;
			s32 materialId          = id >> 16;
			void* jointAnimation    = GetJointAnimation(jointId);
			void* materialAnimation = GetMaterialAnimation(materialId);
			void* shapeAnimation    = GetShapeAnimation(jointId);
			SetAnim(0, TRUE, jointAnimation, materialAnimation, shapeAnimation, (f32)frame, rate / 100.0f, 0.0f, 10.0f);
			if (jointId >= 1000 || materialId >= 1000)
				mStatus.mFlags |= 0x0008000000000000ULL;
		} else {
			SetAnim(id, FALSE, NULL, NULL, NULL, (f32)frame, rate / 100.0f, 0.0f, 10.0f);
		}
		UpdateAnim(1);
	}
	if (!(mStatus.mFlags & 0x800ULL)) {
		if (mJObj != NULL)
			HSD_JObjDispAll(mJObj, view, 7, 0);
	}
}

/**
 * @note Address: 0x80090C84
 * @note Size: 0x250
 */
void XObj::StepColors(u32 frames)
{
	if (mJObj != NULL) {
		for (s32 i = 0; i < 2; ++i) {
			XObjColorStep& color = mColorStep[i];
			StepColor(color.mValue.x, color.mTarget.x, color.mSpeed.x, frames);
			StepColor(color.mValue.y, color.mTarget.y, color.mSpeed.y, frames);
			StepColor(color.mValue.z, color.mTarget.z, color.mSpeed.z, frames);
		}
		// the stepped channels get packed back into the color words
		mColors[1] = ((u32)mColorStep[0].mValue.x << 24) | ((u32)mColorStep[0].mValue.y << 16) | ((u32)mColorStep[0].mValue.z << 8);
		mColors[0] = ((u32)mColorStep[1].mValue.x << 24) | ((u32)mColorStep[1].mValue.y << 16) | ((u32)mColorStep[1].mValue.z << 8);
	}
}

/**
 * @note Address: 0x80090ED4
 * @note Size: 0x2EC
 */
void XObj::UpdateObject(u32 frames)
{
	if (mJObj != NULL) {
		StepColors(frames);
		if (mStatus.mFlags & 0x1000000000000000ULL) { // toppling: sink onto the floor hit
			if (mMoveTranslate.y + mToppleOffset.y > mFloorHit.mHeight) {
				mMoveTranslate.mTarget.y = mFloorHit.mHeight;
				f32 speed                = 0.15f + mMoveTranslate.mSpeed.y; // fall faster, capped at 10
				if (speed > 10.0f)
					speed = 10.0f;
				mMoveTranslate.mSpeed.y = speed;
			} else { // landed: take over the offset and stand straight
				mMoveTranslate.mSpeed.y = 0.0f;
				mMoveTranslate.x += mToppleOffset.x;
				mMoveTranslate.z += mToppleOffset.z;
				mMoveTranslate.y = mFloorHit.mHeight;
				mToppleOffset.x  = 0.0f;
				mToppleOffset.y  = 0.0f;
				mToppleOffset.z  = 0.0f;
				SetRotationX(0.0f, 0);
				SetRotationZ(0.0f, 0);
				mStatus.mFlags &= ~0x1000000000000000ULL;
				if ((s16)GetPool() == 21) {
					mStatus.mFlags |= 0x2000000000000000ULL;
				} else if ((s16)GetPool() == 22) {
					if (mStatus.mFlags & 0x04000000ULL)
						mStatus.mFlags |= 0x2000000000000000ULL;
				}
			}
		}
		// move, scale and turn toward their targets, then animate
		CXObjIntp::StepRotate(frames);
		CXObjIntp::StepTranslate(frames);
		if (CXObjIntp::StepScale(frames)) {
			UpdateObjectFloorHeights((XObj*)this);
			UpdateObjectFloorXZExtent((XObj*)this);
		}
		UpdateAnim(frames);
		HSD_JObjSetupMatrix(mJObj);
	}
	if ((mStatus.mFlags & 0x0010000000000000ULL) || mMoveTranslate.y != mLastPos.y) { // floor heights track the height
		UpdateObjectFloorHeights((XObj*)this);
	}
	if (mEvt.mData.mScript != NULL)
		mEvt.Update(frames);
}

/**
 * @note Address: 0x800911C0
 * @note Size: 0xF4
 */
void XObj::Release()
{
	CJArchive* archive = &mAnim;
	archive->Release();
	if (mEvt.mData.mScript != NULL)
		ResetEventScriptState(&mEvt, TRUE, FALSE);
	if (mStatus.mFlags & 1) {
		CXObjIntp::Release();
		UnkObjData* data = mAttr.mObjData;
		if (data != NULL && data->mSource != NULL) {
			XObjSource* source = data->mSource;
			if (source->mArchive != NULL && source->mArchive->mRefCount <= 0) // its last user is gone
				source->mArchive = NULL;
		}
	}
	mAttr.mObjData   = NULL;
	mAttr.mPlacement = NULL;
	mFloors          = NULL;
	mStatus.mFlags &= ~0x10001ULL;
}

/**
 * @note Address: 0x800912B4
 * @note Size: 0xB8
 */
extern "C" void SetJointMaterialOpaque(HSD_JObj* joint, f32** arguments, s32 type)
{
	HSD_JObjModifyFlags(joint, 0x40000, 0x1C0000);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_DObjModifyFlags(draw, 2, 0xE);
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjClearFlags(material, RENDER_XLU | RENDER_NO_ZUPDATE | RENDER_ZMODE_ALWAYS);
			HSD_MObjSetFlags(material, RENDER_ALPHA_MAT);
			HSD_MObjCompileTev(material);
		}
	}
}

/**
 * @note Address: 0x8009136C
 * @note Size: 0x100
 */
extern "C" void SetJointMaterialTranslucent(HSD_JObj* joint, f32** arguments, s32 type)
{
	CXObj* object = (CXObj*)arguments[0];
	f32 alpha     = *arguments[1];
	HSD_JObjModifyFlags(joint, 0x80000, 0x1C0000);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_DObjModifyFlags(draw, 4, 0xE);
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjSetFlags(material, RENDER_XLU | RENDER_NO_ZUPDATE);
			HSD_MObjSetFlags(material, RENDER_ALPHA_MAT);
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
void XObj::SetOpaque()
{
	void* arguments[1] = { NULL };
	arguments[0]       = this;
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, SetJointMaterialOpaque, (f32**)arguments);
	HSD_JObjModifyFlags(mJObj, 0x10000000, 0x70000000);
}

/**
 * @note Address: 0x800914CC
 * @note Size: 0x74
 */
void XObj::SetTranslucent(f32 alpha)
{
	void* arguments[2] = { NULL, NULL };
	arguments[0]       = this;
	arguments[1]       = &alpha;
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, SetJointMaterialTranslucent, (f32**)arguments);
	HSD_JObjModifyFlags(mJObj, 0x20000000, 0x70000000);
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
void XObj::EnableLighting(f32 factor)
{
	void* arguments[2] = { NULL, NULL };
	arguments[0]       = this;
	arguments[1]       = &factor;
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, ScaleJointMaterialDiffuse, (f32**)arguments);
}

/**
 * @note Address: 0x80091784
 * @note Size: 0xB0
 */
extern "C" void SetJointMaterialDestAlpha(HSD_JObj* joint, f32** arguments, s32 type)
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
void XObj::SetDestinationAlpha(BOOL enabled, u8 value)
{
	void* arguments[2] = { NULL, NULL };
	arguments[0]       = (void*)enabled;
	arguments[1]       = (void*)value;
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, SetJointMaterialDestAlpha, (f32**)arguments);
}

// joint walk callback for XObj::ApplyMaterialPresets, the args are five on/off flags
/**
 * @note Address: 0x80091888
 * @note Size: 0x3BC
 */
extern "C" void ApplyJointMaterialPEPresets(HSD_JObj* joint, f32** arguments, s32 type)
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

#include "game/xobj.h"
#include "game/gmath.h"
#include "game/objectfloorquery.h"
#include "game/cjobjset.h"
#include "game/cgame.h"
#include "game/hsd_dobj.h"
#include "game/globals.h"
#include "game/xobjchr.h"

extern "C" {
void GXProject(f32 x, f32 y, f32 z, const Mtx view, const f32* projection, const f32* viewport, f32* screenX, f32* screenY, f32* screenZ);
}

static inline const void* ObjectScript(CXObj* object)
{
	return ((XObj*)object)->vf128();
}

static inline void TriggerObjectEvent(CXObj* object, u32 first, u32 second)
{
	const void* script = ObjectScript(object);
	if (!IsPlayerBlockingEvents(gGame.GetPlayer())) {
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, (u32)object, first, second);
	}
}

/**
 * @note Address: 0x8008A530
 * @note Size: 0xD4
 */
extern "C" BOOL vf140__4XObjFPC3Vecl(XObj* object, const Vec* position, s32 value)
{
	if (object->mStatus.mFlags & 0x0000040000000000ULL) {
		if (value == 56)
			return FALSE;
		const void* script = ObjectScript(object);
		if (!IsPlayerBlockingEvents(gGame.GetPlayer())) {
			StartOrQueueEventScript(&gGame.mEvt, (u8*)script, (u32)object, (s16)value, -1);
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8008A604
 * @note Size: 0xBC
 */
extern "C" void vf13C__4XObjFPC3Vecll(XObj* object, const Vec* contact, s32 value, s32 tint)
{
	if (object->mStatus.mFlags & 0x0000080000000000ULL) {
		const void* script = ObjectScript(object);
		if (!IsPlayerBlockingEvents(gGame.GetPlayer())) {
			StartOrQueueEventScript(&gGame.mEvt, (u8*)script, (u32)object, -1, (s16)((u32)value + 3000));
		}
	}
}

/**
 * @note Address: 0x8008A6C0
 * @note Size: 0x158
 */
extern "C" void OnHitFromPosition__4XObjFRC3Vecf(XObj* object, const Vec* position, f32 strength)
{
	if (object->mStatus.mFlags & 0x0000080000000000ULL) {
		if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f) {
			TriggerObjectEvent(object, -1, 2001);
		} else {
			TriggerObjectEvent(object, -1, 2000);
		}
	}
}

/**
 * @note Address: 0x8008A818
 * @note Size: 0x8
 */
extern "C" BOOL vf134__4XObjFf(void* object, f32 amount)
{
	return TRUE;
}

/**
 * @note Address: 0x8008A820
 * @note Size: 0x124
 */
extern "C" void vf12C__4XObjFlUcUcUcf(XObj* object, s32 index, u8 red, u8 green, u8 blue, f32 time)
{
	object->mColorStep[index].mTarget.x = red;
	object->mColorStep[index].mTarget.y = green;
	object->mColorStep[index].mTarget.z = blue;
	if (time == 0.0f) {
		object->mColorStep[index].mValue.x = red;
		object->mColorStep[index].mValue.y = green;
		object->mColorStep[index].mValue.z = blue;
		object->mColorStep[index].mSpeed.x = object->mColorStep[index].mSpeed.y = object->mColorStep[index].mSpeed.z = 0.0f;
	} else {
#if defined(VERSION_PAL)
		// retail reads the refresh rate for each one
		object->mColorStep[index].mSpeed.x = ((f32)red - object->mColorStep[index].mValue.x) / (gVideoFramesPerSecond * time);
		object->mColorStep[index].mSpeed.y = ((f32)green - object->mColorStep[index].mValue.y) / (gVideoFramesPerSecond * time);
		object->mColorStep[index].mSpeed.z = ((f32)blue - object->mColorStep[index].mValue.z) / (gVideoFramesPerSecond * time);
#else
		object->mColorStep[index].mSpeed.x = ((f32)red - object->mColorStep[index].mValue.x) / (60.0f * time);
		object->mColorStep[index].mSpeed.y = ((f32)green - object->mColorStep[index].mValue.y) / (60.0f * time);
		object->mColorStep[index].mSpeed.z = ((f32)blue - object->mColorStep[index].mValue.z) / (60.0f * time);
#endif
	}
}

/**
 * @note Address: 0x8008ADA0
 * @note Size: 0x338
 */
extern "C" BOOL SetClampedPushPositionXZ(XObj* object, const Vec* direction, u8 axes, f32 x, f32 z)
{
	BOOL changed = FALSE;
	if (__fpclassifyf(object->mAnchorX) == FP_NAN || __fpclassifyf(object->mAnchorZ) == FP_NAN) {
		object->SetTranslateX(x, 0);
		object->SetTranslateZ(z, 0);
	} else if (object->mAnchorX == x && object->mAnchorZ == z) {
		object->SetTranslateX(x, 0);
		object->SetTranslateZ(z, 0);
	} else {
		Mtx rotation;
		Vec displacement;
		PSMTXRotRad(rotation, 'y', -object->mMoveRotate.y);
		displacement.x = object->mAnchorX - x;
		displacement.z = object->mAnchorZ - z;
		displacement.y = 0.0f;
		PSMTXMultVec(rotation, &displacement, &displacement);

		if (axes & 1) {
			if (direction->x > 0.0f && displacement.x < object->mLimitPosX) {
				displacement.x = object->mLimitPosX;
				changed        = TRUE;
			} else if (direction->x < 0.0f && displacement.x > object->mLimitNegX) {
				displacement.x = object->mLimitNegX;
				changed        = TRUE;
			}
		}
		if (axes & 2) {
			if (direction->z > 0.0f && displacement.z < object->mLimitPosZ) {
				displacement.z = object->mLimitPosZ;
				changed        = TRUE;
			} else if (direction->z < 0.0f && displacement.z > object->mLimitNegZ) {
				displacement.z = object->mLimitNegZ;
				changed        = TRUE;
			}
		}

		PSMTXRotRad(rotation, 'y', object->mMoveRotate.y);
		PSMTXMultVec(rotation, &displacement, &displacement);
		object->SetTranslateX(object->mAnchorX - displacement.x, 0);
		object->SetTranslateZ(object->mAnchorZ - displacement.z, 0);
	}
	return changed;
}

/**
 * @note Address: 0x8008B0D8
 * @note Size: 0x14C
 */
extern "C" void vf120__4XObjFl(CJObj* object, s32 mode)
{
	switch (mode) {
	case 0:
		if (object->mJObj != NULL)
			HSD_JObjWalkTree(object->mJObj, JObjClearPObjFlags, (f32**)0x4000);
		if (object->mJObj != NULL)
			HSD_JObjWalkTree(object->mJObj, JObjClearPObjFlags, (f32**)0x8000);
		break;
	case 2:
		if (object->mJObj != NULL)
			HSD_JObjWalkTree(object->mJObj, JObjClearPObjFlags, (f32**)0x4000);
		if (object->mJObj != NULL)
			HSD_JObjWalkTree(object->mJObj, JObjSetPObjFlags, (f32**)0x8000);
		break;
	case 1:
		if (object->mJObj != NULL)
			HSD_JObjWalkTree(object->mJObj, JObjSetPObjFlags, (f32**)0x4000);
		if (object->mJObj != NULL)
			HSD_JObjWalkTree(object->mJObj, JObjClearPObjFlags, (f32**)0x8000);
		break;
	case 3:
		if (object->mJObj != NULL)
			HSD_JObjWalkTree(object->mJObj, JObjSetPObjFlags, (f32**)0x4000);
		if (object->mJObj != NULL)
			HSD_JObjWalkTree(object->mJObj, JObjSetPObjFlags, (f32**)0x8000);
		break;
	}
}

/**
 * @note Address: 0x8008B224
 * @note Size: 0x114
 */
extern "C" BOOL vf11C__4XObjFP7CCamera(const XObj* object, CCamera* camera)
{
	HSD_JObj* joint = object->mJObj;
	if (joint != NULL) {
		Vec position = GetJointPosition(joint);
		f32 screenX, screenY, screenZ;
		GXProject(position.x, position.y, position.z, camera->mViewMtx, camera->mProjection, camera->mViewport, &screenX, &screenY,
		          &screenZ);
		if (screenZ > 0.0f && screenZ < 1.0f)
			return FALSE;
		return TRUE;
	}
	return TRUE;
}

/**
 * @note Address: 0x8008B338
 * @note Size: 0x29C
 */
extern "C" BOOL ProjectVisibleBounds__4XObjFPfPfP7CCameraff(CXObj* object, f32* width, f32* height, CCamera* camera, f32 radius,
                                                            f32 objectHeight)
{
	camera          = camera != NULL ? camera : &gGame.mMotionControlCamera.mCamera;
	f32 heading     = camera->mAngle.y;
	MtxPtr view     = camera->mViewMtx;
	f32* projection = camera->mProjection;
	f32* viewport   = camera->mViewport;
	f32 left, right, bottom, top, unusedX, unusedY, depth;
	f32 angle  = (PI + HALF_PI) + heading;
	f32 cosine = (f32)cos(angle);
	f32 sine   = (f32)sin(angle);
	GXProject(radius * sine + object->mMoveTranslate.x, object->mMoveTranslate.y, radius * cosine + object->mMoveTranslate.z, view,
	          projection, viewport, &left, &unusedY, &depth);
	angle  = HALF_PI + heading;
	cosine = (f32)cos(angle);
	sine   = (f32)sin(angle);
	GXProject(radius * sine + object->mMoveTranslate.x, object->mMoveTranslate.y, radius * cosine + object->mMoveTranslate.z, view,
	          projection, viewport, &right, &unusedY, &depth);
	angle  = PI + heading;
	cosine = (f32)cos(angle);
	sine   = (f32)sin(angle);
	GXProject(radius * sine + object->mMoveTranslate.x, object->mMoveTranslate.y, radius * cosine + object->mMoveTranslate.z, view,
	          projection, viewport, &unusedX, &bottom, &depth);
	cosine = (f32)cos(heading);
	sine   = (f32)sin(heading);
	GXProject(radius * sine + object->mMoveTranslate.x, object->mMoveTranslate.y + objectHeight, radius * cosine + object->mMoveTranslate.z,
	          view, projection, viewport, &unusedX, &top, &depth);
#if defined(VERSION_PAL)
	f32 scale = 448.0f / (&gGame.mGs.mScreen)->xfbHeight;
	bottom *= scale;
	top *= scale;
#endif
	if (left >= 0.0f && right < 640.0f && bottom >= 0.0f && top < 448.0f && depth > 0.0f && depth < 1.0f) {
		if (width != NULL) {
			f32 value = right - left;
			if (value < 0.0f)
				value = -value;
			*width = value;
		}
		if (height != NULL) {
			f32 value = top - bottom;
			if (value < 0.0f)
				value = -value;
			*height = value;
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8008B5D4
 * @note Size: 0xCC
 */
void XObj::ProjectOffsetToScreen(const Vec* offset, f32* x, f32* y, f32* z, CCamera* camera)
{
	camera = camera != NULL ? camera : &gGame.mMotionControlCamera.mCamera;
	f32 screenX, screenY, screenZ;
	GXProject(mMoveTranslate.x + offset->x, mMoveTranslate.y + offset->y, mMoveTranslate.z + offset->z, camera->mViewMtx,
	          camera->mProjection, camera->mViewport, &screenX, &screenY, &screenZ);
	if (x != NULL)
		*x = screenX;
	if (y != NULL) {
#if defined(VERSION_PAL)
		*y = 448.0f * screenY / (&gGame.mGs.mScreen)->xfbHeight;
#else
		*y = screenY;
#endif
	}
	if (z != NULL)
		*z = screenZ;
}

extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);

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

extern "C" const f32 sXObjRadiansPerDegree;

// Polygon rotation rounds to single precision before adding the object yaw.
static inline f32 FloorRotationRadians(const s32& degrees)
{
	return sXObjRadiansPerDegree * degrees;
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
	to.y   = 0;

	f32 rotation = mMoveRotate.y;
	for (s32 i = 0; floors[i].mId != -1; ++i) {
		CVec center  = mMoveScale * (_794 + ReadFloorCoordinates(floors[i].mCenterOffset, gObjectFloorOffsetTemplate));
		extent       = mMoveScale * ReadFloorCoordinates(floors[i].mExtent, lbl_80378148);
		f32 localYaw = FloorRotationRadians(floors[i].mYawDegrees);
		f32 yaw      = rotation + localYaw;
		switch (floors[i].mId) {
		case 0:
		case 1: {
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
		case 2: {
			Mtx transform;
			MakeFloorTransform(this, center, extent, yaw, transform);
			f32 h = (2.0f / sqrtf(3.0f)) * 0.5f;
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
		case 3: {
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
		case 5: {
			XObjFloor* polygon = mFloors;
			Vec corners[4];
			for (s32 j = 0; j < 4; ++j)
				corners[j] = _794 + ReadFloorCoordinates(polygon[i + j].mCenterOffset, gObjectFloorOffsetTemplate);
			Vec size  = mMoveScale * ReadFloorCoordinates(polygon[i].mExtent, lbl_80378148);
			f32 angle = mMoveRotate.y + FloorRotationRadians(polygon[i].mYawDegrees);
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
			i += 3;
			break;
		}
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x8008C1F4
 * @note Size: 0x80
 */
CVec CVec::operator*(Vec other) const
{
	CVec result = *this;
	result.x *= other.x;
	result.y *= other.y;
	result.z *= other.z;
	return result;
}

extern "C" const Vec gDefaultFloorContactOffset;

/**
 * @note Address: 0x8008C274
 * @note Size: 0x558
 */
extern "C" CVec GetFloorContactPosition(XObj* object, s32 index, const Vec* offset)
{
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return object->mMoveTranslate;

	Vec displacement       = offset ? Vec(*offset) : Vec(gDefaultFloorContactOffset);
	Vec position           = object->mMoveTranslate + displacement;
	Vec center             = object->mMoveScale * (object->_794 + ReadObjectFloorOffset(object->mFloors[index]));
	Vec extent             = object->mMoveScale * ReadObjectFloorExtent(object->mFloors[index]);
	const XObjFloor& floor = object->mFloors[index];
	f32 yaw                = object->mMoveRotate.y + FloorRotationRadians(floor.mYawDegrees);
	f32 height;
	switch (floor.mId) {
	case 0:
	case 1:
		height = -1.0f;
		break;
	case 2: {
		f32 root = sqrtf(3.0f);
		f32 edge = 2.0f / root;
		height   = 0.5f * -edge;
		break;
	}
	default:
		height = NAN;
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

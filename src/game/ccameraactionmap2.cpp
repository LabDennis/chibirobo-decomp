#include <math.h>
#include "game/ccameraaction.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/xobj.h"
#include "game/caminfo.h"
#include "game/cameracollisionpolygons.h"
#include "game/cmotioncontrolcamera.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/hsd_jobj.h"
#include "game/gmath.h"
#include <string.h>

extern "C" {
#include <dolphin/os.h>
#include "game/cvec.h"
#include "game/cgs.h"
#include "game/stagedata.h"
}

static CamCollisionPolygon sCameraCollisionPolygons[256];
static Vec sPreviousMapOrbitAngle;
extern "C" void SetMapCameraModeFlags(CCameraActionMap* map, u32 flags);

extern "C" Vec GetMapCameraTargetWithHeightOffset(CCameraActionMap* map);
extern "C" void UpdateCameraEyeInterpolation(CCameraActionMap* map, u32 step);
extern "C" void StartCameraEyeInterpolation(CCameraActionMap* map, Vec to, f32 time);
extern "C" void SetMapCameraAnglesFromEyeAndInterest(CCameraActionMap* map);
extern "C" void StartMapCameraTurnInterpolation(CCameraActionMap* map, s32 unused);
extern "C" BOOL GetMapCameraTargetOffsetPosition(CCameraActionMap* map, CVec& out);
extern "C" void UpdateMapCameraPadControls(CCameraActionMap* map, u32 step);
extern "C" BOOL TrySetMapCameraTargetYaw(CCameraActionMap* map, f32 yaw, f32 time, f32 unused);
extern "C" BOOL SetMapCameraSideEndpoints(CCameraActionMap* map);
extern "C" s32 CountMapCameraAdjacentSideIntersections(CCameraActionMap* map, CamMapEntry* entry);
extern "C" s32 CountMapCameraEntriesInGroup(CCameraActionMap* map, s32 recordIndex);
extern "C" s32 FindNearestMapCameraEntry(CCameraActionMap* map, const Vec& pos);
extern "C" CamMapEntry* GetNearestMapCameraEntry(CCameraActionMap* map, const Vec& pos);

static BOOL sOrbitUsesSpring;
static s32 sTurningArea;
static s32 sPreviousPlayerHangState;
static s32 sPlayerHangState;
static BOOL sPlayerMovingTowardCamera;
static BOOL sOrbitTurnBlocked;
static f32 sOrbitTurnDirection;

extern "C" XObj* GetPlayerPoolObject(CGame* game, s32 index);
extern "C" void ResetPlayerCameraValue(XObjChr* player);
extern "C" void SetMapCameraPreviousEye(CCameraActionMap* map, const Vec& v);
extern "C" void SetMapCameraPreviousInterest(CCameraActionMap* map, const Vec& v);
extern "C" void UpdateMapCameraOrbit(CCameraActionMap* map, u32 step);
extern "C" BOOL ResolveMapCameraEyeCollision(CCameraActionMap* map, CVec& eye, const CVec& previous, const CVec& interest,
                                             const CVec& previousInterest, const s32* mode);
extern "C" CVec SpringDamp(CVec& position, const Vec& target, const CVec& previous, f32 step, f32 stiffness, f32 damping, f32 restDistance);
// out of line vector helpers
extern "C" void SetVectorComponents(CVec* v, f32 x, f32 y, f32 z);
extern "C" f32 GetHorizontalDistance(const Vec* from, const Vec* to);      // horizontal distance
extern "C" f32 GetHeadingBetweenPositions(const Vec* from, const Vec* to); // horizontal angle
extern "C" s32 ClassifyFloatBits(f32 value);                               // floating-point classification
static const f32 sMapCameraSpringStiffness = 0.5f;
static const s32 sFollowCollision          = 0;
static const s32 sOrbitCollision           = 1;

static inline s32 ClassifyAreaHeight(f32 y, f32 bottom, f32 top)
{
	if (y >= bottom && y <= top) {
		return 1;
	}
	if (y < bottom) {
		return -2;
	}
	if (y > top) {
		return -1;
	}
	return 0;
}

extern "C" void ResetMapCameraInterpolation(CCameraActionMap* map);
extern "C" BOOL TrySetUnobstructedMapCameraYaw(CCameraActionMap* map, f32 yaw);

static inline BOOL CameraPolygonContains(Vec& pos, CamCollisionPolygon& polygon)
{
	return IsPointInsideQuadXZ(&pos, &polygon.point[0], &polygon.point[1], &polygon.point[2], &polygon.point[3]);
}

static inline BOOL CameraEdgeIntersect(const Vec& from, const Vec& to, Vec& a, Vec& b, Vec& hit, s32 segment)
{
	return IntersectLinesXZ(&from, &to, &a, &b, &hit, segment);
}

/**
 * @note Address: 0x8013E26C
 * @note Size: 0x17CC
 */
extern "C" BOOL ResolveMapCameraEyeCollision(CCameraActionMap* map, CVec& eye, const CVec& previous, const CVec& interest, const CVec&,
                                             const s32* mode)
{
	CVec target(interest);
	CVec old(previous);
	CVec candidate(eye);
	if (!(target != old))
		return false;

	CamAreaRecord* records        = gCameraAreaState.mAreaExRecords;
	s32 count                     = gCameraAreaState.mAreaExCount;
	CamCollisionPolygon* polygons = sCameraCollisionPolygons;
	BOOL changed                  = false;
	BOOL targetInside             = false;
	BOOL previousInside           = false;
	BOOL candidateInside          = false;
	for (s32 i = 0; i < count; i++) {
		CamAreaRecord* record        = &records[i];
		CamCollisionPolygon& polygon = polygons[i];
		if (record->mFlags & 0x80) // an event switched this area off
			continue;
		f32 top      = record->GetTop();
		f32 bottom   = record->GetBottom();
		bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
		if (outside)
			continue;
		polygon.point[0].x = polygon.point[4].x = record->_04[0][0] / 100.0f;
		polygon.point[0].z = polygon.point[4].z = record->_04[0][1] / 100.0f;
		polygon.point[0].y = polygon.point[4].y = 0.0f;
		polygon.point[1].x                      = record->_04[1][0] / 100.0f;
		polygon.point[1].z                      = record->_04[1][1] / 100.0f;
		polygon.point[1].y                      = 0.0f;
		polygon.point[2].x                      = record->_04[2][0] / 100.0f;
		polygon.point[2].z                      = record->_04[2][1] / 100.0f;
		polygon.point[2].y                      = 0.0f;
		polygon.point[3].x                      = record->_04[3][0] / 100.0f;
		polygon.point[3].z                      = record->_04[3][1] / 100.0f;
		polygon.point[3].y                      = 0.0f;
		targetInside |= CameraPolygonContains(target, polygon);
		previousInside |= CameraPolygonContains(old, polygon);
	}

	if (previousInside) {
		f32 closest = NAN;
		CVec best;
		for (s32 i = 0; i < count; i++) {
			CamCollisionPolygon& polygon = polygons[i];
			CamAreaRecord* area          = &records[i];
			if (area->mFlags & 0x80)
				continue;
			f32 top      = area->GetTop();
			f32 bottom   = area->GetBottom();
			bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
			if (outside)
				continue;
			if (CameraPolygonContains(candidate, polygon)) {
				CVec normal, hit;
				for (s32 side = 0; side < 4; side++) {
					Vec& a = polygon.point[side];
					Vec& b = polygon.point[side + 1];
					GetOutwardPerpendicularToEdge(&candidate, &a, &b, &normal);
					normal.y = 0.0f;
					PSVECNormalize(&normal, &normal);
					CameraEdgeIntersect(candidate, candidate + normal, a, b, hit, 0);
					f32 distance = PSVECDistance(&candidate, &hit);
					if (__fpclassifyf(closest) == FP_NAN || closest > distance) {
						closest = distance;
						best.Set(hit + normal * 2.0f);
					}
				}
			}
		}
		if (__fpclassifyf(closest) != FP_NAN) {
			candidate.Set(best);
			previousInside  = false;
			candidateInside = false;
			for (s32 i = 0; i < count; i++) {
				CamCollisionPolygon& polygon = polygons[i];
				CamAreaRecord* area          = &records[i];
				if (area->mFlags & 0x80)
					continue;
				f32 top      = area->GetTop();
				f32 bottom   = area->GetBottom();
				bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
				if (outside)
					continue;
				if (CameraPolygonContains(candidate, polygon)) {
					candidateInside = true;
					break;
				}
			}
			if (candidateInside) {
				for (s32 i = 0; i < count; i++) {
					CamCollisionPolygon& polygon = polygons[i];
					CamAreaRecord* area          = &records[i];
					if (area->mFlags & 0x80)
						continue;
					f32 top      = area->GetTop();
					f32 bottom   = area->GetBottom();
					bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
					if (outside)
						continue;
					const CVec& from = targetInside ? static_cast<const CVec&>(map->mTarget->mMoveTranslate) : target;
					CVec normal, hit;
					for (s32 side = 0; side < 4; side++) {
						Vec& a = polygon.point[side];
						Vec& b = polygon.point[side + 1];
						if (CameraEdgeIntersect(from, old, a, b, hit, 1)) {
							GetOutwardPerpendicularToEdge(&old, &a, &b, &normal);
							normal.y = 0.0f;
							PSVECNormalize(&normal, &normal);
							CameraEdgeIntersect(old, old + normal, a, b, hit, 0);
							f32 distance = PSVECDistance(&target, &hit);
							if (__fpclassifyf(closest) == FP_NAN || closest > distance) {
								closest = distance;
								best.Set(hit + normal * 2.0f);
							}
						}
					}
				}
				if (__fpclassifyf(closest) != FP_NAN) {
					candidate.Set(best);
					previousInside = false;
				}
			}
		}
	} else {
		f32 yaw = map->mTarget->mMoveRotate.y;
		Vec behind;
		behind.x = map->mTarget->mMoveTranslate.x - 2.0f * (f32)sin(yaw);
		behind.z = map->mTarget->mMoveTranslate.z - 2.0f * (f32)cos(yaw);
		behind.y = target.y;
		for (s32 i = 0; i < count; i++) {
			CamCollisionPolygon& polygon = polygons[i];
			CamAreaRecord* area          = &records[i];
			if (area->mFlags & 0x80)
				continue;
			f32 top      = area->GetTop();
			f32 bottom   = area->GetBottom();
			bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
			if (outside)
				continue;
			CVec push, firstHit, normal;
			BOOL inside = CameraPolygonContains(target, polygon);
			if (!inside)
				inside = CameraPolygonContains(behind, polygon);
			s32 crossedSide = -1;
			for (s32 side = 0; side < 4; side++) {
				if (ResolveCircleWallCollisionXZ(&old, &candidate, &polygon.point[side], &polygon.point[side + 1], &push, &firstHit, NULL,
				                                 NULL, 2.0f, 0.0f, 0)) {
					crossedSide = side;
					break;
				}
			}
			candidateInside |= CameraPolygonContains(candidate, polygon);
			if (!inside) {
				if (crossedSide >= 0) {
					CVec nearest, hit;
					f32 closest     = NAN;
					s32 nearestSide = -1;
					for (s32 side = 0; side < 4; side++) {
						if (ResolveCircleWallCollisionXZ(&target, &candidate, &polygon.point[side], &polygon.point[side + 1], &push, &hit,
						                                 NULL, NULL, 2.0f, 0.0f, 0)) {
							f32 distance = PSVECDistance(&hit, &target);
							if (__fpclassifyf(closest) == FP_NAN || closest > distance) {
								closest = distance;
								nearest.Set(hit);
								nearestSide = side;
							}
						}
					}
					if (nearestSide >= 0) {
						if (*mode == 1) {
							candidate.Set(nearest);
							candidate.y        = eye.y;
							changed            = true;
							f32 dx             = target.x - map->mTarget->mMoveTranslate.x;
							f32 dz             = target.z - map->mTarget->mMoveTranslate.z;
							f32 playerDistance = sqrtf(dx * dx + dz * dz);
							dx                 = target.x - candidate.x;
							dz                 = target.z - candidate.z;
							if (sqrtf(dx * dx + dz * dz) <= playerDistance) {
								ResetMapCameraInterpolation(map);
								TrySetUnobstructedMapCameraYaw(map, (f32)atan2(candidate.x - old.x, candidate.z - old.z));
							}
						} else {
							s32 difference = nearestSide - crossedSide;
							if (difference < 0)
								difference = -difference;
							if (nearestSide != crossedSide && difference % 2 != 0) {
								candidate.Set(firstHit);
								candidate.y = eye.y;
							} else {
								candidate.Set(nearest);
								candidate.y = eye.y;
							}
						}
					}
				}
			} else if (crossedSide >= 0) {
				Vec& a = polygon.point[crossedSide];
				Vec& b = polygon.point[crossedSide + 1];
				if (*mode == 1) {
					CVec hit;
					GetOutwardPerpendicularToEdge(&old, &a, &b, &normal);
					if (CameraEdgeIntersect(old, old + normal, a, b, hit, 1)) {
						normal.y = 0.0f;
						PSVECNormalize(&normal, &normal);
						candidate.Set(hit - normal * 2.0f);
						candidate.y = eye.y;
					} else
						candidate.Set(old);
					ResetMapCameraInterpolation(map);
					changed = true;
				} else {
					CVec direction, hit;
					GetOutwardPerpendicularToEdge(&candidate, &a, &b, &direction);
					if (CameraEdgeIntersect(candidate, candidate + direction, a, b, hit, 1)) {
						direction.y = 0.0f;
						PSVECNormalize(&direction, &direction);
						candidate.Set(hit - direction * 2.0f);
						candidate.y = eye.y;
					} else
						candidate.Set(old);
				}
			}
		}
	}
	if (!previousInside) {
		s32 insideCount = 0;
		for (s32 i = 0; i < count; i++) {
			CamAreaRecord* area = &records[i];
			if (area->mFlags & 0x80)
				continue;
			f32 top      = area->GetTop();
			f32 bottom   = area->GetBottom();
			bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
			if (outside)
				continue;
			if (CameraPolygonContains(candidate, polygons[i]))
				insideCount++;
		}
		if (insideCount > 0) {
			candidate.Set(old);
			if (*mode == 1) {
				ResetMapCameraInterpolation(map);
				changed = true;
			}
		}
	}
	eye.Set(candidate);
	return changed;
}

/**
 * @note Address: 0x8013FA38
 * @note Size: 0x1574
 */
void CCameraActionMap::Update(u32 step)
{
	if (step > 2)
		step = 2;
	if (mTarget == NULL)
		return;

	Vec pos          = mTarget->mMoveTranslate;
	mInsideAreaCount = 0;
	for (u32 i = 0; i < 32; i++)
		mInsideAreas[i] = -2;
	pos.y += 10.0f;
	mInsideAreaCount = CollectCameraAreasContainingPoint(&gCameraAreaState, pos, mInsideAreas);
	f32 dt           = 0.1f * step;
	mSpringPrevTarget.Set(mSpringTarget);
	GetMapCameraTargetOffsetPosition(this, mSpringTarget);
	UpdateMapCameraPadControls(this, step);
	// orbiting with no turn running and the target moved: go back to trailing it on the spring
	if ((mStatus.mFlags & 0x20000) && !(mStatus.mFlags & 0x3E) && mSpringTarget != mSpringPrevTarget) {
		SetMapCameraModeFlags(this, 0x10000);
	}

	// retail steps these twice
	for (s32 i = 0; i < 2; i++) {
		if (mHeightMode == 0) {
			if (mHeightInterp != NULL)
				mHeightInterp->Update(step, FRAME_TIME);
			f32 height     = mHeightInterp->Get();
			mCurrentHeight = height + gGame.GetPlayer()->GetCameraTarget().y;
		}
		if (mHeightMode == 1) {
			if (mAbsHeightInterp != NULL)
				mAbsHeightInterp->Update(step, FRAME_TIME);
			mCurrentHeight = mAbsHeightInterp->Get();
		}
	}
	if (mTargetHeightOffsetInterp != NULL)
		mTargetHeightOffsetInterp->Update(step, FRAME_TIME);
	if (mEyeYOffsetInterp != NULL) {
		if (!mEyeYOffsetInterp->Update(step, FRAME_TIME))
			mEyeYOffset = mEyeYOffsetInterp->Get();
	} else {
		mEyeYOffsetInterp = NULL;
	}

	// spring mode: the eye trails mSpringTarget on a damped spring
	if (mStatus.mFlags & 0x10000) {
		{
			CVec eye(mEye);
			f32 previousDistance = PSVECDistance(&eye, &mSpringPrevTarget);
			f32 distance         = PSVECDistance(&eye, &mSpringTarget);
			f32 rest             = mRestDistance;
			f32 targetRest       = mTargetRestDistance;
			if (rest != targetRest && distance > previousDistance) {
				if (distance < targetRest && rest != targetRest)
					rest = distance;
				if (distance >= targetRest)
					rest = targetRest;
			} else if (distance < previousDistance) {
				rest = targetRest;
			}
			mRestDistance = rest;
		}
		CVec eye;
		eye.Set(mEye);
		eye.Set(SpringDamp(eye, mSpringTarget, mSpringPrevTarget, dt, sMapCameraSpringStiffness, mSpringDamping, mRestDistance));
		SetEye(eye);
		SetInterest(GetMapCameraTargetWithHeightOffset(this));
		SetEye(mEye.x, mPreviousEye.y, mEye.z);
		SetMapCameraAnglesFromEyeAndInterest(this);
		if (ClassifyFloatBits(mAngle.x) == FP_NAN || ClassifyFloatBits(mAngle.y) == FP_NAN || ClassifyFloatBits(mAngle.z) == FP_NAN) {
			OSPanic("ccameraactionmap2.cpp", 581, "%s%s",
			        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
			                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
			                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
			                             mEye.x, mEye.y, mEye.z, mAngle.x, mAngle.y, mAngle.z, mDistance, mStatus.mFlags,
			                             mPreviousStatus.mFlags, mManualInput, sPlayerHangState, sPreviousPlayerHangState, mTargetEye.x,
			                             mTargetEye.y, mTargetEye.z, mPrevTargetEye.x, mPrevTargetEye.y, mPrevTargetEye.z, mEyeInterp,
			                             mTurnInterpolation, mOrbitEye.x, mOrbitEye.y, mOrbitEye.z, mOrbitAngle.x, mOrbitAngle.y,
			                             mOrbitAngle.z, mPrevOrbitAngle.x, mPrevOrbitAngle.y, mPrevOrbitAngle.z, 0),
			        "Update");
		}

		Vec movement;
		PSVECSubtract(&mSpringTarget, &mSpringPrevTarget, &movement);
		movement.y = 0.0f;
		if (movement.x != 0.0f || movement.z != 0.0f) {
			PSVECNormalize(&movement, &movement);
			Vec view;
			view.x  = sin(mAngle.y);
			view.y  = 0.0f;
			view.z  = cos(mAngle.y);
			f32 dot = PSVECDotProduct(&movement, &view);
			CVec current(mEye);
			f32 distance = GetHorizontalDistance(&current, &mSpringTarget);
			// the target heads back at the camera (over 120 degrees off the camera's view) within 20 of the eye
			if (dot < -0.5f && distance < 20.0f)
				sPlayerMovingTowardCamera = 1;
			else
				sPlayerMovingTowardCamera = 0;
		}
		sPreviousPlayerHangState = sPlayerHangState;
		sPlayerHangState         = gGame.GetPlayer()->mHang.mState;
		// on entering hang state 3, swing round in front of the target if no area edge is in the way
		if (gGame.GetPlayer()->mHang.mState == 3 && sPlayerHangState != sPreviousPlayerHangState) {
			Vec center = mTarget->mMoveTranslate;
			CVec angles;
			SetVectorComponents(&angles, mAngle.x, PI + mTarget->mMoveRotate.mTarget.y, mAngle.z);
			f32 yaw   = HALF_PI + angles.y;
			f32 pitch = mAngle.x;
			f32 distance;
			{
				CVec target(mTarget->GetCameraTarget());
				CVec current(mEye);
				distance = PSVECDistance(&target, &current);
			}
			Vec candidate;
			candidate.x = center.x + distance * (f32)cos(yaw) * (f32)cos(pitch);
			candidate.z = center.z + distance * -(f32)sin(yaw) * (f32)cos(pitch);
			candidate.y = center.y;

			mIntersectionCount = CalcIntersectVec(candidate, center, mIntersectionEntries, 0);
			if (mIntersectionCount == 0 && TrySetMapCameraTargetYaw(this, PI + mTarget->mMoveRotate.mTarget.y, 20.0f, 0.0f)) {
				SetMapCameraModeFlags(this, 0x20002);
				goto checkCollision;
			}
		}
		// the target came back at the camera: put the eye behind its facing, at least 31 away
		if (sPlayerMovingTowardCamera != 0) {
			SetInterest(GetMapCameraTargetWithHeightOffset(this));
			CVec angles(mAngle.x, mTarget->mMoveRotate.y, mAngle.z);
			SetAngle(angles);
			CVec current(mEye);
			f32 distance = PSVECDistance(&current, &mSpringTarget);
			if (distance < 31.0f)
				distance = 31.0f;
			f32 pitch = mAngle.x;
			f32 yaw   = HALF_PI + mAngle.y;
			Vec target;
			target.x = mSpringTarget.x + distance * (f32)cos(yaw) * (f32)cos(pitch);
			target.z = mSpringTarget.z + distance * -(f32)sin(yaw) * (f32)cos(pitch);
			target.y = mCurrentHeight;
			SetEye(target);
			gGame.GetPlayer()->ResetCameraValue();
			SetMapCameraAnglesFromEyeAndInterest(this);
			if (ClassifyFloatBits(mAngle.x) == FP_NAN || ClassifyFloatBits(mAngle.y) == FP_NAN || ClassifyFloatBits(mAngle.z) == FP_NAN) {
				OSPanic("ccameraactionmap2.cpp", 683, "%s%s",
				        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
				                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
				                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
				                             mEye.x, mEye.y, mEye.z, mAngle.x, mAngle.y, mAngle.z, mDistance, mStatus.mFlags,
				                             mPreviousStatus.mFlags, mManualInput, sPlayerHangState, sPreviousPlayerHangState, mTargetEye.x,
				                             mTargetEye.y, mTargetEye.z, mPrevTargetEye.x, mPrevTargetEye.y, mPrevTargetEye.z, mEyeInterp,
				                             mTurnInterpolation, mOrbitEye.x, mOrbitEye.y, mOrbitEye.z, mOrbitAngle.x, mOrbitAngle.y,
				                             mOrbitAngle.z, mPrevOrbitAngle.x, mPrevOrbitAngle.y, mPrevOrbitAngle.z, 0),
				        "Update");
			}
			sPlayerMovingTowardCamera = 0;
		} else {
			SetMapCameraAnglesFromEyeAndInterest(this);
			if (ClassifyFloatBits(mAngle.x) == FP_NAN || ClassifyFloatBits(mAngle.y) == FP_NAN || ClassifyFloatBits(mAngle.z) == FP_NAN) {
				OSPanic("ccameraactionmap2.cpp", 693, "%s%s",
				        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
				                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
				                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
				                             mEye.x, mEye.y, mEye.z, mAngle.x, mAngle.y, mAngle.z, mDistance, mStatus.mFlags,
				                             mPreviousStatus.mFlags, mManualInput, sPlayerHangState, sPreviousPlayerHangState, mTargetEye.x,
				                             mTargetEye.y, mTargetEye.z, mPrevTargetEye.x, mPrevTargetEye.y, mPrevTargetEye.z, mEyeInterp,
				                             mTurnInterpolation, mOrbitEye.x, mOrbitEye.y, mOrbitEye.z, mOrbitAngle.x, mOrbitAngle.y,
				                             mOrbitAngle.z, mPrevOrbitAngle.x, mPrevOrbitAngle.y, mPrevOrbitAngle.z, 0),
				        "Update");
			}
			CVec target(mTarget->mMoveTranslate);
			target.y += 10.0f;
			CVec current(mEye);
			current.y = target.y;
			CVec direction;
			PSVECSubtract(&target, &current, &direction);
			PSVECNormalize(&direction, &direction);
			target += direction * 3.0f;
			mIntersectionCount  = CalcIntersectVec(mEye, target, mIntersectionEntries, 0);
			mNearestTargetIndex = FindNearestMapCameraEntry(this, mTarget->mMoveTranslate);
			mNearestTargetEntry = GetNearestMapCameraEntry(this, mTarget->mMoveTranslate);
			mNearestEyeIndex    = FindNearestMapCameraEntry(this, mEye);
			mNearestEyeEntry    = GetNearestMapCameraEntry(this, mEye);
			SetMapCameraSideEndpoints(this);
			if (mNearestEyeEntry != NULL) {
				// turn only if the eye-to-target line cuts a corner of the area hit nearest the eye: two hits, on neighbouring sides
				s32 count    = CountMapCameraEntriesInGroup(this, mNearestEyeEntry->mRecordIndex);
				s32 adjacent = CountMapCameraAdjacentSideIntersections(this, mNearestEyeEntry);
				if (count < 2)
					mTurnBlocked = 1;
				else if (adjacent == 0)
					mTurnBlocked = 1;
				else {
					s32 allowsTurn;
					{
						CamAreaInfo area(&gCameraAreaState.mAreaExRecords[mNearestEyeEntry->mRecordIndex]);
						allowsTurn = (area.mRecord->mFlags & 1) != 0;
					}
					if (allowsTurn)
						mTurnBlocked = 0;
					else
						mTurnBlocked = 1;
					if (gGame.GetPlayer()->mCopter.mState != 0 || gGame.GetPlayer()->mJump.mState != 0) {
						mTurnBlocked = 1;
					}
				}
			}
			if (_8C.y == _8C.mTarget.y && mNearestEyeEntry != NULL && mTurnBlocked == 0) {
				StartMapCameraTurnInterpolation(this, 0);
			}
		}
	checkCollision:
		CVec current(mEye);
		CVec previous(mPreviousEye);
		ResolveMapCameraEyeCollision(this, current, previous, mInterest, mPreviousInterest, &sFollowCollision);
		SetEye(current);
	}

	// orbit mode: UpdateMapCameraOrbit turns the eye round the interest, then the eye eases to mTargetEye
	if (mStatus.mFlags & 0x20000) {
		SetInterest(GetMapCameraTargetWithHeightOffset(this));
		mTargetEye.y = mPrevTargetEye.y;
		UpdateMapCameraOrbit(this, step);
		mTargetEye.y = mPrevTargetEye.y;
		if (sOrbitUsesSpring != 0) {
			CVec previous(mPrevTargetEye);
			mPrevTargetEye.Set(mTargetEye);
			CVec current;
			current.Set(mTargetEye);
			current.Set(
			    SpringDamp(current, mSpringTarget, mSpringPrevTarget, dt, sMapCameraSpringStiffness, mSpringDamping, mTargetRestDistance));
			mTargetEye.Set(current);
			mPrevTargetEye.Set(previous);
		}

		f32 movementDistance = GetHorizontalDistance(&mTargetEye, &mPrevTargetEye);
		// Skip area collision during the turn interpolation (eye mode 4).
		if (!(mStatus.mFlags & 4)) {
			s32 wasBlocked = sOrbitTurnBlocked;
			f32 before     = GetHeadingBetweenPositions(&mTargetEye, &mInterest);
			BOOL hit       = ResolveMapCameraEyeCollision(this, mTargetEye, mPrevTargetEye, mInterest, mPreviousInterest, &sOrbitCollision);
			// A collision that pushes against the last turn direction blocks the turn.
			if (sOrbitTurnDirection > 0.0f) {
				f32 after = GetHeadingBetweenPositions(&mTargetEye, &mInterest);
				before    = WrapAngle(before);
				after     = WrapAngle(after);
				if (after - before < 0.0f) {
					sOrbitTurnBlocked = 1;
				} else {
					sOrbitTurnBlocked = 0;
				}
			} else if (sOrbitTurnDirection < 0.0f) {
				f32 after = GetHeadingBetweenPositions(&mTargetEye, &mInterest);
				before    = WrapAngle(before);
				after     = WrapAngle(after);
				if (after - before > 0.0f) {
					sOrbitTurnBlocked = 1;
				} else {
					sOrbitTurnBlocked = 0;
				}
			}
			if (sOrbitTurnBlocked == 1 && wasBlocked != sOrbitTurnBlocked) {
				mTargetEye.Set(mPrevTargetEye);
				mOrbitAngle.Set(mPrevOrbitAngle);
			}
			if (hit != 0) {
				if (sOrbitUsesSpring != 0)
					sOrbitUsesSpring = 0;
			} else {
				if (sOrbitUsesSpring == 0)
					sOrbitUsesSpring = 1;
			}
		}
		f32 distance = GetHorizontalDistance(&mTargetEye, &mPrevTargetEye);
		if (distance > movementDistance)
			mEyeInterpTime = 0.0f;
		else {
			mEyeInterpTime = 3.0f;
			if (mStatus.mFlags & 0x10)
				mEyeInterpTime = 5.0f;
		}
		mPrevTargetEye.y = mTargetEye.y = mCurrentHeight;
		SetEye(CVec(mEye.x, mTargetEye.y, mEye.z));
		Vec target = mTargetEye;
		if (mPrevTargetEye != target)
			StartCameraEyeInterpolation(this, target, mEyeInterpTime);
	}

	UpdateCameraEyeInterpolation(this, step);
	f32 weight = GetHorizontalDistance(&mEye, &mSpringTarget) / 60.0f;
	if (weight > 1.0f)
		weight = 1.0f;
	if (weight < 0.0f)
		weight = 0.0f;
	if (gGame.mMotionControlCamera.GetMapDistance() <= 1.0f)
		weight = 0.0f;
	f32 interestHeight = mInterest.y;
	f32 height         = mCurrentHeight;
	f32 difference     = height - interestHeight;
	if (difference < 0.0f)
		difference = 0.0f;
	f32 weighted = difference * weight;
	f32 offset   = interestHeight + weighted;
	offset -= height;
	f32 from = mEyeYOffsetInterp->Get();
	if (0.0f != offset) {
		mEyeYOffsetInterp = &mEyeYOffsetEaseInOut;
		mEyeYOffsetInterp->Start(from, offset, 0.05f);
	}
	Vec eye = mEye;
	eye.y   = mCurrentHeight;
	eye.y += mEyeYOffset;
	SetEye(eye);
	SetMapCameraPreviousEye(this, mEye);
	mPrevTargetEye.Set(mTargetEye);
	mPrevOrbitEye.Set(mOrbitEye);
	SetMapCameraPreviousInterest(this, mInterest);
	mPreviousStatus.mFlags = 0;
	mPreviousStatus.mFlags |= mStatus.mFlags;
}

static inline BOOL SetMapYaw(CCameraActionMap* map, f32 yaw, f32 time)
{
	f32 oldYaw   = map->mTargetYaw;
	f32 oldSpeed = map->mTurnSpeed;

	map->mTargetYaw = WrapAngle(yaw);
	f32 from        = WrapAngle(map->mAngle.y);
	f32 to          = WrapAngle(yaw);

	f32 diff = to - from;
	if (diff < 0.0f) {
		diff = -diff;
	}
	if (diff > 0.0f) {
		if (diff >= TWO_PI) {
			diff -= TWO_PI;
		}
		if (diff > PI) {
			if (to < from) {
				to += TWO_PI;
			} else {
				to -= TWO_PI;
			}
		}
	}

	diff = to - from;
	if (diff < 0.0f) {
		diff = -diff;
	}
	map->mTurnSpeed = diff / time;
	if (map->mTurnSpeed < (2.0f * DEG2RAD)) {
		map->mTargetYaw = oldYaw;
		map->mTurnSpeed = oldSpeed;
		return FALSE;
	}
	return TRUE;
}

/**
 * @note Address: 0x80139E58
 * @note Size: 0x3AC
 */
extern "C" BOOL TrySetUnobstructedMapCameraYaw(CCameraActionMap* map, f32 yaw)
{
	Vec center = map->mTarget->mMoveTranslate;
	f32 pitch  = map->mAngle.x;
	CVec angles(pitch, yaw, map->mAngle.z);
	f32 angle = HALF_PI + angles.y;
	f32 distance;
	{
		CVec target(map->mTarget->GetCameraTarget());
		CVec eye(map->mEye);
		distance = PSVECDistance(&target, &eye);
	}
	Vec pos;
	pos.x = center.x + distance * (f32)cos(angle) * (f32)cos(pitch);
	pos.z = center.z + distance * -(f32)sin(angle) * (f32)cos(pitch);
	pos.y = center.y;

	map->mIntersectionCount = map->CalcIntersectVec(pos, center, map->mIntersectionEntries, 0);
	if (map->mIntersectionCount == 0 && SetMapYaw(map, yaw, 25.0f)) {
		SetMapCameraModeFlags(map, 0x20002);
		return TRUE;
	}
	return FALSE;
}
/**
 * @note Address: 0x8013A204
 * @note Size: 0x44
 */
extern "C" void StartMapCameraTargetHeightInterpolation(CCameraActionMap* map, f32 from, f32 to, f32 time)
{
	if (0.0f != time) {
		map->mTargetHeightOffsetInterp = &map->mTargetHeightOffsetEase;
		map->mTargetHeightOffsetInterp->Start(from, to, time);
	}
}

/**
 * @note Address: 0x8013A248
 * @note Size: 0xA4
 */
extern "C" Vec GetMapCameraTargetWithHeightOffset(CCameraActionMap* map)
{
	Vec target = map->mTarget->GetCameraTarget();
	if (map->mTargetHeightOffsetInterp != NULL) {
		target.y += map->mTargetHeightOffsetInterp->Get();
	}
	return target;
}

extern "C" void ResetMapCameraTargetEye(CCameraActionMap* map);

// mSpringPrevTarget follows mSpringTarget
static inline void SetCenter(CCameraActionMap* map, const Vec& pos)
{
	map->mSpringTarget     = pos;
	map->mSpringPrevTarget = map->mSpringTarget;
}

static inline void ResetCenter(CCameraActionMap* map)
{
	Vec* center = &map->mSpringTarget;
	SetCenter(map, *center);
	CVec eye           = map->mEye;
	map->mRestDistance = PSVECDistance(&eye, center);
	sOrbitUsesSpring   = 1;
	map->mStatus.mFlags &= ~0x700;
	if (map->mEyeInterp != NULL) {
		map->mEyeInterp = NULL;
	}
}

/**
 * @note Address: 0x8013A2EC
 * @note Size: 0x26C
 */
extern "C" void SetMapCameraModeFlags(CCameraActionMap* map, u32 flags)
{
	// eye modes (0x3E): 2 eases the yaw to mTargetYaw, 4 runs mTurnInterpolation, 8 turns by the pad
	// follow modes (0x70000): 0x10000 trails the target on a spring, 0x20000 orbits it (UpdateMapCameraOrbit)
	u32 eyeMode = flags & 0x3E;
	if (!map->mStatus.Check(eyeMode)) {
		map->mStatus.mFlags &= ~0x3E;
		map->mStatus.mFlags |= eyeMode;
		if (eyeMode != 4) {
			map->mTurnInterpolation = NULL;
		}
	}

	u32 mode = flags & 0x70000;
	if (!map->mStatus.Check(mode)) {
		map->mStatus.mFlags &= ~0x70000;
		map->mStatus.mFlags |= mode;
		if (mode == 0x20000) {
			ResetMapCameraTargetEye(map);
			if (sPlayerMovingTowardCamera == 1) {
				map->mOrbitRadius = map->mTargetRestDistance;
			} else if (eyeMode == 4) {
				CVec pos          = map->mSpringTarget;
				CVec eye          = map->mEye;
				map->mOrbitRadius = PSVECDistance(&pos, &eye);
			} else {
				CVec target = map->mTarget->GetCameraTarget();
				CVec eye    = map->mEye;
				f32 dist    = PSVECDistance(&eye, &target);
				if (dist < 31.0f) {
					dist = 31.0f;
				}
				map->mOrbitRadius = dist;
			}
		} else if (mode == 0x10000) {
			ResetCenter(map);
		}
	}
}

/**
 * @note Address: 0x8013A558
 * @note Size: 0xB0
 */
extern "C" f32 GetMapCameraHorizontalTargetDistance(CCameraActionMap* map)
{
	CVec pos = map->mTarget->mMoveTranslate;
	f32 y    = map->mTarget->GetCameraTarget().y;
	pos.y    = y;
	CVec eye = map->mEye;
	eye.y    = y;
	return PSVECDistance(&pos, &eye);
}

/**
 * @note Address: 0x8013A608
 * @note Size: 0x98
 */
extern "C" void UpdateCameraEyeInterpolation(CCameraActionMap* map, u32 step)
{
	if (map->mEyeInterp != NULL) {
		if (!map->mEyeInterp->Update(step, FRAME_TIME)) {
			map->SetEye(map->mEyeInterp->Get());
		} else {
			map->mEyeInterp = NULL;
			map->mStatus.mFlags &= ~0x3E;
		}
	}
}

/**
 * @note Address: 0x8013A6A0
 * @note Size: 0x70
 */
extern "C" void StartCameraEyeInterpolation(CCameraActionMap* map, Vec to, f32 time)
{
	map->mEyeInterp = &map->mEyeEaseOutDivide;
	map->mEyeInterp->Start(map->mEye, to, time);
}

/**
 * @note Address: 0x8013A710
 * @note Size: 0x90
 */
extern "C" void ResetMapCameraInterpolation(CCameraActionMap* map)
{
	map->mOrbitEye       = map->mEye;
	map->mPrevOrbitEye   = map->mEye;
	map->mOrbitAngle     = map->mAngle;
	map->mPrevOrbitAngle = map->mAngle;
	map->mEyeInterp      = NULL;
	sOrbitTurnDirection  = 0.0f;
	map->mOrbitAngle.y   = sPreviousMapOrbitAngle.y;
	map->mStatus.mFlags &= ~0x3E;
}

// distance from pos to v, ignoring pos's height
static inline f32 FlatDistance(const CVec& pos, CVec& v)
{
	CVec flat(pos.x, v.y, pos.z);
	return PSVECDistance(&flat, &v);
}

/**
 * @note Address: 0x8013A7A0
 * @note Size: 0x134
 */
extern "C" void ResetMapCameraTargetEye(CCameraActionMap* map)
{
	map->mOrbitEye       = map->mEye;
	map->mPrevOrbitEye   = map->mEye;
	map->mOrbitAngle     = map->mAngle;
	map->mPrevOrbitAngle = map->mAngle;
	map->mPrevTargetEye  = map->mEye;
	map->mTargetEye      = map->mPrevTargetEye;
	map->mEyeInterp      = NULL;

	map->mFlatDistance = FlatDistance(map->mTargetEye, map->mInterest);
}

/**
 * @note Address: 0x8013A8D4
 * @note Size: 0x288
 */
extern "C" void SetMapCameraAnglesFromEyeAndInterest(CCameraActionMap* map)
{
	f32 dx    = map->mInterest.x - map->mEye.x;
	f32 dy    = map->mInterest.y - map->mEye.y;
	f32 dz    = map->mInterest.z - map->mEye.z;
	f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
	Vec angle;
	angle.x = dy < 0.0f ? HALF_PI - pitch : pitch - HALF_PI;
	angle.y = atan2(map->mInterest.x - map->mEye.x, map->mInterest.z - map->mEye.z);
	angle.z = 0.0f;
	map->SetAngle(angle);
}
/**
 * @note Address: 0x8013AB5C
 * @note Size: 0x668
 */
extern "C" void UpdateMapCameraTurnInterpolation(CCameraActionMap* map, u32 step)
{
	if (map->mTurnInterpolation != NULL) {
		if (!map->mTurnInterpolation->Update(step, FRAME_TIME)) {
			f32 radius = map->mOrbitRadius;
			CVec position(map->mTurnInterpolation->Get());
			CVec origin(map->mSpringTarget);
			origin.y        = 0.0f;
			position        = origin + CVec((position - origin).Normalize()) * radius;
			position.y      = map->mEye.y;
			map->mOrbitEye  = position;
			map->mTargetEye = position;

			f32 dz    = map->mInterest.z - map->mEye.z;
			f32 dx    = map->mInterest.x - map->mEye.x;
			f32 dy    = map->mInterest.y - map->mEye.y;
			f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
			Vec angle;
			angle.x = dy < 0.0f ? HALF_PI - pitch : pitch - HALF_PI;
			angle.y = atan2(map->mInterest.x - map->mEye.x, map->mInterest.z - map->mEye.z);
			angle.z = 0.0f;
			map->SetAngle(angle);
			if (__fpclassifyf(map->mAngle.x) == FP_NAN || __fpclassifyf(map->mAngle.y) == FP_NAN
			    || __fpclassifyf(map->mAngle.z) == FP_NAN) {
				OSPanic("ccameraactionmap2.cpp", 3980, "%s%s",
				        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
				                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
				                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
				                             map->mEye.x, map->mEye.y, map->mEye.z, map->mAngle.x, map->mAngle.y, map->mAngle.z,
				                             map->mDistance, map->mStatus.mFlags, map->mPreviousStatus.mFlags, map->mManualInput,
				                             sPlayerHangState, sPreviousPlayerHangState, map->mTargetEye.x, map->mTargetEye.y,
				                             map->mTargetEye.z, map->mPrevTargetEye.x, map->mPrevTargetEye.y, map->mPrevTargetEye.z,
				                             map->mEyeInterp, map->mTurnInterpolation, map->mOrbitEye.x, map->mOrbitEye.y, map->mOrbitEye.z,
				                             map->mOrbitAngle.x, map->mOrbitAngle.y, map->mOrbitAngle.z, map->mPrevOrbitAngle.x,
				                             map->mPrevOrbitAngle.y, map->mPrevOrbitAngle.z, 0),
				        "UpdateTurn");
			}
			map->mOrbitAngle  = map->mAngle;
			map->mManualInput = 1;
		} else {
			map->mStatus.mFlags &= ~4;
			map->mTurnInterpolation = NULL;
		}
	}
}
extern "C" void SetMapCameraModeFlags(CCameraActionMap* map, u32 flags);

/**
 * @note Address: 0x8013B1C4
 * @note Size: 0x204
 */
extern "C" void StartMapCameraTurnInterpolation(CCameraActionMap* map, s32 unused)
{
	CVec to(map->mSideStart);
	CVec eyeNormal(map->mNearestEyeEntry->mNormal);
	CVec targetNormal(map->mNearestTargetEntry->mNormal);
	CVec move = eyeNormal * 20.0f;
	move += targetNormal * 5.0f;
	to += move;

	Vec from = map->mEye;
	from.y   = 0.0f;
	if (map->mTurnInterpolation == NULL) {
		map->mTurnInterpolation = &map->mTurnEaseInOut;
		map->mTurnInterpolation->Start(from, to, 0.7f);
		map->_DB0 = 0;
		gGame.GetPlayer()->ResetCameraValue();
		sTurningArea = map->mNearestTargetEntry->mRecordIndex;
		SetMapCameraModeFlags(map, 0x20004);
	}
}

extern "C" s32 CollectMapCameraAreaIntersections(CCameraActionMap* map, const Vec& from, const Vec& to, CamMapEntry* out, s32 type);

static inline f32 MapAreaHeight(s32 index)
{
	const CamAreaRecord& area = GetStageCameraAreaRecords()[index];
	const CamAreaEntry* entry = gCameraAreaState.mGroups[index].mEntries;
	f32 height                = gCameraAreaState.mCameraInfo[entry->mId].mEye[1] / 100.0f;
	return area.mCameraAction == -1 ? 0.23f * height : height;
}

/**
 * @note Address: 0x8013B3C8
 * @note Size: 0x738
 */
extern "C" void StartMapCameraAreaHeightInterpolation(CCameraActionMap* map)
{
	if (map->mMapArea >= 0) {
		const CamAreaRecord& area = gCameraAreaState.mAreaRecords[map->mMapArea];
		// flag 1: the area's height is absolute, otherwise it is above the player's camera target
		map->mHeightMode = (area.mFlags & 1) != 0;
		// flag 2: keep the area's height once no area is picked (mMapArea -1), otherwise fall back to 13
		map->mUseAreaHeight = (area.mFlags & 2) != 0;
		if (map->mHeightMode == 0) {
			f32 height         = MapAreaHeight(map->mMapArea);
			Vec target         = gGame.GetPlayer()->GetCameraTarget();
			map->mHeightInterp = &map->mHeightEaseInOut;
			map->mHeightInterp->Start(map->mEye.y - target.y - map->mEyeYOffset, height, 1.0f);
			if (map->mUseAreaHeight == 0) {
				map->mTargetHeight = 13.0f;
			}
			if (map->mUseAreaHeight == 1) {
				map->mTargetHeight = height;
			}
		}
		if (map->mHeightMode == 1) {
			f32 height            = MapAreaHeight(map->mMapArea);
			map->mAbsHeightInterp = &map->mAbsHeightEaseInOut;
			map->mAbsHeightInterp->Start(map->mEye.y - map->mEyeYOffset, height, 1.0f);
			if (map->mUseAreaHeight == 0) {
				map->mTargetHeight = 13.0f;
			}
			if (map->mUseAreaHeight == 1) {
				map->mTargetHeight = height;
			}
		}
	} else if (map->mMapArea == -1) {
		if (map->mHeightMode == 0 || (map->mHeightMode == 1 && map->mUseAreaHeight == 0)) {
			map->mHeightMode   = 0;
			Vec target         = gGame.GetPlayer()->GetCameraTarget();
			map->mHeightInterp = &map->mHeightEaseInOut;
			map->mHeightInterp->Start(map->mEye.y - target.y - map->mEyeYOffset, map->mTargetHeight, 1.0f);
		}
		if (map->mHeightMode == 1 && map->mUseAreaHeight == 1) {
			map->mAbsHeightInterp = &map->mAbsHeightEaseInOut;
			map->mAbsHeightInterp->Start(map->mEye.y - map->mEyeYOffset, map->mTargetHeight, 1.0f);
		}
	}
}

/**
 * @note Address: 0x8013BB00
 * @note Size: 0x1E4
 */
extern "C" s32 SelectMapCameraArea(CCameraActionMap* map)
{
	CVec target = map->mTarget->mMoveTranslate;
	CVec eye    = map->mEye;
	eye.y       = target.y;

	map->mAreaIntersectionCount = CollectMapCameraAreaIntersections(map, eye, target, map->mAreaIntersections, 1);

	s32 eyeArea = FindEnabledCameraAreaContainingPoint(&gCameraAreaState, eye);
	if (eyeArea >= 0 && gCameraAreaState.mAreaRecords[eyeArea].mCameraAction == CAMACTION_AREA) {
		eyeArea = -1;
	}

	s32 targetArea = FindEnabledCameraAreaContainingPoint(&gCameraAreaState, target);
	if (targetArea >= 0 && gCameraAreaState.mAreaRecords[targetArea].mCameraAction == CAMACTION_AREA) {
		targetArea = -1;
	}

	s32 crossedArea = map->mAreaIntersectionCount ? map->mAreaIntersections[0].mRecordIndex : -1;

	if (eyeArea >= 0) {
		map->mMapArea = eyeArea;
	} else if (targetArea >= 0) {
		map->mMapArea = targetArea;
	} else if (crossedArea >= 0) {
		map->mMapArea = crossedArea;
	} else {
		map->mMapArea = -1;
	}
	return map->mMapArea;
}

/**
 * @note Address: 0x8013BCE4
 * @note Size: 0x44
 */
extern "C" void RefreshMapCameraArea(CCameraActionMap* map)
{
	map->mPrevMapArea = map->mMapArea;
	SelectMapCameraArea(map);
	StartMapCameraAreaHeightInterpolation(map);
	map->mPrevMapArea = map->mMapArea;
}

/**
 * @note Address: 0x8013BD28
 * @note Size: 0x374
 */
extern "C" void GetMapCameraEyeBehindTarget(CCameraActionMap* map, Vec& out, s32 mode)
{
	XObj* target = map->mTarget;
	if (target == NULL) {
		out = map->mEye;
		return;
	}

	const f32 distance = map->mTargetRestDistance;
	if (mode != 1) {
		Mtx mtx;
		PSMTXCopy(HSD_JObjGetMtxPtr(target->mJObj), mtx);
		Vec eye = { 0.0f, 13.0f, -distance };
		PSMTXMultVec(mtx, &eye, &eye);
		eye.y = map->mHeightInterp->Get();
		out   = eye;
		return;
	}

	const bool useModel = (target->mStatus.mFlags & 1) && target->mJObj != NULL;
	Vec center;
	if (useModel) {
		Mtx mtx;
		PSMTXCopy(HSD_JObjGetMtxPtr(target->mJObj), mtx);
		center.x = 0.0f;
		center.y = 13.0f;
		center.z = -1.0f;
		PSMTXMultVec(mtx, &center, &center);
	}
	const CCamera& camera    = gGame.mMotionControlCamera.mCamera;
	const f32 angle          = HALF_PI + (f32)atan2(camera.mInterest.x - camera.mEye.x, camera.mInterest.z - camera.mEye.z);
	const f32 dx             = distance * (f32)cos(angle);
	const f32 dz             = distance * -(f32)sin(angle);
	const Vec targetPosition = target->GetCameraTarget();
	if (!useModel) {
		center = targetPosition;
	}
	out.x = center.x + dx;
	out.y = 13.0f + targetPosition.y;
	out.z = center.z + dz;
}
/**
 * @note Address: 0x8013C09C
 * @note Size: 0x138
 */
extern "C" BOOL GetMapCameraTargetOffsetPosition(CCameraActionMap* map, CVec& out)
{
	XObj* target = map->mTarget;
	if (target != NULL && (target->mStatus.mFlags & 1)) {
		Mtx mtx;
		PSMTXCopy(HSD_JObjGetMtxPtr(target->mJObj), mtx);
		CVec pos(0.0f, 13.0f, -1.0f);
		PSMTXMultVec(mtx, &pos, &pos);
		out.x = pos.x;
		out.y = pos.y;
		out.z = pos.z;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8013C1D4
 * @note Size: 0x2C
 */
void CCameraActionMap::DrawDebug3d()
{
	DrawMapCameraAreaDebug3D(&gCameraAreaState, 1);
}

/**
 * @note Address: 0x8013C200
 * @note Size: 0x1CC
 */
void CCameraActionMap::DrawDebug2d()
{
	CGame* game = &gGame;
	DrawGsStrokeTextFormat(&game->mGs, -1, "\\B000000A0AREA(%3d,---)(%3d)\n", 408.0f, 298.0f, 0.0f, 12.0f, 12.0f,
	                       gCameraAreaState.mActiveArea, gCameraAreaState.mCurrentArea);
	DrawGsStrokeTextFormat(&game->mGs, -1, "\\B000000A0ACMAP2\n", 408.0f, 310.0f, 0.0f, 12.0f, 12.0f);
	DrawGsStrokeTextFormat(&game->mGs, -1, "\\B00000080CAMSTAT\t   :%8X\nCAMSTATPRE :%8X\n", 360.0f, 322.0f, 0.0f, 12.0f, 12.0f,
	                       mStatus.mFlags, mPreviousStatus.mFlags);
	DrawGsStrokeTextFormat(&game->mGs, -1, "\\B00000080CHR_InsideCll(%3d,%3d,%3d)\n", 324.0f, 346.0f, 0.0f, 12.0f, 12.0f, mInsideAreas[0],
	                       mInsideAreas[1], mInsideAreas[2]);
	DrawGsStrokeTextFormat(&game->mGs, -1, "\\B00000080CAM_InsideCll(%3d,%3d,%3d)\n", 324.0f, 358.0f, 0.0f, 12.0f, 12.0f, mInsideCll0,
	                       mInsideCll1, mInsideCll2);
	if (mInsideCllCount > 0) {
		DrawGsStrokeTextFormat(&game->mGs, -1, "\\B00000080CamHitCll(%3d,%3d,%3d)\n", 324.0f, 370.0f, 0.0f, 12.0f, 12.0f, mInsideCll0,
		                       mInsideCll1, mInsideCll2);
	}
	if (mNearestTargetEntry != NULL) {
		DrawGsStrokeTextFormat(&game->mGs, -1, "\\B00000080INTRSCT(%3d-%3d)\n", 324.0f, 382.0f, 0.0f, 12.0f, 12.0f,
		                       mNearestTargetEntry->mRecordIndex, mNearestTargetEntry->mSide);
	}
}
/**
 * @note Address: 0x8013C3CC
 * @note Size: 0x144
 */
extern "C" BOOL TrySetMapCameraTargetYaw(CCameraActionMap* map, f32 yaw, f32 time, f32 unused)
{
	return SetMapYaw(map, yaw, time);
}

// eases the fov to the area's fov times one of four zoom steps. PreviousUpdate inlines it too
inline void SetFovStep(CCameraActionMap* map, s32 zoomLevel, f32 time)
{
	f32 fov = gCameraAreaState.mBaseFov;
	switch (zoomLevel) {
	case 0:
	default:
		map->mFovEaseOutDivide.Start(map->mFov, 0.8f * fov, time);
		break;
	case 1:
		map->mFovEaseOutDivide.Start(map->mFov, fov, time);
		break;
	case 2:
		map->mFovEaseOutDivide.Start(map->mFov, 1.5f * fov, time);
		break;
	case 3:
		map->mFovEaseOutDivide.Start(map->mFov, 2.0f * fov, time);
		break;
	}
	gCameraAreaState.mZoomLevel     = zoomLevel;
	gCameraAreaState.mPrevZoomLevel = zoomLevel;
}

/**
 * @note Address: 0x8013C510
 * @note Size: 0x104
 */
extern "C" void SetMapCameraFovStep(CCameraActionMap* map, s32 zoomLevel, f32 time)
{
	SetFovStep(map, zoomLevel, time);
}

extern "C" void UpdateMapCameraTurnInterpolation(CCameraActionMap* map, u32 step);

/**
 * @note Address: 0x8013C614
 * @note Size: 0x524
 */
extern "C" void UpdateMapCameraPadControls(CCameraActionMap* map, u32 step)
{
	if (gGame.mStatus.mFlags & 0xC00) {
		map->mPadControlEnabled = 0;
	} else {
		map->mPadControlEnabled = 1;
	}
	map->mManualInput = 0;
	CPad pad(0);
	u32 left    = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADLEFT : HSD_PAD_CSTICK_LEFT;
	u32 right   = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADRIGHT : HSD_PAD_CSTICK_RIGHT;
	u32 zoomIn  = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADUP : HSD_PAD_CSTICK_UP;
	u32 zoomOut = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADDOWN : HSD_PAD_CSTICK_DOWN;
	f32 turn    = 0.0f;
	if ((pad.mPad->mButton & right) && map->mPadControlEnabled) {
		turn = 1.0f;
	} else if ((pad.mPad->mButton & left) && map->mPadControlEnabled) {
		turn = -1.0f;
	}
	if (turn != 0.0f) {
		map->mManualInput = 1;
		SetMapCameraModeFlags(map, 0x20008);
	} else if (map->mStatus.mFlags & 8) {
		map->mStatus.mFlags &= ~8;
	}
	if (map->mTarget != NULL && map->mTarget->mJObj != NULL && (pad.mPad->mTrigger & PAD_TRIGGER_L) && map->mPadControlEnabled
	    && gGame.GetPlayer()->mHang.mState != 3) {
		if (SetMapYaw(map, map->mTarget->mMoveRotate.mTarget.y, 10.0f)) {
			gGame.GetPlayer()->ResetCameraValue();
			SetMapCameraModeFlags(map, 0x20002);
		}
	}
	UpdateMapCameraTurnInterpolation(map, step);
	s32 prevZoomLevel = gCameraAreaState.mPrevZoomLevel;
	s32 zoomLevel     = gCameraAreaState.mZoomLevel;
	if ((pad.mPad->mTrigger & zoomIn) && map->mPadControlEnabled) {
		if (zoomLevel > 1) {
			zoomLevel--;
		}
	} else if ((pad.mPad->mTrigger & zoomOut) && map->mPadControlEnabled) {
		if (zoomLevel < 2) {
			zoomLevel++;
		}
	}
	if (zoomLevel != prevZoomLevel) {
		SetFovStep(map, zoomLevel, 15.0f);
	}
	if (!map->mFovEaseOutDivide.Update(step, 0.0f)) {
		map->SetFov(map->mFovEaseOutDivide.Get());
	}
}
static inline void RefreshOrbitPosition(CCameraActionMap* map, u32 step, bool manual)
{
	f32 yaw    = HALF_PI + map->mAngle.y;
	f32 radius = FlatDistance(map->mTargetEye, map->mInterest);
	f32 pitch  = 0.0f;
	f32 x      = map->mInterest.x + radius * (f32)cos(yaw) * (f32)cos(pitch);
	f32 z      = map->mInterest.z + radius * -(f32)sin(yaw) * (f32)cos(pitch);
	// The manual turn uses the frame delta left in f31 by the retail caller.
	f32 height      = manual ? 0.1f * step : map->mTargetEye.y;
	map->mOrbitEye  = CVec(x, height, z);
	map->mTargetEye = CVec(x, height, z);
}

// The original rounds each axis increment before adding it to the angle.
static inline void AdvanceOrbitAngles(const CVec& angle, u32 step, f32 pitchStep, f32 yawStep, f32& pitch, f32& heading)
{
	pitch   = angle.x + step * pitchStep;
	heading = angle.y + step * yawStep;
}

/**
 * @note Address: 0x8013CB38
 * @note Size: 0x7A4
 */
extern "C" void UpdateMapCameraOrbit(CCameraActionMap* map, u32 step)
{
	CPad pad(0);
	u32 left               = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADLEFT : HSD_PAD_CSTICK_LEFT;
	u32 right              = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADRIGHT : HSD_PAD_CSTICK_RIGHT;
	sPreviousMapOrbitAngle = map->mOrbitAngle;
	map->mOrbitEye         = map->mTargetEye;
	if (!sOrbitTurnBlocked)
		map->mOrbitAngle = map->mAngle;

	{
		CVec angle;
		f32 turn = 0.0f;
		if ((pad.mPad->mButton & right) && map->mPadControlEnabled)
			turn = 1.0f;
		else if ((pad.mPad->mButton & left) && map->mPadControlEnabled)
			turn = -1.0f;
		if (turn != 0.0f) {
			sOrbitTurnDirection = turn;
			angle               = map->mOrbitAngle;
			f32 pitch, heading;
			AdvanceOrbitAngles(angle, step, 0.0f, (3.5f * DEG2RAD) * turn, pitch, heading);
			if (pitch > 89.1f * DEG2RAD)
				pitch = 89.1f * DEG2RAD;
			if (pitch < -89.1f * DEG2RAD)
				pitch = -89.1f * DEG2RAD;
			map->SetAngle(pitch, heading, angle.z);
			map->mPrevOrbitAngle = map->mOrbitAngle;
			map->mOrbitAngle     = map->mAngle;
			map->SetInterest(GetMapCameraTargetWithHeightOffset(map));
			RefreshOrbitPosition(map, step, true);
		}
	}

	if (map->mStatus.mFlags & 2) {
		map->mOrbitEye = map->mTargetEye;
		if (!sOrbitTurnBlocked)
			map->mOrbitAngle = map->mAngle;
		f32 yaw    = WrapAngle(map->mOrbitAngle.y);
		f32 target = WrapAngle(map->mTargetYaw);
		f32 diff   = target - yaw;
		if (diff < 0.0f)
			diff = -diff;
		if (diff > 0.0f) {
			if (diff >= TWO_PI)
				diff -= TWO_PI;
			if (diff > PI)
				target = target < yaw ? target + TWO_PI : target - TWO_PI;
		}
		if (yaw < target) {
			sOrbitTurnDirection = 1.0f;
			yaw                 = map->mTurnSpeed * step + yaw;
			if (yaw > target)
				yaw = target;
		} else if (yaw > target) {
			yaw = yaw - map->mTurnSpeed * step;
			if (yaw < target)
				yaw = target;
			sOrbitTurnDirection = -1.0f;
		}
		diff = target - yaw;
		if (diff < 0.0f)
			diff = -diff;
		if (diff < 0.001f)
			yaw = target;
		BOOL turning = TRUE;
		Vec angle    = map->mOrbitAngle;
		angle.y      = yaw;
		map->SetAngle(angle);
		map->mPrevOrbitAngle = map->mOrbitAngle;
		map->mOrbitAngle     = map->mAngle;
		map->SetInterest(GetMapCameraTargetWithHeightOffset(map));
		RefreshOrbitPosition(map, step, false);
		map->mManualInput = 1;

		if (yaw == target)
			turning = FALSE;
		if (!turning)
			map->mStatus.mFlags &= ~2;
	}
}

// picks the two corners of mNearestEyeEntry's side, ordered by which side chibi robo is on
/**
 * @note Address: 0x8013D2DC
 * @note Size: 0x250
 */
extern "C" BOOL SetMapCameraSideEndpoints(CCameraActionMap* map)
{
	CamMapEntry* entry = map->mNearestEyeEntry;
	if (entry != NULL) {
		s32 group          = entry->mRecordIndex;
		s32 side           = entry->mSide;
		CVec dir           = entry->mNormal;
		CamAreaRecord* rec = &gCameraAreaState.mAreaExRecords[group];
		CVec toPlayer      = gGame.GetPlayer()->mMoveTranslate;
		toPlayer.y         = 0.0f;
		Vec eye            = map->mEye;
		eye.y              = 0.0f;
		PSVECSubtract(&toPlayer, &eye, &toPlayer);
		PSVECNormalize(&toPlayer, &toPlayer);
		C_VECCrossProduct(&dir, &toPlayer, &dir);
		CVec cross = dir;

		const s32* firstCorner = rec->_04[side];
		const s32* lastCorner  = rec->_04[(side + 1) % 4];
		f32 x0                 = firstCorner[0] / 100.0f;
		f32 z0                 = firstCorner[1] / 100.0f;
		f32 x1                 = lastCorner[0] / 100.0f;
		f32 z1                 = lastCorner[1] / 100.0f;

		if (cross.y > 0.0f) {
			map->mSideStart.x = x0;
			map->mSideStart.y = 0.0f;
			map->mSideStart.z = z0;
			map->mSideEnd.x   = x1;
			map->mSideEnd.y   = 0.0f;
			map->mSideEnd.z   = z1;
			map->mSideSign    = 1;
		}
		if (cross.y < 0.0f) {
			map->mSideStart.x = x1;
			map->mSideStart.y = 0.0f;
			map->mSideStart.z = z1;
			map->mSideEnd.x   = x0;
			map->mSideEnd.y   = 0.0f;
			map->mSideEnd.z   = z0;
			map->mSideSign    = -1;
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8013D52C
 * @note Size: 0xD0
 */
extern "C" s32 CountMapCameraAdjacentSideIntersections(CCameraActionMap* map, CamMapEntry* entry)
{
	// the two neighbours of each of the 4 sides, wraps around
	s32 sides[4][2] = {
		{ 3, 1 },
		{ 0, 2 },
		{ 1, 3 },
		{ 2, 0 },
	};
	s32 num   = map->mIntersectionCount;
	s32 group = entry->mRecordIndex;
	s32 side  = entry->mSide;
	s32 count = 0;
	if (num > 0) {
		for (s32 i = 0; i < num; i++) {
			if (map->mIntersectionEntries[i].mRecordIndex == group) {
				for (s32 j = 0; j < 2; j++) {
					if (map->mIntersectionEntries[i].mSide == sides[side][j]) {
						count++;
					}
				}
			}
		}
	}
	return count;
}

/**
 * @note Address: 0x8013D5FC
 * @note Size: 0x38
 */
extern "C" s32 CountMapCameraEntriesInGroup(CCameraActionMap* map, s32 recordIndex)
{
	s32 count = 0;
	if (map->mIntersectionCount > 0) {
		for (s32 i = 0; i < map->mIntersectionCount; i++) {
			if (map->mIntersectionEntries[i].mRecordIndex == recordIndex) {
				count++;
			}
		}
	}
	return count;
}

/**
 * @note Address: 0x8013D634
 * @note Size: 0xDC
 */
// FindNearestMapCameraEntry
// index of the entry nearest to pos, -1 if there's no choice to make
extern "C" s32 FindNearestMapCameraEntry(CCameraActionMap* map, const Vec& pos)
{
	if (map->mIntersectionCount > 1) {
		CVec p      = pos;
		s32 nearest = -1;
		f32 minDist = 10000.0f;
		for (s32 i = 0; i < map->mIntersectionCount; i++) {
			f32 dist = PSVECDistance(&p, &map->mIntersectionEntries[i].mPos);
			if (dist < minDist) {
				minDist = dist;
				nearest = i;
			}
		}
		return nearest;
	}
	return -1;
}

/**
 * @note Address: 0x8013D710
 * @note Size: 0xF4
 */
// GetNearestMapCameraEntry
extern "C" CamMapEntry* GetNearestMapCameraEntry(CCameraActionMap* map, const Vec& pos)
{
	s32 idx = FindNearestMapCameraEntry(map, pos);
	if (idx == -1) {
		return NULL;
	}
	return &map->mIntersectionEntries[idx];
}

/**
 * @note Address: 0x8013D804
 * @note Size: 0x468
 */
extern "C" s32 CollectMapCameraAreaIntersections(CCameraActionMap* map, const Vec& from, const Vec& to, CamMapEntry* out, s32 type)
{
	if (out == NULL) {
		return -1;
	}
	memset(out, 0, 32 * sizeof(CamMapEntry));
	CamAreaRecord* record;
	s32 numRecords;
	if (type == 0) {
		record     = gCameraAreaState.mAreaExRecords;
		numRecords = gCameraAreaState.mAreaExCount;
	}
	if (type == 1) {
		record     = gCameraAreaState.mAreaRecords;
		numRecords = gCameraAreaState.mAreaCount;
	}
	CamMapEntry* entry = out;
	s32 count          = 0;
	for (s32 i = 0; i < numRecords; i++) {
		if (CamAreaInfo(record).IsCameraOff()) {
			record++;
			continue;
		}
		if (CamAreaInfo(record).UsesAreaCamera()) {
			record++;
			continue;
		}
		f32 top    = CamAreaInfo(record).GetTop();
		f32 bottom = CamAreaInfo(record).GetBottom();
		if (ClassifyAreaHeight(from.y, bottom, top) < 0 && ClassifyAreaHeight(to.y, bottom, top) < 0) {
			record++;
			continue;
		}
		const s32(*corner)[2] = record->_04;
		for (s32 side = 0; side < 4; side++) {
			Vec a, b;
			a.x = (*corner)[0] / 100.0f;
			a.y = 0.0f;
			a.z = (*corner)[1] / 100.0f;
			++corner;
			if (side == 3) {
				corner = record->_04;
			}
			b.x       = (*corner)[0] / 100.0f;
			b.y       = 0.0f;
			b.z       = (*corner)[1] / 100.0f;
			Vec end   = to;
			Vec start = from;
			Vec hit;
			if (IntersectLinesXZ(&a, &b, &end, &start, &hit, 1)) {
				Vec down = { 0.0f, -1.0f, 0.0f };
				f32 nx, ny, nz;
				{
					CVec normal;
					Vec edge;
					edge.x = a.x - b.x;
					edge.z = a.z - b.z;
					edge.y = 0.0f;
					Vec downCopy;
					downCopy.x = down.x;
					downCopy.y = down.y;
					downCopy.z = down.z;
					C_VECCrossProduct(&downCopy, &edge, &normal);
					PSVECNormalize(&normal, &normal);
					nx = normal.x;
					ny = normal.y;
					nz = normal.z;
				}
				count++;
				entry->mPos         = hit;
				entry->mSide        = side;
				entry->mRecordIndex = i;
				entry->mNormal.x    = nx;
				entry->mNormal.y    = ny;
				entry->mNormal.z    = nz;
				entry++;
			}
		}
		record++;
	}
	return count;
}
/**
 * @note Address: 0x8013DC6C
 * @note Size: 0x600
 */
s32 CCameraActionMap::CalcIntersectVec(const Vec& from, const Vec& to, CamMapEntry* out, s32 type)
{
	if (out == NULL || (type != 0 && type != 1)) {
		return -1;
	}
	memset(out, 0, 32 * sizeof(CamMapEntry));
	CamAreaRecord* record;
	s32 numRecords;
	if (type == 0) {
		record     = gCameraAreaState.mAreaExRecords;
		numRecords = gCameraAreaState.mAreaExCount;
	} else {
		record     = gCameraAreaState.mAreaRecords;
		numRecords = gCameraAreaState.mAreaCount;
	}
	CamMapEntry* entry = out;
	s32 count          = 0;
	for (s32 i = 0; i < numRecords; i++) {
		u32 disabled = CamAreaInfo(record).IsDisabled();
		if (disabled != 0) {
			record++;
			continue;
		}
		f32 top    = CamAreaInfo(record).GetTop();
		f32 bottom = CamAreaInfo(record).GetBottom();
		if (ClassifyAreaHeight(from.y, bottom, top) < 0 && ClassifyAreaHeight(to.y, bottom, top) < 0) {
			record++;
			continue;
		}
		// while a turn runs, skip the area StartMapCameraTurnInterpolation stored in sTurningArea
		if (mTurnInterpolation != NULL && i == sTurningArea) {
			record++;
			continue;
		}
		for (s32 side = 0; side < 4; side++) {
			const s32* startCorner = record->_04[side];
			const s32* endCorner   = record->_04[(side + 1) % 4];
			Vec a                  = { startCorner[0] / 100.0f, 0.0f, startCorner[1] / 100.0f };
			Vec b                  = { endCorner[0] / 100.0f, 0.0f, endCorner[1] / 100.0f };
			Vec hit;
			s32 intersects = IntersectLinesXZ(&a, &b, &to, &from, &hit, 1);
			if (intersects && (__fpclassifyf(hit.x) == FP_NAN || __fpclassifyf(hit.y) == FP_NAN || __fpclassifyf(hit.z) == FP_NAN)) {
				OSPanic("ccameraactionmap2.cpp", 2183, "%s%s",
				        FormatToStaticBuffer(
				            "CCameraActionMap ... GetCrossPoint\np1(%f, %f, %f) p2(%f, %f, %f)\nvs(%f, %f, %f) ve(%f, %f, %f)\n(1)\n", a.x,
				            a.y, a.z, b.x, b.y, b.z, to.x, to.y, to.z, from.x, from.y, from.z),
				        "CalcIntersectVec");
			}
			if (intersects) {
				const Vec down = { 0.0f, -1.0f, 0.0f };
				const Vec edge = { a.x - b.x, 0.0f, a.z - b.z };
				Vec normal;
				C_VECCrossProduct(&down, &edge, &normal);
				PSVECNormalize(&normal, &normal);
				count++;
				entry->mPos         = hit;
				entry->mSide        = side;
				entry->mRecordIndex = i;
				entry->mNormal      = normal;
				entry++;
			}
		}
		record++;
	}
	return count;
}
/**
 * @note Address: 0x8013E26C
 * @note Size: 0x17CC
 */
// ResolveMapCameraEyeCollision
/**
 * @note Address: 0x8013FA38
 * @note Size: 0x1574
 */
// Update__16CCameraActionMapFUl
/**
 * @note Address: 0x80140FAC
 * @note Size: 0x40
 */
extern "C" void SetMapCameraPreviousInterest(CCameraActionMap* map, const Vec& v)
{
	map->mPreviousInterest = v;
}

/**
 * @note Address: 0x80140FEC
 * @note Size: 0x40
 */
extern "C" void SetMapCameraPreviousEye(CCameraActionMap* map, const Vec& v)
{
	map->mPreviousEye = v;
}

/**
 * @note Address: 0x8014102C
 * @note Size: 0x40
 */
extern "C" void ResetPlayerCameraValue(XObjChr* player)
{
	player->ResetCameraValue();
}

/**
 * @note Address: 0x8014106C
 * @note Size: 0x18
 */
// pool 2's objects, the player's pool
extern "C" XObj* GetPlayerPoolObject(CGame* game, s32 index)
{
	return game->mXObjs.mPools[2].Get(index);
}

/**
 * @note Address: 0x80141084
 * @note Size: 0x1F8
 */
// vtable slot _0C, PreviousUpdate
void CCameraActionMap::PreviousUpdate()
{
	UpdateCameraAreaIndexAndBaseFov(&gCameraAreaState, 0);
	s32 activeArea = gCameraAreaState.mActiveArea;
	// the player stepped into another camera area
	if (gCameraAreaState.mCurrentArea != gCameraAreaState.mPrevArea && gCameraAreaState.mCurrentArea != -1) {
		if (GetCameraAreaMode(&gCameraAreaState) == CAMACTION_AREA) {
			gGame.mMotionControlCamera.ChangeAction(CAMACTION_AREA);
			gCameraAreaState.mPrevZoomLevel = gCameraAreaState.mZoomLevel;
			SelectAreaCameraEntryByViewDirection(static_cast<CCameraActionArea*>(gGame.mCameraActions.mTable[CAMACTION_AREA]), 1, 2.0f,
			                                     activeArea);
			return;
		}
		// otherwise the mode is CAMACTION_MAP (GetCameraAreaMode returns 1 or 7) and this action stays
		SetFovStep(this, gCameraAreaState.mZoomLevel, 15.0f);
		SelectAreaCameraEntryWithControl(static_cast<CCameraActionArea*>(gGame.mCameraActions.mTable[CAMACTION_AREA]), activeArea, 0, 3, 1,
		                                 1.0f);
	}
	mPrevMapArea = mMapArea;
	SelectMapCameraArea(this);
	if (mMapArea != mPrevMapArea) {
		StartMapCameraAreaHeightInterpolation(this);
	}
}
/**
 * @note Address: 0x8014127C
 * @note Size: 0xC
 */
extern "C" void SetMapCameraKeepPreviousEye(CCameraActionMap* map)
{
	map->mKeepPreviousEye = TRUE;
}

/**
 * @note Address: 0x80141288
 * @note Size: 0x4CC
 */
void CCameraActionMap::SetCamera(CCamera* camera)
{
	// CMotionControlCamera::Interpolate takes the interest's height at once and follows it in on x and z
	mInterestMode = 3;
	SetEye(camera->mEye);
	mPreviousEye = CVec(camera->mEye.x, camera->mEye.y, camera->mEye.z);
	SetInterest(camera->mInterest);
	SetDistance(camera->mDistance);
	SetFov(camera->mFov);
	SetAngle(camera->mAngle);
	mTarget = gGame.GetPlayer();
	if (mTarget != NULL) {
		SetInterest(mTarget->GetCameraTarget());
	}
	mTargetHeightOffsetInterp = NULL;
	mTargetHeightOffsetEase.Start(0.0f, 0.0f, 0.1f);
	if (mTarget != NULL) {
		Mtx mtx;
		PSMTXCopy(HSD_JObjGetMtxPtr(mTarget->mJObj), mtx);
		// start the spring at rest on the point Update tracks (GetMapCameraTargetOffsetPosition)
		CVec pos(0.0f, 13.0f, -1.0f);
		PSMTXMultVec(mtx, &pos, &pos);
		mSpringTarget       = CVec(pos.x, pos.y, pos.z);
		mSpringPrevTarget.x = mSpringTarget.x;
		mSpringPrevTarget.y = mSpringTarget.y;
		mSpringPrevTarget.z = mSpringTarget.z;
	}
	_8C.x               = 0.0f;
	_8C.y               = 0.0f;
	_8C.z               = 0.0f;
	_8C.mTarget.x       = 0.0f;
	_8C.mTarget.y       = 0.0f;
	_8C.mTarget.z       = 0.0f;
	_8C.mSpeed.x        = 0.0f;
	_8C.mSpeed.y        = 0.0f;
	_8C.mSpeed.z        = 0.0f;
	mTargetRestDistance = 60.0f;
	mRestDistance       = mTargetRestDistance;
	mFlatDistance       = mTargetRestDistance;
	SetFovStep(this, gCameraAreaState.mZoomLevel, 15.0f);
	mPadControlEnabled = 1;
	_DB0               = 1;
	mTurnInterpolation = NULL;
	mHeightInterp      = &mHeightEaseInOut;
	mHeightInterp->Start(13.0f, 13.0f, 1.0f);
	mMapArea         = -1;
	mPrevMapArea     = -1;
	mAbsHeightInterp = &mAbsHeightEaseInOut;
	mAbsHeightInterp->Start(mEye.y, mEye.y, 1.0f);
	mHeightMode    = 0;
	mUseAreaHeight = 0;
	mTargetHeight  = 13.0f;
	mTurnBlocked   = 1;
	ResetMapCameraTargetEye(this);
	mStatus.mFlags = 0;
	// mode bit 0x10000 (of SetMapCameraModeFlags's 0x70000): Update springs the eye after mSpringTarget
	mStatus.mFlags |= 0x10000;
	sPlayerMovingTowardCamera = 0;
	mEyeYOffsetInterp         = &mEyeYOffsetEaseInOut;
	mEyeYOffsetEaseInOut.Start(0.0f, 0.0f, 1.0f);
	mEyeYOffset       = 0.0f;
	mPreviousInterest = CVec(mInterest.x, mInterest.y, mInterest.z);
	// set by SetMapCameraKeepPreviousEye when memcardpos restores a saved camera: mPreviousEye then keeps camera->mEye.
	if (!mKeepPreviousEye) {
		mPreviousEye = CVec(mInterest.x, mInterest.y, mInterest.z);
	}
}
/**
 * @note Address: 0x80141754
 * @note Size: 0x4FC
 */
void CCameraActionMap::Reset2()
{
	SetInterest(mTarget->GetCameraTarget());
	Vec eye;
	GetMapCameraEyeBehindTarget(this, eye, 0);
	SetEye(eye);
	SetMapCameraAnglesFromEyeAndInterest(this);
	if (__fpclassifyf(mAngle.x) == FP_NAN || __fpclassifyf(mAngle.y) == FP_NAN || __fpclassifyf(mAngle.z) == FP_NAN) {
		OSPanic("ccameraactionmap2.cpp", 251, "%s%s",
		        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER OP(%d)HANG(%d(%d))TGTEYE(%f, "
		                             "%f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, "
		                             "%f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
		                             mEye.x, mEye.y, mEye.z, mAngle.x, mAngle.y, mAngle.z, mDistance, mStatus.mFlags,
		                             mPreviousStatus.mFlags, mManualInput, sPlayerHangState, sPreviousPlayerHangState, mTargetEye.x,
		                             mTargetEye.y, mTargetEye.z, mPrevTargetEye.x, mPrevTargetEye.y, mPrevTargetEye.z, mEyeInterp,
		                             mTurnInterpolation, mOrbitEye.x, mOrbitEye.y, mOrbitEye.z, mOrbitAngle.x, mOrbitAngle.y, mOrbitAngle.z,
		                             mPrevOrbitAngle.x, mPrevOrbitAngle.y, mPrevOrbitAngle.z, 0),
		        "Reset2");
	}
}
/**
 * @note Address: 0x80141C50
 * @note Size: 0x178
 */
// Reset__16CCameraActionMapFv

// the original's panic names its function Reset through __FUNCTION__
void CCameraActionMap::Reset()
{
	OSPanic(__FILE__, 220, "%s%s", "actionMap::reset\n", "Reset");
	// OSPanic ends in PPCHalt, so nothing below ever runs

	mTarget = gGame.GetPlayer();
	if (mTarget != NULL) {
		Mtx mtx;
		PSMTXCopy(HSD_JObjGetMtxPtr(mTarget->mJObj), mtx);
		CVec eye(0.0f, 13.0f, -mRestDistance);
		PSMTXMultVec(mtx, &eye, &eye);
		SetEye(eye);
		SetInterest(mTarget->GetCameraTarget());
	}
	mPadControlEnabled = 1;
}

/**
 * @note Address: 0x80141DC8
 * @note Size: 0x4C
 */
void CCameraActionMap::Init()
{
	mSpringDamping      = 0.5f;
	mRestDistance       = 60.0f;
	mTargetRestDistance = 60.0f;
	mFlatDistance       = 60.0f;
	_C4                 = 18.0f;
	InitCameraAreaEntries(&gCameraAreaState);
}

/**
 * @note Address: 0x80141E14
 * @note Size: 0x518
 */
extern "C" CVec SpringDamp(CVec& position, const Vec& target, const CVec& previous, f32 step, f32 stiffness, f32 damping, f32 restDistance)
{
	CVec displacement;
	CVec velocity;
	displacement = position - target;
	f32 distance = sqrtf(displacement.z * displacement.z + (displacement.x * displacement.x + displacement.y * displacement.y));
	if (__fpclassifyf(distance) == FP_NAN) {
		OSPanic("ccameraactionmap2.cpp", 167, "%s%s",
		        FormatToStaticBuffer("CCameraActionMap ... disp(%f, %f, %f)\n", displacement.x, displacement.y, displacement.z),
		        "SpringDamp");
	}
	velocity  = (previous - target) * step;
	f32 force = stiffness * (restDistance - distance) + damping * (PSVECDotProduct(&displacement, &velocity) / distance);
	PSVECNormalize(&displacement, &displacement);
	PSVECScale(&displacement, &displacement, force * step);
	Vec delta = displacement;
	position += delta;
	if (__fpclassifyf(position.x) == FP_NAN || __fpclassifyf(position.y) == FP_NAN || __fpclassifyf(position.z) == FP_NAN) {
		OSPanic("ccameraactionmap2.cpp", 181,
		        "CCameraActionMap ... disp(%f, %f, %f), distance %f, force %f, velocity(%f, %f, %f)\nSpringDamp", displacement.x,
		        displacement.y, displacement.z, distance, force, velocity.x, velocity.y, velocity.z);
	}
	return position;
}
/**
 * @note Address: 0x80139E40
 * @note Size: 0x8
 */
u32 CCameraActionMap::GetCameraFlags()
{
	return CAMFLAG_EYE | CAMFLAG_INTEREST | CAMFLAG_FOV;
}

/**
 * @note Address: 0x80139E48
 * @note Size: 0x8
 */
u32 CCameraActionMap::GetInterestMode()
{
	return mInterestMode;
}

/**
 * @note Address: 0x80139E50
 * @note Size: 0x8
 */
f32 CCameraActionMap::GetFollowDivisor()
{
	// CMotionControlCamera::Follow closes 1/6 of the remaining distance per call
	return 6.0f;
}

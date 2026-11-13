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

extern "C" CVec sMapCameraTestEye;
extern "C" CVec sMapCameraTestCenter;
extern "C" CamCollisionPolygon gCameraCollisionPolygons[256];
extern "C" s32 CalcIntersectVec__16CCameraActionMapFRC3VecRC3VecP11CamMapEntryl(CCameraActionMap* map, const Vec& from, const Vec& to,
                                                                                CamMapEntry* entries, s32 arg);
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
extern "C" s32 CountMapCameraEntriesInGroup(CCameraActionMap* map, s32 value);
extern "C" s32 FindNearestMapCameraEntry(CCameraActionMap* map, const Vec& pos);
extern "C" CamMapEntry* GetNearestMapCameraEntry(CCameraActionMap* map, const Vec& pos);

extern "C" {
s32 lbl_8065B330;
s32 lbl_8065B334;
s32 lbl_8065B338;
s32 lbl_8065B33C;
s32 lbl_8065B340;
s32 lbl_8065B344;
s32 lbl_8065B348;
f32 lbl_8065B34C;
s32 lbl_8065B350;
f32 lbl_8065B354;
f32 lbl_8065B358;
}

extern "C" XObj* GetPlayerPoolObject(CGame* game, s32 index);
extern "C" void ResetPlayerCameraValue(XObjChr* player);
extern "C" void SetMapCameraPreviousEye(CCameraActionMap* map, const Vec& v);
extern "C" void SetMapCameraPreviousInterest(CCameraActionMap* map, const Vec& v);
extern "C" void UpdateMapCameraOrbit(CCameraActionMap* map, u32 step);
extern "C" BOOL ResolveMapCameraEyeCollision(CCameraActionMap* map, CVec& eye, const CVec& previous, const CVec& interest,
                                             const CVec& previousInterest, const s32* mode);
extern "C" CVec SpringDamp(CVec& position, const Vec& target, const CVec& previous, f32 step, f32 stiffness, f32 damping, f32 restDistance);
// out of line vector helpers
extern "C" void CopyVecToCVec(CVec& dst, const Vec& src);
extern "C" CVec __ct__4CVecFRC3Vec(const Vec& src);
extern "C" CVec __ct__4CVecFfff(f32 x, f32 y, f32 z);
extern "C" void SetVectorComponents(CVec* v, f32 x, f32 y, f32 z);
extern "C" f32 GetHorizontalDistance(const Vec* from, const Vec* to);      // horizontal distance
extern "C" f32 GetHeadingBetweenPositions(const Vec* from, const Vec* to); // horizontal angle
extern "C" s32 ClassifyFloatBits(f32 value);                               // floating-point classification
extern "C" {
f32 sMapCameraSpringParameters[] = { 0.5f, 2.5f, 4.5f };
s32 sMapCameraCollisionMode0[1];
s32 sMapCameraCollisionMode1 = 1;
}

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

static inline BOOL CameraPolygonContains(const Vec& pos, CamCollisionPolygon& polygon)
{
	return IsPointInsideQuadXZ((Vec*)&pos, &polygon.point[0], &polygon.point[1], &polygon.point[2], &polygon.point[3]);
}

static inline BOOL CameraEdgeIntersect(const Vec& from, const Vec& to, Vec& a, Vec& b, Vec& hit, s32 segment)
{
	return IntersectLinesXZ(&from, &to, &a, &b, &hit, segment);
}

/**
 * @note Address: 0x8013E26C
 * @note Size: 0x17CC
 */
extern "C" BOOL ResolveMapCameraEyeCollision(CCameraActionMap* map, CVec& eye, const CVec& previous, const CVec& interest,
                                             const CVec& previousInterest, const s32* mode)
{
	CVec target((const Vec&)interest);
	CVec old((const Vec&)previous);
	CVec candidate((const Vec&)eye);
	if (!(target != (Vec)old))
		return false;

	CamAreaRecord* records        = gCameraAreaState._1C;
	s32 count                     = gCameraAreaState._20;
	CamCollisionPolygon* polygons = gCameraCollisionPolygons;
	BOOL changed                  = false;
	BOOL targetInside             = false;
	BOOL previousInside           = false;
	BOOL candidateInside          = false;
	for (s32 i = 0; i < count; i++) {
		CamAreaRecord* record        = &records[i];
		CamCollisionPolygon& polygon = polygons[i];
		CamAreaRecord* area          = CamAreaInfo(record).GetRecord();
		if (area->_2E & 0x80)
			continue;
		f32 top      = area->GetTop();
		f32 bottom   = area->GetBottom();
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
			CamAreaRecord* area          = CamAreaInfo(&records[i]).GetRecord();
			if (area->_2E & 0x80)
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
					CameraEdgeIntersect(candidate, candidate + (Vec)normal, a, b, hit, 0);
					f32 distance = PSVECDistance(&candidate, &hit);
					if (__fpclassifyf(closest) == FP_NAN || closest > distance) {
						closest = distance;
						best.Set(hit + (Vec)(normal * 2.0f));
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
				CamAreaRecord* area          = CamAreaInfo(&records[i]).GetRecord();
				if (area->_2E & 0x80)
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
					CamAreaRecord* area          = CamAreaInfo(&records[i]).GetRecord();
					if (area->_2E & 0x80)
						continue;
					f32 top      = area->GetTop();
					f32 bottom   = area->GetBottom();
					bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
					if (outside)
						continue;
					CVec from = targetInside ? (CVec&)map->mTarget->mMoveTranslate : target;
					CVec normal, hit;
					for (s32 side = 0; side < 4; side++) {
						Vec& a = polygon.point[side];
						Vec& b = polygon.point[side + 1];
						if (CameraEdgeIntersect(from, old, a, b, hit, 1)) {
							GetOutwardPerpendicularToEdge(&old, &a, &b, &normal);
							normal.y = 0.0f;
							PSVECNormalize(&normal, &normal);
							CameraEdgeIntersect(old, old + (Vec)normal, a, b, hit, 0);
							f32 distance = PSVECDistance(&target, &hit);
							if (__fpclassifyf(closest) == FP_NAN || closest > distance) {
								closest = distance;
								best.Set(hit + (Vec)(normal * 2.0f));
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
			CamAreaRecord* area          = CamAreaInfo(&records[i]).GetRecord();
			if (area->_2E & 0x80)
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
								Vec a, b; // retail works these out and drops them
								GetOutwardPerpendicularToEdge(&target, &polygon.point[nearestSide], &polygon.point[nearestSide + 1], &a);
								GetOutwardPerpendicularToEdge(&nearest, &polygon.point[nearestSide], &polygon.point[nearestSide + 1], &b);
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
					if (CameraEdgeIntersect(old, old + (Vec)normal, a, b, hit, 1)) {
						normal.y = 0.0f;
						PSVECNormalize(&normal, &normal);
						candidate.Set(hit - (Vec)(normal * 2.0f));
						candidate.y = eye.y;
					} else
						candidate.Set(old);
					ResetMapCameraInterpolation(map);
					changed = true;
				} else {
					CVec direction, hit;
					GetOutwardPerpendicularToEdge(&candidate, &a, &b, &direction);
					if (CameraEdgeIntersect(candidate, candidate + (Vec)direction, a, b, hit, 1)) {
						direction.y = 0.0f;
						PSVECNormalize(&direction, &direction);
						candidate.Set(hit - (Vec)(direction * 2.0f));
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
			CamAreaRecord* area = CamAreaInfo(&records[i]).GetRecord();
			if (area->_2E & 0x80)
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

	Vec pos = mTarget->mMoveTranslate;
	_EA4    = 0;
	for (u32 i = 0; i < 32; i++)
		_EA8[i] = -2;
	pos.y += 10.0f;
	_EA4   = CollectCameraAreasContainingPoint(&gCameraAreaState, pos, _EA8);
	f32 dt = 0.1f * step;
	CopyVecToCVec(_60, _70);
	GetMapCameraTargetOffsetPosition(this, _70);
	UpdateMapCameraPadControls(this, step);
	if ((mStatus.mFlags & 0x20000) && !(mStatus.mFlags & 0x3E) && _70 != _60) {
		SetMapCameraModeFlags(this, 0x10000);
	}

	// retail steps these twice
	for (s32 i = 0; i < 2; i++) {
		if (_E58 == 0) {
			if (_E30 != NULL)
				_E30->Update(step, FRAME_TIME);
			f32 height = _E30->Get();
			_E60       = height + GetPlayerPoolObject(&gGame, 0)->GetCameraTarget().y;
		}
		if (_E58 == 1) {
			if (_E50 != NULL)
				_E50->Update(step, FRAME_TIME);
			_E60 = _E50->Get();
		}
	}
	if (_E64 != NULL)
		_E64->Update(step, FRAME_TIME);
	if (_E78 != NULL) {
		if (!_E78->Update(step, FRAME_TIME))
			_E98 = _E78->Get();
	} else {
		_E78 = NULL;
	}

	if (mStatus.mFlags & 0x10000) {
		{
			CVec eye(mEye);
			f32 previousDistance = PSVECDistance(&eye, &_60);
			f32 distance         = PSVECDistance(&eye, &_70);
			f32 rest             = _58;
			f32 targetRest       = _80;
			if (rest != targetRest && distance > previousDistance) {
				if (distance < targetRest && rest != targetRest)
					rest = distance;
				if (distance >= targetRest)
					rest = targetRest;
			} else if (distance < previousDistance) {
				rest = targetRest;
			}
			_58 = rest;
		}
		CVec eye;
		CopyVecToCVec(eye, mEye);
		CopyVecToCVec(eye, SpringDamp(eye, _70, _60, dt, sMapCameraSpringParameters[0], _5C, _58));
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
			                             mPreviousStatus.mFlags, _EA0, lbl_8065B340, lbl_8065B33C, _F68.x, _F68.y, _F68.z, _F78.x, _F78.y,
			                             _F78.z, _F28, mTurnInterpolation, _F88.x, _F88.y, _F88.z, _F98.x, _F98.y, _F98.z, _FB8.x, _FB8.y,
			                             _FB8.z, lbl_8065B338),
			        "Update");
		}

		Vec movement;
		PSVECSubtract(&_70, &_60, &movement);
		movement.y = 0.0f;
		if (movement.x != 0.0f || movement.z != 0.0f) {
			PSVECNormalize(&movement, &movement);
			Vec facing;
			facing.x = sin(mAngle.y);
			facing.y = 0.0f;
			facing.z = cos(mAngle.y);
			f32 dot  = PSVECDotProduct(&movement, &facing);
			CVec current(mEye);
			f32 distance = GetHorizontalDistance(&current, &_70);
			if (dot < -0.5f && distance < 20.0f)
				lbl_8065B344 = 1;
			else
				lbl_8065B344 = 0;
		}
		lbl_8065B33C = lbl_8065B340;
		lbl_8065B340 = ((XObjChr*)GetPlayerPoolObject(&gGame, 0))->mHang.mState;
		if (((XObjChr*)GetPlayerPoolObject(&gGame, 0))->mHang.mState == 3 && lbl_8065B340 != lbl_8065B33C) {
			Vec center = mTarget->mMoveTranslate;
			CVec angles;
			SetVectorComponents(&angles, mAngle.x, PI + mTarget->mMoveRotate.mTarget.y, mAngle.z);
			f32 yaw   = HALF_PI + angles.y;
			f32 pitch = mAngle.x;
			f32 distance;
			{
				CVec target = __ct__4CVecFRC3Vec(mTarget->GetCameraTarget());
				CVec current(mEye);
				distance = PSVECDistance(&target, &current);
			}
			Vec candidate;
			candidate.x = center.x + distance * (f32)cos(yaw) * (f32)cos(pitch);
			candidate.z = center.z + distance * -(f32)sin(yaw) * (f32)cos(pitch);
			candidate.y = center.y;
			CopyVecToCVec(sMapCameraTestEye, __ct__4CVecFRC3Vec(candidate));
			CopyVecToCVec(sMapCameraTestCenter, __ct__4CVecFRC3Vec(center));
			mIntersectionCount
			    = CalcIntersectVec__16CCameraActionMapFRC3VecRC3VecP11CamMapEntryl(this, candidate, center, mIntersectionEntries, 0);
			if (mIntersectionCount == 0 && TrySetMapCameraTargetYaw(this, PI + mTarget->mMoveRotate.mTarget.y, 20.0f, 0.0f)) {
				SetMapCameraModeFlags(this, 0x20002);
				goto checkCollision;
			}
		}
		if (lbl_8065B344 != 0) {
			SetInterest(GetMapCameraTargetWithHeightOffset(this));
			CVec angles;
			CopyVecToCVec(angles, __ct__4CVecFfff(mAngle.x, mTarget->mMoveRotate.y, mAngle.z));
			SetAngle(angles);
			CVec current(mEye);
			f32 distance = PSVECDistance(&current, &_70);
			if (distance < 31.0f)
				distance = 31.0f;
			f32 pitch = mAngle.x;
			f32 yaw   = HALF_PI + mAngle.y;
			Vec target;
			target.x = _70.x + distance * (f32)cos(yaw) * (f32)cos(pitch);
			target.z = _70.z + distance * -(f32)sin(yaw) * (f32)cos(pitch);
			target.y = _E60;
			SetEye(target);
			ResetPlayerCameraValue((XObjChr*)GetPlayerPoolObject(&gGame, 0));
			SetMapCameraAnglesFromEyeAndInterest(this);
			if (ClassifyFloatBits(mAngle.x) == FP_NAN || ClassifyFloatBits(mAngle.y) == FP_NAN || ClassifyFloatBits(mAngle.z) == FP_NAN) {
				OSPanic("ccameraactionmap2.cpp", 683, "%s%s",
				        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
				                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
				                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
				                             mEye.x, mEye.y, mEye.z, mAngle.x, mAngle.y, mAngle.z, mDistance, mStatus.mFlags,
				                             mPreviousStatus.mFlags, _EA0, lbl_8065B340, lbl_8065B33C, _F68.x, _F68.y, _F68.z, _F78.x,
				                             _F78.y, _F78.z, _F28, mTurnInterpolation, _F88.x, _F88.y, _F88.z, _F98.x, _F98.y, _F98.z,
				                             _FB8.x, _FB8.y, _FB8.z, lbl_8065B338),
				        "Update");
			}
			lbl_8065B344 = 0;
		} else {
			SetMapCameraAnglesFromEyeAndInterest(this);
			if (ClassifyFloatBits(mAngle.x) == FP_NAN || ClassifyFloatBits(mAngle.y) == FP_NAN || ClassifyFloatBits(mAngle.z) == FP_NAN) {
				OSPanic("ccameraactionmap2.cpp", 693, "%s%s",
				        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
				                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
				                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
				                             mEye.x, mEye.y, mEye.z, mAngle.x, mAngle.y, mAngle.z, mDistance, mStatus.mFlags,
				                             mPreviousStatus.mFlags, _EA0, lbl_8065B340, lbl_8065B33C, _F68.x, _F68.y, _F68.z, _F78.x,
				                             _F78.y, _F78.z, _F28, mTurnInterpolation, _F88.x, _F88.y, _F88.z, _F98.x, _F98.y, _F98.z,
				                             _FB8.x, _FB8.y, _FB8.z, lbl_8065B338),
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
			mIntersectionCount
			    = CalcIntersectVec__16CCameraActionMapFRC3VecRC3VecP11CamMapEntryl(this, mEye, target, mIntersectionEntries, 0);
			_978 = FindNearestMapCameraEntry(this, mTarget->mMoveTranslate);
			_974 = GetNearestMapCameraEntry(this, mTarget->mMoveTranslate);
			_980 = FindNearestMapCameraEntry(this, mEye);
			_97C = GetNearestMapCameraEntry(this, mEye);
			SetMapCameraSideEndpoints(this);
			if (_97C != NULL) {
				s32 count = CountMapCameraEntriesInGroup(this, _97C->_0C);
				s32 index = CountMapCameraAdjacentSideIntersections(this, _97C);
				if (count < 2)
					_E9C = 1;
				else if (index == 0)
					_E9C = 1;
				else {
					s32 flag;
					{
						CamAreaInfo area(&gCameraAreaState._1C[_97C->_0C]);
						flag = (area._04->_2E & 1) != 0;
					}
					if (flag)
						_E9C = 0;
					else
						_E9C = 1;
					if (((XObjChr*)GetPlayerPoolObject(&gGame, 0))->mCopter.mState != 0
					    || ((XObjChr*)GetPlayerPoolObject(&gGame, 0))->mJump.mState != 0) {
						_E9C = 1;
					}
				}
			}
			if (_8C.y == _8C.mTarget.y && _97C != NULL && _E9C == 0) {
				StartMapCameraTurnInterpolation(this, 0);
			}
		}
	checkCollision:
		CVec current(mEye);
		CVec previous(mPreviousEye);
		ResolveMapCameraEyeCollision(this, current, previous, mInterest, mPreviousInterest, sMapCameraCollisionMode0);
		SetEye(current);
	}

	if (mStatus.mFlags & 0x20000) {
		SetInterest(GetMapCameraTargetWithHeightOffset(this));
		if (!(mStatus.mFlags & 0x400))
			_F68.y = _E60;
		_F68.y = _F78.y;
		UpdateMapCameraOrbit(this, step);
		_F68.y = _F78.y;
		if (lbl_8065B330 != 0) {
			CVec previous(_F78);
			CopyVecToCVec(_F78, _F68);
			CVec current;
			CopyVecToCVec(current, _F68);
			CopyVecToCVec(current, SpringDamp(current, _70, _60, dt, sMapCameraSpringParameters[0], _5C, _80));
			CopyVecToCVec(_F68, current);
			CopyVecToCVec(_F78, previous);
		}
		lbl_8065B358 = GetHorizontalDistance(&_F68, &_F78);
		if (!(mStatus.mFlags & 4)) {
			s32 wasBlocked = lbl_8065B348;
			f32 before     = GetHeadingBetweenPositions(&_F68, &mInterest);
			BOOL hit       = ResolveMapCameraEyeCollision(this, _F68, _F78, mInterest, mPreviousInterest, &sMapCameraCollisionMode1);
			if (lbl_8065B34C > 0.0f) {
				f32 after = GetHeadingBetweenPositions(&_F68, &mInterest);
				GetHeadingBetweenPositions(&_F78, &mInterest);
				before = WrapAngle(before);
				after  = WrapAngle(after);
				if (after - before < 0.0f) {
					lbl_8065B348 = 1;
					lbl_8065B350++;
				} else {
					lbl_8065B348 = 0;
					lbl_8065B350 = 0;
				}
			} else if (lbl_8065B34C < 0.0f) {
				f32 after = GetHeadingBetweenPositions(&_F68, &mInterest);
				GetHeadingBetweenPositions(&_F78, &mInterest);
				before = WrapAngle(before);
				after  = WrapAngle(after);
				if (after - before > 0.0f) {
					lbl_8065B348 = 1;
					lbl_8065B350++;
				} else {
					lbl_8065B348 = 0;
					lbl_8065B350 = 0;
				}
			}
			if (lbl_8065B348 == 1 && wasBlocked != lbl_8065B348) {
				CopyVecToCVec(_F68, _F78);
				CopyVecToCVec(_F98, _FB8);
			}
			if (hit != 0) {
				if (lbl_8065B330 != 0)
					lbl_8065B330 = 0;
			} else {
				if (lbl_8065B330 == 0)
					lbl_8065B330 = 1;
			}
		}
		f32 distance = GetHorizontalDistance(&_F68, &_F78);
		lbl_8065B354 = distance;
		if (distance > lbl_8065B358)
			_F64 = 0.0f;
		else {
			_F64 = 3.0f;
			if (mStatus.mFlags & 0x10)
				_F64 = 5.0f;
		}
		_F78.y = _F68.y = _E60;
		SetEye(__ct__4CVecFfff(mEye.x, _F68.y, mEye.z));
		Vec target = _F68;
		if (_F78 != target)
			StartCameraEyeInterpolation(this, target, _F64);
	}

	UpdateCameraEyeInterpolation(this, step);
	f32 weight = GetHorizontalDistance(&mEye, &_70) / 60.0f;
	if (weight > 1.0f)
		weight = 1.0f;
	if (weight < 0.0f)
		weight = 0.0f;
	if (gGame.mMotionControlCamera.GetMapDistance() <= 1.0f)
		weight = 0.0f;
	f32 interestHeight = mInterest.y;
	f32 height         = _E60;
	f32 difference     = height - interestHeight;
	if (difference < 0.0f)
		difference = 0.0f;
	f32 weighted = difference * weight;
	f32 offset   = interestHeight + weighted;
	offset -= height;
	f32 from = _E78->Get();
	if (0.0f != offset) {
		_E78 = &_E7C;
		_E78->Start(from, offset, 0.05f);
	}
	Vec eye = mEye;
	eye.y   = _E60;
	eye.y += _E98;
	SetEye(eye);
	SetMapCameraPreviousEye(this, mEye);
	CopyVecToCVec(_F78, _F68);
	CopyVecToCVec(_FA8, _F88);
	SetMapCameraPreviousInterest(this, mInterest);
	mPreviousStatus.mFlags = 0;
	mPreviousStatus.mFlags |= mStatus.mFlags;
}

// todo: cleanup, same body as TrySetMapCameraTargetYaw but it allocates differently out of line
static inline BOOL SetMapYaw(CCameraActionMap* map, f32 yaw, f32 time)
{
	f32 oldYaw   = map->_C0;
	f32 oldSpeed = map->_BC;

	map->_C0 = WrapAngle(yaw);
	f32 from = WrapAngle(map->mAngle.y);
	f32 to   = WrapAngle(yaw);

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
	map->_BC = diff / time;
	if (map->_BC < (2.0f * DEG2RAD)) {
		map->_C0 = oldYaw;
		map->_BC = oldSpeed;
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
	pos.x                = center.x + distance * (f32)cos(angle) * (f32)cos(pitch);
	pos.z                = center.z + distance * -(f32)sin(angle) * (f32)cos(pitch);
	pos.y                = center.y;
	sMapCameraTestEye    = CVec(pos);
	sMapCameraTestCenter = CVec(center);
	map->mIntersectionCount
	    = CalcIntersectVec__16CCameraActionMapFRC3VecRC3VecP11CamMapEntryl(map, pos, center, map->mIntersectionEntries, 0);
	if (map->mIntersectionCount == 0 && SetMapYaw(map, yaw, 25.0f)) {
		SetMapCameraModeFlags(map, 0x20002);
		return TRUE;
	}
	return FALSE;
}
/*
    stwu     r1, -0xe0(r1)
    mflr     r0
    stw      r0, 0xe4(r1)
    stfd     f31, 0xd0(r1)
    psq_st   f31, 0xd8(r1), 0, qr0
    stfd     f30, 0xc0(r1)
    psq_st   f30, 0xc8(r1), 0, qr0
    stfd     f29, 0xb0(r1)
    psq_st   f29, 0xb8(r1), 0, qr0
    stfd     f28, 0xa0(r1)
    psq_st   f28, 0xa8(r1), 0, qr0
    stfd     f27, 0x90(r1)
    psq_st   f27, 0x98(r1), 0, qr0
    stw      r31, 0x8c(r1)
    lwz      r4, 0x44(r3)
    fmr      f31, f1
    mr       r31, r3
    lis      r3, __vt__4CVec@ha
    lwz      r6, 0xc8(r4)
    addi     r0, r3, __vt__4CVec@l
    lwz      r5, 0xcc(r4)
    stw      r0, 0x6c(r1)
    frsp     f0, f31
    lfs      f1, lbl_8065E98C@sda21(r0)
    addi     r3, r1, 0x28
    stw      r6, 0x70(r1)
    fadds    f28, f1, f0
    stw      r5, 0x74(r1)
    lwz      r0, 0xd0(r4)
    stfs     f31, 0x64(r1)
    stw      r0, 0x78(r1)
    lfs      f0, 0x20(r31)
    lwz      r12, 0x34(r4)
    lfs      f1, 0x28(r31)
    fmr      f27, f0
    lwz      r12, 0xec(r12)
    stfs     f0, 0x60(r1)
    stfs     f1, 0x68(r1)
    mtctr    r12
    bctrl
    lfs      f2, 0x28(r1)
    lis      r3, __vt__4CVec@ha
    lfs      f1, 0x2c(r1)
    addi     r6, r3, __vt__4CVec@l
    lfs      f0, 0x30(r1)
    addi     r3, r1, 0x44
    stw      r6, 0x50(r1)
    addi     r4, r1, 0x34
    stfs     f2, 0x44(r1)
    stfs     f1, 0x48(r1)
    stfs     f0, 0x4c(r1)
    lwz      r5, 0x0(r31)
    lwz      r0, 0x4(r31)
    stw      r5, 0x34(r1)
    stw      r0, 0x38(r1)
    lwz      r0, 0x8(r31)
    stw      r0, 0x3c(r1)
    stw      r6, 0x40(r1)
    bl       PSVECDistance
    lis      r3, __vt__4CVec@ha
    fmr      f30, f1
    addi     r0, r3, __vt__4CVec@l
    fmr      f1, f27
    stw      r0, 0x40(r1)
    stw      r0, 0x50(r1)
    bl       cos
    frsp     f29, f1
    fmr      f1, f28
    bl       cos
    frsp     f2, f1
    lfs      f0, 0x70(r1)
    fmr      f1, f27
    fmuls    f2, f30, f2
    fmadds   f0, f2, f29, f0
    stfs     f0, 0x54(r1)
    bl       cos
    frsp     f29, f1
    fmr      f1, f28
    bl       sin
    frsp     f0, f1
    lfs      f5, 0x74(r1)
    lfs      f6, 0x78(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f3, 0x54(r1)
    fneg     f1, f0
    stw      r0, 0x24(r1)
    lfs      f0, 0x70(r1)
    lis      r4, sMapCameraTestEye@ha
    stw      r0, 0x14(r1)
    lis      r3, sMapCameraTestCenter@ha
    fmuls    f1, f30, f1
    addi     r8, r3, sMapCameraTestCenter@l
    addi     r9, r4, sMapCameraTestEye@l
    frsp     f2, f5
    stfs     f5, 0x58(r1)
    mr       r3, r31
    fmadds   f4, f1, f29, f6
    stfs     f3, 0x18(r1)
    addi     r4, r1, 0x54
    addi     r5, r1, 0x70
    stfs     f5, 0x1c(r1)
    addi     r6, r31, 0x570
    frsp     f1, f4
    stfs     f4, 0x5c(r1)
    li       r7, 0x0
    stfs     f4, 0x20(r1)
    stfs     f3, 0x0(r9)
    stfs     f2, 0x4(r9)
    stfs     f1, 0x8(r9)
    stw      r0, 0x24(r1)
    stfs     f0, 0x8(r1)
    stfs     f5, 0xc(r1)
    stfs     f6, 0x10(r1)
    stfs     f0, 0x0(r8)
    stfs     f5, 0x4(r8)
    stfs     f6, 0x8(r8)
    stw      r0, 0x14(r1)
    bl       CalcIntersectVec__16CCameraActionMapFRC3VecRC3VecP11CamMapEntryl
    stw      r3, 0x970(r31)
    lwz      r0, 0x970(r31)
    cmpwi    r0, 0x0
    bne      .L_8013A1B8
    fmr      f4, f31
    lfs      f3, 0xc0(r31)
    lfs      f2, 0xbc(r31)
    lfs      f1, lbl_8065E990@sda21(r0)
    lfs      f0, lbl_8065E994@sda21(r0)
    b        .L_8013A060
.L_8013A05C:
    fadds    f4, f4, f1
.L_8013A060:
    fcmpo    cr0, f4, f0
    blt      .L_8013A05C
    lfs      f0, lbl_8065E990@sda21(r0)
    b        .L_8013A074
.L_8013A070:
    fsubs    f4, f4, f0
.L_8013A074:
    fcmpo    cr0, f4, f0
    cror     eq, gt, eq
    beq      .L_8013A070
    stfs     f4, 0xc0(r31)
    lfs      f1, lbl_8065E990@sda21(r0)
    lfs      f4, 0x24(r31)
    lfs      f0, lbl_8065E994@sda21(r0)
    b        .L_8013A098
.L_8013A094:
    fadds    f4, f4, f1
.L_8013A098:
    fcmpo    cr0, f4, f0
    blt      .L_8013A094
    lfs      f0, lbl_8065E990@sda21(r0)
    b        .L_8013A0AC
.L_8013A0A8:
    fsubs    f4, f4, f0
.L_8013A0AC:
    fcmpo    cr0, f4, f0
    cror     eq, gt, eq
    beq      .L_8013A0A8
    fmr      f5, f31
    lfs      f1, lbl_8065E990@sda21(r0)
    lfs      f0, lbl_8065E994@sda21(r0)
    b        .L_8013A0CC
.L_8013A0C8:
    fadds    f5, f5, f1
.L_8013A0CC:
    fcmpo    cr0, f5, f0
    blt      .L_8013A0C8
    lfs      f0, lbl_8065E990@sda21(r0)
    b        .L_8013A0E0
.L_8013A0DC:
    fsubs    f5, f5, f0
.L_8013A0E0:
    fcmpo    cr0, f5, f0
    cror     eq, gt, eq
    beq      .L_8013A0DC
    fsubs    f1, f5, f4
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_8013A100
    fneg     f1, f1
.L_8013A100:
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_8013A148
    lfs      f0, lbl_8065E990@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_8013A120
    fsubs    f1, f1, f0
.L_8013A120:
    lfs      f0, lbl_8065E998@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_8013A148
    fcmpo    cr0, f5, f4
    bge      .L_8013A140
    lfs      f0, lbl_8065E990@sda21(r0)
    fadds    f5, f5, f0
    b        .L_8013A148
.L_8013A140:
    lfs      f0, lbl_8065E990@sda21(r0)
    fsubs    f5, f5, f0
.L_8013A148:
    fsubs    f4, f5, f4
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f4, f0
    bge      .L_8013A15C
    fneg     f4, f4
.L_8013A15C:
    lfs      f1, lbl_8065E99C@sda21(r0)
    lfs      f0, lbl_8065E9A0@sda21(r0)
    fdivs    f1, f4, f1
    stfs     f1, 0xbc(r31)
    lfs      f1, 0xbc(r31)
    fcmpo    cr0, f1, f0
    bge      .L_8013A188
    stfs     f3, 0xc0(r31)
    li       r0, 0x0
    stfs     f2, 0xbc(r31)
    b        .L_8013A18C
.L_8013A188:
    li       r0, 0x1
.L_8013A18C:
    cmpwi    r0, 0x0
    beq      .L_8013A1B8
    lis      r4, 0x2
    mr       r3, r31
    addi     r4, r4, 0x2
    bl       SetMapCameraModeFlags
    lis      r4, __vt__4CVec@ha
    li       r3, 0x1
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x6c(r1)
    b        .L_8013A1C8
.L_8013A1B8:
    lis      r4, __vt__4CVec@ha
    li       r3, 0x0
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x6c(r1)
.L_8013A1C8:
    psq_l    f31, 0xd8(r1), 0, qr0
    lfd      f31, 0xd0(r1)
    psq_l    f30, 0xc8(r1), 0, qr0
    lfd      f30, 0xc0(r1)
    psq_l    f29, 0xb8(r1), 0, qr0
    lfd      f29, 0xb0(r1)
    psq_l    f28, 0xa8(r1), 0, qr0
    lfd      f28, 0xa0(r1)
    psq_l    f27, 0x98(r1), 0, qr0
    lfd      f27, 0x90(r1)
    lwz      r0, 0xe4(r1)
    lwz      r31, 0x8c(r1)
    mtlr     r0
    addi     r1, r1, 0xe0
    blr
*/

/**
 * @note Address: 0x8013A204
 * @note Size: 0x44
 */
extern "C" void StartMapCameraTargetHeightInterpolation(CCameraActionMap* map, f32 from, f32 to, f32 time)
{
	if (0.0f != time) {
		map->_E64 = &map->_E68;
		map->_E64->Start(from, to, time);
	}
}

/**
 * @note Address: 0x8013A248
 * @note Size: 0xA4
 */
extern "C" Vec GetMapCameraTargetWithHeightOffset(CCameraActionMap* map)
{
	Vec target = map->mTarget->GetCameraTarget();
	if (map->_E64 != NULL) {
		target.y += map->_E64->Get();
	}
	return target;
}

extern "C" void ResetMapCameraTargetEye(CCameraActionMap* map);

// _60 follows _70
static inline void SetCenter(CCameraActionMap* map, const Vec& pos)
{
	map->_70 = pos;
	map->_60 = map->_70;
}

static inline void ResetCenter(CCameraActionMap* map)
{
	Vec* center = &map->_70;
	SetCenter(map, *center);
	CVec eye     = map->mEye;
	map->_58     = PSVECDistance(&eye, center);
	lbl_8065B330 = 1;
	map->mStatus.mFlags &= ~0x700;
	if (map->_F28 != NULL) {
		map->_F28 = NULL;
	}
}

/**
 * @note Address: 0x8013A2EC
 * @note Size: 0x26C
 */
extern "C" void SetMapCameraModeFlags(CCameraActionMap* map, u32 flags)
{
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
			if (lbl_8065B344 == 1) {
				map->_88 = map->_80;
			} else if (eyeMode == 4) {
				CVec pos = map->_70;
				CVec eye = map->mEye;
				map->_88 = PSVECDistance(&pos, &eye);
			} else {
				CVec target = map->mTarget->GetCameraTarget();
				CVec eye    = map->mEye;
				f32 dist    = PSVECDistance(&eye, &target);
				if (dist < 31.0f) {
					dist = 31.0f;
				}
				map->_88 = dist;
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
	if (map->_F28 != NULL) {
		if (!map->_F28->Update(step, FRAME_TIME)) {
			map->SetEye(map->_F28->Get());
		} else {
			map->_F28 = NULL;
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
	map->_F28 = &map->_F2C;
	map->_F28->Start(map->mEye, to, time);
}

extern "C" Vec sPreviousMapOrbitAngle; // saved before updating the orbit

/**
 * @note Address: 0x8013A710
 * @note Size: 0x90
 */
extern "C" void ResetMapCameraInterpolation(CCameraActionMap* map)
{
	map->_F88    = map->mEye;
	map->_FA8    = map->mEye;
	map->_F98    = map->mAngle;
	map->_FB8    = map->mAngle;
	map->_F28    = NULL;
	lbl_8065B34C = 0.0f;
	map->_F98.y  = sPreviousMapOrbitAngle.y;
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
	map->_F88 = map->mEye;
	map->_FA8 = map->mEye;
	map->_F98 = map->mAngle;
	map->_FB8 = map->mAngle;
	map->_F78 = map->mEye;
	map->_F68 = map->_F78;
	map->_F28 = NULL;

	map->_84 = FlatDistance(map->_F68, map->mInterest);
	FlatDistance(map->_F68, map->_70);
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
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stfd     f31, 0x30(r1)
    psq_st   f31, 0x38(r1), 0, qr0
    stw      r31, 0x2c(r1)
    mr       r31, r3
    lfs      f0, lbl_8065E994@sda21(r0)
    lfs      f2, 0x18(r3)
    lfs      f1, 0x8(r3)
    lfs      f4, 0x10(r3)
    fsubs    f5, f2, f1
    lfs      f1, 0x0(r3)
    lfs      f3, 0x14(r3)
    fsubs    f4, f4, f1
    lfs      f2, 0x4(r3)
    fmuls    f1, f5, f5
    fsubs    f31, f3, f2
    fmadds   f1, f4, f4, f1
    fcmpo    cr0, f1, f0
    ble      .L_8013A970
    frsqrte  f2, f1
    lfd      f4, lbl_8065E9B0@sda21(r0)
    lfd      f3, lbl_8065E9B8@sda21(r0)
    fmul     f0, f2, f2
    fmul     f2, f4, f2
    fnmsub   f0, f1, f0, f3
    fmul     f2, f2, f0
    fmul     f0, f2, f2
    fmul     f2, f4, f2
    fnmsub   f0, f1, f0, f3
    fmul     f2, f2, f0
    fmul     f0, f2, f2
    fmul     f2, f4, f2
    fnmsub   f0, f1, f0, f3
    fmul     f0, f2, f0
    fmul     f1, f1, f0
    frsp     f1, f1
    b        .L_8013A9F4
.L_8013A970:
    lfd      f0, lbl_8065E9C0@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_8013A988
    lis      r3, __float_nan@ha
    lfs      f1, __float_nan@l(r3)
    b        .L_8013A9F4
.L_8013A988:
    stfs     f1, 0xc(r1)
    lis      r0, 0x7f80
    lwz      r4, 0xc(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013A9B0
    bge      .L_8013A9E0
    cmpwi    r3, 0x0
    beq      .L_8013A9C8
    b        .L_8013A9E0
.L_8013A9B0:
    clrlwi.  r0, r4, 9
    beq      .L_8013A9C0
    li       r0, 0x1
    b        .L_8013A9E4
.L_8013A9C0:
    li       r0, 0x2
    b        .L_8013A9E4
.L_8013A9C8:
    clrlwi.  r0, r4, 9
    beq      .L_8013A9D8
    li       r0, 0x5
    b        .L_8013A9E4
.L_8013A9D8:
    li       r0, 0x3
    b        .L_8013A9E4
.L_8013A9E0:
    li       r0, 0x4
.L_8013A9E4:
    cmpwi    r0, 0x1
    bne      .L_8013A9F4
    lis      r3, __float_nan@ha
    lfs      f1, __float_nan@l(r3)
.L_8013A9F4:
    fmuls    f2, f31, f31
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f2, f0
    ble      .L_8013AA4C
    frsqrte  f3, f2
    lfd      f5, lbl_8065E9B0@sda21(r0)
    lfd      f4, lbl_8065E9B8@sda21(r0)
    fmul     f0, f3, f3
    fmul     f3, f5, f3
    fnmsub   f0, f2, f0, f4
    fmul     f3, f3, f0
    fmul     f0, f3, f3
    fmul     f3, f5, f3
    fnmsub   f0, f2, f0, f4
    fmul     f3, f3, f0
    fmul     f0, f3, f3
    fmul     f3, f5, f3
    fnmsub   f0, f2, f0, f4
    fmul     f0, f3, f0
    fmul     f2, f2, f0
    frsp     f2, f2
    b        .L_8013AAD0
.L_8013AA4C:
    lfd      f0, lbl_8065E9C0@sda21(r0)
    fcmpo    cr0, f2, f0
    bge      .L_8013AA64
    lis      r3, __float_nan@ha
    lfs      f2, __float_nan@l(r3)
    b        .L_8013AAD0
.L_8013AA64:
    stfs     f2, 0x8(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x8(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013AA8C
    bge      .L_8013AABC
    cmpwi    r3, 0x0
    beq      .L_8013AAA4
    b        .L_8013AABC
.L_8013AA8C:
    clrlwi.  r0, r4, 9
    beq      .L_8013AA9C
    li       r0, 0x1
    b        .L_8013AAC0
.L_8013AA9C:
    li       r0, 0x2
    b        .L_8013AAC0
.L_8013AAA4:
    clrlwi.  r0, r4, 9
    beq      .L_8013AAB4
    li       r0, 0x5
    b        .L_8013AAC0
.L_8013AAB4:
    li       r0, 0x3
    b        .L_8013AAC0
.L_8013AABC:
    li       r0, 0x4
.L_8013AAC0:
    cmpwi    r0, 0x1
    bne      .L_8013AAD0
    lis      r3, __float_nan@ha
    lfs      f2, __float_nan@l(r3)
.L_8013AAD0:
    bl       atan2
    lfs      f0, lbl_8065E994@sda21(r0)
    frsp     f1, f1
    fcmpo    cr0, f31, f0
    bge      .L_8013AAF0
    lfs      f0, lbl_8065E98C@sda21(r0)
    fsubs    f0, f0, f1
    b        .L_8013AAF8
.L_8013AAF0:
    lfs      f0, lbl_8065E98C@sda21(r0)
    fsubs    f0, f1, f0
.L_8013AAF8:
    stfs     f0, 0x10(r1)
    lfs      f3, 0x10(r31)
    lfs      f1, 0x0(r31)
    lfs      f2, 0x18(r31)
    lfs      f0, 0x8(r31)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f1, f1
    lfs      f0, lbl_8065E994@sda21(r0)
    mr       r3, r31
    addi     r4, r1, 0x10
    stfs     f0, 0x18(r1)
    stfs     f1, 0x14(r1)
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x40(r12)
    mtctr    r12
    bctrl
    psq_l    f31, 0x38(r1), 0, qr0
    lwz      r0, 0x44(r1)
    lfd      f31, 0x30(r1)
    lwz      r31, 0x2c(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: 0x8013AB5C
 * @note Size: 0x668
 */
extern "C" void UpdateMapCameraTurnInterpolation(CCameraActionMap* map, u32 step)
{
	if (map->mTurnInterpolation != NULL) {
		if (!map->mTurnInterpolation->Update(step, FRAME_TIME)) {
			f32 radius = map->_88;
			CVec position(map->mTurnInterpolation->Get());
			CVec origin(map->_70);
			origin.y   = 0.0f;
			position   = origin + CVec((position - origin).Normalize()) * radius;
			position.y = map->mEye.y;
			map->_F88  = position;
			map->_F68  = position;

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
				                             map->mDistance, map->mStatus.mFlags, map->mPreviousStatus.mFlags, map->_EA0, lbl_8065B340,
				                             lbl_8065B33C, map->_F68.x, map->_F68.y, map->_F68.z, map->_F78.x, map->_F78.y, map->_F78.z,
				                             map->_F28, map->mTurnInterpolation, map->_F88.x, map->_F88.y, map->_F88.z, map->_F98.x,
				                             map->_F98.y, map->_F98.z, map->_FB8.x, map->_FB8.y, map->_FB8.z, lbl_8065B338),
				        "UpdateTurn");
			}
			map->_F98 = map->mAngle;
			map->_EA0 = 1;
		} else {
			map->mStatus.mFlags &= ~4;
			map->mTurnInterpolation = NULL;
		}
	}
}
/*
    stwu     r1, -0x140(r1)
    mflr     r0
    stw      r0, 0x144(r1)
    stfd     f31, 0x130(r1)
    psq_st   f31, 0x138(r1), 0, qr0
    stw      r31, 0x12c(r1)
    mr       r31, r3
    lwz      r3, 0xdb4(r3)
    cmplwi   r3, 0x0
    beq      .L_8013B1A8
    lwz      r12, 0x0(r3)
    lfs      f1, lbl_8065E9AC@sda21(r0)
    lwz      r12, 0x14(r12)
    mtctr    r12
    bctrl
    clrlwi.  r0, r3, 24
    bne      .L_8013B194
    lwz      r4, 0xdb4(r31)
    addi     r3, r1, 0xe8
    lfs      f31, 0x88(r31)
    lwz      r12, 0x0(r4)
    lwz      r12, 0x18(r12)
    mtctr    r12
    bctrl
    lfs      f3, 0xe8(r1)
    lis      r3, __vt__4CVec@ha
    lfs      f2, 0xec(r1)
    addi     r8, r3, __vt__4CVec@l
    lfs      f1, 0xf0(r1)
    addi     r3, r1, 0xcc
    stw      r8, 0x120(r1)
    addi     r4, r1, 0x114
    lfs      f0, lbl_8065E994@sda21(r0)
    addi     r5, r1, 0xdc
    stfs     f3, 0x114(r1)
    stfs     f2, 0x118(r1)
    stfs     f1, 0x11c(r1)
    lwz      r7, 0x70(r31)
    lwz      r0, 0x74(r31)
    stw      r7, 0x104(r1)
    stw      r0, 0x108(r1)
    lwz      r6, 0x78(r31)
    stfs     f0, 0x108(r1)
    lwz      r0, 0x108(r1)
    stw      r6, 0x10c(r1)
    stw      r8, 0x110(r1)
    stw      r7, 0xdc(r1)
    stw      r0, 0xe0(r1)
    stw      r6, 0xe4(r1)
    bl       fn_800244A0
    addi     r3, r1, 0xcc
    mr       r4, r3
    bl       PSVECNormalize
    lwz      r4, 0xcc(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lwz      r6, 0xd0(r1)
    lwz      r5, 0xd4(r1)
    fmr      f1, f31
    stw      r4, 0xf4(r1)
    addi     r3, r1, 0xb0
    addi     r4, r1, 0xf4
    stw      r6, 0xf8(r1)
    stw      r5, 0xfc(r1)
    stw      r0, 0x100(r1)
    stw      r0, 0xd8(r1)
    bl       fn_8000F9C0
    lwz      r7, 0xb0(r1)
    addi     r3, r1, 0xa0
    lwz      r6, 0xb4(r1)
    addi     r4, r1, 0x104
    lwz      r0, 0xb8(r1)
    addi     r5, r1, 0xc0
    stw      r7, 0xc0(r1)
    stw      r6, 0xc4(r1)
    stw      r0, 0xc8(r1)
    bl       fn_8000F93C
    lfs      f3, 0xa0(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f2, 0xa4(r1)
    lfs      f1, 0xa8(r1)
    stfs     f3, 0x114(r1)
    lfs      f0, lbl_8065E994@sda21(r0)
    stfs     f2, 0x118(r1)
    stfs     f1, 0x11c(r1)
    stw      r0, 0xac(r1)
    stw      r0, 0xbc(r1)
    stw      r0, 0x100(r1)
    stw      r0, 0x110(r1)
    lfs      f1, 0x4(r31)
    stfs     f1, 0x118(r1)
    stfs     f3, 0xf88(r31)
    lfs      f1, 0x118(r1)
    stfs     f1, 0xf8c(r31)
    lfs      f1, 0x11c(r1)
    stfs     f1, 0xf90(r31)
    lfs      f1, 0x114(r1)
    stfs     f1, 0xf68(r31)
    lfs      f1, 0x118(r1)
    stfs     f1, 0xf6c(r31)
    lfs      f1, 0x11c(r1)
    stfs     f1, 0xf70(r31)
    lfs      f2, 0x18(r31)
    lfs      f1, 0x8(r31)
    lfs      f4, 0x10(r31)
    fsubs    f5, f2, f1
    lfs      f1, 0x0(r31)
    lfs      f3, 0x14(r31)
    lfs      f2, 0x4(r31)
    fsubs    f4, f4, f1
    fmuls    f1, f5, f5
    fsubs    f31, f3, f2
    fmadds   f1, f4, f4, f1
    fcmpo    cr0, f1, f0
    ble      .L_8013AD74
    frsqrte  f2, f1
    lfd      f4, lbl_8065E9B0@sda21(r0)
    lfd      f3, lbl_8065E9B8@sda21(r0)
    fmul     f0, f2, f2
    fmul     f2, f4, f2
    fnmsub   f0, f1, f0, f3
    fmul     f2, f2, f0
    fmul     f0, f2, f2
    fmul     f2, f4, f2
    fnmsub   f0, f1, f0, f3
    fmul     f2, f2, f0
    fmul     f0, f2, f2
    fmul     f2, f4, f2
    fnmsub   f0, f1, f0, f3
    fmul     f0, f2, f0
    fmul     f1, f1, f0
    frsp     f1, f1
    b        .L_8013ADF8
.L_8013AD74:
    lfd      f0, lbl_8065E9C0@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_8013AD8C
    lis      r3, __float_nan@ha
    lfs      f1, __float_nan@l(r3)
    b        .L_8013ADF8
.L_8013AD8C:
    stfs     f1, 0x84(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x84(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013ADB4
    bge      .L_8013ADE4
    cmpwi    r3, 0x0
    beq      .L_8013ADCC
    b        .L_8013ADE4
.L_8013ADB4:
    clrlwi.  r0, r4, 9
    beq      .L_8013ADC4
    li       r0, 0x1
    b        .L_8013ADE8
.L_8013ADC4:
    li       r0, 0x2
    b        .L_8013ADE8
.L_8013ADCC:
    clrlwi.  r0, r4, 9
    beq      .L_8013ADDC
    li       r0, 0x5
    b        .L_8013ADE8
.L_8013ADDC:
    li       r0, 0x3
    b        .L_8013ADE8
.L_8013ADE4:
    li       r0, 0x4
.L_8013ADE8:
    cmpwi    r0, 0x1
    bne      .L_8013ADF8
    lis      r3, __float_nan@ha
    lfs      f1, __float_nan@l(r3)
.L_8013ADF8:
    fmuls    f2, f31, f31
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f2, f0
    ble      .L_8013AE50
    frsqrte  f3, f2
    lfd      f5, lbl_8065E9B0@sda21(r0)
    lfd      f4, lbl_8065E9B8@sda21(r0)
    fmul     f0, f3, f3
    fmul     f3, f5, f3
    fnmsub   f0, f2, f0, f4
    fmul     f3, f3, f0
    fmul     f0, f3, f3
    fmul     f3, f5, f3
    fnmsub   f0, f2, f0, f4
    fmul     f3, f3, f0
    fmul     f0, f3, f3
    fmul     f3, f5, f3
    fnmsub   f0, f2, f0, f4
    fmul     f0, f3, f0
    fmul     f2, f2, f0
    frsp     f2, f2
    b        .L_8013AED4
.L_8013AE50:
    lfd      f0, lbl_8065E9C0@sda21(r0)
    fcmpo    cr0, f2, f0
    bge      .L_8013AE68
    lis      r3, __float_nan@ha
    lfs      f2, __float_nan@l(r3)
    b        .L_8013AED4
.L_8013AE68:
    stfs     f2, 0x80(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x80(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013AE90
    bge      .L_8013AEC0
    cmpwi    r3, 0x0
    beq      .L_8013AEA8
    b        .L_8013AEC0
.L_8013AE90:
    clrlwi.  r0, r4, 9
    beq      .L_8013AEA0
    li       r0, 0x1
    b        .L_8013AEC4
.L_8013AEA0:
    li       r0, 0x2
    b        .L_8013AEC4
.L_8013AEA8:
    clrlwi.  r0, r4, 9
    beq      .L_8013AEB8
    li       r0, 0x5
    b        .L_8013AEC4
.L_8013AEB8:
    li       r0, 0x3
    b        .L_8013AEC4
.L_8013AEC0:
    li       r0, 0x4
.L_8013AEC4:
    cmpwi    r0, 0x1
    bne      .L_8013AED4
    lis      r3, __float_nan@ha
    lfs      f2, __float_nan@l(r3)
.L_8013AED4:
    bl       atan2
    lfs      f0, lbl_8065E994@sda21(r0)
    frsp     f1, f1
    fcmpo    cr0, f31, f0
    bge      .L_8013AEF4
    lfs      f0, lbl_8065E98C@sda21(r0)
    fsubs    f0, f0, f1
    b        .L_8013AEFC
.L_8013AEF4:
    lfs      f0, lbl_8065E98C@sda21(r0)
    fsubs    f0, f1, f0
.L_8013AEFC:
    stfs     f0, 0x94(r1)
    lfs      f3, 0x10(r31)
    lfs      f1, 0x0(r31)
    lfs      f2, 0x18(r31)
    lfs      f0, 0x8(r31)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f1, f1
    lfs      f0, lbl_8065E994@sda21(r0)
    mr       r3, r31
    addi     r4, r1, 0x94
    stfs     f0, 0x9c(r1)
    stfs     f1, 0x98(r1)
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x40(r12)
    mtctr    r12
    bctrl
    lfs      f0, 0x20(r31)
    lis      r0, 0x7f80
    stfs     f0, 0x90(r1)
    lwz      r4, 0x90(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013AF70
    bge      .L_8013AFA0
    cmpwi    r3, 0x0
    beq      .L_8013AF88
    b        .L_8013AFA0
.L_8013AF70:
    clrlwi.  r0, r4, 9
    beq      .L_8013AF80
    li       r0, 0x1
    b        .L_8013AFA4
.L_8013AF80:
    li       r0, 0x2
    b        .L_8013AFA4
.L_8013AF88:
    clrlwi.  r0, r4, 9
    beq      .L_8013AF98
    li       r0, 0x5
    b        .L_8013AFA4
.L_8013AF98:
    li       r0, 0x3
    b        .L_8013AFA4
.L_8013AFA0:
    li       r0, 0x4
.L_8013AFA4:
    cmpwi    r0, 0x1
    beq      .L_8013B07C
    lfs      f0, 0x24(r31)
    lis      r0, 0x7f80
    stfs     f0, 0x8c(r1)
    lwz      r4, 0x8c(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013AFD8
    bge      .L_8013B008
    cmpwi    r3, 0x0
    beq      .L_8013AFF0
    b        .L_8013B008
.L_8013AFD8:
    clrlwi.  r0, r4, 9
    beq      .L_8013AFE8
    li       r0, 0x1
    b        .L_8013B00C
.L_8013AFE8:
    li       r0, 0x2
    b        .L_8013B00C
.L_8013AFF0:
    clrlwi.  r0, r4, 9
    beq      .L_8013B000
    li       r0, 0x5
    b        .L_8013B00C
.L_8013B000:
    li       r0, 0x3
    b        .L_8013B00C
.L_8013B008:
    li       r0, 0x4
.L_8013B00C:
    cmpwi    r0, 0x1
    beq      .L_8013B07C
    lfs      f0, 0x28(r31)
    lis      r0, 0x7f80
    stfs     f0, 0x88(r1)
    lwz      r4, 0x88(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013B040
    bge      .L_8013B070
    cmpwi    r3, 0x0
    beq      .L_8013B058
    b        .L_8013B070
.L_8013B040:
    clrlwi.  r0, r4, 9
    beq      .L_8013B050
    li       r0, 0x1
    b        .L_8013B074
.L_8013B050:
    li       r0, 0x2
    b        .L_8013B074
.L_8013B058:
    clrlwi.  r0, r4, 9
    beq      .L_8013B068
    li       r0, 0x5
    b        .L_8013B074
.L_8013B068:
    li       r0, 0x3
    b        .L_8013B074
.L_8013B070:
    li       r0, 0x4
.L_8013B074:
    cmpwi    r0, 0x1
    bne      .L_8013B164
.L_8013B07C:
    lfs      f0, 0xf6c(r31)
    lis      r3, sMapCameraStateFormat@ha
    addi     r3, r3, sMapCameraStateFormat@l
    stfd     f0, 0x8(r1)
    lfs      f0, 0xf70(r31)
    stfd     f0, 0x10(r1)
    lfs      f0, 0xf78(r31)
    stfd     f0, 0x18(r1)
    lfs      f0, 0xf7c(r31)
    stfd     f0, 0x20(r1)
    lfs      f0, 0xf80(r31)
    stfd     f0, 0x28(r1)
    lfs      f0, 0xf88(r31)
    stfd     f0, 0x30(r1)
    lfs      f0, 0xf8c(r31)
    stfd     f0, 0x38(r1)
    lfs      f0, 0xf90(r31)
    stfd     f0, 0x40(r1)
    lfs      f0, 0xf98(r31)
    stfd     f0, 0x48(r1)
    lfs      f0, 0xf9c(r31)
    stfd     f0, 0x50(r1)
    lfs      f0, 0xfa0(r31)
    stfd     f0, 0x58(r1)
    lfs      f0, 0xfb8(r31)
    stfd     f0, 0x60(r1)
    lfs      f0, 0xfbc(r31)
    stfd     f0, 0x68(r1)
    lfs      f0, 0xfc0(r31)
    stfd     f0, 0x70(r1)
    lwz      r0, lbl_8065B338@sda21(r0)
    stw      r0, 0x78(r1)
    lfs      f1, 0x0(r31)
    lfs      f2, 0x4(r31)
    lfs      f3, 0x8(r31)
    lfs      f4, 0x20(r31)
    lfs      f5, 0x24(r31)
    lfs      f6, 0x28(r31)
    lfs      f7, 0x30(r31)
    lwz      r4, 0x11d8(r31)
    lwz      r5, 0x11e0(r31)
    lwz      r6, 0xea0(r31)
    lwz      r7, lbl_8065B340@sda21(r0)
    lwz      r8, lbl_8065B33C@sda21(r0)
    lfs      f8, 0xf68(r31)
    lwz      r9, 0xf28(r31)
    lwz      r10, 0xdb4(r31)
    crset    cr1eq
    bl       FormatToStaticBuffer
    lis      r7, sPathCcameraactionmap2Cpp@ha
    lis      r5, sMapCameraUpdateTurnLabel@ha
    mr       r6, r3
    li       r4, 0xf8c
    addi     r3, r7, sPathCcameraactionmap2Cpp@l
    addi     r7, r5, sMapCameraUpdateTurnLabel@l
    li       r5, lbl_8065E9C8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8013B164:
    lfs      f0, 0x20(r31)
    lis      r3, __vt__4CVec@ha
    li       r4, 0x1
    stfs     f0, 0xf98(r31)
    addi     r0, r3, __vt__4CVec@l
    lfs      f0, 0x24(r31)
    stfs     f0, 0xf9c(r31)
    lfs      f0, 0x28(r31)
    stfs     f0, 0xfa0(r31)
    stw      r4, 0xea0(r31)
    stw      r0, 0x120(r1)
    b        .L_8013B1A8
.L_8013B194:
    lwz      r3, 0x11d8(r31)
    li       r0, 0x0
    rlwinm   r3, r3, 0, 30, 28
    stw      r3, 0x11d8(r31)
    stw      r0, 0xdb4(r31)
.L_8013B1A8:
    psq_l    f31, 0x138(r1), 0, qr0
    lwz      r0, 0x144(r1)
    lfd      f31, 0x130(r1)
    lwz      r31, 0x12c(r1)
    mtlr     r0
    addi     r1, r1, 0x140
    blr
*/

extern "C" void SetMapCameraModeFlags(CCameraActionMap* map, u32 flags);

/**
 * @note Address: 0x8013B1C4
 * @note Size: 0x204
 */
extern "C" void StartMapCameraTurnInterpolation(CCameraActionMap* map, s32 unused)
{
	CVec to(map->_984);
	CVec a(map->_97C->_14);
	CVec b(map->_974->_14);
	CVec move = a * 20.0f;
	move += b * 5.0f;
	to += move;

	Vec from = map->mEye;
	from.y   = 0.0f;
	if (map->mTurnInterpolation == NULL) {
		map->mTurnInterpolation = &map->_DB8;
		map->mTurnInterpolation->Start(from, to, 0.7f);
		map->_DB0 = 0;
		gGame.GetPlayer()->ResetCameraValue();
		lbl_8065B334 = map->_974->_0C;
		SetMapCameraModeFlags(map, 0x20004);
	}
}

static inline f32 MapAreaHeight(s32 index)
{
	CamAreaInfo area = CamAreaInfo(&GetStageCameraAreaRecords()[index]);
	if (area._04->_2C == -1) {
		return 0.23f * CamInfo(&gCameraAreaState._2C[gCameraAreaState._2034[index]._00->_34]).GetEye().y;
	}
	return CamInfo(&gCameraAreaState._2C[gCameraAreaState._2034[index]._00->_34]).GetEye().y;
}

/**
 * @note Address: 0x8013B3C8
 * @note Size: 0x738
 */
extern "C" void StartMapCameraAreaHeightInterpolation(CCameraActionMap* map)
{
	CamAreaInfo area = CamAreaInfo(&gCameraAreaState._24[map->_DA8]);
	if (map->_DA8 >= 0) {
		if (!(area._04->_2E & 1)) {
			map->_E58 = 0;
		} else {
			map->_E58 = 1;
		}
		if (!(area._04->_2E & 2)) {
			map->_E5C = 0;
		} else {
			map->_E5C = 1;
		}
		if (map->_E58 == 0) {
			f32 height = MapAreaHeight(map->_DA8);
			Vec target = (gGame.GetPlayer())->GetCameraTarget();
			map->_E30  = &map->_E14;
			map->_E30->Start(map->mEye.y - target.y - map->_E98, height, 1.0f);
			if (map->_E5C == 0) {
				map->_E54 = 13.0f;
			}
			if (map->_E5C == 1) {
				map->_E54 = height;
			}
		}
		if (map->_E58 == 1) {
			f32 height = MapAreaHeight(map->_DA8);
			map->_E50  = &map->_E34;
			map->_E50->Start(map->mEye.y - map->_E98, height, 1.0f);
			if (map->_E5C == 0) {
				map->_E54 = 13.0f;
			}
			if (map->_E5C == 1) {
				map->_E54 = height;
			}
		}
	} else if (map->_DA8 == -1) {
		if (map->_E58 == 0 || (map->_E58 == 1 && map->_E5C == 0)) {
			map->_E58  = 0;
			Vec target = (gGame.GetPlayer())->GetCameraTarget();
			map->_E30  = &map->_E14;
			map->_E30->Start(map->mEye.y - target.y - map->_E98, map->_E54, 1.0f);
		}
		if (map->_E58 == 1 && map->_E5C == 1) {
			map->_E50 = &map->_E34;
			map->_E50->Start(map->mEye.y - map->_E98, map->_E54, 1.0f);
		}
	}
}
/*
    stwu     r1, -0x100(r1)
    mflr     r0
    stw      r0, 0x104(r1)
    stfd     f31, 0xf0(r1)
    psq_st   f31, 0xf8(r1), 0, qr0
    stw      r31, 0xec(r1)
    stw      r30, 0xe8(r1)
    mr       r31, r3
    lis      r3, gCameraAreaState@ha
    lwz      r0, 0xda8(r31)
    addi     r4, r3, gCameraAreaState@l
    lis      r3, __vt__11CamAreaInfo@ha
    lwz      r5, 0x24(r4)
    mulli    r4, r0, 0x34
    addic.   r0, r1, 0x48
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x48(r1)
    add      r3, r5, r4
    stw      r3, 0x4c(r1)
    stw      r0, 0x50(r1)
    stw      r3, 0x54(r1)
    beq      .L_8013B424
    stw      r0, 0x48(r1)
.L_8013B424:
    lwz      r0, 0xda8(r31)
    cmpwi    r0, 0x0
    blt      .L_8013B9F8
    lwz      r3, 0x54(r1)
    lhz      r0, 0x2e(r3)
    clrlwi.  r0, r0, 31
    bne      .L_8013B44C
    li       r0, 0x0
    stw      r0, 0xe58(r31)
    b        .L_8013B454
.L_8013B44C:
    li       r0, 0x1
    stw      r0, 0xe58(r31)
.L_8013B454:
    lwz      r3, 0x54(r1)
    lhz      r0, 0x2e(r3)
    rlwinm.  r0, r0, 0, 30, 30
    bne      .L_8013B470
    li       r0, 0x0
    stw      r0, 0xe5c(r31)
    b        .L_8013B478
.L_8013B470:
    li       r0, 0x1
    stw      r0, 0xe5c(r31)
.L_8013B478:
    lwz      r0, 0xe58(r31)
    cmpwi    r0, 0x0
    bne      .L_8013B74C
    lwz      r30, 0xda8(r31)
    bl       GetStageCameraAreaRecords
    mulli    r4, r30, 0x34
    lis      r5, __vt__11CamAreaInfo@ha
    addic.   r0, r1, 0x30
    addi     r5, r5, __vt__11CamAreaInfo@l
    add      r0, r3, r4
    stw      r5, 0x30(r1)
    stw      r0, 0x34(r1)
    stw      r5, 0x28(r1)
    stw      r0, 0x2c(r1)
    beq      .L_8013B4B8
    stw      r5, 0x30(r1)
.L_8013B4B8:
    lwz      r3, 0x2c(r1)
    lha      r0, 0x2c(r3)
    cmpwi    r0, -0x1
    bne      .L_8013B5D0
    mulli    r0, r30, 0x1c
    lis      r4, 0x4330
    lis      r5, gCameraAreaState@ha
    stw      r4, 0xd8(r1)
    lis      r3, lbl_805C0190@ha
    lfd      f6, lbl_8065E9E8@sda21(r0)
    addi     r8, r5, gCameraAreaState@l
    addi     r3, r3, lbl_805C0190@l
    add      r5, r8, r0
    lwz      r7, 0x4(r3)
    lwz      r6, 0x2034(r5)
    lis      r5, __vt__7CamInfo@ha
    lwz      r10, 0x2c(r8)
    addi     r9, r5, __vt__7CamInfo@l
    lwz      r5, 0x34(r6)
    addic.   r0, r1, 0x38
    lwz      r8, 0x0(r3)
    slwi     r0, r5, 5
    lwz      r6, 0x8(r3)
    add      r10, r10, r0
    stw      r4, 0xd0(r1)
    lwz      r0, 0x8(r10)
    lwz      r5, 0x4(r10)
    xoris    r3, r0, 0x8000
    lwz      r0, 0xc(r10)
    stw      r3, 0xdc(r1)
    xoris    r3, r5, 0x8000
    xoris    r0, r0, 0x8000
    lfs      f4, lbl_8065E9DC@sda21(r0)
    lfd      f0, 0xd8(r1)
    stw      r3, 0xd4(r1)
    fsubs    f0, f0, f6
    lfs      f1, lbl_8065E9E0@sda21(r0)
    lfd      f2, 0xd0(r1)
    stw      r0, 0xe4(r1)
    fsubs    f5, f2, f6
    stw      r4, 0xe0(r1)
    fdivs    f3, f0, f4
    lfd      f0, 0xe0(r1)
    stw      r7, 0x80(r1)
    stw      r8, 0x7c(r1)
    stw      r6, 0x84(r1)
    stw      r9, 0x38(r1)
    fsubs    f2, f0, f6
    stfs     f3, 0x80(r1)
    fdivs    f3, f5, f4
    lwz      r0, 0x80(r1)
    stw      r10, 0x3c(r1)
    stw      r0, 0xa4(r1)
    lfs      f0, 0xa4(r1)
    fdivs    f2, f2, f4
    stfs     f3, 0x7c(r1)
    lwz      r0, 0x7c(r1)
    stw      r0, 0xa0(r1)
    stfs     f2, 0x84(r1)
    fmuls    f31, f1, f0
    lwz      r0, 0x84(r1)
    stw      r0, 0xa8(r1)
    beq      .L_8013B5B8
    stw      r9, 0x38(r1)
.L_8013B5B8:
    addic.   r0, r1, 0x28
    beq      .L_8013B6CC
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x28(r1)
    b        .L_8013B6CC
.L_8013B5D0:
    mulli    r0, r30, 0x1c
    lis      r4, 0x4330
    lis      r5, gCameraAreaState@ha
    stw      r4, 0xd8(r1)
    lis      r3, lbl_805C0190@ha
    lfd      f4, lbl_8065E9E8@sda21(r0)
    addi     r8, r5, gCameraAreaState@l
    addi     r3, r3, lbl_805C0190@l
    add      r5, r8, r0
    lwz      r7, 0x4(r3)
    lwz      r6, 0x2034(r5)
    lis      r5, __vt__7CamInfo@ha
    lwz      r10, 0x2c(r8)
    addi     r9, r5, __vt__7CamInfo@l
    lwz      r5, 0x34(r6)
    addic.   r0, r1, 0x40
    lwz      r8, 0x0(r3)
    slwi     r0, r5, 5
    lwz      r6, 0x8(r3)
    add      r10, r10, r0
    stw      r4, 0xe0(r1)
    lwz      r0, 0x8(r10)
    lwz      r5, 0x4(r10)
    xoris    r3, r0, 0x8000
    lwz      r0, 0xc(r10)
    stw      r3, 0xdc(r1)
    xoris    r3, r5, 0x8000
    xoris    r0, r0, 0x8000
    lfs      f2, lbl_8065E9DC@sda21(r0)
    stw      r3, 0xe4(r1)
    lfd      f0, 0xd8(r1)
    lfd      f1, 0xe0(r1)
    fsubs    f0, f0, f4
    stw      r0, 0xd4(r1)
    fsubs    f3, f1, f4
    stw      r4, 0xd0(r1)
    fdivs    f1, f0, f2
    lfd      f0, 0xd0(r1)
    stw      r7, 0x74(r1)
    stw      r8, 0x70(r1)
    stw      r6, 0x78(r1)
    stw      r9, 0x40(r1)
    fsubs    f0, f0, f4
    stfs     f1, 0x74(r1)
    fdivs    f1, f3, f2
    lwz      r0, 0x74(r1)
    stw      r10, 0x44(r1)
    stw      r0, 0xb0(r1)
    lfs      f31, 0xb0(r1)
    fdivs    f0, f0, f2
    stfs     f1, 0x70(r1)
    lwz      r0, 0x70(r1)
    stw      r0, 0xac(r1)
    stfs     f0, 0x78(r1)
    lwz      r0, 0x78(r1)
    stw      r0, 0xb4(r1)
    beq      .L_8013B6B8
    stw      r9, 0x40(r1)
.L_8013B6B8:
    addic.   r0, r1, 0x28
    beq      .L_8013B6CC
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x28(r1)
.L_8013B6CC:
    lis      r4, gGame@ha
    addi     r3, r1, 0xc4
    addi     r4, r4, gGame@l
    addis    r4, r4, 0x1
    lwz      r4, -0x6944(r4)
    lwz      r12, 0x34(r4)
    lwz      r12, 0xec(r12)
    mtctr    r12
    bctrl
    lfs      f3, 0xc8(r1)
    addi     r0, r31, 0xe14
    lfs      f1, 0x4(r31)
    fmr      f2, f31
    lfs      f0, 0xe98(r31)
    fsubs    f1, f1, f3
    lfs      f3, lbl_8065E9A8@sda21(r0)
    stw      r0, 0xe30(r31)
    lwz      r3, 0xe30(r31)
    fsubs    f1, f1, f0
    lwz      r12, 0x0(r3)
    lwz      r12, 0xc(r12)
    mtctr    r12
    bctrl
    lwz      r0, 0xe5c(r31)
    cmpwi    r0, 0x0
    bne      .L_8013B73C
    lfs      f0, lbl_8065E9E4@sda21(r0)
    stfs     f0, 0xe54(r31)
.L_8013B73C:
    lwz      r0, 0xe5c(r31)
    cmpwi    r0, 0x1
    bne      .L_8013B74C
    stfs     f31, 0xe54(r31)
.L_8013B74C:
    lwz      r0, 0xe58(r31)
    cmpwi    r0, 0x1
    bne      .L_8013BACC
    lwz      r30, 0xda8(r31)
    bl       GetStageCameraAreaRecords
    mulli    r4, r30, 0x34
    lis      r5, __vt__11CamAreaInfo@ha
    addic.   r0, r1, 0x10
    addi     r5, r5, __vt__11CamAreaInfo@l
    add      r0, r3, r4
    stw      r5, 0x10(r1)
    stw      r0, 0x14(r1)
    stw      r5, 0x8(r1)
    stw      r0, 0xc(r1)
    beq      .L_8013B78C
    stw      r5, 0x10(r1)
.L_8013B78C:
    lwz      r3, 0xc(r1)
    lha      r0, 0x2c(r3)
    cmpwi    r0, -0x1
    bne      .L_8013B8A4
    mulli    r0, r30, 0x1c
    lis      r4, 0x4330
    lis      r5, gCameraAreaState@ha
    stw      r4, 0xd8(r1)
    lis      r3, lbl_805C0190@ha
    lfd      f6, lbl_8065E9E8@sda21(r0)
    addi     r8, r5, gCameraAreaState@l
    addi     r3, r3, lbl_805C0190@l
    add      r5, r8, r0
    lwz      r7, 0x4(r3)
    lwz      r6, 0x2034(r5)
    lis      r5, __vt__7CamInfo@ha
    lwz      r10, 0x2c(r8)
    addi     r9, r5, __vt__7CamInfo@l
    lwz      r5, 0x34(r6)
    addic.   r0, r1, 0x18
    lwz      r8, 0x0(r3)
    slwi     r0, r5, 5
    lwz      r6, 0x8(r3)
    add      r10, r10, r0
    stw      r4, 0xe0(r1)
    lwz      r0, 0x8(r10)
    lwz      r5, 0x4(r10)
    xoris    r3, r0, 0x8000
    lwz      r0, 0xc(r10)
    stw      r3, 0xdc(r1)
    xoris    r3, r5, 0x8000
    xoris    r0, r0, 0x8000
    lfs      f4, lbl_8065E9DC@sda21(r0)
    lfd      f0, 0xd8(r1)
    stw      r3, 0xe4(r1)
    fsubs    f0, f0, f6
    lfs      f1, lbl_8065E9E0@sda21(r0)
    lfd      f2, 0xe0(r1)
    stw      r0, 0xd4(r1)
    fsubs    f5, f2, f6
    stw      r4, 0xd0(r1)
    fdivs    f3, f0, f4
    lfd      f0, 0xd0(r1)
    stw      r7, 0x68(r1)
    stw      r8, 0x64(r1)
    stw      r6, 0x6c(r1)
    stw      r9, 0x18(r1)
    fsubs    f2, f0, f6
    stfs     f3, 0x68(r1)
    fdivs    f3, f5, f4
    lwz      r0, 0x68(r1)
    stw      r10, 0x1c(r1)
    stw      r0, 0x8c(r1)
    lfs      f0, 0x8c(r1)
    fdivs    f2, f2, f4
    stfs     f3, 0x64(r1)
    lwz      r0, 0x64(r1)
    stw      r0, 0x88(r1)
    stfs     f2, 0x6c(r1)
    fmuls    f31, f1, f0
    lwz      r0, 0x6c(r1)
    stw      r0, 0x90(r1)
    beq      .L_8013B88C
    stw      r9, 0x18(r1)
.L_8013B88C:
    addic.   r0, r1, 0x8
    beq      .L_8013B9A0
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x8(r1)
    b        .L_8013B9A0
.L_8013B8A4:
    mulli    r0, r30, 0x1c
    lis      r4, 0x4330
    lis      r5, gCameraAreaState@ha
    stw      r4, 0xd8(r1)
    lis      r3, lbl_805C0190@ha
    lfd      f4, lbl_8065E9E8@sda21(r0)
    addi     r8, r5, gCameraAreaState@l
    addi     r3, r3, lbl_805C0190@l
    add      r5, r8, r0
    lwz      r7, 0x4(r3)
    lwz      r6, 0x2034(r5)
    lis      r5, __vt__7CamInfo@ha
    lwz      r10, 0x2c(r8)
    addi     r9, r5, __vt__7CamInfo@l
    lwz      r5, 0x34(r6)
    addic.   r0, r1, 0x20
    lwz      r8, 0x0(r3)
    slwi     r0, r5, 5
    lwz      r6, 0x8(r3)
    add      r10, r10, r0
    stw      r4, 0xe0(r1)
    lwz      r0, 0x8(r10)
    lwz      r5, 0x4(r10)
    xoris    r3, r0, 0x8000
    lwz      r0, 0xc(r10)
    stw      r3, 0xdc(r1)
    xoris    r3, r5, 0x8000
    xoris    r0, r0, 0x8000
    lfs      f2, lbl_8065E9DC@sda21(r0)
    stw      r3, 0xe4(r1)
    lfd      f0, 0xd8(r1)
    lfd      f1, 0xe0(r1)
    fsubs    f0, f0, f4
    stw      r0, 0xd4(r1)
    fsubs    f3, f1, f4
    stw      r4, 0xd0(r1)
    fdivs    f1, f0, f2
    lfd      f0, 0xd0(r1)
    stw      r7, 0x5c(r1)
    stw      r8, 0x58(r1)
    stw      r6, 0x60(r1)
    stw      r9, 0x20(r1)
    fsubs    f0, f0, f4
    stfs     f1, 0x5c(r1)
    fdivs    f1, f3, f2
    lwz      r0, 0x5c(r1)
    stw      r10, 0x24(r1)
    stw      r0, 0x98(r1)
    lfs      f31, 0x98(r1)
    fdivs    f0, f0, f2
    stfs     f1, 0x58(r1)
    lwz      r0, 0x58(r1)
    stw      r0, 0x94(r1)
    stfs     f0, 0x60(r1)
    lwz      r0, 0x60(r1)
    stw      r0, 0x9c(r1)
    beq      .L_8013B98C
    stw      r9, 0x20(r1)
.L_8013B98C:
    addic.   r0, r1, 0x8
    beq      .L_8013B9A0
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x8(r1)
.L_8013B9A0:
    lfs      f1, 0x4(r31)
    addi     r0, r31, 0xe34
    lfs      f0, 0xe98(r31)
    fmr      f2, f31
    lfs      f3, lbl_8065E9A8@sda21(r0)
    stw      r0, 0xe50(r31)
    fsubs    f1, f1, f0
    lwz      r3, 0xe50(r31)
    lwz      r12, 0x0(r3)
    lwz      r12, 0xc(r12)
    mtctr    r12
    bctrl
    lwz      r0, 0xe5c(r31)
    cmpwi    r0, 0x0
    bne      .L_8013B9E4
    lfs      f0, lbl_8065E9E4@sda21(r0)
    stfs     f0, 0xe54(r31)
.L_8013B9E4:
    lwz      r0, 0xe5c(r31)
    cmpwi    r0, 0x1
    bne      .L_8013BACC
    stfs     f31, 0xe54(r31)
    b        .L_8013BACC
.L_8013B9F8:
    cmpwi    r0, -0x1
    bne      .L_8013BACC
    lwz      r0, 0xe58(r31)
    cmpwi    r0, 0x0
    beq      .L_8013BA20
    cmpwi    r0, 0x1
    bne      .L_8013BA84
    lwz      r0, 0xe5c(r31)
    cmpwi    r0, 0x0
    bne      .L_8013BA84
.L_8013BA20:
    li       r0, 0x0
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    stw      r0, 0xe58(r31)
    addis    r4, r3, 0x1
    addi     r3, r1, 0xb8
    lwz      r4, -0x6944(r4)
    lwz      r12, 0x34(r4)
    lwz      r12, 0xec(r12)
    mtctr    r12
    bctrl
    lfs      f2, 0xbc(r1)
    addi     r0, r31, 0xe14
    lfs      f1, 0x4(r31)
    lfs      f0, 0xe98(r31)
    fsubs    f1, f1, f2
    lfs      f3, lbl_8065E9A8@sda21(r0)
    stw      r0, 0xe30(r31)
    lwz      r3, 0xe30(r31)
    fsubs    f1, f1, f0
    lfs      f2, 0xe54(r31)
    lwz      r12, 0x0(r3)
    lwz      r12, 0xc(r12)
    mtctr    r12
    bctrl
.L_8013BA84:
    lwz      r0, 0xe58(r31)
    cmpwi    r0, 0x1
    bne      .L_8013BACC
    lwz      r0, 0xe5c(r31)
    cmpwi    r0, 0x1
    bne      .L_8013BACC
    lfs      f1, 0x4(r31)
    addi     r0, r31, 0xe34
    lfs      f0, 0xe98(r31)
    lfs      f3, lbl_8065E9A8@sda21(r0)
    stw      r0, 0xe50(r31)
    fsubs    f1, f1, f0
    lwz      r3, 0xe50(r31)
    lfs      f2, 0xe54(r31)
    lwz      r12, 0x0(r3)
    lwz      r12, 0xc(r12)
    mtctr    r12
    bctrl
.L_8013BACC:
    addic.   r0, r1, 0x50
    beq      .L_8013BAE0
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x50(r1)
.L_8013BAE0:
    psq_l    f31, 0xf8(r1), 0, qr0
    lwz      r0, 0x104(r1)
    lfd      f31, 0xf0(r1)
    lwz      r31, 0xec(r1)
    lwz      r30, 0xe8(r1)
    mtlr     r0
    addi     r1, r1, 0x100
    blr
*/

extern "C" s32 CollectMapCameraAreaIntersections(CCameraActionMap* map, const Vec& from, const Vec& to, CamMapEntry* out, s32 type);

/**
 * @note Address: 0x8013BB00
 * @note Size: 0x1E4
 */
extern "C" s32 SelectMapCameraArea(CCameraActionMap* map)
{
	CVec target = map->mTarget->mMoveTranslate;
	CVec eye    = map->mEye;
	eye.y       = target.y;

	map->_DA0 = CollectMapCameraAreaIntersections(map, eye, target, map->_9A0, 1);

	s32 eyeArea = FindEnabledCameraAreaContainingPoint(&gCameraAreaState, eye);
	if (CamAreaInfo(&gCameraAreaState._24[eyeArea]).Is_2C_1()) {
		eyeArea = -1;
	}

	s32 targetArea = FindEnabledCameraAreaContainingPoint(&gCameraAreaState, target);
	if (CamAreaInfo(&gCameraAreaState._24[targetArea]).Is_2C_1()) {
		targetArea = -1;
	}

	s32 mapArea = map->_DA0 ? map->_9A0[0]._0C : -1;

	if (eyeArea >= 0) {
		map->_DA8 = eyeArea;
	} else if (targetArea >= 0) {
		map->_DA8 = targetArea;
	} else if (mapArea >= 0) {
		map->_DA8 = mapArea;
	} else {
		map->_DA8 = -1;
	}
	return map->_DA8;
}

// SelectMapCameraArea
/*
    stwu     r1, -0x60(r1)
    mflr     r0
    lis      r4, __vt__4CVec@ha
    li       r7, 0x1
    stw      r0, 0x64(r1)
    addi     r9, r4, __vt__4CVec@l
    addi     r4, r1, 0x30
    addi     r5, r1, 0x40
    stw      r31, 0x5c(r1)
    stw      r30, 0x58(r1)
    stw      r29, 0x54(r1)
    mr       r29, r3
    addi     r6, r29, 0x9a0
    lwz      r10, 0x44(r3)
    lwz      r8, 0xc8(r10)
    lwz      r0, 0xcc(r10)
    stw      r0, 0x44(r1)
    stw      r8, 0x40(r1)
    lfs      f0, 0x44(r1)
    lwz      r0, 0xd0(r10)
    stw      r0, 0x48(r1)
    stw      r9, 0x4c(r1)
    lwz      r8, 0x0(r3)
    lwz      r0, 0x4(r3)
    stw      r8, 0x30(r1)
    stw      r0, 0x34(r1)
    lwz      r0, 0x8(r3)
    stw      r0, 0x38(r1)
    stw      r9, 0x3c(r1)
    stfs     f0, 0x34(r1)
    bl       CollectMapCameraAreaIntersections
    stw      r3, 0xda0(r29)
    lis      r3, gCameraAreaState@ha
    addi     r3, r3, gCameraAreaState@l
    addi     r4, r1, 0x24
    lwz      r6, 0x30(r1)
    lwz      r5, 0x34(r1)
    lwz      r0, 0x38(r1)
    stw      r6, 0x24(r1)
    stw      r5, 0x28(r1)
    stw      r0, 0x2c(r1)
    bl       FindEnabledCameraAreaContainingPoint
    mr       r30, r3
    lis      r4, gCameraAreaState@ha
    addi     r31, r4, gCameraAreaState@l
    lis      r3, __vt__11CamAreaInfo@ha
    mulli    r0, r30, 0x34
    lwz      r5, 0x24(r31)
    addi     r4, r3, __vt__11CamAreaInfo@l
    stw      r4, 0x10(r1)
    add      r5, r5, r0
    lha      r0, 0x2c(r5)
    stw      r5, 0x14(r1)
    subfic   r0, r0, 0x1
    cntlzw   r3, r0
    addic.   r0, r1, 0x10
    srwi     r0, r3, 5
    beq      .L_8013BBEC
    stw      r4, 0x10(r1)
.L_8013BBEC:
    clrlwi.  r0, r0, 24
    beq      .L_8013BBF8
    li       r30, -0x1
.L_8013BBF8:
    lwz      r4, 0x40(r1)
    lis      r3, gCameraAreaState@ha
    lwz      r5, 0x44(r1)
    addi     r3, r3, gCameraAreaState@l
    lwz      r0, 0x48(r1)
    stw      r4, 0x18(r1)
    addi     r4, r1, 0x18
    stw      r5, 0x1c(r1)
    stw      r0, 0x20(r1)
    bl       FindEnabledCameraAreaContainingPoint
    mulli    r0, r3, 0x34
    lwz      r6, 0x24(r31)
    lis      r4, __vt__11CamAreaInfo@ha
    add      r6, r6, r0
    addi     r5, r4, __vt__11CamAreaInfo@l
    lha      r0, 0x2c(r6)
    stw      r5, 0x8(r1)
    subfic   r4, r0, 0x1
    addic.   r0, r1, 0x8
    stw      r6, 0xc(r1)
    cntlzw   r0, r4
    srwi     r0, r0, 5
    beq      .L_8013BC58
    stw      r5, 0x8(r1)
.L_8013BC58:
    clrlwi.  r0, r0, 24
    beq      .L_8013BC64
    li       r3, -0x1
.L_8013BC64:
    lwz      r0, 0xda0(r29)
    cmpwi    r0, 0x0
    beq      .L_8013BC78
    lwz      r0, 0x9ac(r29)
    b        .L_8013BC7C
.L_8013BC78:
    li       r0, -0x1
.L_8013BC7C:
    cmpwi    r30, 0x0
    blt      .L_8013BC8C
    stw      r30, 0xda8(r29)
    b        .L_8013BCB4
.L_8013BC8C:
    cmpwi    r3, 0x0
    blt      .L_8013BC9C
    stw      r3, 0xda8(r29)
    b        .L_8013BCB4
.L_8013BC9C:
    cmpwi    r0, 0x0
    blt      .L_8013BCAC
    stw      r0, 0xda8(r29)
    b        .L_8013BCB4
.L_8013BCAC:
    li       r0, -0x1
    stw      r0, 0xda8(r29)
.L_8013BCB4:
    lis      r4, __vt__4CVec@ha
    lwz      r3, 0xda8(r29)
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x3c(r1)
    stw      r0, 0x4c(r1)
    lwz      r31, 0x5c(r1)
    lwz      r30, 0x58(r1)
    lwz      r0, 0x64(r1)
    lwz      r29, 0x54(r1)
    mtlr     r0
    addi     r1, r1, 0x60
    blr
*/

extern "C" s32 SelectMapCameraArea(CCameraActionMap* map);
extern "C" void StartMapCameraAreaHeightInterpolation(CCameraActionMap* map);

/**
 * @note Address: 0x8013BCE4
 * @note Size: 0x44
 */
extern "C" void RefreshMapCameraArea(CCameraActionMap* map)
{
	map->_DA4 = map->_DA8;
	SelectMapCameraArea(map);
	StartMapCameraAreaHeightInterpolation(map);
	map->_DA4 = map->_DA8;
}

/**
 * @note Address: 0x8013BD28
 * @note Size: 0x374
 */
extern "C" void GetMapCameraEyeBehindTarget(CCameraActionMap* map, Vec& out, s32 mode)
{
	CVec result;
	if (mode != 1) {
		if (map->mTarget != NULL) {
			Mtx mtx;
			PSMTXCopy(HSD_JObjGetMtxPtr(map->mTarget->mJObj), mtx);
			CVec pos(0.0f, 13.0f, -map->_80);
			PSMTXMultVec(mtx, &pos, &pos);
			pos.y  = map->_E30->Get();
			result = pos;
		}
	} else if (map->mTarget != NULL) {
		f32 x, z;
		XObj* target = map->mTarget;
		if (target != NULL && (target->mStatus.mFlags & 1)) {
			Mtx mtx;
			PSMTXCopy(HSD_JObjGetMtxPtr(target->mJObj), mtx);
			CVec pos(0.0f, 13.0f, -1.0f);
			PSMTXMultVec(mtx, &pos, &pos);
			x = pos.x;
			z = pos.z;
		}
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		CVec eye                     = camera->mCamera.mEye;
		CVec interest                = camera->mCamera.mInterest;
		f32 angle                    = HALF_PI + (f32)atan2(interest.x - eye.x, interest.z - eye.z);
		f32 distance                 = map->_80;
		result.x                     = x + distance * (f32)cos(angle);
		result.z                     = z + distance * -(f32)sin(angle);
		result.y                     = 13.0f + map->mTarget->GetCameraTarget().y;
	}
	out.x = result.x;
	out.y = result.y;
	out.z = result.z;
}
/*
    stwu     r1, -0x120(r1)
    mflr     r0
    stw      r0, 0x124(r1)
    stfd     f31, 0x110(r1)
    psq_st   f31, 0x118(r1), 0, qr0
    stfd     f30, 0x100(r1)
    psq_st   f30, 0x108(r1), 0, qr0
    stfd     f29, 0xf0(r1)
    psq_st   f29, 0xf8(r1), 0, qr0
    stfd     f28, 0xe0(r1)
    psq_st   f28, 0xe8(r1), 0, qr0
    stw      r31, 0xdc(r1)
    stw      r30, 0xd8(r1)
    stw      r29, 0xd4(r1)
    lis      r6, __vt__4CVec@ha
    cmpwi    r5, 0x1
    addi     r0, r6, __vt__4CVec@l
    mr       r30, r3
    stw      r0, 0x60(r1)
    mr       r31, r4
    beq      .L_8013BE70
    lwz      r3, 0x44(r30)
    cmplwi   r3, 0x0
    beq      .L_8013C03C
    lwz      r29, 0x0(r3)
    cmplwi   r29, 0x0
    bne      .L_8013BDA4
    li       r3, lbl_8065E9F0@sda21
    li       r4, 0x47c
    li       r5, lbl_8065E9F8@sda21
    bl       __assert
.L_8013BDA4:
    cmplwi   r29, 0x0
    beq      .L_8013BDEC
    bne      .L_8013BDC0
    li       r3, lbl_8065E9F0@sda21
    li       r4, 0x25d
    li       r5, lbl_8065E9F8@sda21
    bl       __assert
.L_8013BDC0:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_8013BDDC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_8013BDDC
    li       r3, 0x1
.L_8013BDDC:
    clrlwi.  r0, r3, 24
    beq      .L_8013BDEC
    mr       r3, r29
    bl       fn_801C8730
.L_8013BDEC:
    addi     r3, r29, 0x44
    addi     r4, r1, 0x94
    bl       PSMTXCopy
    lfs      f0, 0x80(r30)
    lis      r3, __vt__4CVec@ha
    lfs      f1, lbl_8065E994@sda21(r0)
    addi     r0, r3, __vt__4CVec@l
    fneg     f2, f0
    lfs      f0, lbl_8065E9E4@sda21(r0)
    addi     r4, r1, 0x44
    stw      r0, 0x50(r1)
    mr       r5, r4
    addi     r3, r1, 0x94
    stfs     f1, 0x44(r1)
    stfs     f0, 0x48(r1)
    stfs     f2, 0x4c(r1)
    bl       PSMTXMultVec
    lwz      r3, 0xe30(r30)
    lwz      r12, 0x0(r3)
    lwz      r12, 0x18(r12)
    mtctr    r12
    bctrl
    frsp     f2, f1
    lfs      f3, 0x44(r1)
    lfs      f0, 0x4c(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stfs     f1, 0x48(r1)
    stfs     f3, 0x54(r1)
    stfs     f2, 0x58(r1)
    stfs     f0, 0x5c(r1)
    stw      r0, 0x50(r1)
    b        .L_8013C03C
.L_8013BE70:
    lwz      r6, 0x44(r30)
    cmplwi   r6, 0x0
    beq      .L_8013C03C
    beq      .L_8013BF60
    lwz      r0, 0x228(r6)
    li       r4, 0x0
    lwz      r5, 0x22c(r6)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8013BF60
    lwz      r29, 0x0(r6)
    cmplwi   r29, 0x0
    bne      .L_8013BEC4
    li       r3, lbl_8065E9F0@sda21
    li       r4, 0x47c
    li       r5, lbl_8065E9F8@sda21
    bl       __assert
.L_8013BEC4:
    cmplwi   r29, 0x0
    beq      .L_8013BF0C
    bne      .L_8013BEE0
    li       r3, lbl_8065E9F0@sda21
    li       r4, 0x25d
    li       r5, lbl_8065E9F8@sda21
    bl       __assert
.L_8013BEE0:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_8013BEFC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_8013BEFC
    li       r3, 0x1
.L_8013BEFC:
    clrlwi.  r0, r3, 24
    beq      .L_8013BF0C
    mr       r3, r29
    bl       fn_801C8730
.L_8013BF0C:
    addi     r3, r29, 0x44
    addi     r4, r1, 0x64
    bl       PSMTXCopy
    lfs      f2, lbl_8065E994@sda21(r0)
    lis      r3, __vt__4CVec@ha
    lfs      f1, lbl_8065E9E4@sda21(r0)
    addi     r0, r3, __vt__4CVec@l
    lfs      f0, lbl_8065EA00@sda21(r0)
    addi     r4, r1, 0x8
    stw      r0, 0x14(r1)
    mr       r5, r4
    addi     r3, r1, 0x64
    stfs     f2, 0x8(r1)
    stfs     f1, 0xc(r1)
    stfs     f0, 0x10(r1)
    bl       PSMTXMultVec
    lis      r3, __vt__4CVec@ha
    lfs      f30, 0x8(r1)
    addi     r0, r3, __vt__4CVec@l
    lfs      f29, 0x10(r1)
    stw      r0, 0x14(r1)
.L_8013BF60:
    lis      r4, gGame@ha
    lis      r3, __vt__4CVec@ha
    addi     r4, r4, gGame@l
    addis    r7, r4, 0xe
    addi     r5, r3, __vt__4CVec@l
    lwz      r6, 0x5dcc(r7)
    lwz      r3, 0x5dd4(r7)
    lwz      r4, 0x5ddc(r7)
    lwz      r0, 0x5de4(r7)
    stw      r6, 0x34(r1)
    lwz      r6, 0x5dd0(r7)
    stw      r3, 0x3c(r1)
    lwz      r3, 0x5de0(r7)
    stw      r4, 0x24(r1)
    lfs      f1, 0x34(r1)
    stw      r0, 0x2c(r1)
    lfs      f3, 0x24(r1)
    lfs      f2, 0x2c(r1)
    lfs      f0, 0x3c(r1)
    fsubs    f1, f3, f1
    stw      r6, 0x38(r1)
    fsubs    f2, f2, f0
    stw      r5, 0x40(r1)
    stw      r3, 0x28(r1)
    stw      r5, 0x30(r1)
    bl       atan2
    frsp     f1, f1
    lfs      f0, lbl_8065E98C@sda21(r0)
    lfs      f31, 0x80(r30)
    fadds    f28, f0, f1
    fmr      f1, f28
    bl       cos
    frsp     f0, f1
    fmr      f1, f28
    fmadds   f0, f31, f0, f30
    stfs     f0, 0x54(r1)
    bl       sin
    frsp     f0, f1
    lwz      r4, 0x44(r30)
    addi     r3, r1, 0x18
    lwz      r12, 0x34(r4)
    fneg     f0, f0
    lwz      r12, 0xec(r12)
    fmadds   f0, f31, f0, f29
    stfs     f0, 0x5c(r1)
    mtctr    r12
    bctrl
    lfs      f1, 0x1c(r1)
    lis      r3, __vt__4CVec@ha
    lfs      f0, lbl_8065E9E4@sda21(r0)
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x30(r1)
    fadds    f0, f0, f1
    stw      r0, 0x40(r1)
    stfs     f0, 0x58(r1)
.L_8013C03C:
    lfs      f0, 0x54(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0x58(r1)
    stfs     f0, 0x0(r31)
    lfs      f0, 0x5c(r1)
    stfs     f1, 0x4(r31)
    stfs     f0, 0x8(r31)
    stw      r0, 0x60(r1)
    psq_l    f31, 0x118(r1), 0, qr0
    lfd      f31, 0x110(r1)
    psq_l    f30, 0x108(r1), 0, qr0
    lfd      f30, 0x100(r1)
    psq_l    f29, 0xf8(r1), 0, qr0
    lfd      f29, 0xf0(r1)
    psq_l    f28, 0xe8(r1), 0, qr0
    lfd      f28, 0xe0(r1)
    lwz      r31, 0xdc(r1)
    lwz      r30, 0xd8(r1)
    lwz      r0, 0x124(r1)
    lwz      r29, 0xd4(r1)
    mtlr     r0
    addi     r1, r1, 0x120
    blr
*/

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
extern "C" void vf28__16CCameraActionMapFv()
{
	DrawMapCameraAreaDebug3D(&gCameraAreaState, 1);
}

/**
 * @note Address: 0x8013C200
 * @note Size: 0x1CC
 */
void CCameraActionMap::vf24()
{
	CGame* game = &gGame;
	DrawGsStrokeTextFormat(&game->mGs, -1, "\\B000000A0AREA(%3d,---)(%3d)\n", 408.0f, 298.0f, 0.0f, 12.0f, 12.0f, gCameraAreaState._0C,
	                       gCameraAreaState._04);
	DrawGsStrokeTextFormat(&game->mGs, -1, "\\B000000A0ACMAP2\n", 408.0f, 310.0f, 0.0f, 12.0f, 12.0f);
	DrawGsStrokeTextFormat(&game->mGs, -1, "\\B00000080CAMSTAT\t   :%8X\nCAMSTATPRE :%8X\n", 360.0f, 322.0f, 0.0f, 12.0f, 12.0f,
	                       mStatus.mFlags, mPreviousStatus.mFlags);
	DrawGsStrokeTextFormat(&game->mGs, -1, "\\B00000080CHR_InsideCll(%3d,%3d,%3d)\n", 324.0f, 346.0f, 0.0f, 12.0f, 12.0f, _EA8[0], _EA8[1],
	                       _EA8[2]);
	DrawGsStrokeTextFormat(&game->mGs, -1, "\\B00000080CAM_InsideCll(%3d,%3d,%3d)\n", 324.0f, 358.0f, 0.0f, 12.0f, 12.0f, _4F0, _4F4, _4F8);
	if (_4EC > 0) {
		DrawGsStrokeTextFormat(&game->mGs, -1, "\\B00000080CamHitCll(%3d,%3d,%3d)\n", 324.0f, 370.0f, 0.0f, 12.0f, 12.0f, _4F0, _4F4, _4F8);
	}
	if (_974 != NULL) {
		DrawGsStrokeTextFormat(&game->mGs, -1, "\\B00000080INTRSCT(%3d-%3d)\n", 324.0f, 382.0f, 0.0f, 12.0f, 12.0f, _974->_0C, _974->_10);
	}
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    lfs      f4, lbl_8065EA18@sda21(r0)
    lis      r4, gCameraAreaState@ha
    stw      r0, 0x24(r1)
    addi     r4, r4, gCameraAreaState@l
    lis      r6, lbl_8020C8C0@ha
    lis      r5, gGame@ha
    stw      r31, 0x1c(r1)
    fmr      f5, f4
    addi     r31, r6, lbl_8020C8C0@l
    lfs      f1, lbl_8065EA10@sda21(r0)
    stw      r30, 0x18(r1)
    addi     r30, r5, gGame@l
    lfs      f2, lbl_8065EA14@sda21(r0)
    stw      r29, 0x14(r1)
    mr       r29, r3
    lfs      f3, lbl_8065E994@sda21(r0)
    mr       r3, r30
    lwz      r6, 0xc(r4)
    addi     r5, r31, 0x258
    lwz      r7, 0x4(r4)
    li       r4, -0x1
    crset    cr1eq
    bl       DrawGsStrokeTextFormat
    lfs      f4, lbl_8065EA18@sda21(r0)
    lis      r3, gGame@ha
    lfs      f1, lbl_8065EA10@sda21(r0)
    addi     r3, r3, gGame@l
    fmr      f5, f4
    lfs      f2, lbl_8065EA1C@sda21(r0)
    lfs      f3, lbl_8065E994@sda21(r0)
    addi     r5, r31, 0x278
    li       r4, -0x1
    crset    cr1eq
    bl       DrawGsStrokeTextFormat
    lfs      f4, lbl_8065EA18@sda21(r0)
    lis      r3, gGame@ha
    lfs      f1, lbl_8065EA20@sda21(r0)
    addi     r3, r3, gGame@l
    fmr      f5, f4
    lfs      f2, lbl_8065EA24@sda21(r0)
    lfs      f3, lbl_8065E994@sda21(r0)
    addi     r5, r31, 0x28c
    lwz      r6, 0x11d8(r29)
    li       r4, -0x1
    lwz      r7, 0x11e0(r29)
    crset    cr1eq
    bl       DrawGsStrokeTextFormat
    lfs      f4, lbl_8065EA18@sda21(r0)
    lis      r3, gGame@ha
    lfs      f1, lbl_8065EA28@sda21(r0)
    addi     r3, r3, gGame@l
    fmr      f5, f4
    lfs      f2, lbl_8065EA2C@sda21(r0)
    lfs      f3, lbl_8065E994@sda21(r0)
    addi     r5, r31, 0x2b8
    lwz      r6, 0xea8(r29)
    li       r4, -0x1
    lwz      r7, 0xeac(r29)
    lwz      r8, 0xeb0(r29)
    crset    cr1eq
    bl       DrawGsStrokeTextFormat
    lfs      f4, lbl_8065EA18@sda21(r0)
    lis      r3, gGame@ha
    lfs      f1, lbl_8065EA28@sda21(r0)
    addi     r3, r3, gGame@l
    fmr      f5, f4
    lfs      f2, lbl_8065EA30@sda21(r0)
    lfs      f3, lbl_8065E994@sda21(r0)
    addi     r5, r31, 0x2e0
    lwz      r6, 0x4f0(r29)
    li       r4, -0x1
    lwz      r7, 0x4f4(r29)
    lwz      r8, 0x4f8(r29)
    crset    cr1eq
    bl       DrawGsStrokeTextFormat
    lwz      r0, 0x4ec(r29)
    cmpwi    r0, 0x0
    ble      .L_8013C374
    lfs      f4, lbl_8065EA18@sda21(r0)
    mr       r3, r30
    lfs      f1, lbl_8065EA28@sda21(r0)
    addi     r5, r31, 0x308
    fmr      f5, f4
    lfs      f2, lbl_8065EA34@sda21(r0)
    lfs      f3, lbl_8065E994@sda21(r0)
    li       r4, -0x1
    lwz      r6, 0x4f0(r29)
    lwz      r7, 0x4f4(r29)
    lwz      r8, 0x4f8(r29)
    crset    cr1eq
    bl       DrawGsStrokeTextFormat
.L_8013C374:
    lwz      r7, 0x974(r29)
    cmplwi   r7, 0x0
    beq      .L_8013C3B0
    lfs      f4, lbl_8065EA18@sda21(r0)
    mr       r3, r30
    lfs      f1, lbl_8065EA28@sda21(r0)
    addi     r5, r31, 0x32c
    fmr      f5, f4
    lfs      f2, lbl_8065EA38@sda21(r0)
    lfs      f3, lbl_8065E994@sda21(r0)
    li       r4, -0x1
    lwz      r6, 0xc(r7)
    lwz      r7, 0x10(r7)
    crset    cr1eq
    bl       DrawGsStrokeTextFormat
.L_8013C3B0:
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x8013C3CC
 * @note Size: 0x144
 */
extern "C" BOOL TrySetMapCameraTargetYaw(CCameraActionMap* map, f32 yaw, f32 time, f32 unused)
{
	f32 oldYaw   = map->_C0;
	f32 oldSpeed = map->_BC;

	map->_C0 = WrapAngle(yaw);
	f32 from = WrapAngle(map->mAngle.y);
	f32 to   = WrapAngle(yaw);

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
	map->_BC = diff / time;
	if (map->_BC < (2.0f * DEG2RAD)) {
		map->_C0 = oldYaw;
		map->_BC = oldSpeed;
		return FALSE;
	}
	return TRUE;
}

// eases the fov to the area's fov times one of four zoom steps. PreviousUpdate inlines it too
inline void SetFovStep(CCameraActionMap* map, s32 index, f32 time)
{
	f32 fov = gCameraAreaState.mBaseFov;
	switch (index) {
	case 0:
	default:
		map->_C8.Start(map->mFov, 0.8f * fov, time);
		break;
	case 1:
		map->_C8.Start(map->mFov, fov, time);
		break;
	case 2:
		map->_C8.Start(map->mFov, 1.5f * fov, time);
		break;
	case 3:
		map->_C8.Start(map->mFov, 2.0f * fov, time);
		break;
	}
	gCameraAreaState._14 = index;
	gCameraAreaState._18 = index;
}

/**
 * @note Address: 0x8013C510
 * @note Size: 0x104
 */
extern "C" void SetMapCameraFovStep(CCameraActionMap* map, s32 index, f32 time)
{
	SetFovStep(map, index, time);
}

extern "C" void UpdateMapCameraTurnInterpolation(CCameraActionMap* map, u32 step);

/**
 * @note Address: 0x8013C614
 * @note Size: 0x524
 */
extern "C" void UpdateMapCameraPadControls(CCameraActionMap* map, u32 step)
{
	if (gGame.mStatus.mFlags & 0xC00) {
		map->_DAC = 0;
	} else {
		map->_DAC = 1;
	}
	map->_EA0 = 0;
	CPad pad(0);
	u32 left    = (gGame.mStatus.mFlags & 0x80000000) ? 1 : 0x400000;
	u32 right   = (gGame.mStatus.mFlags & 0x80000000) ? 2 : 0x800000;
	u32 zoomIn  = (gGame.mStatus.mFlags & 0x80000000) ? 8 : 0x100000;
	u32 zoomOut = (gGame.mStatus.mFlags & 0x80000000) ? 4 : 0x200000;
	f32 turn    = 0.0f;
	if ((pad.mPad->mButton & right) && map->_DAC) {
		turn = 1.0f;
	} else if ((pad.mPad->mButton & left) && map->_DAC) {
		turn = -1.0f;
	}
	if (turn != 0.0f) {
		map->_EA0 = 1;
		SetMapCameraModeFlags(map, 0x20008);
	} else if (map->mStatus.mFlags & 8) {
		map->mStatus.mFlags &= ~8;
	}
	if (map->mTarget != NULL && map->mTarget->mJObj != NULL && (pad.mPad->mTrigger & 0x40) && map->_DAC
	    && gGame.GetPlayer()->mHang.mState != 3) {
		if (SetMapYaw(map, map->mTarget->mMoveRotate.mTarget.y, 10.0f)) {
			gGame.GetPlayer()->ResetCameraValue();
			SetMapCameraModeFlags(map, 0x20002);
		}
	}
	UpdateMapCameraTurnInterpolation(map, step);
	s32 previous = gCameraAreaState._18;
	s32 mode     = gCameraAreaState._14;
	if ((pad.mPad->mTrigger & zoomIn) && map->_DAC) {
		if (mode > 1) {
			mode--;
		}
	} else if ((pad.mPad->mTrigger & zoomOut) && map->_DAC) {
		if (mode < 2) {
			mode++;
		}
	}
	if (mode != previous) {
		SetFovStep(map, mode, 15.0f);
	}
	if (!map->_C8.Update(step, 0.0f)) {
		map->SetFov(map->_C8.Get());
	}
}
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    lis      r5, gGame@ha
    li       r6, 0xc00
    stw      r0, 0x34(r1)
    stmw     r27, 0x1c(r1)
    addi     r27, r5, gGame@l
    mr       r28, r3
    li       r5, 0x0
    mr       r29, r4
    lwz      r0, 0x1b10(r27)
    lwz      r3, 0x1b14(r27)
    and      r0, r0, r5
    and      r3, r3, r6
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_8013C664
    stw      r5, 0xdac(r28)
    b        .L_8013C66C
.L_8013C664:
    li       r0, 0x1
    stw      r0, 0xdac(r28)
.L_8013C66C:
    li       r0, 0x0
    addi     r3, r1, 0x8
    stw      r0, 0xea0(r28)
    li       r4, 0x0
    bl       fn_8019E3C8
    lwz      r4, 0x1b10(r27)
    li       r0, 0x0
    lwz      r5, 0x1b14(r27)
    lis      r3, 0x8000
    and      r6, r4, r0
    and      r5, r5, r3
    xor      r3, r5, r0
    xor      r0, r6, r0
    or.      r0, r3, r0
    beq      .L_8013C6B0
    li       r4, 0x1
    b        .L_8013C6B4
.L_8013C6B0:
    lis      r4, 0x40
.L_8013C6B4:
    li       r0, 0x0
    xor      r3, r5, r0
    xor      r0, r6, r0
    or.      r0, r3, r0
    beq      .L_8013C6D0
    li       r7, 0x2
    b        .L_8013C6D4
.L_8013C6D0:
    lis      r7, 0x80
.L_8013C6D4:
    li       r0, 0x0
    xor      r3, r5, r0
    xor      r0, r6, r0
    or.      r0, r3, r0
    beq      .L_8013C6F0
    li       r31, 0x8
    b        .L_8013C6F4
.L_8013C6F0:
    lis      r31, 0x10
.L_8013C6F4:
    li       r0, 0x0
    xor      r3, r5, r0
    xor      r0, r6, r0
    or.      r0, r3, r0
    beq      .L_8013C710
    li       r30, 0x4
    b        .L_8013C714
.L_8013C710:
    lis      r30, 0x20
.L_8013C714:
    lwz      r3, 0x8(r1)
    lfs      f1, lbl_8065E994@sda21(r0)
    lwz      r3, 0x4(r3)
    and.     r0, r3, r7
    beq      .L_8013C73C
    lwz      r0, 0xdac(r28)
    cmpwi    r0, 0x0
    beq      .L_8013C73C
    lfs      f1, lbl_8065E9A8@sda21(r0)
    b        .L_8013C754
.L_8013C73C:
    and.     r0, r3, r4
    beq      .L_8013C754
    lwz      r0, 0xdac(r28)
    cmpwi    r0, 0x0
    beq      .L_8013C754
    lfs      f1, lbl_8065EA00@sda21(r0)
.L_8013C754:
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpu    cr0, f1, f0
    bne      .L_8013C764
    b        .L_8013C780
.L_8013C764:
    li       r0, 0x1
    lis      r4, 0x2
    stw      r0, 0xea0(r28)
    mr       r3, r28
    addi     r4, r4, 0x8
    bl       SetMapCameraModeFlags
    b        .L_8013C798
.L_8013C780:
    lwz      r0, 0x11d8(r28)
    rlwinm.  r0, r0, 0, 28, 28
    beq      .L_8013C798
    lwz      r0, 0x11d8(r28)
    rlwinm   r0, r0, 0, 29, 27
    stw      r0, 0x11d8(r28)
.L_8013C798:
    lwz      r4, 0x44(r28)
    cmplwi   r4, 0x0
    beq      .L_8013C988
    lwz      r0, 0x0(r4)
    cmplwi   r0, 0x0
    beq      .L_8013C988
    lwz      r3, 0x8(r1)
    lwz      r0, 0x14(r3)
    rlwinm.  r0, r0, 0, 25, 25
    beq      .L_8013C988
    lwz      r0, 0xdac(r28)
    cmpwi    r0, 0x0
    beq      .L_8013C988
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r5, r3, 0x1
    lwz      r3, -0x6944(r5)
    lwz      r0, 0xac4(r3)
    cmpwi    r0, 0x3
    beq      .L_8013C988
    lfs      f5, 0x10c(r4)
    lfs      f3, 0xc0(r28)
    fmr      f4, f5
    lfs      f2, 0xbc(r28)
    lfs      f1, lbl_8065E990@sda21(r0)
    lfs      f0, lbl_8065E994@sda21(r0)
    b        .L_8013C808
.L_8013C804:
    fadds    f4, f4, f1
.L_8013C808:
    fcmpo    cr0, f4, f0
    blt      .L_8013C804
    lfs      f0, lbl_8065E990@sda21(r0)
    b        .L_8013C81C
.L_8013C818:
    fsubs    f4, f4, f0
.L_8013C81C:
    fcmpo    cr0, f4, f0
    cror     eq, gt, eq
    beq      .L_8013C818
    stfs     f4, 0xc0(r28)
    lfs      f1, lbl_8065E990@sda21(r0)
    lfs      f4, 0x24(r28)
    lfs      f0, lbl_8065E994@sda21(r0)
    b        .L_8013C840
.L_8013C83C:
    fadds    f4, f4, f1
.L_8013C840:
    fcmpo    cr0, f4, f0
    blt      .L_8013C83C
    lfs      f0, lbl_8065E990@sda21(r0)
    b        .L_8013C854
.L_8013C850:
    fsubs    f4, f4, f0
.L_8013C854:
    fcmpo    cr0, f4, f0
    cror     eq, gt, eq
    beq      .L_8013C850
    lfs      f1, lbl_8065E990@sda21(r0)
    lfs      f0, lbl_8065E994@sda21(r0)
    b        .L_8013C870
.L_8013C86C:
    fadds    f5, f5, f1
.L_8013C870:
    fcmpo    cr0, f5, f0
    blt      .L_8013C86C
    lfs      f0, lbl_8065E990@sda21(r0)
    b        .L_8013C884
.L_8013C880:
    fsubs    f5, f5, f0
.L_8013C884:
    fcmpo    cr0, f5, f0
    cror     eq, gt, eq
    beq      .L_8013C880
    fsubs    f1, f5, f4
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_8013C8A4
    fneg     f1, f1
.L_8013C8A4:
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_8013C8EC
    lfs      f0, lbl_8065E990@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_8013C8C4
    fsubs    f1, f1, f0
.L_8013C8C4:
    lfs      f0, lbl_8065E998@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_8013C8EC
    fcmpo    cr0, f5, f4
    bge      .L_8013C8E4
    lfs      f0, lbl_8065E990@sda21(r0)
    fadds    f5, f5, f0
    b        .L_8013C8EC
.L_8013C8E4:
    lfs      f0, lbl_8065E990@sda21(r0)
    fsubs    f5, f5, f0
.L_8013C8EC:
    fsubs    f4, f5, f4
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f4, f0
    bge      .L_8013C900
    fneg     f4, f4
.L_8013C900:
    lfs      f1, lbl_8065EA44@sda21(r0)
    lfs      f0, lbl_8065E9A0@sda21(r0)
    fdivs    f1, f4, f1
    stfs     f1, 0xbc(r28)
    lfs      f1, 0xbc(r28)
    fcmpo    cr0, f1, f0
    bge      .L_8013C92C
    stfs     f3, 0xc0(r28)
    li       r0, 0x0
    stfs     f2, 0xbc(r28)
    b        .L_8013C930
.L_8013C92C:
    li       r0, 0x1
.L_8013C930:
    cmpwi    r0, 0x0
    beq      .L_8013C988
    lwz      r3, -0x6944(r5)
    lwz      r0, 0xac4(r3)
    cmpwi    r0, 0x5
    beq      .L_8013C978
    lwz      r0, 0xbac(r3)
    cmpwi    r0, 0x5
    beq      .L_8013C978
    lwz      r0, 0xb68(r3)
    cmpwi    r0, 0x5
    beq      .L_8013C978
    lwz      r0, 0xbec(r3)
    cmpwi    r0, 0x2
    beq      .L_8013C978
    addis    r3, r3, 0x1
    lfs      f0, -0x6554(r3)
    stfs     f0, -0x6558(r3)
.L_8013C978:
    lis      r4, 0x2
    mr       r3, r28
    addi     r4, r4, 0x2
    bl       SetMapCameraModeFlags
.L_8013C988:
    mr       r3, r28
    mr       r4, r29
    bl       UpdateMapCameraTurnInterpolation
    lwz      r3, 0x8(r1)
    lis      r4, gCameraAreaState@ha
    addi     r27, r4, gCameraAreaState@l
    lwz      r5, 0x14(r3)
    lwz      r3, 0x14(r27)
    and.     r0, r5, r31
    lwz      r4, 0x18(r27)
    mr       r31, r3
    beq      .L_8013C9D4
    lwz      r0, 0xdac(r28)
    cmpwi    r0, 0x0
    beq      .L_8013C9D4
    cmpwi    r3, 0x1
    ble      .L_8013C9F4
    subi     r31, r3, 0x1
    b        .L_8013C9F4
.L_8013C9D4:
    and.     r0, r5, r30
    beq      .L_8013C9F4
    lwz      r0, 0xdac(r28)
    cmpwi    r0, 0x0
    beq      .L_8013C9F4
    cmpwi    r3, 0x2
    bge      .L_8013C9F4
    addi     r31, r31, 0x1
.L_8013C9F4:
    cmpw     r31, r4
    beq      .L_8013CACC
    lis      r3, gCameraAreaState@ha
    cmpwi    r31, 0x2
    addi     r3, r3, gCameraAreaState@l
    lfs      f2, 0x10(r3)
    beq      .L_8013CA78
    bge      .L_8013CA24
    cmpwi    r31, 0x0
    beq      .L_8013CA30
    bge      .L_8013CA58
    b        .L_8013CA30
.L_8013CA24:
    cmpwi    r31, 0x4
    bge      .L_8013CA30
    b        .L_8013CAA0
.L_8013CA30:
    addi     r3, r28, 0xc8
    lfs      f0, lbl_8065EA3C@sda21(r0)
    lwz      r12, 0xc8(r28)
    fmuls    f2, f0, f2
    lfs      f1, 0x34(r28)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
    b        .L_8013CAC4
.L_8013CA58:
    lwz      r12, 0xc8(r28)
    addi     r3, r28, 0xc8
    lfs      f1, 0x34(r28)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
    b        .L_8013CAC4
.L_8013CA78:
    addi     r3, r28, 0xc8
    lfs      f0, lbl_8065EA40@sda21(r0)
    lwz      r12, 0xc8(r28)
    fmuls    f2, f0, f2
    lfs      f1, 0x34(r28)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
    b        .L_8013CAC4
.L_8013CAA0:
    addi     r3, r28, 0xc8
    lfs      f0, lbl_8065EA04@sda21(r0)
    lwz      r12, 0xc8(r28)
    fmuls    f2, f0, f2
    lfs      f1, 0x34(r28)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
.L_8013CAC4:
    stw      r31, 0x14(r27)
    stw      r31, 0x18(r27)
.L_8013CACC:
    addi     r3, r28, 0xc8
    mr       r4, r29
    lwz      r12, 0xc8(r28)
    lfs      f1, lbl_8065E994@sda21(r0)
    lwz      r12, 0x14(r12)
    mtctr    r12
    bctrl
    clrlwi.  r0, r3, 24
    bne      .L_8013CB18
    addi     r3, r28, 0xc8
    lwz      r12, 0xc8(r28)
    lwz      r12, 0x18(r12)
    mtctr    r12
    bctrl
    mr       r3, r28
    lwz      r12, 0x3c(r28)
    lwz      r12, 0x48(r12)
    mtctr    r12
    bctrl
.L_8013CB18:
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0xc(r1)
    lmw      r27, 0x1c(r1)
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

static inline void RefreshOrbitPosition(CCameraActionMap* map, u32 step, bool manual)
{
	f32 yaw    = HALF_PI + map->mAngle.y;
	f32 radius = FlatDistance(map->_F68, map->mInterest);
	f32 x      = radius * (f32)cos(yaw) + map->mInterest.x;
	f32 z      = radius * -(f32)sin(yaw) + map->mInterest.z;
	// The manual turn uses the frame delta left in f31 by the retail caller.
	f32 height = manual ? 0.1f * step : map->_F68.y;
	map->_F88  = CVec(x, height, z);
	map->_F68  = CVec(x, height, z);
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
	u32 left               = (gGame.mStatus.mFlags & 0x80000000) ? 1 : 0x400000;
	u32 right              = (gGame.mStatus.mFlags & 0x80000000) ? 2 : 0x800000;
	sPreviousMapOrbitAngle = map->_F98;
	map->_F88              = map->_F68;
	if (!lbl_8065B348)
		map->_F98 = map->mAngle;

	{
		CVec angle;
		f32 turn = 0.0f;
		if ((pad.mPad->mButton & right) && map->_DAC)
			turn = 1.0f;
		else if ((pad.mPad->mButton & left) && map->_DAC)
			turn = -1.0f;
		if (turn != 0.0f) {
			lbl_8065B34C = turn;
			angle        = map->_F98;
			f32 pitch, heading;
			AdvanceOrbitAngles(angle, step, 0.0f, (3.5f * DEG2RAD) * turn, pitch, heading);
			if (pitch > 89.1f * DEG2RAD)
				pitch = 89.1f * DEG2RAD;
			if (pitch < -89.1f * DEG2RAD)
				pitch = -89.1f * DEG2RAD;
			map->SetAngle(pitch, heading, angle.z);
			map->_FB8 = map->_F98;
			map->_F98 = map->mAngle;
			map->SetInterest(GetMapCameraTargetWithHeightOffset(map));
			RefreshOrbitPosition(map, step, true);
		}
	}

	if (map->mStatus.mFlags & 2) {
		map->_F88 = map->_F68;
		if (!lbl_8065B348)
			map->_F98 = map->mAngle;
		f32 yaw    = WrapAngle(map->_F98.y);
		f32 target = WrapAngle(map->_C0);
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
			lbl_8065B34C = 1.0f;
			yaw          = map->_BC * step + yaw;
			if (yaw > target)
				yaw = target;
		} else if (yaw > target) {
			yaw = yaw - map->_BC * step;
			if (yaw < target)
				yaw = target;
			lbl_8065B34C = -1.0f;
		}
		diff = target - yaw;
		if (diff < 0.0f)
			diff = -diff;
		if (diff < 0.001f)
			yaw = target;
		BOOL turning = TRUE;
		Vec angle    = map->_F98;
		angle.y      = yaw;
		map->SetAngle(angle);
		map->_FB8 = map->_F98;
		map->_F98 = map->mAngle;
		map->SetInterest(GetMapCameraTargetWithHeightOffset(map));
		RefreshOrbitPosition(map, step, false);
		map->_EA0 = 1;

		if (yaw == target)
			turning = FALSE;
		if (!turning)
			map->mStatus.mFlags &= ~2;
	}
}

// picks the two corners of the current entry's side, ordered by which side chibi robo is on
extern "C" void C_VECCrossProduct(Vec* a, Vec* b, Vec* axb);

/**
 * @note Address: 0x8013D2DC
 * @note Size: 0x250
 */
extern "C" BOOL SetMapCameraSideEndpoints(CCameraActionMap* map)
{
	CamMapEntry* entry = map->_97C;
	if (entry != NULL) {
		s32 group          = entry->_0C;
		s32 side           = entry->_10;
		CVec dir           = entry->_14;
		CamAreaRecord* rec = &gCameraAreaState._1C[group];
		CVec toPlayer      = gGame.GetPlayer()->mMoveTranslate;
		toPlayer.y         = 0.0f;
		Vec eye            = map->mEye;
		eye.y              = 0.0f;
		PSVECSubtract(&toPlayer, &eye, &toPlayer);
		PSVECNormalize(&toPlayer, &toPlayer);
		C_VECCrossProduct(&dir, &toPlayer, &dir);
		CVec cross = dir;

		s32* corner = rec->_04[side];
		f32 x0      = corner[0] / 100.0f;
		f32 z0      = corner[1] / 100.0f;
		if (side == 3) {
			corner = rec->_04[0];
		} else {
			corner += 2;
		}
		f32 x1 = corner[0] / 100.0f;
		f32 z1 = corner[1] / 100.0f;

		if (cross.y > 0.0f) {
			map->_984.x = x0;
			map->_984.y = 0.0f;
			map->_984.z = z0;
			map->_990.x = x1;
			map->_990.y = 0.0f;
			map->_990.z = z1;
			map->_99C   = 1;
		}
		if (cross.y < 0.0f) {
			map->_984.x = x1;
			map->_984.y = 0.0f;
			map->_984.z = z1;
			map->_990.x = x0;
			map->_990.y = 0.0f;
			map->_990.z = z0;
			map->_99C   = -1;
		}
		return TRUE;
	}
	return FALSE;
}

/*
    stwu     r1, -0x70(r1)
    mflr     r0
    stw      r0, 0x74(r1)
    stw      r31, 0x6c(r1)
    stw      r30, 0x68(r1)
    stw      r29, 0x64(r1)
    mr       r29, r3
    lwz      r9, 0x97c(r3)
    cmplwi   r9, 0x0
    beq      .L_8013D50C
    lwz      r0, 0xc(r9)
    lis      r3, __vt__4CVec@ha
    lwz      r31, 0x10(r9)
    addi     r8, r3, __vt__4CVec@l
    lis      r3, gGame@ha
    lis      r5, gCameraAreaState@ha
    stw      r8, 0x40(r1)
    addi     r3, r3, gGame@l
    addis    r4, r3, 0x1
    addi     r6, r5, gCameraAreaState@l
    lfs      f0, 0x14(r9)
    addi     r3, r1, 0x24
    lwz      r7, -0x6944(r4)
    mulli    r0, r0, 0x34
    stfs     f0, 0x34(r1)
    mr       r5, r3
    lwz      r30, 0x1c(r6)
    addi     r4, r1, 0x18
    lfs      f1, 0x18(r9)
    lfs      f0, lbl_8065E994@sda21(r0)
    add      r30, r30, r0
    stfs     f1, 0x38(r1)
    lfs      f1, 0x1c(r9)
    stfs     f1, 0x3c(r1)
    lwz      r6, 0xc8(r7)
    lwz      r0, 0xcc(r7)
    stw      r6, 0x24(r1)
    stw      r0, 0x28(r1)
    lwz      r0, 0xd0(r7)
    stw      r0, 0x2c(r1)
    stw      r8, 0x30(r1)
    stfs     f0, 0x28(r1)
    lwz      r6, 0x0(r29)
    lwz      r0, 0x4(r29)
    stw      r6, 0x18(r1)
    stw      r0, 0x1c(r1)
    lwz      r0, 0x8(r29)
    stw      r0, 0x20(r1)
    stfs     f0, 0x1c(r1)
    bl       PSVECSubtract
    addi     r3, r1, 0x24
    mr       r4, r3
    bl       PSVECNormalize
    addi     r3, r1, 0x34
    addi     r4, r1, 0x24
    mr       r5, r3
    bl       C_VECCrossProduct
    slwi     r0, r31, 2
    addi     r8, r30, 0x4
    add      r8, r8, r0
    lis      r3, 0x4330
    add      r8, r8, r0
    lwz      r7, 0x34(r1)
    lwz      r4, 0x0(r8)
    lis      r5, __vt__4CVec@ha
    lwz      r0, 0x4(r8)
    addi     r5, r5, __vt__4CVec@l
    xoris    r4, r4, 0x8000
    lwz      r6, 0x38(r1)
    xoris    r0, r0, 0x8000
    stw      r4, 0x4c(r1)
    lwz      r4, 0x3c(r1)
    cmpwi    r31, 0x3
    stw      r3, 0x48(r1)
    lfd      f2, lbl_8065E9E8@sda21(r0)
    lfd      f0, 0x48(r1)
    stw      r0, 0x54(r1)
    fsubs    f5, f0, f2
    lfs      f1, lbl_8065E9DC@sda21(r0)
    stw      r3, 0x50(r1)
    lfd      f0, 0x50(r1)
    fdivs    f5, f5, f1
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    stw      r4, 0x10(r1)
    stw      r5, 0x14(r1)
    fsubs    f6, f0, f2
    fdivs    f6, f6, f1
    bne      .L_8013D448
    addi     r8, r30, 0x4
    b        .L_8013D44C
.L_8013D448:
    addi     r8, r8, 0x8
.L_8013D44C:
    lwz      r4, 0x0(r8)
    lis      r3, 0x4330
    lwz      r0, 0x4(r8)
    xoris    r4, r4, 0x8000
    stw      r3, 0x50(r1)
    xoris    r0, r0, 0x8000
    lfd      f4, lbl_8065E9E8@sda21(r0)
    stw      r4, 0x54(r1)
    lfs      f3, lbl_8065E9DC@sda21(r0)
    lfd      f0, 0x50(r1)
    stw      r0, 0x4c(r1)
    fsubs    f7, f0, f4
    lfs      f1, 0xc(r1)
    stw      r3, 0x48(r1)
    lfs      f0, lbl_8065E994@sda21(r0)
    lfd      f2, 0x48(r1)
    fdivs    f7, f7, f3
    fsubs    f2, f2, f4
    fcmpo    cr0, f1, f0
    fdivs    f2, f2, f3
    ble      .L_8013D4C0
    stfs     f5, 0x984(r29)
    li       r0, 0x1
    stfs     f0, 0x988(r29)
    stfs     f6, 0x98c(r29)
    stfs     f7, 0x990(r29)
    stfs     f0, 0x994(r29)
    stfs     f2, 0x998(r29)
    stw      r0, 0x99c(r29)
.L_8013D4C0:
    lfs      f1, 0xc(r1)
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_8013D4F0
    stfs     f7, 0x984(r29)
    li       r0, -0x1
    stfs     f0, 0x988(r29)
    stfs     f2, 0x98c(r29)
    stfs     f5, 0x990(r29)
    stfs     f0, 0x994(r29)
    stfs     f6, 0x998(r29)
    stw      r0, 0x99c(r29)
.L_8013D4F0:
    lis      r4, __vt__4CVec@ha
    li       r3, 0x1
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x14(r1)
    stw      r0, 0x30(r1)
    stw      r0, 0x40(r1)
    b        .L_8013D510
.L_8013D50C:
    li       r3, 0x0
.L_8013D510:
    lwz      r0, 0x74(r1)
    lwz      r31, 0x6c(r1)
    lwz      r30, 0x68(r1)
    lwz      r29, 0x64(r1)
    mtlr     r0
    addi     r1, r1, 0x70
    blr
*/

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
	s32 group = entry->_0C;
	s32 side  = entry->_10;
	s32 count = 0;
	if (num > 0) {
		for (s32 i = 0; i < num; i++) {
			if (map->mIntersectionEntries[i]._0C == group) {
				for (s32 j = 0; j < 2; j++) {
					if (map->mIntersectionEntries[i]._10 == sides[side][j]) {
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
extern "C" s32 CountMapCameraEntriesInGroup(CCameraActionMap* map, s32 value)
{
	s32 count = 0;
	if (map->mIntersectionCount > 0) {
		for (s32 i = 0; i < map->mIntersectionCount; i++) {
			if (map->mIntersectionEntries[i]._0C == value) {
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
			f32 dist = PSVECDistance(&p, &map->mIntersectionEntries[i]._00);
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
		record     = gCameraAreaState._1C;
		numRecords = gCameraAreaState._20;
	}
	if (type == 1) {
		record     = gCameraAreaState._24;
		numRecords = gCameraAreaState._28;
	}
	CamMapEntry* entry = out;
	s32 count          = 0;
	for (s32 i = 0; i < numRecords; i++) {
		if (CamAreaInfo(record).IsCameraOff()) {
			record++;
			continue;
		}
		if (CamAreaInfo(record).Is_2C_1()) {
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
				entry->_00   = hit;
				entry->_10   = side;
				entry->_0C   = i;
				entry->_14.x = nx;
				entry->_14.y = ny;
				entry->_14.z = nz;
				entry++;
			}
		}
		record++;
	}
	return count;
}
/*
    stwu     r1, -0x120(r1)
    mflr     r0
    stw      r0, 0x124(r1)
    stfd     f31, 0x110(r1)
    psq_st   f31, 0x118(r1), 0, qr0
    stfd     f30, 0x100(r1)
    psq_st   f30, 0x108(r1), 0, qr0
    stfd     f29, 0xf0(r1)
    psq_st   f29, 0xf8(r1), 0, qr0
    stmw     r15, 0xac(r1)
    mr.      r18, r6
    mr       r15, r4
    mr       r16, r5
    mr       r17, r7
    bne      .L_8013D848
    li       r3, -0x1
    b        .L_8013DC40
.L_8013D848:
    mr       r3, r18
    li       r4, 0x0
    li       r5, 0x400
    bl       memset
    cmpwi    r17, 0x0
    bne      .L_8013D870
    lis      r3, gCameraAreaState@ha
    addi     r3, r3, gCameraAreaState@l
    lwz      r23, 0x1c(r3)
    lwz      r22, 0x20(r3)
.L_8013D870:
    cmpwi    r17, 0x1
    bne      .L_8013D888
    lis      r3, gCameraAreaState@ha
    addi     r3, r3, gCameraAreaState@l
    lwz      r23, 0x24(r3)
    lwz      r22, 0x28(r3)
.L_8013D888:
    lis      r3, __vt__11CamAreaInfo@ha
    mr       r21, r18
    addi     r24, r3, __vt__11CamAreaInfo@l
    li       r20, 0x0
    li       r18, 0x0
    b        .L_8013DC34
.L_8013D8A0:
    lhz      r3, 0x2e(r23)
    addic.   r0, r1, 0x20
    stw      r24, 0x20(r1)
    rlwinm   r0, r3, 0, 29, 29
    cntlzw   r0, r0
    stw      r23, 0x24(r1)
    srwi     r4, r0, 5
    beq      .L_8013D8CC
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x20(r1)
.L_8013D8CC:
    clrlwi.  r0, r4, 24
    beq      .L_8013D8DC
    addi     r23, r23, 0x34
    b        .L_8013DC30
.L_8013D8DC:
    lha      r0, 0x2c(r23)
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r4, r3, __vt__11CamAreaInfo@l
    stw      r23, 0x1c(r1)
    subfic   r3, r0, 0x1
    addic.   r0, r1, 0x18
    stw      r4, 0x18(r1)
    cntlzw   r0, r3
    srwi     r0, r0, 5
    beq      .L_8013D908
    stw      r4, 0x18(r1)
.L_8013D908:
    clrlwi.  r0, r0, 24
    beq      .L_8013D918
    addi     r23, r23, 0x34
    b        .L_8013DC30
.L_8013D918:
    lwz      r3, 0x24(r23)
    lis      r0, 0x4330
    lis      r4, __vt__11CamAreaInfo@ha
    stw      r0, 0x98(r1)
    xoris    r0, r3, 0x8000
    lfd      f2, lbl_8065E9E8@sda21(r0)
    stw      r0, 0x9c(r1)
    addi     r3, r4, __vt__11CamAreaInfo@l
    lfs      f0, lbl_8065E9DC@sda21(r0)
    addic.   r0, r1, 0x10
    lfd      f1, 0x98(r1)
    stw      r3, 0x10(r1)
    fsubs    f1, f1, f2
    stw      r23, 0x14(r1)
    fdivs    f3, f1, f0
    beq      .L_8013D95C
    stw      r3, 0x10(r1)
.L_8013D95C:
    lwz      r3, 0x28(r23)
    lis      r0, 0x4330
    lis      r4, __vt__11CamAreaInfo@ha
    stw      r0, 0xa0(r1)
    xoris    r0, r3, 0x8000
    lfd      f2, lbl_8065E9E8@sda21(r0)
    stw      r0, 0xa4(r1)
    addi     r3, r4, __vt__11CamAreaInfo@l
    lfs      f0, lbl_8065E9DC@sda21(r0)
    addic.   r0, r1, 0x8
    lfd      f1, 0xa0(r1)
    stw      r3, 0x8(r1)
    fsubs    f1, f1, f2
    stw      r23, 0xc(r1)
    fdivs    f1, f1, f0
    beq      .L_8013D9A0
    stw      r3, 0x8(r1)
.L_8013D9A0:
    lfs      f0, 0x4(r15)
    fcmpo    cr0, f0, f1
    cror     eq, gt, eq
    bne      .L_8013D9C4
    fcmpo    cr0, f0, f3
    cror     eq, lt, eq
    bne      .L_8013D9C4
    li       r0, 0x1
    b        .L_8013D9E8
.L_8013D9C4:
    fcmpo    cr0, f0, f1
    bge      .L_8013D9D4
    li       r0, -0x2
    b        .L_8013D9E8
.L_8013D9D4:
    fcmpo    cr0, f0, f3
    ble      .L_8013D9E4
    li       r0, -0x1
    b        .L_8013D9E8
.L_8013D9E4:
    li       r0, 0x0
.L_8013D9E8:
    cmpwi    r0, 0x0
    bge      .L_8013DA48
    lfs      f0, 0x4(r16)
    fcmpo    cr0, f0, f1
    cror     eq, gt, eq
    bne      .L_8013DA14
    fcmpo    cr0, f0, f3
    cror     eq, lt, eq
    bne      .L_8013DA14
    li       r0, 0x1
    b        .L_8013DA38
.L_8013DA14:
    fcmpo    cr0, f0, f1
    bge      .L_8013DA24
    li       r0, -0x2
    b        .L_8013DA38
.L_8013DA24:
    fcmpo    cr0, f0, f3
    ble      .L_8013DA34
    li       r0, -0x1
    b        .L_8013DA38
.L_8013DA34:
    li       r0, 0x0
.L_8013DA38:
    cmpwi    r0, 0x0
    bge      .L_8013DA48
    addi     r23, r23, 0x34
    b        .L_8013DC30
.L_8013DA48:
    lfd      f29, lbl_8065E9E8@sda21(r0)
    addi     r19, r23, 0x4
    lfs      f30, lbl_8065E9DC@sda21(r0)
    li       r17, 0x0
    lfs      f31, lbl_8065E994@sda21(r0)
    lis      r25, 0x4330
    lwz      r26, 0x0(r16)
    lwz      r27, 0x4(r16)
    lwz      r28, 0x8(r16)
    lwz      r29, 0x0(r15)
    lwz      r30, 0x4(r15)
    lwz      r31, 0x8(r15)
.L_8013DA78:
    lwz      r0, 0x0(r19)
    cmpwi    r17, 0x3
    stw      r25, 0xa0(r1)
    xoris    r0, r0, 0x8000
    stw      r0, 0xa4(r1)
    lfd      f0, 0xa0(r1)
    stfs     f31, 0x90(r1)
    fsubs    f0, f0, f29
    stw      r25, 0x98(r1)
    fdivs    f0, f0, f30
    stfs     f0, 0x8c(r1)
    lwz      r0, 0x4(r19)
    addi     r19, r19, 0x8
    xoris    r0, r0, 0x8000
    stw      r0, 0x9c(r1)
    lfd      f0, 0x98(r1)
    fsubs    f0, f0, f29
    fdivs    f0, f0, f30
    stfs     f0, 0x94(r1)
    bne      .L_8013DACC
    addi     r19, r23, 0x4
.L_8013DACC:
    lwz      r0, 0x0(r19)
    addi     r3, r1, 0x8c
    stw      r25, 0xa0(r1)
    addi     r4, r1, 0x80
    xoris    r0, r0, 0x8000
    addi     r5, r1, 0x74
    stw      r0, 0xa4(r1)
    addi     r6, r1, 0x68
    addi     r7, r1, 0x5c
    li       r8, 0x1
    lfd      f0, 0xa0(r1)
    stfs     f31, 0x84(r1)
    fsubs    f0, f0, f29
    stw      r25, 0x98(r1)
    fdivs    f0, f0, f30
    stfs     f0, 0x80(r1)
    lwz      r0, 0x4(r19)
    xoris    r0, r0, 0x8000
    stw      r26, 0x74(r1)
    stw      r0, 0x9c(r1)
    lfd      f0, 0x98(r1)
    stw      r27, 0x78(r1)
    fsubs    f0, f0, f29
    stw      r28, 0x7c(r1)
    fdivs    f0, f0, f30
    stw      r29, 0x68(r1)
    stw      r30, 0x6c(r1)
    stw      r31, 0x70(r1)
    stfs     f0, 0x88(r1)
    bl       IntersectLinesXZ
    cmpwi    r3, 0x0
    beq      .L_8013DC20
    lis      r3, sMapCameraEdgeNormalDown@ha
    lfs      f0, lbl_8065E994@sda21(r0)
    addi     r4, r3, sMapCameraEdgeNormalDown@l
    lis      r3, __vt__4CVec@ha
    lwz      r5, 0x0(r4)
    addi     r0, r3, __vt__4CVec@l
    lwz      r7, 0x4(r4)
    addi     r3, r1, 0x28
    lwz      r6, 0x8(r4)
    addi     r4, r1, 0x34
    stw      r5, 0x50(r1)
    addi     r5, r1, 0x40
    lfs      f2, 0x8c(r1)
    lfs      f1, 0x80(r1)
    stw      r7, 0x54(r1)
    fsubs    f3, f2, f1
    lfs      f6, 0x50(r1)
    stw      r6, 0x58(r1)
    lfs      f5, 0x54(r1)
    lfs      f4, 0x58(r1)
    lfs      f2, 0x94(r1)
    lfs      f1, 0x88(r1)
    stfs     f6, 0x28(r1)
    fsubs    f1, f2, f1
    stfs     f5, 0x2c(r1)
    stfs     f4, 0x30(r1)
    stfs     f3, 0x34(r1)
    stfs     f1, 0x3c(r1)
    stfs     f0, 0x38(r1)
    stw      r0, 0x4c(r1)
    bl       C_VECCrossProduct
    addi     r3, r1, 0x40
    mr       r4, r3
    bl       PSVECNormalize
    lis      r3, __vt__4CVec@ha
    lfs      f1, 0x40(r1)
    addi     r0, r3, __vt__4CVec@l
    lfs      f2, 0x44(r1)
    stw      r0, 0x4c(r1)
    addi     r20, r20, 0x1
    lfs      f3, 0x48(r1)
    lfs      f0, 0x5c(r1)
    stfs     f0, 0x0(r21)
    lfs      f0, 0x60(r1)
    stfs     f0, 0x4(r21)
    lfs      f0, 0x64(r1)
    stfs     f0, 0x8(r21)
    stw      r17, 0x10(r21)
    stw      r18, 0xc(r21)
    stfs     f1, 0x14(r21)
    stfs     f2, 0x18(r21)
    stfs     f3, 0x1c(r21)
    addi     r21, r21, 0x20
.L_8013DC20:
    addi     r17, r17, 0x1
    cmpwi    r17, 0x4
    blt      .L_8013DA78
    addi     r23, r23, 0x34
.L_8013DC30:
    addi     r18, r18, 0x1
.L_8013DC34:
    cmpw     r18, r22
    blt      .L_8013D8A0
    mr       r3, r20
.L_8013DC40:
    psq_l    f31, 0x118(r1), 0, qr0
    lfd      f31, 0x110(r1)
    psq_l    f30, 0x108(r1), 0, qr0
    lfd      f30, 0x100(r1)
    psq_l    f29, 0xf8(r1), 0, qr0
    lfd      f29, 0xf0(r1)
    lmw      r15, 0xac(r1)
    lwz      r0, 0x124(r1)
    mtlr     r0
    addi     r1, r1, 0x120
    blr
*/

/**
 * @note Address: 0x8013DC6C
 * @note Size: 0x600
 */
extern "C" s32 CalcIntersectVec__16CCameraActionMapFRC3VecRC3VecP11CamMapEntryl(CCameraActionMap* map, const Vec& from, const Vec& to,
                                                                                CamMapEntry* out, s32 type)
{
	if (out == NULL) {
		return -1;
	}
	memset(out, 0, 32 * sizeof(CamMapEntry));
	CamAreaRecord* record;
	s32 numRecords;
	if (type == 0) {
		record     = gCameraAreaState._1C;
		numRecords = gCameraAreaState._20;
	}
	if (type == 1) {
		record     = gCameraAreaState._24;
		numRecords = gCameraAreaState._28;
	}
	CamMapEntry* entry = out;
	s32 count          = 0;
	for (s32 i = 0; i < numRecords; i++) {
		u32 flags = CamAreaInfo(record).IsDisabled();
		if (flags != 0) {
			record++;
			continue;
		}
		f32 top    = CamAreaInfo(record).GetTop();
		f32 bottom = CamAreaInfo(record).GetBottom();
		if (ClassifyAreaHeight(from.y, bottom, top) < 0 && ClassifyAreaHeight(to.y, bottom, top) < 0) {
			record++;
			continue;
		}
		if (map->mTurnInterpolation != NULL && i == lbl_8065B334) {
			record++;
			continue;
		}
		s32* corner = record->_04[0];
		for (s32 side = 0; side < 4; side++) {
			Vec a, b;
			a.x = corner[0] / 100.0f;
			a.y = 0.0f;
			a.z = corner[1] / 100.0f;
			corner += 2;
			if (side == 3) {
				corner = record->_04[0];
			}
			b.x       = corner[0] / 100.0f;
			b.y       = 0.0f;
			b.z       = corner[1] / 100.0f;
			Vec end   = to;
			Vec start = from;
			Vec hit;
			s32 intersects = IntersectLinesXZ(&a, &b, &end, &start, &hit, 1);
			if (intersects && (__fpclassifyf(hit.x) == FP_NAN || __fpclassifyf(hit.y) == FP_NAN || __fpclassifyf(hit.z) == FP_NAN)) {
				OSPanic("ccameraactionmap2.cpp", 2183, "%s%s",
				        FormatToStaticBuffer(
				            "CCameraActionMap ... GetCrossPoint\np1(%f, %f, %f) p2(%f, %f, %f)\nvs(%f, %f, %f) ve(%f, %f, %f)\n(1)\n", a.x,
				            a.y, a.z, b.x, b.y, b.z, end.x, end.y, end.z, start.x, start.y, start.z),
				        "CalcIntersectVec");
			}
			if (intersects) {
				Vec down = { 0.0f, -1.0f, 0.0f };
				Vec edge;
				edge.x = a.x - b.x;
				edge.z = a.z - b.z;
				edge.y = 0.0f;
				f32 nx, ny, nz;
				{
					Vec downCopy = { down.x, down.y, down.z };
					CVec normal;
					C_VECCrossProduct(&downCopy, &edge, &normal);
					PSVECNormalize(&normal, &normal);
					nx = normal.x;
					ny = normal.y;
					nz = normal.z;
				}
				count++;
				entry->_00   = hit;
				entry->_10   = side;
				entry->_0C   = i;
				entry->_14.x = nx;
				entry->_14.y = ny;
				entry->_14.z = nz;
				entry++;
			}
		}
		record++;
	}
	return count;
}
/*
    stwu     r1, -0x150(r1)
    mflr     r0
    stw      r0, 0x154(r1)
    stfd     f31, 0x140(r1)
    psq_st   f31, 0x148(r1), 0, qr0
    stfd     f30, 0x130(r1)
    psq_st   f30, 0x138(r1), 0, qr0
    stfd     f29, 0x120(r1)
    psq_st   f29, 0x128(r1), 0, qr0
    stmw     r14, 0xd8(r1)
    mr.      r18, r6
    lis      r6, lbl_8020C8C0@ha
    stw      r3, 0x28(r1)
    mr       r15, r4
    mr       r16, r5
    mr       r17, r7
    addi     r23, r6, lbl_8020C8C0@l
    bne      .L_8013DCBC
    li       r3, -0x1
    b        .L_8013E240
.L_8013DCBC:
    mr       r3, r18
    li       r4, 0x0
    li       r5, 0x400
    bl       memset
    cmpwi    r17, 0x0
    bne      .L_8013DCE4
    lis      r3, gCameraAreaState@ha
    addi     r3, r3, gCameraAreaState@l
    lwz      r22, 0x1c(r3)
    lwz      r14, 0x20(r3)
.L_8013DCE4:
    cmpwi    r17, 0x1
    bne      .L_8013DCFC
    lis      r3, gCameraAreaState@ha
    addi     r3, r3, gCameraAreaState@l
    lwz      r22, 0x24(r3)
    lwz      r14, 0x28(r3)
.L_8013DCFC:
    lis      r3, __vt__11CamAreaInfo@ha
    mr       r21, r18
    addi     r0, r3, __vt__11CamAreaInfo@l
    li       r20, 0x0
    stw      r0, 0xd0(r1)
    li       r18, 0x0
    b        .L_8013E234
.L_8013DD18:
    addic.   r0, r1, 0x48
    lwz      r0, 0xd0(r1)
    lhz      r3, 0x2e(r22)
    stw      r22, 0x4c(r1)
    rlwinm   r4, r3, 0, 24, 24
    stw      r0, 0x48(r1)
    beq      .L_8013DD40
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x48(r1)
.L_8013DD40:
    cmplwi   r4, 0x0
    beq      .L_8013DD50
    addi     r22, r22, 0x34
    b        .L_8013E230
.L_8013DD50:
    lwz      r3, 0x24(r22)
    lis      r0, 0x4330
    lis      r4, __vt__11CamAreaInfo@ha
    stw      r0, 0xc0(r1)
    xoris    r0, r3, 0x8000
    lfd      f2, lbl_8065E9E8@sda21(r0)
    stw      r0, 0xc4(r1)
    addi     r3, r4, __vt__11CamAreaInfo@l
    lfs      f0, lbl_8065E9DC@sda21(r0)
    addic.   r0, r1, 0x40
    lfd      f1, 0xc0(r1)
    stw      r3, 0x40(r1)
    fsubs    f1, f1, f2
    stw      r22, 0x44(r1)
    fdivs    f3, f1, f0
    beq      .L_8013DD94
    stw      r3, 0x40(r1)
.L_8013DD94:
    lwz      r3, 0x28(r22)
    lis      r0, 0x4330
    lis      r4, __vt__11CamAreaInfo@ha
    stw      r0, 0xc8(r1)
    xoris    r0, r3, 0x8000
    lfd      f2, lbl_8065E9E8@sda21(r0)
    stw      r0, 0xcc(r1)
    addi     r3, r4, __vt__11CamAreaInfo@l
    lfs      f0, lbl_8065E9DC@sda21(r0)
    addic.   r0, r1, 0x38
    lfd      f1, 0xc8(r1)
    stw      r3, 0x38(r1)
    fsubs    f1, f1, f2
    stw      r22, 0x3c(r1)
    fdivs    f1, f1, f0
    beq      .L_8013DDD8
    stw      r3, 0x38(r1)
.L_8013DDD8:
    lfs      f0, 0x4(r15)
    fcmpo    cr0, f0, f1
    cror     eq, gt, eq
    bne      .L_8013DDFC
    fcmpo    cr0, f0, f3
    cror     eq, lt, eq
    bne      .L_8013DDFC
    li       r0, 0x1
    b        .L_8013DE20
.L_8013DDFC:
    fcmpo    cr0, f0, f1
    bge      .L_8013DE0C
    li       r0, -0x2
    b        .L_8013DE20
.L_8013DE0C:
    fcmpo    cr0, f0, f3
    ble      .L_8013DE1C
    li       r0, -0x1
    b        .L_8013DE20
.L_8013DE1C:
    li       r0, 0x0
.L_8013DE20:
    cmpwi    r0, 0x0
    bge      .L_8013DE80
    lfs      f0, 0x4(r16)
    fcmpo    cr0, f0, f1
    cror     eq, gt, eq
    bne      .L_8013DE4C
    fcmpo    cr0, f0, f3
    cror     eq, lt, eq
    bne      .L_8013DE4C
    li       r0, 0x1
    b        .L_8013DE70
.L_8013DE4C:
    fcmpo    cr0, f0, f1
    bge      .L_8013DE5C
    li       r0, -0x2
    b        .L_8013DE70
.L_8013DE5C:
    fcmpo    cr0, f0, f3
    ble      .L_8013DE6C
    li       r0, -0x1
    b        .L_8013DE70
.L_8013DE6C:
    li       r0, 0x0
.L_8013DE70:
    cmpwi    r0, 0x0
    bge      .L_8013DE80
    addi     r22, r22, 0x34
    b        .L_8013E230
.L_8013DE80:
    lwz      r3, 0x28(r1)
    lwz      r0, 0xdb4(r3)
    cmplwi   r0, 0x0
    beq      .L_8013DEA4
    lwz      r0, lbl_8065B334@sda21(r0)
    cmpw     r18, r0
    bne      .L_8013DEA4
    addi     r22, r22, 0x34
    b        .L_8013E230
.L_8013DEA4:
    lfd      f29, lbl_8065E9E8@sda21(r0)
    addi     r19, r22, 0x4
    lfs      f30, lbl_8065E9DC@sda21(r0)
    li       r17, 0x0
    lfs      f31, lbl_8065E994@sda21(r0)
    lis      r24, 0x4330
    lwz      r25, 0x0(r16)
    lwz      r26, 0x4(r16)
    lwz      r27, 0x8(r16)
    lwz      r28, 0x0(r15)
    lwz      r29, 0x4(r15)
    lwz      r30, 0x8(r15)
.L_8013DED4:
    lwz      r0, 0x0(r19)
    cmpwi    r17, 0x3
    stw      r24, 0xc8(r1)
    xoris    r0, r0, 0x8000
    stw      r0, 0xcc(r1)
    lfd      f0, 0xc8(r1)
    stfs     f31, 0xb8(r1)
    fsubs    f0, f0, f29
    stw      r24, 0xc0(r1)
    fdivs    f0, f0, f30
    stfs     f0, 0xb4(r1)
    lwz      r0, 0x4(r19)
    addi     r19, r19, 0x8
    xoris    r0, r0, 0x8000
    stw      r0, 0xc4(r1)
    lfd      f0, 0xc0(r1)
    fsubs    f0, f0, f29
    fdivs    f0, f0, f30
    stfs     f0, 0xbc(r1)
    bne      .L_8013DF28
    addi     r19, r22, 0x4
.L_8013DF28:
    lwz      r0, 0x0(r19)
    addi     r3, r1, 0xb4
    stw      r24, 0xc8(r1)
    addi     r4, r1, 0xa8
    xoris    r0, r0, 0x8000
    addi     r5, r1, 0x9c
    stw      r0, 0xcc(r1)
    addi     r6, r1, 0x90
    addi     r7, r1, 0x84
    li       r8, 0x1
    lfd      f0, 0xc8(r1)
    stfs     f31, 0xac(r1)
    fsubs    f0, f0, f29
    stw      r24, 0xc0(r1)
    fdivs    f0, f0, f30
    stfs     f0, 0xa8(r1)
    lwz      r0, 0x4(r19)
    xoris    r0, r0, 0x8000
    stw      r25, 0x9c(r1)
    stw      r0, 0xc4(r1)
    lfd      f0, 0xc0(r1)
    stw      r26, 0xa0(r1)
    fsubs    f0, f0, f29
    stw      r27, 0xa4(r1)
    fdivs    f0, f0, f30
    stw      r28, 0x90(r1)
    stw      r29, 0x94(r1)
    stw      r30, 0x98(r1)
    stfs     f0, 0xb0(r1)
    bl       IntersectLinesXZ
    mr.      r31, r3
    beq      .L_8013E14C
    lfs      f0, 0x84(r1)
    lis      r0, 0x7f80
    stfs     f0, 0x34(r1)
    lwz      r4, 0x34(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013DFD4
    bge      .L_8013E004
    cmpwi    r3, 0x0
    beq      .L_8013DFEC
    b        .L_8013E004
.L_8013DFD4:
    clrlwi.  r0, r4, 9
    beq      .L_8013DFE4
    li       r0, 0x1
    b        .L_8013E008
.L_8013DFE4:
    li       r0, 0x2
    b        .L_8013E008
.L_8013DFEC:
    clrlwi.  r0, r4, 9
    beq      .L_8013DFFC
    li       r0, 0x5
    b        .L_8013E008
.L_8013DFFC:
    li       r0, 0x3
    b        .L_8013E008
.L_8013E004:
    li       r0, 0x4
.L_8013E008:
    cmpwi    r0, 0x1
    beq      .L_8013E0E0
    lfs      f0, 0x88(r1)
    lis      r0, 0x7f80
    stfs     f0, 0x30(r1)
    lwz      r4, 0x30(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013E03C
    bge      .L_8013E06C
    cmpwi    r3, 0x0
    beq      .L_8013E054
    b        .L_8013E06C
.L_8013E03C:
    clrlwi.  r0, r4, 9
    beq      .L_8013E04C
    li       r0, 0x1
    b        .L_8013E070
.L_8013E04C:
    li       r0, 0x2
    b        .L_8013E070
.L_8013E054:
    clrlwi.  r0, r4, 9
    beq      .L_8013E064
    li       r0, 0x5
    b        .L_8013E070
.L_8013E064:
    li       r0, 0x3
    b        .L_8013E070
.L_8013E06C:
    li       r0, 0x4
.L_8013E070:
    cmpwi    r0, 0x1
    beq      .L_8013E0E0
    lfs      f0, 0x8c(r1)
    lis      r0, 0x7f80
    stfs     f0, 0x2c(r1)
    lwz      r4, 0x2c(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013E0A4
    bge      .L_8013E0D4
    cmpwi    r3, 0x0
    beq      .L_8013E0BC
    b        .L_8013E0D4
.L_8013E0A4:
    clrlwi.  r0, r4, 9
    beq      .L_8013E0B4
    li       r0, 0x1
    b        .L_8013E0D8
.L_8013E0B4:
    li       r0, 0x2
    b        .L_8013E0D8
.L_8013E0BC:
    clrlwi.  r0, r4, 9
    beq      .L_8013E0CC
    li       r0, 0x5
    b        .L_8013E0D8
.L_8013E0CC:
    li       r0, 0x3
    b        .L_8013E0D8
.L_8013E0D4:
    li       r0, 0x4
.L_8013E0D8:
    cmpwi    r0, 0x1
    bne      .L_8013E14C
.L_8013E0E0:
    lfs      f0, 0xa4(r1)
    addi     r3, r23, 0x3b4
    stfd     f0, 0x8(r1)
    lfs      f0, 0x90(r1)
    stfd     f0, 0x10(r1)
    lfs      f0, 0x94(r1)
    stfd     f0, 0x18(r1)
    lfs      f0, 0x98(r1)
    stfd     f0, 0x20(r1)
    lfs      f1, 0xb4(r1)
    lfs      f2, 0xb8(r1)
    lfs      f3, 0xbc(r1)
    lfs      f4, 0xa8(r1)
    lfs      f5, 0xac(r1)
    lfs      f6, 0xb0(r1)
    lfs      f7, 0x9c(r1)
    lfs      f8, 0xa0(r1)
    crset    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sMapCameraIntersectVecLabel@ha
    mr       r6, r3
    addi     r7, r4, sMapCameraIntersectVecLabel@l
    addi     r3, r23, 0x120
    li       r4, 0x887
    li       r5, lbl_8065E9C8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8013E14C:
    cmpwi    r31, 0x0
    beq      .L_8013E220
    lwz      r4, 0xd0(r23)
    lis      r3, __vt__4CVec@ha
    lwz      r7, 0xd4(r23)
    addi     r0, r3, __vt__4CVec@l
    lwz      r6, 0xd8(r23)
    addi     r3, r1, 0x50
    stw      r4, 0x78(r1)
    addi     r4, r1, 0x5c
    lfs      f0, lbl_8065E994@sda21(r0)
    addi     r5, r1, 0x68
    stw      r7, 0x7c(r1)
    lfs      f7, 0x78(r1)
    stw      r6, 0x80(r1)
    lfs      f6, 0x7c(r1)
    lfs      f5, 0x80(r1)
    lfs      f4, 0xb4(r1)
    lfs      f3, 0xa8(r1)
    lfs      f2, 0xbc(r1)
    lfs      f1, 0xb0(r1)
    fsubs    f3, f4, f3
    stfs     f7, 0x50(r1)
    fsubs    f1, f2, f1
    stfs     f6, 0x54(r1)
    stfs     f5, 0x58(r1)
    stfs     f3, 0x5c(r1)
    stfs     f1, 0x64(r1)
    stfs     f0, 0x60(r1)
    stw      r0, 0x74(r1)
    bl       C_VECCrossProduct
    addi     r3, r1, 0x68
    mr       r4, r3
    bl       PSVECNormalize
    lis      r3, __vt__4CVec@ha
    lfs      f1, 0x68(r1)
    addi     r0, r3, __vt__4CVec@l
    lfs      f2, 0x6c(r1)
    stw      r0, 0x74(r1)
    addi     r20, r20, 0x1
    lfs      f3, 0x70(r1)
    lfs      f0, 0x84(r1)
    stfs     f0, 0x0(r21)
    lfs      f0, 0x88(r1)
    stfs     f0, 0x4(r21)
    lfs      f0, 0x8c(r1)
    stfs     f0, 0x8(r21)
    stw      r17, 0x10(r21)
    stw      r18, 0xc(r21)
    stfs     f1, 0x14(r21)
    stfs     f2, 0x18(r21)
    stfs     f3, 0x1c(r21)
    addi     r21, r21, 0x20
.L_8013E220:
    addi     r17, r17, 0x1
    cmpwi    r17, 0x4
    blt      .L_8013DED4
    addi     r22, r22, 0x34
.L_8013E230:
    addi     r18, r18, 0x1
.L_8013E234:
    cmpw     r18, r14
    blt      .L_8013DD18
    mr       r3, r20
.L_8013E240:
    psq_l    f31, 0x148(r1), 0, qr0
    lfd      f31, 0x140(r1)
    psq_l    f30, 0x138(r1), 0, qr0
    lfd      f30, 0x130(r1)
    psq_l    f29, 0x128(r1), 0, qr0
    lfd      f29, 0x120(r1)
    lmw      r14, 0xd8(r1)
    lwz      r0, 0x154(r1)
    mtlr     r0
    addi     r1, r1, 0x150
    blr
*/

/**
 * @note Address: 0x8013E26C
 * @note Size: 0x17CC
 */
// ResolveMapCameraEyeCollision
/*
    stwu     r1, -0x370(r1)
    mflr     r0
    stw      r0, 0x374(r1)
    stfd     f31, 0x360(r1)
    psq_st   f31, 0x368(r1), 0, qr0
    stfd     f30, 0x350(r1)
    psq_st   f30, 0x358(r1), 0, qr0
    stmw     r14, 0x308(r1)
    lfs      f3, 0x0(r6)
    lis      r7, __vt__4CVec@ha
    addi     r7, r7, __vt__4CVec@l
    lfs      f2, 0x4(r6)
    lfs      f1, 0x8(r6)
    mr       r26, r4
    stw      r7, 0x2b8(r1)
    mr       r25, r3
    mr       r27, r8
    li       r0, 0x0
    stfs     f3, 0x2ac(r1)
    stfs     f2, 0x2b0(r1)
    stfs     f1, 0x2b4(r1)
    stw      r7, 0x2a8(r1)
    lfs      f0, 0x0(r5)
    stfs     f0, 0x29c(r1)
    lfs      f0, 0x4(r5)
    lwz      r3, 0x29c(r1)
    stfs     f0, 0x2a0(r1)
    lfs      f0, 0x8(r5)
    stw      r3, 0x17c(r1)
    lwz      r4, 0x2a0(r1)
    stfs     f0, 0x2a4(r1)
    lfs      f0, 0x17c(r1)
    stw      r7, 0x298(r1)
    lwz      r3, 0x2a4(r1)
    fcmpu    cr0, f3, f0
    lfs      f0, 0x0(r26)
    stw      r4, 0x180(r1)
    stfs     f0, 0x28c(r1)
    lfs      f0, 0x4(r26)
    stw      r3, 0x184(r1)
    stfs     f0, 0x290(r1)
    lfs      f0, 0x8(r26)
    stfs     f0, 0x294(r1)
    bne      .L_8013E338
    lfs      f0, 0x180(r1)
    fcmpu    cr0, f2, f0
    bne      .L_8013E338
    lfs      f0, 0x184(r1)
    fcmpu    cr0, f1, f0
    bne      .L_8013E338
    li       r0, 0x1
.L_8013E338:
    clrlwi.  r0, r0, 24
    beq      .L_8013E35C
    lis      r4, __vt__4CVec@ha
    li       r3, 0x0
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x298(r1)
    stw      r0, 0x2a8(r1)
    stw      r0, 0x2b8(r1)
    b        .L_8013FA14
.L_8013E35C:
    lis      r3, gCameraAreaState@ha
    lis      r4, gCameraCollisionPolygons@ha
    addi     r5, r3, gCameraAreaState@l
    li       r29, 0x0
    lwz      r14, 0x1c(r5)
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r31, r4, gCameraCollisionPolygons@l
    lwz      r30, 0x20(r5)
    mr       r17, r14
    addi     r15, r3, __vt__11CamAreaInfo@l
    mr       r16, r31
    li       r18, 0x0
    li       r28, 0x0
    li       r19, 0x0
    li       r20, 0x0
    b        .L_8013E620
.L_8013E39C:
    addic.   r0, r1, 0x54
    stw      r15, 0x54(r1)
    stw      r17, 0x58(r1)
    beq      .L_8013E3B8
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x54(r1)
.L_8013E3B8:
    lwz      r4, 0x58(r1)
    lhz      r0, 0x2e(r4)
    rlwinm.  r0, r0, 0, 24, 24
    bne      .L_8013E614
    lwz      r0, 0x28(r4)
    lis      r3, 0x4330
    lwz      r4, 0x24(r4)
    xoris    r0, r0, 0x8000
    stw      r3, 0x2c8(r1)
    xoris    r4, r4, 0x8000
    lfd      f3, lbl_8065E9E8@sda21(r0)
    stw      r0, 0x2cc(r1)
    lfs      f1, lbl_8065E9DC@sda21(r0)
    lfd      f0, 0x2c8(r1)
    stw      r4, 0x2c4(r1)
    fsubs    f0, f0, f3
    lfs      f4, 0x2b0(r1)
    stw      r3, 0x2c0(r1)
    lfd      f2, 0x2c0(r1)
    fdivs    f5, f0, f1
    fsubs    f0, f2, f3
    fcmpo    cr0, f4, f5
    fdivs    f1, f0, f1
    cror     eq, gt, eq
    bne      .L_8013E430
    fcmpo    cr0, f4, f1
    cror     eq, lt, eq
    bne      .L_8013E430
    li       r0, 0x1
    b        .L_8013E454
.L_8013E430:
    fcmpo    cr0, f4, f5
    bge      .L_8013E440
    li       r0, -0x2
    b        .L_8013E454
.L_8013E440:
    fcmpo    cr0, f4, f1
    ble      .L_8013E450
    li       r0, -0x1
    b        .L_8013E454
.L_8013E450:
    li       r0, 0x0
.L_8013E454:
    srwi.    r0, r0, 31
    beq      .L_8013E4A8
    lfs      f0, 0x4(r26)
    fcmpo    cr0, f0, f5
    cror     eq, gt, eq
    bne      .L_8013E480
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_8013E480
    li       r0, 0x1
    b        .L_8013E4A4
.L_8013E480:
    fcmpo    cr0, f0, f5
    bge      .L_8013E490
    li       r0, -0x2
    b        .L_8013E4A4
.L_8013E490:
    fcmpo    cr0, f0, f1
    ble      .L_8013E4A0
    li       r0, -0x1
    b        .L_8013E4A4
.L_8013E4A0:
    li       r0, 0x0
.L_8013E4A4:
    srwi     r0, r0, 31
.L_8013E4A8:
    clrlwi.  r0, r0, 24
    bne      .L_8013E614
    lwz      r3, 0x4(r17)
    lis      r0, 0x4330
    stw      r0, 0x2c8(r1)
    mr       r4, r16
    xoris    r3, r3, 0x8000
    lfd      f3, lbl_8065E9E8@sda21(r0)
    stw      r3, 0x2cc(r1)
    addi     r3, r1, 0x2ac
    lfs      f2, lbl_8065E9DC@sda21(r0)
    addi     r5, r16, 0xc
    lfd      f0, 0x2c8(r1)
    addi     r6, r16, 0x18
    stw      r0, 0x2c0(r1)
    addi     r7, r16, 0x24
    fsubs    f0, f0, f3
    lfs      f1, lbl_8065E994@sda21(r0)
    stw      r0, 0x2d0(r1)
    fdivs    f0, f0, f2
    stw      r0, 0x2d8(r1)
    stw      r0, 0x2e0(r1)
    stw      r0, 0x2e8(r1)
    stw      r0, 0x2f0(r1)
    stw      r0, 0x2f8(r1)
    stfs     f0, 0x30(r16)
    stfs     f0, 0x0(r16)
    lwz      r0, 0x8(r17)
    xoris    r0, r0, 0x8000
    stw      r0, 0x2c4(r1)
    lfd      f0, 0x2c0(r1)
    fsubs    f0, f0, f3
    fdivs    f0, f0, f2
    stfs     f0, 0x38(r16)
    stfs     f0, 0x8(r16)
    stfs     f1, 0x34(r16)
    stfs     f1, 0x4(r16)
    lwz      r0, 0xc(r17)
    xoris    r0, r0, 0x8000
    stw      r0, 0x2d4(r1)
    lfd      f0, 0x2d0(r1)
    fsubs    f0, f0, f3
    fdivs    f0, f0, f2
    stfs     f0, 0xc(r16)
    lwz      r0, 0x10(r17)
    xoris    r0, r0, 0x8000
    stw      r0, 0x2dc(r1)
    lfd      f0, 0x2d8(r1)
    fsubs    f0, f0, f3
    fdivs    f0, f0, f2
    stfs     f0, 0x14(r16)
    stfs     f1, 0x10(r16)
    lwz      r0, 0x14(r17)
    xoris    r0, r0, 0x8000
    stw      r0, 0x2e4(r1)
    lfd      f0, 0x2e0(r1)
    fsubs    f0, f0, f3
    fdivs    f0, f0, f2
    stfs     f0, 0x18(r16)
    lwz      r0, 0x18(r17)
    xoris    r0, r0, 0x8000
    stw      r0, 0x2ec(r1)
    lfd      f0, 0x2e8(r1)
    fsubs    f0, f0, f3
    fdivs    f0, f0, f2
    stfs     f0, 0x20(r16)
    stfs     f1, 0x1c(r16)
    lwz      r0, 0x1c(r17)
    xoris    r0, r0, 0x8000
    stw      r0, 0x2f4(r1)
    lfd      f0, 0x2f0(r1)
    fsubs    f0, f0, f3
    fdivs    f0, f0, f2
    stfs     f0, 0x24(r16)
    lwz      r0, 0x20(r17)
    xoris    r0, r0, 0x8000
    stw      r0, 0x2fc(r1)
    lfd      f0, 0x2f8(r1)
    fsubs    f0, f0, f3
    fdivs    f0, f0, f2
    stfs     f0, 0x2c(r16)
    stfs     f1, 0x28(r16)
    bl       IsPointInsideQuadXZ
    or       r18, r18, r3
    mr       r4, r16
    addi     r3, r1, 0x29c
    addi     r5, r16, 0xc
    addi     r6, r16, 0x18
    addi     r7, r16, 0x24
    bl       IsPointInsideQuadXZ
    or       r28, r28, r3
.L_8013E614:
    addi     r16, r16, 0x3c
    addi     r20, r20, 0x1
    addi     r17, r17, 0x34
.L_8013E620:
    cmpw     r20, r30
    blt      .L_8013E39C
    cmpwi    r28, 0x0
    beq      .L_8013EEF0
    lis      r3, __float_nan@ha
    lis      r4, __vt__4CVec@ha
    addi     r5, r3, __float_nan@l
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r4, __vt__4CVec@l
    lfs      f31, 0x0(r5)
    stw      r0, 0x288(r1)
    mr       r17, r14
    mr       r20, r31
    addi     r22, r3, __vt__11CamAreaInfo@l
    li       r16, 0x0
    b        .L_8013E930
.L_8013E660:
    addic.   r0, r1, 0x4c
    stw      r22, 0x4c(r1)
    stw      r17, 0x50(r1)
    beq      .L_8013E67C
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x4c(r1)
.L_8013E67C:
    lwz      r4, 0x50(r1)
    lhz      r0, 0x2e(r4)
    rlwinm.  r0, r0, 0, 24, 24
    bne      .L_8013E924
    lwz      r0, 0x28(r4)
    lis      r3, 0x4330
    lwz      r4, 0x24(r4)
    xoris    r0, r0, 0x8000
    stw      r3, 0x2f0(r1)
    xoris    r4, r4, 0x8000
    lfd      f3, lbl_8065E9E8@sda21(r0)
    stw      r0, 0x2f4(r1)
    lfs      f1, lbl_8065E9DC@sda21(r0)
    lfd      f0, 0x2f0(r1)
    stw      r4, 0x2fc(r1)
    fsubs    f0, f0, f3
    lfs      f4, 0x2b0(r1)
    stw      r3, 0x2f8(r1)
    lfd      f2, 0x2f8(r1)
    fdivs    f5, f0, f1
    fsubs    f0, f2, f3
    fcmpo    cr0, f4, f5
    fdivs    f1, f0, f1
    cror     eq, gt, eq
    bne      .L_8013E6F4
    fcmpo    cr0, f4, f1
    cror     eq, lt, eq
    bne      .L_8013E6F4
    li       r0, 0x1
    b        .L_8013E718
.L_8013E6F4:
    fcmpo    cr0, f4, f5
    bge      .L_8013E704
    li       r0, -0x2
    b        .L_8013E718
.L_8013E704:
    fcmpo    cr0, f4, f1
    ble      .L_8013E714
    li       r0, -0x1
    b        .L_8013E718
.L_8013E714:
    li       r0, 0x0
.L_8013E718:
    srwi.    r0, r0, 31
    beq      .L_8013E76C
    lfs      f0, 0x4(r26)
    fcmpo    cr0, f0, f5
    cror     eq, gt, eq
    bne      .L_8013E744
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_8013E744
    li       r0, 0x1
    b        .L_8013E768
.L_8013E744:
    fcmpo    cr0, f0, f5
    bge      .L_8013E754
    li       r0, -0x2
    b        .L_8013E768
.L_8013E754:
    fcmpo    cr0, f0, f1
    ble      .L_8013E764
    li       r0, -0x1
    b        .L_8013E768
.L_8013E764:
    li       r0, 0x0
.L_8013E768:
    srwi     r0, r0, 31
.L_8013E76C:
    clrlwi.  r0, r0, 24
    bne      .L_8013E924
    mr       r4, r20
    addi     r3, r1, 0x28c
    addi     r5, r20, 0xc
    addi     r6, r20, 0x18
    addi     r7, r20, 0x24
    bl       IsPointInsideQuadXZ
    cmpwi    r3, 0x0
    beq      .L_8013E924
    lis      r3, __vt__4CVec@ha
    lfs      f30, lbl_8065E994@sda21(r0)
    addi     r24, r3, __vt__4CVec@l
    mr       r21, r20
    stw      r24, 0x278(r1)
    li       r15, 0x0
    lis      r23, 0x7f80
    stw      r24, 0x268(r1)
.L_8013E7B4:
    addi     r0, r15, 0x1
    mr       r4, r21
    mulli    r0, r0, 0xc
    addi     r3, r1, 0x28c
    addi     r6, r1, 0x26c
    add      r19, r20, r0
    mr       r5, r19
    bl       GetOutwardPerpendicularToEdge
    addi     r3, r1, 0x26c
    stfs     f30, 0x270(r1)
    mr       r4, r3
    bl       PSVECNormalize
    lwz      r7, 0x26c(r1)
    addi     r3, r1, 0x160
    lwz      r6, 0x270(r1)
    addi     r4, r1, 0x28c
    lwz      r0, 0x274(r1)
    addi     r5, r1, 0x170
    stw      r7, 0x170(r1)
    stw      r6, 0x174(r1)
    stw      r0, 0x178(r1)
    bl       fn_8000F93C
    mr       r5, r21
    mr       r6, r19
    addi     r3, r1, 0x28c
    addi     r4, r1, 0x160
    addi     r7, r1, 0x25c
    li       r8, 0x0
    bl       IntersectLinesXZ
    stw      r24, 0x16c(r1)
    addi     r3, r1, 0x28c
    addi     r4, r1, 0x25c
    bl       PSVECDistance
    stfs     f31, 0x28(r1)
    lwz      r3, 0x28(r1)
    rlwinm   r0, r3, 0, 1, 8
    cmpw     r0, r23
    beq      .L_8013E85C
    bge      .L_8013E88C
    cmpwi    r0, 0x0
    beq      .L_8013E874
    b        .L_8013E88C
.L_8013E85C:
    clrlwi.  r0, r3, 9
    beq      .L_8013E86C
    li       r0, 0x1
    b        .L_8013E890
.L_8013E86C:
    li       r0, 0x2
    b        .L_8013E890
.L_8013E874:
    clrlwi.  r0, r3, 9
    beq      .L_8013E884
    li       r0, 0x5
    b        .L_8013E890
.L_8013E884:
    li       r0, 0x3
    b        .L_8013E890
.L_8013E88C:
    li       r0, 0x4
.L_8013E890:
    cmpwi    r0, 0x1
    beq      .L_8013E8A0
    fcmpo    cr0, f31, f1
    ble      .L_8013E904
.L_8013E8A0:
    fmr      f31, f1
    lfs      f1, lbl_8065EA04@sda21(r0)
    addi     r3, r1, 0x144
    addi     r4, r1, 0x26c
    bl       fn_8000F9C0
    lwz      r7, 0x144(r1)
    addi     r3, r1, 0x134
    lwz      r6, 0x148(r1)
    addi     r4, r1, 0x25c
    lwz      r0, 0x14c(r1)
    addi     r5, r1, 0x154
    stw      r7, 0x154(r1)
    stw      r6, 0x158(r1)
    stw      r0, 0x15c(r1)
    bl       fn_8000F93C
    lfs      f2, 0x134(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0x138(r1)
    lfs      f0, 0x13c(r1)
    stfs     f2, 0x27c(r1)
    stfs     f1, 0x280(r1)
    stfs     f0, 0x284(r1)
    stw      r0, 0x140(r1)
    stw      r0, 0x150(r1)
.L_8013E904:
    addi     r15, r15, 0x1
    addi     r21, r21, 0xc
    cmpwi    r15, 0x4
    blt      .L_8013E7B4
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x268(r1)
    stw      r0, 0x278(r1)
.L_8013E924:
    addi     r20, r20, 0x3c
    addi     r16, r16, 0x1
    addi     r17, r17, 0x34
.L_8013E930:
    cmpw     r16, r30
    blt      .L_8013E660
    stfs     f31, 0x24(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x24(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013E960
    bge      .L_8013E990
    cmpwi    r3, 0x0
    beq      .L_8013E978
    b        .L_8013E990
.L_8013E960:
    clrlwi.  r0, r4, 9
    beq      .L_8013E970
    li       r0, 0x1
    b        .L_8013E994
.L_8013E970:
    li       r0, 0x2
    b        .L_8013E994
.L_8013E978:
    clrlwi.  r0, r4, 9
    beq      .L_8013E988
    li       r0, 0x5
    b        .L_8013E994
.L_8013E988:
    li       r0, 0x3
    b        .L_8013E994
.L_8013E990:
    li       r0, 0x4
.L_8013E994:
    cmpwi    r0, 0x1
    beq      .L_8013EEE0
    lfs      f2, 0x27c(r1)
    lis      r3, __vt__11CamAreaInfo@ha
    lfs      f1, 0x280(r1)
    mr       r17, r14
    lfs      f0, 0x284(r1)
    mr       r16, r31
    stfs     f2, 0x28c(r1)
    addi     r15, r3, __vt__11CamAreaInfo@l
    li       r28, 0x0
    li       r19, 0x0
    stfs     f1, 0x290(r1)
    li       r20, 0x0
    stfs     f0, 0x294(r1)
    b        .L_8013EB1C
.L_8013E9D4:
    addic.   r0, r1, 0x44
    stw      r15, 0x44(r1)
    stw      r17, 0x48(r1)
    beq      .L_8013E9F0
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x44(r1)
.L_8013E9F0:
    lwz      r4, 0x48(r1)
    lhz      r0, 0x2e(r4)
    rlwinm.  r0, r0, 0, 24, 24
    bne      .L_8013EB10
    lwz      r0, 0x28(r4)
    lis      r3, 0x4330
    lwz      r4, 0x24(r4)
    xoris    r0, r0, 0x8000
    stw      r3, 0x2f0(r1)
    xoris    r4, r4, 0x8000
    lfd      f3, lbl_8065E9E8@sda21(r0)
    stw      r0, 0x2f4(r1)
    lfs      f1, lbl_8065E9DC@sda21(r0)
    lfd      f0, 0x2f0(r1)
    stw      r4, 0x2fc(r1)
    fsubs    f0, f0, f3
    lfs      f4, 0x2b0(r1)
    stw      r3, 0x2f8(r1)
    lfd      f2, 0x2f8(r1)
    fdivs    f5, f0, f1
    fsubs    f0, f2, f3
    fcmpo    cr0, f4, f5
    fdivs    f1, f0, f1
    cror     eq, gt, eq
    bne      .L_8013EA68
    fcmpo    cr0, f4, f1
    cror     eq, lt, eq
    bne      .L_8013EA68
    li       r0, 0x1
    b        .L_8013EA8C
.L_8013EA68:
    fcmpo    cr0, f4, f5
    bge      .L_8013EA78
    li       r0, -0x2
    b        .L_8013EA8C
.L_8013EA78:
    fcmpo    cr0, f4, f1
    ble      .L_8013EA88
    li       r0, -0x1
    b        .L_8013EA8C
.L_8013EA88:
    li       r0, 0x0
.L_8013EA8C:
    srwi.    r0, r0, 31
    beq      .L_8013EAE0
    lfs      f0, 0x4(r26)
    fcmpo    cr0, f0, f5
    cror     eq, gt, eq
    bne      .L_8013EAB8
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_8013EAB8
    li       r0, 0x1
    b        .L_8013EADC
.L_8013EAB8:
    fcmpo    cr0, f0, f5
    bge      .L_8013EAC8
    li       r0, -0x2
    b        .L_8013EADC
.L_8013EAC8:
    fcmpo    cr0, f0, f1
    ble      .L_8013EAD8
    li       r0, -0x1
    b        .L_8013EADC
.L_8013EAD8:
    li       r0, 0x0
.L_8013EADC:
    srwi     r0, r0, 31
.L_8013EAE0:
    clrlwi.  r0, r0, 24
    bne      .L_8013EB10
    mr       r4, r16
    addi     r3, r1, 0x28c
    addi     r5, r16, 0xc
    addi     r6, r16, 0x18
    addi     r7, r16, 0x24
    bl       IsPointInsideQuadXZ
    cmpwi    r3, 0x0
    beq      .L_8013EB10
    li       r19, 0x1
    b        .L_8013EB24
.L_8013EB10:
    addi     r16, r16, 0x3c
    addi     r20, r20, 0x1
    addi     r17, r17, 0x34
.L_8013EB1C:
    cmpw     r20, r30
    blt      .L_8013E9D4
.L_8013EB24:
    cmpwi    r19, 0x0
    beq      .L_8013EEE0
    lis      r3, __vt__11CamAreaInfo@ha
    mr       r20, r14
    mr       r17, r31
    li       r21, 0x0
    addi     r15, r3, __vt__11CamAreaInfo@l
    b        .L_8013EE58
.L_8013EB44:
    addic.   r0, r1, 0x3c
    stw      r15, 0x3c(r1)
    stw      r20, 0x40(r1)
    beq      .L_8013EB60
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x3c(r1)
.L_8013EB60:
    lwz      r4, 0x40(r1)
    lhz      r0, 0x2e(r4)
    rlwinm.  r0, r0, 0, 24, 24
    bne      .L_8013EE4C
    lwz      r0, 0x28(r4)
    lis      r3, 0x4330
    lwz      r4, 0x24(r4)
    xoris    r0, r0, 0x8000
    stw      r3, 0x2f0(r1)
    xoris    r4, r4, 0x8000
    lfd      f3, lbl_8065E9E8@sda21(r0)
    stw      r0, 0x2f4(r1)
    lfs      f1, lbl_8065E9DC@sda21(r0)
    lfd      f0, 0x2f0(r1)
    stw      r4, 0x2fc(r1)
    fsubs    f0, f0, f3
    lfs      f4, 0x2b0(r1)
    stw      r3, 0x2f8(r1)
    lfd      f2, 0x2f8(r1)
    fdivs    f5, f0, f1
    fsubs    f0, f2, f3
    fcmpo    cr0, f4, f5
    fdivs    f1, f0, f1
    cror     eq, gt, eq
    bne      .L_8013EBD8
    fcmpo    cr0, f4, f1
    cror     eq, lt, eq
    bne      .L_8013EBD8
    li       r0, 0x1
    b        .L_8013EBFC
.L_8013EBD8:
    fcmpo    cr0, f4, f5
    bge      .L_8013EBE8
    li       r0, -0x2
    b        .L_8013EBFC
.L_8013EBE8:
    fcmpo    cr0, f4, f1
    ble      .L_8013EBF8
    li       r0, -0x1
    b        .L_8013EBFC
.L_8013EBF8:
    li       r0, 0x0
.L_8013EBFC:
    srwi.    r0, r0, 31
    beq      .L_8013EC50
    lfs      f0, 0x4(r26)
    fcmpo    cr0, f0, f5
    cror     eq, gt, eq
    bne      .L_8013EC28
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_8013EC28
    li       r0, 0x1
    b        .L_8013EC4C
.L_8013EC28:
    fcmpo    cr0, f0, f5
    bge      .L_8013EC38
    li       r0, -0x2
    b        .L_8013EC4C
.L_8013EC38:
    fcmpo    cr0, f0, f1
    ble      .L_8013EC48
    li       r0, -0x1
    b        .L_8013EC4C
.L_8013EC48:
    li       r0, 0x0
.L_8013EC4C:
    srwi     r0, r0, 31
.L_8013EC50:
    clrlwi.  r0, r0, 24
    bne      .L_8013EE4C
    cmpwi    r18, 0x0
    beq      .L_8013EC6C
    lwz      r3, 0x44(r25)
    addi     r6, r3, 0xc8
    b        .L_8013EC70
.L_8013EC6C:
    addi     r6, r1, 0x2ac
.L_8013EC70:
    lwz      r5, 0x0(r6)
    lis      r3, __vt__4CVec@ha
    lwz      r4, 0x4(r6)
    addi     r0, r3, __vt__4CVec@l
    mr       r16, r17
    li       r22, 0x0
    stw      r5, 0x24c(r1)
    stw      r4, 0x250(r1)
    lwz      r3, 0x8(r6)
    stw      r3, 0x254(r1)
    stw      r0, 0x258(r1)
    stw      r0, 0x248(r1)
    stw      r0, 0x238(r1)
.L_8013ECA4:
    addi     r0, r22, 0x1
    mr       r5, r16
    mulli    r0, r0, 0xc
    addi     r3, r1, 0x24c
    addi     r4, r1, 0x29c
    addi     r7, r1, 0x22c
    add      r19, r17, r0
    li       r8, 0x1
    mr       r6, r19
    bl       IntersectLinesXZ
    cmpwi    r3, 0x0
    beq      .L_8013EE28
    mr       r4, r16
    mr       r5, r19
    addi     r3, r1, 0x29c
    addi     r6, r1, 0x23c
    bl       GetOutwardPerpendicularToEdge
    lfs      f0, lbl_8065E994@sda21(r0)
    addi     r3, r1, 0x23c
    mr       r4, r3
    stfs     f0, 0x240(r1)
    bl       PSVECNormalize
    lwz      r7, 0x23c(r1)
    addi     r3, r1, 0x118
    lwz      r6, 0x240(r1)
    addi     r4, r1, 0x29c
    lwz      r0, 0x244(r1)
    addi     r5, r1, 0x128
    stw      r7, 0x128(r1)
    stw      r6, 0x12c(r1)
    stw      r0, 0x130(r1)
    bl       fn_8000F93C
    mr       r5, r16
    mr       r6, r19
    addi     r3, r1, 0x29c
    addi     r4, r1, 0x118
    addi     r7, r1, 0x22c
    li       r8, 0x0
    bl       IntersectLinesXZ
    lis      r4, __vt__4CVec@ha
    addi     r3, r1, 0x2ac
    addi     r0, r4, __vt__4CVec@l
    addi     r4, r1, 0x22c
    stw      r0, 0x124(r1)
    bl       PSVECDistance
    stfs     f31, 0x20(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x20(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013ED80
    bge      .L_8013EDB0
    cmpwi    r3, 0x0
    beq      .L_8013ED98
    b        .L_8013EDB0
.L_8013ED80:
    clrlwi.  r0, r4, 9
    beq      .L_8013ED90
    li       r0, 0x1
    b        .L_8013EDB4
.L_8013ED90:
    li       r0, 0x2
    b        .L_8013EDB4
.L_8013ED98:
    clrlwi.  r0, r4, 9
    beq      .L_8013EDA8
    li       r0, 0x5
    b        .L_8013EDB4
.L_8013EDA8:
    li       r0, 0x3
    b        .L_8013EDB4
.L_8013EDB0:
    li       r0, 0x4
.L_8013EDB4:
    cmpwi    r0, 0x1
    beq      .L_8013EDC4
    fcmpo    cr0, f31, f1
    ble      .L_8013EE28
.L_8013EDC4:
    fmr      f31, f1
    lfs      f1, lbl_8065EA04@sda21(r0)
    addi     r3, r1, 0xfc
    addi     r4, r1, 0x23c
    bl       fn_8000F9C0
    lwz      r7, 0xfc(r1)
    addi     r3, r1, 0xec
    lwz      r6, 0x100(r1)
    addi     r4, r1, 0x22c
    lwz      r0, 0x104(r1)
    addi     r5, r1, 0x10c
    stw      r7, 0x10c(r1)
    stw      r6, 0x110(r1)
    stw      r0, 0x114(r1)
    bl       fn_8000F93C
    lfs      f2, 0xec(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0xf0(r1)
    lfs      f0, 0xf4(r1)
    stfs     f2, 0x27c(r1)
    stfs     f1, 0x280(r1)
    stfs     f0, 0x284(r1)
    stw      r0, 0xf8(r1)
    stw      r0, 0x108(r1)
.L_8013EE28:
    addi     r22, r22, 0x1
    addi     r16, r16, 0xc
    cmpwi    r22, 0x4
    blt      .L_8013ECA4
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x238(r1)
    stw      r0, 0x248(r1)
    stw      r0, 0x258(r1)
.L_8013EE4C:
    addi     r17, r17, 0x3c
    addi     r21, r21, 0x1
    addi     r20, r20, 0x34
.L_8013EE58:
    cmpw     r21, r30
    blt      .L_8013EB44
    stfs     f31, 0x1c(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x1c(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013EE88
    bge      .L_8013EEB8
    cmpwi    r3, 0x0
    beq      .L_8013EEA0
    b        .L_8013EEB8
.L_8013EE88:
    clrlwi.  r0, r4, 9
    beq      .L_8013EE98
    li       r0, 0x1
    b        .L_8013EEBC
.L_8013EE98:
    li       r0, 0x2
    b        .L_8013EEBC
.L_8013EEA0:
    clrlwi.  r0, r4, 9
    beq      .L_8013EEB0
    li       r0, 0x5
    b        .L_8013EEBC
.L_8013EEB0:
    li       r0, 0x3
    b        .L_8013EEBC
.L_8013EEB8:
    li       r0, 0x4
.L_8013EEBC:
    cmpwi    r0, 0x1
    beq      .L_8013EEE0
    lfs      f2, 0x27c(r1)
    li       r28, 0x0
    lfs      f1, 0x280(r1)
    lfs      f0, 0x284(r1)
    stfs     f2, 0x28c(r1)
    stfs     f1, 0x290(r1)
    stfs     f0, 0x294(r1)
.L_8013EEE0:
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x288(r1)
    b        .L_8013F840
.L_8013EEF0:
    lwz      r3, 0x44(r25)
    lfs      f30, 0xfc(r3)
    fmr      f1, f30
    bl       sin
    lwz      r3, 0x44(r25)
    frsp     f3, f1
    lfs      f2, lbl_8065EA04@sda21(r0)
    fmr      f1, f30
    lfs      f0, 0xc8(r3)
    fnmsubs  f0, f2, f3, f0
    stfs     f0, 0x220(r1)
    bl       cos
    lwz      r4, 0x44(r25)
    lis      r3, __vt__11CamAreaInfo@ha
    frsp     f3, f1
    lfs      f2, lbl_8065EA04@sda21(r0)
    lfs      f1, 0xd0(r4)
    mr       r15, r14
    lfs      f0, 0x2b0(r1)
    mr       r21, r31
    fnmsubs  f1, f2, f3, f1
    stfs     f0, 0x224(r1)
    addi     r23, r3, __vt__11CamAreaInfo@l
    li       r16, 0x0
    stfs     f1, 0x228(r1)
    b        .L_8013F838
.L_8013EF58:
    addic.   r0, r1, 0x34
    stw      r23, 0x34(r1)
    stw      r15, 0x38(r1)
    beq      .L_8013EF74
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x34(r1)
.L_8013EF74:
    lwz      r4, 0x38(r1)
    lhz      r0, 0x2e(r4)
    rlwinm.  r0, r0, 0, 24, 24
    bne      .L_8013F82C
    lwz      r0, 0x28(r4)
    lis      r3, 0x4330
    lwz      r4, 0x24(r4)
    xoris    r0, r0, 0x8000
    stw      r3, 0x2f0(r1)
    xoris    r4, r4, 0x8000
    lfd      f3, lbl_8065E9E8@sda21(r0)
    stw      r0, 0x2f4(r1)
    lfs      f1, lbl_8065E9DC@sda21(r0)
    lfd      f0, 0x2f0(r1)
    stw      r4, 0x2fc(r1)
    fsubs    f0, f0, f3
    lfs      f4, 0x2b0(r1)
    stw      r3, 0x2f8(r1)
    lfd      f2, 0x2f8(r1)
    fdivs    f5, f0, f1
    fsubs    f0, f2, f3
    fcmpo    cr0, f4, f5
    fdivs    f1, f0, f1
    cror     eq, gt, eq
    bne      .L_8013EFEC
    fcmpo    cr0, f4, f1
    cror     eq, lt, eq
    bne      .L_8013EFEC
    li       r0, 0x1
    b        .L_8013F010
.L_8013EFEC:
    fcmpo    cr0, f4, f5
    bge      .L_8013EFFC
    li       r0, -0x2
    b        .L_8013F010
.L_8013EFFC:
    fcmpo    cr0, f4, f1
    ble      .L_8013F00C
    li       r0, -0x1
    b        .L_8013F010
.L_8013F00C:
    li       r0, 0x0
.L_8013F010:
    srwi.    r0, r0, 31
    beq      .L_8013F064
    lfs      f0, 0x4(r26)
    fcmpo    cr0, f0, f5
    cror     eq, gt, eq
    bne      .L_8013F03C
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_8013F03C
    li       r0, 0x1
    b        .L_8013F060
.L_8013F03C:
    fcmpo    cr0, f0, f5
    bge      .L_8013F04C
    li       r0, -0x2
    b        .L_8013F060
.L_8013F04C:
    fcmpo    cr0, f0, f1
    ble      .L_8013F05C
    li       r0, -0x1
    b        .L_8013F060
.L_8013F05C:
    li       r0, 0x0
.L_8013F060:
    srwi     r0, r0, 31
.L_8013F064:
    clrlwi.  r0, r0, 24
    bne      .L_8013F82C
    lis      r3, __vt__4CVec@ha
    mr       r4, r21
    addi     r0, r3, __vt__4CVec@l
    addi     r3, r1, 0x2ac
    stw      r0, 0x21c(r1)
    addi     r5, r21, 0xc
    addi     r6, r21, 0x18
    addi     r7, r21, 0x24
    stw      r0, 0x20c(r1)
    stw      r0, 0x1fc(r1)
    bl       IsPointInsideQuadXZ
    mr.      r17, r3
    bne      .L_8013F0BC
    mr       r4, r21
    addi     r3, r1, 0x220
    addi     r5, r21, 0xc
    addi     r6, r21, 0x18
    addi     r7, r21, 0x24
    bl       IsPointInsideQuadXZ
    mr       r17, r3
.L_8013F0BC:
    mr       r20, r21
    li       r18, -0x1
    li       r22, 0x0
    li       r24, 0x0
.L_8013F0CC:
    addi     r0, r22, 0x1
    stw      r24, 0x8(r1)
    mulli    r0, r0, 0xc
    lfs      f1, lbl_8065EA04@sda21(r0)
    lfs      f2, lbl_8065E994@sda21(r0)
    mr       r5, r20
    addi     r3, r1, 0x29c
    addi     r4, r1, 0x28c
    add      r6, r21, r0
    addi     r7, r1, 0x210
    addi     r8, r1, 0x200
    li       r9, 0x0
    li       r10, 0x0
    bl       ResolveCircleWallCollisionXZ
    cmplwi   r3, 0x0
    beq      .L_8013F114
    mr       r18, r22
    b        .L_8013F124
.L_8013F114:
    addi     r22, r22, 0x1
    addi     r20, r20, 0xc
    cmpwi    r22, 0x4
    blt      .L_8013F0CC
.L_8013F124:
    mr       r4, r21
    addi     r3, r1, 0x28c
    addi     r5, r21, 0xc
    addi     r6, r21, 0x18
    addi     r7, r21, 0x24
    bl       IsPointInsideQuadXZ
    cmpwi    r17, 0x0
    or       r19, r19, r3
    bne      .L_8013F58C
    cmpwi    r18, 0x0
    blt      .L_8013F818
    lis      r3, __vt__4CVec@ha
    lis      r4, __float_nan@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f30, __float_nan@l(r4)
    stw      r0, 0x1ec(r1)
    mr       r17, r21
    li       r20, -0x1
    li       r22, 0x0
    stw      r0, 0x1dc(r1)
.L_8013F174:
    addi     r0, r22, 0x1
    li       r3, 0x0
    mulli    r0, r0, 0xc
    stw      r3, 0x8(r1)
    lfs      f1, lbl_8065EA04@sda21(r0)
    mr       r5, r17
    lfs      f2, lbl_8065E994@sda21(r0)
    addi     r3, r1, 0x2ac
    addi     r4, r1, 0x28c
    add      r6, r21, r0
    addi     r7, r1, 0x210
    addi     r8, r1, 0x1d0
    li       r9, 0x0
    li       r10, 0x0
    bl       ResolveCircleWallCollisionXZ
    cmplwi   r3, 0x0
    beq      .L_8013F250
    addi     r3, r1, 0x1d0
    addi     r4, r1, 0x2ac
    bl       PSVECDistance
    stfs     f30, 0x18(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x18(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013F1EC
    bge      .L_8013F21C
    cmpwi    r3, 0x0
    beq      .L_8013F204
    b        .L_8013F21C
.L_8013F1EC:
    clrlwi.  r0, r4, 9
    beq      .L_8013F1FC
    li       r0, 0x1
    b        .L_8013F220
.L_8013F1FC:
    li       r0, 0x2
    b        .L_8013F220
.L_8013F204:
    clrlwi.  r0, r4, 9
    beq      .L_8013F214
    li       r0, 0x5
    b        .L_8013F220
.L_8013F214:
    li       r0, 0x3
    b        .L_8013F220
.L_8013F21C:
    li       r0, 0x4
.L_8013F220:
    cmpwi    r0, 0x1
    beq      .L_8013F230
    fcmpo    cr0, f30, f1
    ble      .L_8013F250
.L_8013F230:
    lfs      f2, 0x1d0(r1)
    fmr      f30, f1
    lfs      f1, 0x1d4(r1)
    mr       r20, r22
    lfs      f0, 0x1d8(r1)
    stfs     f2, 0x1e0(r1)
    stfs     f1, 0x1e4(r1)
    stfs     f0, 0x1e8(r1)
.L_8013F250:
    addi     r22, r22, 0x1
    addi     r17, r17, 0xc
    cmpwi    r22, 0x4
    blt      .L_8013F174
    cmpwi    r20, 0x0
    blt      .L_8013F578
    lwz      r0, 0x0(r27)
    cmpwi    r0, 0x1
    bne      .L_8013F4CC
    lfs      f2, 0x1e0(r1)
    li       r29, 0x1
    lfs      f1, 0x1e4(r1)
    lfs      f0, 0x1e8(r1)
    stfs     f2, 0x28c(r1)
    lfs      f2, 0x2b4(r1)
    stfs     f1, 0x290(r1)
    lfs      f4, 0x2ac(r1)
    stfs     f0, 0x294(r1)
    lfs      f1, lbl_8065E994@sda21(r0)
    lfs      f0, 0x4(r26)
    stfs     f0, 0x290(r1)
    lwz      r3, 0x44(r25)
    lfs      f0, 0xd0(r3)
    lfs      f3, 0xc8(r3)
    fsubs    f0, f2, f0
    fsubs    f2, f4, f3
    fmuls    f0, f0, f0
    fmadds   f0, f2, f2, f0
    fcmpo    cr0, f0, f1
    ble      .L_8013F310
    frsqrte  f2, f0
    lfd      f4, lbl_8065E9B0@sda21(r0)
    lfd      f3, lbl_8065E9B8@sda21(r0)
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f0, f1, f3
    fmul     f2, f2, f1
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f0, f1, f3
    fmul     f2, f2, f1
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f0, f1, f3
    fmul     f1, f2, f1
    fmul     f0, f0, f1
    frsp     f0, f0
    b        .L_8013F394
.L_8013F310:
    lfd      f1, lbl_8065E9C0@sda21(r0)
    fcmpo    cr0, f0, f1
    bge      .L_8013F328
    lis      r3, __float_nan@ha
    lfs      f0, __float_nan@l(r3)
    b        .L_8013F394
.L_8013F328:
    stfs     f0, 0x14(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x14(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013F350
    bge      .L_8013F380
    cmpwi    r3, 0x0
    beq      .L_8013F368
    b        .L_8013F380
.L_8013F350:
    clrlwi.  r0, r4, 9
    beq      .L_8013F360
    li       r0, 0x1
    b        .L_8013F384
.L_8013F360:
    li       r0, 0x2
    b        .L_8013F384
.L_8013F368:
    clrlwi.  r0, r4, 9
    beq      .L_8013F378
    li       r0, 0x5
    b        .L_8013F384
.L_8013F378:
    li       r0, 0x3
    b        .L_8013F384
.L_8013F380:
    li       r0, 0x4
.L_8013F384:
    cmpwi    r0, 0x1
    bne      .L_8013F394
    lis      r3, __float_nan@ha
    lfs      f0, __float_nan@l(r3)
.L_8013F394:
    lfs      f2, 0x2b4(r1)
    lfs      f1, 0x294(r1)
    lfs      f3, 0x2ac(r1)
    fsubs    f4, f2, f1
    lfs      f2, 0x28c(r1)
    lfs      f1, lbl_8065E994@sda21(r0)
    fsubs    f3, f3, f2
    fmuls    f2, f4, f4
    fmadds   f5, f3, f3, f2
    fcmpo    cr0, f5, f1
    ble      .L_8013F408
    frsqrte  f2, f5
    lfd      f4, lbl_8065E9B0@sda21(r0)
    lfd      f3, lbl_8065E9B8@sda21(r0)
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f5, f1, f3
    fmul     f2, f2, f1
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f5, f1, f3
    fmul     f2, f2, f1
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f5, f1, f3
    fmul     f1, f2, f1
    fmul     f5, f5, f1
    frsp     f5, f5
    b        .L_8013F48C
.L_8013F408:
    lfd      f1, lbl_8065E9C0@sda21(r0)
    fcmpo    cr0, f5, f1
    bge      .L_8013F420
    lis      r3, __float_nan@ha
    lfs      f5, __float_nan@l(r3)
    b        .L_8013F48C
.L_8013F420:
    stfs     f5, 0x10(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x10(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8013F448
    bge      .L_8013F478
    cmpwi    r3, 0x0
    beq      .L_8013F460
    b        .L_8013F478
.L_8013F448:
    clrlwi.  r0, r4, 9
    beq      .L_8013F458
    li       r0, 0x1
    b        .L_8013F47C
.L_8013F458:
    li       r0, 0x2
    b        .L_8013F47C
.L_8013F460:
    clrlwi.  r0, r4, 9
    beq      .L_8013F470
    li       r0, 0x5
    b        .L_8013F47C
.L_8013F470:
    li       r0, 0x3
    b        .L_8013F47C
.L_8013F478:
    li       r0, 0x4
.L_8013F47C:
    cmpwi    r0, 0x1
    bne      .L_8013F48C
    lis      r3, __float_nan@ha
    lfs      f5, __float_nan@l(r3)
.L_8013F48C:
    fcmpo    cr0, f5, f0
    cror     eq, lt, eq
    bne      .L_8013F578
    mr       r3, r25
    bl       ResetMapCameraInterpolation
    lfs      f3, 0x28c(r1)
    lfs      f1, 0x29c(r1)
    lfs      f2, 0x294(r1)
    lfs      f0, 0x2a4(r1)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f1, f1
    mr       r3, r25
    bl       TrySetUnobstructedMapCameraYaw
    b        .L_8013F578
.L_8013F4CC:
    cmpw     r20, r18
    beq      .L_8013F51C
    subf.    r0, r18, r20
    subf     r0, r18, r20
    bge      .L_8013F4E4
    neg      r0, r0
.L_8013F4E4:
    srwi     r3, r0, 31
    clrlwi   r0, r0, 31
    xor      r0, r0, r3
    subf.    r0, r3, r0
    beq      .L_8013F51C
    lfs      f2, 0x200(r1)
    lfs      f1, 0x204(r1)
    lfs      f0, 0x208(r1)
    stfs     f2, 0x28c(r1)
    stfs     f1, 0x290(r1)
    stfs     f0, 0x294(r1)
    lfs      f0, 0x4(r26)
    stfs     f0, 0x290(r1)
    b        .L_8013F578
.L_8013F51C:
    addi     r0, r20, 0x1
    addi     r3, r1, 0x2ac
    mulli    r4, r20, 0xc
    addi     r6, r1, 0x1c4
    mulli    r0, r0, 0xc
    add      r18, r21, r4
    add      r17, r21, r0
    mr       r4, r18
    mr       r5, r17
    bl       GetOutwardPerpendicularToEdge
    mr       r4, r18
    mr       r5, r17
    addi     r3, r1, 0x1e0
    addi     r6, r1, 0x1b8
    bl       GetOutwardPerpendicularToEdge
    lfs      f2, 0x1e0(r1)
    lfs      f1, 0x1e4(r1)
    lfs      f0, 0x1e8(r1)
    stfs     f2, 0x28c(r1)
    stfs     f1, 0x290(r1)
    stfs     f0, 0x294(r1)
    lfs      f0, 0x4(r26)
    stfs     f0, 0x290(r1)
.L_8013F578:
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x1dc(r1)
    stw      r0, 0x1ec(r1)
    b        .L_8013F818
.L_8013F58C:
    cmpwi    r18, 0x0
    blt      .L_8013F818
    lwz      r0, 0x0(r27)
    cmpwi    r0, 0x1
    bne      .L_8013F6E0
    addi     r0, r18, 0x1
    lis      r5, __vt__4CVec@ha
    mulli    r4, r18, 0xc
    addi     r3, r1, 0x29c
    addi     r5, r5, __vt__4CVec@l
    stw      r5, 0x1b4(r1)
    addi     r6, r1, 0x1f0
    mulli    r0, r0, 0xc
    add      r17, r21, r4
    add      r18, r21, r0
    mr       r4, r17
    mr       r5, r18
    bl       GetOutwardPerpendicularToEdge
    lwz      r7, 0x1f0(r1)
    addi     r3, r1, 0xd0
    lwz      r6, 0x1f4(r1)
    addi     r4, r1, 0x29c
    lwz      r0, 0x1f8(r1)
    addi     r5, r1, 0xe0
    stw      r7, 0xe0(r1)
    stw      r6, 0xe4(r1)
    stw      r0, 0xe8(r1)
    bl       fn_8000F93C
    mr       r5, r17
    mr       r6, r18
    addi     r3, r1, 0x29c
    addi     r4, r1, 0xd0
    addi     r7, r1, 0x1a8
    li       r8, 0x1
    bl       IntersectLinesXZ
    lis      r4, __vt__4CVec@ha
    cmpwi    r3, 0x0
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0xdc(r1)
    beq      .L_8013F6AC
    lfs      f0, lbl_8065E994@sda21(r0)
    addi     r3, r1, 0x1f0
    mr       r4, r3
    stfs     f0, 0x1f4(r1)
    bl       PSVECNormalize
    lfs      f1, lbl_8065EA04@sda21(r0)
    addi     r3, r1, 0xb4
    addi     r4, r1, 0x1f0
    bl       fn_8000F9C0
    lwz      r7, 0xb4(r1)
    addi     r3, r1, 0xa4
    lwz      r6, 0xb8(r1)
    addi     r4, r1, 0x1a8
    lwz      r0, 0xbc(r1)
    addi     r5, r1, 0xc4
    stw      r7, 0xc4(r1)
    stw      r6, 0xc8(r1)
    stw      r0, 0xcc(r1)
    bl       fn_800244A0
    lfs      f2, 0xa4(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0xa8(r1)
    lfs      f0, 0xac(r1)
    stfs     f2, 0x28c(r1)
    stfs     f1, 0x290(r1)
    stfs     f0, 0x294(r1)
    stw      r0, 0xb0(r1)
    stw      r0, 0xc0(r1)
    lfs      f0, 0x4(r26)
    stfs     f0, 0x290(r1)
    b        .L_8013F6C4
.L_8013F6AC:
    lfs      f2, 0x29c(r1)
    lfs      f1, 0x2a0(r1)
    lfs      f0, 0x2a4(r1)
    stfs     f2, 0x28c(r1)
    stfs     f1, 0x290(r1)
    stfs     f0, 0x294(r1)
.L_8013F6C4:
    mr       r3, r25
    bl       ResetMapCameraInterpolation
    lis      r3, __vt__4CVec@ha
    li       r29, 0x1
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x1b4(r1)
    b        .L_8013F818
.L_8013F6E0:
    addi     r0, r18, 0x1
    lis      r5, __vt__4CVec@ha
    mulli    r4, r18, 0xc
    addi     r3, r1, 0x28c
    addi     r5, r5, __vt__4CVec@l
    stw      r5, 0x1a4(r1)
    addi     r6, r1, 0x198
    mulli    r0, r0, 0xc
    add      r18, r21, r4
    stw      r5, 0x194(r1)
    add      r17, r21, r0
    mr       r4, r18
    mr       r5, r17
    bl       GetOutwardPerpendicularToEdge
    lwz      r7, 0x198(r1)
    addi     r3, r1, 0x88
    lwz      r6, 0x19c(r1)
    addi     r4, r1, 0x28c
    lwz      r0, 0x1a0(r1)
    addi     r5, r1, 0x98
    stw      r7, 0x98(r1)
    stw      r6, 0x9c(r1)
    stw      r0, 0xa0(r1)
    bl       fn_8000F93C
    mr       r5, r18
    mr       r6, r17
    addi     r3, r1, 0x28c
    addi     r4, r1, 0x88
    addi     r7, r1, 0x188
    li       r8, 0x1
    bl       IntersectLinesXZ
    lis      r4, __vt__4CVec@ha
    cmpwi    r3, 0x0
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x94(r1)
    beq      .L_8013F7F0
    lfs      f0, lbl_8065E994@sda21(r0)
    addi     r3, r1, 0x198
    mr       r4, r3
    stfs     f0, 0x19c(r1)
    bl       PSVECNormalize
    lfs      f1, lbl_8065EA04@sda21(r0)
    addi     r3, r1, 0x6c
    addi     r4, r1, 0x198
    bl       fn_8000F9C0
    lwz      r7, 0x6c(r1)
    addi     r3, r1, 0x5c
    lwz      r6, 0x70(r1)
    addi     r4, r1, 0x188
    lwz      r0, 0x74(r1)
    addi     r5, r1, 0x7c
    stw      r7, 0x7c(r1)
    stw      r6, 0x80(r1)
    stw      r0, 0x84(r1)
    bl       fn_800244A0
    lfs      f2, 0x5c(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0x60(r1)
    lfs      f0, 0x64(r1)
    stfs     f2, 0x28c(r1)
    stfs     f1, 0x290(r1)
    stfs     f0, 0x294(r1)
    stw      r0, 0x68(r1)
    stw      r0, 0x78(r1)
    lfs      f0, 0x4(r26)
    stfs     f0, 0x290(r1)
    b        .L_8013F808
.L_8013F7F0:
    lfs      f2, 0x29c(r1)
    lfs      f1, 0x2a0(r1)
    lfs      f0, 0x2a4(r1)
    stfs     f2, 0x28c(r1)
    stfs     f1, 0x290(r1)
    stfs     f0, 0x294(r1)
.L_8013F808:
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x194(r1)
    stw      r0, 0x1a4(r1)
.L_8013F818:
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x1fc(r1)
    stw      r0, 0x20c(r1)
    stw      r0, 0x21c(r1)
.L_8013F82C:
    addi     r21, r21, 0x3c
    addi     r16, r16, 0x1
    addi     r15, r15, 0x34
.L_8013F838:
    cmpw     r16, r30
    blt      .L_8013EF58
.L_8013F840:
    cmpwi    r28, 0x0
    bne      .L_8013F9E4
    lis      r3, __vt__11CamAreaInfo@ha
    mr       r15, r14
    li       r16, 0x0
    li       r17, 0x0
    addi     r14, r3, __vt__11CamAreaInfo@l
    b        .L_8013F9A4
.L_8013F860:
    addic.   r0, r1, 0x2c
    stw      r14, 0x2c(r1)
    stw      r15, 0x30(r1)
    beq      .L_8013F87C
    lis      r3, __vt__11CamAreaInfo@ha
    addi     r0, r3, __vt__11CamAreaInfo@l
    stw      r0, 0x2c(r1)
.L_8013F87C:
    lwz      r4, 0x30(r1)
    lhz      r0, 0x2e(r4)
    rlwinm.  r0, r0, 0, 24, 24
    bne      .L_8013F998
    lwz      r0, 0x28(r4)
    lis      r3, 0x4330
    lwz      r4, 0x24(r4)
    xoris    r0, r0, 0x8000
    stw      r3, 0x2f0(r1)
    xoris    r4, r4, 0x8000
    lfd      f3, lbl_8065E9E8@sda21(r0)
    stw      r0, 0x2f4(r1)
    lfs      f1, lbl_8065E9DC@sda21(r0)
    lfd      f0, 0x2f0(r1)
    stw      r4, 0x2fc(r1)
    fsubs    f0, f0, f3
    lfs      f4, 0x2b0(r1)
    stw      r3, 0x2f8(r1)
    lfd      f2, 0x2f8(r1)
    fdivs    f5, f0, f1
    fsubs    f0, f2, f3
    fcmpo    cr0, f4, f5
    fdivs    f1, f0, f1
    cror     eq, gt, eq
    bne      .L_8013F8F4
    fcmpo    cr0, f4, f1
    cror     eq, lt, eq
    bne      .L_8013F8F4
    li       r0, 0x1
    b        .L_8013F918
.L_8013F8F4:
    fcmpo    cr0, f4, f5
    bge      .L_8013F904
    li       r0, -0x2
    b        .L_8013F918
.L_8013F904:
    fcmpo    cr0, f4, f1
    ble      .L_8013F914
    li       r0, -0x1
    b        .L_8013F918
.L_8013F914:
    li       r0, 0x0
.L_8013F918:
    srwi.    r0, r0, 31
    beq      .L_8013F96C
    lfs      f0, 0x4(r26)
    fcmpo    cr0, f0, f5
    cror     eq, gt, eq
    bne      .L_8013F944
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_8013F944
    li       r0, 0x1
    b        .L_8013F968
.L_8013F944:
    fcmpo    cr0, f0, f5
    bge      .L_8013F954
    li       r0, -0x2
    b        .L_8013F968
.L_8013F954:
    fcmpo    cr0, f0, f1
    ble      .L_8013F964
    li       r0, -0x1
    b        .L_8013F968
.L_8013F964:
    li       r0, 0x0
.L_8013F968:
    srwi     r0, r0, 31
.L_8013F96C:
    clrlwi.  r0, r0, 24
    bne      .L_8013F998
    mr       r4, r31
    addi     r3, r1, 0x28c
    addi     r5, r31, 0xc
    addi     r6, r31, 0x18
    addi     r7, r31, 0x24
    bl       IsPointInsideQuadXZ
    cmpwi    r3, 0x0
    beq      .L_8013F998
    addi     r16, r16, 0x1
.L_8013F998:
    addi     r31, r31, 0x3c
    addi     r17, r17, 0x1
    addi     r15, r15, 0x34
.L_8013F9A4:
    cmpw     r17, r30
    blt      .L_8013F860
    cmpwi    r16, 0x0
    ble      .L_8013F9E4
    lwz      r0, 0x0(r27)
    lfs      f2, 0x29c(r1)
    lfs      f1, 0x2a0(r1)
    cmpwi    r0, 0x1
    lfs      f0, 0x2a4(r1)
    stfs     f2, 0x28c(r1)
    stfs     f1, 0x290(r1)
    stfs     f0, 0x294(r1)
    bne      .L_8013F9E4
    mr       r3, r25
    bl       ResetMapCameraInterpolation
    li       r29, 0x1
.L_8013F9E4:
    lfs      f0, 0x28c(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    mr       r3, r29
    stfs     f0, 0x0(r26)
    lfs      f0, 0x290(r1)
    stfs     f0, 0x4(r26)
    lfs      f0, 0x294(r1)
    stfs     f0, 0x8(r26)
    stw      r0, 0x298(r1)
    stw      r0, 0x2a8(r1)
    stw      r0, 0x2b8(r1)
.L_8013FA14:
    psq_l    f31, 0x368(r1), 0, qr0
    lfd      f31, 0x360(r1)
    psq_l    f30, 0x358(r1), 0, qr0
    lfd      f30, 0x350(r1)
    lmw      r14, 0x308(r1)
    lwz      r0, 0x374(r1)
    mtlr     r0
    addi     r1, r1, 0x370
    blr
*/

/**
 * @note Address: 0x8013FA38
 * @note Size: 0x1574
 */
// Update__16CCameraActionMapFUl
/*
    stwu     r1, -0x320(r1)
    mflr     r0
    stw      r0, 0x324(r1)
    stfd     f31, 0x310(r1)
    psq_st   f31, 0x318(r1), 0, qr0
    stfd     f30, 0x300(r1)
    psq_st   f30, 0x308(r1), 0, qr0
    stfd     f29, 0x2f0(r1)
    psq_st   f29, 0x2f8(r1), 0, qr0
    stfd     f28, 0x2e0(r1)
    psq_st   f28, 0x2e8(r1), 0, qr0
    stfd     f27, 0x2d0(r1)
    psq_st   f27, 0x2d8(r1), 0, qr0
    stw      r31, 0x2cc(r1)
    stw      r30, 0x2c8(r1)
    stw      r29, 0x2c4(r1)
    stw      r28, 0x2c0(r1)
    mr       r30, r4
    mr       r31, r3
    cmplwi   r30, 0x2
    ble      .L_8013FA90
    li       r30, 0x2
.L_8013FA90:
    lwz      r5, 0x44(r31)
    cmplwi   r5, 0x0
    beq      .L_80140F64
    lwz      r8, 0xc8(r5)
    lis      r3, gCameraAreaState@ha
    lwz      r4, 0xcc(r5)
    li       r0, 0x0
    lwz      r7, 0xd0(r5)
    li       r6, -0x2
    stw      r4, 0x2ac(r1)
    addi     r3, r3, gCameraAreaState@l
    lfs      f0, lbl_8065EA44@sda21(r0)
    addi     r4, r1, 0x164
    stw      r0, 0xea4(r31)
    addi     r5, r31, 0xea8
    lfs      f1, 0x2ac(r1)
    stw      r6, 0xea8(r31)
    fadds    f0, f1, f0
    stw      r6, 0xeac(r31)
    stw      r6, 0xeb0(r31)
    stw      r6, 0xeb4(r31)
    stw      r6, 0xeb8(r31)
    stw      r6, 0xebc(r31)
    stw      r6, 0xec0(r31)
    stw      r6, 0xec4(r31)
    stw      r6, 0xec8(r31)
    stw      r6, 0xecc(r31)
    stw      r6, 0xed0(r31)
    stw      r6, 0xed4(r31)
    stw      r6, 0xed8(r31)
    stw      r6, 0xedc(r31)
    stw      r6, 0xee0(r31)
    stw      r6, 0xee4(r31)
    stw      r6, 0xee8(r31)
    stw      r6, 0xeec(r31)
    stw      r6, 0xef0(r31)
    stw      r6, 0xef4(r31)
    stw      r6, 0xef8(r31)
    stw      r6, 0xefc(r31)
    stw      r6, 0xf00(r31)
    stw      r6, 0xf04(r31)
    stw      r6, 0xf08(r31)
    stw      r6, 0xf0c(r31)
    stw      r6, 0xf10(r31)
    stw      r6, 0xf14(r31)
    stw      r6, 0xf18(r31)
    stw      r6, 0xf1c(r31)
    stfs     f0, 0x2ac(r1)
    stw      r6, 0xf20(r31)
    lwz      r0, 0x2ac(r1)
    stw      r6, 0xf24(r31)
    stw      r8, 0x2a8(r1)
    stw      r7, 0x2b0(r1)
    stw      r8, 0x164(r1)
    stw      r0, 0x168(r1)
    stw      r7, 0x16c(r1)
    bl       CollectCameraAreasContainingPoint
    lis      r0, 0x4330
    stw      r3, 0xea4(r31)
    lfd      f1, lbl_8065EA58@sda21(r0)
    addi     r3, r31, 0x60
    stw      r30, 0x2bc(r1)
    addi     r4, r31, 0x70
    lfs      f2, lbl_8065EA68@sda21(r0)
    stw      r0, 0x2b8(r1)
    lfd      f0, 0x2b8(r1)
    fsubs    f0, f0, f1
    fmuls    f31, f2, f0
    fmr      f29, f31
    bl       CopyVecToCVec
    mr       r3, r31
    addi     r4, r31, 0x70
    bl       GetMapCameraTargetOffsetPosition
    mr       r3, r31
    mr       r4, r30
    bl       UpdateMapCameraPadControls
    lwz      r3, 0x11d8(r31)
    rlwinm.  r0, r3, 0, 14, 14
    beq      .L_8013FC38
    rlwinm.  r0, r3, 0, 26, 30
    bne      .L_8013FC38
    lwz      r3, 0x60(r31)
    li       r0, 0x0
    lwz      r4, 0x64(r31)
    stw      r3, 0x158(r1)
    lwz      r3, 0x68(r31)
    lfs      f1, 0x70(r31)
    lfs      f0, 0x158(r1)
    stw      r4, 0x15c(r1)
    fcmpu    cr0, f1, f0
    stw      r3, 0x160(r1)
    bne      .L_8013FC20
    lfs      f1, 0x74(r31)
    lfs      f0, 0x15c(r1)
    fcmpu    cr0, f1, f0
    bne      .L_8013FC20
    lfs      f1, 0x78(r31)
    lfs      f0, 0x160(r1)
    fcmpu    cr0, f1, f0
    beq      .L_8013FC24
.L_8013FC20:
    li       r0, 0x1
.L_8013FC24:
    clrlwi.  r0, r0, 24
    beq      .L_8013FC38
    mr       r3, r31
    lis      r4, 0x1
    bl       SetMapCameraModeFlags
.L_8013FC38:
    li       r29, 0x0
.L_8013FC3C:
    lwz      r0, 0xe58(r31)
    cmpwi    r0, 0x0
    bne      .L_8013FCB8
    lwz      r3, 0xe30(r31)
    cmplwi   r3, 0x0
    beq      .L_8013FC6C
    lwz      r12, 0x0(r3)
    mr       r4, r30
    lfs      f1, lbl_8065E9AC@sda21(r0)
    lwz      r12, 0x14(r12)
    mtctr    r12
    bctrl
.L_8013FC6C:
    lwz      r3, 0xe30(r31)
    lwz      r12, 0x0(r3)
    lwz      r12, 0x18(r12)
    mtctr    r12
    bctrl
    fmr      f30, f1
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    li       r4, 0x0
    bl       GetPlayerPoolObject
    mr       r4, r3
    addi     r3, r1, 0x14c
    lwz      r12, 0x34(r4)
    lwz      r12, 0xec(r12)
    mtctr    r12
    bctrl
    lfs      f0, 0x150(r1)
    fadds    f0, f30, f0
    stfs     f0, 0xe60(r31)
.L_8013FCB8:
    lwz      r0, 0xe58(r31)
    cmpwi    r0, 0x1
    bne      .L_8013FD00
    lwz      r3, 0xe50(r31)
    cmplwi   r3, 0x0
    beq      .L_8013FCE8
    lwz      r12, 0x0(r3)
    mr       r4, r30
    lfs      f1, lbl_8065E9AC@sda21(r0)
    lwz      r12, 0x14(r12)
    mtctr    r12
    bctrl
.L_8013FCE8:
    lwz      r3, 0xe50(r31)
    lwz      r12, 0x0(r3)
    lwz      r12, 0x18(r12)
    mtctr    r12
    bctrl
    stfs     f1, 0xe60(r31)
.L_8013FD00:
    addi     r29, r29, 0x1
    cmpwi    r29, 0x2
    blt      .L_8013FC3C
    lwz      r3, 0xe64(r31)
    cmplwi   r3, 0x0
    beq      .L_8013FD30
    lwz      r12, 0x0(r3)
    mr       r4, r30
    lfs      f1, lbl_8065E9AC@sda21(r0)
    lwz      r12, 0x14(r12)
    mtctr    r12
    bctrl
.L_8013FD30:
    lwz      r3, 0xe78(r31)
    cmplwi   r3, 0x0
    beq      .L_8013FD78
    lwz      r12, 0x0(r3)
    mr       r4, r30
    lfs      f1, lbl_8065E9AC@sda21(r0)
    lwz      r12, 0x14(r12)
    mtctr    r12
    bctrl
    clrlwi.  r0, r3, 24
    bne      .L_8013FD80
    lwz      r3, 0xe78(r31)
    lwz      r12, 0x0(r3)
    lwz      r12, 0x18(r12)
    mtctr    r12
    bctrl
    stfs     f1, 0xe98(r31)
    b        .L_8013FD80
.L_8013FD78:
    li       r0, 0x0
    stw      r0, 0xe78(r31)
.L_8013FD80:
    lwz      r0, 0x11d8(r31)
    rlwinm.  r0, r0, 0, 15, 15
    beq      .L_801409A0
    lwz      r6, 0x0(r31)
    lis      r3, __vt__4CVec@ha
    lwz      r5, 0x4(r31)
    addi     r0, r3, __vt__4CVec@l
    addi     r3, r1, 0x298
    addi     r4, r31, 0x60
    stw      r6, 0x298(r1)
    stw      r5, 0x29c(r1)
    lwz      r5, 0x8(r31)
    stw      r5, 0x2a0(r1)
    stw      r0, 0x2a4(r1)
    bl       PSVECDistance
    fmr      f30, f1
    addi     r3, r1, 0x298
    addi     r4, r31, 0x70
    bl       PSVECDistance
    lfs      f2, 0x58(r31)
    lfs      f0, 0x80(r31)
    fcmpu    cr0, f2, f0
    beq      .L_8013FE0C
    fcmpo    cr0, f1, f30
    ble      .L_8013FE0C
    fcmpo    cr0, f1, f0
    bge      .L_8013FDF8
    fcmpu    cr0, f2, f0
    beq      .L_8013FDF8
    fmr      f2, f1
.L_8013FDF8:
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_8013FE18
    fmr      f2, f0
    b        .L_8013FE18
.L_8013FE0C:
    fcmpo    cr0, f1, f30
    bge      .L_8013FE18
    fmr      f2, f0
.L_8013FE18:
    lis      r3, __vt__4CVec@ha
    stfs     f2, 0x58(r31)
    addi     r0, r3, __vt__4CVec@l
    mr       r4, r31
    stw      r0, 0x2a4(r1)
    addi     r3, r1, 0x288
    stw      r0, 0x294(r1)
    bl       CopyVecToCVec
    lis      r3, sMapCameraSpringParameters@ha
    fmr      f1, f29
    lfs      f2, sMapCameraSpringParameters@l(r3)
    addi     r3, r1, 0x13c
    lfs      f3, 0x5c(r31)
    addi     r4, r1, 0x288
    lfs      f4, 0x58(r31)
    addi     r5, r31, 0x70
    addi     r6, r31, 0x60
    bl       SpringDamp
    addi     r3, r1, 0x288
    addi     r4, r1, 0x13c
    bl       CopyVecToCVec
    lis      r4, __vt__4CVec@ha
    mr       r3, r31
    addi     r0, r4, __vt__4CVec@l
    addi     r4, r1, 0x288
    stw      r0, 0x148(r1)
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x38(r12)
    mtctr    r12
    bctrl
    mr       r4, r31
    addi     r3, r1, 0x130
    bl       GetMapCameraTargetWithHeightOffset
    mr       r3, r31
    addi     r4, r1, 0x130
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x3c(r12)
    mtctr    r12
    bctrl
    mr       r3, r31
    lfs      f1, 0x0(r31)
    lwz      r12, 0x3c(r31)
    lfs      f2, 0xdc(r31)
    lwz      r12, 0x2c(r12)
    lfs      f3, 0x8(r31)
    mtctr    r12
    bctrl
    mr       r3, r31
    bl       SetMapCameraAnglesFromEyeAndInterest
    lfs      f1, 0x20(r31)
    bl       ClassifyFloatBits
    cmpwi    r3, 0x1
    beq      .L_8013FF0C
    lfs      f1, 0x24(r31)
    bl       ClassifyFloatBits
    cmpwi    r3, 0x1
    beq      .L_8013FF0C
    lfs      f1, 0x28(r31)
    bl       ClassifyFloatBits
    cmpwi    r3, 0x1
    bne      .L_8013FFF0
.L_8013FF0C:
    lfs      f0, 0xf6c(r31)
    lis      r3, sMapCameraStateFormat@ha
    addi     r3, r3, sMapCameraStateFormat@l
    stfd     f0, 0x8(r1)
    lfs      f0, 0xf70(r31)
    stfd     f0, 0x10(r1)
    lfs      f0, 0xf78(r31)
    stfd     f0, 0x18(r1)
    lfs      f0, 0xf7c(r31)
    stfd     f0, 0x20(r1)
    lfs      f0, 0xf80(r31)
    stfd     f0, 0x28(r1)
    lfs      f0, 0xf88(r31)
    stfd     f0, 0x30(r1)
    lfs      f0, 0xf8c(r31)
    stfd     f0, 0x38(r1)
    lfs      f0, 0xf90(r31)
    stfd     f0, 0x40(r1)
    lfs      f0, 0xf98(r31)
    stfd     f0, 0x48(r1)
    lfs      f0, 0xf9c(r31)
    stfd     f0, 0x50(r1)
    lfs      f0, 0xfa0(r31)
    stfd     f0, 0x58(r1)
    lfs      f0, 0xfb8(r31)
    stfd     f0, 0x60(r1)
    lfs      f0, 0xfbc(r31)
    stfd     f0, 0x68(r1)
    lfs      f0, 0xfc0(r31)
    stfd     f0, 0x70(r1)
    lwz      r0, lbl_8065B338@sda21(r0)
    stw      r0, 0x78(r1)
    lfs      f1, 0x0(r31)
    lfs      f2, 0x4(r31)
    lfs      f3, 0x8(r31)
    lfs      f4, 0x20(r31)
    lfs      f5, 0x24(r31)
    lfs      f6, 0x28(r31)
    lfs      f7, 0x30(r31)
    lwz      r4, 0x11d8(r31)
    lwz      r5, 0x11e0(r31)
    lwz      r6, 0xea0(r31)
    lwz      r7, lbl_8065B340@sda21(r0)
    lwz      r8, lbl_8065B33C@sda21(r0)
    lfs      f8, 0xf68(r31)
    lwz      r9, 0xf28(r31)
    lwz      r10, 0xdb4(r31)
    crset    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sPathCcameraactionmap2Cpp@ha
    mr       r6, r3
    addi     r3, r4, sPathCcameraactionmap2Cpp@l
    li       r5, lbl_8065E9C8@sda21
    li       r4, 0x245
    li       r7, sMapCameraUpdateLabel@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8013FFF0:
    addi     r3, r31, 0x70
    addi     r4, r31, 0x60
    addi     r5, r1, 0x27c
    bl       PSVECSubtract
    lfs      f1, lbl_8065E994@sda21(r0)
    lfs      f0, 0x27c(r1)
    stfs     f1, 0x280(r1)
    fcmpu    cr0, f0, f1
    bne      .L_80140020
    lfs      f0, 0x284(r1)
    fcmpu    cr0, f0, f1
    beq      .L_801400CC
.L_80140020:
    addi     r3, r1, 0x27c
    mr       r4, r3
    bl       PSVECNormalize
    lfs      f1, 0x24(r31)
    bl       sin
    frsp     f1, f1
    lfs      f0, lbl_8065E994@sda21(r0)
    stfs     f0, 0x274(r1)
    stfs     f1, 0x270(r1)
    lfs      f1, 0x24(r31)
    bl       cos
    frsp     f0, f1
    addi     r3, r1, 0x27c
    addi     r4, r1, 0x270
    stfs     f0, 0x278(r1)
    bl       PSVECDotProduct
    lwz      r4, 0x0(r31)
    lis      r3, __vt__4CVec@ha
    lwz      r5, 0x4(r31)
    fmr      f30, f1
    addi     r0, r3, __vt__4CVec@l
    addi     r3, r1, 0x260
    stw      r4, 0x260(r1)
    addi     r4, r31, 0x70
    stw      r5, 0x264(r1)
    lwz      r5, 0x8(r31)
    stw      r5, 0x268(r1)
    stw      r0, 0x26c(r1)
    bl       GetHorizontalDistance
    lfs      f0, lbl_8065EA6C@sda21(r0)
    fcmpo    cr0, f30, f0
    bge      .L_801400B8
    lfs      f0, lbl_8065E9D0@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801400B8
    li       r0, 0x1
    stw      r0, lbl_8065B344@sda21(r0)
    b        .L_801400C0
.L_801400B8:
    li       r0, 0x0
    stw      r0, lbl_8065B344@sda21(r0)
.L_801400C0:
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x26c(r1)
.L_801400CC:
    lwz      r0, lbl_8065B340@sda21(r0)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    li       r4, 0x0
    stw      r0, lbl_8065B33C@sda21(r0)
    bl       GetPlayerPoolObject
    lwz      r0, 0xac4(r3)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    li       r4, 0x0
    stw      r0, lbl_8065B340@sda21(r0)
    bl       GetPlayerPoolObject
    lwz      r0, 0xac4(r3)
    cmpwi    r0, 0x3
    bne      .L_801402FC
    lwz      r3, lbl_8065B340@sda21(r0)
    lwz      r0, lbl_8065B33C@sda21(r0)
    cmpw     r3, r0
    beq      .L_801402FC
    lwz      r6, 0x44(r31)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, lbl_8065E998@sda21(r0)
    lwz      r5, 0xc8(r6)
    addi     r3, r1, 0x244
    lwz      r4, 0xcc(r6)
    stw      r5, 0x254(r1)
    stw      r4, 0x258(r1)
    lwz      r4, 0xd0(r6)
    stw      r4, 0x25c(r1)
    lfs      f0, 0x10c(r6)
    stw      r0, 0x250(r1)
    fadds    f2, f1, f0
    lfs      f1, 0x20(r31)
    lfs      f3, 0x28(r31)
    bl       SetVectorComponents
    lwz      r4, 0x44(r31)
    addi     r3, r1, 0x124
    lfs      f1, lbl_8065E98C@sda21(r0)
    lwz      r12, 0x34(r4)
    lfs      f0, 0x248(r1)
    lwz      r12, 0xec(r12)
    fadds    f28, f1, f0
    lfs      f27, 0x20(r31)
    mtctr    r12
    bctrl
    addi     r3, r1, 0x228
    addi     r4, r1, 0x124
    bl       __ct__4CVecFRC3Vec
    lwz      r6, 0x0(r31)
    lis      r3, __vt__4CVec@ha
    lwz      r5, 0x4(r31)
    addi     r0, r3, __vt__4CVec@l
    addi     r3, r1, 0x228
    addi     r4, r1, 0x218
    stw      r6, 0x218(r1)
    stw      r5, 0x21c(r1)
    lwz      r5, 0x8(r31)
    stw      r5, 0x220(r1)
    stw      r0, 0x224(r1)
    bl       PSVECDistance
    lis      r3, __vt__4CVec@ha
    fmr      f30, f1
    addi     r0, r3, __vt__4CVec@l
    fmr      f1, f27
    stw      r0, 0x224(r1)
    stw      r0, 0x234(r1)
    bl       cos
    frsp     f29, f1
    fmr      f1, f28
    bl       cos
    frsp     f2, f1
    lfs      f0, 0x254(r1)
    fmr      f1, f27
    fmuls    f2, f30, f2
    fmadds   f0, f2, f29, f0
    stfs     f0, 0x238(r1)
    bl       cos
    frsp     f29, f1
    fmr      f1, f28
    bl       sin
    frsp     f2, f1
    lfs      f0, 0x258(r1)
    lfs      f1, 0x25c(r1)
    addi     r3, r1, 0x114
    stfs     f0, 0x23c(r1)
    addi     r4, r1, 0x238
    fneg     f0, f2
    fmuls    f0, f30, f0
    fmadds   f0, f0, f29, f1
    stfs     f0, 0x240(r1)
    bl       __ct__4CVecFRC3Vec
    lis      r3, sMapCameraTestEye@ha
    addi     r4, r1, 0x114
    addi     r3, r3, sMapCameraTestEye@l
    bl       CopyVecToCVec
    lis      r4, __vt__4CVec@ha
    addi     r3, r1, 0x104
    addi     r0, r4, __vt__4CVec@l
    addi     r4, r1, 0x254
    stw      r0, 0x120(r1)
    bl       __ct__4CVecFRC3Vec
    lis      r3, sMapCameraTestCenter@ha
    addi     r4, r1, 0x104
    addi     r3, r3, sMapCameraTestCenter@l
    bl       CopyVecToCVec
    lis      r4, __vt__4CVec@ha
    mr       r3, r31
    addi     r0, r4, __vt__4CVec@l
    addi     r4, r1, 0x238
    stw      r0, 0x110(r1)
    addi     r5, r1, 0x254
    addi     r6, r31, 0x570
    li       r7, 0x0
    bl       CalcIntersectVec__16CCameraActionMapFRC3VecRC3VecP11CamMapEntryl
    stw      r3, 0x970(r31)
    lwz      r0, 0x970(r31)
    cmpwi    r0, 0x0
    bne      .L_801402F0
    lwz      r4, 0x44(r31)
    mr       r3, r31
    lfs      f1, lbl_8065E998@sda21(r0)
    lfs      f0, 0x10c(r4)
    lfs      f2, lbl_8065E9D0@sda21(r0)
    fadds    f1, f1, f0
    lfs      f3, lbl_8065E994@sda21(r0)
    bl       TrySetMapCameraTargetYaw
    cmpwi    r3, 0x0
    beq      .L_801402F0
    lis      r4, 0x2
    mr       r3, r31
    addi     r4, r4, 0x2
    bl       SetMapCameraModeFlags
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x250(r1)
    b        .L_80140918
.L_801402F0:
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x250(r1)
.L_801402FC:
    lwz      r0, lbl_8065B344@sda21(r0)
    cmpwi    r0, 0x0
    beq      .L_80140594
    mr       r4, r31
    addi     r3, r1, 0xf8
    bl       GetMapCameraTargetWithHeightOffset
    mr       r3, r31
    addi     r4, r1, 0xf8
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x3c(r12)
    mtctr    r12
    bctrl
    lis      r4, __vt__4CVec@ha
    addi     r3, r1, 0xe8
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x214(r1)
    lwz      r4, 0x44(r31)
    lfs      f1, 0x20(r31)
    lfs      f2, 0xfc(r4)
    lfs      f3, 0x28(r31)
    bl       __ct__4CVecFfff
    addi     r3, r1, 0x208
    addi     r4, r1, 0xe8
    bl       CopyVecToCVec
    lis      r4, __vt__4CVec@ha
    mr       r3, r31
    addi     r0, r4, __vt__4CVec@l
    addi     r4, r1, 0x208
    stw      r0, 0xf4(r1)
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x40(r12)
    mtctr    r12
    bctrl
    lwz      r6, 0x0(r31)
    lis      r3, __vt__4CVec@ha
    lwz      r5, 0x4(r31)
    addi     r0, r3, __vt__4CVec@l
    addi     r3, r1, 0x1f8
    addi     r4, r31, 0x70
    stw      r6, 0x1f8(r1)
    stw      r5, 0x1fc(r1)
    lwz      r5, 0x8(r31)
    stw      r5, 0x200(r1)
    stw      r0, 0x204(r1)
    bl       PSVECDistance
    lfs      f0, lbl_8065E9A4@sda21(r0)
    fmr      f27, f1
    fcmpo    cr0, f1, f0
    bge      .L_801403C4
    fmr      f27, f0
.L_801403C4:
    lfs      f30, 0x20(r31)
    lfs      f2, lbl_8065E98C@sda21(r0)
    lfs      f0, 0x24(r31)
    fmr      f1, f30
    fadds    f28, f2, f0
    bl       cos
    frsp     f29, f1
    fmr      f1, f28
    bl       cos
    frsp     f2, f1
    lfs      f0, 0x70(r31)
    fmr      f1, f30
    fmuls    f2, f27, f2
    fmadds   f0, f2, f29, f0
    stfs     f0, 0x1ec(r1)
    bl       cos
    frsp     f29, f1
    fmr      f1, f28
    bl       sin
    frsp     f1, f1
    lfs      f0, 0x78(r31)
    mr       r3, r31
    addi     r4, r1, 0x1ec
    fneg     f1, f1
    fmuls    f1, f27, f1
    fmadds   f0, f1, f29, f0
    stfs     f0, 0x1f4(r1)
    lfs      f0, 0xe60(r31)
    stfs     f0, 0x1f0(r1)
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x38(r12)
    mtctr    r12
    bctrl
    lis      r3, gGame@ha
    li       r4, 0x0
    addi     r3, r3, gGame@l
    bl       GetPlayerPoolObject
    bl       ResetPlayerCameraValue
    mr       r3, r31
    bl       SetMapCameraAnglesFromEyeAndInterest
    lfs      f1, 0x20(r31)
    bl       ClassifyFloatBits
    cmpwi    r3, 0x1
    beq      .L_80140494
    lfs      f1, 0x24(r31)
    bl       ClassifyFloatBits
    cmpwi    r3, 0x1
    beq      .L_80140494
    lfs      f1, 0x28(r31)
    bl       ClassifyFloatBits
    cmpwi    r3, 0x1
    bne      .L_80140578
.L_80140494:
    lfs      f0, 0xf6c(r31)
    lis      r3, sMapCameraStateFormat@ha
    addi     r3, r3, sMapCameraStateFormat@l
    stfd     f0, 0x8(r1)
    lfs      f0, 0xf70(r31)
    stfd     f0, 0x10(r1)
    lfs      f0, 0xf78(r31)
    stfd     f0, 0x18(r1)
    lfs      f0, 0xf7c(r31)
    stfd     f0, 0x20(r1)
    lfs      f0, 0xf80(r31)
    stfd     f0, 0x28(r1)
    lfs      f0, 0xf88(r31)
    stfd     f0, 0x30(r1)
    lfs      f0, 0xf8c(r31)
    stfd     f0, 0x38(r1)
    lfs      f0, 0xf90(r31)
    stfd     f0, 0x40(r1)
    lfs      f0, 0xf98(r31)
    stfd     f0, 0x48(r1)
    lfs      f0, 0xf9c(r31)
    stfd     f0, 0x50(r1)
    lfs      f0, 0xfa0(r31)
    stfd     f0, 0x58(r1)
    lfs      f0, 0xfb8(r31)
    stfd     f0, 0x60(r1)
    lfs      f0, 0xfbc(r31)
    stfd     f0, 0x68(r1)
    lfs      f0, 0xfc0(r31)
    stfd     f0, 0x70(r1)
    lwz      r0, lbl_8065B338@sda21(r0)
    stw      r0, 0x78(r1)
    lfs      f1, 0x0(r31)
    lfs      f2, 0x4(r31)
    lfs      f3, 0x8(r31)
    lfs      f4, 0x20(r31)
    lfs      f5, 0x24(r31)
    lfs      f6, 0x28(r31)
    lfs      f7, 0x30(r31)
    lwz      r4, 0x11d8(r31)
    lwz      r5, 0x11e0(r31)
    lwz      r6, 0xea0(r31)
    lwz      r7, lbl_8065B340@sda21(r0)
    lwz      r8, lbl_8065B33C@sda21(r0)
    lfs      f8, 0xf68(r31)
    lwz      r9, 0xf28(r31)
    lwz      r10, 0xdb4(r31)
    crset    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sPathCcameraactionmap2Cpp@ha
    mr       r6, r3
    addi     r3, r4, sPathCcameraactionmap2Cpp@l
    li       r5, lbl_8065E9C8@sda21
    li       r4, 0x2ab
    li       r7, sMapCameraUpdateLabel@sda21
    crclr    cr1eq
    bl       OSPanic
.L_80140578:
    lis      r3, __vt__4CVec@ha
    li       r4, 0x0
    addi     r0, r3, __vt__4CVec@l
    stw      r4, lbl_8065B344@sda21(r0)
    stw      r0, 0x204(r1)
    stw      r0, 0x214(r1)
    b        .L_80140918
.L_80140594:
    mr       r3, r31
    bl       SetMapCameraAnglesFromEyeAndInterest
    lfs      f1, 0x20(r31)
    bl       ClassifyFloatBits
    cmpwi    r3, 0x1
    beq      .L_801405CC
    lfs      f1, 0x24(r31)
    bl       ClassifyFloatBits
    cmpwi    r3, 0x1
    beq      .L_801405CC
    lfs      f1, 0x28(r31)
    bl       ClassifyFloatBits
    cmpwi    r3, 0x1
    bne      .L_801406B0
.L_801405CC:
    lfs      f0, 0xf6c(r31)
    lis      r3, sMapCameraStateFormat@ha
    addi     r3, r3, sMapCameraStateFormat@l
    stfd     f0, 0x8(r1)
    lfs      f0, 0xf70(r31)
    stfd     f0, 0x10(r1)
    lfs      f0, 0xf78(r31)
    stfd     f0, 0x18(r1)
    lfs      f0, 0xf7c(r31)
    stfd     f0, 0x20(r1)
    lfs      f0, 0xf80(r31)
    stfd     f0, 0x28(r1)
    lfs      f0, 0xf88(r31)
    stfd     f0, 0x30(r1)
    lfs      f0, 0xf8c(r31)
    stfd     f0, 0x38(r1)
    lfs      f0, 0xf90(r31)
    stfd     f0, 0x40(r1)
    lfs      f0, 0xf98(r31)
    stfd     f0, 0x48(r1)
    lfs      f0, 0xf9c(r31)
    stfd     f0, 0x50(r1)
    lfs      f0, 0xfa0(r31)
    stfd     f0, 0x58(r1)
    lfs      f0, 0xfb8(r31)
    stfd     f0, 0x60(r1)
    lfs      f0, 0xfbc(r31)
    stfd     f0, 0x68(r1)
    lfs      f0, 0xfc0(r31)
    stfd     f0, 0x70(r1)
    lwz      r0, lbl_8065B338@sda21(r0)
    stw      r0, 0x78(r1)
    lfs      f1, 0x0(r31)
    lfs      f2, 0x4(r31)
    lfs      f3, 0x8(r31)
    lfs      f4, 0x20(r31)
    lfs      f5, 0x24(r31)
    lfs      f6, 0x28(r31)
    lfs      f7, 0x30(r31)
    lwz      r4, 0x11d8(r31)
    lwz      r5, 0x11e0(r31)
    lwz      r6, 0xea0(r31)
    lwz      r7, lbl_8065B340@sda21(r0)
    lwz      r8, lbl_8065B33C@sda21(r0)
    lfs      f8, 0xf68(r31)
    lwz      r9, 0xf28(r31)
    lwz      r10, 0xdb4(r31)
    crset    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sPathCcameraactionmap2Cpp@ha
    mr       r6, r3
    addi     r3, r4, sPathCcameraactionmap2Cpp@l
    li       r5, lbl_8065E9C8@sda21
    li       r4, 0x2b5
    li       r7, sMapCameraUpdateLabel@sda21
    crclr    cr1eq
    bl       OSPanic
.L_801406B0:
    lwz      r8, 0x44(r31)
    lis      r3, __vt__4CVec@ha
    lfs      f0, lbl_8065EA44@sda21(r0)
    addi     r7, r3, __vt__4CVec@l
    lwz      r6, 0xc8(r8)
    addi     r3, r1, 0x1dc
    lwz      r0, 0xcc(r8)
    addi     r4, r1, 0x1cc
    addi     r5, r1, 0x1bc
    stw      r0, 0x1e0(r1)
    lfs      f1, 0x1e0(r1)
    stw      r6, 0x1dc(r1)
    fadds    f0, f1, f0
    lwz      r0, 0xd0(r8)
    stw      r0, 0x1e4(r1)
    stw      r7, 0x1e8(r1)
    stfs     f0, 0x1e0(r1)
    lwz      r6, 0x0(r31)
    lwz      r0, 0x4(r31)
    stw      r6, 0x1cc(r1)
    stw      r0, 0x1d0(r1)
    lwz      r0, 0x8(r31)
    stw      r0, 0x1d4(r1)
    stw      r7, 0x1d8(r1)
    stfs     f0, 0x1d0(r1)
    stw      r7, 0x1c8(r1)
    bl       PSVECSubtract
    addi     r3, r1, 0x1bc
    mr       r4, r3
    bl       PSVECNormalize
    lfs      f1, lbl_8065EA08@sda21(r0)
    addi     r3, r1, 0xd8
    addi     r4, r1, 0x1bc
    bl       fn_8000F9C0
    addi     r3, r1, 0x1dc
    addi     r4, r1, 0xd8
    mr       r5, r3
    bl       PSVECAdd
    lis      r4, __vt__4CVec@ha
    mr       r3, r31
    addi     r0, r4, __vt__4CVec@l
    mr       r4, r31
    stw      r0, 0xe4(r1)
    addi     r5, r1, 0x1dc
    addi     r6, r31, 0x570
    li       r7, 0x0
    bl       CalcIntersectVec__16CCameraActionMapFRC3VecRC3VecP11CamMapEntryl
    stw      r3, 0x970(r31)
    mr       r3, r31
    lwz      r4, 0x44(r31)
    addi     r4, r4, 0xc8
    bl       FindNearestMapCameraEntry
    stw      r3, 0x978(r31)
    mr       r3, r31
    lwz      r4, 0x44(r31)
    addi     r4, r4, 0xc8
    bl       GetNearestMapCameraEntry
    stw      r3, 0x974(r31)
    mr       r3, r31
    mr       r4, r31
    bl       FindNearestMapCameraEntry
    stw      r3, 0x980(r31)
    mr       r3, r31
    mr       r4, r31
    bl       GetNearestMapCameraEntry
    stw      r3, 0x97c(r31)
    mr       r3, r31
    bl       SetMapCameraSideEndpoints
    lwz      r3, 0x97c(r31)
    cmplwi   r3, 0x0
    beq      .L_801408B8
    lwz      r4, 0xc(r3)
    mr       r3, r31
    bl       CountMapCameraEntriesInGroup
    lwz      r4, 0x97c(r31)
    mr       r29, r3
    mr       r3, r31
    bl       CountMapCameraAdjacentSideIntersections
    cmpwi    r29, 0x2
    bge      .L_801407FC
    li       r0, 0x1
    stw      r0, 0xe9c(r31)
    b        .L_801408B8
.L_801407FC:
    cmpwi    r3, 0x0
    bne      .L_80140810
    li       r0, 0x1
    stw      r0, 0xe9c(r31)
    b        .L_801408B8
.L_80140810:
    lwz      r4, 0x97c(r31)
    lis      r3, gCameraAreaState@ha
    addi     r6, r3, gCameraAreaState@l
    lis      r3, __vt__11CamAreaInfo@ha
    lwz      r5, 0xc(r4)
    addi     r4, r3, __vt__11CamAreaInfo@l
    lwz      r3, 0x1c(r6)
    addic.   r0, r1, 0x80
    mulli    r0, r5, 0x34
    stw      r4, 0x80(r1)
    add      r3, r3, r0
    lhz      r0, 0x2e(r3)
    stw      r3, 0x84(r1)
    clrlwi   r3, r0, 31
    neg      r0, r3
    or       r0, r0, r3
    srwi     r0, r0, 31
    beq      .L_8014085C
    stw      r4, 0x80(r1)
.L_8014085C:
    cmplwi   r0, 0x0
    beq      .L_80140870
    li       r0, 0x0
    stw      r0, 0xe9c(r31)
    b        .L_80140878
.L_80140870:
    li       r0, 0x1
    stw      r0, 0xe9c(r31)
.L_80140878:
    lis      r3, gGame@ha
    li       r4, 0x0
    addi     r3, r3, gGame@l
    bl       GetPlayerPoolObject
    lwz      r0, 0xc8c(r3)
    cmpwi    r0, 0x0
    bne      .L_801408B0
    lis      r3, gGame@ha
    li       r4, 0x0
    addi     r3, r3, gGame@l
    bl       GetPlayerPoolObject
    lwz      r0, 0xb14(r3)
    cmpwi    r0, 0x0
    beq      .L_801408B8
.L_801408B0:
    li       r0, 0x1
    stw      r0, 0xe9c(r31)
.L_801408B8:
    lfs      f1, 0x90(r31)
    lfs      f0, 0xa0(r31)
    fcmpu    cr0, f1, f0
    bne      .L_80140904
    lwz      r0, 0x97c(r31)
    cmplwi   r0, 0x0
    beq      .L_80140904
    lwz      r0, 0xe9c(r31)
    cmpwi    r0, 0x0
    bne      .L_80140904
    mr       r3, r31
    li       r4, 0x0
    bl       StartMapCameraTurnInterpolation
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x1c8(r1)
    stw      r0, 0x1d8(r1)
    stw      r0, 0x1e8(r1)
    b        .L_80140918
.L_80140904:
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x1c8(r1)
    stw      r0, 0x1d8(r1)
    stw      r0, 0x1e8(r1)
.L_80140918:
    lwz      r5, 0x0(r31)
    lis      r3, __vt__4CVec@ha
    lwz      r0, 0x4(r31)
    addi     r10, r3, __vt__4CVec@l
    mr       r3, r31
    addi     r4, r1, 0x1ac
    stw      r5, 0x1ac(r1)
    addi     r5, r1, 0x19c
    addi     r6, r31, 0x10
    addi     r7, r31, 0x48
    stw      r0, 0x1b0(r1)
    li       r8, sMapCameraCollisionMode0@sda21
    lwz      r0, 0x8(r31)
    stw      r0, 0x1b4(r1)
    stw      r10, 0x1b8(r1)
    lwz      r9, 0xd8(r31)
    lwz      r0, 0xdc(r31)
    stw      r9, 0x19c(r1)
    stw      r0, 0x1a0(r1)
    lwz      r0, 0xe0(r31)
    stw      r0, 0x1a4(r1)
    stw      r10, 0x1a8(r1)
    bl       ResolveMapCameraEyeCollision
    mr       r3, r31
    addi     r4, r1, 0x1ac
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x38(r12)
    mtctr    r12
    bctrl
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x1a8(r1)
    stw      r0, 0x1b8(r1)
    stw      r0, 0x294(r1)
.L_801409A0:
    lwz      r0, 0x11d8(r31)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_80140E04
    mr       r4, r31
    addi     r3, r1, 0xcc
    bl       GetMapCameraTargetWithHeightOffset
    mr       r3, r31
    addi     r4, r1, 0xcc
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x3c(r12)
    mtctr    r12
    bctrl
    lwz      r0, 0x11d8(r31)
    rlwinm.  r0, r0, 0, 21, 21
    bne      .L_801409E4
    lfs      f0, 0xe60(r31)
    stfs     f0, 0xf6c(r31)
.L_801409E4:
    lfs      f0, 0xf7c(r31)
    mr       r3, r31
    mr       r4, r30
    stfs     f0, 0xf6c(r31)
    bl       UpdateMapCameraOrbit
    lfs      f0, 0xf7c(r31)
    stfs     f0, 0xf6c(r31)
    lwz      r0, lbl_8065B330@sda21(r0)
    cmpwi    r0, 0x0
    beq      .L_80140ABC
    lwz      r6, 0xf78(r31)
    lis      r3, __vt__4CVec@ha
    lwz      r5, 0xf7c(r31)
    addi     r0, r3, __vt__4CVec@l
    addi     r3, r31, 0xf78
    addi     r4, r31, 0xf68
    stw      r6, 0x18c(r1)
    stw      r5, 0x190(r1)
    lwz      r5, 0xf80(r31)
    stw      r5, 0x194(r1)
    stw      r0, 0x198(r1)
    bl       CopyVecToCVec
    lis      r4, __vt__4CVec@ha
    addi     r3, r1, 0x17c
    addi     r0, r4, __vt__4CVec@l
    addi     r4, r31, 0xf68
    stw      r0, 0x188(r1)
    bl       CopyVecToCVec
    lis      r3, sMapCameraSpringParameters@ha
    fmr      f1, f31
    lfs      f2, sMapCameraSpringParameters@l(r3)
    addi     r3, r1, 0xbc
    lfs      f3, 0x5c(r31)
    addi     r4, r1, 0x17c
    lfs      f4, 0x80(r31)
    addi     r5, r31, 0x70
    addi     r6, r31, 0x60
    bl       SpringDamp
    addi     r3, r1, 0x17c
    addi     r4, r1, 0xbc
    bl       CopyVecToCVec
    lis      r4, __vt__4CVec@ha
    addi     r3, r31, 0xf68
    addi     r0, r4, __vt__4CVec@l
    addi     r4, r1, 0x17c
    stw      r0, 0xc8(r1)
    bl       CopyVecToCVec
    addi     r3, r31, 0xf78
    addi     r4, r1, 0x18c
    bl       CopyVecToCVec
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x188(r1)
    stw      r0, 0x198(r1)
.L_80140ABC:
    addi     r3, r31, 0xf68
    addi     r4, r31, 0xf78
    bl       GetHorizontalDistance
    stfs     f1, lbl_8065B358@sda21(r0)
    lwz      r0, 0x11d8(r31)
    rlwinm.  r0, r0, 0, 29, 29
    bne      .L_80140CEC
    lwz      r28, lbl_8065B348@sda21(r0)
    addi     r3, r31, 0xf68
    addi     r4, r31, 0x10
    bl       GetHeadingBetweenPositions
    fmr      f27, f1
    mr       r3, r31
    addi     r4, r31, 0xf68
    addi     r5, r31, 0xf78
    addi     r6, r31, 0x10
    addi     r7, r31, 0x48
    li       r8, sMapCameraCollisionMode1@sda21
    bl       ResolveMapCameraEyeCollision
    lfs      f1, lbl_8065B34C@sda21(r0)
    mr       r29, r3
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_80140BD4
    addi     r3, r31, 0xf68
    addi     r4, r31, 0x10
    bl       GetHeadingBetweenPositions
    fmr      f29, f1
    addi     r3, r31, 0xf78
    addi     r4, r31, 0x10
    bl       GetHeadingBetweenPositions
    fmr      f2, f27
    lfs      f1, lbl_8065E990@sda21(r0)
    lfs      f0, lbl_8065E994@sda21(r0)
    b        .L_80140B4C
.L_80140B48:
    fadds    f2, f2, f1
.L_80140B4C:
    fcmpo    cr0, f2, f0
    blt      .L_80140B48
    lfs      f0, lbl_8065E990@sda21(r0)
    b        .L_80140B60
.L_80140B5C:
    fsubs    f2, f2, f0
.L_80140B60:
    fcmpo    cr0, f2, f0
    cror     eq, gt, eq
    beq      .L_80140B5C
    lfs      f1, lbl_8065E990@sda21(r0)
    lfs      f0, lbl_8065E994@sda21(r0)
    b        .L_80140B7C
.L_80140B78:
    fadds    f29, f29, f1
.L_80140B7C:
    fcmpo    cr0, f29, f0
    blt      .L_80140B78
    lfs      f0, lbl_8065E990@sda21(r0)
    b        .L_80140B90
.L_80140B8C:
    fsubs    f29, f29, f0
.L_80140B90:
    fcmpo    cr0, f29, f0
    cror     eq, gt, eq
    beq      .L_80140B8C
    fsubs    f1, f29, f2
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_80140BC4
    lwz      r3, lbl_8065B350@sda21(r0)
    li       r0, 0x1
    stw      r0, lbl_8065B348@sda21(r0)
    addi     r0, r3, 0x1
    stw      r0, lbl_8065B350@sda21(r0)
    b        .L_80140C8C
.L_80140BC4:
    li       r0, 0x0
    stw      r0, lbl_8065B348@sda21(r0)
    stw      r0, lbl_8065B350@sda21(r0)
    b        .L_80140C8C
.L_80140BD4:
    bge      .L_80140C8C
    addi     r3, r31, 0xf68
    addi     r4, r31, 0x10
    bl       GetHeadingBetweenPositions
    fmr      f29, f1
    addi     r3, r31, 0xf78
    addi     r4, r31, 0x10
    bl       GetHeadingBetweenPositions
    fmr      f2, f27
    lfs      f1, lbl_8065E990@sda21(r0)
    lfs      f0, lbl_8065E994@sda21(r0)
    b        .L_80140C08
.L_80140C04:
    fadds    f2, f2, f1
.L_80140C08:
    fcmpo    cr0, f2, f0
    blt      .L_80140C04
    lfs      f0, lbl_8065E990@sda21(r0)
    b        .L_80140C1C
.L_80140C18:
    fsubs    f2, f2, f0
.L_80140C1C:
    fcmpo    cr0, f2, f0
    cror     eq, gt, eq
    beq      .L_80140C18
    lfs      f1, lbl_8065E990@sda21(r0)
    lfs      f0, lbl_8065E994@sda21(r0)
    b        .L_80140C38
.L_80140C34:
    fadds    f29, f29, f1
.L_80140C38:
    fcmpo    cr0, f29, f0
    blt      .L_80140C34
    lfs      f0, lbl_8065E990@sda21(r0)
    b        .L_80140C4C
.L_80140C48:
    fsubs    f29, f29, f0
.L_80140C4C:
    fcmpo    cr0, f29, f0
    cror     eq, gt, eq
    beq      .L_80140C48
    fsubs    f1, f29, f2
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_80140C80
    lwz      r3, lbl_8065B350@sda21(r0)
    li       r0, 0x1
    stw      r0, lbl_8065B348@sda21(r0)
    addi     r0, r3, 0x1
    stw      r0, lbl_8065B350@sda21(r0)
    b        .L_80140C8C
.L_80140C80:
    li       r0, 0x0
    stw      r0, lbl_8065B348@sda21(r0)
    stw      r0, lbl_8065B350@sda21(r0)
.L_80140C8C:
    lwz      r0, lbl_8065B348@sda21(r0)
    cmpwi    r0, 0x1
    bne      .L_80140CB8
    cmpw     r28, r0
    beq      .L_80140CB8
    addi     r3, r31, 0xf68
    addi     r4, r31, 0xf78
    bl       CopyVecToCVec
    addi     r3, r31, 0xf98
    addi     r4, r31, 0xfb8
    bl       CopyVecToCVec
.L_80140CB8:
    cmpwi    r29, 0x0
    beq      .L_80140CD8
    lwz      r0, lbl_8065B330@sda21(r0)
    cmpwi    r0, 0x0
    beq      .L_80140CEC
    li       r0, 0x0
    stw      r0, lbl_8065B330@sda21(r0)
    b        .L_80140CEC
.L_80140CD8:
    lwz      r0, lbl_8065B330@sda21(r0)
    cmpwi    r0, 0x0
    bne      .L_80140CEC
    li       r0, 0x1
    stw      r0, lbl_8065B330@sda21(r0)
.L_80140CEC:
    addi     r3, r31, 0xf68
    addi     r4, r31, 0xf78
    bl       GetHorizontalDistance
    frsp     f2, f1
    lfs      f0, lbl_8065B358@sda21(r0)
    stfs     f1, lbl_8065B354@sda21(r0)
    fcmpo    cr0, f2, f0
    ble      .L_80140D18
    lfs      f0, lbl_8065E994@sda21(r0)
    stfs     f0, 0xf64(r31)
    b        .L_80140D34
.L_80140D18:
    lfs      f0, lbl_8065EA08@sda21(r0)
    stfs     f0, 0xf64(r31)
    lwz      r0, 0x11d8(r31)
    rlwinm.  r0, r0, 0, 27, 27
    beq      .L_80140D34
    lfs      f0, lbl_8065E9D4@sda21(r0)
    stfs     f0, 0xf64(r31)
.L_80140D34:
    lfs      f0, 0xe60(r31)
    addi     r3, r1, 0xac
    stfs     f0, 0xf7c(r31)
    stfs     f0, 0xf6c(r31)
    lfs      f1, 0x0(r31)
    lfs      f2, 0xf6c(r31)
    lfs      f3, 0x8(r31)
    bl       __ct__4CVecFfff
    mr       r3, r31
    addi     r4, r1, 0xac
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x38(r12)
    mtctr    r12
    bctrl
    lis      r3, __vt__4CVec@ha
    li       r0, 0x0
    addi     r3, r3, __vt__4CVec@l
    stw      r3, 0xb8(r1)
    lwz      r5, 0xf68(r31)
    lwz      r4, 0xf6c(r31)
    stw      r5, 0xa0(r1)
    lwz      r3, 0xf70(r31)
    lfs      f1, 0xf78(r31)
    lfs      f0, 0xa0(r1)
    stw      r5, 0x88(r1)
    fcmpu    cr0, f1, f0
    stw      r4, 0x8c(r1)
    stw      r3, 0x90(r1)
    stw      r4, 0xa4(r1)
    stw      r3, 0xa8(r1)
    bne      .L_80140DD0
    lfs      f1, 0xf7c(r31)
    lfs      f0, 0xa4(r1)
    fcmpu    cr0, f1, f0
    bne      .L_80140DD0
    lfs      f1, 0xf80(r31)
    lfs      f0, 0xa8(r1)
    fcmpu    cr0, f1, f0
    beq      .L_80140DD4
.L_80140DD0:
    li       r0, 0x1
.L_80140DD4:
    clrlwi.  r0, r0, 24
    beq      .L_80140E04
    lwz      r6, 0x88(r1)
    mr       r3, r31
    lwz      r5, 0x8c(r1)
    addi     r4, r1, 0x94
    lwz      r0, 0x90(r1)
    stw      r6, 0x94(r1)
    stw      r5, 0x98(r1)
    stw      r0, 0x9c(r1)
    lfs      f1, 0xf64(r31)
    bl       StartCameraEyeInterpolation
.L_80140E04:
    mr       r3, r31
    mr       r4, r30
    bl       UpdateCameraEyeInterpolation
    mr       r3, r31
    addi     r4, r31, 0x70
    bl       GetHorizontalDistance
    lfs      f2, lbl_8065EA70@sda21(r0)
    lfs      f0, lbl_8065E9A8@sda21(r0)
    fdivs    f27, f1, f2
    fcmpo    cr0, f27, f0
    ble      .L_80140E34
    fmr      f27, f0
.L_80140E34:
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f27, f0
    bge      .L_80140E44
    fmr      f27, f0
.L_80140E44:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x5d38
    bl       fn_801486D8
    lfs      f0, lbl_8065E9A8@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, lt, eq
    bne      .L_80140E6C
    lfs      f27, lbl_8065E994@sda21(r0)
.L_80140E6C:
    lfs      f2, 0x14(r31)
    lfs      f1, 0xe60(r31)
    lfs      f0, lbl_8065E994@sda21(r0)
    fsubs    f3, f1, f2
    fcmpo    cr0, f3, f0
    bge      .L_80140E88
    fmr      f3, f0
.L_80140E88:
    fmuls    f0, f3, f27
    lwz      r3, 0xe78(r31)
    lwz      r12, 0x0(r3)
    fadds    f0, f2, f0
    lwz      r12, 0x18(r12)
    fsubs    f27, f0, f1
    mtctr    r12
    bctrl
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpu    cr0, f0, f27
    beq      .L_80140ED8
    addi     r0, r31, 0xe7c
    fmr      f2, f27
    stw      r0, 0xe78(r31)
    lfs      f3, lbl_8065EA74@sda21(r0)
    lwz      r3, 0xe78(r31)
    lwz      r12, 0x0(r3)
    lwz      r12, 0xc(r12)
    mtctr    r12
    bctrl
.L_80140ED8:
    lwz      r5, 0x0(r31)
    mr       r3, r31
    lwz      r0, 0x4(r31)
    addi     r4, r1, 0x170
    stw      r5, 0x170(r1)
    stw      r0, 0x174(r1)
    lwz      r0, 0x8(r31)
    stw      r0, 0x178(r1)
    lfs      f1, 0xe60(r31)
    stfs     f1, 0x174(r1)
    lfs      f0, 0xe98(r31)
    fadds    f0, f1, f0
    stfs     f0, 0x174(r1)
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x38(r12)
    mtctr    r12
    bctrl
    mr       r3, r31
    mr       r4, r31
    bl       SetMapCameraPreviousEye
    addi     r3, r31, 0xf78
    addi     r4, r31, 0xf68
    bl       CopyVecToCVec
    addi     r3, r31, 0xfa8
    addi     r4, r31, 0xf88
    bl       CopyVecToCVec
    mr       r3, r31
    addi     r4, r31, 0x10
    bl       SetMapCameraPreviousInterest
    li       r0, 0x0
    stw      r0, 0x11e0(r31)
    lwz      r3, 0x11e0(r31)
    lwz      r0, 0x11d8(r31)
    or       r0, r3, r0
    stw      r0, 0x11e0(r31)
.L_80140F64:
    psq_l    f31, 0x318(r1), 0, qr0
    lfd      f31, 0x310(r1)
    psq_l    f30, 0x308(r1), 0, qr0
    lfd      f30, 0x300(r1)
    psq_l    f29, 0x2f8(r1), 0, qr0
    lfd      f29, 0x2f0(r1)
    psq_l    f28, 0x2e8(r1), 0, qr0
    lfd      f28, 0x2e0(r1)
    psq_l    f27, 0x2d8(r1), 0, qr0
    lfd      f27, 0x2d0(r1)
    lwz      r31, 0x2cc(r1)
    lwz      r30, 0x2c8(r1)
    lwz      r29, 0x2c4(r1)
    lwz      r0, 0x324(r1)
    lwz      r28, 0x2c0(r1)
    mtlr     r0
    addi     r1, r1, 0x320
    blr
*/

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
	s32 arg = gCameraAreaState._0C;
	if (gCameraAreaState._04 != gCameraAreaState._08 && gCameraAreaState._04 != -1) {
		if (GetCameraAreaMode(&gCameraAreaState) == 1) {
			gGame.mMotionControlCamera.ChangeAction(1);
			gCameraAreaState._18 = gCameraAreaState._14;
			SelectAreaCameraEntryByViewDirection((CCameraActionArea*)gGame.mCameraActions.mTable[1], 1, 2.0f, arg);
			return;
		}
		SetFovStep(this, gCameraAreaState._14, 15.0f);
		SelectAreaCameraEntryWithControl((CCameraActionArea*)gGame.mCameraActions.mTable[1], arg, 0, 3, 1, 1.0f);
	}
	_DA4 = _DA8;
	SelectMapCameraArea(this);
	if (_DA8 != _DA4) {
		StartMapCameraAreaHeightInterpolation(this);
	}
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    lis      r4, gCameraAreaState@ha
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    mr       r30, r3
    addi     r3, r4, gCameraAreaState@l
    li       r4, 0x0
    stw      r29, 0x14(r1)
    stw      r28, 0x10(r1)
    bl       UpdateCameraAreaIndexAndBaseFov
    lis      r3, gCameraAreaState@ha
    addi     r3, r3, gCameraAreaState@l
    lwz      r4, 0x4(r3)
    lwz      r0, 0x8(r3)
    lwz      r31, 0xc(r3)
    cmpw     r4, r0
    beq      .L_80141234
    cmpwi    r4, -0x1
    beq      .L_80141234
    bl       GetCameraAreaMode
    cmpwi    r3, 0x1
    bne      .L_80141130
    lis      r3, gGame@ha
    li       r4, 0x1
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x5d38
    bl       fn_8014946C
    lis      r4, gCameraAreaState@ha
    lis      r3, gGame@ha
    addi     r6, r4, gCameraAreaState@l
    lfs      f1, lbl_8065EA04@sda21(r0)
    lwz      r0, 0x14(r6)
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    mr       r5, r31
    stw      r0, 0x18(r6)
    li       r4, 0x1
    lwz      r3, 0x7df0(r3)
    bl       SelectAreaCameraEntryByViewDirection
    b        .L_8014125C
.L_80141130:
    lis      r3, gCameraAreaState@ha
    addi     r29, r3, gCameraAreaState@l
    lwz      r28, 0x14(r29)
    lfs      f2, 0x10(r29)
    cmpwi    r28, 0x2
    beq      .L_801411B0
    bge      .L_8014115C
    cmpwi    r28, 0x0
    beq      .L_80141168
    bge      .L_80141190
    b        .L_80141168
.L_8014115C:
    cmpwi    r28, 0x4
    bge      .L_80141168
    b        .L_801411D8
.L_80141168:
    addi     r3, r30, 0xc8
    lfs      f0, lbl_8065EA3C@sda21(r0)
    lwz      r12, 0xc8(r30)
    fmuls    f2, f0, f2
    lfs      f1, 0x34(r30)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
    b        .L_801411FC
.L_80141190:
    lwz      r12, 0xc8(r30)
    addi     r3, r30, 0xc8
    lfs      f1, 0x34(r30)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
    b        .L_801411FC
.L_801411B0:
    addi     r3, r30, 0xc8
    lfs      f0, lbl_8065EA40@sda21(r0)
    lwz      r12, 0xc8(r30)
    fmuls    f2, f0, f2
    lfs      f1, 0x34(r30)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
    b        .L_801411FC
.L_801411D8:
    addi     r3, r30, 0xc8
    lfs      f0, lbl_8065EA04@sda21(r0)
    lwz      r12, 0xc8(r30)
    fmuls    f2, f0, f2
    lfs      f1, 0x34(r30)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
.L_801411FC:
    lis      r3, gGame@ha
    lis      r4, gCameraAreaState@ha
    addi     r3, r3, gGame@l
    stw      r28, 0x14(r29)
    addi     r4, r4, gCameraAreaState@l
    lfs      f1, lbl_8065E9A8@sda21(r0)
    addis    r3, r3, 0xe
    stw      r28, 0x18(r4)
    lwz      r3, 0x7df0(r3)
    mr       r4, r31
    li       r5, 0x0
    li       r6, 0x3
    li       r7, 0x1
    bl       SelectAreaCameraEntryWithControl
.L_80141234:
    lwz      r0, 0xda8(r30)
    mr       r3, r30
    stw      r0, 0xda4(r30)
    bl       SelectMapCameraArea
    lwz      r3, 0xda8(r30)
    lwz      r0, 0xda4(r30)
    cmpw     r3, r0
    beq      .L_8014125C
    mr       r3, r30
    bl       StartMapCameraAreaHeightInterpolation
.L_8014125C:
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

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
	_40 = 3;
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
	_E64 = NULL;
	_E68.Start(0.0f, 0.0f, 0.1f);
	if (mTarget != NULL) {
		Mtx mtx;
		PSMTXCopy(HSD_JObjGetMtxPtr(mTarget->mJObj), mtx);
		CVec pos(0.0f, 13.0f, -1.0f);
		PSMTXMultVec(mtx, &pos, &pos);
		_70   = CVec(pos.x, pos.y, pos.z);
		_60.x = _70.x;
		_60.y = _70.y;
		_60.z = _70.z;
	}
	_8C.x         = 0.0f;
	_8C.y         = 0.0f;
	_8C.z         = 0.0f;
	_8C.mTarget.x = 0.0f;
	_8C.mTarget.y = 0.0f;
	_8C.mTarget.z = 0.0f;
	_8C.mSpeed.x  = 0.0f;
	_8C.mSpeed.y  = 0.0f;
	_8C.mSpeed.z  = 0.0f;
	_80           = 60.0f;
	_58           = _80;
	_84           = _80;
	SetFovStep(this, gCameraAreaState._14, 15.0f);
	_DAC               = 1;
	_DB0               = 1;
	mTurnInterpolation = NULL;
	mTurnInterpolation = NULL;
	_E30               = &_E14;
	_E30->Start(13.0f, 13.0f, 1.0f);
	_DA8 = -1;
	_DA4 = -1;
	_E50 = &_E34;
	_E50->Start(mEye.y, mEye.y, 1.0f);
	_E58 = 0;
	_E5C = 0;
	_E54 = 13.0f;
	_E9C = 1;
	ResetMapCameraTargetEye(this);
	mStatus.mFlags = 0;
	mStatus.mFlags |= 0x10000;
	lbl_8065B344 = 0;
	_E78         = &_E7C;
	_E7C.Start(0.0f, 0.0f, 1.0f);
	_E98              = 0.0f;
	mPreviousInterest = CVec(mInterest.x, mInterest.y, mInterest.z);
	if (!mKeepPreviousEye) {
		mPreviousEye     = CVec(mInterest.x, mInterest.y, mInterest.z);
		mKeepPreviousEye = FALSE;
	}
}
/*
    stwu     r1, -0xb0(r1)
    mflr     r0
    stw      r0, 0xb4(r1)
    li       r0, 0x3
    stw      r31, 0xac(r1)
    mr       r31, r3
    stw      r30, 0xa8(r1)
    stw      r29, 0xa4(r1)
    mr       r29, r4
    addi     r4, r29, 0x94
    stw      r0, 0x40(r3)
    lwz      r12, 0x3c(r3)
    lwz      r12, 0x38(r12)
    mtctr    r12
    bctrl
    lfs      f2, 0x94(r29)
    lis      r3, __vt__4CVec@ha
    lfs      f1, 0x98(r29)
    addi     r0, r3, __vt__4CVec@l
    stfs     f2, 0xd8(r31)
    mr       r3, r31
    lfs      f0, 0x9c(r29)
    addi     r4, r29, 0xa4
    stfs     f1, 0xdc(r31)
    stfs     f0, 0xe0(r31)
    lwz      r12, 0x3c(r31)
    stw      r0, 0x44(r1)
    lwz      r12, 0x3c(r12)
    stfs     f2, 0x38(r1)
    stfs     f1, 0x3c(r1)
    stfs     f0, 0x40(r1)
    stw      r0, 0x44(r1)
    mtctr    r12
    bctrl
    mr       r3, r31
    lfs      f1, 0xe4(r29)
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x44(r12)
    mtctr    r12
    bctrl
    mr       r3, r31
    lfs      f1, 0xd8(r29)
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x48(r12)
    mtctr    r12
    bctrl
    mr       r3, r31
    addi     r4, r29, 0xb4
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x40(r12)
    mtctr    r12
    bctrl
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r0, -0x6944(r3)
    stw      r0, 0x44(r31)
    lwz      r4, 0x44(r31)
    cmplwi   r4, 0x0
    beq      .L_801413A4
    lwz      r12, 0x34(r4)
    addi     r3, r1, 0x48
    lwz      r12, 0xec(r12)
    mtctr    r12
    bctrl
    mr       r3, r31
    addi     r4, r1, 0x48
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x3c(r12)
    mtctr    r12
    bctrl
.L_801413A4:
    li       r0, 0x0
    lfs      f1, lbl_8065E994@sda21(r0)
    stw      r0, 0xe64(r31)
    addi     r3, r31, 0xe68
    fmr      f2, f1
    lfs      f3, lbl_8065EA68@sda21(r0)
    lwz      r12, 0xe68(r31)
    lwz      r12, 0xc(r12)
    mtctr    r12
    bctrl
    lwz      r3, 0x44(r31)
    cmplwi   r3, 0x0
    beq      .L_801414CC
    lwz      r30, 0x0(r3)
    cmplwi   r30, 0x0
    bne      .L_801413F4
    li       r3, lbl_8065E9F0@sda21
    li       r4, 0x47c
    li       r5, lbl_8065E9F8@sda21
    bl       __assert
.L_801413F4:
    cmplwi   r30, 0x0
    beq      .L_8014143C
    bne      .L_80141410
    li       r3, lbl_8065E9F0@sda21
    li       r4, 0x25d
    li       r5, lbl_8065E9F8@sda21
    bl       __assert
.L_80141410:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_8014142C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_8014142C
    li       r3, 0x1
.L_8014142C:
    clrlwi.  r0, r3, 24
    beq      .L_8014143C
    mr       r3, r30
    bl       fn_801C8730
.L_8014143C:
    addi     r3, r30, 0x44
    addi     r4, r1, 0x64
    bl       PSMTXCopy
    lfs      f2, lbl_8065E994@sda21(r0)
    lis      r3, __vt__4CVec@ha
    lfs      f1, lbl_8065E9E4@sda21(r0)
    addi     r0, r3, __vt__4CVec@l
    lfs      f0, lbl_8065EA00@sda21(r0)
    addi     r4, r1, 0x54
    stw      r0, 0x60(r1)
    mr       r5, r4
    addi     r3, r1, 0x64
    stfs     f2, 0x54(r1)
    stfs     f1, 0x58(r1)
    stfs     f0, 0x5c(r1)
    bl       PSMTXMultVec
    lfs      f3, 0x54(r1)
    lis      r3, __vt__4CVec@ha
    lfs      f2, 0x58(r1)
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0x5c(r1)
    stw      r0, 0x34(r1)
    stfs     f3, 0x70(r31)
    stfs     f2, 0x74(r31)
    stfs     f1, 0x78(r31)
    lfs      f0, 0x70(r31)
    stfs     f3, 0x28(r1)
    stfs     f0, 0x60(r31)
    lfs      f0, 0x74(r31)
    stfs     f2, 0x2c(r1)
    stfs     f0, 0x64(r31)
    lfs      f0, 0x78(r31)
    stfs     f1, 0x30(r1)
    stfs     f0, 0x68(r31)
    stw      r0, 0x34(r1)
    stw      r0, 0x60(r1)
.L_801414CC:
    lfs      f1, lbl_8065E994@sda21(r0)
    lis      r3, gCameraAreaState@ha
    lfs      f0, lbl_8065EA70@sda21(r0)
    addi     r30, r3, gCameraAreaState@l
    stfs     f1, 0x8c(r31)
    stfs     f1, 0x90(r31)
    stfs     f1, 0x94(r31)
    stfs     f1, 0x9c(r31)
    stfs     f1, 0xa0(r31)
    stfs     f1, 0xa4(r31)
    stfs     f1, 0xac(r31)
    stfs     f1, 0xb0(r31)
    stfs     f1, 0xb4(r31)
    stfs     f0, 0x80(r31)
    lfs      f0, 0x80(r31)
    stfs     f0, 0x58(r31)
    lfs      f0, 0x80(r31)
    stfs     f0, 0x84(r31)
    lwz      r29, 0x14(r30)
    lfs      f2, 0x10(r30)
    cmpwi    r29, 0x2
    beq      .L_8014158C
    bge      .L_80141538
    cmpwi    r29, 0x0
    beq      .L_80141544
    bge      .L_8014156C
    b        .L_80141544
.L_80141538:
    cmpwi    r29, 0x4
    bge      .L_80141544
    b        .L_801415B4
.L_80141544:
    addi     r3, r31, 0xc8
    lfs      f0, lbl_8065EA3C@sda21(r0)
    lwz      r12, 0xc8(r31)
    fmuls    f2, f0, f2
    lfs      f1, 0x34(r31)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
    b        .L_801415D8
.L_8014156C:
    lwz      r12, 0xc8(r31)
    addi     r3, r31, 0xc8
    lfs      f1, 0x34(r31)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
    b        .L_801415D8
.L_8014158C:
    addi     r3, r31, 0xc8
    lfs      f0, lbl_8065EA40@sda21(r0)
    lwz      r12, 0xc8(r31)
    fmuls    f2, f0, f2
    lfs      f1, 0x34(r31)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
    b        .L_801415D8
.L_801415B4:
    addi     r3, r31, 0xc8
    lfs      f0, lbl_8065EA04@sda21(r0)
    lwz      r12, 0xc8(r31)
    fmuls    f2, f0, f2
    lfs      f1, 0x34(r31)
    lwz      r12, 0xc(r12)
    lfs      f3, lbl_8065EA48@sda21(r0)
    mtctr    r12
    bctrl
.L_801415D8:
    lis      r3, gCameraAreaState@ha
    lfs      f1, lbl_8065E9E4@sda21(r0)
    addi     r5, r3, gCameraAreaState@l
    stw      r29, 0x14(r30)
    li       r4, 0x1
    li       r3, 0x0
    stw      r29, 0x18(r5)
    addi     r0, r31, 0xe14
    fmr      f2, f1
    lfs      f3, lbl_8065E9A8@sda21(r0)
    stw      r4, 0xdac(r31)
    stw      r4, 0xdb0(r31)
    stw      r3, 0xdb4(r31)
    stw      r3, 0xdb4(r31)
    stw      r0, 0xe30(r31)
    lwz      r3, 0xe30(r31)
    lwz      r12, 0x0(r3)
    lwz      r12, 0xc(r12)
    mtctr    r12
    bctrl
    li       r3, -0x1
    addi     r0, r31, 0xe34
    stw      r3, 0xda8(r31)
    lfs      f3, lbl_8065E9A8@sda21(r0)
    stw      r3, 0xda4(r31)
    stw      r0, 0xe50(r31)
    lwz      r3, 0xe50(r31)
    lfs      f1, 0x4(r31)
    lwz      r12, 0x0(r3)
    fmr      f2, f1
    lwz      r12, 0xc(r12)
    mtctr    r12
    bctrl
    li       r4, 0x0
    lfs      f0, lbl_8065E9E4@sda21(r0)
    stw      r4, 0xe58(r31)
    li       r0, 0x1
    mr       r3, r31
    stw      r4, 0xe5c(r31)
    stfs     f0, 0xe54(r31)
    stw      r0, 0xe9c(r31)
    bl       ResetMapCameraTargetEye
    li       r4, 0x0
    lfs      f1, lbl_8065E994@sda21(r0)
    stw      r4, 0x11d8(r31)
    addi     r3, r31, 0xe7c
    fmr      f2, f1
    lfs      f3, lbl_8065E9A8@sda21(r0)
    lwz      r0, 0x11d8(r31)
    oris     r0, r0, 0x1
    stw      r0, 0x11d8(r31)
    stw      r4, lbl_8065B344@sda21(r0)
    stw      r3, 0xe78(r31)
    lwz      r12, 0xe7c(r31)
    lwz      r12, 0xc(r12)
    mtctr    r12
    bctrl
    lfs      f0, lbl_8065E994@sda21(r0)
    lis      r3, __vt__4CVec@ha
    addi     r3, r3, __vt__4CVec@l
    stfs     f0, 0xe98(r31)
    lfs      f2, 0x10(r31)
    lfs      f1, 0x14(r31)
    lfs      f0, 0x18(r31)
    stw      r3, 0x24(r1)
    stfs     f2, 0x48(r31)
    stfs     f1, 0x4c(r31)
    stfs     f0, 0x50(r31)
    lwz      r0, 0x11e8(r31)
    stfs     f2, 0x18(r1)
    cmpwi    r0, 0x0
    stfs     f1, 0x1c(r1)
    stfs     f0, 0x20(r1)
    stw      r3, 0x24(r1)
    bne      .L_80141738
    lfs      f2, 0x10(r31)
    li       r0, 0x0
    lfs      f1, 0x14(r31)
    lfs      f0, 0x18(r31)
    stw      r3, 0x14(r1)
    stfs     f2, 0xd8(r31)
    stfs     f1, 0xdc(r31)
    stfs     f0, 0xe0(r31)
    stfs     f2, 0x8(r1)
    stfs     f1, 0xc(r1)
    stfs     f0, 0x10(r1)
    stw      r3, 0x14(r1)
    stw      r0, 0x11e8(r31)
.L_80141738:
    lwz      r0, 0xb4(r1)
    lwz      r31, 0xac(r1)
    lwz      r30, 0xa8(r1)
    lwz      r29, 0xa4(r1)
    mtlr     r0
    addi     r1, r1, 0xb0
    blr
*/

/**
 * @note Address: 0x80141754
 * @note Size: 0x4FC
 */
extern "C" void Reset2__16CCameraActionMapFv(CCameraActionMap* map)
{
	map->SetInterest(map->mTarget->GetCameraTarget());
	Vec eye;
	GetMapCameraEyeBehindTarget(map, eye, 0);
	map->SetEye(eye);
	SetMapCameraAnglesFromEyeAndInterest(map);
	if (__fpclassifyf(map->mAngle.x) == FP_NAN || __fpclassifyf(map->mAngle.y) == FP_NAN || __fpclassifyf(map->mAngle.z) == FP_NAN) {
		OSPanic("ccameraactionmap2.cpp", 251, "%s%s",
		        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER OP(%d)HANG(%d(%d))TGTEYE(%f, "
		                             "%f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, "
		                             "%f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
		                             map->mEye.x, map->mEye.y, map->mEye.z, map->mAngle.x, map->mAngle.y, map->mAngle.z, map->mDistance,
		                             map->mStatus.mFlags, map->mPreviousStatus.mFlags, map->_EA0, lbl_8065B340, lbl_8065B33C, map->_F68.x,
		                             map->_F68.y, map->_F68.z, map->_F78.x, map->_F78.y, map->_F78.z, map->_F28, map->mTurnInterpolation,
		                             map->_F88.x, map->_F88.y, map->_F88.z, map->_F98.x, map->_F98.y, map->_F98.z, map->_FB8.x, map->_FB8.y,
		                             map->_FB8.z, lbl_8065B338),
		        "Reset2");
	}
}
/*
    stwu     r1, -0xd0(r1)
    mflr     r0
    stw      r0, 0xd4(r1)
    stfd     f31, 0xc0(r1)
    psq_st   f31, 0xc8(r1), 0, qr0
    stw      r31, 0xbc(r1)
    mr       r31, r3
    addi     r3, r1, 0xa0
    lwz      r4, 0x44(r31)
    lwz      r12, 0x34(r4)
    lwz      r12, 0xec(r12)
    mtctr    r12
    bctrl
    mr       r3, r31
    addi     r4, r1, 0xa0
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x3c(r12)
    mtctr    r12
    bctrl
    mr       r3, r31
    addi     r4, r1, 0xac
    li       r5, 0x0
    bl       GetMapCameraEyeBehindTarget
    mr       r3, r31
    addi     r4, r1, 0xac
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x38(r12)
    mtctr    r12
    bctrl
    lfs      f1, 0x18(r31)
    lfs      f0, 0x8(r31)
    lfs      f2, 0x10(r31)
    fsubs    f1, f1, f0
    lfs      f0, 0x0(r31)
    lfs      f3, 0x14(r31)
    fsubs    f4, f2, f0
    lfs      f2, 0x4(r31)
    fmuls    f1, f1, f1
    lfs      f0, lbl_8065E994@sda21(r0)
    fsubs    f31, f3, f2
    fmadds   f1, f4, f4, f1
    fcmpo    cr0, f1, f0
    ble      .L_80141848
    frsqrte  f2, f1
    lfd      f4, lbl_8065E9B0@sda21(r0)
    lfd      f3, lbl_8065E9B8@sda21(r0)
    fmul     f0, f2, f2
    fmul     f2, f4, f2
    fnmsub   f0, f1, f0, f3
    fmul     f2, f2, f0
    fmul     f0, f2, f2
    fmul     f2, f4, f2
    fnmsub   f0, f1, f0, f3
    fmul     f2, f2, f0
    fmul     f0, f2, f2
    fmul     f2, f4, f2
    fnmsub   f0, f1, f0, f3
    fmul     f0, f2, f0
    fmul     f1, f1, f0
    frsp     f1, f1
    b        .L_801418CC
.L_80141848:
    lfd      f0, lbl_8065E9C0@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_80141860
    lis      r3, __float_nan@ha
    lfs      f1, __float_nan@l(r3)
    b        .L_801418CC
.L_80141860:
    stfs     f1, 0x84(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x84(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_80141888
    bge      .L_801418B8
    cmpwi    r3, 0x0
    beq      .L_801418A0
    b        .L_801418B8
.L_80141888:
    clrlwi.  r0, r4, 9
    beq      .L_80141898
    li       r0, 0x1
    b        .L_801418BC
.L_80141898:
    li       r0, 0x2
    b        .L_801418BC
.L_801418A0:
    clrlwi.  r0, r4, 9
    beq      .L_801418B0
    li       r0, 0x5
    b        .L_801418BC
.L_801418B0:
    li       r0, 0x3
    b        .L_801418BC
.L_801418B8:
    li       r0, 0x4
.L_801418BC:
    cmpwi    r0, 0x1
    bne      .L_801418CC
    lis      r3, __float_nan@ha
    lfs      f1, __float_nan@l(r3)
.L_801418CC:
    fmuls    f2, f31, f31
    lfs      f0, lbl_8065E994@sda21(r0)
    fcmpo    cr0, f2, f0
    ble      .L_80141924
    frsqrte  f3, f2
    lfd      f5, lbl_8065E9B0@sda21(r0)
    lfd      f4, lbl_8065E9B8@sda21(r0)
    fmul     f0, f3, f3
    fmul     f3, f5, f3
    fnmsub   f0, f2, f0, f4
    fmul     f3, f3, f0
    fmul     f0, f3, f3
    fmul     f3, f5, f3
    fnmsub   f0, f2, f0, f4
    fmul     f3, f3, f0
    fmul     f0, f3, f3
    fmul     f3, f5, f3
    fnmsub   f0, f2, f0, f4
    fmul     f0, f3, f0
    fmul     f2, f2, f0
    frsp     f2, f2
    b        .L_801419A8
.L_80141924:
    lfd      f0, lbl_8065E9C0@sda21(r0)
    fcmpo    cr0, f2, f0
    bge      .L_8014193C
    lis      r3, __float_nan@ha
    lfs      f2, __float_nan@l(r3)
    b        .L_801419A8
.L_8014193C:
    stfs     f2, 0x80(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x80(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_80141964
    bge      .L_80141994
    cmpwi    r3, 0x0
    beq      .L_8014197C
    b        .L_80141994
.L_80141964:
    clrlwi.  r0, r4, 9
    beq      .L_80141974
    li       r0, 0x1
    b        .L_80141998
.L_80141974:
    li       r0, 0x2
    b        .L_80141998
.L_8014197C:
    clrlwi.  r0, r4, 9
    beq      .L_8014198C
    li       r0, 0x5
    b        .L_80141998
.L_8014198C:
    li       r0, 0x3
    b        .L_80141998
.L_80141994:
    li       r0, 0x4
.L_80141998:
    cmpwi    r0, 0x1
    bne      .L_801419A8
    lis      r3, __float_nan@ha
    lfs      f2, __float_nan@l(r3)
.L_801419A8:
    bl       atan2
    lfs      f0, lbl_8065E994@sda21(r0)
    frsp     f1, f1
    fcmpo    cr0, f31, f0
    bge      .L_801419C8
    lfs      f0, lbl_8065E98C@sda21(r0)
    fsubs    f0, f0, f1
    b        .L_801419D0
.L_801419C8:
    lfs      f0, lbl_8065E98C@sda21(r0)
    fsubs    f0, f1, f0
.L_801419D0:
    stfs     f0, 0x94(r1)
    lfs      f3, 0x10(r31)
    lfs      f1, 0x0(r31)
    lfs      f2, 0x18(r31)
    lfs      f0, 0x8(r31)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f1, f1
    lfs      f0, lbl_8065E994@sda21(r0)
    mr       r3, r31
    addi     r4, r1, 0x94
    stfs     f0, 0x9c(r1)
    stfs     f1, 0x98(r1)
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x40(r12)
    mtctr    r12
    bctrl
    lfs      f0, 0x20(r31)
    lis      r0, 0x7f80
    stfs     f0, 0x90(r1)
    lwz      r4, 0x90(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_80141A44
    bge      .L_80141A74
    cmpwi    r3, 0x0
    beq      .L_80141A5C
    b        .L_80141A74
.L_80141A44:
    clrlwi.  r0, r4, 9
    beq      .L_80141A54
    li       r0, 0x1
    b        .L_80141A78
.L_80141A54:
    li       r0, 0x2
    b        .L_80141A78
.L_80141A5C:
    clrlwi.  r0, r4, 9
    beq      .L_80141A6C
    li       r0, 0x5
    b        .L_80141A78
.L_80141A6C:
    li       r0, 0x3
    b        .L_80141A78
.L_80141A74:
    li       r0, 0x4
.L_80141A78:
    cmpwi    r0, 0x1
    beq      .L_80141B50
    lfs      f0, 0x24(r31)
    lis      r0, 0x7f80
    stfs     f0, 0x8c(r1)
    lwz      r4, 0x8c(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_80141AAC
    bge      .L_80141ADC
    cmpwi    r3, 0x0
    beq      .L_80141AC4
    b        .L_80141ADC
.L_80141AAC:
    clrlwi.  r0, r4, 9
    beq      .L_80141ABC
    li       r0, 0x1
    b        .L_80141AE0
.L_80141ABC:
    li       r0, 0x2
    b        .L_80141AE0
.L_80141AC4:
    clrlwi.  r0, r4, 9
    beq      .L_80141AD4
    li       r0, 0x5
    b        .L_80141AE0
.L_80141AD4:
    li       r0, 0x3
    b        .L_80141AE0
.L_80141ADC:
    li       r0, 0x4
.L_80141AE0:
    cmpwi    r0, 0x1
    beq      .L_80141B50
    lfs      f0, 0x28(r31)
    lis      r0, 0x7f80
    stfs     f0, 0x88(r1)
    lwz      r4, 0x88(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_80141B14
    bge      .L_80141B44
    cmpwi    r3, 0x0
    beq      .L_80141B2C
    b        .L_80141B44
.L_80141B14:
    clrlwi.  r0, r4, 9
    beq      .L_80141B24
    li       r0, 0x1
    b        .L_80141B48
.L_80141B24:
    li       r0, 0x2
    b        .L_80141B48
.L_80141B2C:
    clrlwi.  r0, r4, 9
    beq      .L_80141B3C
    li       r0, 0x5
    b        .L_80141B48
.L_80141B3C:
    li       r0, 0x3
    b        .L_80141B48
.L_80141B44:
    li       r0, 0x4
.L_80141B48:
    cmpwi    r0, 0x1
    bne      .L_80141C34
.L_80141B50:
    lfs      f0, 0xf6c(r31)
    lis      r3, sMapCameraStateFormat@ha
    addi     r3, r3, sMapCameraStateFormat@l
    stfd     f0, 0x8(r1)
    lfs      f0, 0xf70(r31)
    stfd     f0, 0x10(r1)
    lfs      f0, 0xf78(r31)
    stfd     f0, 0x18(r1)
    lfs      f0, 0xf7c(r31)
    stfd     f0, 0x20(r1)
    lfs      f0, 0xf80(r31)
    stfd     f0, 0x28(r1)
    lfs      f0, 0xf88(r31)
    stfd     f0, 0x30(r1)
    lfs      f0, 0xf8c(r31)
    stfd     f0, 0x38(r1)
    lfs      f0, 0xf90(r31)
    stfd     f0, 0x40(r1)
    lfs      f0, 0xf98(r31)
    stfd     f0, 0x48(r1)
    lfs      f0, 0xf9c(r31)
    stfd     f0, 0x50(r1)
    lfs      f0, 0xfa0(r31)
    stfd     f0, 0x58(r1)
    lfs      f0, 0xfb8(r31)
    stfd     f0, 0x60(r1)
    lfs      f0, 0xfbc(r31)
    stfd     f0, 0x68(r1)
    lfs      f0, 0xfc0(r31)
    stfd     f0, 0x70(r1)
    lwz      r0, lbl_8065B338@sda21(r0)
    stw      r0, 0x78(r1)
    lfs      f1, 0x0(r31)
    lfs      f2, 0x4(r31)
    lfs      f3, 0x8(r31)
    lfs      f4, 0x20(r31)
    lfs      f5, 0x24(r31)
    lfs      f6, 0x28(r31)
    lfs      f7, 0x30(r31)
    lwz      r4, 0x11d8(r31)
    lwz      r5, 0x11e0(r31)
    lwz      r6, 0xea0(r31)
    lwz      r7, lbl_8065B340@sda21(r0)
    lwz      r8, lbl_8065B33C@sda21(r0)
    lfs      f8, 0xf68(r31)
    lwz      r9, 0xf28(r31)
    lwz      r10, 0xdb4(r31)
    crset    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sPathCcameraactionmap2Cpp@ha
    mr       r6, r3
    addi     r3, r4, sPathCcameraactionmap2Cpp@l
    li       r5, lbl_8065E9C8@sda21
    li       r4, 0xfb
    li       r7, sMapCameraReset2Label@sda21
    crclr    cr1eq
    bl       OSPanic
.L_80141C34:
    psq_l    f31, 0xc8(r1), 0, qr0
    lwz      r0, 0xd4(r1)
    lfd      f31, 0xc0(r1)
    lwz      r31, 0xbc(r1)
    mtlr     r0
    addi     r1, r1, 0xd0
    blr
*/

/**
 * @note Address: 0x80141C50
 * @note Size: 0x178
 */
// Reset__16CCameraActionMapFv

// the original's panic names its function Reset through __FUNCTION__
void CCameraActionMap::Reset()
{
	OSPanic(__FILE__, 220, "%s%s", "actionMap::reset\n", "Reset");

	mTarget = gGame.GetPlayer();
	if (mTarget != NULL) {
		Mtx mtx;
		PSMTXCopy(HSD_JObjGetMtxPtr(mTarget->mJObj), mtx);
		CVec eye(0.0f, 13.0f, -_58);
		PSMTXMultVec(mtx, &eye, &eye);
		SetEye(eye);
		SetInterest(mTarget->GetCameraTarget());
	}
	_DAC = 1;
}

/**
 * @note Address: 0x80141DC8
 * @note Size: 0x4C
 */
void CCameraActionMap::Init()
{
	_5C = 0.5f;
	_58 = 60.0f;
	_80 = 60.0f;
	_84 = 60.0f;
	_C4 = 18.0f;
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
		// retail's format and args don't line up
		OSPanic("ccameraactionmap2.cpp", 181, "%s%s",
		        FormatToStaticBuffer("CCameraActionMap ... (%f, %f, %f), %f, %f, %f, %s\n", displacement.x, displacement.y, displacement.z,
		                             distance, force, velocity, 181),
		        "SpringDamp");
	}
	return position;
}
/*
    stwu     r1, -0x110(r1)
    mflr     r0
    stw      r0, 0x114(r1)
    stfd     f31, 0x100(r1)
    psq_st   f31, 0x108(r1), 0, qr0
    stfd     f30, 0xf0(r1)
    psq_st   f30, 0xf8(r1), 0, qr0
    stfd     f29, 0xe0(r1)
    psq_st   f29, 0xe8(r1), 0, qr0
    stfd     f28, 0xd0(r1)
    psq_st   f28, 0xd8(r1), 0, qr0
    stfd     f27, 0xc0(r1)
    psq_st   f27, 0xc8(r1), 0, qr0
    stmw     r27, 0xac(r1)
    lis      r7, __vt__4CVec@ha
    mr       r29, r5
    addi     r0, r7, __vt__4CVec@l
    lis      r5, lbl_8020C8C0@ha
    stw      r0, 0x9c(r1)
    mr       r27, r3
    fmr      f27, f1
    mr       r28, r4
    stw      r0, 0x8c(r1)
    fmr      f28, f2
    fmr      f29, f3
    mr       r30, r6
    lwz      r6, 0x0(r29)
    fmr      f30, f4
    lwz      r0, 0x4(r29)
    addi     r31, r5, lbl_8020C8C0@l
    addi     r3, r1, 0x64
    addi     r5, r1, 0x74
    stw      r6, 0x74(r1)
    stw      r0, 0x78(r1)
    lwz      r0, 0x8(r29)
    stw      r0, 0x7c(r1)
    bl       fn_800244A0
    lfs      f6, 0x64(r1)
    lis      r3, __vt__4CVec@ha
    lfs      f5, 0x68(r1)
    addi     r0, r3, __vt__4CVec@l
    fmuls    f2, f6, f6
    lfs      f4, 0x6c(r1)
    fmuls    f1, f5, f5
    lfs      f0, lbl_8065E994@sda21(r0)
    fmuls    f3, f4, f4
    stfs     f6, 0x90(r1)
    fadds    f1, f2, f1
    stfs     f5, 0x94(r1)
    stfs     f4, 0x98(r1)
    fadds    f31, f3, f1
    stw      r0, 0x70(r1)
    fcmpo    cr0, f31, f0
    ble      .L_80141F34
    frsqrte  f1, f31
    lfd      f3, lbl_8065E9B0@sda21(r0)
    lfd      f2, lbl_8065E9B8@sda21(r0)
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f31, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f31, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f31, f0, f2
    fmul     f0, f1, f0
    fmul     f31, f31, f0
    frsp     f31, f31
    b        .L_80141FB8
.L_80141F34:
    lfd      f0, lbl_8065E9C0@sda21(r0)
    fcmpo    cr0, f31, f0
    bge      .L_80141F4C
    lis      r3, __float_nan@ha
    lfs      f31, __float_nan@l(r3)
    b        .L_80141FB8
.L_80141F4C:
    stfs     f31, 0x8(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x8(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_80141F74
    bge      .L_80141FA4
    cmpwi    r3, 0x0
    beq      .L_80141F8C
    b        .L_80141FA4
.L_80141F74:
    clrlwi.  r0, r4, 9
    beq      .L_80141F84
    li       r0, 0x1
    b        .L_80141FA8
.L_80141F84:
    li       r0, 0x2
    b        .L_80141FA8
.L_80141F8C:
    clrlwi.  r0, r4, 9
    beq      .L_80141F9C
    li       r0, 0x5
    b        .L_80141FA8
.L_80141F9C:
    li       r0, 0x3
    b        .L_80141FA8
.L_80141FA4:
    li       r0, 0x4
.L_80141FA8:
    cmpwi    r0, 0x1
    bne      .L_80141FB8
    lis      r3, __float_nan@ha
    lfs      f31, __float_nan@l(r3)
.L_80141FB8:
    stfs     f31, 0x18(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x18(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_80141FE0
    bge      .L_80142010
    cmpwi    r3, 0x0
    beq      .L_80141FF8
    b        .L_80142010
.L_80141FE0:
    clrlwi.  r0, r4, 9
    beq      .L_80141FF0
    li       r0, 0x1
    b        .L_80142014
.L_80141FF0:
    li       r0, 0x2
    b        .L_80142014
.L_80141FF8:
    clrlwi.  r0, r4, 9
    beq      .L_80142008
    li       r0, 0x5
    b        .L_80142014
.L_80142008:
    li       r0, 0x3
    b        .L_80142014
.L_80142010:
    li       r0, 0x4
.L_80142014:
    cmpwi    r0, 0x1
    bne      .L_80142054
    lfs      f1, 0x90(r1)
    addi     r3, r31, 0x42c
    lfs      f2, 0x94(r1)
    lfs      f3, 0x98(r1)
    crset    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sMapCameraSpringDampLabel@ha
    mr       r6, r3
    addi     r7, r4, sMapCameraSpringDampLabel@l
    addi     r3, r31, 0x120
    li       r4, 0xa7
    li       r5, lbl_8065E9C8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_80142054:
    lwz      r6, 0x0(r29)
    mr       r4, r30
    lwz      r0, 0x4(r29)
    addi     r3, r1, 0x48
    addi     r5, r1, 0x58
    stw      r6, 0x58(r1)
    stw      r0, 0x5c(r1)
    lwz      r0, 0x8(r29)
    stw      r0, 0x60(r1)
    bl       fn_800244A0
    fmr      f1, f27
    addi     r3, r1, 0x38
    addi     r4, r1, 0x48
    bl       fn_8000F9C0
    lfs      f2, 0x38(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0x3c(r1)
    lfs      f0, 0x40(r1)
    addi     r3, r1, 0x90
    stfs     f2, 0x80(r1)
    addi     r4, r1, 0x80
    stfs     f1, 0x84(r1)
    stfs     f0, 0x88(r1)
    stw      r0, 0x44(r1)
    stw      r0, 0x54(r1)
    bl       PSVECDotProduct
    fdivs    f0, f1, f31
    addi     r3, r1, 0x90
    mr       r4, r3
    fsubs    f1, f30, f31
    fmuls    f0, f29, f0
    fmadds   f28, f28, f1, f0
    bl       PSVECNormalize
    fmuls    f1, f28, f27
    addi     r3, r1, 0x90
    mr       r4, r3
    bl       PSVECScale
    lwz      r7, 0x90(r1)
    mr       r3, r28
    lwz      r6, 0x94(r1)
    mr       r5, r28
    lwz      r0, 0x98(r1)
    addi     r4, r1, 0x2c
    stw      r7, 0x2c(r1)
    stw      r6, 0x30(r1)
    stw      r0, 0x34(r1)
    bl       PSVECAdd
    lfs      f0, 0x0(r28)
    lis      r0, 0x7f80
    stfs     f0, 0x14(r1)
    lwz      r4, 0x14(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_80142140
    bge      .L_80142170
    cmpwi    r3, 0x0
    beq      .L_80142158
    b        .L_80142170
.L_80142140:
    clrlwi.  r0, r4, 9
    beq      .L_80142150
    li       r0, 0x1
    b        .L_80142174
.L_80142150:
    li       r0, 0x2
    b        .L_80142174
.L_80142158:
    clrlwi.  r0, r4, 9
    beq      .L_80142168
    li       r0, 0x5
    b        .L_80142174
.L_80142168:
    li       r0, 0x3
    b        .L_80142174
.L_80142170:
    li       r0, 0x4
.L_80142174:
    cmpwi    r0, 0x1
    beq      .L_8014224C
    lfs      f0, 0x4(r28)
    lis      r0, 0x7f80
    stfs     f0, 0x10(r1)
    lwz      r4, 0x10(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_801421A8
    bge      .L_801421D8
    cmpwi    r3, 0x0
    beq      .L_801421C0
    b        .L_801421D8
.L_801421A8:
    clrlwi.  r0, r4, 9
    beq      .L_801421B8
    li       r0, 0x1
    b        .L_801421DC
.L_801421B8:
    li       r0, 0x2
    b        .L_801421DC
.L_801421C0:
    clrlwi.  r0, r4, 9
    beq      .L_801421D0
    li       r0, 0x5
    b        .L_801421DC
.L_801421D0:
    li       r0, 0x3
    b        .L_801421DC
.L_801421D8:
    li       r0, 0x4
.L_801421DC:
    cmpwi    r0, 0x1
    beq      .L_8014224C
    lfs      f0, 0x8(r28)
    lis      r0, 0x7f80
    stfs     f0, 0xc(r1)
    lwz      r4, 0xc(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_80142210
    bge      .L_80142240
    cmpwi    r3, 0x0
    beq      .L_80142228
    b        .L_80142240
.L_80142210:
    clrlwi.  r0, r4, 9
    beq      .L_80142220
    li       r0, 0x1
    b        .L_80142244
.L_80142220:
    li       r0, 0x2
    b        .L_80142244
.L_80142228:
    clrlwi.  r0, r4, 9
    beq      .L_80142238
    li       r0, 0x5
    b        .L_80142244
.L_80142238:
    li       r0, 0x3
    b        .L_80142244
.L_80142240:
    li       r0, 0x4
.L_80142244:
    cmpwi    r0, 0x1
    bne      .L_801422C4
.L_8014224C:
    lwz      r6, 0x80(r1)
    lis      r3, __vt__4CVec@ha
    lwz      r5, 0x84(r1)
    addi     r0, r3, __vt__4CVec@l
    lwz      r4, 0x88(r1)
    fmr      f4, f31
    fmr      f5, f28
    stw      r6, 0x1c(r1)
    lfs      f1, 0x90(r1)
    addi     r3, r31, 0x454
    stw      r5, 0x20(r1)
    lfs      f2, 0x94(r1)
    stw      r4, 0x24(r1)
    addi     r4, r1, 0x1c
    lfs      f3, 0x98(r1)
    li       r5, 0xb5
    stw      r0, 0x28(r1)
    crset    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sMapCameraSpringDampLabel@ha
    mr       r6, r3
    addi     r7, r4, sMapCameraSpringDampLabel@l
    addi     r3, r31, 0x120
    li       r4, 0xb5
    li       r5, lbl_8065E9C8@sda21
    crclr    cr1eq
    bl       OSPanic
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x28(r1)
.L_801422C4:
    lwz      r5, 0x0(r28)
    lis      r3, __vt__4CVec@ha
    lwz      r4, 0x4(r28)
    addi     r0, r3, __vt__4CVec@l
    stw      r5, 0x0(r27)
    stw      r4, 0x4(r27)
    lwz      r3, 0x8(r28)
    stw      r3, 0x8(r27)
    stw      r0, 0xc(r27)
    stw      r0, 0x8c(r1)
    stw      r0, 0x9c(r1)
    psq_l    f31, 0x108(r1), 0, qr0
    lfd      f31, 0x100(r1)
    psq_l    f30, 0xf8(r1), 0, qr0
    lfd      f30, 0xf0(r1)
    psq_l    f29, 0xe8(r1), 0, qr0
    lfd      f29, 0xe0(r1)
    psq_l    f28, 0xd8(r1), 0, qr0
    lfd      f28, 0xd0(r1)
    psq_l    f27, 0xc8(r1), 0, qr0
    lfd      f27, 0xc0(r1)
    lmw      r27, 0xac(r1)
    lwz      r0, 0x114(r1)
    mtlr     r0
    addi     r1, r1, 0x110
    blr
*/

/**
 * @note Address: 0x8014232C
 * @note Size: 0x10C
 */
// NONMATCHING: the sinit's objects sit behind 0x140 bytes of the unit's other
// bss in retail; this build starts the blob with them.
// __sinit_ccameraactionmap2_cpp
// builds these vecs and registers their dtors. the bss between them is unknown
extern "C" {
CVec lbl_805C02DC;
CVec lbl_805C02F8;
CVec lbl_805C032C;
CVec lbl_805C0348;
CVec lbl_805C0364;
Vec sPreviousMapOrbitAngle;
CVec sMapCameraTestEye;
CVec sMapCameraTestCenter;
CamCollisionPolygon gCameraCollisionPolygons[256];
}

/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lis      r4, __dt__4CVecFv@ha
    lis      r3, lbl_805C0190@ha
    stw      r0, 0x14(r1)
    lis      r5, __vt__4CVec@ha
    addi     r0, r5, __vt__4CVec@l
    addi     r4, r4, __dt__4CVecFv@l
    stw      r31, 0xc(r1)
    addi     r31, r3, lbl_805C0190@l
    addi     r3, r31, 0x14c
    stw      r0, 0xc(r3)
    addi     r5, r31, 0x140
    bl       __register_global_object
    lis      r4, __vt__4CVec@ha
    addi     r3, r31, 0x168
    addi     r0, r4, __vt__4CVec@l
    addi     r5, r31, 0x15c
    lis      r4, __dt__4CVecFv@ha
    stw      r0, 0xc(r3)
    addi     r4, r4, __dt__4CVecFv@l
    bl       __register_global_object
    lis      r4, __vt__4CVec@ha
    addi     r3, r31, 0x19c
    addi     r0, r4, __vt__4CVec@l
    addi     r5, r31, 0x190
    lis      r4, __dt__4CVecFv@ha
    stw      r0, 0xc(r3)
    addi     r4, r4, __dt__4CVecFv@l
    bl       __register_global_object
    lis      r4, __vt__4CVec@ha
    addi     r3, r31, 0x1b8
    addi     r0, r4, __vt__4CVec@l
    addi     r5, r31, 0x1ac
    lis      r4, __dt__4CVecFv@ha
    stw      r0, 0xc(r3)
    addi     r4, r4, __dt__4CVecFv@l
    bl       __register_global_object
    lis      r4, __vt__4CVec@ha
    addi     r3, r31, 0x1d4
    addi     r0, r4, __vt__4CVec@l
    addi     r5, r31, 0x1c8
    lis      r4, __dt__4CVecFv@ha
    stw      r0, 0xc(r3)
    addi     r4, r4, __dt__4CVecFv@l
    bl       __register_global_object
    lis      r4, __vt__4CVec@ha
    addi     r3, r31, 0x1fc
    addi     r0, r4, __vt__4CVec@l
    addi     r5, r31, 0x1f0
    lis      r4, __dt__4CVecFv@ha
    stw      r0, 0xc(r3)
    addi     r4, r4, __dt__4CVecFv@l
    bl       __register_global_object
    lis      r4, __vt__4CVec@ha
    addi     r3, r31, 0x218
    addi     r0, r4, __vt__4CVec@l
    addi     r5, r31, 0x20c
    lis      r4, __dt__4CVecFv@ha
    stw      r0, 0xc(r3)
    addi     r4, r4, __dt__4CVecFv@l
    bl       __register_global_object
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: 0x80139E40
 * @note Size: 0x8
 */
u32 CCameraActionMap::GetCameraFlags()
{
	return 11;
}

/**
 * @note Address: 0x80139E48
 * @note Size: 0x8
 */
extern "C" s32 vf5C__16CCameraActionMapFv(CCameraActionMap* map)
{
	return map->_40;
}

/**
 * @note Address: 0x80139E50
 * @note Size: 0x8
 */
f32 CCameraActionMap::GetFollowDivisor()
{
	return 6.0f;
}

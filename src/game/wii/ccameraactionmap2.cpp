#include "nw4r/math/math_types.h"
#include "game/ccameraaction.h"
#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#endif
#include "game/cameracollisionpolygons.h"
#include "game/caminfo.h"
#include "game/stagedata.h"
#include "game/gmath.h"
#include "game/ut.h"
#include "game/cvec.h"
#include <string.h>
#include <math.h>
#include <cmath>
extern "C" {
#include <dolphin/os.h>
}

// the map camera's half of the Wii camera code; the area camera's half is ccameraactionarea.cpp

extern "C" void DrawMapCameraAreaDebug3D(CCameraActionAreaEx* state, s32 mode);

extern "C" {
void UpdateCameraAreaIndexAndBaseFov(CamAreaState* state, s32 unused);
void StartMapCameraTargetHeightInterpolation(CCameraActionMap* map, f32 from, f32 to, f32 time);
void InitCameraAreaEntries(CamAreaState* state);
void SelectAreaCameraEntryByViewDirection(CCameraActionArea* area, s32 mode, f32 time, s32 slotIndex);
void SelectAreaCameraEntryWithControl(CCameraActionArea* area, s32 slotIndex, s32 entryIndex, s32 mode, s32 control, f32 time);
void SetMapCameraModeFlags(CCameraActionMap* map, u32 flags);
f32 MapAreaHeight(CCameraActionMap* map, s32 index);
}

// the area index the last turn interpolation aims at, Update skips starting another for it
extern s32 lbl_805A1D10;

// the Wii build resolves the current area's action in a second, C++ step that falls back to
// the map camera while the player stands outside every area
s32 GetCameraAreaMode(CamAreaState* state);

extern s32 gScreenSafeWidth;

/**
 * @note Address: 0x80157154
 * @note Size: 0x134
 */
void CCameraActionMap::Init()
{
	mSpringDamping      = 0.5f;
	mRestDistance       = 60.0f;
	mTargetRestDistance = 60.0f;
	mFlatDistance       = 60.0f;
	_C4                 = 18.0f;
	InitCameraAreaEntries(&gCameraAreaState);

	// the turn arrows, as the area camera's
	nw4r::math::_VEC2 pos[2] = { 0 };
	pos[0].x                 = gScreenSafeWidth / 2 - 30;
	pos[1].x                 = -(gScreenSafeWidth / 2 - 30);
	f32 rot[2]               = { 0.0f, PI };
	mArrows[0].mPos.x        = pos[0].x;
	mArrows[0].mPos.y        = pos[0].y;
	mArrows[0].mPos.z        = rot[0];
	mArrows[0].mShown        = 0;
	mArrows[0]._10           = 0.0f;
	mArrows[0]._14           = 0.0f;
	mArrows[1].mPos.x        = pos[1].x;
	mArrows[1].mPos.y        = pos[1].y;
	mArrows[1].mPos.z        = rot[1];
	mArrows[1].mShown        = 0;
	mArrows[1]._10           = 0.0f;
	mArrows[1]._14           = 0.0f;
	_126C                    = 0.0f;
	_1270                    = 0.0f;
	_127C                    = 0.0f;
	_1280                    = 0.0f;
	_1274                    = 1.0f;
	_1278                    = 1.0f;
	_1284                    = 0.0f;
	_1288                    = 3.0f;
}

extern s32 gScreenSafeRight;
extern s32 gScreenSafeTop;
extern "C" void fn_8016AA74(const f32* params);

void CCameraActionMap::Reset()
{
	OSPanic(__FILE__, 285, "actionMap::reset\n");
	// OSPanic ends in PPCHalt, so nothing below ever runs

	mTarget = (XObj*)gGame.GetPlayer();
	if (mTarget != NULL) {
		Mtx mtx;
		PSMTXCopy(mTarget->GetWorldMtx(), mtx);
		CVec eye(0.0f, 13.0f, -mRestDistance);
		PSMTXMultVec(mtx, &eye, &eye);
		SetEye(eye);
		SetInterest(mTarget->GetCameraTarget());
	}
	mPadControlEnabled = 1;
}

extern "C" void SetMapCameraKeepPreviousEye(CCameraActionMap* map)
{
	map->mKeepPreviousEye = TRUE;
}

extern "C" s32 FindEnabledCameraAreaContainingPoint(CamAreaState* state, Vec pos);
extern "C" s32 SelectMapCameraArea(CCameraActionMap* map);
extern "C" void StartMapCameraAreaHeightInterpolation(CCameraActionMap* map);

extern "C" void RefreshMapCameraArea(CCameraActionMap* map)
{
	map->mPrevMapArea = map->mMapArea;
	SelectMapCameraArea(map);
	StartMapCameraAreaHeightInterpolation(map);
	map->mPrevMapArea = map->mMapArea;
}

extern "C" void StartMapCameraTargetHeightInterpolation(CCameraActionMap* map, f32 from, f32 to, f32 time)
{
	if (0.0f != time) {
		map->mTargetHeightOffsetInterp = &map->mTargetHeightOffsetEase;
		map->mTargetHeightOffsetInterp->Start(from, to, time);
	}
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

// the outward normal of an area side from a to b, turned about the up or the down axis; the
// GameCube inlines it into both intersection collectors
static void GetMapAreaEdgeNormal(const Vec& a, const Vec& b, Vec* out, BOOL up)
{
	Vec axis;
	if (up) {
		Vec upAxis = { 0.0f, 1.0f, 0.0f };
		axis       = upAxis;
	} else {
		Vec downAxis = { 0.0f, -1.0f, 0.0f };
		axis         = downAxis;
	}
	Vec edge;
	edge.x = a.x - b.x;
	edge.z = a.z - b.z;
	edge.y = 0.0f;
	CVec normal;
	C_VECCrossProduct(&axis, &edge, &normal);
	PSVECNormalize(&normal, &normal);
	*out = normal;
}

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
	const s32(*corner)[2];
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
		corner = record->_04;
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
				Vec normal;
				GetMapAreaEdgeNormal(a, b, &normal, FALSE);
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
 * @note Address: 0x8015D6B0
 * @note Size: 0x278
 */
extern "C" BOOL SetMapCameraSideEndpoints(CCameraActionMap* map)
{
	CamMapEntry* entry = map->mNearestEyeEntry;
	if (entry != NULL) {
		s32 group          = entry->mRecordIndex;
		s32 side           = entry->mSide;
		CVec dir           = entry->mNormal;
		CamAreaRecord* rec = gCameraAreaState.mAreaExRecords;
		rec += group;
		CVec toPlayer = *gGame.GetPlayer()->GetPos();
		toPlayer.y    = 0.0f;
		Vec eye       = map->mEye;
		eye.y         = 0.0f;
		toPlayer -= eye;
		PSVECNormalize(&toPlayer, &toPlayer);
		PSVECCrossProduct(&dir, &toPlayer, &dir);
		CVec cross = dir;

		s32* corner = &rec->_04[0][0];
		corner += side;
		corner += side;
		f32 x0 = corner[0];
		x0 /= 100.0f;
		f32 z0 = corner[1];
		z0 /= 100.0f;
		if (side == 3) {
			corner = rec->_04[0];
		} else {
			corner += 2;
		}
		f32 x1 = corner[0];
		x1 /= 100.0f;
		f32 z1 = corner[1];
		z1 /= 100.0f;

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

// NONMATCHING: retail reads a debug flag (sbss 0x805A1D08) after storing the speed and reloads it, and keeps
// from and to in each other's registers (88.9 %)
extern "C" BOOL TrySetMapCameraTargetYaw(CCameraActionMap* map, f32 yaw, f32 time, f32 unused)
{
	f32 oldYaw   = map->mTargetYaw;
	f32 oldSpeed = map->mTurnSpeed;

	map->mTargetYaw = WrapAngle(yaw);
	f32 from        = WrapAngle(map->mAngle.y);
	f32 to          = WrapAngle(yaw);

	f32 diff = __fabsf(to - from);
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

	map->mTurnSpeed = __fabsf(to - from) / time;
	if (map->mTurnSpeed < (2.0f * DEG2RAD)) {
		map->mTargetYaw = oldYaw;
		map->mTurnSpeed = oldSpeed;
		return FALSE;
	}
	return TRUE;
}

extern "C" s32 FindNearestMapCameraEntry(CCameraActionMap* map, const Vec& pos)
{
	if (map->mIntersectionCount > 1) {
		CVec p(pos.x, pos.y, pos.z);
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

// NONMATCHING: map and the eye's area sit in other saved registers (96.9 %)
extern "C" s32 SelectMapCameraArea(CCameraActionMap* map)
{
	CVec target = *map->mTarget->GetPos();
	CVec eye    = map->mEye;
	eye.y       = target.y;

	map->mAreaIntersectionCount = CollectMapCameraAreaIntersections(map, eye, target, map->mAreaIntersections, 1);

	s32 eyeArea = FindEnabledCameraAreaContainingPoint(&gCameraAreaState, eye);
	if (CamAreaInfo(&gCameraAreaState.mAreaRecords[eyeArea]).UsesAreaCamera()) {
		eyeArea = -1;
	}

	s32 targetArea = FindEnabledCameraAreaContainingPoint(&gCameraAreaState, target);
	if (CamAreaInfo(&gCameraAreaState.mAreaRecords[targetArea]).UsesAreaCamera()) {
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
 * @note Address: 0x8015FB20
 * @note Size: 0x1D4
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
		lbl_805A1D10 = map->mNearestTargetEntry->mRecordIndex;
		SetMapCameraModeFlags(map, 0x20004);
	}
}

extern "C" f32 GetMapCameraHorizontalTargetDistance(CCameraActionMap* map)
{
	CVec pos = *map->mTarget->GetPos();
	f32 y    = map->mTarget->GetCameraTarget().y;
	pos.y    = y;
	CVec eye = map->mEye;
	eye.y    = y;
	return PSVECDistance(&pos, &eye);
}

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

extern "C" void SetMapCameraFovStep(CCameraActionMap* map, s32 zoomLevel, f32 time)
{
	SetFovStep(map, zoomLevel, time);
}

/**
 * @note Address: 0x8015F8A0
 * @note Size: 0x29C
 */
extern "C" void StartMapCameraAreaHeightInterpolation(CCameraActionMap* map)
{
	CamAreaInfo area = CamAreaInfo(&gCameraAreaState.mAreaRecords[map->mMapArea]);
	if (map->mMapArea >= 0) {
		// flag 1: the area's height is absolute, otherwise it is above the player's camera target
		if (!(area.mRecord->mFlags & 1)) {
			map->mHeightMode = 0;
		} else {
			map->mHeightMode = 1;
		}
		// flag 2: keep the area's height once no area is picked (mMapArea -1), otherwise fall back to 13
		if (!(area.mRecord->mFlags & 2)) {
			map->mUseAreaHeight = 0;
		} else {
			map->mUseAreaHeight = 1;
		}
		if (map->mHeightMode == 0) {
			f32 height         = MapAreaHeight(map, map->mMapArea);
			f32 from           = map->mEye.y - (gGame.GetPlayer())->GetCameraTarget().y - map->mEyeYOffset;
			map->mHeightInterp = &map->mHeightEaseInOut;
			map->mHeightInterp->Start(from, height, 1.0f);
			if (map->mUseAreaHeight == 0) {
				map->mTargetHeight = 13.0f;
			}
			if (map->mUseAreaHeight == 1) {
				map->mTargetHeight = height;
			}
		}
		if (map->mHeightMode == 1) {
			f32 height            = MapAreaHeight(map, map->mMapArea);
			f32 from              = map->mEye.y - map->mEyeYOffset;
			map->mAbsHeightInterp = &map->mAbsHeightEaseInOut;
			map->mAbsHeightInterp->Start(from, height, 1.0f);
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
			f32 from           = map->mEye.y - (gGame.GetPlayer())->GetCameraTarget().y - map->mEyeYOffset;
			map->mHeightInterp = &map->mHeightEaseInOut;
			map->mHeightInterp->Start(from, map->mTargetHeight, 1.0f);
		}
		if (map->mHeightMode == 1 && map->mUseAreaHeight == 1) {
			f32 from              = map->mEye.y - map->mEyeYOffset;
			map->mAbsHeightInterp = &map->mAbsHeightEaseInOut;
			map->mAbsHeightInterp->Start(from, map->mTargetHeight, 1.0f);
		}
	}
}

/**
 * @note Address: 0x8015EE1C
 * @note Size: 0x190
 */
extern "C" f32 MapAreaHeight(CCameraActionMap* map, s32 index)
{
	CamAreaRecord* records = GetStageCameraAreaRecords();
	CamAreaInfo area       = CamAreaInfo(&records[index]);
	if (area.mRecord->mCameraAction == -1) {
		return 0.23f * CamInfo(&gCameraAreaState.mCameraInfo[gCameraAreaState.mGroups[index].mEntries->mId]).GetEye().y;
	}
	return CamInfo(&gCameraAreaState.mCameraInfo[gCameraAreaState.mGroups[index].mEntries->mId]).GetEye().y;
}

// the point 13 above and 1 behind the target's origin, in world space; GetMapCameraTargetOffsetPosition is its
// out-of-line copy, GetMapCameraEyeBehindTarget inlines it
static inline BOOL CalcMapCameraTargetOffsetPosition(CCameraActionMap* map, Vec& out)
{
	XObj* target = map->mTarget;
	if (target != NULL && (target->mStatus.mFlags & 1)) {
		Mtx mtx;
		PSMTXCopy(target->GetWorldMtx(), mtx);
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
 * @note Address: 0x8015F460
 * @note Size: 0x264
 */
extern "C" void GetMapCameraEyeBehindTarget(CCameraActionMap* map, Vec& out, s32 mode)
{
	CVec result;
	if (mode != 1) {
		if (map->mTarget != NULL) {
			Mtx mtx;
			PSMTXCopy(map->mTarget->GetWorldMtx(), mtx);
			CVec pos(0.0f, 13.0f, -map->mTargetRestDistance);
			PSMTXMultVec(mtx, &pos, &pos);
			pos.y  = map->mHeightInterp->Get();
			result = pos;
		}
	} else if (map->mTarget != NULL) {
		Vec offset;
		CalcMapCameraTargetOffsetPosition(map, offset);
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		CVec eye                     = camera->mView.mEye;
		CVec interest                = camera->mView.mInterest;
		f32 angle                    = HALF_PI + (f32)atan2(interest.x - eye.x, interest.z - eye.z);
		f32 distance                 = map->mTargetRestDistance;
		result.x                     = offset.x + distance * (f32)cos(angle);
		result.z                     = offset.z + distance * -(f32)sin(angle);
		result.y                     = 13.0f + map->mTarget->GetCameraTarget().y;
	}
	out.x = result.x;
	out.y = result.y;
	out.z = result.z;
}

void CCameraActionMap::PreviousUpdate()
{
	UpdateCameraAreaIndexAndBaseFov(&gCameraAreaState, 0);
	s32 activeArea = gCameraAreaState.mActiveArea;
	// the player stepped into another camera area
	if (gCameraAreaState.mCurrentArea != gCameraAreaState.mPrevArea && gCameraAreaState.mCurrentArea != -1) {
		if (GetCameraAreaMode(&gCameraAreaState) == CAMACTION_AREA) {
			gGame.mMotionControlCamera.ChangeAction(CAMACTION_AREA);
			gCameraAreaState.mPrevZoomLevel = gCameraAreaState.mZoomLevel;
			SelectAreaCameraEntryByViewDirection((CCameraActionArea*)gGame.mCameraActions.mTable[CAMACTION_AREA], 1, 2.0f, activeArea);
			return;
		}
		// otherwise the mode is CAMACTION_MAP and this action stays
		SetFovStep(this, gCameraAreaState.mZoomLevel, 15.0f);
		SelectAreaCameraEntryWithControl((CCameraActionArea*)gGame.mCameraActions.mTable[CAMACTION_AREA], activeArea, 0, 3, 1, 1.0f);
	}
	mPrevMapArea = mMapArea;
	SelectMapCameraArea(this);
	if (mMapArea != mPrevMapArea) {
		StartMapCameraAreaHeightInterpolation(this);
	}
}

f32 CCameraActionMap::GetFollowDivisor()
{
	// CMotionControlCamera::Follow closes 1/6 of the remaining distance per call
	return 6.0f;
}

u32 CCameraActionMap::GetInterestMode()
{
	return mInterestMode;
}

u32 CCameraActionMap::GetCameraFlags()
{
	return CAMFLAG_EYE | CAMFLAG_INTEREST | CAMFLAG_FOV;
}

// set by ResetCenter; the spring update starts over from the new center
s32 sMapCameraCenterReset;
// the area index the last turn interpolation aims at, Update skips starting another for it
s32 lbl_805A1D10;
// 1: a new orbit keeps the target rest distance instead of the eye's current one
s32 sMapCameraOrbitAtRestDistance;

static inline f32 FlatDistance(const CVec& pos, CVec& v)
{
	CVec flat(pos.x, v.y, pos.z);
	return PSVECDistance(&flat, &v);
}

// the orbit starts from where the camera is
static inline void ResetMapCameraTargetEye(CCameraActionMap* map)
{
	map->mOrbitEye       = map->mEye;
	map->mPrevOrbitEye   = map->mEye;
	map->mOrbitAngle     = map->mAngle;
	map->mPrevOrbitAngle = map->mAngle;
	map->mPrevTargetEye  = map->mEye;
	map->mTargetEye      = map->mPrevTargetEye;
	map->mEyeInterp      = NULL;

	map->mFlatDistance = FlatDistance(map->mTargetEye, map->mInterest);
	FlatDistance(map->mTargetEye, map->mSpringTarget); // retail computes and drops it (bl PSVECDistance at 0x80160350)
}

static inline void SetCenter(CCameraActionMap* map, const Vec& pos)
{
	map->mSpringTarget     = pos;
	map->mSpringPrevTarget = map->mSpringTarget;
}

static inline void ResetCenter(CCameraActionMap* map)
{
	Vec* center = &map->mSpringTarget;
	SetCenter(map, *center);
	CVec eye              = map->mEye;
	map->mRestDistance    = PSVECDistance(&eye, center);
	sMapCameraCenterReset = 1;
	map->mStatus.mFlags &= ~0x700;
	if (map->mEyeInterp != NULL) { // retail tests before clearing (0x801604C4)
		map->mEyeInterp = NULL;
	}
}

/**
 * @note Address: 0x80160220
 * @note Size: 0x2D4
 */
// NONMATCHING: retail tests mask & flags, reads FlatDistance's z before x and copies the spring
// center through a stack temporary before storing it back (91 %)
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
			if (sMapCameraOrbitAtRestDistance == 1) {
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

// the probe the last unobstructed-yaw test cast, kept for the debug draw
extern "C" CVec sMapCameraTestEye;
extern "C" CVec sMapCameraTestCenter;

/**
 * @note Address: 0x80160520
 * @note Size: 0x268
 */
// NONMATCHING: retail builds the two probe CVecs in word-copied stack temporaries before storing
// them, and map and the vtable sit in swapped saved registers (83 %)
extern "C" BOOL TrySetUnobstructedMapCameraYaw(CCameraActionMap* map, f32 yaw)
{
	Vec center = *map->mTarget->GetPos();
	CVec angles(map->mAngle.x, yaw, map->mAngle.z);
	f32 angle = HALF_PI + angles.y;
	f32 pitch = angles.x;
	Vec pos;
	f32 distance;
	{
		CVec target(map->mTarget->GetCameraTarget());
		CVec eye(map->mEye);
		distance = PSVECDistance(&target, &eye);
	}
	pos.x                   = center.x + distance * std::cosf(angle) * std::cosf(pitch);
	pos.z                   = center.z + distance * -std::sinf(angle) * std::cosf(pitch);
	pos.y                   = center.y;
	sMapCameraTestEye       = CVec(pos);
	sMapCameraTestCenter    = CVec(center);
	map->mIntersectionCount = map->CalcIntersectVec(pos, center, map->mIntersectionEntries, 0);
	if (map->mIntersectionCount == 0 && TrySetMapCameraTargetYaw(map, yaw, 25.0f, 0.0f)) {
		SetMapCameraModeFlags(map, 0x20002);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8015F0DC
 * @note Size: 0x270
 */
void CCameraActionMap::DrawDebug2d()
{
	static const f32 sDebugParams[4] = { 1.1f, 0.9f, 1.0f, 0.2f };
	fn_8016AA74(sDebugParams);
	f32 z = 0.0f;
	f32 y, x;
	x = gScreenSafeRight - 171;
	y = gScreenSafeTop + 18;
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B000000A0AREA(%3d,---)(%3d)\n", x, y, z, 9.0f, 9.0f, gCameraAreaState.mActiveArea,
	                       gCameraAreaState.mCurrentArea);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B000000A0ACMAP2\n", x, y + 9.0f, z, 9.0f, 9.0f);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B00000080CAMSTAT\t   :%8X\nCAMSTATPRE :%8X\n", x - 36.0f, y + 18.0f, z, 9.0f, 9.0f,
	                       mStatus.mFlags, mPreviousStatus.mFlags);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B00000080CHR_InsideCll(%3d,%3d,%3d)\n", x - 63.0f, y + 36.0f, z, 9.0f, 9.0f, mInsideAreas[0],
	                       mInsideAreas[1], mInsideAreas[2]);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B00000080CAM_InsideCll(%3d,%3d,%3d)\n", x - 63.0f, y + 45.0f, z, 9.0f, 9.0f, mInsideCll0,
	                       mInsideCll1, mInsideCll2);
	if (mInsideCllCount > 0) {
		DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B00000080CamHitCll(%3d,%3d,%3d)\n", x - 63.0f, y + 54.0f, z, 9.0f, 9.0f, mInsideCll0,
		                       mInsideCll1, mInsideCll2);
	}
	if (mNearestTargetEntry != NULL) {
		DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B00000080INTRSCT(%3d-%3d)\n", x - 63.0f, y + 63.0f, z, 9.0f, 9.0f,
		                       mNearestTargetEntry->mRecordIndex, mNearestTargetEntry->mSide);
	}
}

void CCameraActionMap::DrawDebug3d()
{
	DrawMapCameraAreaDebug3D(&gCameraAreaState, 1);
}

// the file's statics, in the bss layout retail has: five CVecs nothing references, the orbit's
// previous angle, one more unreferenced CVec and the two probes the unobstructed-yaw test keeps
extern "C" {
CVec lbl_8047F0C8;
CVec lbl_8047F0E8;
CVec lbl_8047F108;
CVec lbl_8047F128;
CVec lbl_8047F148;
Vec sPreviousMapOrbitAngle;
CVec lbl_8047F170(0.0f, 10.0f, 0.0f);
CVec sMapCameraTestEye;
CVec sMapCameraTestCenter;
CamCollisionPolygon gCameraCollisionPolygons[256];
}

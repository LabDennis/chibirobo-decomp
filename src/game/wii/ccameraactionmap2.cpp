#include "nw4r/math/math_types.h"
#include "game/ccameraaction.h"
#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#endif
#include "game/gmath.h"
#include "game/ut.h"
#include <string.h>
#include <math.h>
#include <cmath>
extern "C" {
#include <dolphin/os.h>
}

extern "C" void DrawMapCameraAreaDebug3D(CCameraActionAreaEx* state, s32 mode);

// the Wii build merged the area and map camera code into this file; the retail
// rodata carries "ccameraactionmap2.cpp" as the __FILE__ string for both halves.
// CamAreaSlot's constructor is the file's first emitted function.

/**
 * @note Address: 0x80154310
 * @note Size: 0x10
 */
CamAreaSlot::CamAreaSlot()
{
}

// the area camera's half

u32 CCameraActionArea::GetCameraFlags()
{
	return CAMFLAG_EYE | CAMFLAG_INTEREST | CAMFLAG_FOV;
}

u32 CCameraActionArea::GetInterestMode()
{
	return 2; // CMotionControlCamera eases its interest towards ours with Follow instead of taking it at once
}

XObj* CCameraActionArea::GetTarget()
{
	return mTarget;
}

extern "C" {
void UpdateCameraAreaIndexAndBaseFov(CamAreaState* state, s32 unused);
void StartMapCameraTargetHeightInterpolation(CCameraActionMap* map, f32 from, f32 to, f32 time);
void RefreshMapCameraArea(CCameraActionMap* map);
void SetMapCameraFovStep(CCameraActionMap* map, s32 zoomLevel, f32 time);
void InitCameraAreaEntries(CamAreaState* state);
void SetCameraAreaIndexAndBaseFov(CamAreaState* state, s32 index);
void ApplyAreaCameraEntry(CCameraActionArea* area, CamAreaEntry* entry, s32 mode, s32 force, f32 time);
}

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

/**
 * @note Address: 0x80154320
 * @note Size: 0x154
 */
void CCameraActionArea::Init()
{
	InitCameraAreaEntries(&gCameraAreaState);
	// by interpolation mode; mode 3 or a time <= 0 sets the value at once, and the eye and fov take mode 5 as 4
	mEyeInterps[0]      = &mEyeLinear;
	mEyeInterps[1]      = &mEyeEaseInOut;
	mEyeInterps[2]      = &mEyeEaseOut;
	mEyeInterps[3]      = NULL;
	mEyeInterps[4]      = &mEyeEaseOutDivide;
	mFovInterps[0]      = &mFovLinear;
	mFovInterps[1]      = &mFovEaseInOut;
	mFovInterps[2]      = &mFovEaseOut;
	mFovInterps[3]      = NULL;
	mFovInterps[4]      = &mFovEaseOutDivide;
	mInterestInterps[0] = NULL;
	mInterestInterps[1] = &mInterestEaseInOut;
	mInterestInterps[2] = NULL;
	mInterestInterps[3] = NULL;
	mInterestInterps[4] = NULL;

	// the turn arrows sit 30 pixels in from the safe area's edges, the right one turned round
	nw4r::math::_VEC2 pos[2] = { 0 };
	pos[0].x                 = gScreenSafeWidth / 2 - 30;
	pos[1].x                 = -(gScreenSafeWidth / 2 - 30);
	f32 rot[2]               = { 0.0f, PI };
	mArrows[0].mPos.x        = pos[0].x;
	mArrows[0].mPos.y        = pos[0].y;
	mArrows[0].mPos.z        = rot[0];
	mArrows[0].mShown        = 0;
	mArrows[0]._0D           = 0;
	mArrows[0]._10           = 0.0f;
	mArrows[0]._14           = 0.0f;
	mArrows[1].mPos.x        = pos[1].x;
	mArrows[1].mPos.y        = pos[1].y;
	mArrows[1].mPos.z        = rot[1];
	mArrows[1].mShown        = 0;
	mArrows[1]._0D           = 0;
	mArrows[1]._10           = 0.0f;
	mArrows[1]._14           = 0.0f;
}

extern s32 gScreenSafeRight;
extern s32 gScreenSafeTop;
extern "C" void fn_8016AA74(const f32* params);

void CCameraActionArea::PreviousUpdate()
{
	if (mControlEnabled == 1) {
		UpdateCameraAreaIndexAndBaseFov(&gCameraAreaState, 0);
	}
	// entering an area run by the map camera hands the camera over to it
	if (gCameraAreaState.mCurrentArea != gCameraAreaState.mPrevArea && gCameraAreaState.mCurrentArea != -1
	    && GetCameraAreaMode(&gCameraAreaState) == CAMACTION_MAP) {
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_MAP);
		gCameraAreaState.mPrevZoomLevel = gCameraAreaState.mZoomLevel;
		SetMapCameraFovStep((CCameraActionMap*)gGame.mCameraActions.mTable[CAMACTION_MAP], gCameraAreaState.mZoomLevel, 15.0f);
		RefreshMapCameraArea((CCameraActionMap*)gGame.mCameraActions.mTable[CAMACTION_MAP]);
		f32 dy = gGame.mMotionControlCamera.mView.mInterest.y - mTarget->GetCameraTarget().y;
		if (0.0f != dy) {
			StartMapCameraTargetHeightInterpolation((CCameraActionMap*)gGame.mCameraActions.mTable[CAMACTION_MAP], dy, 0.0f, 8.0f);
		}
	}
}

void CCameraActionArea::Reset()
{
	LoadStageCameraAreaState(&gCameraAreaState);
	mControlEnabled     = 1;
	mPadControlEnabled  = TRUE;
	mEntryIndex         = 0;
	CamAreaEntry* entry = gCameraAreaState.mGroups[0].mEntries;
	mEntry              = entry;
	mPrevEntry          = entry;
	mTarget             = (XObj*)gGame.GetPlayer();
	mKeepViewDirection  = 0;
	mForceApply         = FALSE;
	_2D0                = 0;
	_2D1                = 0;
	_2D2                = 0;
	_2E8                = 0;
	_2EC                = 0;
	_2F8                = 0;
	mArrows[0]._14      = 0.0f;
	mArrows[1]._14      = 0.0f;
}

void CCameraActionArea::SetCamera(CCamera* camera)
{
	SetEye(camera->mView.mEye);
	SetInterest(camera->mView.mInterest);
	SetDistance(camera->mView.mDistance);
	SetFov(camera->mFov);
	mKeepViewDirection = 1;
	Vec zero           = { 0 };
	_2D4               = zero;
	_2CC               = 0.0f;
	_2E8               = 0;
	_2EC               = 0;
	_2F8               = 0;
	mArrows[0]._14     = 0.0f;
	mArrows[1]._14     = 0.0f;
}

// the GameCube's SelectAreaCameraEntry, which the Wii expands here
static inline void SelectAreaCameraEntry(CCameraActionArea* area, s32 slotIndex, s32 entryIndex, s32 mode, f32 time)
{
	CamAreaEntry* entry   = &gCameraAreaState.mGroups[slotIndex].mEntries[entryIndex];
	area->mEntry          = entry;
	area->mPrevEntry      = entry;
	area->mEntryIndex     = entryIndex;
	area->mPrevEntryIndex = entryIndex;
	SetCameraAreaIndexAndBaseFov(&gCameraAreaState, slotIndex);
	ApplyAreaCameraEntry(area, entry, mode, 1, time);
}

// the yaw of angle as a unit direction in the xz plane; the Wii MSL's float sinf/cosf wrappers
// (cos's float argument goes through the wrapper's parameter, the fmr before `bl cos`)
inline void YawDir(CVec& dir, const Vec& angle)
{
	CVec ang(angle);
	f32 x = std::sinf(ang.y);
	f32 z = std::cosf(ang.y);
	dir.x = x;
	dir.y = 0.0f;
	dir.z = z;
	PSVECNormalize(&dir, &dir);
}

// the entry of the slot that faces most along dir, the first when none does; the GameCube inlines it
// NONMATCHING: the dot product's paired-single registers differ (99.4 %)
s32 CCameraActionArea::FindFacing(s32 slotIndex, const Vec& dir)
{
	CVec look(dir.x, dir.y, dir.z);
	CamAreaSlot* slot   = &gCameraAreaState.mGroups[slotIndex];
	f32 best            = -1.0f;
	s32 bestIndex       = -1;
	CamAreaEntry* entry = slot->mEntries;
	for (s32 i = 0; i < slot->mEntryCount; entry++, i++) {
		CVec facing;
		YawDir(facing, entry->mAngle);
		f32 dot = nw4r::math::VEC3Dot((nw4r::math::VEC3*)&look, (nw4r::math::VEC3*)&facing);
		if (dot > best) {
			bestIndex = i;
			best      = dot;
		}
	}
	if (best == -1.0f) {
		bestIndex = 0;
	}
	return bestIndex;
}

extern "C" void SelectAreaCameraEntryByViewDirection(CCameraActionArea* area, s32 mode, f32 time, s32 slotIndex)
{
	area->mControlEnabled = 1;
	CVec view             = area->mInterest - area->mEye;
	PSVECNormalize(&view, &view);
	SelectAreaCameraEntry(area, slotIndex, area->FindFacing(slotIndex, view), mode, time);
}

// the Wii-only variant: picks the entry facing the current view in the active area's group
// NONMATCHING: the view and the eye copy swap stack slots (99.9 %); retail expands
// SelectAreaCameraEntryByViewDirection here, which this merged unit's -inline noauto does not
extern "C" void SelectActiveAreaCameraEntryByViewDirection(CCameraActionArea* area, s32 mode, f32 time)
{
	s32 slotIndex         = gCameraAreaState.mActiveArea;
	area->mControlEnabled = 1;
	CVec view             = area->mInterest - area->mEye;
	PSVECNormalize(&view, &view);
	SelectAreaCameraEntry(area, slotIndex, area->FindFacing(slotIndex, view), mode, time);
}

// NONMATCHING: retail copies both Vecs once more on the stack before SetInte_I (95.2 %)
extern "C" void SetAreaCameraInterestTarget(CCameraActionArea* area, s32 mode, XObj* target, f32 time)
{
	if (target != NULL) {
		area->mTarget = target;
		Vec interest  = area->mTarget->GetCameraTarget();
		Vec current   = area->mInterest;
		area->SetInte_I(mode, current, interest, time);
	}
}

extern "C" void SelectAreaCameraEntryWithControl(CCameraActionArea* area, s32 slotIndex, s32 entryIndex, s32 mode, s32 control, f32 time)
{
	area->mControlEnabled = control;
	SelectAreaCameraEntry(area, slotIndex, entryIndex, mode, time);
}

extern "C" void SetAreaCameraHeightIndices(CCameraActionArea*, s32 zoomLevel)
{
	gCameraAreaState.mZoomLevel     = zoomLevel;
	gCameraAreaState.mPrevZoomLevel = zoomLevel;
}

extern "C" void ResumeAreaCameraControl(CCameraActionArea* area)
{
	area->mControlEnabled           = 1;
	gCameraAreaState.mPrevZoomLevel = gCameraAreaState.mZoomLevel;
}

// the map camera's half

// NONMATCHING: the string sits elsewhere in the file's pool; the pool is only right once the whole file is
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

// the area tables' debug shapes, the GameCube's DrawDebug3d of both area actions
void CCameraActionArea::DrawDebug3d()
{
	gCameraAreaState.DrawDebug3d();
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

// retail keeps the area half's strings in a pool of their own (behind "ccameraactionarea.cpp"); defined
// here, after every map string, the map half's pool keeps retail's order
/**
 * @note Address: 0x801555AC
 * @note Size: 0x12C
 */
void CCameraActionArea::DrawDebug2d()
{
	static const f32 sDebugParams[4] = { 1.1f, 0.9f, 1.0f, 0.6f };
	fn_8016AA74(sDebugParams);
	f32 z = 0.0f;
	f32 y, x;
	x = gScreenSafeRight - 234;
	y = gScreenSafeTop + 18;
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B000000A0AREA(%3d,%3d)(%3d)\n", x, y, z, 9.0f, 9.0f, gCameraAreaState.mActiveArea,
	                       mEntryIndex, gCameraAreaState.mCurrentArea);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B000000A0ACAREA\n", x, y + 9.0f, z, 9.0f, 9.0f);
}

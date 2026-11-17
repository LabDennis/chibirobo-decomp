#include "game/ccameraaction.h"
#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#endif
#include "game/gmath.h"
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
	_2A8                = 0.0f;
	_2C0                = 0.0f;
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
	_2A8               = 0.0f;
	_2C0               = 0.0f;
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

// the GameCube's FindFacing, the entry of the slot that faces most along view; the Wii keeps it out of line
extern "C" s32 fn_801559D0(CCameraActionArea* area, s32 slotIndex, const Vec& view);

extern "C" void SelectAreaCameraEntryByViewDirection(CCameraActionArea* area, s32 mode, f32 time, s32 slotIndex)
{
	area->mControlEnabled = 1;
	CVec view             = area->mInterest - area->mEye;
	PSVECNormalize(&view, &view);
	SelectAreaCameraEntry(area, slotIndex, fn_801559D0(area, slotIndex, view), mode, time);
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

extern "C" s32 CollectMapCameraAreaIntersections(CCameraActionMap* map, const Vec& from, const Vec& to, CamMapEntry* out, s32 type);
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

void CCameraActionMap::DrawDebug3d()
{
	DrawMapCameraAreaDebug3D(&gCameraAreaState, 1);
}

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

// the area camera's half of the Wii camera code; retail keeps it in its own file, whose string pool
// carries "ccameraactionarea.cpp" ahead of ccameraactionmap2.cpp's. CamAreaSlot's constructor is the
// file's first emitted function.

/**
 * @note Address: 0x80154310
 * @note Size: 0x10
 */
CamAreaSlot::CamAreaSlot()
{
}

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

// the entry of the active area's group that faces most along dir becomes the current one, the first
// when none does; the GameCube writes this out in UpdateAreaCameraEntrySelection
// NONMATCHING: the dot product's paired-single registers differ, as in FindFacing (99.4 %)
extern "C" void SelectActiveAreaCameraEntryFacing(CCameraActionArea* area, const Vec& dir)
{
	CVec look(dir.x, dir.y, dir.z);
	CamAreaSlot* slot   = &gCameraAreaState.mGroups[gCameraAreaState.mActiveArea];
	f32 best            = -1.0f;
	CamAreaEntry* entry = slot->mEntries;
	for (s32 i = 0; i < slot->mEntryCount; entry++, i++) {
		CVec facing;
		YawDir(facing, entry->mAngle);
		f32 dot = nw4r::math::VEC3Dot((nw4r::math::VEC3*)&look, (nw4r::math::VEC3*)&facing);
		if (dot > best) {
			area->mEntryIndex = i;
			best              = dot;
		}
	}
	if (best == -1.0f) {
		area->mEntryIndex = 0;
	}
}

// the active area's entries nearest the view's bearing at or below it ([0]) and the one before that ([1])
// NONMATCHING: retail guards the loop with a zero-count test and hoists the wrap constants behind it,
// which the GameCube build of the same loop does not do either (89.9 %)
extern "C" void FindActiveAreaCameraEntriesByBearing(CCameraActionArea* area, s32* indices)
{
	f32 yaw  = WrapAngle(atan2(area->mEye.x - area->mInterest.x, area->mEye.z - area->mInterest.z));
	f32 best = TWO_PI;
	s32 index;
	CamAreaSlot* slot   = &gCameraAreaState.mGroups[gCameraAreaState.mActiveArea];
	CamAreaEntry* entry = slot->mEntries;
	for (s32 i = 0; i < slot->mEntryCount; entry++, i++) {
		f32 facing = WrapAngle(atan2(entry->mEye.x - area->mInterest.x, entry->mEye.z - area->mInterest.z));
		f32 turn   = WrapAngle(yaw - facing);
		if (turn <= best) {
			best  = turn;
			index = i;
		}
	}
	s32 prev = index - 1;
	if (prev < 0) {
		prev = slot->mEntryCount - 1;
	}
	indices[0] = index;
	indices[1] = prev;
}

// the Wii-only variant: picks the entry facing the current view in the active area's group
// NONMATCHING: the view and the eye copy swap stack slots (99.9 %); retail expands
// SelectAreaCameraEntryByViewDirection here, which this build does not
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

// the area tables' debug shapes, the GameCube's DrawDebug3d of both area actions
void CCameraActionArea::DrawDebug3d()
{
	gCameraAreaState.DrawDebug3d();
}

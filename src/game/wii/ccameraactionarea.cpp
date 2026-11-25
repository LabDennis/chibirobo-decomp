#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf
#include "nw4r/math/math_types.h"
#include "nw4r/math/math_arithmetic.h"
#include "game/ccameraaction.h"
#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#endif
#include "game/gmath.h"
#include "game/cvec.h"
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
s32 UpdateAreaCameraEntrySelection(CCameraActionArea* area);
void FindActiveAreaCameraEntriesByBearing(CCameraActionArea* area, s32* indices);
void SelectActiveAreaCameraEntryFacing(CCameraActionArea* area, const Vec& dir);
f32 GetAreaCameraFovForHeightLevel(CamAreaState* state, s32 level);
Vec fn_8016AA78(Vec in, f32* limits);
f32 fn_8016A81C(f32 value, f32 min, f32 max);
f32 fn_8016A8D4(f32 from, f32 to, f32 t, f32 scale);
Vec GetCornerIconPosition(CGame* game);
}

// the Wii build resolves the current area's action in a second, C++ step that falls back to
// the map camera while the player stands outside every area
s32 GetCameraAreaMode(CamAreaState* state);

extern s32 gScreenSafeWidth;
extern s32 gPointerScreenWidth;
extern s32 gPointerScreenHeight;
extern s32 gScreenSafeBottom;
extern s32 gRefreshRate;
extern f32 lbl_805A1288[2];
extern f32 lbl_805A1290[2];
extern f32 lbl_805A1298[2];
extern s32 lbl_805A1CF8;
extern s32 lbl_805A1CFC;
extern s32 lbl_805A1D00;
extern s32 lbl_805A1D04;

static const f32 sDebugParams[4] = { 1.1f, 0.9f, 1.0f, 0.6f };

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
 * @note Address: 0x801546F4
 * @note Size: 0xEB8
 */
// NONMATCHING: Wii calls, pad bits, field offsets and vector state are checked. Retail has the
// CPad construction, transformed-eye copy, CVec direction and pointer snapshots. Its 0x1B0 frame
// still exceeds ours by 0x70, mainly from the two scalar pointer clamps and repeat-state scheduling.
void CCameraActionArea::Update(u32 step)
{
	if (gGame.GetStatusFlags(0xC00)) {
		mPadControlEnabled = FALSE;
	} else {
		mPadControlEnabled = TRUE;
	}
	if (mTarget != NULL && mTarget->HasModel()) {
		CPad pad(0);
		Mtx mtx;
		PSMTXCopy(mTarget->GetWorldMtx(), mtx);
		CVec flatEye(mEye);
		flatEye.y    = mTarget->GetPos()->y;
		f32 distance = PSVECDistance(&flatEye, mTarget->GetPos());
		CVec localEye(0.0f, mEye.y - flatEye.y, -distance);
		PSMTXMultVec(mtx, &localEye, &localEye);
		CVec transformedEye(localEye);
		CVec pos = *mTarget->GetPos();
	}

	lbl_805A1CF8 = (gWiiPointer.mFlags & 0x40000000) != 0;
	lbl_805A1CFC = (gWiiPointer.mFlags & 0x80000000) != 0;
	lbl_805A1D00 = (gWiiPointer.mFlags & 0x100000000ULL) != 0;
	lbl_805A1D04 = (gWiiPointer.mFlags & 0x200000000ULL) != 0;
	gWiiPointer.mFlags &= ~0xC0000000ULL;
	gWiiPointer.mFlags &= ~0x300000000ULL;
	_2C4 = 0.0f;
	_2C8 = 0.0f;

	s32 entryCount = 0;
	if (gCameraAreaState.mActiveArea >= 0) {
		entryCount = gCameraAreaState.mGroups[gCameraAreaState.mActiveArea].mEntryCount;
	}
	BOOL padControl = mPadControlEnabled && entryCount > 1;
	if ((gWiiPointer.mButton & 4) && padControl) {
		if (_2CC > lbl_805A1288[0]) {
			if (!_2D0) {
				_2D0 = 1;
			}
		} else {
			_2CC += step;
		}
	} else {
		_2D0     = 0;
		_2CC     = 0.0f;
		Vec zero = { 0.0f, 0.0f, 0.0f };
		_2D4     = zero;
	}

	Vec input = { 0.0f, 0.0f, 0.0f };
	if (gWiiPointer.mPointerValid) {
		Vec pointer;
		pointer.x   = 2.0f * (gWiiPointer.mPointer.x / gPointerScreenWidth) - 1.0f;
		pointer.y   = 2.0f * (gWiiPointer.mPointer.y / gPointerScreenHeight) - 1.0f;
		pointer.z   = gWiiPointer.mPointer.z;
		Vec clamped = fn_8016AA78(pointer, (f32*)sDebugParams);
		input       = clamped;
		_2D4        = input;
	} else if (!(gGame._41B0 & 8)) {
		Vec previous = _2D4;
		input        = previous;
	}

	CVec direction(0.0f, 0.0f, 0.0f);
	if ((gWiiPointer.mButton & 2) && padControl) {
		direction.y = 1.0f;
	} else if ((gWiiPointer.mButton & 1) && padControl) {
		direction.y = -1.0f;
	}
	if (_2D0) {
		if (gWiiPointer.mPointerValid) {
			f32 y = fn_8016A81C(2.0f * (gWiiPointer.mPointer.y / gPointerScreenHeight) - 1.0f, -sDebugParams[0], sDebugParams[0]);
			f32 x = fn_8016A81C(2.0f * (gWiiPointer.mPointer.x / gPointerScreenWidth) - 1.0f, -sDebugParams[0], sDebugParams[0]);
			if (nw4r::math::FAbs(y) < sDebugParams[2] && nw4r::math::FAbs(x) < sDebugParams[3]) {
				_2F8 = 1;
			}
		}
		if (_2F8) {
			_2C4 = input.y;
		}
	} else {
		_2F8 = 0;
		_2C4 = direction.y;
	}

	if (_2C4 > 0.0f) {
		_2F0 += step;
	} else {
		_2F0 = 0;
	}
	if (_2C4 < 0.0f) {
		_2F4 += step;
	} else {
		_2F4 = 0;
	}

	BOOL changed = FALSE;
	if (_2C4 > 0.0f) {
		if (_2E8 == 0) {
			_2E8    = 1;
			changed = TRUE;
		}
	} else {
		_2E8 = 0;
	}
	if (changed) {
		gWiiPointer.mFlags |= 0x100000000ULL;
		_2E8 = 1;
	}
	// retail tests both limits in turn (cmplwi 30, cmplwi 60 at 0x80154DF0), the repeat delay and its step
	if (_2E8 && _2F0 >= 30 && _2F0 >= 60) {
		gWiiPointer.mFlags |= 0x100000000ULL;
		_2F0 = 30;
	}

	changed = FALSE;
	if (_2C4 < 0.0f) {
		if (_2EC == 0) {
			_2EC    = 1;
			changed = TRUE;
		}
	} else if (_2EC != 0) {
		_2EC = 0;
	}
	if (changed) {
		gWiiPointer.mFlags |= 0x200000000ULL;
		_2EC = 1;
	}
	if (_2EC && _2F4 >= 30 && _2F4 >= 60) {
		gWiiPointer.mFlags |= 0x200000000ULL;
		_2F4 = 30;
	}

	mArrows[0]._0D    = 0;
	mArrows[1]._0D    = 0;
	mArrows[0].mShown = 0;
	mArrows[1].mShown = 0;
	if (_2D0) {
		mArrows[0]._0D = 1;
		mArrows[1]._0D = 1;
		if (_2C4 > 0.0f) {
			mArrows[0].mShown = 1;
		} else if (_2C4 < 0.0f) {
			mArrows[1].mShown = 1;
		}
	} else {
		if (_2C4 > 0.0f) {
			mArrows[0]._0D    = 1;
			mArrows[0].mShown = 1;
		} else if (_2C4 < 0.0f) {
			mArrows[1]._0D    = 1;
			mArrows[1].mShown = 1;
		}
	}

	if (mArrows[0]._0D) {
		if (mArrows[0]._14 < 1.0f) {
			mArrows[0]._14 += 0.1f * lbl_805A1290[0] * step;
			if (mArrows[0]._14 > 1.0f) {
				mArrows[0]._14 = 1.0f;
			}
		}
	} else if (mArrows[0]._14 > 0.0f) {
		mArrows[0]._14 -= 0.1f * lbl_805A1290[1] * step;
		if (mArrows[0]._14 < 0.0f) {
			mArrows[0]._14 = 0.0f;
		}
	}
	if (mArrows[1]._0D) {
		if (mArrows[1]._14 < 1.0f) {
			mArrows[1]._14 += 0.1f * lbl_805A1290[0] * step;
			if (mArrows[1]._14 > 1.0f) {
				mArrows[1]._14 = 1.0f;
			}
		}
	} else if (mArrows[1]._14 > 0.0f) {
		mArrows[1]._14 -= 0.1f * lbl_805A1290[1] * step;
		if (mArrows[1]._14 < 0.0f) {
			mArrows[1]._14 = 0.0f;
		}
	}

	if (mPadControlEnabled && (gWiiPointer.mTrigger & 8)) {
		if (gCameraAreaState.mZoomLevel > 1) {
			gWiiPointer.mFlags |= 0x40000000;
		}
		if (gCameraAreaState.mZoomLevel < 2) {
			gWiiPointer.mFlags |= 0x80000000;
		}
	}
	if (mControlEnabled == 1 && mPadControlEnabled) {
		UpdateAreaCameraEntrySelection(this);
	}
	s32 currentArea  = gCameraAreaState.mCurrentArea;
	s32 previousArea = gCameraAreaState.mPrevArea;
	if (mEntry->mId != mPrevEntry->mId || mForceApply) {
		if (currentArea != previousArea && currentArea != -1) {
			mTransitionTime = 2.0f;
			ApplyAreaCameraEntry(this, mEntry, 1, 1, mTransitionTime);
		} else if (mForceApply) {
			mTransitionTime = 15.0f;
			ApplyAreaCameraEntry(this, mEntry, 5, 1, mTransitionTime);
			mForceApply = FALSE;
		} else if (currentArea == previousArea) {
			mTransitionTime = 15.0f;
			ApplyAreaCameraEntry(this, mEntry, 5, 1, mTransitionTime);
		}
		gGame.GetPlayer()->ResetCameraValue();
		mPrevEntryIndex = mEntryIndex;
		mPrevEntry      = mEntry;
		if (mKeepViewDirection != 0) {
			mKeepViewDirection = 0;
		}
	}
	s32 zoomLevel = gCameraAreaState.mZoomLevel;
	if (zoomLevel != gCameraAreaState.mPrevZoomLevel) {
		f32 fov    = GetAreaCameraFovForHeightLevel(&gCameraAreaState, zoomLevel);
		mFovInterp = mFovInterps[4];
		if (mFovInterp != NULL) {
			mFovInterp->Start(mFov, fov, 15.0f);
		}
		gGame.GetPlayer()->ResetCameraValue();
		gCameraAreaState.mZoomLevel     = zoomLevel;
		gCameraAreaState.mPrevZoomLevel = zoomLevel;
	}
	if (mEyeInterp != NULL) {
		if (!mEyeInterp->Update(step, 1.0f / gRefreshRate)) {
			SetEye(mEyeInterp->Get());
		} else {
			mEyeInterp = NULL;
		}
	}
	if (mFovInterp != NULL) {
		if (!mFovInterp->Update(step, 1.0f / gRefreshRate)) {
			SetFov(mFovInterp->Get());
		} else {
			mFovInterp = NULL;
		}
	}
	if (mInterestInterp != NULL) {
		if (!mInterestInterp->Update(step, 1.0f / gRefreshRate)) {
			SetInterest(mInterestInterp->Get());
		} else {
			mInterestInterp = NULL;
		}
	}
	if (mInterestInterp == NULL && mTarget != NULL) {
		SetInterest(mTarget->GetCameraTarget());
	}

	if (lbl_805A1CF8) {
		gWiiPointer.mFlags |= 0x40000000;
	} else {
		gWiiPointer.mFlags &= ~0x40000000ULL;
	}
	if (lbl_805A1CFC) {
		gWiiPointer.mFlags |= 0x80000000;
	} else {
		gWiiPointer.mFlags &= ~0x80000000ULL;
	}
	if (lbl_805A1D04) {
		gWiiPointer.mFlags |= 0x200000000ULL;
	} else {
		gWiiPointer.mFlags &= ~0x200000000ULL;
	}
	if (lbl_805A1D00) {
		gWiiPointer.mFlags |= 0x100000000ULL;
	} else {
		gWiiPointer.mFlags &= ~0x100000000ULL;
	}
}

/**
 * @note Address: 0x801555AC
 * @note Size: 0x12C
 */
void CCameraActionArea::DrawDebug2d()
{
	fn_8016AA74(sDebugParams);
	f32 z = 0.0f;
	f32 y, x;
	x = gScreenSafeRight - 234;
	y = gScreenSafeTop + 18;
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B000000A0AREA(%3d,%3d)(%3d)\n", x, y, z, 9.0f, 9.0f, gCameraAreaState.mActiveArea,
	                       mEntryIndex, gCameraAreaState.mCurrentArea);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B000000A0ACAREA\n", x, y + 9.0f, z, 9.0f, 9.0f);
}

inline void YawDir(CVec& dir, const Vec& angle);

/**
 * @note Address: 0x801556D8
 * @note Size: 0x2F8
 */
// NONMATCHING: the control flow and calls match; register allocation and the direction vectors' stack slots differ
extern "C" s32 UpdateAreaCameraEntrySelection(CCameraActionArea* area)
{
	s32 slotIndex    = gCameraAreaState.mActiveArea;
	s32 currentArea  = gCameraAreaState.mCurrentArea;
	s32 previousArea = gCameraAreaState.mPrevArea;
	s32 zoomLevel    = gCameraAreaState.mZoomLevel;
	if (gGame.GetStatusFlags(0x20)) {
		return area->mEntry->mId;
	}
	if (gWiiPointer.mFlags & 0x40000000) {
		if (zoomLevel > 1) {
			gCameraAreaState.mZoomLevel = zoomLevel - 1;
			return area->mEntry->mId;
		}
	} else if (gWiiPointer.mFlags & 0x80000000) {
		if (zoomLevel < 2) {
			gCameraAreaState.mZoomLevel = zoomLevel + 1;
			return area->mEntry->mId;
		}
	} else if (area->mKeepViewDirection == 1 && (gWiiPointer.mFlags & 0x100000000ULL)) {
		s32 indices[2];
		FindActiveAreaCameraEntriesByBearing(area, indices);
		area->mEntryIndex = indices[0];
		area->mForceApply = TRUE;
	} else if (area->mKeepViewDirection == 1 && (gWiiPointer.mFlags & 0x200000000ULL)) {
		s32 indices[2];
		FindActiveAreaCameraEntriesByBearing(area, indices);
		area->mEntryIndex = indices[1];
		area->mForceApply = TRUE;
	} else if (gWiiPointer.mFlags & 0x100000000ULL) {
		if (++area->mEntryIndex >= gCameraAreaState.mGroups[slotIndex].mEntryCount) {
			area->mEntryIndex = 0;
		}
	} else if (gWiiPointer.mFlags & 0x200000000ULL) {
		CamAreaSlot* slot = &gCameraAreaState.mGroups[slotIndex];
		if (--area->mEntryIndex < 0) {
			area->mEntryIndex = slot->mEntryCount - 1;
		}
	} else if (currentArea != previousArea && currentArea != -1) {
		CVec dir;
		if (area->mKeepViewDirection != 0) {
			CVec delta = area->mInterest - area->mEye;
			delta.Normalize();
			dir.Set(delta);
		} else {
			YawDir(dir, area->mEntry->mAngle);
		}
		SelectActiveAreaCameraEntryFacing(area, dir);
	}
	area->mEntry = &gCameraAreaState.mGroups[slotIndex].mEntries[area->mEntryIndex];
	return area->mEntry->mId;
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
// when none does; the GameCube inlines it in UpdateAreaCameraEntrySelection
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
// NONMATCHING: the zero-count guard matches. This build schedules the loop counter's li after the wrap
// constants, retail before the group address; the remaining symbol differences are the incomplete literal pool.
extern "C" void FindActiveAreaCameraEntriesByBearing(CCameraActionArea* area, s32* indices)
{
	f32 yaw  = WrapAngle(atan2(area->mEye.x - area->mInterest.x, area->mEye.z - area->mInterest.z));
	f32 best = TWO_PI;
	s32 index;
	CamAreaSlot* slot   = &gCameraAreaState.mGroups[gCameraAreaState.mActiveArea];
	CamAreaEntry* entry = slot->mEntries;
	if (slot->mEntryCount > 0) {
		for (s32 i = 0; i < slot->mEntryCount; entry++, i++) {
			f32 facing = WrapAngle(atan2(entry->mEye.x - area->mInterest.x, entry->mEye.z - area->mInterest.z));
			f32 turn   = WrapAngle(yaw - facing);
			if (turn <= best) {
				best  = turn;
				index = i;
			}
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

inline void CCameraActionArea::SetEye_I(s32 mode, Vec from, Vec to, f32 time)
{
	if (mode == 5) {
		OSPanic("ccameraactionarea.cpp", 1415, "error ! : CCameraActionArea::SetEye_I()\n");
		return;
	}
	CVec(0.0f, 0.0f, 0.0f);
	SetEyeInterpolation(mode, from, to, time);
	gGame.GetPlayer()->ResetCameraValue();
}

/**
 * @note Address: 0x80156200
 * @note Size: 0x7A0
 */
// NONMATCHING: the by-value vectors and interpolation starts use other stack slots and schedules
extern "C" void ApplyAreaCameraEntry(CCameraActionArea* area, CamAreaEntry* entry, s32 mode, s32 adjustFov, f32 time)
{
	switch (mode) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		area->SetEye_I(mode, area->mEye, entry->mEye, time);
		break;
	case 5:
		area->SetEyeInterpolation(mode, area->mEye, entry->mEye, time);
		gGame.GetPlayer()->ResetCameraValue();
		break;
	}
	f32 fov = gCameraAreaState.mBaseFov;
	if (adjustFov) {
		switch (gCameraAreaState.mZoomLevel) {
		case 0:
			area->SetFovInterpolation(mode, area->mFov, 0.8f * fov, time);
			gGame.GetPlayer()->ResetCameraValue();
			break;
		default:
			area->SetFovInterpolation(mode, area->mFov, fov, time);
			gGame.GetPlayer()->ResetCameraValue();
			break;
		case 2:
			area->SetFovInterpolation(mode, area->mFov, 1.5f * fov, time);
			gGame.GetPlayer()->ResetCameraValue();
			break;
		case 3:
			area->SetFovInterpolation(mode, area->mFov, 2.0f * fov, time);
			gGame.GetPlayer()->ResetCameraValue();
			break;
		}
	} else {
		area->SetFovInterpolation(mode, area->mFov, fov, time);
		gGame.GetPlayer()->ResetCameraValue();
	}
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

/**
 * @note Address: 0x801569B4
 * @note Size: 0x4A4
 */
// NONMATCHING: the arrow loop and calls are reconstructed; stack copies and floating-point schedules differ
void CCameraActionArea::vf24()
{
	if (mControlEnabled == 0) {
		return;
	}
	Vec corner = GetCornerIconPosition(&gGame);
	f32 limit  = gGame.mWipeCine.mY + 0.5f * gGame.mWipeCine.mSpeedHeight;
	if (limit >= gScreenSafeBottom) {
		limit = gScreenSafeBottom;
	}
	f32 bottom = corner.y;
	if (bottom > limit) {
		bottom = gGame.mWipeCine.mY + 0.5f * gGame.mWipeCine.mSpeedHeight;
		if (bottom >= gScreenSafeBottom) {
			bottom = gScreenSafeBottom;
		}
	}
	f32 maxY = ((bottom - 58.0f) - 40.0f) - 50.0f;
	for (s32 i = 0; i < 2; i++) {
		CamEdgeArrow* arrow = &mArrows[i];
		f32 scale;
		if (arrow->_0D) {
			arrow->_10 = 0.5f;
			if (arrow->mShown) {
				arrow->_10 = 1.0f;
			}
			scale = arrow->_10 * fn_8016A8D4(0.0f, 1.0f, arrow->_14, lbl_805A1298[0]);
		} else {
			scale = arrow->_10 * fn_8016A8D4(0.0f, 1.0f, arrow->_14, lbl_805A1298[0]);
		}
		if (scale <= 0.0f) {
			continue;
		}
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		if (i == 1) {
			gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_NONE);
		}
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[263], 0);
		CTexObj tex = gGame.mTpl.mTex;
		PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
		tex.mTexMtx.mDirty    = true;
		u32 color             = 0xFFFF00FF;
		((GXColor*)&color)->a = (u8)(((GXColor*)&color)->a * scale);
		f32 x                 = arrow->mPos.x + 0.5f * gPointerScreenWidth;
		f32 y                 = arrow->mPos.y + 0.5f * gPointerScreenHeight;
		if (y > maxY) {
			y = maxY;
		}
		f32 width  = GXGetTexObjWidth(&tex);
		f32 height = GXGetTexObjHeight(&tex);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, arrow->mPos.z, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);
	}
}

// the area tables' debug shapes, the GameCube's DrawDebug3d of both area actions
void CCameraActionArea::DrawDebug3d()
{
	gCameraAreaState.DrawDebug3d();
}

// retail links SpringDamp's text into this unit (between GetCameraFlags and the map camera's Init), but its
// panics name ccameraactionmap2.cpp, its true source file; the GameCube's src/game/ccameraactionmap2.cpp
// has the same function

/**
 * @note Address: 0x80156E7C
 * @note Size: 0x2D8
 */
// NONMATCHING: the arithmetic and call set match; retail copies displacement into a stack Vec (0x18) before the add, which this build
// skips, so the frame is 0xF0 instead of 0x100 and the dot product's registers swap
extern "C" CVec SpringDamp(CVec& position, Vec& target, const CVec& previous, f32 step, f32 stiffness, f32 damping, f32 restDistance)
{
	CVec displacement;
	CVec velocity;
	displacement = position - target;
	f32 distance = sqrt(displacement.z * displacement.z + (displacement.x * displacement.x + displacement.y * displacement.y));
	if (__fpclassifyf(distance) == FP_NAN) {
		OSPanic("ccameraactionmap2.cpp", 192,
		        FormatToStaticBuffer("CCameraActionMap ... disp(%f, %f, %f)\n", displacement.x, displacement.y, displacement.z));
	}
	velocity  = (previous - target) * step;
	f32 speed = nw4r::math::VEC3Dot((nw4r::math::VEC3*)&displacement, (nw4r::math::VEC3*)&velocity) / distance;
	f32 force = stiffness * (restDistance - distance) + damping * speed;
	PSVECNormalize(&displacement, &displacement);
	nw4r::math::VEC3Scale((nw4r::math::VEC3*)&displacement, (nw4r::math::VEC3*)&displacement, force * step);
	position += displacement;
	if (__fpclassifyf(position.x) == FP_NAN || __fpclassifyf(position.y) == FP_NAN || __fpclassifyf(position.z) == FP_NAN) {
		// retail's format and args don't line up
		OSPanic("ccameraactionmap2.cpp", 206,
		        FormatToStaticBuffer("CCameraActionMap ... (%f, %f, %f), %f, %f, %f, %s\n", displacement.x, displacement.y, displacement.z,
		                             distance, force, velocity, 206));
	}
	return position;
}

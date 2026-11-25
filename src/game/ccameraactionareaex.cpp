#include "game/ccameraaction.h"
#include "game/caminfo.h"
#include "game/cmotioncontrolcamera.h"
#include "game/stagedata.h"

extern "C" void OSReport(const char* format, ...);

extern "C" void BuildCameraAreaGroups(CCameraActionAreaEx* state);
extern "C" void UpdateCameraAreaIndexFromPlayer(CCameraActionAreaEx* state, s32 unused);

static BOOL ContainsPoint(const CamAreaInfo& area, Vec& pos)
{
	Vec corners[4] = { area.GetCorner(0), area.GetCorner(1), area.GetCorner(2), area.GetCorner(3) };
	return IsPointInsideQuadXZ(&pos, &corners[0], &corners[1], &corners[2], &corners[3]) && pos.y <= area.GetTop()
	    && pos.y >= area.GetBottom();
}

/**
 * @note Address: 0x801371D0
 * @note Size: 0x324
 */
extern "C" s32 CollectCameraAreasContainingPoint(CCameraActionAreaEx* state, Vec pos, s32* ids)
{
	s32 count = 0;
	for (s32 i = 0; i < state->mAreaExCount; ++i) {
		if (ContainsPoint(CamAreaInfo(&state->mAreaExRecords[i]), pos)) {
			*ids++ = i;
			++count;
		}
	}
	return count;
}

/**
 * @note Address: 0x801374F4
 * @note Size: 0x31C
 */
extern "C" s32 FindFirstCameraAreaContainingPoint(CCameraActionAreaEx* state, Vec pos)
{
	for (s32 i = 0; i < state->mAreaExCount; ++i) {
		if (ContainsPoint(CamAreaInfo(&state->mAreaExRecords[i]), pos))
			return i;
	}
	return -1;
}

/**
 * @note Address: 0x80137810
 * @note Size: 0x348
 */
extern "C" s32 FindEnabledCameraAreaContainingPoint(CCameraActionAreaEx* state, Vec pos)
{
	for (s32 i = 1; i < state->mAreaCount; ++i) { // record 0 is reserved
		CamAreaInfo area(&state->mAreaRecords[i]);
		if (!area.IsCameraOff() && ContainsPoint(area, pos))
			return i;
	}
	return -1;
}

/**
 * @note Address: 0x80137B58
 * @note Size: 0x1A8
 */
extern "C" void SetCameraAreaIndexAndBaseFov(CCameraActionAreaEx* state, s32 index)
{
	if (state->mAreaRecords != NULL && state->mCameraInfo != NULL) {
		state->mActiveArea  = index;
		state->mCurrentArea = index;
		state->mPrevArea    = index;
		f32 fov;
		if (GetCameraAreaMode(state) == CAMACTION_AREA) {
			fov = CamInfo(&state->mCameraInfo[state->mGroups[state->mActiveArea].mEntries->mId]).GetFov();
		} else {
			fov = 30.5f; // other modes use a fixed fov
		}
		state->mBaseFov = fov;
	}
}

/**
 * @note Address: 0x80137D00
 * @note Size: 0x1B8
 */
extern "C" void UpdateCameraAreaIndexAndBaseFov(CCameraActionAreaEx* state, s32 unused)
{
	if (state->mAreaRecords != NULL && state->mCameraInfo != NULL) {
		UpdateCameraAreaIndexFromPlayer(state, unused);
		if (state->mPrevArea != state->mCurrentArea && state->mCurrentArea != -1) {
			f32 fov;
			if (GetCameraAreaMode(state) == CAMACTION_AREA) {
				fov = CamInfo(&state->mCameraInfo[state->mGroups[state->mActiveArea].mEntries->mId]).GetFov();
			} else {
				fov = 30.5f; // other modes use a fixed fov
			}
			state->mBaseFov = fov;
		}
	}
}

static s32 CountAreaRecords(const CamAreaRecord* records)
{
	s32 count = 0;
	if (records != NULL) {
		while (records[count]._00 != -1)
			++count;
	}
	return count;
}

/**
 * @note Address: 0x80137EB8
 * @note Size: 0x164
 */
extern "C" void LoadStageCameraAreaState(CCameraActionAreaEx* state)
{
	state->mAreaExRecords = GetStageCameraAreaExRecords();
	state->mAreaExCount   = CountAreaRecords(state->mAreaExRecords);
	state->mAreaRecords   = GetStageCameraAreaRecords();
	state->mAreaCount     = CountAreaRecords(state->mAreaRecords);
	state->mCameraInfo    = GetStageCameraInfoRecords();
	BuildCameraAreaGroups(state);
	state->mCurrentArea = -1;
	state->mPrevArea    = -1;
	state->mActiveArea  = -1;
	state->mBaseFov     = 30.5f;
#if !defined(VERSION_PAL)
	OSReport("reset zoom lv = CAM_AREA_ZOOM_LV1\n");
#endif
}

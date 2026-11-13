#include "game/ccameraaction.h"
#include "game/caminfo.h"
#include <math.h>
#include "game/cgs.h"
#include "game/stagedata.h"
#include "game/cgame.h"
#include "game/xchr.h"

CCameraActionAreaEx gCameraAreaState;

extern "C" void OSReport(const char* format, ...);

extern "C" void BuildCameraAreaGroups(CCameraActionAreaEx* state);
extern "C" void UpdateCameraAreaIndexFromPlayer(CCameraActionAreaEx* state, s32 unused);

/**
 * @note Address: 0x80135D28
 * @note Size: 0x10C
 */
extern "C" s32 GetCameraAreaMode(CCameraActionAreaEx* state)
{
	s32 index = state->mActiveArea;
	if (index == -1)
		return 7;
	const CamAreaRecord& area = GetStageCameraAreaRecords()[index];
	switch (area.mCameraAction) {
	case 1:
	case 7:
		return area.mCameraAction;
	default:
		return area.GetBottom() < -5.0f ? 7 : 1;
	}
}

/**
 * @note Address: 0x801361C4
 * @note Size: 0x890
 */
extern "C" void DrawDebug3d__19CCameraActionAreaExFv(CCameraActionAreaEx* state)
{
	if (state->mAreaRecords == NULL)
		return;
	u32 colors[7] = { 0xFF000064, 0x00FF0064, 0x0000FF64, 0xFFFF0064, 0xFF00FF64, 0xFFFFFF64, 0xFF960064 };
	f32 scale     = 2.0f / sqrtf(3.0f);
	for (s32 i = 0; i < state->mAreaCount; ++i) {
		u16 flags = CamAreaInfo(&state->mAreaRecords[i]).mRecord->mFlags & 4;
		if (flags == 0)
			continue;
		static s32 selected = 0;
		selected            = i;
		CPad pad(3);
		if (pad.mPad->mTrigger & 2) {
			if (++selected >= state->mAreaCount)
				selected = state->mAreaCount - 1;
#if !defined(VERSION_PAL)
			OSReport("DEBUG:CCameraActionAreaEx::DrawDebug3d():%d\n", selected);
#endif
		} else if (pad.mPad->mTrigger & 1) {
			if (--selected <= 0)
				selected = 0;
#if !defined(VERSION_PAL)
			OSReport("DEBUG:CCameraActionAreaEx::DrawDebug3d():%d\n", selected);
#endif
		}

		CamAreaInfo area(&state->mAreaRecords[selected]);
		CVec a     = area.mRecord->GetPoint(0);
		CVec b     = area.mRecord->GetPoint(1);
		CVec c     = area.mRecord->GetPoint(2);
		f32 top    = area.GetTop();
		f32 bottom = area.GetBottom();
		f32 width  = PSVECDistance(&a, &b);
		f32 depth  = PSVECDistance(&b, &c);
		f32 height = fabs(top - bottom);
		Vec center = a + (c - a) / 2.0f;
		width /= scale;
		height /= scale;
		depth /= scale;
		center.y = (top - bottom) / 2.0f + bottom;
		DrawGsCube(&gGame.mGs, colors[selected % 5], center.x, center.y, center.z, 0.0f, 0.0f, 0.0f, width, height, depth);

		s32 index           = selected;
		CamAreaEntry* entry = state->mGroups[index].mEntries;
		for (s32 j = 0; j < state->mGroups[index].mEntryCount; ++j, ++entry) {
			CVec pos, next;
			pos  = entry->mEye;
			next = j == state->mGroups[index].mEntryCount - 1 ? state->mGroups[index].mEntries[0].mEye : entry[1].mEye;
			DrawGsLineSegment(&gGame.mGs, NULL, pos.x, pos.y, pos.z, colors[5], next.x, next.y, next.z, colors[5]);
			DrawGsSphere(&gGame.mGs, colors[index % 5], 12, 16, pos.x, pos.y, pos.z, 0.0f, 0.0f, 0.0f, 3.0f, 3.0f, 3.0f);
			CVec angle  = entry->mAngle;
			f32 sine    = sin(angle.y);
			f32 cosine  = cos(angle.y);
			f32 offsetX = 15.0f * sine;
			f32 offsetZ = 15.0f * cosine;
			DrawGsLineSegment(&gGame.mGs, NULL, entry->mEye.x, entry->mEye.y, entry->mEye.z, 0xFFFF, entry->mEye.x + offsetX, entry->mEye.y,
			                  entry->mEye.z + offsetZ, 0xFFFF);
		}
		DrawGsSphere(&gGame.mGs, colors[index % 5], 12, 16, state->mGroups[index].mEye.x, state->mGroups[index].mEye.y,
		             state->mGroups[index].mEye.z, 0.0f, 0.0f, 0.0f, 3.0f, 3.0f, 3.0f);
	}
}

/**
 * @note Address: 0x80136A54
 * @note Size: 0x77C
 */
extern "C" void DrawMapCameraAreaDebug3D(CCameraActionAreaEx* state, s32 mode)
{
	CamAreaRecord* records;
	s32 count;
	if (mode == 0) {
		records = state->mAreaRecords;
		count   = state->mAreaCount;
	} else if (mode == 1) {
		records = state->mAreaExRecords;
		count   = state->mAreaExCount;
	} else {
		return;
	}
	if (records == NULL)
		return;
	u32 colors[7] = { 0xFF000064, 0x00FF0064, 0x0000FF64, 0xFFFF0064, 0xFF00FF64, 0xFFFFFF64, 0xFF960064 };
	f32 scale     = 2.0f / sqrtf(3.0f);
	for (s32 i = 0; i < count; ++records, ++i) {
		static s32 selected = 0;
		CPad pad(3);
#if !defined(VERSION_PAL)
		if (pad.mPad->mTrigger & 2) {
			if (++selected >= count)
				selected = count - 1;
			OSReport("DEBUG:CCameraActionMap::DrawDebug3d():%d\n", selected);
		} else if (pad.mPad->mTrigger & 1) {
			if (--selected <= 0)
				selected = 0;
			OSReport("DEBUG:CCameraActionMap::DrawDebug3d():%d\n", selected);
		}
#endif
		selected = i;
		CamAreaInfo area(records);
		if (area.IsDisabled())
			continue;
		CVec a     = area.mRecord->GetPoint(0);
		CVec b     = area.mRecord->GetPoint(1);
		CVec c     = area.mRecord->GetPoint(2);
		f32 top    = area.GetTop();
		f32 bottom = area.GetBottom();
		f32 ax     = a.x;
		f32 az     = a.z;
		f32 bx     = b.x;
		f32 bz     = b.z;
		Vec edge, from, to;
		from.x = bx;
		from.y = top;
		from.z = bz;
		to.x   = c.x;
		to.y   = top;
		to.z   = c.z;
		PSVECSubtract(&from, &to, &edge);
		PSVECMag(&edge);
		from.x = ax;
		from.y = top;
		from.z = az;
		to.x   = bx;
		to.y   = top;
		to.z   = bz;
		PSVECSubtract(&to, &from, &edge);
		PSVECMag(&edge);
		f32 yaw;
		if (az - 0.01f < bz && bz < az + 0.01f) {
			yaw = 0.0f;
		} else {
			from.x = 1.0f;
			from.y = 0.0f;
			from.z = 0.0f;
			PSVECNormalize(&edge, &edge);
			yaw = acos(PSVECDotProduct(&from, &edge));
		}
		f32 width  = PSVECDistance(&a, &b);
		f32 depth  = PSVECDistance(&b, &c);
		f32 height = fabs(top - bottom);
		Vec center = a + (c - a) / 2.0f;
		width /= scale;
		height /= scale;
		depth /= scale;
		center.y = (top - bottom) / 2.0f + bottom;
		DrawGsCube(&gGame.mGs, colors[selected % 5], center.x, center.y, center.z, 0.0f, yaw, 0.0f, width, height, depth);
	}
}

static bool ContainsPoint(const CamAreaRecord& area, const Vec& pos)
{
	Vec a = area.GetPoint(0);
	Vec b = area.GetPoint(1);
	Vec c = area.GetPoint(2);
	Vec d = area.GetPoint(3);
	return IsPointInsideQuadXZ(&pos, &a, &b, &c, &d) && pos.y <= area.GetTop() && pos.y >= area.GetBottom();
}

/**
 * @note Address: 0x801371D0
 * @note Size: 0x324
 */
extern "C" s32 CollectCameraAreasContainingPoint(CCameraActionAreaEx* state, Vec pos, s32* ids)
{
	s32 count = 0;
	for (s32 i = 0; i < state->mAreaExCount; ++i) {
		if (ContainsPoint(state->mAreaExRecords[i], pos)) {
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
	s32 index = -1;
	for (s32 i = 0; i < state->mAreaExCount; ++i) {
		if (ContainsPoint(state->mAreaExRecords[i], pos)) {
			index = i;
			break;
		}
	}
	return index;
}

/**
 * @note Address: 0x80137810
 * @note Size: 0x348
 */
extern "C" s32 FindEnabledCameraAreaContainingPoint(CCameraActionAreaEx* state, Vec pos)
{
	s32 index = -1;
	for (s32 i = 1; i < state->mAreaCount; ++i) {
		if (CamAreaInfo(&state->mAreaRecords[i]).IsCameraOff())
			continue;
		if (ContainsPoint(state->mAreaRecords[i], pos)) {
			index = i;
			break;
		}
	}
	return index;
}

/**
 * @note Address: 0x80135E34
 * @note Size: 0x390
 */
extern "C" void UpdateCameraAreaIndexFromPlayer(CCameraActionAreaEx* state, s32)
{
	state->mPrevArea    = state->mCurrentArea;
	state->mCurrentArea = FindEnabledCameraAreaContainingPoint(state, gGame.GetPlayer()->mMoveTranslate);
	if (state->mPrevArea != state->mCurrentArea && state->mCurrentArea != -1)
		state->mActiveArea = state->mCurrentArea;
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
		if (GetCameraAreaMode(state) == 1) {
			fov = CamInfo(&state->mCameraInfo[state->mGroups[state->mActiveArea].mEntries->mId]).GetFov();
		} else {
			fov = 30.5f;
		}
		state->mBaseFov = fov;
	}
}

/**
 * @note Address: 0x80137D00
 * @note Size: 0x1B8
 */
// NONMATCHING: retail has the two loads of the -1 test in r0 and r3 the other way round (as for a -1L compare)
extern "C" void UpdateCameraAreaIndexAndBaseFov(CCameraActionAreaEx* state, s32 unused)
{
	if (state->mAreaRecords != NULL && state->mCameraInfo != NULL) {
		UpdateCameraAreaIndexFromPlayer(state, unused);
		if (state->mPrevArea != state->mCurrentArea && state->mCurrentArea != -1) {
			f32 fov;
			if (GetCameraAreaMode(state) == 1) {
				fov = CamInfo(&state->mCameraInfo[state->mGroups[state->mActiveArea].mEntries->mId]).GetFov();
			} else {
				fov = 30.5f;
			}
			state->mBaseFov = fov;
		}
	}
}

static inline bool IsAreaEnd(CamAreaRecord* record)
{
	CamAreaInfo area(record);
	return area.mRecord->_00 == -1;
}

/**
 * @note Address: 0x80137EB8
 * @note Size: 0x164
 */
// NONMATCHING: retail reloads _1C and _24 from the state on every pass of the counting loops, as for a helper
// that takes the table by reference; these loops keep the pointer in a register
extern "C" void LoadStageCameraAreaState(CCameraActionAreaEx* state)
{
	state->mAreaExRecords = NULL;
	state->mAreaExCount   = 0;
	state->mAreaExRecords = GetStageCameraAreaExRecords();
	s32 count             = 0;
	if (state->mAreaExRecords != NULL) {
		for (;;) {
			if (IsAreaEnd(&state->mAreaExRecords[count]))
				break;
			++count;
		}
	}
	state->mAreaExCount = count;
	state->mAreaRecords = NULL;
	state->mAreaCount   = 0;
	state->mAreaRecords = GetStageCameraAreaRecords();
	count               = 0;
	if (state->mAreaRecords != NULL) {
		for (;;) {
			if (IsAreaEnd(&state->mAreaRecords[count]))
				break;
			++count;
		}
	}
	state->mAreaCount  = count;
	state->mCameraInfo = NULL;
	state->mCameraInfo = GetStageCameraInfoRecords();
	BuildCameraAreaGroups(state);
	state->mCurrentArea = -1;
	state->mPrevArea    = -1;
	state->mActiveArea  = -1;
	state->mBaseFov     = 30.5f;
#if !defined(VERSION_PAL)
	OSReport("reset zoom lv = CAM_AREA_ZOOM_LV1\n");
#endif
}

/**
 * @note Address: 0x8013801C
 * @note Size: 0x138
 */
extern "C" void InitCameraAreaEntries(CCameraActionAreaEx* state)
{
	// the compiler emits its array dtor, CamAreaEntry's dtor and ctor at 0x80135C5C to 0x80135D10
	static CamAreaEntry gCameraAreaEntries[2048];
#if !defined(VERSION_PAL)
	OSReport("CCameraActionAreaEx :: "
	         "\x83\x4A\x83\x81\x83\x89\x83\x66\x81\x5B\x83\x5E\x82\xCC\x83\x41\x83\x68\x83\x8C\x83\x58\x82\xCD"
	         "0x%x\x82\xC5\x82\xB7\x81\x42\n",
	         gCameraAreaEntries);
#endif
	for (int i = 0; i < 2048; ++i)
		state->mEntryPool[i] = &gCameraAreaEntries[i];
	state->mPrevZoomLevel = 1;
	state->mZoomLevel     = 1;
}

/**
 * @note Address: 0x80135434
 * @note Size: 0x7E0
 */
extern "C" void BuildCameraAreaGroups(CCameraActionAreaEx* state)
{
	if (state->mCameraInfo == NULL)
		return;

	CamAreaEntry* entry = state->mEntryPool[0];
	for (s32 i = 0;; ++i, ++entry) {
		if (CamInfo(&state->mCameraInfo[i]).mData->mGroupId == -1)
			break;
		entry->mInterest = CamInfo(&state->mCameraInfo[i]).GetInterest();
		entry->mEye      = CamInfo(&state->mCameraInfo[i]).GetEye();
		entry->mAngle    = CVec(0.0f, atan2(entry->mInterest.x - entry->mEye.x, entry->mInterest.z - entry->mEye.z), 0.0f);
		entry->mFov      = CamInfo(&state->mCameraInfo[i]).GetFov();
		entry->mId       = i;
	}

	s32 first         = 0;
	CamInfoData* data = state->mCameraInfo;
	s32 group         = 0;
	for (;;) {
		if (data->mGroupId == -1)
			break;
		state->mGroups[group].mEntries = state->mEntryPool[first];
		state->mGroups[group].mGroupId = group;
		s32 count                      = 0;
		while (data->mGroupId == group) {
			++count;
			++data;
		}
		state->mGroups[group].mEntryCount = count;
		first += count;
		++group;
	}
	state->mGroupCount = group;

	for (s32 i = 0; i < state->mGroupCount; ++i) {
		CamAreaSlot* slot = &state->mGroups[i];
		switch (slot->mEntryCount) {
		case 0:
			break;
		case 1: {
			CVec center = (Vec)slot->mEntries[0].mEye;
			slot->mEye  = center;
			break;
		}
		case 2: {
			CVec center;
			CamAreaEntry* entries = slot->mEntries;
			center                = entries[0].mEye + (entries[1].mEye - entries[0].mEye) / 2.0f;
			slot->mEye            = center;
			break;
		}
		case 3: {
			CVec center;
			CamAreaEntry* entries = slot->mEntries;
			center                = entries[0].mEye + (entries[1].mEye - entries[0].mEye) / 2.0f;
			slot->mEye            = center + (entries[2].mEye - center) / 3.0f;
			break;
		}
		case 4: {
			CVec center;
			CamAreaEntry* entries = slot->mEntries;
			center                = entries[0].mEye + (entries[2].mEye - entries[0].mEye) / 2.0f;
			slot->mEye            = center;
			break;
		}
		default: {
			CVec center;
			CamAreaEntry* entries = slot->mEntries;
			center                = entries[0].mEye + (entries[1].mEye - entries[0].mEye) / 2.0f;
			slot->mEye            = center + (entries[2].mEye - center) / 3.0f;
			break;
		}
		}
	}
}

/**
 * @note Address: 0x801353E4
 * @note Size: 0x50
 */
extern "C" f32 GetAreaCameraFovForHeightLevel(CCameraActionAreaEx* state, s32 level)
{
	f32 scale;
	switch (level) {
	case 0:
	default:
		scale = 0.8f;
		break;
	case 1:
		scale = 1.0f;
		break;
	case 2:
		scale = 1.5f;
		break;
	case 3:
		scale = 2.0f;
		break;
	}
	return scale * state->mBaseFov;
}

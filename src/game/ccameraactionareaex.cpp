#include "game/ccameraaction.h"
#include "game/caminfo.h"
#include "game/cgame.h"
#include "game/cmotioncontrolcamera.h"
#include "game/stagedata.h"
#include "game/xchr.h"
#include <math.h>
#include "game/cgs.h"
#include "game/hsd_jobj.h"

extern "C" void OSReport(const char* format, ...);

/**
 * @note Address: 0x8013801C
 * @note Size: 0x138
 */
extern "C" void InitCameraAreaEntries(CCameraActionAreaEx* state)
{
	static CamAreaEntry entries[sizeof(state->mEntryPool) / sizeof(state->mEntryPool[0])];
#if !defined(VERSION_PAL)
	OSReport("CCameraActionAreaEx :: "
	         "\x83\x4A\x83\x81\x83\x89\x83\x66\x81\x5B\x83\x5E\x82\xCC\x83\x41\x83\x68\x83\x8C\x83\x58\x82\xCD"
	         "0x%x\x82\xC5\x82\xB7\x81\x42\n",
	         reinterpret_cast<u32>(entries));
#endif
	for (u32 i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i)
		state->mEntryPool[i] = &entries[i];
	state->mPrevZoomLevel = 1;
	state->mZoomLevel     = 1;
}

/**
 * @note Address: 0x801353E4
 * @note Size: 0x50
 */
extern "C" f32 GetAreaCameraFovForHeightLevel(CCameraActionAreaEx* state, s32 level)
{
	f32 scale;
	switch (level) { // the zoom level scales the base fov
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

/**
 * @note Address: 0x80135434
 * @note Size: 0x7E0
 */
extern "C" void BuildCameraAreaGroups(CCameraActionAreaEx* state)
{
	if (state->mCameraInfo == NULL)
		return;

	CamAreaEntry* entry = state->mEntryPool[0];
	for (s32 i = 0; state->mCameraInfo[i].mGroupId != -1; ++i, ++entry) {
		CamInfo camera(&state->mCameraInfo[i]);
		entry->mInterest = camera.GetInterest();
		entry->mEye      = camera.GetEye();
		entry->mAngle
		    = CVec(0.0f, atan2(entry->mInterest.x - entry->mEye.x, entry->mInterest.z - entry->mEye.z), 0.0f); // eye to interest yaw
		entry->mFov = camera.GetFov();
		entry->mId  = i;
	}

	s32 first         = 0;
	CamInfoData* data = state->mCameraInfo;
	s32 group         = 0;
	while (data->mGroupId != -1) {
		CamAreaSlot& slot = state->mGroups[group];
		slot.mEntries     = state->mEntryPool[first];
		slot.mGroupId     = group;

		s32 count = 0;
		while (data->mGroupId == group) {
			++count;
			++data;
		}
		slot.mEntryCount = count;
		first += count;
		++group;
	}
	state->mGroupCount = group;

	for (s32 i = 0; i < state->mGroupCount; ++i) {
		CamAreaSlot& slot           = state->mGroups[i];
		const CamAreaEntry* entries = slot.mEntries;
		switch (slot.mEntryCount) {
		case 0:
			break;
		case 1:
			slot.mEye = entries[0].mEye;
			break;
		case 2:
			slot.mEye = entries[0].mEye + (entries[1].mEye - entries[0].mEye) / 2.0f;
			break;
		case 4: // use the midpoint of the quad's diagonal
			slot.mEye = entries[0].mEye + (entries[2].mEye - entries[0].mEye) / 2.0f;
			break;
		default: { // use the centroid of the first three entries
			CVec midpoint = entries[0].mEye + (entries[1].mEye - entries[0].mEye) / 2.0f;
			slot.mEye     = midpoint + (entries[2].mEye - midpoint) / 3.0f;
			break;
		}
		}
	}
}

/**
 * @note Address: 0x80135D28
 * @note Size: 0x10C
 */
extern "C" s32 GetCameraAreaMode(CCameraActionAreaEx* state)
{
	s32 index = state->mActiveArea;
	if (index == -1)
		return CAMACTION_MAP;
	const CamAreaRecord& area = GetStageCameraAreaRecords()[index];
	switch (area.mCameraAction) {
	case CAMACTION_AREA:
	case CAMACTION_MAP:
		return area.mCameraAction;
	default:
		return area.GetBottom() < -5.0f ? CAMACTION_MAP : CAMACTION_AREA;
	}
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

extern "C" void OSReport(const char* format, ...);

/**
 * @note Address: 0x801361C4
 * @note Size: 0x890
 */
void CCameraActionAreaEx::DrawDebug3d()
{
	if (mAreaRecords == NULL)
		return;
	// [0..4] cycle per area, [5] marks the camera path between a group's entries
	u32 colors[7] = { 0xFF000064, 0x00FF0064, 0x0000FF64, 0xFFFF0064, 0xFF00FF64, 0xFFFFFF64, 0xFF960064 };
	f32 scale     = 2.0f / sqrtf(3.0f);
	for (s32 i = 0; i < mAreaCount; ++i) {
		u16 flags = CamAreaInfo(&mAreaRecords[i]).mRecord->mFlags & 4;
		if (flags == 0) // skip areas whose camera is off
			continue;
		static s32 selected = 0;
		selected            = i;
		CPad pad(3);
		if (pad.mPad->mTrigger & 2) { // d-pad right
			if (++selected >= mAreaCount)
				selected = mAreaCount - 1;
#if !defined(VERSION_PAL)
			OSReport("DEBUG:CCameraActionAreaEx::DrawDebug3d():%d\n", selected);
#endif
		} else if (pad.mPad->mTrigger & 1) { // d-pad left
			if (--selected <= 0)
				selected = 0;
#if !defined(VERSION_PAL)
			OSReport("DEBUG:CCameraActionAreaEx::DrawDebug3d():%d\n", selected);
#endif
		}

		CamAreaInfo area(&mAreaRecords[selected]);
		CVec a     = area.GetCorner(0);
		CVec b     = area.GetCorner(1);
		CVec c     = area.GetCorner(2);
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
		CamAreaEntry* entry = mGroups[index].mEntries;
		for (s32 j = 0; j < mGroups[index].mEntryCount; ++j, ++entry) {
			CVec pos, next;
			pos  = entry->mEye;
			next = j == mGroups[index].mEntryCount - 1 ? mGroups[index].mEntries[0].mEye : entry[1].mEye; // wraps
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
		DrawGsSphere(&gGame.mGs, colors[index % 5], 12, 16, mGroups[index].mEye.x, mGroups[index].mEye.y, mGroups[index].mEye.z, 0.0f, 0.0f,
		             0.0f, 3.0f, 3.0f, 3.0f);
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
	switch (mode) {
	case 0:
		records = state->mAreaRecords;
		count   = state->mAreaCount;
		break;
	case 1:
		records = state->mAreaExRecords;
		count   = state->mAreaExCount;
		break;
	default:
		return;
	}
	if (records == NULL)
		return;
	u32 colors[7] = { 0xFF000064, 0x00FF0064, 0x0000FF64, 0xFFFF0064, 0xFF00FF64, 0xFFFFFF64, 0xFF960064 };
	f32 scale     = 2.0f / sqrtf(3.0f);
	for (s32 i = 0; i < count; ++records, ++i) {
		static s32 selected = 0;
#if !defined(VERSION_PAL)
		CPad pad(3);
		if (pad.mPad->mTrigger & 2) { // d-pad right
			if (++selected >= count)
				selected = count - 1;
			OSReport("DEBUG:CCameraActionMap::DrawDebug3d():%d\n", selected);
		} else if (pad.mPad->mTrigger & 1) { // d-pad left
			if (--selected <= 0)
				selected = 0;
			OSReport("DEBUG:CCameraActionMap::DrawDebug3d():%d\n", selected);
		}
#endif
		selected = i;
		CamAreaInfo area(records);
		if (area.IsDisabled())
			continue;
		CVec a     = area.GetCorner(0);
		CVec b     = area.GetCorner(1);
		CVec c     = area.GetCorner(2);
		f32 top    = area.GetTop();
		f32 bottom = area.GetBottom();
		f32 yaw    = 0.0f;
		if (!(a.z - 0.01f < b.z && b.z < a.z + 0.01f)) {
			Vec edge;
			Vec axis = { 1.0f, 0.0f, 0.0f };
			PSVECSubtract(&b, &a, &edge);
			PSVECNormalize(&edge, &edge);
			yaw = acos(PSVECDotProduct(&axis, &edge));
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

// __sinit at 0x80138154 builds it and registers its dtor
CCameraActionAreaEx gCameraAreaState;

/**
 * @note Address: 0x801381C0
 * @note Size: 0x70
 */
CCameraActionAreaEx::~CCameraActionAreaEx()
{
}

// 0x80138230
CamAreaSlot::~CamAreaSlot()
{
}

/**
 * @note Address: 0x80138284
 * @note Size: 0x10
 */
CamAreaSlot::CamAreaSlot()
{
}

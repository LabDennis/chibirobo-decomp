#include "game/ccameraaction.h"
#include "game/caminfo.h"
#include <math.h>
#include "game/cgs.h"
#include "game/stagedata.h"
#include "game/cgame.h"
#include "game/xchr.h"

extern "C" void OSReport(const char* format, ...);
extern "C" Vec lbl_805A0400[];
extern "C" s8 lbl_8065B324;
extern "C" s32 lbl_8065B320;
extern "C" s8 lbl_8065B32C;
extern "C" s32 lbl_8065B328;

class CGs;

extern "C" void BuildCameraAreaGroups(CCameraActionAreaEx* state);
extern "C" void UpdateCameraAreaIndexFromPlayer(CCameraActionAreaEx* state, s32 unused);

static inline s32 AreaMode(CCameraActionAreaEx* state)
{
	s32 index = state->_0C;
	if (index == -1)
		return 7;
	CamAreaInfo area = CamAreaInfo(&GetStageCameraAreaRecords()[index]);
	s32 mode;
	switch (area._04->_2C) {
	case 1:
		mode = 1;
		break;
	case 7:
		mode = 7;
		break;
	case -1:
	default:
		f32 height = area.GetBottom();
		if (height < -5.0f)
			mode = 7;
		else if (height >= -5.0f)
			mode = 1;
		break;
	}
	return mode;
}

/**
 * @note Address: 0x80135D28
 * @note Size: 0x10C
 */
// NONMATCHING: regalloc
extern "C" s32 GetCameraAreaMode(CCameraActionAreaEx* state)
{
	return AreaMode(state);
}

static inline Vec AreaPoint(const CamAreaInfo& area, s32 index)
{
	Vec point = lbl_805A0400[2 + index];
	point.x   = area._04->_04[index][0] / 100.0f;
	point.z   = area._04->_04[index][1] / 100.0f;
	return point;
}

static inline BOOL PointInQuad(const Vec& pos, Vec a, Vec b, Vec c, Vec d)
{
	return IsPointInsideQuadXZ((Vec*)&pos, &a, &b, &c, &d);
}

/**
 * @note Address: 0x801361C4
 * @note Size: 0x890
 */
extern "C" void DrawDebug3d__19CCameraActionAreaExFv(CCameraActionAreaEx* state)
{
	if (state->_24 == NULL)
		return;
	u32 colors[7] = { 0xFF000064, 0x00FF0064, 0x0000FF64, 0xFFFF0064, 0xFF00FF64, 0xFFFFFF64, 0xFF960064 };
	f32 scale     = 2.0f / sqrtf(3.0f);
	for (s32 i = 0; i < state->_28; ++i) {
		if ((CamAreaInfo(&state->_24[i])._04->_2E & 4) == 0)
			continue;
		if (lbl_8065B32C == 0)
			lbl_8065B32C = 1;
		lbl_8065B328 = i;
		CPad pad(3);
		if (pad.mPad->mTrigger & 2) {
			if (++lbl_8065B328 >= state->_28)
				lbl_8065B328 = state->_28 - 1;
#if !defined(VERSION_PAL)
			OSReport("DEBUG:CCameraActionAreaEx::DrawDebug3d():%d\n", lbl_8065B328);
#endif
		} else if (pad.mPad->mTrigger & 1) {
			if (--lbl_8065B328 <= 0)
				lbl_8065B328 = 0;
#if !defined(VERSION_PAL)
			OSReport("DEBUG:CCameraActionAreaEx::DrawDebug3d():%d\n", lbl_8065B328);
#endif
		}

		CamAreaInfo area(&state->_24[lbl_8065B328]);
		CVec a     = AreaPoint(area, 0);
		CVec b     = AreaPoint(area, 1);
		CVec c     = AreaPoint(area, 2);
		CVec d     = AreaPoint(area, 3);
		f32 top    = area.GetTop();
		f32 bottom = area.GetBottom();
		f32 width  = PSVECDistance(&a, &b);
		f32 depth  = PSVECDistance(&b, &c);
		f32 height = fabs(top - bottom);
		Vec center = a + (c - a) / 2.0f;
		width /= scale;
		height /= scale;
		depth /= scale;
		center.y       = (top - bottom) * 0.5f + bottom;
		CVec axis      = CVec(1.0f, 0.0f, 0.0f);
		CVec direction = b - a;
		PSVECNormalize(&direction, &direction);
		DrawGsCube(&gGame.mGs, colors[lbl_8065B328 % 5], center.x, center.y, center.z, 0.0f, 0.0f, 0.0f, width, height, depth);

		CamAreaSlot* slot   = &state->_2034[lbl_8065B328];
		CamAreaEntry* entry = slot->_00;
		s32 colorIndex      = lbl_8065B328 % 5;
		for (s32 j = 0; j < slot->_08; ++j, ++entry) {
			CVec pos = (Vec)entry->_10;
			CVec next;
			if (j == slot->_08 - 1)
				next.Set(slot->_00[0]._10);
			else
				next.Set(entry[1]._10);
			DrawGsLineSegment(&gGame.mGs, NULL, pos.x, pos.y, pos.z, colors[5], next.x, next.y, next.z, colors[5]);
			DrawGsSphere(&gGame.mGs, colors[colorIndex], 12, 16, pos.x, pos.y, pos.z, 0.0f, 0.0f, 0.0f, 3.0f, 3.0f, 3.0f);
			CVec angle  = entry->_20;
			f32 sine    = sin(angle.y);
			f32 cosine  = cos(angle.y);
			f32 offsetX = 15.0f * sine;
			f32 offsetZ = 15.0f * cosine;
			DrawGsLineSegment(&gGame.mGs, NULL, entry->_10.x, entry->_10.y, entry->_10.z, 0xFFFF, entry->_10.x + offsetX, entry->_10.y,
			                  entry->_10.z + offsetZ, 0xFFFF);
		}
		DrawGsSphere(&gGame.mGs, colors[colorIndex], 12, 16, slot->_0C.x, slot->_0C.y, slot->_0C.z, 0.0f, 0.0f, 0.0f, 3.0f, 3.0f, 3.0f);
	}
}

/**
 * @note Address: 0x80136A54
 * @note Size: 0x77C
 */
extern "C" void DrawMapCameraAreaDebug3D(CCameraActionAreaEx* state, s32 mode)
{
	// callers only pass 0 or 1
	CamAreaRecord* records;
	s32 count;
	if (mode == 0) {
		records = state->_24;
		count   = state->_28;
	}
	if (mode == 1) {
		records = state->_1C;
		count   = state->_20;
	}
	if (records == NULL)
		return;
	u32 colors[7] = { 0xFF000064, 0x00FF0064, 0x0000FF64, 0xFFFF0064, 0xFF00FF64, 0xFFFFFF64, 0xFF960064 };
	f32 scale     = 2.0f / sqrtf(3.0f);
	for (s32 i = 0; i < count; ++i, ++records) {
		if (lbl_8065B324 == 0) {
			lbl_8065B320 = 0;
			lbl_8065B324 = 1;
		}
		CPad pad(3);
#if !defined(VERSION_PAL)
		if (pad.mPad->mTrigger & 2) {
			if (++lbl_8065B320 >= count)
				lbl_8065B320 = count - 1;
			OSReport("DEBUG:CCameraActionMap::DrawDebug3d():%d\n", lbl_8065B320);
		} else if (pad.mPad->mTrigger & 1) {
			if (--lbl_8065B320 <= 0)
				lbl_8065B320 = 0;
			OSReport("DEBUG:CCameraActionMap::DrawDebug3d():%d\n", lbl_8065B320);
		}
#endif
		lbl_8065B320 = i;
		CamAreaInfo area(records);
		if (area.IsDisabled())
			continue;
		CVec a     = AreaPoint(area, 0);
		CVec b     = AreaPoint(area, 1);
		CVec c     = AreaPoint(area, 2);
		CVec d     = AreaPoint(area, 3);
		f32 top    = area.GetTop();
		f32 bottom = area.GetBottom();
		Vec from, to, edge;
		from.x = b.x;
		from.y = top;
		from.z = b.z;
		to.x   = c.x;
		to.y   = top;
		to.z   = c.z;
		PSVECSubtract(&from, &to, &edge);
		PSVECMag(&edge);
		from.x = a.x;
		from.y = top;
		from.z = a.z;
		to.x   = b.x;
		to.y   = top;
		to.z   = b.z;
		PSVECSubtract(&to, &from, &edge);
		PSVECMag(&edge);
		f32 yaw;
		if (a.z - 0.01f < b.z && b.z < a.z + 0.01f) {
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
		center.y       = (top - bottom) * 0.5f + bottom;
		CVec axis      = CVec(1.0f, 0.0f, 0.0f);
		CVec direction = b - a;
		PSVECNormalize(&direction, &direction);
		DrawGsCube(&gGame.mGs, colors[lbl_8065B320 % 5], center.x, center.y, center.z, 0.0f, yaw, 0.0f, width, height, depth);
	}
}

static inline BOOL BothAreaTests(BOOL polygon, BOOL height)
{
	if (polygon && height)
		return TRUE;
	return FALSE;
}

static inline BOOL ContainsPoint(const CamAreaInfo& area, const Vec& pos)
{
	BOOL inside       = PointInQuad(pos, AreaPoint(area, 0), AreaPoint(area, 1), AreaPoint(area, 2), AreaPoint(area, 3));
	bool withinHeight = false;
	if (pos.y <= area.GetTop() && pos.y >= area.GetBottom())
		withinHeight = true;
	return BothAreaTests(inside, withinHeight != false);
}

/**
 * @note Address: 0x801371D0
 * @note Size: 0x324
 */
extern "C" s32 CollectCameraAreasContainingPoint(CCameraActionAreaEx* state, Vec pos, s32* ids)
{
	s32 count = 0;
	for (s32 i = 0; i < state->_20; ++i) {
		if (ContainsPoint(CamAreaInfo(&state->_1C[i]), pos)) {
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
	for (s32 i = 0; i < state->_20; ++i) {
		if (ContainsPoint(CamAreaInfo(&state->_1C[i]), pos)) {
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
	for (s32 i = 1; i < state->_28; ++i) {
		if (CamAreaInfo(&state->_24[i]).IsCameraOff())
			continue;
		if (ContainsPoint(CamAreaInfo(&state->_24[i]), pos)) {
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
	state->_08 = state->_04;
	Vec pos    = gGame.GetPlayer()->mMoveTranslate;
	s32 index  = -1;
	for (s32 i = 1; i < state->_28; ++i) {
		if (CamAreaInfo(&state->_24[i]).IsCameraOff())
			continue;
		if (ContainsPoint(CamAreaInfo(&state->_24[i]), pos)) {
			index = i;
			break;
		}
	}
	state->_04 = index;
	if (state->_08 != state->_04 && state->_04 != -1)
		state->_0C = state->_04;
}

/**
 * @note Address: 0x80137B58
 * @note Size: 0x1A8
 */
extern "C" void SetCameraAreaIndexAndBaseFov(CCameraActionAreaEx* state, s32 index)
{
	if (state->_24 != NULL && state->_2C != NULL) {
		state->_0C = index;
		state->_04 = index;
		state->_08 = index;
		f32 fov;
		if (AreaMode(state) == 1) {
			fov = CamInfo(&state->_2C[state->_2034[state->_0C]._00->_34]).GetFov();
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
extern "C" void UpdateCameraAreaIndexAndBaseFov(CCameraActionAreaEx* state, s32 unused)
{
	if (state->_24 != NULL && state->_2C != NULL) {
		UpdateCameraAreaIndexFromPlayer(state, unused);
		if (state->_08 != state->_04 && state->_04 != -1) {
			f32 fov;
			if (AreaMode(state) == 1) {
				fov = CamInfo(&state->_2C[state->_2034[state->_0C]._00->_34]).GetFov();
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
	return area._04->_00 == -1;
}

/**
 * @note Address: 0x80137EB8
 * @note Size: 0x164
 */
extern "C" void LoadStageCameraAreaState(CCameraActionAreaEx* state)
{
	state->_1C = NULL;
	state->_20 = 0;
	state->_1C = GetStageCameraAreaExRecords();
	s32 count  = 0;
	if (state->_1C != NULL) {
		for (;;) {
			if (IsAreaEnd(&state->_1C[count]))
				break;
			++count;
		}
	}
	state->_20 = count;
	state->_24 = NULL;
	state->_28 = 0;
	state->_24 = GetStageCameraAreaRecords();
	count      = 0;
	if (state->_24 != NULL) {
		for (;;) {
			if (IsAreaEnd(&state->_24[count]))
				break;
			++count;
		}
	}
	state->_28 = count;
	state->_2C = NULL;
	state->_2C = GetStageCameraInfoRecords();
	BuildCameraAreaGroups(state);
	state->_04      = -1;
	state->_08      = -1;
	state->_0C      = -1;
	state->mBaseFov = 30.5f;
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
		state->_30[i] = &gCameraAreaEntries[i];
	state->_18 = 1;
	state->_14 = 1;
}

static inline bool IsCameraEnd(CamInfoData* record)
{
	CamInfo camera(record);
	return camera.mData->_00 == -1;
}

/**
 * @note Address: 0x80135434
 * @note Size: 0x7E0
 */
extern "C" void BuildCameraAreaGroups(CCameraActionAreaEx* state)
{
	if (state->_2C == NULL)
		return;

	CamAreaEntry* entry = state->_30[0];
	for (s32 i = 0;; ++i, ++entry) {
		if (IsCameraEnd(&state->_2C[i]))
			break;
		entry->_00 = CamInfo(&state->_2C[i]).GetInterest();
		entry->_10 = CamInfo(&state->_2C[i]).GetEye();
		entry->_20 = CVec(0.0f, atan2(entry->_00.x - entry->_10.x, entry->_00.z - entry->_10.z), 0.0f);
		entry->_30 = CamInfo(&state->_2C[i]).GetFov();
		entry->_34 = i;
	}

	CamInfoData* data = state->_2C;
	CamAreaSlot* slot = state->_2034;
	s32 first         = 0;
	s32 group         = 0;
	for (;;) {
		if (data->_00 == -1)
			break;
		slot->_00 = state->_30[first];
		slot->_04 = group;
		s32 count = 0;
		while (data->_00 == group) {
			++count;
			++data;
		}
		slot->_08 = count;
		first += count;
		++slot;
		++group;
	}
	state->_2030 = group;

	for (s32 i = 0; i < state->_2030; ++i) {
		CamAreaSlot* slot = &state->_2034[i];
		switch (slot->_08) {
		case 0:
			break;
		case 1:
			slot->_0C = (Vec)slot->_00[0]._10;
			break;
		case 2: {
			CamAreaEntry* entries = slot->_00;
			slot->_0C             = (Vec)(entries[0]._10 + (entries[1]._10 - entries[0]._10) / 2.0f);
			break;
		}
		case 3: {
			CamAreaEntry* entries = slot->_00;
			CVec center           = (Vec)(entries[0]._10 + (entries[1]._10 - entries[0]._10) / 2.0f);
			slot->_0C             = center + (entries[2]._10 - center) / 3.0f;
			break;
		}
		case 4: {
			CamAreaEntry* entries = slot->_00;
			slot->_0C             = (Vec)(entries[0]._10 + (entries[2]._10 - entries[0]._10) / 2.0f);
			break;
		}
		default: {
			CamAreaEntry* entries = slot->_00;
			CVec center           = (Vec)(entries[0]._10 + (entries[1]._10 - entries[0]._10) / 2.0f);
			slot->_0C             = center + (entries[2]._10 - center) / 3.0f;
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

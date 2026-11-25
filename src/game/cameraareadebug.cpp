#include "game/ccameraaction.h"
#include "game/cgame.h"
#include "game/cgs.h"

#include <math.h>

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

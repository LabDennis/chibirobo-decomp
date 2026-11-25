#include "game/ccameraaction.h"
#include "game/caminfo.h"
#include "game/cgame.h"
#include "game/cmotioncontrolcamera.h"
#include "game/stagedata.h"
#include "game/xchr.h"

#include <math.h>

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

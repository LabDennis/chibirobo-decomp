#include "game/cgame.h"
#include "game/xobj.h"

namespace Car {
void Init();
}

extern "C" {
XObj* takiObj;

void EndFighter();
void RestoreDefaultCamera();
void DrawFighterHud();
void DrawCarHud(void* arg);
void DrawShots();
void DrawFighterStage();
void UpdateFighter(u32 arg);
void UpdateCars(u32 arg);
void ClearFighterObjects();
void HSD_AudioSFXSetPanByID(s32 id, s32 pan);
}

/**
 * @note Address: stage02 .text 0x6B9C
 * @note Size: 0x24
 */
extern "C" void ExitStage(void* arg)
{
	EndFighter();
	RestoreDefaultCamera();
}

/**
 * @note Address: stage02 .text 0x6BC0
 * @note Size: 0x34
 */
extern "C" void DrawStageHud(void* arg)
{
	DrawFighterHud();
	DrawCarHud(arg);
}

/**
 * @note Address: stage02 .text 0x6BF4
 * @note Size: 0x20
 */
extern "C" void DrawStageEffects(void* arg)
{
	DrawShots();
}

/**
 * @note Address: stage02 .text 0x6C14
 * @note Size: 0x20
 */
extern "C" void DrawStage(void* arg)
{
	DrawFighterStage();
}

/**
 * @note Address: stage02 .text 0x6C34
 * @note Size: 0x178
 */
extern "C" void UpdateStage(void* arg)
{
	UpdateFighter((u32)arg);
	UpdateCars((u32)arg);
	if (takiObj != NULL) {
		if (takiObj->mAnimId == 0) {
			XSound* sound = &gGame.mSound;
			if (IsGameSoundPlaying(sound, 0x20019) == 0) {
				PlayGameSound(sound, 0x20019, 0xFF);
				HSD_AudioSFXSetPanByID(0x20019, 0);
			}
			if (IsGameSoundPlaying(sound, 0x2001A) == 0) {
				PlayGameSound(sound, 0x2001A, 0xFF);
				HSD_AudioSFXSetPanByID(0x2001A, 0xFE);
			}
			SetGameSfxVolume(sound, 0x20019, GetSoundVolumeAtPosition(sound, &takiObj->mMoveTranslate, 1), 0);
			SetGameSfxVolume(sound, 0x2001A, GetSoundVolumeAtPosition(sound, &takiObj->mMoveTranslate, 1), 0);
		} else {
			XSound* sound = &gGame.mSound;
			StopGameSfx(sound, 0x20019);
			StopGameSfx(sound, 0x2001A);
		}
	}
}

/**
 * @note Address: stage02 .text 0x6DAC
 * @note Size: 0xB0
 */
extern "C" void InitStage(void* arg)
{
	XObj* obj;
	s32 count;
	s32 i;

	ClearFighterObjects();
	Car::Init();
	takiObj = NULL;
	count   = gGame.mXObjs.mObjectCount;
	for (i = 0; takiObj == NULL && i < count; i++) {
		obj = gGame.GetObject(i);
		if ((obj->mStatus.mFlags & 1) != 0 && obj->mCoord._00->_00 == 0x254) {
			takiObj = obj;
		}
	}
}

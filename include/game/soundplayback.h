#ifndef _GAME_SOUNDPLAYBACK_H
#define _GAME_SOUNDPLAYBACK_H

#include "game/xsound.h"
#include <hsd/sysdolphin/axdriver.h>

// guess: ids with high half 0xFFFF (but not -1) mean "every sfx", stops all slots
static inline bool isAllSfx(s32 sfx)
{
	if (sfx == -1) {
		return false;
	}
	return (sfx & 0xFFFF0000) == 0xFFFF0000;
}

// the every-sfx id goes to the master volume and remembers it, anything else to that one sfx
static inline void setSfxVolume(XSound* sound, s32 sfx, u8 vol, u16 frames)
{
	if (isAllSfx(sfx)) {
		sound->mMusicVolume = vol;
		AXDriverSetAllStreamVolumes(vol, frames);
	} else {
		HSD_AudioSFXSetVolumeByID(sfx, vol, frames);
	}
}

static inline void PauseMusic(XSound* sound, s32 frames)
{
	AXDriverPauseAllStreams();
	sound->mMusicResumeDelay = frames;
	sound->mPausedMusicId    = sound->mCurrentMusicId;
}

static inline BOOL stopSfx(XSound* sound, s32 sfx)
{
	if (isAllSfx(sfx)) {
		sound->mCurrentMusicId = 0;
		sound->mPausedMusicId  = 0;
		AXDriverStopAllStreams();
		return TRUE;
	}
	return AXDriverKeyOffSfx(sfx);
}

#endif

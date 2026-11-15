#ifndef GAME_WII_CSOUND_H
#define GAME_WII_CSOUND_H

#include "types.h"

class CSound {
public:
	// the player keeps 64 track slots of 0x20 bytes; the SFX commands walk
	// them for the stop-all / pause-all / resume-all paths
	struct TrackSlot {
		void* mTrack;
		u8 _04[0x1C];
	};
	STATIC_ASSERT(sizeof(TrackSlot) == 0x20);

	u8 mDvdSoundArchive[0x190]; // CDvdSoundArchive
	u8 mArchivePlayer[0xF4];    // CSoundArchivePlayer head
	TrackSlot mTracks[0x40];    // CSoundArchivePlayer track table (at _284)
	u8 mArchivePlayerTail[0x8F8 - 0xF4 - 0x800];
	u8 mSoundHeap[0x2C]; // CSoundHeap
	u32 mSoundHeapSize;
	u8 mLoadPending;

	virtual ~CSound();
	virtual BOOL OpenArchive(const char* path, s32 initialGroup);
	virtual BOOL OpenArchiveAsync(const char* path, s32 initialGroup);
	virtual BOOL LoadGroup(s32 groupId);
	virtual BOOL LoadGroupAsync(s32 groupId);
	virtual void RestoreHeapLevel(s32 level);

	u8 mSound3DManager[0xA0]; // CSound3DManager
};
STATIC_ASSERT(sizeof(CSound) == 0xB60);

class XSound : public CSound {
public:
	u8 mPlaybackState[0x113C - 0xB60]; // Begins with CSoundFxReverbHiDpl2.
	f32 mMusicVolume;
};
STATIC_ASSERT(sizeof(XSound) == 0x1140);

extern "C" {
s32 PlayWiiSoundEffect(XSound* sound, s32 id, f32 volume, f32 pan);
s32 IsWiiSfxPlaying(XSound* sound, s32 id);                        // 0x80037B04
void StopWiiSfx(XSound* sound, s32 id, f32 fade);                  // 0x8003993C
void PauseWiiSfx(XSound* sound, s32 id, f32 fade);                 // 0x80039AA8
void ResumeWiiSfx(XSound* sound, s32 id, f32 fade);                // 0x80039BD4
void SetWiiSfxVolume(XSound* sound, s32 id, f32 volume, f32 fade); // 0x80039DB4
BOOL LoadWiiSoundBankByName(XSound* sound, const char* name);
// SoundArchivePlayer track helpers used by the SFX -2 (all tracks) paths
void StopWiiSfxTrack(void* track, u32 frames);                  // 0x8028A04C
s32 IsWiiSfxTrackPaused(void* track);                           // 0x8028A59C
void SetWiiSfxTrackPaused(void* track, u32 paused, u32 frames); // 0x8028A234
}

#endif

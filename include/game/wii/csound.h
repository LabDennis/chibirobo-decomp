#ifndef GAME_WII_CSOUND_H
#define GAME_WII_CSOUND_H

#include "types.h"

class CSound {
public:
	u8 mDvdSoundArchive[0x190]; // CDvdSoundArchive
	u8 mArchivePlayer[0x8F8];   // CSoundArchivePlayer
	u8 mSoundHeap[0x2C];        // CSoundHeap
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
BOOL LoadWiiSoundBankByName(XSound* sound, const char* name);
}

#endif

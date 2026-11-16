#ifndef GAME_WII_CSOUND_H
#define GAME_WII_CSOUND_H

#include "types.h"
#include "Dolphin/mtx.h"

class CSound;

// one mixer track: current/target/step for volume and pan, faded by
// UpdateGameSound every frame; the SFX commands only reach mTrack
struct CSoundTrackSlot {
	void* mTrack;      // _00
	f32 mVolume;       // _04
	f32 mVolumeTarget; // _08
	f32 mVolumeStep;   // _0C
	f32 mPan;          // _10
	f32 mPanTarget;    // _14
	f32 mPanStep;      // _18
	u8 _1C[4];
};
STATIC_ASSERT(sizeof(CSoundTrackSlot) == 0x20);

// the nw4r archive player seen through the offsets this unit touches; CSound
// holds its bytes flat, with the track table broken out as its own member
class CSoundArchivePlayer {
public:
	u8 _000[0xEC];
	void* mSetupData;  // _EC
	void* mSetupData2; // _F0
	u8 _F4[0x800];
	u32 mReady; // _8F4
};
STATIC_ASSERT(sizeof(CSoundArchivePlayer) == 0x8F8);

// the embedded 3D context at CSound::_AC0, linking back to owner and player
class CSound3DManager {
public:
	CSound* mOwner; // _00
	void* mPlayer;  // _04
	u8 _08[0x98];

	void Init(CSound* owner, void* player);
};
STATIC_ASSERT(sizeof(CSound3DManager) == 0xA0);

// a 3D sound source the manager registers; member names are ours
class CSound3D {
public:
	void* mArg; // _00
	u8 _04[4];
	u8 mActor[0x0C]; // _08 nw4r actor data (fn_802958C0/fn_80295914)
	u8 mNode[0x10];  // _14 link node; the insert passes &mNode[4]
	u32 mFlags;      // _24
	u8 _28[0x0C];
	u8 mSound[0x64]; // _34 nw4r sound handle
	u8 mList[4];     // _98

	void Init(void* arg);
	void SetListener(f32 angle, const Vec* pos);
	void SetVolumePanPitch(f32 volume, f32 pan, f32 pitch);
};

class CSound {
public:
	u8 mDvdSoundArchive[0x190];                  // CDvdSoundArchive: path open, size@_124, data@_18C
	u8 mArchivePlayer[0xF4];                     // CSoundArchivePlayer head
	CSoundTrackSlot mTracks[0x40];               // the player's 64-track table
	u8 mArchivePlayerTail[0x8F8 - 0xF4 - 0x800]; // through _A88
	u8 mSoundHeap[0x2C];                         // CSoundHeap
	u32 mSoundHeapSize;
	u8 mLoadPending;

	virtual ~CSound();
	virtual BOOL OpenArchive(const char* path, s32 initialGroup);
	virtual BOOL OpenArchiveAsync(const char* path, s32 initialGroup);
	virtual BOOL LoadGroup(s32 groupId);
	virtual BOOL LoadGroupAsync(s32 groupId);
	virtual void RestoreHeapLevel(s32 level);
	BOOL OpenArchiveFile(const char* path); // reads "<path>.brsar" into the archive's heap buffer

	CSound3DManager mSound3DManager; // _AC0
};
STATIC_ASSERT(sizeof(CSound) == 0xB60);

class XSound : public CSound {
public:
	u8 mPlaybackState[0x113C - 0xB60]; // Begins with CSoundFxReverbHiDpl2.
	f32 mMusicVolume;
};
STATIC_ASSERT(sizeof(XSound) == 0x1140);

extern "C" {
void InitAudio(CSound* sound, u32 heapSize);
void ShutdownAudio();
void UpdateGameSound(CSound* sound, u32 step);
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

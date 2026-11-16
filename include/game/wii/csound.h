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

	// mTrack is an nw4r::snd::SoundHandle (one pointer to the sound); these are its two inlines
	// (Stop is defined where it is used, behind nw4r/snd/snd_BasicSound.h)
	bool IsAttachedSound() const { return mTrack != NULL; }
	void Stop(int fadeFrames);
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

	// stops every playing track over fade seconds; Reset and the message script's SFX -2 both expand it.
	// Defined in gamemain.cpp: the files do not agree on gRefreshRate's type yet, so it cannot stand here
	void StopAllTracks(f32 fade);
};
STATIC_ASSERT(sizeof(CSound) == 0xB60);

class XSound : public CSound {
public:
	// the archive calls go straight to CSound; retail still has them as XSound's own vtable entries
	virtual BOOL OpenArchive(const char* path, s32 initialGroup);
	virtual BOOL OpenArchiveAsync(const char* path, s32 initialGroup);
	virtual BOOL LoadGroup(s32 groupId);
	virtual BOOL LoadGroupAsync(s32 groupId);

	u8 mPlaybackState[0xF74 - 0xB60]; // Begins with CSoundFxReverbHiDpl2.
	s32 mStartedSfx[32];              // _F74, ids started since the last update; zero means free
	s32 mMusicPitchFrames;            // _FF4
	s32 mAltSoundIndex;               // _FF8, the GameCube XSound's fields of the same names from here on
	s32 mVoiceSeqIndex;               // _FFC
	s32 mSoundSeqIndex;               // _1000
	s32 mCooldown;                    // _1004
	u8 _1008[0x100C - 0x1008];
	u8 mQueueRead;  // _100C
	u8 mQueueWrite; // _100D
	u8 _100E[0x1018 - 0x100E];
	s32 mQueuedSoundIds[64]; // _1018
	s32 mLastQueuedSoundId;  // _1118
	u8 _111C[0x1120 - 0x111C];
	s32 mCurrentMusicId;     // _1120
	f32 mMusicPitch;         // _1124, in cents; a float on the Wii
	s16 mTargetMusicPitch;   // _1128
	s32 _112C;               // _112C, written 0 when a music track starts
	s32 mMusicResumeDelay;   // _1130
	s32 mMusicFadeOutFrames; // _1134
	s32 mPausedMusicId;      // _1138
	f32 mMusicVolume;
};
STATIC_ASSERT(sizeof(XSound) == 0x1140);
#ifndef offsetof
#define offsetof(type, member) ((u32) & (((type*)0)->member))
#endif
STATIC_ASSERT(offsetof(XSound, mStartedSfx) == 0xF74);
STATIC_ASSERT(offsetof(XSound, mAltSoundIndex) == 0xFF8);
STATIC_ASSERT(offsetof(XSound, mQueuedSoundIds) == 0x1018);
STATIC_ASSERT(offsetof(XSound, mMusicPitch) == 0x1124);
STATIC_ASSERT(offsetof(XSound, mQueueRead) == 0x100C);
STATIC_ASSERT(offsetof(XSound, mCurrentMusicId) == 0x1120);
STATIC_ASSERT(offsetof(XSound, mPausedMusicId) == 0x1138);

extern "C" {
void InitAudio(CSound* sound, u32 heapSize);
void ShutdownAudio(CSound* sound);
void UpdateGameSound(CSound* sound, u32 step);
s32 PlayWiiSoundEffect(XSound* sound, s32 id, f32 volume, f32 pan);
s32 IsWiiSfxPlaying(XSound* sound, s32 id);                        // 0x80037B04
void StopWiiSfx(XSound* sound, s32 id, f32 fade);                  // 0x8003993C
void PauseWiiSfx(XSound* sound, s32 id, f32 fade);                 // 0x80039AA8
void ResumeWiiSfx(XSound* sound, s32 id, f32 fade);                // 0x80039BD4
void SetWiiSfxVolume(XSound* sound, s32 id, f32 volume, f32 fade); // 0x80039DB4
BOOL LoadWiiSoundBankByName(XSound* sound, const char* name);
// gamesound.cpp; the slot results are CSound::mTracks entries
void UpdateXSound(XSound* sound, s32 step);                        // 0x800366EC
CSoundTrackSlot* GetGameSoundSlot(XSound* sound, s32 sfx);         // 0x800394B0
CSoundTrackSlot* GetGameMusicSlot(XSound* sound);                  // 0x80039924
void SetGameSfxPitch(XSound* sound, s32 sfx, f32 cents, f32 fade); // 0x80039FDC
void FadeInGameSfx(XSound* sound, s32 sfx, f32 fade);              // 0x8003A13C
void FadeOutGameSfx(XSound* sound, s32 sfx, f32 fade);             // 0x8003A60C
}

#endif

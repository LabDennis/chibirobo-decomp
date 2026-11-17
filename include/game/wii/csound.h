#ifndef GAME_WII_CSOUND_H
#define GAME_WII_CSOUND_H

#include "types.h"
#include "Dolphin/mtx.h"
// units that already have the shared Dolphin <dvd.h> keep its DVD types; the nw4r archive header would redeclare them
#ifdef _DOLPHIN_DVD_H
#ifndef _REVOLUTION_DVD_DVD_H
#define _REVOLUTION_DVD_DVD_H
#endif
#endif
#include "nw4r/snd/snd_DvdSoundArchive.h"
#include "nw4r/snd/snd_Sound3DListener.h"
#include "nw4r/snd/snd_Sound3DManager.h"
#include "nw4r/snd/snd_SoundArchivePlayer.h"
#include "nw4r/snd/snd_SoundHandle.h"
#include "nw4r/snd/snd_SoundHeap.h"
#include "game/wii/csoundfx.h"

class CSound;

// a sound handle with the volume and pitch it fades to. The class has no RTTI, the name is ours.
// CSound keeps 64 of them, one per sound it can play at once
class CSoundHandle : public nw4r::snd::SoundHandle {
public:
	f32 mVolume;       // _04
	f32 mVolumeTarget; // _08
	f32 mVolumeStep;   // _0C
	f32 mPitch;        // _10
	f32 mPitchTarget;  // _14
	f32 mPitchStep;    // _18
	u8 _1C[4];

	// frames of 0 set the value at once, otherwise UpdateGameSound steps to it
	void SetVolume(f32 volume, s32 frames)
	{
		mVolumeTarget = volume;
		if (frames == 0 || mVolumeTarget == mVolume) {
			mVolume = mVolumeTarget;
		} else {
			mVolumeStep = (mVolumeTarget - mVolume) / frames;
		}
		nw4r::snd::SoundHandle::SetVolume(mVolume, 0);
	}
	void SetPitch(f32 pitch, s32 frames)
	{
		mPitchTarget = pitch;
		if (frames == 0 || mPitchTarget == mPitch) {
			mPitch = mPitchTarget;
		} else {
			mPitchStep = (mPitchTarget - mPitch) / frames;
		}
		nw4r::snd::SoundHandle::SetPitch(mPitch);
	}
	// one frame (of step) of the fades SetVolume/SetPitch started
	void UpdateVolume(u32 step)
	{
		if (IsAttachedSound()) {
			if (mVolume < mVolumeTarget) {
				mVolume += mVolumeStep * step;
				if (mVolume > mVolumeTarget) {
					mVolume = mVolumeTarget;
				}
				nw4r::snd::SoundHandle::SetVolume(mVolume, 0);
			} else if (mVolume > mVolumeTarget) {
				mVolume += mVolumeStep * step;
				if (mVolume < mVolumeTarget) {
					mVolume = mVolumeTarget;
				}
				nw4r::snd::SoundHandle::SetVolume(mVolume, 0);
			}
		}
	}
	void UpdatePitch(u32 step)
	{
		if (IsAttachedSound()) {
			if (mPitch < mPitchTarget) {
				mPitch += mPitchStep * step;
				if (mPitch > mPitchTarget) {
					mPitch = mPitchTarget;
				}
				nw4r::snd::SoundHandle::SetPitch(mPitch);
			} else if (mPitch > mPitchTarget) {
				mPitch += mPitchStep * step;
				if (mPitch < mPitchTarget) {
					mPitch = mPitchTarget;
				}
				nw4r::snd::SoundHandle::SetPitch(mPitch);
			}
		}
	}
	f32 GetVolume() const { return mVolume; }
	f32 GetPitch() const { return mPitch; }
	f32 GetVolumeTarget() const { return mVolumeTarget; }
	f32 GetPitchTarget() const { return mPitchTarget; }
	void Pause(s32 frames)
	{
		if (!IsPause()) {
			nw4r::snd::SoundHandle::Pause(true, frames);
		}
	}
	void Resume(s32 frames)
	{
		if (IsPause()) {
			nw4r::snd::SoundHandle::Pause(false, frames);
		}
	}
};
STATIC_ASSERT(sizeof(CSoundHandle) == 0x20);

class CDvdSoundArchive : public nw4r::snd::DvdSoundArchive {
public:
	void* mData; // _18C
};
STATIC_ASSERT(sizeof(CDvdSoundArchive) == 0x190);

class CSoundArchivePlayer : public nw4r::snd::SoundArchivePlayer {
public:
	void* mSetupData;           // _EC
	void* mSetupData2;          // _F0
	CSoundHandle mTracks[0x40]; // _F4
	u32 mReady;                 // _8F4
};
STATIC_ASSERT(sizeof(CSoundArchivePlayer) == 0x8F8);

class CSoundHeap : public nw4r::snd::SoundHeap { };
STATIC_ASSERT(sizeof(CSoundHeap) == 0x2C);

class CSound3DManager : public nw4r::snd::Sound3DManager { };
STATIC_ASSERT(sizeof(CSound3DManager) == 0x2C);

// the embedded 3D context at CSound::_AC0, linking back to owner and player
class WiiSound3D {
public:
	CSound* mOwner;                       // _00
	void* mPlayer;                        // _04
	CSound3DManager mManager;             // _08
	nw4r::snd::Sound3DListener mListener; // _34

	void Init(CSound* owner, void* player);
};
STATIC_ASSERT(sizeof(WiiSound3D) == 0xA0);

// the WiiSound3D context as its 3D manager/listener calls see it; member names are ours
class CSound3D {
public:
	const nw4r::snd::SoundArchive* mArchive; // _00 the owner CSound's archive
	void* mPlayer;                           // _04
	CSound3DManager mManager;                // _08
	nw4r::snd::Sound3DListener mListener;    // _34

	void Init(void* arg);
	void SetListener(f32 angle, const Vec* pos);
	void SetVolumePanPitch(f32 volume, f32 pan, f32 pitch);
};

class CSound {
public:
	CDvdSoundArchive mDvdSoundArchive;  // _000
	CSoundArchivePlayer mArchivePlayer; // _190
	CSoundHeap mSoundHeap;              // _A88
	u32 mSoundHeapSize;                 // _AB4
	u8 mLoadPending;                    // _AB8

	virtual ~CSound() { }
	virtual BOOL OpenArchive(const char* path, s32 initialGroup);
	virtual BOOL OpenArchiveAsync(const char* path, s32 initialGroup);
	virtual BOOL LoadGroup(s32 groupId);
	virtual BOOL LoadGroupAsync(s32 groupId);
	virtual void RestoreHeapLevel(s32 level);
	BOOL OpenArchiveFile(const char* path); // reads "<path>.brsar" into the archive's heap buffer

	WiiSound3D mSound3DManager; // _AC0

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

	CSoundFxReverbHiDpl2 mReverb; // _B60
	u8 mPlaybackState[0xF74 - 0xB60 - sizeof(CSoundFxReverbHiDpl2)];
	s32 mStartedSfx[32];   // _F74, ids started since the last update; zero means free
	s32 mMusicPitchFrames; // _FF4
	s32 mAltSoundIndex;    // _FF8, the GameCube XSound's fields of the same names from here on
	s32 mVoiceSeqIndex;    // _FFC
	s32 mSoundSeqIndex;    // _1000
	s32 mCooldown;         // _1004
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
CSoundHandle* GetGameSoundSlot(XSound* sound, s32 sfx);            // 0x800394B0
CSoundHandle* GetGameMusicSlot(XSound* sound);                     // 0x80039924
void SetGameSfxPitch(XSound* sound, s32 sfx, f32 cents, f32 fade); // 0x80039FDC
void FadeInGameSfx(XSound* sound, s32 sfx, f32 fade);              // 0x8003A13C
void FadeOutGameSfx(XSound* sound, s32 sfx, f32 fade);             // 0x8003A60C
}

#endif

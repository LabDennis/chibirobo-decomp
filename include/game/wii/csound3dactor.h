#ifndef GAME_WII_CSOUND3DACTOR_H
#define GAME_WII_CSOUND3DACTOR_H

#include "types.h"
#include "nw4r/snd/snd_SoundHandle.h"
#include "nw4r/snd/snd_Sound3DActor.h"

// a sound handle with the volume and pitch it fades to. The class has no RTTI, the name is ours.
// todo: CSound's 64 CSoundTrackSlot (csound.h) are this class; that header still spells them flat
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

// a positioned sound source with the one sound it plays, class name from RTTI
class CSound3DActor : public nw4r::snd::Sound3DActor {
public:
	CSound3DActor(nw4r::snd::SoundArchivePlayer& player, nw4r::snd::Sound3DManager& manager)
	    : nw4r::snd::Sound3DActor(player, manager)
	{
	}
	virtual ~CSound3DActor(); // _08

	// steps the volume and pitch towards the handle's targets
	void UpdateFade(f32 step)
	{
		if (mHandle.GetVolume() != mHandle.GetVolumeTarget()) {
			if (mHandle.IsAttachedSound()) {
				if (mHandle.mVolume < mHandle.mVolumeTarget) {
					mHandle.mVolume += mHandle.mVolumeStep * step;
					if (mHandle.mVolume > mHandle.mVolumeTarget) {
						mHandle.mVolume = mHandle.mVolumeTarget;
					}
				} else if (mHandle.mVolume > mHandle.mVolumeTarget) {
					mHandle.mVolume += mHandle.mVolumeStep * step;
					if (mHandle.mVolume < mHandle.mVolumeTarget) {
						mHandle.mVolume = mHandle.mVolumeTarget;
					}
				}
			}
			SetVolume(mHandle.mVolume);
		}
		if (mHandle.GetPitch() != mHandle.GetPitchTarget()) {
			if (mHandle.IsAttachedSound()) {
				if (mHandle.mPitch < mHandle.mPitchTarget) {
					mHandle.mPitch += mHandle.mPitchStep * step;
					if (mHandle.mPitch > mHandle.mPitchTarget) {
						mHandle.mPitch = mHandle.mPitchTarget;
					}
				} else if (mHandle.mPitch > mHandle.mPitchTarget) {
					mHandle.mPitch += mHandle.mPitchStep * step;
					if (mHandle.mPitch < mHandle.mPitchTarget) {
						mHandle.mPitch = mHandle.mPitchTarget;
					}
				}
			}
			SetPitch(mHandle.mPitch);
		}
	}

	CSoundHandle mHandle; // _80
};
STATIC_ASSERT(sizeof(CSound3DActor) == 0xA0);

#endif

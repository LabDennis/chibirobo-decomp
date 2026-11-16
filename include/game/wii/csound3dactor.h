#ifndef GAME_WII_CSOUND3DACTOR_H
#define GAME_WII_CSOUND3DACTOR_H

#include "types.h"
#include "game/wii/csound.h"
#include "nw4r/snd/snd_Sound3DActor.h"

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

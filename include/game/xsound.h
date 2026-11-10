#ifndef _GAME_XSOUND_H
#define _GAME_XSOUND_H

#include "types.h"
#include "game/csound.h"
#include "game/cfile.h"

extern "C" s32 HSD_SynthSFXGetPendingLoadCount();

inline bool HasPendingSoundBankLoad()
{
	bool pending = HSD_SynthSFXGetPendingLoadCount();
	return pending;
}

// one sfx name for the stage bank
struct SfxLoadRequest {
	const char* mName; // _00
	s32 mBank;         // _04
	s32 mResult;       // _08
	u8 mLoaded;        // _0C
};
STATIC_ASSERT(sizeof(SfxLoadRequest) == 0x10);

struct StageSoundRequests {
	u8 _00[0x10];
	SfxLoadRequest entries[16];

	void Clear()
	{
		for (u32 i = 0; i < 16; ++i) {
			SfxLoadRequest* request = &entries[i];
			request->mName          = NULL;
			request->mResult        = -1;
			request->mLoaded        = 0;
		}
	}
};
STATIC_ASSERT(sizeof(StageSoundRequests) == 0x110);

// the sound player CGame keeps at +0xE7E24, class name from RTTI. CmdSetSoundPitch sets mTargetMusicPitch
// to the pitch target in cents and _188 to the number of frames to get there
class XSound : public CSound {
public:
	CFile mFile;                       // _08, the sample table is read through it
	s32 mSampleBank;                   // _F4
	u8 mSampleReady;                   // _F8
	u8 _F9[0xFC - 0xF9];               // _F9
	s32 mDefaultTrack;                 // _FC, SetMusicPlaybackPaused also writes 1 or 0 here
	s32 mStageBank;                    // _100, the stage's sfx bank, cleared on every stage change
	u8 _104[0x108 - 0x104];            // _104
	s32 _108[32];                      // _108, ids that PlayGameSound started, zero means free
	s32 _188;                          // _188, pitch interpolation and queued sound delay
	s32 _18C;                          // _18C, alternating sound sequence index
	s32 _190;                          // _190, voice sequence index
	s32 _194;                          // _194, sound sequence index
	s32 _198;                          // _198, cooldown
	u8 _19C[0x1A0 - 0x19C];            // _19C
	u8 mQueueRead;                     // _1A0
	u8 mQueueWrite;                    // _1A1
	u8 _1A2[0x1AC - 0x1A2];            // _1A2
	s32 mQueuedSoundIds[64];           // _1AC, queued sound ids
	s32 mLastQueuedSoundId;            // _2AC, last queued sound
	u8 _2B0[0x2B4 - 0x2B0];            // _2B0
	s32 mCurrentMusicId;               // _2B4
	s16 mMusicPitch;                   // _2B8, current music pitch
	s16 mTargetMusicPitch;             // _2BA, target music pitch
	s32 _2BC;                          // _2BC
	s32 mMusicResumeDelay;             // _2C0
	s32 mMusicFadeOutFrames;           // _2C4, fade length, frames * 60 / 200
	s32 mPausedMusicId;                // _2C8
	u8 mMusicVolume;                   // _2CC, volume for the all-sfx id
	u8 _2CD[0x2D0 - 0x2CD];            // _2CD
	StageSoundRequests mStageRequests; // _2D0, stage sfx names waiting for HSD_SynthSFXLoad
	s32 mLoadRequestCount;             // _3E0

	inline void registerName(const char* name, s32 bank);
	inline s32 resolveTrack(s32 track);

	// stops the sfx if it is still playing
	inline void StopIfPlaying(s32 sfx);
};
STATIC_ASSERT(sizeof(XSound) == 0x3E4);

inline void XSound::StopIfPlaying(s32 sfx)
{
	if (IsGameSoundPlaying(this, sfx)) {
		StopGameSfx(this, sfx);
	}
}

#endif

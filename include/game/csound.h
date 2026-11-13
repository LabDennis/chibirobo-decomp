#ifndef _GAME_CSOUND_H
#define _GAME_CSOUND_H

#include "types.h"

struct Vec;
class XSound;

// the sound player's base, class name from RTTI. CGame's XSound has the rest
class CSound {
public:
	virtual ~CSound() { } // _08

	s32 mLoadState; // _04, LoadGameSoundBank's step
};
STATIC_ASSERT(sizeof(CSound) == 0x8);

extern "C" {
extern f32 gSoundFalloffRanges[5][4];
#if defined(VERSION_JP)
extern s32 gSoundMenuEffects[10];
extern s32 gSoundMenuSelections[5];
#endif

// XSound, sound.cpp. sfx ids from 0xFFFF0000 up are music tracks
void PlayMessageGlyphSound(XSound* sound, u32 group, f32 strength, u8 first, u8 second);
u8 GetSoundVolumeAtPosition(XSound* sound, Vec* pos, s32 kind);
void UpdateGameSound(XSound* sound, s32 step);
BOOL RegisterGameSoundName(XSound* sound, const char* name);
void FadeOutGameSfx(XSound* sound, s32 sfx, u16 frames);
void FadeInGameSfx(XSound* sound, s32 sfx, u16 frames);
void SetGameSfxVolume(XSound* sound, s32 sfx, u8 vol, u16 frames);
void ResumeGameSoundTrack(XSound* sound, s32 track);
void ResumeGameSfx(XSound* sound, s32 sfx);
void PauseGameSoundTrack(XSound* sound, s32 track);
void PauseGameMusic(XSound* sound, s32 sfx, s32 resumeDelay);
void KeyOffGameSoundTrack(XSound* sound, s32 track);
// stop, is it playing, and play, which gives back the voice or stream id
BOOL StopGameSfx(XSound* sound, s32 sfx);
BOOL IsGameSoundPlaying(XSound* sound, s32 sfx);
s32 PlayGameSound(XSound* sound, s32 sfx, u8 vol);

// hsd axdriver.c: key off an sfx, a pal only one, pause an sfx, stop every stream slot
BOOL AXDriverKeyOffSfx(s32 sfx);
BOOL HSD_AudioSFXKeyOffAll(void);
BOOL AXDriverPauseSfx(s32 sfx);
void AXDriverStopAllStreams(void);
}

// one sound an anim plays between two frames, a negative mAnim ends an object's table
struct AnimSfxEvent {
	s32 mAnim;  // _00
	f32 mStart; // _04
	f32 mEnd;   // _08
	s32 mKind;  // _0C, 1 plays when the frame passes mStart or mEnd
	s32 mSfx;   // _10, a track id from 0xFFFF0000 up
	u8 _14;     // _14, no access in the source, only filled by the data tables
	u8 mCount;  // _15, mSfx and the next ones are played in turn
	u8 mIndex;  // _16
	u8 _17;     // _17, no access in the source, only filled by the data tables
};
STATIC_ASSERT(sizeof(AnimSfxEvent) == 0x18);

// plays an anim's sound events as its frames pass, sound.cpp, vtable at 0x802112A8
class AnimSfx {
public:
	AnimSfxEvent* mEvents; // _00, ends at an anim id below 0
	s32 mAnimId;           // _04
	f32 mFrame;            // _08, the last frame Update saw
	// _0C vtable

	virtual void vf08();                                            // _08, vf08__7AnimSfxFv
	virtual void Update(s32 id, f32 frame, u8 volume);              // _0C, Update__7AnimSfxFlfUc
	virtual BOOL CheckStart(AnimSfxEvent* event, f32 from, f32 to); // _10, CheckStart__7AnimSfxFP12AnimSfxEventff, starts the event
	virtual BOOL CheckStop(AnimSfxEvent* event);                    // _14, CheckStop__7AnimSfxFP12AnimSfxEvent, stops it
	virtual s32 GetSfx(AnimSfxEvent* event);                        // _18, GetSfx__7AnimSfxFP12AnimSfxEvent, its sfx
};
STATIC_ASSERT(sizeof(AnimSfx) == 0x10);

#endif

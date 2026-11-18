// The game's sound front end, the Wii port of the GameCube's sound.cpp: the same functions, on
// nw4r::snd instead of HSD, in the GameCube file's reverse order. Music ids are 0x00FFxxxx here
// (0xFFFFxxxx on the GameCube), volumes are 0..1 floats and times are seconds. After the front end
// come the sound banks, the sound picks PlayWiiSoundEffect makes, the message glyph sounds, the stage
// reverb and the sounds of an object's anims (AnimSfx).
// before cgame.h: xobj.h's nw4r::math stand-in gives way to the real one
#include "nw4r/math/math_types.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/wii/csoundfx.h"
#include "game/soundtracks.h"
#include "nw4r/snd/snd_SoundStartable.h"
#include "nw4r/snd/snd_SoundHandle.h"
#include "stl/stdlib.h"
#include "types.h"
#include <stdlib.h>
#include "Dolphin/mtx.h"
#include "game/wii/csound3dactor.h"
#include "game/wii/animsfx.h"
#include "nw4r/snd/snd_AxManager.h"
#include "nw4r/snd/snd_SoundArchivePlayer.h"
#include "nw4r/snd/snd_Sound3DManager.h"
#include <string.h>

extern s32 gRefreshRate;
// the frames a started sound takes to reach its volume and pitch; nothing in retail writes it
extern s32 gSoundFadeFrames;
// the reverb's parameters (defined with the stage effects below)
extern nw4r::snd::detail::FxReverbHiParam gSoundReverbParam;

extern "C" {
BOOL IsLiftTargetPlug(XAction* action);
BOOL IsPlayerCarryingPlug(WiiPlayerState* player);
void InitGameSound(XSound* sound, u32 heapSize);

// defined further down
BOOL CanPlayNightStream(XSound* sound);
s32 SelectFootstepSound(XSound* sound);
s32 SelectAlternatingSound7E(XSound* sound);
s32 SelectAlternatingSound82(XSound* sound);
s32 SelectVoiceSequenceSound(XSound* sound);
s32 SelectCookingSequenceSound(XSound* sound);
s32 SelectSequenceSound1A4(XSound* sound);
s32 SelectVariantSound(XSound* sound, u32 sfx);
s32 SelectMappedSound(s32 sfx);
s32 PauseMusicForJingle(XSound* sound, s32 sfx);
}

// shortcut: csound.h still holds the archive player as raw bytes; these two give its parts their
// types. Drop them once CSound::mArchivePlayer is a nw4r::snd::SoundArchivePlayer.
// the player's SoundStartable base, behind its 0xC of DisposeCallback: snd_SoundArchivePlayer.h
// does not compile with the game's include paths (<new>, <climits>)
static inline nw4r::snd::SoundStartable& ArchivePlayer(CSound* sound)
{
	return static_cast<nw4r::snd::SoundStartable&>(sound->mArchivePlayer);
}

static inline s32 OutputLineFlag(CSound* sound)
{
	return *(s32*)&sound->mArchivePlayer.mReady;
}

bool isAllSfx(XSound* sound, s32 sfx);
s32 SecondsToFrames(f32 seconds);
f32 CentsToPitch(f32 cents);
s32 Random(s32 max);
static inline void SetSlotVolume(CSoundHandle* slot, f32 volume, s32 frames, BOOL apply);
void SetSlotPitch(CSoundHandle* slot, f32 pitch, s32 frames, BOOL apply);
CSoundHandle* FindSound(CSound* sound, u32 id);
CSoundHandle* StartSound(CSound* sound, u32 id, f32 volume, s32 fadeFrames);
static inline void StopSound(XSound* sound, u32 id, s32 fadeFrames);
static inline void ResumeSound(XSound* sound, u32 id, s32 fadeFrames);
static inline void SetSoundVolume(XSound* sound, u32 id, f32 volume, s32 frames);
static inline void SetSoundPitch(XSound* sound, u32 id, f32 pitch, s32 frames);
static inline void PauseSound(XSound* sound, u32 id);

/**
 * @note Address: 0x80036694
 * @note Size: 0x48
 */
extern "C" void InitGameSound(XSound* sound, u32 heapSize)
{
	InitAudio(sound, heapSize);
	// the reverb sits at the head of mPlaybackState; retail calls it through its CSoundFx base
	CSoundFx* fx = &sound->mFx.mReverb;
	fx->Setup(gSoundReverbParam);
}

/**
 * @note Address: 0x800366DC
 * @note Size: 0x4
 */
BOOL XSound::OpenArchive(const char* path, s32 initialGroup)
{
	return CSound::OpenArchive(path, initialGroup);
}

/**
 * @note Address: 0x800366E0
 * @note Size: 0x4
 */
BOOL XSound::OpenArchiveAsync(const char* path, s32 initialGroup)
{
	return CSound::OpenArchiveAsync(path, initialGroup);
}

/**
 * @note Address: 0x800366E4
 * @note Size: 0x4
 */
BOOL XSound::LoadGroup(s32 groupId)
{
	return CSound::LoadGroup(groupId);
}

/**
 * @note Address: 0x800366E8
 * @note Size: 0x4
 */
BOOL XSound::LoadGroupAsync(s32 groupId)
{
	return CSound::LoadGroupAsync(groupId);
}

/**
 * @note Address: 0x800366EC
 * @note Size: 0x8FC
 */
// The XSound half of the frame's sound update; CGame::Update calls it. The name is ours, the
// GameCube's UpdateGameSound is the counterpart (that name is the CSound update's here).
// NONMATCHING: retail clears mStartedSfx in a counted loop of four rounds with eight stores each
// (0x80036738), this compiler writes the 32 stores out; for, while, do-while, pointer and nested
// forms were tried.
extern "C" void UpdateXSound(XSound* sound, s32 step)
{
	UpdateGameSound(sound, step);
	for (int i = 0; i < 32; i++) {
		sound->mStartedSfx[i] = 0;
	}
	if (sound->mCurrentMusicId != 0 && sound->mPausedMusicId == 0 && FindSound(sound, sound->mCurrentMusicId & 0xFFFF) == NULL) {
		sound->mCurrentMusicId = 0;
	}
	const s32 pitchFrames = sound->mMusicPitchFrames;
	if (pitchFrames > 0 && sound->mMusicPitch != sound->mTargetMusicPitch && !gGame.mHomeMenu.IsActive() && !gGame.mMenu.mActive) {
		sound->mMusicPitch += (sound->mTargetMusicPitch - sound->mMusicPitch) / pitchFrames;
		sound->mMusicPitchFrames -= step;
		if (sound->mMusicPitchFrames <= 0) {
			sound->mMusicPitch = sound->mTargetMusicPitch;
		}
		SetGameSfxPitch(sound, sound->mCurrentMusicId & 0xFFFF, sound->mMusicPitch, 0.0f);
	}
	if (sound->mCurrentMusicId == 0xFF0005) {
		if (IsPlayerCarryingPlug(gGame.GetPlayer(0))) {
			if (sound->mTargetMusicPitch != 200) {
				sound->mTargetMusicPitch = 200;
				sound->mMusicPitchFrames = 15;
			}
		} else if (sound->mTargetMusicPitch != 0) {
			sound->mTargetMusicPitch = 0;
			sound->mMusicPitchFrames = 15;
		}
	}
	// paused music comes back when mMusicResumeDelay runs out
	if (sound->mMusicResumeDelay > 0 && !gGame.mHomeMenu.IsActive() && !gGame.mMenu.mActive) {
		if (!IsWiiSfxPlaying(sound, 0x1F9) && !IsWiiSfxPlaying(sound, 0x1FA) && !IsWiiSfxPlaying(sound, 0x1FB)) {
			sound->mMusicResumeDelay -= step;
		}
		if (sound->mMusicResumeDelay <= 0) {
			ResumeWiiSfx(sound, 0xFF0000, 1.0f);
		}
	}
	if (sound->mCurrentMusicId == 0xFF0010 && !CanPlayNightStream(sound)) {
		if (sound->mMusicFadeOutFrames <= 0) {
			FadeOutGameSfx(sound, 0xFF0000, 3.0f);
		}
	}
	if (sound->mMusicFadeOutFrames > 0) {
		sound->mMusicFadeOutFrames -= step;
		if (sound->mMusicFadeOutFrames <= 0) {
			StopWiiSfx(sound, 0xFF0000, 0.0f);
		}
	}
	if (sound->mCooldown > 0 && !gGame.mHomeMenu.IsActive() && !gGame.mMenu.mActive) {
		sound->mCooldown -= step;
	}
	if (sound->mQueueRead != sound->mQueueWrite && !gGame.mHomeMenu.IsActive() && !gGame.mMenu.mActive) {
		if (sound->mMusicPitchFrames > 0) {
			sound->mMusicPitchFrames -= step;
		}
		if (sound->mLastQueuedSoundId != 0 && IsWiiSfxPlaying(sound, sound->mLastQueuedSoundId)) {
			return;
		}
		if (sound->mMusicPitchFrames <= 0) {
			sound->mLastQueuedSoundId = sound->mQueuedSoundIds[sound->mQueueRead];
			PlayWiiSoundEffect(&gGame.mSound, sound->mLastQueuedSoundId, 1.0f, 0.0f);
			if (++sound->mQueueRead >= 64) {
				sound->mQueueRead = 0;
			}
		}
	}
}

/**
 * @note Address: 0x80036FE8
 * @note Size: 0x100
 */
// the slot playing the archive sound id, or NULL
CSoundHandle* FindSound(CSound* sound, u32 id)
{
	return sound->FindTrack(id);
}

/**
 * @note Address: 0x800370E8
 * @note Size: 0x4C
 */
// NONMATCHING: retail loads mLift.mState ahead of the flag's li r31, 0 (0x800370F4), ours behind
// it. A state local, an inline taking the state, an explicit flag and a ternary all give the same.
extern "C" BOOL IsPlayerCarryingPlug(WiiPlayerState* player)
{
	return player->mLift.mState == 4 && IsLiftTargetPlug(&player->mLift);
}

/**
 * @note Address: 0x8003714C
 * @note Size: 0x640
 */
// returns the slot the sound started in, 0 when it did not start and 0x80000000 when the music
// asked for needs no start; csound.h declares the result as s32 and calls the fade time pan
// It calls isAllSfx, CGame::GetTimeMode, FindSound, CGame::GetPlayer(s32), IsPlayerCarryingPlug,
// SecondsToFrames, StartSound, CentsToPitch, SetSlotPitch and Random rather than expanding them, which is
// why those have out-of-line copies around it: in the whole file the compiler's inline budget runs out here.
extern "C" s32 PlayWiiSoundEffect(XSound* sound, s32 sfx, f32 volume, f32 fade)
{
	CSoundHandle* slot;
	if (isAllSfx(sound, sfx)) {
		if (sfx == 0xFF0000) {
			return 0;
		}
		// at night (time mode 1) the day music, musicTable[5], gives way to musicTable[0x10]
		if (sfx == 0xFF0005 && gGame.GetTimeMode() == 1) {
			sfx = 0xFF0010;
		}
		if (sfx == 0xFF0010 && !CanPlayNightStream(sound)) {
			StopWiiSfx(sound, sfx, 0.0f);
			return 0x80000000;
		}
		u8 index = sfx & 0xFF;
		if (index >= 0x6D) {
			index = 0;
		}
		MusicEntry* music = musicTable + index;
		if (sound->mCurrentMusicId == sfx) {
			return 0x80000000;
		}
		if (sound->mCurrentMusicId != 0 && FindSound(sound, sound->mCurrentMusicId & 0xFFFF) != NULL) {
			StopWiiSfx(sound, sound->mCurrentMusicId, 0.0f);
		}
		sound->mCurrentMusicId = sfx;
		if (sfx == 0xFF0005 && IsPlayerCarryingPlug(gGame.GetPlayer(0))) {
			sound->mMusicPitch = sound->mTargetMusicPitch = 200;
		} else {
			sound->mMusicPitch = sound->mTargetMusicPitch = 0;
		}
		sound->_112C               = 0;
		sound->mMusicFadeOutFrames = 0;
		// 1 asks for the track's own volume from musicTable
		if (1.0f == volume) {
			volume = music->volume / 255.0f;
		}
		sound->mMusicVolume = volume;
		slot                = StartSound(sound, index, volume, SecondsToFrames(fade));
		SetSlotPitch(slot, CentsToPitch(sound->mMusicPitch), 0, TRUE);
		if (sfx == 0xFF0005 || sfx == 0xFF0010) {
			if (IsWiiSfxPlaying(sound, 0x1F9) || IsWiiSfxPlaying(sound, 0x1FA) || IsWiiSfxPlaying(sound, 0x1FB)) {
				PauseWiiSfx(sound, sound->mCurrentMusicId, 2.0f);
			}
		}
		return (s32)slot;
	}

	switch (sfx) {
	case 0x88:
		sfx += Random(4);
		break;
	case 0x86:
		sfx = SelectFootstepSound(sound);
		if (sfx < 0) {
			return 0;
		}
		if (sound->mMusicResumeDelay > 10) {
			sound->mMusicResumeDelay = 10;
		}
		break;
	case 0x77:
		// with volume 0 it is started silent
		volume = 0.0f;
		break;
	case 0xAB:
		// restarts the turn order of 0x7E's and 0x82's sets
		sound->mAltSoundIndex = 0;
		break;
	case 0x7E:
		sfx = SelectAlternatingSound7E(sound);
		if (sfx < 0) {
			return 0;
		}
		break;
	case 0x82:
		sfx = SelectAlternatingSound82(sound);
		if (sfx < 0) {
			return 0;
		}
		break;
	case 0xA9:
		if (IsWiiSfxPlaying(sound, sfx)) {
			return 0;
		}
		break;
	case 0x16A:
		sfx = SelectVoiceSequenceSound(sound);
		break;
	case 0x17D:
		sfx = SelectCookingSequenceSound(sound);
		if (sfx < 0) {
			return 0;
		}
		break;
	case 0x1E2:
	case 0x1E3:
	case 0x1E4:
	case 0x1E5:
		sfx = SelectVariantSound(sound, sfx);
		break;
	case 0x1A4:
		sfx = SelectSequenceSound1A4(sound);
		break;
	case 0xF1:
	case 0xF2:
	case 0xF3:
	case 0xF4:
	case 0xF5:
		sfx = SelectMappedSound(sfx);
		break;
	case 0x18A:
		// at most once in 60 frames, UpdateXSound counts mCooldown down
		if (sound->mCooldown > 0) {
			return 0;
		}
		sound->mCooldown = 60;
		break;
	case 0xB1:
	case 0x1F9:
	case 0x1FA:
	case 0x1FB:
		sfx = PauseMusicForJingle(sound, sfx);
		break;
	case 0x520:
		if (IsWiiSfxPlaying(sound, sfx)) {
			return 0;
		}
		break;
	case 0x1F3:
		if (IsWiiSfxPlaying(sound, sfx)) {
			return 0;
		}
		break;
	case 0x1F1:
		if (IsWiiSfxPlaying(sound, sfx)) {
			return 0;
		}
		break;
	case 0x294:
	case 0x295:
		if (IsWiiSfxPlaying(sound, sfx)) {
			return 0;
		}
		break;
	case 0x1FD:
	case 0x1FE:
	case 0x503:
		if (IsWiiSfxPlaying(sound, sfx)) {
			return 0;
		}
		break;
	}

	slot  = StartSound(sound, sfx, volume, SecondsToFrames(fade));
	u32 i = 0;
	while (i < 32 && sound->mStartedSfx[i] != 0) {
		i++;
	}
	if (i < 32) {
		sound->mStartedSfx[i] = sfx;
	}
	switch (sfx) {
	case 0x76:
		SetSlotPitch(slot, CentsToPitch((s16)(60 - Random(20) * 6)), 0, TRUE);
		break;
	}
	return (s32)slot;
}

/**
 * @note Address: 0x8003778C
 * @note Size: 0x28
 */
// ids with 0xFF in the third byte (but not -1) are music; retail's copy takes the unused sound too
bool isAllSfx(XSound* sound, s32 sfx)
{
	if (sfx == -1) {
		return false;
	}
	return (sfx & 0x00FF0000) == 0x00FF0000;
}

static inline CSoundHandle* FindFreeSlot(CSound* sound)
{
	for (int i = 0; i < 0x40; i++) {
		if (!sound->mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()) {
			return &sound->mArchivePlayer.mTracks.mHandles[i];
		}
	}
	return NULL;
}

// the slot's volume moves to its target over frames; UpdateGameSound steps it
static inline void SetSlotVolume(CSoundHandle* slot, f32 volume, s32 frames, BOOL apply)
{
	slot->mVolumeTarget = volume;
	if (frames == 0 || slot->mVolumeTarget == slot->mVolume) {
		slot->mVolume = slot->mVolumeTarget;
	} else {
		slot->mVolumeStep = (slot->mVolumeTarget - slot->mVolume) / frames;
	}
	if (apply) {
		slot->nw4r::snd::SoundHandle::SetVolume(slot->mVolume, 0);
	}
}

/**
 * @note Address: 0x800377C0
 * @note Size: 0x1F8
 */
// NONMATCHING: retail stores the volume target ahead of the compare of gSoundFadeFrames (0x8003786C), ours after
CSoundHandle* StartSound(CSound* sound, u32 id, f32 volume, s32 fadeFrames)
{
	CSoundHandle* slot = FindFreeSlot(sound);
	if (slot != NULL) {
		if (ArchivePlayer(sound).StartSound(&(*slot), id)) {
			slot->SetOutputLineFlag(OutputLineFlag(sound));
			SetSlotVolume(slot, volume, gSoundFadeFrames, TRUE);
			SetSlotPitch(slot, 1.0f, gSoundFadeFrames, TRUE);
			if (OutputLineFlag(sound) & 2) {
				slot->SetRemoteOutVolume(0, 0.5f);
			}
			if (fadeFrames > 0) {
				slot->FadeIn(fadeFrames);
			}
		} else {
			slot = NULL;
		}
	} else {
		slot = NULL;
	}
	return slot;
}

/**
 * @note Address: 0x800379B8
 * @note Size: 0x8C
 */
// csound.h calls these three fields pan; what they hold goes to SetPitch
void SetSlotPitch(CSoundHandle* slot, f32 pitch, s32 frames, BOOL apply)
{
	slot->mPitchTarget = pitch;
	if (frames == 0 || slot->mPitchTarget == slot->mPitch) {
		slot->mPitch = slot->mPitchTarget;
	} else {
		slot->mPitchStep = (slot->mPitchTarget - slot->mPitch) / frames;
	}
	if (apply) {
		slot->nw4r::snd::SoundHandle::SetPitch(slot->mPitch);
	}
}

/**
 * @note Address: 0x80037A44
 * @note Size: 0x3C
 */
s32 SecondsToFrames(f32 seconds)
{
	return gRefreshRate * seconds;
}

/**
 * @note Address: 0x80037A80
 * @note Size: 0x4C
 */
// a pitch offset in cents as a playback rate
f32 CentsToPitch(f32 cents)
{
	f32 octave = 1200.0f;
	if (cents > 0.0f) {
		return 1.0f + 0.6666667f * (cents / octave);
	}
	if (cents < 0.0f) {
		return 1.0f / (1.0f + 0.6666667f * (-cents / octave));
	}
	return 1.0f;
}

/**
 * @note Address: 0x80037ACC
 * @note Size: 0x38
 */
s32 Random(s32 max)
{
	return rand() % max;
}

/**
 * @note Address: 0x80037B04
 * @note Size: 0x19AC
 */
// returns the slot the sound plays in; csound.h declares the result as s32
extern "C" s32 IsWiiSfxPlaying(XSound* sound, s32 sfx)
{
	// steps by two, so only even slots are searched, as on the GameCube
	for (u32 i = 0; i < 32 && sound->mStartedSfx[i] != 0; i += 2) {
		if (sound->mStartedSfx[i] == sfx) {
			return (s32)sound->FindTrack(sfx);
		}
	}

	if (isAllSfx(sound, sfx)) {
		if (sound->mCurrentMusicId == sfx) {
			return (s32)sound->FindTrack(sound->mCurrentMusicId & 0xFFFF);
		}
		if (sfx == 0xFF0000 && sound->mCurrentMusicId != 0) {
			return (s32)sound->FindTrack(sound->mCurrentMusicId & 0xFFFF);
		}
		if (sfx == 0xFF0005 && sound->mCurrentMusicId != 0) {
			return (s32)sound->FindTrack(sound->mCurrentMusicId & 0xFFFF);
		}
		return 0;
	}

	CSoundHandle* slot;
	// 0x88 and 0x16A stand for every sound PlayWiiSoundEffect picks for them
	switch (sfx) {
	case 0x88:
		if ((slot = sound->FindTrack(sfx)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(sfx + 1)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(sfx + 2)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(sfx + 3)) != NULL) {
			return (s32)slot;
		}
		break;
	case 0x16A:
		if ((slot = sound->FindTrack(0x16A)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x16B)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x16C)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x16D)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x16E)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x16F)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x170)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x171)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x172)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x173)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x174)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x175)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x176)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x177)) != NULL) {
			return (s32)slot;
		}
		if ((slot = sound->FindTrack(0x178)) != NULL) {
			return (s32)slot;
		}
		break;
	default:
		if ((slot = sound->FindTrack(sfx)) != NULL) {
			return (s32)slot;
		}
		break;
	}
	return (s32)slot;
}

/**
 * @note Address: 0x800394B0
 * @note Size: 0x474
 */
// the slot a sound plays in, without IsWiiSfxPlaying's shortcuts; CEvt's SFX command asks it
extern "C" CSoundHandle* GetGameSoundSlot(XSound* sound, s32 sfx)
{
	if (isAllSfx(sound, sfx)) {
		if (sound->mCurrentMusicId == sfx) {
			return FindSound(sound, sound->mCurrentMusicId & 0xFFFF);
		}
		// 0xFF0000 asks for whatever music is playing
		if (sfx == 0xFF0000 && sound->mCurrentMusicId != 0) {
			return FindSound(sound, sound->mCurrentMusicId & 0xFFFF);
		}
		if (sfx == 0xFF0005 && sound->mCurrentMusicId != 0) {
			return FindSound(sound, sound->mCurrentMusicId & 0xFFFF);
		}
		return NULL;
	}
	return FindSound(sound, sfx);
}

/**
 * @note Address: 0x80039924
 * @note Size: 0x18
 */
// NONMATCHING: retail leaves GetGameSoundSlot a call here (a tail branch); this compiler expands
// it, in either order of the two and with or without -ipa file
extern "C" CSoundHandle* GetGameMusicSlot(XSound* sound)
{
	if (sound->mCurrentMusicId != 0) {
		return GetGameSoundSlot(sound, sound->mCurrentMusicId);
	}
	return NULL;
}

static inline void StopSound(XSound* sound, u32 id, s32 fadeFrames)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mArchivePlayer.mTracks.mHandles[i].IsAttachedSound() && sound->mArchivePlayer.mTracks.mHandles[i].GetId() == id) {
			sound->mArchivePlayer.mTracks.mHandles[i].Stop(fadeFrames);
		}
	}
}

/**
 * @note Address: 0x8003993C
 * @note Size: 0x16C
 */
extern "C" void StopWiiSfx(XSound* sound, s32 sfx, f32 fade)
{
	if (isAllSfx(sound, sfx)) {
		StopSound(sound, sound->mCurrentMusicId & 0xFFFF, gRefreshRate * fade);
		sound->mCurrentMusicId = 0;
		sound->mPausedMusicId  = 0;
	} else {
		StopSound(sound, sfx, gRefreshRate * fade);
	}
}

static inline void PauseSound(XSound* sound, u32 id)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mArchivePlayer.mTracks.mHandles[i].IsAttachedSound() && sound->mArchivePlayer.mTracks.mHandles[i].GetId() == id) {
			if (!sound->mArchivePlayer.mTracks.mHandles[i].IsPause()) {
				sound->mArchivePlayer.mTracks.mHandles[i].nw4r::snd::SoundHandle::Pause(true, 0);
			}
		}
	}
}

/**
 * @note Address: 0x80039AA8
 * @note Size: 0x12C
 */
// the music comes back after resumeDelay seconds; 0 never runs out
extern "C" void PauseWiiSfx(XSound* sound, s32 sfx, f32 resumeDelay)
{
	if (isAllSfx(sound, sfx)) {
		PauseSound(sound, sound->mCurrentMusicId & 0xFFFF);
		sound->mMusicResumeDelay = gRefreshRate * resumeDelay;
		sound->mPausedMusicId    = sound->mCurrentMusicId;
	}
}

static inline void ResumeSound(XSound* sound, u32 id, s32 fadeFrames)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mArchivePlayer.mTracks.mHandles[i].IsAttachedSound() && sound->mArchivePlayer.mTracks.mHandles[i].GetId() == id) {
			if (sound->mArchivePlayer.mTracks.mHandles[i].IsPause()) {
				sound->mArchivePlayer.mTracks.mHandles[i].nw4r::snd::SoundHandle::Pause(false, fadeFrames);
			}
		}
	}
}

/**
 * @note Address: 0x80039BD4
 * @note Size: 0x1E0
 */
extern "C" void ResumeWiiSfx(XSound* sound, s32 sfx, f32 fade)
{
	if (isAllSfx(sound, sfx)) {
		if (sound->mPausedMusicId == sound->mCurrentMusicId) {
			ResumeSound(sound, sound->mPausedMusicId & 0xFFFF, gRefreshRate * fade);
		} else {
			PlayWiiSoundEffect(sound, sfx, 1.0f, 0.0f);
		}
		sound->mPausedMusicId = 0;
	} else {
		ResumeSound(sound, sfx & 0xFFFF, gRefreshRate * fade);
	}
}

static inline void SetSoundVolume(XSound* sound, u32 id, f32 volume, s32 frames)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mArchivePlayer.mTracks.mHandles[i].IsAttachedSound() && sound->mArchivePlayer.mTracks.mHandles[i].GetId() == id) {
			SetSlotVolume(&sound->mArchivePlayer.mTracks.mHandles[i], volume, frames, TRUE);
		}
	}
}

/**
 * @note Address: 0x80039DB4
 * @note Size: 0x228
 */
// a music id moves the playing music, anything else that one sfx
extern "C" void SetWiiSfxVolume(XSound* sound, s32 sfx, f32 volume, f32 fade)
{
	if (isAllSfx(sound, sfx)) {
		SetSoundVolume(sound, sound->mCurrentMusicId & 0xFFFF, volume, SecondsToFrames(fade));
	} else {
		SetSoundVolume(sound, sfx & 0xFFFF, volume, SecondsToFrames(fade));
	}
}

static inline void SetSoundPitch(XSound* sound, u32 id, f32 pitch, s32 frames)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mArchivePlayer.mTracks.mHandles[i].IsAttachedSound() && sound->mArchivePlayer.mTracks.mHandles[i].GetId() == id) {
			SetSlotPitch(&sound->mArchivePlayer.mTracks.mHandles[i], pitch, frames, TRUE);
		}
	}
}

/**
 * @note Address: 0x80039FDC
 * @note Size: 0x160
 */
extern "C" void SetGameSfxPitch(XSound* sound, s32 sfx, f32 cents, f32 fade)
{
	SetSoundPitch(sound, sfx, CentsToPitch(cents), SecondsToFrames(fade));
}

// a music id's volume from musicTable, 0 for anything that isn't music
static inline f32 getMusicVolume(XSound* sound, s32 sfx)
{
	if (!isAllSfx(sound, sfx)) {
		return 0.0f;
	}
	u8 idx = sfx;
	if (idx > 0x6D) {
		idx = 0;
	}
	return musicTable[idx].volume / 255.0f;
}

/**
 * @note Address: 0x8003A13C
 * @note Size: 0x4D0
 */
extern "C" void FadeInGameSfx(XSound* sound, s32 sfx, f32 fade)
{
	if (isAllSfx(sound, sfx)) {
		PlayWiiSoundEffect(sound, sfx, 0.0f, 0.0f);
		SetWiiSfxVolume(sound, sfx, getMusicVolume(sound, sfx), fade);
	} else {
		if (!IsWiiSfxPlaying(sound, sfx)) {
			PlayWiiSoundEffect(sound, sfx, 1.0f, 0.0f);
		}
		SetWiiSfxVolume(sound, sfx, 1.0f, fade);
	}
}

/**
 * @note Address: 0x8003A60C
 * @note Size: 0x44C
 */
extern "C" void FadeOutGameSfx(XSound* sound, s32 sfx, f32 fade)
{
	if (isAllSfx(sound, sfx)) {
		sound->mMusicFadeOutFrames = fade * gRefreshRate;
		SetWiiSfxVolume(sound, sfx, 0.0f, fade);
	} else {
		SetWiiSfxVolume(sound, sfx, 0.0f, fade);
	}
}

struct WiiSoundBank {
	const char* name;
	s32 groupId;
};

WiiSoundBank gWiiSoundBanks[] = {
	{ "sfx_kitchen.ssm", 1 },
	{ "sfx_stage02.ssm", 2 },
	{ "sfx_souko.ssm", 3 },
	{ "sfx_jennyroom.ssm", 4 },
	{ "sfx_stage05.ssm", 5 },
	{ "sfx_papamama.ssm", 6 },
	{ "sfx_stage07.ssm", 7 },
	{ "sfx_uraniwa.ssm", 8 },
	{ "sfx_pipe.ssm", 9 },
	{ "sfx_manual.ssm", 10 },
	{ "sfx_ufo.ssm", 11 },
	{ "sfx_martial.ssm", 12 },
	{ "sfx_hock.ssm", 13 },
	{ "sfx_gicco.ssm", 14 },
	{ "sfx_bb.ssm", 15 },
	{ "sfx_sanpoo.ssm", 16 },
	{ "sfx_trex.ssm", 17 },
	{ "sfx_army.ssm", 18 },
	{ "sfx_kaeru.ssm", 19 },
	{ "sfx_uraniwa_ambient1.ssm", 20 },
	{ "sfx_uraniwa_ambient2.ssm", 21 },
	{ "sfx_uraniwa_ambient3.ssm", 22 },
	{ "sfx_concert.ssm", 23 },
	{ "sfx_ending.ssm", 24 },
	{ NULL, 0 },
};

/**
 * @note Address: 0x8003AA58
 * @note Size: 0xA8
 */
extern "C" BOOL LoadWiiSoundBankByName(XSound* sound, const char* name)
{
	for (int i = 0; gWiiSoundBanks[i].name; ++i) {
		if (strcmp(name, gWiiSoundBanks[i].name) == 0)
			return sound->LoadGroupAsync(gWiiSoundBanks[i].groupId);
	}
	return FALSE;
}

// the reverb's parameters: InitGameSound sets it up with the library's defaults, ApplyStageSoundEffects
// changes them for the stage
nw4r::snd::detail::FxReverbHiParam gSoundReverbParam;

// GetSoundVolumeAtPosition's distances from the camera's interest for each kind, farthest first:
// past [0] a sound is silent, within [3] it is at full volume
f32 gSoundFalloffRanges[5][4] = {
	{ 100.0f, 75.0f, 50.0f, 25.0f }, { 300.0f, 200.0f, 200.0f, 150.0f }, { 75.0f, 40.0f, 30.0f, 25.0f },
	{ 30.0f, 20.0f, 10.0f, 5.0f },   { 600.0f, 300.0f, 200.0f, 150.0f },
};

// five footstep sounds for each kind of floor
static s32 sSurfaceSoundIds[13][5] = {
	{ 0x8E, 0x8F, 0x90, 0x91, 0x92 }, { 0x93, 0x94, 0x95, 0x96, 0x97 }, { 0x8E, 0x8F, 0x90, 0x91, 0x92 }, { 0x98, 0x99, 0x9A, 0x9B, 0x9C },
	{ 0x9D, 0x9E, 0x9F, 0xA0, 0xA1 }, { 0x8E, 0x8F, 0x90, 0x91, 0x92 }, { 0x8E, 0x8F, 0x90, 0x91, 0x92 }, { 0x98, 0x99, 0x9A, 0x9B, 0x9C },
	{ 0x98, 0x99, 0x9A, 0x9B, 0x9C }, { 0x8E, 0x8F, 0x90, 0x91, 0x92 }, { 0x9D, 0x9E, 0x9F, 0xA0, 0xA1 }, { 0x98, 0x99, 0x9A, 0x9B, 0x9C },
	{ 0x8E, 0x8F, 0x90, 0x91, 0x92 },
};

static s32 sAlternatingSound7E[4]      = { 0x7E, 0x7F, 0x80, 0x81 };
static s32 sAlternatingSound82[4]      = { 0x82, 0x83, 0x84, 0x85 };
static s32 sSound16ASequence[25]       = { 0x16A, 0x16B, 0x16C, 0x16D, 0x16A, 0x16B, 0x16C, 0x16D, 0x16E, 0x16E, 0x16F, 0x16F, 0x170,
	                                       0x170, 0x171, 0x172, 0x173, 0x16E, 0x174, 0x16F, 0x175, 0x170, 0x176, 0x177, 0x178 };
static s32 sSound17DSequence[8]        = { 0x17D, 0x17E, 0x17F, 0x180, 0x181, 0x182, 0x183, 0x184 };
static s32 sSound1A4Sequence[11]       = { 0x1A4, 0x1A5, 0x1A6, 0x1A7, 0x1A8, 0x1A9, 0x1AA, 0x1AB, 0x1AC, 0x1AD, 0x1AE };
static s32 sSound1E3Variants[6]        = { 0x1E3, 0x1E4, 0x1E5, 0x1E6, 0x1E7, 0x1E8 };
static s32 sSound1E2Variants[6]        = { 0x1DC, 0x1DD, 0x1DE, 0x1DF, 0x1E0, 0x1E1 };
static s32 sMessageGroup0SoundIds[7]   = { 0x12E, 0x12F, 0x130, 0x131, 0x132, 0x133, 0x134 };
static s32 sMessageGroup2SoundIds[10]  = { 0xFF, 0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108 };
static s32 sMessageGroup4SoundIds[10]  = { 0x10B, 0x10C, 0x10D, 0x10E, 0x10F, 0x110, 0x111, 0x112, 0x113, 0x114 };
static s32 sMessageGroup3SoundIds[10]  = { 0x124, 0x125, 0x126, 0x127, 0x128, 0x129, 0x12A, 0x12B, 0x12C, 0x12D };
static s32 sMessageGroup9SoundIds[10]  = { 0x504, 0x505, 0x506, 0x507, 0x508, 0x509, 0x50A, 0x50B, 0x50C, 0x50D };
static s32 sMessageGroup8SoundIds[5]   = { 0x47F, 0x480, 0x481, 0x482, 0x483 };
static s32 sMessageGroup7SoundIds[10]  = { 0x491, 0x492, 0x493, 0x494, 0x495, 0x496, 0x497, 0x498, 0x499, 0x49A };
static s32 sMessageGroup10SoundIds[5]  = { 0xBD, 0xBE, 0xBF, 0xC0, 0xC1 };
static s32 sMessageGroup29SoundIds[6]  = { 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7 };
static s32 sMessageGroup11SoundIds[5]  = { 0x4AB, 0x4AC, 0x4AD, 0x4AE, 0x4AF };
static s32 sMessageGroup12SoundIds[6]  = { 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD };
static s32 sMessageGroup13SoundIds[10] = { 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7 };
static s32 sMessageGroup15SoundIds[12] = { 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3 };
static s32 sMessageGroup16SoundIds[10] = { 0x135, 0x136, 0x137, 0x138, 0x139, 0x13A, 0x13B, 0x13C, 0x13D, 0x13E };
static s32 sMessageGroup17SoundIds[10] = { 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED };
static s32 sMessageGroup19SoundIds[10] = { 0x4E4, 0x4E5, 0x4E6, 0x4E7, 0x4E8, 0x4E9, 0x4EA, 0x4EB, 0x4EC, 0x4ED };
static s32 sMessageGroup20SoundIds[10] = { 0x145, 0x146, 0x147, 0x148, 0x149, 0x14A, 0x14B, 0x14C, 0x14D, 0x14E };
static s32 sMessageGroup21SoundIds[10] = { 0x14F, 0x150, 0x151, 0x152, 0x153, 0x154, 0x155, 0x156, 0x157, 0x158 };
static s32 sMessageGroup22SoundIds[10] = { 0x4BC, 0x4BD, 0x4BE, 0x4BF, 0x4C0, 0x4C1, 0x4C2, 0x4C3, 0x4C4, 0x4C5 };
static s32 sMessageGroup23SoundIds[10] = { 0x4C6, 0x4C7, 0x4C8, 0x4C9, 0x4CA, 0x4CB, 0x4CC, 0x4CD, 0x4CE, 0x4CF };
static s32 sMessageGroup18SoundIds[10] = { 0x1B9, 0x1BA, 0x1BB, 0x1BC, 0x1BD, 0x1BE, 0x1BF, 0x1C0, 0x1C1, 0x1C2 };
static s32 sMessageGroup1SoundIds[5]   = { 0x1C3, 0x1C4, 0x1C5, 0x1C6, 0x1C7 };
static s32 sMessageGroup24SoundIds[6]  = { 0x4EE, 0x4EF, 0x4F0, 0x4F1, 0x4F2, 0x4F3 };
static s32 sMessageGroup27SoundIds[10] = { 0x1D0, 0x1D1, 0x1D2, 0x1D3, 0x1D4, 0x1D5, 0x1D6, 0x1D7, 0x1D8, 0x1D9 };
static s32 sMessageGroup25SoundIds[11] = { 0x53A, 0x53B, 0x53C, 0x53D, 0x53E, 0x53F, 0x540, 0x541, 0x542, 0x543, 0x544 };
static s32 sMessageGroup26SoundIds[12] = { 0x545, 0x546, 0x547, 0x548, 0x549, 0x54A, 0x54B, 0x54C, 0x54D, 0x54E, 0x54F, 0x550 };
static s32 sMessageGroup28SoundIds[6]  = { 0x448, 0x449, 0x44A, 0x44B, 0x44C, 0x44D };
static s32 sMessageGroup30SoundIds[8]  = { 0x40B, 0x40C, 0x40D, 0x40E, 0x40F, 0x410, 0x411, 0x412 };
static s32 sMessageGroup31SoundIds[4]  = { 0x21A, 0x21B, 0x21C, 0x21D };
static s32 sMessageGroup32SoundIds[4]  = { 0x21E, 0x21F, 0x220, 0x221 };
static s32 sMessageGroup33SoundIds[6]  = { 0x3E5, 0x3E6, 0x3E7, 0x3E8, 0x3E9, 0x3EA };

static s32 sMessageGroup14SoundIds[2] = { 0x109, 0x10A };

/**
 * @note Address: 0x8003AB00
 * @note Size: 0x12C
 */
// how loud a sound at pos is, 0 to 1
extern "C" f32 GetSoundVolumeAtPosition(XSound* sound, Vec* pos, s32 kind)
{
	f32 dist   = PSVECDistance(&gGame.mMotionControlCamera.mView.mInterest, pos);
	f32* range = gSoundFalloffRanges[kind];
	u8 volume;

	if (range[0] < dist) {
		volume = 0;
	} else if (range[1] < dist) {
		volume = 127.0f - (64.0f * (dist - range[1])) / (range[0] - range[1]);
	} else if (range[2] < dist) {
		volume = 171.0f - (64.0f * (dist - range[2])) / (range[1] - range[2]);
	} else if (range[3] < dist) {
		volume = 255.0f - (64.0f * (dist - range[3])) / (range[2] - range[3]);
	} else {
		volume = 255;
	}
	return volume / 255.0f;
}

/**
 * @note Address: 0x8003AC2C
 * @note Size: 0x38
 */
// bgm_night does not start in the second half of the night
extern "C" BOOL CanPlayNightStream(XSound* sound)
{
	if (gGame.mTimeState.mMode == 1 && gGame.mTimeState.mTime > 0.5f) {
		return FALSE;
	}
	return TRUE;
}

/**
 * @note Address: 0x8003AC64
 * @note Size: 0x518
 */
// one of the five footstep sounds of the floor chibi robo walks on, -1 on a silent one; three kinds of
// floor play a sound of their own under it
extern "C" s32 SelectFootstepSound(XSound* sound)
{
	s32 surface = gGame.GetPlayer()->mGroundType;
	if (surface >= 13) {
		surface = 0;
	}
	if (surface == 12) {
		return -1;
	}
	switch (surface) {
	case 8:
		StartSound(sound, 0x20A, 70.0f, 0);
		break;
	case 9:
		StartSound(sound, 0x209, 1.0f, 0);
		break;
	case 10:
		StartSound(sound, 0x20B, 50.0f, 0);
		break;
	}
	return sSurfaceSoundIds[surface][rand() % 5];
}

/**
 * @note Address: 0x8003B17C
 * @note Size: 0x230
 */
extern "C" s32 SelectAlternatingSound7E(XSound* sound)
{
	for (u32 i = 0; i < 4; i++) {
		if (IsWiiSfxPlaying(sound, sAlternatingSound82[i])) {
			StopWiiSfx(sound, sAlternatingSound82[i], 0.0f);
			sound->mAltSoundIndex = 0;
		}
	}
	for (u32 i = 0; i < 4; i++) {
		if (IsWiiSfxPlaying(sound, sAlternatingSound7E[i])) {
			return -1;
		}
	}
	if (sound->mAltSoundIndex < 0 || (u32)sound->mAltSoundIndex >= 4) {
		sound->mAltSoundIndex = 0;
	}
	return sAlternatingSound7E[sound->mAltSoundIndex++];
}

/**
 * @note Address: 0x8003B3AC
 * @note Size: 0x230
 */
// 0x7E and 0x82 each play the four sounds of their set in turn: starting one set stops the other, and a set
// does not start again while one of its own sounds still plays
extern "C" s32 SelectAlternatingSound82(XSound* sound)
{
	for (u32 i = 0; i < 4; i++) {
		if (IsWiiSfxPlaying(sound, sAlternatingSound7E[i])) {
			StopWiiSfx(sound, sAlternatingSound7E[i], 0.0f);
			sound->mAltSoundIndex = 0;
		}
	}
	for (u32 i = 0; i < 4; i++) {
		if (IsWiiSfxPlaying(sound, sAlternatingSound82[i])) {
			return -1;
		}
	}
	if (sound->mAltSoundIndex < 0 || (u32)sound->mAltSoundIndex >= 4) {
		sound->mAltSoundIndex = 0;
	}
	return sAlternatingSound82[sound->mAltSoundIndex++];
}

/**
 * @note Address: 0x8003B5DC
 * @note Size: 0x154
 */
// 0x16A's sequence: pauses the music and gives the next sound unless 0x16A still plays
extern "C" s32 SelectVoiceSequenceSound(XSound* sound)
{
	if (IsWiiSfxPlaying(sound, 0xFF0000)) {
		PauseWiiSfx(sound, 0xFF0000, 2.0f);
	}
	if (IsWiiSfxPlaying(sound, 0x16A)) {
		return -1;
	}
	if (sound->mVoiceSeqIndex < 0 || (u32)sound->mVoiceSeqIndex >= 25) {
		sound->mVoiceSeqIndex = 0;
	}
	return sSound16ASequence[sound->mVoiceSeqIndex++];
}

/**
 * @note Address: 0x8003B730
 * @note Size: 0x158
 */
// 0x17D's sequence; silent while the cooking music (0xFF0037) plays, otherwise it pauses the music
extern "C" s32 SelectCookingSequenceSound(XSound* sound)
{
	if (IsWiiSfxPlaying(sound, 0xFF0037)) {
		return -1;
	}
	if (IsWiiSfxPlaying(sound, 0xFF0000)) {
		PauseWiiSfx(sound, 0xFF0000, 2.0f);
	}
	if (sound->mVoiceSeqIndex < 0 || (u32)sound->mVoiceSeqIndex >= 8) {
		sound->mVoiceSeqIndex = 0;
	}
	return sSound17DSequence[sound->mVoiceSeqIndex++];
}

/**
 * @note Address: 0x8003B888
 * @note Size: 0x3C
 */
// 0x1A4 plays its eleven sounds in turn
extern "C" s32 SelectSequenceSound1A4(XSound* sound)
{
	if (sound->mSoundSeqIndex < 0 || (u32)sound->mSoundSeqIndex >= 11) {
		sound->mSoundSeqIndex = 0;
	}
	return sSound1A4Sequence[sound->mSoundSeqIndex++];
}

/**
 * @note Address: 0x8003B8C4
 * @note Size: 0x244
 */
// PlayWiiSoundEffect's 0x1E2 and its neighbours: pause bgm or bgm_night for three seconds and pick one of six sounds
extern "C" s32 SelectVariantSound(XSound* sound, u32 sfx)
{
	if (IsWiiSfxPlaying(sound, 0xFF0005)) {
		PauseWiiSfx(sound, 0xFF0005, 3.0f);
	}
	if (IsWiiSfxPlaying(sound, 0xFF0010)) {
		PauseWiiSfx(sound, 0xFF0010, 3.0f);
	}
	return sfx == 0x1E2 ? sSound1E2Variants[rand() % ARRAY_SIZE(sSound1E2Variants)]
	                    : sSound1E3Variants[rand() % ARRAY_SIZE(sSound1E3Variants)];
}

/**
 * @note Address: 0x8003BB08
 * @note Size: 0x318
 */
// NONMATCHING: only the 31 table offsets: retail counts them from the file's first data (the music table,
// 0x430 before the jump table), and the unit's .data split starts at the sound banks.
// msg.cpp calls this for each character it prints, with the message's mGlyphSoundGroup and its font size / 22
// as strength
extern "C" void PlayMessageGlyphSound(XSound* sound, u32 group, f32 strength)
{
	u32 volume = 255.0f * strength;
	if (strength < 0.5f) {
		volume *= 0.2f;
	} else if (strength < 1.0f) {
		volume *= 0.6;
	}

	s32* ids;
	s32 count;
	switch (group) {
	case 0:
		ids   = sMessageGroup0SoundIds;
		count = 7;
		break;
	case 1:
		ids   = sMessageGroup1SoundIds;
		count = 5;
		break;
	case 2:
		ids   = sMessageGroup2SoundIds;
		count = 10;
		break;
	case 3:
		ids   = sMessageGroup3SoundIds;
		count = 10;
		break;
	case 4:
		ids   = sMessageGroup4SoundIds;
		count = 10;
		break;
	case 7:
		ids   = sMessageGroup7SoundIds;
		count = 10;
		break;
	case 8:
		ids   = sMessageGroup8SoundIds;
		count = 5;
		break;
	case 9:
		ids   = sMessageGroup9SoundIds;
		count = 10;
		break;
	case 10:
		ids   = sMessageGroup10SoundIds;
		count = 5;
		break;
	case 11:
		ids   = sMessageGroup11SoundIds;
		count = 5;
		break;
	case 12:
		ids   = sMessageGroup12SoundIds;
		count = 6;
		break;
	case 13:
		ids   = sMessageGroup13SoundIds;
		count = 10;
		break;
	case 14:
		ids   = sMessageGroup14SoundIds;
		count = 2;
		break;
	case 15:
		ids   = sMessageGroup15SoundIds;
		count = 12;
		break;
	case 16:
		ids   = sMessageGroup16SoundIds;
		count = 10;
		break;
	case 17:
		ids   = sMessageGroup17SoundIds;
		count = 10;
		break;
	case 18:
		ids   = sMessageGroup18SoundIds;
		count = 10;
		break;
	case 19:
		ids   = sMessageGroup19SoundIds;
		count = 10;
		break;
	case 20:
		ids   = sMessageGroup20SoundIds;
		count = 10;
		break;
	case 21:
		ids   = sMessageGroup21SoundIds;
		count = 10;
		break;
	case 22:
		ids   = sMessageGroup22SoundIds;
		count = 10;
		break;
	case 23:
		ids   = sMessageGroup23SoundIds;
		count = 10;
		break;
	case 24:
		ids   = sMessageGroup24SoundIds;
		count = 6;
		break;
	case 25:
		ids   = sMessageGroup25SoundIds;
		count = 11;
		break;
	case 26:
		ids   = sMessageGroup26SoundIds;
		count = 12;
		break;
	case 27:
		ids   = sMessageGroup27SoundIds;
		count = 10;
		break;
	case 28:
		ids   = sMessageGroup28SoundIds;
		count = 6;
		break;
	case 29:
		ids   = sMessageGroup29SoundIds;
		count = 6;
		break;
	case 30:
		ids   = sMessageGroup30SoundIds;
		count = 8;
		break;
	case 31:
		ids   = sMessageGroup31SoundIds;
		count = 4;
		break;
	case 32:
		ids   = sMessageGroup32SoundIds;
		count = 4;
		break;
	case 33:
		ids   = sMessageGroup33SoundIds;
		count = 6;
		break;
	default:
		return;
	}
	if (count > 0) {
		for (s32 i = 0; i < count; i++) {
			if (IsWiiSfxPlaying(sound, ids[i])) {
				return;
			}
		}
		s32 index = rand() % count;
		// the volume is a byte's worth: one over 255 plays the sound twice, the part above 256 and a full one
		if (volume >= 512) {
			volume = 511;
		}
		PlayWiiSoundEffect(sound, ids[index], (volume & 0xFF) / 255.0f, 0.0f);
		if (volume > 255) {
			PlayWiiSoundEffect(sound, ids[index], 1.0f, 0.0f);
		}
	}
}

/**
 * @note Address: 0x8003BE20
 * @note Size: 0x10
 */
// empties the sound's queue
extern "C" void ClearGameSoundQueue(XSound* sound)
{
	sound->mQueueWrite = 0;
	sound->mQueueRead  = 0;
}

/**
 * @note Address: 0x8003BE30
 * @note Size: 0x7C
 */
// 0xF1-0xF5 each play the effect picked for their row
extern "C" s32 SelectMappedSound(s32 sfx)
{
	// the GameCube's gSoundMenuEffects and gSoundMenuSelections; only this function reads them on the Wii
	static s32 sEffects[10]   = { 0x15B, 0x15C, 0x15D, 0x15E, 0x15F, 0x160, 0x161, 0x162, 0x163, 0x164 };
	static s32 sSelections[5] = { 8, 9, 7, 0, 4 };

	s32 index;
	switch (sfx) {
	case 0xF3:
		index = 0;
		break;
	case 0xF4:
		index = 1;
		break;
	case 0xF5:
		index = 2;
		break;
	case 0xF2:
		index = 3;
		break;
	case 0xF1:
		index = 4;
		break;
	default:
		return sfx;
	}
	return sEffects[sSelections[index]];
}

/**
 * @note Address: 0x8003BEAC
 * @note Size: 0x380
 */
// PlayWiiSoundEffect's 0x1F9-0x1FB: pause bgm or bgm_night for two seconds and stop whichever of the three plays
extern "C" s32 PauseMusicForJingle(XSound* sound, s32 sfx)
{
	if (IsWiiSfxPlaying(sound, 0xFF0005)) {
		PauseWiiSfx(sound, 0xFF0005, 2.0f);
	}
	if (IsWiiSfxPlaying(sound, 0xFF0010)) {
		PauseWiiSfx(sound, 0xFF0010, 2.0f);
	}
	if (IsWiiSfxPlaying(sound, 0x1F9)) {
		StopWiiSfx(sound, 0x1F9, 0.0f);
	}
	if (IsWiiSfxPlaying(sound, 0x1FA)) {
		StopWiiSfx(sound, 0x1FA, 0.0f);
	}
	if (IsWiiSfxPlaying(sound, 0x1FB)) {
		StopWiiSfx(sound, 0x1FB, 0.0f);
	}
	return sfx;
}

/**
 * @note Address: 0x8003C22C
 * @note Size: 0x140
 */
// the stages that echo get their reverb on the first aux bus
extern "C" void ApplyStageSoundEffects(XSound* sound)
{
	// todo: csound.h holds XSound's CSoundFxReverbHiDpl2 (csoundfx.h) as the first bytes of mPlaybackState; its
	// nw4r base, the effect itself, starts 8 bytes in
	switch (gGame.mStage) {
	case 0x15:
	case 0x1D:
		gSoundReverbParam.outGain   = 0.6f;
		gSoundReverbParam.fusedTime = 2.0f;
		gSoundReverbParam.earlyMode = 0;
		gSoundReverbParam.fusedMode = 5;
		gSoundReverbParam.fusedGain = 0.7f;
		(static_cast<nw4r::snd::FxReverbHiDpl2*>(&sound->mFx.mReverb))->SetParam(gSoundReverbParam);
		nw4r::snd::detail::AxManager::GetInstance().AppendEffect(nw4r::snd::AUX_A,
		                                                         static_cast<nw4r::snd::FxReverbHiDpl2*>(&sound->mFx.mReverb));
		break;
	case 0x17:
		gSoundReverbParam.outGain   = 0.6f;
		gSoundReverbParam.fusedTime = 3.0f;
		gSoundReverbParam.earlyMode = 5;
		gSoundReverbParam.fusedMode = 5;
		gSoundReverbParam.fusedGain = 1.0f;
		(static_cast<nw4r::snd::FxReverbHiDpl2*>(&sound->mFx.mReverb))->SetParam(gSoundReverbParam);
		nw4r::snd::detail::AxManager::GetInstance().AppendEffect(nw4r::snd::AUX_A,
		                                                         static_cast<nw4r::snd::FxReverbHiDpl2*>(&sound->mFx.mReverb));
		break;
	case 0x18:
		gSoundReverbParam.outGain   = 0.6f;
		gSoundReverbParam.fusedTime = 2.0f;
		gSoundReverbParam.earlyMode = 0;
		gSoundReverbParam.fusedMode = 2;
		gSoundReverbParam.fusedGain = 0.8f;
		(static_cast<nw4r::snd::FxReverbHiDpl2*>(&sound->mFx.mReverb))->SetParam(gSoundReverbParam);
		nw4r::snd::detail::AxManager::GetInstance().AppendEffect(nw4r::snd::AUX_A,
		                                                         static_cast<nw4r::snd::FxReverbHiDpl2*>(&sound->mFx.mReverb));
		break;
	}
}

/**
 * @note Address: 0x8003C36C
 * @note Size: 0x58
 */
extern "C" void ClearSoundEffects(XSound* sound, s32 frames)
{
	nw4r::snd::detail::AxManager::GetInstance().ClearEffect(nw4r::snd::AUX_A, frames);
	nw4r::snd::detail::AxManager::GetInstance().ClearEffect(nw4r::snd::AUX_B, frames);
	nw4r::snd::detail::AxManager::GetInstance().ClearEffect(nw4r::snd::AUX_C, frames);
}

/**
 * @note Address: 0x8003C3C4
 * @note Size: 0xA0
 */
// frees the actor; a source without a manager takes the game's
void AnimSfxActor::Reset()
{
	if (mManager == NULL) {
		mManager = &gGame.mSound.mSound3DManager;
	}
	Release();
}

/**
 * @note Address: 0x8003C464
 * @note Size: 0x6C
 */
CSound3DActor::~CSound3DActor()
{
}

/**
 * @note Address: 0x8003C4D0
 * @note Size: 0x188
 */
// NONMATCHING: registers only: retail keeps the freed actor of both loops in r27 and the counters in r29/r30,
// here the counters share r27.
// takes the object and its event table and gives each of the six sources a fresh actor
void AnimSfx::Init(WiiPlayerState* owner, AnimSfxEvent* events)
{
	mOwner  = owner;
	mEvents = events;
	for (s32 i = 0; i < 6; i++) {
		mActors[i].Release();
	}
	for (s32 i = 0; i < 6; i++) {
		mActors[i].Reset();
	}
	for (s32 i = 0; i < 6; i++) {
		mActors[i].Create();
	}
}

/**
 * @note Address: 0x8003C658
 * @note Size: 0x98
 */
// whether the event's start or end frame went by
BOOL AnimSfx::CheckStart(AnimSfxEvent* event, f32 min, f32 max)
{
	if (gGame.mMovie.mState) {
		return FALSE;
	}
	f32 a = event->mStart;
	f32 b = event->mEnd;

	switch (event->mKind) {
	case 1:
		if (0.0f <= a && min <= a && a < max) {
			return TRUE;
		}
		if (0.0f <= b && min <= b && b < max) {
			return TRUE;
		}
		break;
	}
	return FALSE;
}

/**
 * @note Address: 0x8003C6F0
 * @note Size: 0x8
 */
BOOL AnimSfx::CheckStop(AnimSfxEvent* event)
{
	return FALSE;
}

/**
 * @note Address: 0x8003C6F8
 * @note Size: 0x44
 */
// steps round the event's sfx ids when it has more than one
s32 AnimSfx::GetSfx(AnimSfxEvent* event)
{
	if (event->mCount == 0) {
		return event->mSfx;
	}
	event->mIndex++;
	if (event->mCount <= event->mIndex) {
		event->mIndex = 0;
	}
	return event->mSfx + event->mIndex;
}

/**
 * @note Address: 0x8003C73C
 * @note Size: 0x4
 */
void AnimSfx::vf08()
{
}

/**
 * @note Address: 0x8003C740
 * @note Size: 0x4EC
 */
// NONMATCHING: the actor and loop pointer swap r17/r18 behind the owner's GetPos call, and the stop loops
// wait for the typed mTracks (see SelectFootstepSound).
void AnimSfx::Update(s32 anim, f32 frame, f32 vol)
{
	s32 prev = mAnimId;

	if (prev != anim) {
		mAnimId = anim;
		mFrame  = 0.0f;
	}
	AnimSfxEvent* event;
	for (s32 i = 0; mEvents != NULL && (event = &mEvents[i])->mAnim >= 0; i++) { // the table ends at -1

		if (event->mAnim == anim || event->mAnim == prev) {
			if (prev != anim && event->mAnim == prev && CheckStop(event)) {
				StopWiiSfx(&gGame.mSound, GetSfx(event), 0.0f);
			}
			if (event->mAnim == anim && CheckStart(event, mFrame, frame)) {
				s32 sfx = GetSfx(event);
				switch (sfx) {
				case 0x1A4:
					sfx = SelectSequenceSound1A4(&gGame.mSound);
					break;
				// these wait for the last actor's sound to end
				case 0x1FD:
				case 0x4B3:
				case 0x503:
					if (mActors[5].GetHandle() != NULL) {
						continue;
					}
					break;
				}
				CSound3DActor* actor = mActors[event->mActor].mActor;
				if (actor != NULL) {
					if (actor->StartSound(&actor->mHandle, (u32)sfx)) {
						actor->mHandle.SetVolume(1.0f, 0);
					}
				}
			}
		}
	}
	mFrame = frame;

	// the actors follow the object and fade towards their volume and pitch
	for (s32 i = 0; i < 6; i++) {
		mActors[i].Update(mOwner->GetPos());
	}
}

/**
 * @note Address: 0x8003CC2C
 * @note Size: 0x10C
 */
void AnimSfx::Pause(f32 fade)
{
	for (s32 i = 0; i < 6; i++) {
		if (mActors[i].GetHandle() != NULL) {
			CSound3DActor* actor = mActors[i].mActor;
			if (actor != NULL) {
				actor->mHandle.Pause(gRefreshRate * fade);
			}
		}
	}
}

/**
 * @note Address: 0x8003CD38
 * @note Size: 0x10C
 */
void AnimSfx::Resume(f32 fade)
{
	for (s32 i = 0; i < 6; i++) {
		if (mActors[i].GetHandle() != NULL) {
			CSound3DActor* actor = mActors[i].mActor;
			if (actor != NULL) {
				actor->mHandle.Resume(gRefreshRate * fade);
			}
		}
	}
}

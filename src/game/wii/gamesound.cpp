#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/wii/csoundfx.h"
#include "game/soundtracks.h"
#include "nw4r/snd/snd_SoundStartable.h"
#include "nw4r/snd/snd_SoundHandle.h"
#include "stl/stdlib.h"

// The game's sound front end, the Wii port of the GameCube's sound.cpp: the same functions, on
// nw4r::snd instead of HSD. Music ids are 0x00FFxxxx here (0xFFFFxxxx on the GameCube), volumes are
// 0..1 floats and times are seconds. Retail has this unit, soundbank.cpp and sound.cpp in one file.
//
// The unit's first four functions (0x80036618..0x80036694) are not this file's: CCamera::CalcUp,
// CCamera::CalcInterest, CCameraActionMap::GetTarget and CCamera's viewport virtual at slot 0x84 are
// in-class virtuals the file in front of this one (gxa.cpp on the GameCube) left behind.

extern s32 gRefreshRate;
// the frames a started sound takes to reach its volume and pitch; nothing in retail writes it
extern s32 gSoundFadeFrames;
// the reverb's parameters, a static of this file in retail (sound.cpp's unit defines it)
extern nw4r::snd::detail::FxReverbHiParam gSoundReverbParam;

extern "C" {
BOOL IsLiftTargetPlug(WiiPlayerAction* action);
BOOL IsPlayerCarryingPlug(WiiPlayerState* player);
void InitGameSound(XSound* sound, u32 heapSize);

// the rest of the same original file, in the unit behind this one (sound.cpp)
BOOL CanPlayNightStream(XSound* sound);
s32 SelectFootstepSound(XSound* sound);
s32 SelectAlternatingSound7E(XSound* sound);
s32 SelectAlternatingSound82(XSound* sound);
s32 SelectVoiceSequenceSound(XSound* sound);
s32 SelectCookingSequenceSound(XSound* sound);
s32 SelectSequenceSound1A4(XSound* sound);
s32 SelectVariantSound(XSound* sound, s32 sfx);
s32 SelectMappedSound(s32 sfx);
s32 PauseMusicForJingle(XSound* sound, s32 sfx);
}

// shortcut: csound.h still holds the archive player as raw bytes; these two give its parts their
// types. Drop them once CSound::mArchivePlayer is a nw4r::snd::SoundArchivePlayer.
// the player's SoundStartable base, behind its 0xC of DisposeCallback: snd_SoundArchivePlayer.h
// does not compile with the game's include paths (<new>, <climits>)
static inline nw4r::snd::SoundStartable& ArchivePlayer(CSound* sound)
{
	return *(nw4r::snd::SoundStartable*)&sound->mArchivePlayer[0xC];
}

static inline s32 OutputLineFlag(CSound* sound)
{
	return *(s32*)sound->mArchivePlayerTail;
}

bool isAllSfx(XSound* sound, s32 sfx);
s32 SecondsToFrames(f32 seconds);
f32 CentsToPitch(f32 cents);
s32 Random(s32 max);
static inline void SetSlotVolume(CSoundHandle* slot, f32 volume, s32 frames, BOOL apply);
void SetSlotPitch(CSoundHandle* slot, f32 pitch, s32 frames, BOOL apply);
CSoundHandle* FindSound(CSound* sound, u32 id);
CSoundHandle* StartSound(CSound* sound, u32 id, f32 volume, s32 fadeFrames);
static inline void StopSound(CSound* sound, u32 id, s32 fadeFrames);
static inline void ResumeSound(CSound* sound, u32 id, s32 fadeFrames);
static inline void SetSoundVolume(CSound* sound, u32 id, f32 volume, s32 frames);
static inline void SetSoundPitch(CSound* sound, u32 id, f32 pitch, s32 frames);
static inline void PauseSound(CSound* sound, u32 id);

// todo: this is CanPlayNightStream (0x8003AC2C), which sound.cpp's unit holds; retail expands it in
// UpdateXSound and calls it from PlayWiiSoundEffect
static inline bool canPlayNightStream()
{
	if (gGame.mTimeState.mMode == 1 && gGame.mTimeState.mTime > 0.5f)
		return false;
	return true;
}

/**
 * @note Address: 0x80036694
 * @note Size: 0x48
 */
extern "C" void InitGameSound(XSound* sound, u32 heapSize)
{
	InitAudio(sound, heapSize);
	// the reverb sits at the head of mPlaybackState; retail calls it through its CSoundFx base
	CSoundFx* fx = (CSoundFxReverbHiDpl2*)sound->mPlaybackState;
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
	if (pitchFrames > 0 && sound->mMusicPitch != sound->mTargetMusicPitch && !gGame.mHomeMenu.IsActive() && !gGame.mMenuActive) {
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
	if (sound->mMusicResumeDelay > 0 && !gGame.mHomeMenu.IsActive() && !gGame.mMenuActive) {
		if (!IsWiiSfxPlaying(sound, 0x1F9) && !IsWiiSfxPlaying(sound, 0x1FA) && !IsWiiSfxPlaying(sound, 0x1FB)) {
			sound->mMusicResumeDelay -= step;
		}
		if (sound->mMusicResumeDelay <= 0) {
			ResumeWiiSfx(sound, 0xFF0000, 1.0f);
		}
	}
	if (sound->mCurrentMusicId == 0xFF0010 && !canPlayNightStream()) {
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
	if (sound->mCooldown > 0 && !gGame.mHomeMenu.IsActive() && !gGame.mMenuActive) {
		sound->mCooldown -= step;
	}
	if (sound->mQueueRead != sound->mQueueWrite && !gGame.mHomeMenu.IsActive() && !gGame.mMenuActive) {
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
// NONMATCHING: the walker, the index and the handle take r5, r7, r6 where retail has r6, r5, r7
CSoundHandle* FindSound(CSound* sound, u32 id)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mTracks[i].IsAttachedSound() && sound->mTracks[i].GetId() == id) {
			return &sound->mTracks[i];
		}
	}
	return NULL;
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
// NONMATCHING: retail expands nothing in this function, not even the one-line getters: it calls
// isAllSfx, CGame::GetTimeMode (0x800377B4), FindSound, CGame::GetPlayer(s32) (0x80037134),
// IsPlayerCarryingPlug, SecondsToFrames, StartSound, CentsToPitch, SetSlotPitch and Random, and
// those out-of-line copies are why they stand around it. Every other function of the file expands
// them. That is the compiler's inline budget (a caller of 7000 units or more expands nothing), so
// the original function carried far more source than its code shows. With -ipa file there is no
// such budget (2000 filler statements still expand everything); without it and with
// -inline auto,deferred about 2400 filler statements bring this function to retail's shape
// (0x64C bytes against 0x640). Nothing honest supplies that weight, so the helpers expand here.
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
		if (!sound->mTracks[i].IsAttachedSound()) {
			return &sound->mTracks[i];
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
// NONMATCHING: where a
// case returns its last search, retail tests the result and returns it either way (cmpwi r3, 0;
// beqlr; blr at 0x800383EC, 0x8003939C, 0x800394A4) and ours loads the 0 again (bnelr; li r3, 0).
extern "C" s32 IsWiiSfxPlaying(XSound* sound, s32 sfx)
{
	// steps by two, so only even slots are searched, as on the GameCube
	for (u32 i = 0; i < 32 && sound->mStartedSfx[i] != 0; i += 2) {
		if (sound->mStartedSfx[i] == sfx) {
			return (s32)FindSound(sound, sfx);
		}
	}

	if (isAllSfx(sound, sfx)) {
		if (sound->mCurrentMusicId == sfx) {
			return (s32)FindSound(sound, sound->mCurrentMusicId & 0xFFFF);
		}
		if (sfx == 0xFF0000 && sound->mCurrentMusicId != 0) {
			return (s32)FindSound(sound, sound->mCurrentMusicId & 0xFFFF);
		}
		if (sfx == 0xFF0005 && sound->mCurrentMusicId != 0) {
			return (s32)FindSound(sound, sound->mCurrentMusicId & 0xFFFF);
		}
		return 0;
	}

	CSoundHandle* slot;
	// 0x88 and 0x16A stand for every sound PlayWiiSoundEffect picks for them
	switch (sfx) {
	case 0x88:
		if ((slot = FindSound(sound, sfx)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, sfx + 1)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, sfx + 2)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, sfx + 3)) != NULL) {
			return (s32)slot;
		}
		return 0;
	case 0x16A:
		if ((slot = FindSound(sound, 0x16A)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x16B)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x16C)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x16D)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x16E)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x16F)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x170)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x171)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x172)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x173)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x174)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x175)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x176)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x177)) != NULL) {
			return (s32)slot;
		}
		if ((slot = FindSound(sound, 0x178)) != NULL) {
			return (s32)slot;
		}
		return 0;
	default:
		if ((slot = FindSound(sound, sfx)) != NULL) {
			return (s32)slot;
		}
		return 0;
	}
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

static inline void StopSound(CSound* sound, u32 id, s32 fadeFrames)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mTracks[i].IsAttachedSound() && sound->mTracks[i].GetId() == id) {
			sound->mTracks[i].Stop(fadeFrames);
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

static inline void PauseSound(CSound* sound, u32 id)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mTracks[i].IsAttachedSound() && sound->mTracks[i].GetId() == id) {
			if (!sound->mTracks[i].IsPause()) {
				sound->mTracks[i].nw4r::snd::SoundHandle::Pause(true, 0);
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

static inline void ResumeSound(CSound* sound, u32 id, s32 fadeFrames)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mTracks[i].IsAttachedSound() && sound->mTracks[i].GetId() == id) {
			if (sound->mTracks[i].IsPause()) {
				sound->mTracks[i].nw4r::snd::SoundHandle::Pause(false, fadeFrames);
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

static inline void SetSoundVolume(CSound* sound, u32 id, f32 volume, s32 frames)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mTracks[i].IsAttachedSound() && sound->mTracks[i].GetId() == id) {
			SetSlotVolume(&sound->mTracks[i], volume, frames, TRUE);
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

static inline void SetSoundPitch(CSound* sound, u32 id, f32 pitch, s32 frames)
{
	for (int i = 0; i < 0x40; i++) {
		if (sound->mTracks[i].IsAttachedSound() && sound->mTracks[i].GetId() == id) {
			SetSlotPitch(&sound->mTracks[i], pitch, frames, TRUE);
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

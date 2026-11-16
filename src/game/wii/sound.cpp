// The first half of the Wii's sound.cpp: the sounds of an object's anims (AnimSfx), the stage reverb,
// the message glyph sounds and the sound picks PlayWiiSoundEffect makes. Retail has this unit,
// soundbank.cpp and gamesound.cpp in one file, in the reverse of the GameCube file's order.
#include "types.h"
#include <stdlib.h>
#include "Dolphin/mtx.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/wii/csound3dactor.h"
#include "game/wii/csoundfx.h"
#include "game/wii/animsfx.h"
#include "nw4r/snd/snd_AxManager.h"
#include "nw4r/snd/snd_SoundArchivePlayer.h"
#include "nw4r/snd/snd_Sound3DManager.h"

extern s32 gRefreshRate;
// frames a started sound takes to reach its volume and pitch; nothing in the game writes it. The name is ours
extern s32 gSoundFadeFrames;

extern "C" {
s32 SelectFootstepSound(XSound* sound);
s32 SelectAlternatingSound7E(XSound* sound);
s32 SelectAlternatingSound82(XSound* sound);
s32 SelectVoiceSequenceSound(XSound* sound);
s32 SelectCookingSequenceSound(XSound* sound);
s32 SelectSequenceSound1A4(XSound* sound);
s32 SelectVariantSound(XSound* sound, u32 sfx);
s32 SelectMappedSound(s32 sfx);
s32 PauseMusicForJingle(XSound* sound, s32 sfx);
BOOL CanPlayNightStream(XSound* sound);
f32 GetSoundVolumeAtPosition(XSound* sound, Vec* pos, s32 kind);
void PlayMessageGlyphSound(XSound* sound, u32 group, f32 strength);
void ClearGameSoundQueue(XSound* sound);
void ApplyStageSoundEffects(XSound* sound);
void ClearSoundEffects(XSound* sound, s32 frames);
}

// todo: the inline functions below are the bodies of gamesound.cpp's functions (the sound start at
// 0x800377C0, StopWiiSfx, the music pause). Retail compiles both units as one file and expands them
// here; they go when the file is put back together.

// ids with 0xFF in the third byte (but not -1) are music
static inline bool isMusic(s32 sfx)
{
	if (sfx == -1) {
		return false;
	}
	return (sfx & 0xFF0000) == 0xFF0000;
}

static inline CSoundHandle* findFreeHandle(XSound* sound)
{
	for (s32 i = 0; i < 0x40; i++) {
		if (!((CSoundHandle&)sound->mTracks[i]).IsAttachedSound()) {
			return (CSoundHandle*)&sound->mTracks[i];
		}
	}
	return NULL;
}

static inline CSoundHandle* startSound(XSound* sound, u32 id, f32 volume, s32 fadeFrames)
{
	CSoundHandle* handle = findFreeHandle(sound);
	if (handle != NULL) {
		if (((nw4r::snd::SoundArchivePlayer*)sound->mArchivePlayer)->StartSound(handle, id)) {
			handle->SetOutputLineFlag(*(s32*)sound->mArchivePlayerTail);
			handle->SetVolume(volume, gSoundFadeFrames);
			handle->SetPitch(1.0f, gSoundFadeFrames);
			// the remote's speaker plays it at half volume
			if (*(s32*)sound->mArchivePlayerTail & 2) {
				handle->SetRemoteOutVolume(0, 0.5f);
			}
			if (fadeFrames > 0) {
				handle->FadeIn(fadeFrames);
			}
		} else {
			handle = NULL;
		}
	}
	return handle;
}

static inline void stopTracks(XSound* sound, u32 id, s32 frames)
{
	for (s32 i = 0; i < 0x40; i++) {
		if (((CSoundHandle&)sound->mTracks[i]).IsAttachedSound() && ((CSoundHandle&)sound->mTracks[i]).GetId() == id) {
			((CSoundHandle&)sound->mTracks[i]).Stop(frames);
		}
	}
}

static inline void stopSfx(XSound* sound, s32 sfx, f32 fade)
{
	if (isMusic(sfx)) {
		stopTracks(sound, sound->mCurrentMusicId & 0xFFFF, gRefreshRate * fade);
		sound->mCurrentMusicId = 0;
		sound->mPausedMusicId  = 0;
	} else {
		stopTracks(sound, sfx, gRefreshRate * fade);
	}
}

// pauses the music; UpdateGameSound resumes it after so many seconds
static inline void pauseMusic(XSound* sound, f32 seconds)
{
	u32 id = sound->mCurrentMusicId & 0xFFFF;
	for (s32 i = 0; i < 0x40; i++) {
		if (((CSoundHandle&)sound->mTracks[i]).IsAttachedSound() && ((CSoundHandle&)sound->mTracks[i]).GetId() == id) {
			((CSoundHandle&)sound->mTracks[i]).Pause(0);
		}
	}
	s32 frames               = gRefreshRate * seconds;
	sound->mPausedMusicId    = sound->mCurrentMusicId;
	sound->mMusicResumeDelay = frames;
}

// the reverb's parameters: InitGameSound sets it up with the library's defaults, ApplyStageSoundEffects
// changes them for the stage. todo: a global only because gamesound.cpp is a unit of its own
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
// NONMATCHING: csound.h types the 64 handles as CSoundTrackSlot with a void*, so the loops reach them through a
// cast and keep their own pointer (addi rX, sound, 0x284) where retail indexes off the sound object. With
// mTracks typed as CSoundHandle this function is 100 %.
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
		startSound(sound, 0x20A, 70.0f, 0);
		break;
	case 9:
		startSound(sound, 0x209, 1.0f, 0);
		break;
	case 10:
		startSound(sound, 0x20B, 50.0f, 0);
		break;
	}
	return sSurfaceSoundIds[surface][rand() % 5];
}

/**
 * @note Address: 0x8003B17C
 * @note Size: 0x230
 */
// NONMATCHING: csound.h types the 64 handles as CSoundTrackSlot with a void*, so the loops reach them through a
// cast and keep their own pointer (addi rX, sound, 0x284) where retail indexes off the sound object. With
// mTracks typed as CSoundHandle this function is 100 %.
extern "C" s32 SelectAlternatingSound7E(XSound* sound)
{
	for (u32 i = 0; i < 4; i++) {
		if (IsWiiSfxPlaying(sound, sAlternatingSound82[i])) {
			stopSfx(sound, sAlternatingSound82[i], 0.0f);
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
// NONMATCHING: csound.h types the 64 handles as CSoundTrackSlot with a void*, so the loops reach them through a
// cast and keep their own pointer (addi rX, sound, 0x284) where retail indexes off the sound object. With
// mTracks typed as CSoundHandle this function is 100 %.
// 0x7E and 0x82 each play the four sounds of their set in turn: starting one set stops the other, and a set
// does not start again while one of its own sounds still plays
extern "C" s32 SelectAlternatingSound82(XSound* sound)
{
	for (u32 i = 0; i < 4; i++) {
		if (IsWiiSfxPlaying(sound, sAlternatingSound7E[i])) {
			stopSfx(sound, sAlternatingSound7E[i], 0.0f);
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
// NONMATCHING: csound.h types the 64 handles as CSoundTrackSlot with a void*, so the loops reach them through a
// cast and keep their own pointer (addi rX, sound, 0x284) where retail indexes off the sound object. With
// mTracks typed as CSoundHandle this function is 100 %.
// 0x16A's sequence: pauses the music and gives the next sound unless 0x16A still plays
extern "C" s32 SelectVoiceSequenceSound(XSound* sound)
{
	if (IsWiiSfxPlaying(sound, 0xFF0000)) {
		pauseMusic(sound, 2.0f);
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
// NONMATCHING: csound.h types the 64 handles as CSoundTrackSlot with a void*, so the loops reach them through a
// cast and keep their own pointer (addi rX, sound, 0x284) where retail indexes off the sound object. With
// mTracks typed as CSoundHandle this function is 100 %.
// 0x17D's sequence; silent while the cooking music (0xFF0037) plays, otherwise it pauses the music
extern "C" s32 SelectCookingSequenceSound(XSound* sound)
{
	if (IsWiiSfxPlaying(sound, 0xFF0037)) {
		return -1;
	}
	if (IsWiiSfxPlaying(sound, 0xFF0000)) {
		pauseMusic(sound, 2.0f);
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
// NONMATCHING: csound.h types the 64 handles as CSoundTrackSlot with a void*, so the loops reach them through a
// cast and keep their own pointer (addi rX, sound, 0x284) where retail indexes off the sound object. With
// mTracks typed as CSoundHandle this function is 100 %.
// PlayWiiSoundEffect's 0x1E2 and its neighbours: pause bgm or bgm_night for three seconds and pick one of six sounds
extern "C" s32 SelectVariantSound(XSound* sound, u32 sfx)
{
	if (IsWiiSfxPlaying(sound, 0xFF0005)) {
		pauseMusic(sound, 3.0f);
	}
	if (IsWiiSfxPlaying(sound, 0xFF0010)) {
		pauseMusic(sound, 3.0f);
	}
	return sfx == 0x1E2 ? sSound1E2Variants[rand() % ARRAY_SIZE(sSound1E2Variants)]
	                    : sSound1E3Variants[rand() % ARRAY_SIZE(sSound1E3Variants)];
}

/**
 * @note Address: 0x8003BB08
 * @note Size: 0x318
 */
// NONMATCHING: only the 31 table offsets: retail counts them from the first data of the whole file (the music
// table in gamesound.cpp, 0x430 before this unit's), this unit from its own.
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
// NONMATCHING: csound.h types the 64 handles as CSoundTrackSlot with a void*, so the loops reach them through a
// cast and keep their own pointer (addi rX, sound, 0x284) where retail indexes off the sound object. With
// mTracks typed as CSoundHandle this function is 100 %.
// PlayWiiSoundEffect's 0x1F9-0x1FB: pause bgm or bgm_night for two seconds and stop whichever of the three plays
extern "C" s32 PauseMusicForJingle(XSound* sound, s32 sfx)
{
	if (IsWiiSfxPlaying(sound, 0xFF0005)) {
		pauseMusic(sound, 2.0f);
	}
	if (IsWiiSfxPlaying(sound, 0xFF0010)) {
		pauseMusic(sound, 2.0f);
	}
	if (IsWiiSfxPlaying(sound, 0x1F9)) {
		stopSfx(sound, 0x1F9, 0.0f);
	}
	if (IsWiiSfxPlaying(sound, 0x1FA)) {
		stopSfx(sound, 0x1FA, 0.0f);
	}
	if (IsWiiSfxPlaying(sound, 0x1FB)) {
		stopSfx(sound, 0x1FB, 0.0f);
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
		((nw4r::snd::FxReverbHiDpl2*)&sound->mPlaybackState[8])->SetParam(gSoundReverbParam);
		nw4r::snd::detail::AxManager::GetInstance().AppendEffect(nw4r::snd::AUX_A, (nw4r::snd::FxReverbHiDpl2*)&sound->mPlaybackState[8]);
		break;
	case 0x17:
		gSoundReverbParam.outGain   = 0.6f;
		gSoundReverbParam.fusedTime = 3.0f;
		gSoundReverbParam.earlyMode = 5;
		gSoundReverbParam.fusedMode = 5;
		gSoundReverbParam.fusedGain = 1.0f;
		((nw4r::snd::FxReverbHiDpl2*)&sound->mPlaybackState[8])->SetParam(gSoundReverbParam);
		nw4r::snd::detail::AxManager::GetInstance().AppendEffect(nw4r::snd::AUX_A, (nw4r::snd::FxReverbHiDpl2*)&sound->mPlaybackState[8]);
		break;
	case 0x18:
		gSoundReverbParam.outGain   = 0.6f;
		gSoundReverbParam.fusedTime = 2.0f;
		gSoundReverbParam.earlyMode = 0;
		gSoundReverbParam.fusedMode = 2;
		gSoundReverbParam.fusedGain = 0.8f;
		((nw4r::snd::FxReverbHiDpl2*)&sound->mPlaybackState[8])->SetParam(gSoundReverbParam);
		nw4r::snd::detail::AxManager::GetInstance().AppendEffect(nw4r::snd::AUX_A, (nw4r::snd::FxReverbHiDpl2*)&sound->mPlaybackState[8]);
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
// NONMATCHING: the owner's GetPosition goes through WiiPlayerVtable, a struct of function pointers (lwz r4 /
// lwz r12,0x244(r4)); retail makes a virtual call (r12 twice), and the actor and loop pointer swap r17/r18
// behind it. The stop loops also wait for the typed mTracks (see SelectFootstepSound).
void AnimSfx::Update(s32 anim, f32 frame, u8 vol)
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
				stopSfx(&gGame.mSound, GetSfx(event), 0.0f);
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
		mActors[i].Update(mOwner->GetPosition());
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

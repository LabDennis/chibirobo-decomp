// The game's sound front end, one retail file (US .text 0x8001D9B0..0x8001FCF4):
// PlayGameSfxAtFullVolume through ClearGameSoundQueue and AnimSfx.
//
// NONMATCHING: retail's .rodata starts with the 0x48-byte six-Vec header block (US 0x801FE2B0)
// which no header emits here; sSoundReadOperation ("Read", US .sdata 0x8065A1B8) came from
// __FUNCTION__ of XSound::Read in retail where LoadGameSoundBank keeps the string literal.

#include "game/cgame.h"
#include <types.h>
#include <string.h>
#include <dolphin/os.h>
#include "Version.h"
#include "game/csound.h"
#include "game/soundtracks.h"
#include "game/xchr.h"

#include "game/soundplayback.h"

// a negative track means the default one
inline s32 XSound::resolveTrack(s32 track)
{
	if (track < 0) {
		track = mDefaultTrack;
	}
	return track;
}

extern "C" {
s32 HSD_AudioSFXStartParam(u32 sfx, u8 vol, u8 pan, s32 group, s32 track);
BOOL HSD_AudioSFXSetPitchByID(s32 sfx, s16 pitch);
s32 HSD_AudioPStreamStartChParam(const char* path, u8 vol, s32 channel);
s32 HSD_SynthSFXNewBank(s32 size);
s32 HSD_SynthSFXBankClear(s32 bank);
s32 HSD_SynthSFXLoad(const char* name, s32 bank, void (*callback)(s32, s32), s32 arg);
s32 HSD_Randi(s32 max);
void HSD_SynthSetSoundMode(s32 mode);
void HSD_JObjSetJSoundCallback(void (*callback)(s32));
void InitAudio(void* owner, u32 heapSize, u32 voices, u32 groups);
BOOL LoadSoundSampleMetadata(void* data);
BOOL AXDriverIsStreamActive(void);
BOOL AXDriverSetAllStreamPitches(s16 pitch);
}

// todo: these 26 words are not this file's. they are the joint paths of eight inline functions of
// the family's actor classes (two each for Jenny, Papa, Mama and Deca), which retail has in a
// header: every file that includes it starts its data with them. they stand here until the header
// makes them, because the tables below are reached through this address
s32 lbl_802107A0[26] = {
	0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 3, 0,
};

// the sound bank of each stage
const char* stageSfxTable[40] = {
	NULL,
	"sfx_kitchen.ssm",
	"sfx_stage02.ssm",
	"sfx_souko.ssm",
	"sfx_jennyroom.ssm",
	"sfx_stage05.ssm",
	"sfx_papamama.ssm",
	"sfx_stage07.ssm",
	NULL,
	"sfx_uraniwa.ssm",
	NULL,
	"sfx_pipe.ssm",
	NULL,
	"sfx_manual.ssm",
	"sfx_stage07.ssm",
	NULL,
	"sfx_ufo.ssm",
	NULL,
	"sfx_ufo.ssm",
	"sfx_stage07.ssm",
	NULL,
	NULL,
	"sfx_martial.ssm",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"sfx_manual.ssm",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

// the streamed tracks and how loud each plays
MusicEntry musicTable[109] = {
	{ "streaming/sample.hps", 255 },
	{ "streaming/sample.hps", 255 },
	{ "streaming/funk.hps", 255 },
	{ "streaming/theme.hps", 150 },
	{ "streaming/menu.hps", 150 },
	{ "streaming/bgm.hps", 180 },
	{ "streaming/k_sabi.hps", 180 },
	{ "streaming/k_theme1.hps", 255 },
	{ "streaming/UFOBGM.hps", 255 },
	{ "streaming/blues.hps", 255 },
	{ "streaming/party.hps", 255 },
	{ "streaming/dokodaka.hps", 120 },
	{ "streaming/spider.hps", 255 },
	{ "streaming/appearance.hps", 255 },
	{ "streaming/recollection_intro.hps", 255 },
	{ "streaming/recollection.hps", 255 },
	{ "streaming/bgm_night.hps", 180 },
	{ "streaming/army_theme.hps", 255 },
	{ "streaming/prelude.hps", 255 },
	{ "streaming/nosehair.hps", 255 },
	{ "streaming/angry.hps", 255 },
#if defined(VERSION_JP)
	{ "streaming/gicco.hps", 255 },
#else
	{ "streaming/gicco_us.hps", 255 },
#endif
	{ "streaming/reminiscence.hps", 255 },
	{ "streaming/teriyaki.hps", 255 },
	{ "streaming/zobin.hps", 255 },
	{ "streaming/dexy.hps", 255 },
	{ "streaming/ds.hps", 180 },
	{ "streaming/tpds.hps", 180 },
	{ "streaming/ship.hps", 150 },
	{ "streaming/toy_BGM1.hps", 255 },
	{ "streaming/toy_BGM2.hps", 255 },
	{ "streaming/toy_BGM3.hps", 255 },
	{ "streaming/spider2.hps", 255 },
	{ "streaming/training.hps", 255 },
	{ "streaming/army_training.hps", 255 },
	{ "streaming/mission_success.hps", 255 },
	{ "streaming/mission_failure.hps", 255 },
	{ "streaming/snare_loop.hps", 255 },
	{ "streaming/army_escape_j1.hps", 255 },
	{ "streaming/army_escape_j2.hps", 255 },
	{ "streaming/ennui.hps", 255 },
	{ "streaming/tao.hps", 255 },
	{ "streaming/shadow.hps", 255 },
	{ "streaming/reunion.hps", 255 },
	{ "streaming/kaimono.hps", 255 },
	{ "streaming/chip.hps", 255 },
	{ "streaming/hiru.hps", 255 },
	{ "streaming/hiru_wipe.hps", 255 },
	{ "streaming/yoru.hps", 255 },
	{ "streaming/yoru_wipe.hps", 255 },
	{ "streaming/rankup_j1.hps", 255 },
	{ "streaming/rankup_j2.hps", 255 },
	{ "streaming/super_robo.hps", 255 },
	{ "streaming/capsule.hps", 255 },
	{ "streaming/kako.hps", 255 },
	{ "streaming/cooking.hps", 255 },
	{ "streaming/timeslip.hps", 155 },
	{ "streaming/faceoff.hps", 255 },
	{ "streaming/bb.hps", 150 },
	{ "streaming/love.hps", 220 },
	{ "streaming/heaven.hps", 255 },
	{ "streaming/papa1.hps", 255 },
	{ "streaming/papa2.hps", 255 },
	{ "streaming/papa3.hps", 255 },
	{ "streaming/m_spd1.hps", 255 },
	{ "streaming/m_spd2.hps", 255 },
	{ "streaming/martial_law.hps", 255 },
	{ "streaming/fear.hps", 255 },
	{ "streaming/blue.hps", 255 },
	{ "streaming/battle1.hps", 255 },
	{ "streaming/battle2.hps", 255 },
	{ "streaming/violin.hps", 255 },
	{ "streaming/wrench.hps", 60 },
	{ "streaming/peach.hps", 255 },
	{ "streaming/peach2.hps", 255 },
	{ "streaming/present.hps", 180 },
	{ "streaming/toyrex.hps", 255 },
	{ "streaming/ff.hps", 255 },
	{ "streaming/kofunk1.hps", 255 },
	{ "streaming/kofunk2.hps", 255 },
	{ "streaming/death.hps", 255 },
	{ "streaming/tea_party.hps", 150 },
	{ "streaming/arise.hps", 255 },
	{ "streaming/grief.hps", 255 },
	{ "streaming/fake.hps", 255 },
	{ "streaming/memory.hps", 255 },
	{ "streaming/hock.hps", 255 },
	{ "streaming/yodomi.hps", 200 },
	{ "streaming/yodomi2.hps", 220 },
	{ "streaming/departure.hps", 220 },
	{ "streaming/abare.hps", 255 },
	{ "streaming/sanpoo.hps", 255 },
	{ "streaming/conquest.hps", 255 },
	{ "streaming/change.hps", 255 },
	{ "streaming/sunmida.hps", 255 },
	{ "streaming/dupe.hps", 255 },
	{ "streaming/ecstasy.hps", 255 },
	{ "streaming/rain.hps", 255 },
	{ "streaming/car.hps", 230 },
	{ "streaming/chicken.hps", 255 },
	{ "streaming/american.hps", 255 },
	{ "streaming/pipe.hps", 255 },
	{ "streaming/nwing.hps", 255 },
	{ "streaming/victory.hps", 180 },
	{ "streaming/mugen.hps", 255 },
	{ "streaming/yusho.hps", 255 },
	{ "streaming/souko.hps", 255 },
	{ "streaming/living.hps", 255 },
	{ "streaming/entrance.hps", 255 },
};

// GetSoundVolumeAtPosition's distances from the camera's interest for each kind, farthest first:
// past [0] a sound is silent, within [3] it is at full volume
f32 gSoundFalloffRanges[5][4] = {
	{ 100.0f, 75.0f, 50.0f, 25.0f }, { 300.0f, 200.0f, 200.0f, 150.0f }, { 75.0f, 40.0f, 30.0f, 25.0f },
	{ 30.0f, 20.0f, 10.0f, 5.0f },   { 600.0f, 300.0f, 200.0f, 150.0f },
};

// five footstep sounds for each kind of floor
static s32 sSurfaceSoundIds[13][5] = {
	{ 0x21, 0x22, 0x23, 0x24, 0x25 }, { 0x26, 0x27, 0x28, 0x29, 0x2A }, { 0x21, 0x22, 0x23, 0x24, 0x25 }, { 0x2B, 0x2C, 0x2D, 0x2E, 0x2F },
	{ 0x30, 0x31, 0x32, 0x33, 0x34 }, { 0x21, 0x22, 0x23, 0x24, 0x25 }, { 0x21, 0x22, 0x23, 0x24, 0x25 }, { 0x2B, 0x2C, 0x2D, 0x2E, 0x2F },
	{ 0x2B, 0x2C, 0x2D, 0x2E, 0x2F }, { 0x21, 0x22, 0x23, 0x24, 0x25 }, { 0x30, 0x31, 0x32, 0x33, 0x34 }, { 0x2B, 0x2C, 0x2D, 0x2E, 0x2F },
	{ 0x21, 0x22, 0x23, 0x24, 0x25 },
};

static s32 sAlternatingSound11[4]     = { 0x11, 0x12, 0x13, 0x14 };
static s32 sAlternatingSound15[4]     = { 0x15, 0x16, 0x17, 0x18 };
static s32 sSoundFDSequence[25]       = { 0xFD,  0xFE,  0xFF,  0x100, 0xFD,  0xFE,  0xFF,  0x100, 0x101, 0x101, 0x102, 0x102, 0x103,
	                                      0x103, 0x104, 0x105, 0x106, 0x101, 0x107, 0x102, 0x108, 0x103, 0x109, 0x10A, 0x10B };
static s32 sSound110Sequence[8]       = { 0x110, 0x111, 0x112, 0x113, 0x114, 0x115, 0x116, 0x117 };
static s32 sSound137Sequence[11]      = { 0x137, 0x138, 0x139, 0x13A, 0x13B, 0x13C, 0x13D, 0x13E, 0x13F, 0x140, 0x141 };
static s32 sSound176Variants[6]       = { 0x176, 0x177, 0x178, 0x179, 0x17A, 0x17B };
static s32 sSound175Variants[6]       = { 0x16F, 0x170, 0x171, 0x172, 0x173, 0x174 };
static s32 sMessageGroup0SoundIds[7]  = { 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7 };
static s32 sMessageGroup2SoundIds[10] = { 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B };
static s32 sMessageGroup4SoundIds[10] = { 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7 };
static s32 sMessageGroup3SoundIds[10] = { 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0 };
static s32 sMessageGroup9SoundIds[10]
    = { 0x110000, 0x110001, 0x110002, 0x110003, 0x110004, 0x110005, 0x110006, 0x110007, 0x110008, 0x110009 };
static s32 sMessageGroup8SoundIds[5]   = { 0xC0000, 0xC0001, 0xC0002, 0xC0003, 0xC0004 };
static s32 sMessageGroup7SoundIds[10]  = { 0xD0000, 0xD0001, 0xD0002, 0xD0003, 0xD0004, 0xD0005, 0xD0006, 0xD0007, 0xD0008, 0xD0009 };
static s32 sMessageGroup10SoundIds[5]  = { 0x50, 0x51, 0x52, 0x53, 0x54 };
static s32 sMessageGroup29SoundIds[6]  = { 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A };
static s32 sMessageGroup11SoundIds[5]  = { 0xE0000, 0xE0001, 0xE0002, 0xE0003, 0xE0004 };
static s32 sMessageGroup12SoundIds[6]  = { 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60 };
static s32 sMessageGroup13SoundIds[10] = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A };
static s32 sMessageGroup15SoundIds[12] = { 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76 };
static s32 sMessageGroup16SoundIds[10] = { 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1 };
static s32 sMessageGroup17SoundIds[10] = { 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80 };
static s32 sMessageGroup19SoundIds[10]
    = { 0x100000, 0x100001, 0x100002, 0x100003, 0x100004, 0x100005, 0x100006, 0x100007, 0x100008, 0x100009 };
static s32 sMessageGroup20SoundIds[10] = { 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1 };
static s32 sMessageGroup21SoundIds[10] = { 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB };
static s32 sMessageGroup22SoundIds[10] = { 0xF0000, 0xF0001, 0xF0002, 0xF0003, 0xF0004, 0xF0005, 0xF0006, 0xF0007, 0xF0008, 0xF0009 };
static s32 sMessageGroup23SoundIds[10] = { 0xF000A, 0xF000B, 0xF000C, 0xF000D, 0xF000E, 0xF000F, 0xF0010, 0xF0011, 0xF0012, 0xF0013 };
static s32 sMessageGroup18SoundIds[10] = { 0x14C, 0x14D, 0x14E, 0x14F, 0x150, 0x151, 0x152, 0x153, 0x154, 0x155 };
static s32 sMessageGroup1SoundIds[5]   = { 0x156, 0x157, 0x158, 0x159, 0x15A };
static s32 sMessageGroup24SoundIds[6]  = { 0x10000A, 0x10000B, 0x10000C, 0x10000D, 0x10000E, 0x10000F };
static s32 sMessageGroup27SoundIds[10] = { 0x163, 0x164, 0x165, 0x166, 0x167, 0x168, 0x169, 0x16A, 0x16B, 0x16C };
static s32 sMessageGroup25SoundIds[11]
    = { 0x120000, 0x120001, 0x120002, 0x120003, 0x120004, 0x120005, 0x120006, 0x120007, 0x120008, 0x120009, 0x12000A };
static s32 sMessageGroup26SoundIds[12]
    = { 0x12000B, 0x12000C, 0x12000D, 0x12000E, 0x12000F, 0x120010, 0x120011, 0x120012, 0x120013, 0x120014, 0x120015, 0x120016 };
static s32 sMessageGroup28SoundIds[6] = { 0x90014, 0x90015, 0x90016, 0x90017, 0x90018, 0x90019 };
static s32 sMessageGroup30SoundIds[8] = { 0x60010, 0x60011, 0x60012, 0x60013, 0x60014, 0x60015, 0x60016, 0x60017 };
static s32 sMessageGroup31SoundIds[4] = { 0x1AD, 0x1AE, 0x1AF, 0x1B0 };
static s32 sMessageGroup32SoundIds[4] = { 0x1B1, 0x1B2, 0x1B3, 0x1B4 };
static s32 sMessageGroup33SoundIds[6] = { 0x10027, 0x10028, 0x10029, 0x1002A, 0x1002B, 0x1002C };
s32 gSoundMenuEffects[10]             = { 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7 };
s32 gSoundMenuSelections[5]           = { 8, 9, 7, 0, 4 };

static s32 sMessageGroup14SoundIds[2] = { 0x9C, 0x9D };

// drops every sfx name waiting to load and empties the stage bank
static inline void ClearStageBank(XSound* sound)
{
	sound->mStageRequests.Clear();
	sound->mLoadRequestCount = 0;
	HSD_SynthSFXBankClear(sound->mStageBank);
}

// 0x11 and 0x15 each play the four sounds of their set in turn: starting one set stops the other, and a set
// does not start again while one of its own sounds still plays
static inline s32 SelectAlternatingSound(XSound* sound, s32* stop, s32* play)
{
	for (u32 i = 0; i < 4; i++) {
		if (IsGameSoundPlaying(sound, stop[i])) {
			stopSfx(sound, stop[i]);
			sound->mAltSoundIndex = 0;
		}
	}
	for (u32 i = 0; i < 4; i++) {
		if (IsGameSoundPlaying(sound, play[i])) {
			return -1;
		}
	}
	if (sound->mAltSoundIndex < 0 || (u32)sound->mAltSoundIndex >= 4) {
		sound->mAltSoundIndex = 0;
	}
	return play[sound->mAltSoundIndex++];
}

// 0x84-0x88 each play the effect gSoundMenuSelections holds for their row; only the japanese sound menu
// changes it (SetSoundMenuSelection below)
static inline s32 SelectMappedSound(s32 sfx)
{
	s32 index;
	switch (sfx) {
	case 0x86:
		index = 0;
		break;
	case 0x87:
		index = 1;
		break;
	case 0x88:
		index = 2;
		break;
	case 0x85:
		index = 3;
		break;
	case 0x84:
		index = 4;
		break;
	default:
		return sfx;
	}
	return gSoundMenuEffects[gSoundMenuSelections[index]];
}

// a music id's volume from musicTable, 0 for anything that isn't music
static inline u8 getMusicVolume(s32 sfx)
{
	if (!isAllSfx(sfx)) {
		return 0;
	}
	u8 idx = sfx;
	// retail tests > where PlayGameSound tests >=, so 0x6D reads one past musicTable's 109 entries
	if (idx > 0x6D) {
		idx = 0;
	}
	return musicTable[idx].volume;
}

static inline bool CanPlayNightStream()
{
	if (gGame.mTimeState.mMode == 1 && gGame.mTimeState.mTime > 0.5f)
		return false;
	return true;
}

static inline bool UseRaisedLiftPitch()
{
	return gGame.GetPlayer()->mLift.mState == 4 && IsLiftTargetPlug(&gGame.GetPlayer()->mLift);
}

/**
 * @note Address: 0x8001FCBC
 * @note Size: 0x38
 */
// particle sounds from joint anims
extern "C" void PlayGameSfxAtFullVolume(s32 sfx)
{
	PlayGameSound(&gGame.mSound, sfx, 255);
}

/**
 * @note Address: 0x8001FC54
 * @note Size: 0x68
 */
extern "C" void InitGameSound(XSound* sound)
{
	InitAudio(sound, 0x700000, 0x60, 0x20);
	sound->mSampleBank  = HSD_SynthSFXNewBank(0x480000);
	sound->mStageBank   = HSD_SynthSFXNewBank(0x280000);
	sound->mLoadState   = 0;
	sound->mSampleReady = 0;
	HSD_JObjSetJSoundCallback(PlayGameSfxAtFullVolume);
}

/**
 * @note Address: 0x8001F97C
 * @note Size: 0x2D8
 */
// a negative stage loads the shared samples a step per call, TRUE once they're in
extern "C" BOOL LoadGameSoundBank(XSound* sound, s32 stage)
{
	if (stage < 0) {
		switch (sound->mLoadState) {
		case 0:
			sound->mSampleReady = 0;
			if (sound->mFile.ReadAsync("sfx_sample.sem", 0, 0, FALSE) >= 0) {
				++sound->mLoadState;
			} else {
#if defined(VERSION_JP)
				OSPanic("sound.cpp", 0xED, "%s%s", "", "Read");
#else
				OSPanic("sound.cpp", 0xF1, "%s%s", "", "Read");
#endif
			}
			break;
		case 1: {
			s32 result = sound->mFile.GetResult();
			if (result == 0) {
				++sound->mLoadState;
			} else if (result < 0) {
#if defined(VERSION_JP)
				OSPanic("sound.cpp", 0xF5, "%s%s", "", "Read");
#else
				OSPanic("sound.cpp", 0xF9, "%s%s", "", "Read");
#endif
			}
			break;
		}
		case 2:
			LoadSoundSampleMetadata(sound->mFile.mBuffer);
			HSD_SynthSFXLoad("sfx_sample.ssm", sound->mSampleBank, 0, 0);
			++sound->mLoadState;
			break;
		case 3:
			if (!HasPendingSoundBankLoad()) {
				sound->mLoadState   = 0;
				sound->mSampleReady = 1;
				return TRUE;
			}
			break;
		default:
			sound->mLoadState = 0;
			break;
		}
		return FALSE;
	}
	sound->mDefaultTrack = 0;
	ClearStageBank(sound);
#if !defined(VERSION_PAL)
	OSReport(">>>%s\n", stageSfxTable[stage]);
#endif
	sound->registerName(stageSfxTable[stage], sound->mStageBank);
	return TRUE;
}

/**
 * @note Address: 0x8001F8D8
 * @note Size: 0xA4
 */
extern "C" void ClearStageSoundBank(XSound* sound)
{
	ClearStageBank(sound);
}

/**
 * @note Address: 0x8001F46C
 * @note Size: 0x46C
 */
// NONMATCHING: regalloc (stage request loop spills slwi into r29 and pointer into r28; retail uses r0/r29)
extern "C" void UpdateGameSound(XSound* sound, s32 step)
{
	if (!sound->mSampleReady) {
		return;
	}
	if (sound->mLoadRequestCount > 0 && !HasPendingSoundBankLoad()) {
		for (s32 i = 0; i < 16; i++) {
			if (sound->mStageRequests.entries[i].mName != NULL && !sound->mStageRequests.entries[i].mLoaded) {
#if !defined(VERSION_PAL)
				OSReport("HSD_AudioSFXLoad::%s\n", sound->mStageRequests.entries[i].mName);
#endif
				sound->mStageRequests.entries[i].mResult
				    = HSD_SynthSFXLoad(sound->mStageRequests.entries[i].mName, sound->mStageRequests.entries[i].mBank, 0, 0);
				sound->mStageRequests.entries[i].mLoaded = 1;
				sound->mLoadRequestCount--;
				break;
			}
		}
	}
	for (int i = 0; i < 32; i++) {
		sound->mStartedSfx[i] = 0;
	}
	if (sound->mCurrentMusicId != 0 && sound->mPausedMusicId == 0 && !AXDriverIsStreamActive()) {
		sound->mCurrentMusicId = 0;
	}
	const s32 pitchFrames = sound->mMusicPitchFrames;
	if (pitchFrames > 0 && sound->mMusicPitch != sound->mTargetMusicPitch) {
		sound->mMusicPitch += (sound->mTargetMusicPitch - sound->mMusicPitch) / pitchFrames;
		sound->mMusicPitchFrames -= step;
		if (sound->mMusicPitchFrames <= 0) {
			sound->mMusicPitch = sound->mTargetMusicPitch;
		}
		AXDriverSetAllStreamPitches(sound->mMusicPitch);
	}
	if (IsGameSoundPlaying(sound, 0xFFFF0005)) {
		if (UseRaisedLiftPitch()) {
			if (sound->mTargetMusicPitch != 200) {
				sound->mTargetMusicPitch = 200;
				sound->mMusicPitchFrames = 15;
			}
		} else if (sound->mTargetMusicPitch != 0) {
			sound->mTargetMusicPitch = 0;
			sound->mMusicPitchFrames = 15;
		}
	}
	// paused music comes back when mMusicResumeDelay runs out; no countdown while SetMusicPlaybackPaused holds playback
	if (sound->mDefaultTrack == 0 && sound->mMusicResumeDelay > 0) {
		if (!IsGameSoundPlaying(sound, 0x18C) && !IsGameSoundPlaying(sound, 0x18D) && !IsGameSoundPlaying(sound, 0x18E)) {
			sound->mMusicResumeDelay -= step;
		}
		if (sound->mMusicResumeDelay <= 0) {
			if (sound->mPausedMusicId == sound->mCurrentMusicId) {
				AXDriverResumeAllStreams();
			} else {
				// a no-op: PlayGameSound returns at once for 0xFFFF0000
				PlayGameSound(sound, 0xFFFF0000, 255);
			}
			sound->mPausedMusicId = 0;
		}
	}
	if (sound->mCurrentMusicId == 0xFFFF0010 && !CanPlayNightStream()) {
		if (sound->mMusicFadeOutFrames <= 0) {
			sound->mMusicFadeOutFrames = 180;
			sound->mMusicVolume        = 0;
			AXDriverSetAllStreamVolumes(0, 600);
		}
	}
	if (sound->mMusicFadeOutFrames > 0) {
		sound->mMusicFadeOutFrames -= step;
		if (sound->mMusicFadeOutFrames <= 0) {
			sound->mCurrentMusicId = 0;
			sound->mPausedMusicId  = 0;
			AXDriverStopAllStreams();
		}
	}
	if (sound->mCooldown > 0) {
		sound->mCooldown -= step;
	}
	if (sound->mQueueRead != sound->mQueueWrite) {
		if (sound->mMusicPitchFrames > 0) {
			sound->mMusicPitchFrames -= step;
		}
		if (sound->mLastQueuedSoundId != 0 && IsGameSoundPlaying(sound, sound->mLastQueuedSoundId)) {
			return;
		}
		if (sound->mMusicPitchFrames <= 0) {
			sound->mLastQueuedSoundId = sound->mQueuedSoundIds[sound->mQueueRead];
			PlayGameSound(&gGame.mSound, sound->mLastQueuedSoundId, 255);
			if (++sound->mQueueRead >= 64) {
				sound->mQueueRead = 0;
			}
		}
	}
}

/**
 * @note Address: 0x8001E904
 * @note Size: 0xB68
 */
// NONMATCHING: regalloc only; the call set, CFG and 732-instruction schedule otherwise agree.
extern "C" s32 PlayGameSound(XSound* sound, s32 sfx, u8 vol)
{
	if (isAllSfx(sfx)) {
		if (sfx == (s32)0xFFFF0000) {
			return 0;
		}
		// at night (time mode 1) bgm.hps, musicTable[5], gives way to bgm_night.hps, musicTable[0x10]
		if (sfx == (s32)0xFFFF0005 && gGame.mTimeState.mMode == 1) {
			sfx = 0xFFFF0010;
		}
		if (sfx == (s32)0xFFFF0010 && !CanPlayNightStream()) {
			stopSfx(sound, sfx);
			return 1;
		}
		u8 index = sfx & 0xFF;
		if (index >= 0x6D) {
			index = 0;
		}
		MusicEntry* music = musicTable + index;
		if (sound->mCurrentMusicId == sfx) {
			return 1;
		}
		sound->mCurrentMusicId = sfx;
		if (sfx == (s32)0xFFFF0005 && UseRaisedLiftPitch()) {
			sound->mMusicPitch = sound->mTargetMusicPitch = 200;
		} else {
			sound->mMusicPitch = sound->mTargetMusicPitch = 0;
		}
		sound->_2BC                = 0;
		sound->mMusicFadeOutFrames = 0;
		// 255 asks for the track's own volume from musicTable
		if (vol == 255) {
			vol = music->volume;
		}
		sound->mMusicVolume = vol;
		s32 result          = HSD_AudioPStreamStartChParam(music->path, vol, 31);
		AXDriverSetAllStreamPitches(sound->mMusicPitch);
		if (sfx == (s32)0xFFFF0005 || sfx == (s32)0xFFFF0010) {
			if (IsGameSoundPlaying(sound, 0x18C) || IsGameSoundPlaying(sound, 0x18D) || IsGameSoundPlaying(sound, 0x18E)) {
				if (isAllSfx(sound->mCurrentMusicId)) {
					PauseMusic(sound, 120);
				}
			}
		}
		return result;
	}

	s32 track = sound->mDefaultTrack;
	switch (sfx) {
	case 0x1B:
		sfx += HSD_Randi(4);
		break;
	case 0x19: {
		s32 surface = gGame.GetPlayer()->mGroundType;
		if (surface >= 13) {
			surface = 0;
		}
		s32 step;
		if (surface == 12) {
			step = -1;
		} else {
			switch (surface) {
			case 8:
				HSD_AudioSFXStartParam(0x19D, 70, 127, 0, 0);
				break;
			case 9:
				HSD_AudioSFXStartParam(0x19C, 255, 127, 0, 0);
				break;
			case 10:
				HSD_AudioSFXStartParam(0x19E, 50, 127, 0, 0);
				break;
			}
			step = sSurfaceSoundIds[surface][HSD_Randi(5)];
		}
		sfx = step;
		if (sfx < 0) {
			return -1;
		}
		if (sound->mMusicResumeDelay > 10) {
			sound->mMusicResumeDelay = 10;
		}
		break;
	}
	case 0x0A:
		// with vol 0 it is never started, only recorded in mStartedSfx below
		vol = 0;
		break;
	case 0x3E:
		// restarts the turn order of 0x11's and 0x15's sets
		sound->mAltSoundIndex = 0;
		break;
	case 0x11:
		sfx = SelectAlternatingSound(sound, sAlternatingSound15, sAlternatingSound11);
		if (sfx < 0) {
			return -1;
		}
		break;
	case 0x15:
		sfx = SelectAlternatingSound(sound, sAlternatingSound11, sAlternatingSound15);
		if (sfx < 0) {
			return -1;
		}
		break;
	case 0x3C:
		if (IsGameSoundPlaying(sound, sfx)) {
			return -1;
		}
		break;
	case 0xFD: {
		if (IsGameSoundPlaying(sound, 0xFFFF0000)) {
			PauseMusic(sound, 120);
		}
		s32 next;
		if (IsGameSoundPlaying(sound, 0xFD)) {
			next = -1;
		} else {
			if (sound->mVoiceSeqIndex < 0 || (u32)sound->mVoiceSeqIndex >= 25) {
				sound->mVoiceSeqIndex = 0;
			}
			next = sSoundFDSequence[sound->mVoiceSeqIndex++];
		}
		sfx = next;
		break;
	}
	case 0x110: {
		s32 next;
		// 0xFFFF0037 is cooking.hps
		if (IsGameSoundPlaying(sound, 0xFFFF0037)) {
			next = -1;
		} else {
			if (IsGameSoundPlaying(sound, 0xFFFF0000)) {
				PauseMusic(sound, 120);
			}
			if (sound->mVoiceSeqIndex < 0 || (u32)sound->mVoiceSeqIndex >= 8) {
				sound->mVoiceSeqIndex = 0;
			}
			next = sSound110Sequence[sound->mVoiceSeqIndex++];
		}
		sfx = next;
		if (sfx < 0) {
			return -1;
		}
		break;
	}
	case 0x175:
	case 0x176:
	case 0x177:
	case 0x178:
		if (IsGameSoundPlaying(sound, 0xFFFF0005)) {
			PauseMusic(sound, 180);
		}
		if (IsGameSoundPlaying(sound, 0xFFFF0010)) {
			PauseMusic(sound, 180);
		}
		sfx = sfx == 0x175 ? sSound175Variants[HSD_Randi(6)] : sSound176Variants[HSD_Randi(6)];
		break;
	case 0x137:
		if (sound->mSoundSeqIndex < 0 || (u32)sound->mSoundSeqIndex >= 11) {
			sound->mSoundSeqIndex = 0;
		}
		sfx = sSound137Sequence[sound->mSoundSeqIndex++];
		break;
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	case 0x88:
		sfx = SelectMappedSound(sfx);
		break;
	case 0x11D:
		// at most once in 60 frames, UpdateGameSound counts mCooldown down
		if (sound->mCooldown > 0) {
			return -1;
		}
		sound->mCooldown = 60;
		break;
	// pause bgm.hps or bgm_night.hps for 120 frames; UpdateGameSound holds that countdown while 0x18C-0x18E play
	case 0x44:
	case 0x18C:
	case 0x18D:
	case 0x18E:
		if (IsGameSoundPlaying(sound, 0xFFFF0005)) {
			PauseMusic(sound, 120);
		}
		if (IsGameSoundPlaying(sound, 0xFFFF0010)) {
			PauseMusic(sound, 120);
		}
		if (IsGameSoundPlaying(sound, 0x18C)) {
			AXDriverKeyOffSfx(0x18C);
		}
		if (IsGameSoundPlaying(sound, 0x18D)) {
			AXDriverKeyOffSfx(0x18D);
		}
		if (IsGameSoundPlaying(sound, 0x18E)) {
			AXDriverKeyOffSfx(0x18E);
		}
		break;
	case 0x11001C:
		if (IsGameSoundPlaying(sound, sfx)) {
			return -1;
		}
		break;
	case 0x186:
		if (IsGameSoundPlaying(sound, sfx)) {
			return -1;
		}
		break;
	case 0x184:
		if (IsGameSoundPlaying(sound, sfx)) {
			return -1;
		}
		break;
	case 0x4001C:
	case 0x4001D:
		if (IsGameSoundPlaying(sound, sfx)) {
			return -1;
		}
		break;
	case 0x190:
	case 0x191:
	case 0x10001F:
		if (IsGameSoundPlaying(sound, sfx)) {
			return -1;
		}
		break;
	}

	s32 result;
	if (vol != 0) {
		result = HSD_AudioSFXStartParam(sfx, vol, 127, 0, track);
	} else {
		result = 1;
	}
	u32 i = 0;
	while (i < 32 && sound->mStartedSfx[i] != 0) {
		i++;
	}
	if (i < 32) {
		sound->mStartedSfx[i] = sfx;
	}
	switch (sfx) {
	case 9:
		HSD_AudioSFXSetPitchByID(sfx, 60 - HSD_Randi(20) * 6);
		break;
	}
	return result;
}

/**
 * @note Address: 0x8001E6C8
 * @note Size: 0x23C
 */
extern "C" BOOL IsGameSoundPlaying(XSound* sound, s32 sfx)
{
	// steps by two, so only even slots are searched though PlayGameSound fills them in order: an id in an odd slot is missed
	for (u32 i = 0; i < 32 && sound->mStartedSfx[i] != 0; i += 2) {
		if (sound->mStartedSfx[i] == sfx) {
			return TRUE;
		}
	}

	if (isAllSfx(sfx)) {
		if (sound->mCurrentMusicId == sfx) {
			return TRUE;
		}
		// 0xFFFF0000 asks whether any music is playing
		if (sfx == 0xFFFF0000 && sound->mCurrentMusicId != 0) {
			return TRUE;
		}
		return FALSE;
	}

	// 0x1B and 0xFD stand for every sound PlayGameSound picks for them: 0x1B-0x1E, and sSoundFDSequence's 0xFD-0x10B
	switch (sfx) {
	case 0x1B:
		if (HSD_AudioSFXCountByID(sfx) || HSD_AudioSFXCountByID(sfx + 1) || HSD_AudioSFXCountByID(sfx + 2)
		    || HSD_AudioSFXCountByID(sfx + 3)) {
			return TRUE;
		}
		return FALSE;
	case 0xFD:
		if (HSD_AudioSFXCountByID(0xFD) || HSD_AudioSFXCountByID(0xFE) || HSD_AudioSFXCountByID(0xFF) || HSD_AudioSFXCountByID(0x100)
		    || HSD_AudioSFXCountByID(0x101) || HSD_AudioSFXCountByID(0x102) || HSD_AudioSFXCountByID(0x103) || HSD_AudioSFXCountByID(0x104)
		    || HSD_AudioSFXCountByID(0x105) || HSD_AudioSFXCountByID(0x106) || HSD_AudioSFXCountByID(0x107) || HSD_AudioSFXCountByID(0x108)
		    || HSD_AudioSFXCountByID(0x109) || HSD_AudioSFXCountByID(0x10A) || HSD_AudioSFXCountByID(0x10B)) {
			return TRUE;
		}
		return FALSE;
	default:
		return HSD_AudioSFXCountByID(sfx);
	}
}

/**
 * @note Address: 0x8001E660
 * @note Size: 0x68
 */
extern "C" BOOL StopGameSfx(XSound* sound, s32 sfx)
{
	if (isAllSfx(sfx)) {
		sound->mCurrentMusicId = 0;
		sound->mPausedMusicId  = 0;
		AXDriverStopAllStreams();
		return TRUE;
	}
	return AXDriverKeyOffSfx(sfx);
}

/**
 * @note Address: 0x8001E630
 * @note Size: 0x30
 */
extern "C" void KeyOffGameSoundTrack(XSound* sound, s32 track)
{
	if (track < 0) {
		track = sound->mDefaultTrack;
	}
	HSD_AudioSFXKeyOffTrack(track);
}

/**
 * @note Address: 0x8001E5C0
 * @note Size: 0x70
 */
// a resumeDelay of 0 never runs out: UpdateGameSound only counts down a delay above 0
extern "C" void PauseGameMusic(XSound* sound, s32 sfx, s32 resumeDelay)
{
	if (isAllSfx(sfx)) {
		AXDriverPauseAllStreams();
		sound->mMusicResumeDelay = resumeDelay;
		sound->mPausedMusicId    = sound->mCurrentMusicId;
	}
}

/**
 * @note Address: 0x8001E590
 * @note Size: 0x30
 */
extern "C" void PauseGameSoundTrack(XSound* sound, s32 track)
{
	if (track < 0) {
		track = sound->mDefaultTrack;
	}
	AXDriverPauseTrack(track);
}

/**
 * @note Address: 0x8001E504
 * @note Size: 0x8C
 */
extern "C" void ResumeGameSfx(XSound* sound, s32 sfx)
{
	if (isAllSfx(sfx)) {
		if (sound->mPausedMusicId == sound->mCurrentMusicId) {
			AXDriverResumeAllStreams();
		} else {
			PlayGameSound(sound, sfx, 0xFF);
		}
		sound->mPausedMusicId = 0;
	} else {
		AXDriverResumeSfx(sfx);
	}
}

/**
 * @note Address: 0x8001E4D4
 * @note Size: 0x30
 */
extern "C" void ResumeGameSoundTrack(XSound* sound, s32 track)
{
	if (track < 0) {
		track = sound->mDefaultTrack;
	}
	AXDriverResumeTrack(track);
}

/**
 * @note Address: 0x8001E468
 * @note Size: 0x6C
 */
extern "C" void SetGameSfxVolume(XSound* sound, s32 sfx, u8 vol, u16 frames)
{
	setSfxVolume(sound, sfx, vol, frames);
}

/**
 * @note Address: 0x8001E2CC
 * @note Size: 0x19C
 */
extern "C" void FadeInGameSfx(XSound* sound, s32 sfx, u16 frames)
{
	if (isAllSfx(sfx)) {
		PlayGameSound(sound, sfx, 0);
		setSfxVolume(sound, sfx, getMusicVolume(sfx), frames);
	} else {
		if (!IsGameSoundPlaying(sound, sfx)) {
			PlayGameSound(sound, sfx, 0xFF);
		}
		setSfxVolume(sound, sfx, 0xFF, frames);
	}
}

/**
 * @note Address: 0x8001E1A4
 * @note Size: 0x128
 */
extern "C" void FadeOutGameSfx(XSound* sound, s32 sfx, u16 frames)
{
	if (isAllSfx(sfx)) {
		sound->mMusicFadeOutFrames = (60.0f * frames) / 200.0f;
		setSfxVolume(sound, sfx, 0, frames);
	} else {
		setSfxVolume(sound, sfx, 0, frames);
	}
}

/**
 * @note Address: 0x8001E11C
 * @note Size: 0x88
 */
extern "C" void SetMusicPlaybackPaused(XSound* sound, BOOL pause)
{
	if (pause) {
		XSound* game = &gGame.mSound;
		AXDriverPauseTrack(game->resolveTrack(0));
		AXDriverPauseAllStreams();
		// sfx PlayGameSound starts from now on go to track 1, which this does not pause
		game->mDefaultTrack = 1;
	} else {
		XSound* game = &gGame.mSound;
		AXDriverResumeTrack(game->resolveTrack(0));
		AXDriverResumeAllStreams();
		game->mDefaultTrack = 0;
	}
}

/**
 * @note Address: 0x8001E044
 * @note Size: 0xD8
 */
// NONMATCHING: regalloc
extern "C" BOOL RegisterGameSoundName(XSound* sound, const char* name)
{
	sound->registerName(name, sound->mStageBank);
	return FALSE;
}

/**
 * @note Address: 0x8001E00C
 * @note Size: 0x38
 */
extern "C" void SetGameSoundStereo(XSound* sound, s32 stereo)
{
	if (stereo == 1) {
		HSD_SynthSetSoundMode(1);
	} else {
		HSD_SynthSetSoundMode(0);
	}
}

/**
 * @note Address: 0x8001DF00
 * @note Size: 0x10C
 */
extern "C" u8 GetSoundVolumeAtPosition(XSound* sound, Vec* pos, s32 kind)
{
	f32 dist   = PSVECDistance(&gGame.mMotionControlCamera.mCamera.mInterest, pos);
	f32* range = gSoundFalloffRanges[kind];

	if (range[0] < dist) {
		return 0;
	}
	if (range[1] < dist) {
		return 127.0f - (64.0f * (dist - range[1])) / (range[0] - range[1]);
	}
	if (range[2] < dist) {
		return 171.0f - (64.0f * (dist - range[2])) / (range[1] - range[2]);
	}
	if (range[3] < dist) {
		return 255.0f - (64.0f * (dist - range[3])) / (range[2] - range[3]);
	}
	return 255;
}

/**
 * @note Address: 0x8001DC20
 * @note Size: 0x2E0
 */
// msg.cpp calls this for each character it prints, with the message's mGlyphSoundGroup and its font size / 22
// as strength; first and second are the character's bytes and go unused
extern "C" void PlayMessageGlyphSound(XSound* sound, u32 group, f32 strength, u8 first, u8 second)
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
			if (IsGameSoundPlaying(sound, ids[i])) {
				return;
			}
		}
		s32 index = HSD_Randi(count);
		// vol is a u8: a volume over 255 plays the sound twice, the part above 256 and a full one
		if (volume >= 512) {
			volume = 511;
		}
		PlayGameSound(sound, ids[index], volume & 0xFF);
		if (volume > 255) {
			PlayGameSound(sound, ids[index], 255);
		}
	}
}

/**
 * @note Address: 0x8001DC10
 * @note Size: 0x10
 */
// empties the sound's queue
extern "C" void ClearGameSoundQueue(XSound* sound)
{
	sound->mQueueWrite = 0;
	sound->mQueueRead  = 0;
}

#if defined(VERSION_JP)
/**
 * @note Address: JP 0x8001DBC8
 * @note Size: 0x2C
 */
extern "C" s32 GetSoundMenuSelection(XSound*, s32 index)
{
	if (index < 0 || index >= ARRAY_SIZE(gSoundMenuSelections))
		return -1;
	return gSoundMenuSelections[index];
}

/**
 * @note Address: JP 0x8001DB48
 * @note Size: 0x80
 */
// the japanese sound menu: plays the effect picked for a row and remembers the pick
extern "C" void SetSoundMenuSelection(XSound* sound, s32 index, s32 selection)
{
	if (index < 0 || index >= ARRAY_SIZE(gSoundMenuSelections))
		return;
	if (selection < 0 || selection >= ARRAY_SIZE(gSoundMenuEffects))
		return;
	PlayGameSound(sound, gSoundMenuEffects[selection], 0xFF);
	gSoundMenuSelections[index] = selection;
}
#endif

/**
 * @note Address: 0x8001DB98
 * @note Size: 0x78
 */
// whether the event's start or end frame went by
BOOL AnimSfx::CheckStart(AnimSfxEvent* event, f32 min, f32 max)
{
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
 * @note Address: 0x8001DB90
 * @note Size: 0x8
 */
BOOL AnimSfx::CheckStop(AnimSfxEvent* event)
{
	return FALSE;
}

/**
 * @note Address: 0x8001DB48
 * @note Size: 0x48
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
 * @note Address: 0x8001DB44
 * @note Size: 0x4
 */
void AnimSfx::vf08()
{
}

/**
 * @note Address: 0x8001D9B0
 * @note Size: 0x194
 */
void AnimSfx::Update(s32 anim, f32 frame, u8 vol)
{
	XSound* sound;
	s32 prev = mAnimId;

	if (prev != anim) {
		mAnimId = anim;
		mFrame  = 0.0f;
	}
	sound = &gGame.mSound;
	AnimSfxEvent* event;
	for (s32 i = 0; mEvents != NULL && (event = &mEvents[i])->mAnim >= 0; i++) { // the table ends at -1

		if (event->mAnim == anim || event->mAnim == prev) {
			if (prev != anim && event->mAnim == prev && CheckStop(event)) {
				stopSfx(sound, GetSfx(event));
			}
			if (event->mAnim == anim && CheckStart(event, mFrame, frame)) {
				PlayGameSound(sound, GetSfx(event), vol);
			}
		}
	}
	mFrame = frame;
}

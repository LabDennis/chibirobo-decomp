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

#if defined(VERSION_JP)
#define SOUND_PLAYER_SURFACE (*(s32*)((u8*)gGame.GetPlayer() + 0x14480))
#else
#define SOUND_PLAYER_SURFACE (*(s32*)((u8*)gGame.GetPlayer() + 0x158C0))
#endif

extern "C" {
s32 HSD_AudioSFXStartParam(u32 sfx, u8 vol, u8 pan, s32 group, s32 track);
BOOL HSD_AudioSFXSetPitchByID(s32 sfx, s16 pitch);
s32 HSD_AudioPStreamStartChParam(const char* path, u8 vol, s32 channel);
s32 HSD_SynthSFXLoad(const char* name, s32 bank, void (*callback)(s32, s32), s32 arg);
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

extern "C" s32 HSD_Randi(s32 max);
extern "C" void HSD_SynthSetSoundMode(s32 mode);

static inline s32 SelectAlternatingSound(XSound* sound, s32* stop, s32* play)
{
	for (u32 i = 0; i < 4; i++) {
		if (IsGameSoundPlaying(sound, stop[i])) {
			stopSfx(sound, stop[i]);
			sound->_18C = 0;
		}
	}
	for (u32 i = 0; i < 4; i++) {
		if (IsGameSoundPlaying(sound, play[i])) {
			return -1;
		}
	}
	if (sound->_18C < 0 || (u32)sound->_18C >= 4) {
		sound->_18C = 0;
	}
	return play[sound->_18C++];
}

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
 * @note Address: 0x8001DC20
 * @note Size: 0x2E0
 */
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
		if (volume >= 512) {
			volume = 511;
		}
		PlayGameSound(sound, ids[index], volume);
		if (volume > 255) {
			PlayGameSound(sound, ids[index], 255);
		}
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

inline void XSound::registerName(const char* name, s32 bank)
{
	if (name == NULL) {
		return;
	}
	for (u32 i = 0; i < 16; ++i) {
		if (mStageRequests.entries[i].mName != NULL && strcmp(mStageRequests.entries[i].mName, name) == 0) {
			return;
		}
	}
	for (u32 i = 0; i < 16; ++i) {
		if (mStageRequests.entries[i].mName == NULL) {
			++mLoadRequestCount;
			mStageRequests.entries[i].mName = name;
			mStageRequests.entries[i].mBank = bank;
#if !defined(VERSION_PAL)
			OSReport("Register:%s\n", name);
#endif
			return;
		}
	}
#if !defined(VERSION_PAL)
	OSReport("Ssm::Load xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
#endif
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
 * @note Address: 0x8001E11C
 * @note Size: 0x88
 */
extern "C" void SetMusicPlaybackPaused(XSound* sound, BOOL on)
{
	if (on) {
		XSound* game = (XSound*)&gGame.mSound;
		AXDriverPauseTrack(game->resolveTrack(0));
		AXDriverPauseAllStreams();
		game->mDefaultTrack = 1;
	} else {
		XSound* game = (XSound*)&gGame.mSound;
		AXDriverResumeTrack(game->resolveTrack(0));
		AXDriverResumeAllStreams();
		game->mDefaultTrack = 0;
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
 * @note Address: 0x8001E468
 * @note Size: 0x6C
 */
extern "C" void SetGameSfxVolume(XSound* sound, s32 sfx, u8 vol, u16 frames)
{
	setSfxVolume(sound, sfx, vol, frames);
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
 * @note Address: 0x8001E5C0
 * @note Size: 0x70
 */
extern "C" void PauseGameMusic(XSound* sound, s32 sfx, s32 arg2)
{
	if (isAllSfx(sfx)) {
		AXDriverPauseAllStreams();
		sound->mMusicResumeDelay = arg2;
		sound->mPausedMusicId    = sound->mCurrentMusicId;
	}
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
 * @note Address: 0x8001E6C8
 * @note Size: 0x23C
 */
extern "C" BOOL IsGameSoundPlaying(XSound* sound, s32 sfx)
{
	// steps by two so it only ever looks at every other slot, nice one
	for (u32 i = 0; i < 32 && sound->_108[i] != 0; i += 2) {
		if (sound->_108[i] == sfx) {
			return TRUE;
		}
	}

	if (isAllSfx(sfx)) {
		if (sound->mCurrentMusicId == sfx) {
			return TRUE;
		}
		if (sfx == 0xFFFF0000 && sound->mCurrentMusicId != 0) {
			return TRUE;
		}
		return FALSE;
	}

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
 * @note Address: 0x8001E904
 * @note Size: 0xB68
 */
extern "C" s32 PlayGameSound(XSound* sound, s32 sfx, u8 vol)
{
	if (isAllSfx(sfx)) {
		if (sfx == (s32)0xFFFF0000) {
			return 0;
		}
		if (sfx == (s32)0xFFFF0005 && gGame.mTimeState.mMode == 1) {
			sfx = 0xFFFF0010;
		}
		if (sfx == (s32)0xFFFF0010 && !CanPlayNightStream()) {
			stopSfx(sound, sfx);
			return 1;
		}
		u8 index = sfx;
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
		s32 surface = SOUND_PLAYER_SURFACE;
		if (surface >= 13) {
			surface = 0;
		}
		if (surface == 12) {
			sfx = -1;
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
			sfx = sSurfaceSoundIds[surface][HSD_Randi(5)];
		}
		if (sfx < 0) {
			return -1;
		}
		if (sound->mMusicResumeDelay > 10) {
			sound->mMusicResumeDelay = 10;
		}
		break;
	}
	case 0x0A:
		vol = 0;
		break;
	case 0x3E:
		sound->_18C = 0;
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
	case 0xFD:
		if (IsGameSoundPlaying(sound, 0xFFFF0000)) {
			PauseMusic(sound, 120);
		}
		if (IsGameSoundPlaying(sound, 0xFD)) {
			sfx = -1;
		} else {
			if (sound->_190 < 0 || (u32)sound->_190 >= 25) {
				sound->_190 = 0;
			}
			sfx = sSoundFDSequence[sound->_190++];
		}
		break;
	case 0x110:
		if (IsGameSoundPlaying(sound, 0xFFFF0037)) {
			sfx = -1;
		} else {
			if (IsGameSoundPlaying(sound, 0xFFFF0000)) {
				PauseMusic(sound, 120);
			}
			if (sound->_190 < 0 || (u32)sound->_190 >= 8) {
				sound->_190 = 0;
			}
			sfx = sSound110Sequence[sound->_190++];
		}
		if (sfx < 0) {
			return -1;
		}
		break;
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
		if (sfx == 0x175) {
			sfx = sSound175Variants[HSD_Randi(6)];
		} else {
			sfx = sSound176Variants[HSD_Randi(6)];
		}
		break;
	case 0x137:
		if (sound->_194 < 0 || (u32)sound->_194 >= 11) {
			sound->_194 = 0;
		}
		sfx = sSound137Sequence[sound->_194++];
		break;
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	case 0x88:
		sfx = SelectMappedSound(sfx);
		break;
	case 0x11D:
		if (sound->_198 > 0) {
			return -1;
		}
		sound->_198 = 60;
		break;
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
	while (i < 32 && sound->_108[i] != 0) {
		i++;
	}
	if (i < 32) {
		sound->_108[i] = sfx;
	}
	switch (sfx) {
	case 9:
		HSD_AudioSFXSetPitchByID(sfx, 60 - HSD_Randi(20) * 6);
		break;
	}
	return result;
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stmw     r26, 0x8(r1)
    mr       r28, r4
    lis      r4, lbl_802107A0@ha
    mr       r27, r3
    cmpwi    r28, -0x1
    mr       r29, r5
    addi     r31, r4, lbl_802107A0@l
    bne      .L_8001E938
    li       r0, 0x0
    b        .L_8001E94C
.L_8001E938:
    clrrwi   r3, r28, 16
    lis      r0, 0xffff
    subf     r0, r3, r0
    cntlzw   r0, r0
    srwi     r0, r0, 5
.L_8001E94C:
    clrlwi.  r0, r0, 24
    beq      .L_8001EB9C
    addis    r0, r28, 0x1
    cmplwi   r0, 0x0
    bne      .L_8001E968
    li       r3, 0x0
    b        .L_8001F458
.L_8001E968:
    cmplwi   r0, 0x5
    bne      .L_8001E990
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    lwz      r0, 0x5d28(r3)
    cmpwi    r0, 0x1
    bne      .L_8001E990
    lis      r3, 0xffff
    addi     r28, r3, 0x10
.L_8001E990:
    addis    r0, r28, 0x1
    cmplwi   r0, 0x10
    bne      .L_8001EA28
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    lwz      r0, 0x5d28(r3)
    cmpwi    r0, 0x1
    bne      .L_8001E9CC
    lfs      f1, 0x5d2c(r3)
    lfs      f0, lbl_8065C090@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_8001E9CC
    li       r0, 0x0
    b        .L_8001E9D0
.L_8001E9CC:
    li       r0, 0x1
.L_8001E9D0:
    clrlwi.  r0, r0, 24
    bne      .L_8001EA28
    cmpwi    r28, -0x1
    mr       r3, r28
    bne      .L_8001E9EC
    li       r0, 0x0
    b        .L_8001EA00
.L_8001E9EC:
    clrrwi   r4, r28, 16
    lis      r0, 0xffff
    subf     r0, r4, r0
    cntlzw   r0, r0
    srwi     r0, r0, 5
.L_8001EA00:
    clrlwi.  r0, r0, 24
    beq      .L_8001EA1C
    li       r0, 0x0
    stw      r0, 0x2b4(r27)
    stw      r0, 0x2c8(r27)
    bl       AXDriverStopAllStreams
    b        .L_8001EA20
.L_8001EA1C:
    bl       AXDriverKeyOffSfx
.L_8001EA20:
    li       r3, 0x1
    b        .L_8001F458
.L_8001EA28:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0x6d
    mr       r3, r0
    blt      .L_8001EA3C
    li       r3, 0x0
.L_8001EA3C:
    lwz      r0, 0x2b4(r27)
    clrlslwi r3, r3, 24, 3
    addi     r26, r31, 0x108
    cmpw     r0, r28
    add      r26, r26, r3
    bne      .L_8001EA5C
    li       r3, 0x1
    b        .L_8001F458
.L_8001EA5C:
    addis    r0, r28, 0x1
    stw      r28, 0x2b4(r27)
    cmplwi   r0, 0x5
    bne      .L_8001EAB8
    lis      r3, gGame@ha
    li       r30, 0x0
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    lwz      r0, 0xa48(r3)
    cmpwi    r0, 0x4
    bne      .L_8001EAA0
    addi     r3, r3, 0xa44
    bl       IsLiftTargetPlug
    cmpwi    r3, 0x0
    beq      .L_8001EAA0
    li       r30, 0x1
.L_8001EAA0:
    clrlwi.  r0, r30, 24
    beq      .L_8001EAB8
    li       r0, 0xc8
    sth      r0, 0x2ba(r27)
    sth      r0, 0x2b8(r27)
    b        .L_8001EAC4
.L_8001EAB8:
    li       r0, 0x0
    sth      r0, 0x2ba(r27)
    sth      r0, 0x2b8(r27)
.L_8001EAC4:
    li       r3, 0x0
    clrlwi   r0, r29, 24
    stw      r3, 0x2bc(r27)
    cmplwi   r0, 0xff
    stw      r3, 0x2c4(r27)
    bne      .L_8001EAE0
    lbz      r29, 0x4(r26)
.L_8001EAE0:
    stb      r29, 0x2cc(r27)
    mr       r4, r29
    li       r5, 0x1f
    lwz      r3, 0x0(r26)
    bl       HSD_AudioPStreamStartChParam
    mr       r29, r3
    lha      r3, 0x2b8(r27)
    bl       AXDriverSetAllStreamPitches
    addis    r0, r28, 0x1
    cmplwi   r0, 0x5
    beq      .L_8001EB14
    cmplwi   r0, 0x10
    bne      .L_8001EB94
.L_8001EB14:
    mr       r3, r27
    li       r4, 0x18c
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_8001EB50
    mr       r3, r27
    li       r4, 0x18d
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_8001EB50
    mr       r3, r27
    li       r4, 0x18e
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001EB94
.L_8001EB50:
    lwz      r0, 0x2b4(r27)
    cmpwi    r0, -0x1
    bne      .L_8001EB64
    li       r0, 0x0
    b        .L_8001EB78
.L_8001EB64:
    clrrwi   r3, r0, 16
    lis      r0, 0xffff
    subf     r0, r3, r0
    cntlzw   r0, r0
    srwi     r0, r0, 5
.L_8001EB78:
    clrlwi.  r0, r0, 24
    beq      .L_8001EB94
    bl       AXDriverPauseAllStreams
    li       r0, 0x78
    stw      r0, 0x2c0(r27)
    lwz      r0, 0x2b4(r27)
    stw      r0, 0x2c8(r27)
.L_8001EB94:
    mr       r3, r29
    b        .L_8001F458
.L_8001EB9C:
    cmpwi    r28, 0x110
    lwz      r30, 0xfc(r27)
    beq      .L_8001F090
    bge      .L_8001EC38
    cmpwi    r28, 0x3c
    beq      .L_8001EFF4
    bge      .L_8001EC00
    cmpwi    r28, 0x15
    beq      .L_8001EEFC
    bge      .L_8001EBDC
    cmpwi    r28, 0x11
    beq      .L_8001EE04
    bge      .L_8001F3C4
    cmpwi    r28, 0xa
    beq      .L_8001EDF0
    b        .L_8001F3C4
.L_8001EBDC:
    cmpwi    r28, 0x1a
    beq      .L_8001F3C4
    bge      .L_8001EBF4
    cmpwi    r28, 0x19
    bge      .L_8001ED00
    b        .L_8001F3C4
.L_8001EBF4:
    cmpwi    r28, 0x1c
    bge      .L_8001F3C4
    b        .L_8001ECF0
.L_8001EC00:
    cmpwi    r28, 0x84
    bge      .L_8001EC20
    cmpwi    r28, 0x44
    beq      .L_8001F288
    bge      .L_8001F3C4
    cmpwi    r28, 0x3e
    beq      .L_8001EDF8
    b        .L_8001F3C4
.L_8001EC20:
    cmpwi    r28, 0xfd
    beq      .L_8001F010
    bge      .L_8001F3C4
    cmpwi    r28, 0x89
    bge      .L_8001F3C4
    b        .L_8001F1EC
.L_8001EC38:
    cmpwi    r28, 0x18c
    bge      .L_8001EC90
    cmpwi    r28, 0x179
    bge      .L_8001EC6C
    cmpwi    r28, 0x137
    beq      .L_8001F1B4
    bge      .L_8001EC60
    cmpwi    r28, 0x11d
    beq      .L_8001F268
    b        .L_8001F3C4
.L_8001EC60:
    cmpwi    r28, 0x175
    bge      .L_8001F120
    b        .L_8001F3C4
.L_8001EC6C:
    cmpwi    r28, 0x185
    beq      .L_8001F3C4
    bge      .L_8001EC84
    cmpwi    r28, 0x184
    bge      .L_8001F370
    b        .L_8001F3C4
.L_8001EC84:
    cmpwi    r28, 0x187
    bge      .L_8001F3C4
    b        .L_8001F354
.L_8001EC90:
    lis      r3, 0x4
    addi     r0, r3, 0x1e
    cmpw     r28, r0
    bge      .L_8001ECC8
    cmpwi    r28, 0x192
    bge      .L_8001ECB8
    cmpwi    r28, 0x18f
    beq      .L_8001F3C4
    bge      .L_8001F3A8
    b        .L_8001F288
.L_8001ECB8:
    addi     r0, r3, 0x1c
    cmpw     r28, r0
    bge      .L_8001F38C
    b        .L_8001F3C4
.L_8001ECC8:
    lis      r3, 0x11
    addi     r0, r3, 0x1c
    cmpw     r28, r0
    beq      .L_8001F338
    bge      .L_8001F3C4
    lis      r3, 0x10
    addi     r0, r3, 0x1f
    cmpw     r28, r0
    beq      .L_8001F3A8
    b        .L_8001F3C4
.L_8001ECF0:
    li       r3, 0x4
    bl       HSD_Randi
    add      r28, r28, r3
    b        .L_8001F3C4
.L_8001ED00:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r28, 0x58c0(r3)
    cmpwi    r28, 0xd
    blt      .L_8001ED24
    li       r28, 0x0
.L_8001ED24:
    cmpwi    r28, 0xc
    bne      .L_8001ED34
    li       r0, -0x1
    b        .L_8001EDC4
.L_8001ED34:
    cmpwi    r28, 0x9
    beq      .L_8001ED74
    bge      .L_8001ED4C
    cmpwi    r28, 0x8
    bge      .L_8001ED58
    b        .L_8001EDA8
.L_8001ED4C:
    cmpwi    r28, 0xb
    bge      .L_8001EDA8
    b        .L_8001ED90
.L_8001ED58:
    li       r3, 0x19d
    li       r4, 0x46
    li       r5, 0x7f
    li       r6, 0x0
    li       r7, 0x0
    bl       HSD_AudioSFXStartParam
    b        .L_8001EDA8
.L_8001ED74:
    li       r3, 0x19c
    li       r4, 0xff
    li       r5, 0x7f
    li       r6, 0x0
    li       r7, 0x0
    bl       HSD_AudioSFXStartParam
    b        .L_8001EDA8
.L_8001ED90:
    li       r3, 0x19e
    li       r4, 0x32
    li       r5, 0x7f
    li       r6, 0x0
    li       r7, 0x0
    bl       HSD_AudioSFXStartParam
.L_8001EDA8:
    li       r3, 0x5
    bl       HSD_Randi
    mulli    r0, r28, 0x14
    slwi     r4, r3, 2
    addi     r3, r31, 0x4c0
    add      r0, r0, r4
    lwzx     r0, r3, r0
.L_8001EDC4:
    cmpwi    r0, 0x0
    mr       r28, r0
    bge      .L_8001EDD8
    li       r3, -0x1
    b        .L_8001F458
.L_8001EDD8:
    lwz      r0, 0x2c0(r27)
    cmpwi    r0, 0xa
    ble      .L_8001F3C4
    li       r0, 0xa
    stw      r0, 0x2c0(r27)
    b        .L_8001F3C4
.L_8001EDF0:
    li       r29, 0x0
    b        .L_8001F3C4
.L_8001EDF8:
    li       r0, 0x0
    stw      r0, 0x18c(r27)
    b        .L_8001F3C4
.L_8001EE04:
    li       r28, 0x0
    addi     r26, r31, 0x5d4
.L_8001EE0C:
    lwz      r4, 0x0(r26)
    mr       r3, r27
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001EE70
    lwz      r3, 0x0(r26)
    cmpwi    r3, -0x1
    bne      .L_8001EE34
    li       r0, 0x0
    b        .L_8001EE48
.L_8001EE34:
    clrrwi   r4, r3, 16
    lis      r0, 0xffff
    subf     r0, r4, r0
    cntlzw   r0, r0
    srwi     r0, r0, 5
.L_8001EE48:
    clrlwi.  r0, r0, 24
    beq      .L_8001EE64
    li       r0, 0x0
    stw      r0, 0x2b4(r27)
    stw      r0, 0x2c8(r27)
    bl       AXDriverStopAllStreams
    b        .L_8001EE68
.L_8001EE64:
    bl       AXDriverKeyOffSfx
.L_8001EE68:
    li       r0, 0x0
    stw      r0, 0x18c(r27)
.L_8001EE70:
    addi     r28, r28, 0x1
    addi     r26, r26, 0x4
    cmplwi   r28, 0x4
    blt      .L_8001EE0C
    li       r28, 0x0
    addi     r26, r31, 0x5c4
.L_8001EE88:
    lwz      r4, 0x0(r26)
    mr       r3, r27
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001EEA4
    li       r0, -0x1
    b        .L_8001EEE8
.L_8001EEA4:
    addi     r28, r28, 0x1
    addi     r26, r26, 0x4
    cmplwi   r28, 0x4
    blt      .L_8001EE88
    lwz      r0, 0x18c(r27)
    cmpwi    r0, 0x0
    blt      .L_8001EEC8
    cmplwi   r0, 0x4
    blt      .L_8001EED0
.L_8001EEC8:
    li       r0, 0x0
    stw      r0, 0x18c(r27)
.L_8001EED0:
    lwz      r5, 0x18c(r27)
    addi     r3, r31, 0x5c4
    addi     r4, r5, 0x1
    slwi     r0, r5, 2
    stw      r4, 0x18c(r27)
    lwzx     r0, r3, r0
.L_8001EEE8:
    cmpwi    r0, 0x0
    mr       r28, r0
    bge      .L_8001F3C4
    li       r3, -0x1
    b        .L_8001F458
.L_8001EEFC:
    li       r28, 0x0
    addi     r26, r31, 0x5c4
.L_8001EF04:
    lwz      r4, 0x0(r26)
    mr       r3, r27
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001EF68
    lwz      r3, 0x0(r26)
    cmpwi    r3, -0x1
    bne      .L_8001EF2C
    li       r0, 0x0
    b        .L_8001EF40
.L_8001EF2C:
    clrrwi   r4, r3, 16
    lis      r0, 0xffff
    subf     r0, r4, r0
    cntlzw   r0, r0
    srwi     r0, r0, 5
.L_8001EF40:
    clrlwi.  r0, r0, 24
    beq      .L_8001EF5C
    li       r0, 0x0
    stw      r0, 0x2b4(r27)
    stw      r0, 0x2c8(r27)
    bl       AXDriverStopAllStreams
    b        .L_8001EF60
.L_8001EF5C:
    bl       AXDriverKeyOffSfx
.L_8001EF60:
    li       r0, 0x0
    stw      r0, 0x18c(r27)
.L_8001EF68:
    addi     r28, r28, 0x1
    addi     r26, r26, 0x4
    cmplwi   r28, 0x4
    blt      .L_8001EF04
    li       r28, 0x0
    addi     r26, r31, 0x5d4
.L_8001EF80:
    lwz      r4, 0x0(r26)
    mr       r3, r27
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001EF9C
    li       r0, -0x1
    b        .L_8001EFE0
.L_8001EF9C:
    addi     r28, r28, 0x1
    addi     r26, r26, 0x4
    cmplwi   r28, 0x4
    blt      .L_8001EF80
    lwz      r0, 0x18c(r27)
    cmpwi    r0, 0x0
    blt      .L_8001EFC0
    cmplwi   r0, 0x4
    blt      .L_8001EFC8
.L_8001EFC0:
    li       r0, 0x0
    stw      r0, 0x18c(r27)
.L_8001EFC8:
    lwz      r5, 0x18c(r27)
    addi     r3, r31, 0x5d4
    addi     r4, r5, 0x1
    slwi     r0, r5, 2
    stw      r4, 0x18c(r27)
    lwzx     r0, r3, r0
.L_8001EFE0:
    cmpwi    r0, 0x0
    mr       r28, r0
    bge      .L_8001F3C4
    li       r3, -0x1
    b        .L_8001F458
.L_8001EFF4:
    mr       r3, r27
    mr       r4, r28
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F3C4
    li       r3, -0x1
    b        .L_8001F458
.L_8001F010:
    mr       r3, r27
    lis      r4, 0xffff
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F038
    bl       AXDriverPauseAllStreams
    li       r0, 0x78
    stw      r0, 0x2c0(r27)
    lwz      r0, 0x2b4(r27)
    stw      r0, 0x2c8(r27)
.L_8001F038:
    mr       r3, r27
    li       r4, 0xfd
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F054
    li       r0, -0x1
    b        .L_8001F088
.L_8001F054:
    lwz      r0, 0x190(r27)
    cmpwi    r0, 0x0
    blt      .L_8001F068
    cmplwi   r0, 0x19
    blt      .L_8001F070
.L_8001F068:
    li       r0, 0x0
    stw      r0, 0x190(r27)
.L_8001F070:
    lwz      r5, 0x190(r27)
    addi     r3, r31, 0x5e4
    addi     r4, r5, 0x1
    slwi     r0, r5, 2
    stw      r4, 0x190(r27)
    lwzx     r0, r3, r0
.L_8001F088:
    mr       r28, r0
    b        .L_8001F3C4
.L_8001F090:
    lis      r4, 0xffff
    mr       r3, r27
    addi     r4, r4, 0x37
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F0B0
    li       r0, -0x1
    b        .L_8001F10C
.L_8001F0B0:
    mr       r3, r27
    lis      r4, 0xffff
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F0D8
    bl       AXDriverPauseAllStreams
    li       r0, 0x78
    stw      r0, 0x2c0(r27)
    lwz      r0, 0x2b4(r27)
    stw      r0, 0x2c8(r27)
.L_8001F0D8:
    lwz      r0, 0x190(r27)
    cmpwi    r0, 0x0
    blt      .L_8001F0EC
    cmplwi   r0, 0x8
    blt      .L_8001F0F4
.L_8001F0EC:
    li       r0, 0x0
    stw      r0, 0x190(r27)
.L_8001F0F4:
    lwz      r5, 0x190(r27)
    addi     r3, r31, 0x648
    addi     r4, r5, 0x1
    slwi     r0, r5, 2
    stw      r4, 0x190(r27)
    lwzx     r0, r3, r0
.L_8001F10C:
    cmpwi    r0, 0x0
    mr       r28, r0
    bge      .L_8001F3C4
    li       r3, -0x1
    b        .L_8001F458
.L_8001F120:
    lis      r4, 0xffff
    mr       r3, r27
    addi     r4, r4, 0x5
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F14C
    bl       AXDriverPauseAllStreams
    li       r0, 0xb4
    stw      r0, 0x2c0(r27)
    lwz      r0, 0x2b4(r27)
    stw      r0, 0x2c8(r27)
.L_8001F14C:
    lis      r4, 0xffff
    mr       r3, r27
    addi     r4, r4, 0x10
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F178
    bl       AXDriverPauseAllStreams
    li       r0, 0xb4
    stw      r0, 0x2c0(r27)
    lwz      r0, 0x2b4(r27)
    stw      r0, 0x2c8(r27)
.L_8001F178:
    cmpwi    r28, 0x175
    bne      .L_8001F198
    li       r3, 0x6
    bl       HSD_Randi
    slwi     r0, r3, 2
    addi     r3, r31, 0x6ac
    lwzx     r0, r3, r0
    b        .L_8001F1AC
.L_8001F198:
    li       r3, 0x6
    bl       HSD_Randi
    slwi     r0, r3, 2
    addi     r3, r31, 0x694
    lwzx     r0, r3, r0
.L_8001F1AC:
    mr       r28, r0
    b        .L_8001F3C4
.L_8001F1B4:
    lwz      r0, 0x194(r27)
    cmpwi    r0, 0x0
    blt      .L_8001F1C8
    cmplwi   r0, 0xb
    blt      .L_8001F1D0
.L_8001F1C8:
    li       r0, 0x0
    stw      r0, 0x194(r27)
.L_8001F1D0:
    lwz      r5, 0x194(r27)
    addi     r3, r31, 0x668
    addi     r4, r5, 0x1
    slwi     r0, r5, 2
    stw      r4, 0x194(r27)
    lwzx     r28, r3, r0
    b        .L_8001F3C4
.L_8001F1EC:
    cmpwi    r28, 0x86
    beq      .L_8001F218
    bge      .L_8001F208
    cmpwi    r28, 0x84
    beq      .L_8001F238
    bge      .L_8001F230
    b        .L_8001F240
.L_8001F208:
    cmpwi    r28, 0x88
    beq      .L_8001F228
    bge      .L_8001F240
    b        .L_8001F220
.L_8001F218:
    li       r0, 0x0
    b        .L_8001F248
.L_8001F220:
    li       r0, 0x1
    b        .L_8001F248
.L_8001F228:
    li       r0, 0x2
    b        .L_8001F248
.L_8001F230:
    li       r0, 0x3
    b        .L_8001F248
.L_8001F238:
    li       r0, 0x4
    b        .L_8001F248
.L_8001F240:
    mr       r0, r28
    b        .L_8001F260
.L_8001F248:
    slwi     r0, r0, 2
    addi     r3, r31, 0xaf4
    lwzx     r0, r3, r0
    addi     r3, r31, 0xacc
    slwi     r0, r0, 2
    lwzx     r0, r3, r0
.L_8001F260:
    mr       r28, r0
    b        .L_8001F3C4
.L_8001F268:
    lwz      r0, 0x198(r27)
    cmpwi    r0, 0x0
    ble      .L_8001F27C
    li       r3, -0x1
    b        .L_8001F458
.L_8001F27C:
    li       r0, 0x3c
    stw      r0, 0x198(r27)
    b        .L_8001F3C4
.L_8001F288:
    lis      r4, 0xffff
    mr       r3, r27
    addi     r4, r4, 0x5
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F2B4
    bl       AXDriverPauseAllStreams
    li       r0, 0x78
    stw      r0, 0x2c0(r27)
    lwz      r0, 0x2b4(r27)
    stw      r0, 0x2c8(r27)
.L_8001F2B4:
    lis      r4, 0xffff
    mr       r3, r27
    addi     r4, r4, 0x10
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F2E0
    bl       AXDriverPauseAllStreams
    li       r0, 0x78
    stw      r0, 0x2c0(r27)
    lwz      r0, 0x2b4(r27)
    stw      r0, 0x2c8(r27)
.L_8001F2E0:
    mr       r3, r27
    li       r4, 0x18c
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F2FC
    li       r3, 0x18c
    bl       AXDriverKeyOffSfx
.L_8001F2FC:
    mr       r3, r27
    li       r4, 0x18d
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F318
    li       r3, 0x18d
    bl       AXDriverKeyOffSfx
.L_8001F318:
    mr       r3, r27
    li       r4, 0x18e
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F3C4
    li       r3, 0x18e
    bl       AXDriverKeyOffSfx
    b        .L_8001F3C4
.L_8001F338:
    mr       r3, r27
    mr       r4, r28
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F3C4
    li       r3, -0x1
    b        .L_8001F458
.L_8001F354:
    mr       r3, r27
    mr       r4, r28
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F3C4
    li       r3, -0x1
    b        .L_8001F458
.L_8001F370:
    mr       r3, r27
    mr       r4, r28
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F3C4
    li       r3, -0x1
    b        .L_8001F458
.L_8001F38C:
    mr       r3, r27
    mr       r4, r28
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F3C4
    li       r3, -0x1
    b        .L_8001F458
.L_8001F3A8:
    mr       r3, r27
    mr       r4, r28
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F3C4
    li       r3, -0x1
    b        .L_8001F458
.L_8001F3C4:
    clrlwi.  r0, r29, 24
    beq      .L_8001F3EC
    mr       r3, r28
    mr       r4, r29
    mr       r7, r30
    li       r5, 0x7f
    li       r6, 0x0
    bl       HSD_AudioSFXStartParam
    mr       r26, r3
    b        .L_8001F3F0
.L_8001F3EC:
    li       r26, 0x1
.L_8001F3F0:
    mr       r3, r27
    li       r4, 0x0
    b        .L_8001F404
.L_8001F3FC:
    addi     r3, r3, 0x4
    addi     r4, r4, 0x1
.L_8001F404:
    cmplwi   r4, 0x20
    bge      .L_8001F418
    lwz      r0, 0x108(r3)
    cmpwi    r0, 0x0
    bne      .L_8001F3FC
.L_8001F418:
    cmplwi   r4, 0x20
    bge      .L_8001F42C
    slwi     r0, r4, 2
    add      r3, r27, r0
    stw      r28, 0x108(r3)
.L_8001F42C:
    cmpwi    r28, 0x9
    beq      .L_8001F438
    b        .L_8001F454
.L_8001F438:
    li       r3, 0x14
    bl       HSD_Randi
    mulli    r0, r3, 0x6
    mr       r3, r28
    subfic   r0, r0, 0x3c
    extsh    r4, r0
    bl       HSD_AudioSFXSetPitchByID
.L_8001F454:
    mr       r3, r26
.L_8001F458:
    lmw      r26, 0x8(r1)
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x8001F46C
 * @note Size: 0x46C
 */
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
		sound->_108[i] = 0;
	}
	if (sound->mCurrentMusicId != 0 && sound->mPausedMusicId == 0 && !AXDriverIsStreamActive()) {
		sound->mCurrentMusicId = 0;
	}
	const s32 pitchFrames = sound->_188;
	if (pitchFrames > 0 && sound->mMusicPitch != sound->mTargetMusicPitch) {
		sound->mMusicPitch += (sound->mTargetMusicPitch - sound->mMusicPitch) / pitchFrames;
		sound->_188 -= step;
		if (sound->_188 <= 0) {
			sound->mMusicPitch = sound->mTargetMusicPitch;
		}
		AXDriverSetAllStreamPitches(sound->mMusicPitch);
	}
	if (IsGameSoundPlaying(sound, 0xFFFF0005)) {
		if (UseRaisedLiftPitch()) {
			if (sound->mTargetMusicPitch != 200) {
				sound->mTargetMusicPitch = 200;
				sound->_188              = 15;
			}
		} else if (sound->mTargetMusicPitch != 0) {
			sound->mTargetMusicPitch = 0;
			sound->_188              = 15;
		}
	}
	if (sound->mDefaultTrack == 0 && sound->mMusicResumeDelay > 0) {
		if (!IsGameSoundPlaying(sound, 0x18C) && !IsGameSoundPlaying(sound, 0x18D) && !IsGameSoundPlaying(sound, 0x18E)) {
			sound->mMusicResumeDelay -= step;
		}
		if (sound->mMusicResumeDelay <= 0) {
			if (sound->mPausedMusicId == sound->mCurrentMusicId) {
				AXDriverResumeAllStreams();
			} else {
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
	if (sound->_198 > 0) {
		sound->_198 -= step;
	}
	if (sound->mQueueRead != sound->mQueueWrite) {
		if (sound->_188 > 0) {
			sound->_188 -= step;
		}
		if (sound->mLastQueuedSoundId != 0 && IsGameSoundPlaying(sound, sound->mLastQueuedSoundId)) {
			return;
		}
		if (sound->_188 <= 0) {
			sound->mLastQueuedSoundId = sound->mQueuedSoundIds[sound->mQueueRead];
			PlayGameSound((XSound*)&gGame.mSound, sound->mLastQueuedSoundId, 255);
			if (++sound->mQueueRead >= 64) {
				sound->mQueueRead = 0;
			}
		}
	}
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    mr       r31, r4
    stw      r30, 0x18(r1)
    mr       r30, r3
    stw      r29, 0x14(r1)
    lbz      r0, 0xf8(r3)
    cmplwi   r0, 0x0
    beq      .L_8001F8BC
    lwz      r0, 0x3e0(r30)
    cmpwi    r0, 0x0
    ble      .L_8001F538
    bl       HSD_SynthSFXGetPendingLoadCount
    neg      r0, r3
    or       r0, r0, r3
    srwi.    r0, r0, 31
    bne      .L_8001F538
    li       r0, 0x10
    mr       r4, r30
    li       r3, 0x0
    mtctr    r0
.L_8001F4C8:
    lwz      r0, 0x2e0(r4)
    cmplwi   r0, 0x0
    beq      .L_8001F52C
    lbz      r0, 0x2ec(r4)
    cmplwi   r0, 0x0
    bne      .L_8001F52C
    slwi     r0, r3, 4
    lis      r3, lbl_801FEDD0@ha
    add      r29, r30, r0
    lwz      r4, 0x2e0(r29)
    addi     r3, r3, lbl_801FEDD0@l
    crclr    cr1eq
    bl       OSReport
    lwz      r3, 0x2e0(r29)
    li       r5, 0x0
    lwz      r4, 0x2e4(r29)
    li       r6, 0x0
    bl       HSD_SynthSFXLoad
    stw      r3, 0x2e8(r29)
    li       r0, 0x1
    stb      r0, 0x2ec(r29)
    lwz      r3, 0x3e0(r30)
    subi     r0, r3, 0x1
    stw      r0, 0x3e0(r30)
    b        .L_8001F538
.L_8001F52C:
    addi     r4, r4, 0x10
    addi     r3, r3, 0x1
    bdnz     .L_8001F4C8
.L_8001F538:
    li       r0, 0x0
    stw      r0, 0x108(r30)
    stw      r0, 0x10c(r30)
    stw      r0, 0x110(r30)
    stw      r0, 0x114(r30)
    stw      r0, 0x118(r30)
    stw      r0, 0x11c(r30)
    stw      r0, 0x120(r30)
    stw      r0, 0x124(r30)
    stw      r0, 0x128(r30)
    stw      r0, 0x12c(r30)
    stw      r0, 0x130(r30)
    stw      r0, 0x134(r30)
    stw      r0, 0x138(r30)
    stw      r0, 0x13c(r30)
    stw      r0, 0x140(r30)
    stw      r0, 0x144(r30)
    stw      r0, 0x148(r30)
    stw      r0, 0x14c(r30)
    stw      r0, 0x150(r30)
    stw      r0, 0x154(r30)
    stw      r0, 0x158(r30)
    stw      r0, 0x15c(r30)
    stw      r0, 0x160(r30)
    stw      r0, 0x164(r30)
    stw      r0, 0x168(r30)
    stw      r0, 0x16c(r30)
    stw      r0, 0x170(r30)
    stw      r0, 0x174(r30)
    stw      r0, 0x178(r30)
    stw      r0, 0x17c(r30)
    stw      r0, 0x180(r30)
    stw      r0, 0x184(r30)
    lwz      r0, 0x2b4(r30)
    cmpwi    r0, 0x0
    beq      .L_8001F5E8
    lwz      r0, 0x2c8(r30)
    cmpwi    r0, 0x0
    bne      .L_8001F5E8
    bl       AXDriverIsStreamActive
    cmpwi    r3, 0x0
    bne      .L_8001F5E8
    li       r0, 0x0
    stw      r0, 0x2b4(r30)
.L_8001F5E8:
    lwz      r4, 0x188(r30)
    cmpwi    r4, 0x0
    ble      .L_8001F63C
    lha      r3, 0x2b8(r30)
    lha      r0, 0x2ba(r30)
    cmpw     r3, r0
    beq      .L_8001F63C
    subf     r0, r3, r0
    divw     r0, r0, r4
    add      r0, r3, r0
    sth      r0, 0x2b8(r30)
    lwz      r0, 0x188(r30)
    subf     r0, r31, r0
    stw      r0, 0x188(r30)
    lwz      r0, 0x188(r30)
    cmpwi    r0, 0x0
    bgt      .L_8001F634
    lha      r0, 0x2ba(r30)
    sth      r0, 0x2b8(r30)
.L_8001F634:
    lha      r3, 0x2b8(r30)
    bl       AXDriverSetAllStreamPitches
.L_8001F63C:
    lis      r4, 0xffff
    mr       r3, r30
    addi     r4, r4, 0x5
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_8001F6CC
    lis      r3, gGame@ha
    li       r29, 0x0
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    lwz      r0, 0xa48(r3)
    cmpwi    r0, 0x4
    bne      .L_8001F688
    addi     r3, r3, 0xa44
    bl       IsLiftTargetPlug
    cmpwi    r3, 0x0
    beq      .L_8001F688
    li       r29, 0x1
.L_8001F688:
    clrlwi.  r0, r29, 24
    beq      .L_8001F6B0
    lha      r0, 0x2ba(r30)
    cmpwi    r0, 0xc8
    beq      .L_8001F6CC
    li       r3, 0xc8
    li       r0, 0xf
    sth      r3, 0x2ba(r30)
    stw      r0, 0x188(r30)
    b        .L_8001F6CC
.L_8001F6B0:
    lha      r0, 0x2ba(r30)
    cmpwi    r0, 0x0
    beq      .L_8001F6CC
    li       r3, 0x0
    li       r0, 0xf
    sth      r3, 0x2ba(r30)
    stw      r0, 0x188(r30)
.L_8001F6CC:
    lwz      r0, 0xfc(r30)
    cmpwi    r0, 0x0
    bne      .L_8001F768
    lwz      r0, 0x2c0(r30)
    cmpwi    r0, 0x0
    ble      .L_8001F768
    mr       r3, r30
    li       r4, 0x18c
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_8001F72C
    mr       r3, r30
    li       r4, 0x18d
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_8001F72C
    mr       r3, r30
    li       r4, 0x18e
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_8001F72C
    lwz      r0, 0x2c0(r30)
    subf     r0, r31, r0
    stw      r0, 0x2c0(r30)
.L_8001F72C:
    lwz      r0, 0x2c0(r30)
    cmpwi    r0, 0x0
    bgt      .L_8001F768
    lwz      r3, 0x2c8(r30)
    lwz      r0, 0x2b4(r30)
    cmpw     r3, r0
    bne      .L_8001F750
    bl       AXDriverResumeAllStreams
    b        .L_8001F760
.L_8001F750:
    mr       r3, r30
    lis      r4, 0xffff
    li       r5, 0xff
    bl       PlayGameSound
.L_8001F760:
    li       r0, 0x0
    stw      r0, 0x2c8(r30)
.L_8001F768:
    lwz      r3, 0x2b4(r30)
    addis    r0, r3, 0x1
    cmplwi   r0, 0x10
    bne      .L_8001F7DC
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    lwz      r0, 0x5d28(r3)
    cmpwi    r0, 0x1
    bne      .L_8001F7A8
    lfs      f1, 0x5d2c(r3)
    lfs      f0, lbl_8065C090@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_8001F7A8
    li       r0, 0x0
    b        .L_8001F7AC
.L_8001F7A8:
    li       r0, 0x1
.L_8001F7AC:
    clrlwi.  r0, r0, 24
    bne      .L_8001F7DC
    lwz      r0, 0x2c4(r30)
    cmpwi    r0, 0x0
    bgt      .L_8001F7DC
    li       r3, 0xb4
    li       r0, 0x0
    stw      r3, 0x2c4(r30)
    li       r3, 0x0
    li       r4, 0x258
    stb      r0, 0x2cc(r30)
    bl       AXDriverSetAllStreamVolumes
.L_8001F7DC:
    lwz      r0, 0x2c4(r30)
    cmpwi    r0, 0x0
    ble      .L_8001F80C
    subf     r0, r31, r0
    stw      r0, 0x2c4(r30)
    lwz      r0, 0x2c4(r30)
    cmpwi    r0, 0x0
    bgt      .L_8001F80C
    li       r0, 0x0
    stw      r0, 0x2b4(r30)
    stw      r0, 0x2c8(r30)
    bl       AXDriverStopAllStreams
.L_8001F80C:
    lwz      r0, 0x198(r30)
    cmpwi    r0, 0x0
    ble      .L_8001F820
    subf     r0, r31, r0
    stw      r0, 0x198(r30)
.L_8001F820:
    lbz      r3, 0x1a0(r30)
    lbz      r0, 0x1a1(r30)
    cmplw    r3, r0
    beq      .L_8001F8BC
    lwz      r0, 0x188(r30)
    cmpwi    r0, 0x0
    ble      .L_8001F844
    subf     r0, r31, r0
    stw      r0, 0x188(r30)
.L_8001F844:
    lwz      r4, 0x2ac(r30)
    cmpwi    r4, 0x0
    beq      .L_8001F860
    mr       r3, r30
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_8001F8BC
.L_8001F860:
    lwz      r0, 0x188(r30)
    cmpwi    r0, 0x0
    bgt      .L_8001F8BC
    lbz      r0, 0x1a0(r30)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    li       r5, 0xff
    slwi     r0, r0, 2
    add      r4, r30, r0
    addis    r3, r3, 0xe
    lwz      r0, 0x1ac(r4)
    addi     r3, r3, 0x7e24
    stw      r0, 0x2ac(r30)
    lwz      r4, 0x2ac(r30)
    bl       PlayGameSound
    lbz      r3, 0x1a0(r30)
    addi     r3, r3, 0x1
    clrlwi   r0, r3, 24
    stb      r3, 0x1a0(r30)
    cmplwi   r0, 0x40
    blt      .L_8001F8BC
    li       r0, 0x0
    stb      r0, 0x1a0(r30)
.L_8001F8BC:
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

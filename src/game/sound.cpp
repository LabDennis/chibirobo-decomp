#include "game/cgame.h"
#include <types.h>
#include <string.h>
#include <dolphin/os.h>
#include "Version.h"
#include "game/csound.h"
#include "game/soundtracks.h"
#include "game/xchr.h"

#include "game/soundplayback.h"

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

// Sound-effect tables share a data pool.
extern "C" u8 lbl_802107A0[];
extern "C" s32 HSD_Randi(s32 max);

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

static inline s32 SelectMappedSound(s32 sfx, u8* data)
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
	return ((s32*)(data + 0xACC))[((s32*)(data + 0xAF4))[index]];
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
 * @note Address: 0x8001E904
 * @note Size: 0xB68
 */
extern "C" s32 PlayGameSound(XSound* sound, s32 sfx, u8 vol)
{
	u8* data = lbl_802107A0;
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
		MusicEntry* music = (MusicEntry*)(data + 0x108) + index;
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
			sfx = ((s32(*)[5])(data + 0x4C0))[surface][HSD_Randi(5)];
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
		sfx = SelectAlternatingSound(sound, (s32*)(data + 0x5D4), (s32*)(data + 0x5C4));
		if (sfx < 0) {
			return -1;
		}
		break;
	case 0x15:
		sfx = SelectAlternatingSound(sound, (s32*)(data + 0x5C4), (s32*)(data + 0x5D4));
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
			sfx = ((s32*)(data + 0x5E4))[sound->_190++];
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
			sfx = ((s32*)(data + 0x648))[sound->_190++];
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
			sfx = ((s32*)(data + 0x6AC))[HSD_Randi(6)];
		} else {
			sfx = ((s32*)(data + 0x694))[HSD_Randi(6)];
		}
		break;
	case 0x137:
		if (sound->_194 < 0 || (u32)sound->_194 >= 11) {
			sound->_194 = 0;
		}
		sfx = ((s32*)(data + 0x668))[sound->_194++];
		break;
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	case 0x88:
		sfx = SelectMappedSound(sfx, data);
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
				OSReport(sSoundBankLoadFormat, sound->mStageRequests.entries[i].mName);
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

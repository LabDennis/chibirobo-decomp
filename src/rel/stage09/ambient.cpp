#include "Version.h"
#include "game/hsd_jobj.h"

// CGame
extern "C" u8 gGame[];
#define GAME_AT(off) (gGame + (off))
#if defined(VERSION_JP)
#define SOUND() GAME_AT(0xE713C)
#else
#define SOUND() GAME_AT(0xE7E24)
#endif

extern "C" {
s32 HSD_Randi(s32 max);
void RegisterGameSoundName(void* p, const char* name);
s32 IsGameSoundPlaying(void* p, s32 id);
void PlayGameSound(void* p, s32 id, s32 v);
void HSD_AudioSFXSetPanByID(s32 id, s32 v);
s32 GetSoundVolumeAtPosition(void* p, void* q, s32 v);
void SetGameSfxVolume(void* p, s32 id, s32 v, s32 w);
void StopGameSfx(void* p, s32 id);

// rel/ana.cpp
void DrawAnas(void);
void UpdateAnas(s32 dt);
void InitAnas(void);
s32 CmdAttachAnaToRobo(s32 cmd, s32 count, s32* args);
s32 CmdSetAnaRotY(s32 cmd, s32 count, s32* args);
s32 CmdSetAnaJObj(s32 cmd, s32 count, s32* args);
s32 CmdPlantTane(s32 cmd, s32 count, s32* args);
s32 CmdFindNearestAna(s32 cmd, s32 count, s32* args);
s32 CmdSetTutiAna(s32 cmd, s32 count, s32* args);
s32 CmdAddAna(s32 cmd, s32 count, s32* args);
s32 CmdDropHeldObj(s32 cmd, s32 count, s32* args);
}

class AmbientSeq;
extern "C" void AmbientSeq_Update(AmbientSeq* seq, s32 dt);

// one running sound script, commands are pairs or triples of ints
class AmbientSeq {
public:
	AmbientSeq() { data = NULL; }
	void Clear()
	{
		data = NULL;
		pos  = 0;
		wait = 0;
	}

	s32* data; // _00
	s32 pos;   // _04
	s32 wait;  // _08
};

struct AmbientPlayer {
	u8 mPaused;          // _00, nonzero stops the ambient sequences and bird calls
	AmbientSeq seqs[10]; // _04
	s32 mBirdTimer;      // _7C, frames until the next random bird call, reset to 600-1199
	s32* mBirdSounds;    // _80, table of bird sfx ids one is picked from at random, NULL for none
	s32 mBirdSoundCount; // _84, number of ids in mBirdSounds

	void UpdateSeqs(s32 dt)
	{
		u32 i;
		for (i = 0; i < 10; i++) {
			AmbientSeq* seq = &seqs[i];
			if (seq->data != NULL) {
				AmbientSeq_Update(seq, dt);
			}
		}
	}

	void Clear()
	{
		u32 i;
		for (i = 0; i < 10; i++) {
			seqs[i].Clear();
		}
		mPaused         = 0;
		mBirdSounds     = NULL;
		mBirdSoundCount = 0;
	}

	void Start(s32* data)
	{
		u32 i;
		for (i = 0; i < 10; i++) {
			AmbientSeq* seq = &seqs[i];
			if (seq->data == NULL) {
				seq->data = data;
				seq->pos  = 0;
				seq->wait = 0;
				break;
			}
		}
	}
};

extern "C" {
static s32 ambientTrafficCarLr[]            = { 5, 1000, 2000, 1, 0x60003, 2, 0x60003, 5, 3000, 6000, 0 };
static s32 ambientTrafficCarRl[]            = { 5, 2000, 3000, 1, 0x60004, 2, 0x60004, 5, 3000, 6000, 0 };
static s32 ambientTrafficDump[]             = { 5, 3000, 4000, 1, 0x60002, 2, 0x60002, 5, 3000, 6000, 0 };
static s32 ambientConstructionNoko[]        = { 3, 3000, 1, 0x130002, 4, 0x130002, 0 };
static s32 ambientConstructionHammer1[]     = { 1, 0x130000, 2, 0x130000, 3, 300, 0 };
static s32 ambientConstructionHammer2[]     = { 3, 500, 1, 0x130001, 2, 0x130001, 0 };
static s32 ambientConstructionHammer1Fast[] = { 1, 0x130000, 2, 0x130000, 3, 100, 0 };
static s32 ambientConstructionHammer2Fast[] = { 3, 200, 1, 0x130001, 4, 0x130001, 0 };
static s32 ambientNeighborMower[]           = {
    3,  500, 1,   0x140001, 3,        1000,     4,   0x140001, 1,   0x140002, 2,        0x140002, 3,   500, 1,        0x140000,
    5,  70,  100, 1,        0x140000, 5,        70,  100,      1,   0x140000, 5,        70,       100, 1,   0x140000, 5,
    70, 100, 1,   0x140000, 5,        70,       100, 3,        200, 1,        0x140000, 5,        70,  100, 1,        0x140000,
    5,  70,  100, 1,        0x140000, 5,        70,  100,      1,   0x140000, 5,        70,       100, 1,   0x140000, 5,
    70, 100, 3,   200,      1,        0x140000, 5,   70,       100, 1,        0x140000, 5,        70,  100, 1,        0x140000,
    5,  70,  100, 1,        0x140000, 5,        70,  100,      1,   0x140000, 5,        70,       100, 0,
};
static s32 ambientNightHowling[] = { 5, 1000, 1500, 1, 0x150000, 2, 0x150000, 5, 500, 1000, 0 };
static s32 ambientNightCricket[] = { 5, 100, 500, 1, 0x150001, 5, 500, 1500, 4, 0x150001, 5, 500, 1000, 0 };
static s32 ambientNatureBirds1[] = { 0x130003, 0x130004, 0x130005, 0x130006, 0x130007 };
static s32 ambientNatureBirds2[] = { 0x140003, 0x140004, 0x140005, 0x140006, 0x140007 };

s32 CmdSetAmbientSound(s32 cmd, s32 count, s32* args);
void DrawStage(s32 dt);
void UpdateStage(s32 dt);
void InitStage(s32 dt);
void AmbientSeq_Update(AmbientSeq* seq, s32 dt);
}

typedef void (*RelSlotFunc)(s32 arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage, DrawStage };
RelCmdFunc _prolog_cmds[]    = {
    CmdSetAmbientSound, CmdAddAna,     CmdSetTutiAna,  CmdFindNearestAna,  CmdPlantTane,
    CmdSetAnaJObj,      CmdSetAnaRotY, CmdDropHeldObj, CmdAttachAnaToRobo, NULL,
};
}

static AmbientPlayer ambient;

/**
 * @note Address: stage09 .text 0x0
 * @note Size: 0x730
 */
extern "C" s32 CmdSetAmbientSound(s32 cmd, s32 count, s32* args)
{
	switch (args[0]) {
	case 0:
		if (count >= 2) {
			if (args[1] != 0) {
				void* sound;
				ambient.mPaused = 1;
				sound           = SOUND();
				if (IsGameSoundPlaying(sound, 0x140001) != 0) {
					StopGameSfx(sound, 0x140001);
				}
			} else {
				ambient.mPaused = 0;
			}
		} else {
			ambient.mPaused = 1;
		}
		break;
	case 1:
		RegisterGameSoundName(SOUND(), "sfx_uraniwa_ambient1.ssm");
		ambient.Start(ambientConstructionHammer1);
		ambient.Start(ambientConstructionHammer2);
		ambient.Start(ambientConstructionNoko);
		ambient.Start(ambientTrafficCarLr);
		ambient.Start(ambientTrafficCarRl);
		ambient.Start(ambientTrafficDump);
		ambient.mBirdTimer      = HSD_Randi(600) + 600;
		ambient.mBirdSounds     = ambientNatureBirds1;
		ambient.mBirdSoundCount = 5;
		break;
	case 2:
		RegisterGameSoundName(SOUND(), "sfx_uraniwa_ambient1.ssm");
		ambient.Start(ambientConstructionHammer1Fast);
		ambient.Start(ambientConstructionHammer2Fast);
		ambient.Start(ambientConstructionNoko);
		ambient.Start(ambientTrafficCarLr);
		ambient.Start(ambientTrafficCarRl);
		ambient.Start(ambientTrafficDump);
		ambient.mBirdTimer      = HSD_Randi(600) + 600;
		ambient.mBirdSounds     = ambientNatureBirds1;
		ambient.mBirdSoundCount = 5;
		break;
	case 3:
		RegisterGameSoundName(SOUND(), "sfx_uraniwa_ambient2.ssm");
		ambient.Start(ambientNeighborMower);
		ambient.Start(ambientTrafficCarLr);
		ambient.Start(ambientTrafficCarRl);
		ambient.Start(ambientTrafficDump);
		ambient.mBirdTimer      = HSD_Randi(600) + 600;
		ambient.mBirdSounds     = ambientNatureBirds2;
		ambient.mBirdSoundCount = 5;
		break;
	case 4:
		RegisterGameSoundName(SOUND(), "sfx_uraniwa_ambient3.ssm");
		ambient.Start(ambientNightHowling);
		ambient.Start(ambientNightCricket);
		ambient.Start(ambientTrafficCarLr);
		ambient.Start(ambientTrafficCarRl);
		ambient.Start(ambientTrafficDump);
		break;
	default:
		return 0;
	}
	return 0;
}

// CmdSetAmbientSound
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    lwz      r0, 0x0(r5)
    cmpwi    r0, 0x2
    beq      .L_0000028C
    bge      .L_00000030
    cmpwi    r0, 0x0
    beq      .L_00000040
    bge      .L_000000BC
    b        .L_00000710
.L_00000030:
    cmpwi    r0, 0x4
    beq      .L_000005AC
    bge      .L_00000710
    b        .L_0000045C
.L_00000040:
    cmpwi    r4, 0x2
    blt      .L_000000AC
    lwz      r0, 0x4(r5)
    cmpwi    r0, 0x0
    beq      .L_0000009C
    lis      r3, gGame@ha
    lis      r5, ambient@ha
    addi     r4, r3, gGame@l
    li       r0, 0x1
    addis    r31, r4, 0xe
    lis      r3, 0x14
    stb      r0, ambient@l(r5)
    addi     r31, r31, 0x7e24
    addi     r4, r3, 0x1
    mr       r3, r31
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_00000718
    lis      r4, 0x14
    mr       r3, r31
    addi     r4, r4, 0x1
    bl       StopGameSfx
    b        .L_00000718
.L_0000009C:
    lis      r3, ambient@ha
    li       r0, 0x0
    stb      r0, ambient@l(r3)
    b        .L_00000718
.L_000000AC:
    lis      r3, ambient@ha
    li       r0, 0x1
    stb      r0, ambient@l(r3)
    b        .L_00000718
.L_000000BC:
    lis      r3, gGame@ha
    lis      r4, sStage09PathSfxUraniwaAmbient1Ssm@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r4, r4, sStage09PathSfxUraniwaAmbient1Ssm@l
    addi     r3, r3, 0x7e24
    bl       RegisterGameSoundName
    lis      r3, ambient@ha
    li       r0, 0xa
    addi     r3, r3, ambient@l
    mr       r4, r3
    mtctr    r0
.L_000000EC:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_00000118
    lis      r4, ambientConstructionHammer1@ha
    li       r0, 0x0
    addi     r4, r4, ambientConstructionHammer1@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_00000120
.L_00000118:
    addi     r4, r4, 0xc
    bdnz     .L_000000EC
.L_00000120:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_0000012C:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_00000158
    lis      r4, ambientConstructionHammer2@ha
    li       r0, 0x0
    addi     r4, r4, ambientConstructionHammer2@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_00000160
.L_00000158:
    addi     r4, r4, 0xc
    bdnz     .L_0000012C
.L_00000160:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_0000016C:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_00000198
    lis      r4, ambientConstructionNoko@ha
    li       r0, 0x0
    addi     r4, r4, ambientConstructionNoko@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_000001A0
.L_00000198:
    addi     r4, r4, 0xc
    bdnz     .L_0000016C
.L_000001A0:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_000001AC:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_000001D8
    lis      r4, ambientTrafficCarLr@ha
    li       r0, 0x0
    addi     r4, r4, ambientTrafficCarLr@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_000001E0
.L_000001D8:
    addi     r4, r4, 0xc
    bdnz     .L_000001AC
.L_000001E0:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_000001EC:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_00000218
    lis      r4, ambientTrafficCarRl@ha
    li       r0, 0x0
    addi     r4, r4, ambientTrafficCarRl@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_00000220
.L_00000218:
    addi     r4, r4, 0xc
    bdnz     .L_000001EC
.L_00000220:
    li       r0, 0xa
    mtctr    r0
.L_00000228:
    addi     r4, r3, 0x4
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_00000254
    lis      r3, ambientTrafficDump@ha
    li       r0, 0x0
    addi     r3, r3, ambientTrafficDump@l
    stw      r3, 0x0(r4)
    stw      r0, 0x4(r4)
    stw      r0, 0x8(r4)
    b        .L_0000025C
.L_00000254:
    addi     r3, r3, 0xc
    bdnz     .L_00000228
.L_0000025C:
    li       r3, 0x258
    bl       HSD_Randi
    lis      r5, ambient@ha
    lis      r4, ambientNatureBirds1@ha
    addi     r6, r3, 0x258
    li       r0, 0x5
    addi     r5, r5, ambient@l
    addi     r3, r4, ambientNatureBirds1@l
    stw      r6, 0x7c(r5)
    stw      r3, 0x80(r5)
    stw      r0, 0x84(r5)
    b        .L_00000718
.L_0000028C:
    lis      r3, gGame@ha
    lis      r4, sStage09PathSfxUraniwaAmbient1Ssm@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r4, r4, sStage09PathSfxUraniwaAmbient1Ssm@l
    addi     r3, r3, 0x7e24
    bl       RegisterGameSoundName
    lis      r3, ambient@ha
    li       r0, 0xa
    addi     r3, r3, ambient@l
    mr       r4, r3
    mtctr    r0
.L_000002BC:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_000002E8
    lis      r4, ambientConstructionHammer1Fast@ha
    li       r0, 0x0
    addi     r4, r4, ambientConstructionHammer1Fast@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_000002F0
.L_000002E8:
    addi     r4, r4, 0xc
    bdnz     .L_000002BC
.L_000002F0:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_000002FC:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_00000328
    lis      r4, ambientConstructionHammer2Fast@ha
    li       r0, 0x0
    addi     r4, r4, ambientConstructionHammer2Fast@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_00000330
.L_00000328:
    addi     r4, r4, 0xc
    bdnz     .L_000002FC
.L_00000330:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_0000033C:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_00000368
    lis      r4, ambientConstructionNoko@ha
    li       r0, 0x0
    addi     r4, r4, ambientConstructionNoko@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_00000370
.L_00000368:
    addi     r4, r4, 0xc
    bdnz     .L_0000033C
.L_00000370:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_0000037C:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_000003A8
    lis      r4, ambientTrafficCarLr@ha
    li       r0, 0x0
    addi     r4, r4, ambientTrafficCarLr@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_000003B0
.L_000003A8:
    addi     r4, r4, 0xc
    bdnz     .L_0000037C
.L_000003B0:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_000003BC:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_000003E8
    lis      r4, ambientTrafficCarRl@ha
    li       r0, 0x0
    addi     r4, r4, ambientTrafficCarRl@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_000003F0
.L_000003E8:
    addi     r4, r4, 0xc
    bdnz     .L_000003BC
.L_000003F0:
    li       r0, 0xa
    mtctr    r0
.L_000003F8:
    addi     r4, r3, 0x4
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_00000424
    lis      r3, ambientTrafficDump@ha
    li       r0, 0x0
    addi     r3, r3, ambientTrafficDump@l
    stw      r3, 0x0(r4)
    stw      r0, 0x4(r4)
    stw      r0, 0x8(r4)
    b        .L_0000042C
.L_00000424:
    addi     r3, r3, 0xc
    bdnz     .L_000003F8
.L_0000042C:
    li       r3, 0x258
    bl       HSD_Randi
    lis      r5, ambient@ha
    lis      r4, ambientNatureBirds1@ha
    addi     r6, r3, 0x258
    li       r0, 0x5
    addi     r5, r5, ambient@l
    addi     r3, r4, ambientNatureBirds1@l
    stw      r6, 0x7c(r5)
    stw      r3, 0x80(r5)
    stw      r0, 0x84(r5)
    b        .L_00000718
.L_0000045C:
    lis      r3, gGame@ha
    lis      r4, sStage09PathSfxUraniwaAmbient2Ssm@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r4, r4, sStage09PathSfxUraniwaAmbient2Ssm@l
    addi     r3, r3, 0x7e24
    bl       RegisterGameSoundName
    lis      r3, ambient@ha
    li       r0, 0xa
    addi     r5, r3, ambient@l
    mr       r3, r5
    mtctr    r0
.L_0000048C:
    addi     r4, r3, 0x4
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_000004B8
    lis      r3, ambientNeighborMower@ha
    li       r0, 0x0
    addi     r3, r3, ambientNeighborMower@l
    stw      r3, 0x0(r4)
    stw      r0, 0x4(r4)
    stw      r0, 0x8(r4)
    b        .L_000004C0
.L_000004B8:
    addi     r3, r3, 0xc
    bdnz     .L_0000048C
.L_000004C0:
    li       r0, 0xa
    mr       r3, r5
    mtctr    r0
.L_000004CC:
    addi     r4, r3, 0x4
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_000004F8
    lis      r3, ambientTrafficCarLr@ha
    li       r0, 0x0
    addi     r3, r3, ambientTrafficCarLr@l
    stw      r3, 0x0(r4)
    stw      r0, 0x4(r4)
    stw      r0, 0x8(r4)
    b        .L_00000500
.L_000004F8:
    addi     r3, r3, 0xc
    bdnz     .L_000004CC
.L_00000500:
    li       r0, 0xa
    mr       r3, r5
    mtctr    r0
.L_0000050C:
    addi     r4, r3, 0x4
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_00000538
    lis      r3, ambientTrafficCarRl@ha
    li       r0, 0x0
    addi     r3, r3, ambientTrafficCarRl@l
    stw      r3, 0x0(r4)
    stw      r0, 0x4(r4)
    stw      r0, 0x8(r4)
    b        .L_00000540
.L_00000538:
    addi     r3, r3, 0xc
    bdnz     .L_0000050C
.L_00000540:
    li       r0, 0xa
    mtctr    r0
.L_00000548:
    addi     r4, r5, 0x4
    lwz      r0, 0x4(r5)
    cmplwi   r0, 0x0
    bne      .L_00000574
    lis      r3, ambientTrafficDump@ha
    li       r0, 0x0
    addi     r3, r3, ambientTrafficDump@l
    stw      r3, 0x0(r4)
    stw      r0, 0x4(r4)
    stw      r0, 0x8(r4)
    b        .L_0000057C
.L_00000574:
    addi     r5, r5, 0xc
    bdnz     .L_00000548
.L_0000057C:
    li       r3, 0x258
    bl       HSD_Randi
    lis      r5, ambient@ha
    lis      r4, ambientNatureBirds2@ha
    addi     r6, r3, 0x258
    li       r0, 0x5
    addi     r5, r5, ambient@l
    addi     r3, r4, ambientNatureBirds2@l
    stw      r6, 0x7c(r5)
    stw      r3, 0x80(r5)
    stw      r0, 0x84(r5)
    b        .L_00000718
.L_000005AC:
    lis      r3, gGame@ha
    lis      r4, sStage09PathSfxUraniwaAmbient3Ssm@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r4, r4, sStage09PathSfxUraniwaAmbient3Ssm@l
    addi     r3, r3, 0x7e24
    bl       RegisterGameSoundName
    lis      r3, ambient@ha
    li       r0, 0xa
    addi     r3, r3, ambient@l
    mr       r4, r3
    mtctr    r0
.L_000005DC:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_00000608
    lis      r4, ambientNightHowling@ha
    li       r0, 0x0
    addi     r4, r4, ambientNightHowling@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_00000610
.L_00000608:
    addi     r4, r4, 0xc
    bdnz     .L_000005DC
.L_00000610:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_0000061C:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_00000648
    lis      r4, ambientNightCricket@ha
    li       r0, 0x0
    addi     r4, r4, ambientNightCricket@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_00000650
.L_00000648:
    addi     r4, r4, 0xc
    bdnz     .L_0000061C
.L_00000650:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_0000065C:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_00000688
    lis      r4, ambientTrafficCarLr@ha
    li       r0, 0x0
    addi     r4, r4, ambientTrafficCarLr@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_00000690
.L_00000688:
    addi     r4, r4, 0xc
    bdnz     .L_0000065C
.L_00000690:
    li       r0, 0xa
    mr       r4, r3
    mtctr    r0
.L_0000069C:
    addi     r5, r4, 0x4
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_000006C8
    lis      r4, ambientTrafficCarRl@ha
    li       r0, 0x0
    addi     r4, r4, ambientTrafficCarRl@l
    stw      r4, 0x0(r5)
    stw      r0, 0x4(r5)
    stw      r0, 0x8(r5)
    b        .L_000006D0
.L_000006C8:
    addi     r4, r4, 0xc
    bdnz     .L_0000069C
.L_000006D0:
    li       r0, 0xa
    mtctr    r0
.L_000006D8:
    addi     r4, r3, 0x4
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_00000704
    lis      r3, ambientTrafficDump@ha
    li       r0, 0x0
    addi     r3, r3, ambientTrafficDump@l
    stw      r3, 0x0(r4)
    stw      r0, 0x4(r4)
    stw      r0, 0x8(r4)
    b        .L_00000718
.L_00000704:
    addi     r3, r3, 0xc
    bdnz     .L_000006D8
    b        .L_00000718
.L_00000710:
    li       r3, 0x0
    b        .L_0000071C
.L_00000718:
    li       r3, 0x0
.L_0000071C:
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage09 .text 0x730
 * @note Size: 0x20
 */
extern "C" void DrawStage(s32 dt)
{
	DrawAnas();
}

/**
 * @note Address: stage09 .text 0x750
 * @note Size: 0x1AC
 */
extern "C" void UpdateStage(s32 dt)
{
	void* sound = SOUND();
	Vec pos;

	if (IsGameSoundPlaying(sound, 0x12001A) != 0) {
		HSD_AudioSFXSetPanByID(0x12001A, 0);
	}
	if (IsGameSoundPlaying(sound, 0x12001B) != 0) {
		HSD_AudioSFXSetPanByID(0x12001B, 0xFE);
	}
	pos.x = 458.0f;
	pos.y = 0.0f;
	pos.z = -555.5f;
	if (IsGameSoundPlaying(sound, 0x60005) != 0) {
		SetGameSfxVolume(sound, 0x60005, GetSoundVolumeAtPosition(sound, &pos, 4), 0);
	}
	if (!ambient.mPaused) {
		ambient.UpdateSeqs(dt);
		if (ambient.mBirdSounds != NULL) {
			ambient.mBirdTimer -= dt;
			if (ambient.mBirdTimer <= 0) {
				PlayGameSound(sound, ambient.mBirdSounds[HSD_Randi(ambient.mBirdSoundCount)], 0xFF);
				ambient.mBirdTimer = HSD_Randi(600) + 600;
			}
		}
	}
	UpdateAnas(dt);
}

// UpdateStage
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    lis      r5, gGame@ha
    lis      r4, 0x12
    stw      r0, 0x34(r1)
    addi     r5, r5, gGame@l
    addi     r4, r4, 0x1a
    stw      r31, 0x2c(r1)
    addis    r31, r5, 0xe
    addi     r31, r31, 0x7e24
    stw      r30, 0x28(r1)
    mr       r30, r3
    mr       r3, r31
    stw      r29, 0x24(r1)
    stw      r28, 0x20(r1)
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_000007A8
    lis      r3, 0x12
    li       r4, 0x0
    addi     r3, r3, 0x1a
    bl       HSD_AudioSFXSetPanByID
.L_000007A8:
    lis      r4, 0x12
    mr       r3, r31
    addi     r4, r4, 0x1b
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_000007D0
    lis      r3, 0x12
    li       r4, 0xfe
    addi     r3, r3, 0x1b
    bl       HSD_AudioSFXSetPanByID
.L_000007D0:
    lis      r6, lbl_11_rodata_54@ha
    lis      r5, lbl_11_rodata_58@ha
    lis      r3, lbl_11_rodata_5C@ha
    lis      r4, 0x6
    addi     r7, r6, lbl_11_rodata_54@l
    addi     r6, r5, lbl_11_rodata_58@l
    addi     r5, r3, lbl_11_rodata_5C@l
    lfs      f2, 0x0(r7)
    lfs      f1, 0x0(r6)
    mr       r3, r31
    lfs      f0, 0x0(r5)
    addi     r4, r4, 0x5
    stfs     f2, 0x8(r1)
    stfs     f1, 0xc(r1)
    stfs     f0, 0x10(r1)
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_00000840
    mr       r3, r31
    addi     r4, r1, 0x8
    li       r5, 0x4
    bl       GetSoundVolumeAtPosition
    lis      r4, 0x6
    mr       r5, r3
    mr       r3, r31
    li       r6, 0x0
    addi     r4, r4, 0x5
    bl       SetGameSfxVolume
.L_00000840:
    lis      r3, ambient@ha
    addi     r28, r3, ambient@l
    lbz      r0, 0x0(r28)
    cmplwi   r0, 0x0
    bne      .L_000008D4
    li       r29, 0x0
.L_00000858:
    addi     r3, r28, 0x4
    lwz      r0, 0x4(r28)
    cmplwi   r0, 0x0
    beq      .L_00000870
    mr       r4, r30
    bl       AmbientSeq_Update
.L_00000870:
    addi     r29, r29, 0x1
    addi     r28, r28, 0xc
    cmplwi   r29, 0xa
    blt      .L_00000858
    lis      r3, ambient@ha
    addi     r29, r3, ambient@l
    lwz      r0, 0x80(r29)
    cmplwi   r0, 0x0
    beq      .L_000008D4
    lwz      r0, 0x7c(r29)
    subf.    r0, r30, r0
    stw      r0, 0x7c(r29)
    bgt      .L_000008D4
    lwz      r3, 0x84(r29)
    bl       HSD_Randi
    lwz      r4, 0x80(r29)
    slwi     r0, r3, 2
    mr       r3, r31
    li       r5, 0xff
    lwzx     r4, r4, r0
    bl       PlayGameSound
    li       r3, 0x258
    bl       HSD_Randi
    addi     r0, r3, 0x258
    stw      r0, 0x7c(r29)
.L_000008D4:
    mr       r3, r30
    bl       UpdateAnas
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    lwz      r28, 0x20(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: stage09 .text 0x8FC
 * @note Size: 0x84
 */
extern "C" void InitStage(s32 dt)
{
	ambient.Clear();
	InitAnas();
}

// InitStage
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lis      r3, ambient@ha
    li       r4, 0x0
    stw      r0, 0x14(r1)
    li       r0, 0x2
    addi     r5, r3, ambient@l
    mtctr    r0
.L_0000091C:
    stw      r4, 0x4(r5)
    stw      r4, 0x8(r5)
    stw      r4, 0xc(r5)
    stw      r4, 0x10(r5)
    stw      r4, 0x14(r5)
    stw      r4, 0x18(r5)
    stw      r4, 0x1c(r5)
    stw      r4, 0x20(r5)
    stw      r4, 0x24(r5)
    stw      r4, 0x28(r5)
    stw      r4, 0x2c(r5)
    stw      r4, 0x30(r5)
    stw      r4, 0x34(r5)
    stw      r4, 0x38(r5)
    stwu     r4, 0x3c(r5)
    bdnz     .L_0000091C
    lis      r3, ambient@ha
    stbu     r4, ambient@l(r3)
    stw      r4, 0x80(r3)
    stw      r4, 0x84(r3)
    bl       InitAnas
    lwz      r0, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage09 .text 0x980
 * @note Size: 0x21C
 */
extern "C" void AmbientSeq_Update(AmbientSeq* seq, s32 dt)
{
	void* sound;
	bool cont;

	if (seq->wait > 0) {
		seq->wait -= dt;
		if (seq->wait > 0) {
			return;
		}
	}
	cont  = true;
	sound = SOUND();
	while (cont) {
		s32* cmd = &seq->data[seq->pos];
		switch (cmd[0]) {
		case 0:
			seq->pos  = 0;
			seq->wait = 0;
			break;
		case 1:
			PlayGameSound(sound, cmd[1], 0xFF);
			seq->pos += 2;
			break;
		case 2:
			if (IsGameSoundPlaying(sound, cmd[1]) != 0) {
				cont = false;
			} else {
				seq->pos += 2;
			}
			break;
		case 3:
			cont      = false;
			seq->wait = (60.0f * cmd[1]) / 100.0f;
			seq->pos += 2;
			break;
		case 4:
			StopGameSfx(sound, cmd[1]);
			seq->pos += 2;
			break;
		case 5: {
			// random wait between the two args, in hundredths of a second
			s32 time  = seq->data[seq->pos + 1] + HSD_Randi(cmd[2] - cmd[1]);
			cont      = false;
			seq->wait = (60.0f * time) / 100.0f;
			seq->pos += 3;
			break;
		}
		}
	}
}

/**
 * @note Address: stage09 .text 0xB9C
 * @note Size: 0x40
 */
// fn_11_B9C

/**
 * @note Address: stage09 .text 0xBDC
 * @note Size: 0xC
 */
// fn_11_BDC

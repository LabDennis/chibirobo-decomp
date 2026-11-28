#include "game/relana.h"
#include "game/relmodule.h"
#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include <sysdolphin/axdriver.h>

extern "C" {
#include <sysdolphin/random.h>
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
// the script commands: loop(0), play(1, id), wait for it(2, id), wait(3, cs), stop(4, id), random wait(5, lo, hi)
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

extern "C" {
// the rel's slots (three used) and its script commands
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
	case 0: // pause or unpause the ambience
		if (count >= 2) {
			if (args[1] != 0) {
				XSound* sound;
				ambient.mPaused = 1;
				sound           = &gGame.mSound;
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
	case 1: // construction work
		RegisterGameSoundName(&gGame.mSound, "sfx_uraniwa_ambient1.ssm");
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
	case 2: // construction work, faster hammering
		RegisterGameSoundName(&gGame.mSound, "sfx_uraniwa_ambient1.ssm");
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
	case 3: // the neighbor's mower
		RegisterGameSoundName(&gGame.mSound, "sfx_uraniwa_ambient2.ssm");
		ambient.Start(ambientNeighborMower);
		ambient.Start(ambientTrafficCarLr);
		ambient.Start(ambientTrafficCarRl);
		ambient.Start(ambientTrafficDump);
		ambient.mBirdTimer      = HSD_Randi(600) + 600;
		ambient.mBirdSounds     = ambientNatureBirds2;
		ambient.mBirdSoundCount = 5;
		break;
	case 4: // night
		RegisterGameSoundName(&gGame.mSound, "sfx_uraniwa_ambient3.ssm");
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
	XSound* sound = &gGame.mSound;
	Vec pos;

	// these two sounds always sit on fixed pans
	if (IsGameSoundPlaying(sound, 0x12001A) != 0) {
		HSD_AudioSFXSetPanByID(0x12001A, 0);
	}
	if (IsGameSoundPlaying(sound, 0x12001B) != 0) {
		HSD_AudioSFXSetPanByID(0x12001B, 0xFE);
	}
	pos.x = 458.0f;
	pos.y = 0.0f;
	pos.z = -555.5f; // 0x60005's volume follows the distance to here
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

/**
 * @note Address: stage09 .text 0x980
 * @note Size: 0x21C
 */
extern "C" void AmbientSeq_Update(AmbientSeq* seq, s32 dt)
{
	XSound* sound;
	bool cont;

	if (seq->wait > 0) {
		seq->wait -= dt;
		if (seq->wait > 0) {
			return;
		}
	}
	cont  = true;
	sound = &gGame.mSound;
	while (cont) {
		s32* cmd = &seq->data[seq->pos];
		switch (cmd[0]) {
		case 0: // back to the top
			seq->pos  = 0;
			seq->wait = 0;
			break;
		case 1: // play a sound
			PlayGameSound(sound, cmd[1], 0xFF);
			seq->pos += 2;
			break;
		case 2: // wait for a sound to finish
			if (IsGameSoundPlaying(sound, cmd[1]) != 0) {
				cont = false;
			} else {
				seq->pos += 2;
			}
			break;
		case 3: // wait, in hundredths of a second
			cont      = false;
			seq->wait = (60.0f * cmd[1]) / 100.0f;
			seq->pos += 2;
			break;
		case 4: // stop a sound
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

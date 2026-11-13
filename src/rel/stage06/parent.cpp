#include "game/cgame.h"
#include "types.h"
#include "Version.h"

// rel/ana.cpp
extern "C" s32 CmdDropHeldObj(s32 cmd, s32 count, s32* args);

// JP: _C8 at 0xC0, _980 at 0x970, _12DF4 at 0x12CF4
struct RoboView {
#if defined(VERSION_JP)
	u8 _00[0xC0]; // _00
#else
	u8 _00[0xC8]; // _00
#endif
	f32 mMoveTranslateX; // _C8, the player's CXObj::mMoveTranslate.x (JP _C0), his position x; copied into the stop camera
	f32 mMoveTranslateY; // _CC, the player's CXObj::mMoveTranslate.y (JP _C4), his position y; copied into the stop camera
	f32 mMoveTranslateZ; // _D0, the player's CXObj::mMoveTranslate.z (JP _C8), his position z; copied into the stop camera
#if defined(VERSION_JP)
	u8 _D4[0x970 - 0xCC]; // _D4
#else
	u8 _D4[0x980 - 0xD4]; // _D4
#endif
	u32 mPlayerStatus; // _980, XObjChr::mPlayerStatus.mFlags (JP _970); 0x40000000 hands him to a script, 0x20000000 | 7 gives control back
#if defined(VERSION_JP)
	u8 _984[0x12CF4 - 0x974]; // _984
#else
	u8 _984[0x12DF4 - 0x984]; // _984
#endif
	s32 mPadReset; // _12DF4, XObjChr::mPad.mReset (XPad at _12DA0, mReset at +0x54; JP _12CF4); 1 with the give-back flags, 0 with the
	               // script flags
};

struct Stage06Work {
	u8 _00[0x4];  // _00
	s32 _04;      // _04, never accessed in any edition
	s32 _08;      // _08, counted up by CmdStopRobo each time it stops the player, never read
	u8 _0C[0x18]; // _0C, SetCam copies the player's y (RoboView _CC) here, never read
};

struct Stage06Cam {
	u8 _00[0x4]; // _00
	s32 _04;     // _04, set to 1 by SetCam, never read
	f32 _08;     // _08, SetCam copies the player's x (RoboView _C8, CXObj::mMoveTranslate.x) here, never read
	f32 _0C;     // _0C
	f32 _10;     // _10, SetCam copies the player's z (RoboView _D0) here, never read
	f32 _14;     // _14, SetCam's first angle in radians, -pi/8 for preset 0, -7pi/8 for preset 1, never read
	f32 _18;     // _18, always 0.15f from SetCam, never read
	u8 _1C[0x4]; // _1C
	f32 _20;     // _20, SetCam's second angle in radians, about -pi/2 for preset 0, 0 for preset 1, never read
};

static Stage06Work lbl_8_bss_0;
static Stage06Cam lbl_8_bss_24;

extern "C" s32 CmdParentNoOpSlot5(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdStopRoboPreset(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdStopRobo(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdParentNoOpSlot4(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdParentNoOpSlot2(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdSetSoundPitch(s32 cmd, s32 count, s32* args);
extern "C" void ParentNoOpSlot5(void* arg);
extern "C" void UpdateStage(void* arg);
extern "C" void InitStage(void* arg);

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage, 0, 0, 0, ParentNoOpSlot5 };
RelCmdFunc _prolog_cmds[]
    = { CmdSetSoundPitch, CmdStopRobo, CmdParentNoOpSlot2, CmdStopRoboPreset, CmdParentNoOpSlot4, CmdParentNoOpSlot5, CmdDropHeldObj };
}

static inline void StopPlayer(RoboView* player)
{
	player->mPlayerStatus = 0;
	player->mPlayerStatus |= 0x40000000;
	player->mPadReset = 0;
}

static inline void SetCam(f32 a, f32 b)
{
	RoboView* player = (RoboView*)gGame.GetPlayer();

	StopPlayer(player);
	lbl_8_bss_24._04 = 1;
	lbl_8_bss_24._08 = player->mMoveTranslateX;
	lbl_8_bss_24._0C = player->mMoveTranslateY;
	lbl_8_bss_24._10 = player->mMoveTranslateZ;
	lbl_8_bss_24._14 = a;
	lbl_8_bss_24._20 = b;
	lbl_8_bss_24._18 = 0.15f;
}

/**
 * @note Address: stage06 .text 0x0
 * @note Size: 0x8
 */
extern "C" s32 CmdParentNoOpSlot5(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: N/A
 * @note Size: unknown
 */
// UNUSED FUNCTION
// stand-in for a stripped function that set the type 1 camera. only here so -2.7488937f
// and 0.0f come first in .rodata, ahead of SetCam's 0.15f and the type 0 pair
extern "C" void UnusedSetCamUser(void)
{
	SetCam(-2.7488937f, 0.0f);
}

/**
 * @note Address: stage06 .text 0x8
 * @note Size: 0x110
 */
extern "C" s32 CmdStopRoboPreset(s32 cmd, s32 count, s32* args)
{
	s32 type = *args;
	if (type == 0) {
		SetCam(-0.3926991f, -1.570196f);
	} else if (type == 1) {
		SetCam(-2.7488937f, 0.0f);
	}
	return 0;
}

/**
 * @note Address: stage06 .text 0x118
 * @note Size: 0x48
 */
extern "C" s32 CmdStopRobo(s32 cmd, s32 count, s32* args)
{
	StopPlayer((RoboView*)gGame.GetPlayer());
	lbl_8_bss_0._08++;
	return 0;
}

/**
 * @note Address: stage06 .text 0x160
 * @note Size: 0x8
 */
extern "C" s32 CmdParentNoOpSlot4(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage06 .text 0x168
 * @note Size: 0x8
 */
extern "C" s32 CmdParentNoOpSlot2(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage06 .text 0x170
 * @note Size: 0x50
 */
extern "C" s32 CmdSetSoundPitch(s32 cmd, s32 count, s32* args)
{
	s16 pitch = args[0];
	s32 time  = args[1] * 60 / 100;
	XSound* sound;

	if (time <= 0) {
		time = 1;
	}
	sound                    = &gGame.mSound;
	sound->mTargetMusicPitch = pitch;
	sound->mMusicPitchFrames = time;
	return 0;
}

/**
 * @note Address: stage06 .text 0x1C0
 * @note Size: 0x4
 */
extern "C" void ParentNoOpSlot5(void* arg)
{
}

/**
 * @note Address: stage06 .text 0x1C4
 * @note Size: 0xD8
 */
extern "C" void UpdateStage(void* arg)
{
	Vec pos;

	pos.x = 241.83f;
	pos.y = 220.56f;
	pos.z = 195.85f;
	if (IsGameSoundPlaying(&gGame.mSound, 0x10001D)) {
		SetGameSfxVolume(&gGame.mSound, 0x10001D, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 0), 0);
	}
	if (IsGameSoundPlaying(&gGame.mSound, 0x10001E)) {
		SetGameSfxVolume(&gGame.mSound, 0x10001E, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 0), 0);
	}
}

/**
 * @note Address: stage06 .text 0x29C
 * @note Size: 0x4
 */
extern "C" void InitStage(void* arg)
{
}

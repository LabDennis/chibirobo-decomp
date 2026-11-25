#include "game/hsd_jobj.h"
#include "game/xchr.h"
#include "game/cgame.h"
#include "game/playeranimationdata.h"
#include "game/animframe.h"
#include "game/globals.h"
#include <math.h>

extern "C" {
// the idle anim id of each suit, in mCurrent order
s32 gPlayerSuitIdleAnimations[9] = { 1, 0xCF7, 0xCF9, 0xCF8, 0xCFD, 1, 0xCFA, 0xCFB, 0xCFC };

// the per-anim footstep and action sounds, terminated by anim -1
FrameEvent gPlayerAnimationFrameEvents[] = {
	{ 18, -1.0f, -1.0f, FrameSoundStartIfSilent, FrameSoundStopIfPlaying, 12, 255 },
	{ 3, 2.0f, 17.0f, FrameSoundOnEitherBoundary, NULL, 10, 255 },
	{ 3, 7.0f, 22.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 12, 7.0f, 22.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 2198, 7.0f, 22.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 2072, 7.0f, 22.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 119, 11.0f, 26.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 136, 11.0f, 26.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 2199, 11.0f, 26.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 2178, 11.0f, 26.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 4, 6.0f, 21.0f, FrameSoundOnEitherBoundary, NULL, 10, 255 },
	{ 4, 11.0f, 26.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 13, 6.0f, 21.0f, FrameSoundOnEitherBoundary, NULL, 10, 255 },
	{ 13, 11.0f, 26.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 2200, 6.0f, 21.0f, FrameSoundOnEitherBoundary, NULL, 10, 255 },
	{ 2200, 11.0f, 26.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 2073, 6.0f, 21.0f, FrameSoundOnEitherBoundary, NULL, 10, 255 },
	{ 2073, 11.0f, 26.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 179, 2.0f, 16.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 298, 2.0f, 17.0f, FrameSoundOnEitherBoundary, NULL, 10, 255 },
	{ 298, 7.0f, 22.0f, FrameSoundOnEitherBoundary, NULL, 25, 255 },
	{ 10, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 5, 255 },
	{ 10, 42.0f, -1.0f, FrameSoundOnStartFrame, NULL, 6, 255 },
	{ 11, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 5, 255 },
	{ 11, 42.0f, -1.0f, FrameSoundOnStartFrame, NULL, 6, 255 },
	{ 159, 34.0f, -1.0f, FrameSoundOnStartFrame, NULL, 56, 255 },
	{ 160, 24.0f, -1.0f, FrameSoundOnStartFrame, NULL, 56, 255 },
	{ 173, 76.0f, -1.0f, FrameSoundOnStartFrame, NULL, 55, 255 },
	{ 172, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 55, 255 },
	{ 32, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 150, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 31, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 149, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 27, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 30, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 148, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 291, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 117, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 57, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 89, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 159, 92.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 173, 80.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 92, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 273, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 279, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 98, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 276, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 282, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 101, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 285, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 288, 2.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 91, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 60, 255 },
	{ 272, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 60, 255 },
	{ 278, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 60, 255 },
	{ 97, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 60, 255 },
	{ 275, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 60, 255 },
	{ 281, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 60, 255 },
	{ 100, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 60, 255 },
	{ 284, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 60, 255 },
	{ 287, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 60, 255 },
	{ 23, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 13, 255 },
	{ 125, 12.0f, -1.0f, FrameSoundOnStartFrame, NULL, 14, 255 },
	{ 21, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 63, 255 },
	{ 22, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 63, 255 },
	{ 24, 0.0f, 48.0f, FrameSoundInFrameWindow, NULL, 12, 255 },
	{ 24, 58.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 152, 0.0f, 48.0f, FrameSoundInFrameWindow, NULL, 12, 255 },
	{ 152, 58.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 109, 0.0f, 48.0f, FrameSoundInFrameWindow, NULL, 12, 255 },
	{ 109, 58.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 109, 97.0f, 116.0f, FrameSoundOnEitherBoundary, NULL, 26, 255 },
	{ 109, 133.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 109, 50.0f, -1.0f, FrameSoundOnStartFrame, NULL, 62, 255 },
	{ 139, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 16, 255 },
	{ 141, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 16, 255 },
	{ 64, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 32, 255 },
	{ 65, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 31, 255 },
	{ 53, 1.0f, -1.0f, FrameSoundOnStartFrame, NULL, 1, 255 },
	{ 269, 1.0f, -1.0f, FrameSoundOnStartFrame, NULL, 1, 255 },
	{ 67, 1.0f, -1.0f, FrameSoundOnStartFrame, NULL, 1, 255 },
	{ 270, 1.0f, -1.0f, FrameSoundOnStartFrame, NULL, 1, 255 },
	{ 67, 1.0f, -1.0f, FrameSoundStopIfPlaying, NULL, 59, 255 },
	{ 270, 1.0f, -1.0f, FrameSoundStopIfPlaying, NULL, 59, 255 },
	{ 67, 1.0f, -1.0f, FrameSoundStopIfPlaying, NULL, 54, 255 },
	{ 270, 1.0f, -1.0f, FrameSoundStopIfPlaying, NULL, 54, 255 },
	{ 90, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 271, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 277, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 96, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 274, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 280, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 99, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 283, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 286, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 19, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 19, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 62, 255 },
	{ 25, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 102, 10.0f, 33.0f, FrameSoundOnEitherBoundary, NULL, 9, 255 },
	{ 102, 37.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 102, 25.0f, -1.0f, FrameSoundOnStartFrame, NULL, 62, 255 },
	{ 105, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 105, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 106, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 106, 18.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 110, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 110, 20.0f, 40.0f, FrameSoundOnEitherBoundary, NULL, 9, 255 },
	{ 110, 17.0f, 38.0f, FrameSoundOnEitherBoundary, NULL, 26, 255 },
	{ 110, 58.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 111, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 111, 20.0f, 40.0f, FrameSoundOnEitherBoundary, NULL, 9, 255 },
	{ 111, 17.0f, 38.0f, FrameSoundOnEitherBoundary, NULL, 26, 255 },
	{ 111, 58.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 107, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 107, 18.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 108, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 108, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 114, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 115, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 33, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 33, 28.0f, -1.0f, FrameSoundOnStartFrame, NULL, 62, 255 },
	{ 37, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 5, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 7, 24.0f, -1.0f, FrameSoundOnStartFrame, NULL, 7, 255 },
	{ 62, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 62, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 62, 255 },
	{ 66, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 9, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 159, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 173, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 160, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 9, 255 },
	{ 122, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 61, 255 },
	{ 202, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 61, 255 },
	{ 2068, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 5, 255 },
	{ 2071, 26.0f, -1.0f, FrameSoundOnStartFrame, NULL, 6, 255 },
	{ 2070, 4.0f, -1.0f, FrameSoundOnStartFrame, NULL, 25, 255 },
	{ 2169, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 5, 255 },
	{ 2170, 26.0f, -1.0f, FrameSoundOnStartFrame, NULL, 6, 255 },
	{ 2079, 1.0f, -1.0f, FrameSoundOnStartFrame, NULL, 4, 255 },
	{ 2080, 1.0f, -1.0f, FrameSoundOnStartFrame, NULL, 4, 255 },
	{ 2081, 1.0f, -1.0f, FrameSoundOnStartFrame, NULL, 2, 255 },
	{ 2084, 1.0f, -1.0f, FrameSoundOnStartFrame, NULL, 2, 255 },
	{ 120, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 5, 255 },
	{ 120, 60.0f, -1.0f, FrameSoundOnStartFrame, NULL, 6, 255 },
	{ 239, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 5, 255 },
	{ 239, 60.0f, -1.0f, FrameSoundOnStartFrame, NULL, 6, 255 },
	{ 315, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 5, 255 },
	{ 315, 60.0f, -1.0f, FrameSoundOnStartFrame, NULL, 6, 255 },
	{ 175, 15.0f, 37.0f, FrameSoundOnEitherBoundary, NULL, 26, 255 },
	{ 175, 59.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 176, 18.0f, 40.0f, FrameSoundOnEitherBoundary, NULL, 26, 255 },
	{ 176, 61.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 234, 15.0f, 37.0f, FrameSoundOnEitherBoundary, NULL, 26, 255 },
	{ 234, 59.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 235, 18.0f, 40.0f, FrameSoundOnEitherBoundary, NULL, 26, 255 },
	{ 235, 61.0f, -1.0f, FrameSoundOnStartFrame, NULL, 26, 255 },
	{ 2197, 10.0f, -1.0f, FrameSoundUntilAnimEnd, NULL, 253, 255 },
	{ 188, 48.0f, -1.0f, FrameSoundOnStartFrame, NULL, 56, 255 },
	{ 191, 85.0f, -1.0f, FrameSoundOnStartFrame, NULL, 280, 255 },
	{ 191, 142.0f, -1.0f, FrameSoundOnStartFrame, NULL, 55, 255 },
	{ 2222, 5.0f, -1.0f, FrameSoundOnStartFrame, NULL, 268, 255 },
	{ 2222, 80.0f, -1.0f, FrameSoundOnStartFrame, NULL, 269, 255 },
	{ 2225, 10.0f, -1.0f, FrameSoundOnStartFrame, NULL, 57, 255 },
	{ 1187, 35.0f, 47.0f, FrameSoundOnEitherBoundary, NULL, 281, 255 },
	{ 221, 22.0f, -1.0f, FrameSoundOnStartFrame, NULL, 347, 255 },
	{ 213, 25.0f, -1.0f, FrameSoundOnStartFrame, NULL, 289, 255 },
	{ 246, 25.0f, -1.0f, FrameSoundOnStartFrame, NULL, 289, 255 },
	{ 1232, 25.0f, -1.0f, FrameSoundOnStartFrame, NULL, 289, 255 },
	{ 55, 30.0f, 35.0f, FrameSoundOnEitherBoundary, NULL, 26, 255 },
	{ 55, 110.0f, 120.0f, FrameSoundInFrameWindow, NULL, 12, 255 },
	{ 238, 110.0f, 120.0f, FrameSoundInFrameWindow, NULL, 12, 255 },
	{ 3308, 100.0f, 110.0f, FrameSoundInFrameWindow, NULL, 12, 255 },
	{ 2245, 7.0f, -1.0f, FrameSoundOnStartFrame, NULL, 365, 255 },
	{ 2245, 32.0f, -1.0f, FrameSoundOnStartFrame, NULL, 366, 255 },
	{ 2240, 1.0f, -1.0f, FrameSoundOnStartFrame, NULL, 373, 255 },
	{ 2241, 57.0f, -1.0f, FrameSoundOnStartFrame, NULL, 373, 255 },
	{ 2242, 48.0f, -1.0f, FrameSoundOnStartFrame, NULL, 374, 255 },
	{ 2243, 48.0f, -1.0f, FrameSoundOnStartFrame, NULL, 375, 255 },
	{ 2244, 48.0f, -1.0f, FrameSoundOnStartFrame, NULL, 376, 255 },
	{ 1299, 20.0f, -1.0f, FrameSoundOnStartFrame, NULL, 408, 255 },
	{ -1, 0.0f, 0.0f, NULL, NULL, 0, 0 },
};
// the copter anim events, terminated by anim -1
FrameEvent gCopterAnimationFrameEvents[] = {
	{ 5, 22.0f, -1.0f, FrameSoundOnStartFrame, NULL, 15, 255 }, { 8, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 15, 255 },
	{ 6, 0.0f, -1.0f, FrameSoundOnStartFrame, NULL, 8, 255 },   { 2, 0.0f, -1.0f, FrameSoundStartIfSilent, NULL, 8, 255 },
	{ 3, 0.0f, -1.0f, FrameSoundStartIfSilent, NULL, 8, 255 },  { 7, 0.0f, -1.0f, FrameSoundStopIfPlaying, NULL, 8, 255 },
	{ 0, 0.0f, -1.0f, FrameSoundStopIfPlaying, NULL, 8, 255 },  { -1, 0.0f, 0.0f, NULL, NULL, 0, 0 },
};
// the alternate anim ids for carrying, lifting and attachments, terminated by -1
PlayerAnimVariants gPlayerAnimVariants[] = {
	{ 17, 8, 2196, -1 },   { 1, 8, 2196, -1 },    { 93, 8, 2196, -1 },    { 94, 8, 2196, -1 },    { 95, 8, 2196, -1 },
	{ 8, 8, 2196, -1 },    { 212, 228, 229, -1 }, { 121, 137, 224, -1 },  { 3, 12, 2198, 2072 },  { 119, 136, 2199, 2178 },
	{ 4, 13, 2200, 2073 }, { 2, 2, 2, 266 },      { 53, 53, 53, 269 },    { 67, 67, 67, 270 },    { 120, 315, 239, -1 },
	{ 90, 271, 277, -1 },  { 91, 272, 278, -1 },  { 92, 273, 279, -1 },   { 96, 274, 280, -1 },   { 97, 275, 281, -1 },
	{ 98, 276, 282, -1 },  { 99, 283, 286, -1 },  { 100, 284, 287, -1 },  { 101, 285, 288, -1 },  { 122, 202, -1, -1 },
	{ 32, 150, 297, -1 },  { 31, 149, 296, -1 },  { 28, 146, 289, -1 },   { 29, 147, 290, -1 },   { 30, 148, 291, -1 },
	{ 142, 155, 292, -1 }, { 143, 156, 293, -1 }, { 144, 157, 294, 267 }, { 154, 158, 295, 268 }, { -1, 0, 0, 0 },
};
}

// a particle an anim frame table asks for
struct PlayerFrameParticle {
	s32 mCode;      // _00, -1 for none
	Vec mTranslate; // _04
	Vec mScale;     // _10
	Vec mRotate;    // _1C
};
STATIC_ASSERT(sizeof(PlayerFrameParticle) == 0x28);

extern "C" {
PlayerFrameParticle ProcessPlayerFrameEvents(XObj*, const FrameEventTime*, const FrameEvent*);
}

// steps the anim frame and fires the particles the frame tables ask for
/**
 * @note Address: 0x800A7C70
 * @note Size: 0x1AC
 */
extern "C" void UpdatePlayerFrameEffects(XObjChr* player)
{
	player->mFrameTime.mLastFrame = player->mFrameTime.mFrame;
	HSD_AObj* aobj                = GetAObj(player->mJObj);
	player->mFrameTime.mFrame     = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
	PlayerFrameParticle particle  = ProcessPlayerFrameEvents(player, &player->mFrameTime, gPlayerAnimationFrameEvents);
	if (particle.mCode >= 0 && !(gGame.mStatus.mFlags & 0x100ULL)) {
		gGame.mParticle.CreateSRT((u32)particle.mCode | 0x10000, 1, particle.mTranslate, particle.mScale, particle.mRotate, FALSE);
	}
	if (player->mCopterObj.mVisible != 0)
		ProcessPlayerFrameEvents(&player->mCopterObj, NULL, gCopterAnimationFrameEvents);
}

static void SetFrameParticleAtJoint(PlayerFrameParticle& particle, XObjChr* player, s32 code)
{
	particle.mCode      = code;
	s32 path[]          = { 0, 1, 0, 1, 2, 0, 1 };
	Vec offset          = { 0.0f, 2.0f, 0.0f };
	particle.mTranslate = GetEventJointWorldPosition(player, 7, path) + offset;
	particle.mScale     = player->mMoveScale;
	particle.mRotate    = player->mMoveRotate;
}

/**
 * @note Address: 0x800A7E1C
 * @note Size: 0x638
 */
extern "C" PlayerFrameParticle ProcessPlayerFrameEvents(XObj* object, const FrameEventTime* frames, const FrameEvent* table)
{
	PlayerFrameParticle particle = { -1 };
	HSD_AObj* aobj               = GetAObj(object->mJObj);
	if (aobj == NULL)
		return particle;
	s32 anim = object->mAnimId;
	FrameEventTime time;
	if (frames != NULL) {
		time = *frames;
	} else {
		time.mFrame     = HSD_AObjGetCurrentFrame(aobj);
		time.mLastFrame = time.mFrame - 1.0f;
	}
	for (const FrameEvent* event = table; event->mAnim >= 0; ++event) {
		s32 result = 0;
		if (anim == event->mAnim && event->mWhenAnimMatches != NULL)
			result = event->mWhenAnimMatches(&aobj->flags, &time, event);
		else if (anim != event->mAnim && event->mWhenAnimDiffers != NULL)
			result = event->mWhenAnimDiffers(&aobj->flags, &time, event);
		if (result > 0) {
			XObjChr* player = gGame.GetPlayer();
			if (event->mId == 7 && anim == 7 && player->mLift.mTarget != NULL) {
				// sound 7 in anim 7 sounds like what he holds: an item (pool 8) by its own sound, anything else by its floor material
				XObj* held = player->mLift.mTarget;
				if ((s16)held->GetPool() == 8) {
					s8 material = GetItemSoundIndex((s16)held->GetItem(), 0);
					if (material >= 0)
						PlayGameSound(&gGame.mSound, GetMaterialSoundId(material), event->mVolume);
				} else {
					PlayGameSound(&gGame.mSound, GetMaterialSoundId(held->mFloors->mType >> 8), event->mVolume);
				}
			} else {
				PlayGameSound(&gGame.mSound, event->mId, event->mVolume);
			}
			if (event->mId == 5) {
				SetFrameParticleAtJoint(particle, player, 0x18);
			} else if (event->mId == 6) {
				SetFrameParticleAtJoint(particle, player, 0x19);
			} else if (event->mId == 0x38 && anim == 0x9F) {
				const Vec& tint = player->mColorStep[0].mValue;
				u32 color       = ((u32)tint.x << 24) | ((u32)tint.y << 16) | ((u32)tint.z << 8);
				// no particle while his modulate color is plain white (untinted)
				if (color < 0xFFFFFF00) {
					particle.mCode      = 0x55;
					s32 path[]          = { 0, 0 };
					particle.mTranslate = GetEventJointWorldPosition(player, 2, path);
					particle.mTranslate.x += 2.4f * (f32)sin(player->mMoveRotate.y);
					particle.mTranslate.z += 2.4f * (f32)cos(player->mMoveRotate.y);
					particle.mTranslate.y += 0.5f;
					particle.mScale  = player->mMoveScale;
					particle.mRotate = player->mMoveRotate;
				}
			}
		} else if (result < 0) {
			StopGameSfx(&gGame.mSound, event->mId);
		}
	}
	return particle;
}

// stop once the sound is playing
/**
 * @note Address: 0x800A8454
 * @note Size: 0x40
 */
extern "C" s32 FrameSoundStopIfPlaying(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	return IsGameSoundPlaying(&gGame.mSound, event->mId) ? -1 : 0;
}

// start while the sound isn't playing
/**
 * @note Address: 0x800A8494
 * @note Size: 0x3C
 */
extern "C" s32 FrameSoundStartIfSilent(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	return !IsGameSoundPlaying(&gGame.mSound, event->mId);
}

// keep the sound playing inside the window only
/**
 * @note Address: 0x800A84D0
 * @note Size: 0x90
 */
extern "C" s32 FrameSoundInFrameWindow(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	if (time->mFrame >= event->mStart && time->mFrame < event->mEnd) {
		if (!IsGameSoundPlaying(&gGame.mSound, event->mId))
			return 1;
	} else {
		if (IsGameSoundPlaying(&gGame.mSound, event->mId))
			return -1;
	}
	return 0;
}

// keep the sound playing until the anim ends
/**
 * @note Address: 0x800A8560
 * @note Size: 0x7C
 */
extern "C" s32 FrameSoundUntilAnimEnd(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	if (*aobjFlags & 0x44000000) {
		if (IsGameSoundPlaying(&gGame.mSound, event->mId))
			return -1;
	} else {
		if (!IsGameSoundPlaying(&gGame.mSound, event->mId))
			return 1;
	}
	return 0;
}

// fires on the frame it crosses mStart or mEnd, a loop back counts as crossing
/**
 * @note Address: 0x800A85DC
 * @note Size: 0x64
 */
extern "C" s32 FrameSoundOnEitherBoundary(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	if (((time->mLastFrame < event->mStart || time->mLastFrame > time->mFrame) && time->mFrame >= event->mStart)
	    || ((time->mLastFrame < event->mEnd || time->mLastFrame > time->mFrame) && time->mFrame >= event->mEnd))
		return 1;
	return 0;
}

// fires on the frame it crosses mStart
/**
 * @note Address: 0x800A8640
 * @note Size: 0x3C
 */
extern "C" s32 FrameSoundOnStartFrame(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	if ((time->mLastFrame < event->mStart || time->mLastFrame > time->mFrame) && time->mFrame >= event->mStart)
		return 1;
	return 0;
}

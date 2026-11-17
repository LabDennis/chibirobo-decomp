#pragma once

#include <types.h>

struct FrameEventTime;
struct FrameEvent;

#if defined(VERSION_R24J01)
class XObj;
// the anim's frame now and the frame it was at the last update
struct FrameEventTime {
	f32 mFrame;     // _00
	f32 mLastFrame; // _04
};
// a frame window check, 1 starts the event, -1 stops it, 0 leaves it. The Wii's take the object
// whose anim it is where the GameCube's take its aobj flags
typedef s32 (*FrameEventFunc)(XObj* object, FrameEventTime* time, const FrameEvent* event);
#else
// a frame window check, 1 starts the event, -1 stops it, 0 leaves it
typedef s32 (*FrameEventFunc)(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event);
#endif

// one entry of an anim's frame event table, like gPlayerAnimationFrameEvents
struct FrameEvent {
	s32 mAnim;                       // _00
	f32 mStart;                      // _04
	f32 mEnd;                        // _08
	FrameEventFunc mWhenAnimMatches; // _0C
	FrameEventFunc mWhenAnimDiffers; // _10
	s32 mId;                         // _14, a sound or particle
	u8 mVolume;                      // _18
};
STATIC_ASSERT(sizeof(FrameEvent) == 0x1C);

struct PlayerAnimVariants {
	s32 normal;
	s32 lifted;
	s32 carrying;
	s32 attachment;
};

extern "C" {
#if defined(VERSION_R24J01)
s32 FrameSoundInFrameWindow(XObj* object, FrameEventTime* time, const FrameEvent* event);
s32 FrameSoundOnEitherBoundary(XObj* object, FrameEventTime* time, const FrameEvent* event);
s32 FrameSoundOnStartFrame(XObj* object, FrameEventTime* time, const FrameEvent* event);
s32 FrameSoundStartIfSilent(XObj* object, FrameEventTime* time, const FrameEvent* event);
s32 FrameSoundStopIfPlaying(XObj* object, FrameEventTime* time, const FrameEvent* event);
s32 FrameSoundUntilAnimEnd(XObj* object, FrameEventTime* time, const FrameEvent* event);
#else
s32 FrameSoundInFrameWindow(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event);
s32 FrameSoundOnEitherBoundary(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event);
s32 FrameSoundOnStartFrame(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event);
s32 FrameSoundStartIfSilent(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event);
s32 FrameSoundStopIfPlaying(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event);
s32 FrameSoundUntilAnimEnd(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event);
#endif
extern FrameEvent gPlayerAnimationFrameEvents[];
extern FrameEvent gCopterAnimationFrameEvents[];
extern PlayerAnimVariants gPlayerAnimVariants[];
}

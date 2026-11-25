#ifndef _GAME_VIDEO_H
#define _GAME_VIDEO_H

#include "types.h"
#include "Version.h"

extern "C" {
// frames per second, 50 or 60 by the video mode the player picked. europe only
extern s32 gVideoFramesPerSecond;
}

// what the frame based timers count in a second
#if defined(VERSION_PAL)
#define FRAMES_PER_SECOND gVideoFramesPerSecond
#else
#define FRAMES_PER_SECOND 60
#endif
#define FRAME_RATE ((f32)FRAMES_PER_SECOND)

#endif

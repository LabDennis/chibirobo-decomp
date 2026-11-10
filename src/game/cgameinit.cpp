#include "game/cgame.h"
#include "game/arcache.h"
#include "game/cthread.h"

// The game destructor node follows this storage, at 0x134 in JP and 0x140 in US/PAL.
extern "C" {
u8 lbl_8025C200[0xF8];
Vec gJointTranslateZero;
Vec gJointScaleZero;
Vec gJointRotationZero;
#if defined(VERSION_JP)
u8 sJointPoolRemainder[0x18];
#else
u8 sJointPoolRemainder[0x24];
#endif
}

CGame gGame __attribute__((aligned(256)));

// The cache destructor node follows these 0x190 bytes.
#if defined(VERSION_JP)
u8 lbl_80339470[0x190];
#elif defined(VERSION_PAL)
u8 lbl_80362470[0x190];
#else
u8 lbl_8035B870[0x190];
#endif
ARCacheInfo gARAMCacheInfo;
FbMirrorEx gFramebufferMirrors[5];
ARPreCache gAramPrecache;
CThread gMovieUpdateThread;
u8 gMovieUpdateThreadStack[0x1000];

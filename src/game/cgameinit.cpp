#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/arcache.h"
#include "game/cthread.h"

// retail's bss ahead of gGame: 0xf8 bytes, three zeroed joint vecs and the joint pool
// remainder, never referenced from this file's code
u8 lbl_8025C200[0xF8];
Vec gJointTranslateZero;
Vec gJointScaleZero;
Vec gJointRotationZero;
#if defined(VERSION_PAL)
u8 sJointPoolRemainder[0x24];
#else
u8 sJointPoolRemainder[0x18];
#endif

// retail places gGame on a 0x200 boundary, leaving 0xc0 bytes after the
// compiler's destructor chain
CGame gGame ATTRIBUTE_ALIGN(0x200);
// retail bss between gGame and the aram cache info, never referenced from code
u8 lbl_80339470[0x190];
ARCacheInfo gARAMCacheInfo;
FbMirrorEx gFramebufferMirrors[5];
extern "C" {
ARPreCache gAramPrecache;
CThread gMovieUpdateThread;
u8 gMovieUpdateThreadStack[0x1000];
}

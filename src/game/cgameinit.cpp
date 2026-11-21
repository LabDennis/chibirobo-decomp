#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/arcache.h"
#include "game/cthread.h"

// NONMATCHING: __sinit_cgameinit_cpp is the compiler's and its code is the same as retail's, but retail's .bss
// holds more data ahead of these objects, so the offsets from the section base differ
CGame gGame;
ARCacheInfo gARAMCacheInfo;
FbMirrorEx gFramebufferMirrors[5];
extern "C" {
ARPreCache gAramPrecache;
CThread gMovieUpdateThread;
u8 gMovieUpdateThreadStack[0x1000];
}

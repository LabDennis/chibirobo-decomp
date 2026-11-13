#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/arcache.h"
#include "game/cthread.h"

CGame gGame;
ARCacheInfo gARAMCacheInfo;
FbMirrorEx gFramebufferMirrors[5];
extern "C" {
ARPreCache gAramPrecache;
CThread gMovieUpdateThread;
u8 gMovieUpdateThreadStack[0x1000];
}

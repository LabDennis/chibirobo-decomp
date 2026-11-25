#include "game/cgame.h"
#include "game/arcache.h"
#include "game/cthread.h"

CGame gGame;
ARCacheInfo gARAMCacheInfo;
FbMirrorEx gFramebufferMirrors[5];
ARPreCache gAramPrecache;
CThread gMovieUpdateThread;
u8 gMovieUpdateThreadStack[0x1000];

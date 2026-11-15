#include "Version.h"
#if !defined(VERSION_R24J01)
#include "game/hsd_jobj.h"
#endif
#include "game/ccameraaction.h"

// __sinit at 0x80138154 builds it and registers its dtor
CCameraActionAreaEx gCameraAreaState;

#include "Version.h"
#include "game/ccameraaction.h"

// the Wii binary places this __sinit (0x801542A0) after the destructors; the GameCube defines the global in
// game/ccameraactionareaex.cpp
CCameraActionAreaEx gCameraAreaState;

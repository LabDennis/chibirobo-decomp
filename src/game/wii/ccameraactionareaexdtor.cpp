#include "Version.h"
#include "game/ccameraaction.h"
#include "game/gmath.h"
extern "C" {
#include <dolphin/os.h>
}

// the GameCube defines these in game/ccameraactionareaex.cpp

/**
 * @note Address: 0x801541F8
 * @note Size: 0x68
 */
CCameraActionAreaEx::~CCameraActionAreaEx()
{
}

// 0x80154260
CamAreaSlot::~CamAreaSlot()
{
}

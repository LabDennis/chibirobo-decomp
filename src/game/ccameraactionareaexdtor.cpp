#if !defined(VERSION_R24J01)
#include "game/hsd_jobj.h"
#endif
#include "game/ccameraaction.h"
#if defined(VERSION_R24J01)
#include "game/gmath.h"
extern "C" {
#include <dolphin/os.h>
}
#endif

/**
 * @note Address: 0x801381C0
 * @note Size: 0x70
 */
CCameraActionAreaEx::~CCameraActionAreaEx()
{
}

// 0x80138230
CamAreaSlot::~CamAreaSlot()
{
}

#if !defined(VERSION_R24J01)
/**
 * @note Address: 0x80138284
 * @note Size: 0x10
 */
// the Wii file emits this constructor inside ccameraactionmap2.cpp, where the
// retail binary placed it at 0x80154310
CamAreaSlot::CamAreaSlot()
{
}
#endif

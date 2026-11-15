#include <nw4r/g3d.h>
#include <nw4r/ut/ut_Color.h>
#include <nw4r/ut/ut_LockedCache.h>
#include <nw4r/ut/ut_algorithm.h>
#include <nw4r/ut/ut_binaryFileFormat.h>

#include "revolution/gx.h"
#include "revolution/gx/GXHardwareFields.h"
#include "revolution/os/OSAlarm.h"
#include "Dolphin/os.h"
#include "Dolphin/vi.h"

namespace {

NW4R_LIB_VERSION(G3D, "Nov 20 2008", "03:23:14", "0x4199_60831");

} // namespace

namespace nw4r {
namespace g3d {

void G3dInit(bool enableLockedCache) {
    OSRegisterVersion(NW4R_G3D_Version_);

    if (enableLockedCache) {
        ut::LC::Enable();
    } else {
        ut::LC::Disable();
    }

    InitFastCast();

    GXRenderModeObj* pMode;
    switch (VIGetTvFormat()) {
    case VI_NTSC: {
        pMode = &GXNtsc480IntDf;
        break;
    }

    case VI_PAL: {
        pMode = &GXPal528IntDf;
        break;
    }

    case VI_EURGB60: {
        pMode = &GXEurgb60Hz480IntDf;
        break;
    }

    case VI_MPAL: {
        pMode = &GXMpal480IntDf;
        break;
    }

    default: {
        pMode = &GXNtsc480IntDf;
        break;
    }
    }

    G3DState::SetRenderModeObj(*pMode);
}

void G3dReset() {
    G3DState::Invalidate();
}

} // namespace g3d
} // namespace nw4r

#include "Version.h"

extern const char sTypeNameCCameraActionReset[];
extern const char sResetTypeNameCInterfaceCameraAction[];
extern const char sResetTypeNameCCameraActionAreaExM[];

char sResetCameraPreviousUpdateName[] = "PreviousUpdate";

const void* gResetTypeInfoCInterfaceCameraAction[] = { sResetTypeNameCInterfaceCameraAction, 0 };
const void* sResetBaseTypesCCameraActionAreaExM[]  = { gResetTypeInfoCInterfaceCameraAction, 0, 0 };
const void* gResetTypeInfoCCameraActionAreaExM[]   = { sResetTypeNameCCameraActionAreaExM, sResetBaseTypesCCameraActionAreaExM };
const void* sBaseTypesCCameraActionReset[]         = { gResetTypeInfoCInterfaceCameraAction, 0, gResetTypeInfoCCameraActionAreaExM, 0, 0 };
const void* __RTTI__18CCameraActionReset[]         = { sTypeNameCCameraActionReset, sBaseTypesCCameraActionReset };

const float sResetCameraFollowDivisor = 4.0f;
const float sResetCameraZero          = 0.0f;
#if defined(VERSION_PAL)
const float sResetCameraUnit                  = 1.0f;
const double sResetCameraSignedConversionBias = 4503601774854144.0;
#else
const float sResetCameraFrameTime = 1.0f / 60.0f;
#endif
const float sResetCameraFovStepTime     = 15.0f;
const float sResetCameraHeightBlendTime = 8.0f;
const char sResetCameraPanicFormat[]    = "%s%s";
const float sResetCameraDefaultTime     = 2.0f;

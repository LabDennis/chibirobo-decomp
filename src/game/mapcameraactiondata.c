extern const char sTypeNameCCameraActionMap[];
extern const char sMapTypeNameCInterfaceCameraAction[];
extern const char sMapTypeNameCCameraActionAreaExM[];

char sMapCameraSpringDampLabel[]   = "SpringDamp";
float sMapCameraSpringParameters[] = { 0.5f, 2.5f, 4.5f };
char sMapCameraIntersectVecLabel[] = "CalcIntersectVec";
char sMapCameraUpdateTurnLabel[]   = "UpdateTurn";

char sMapCameraResetLabel[]  = "Reset";
char sMapCameraReset2Label[] = "Reset2";
#pragma push
#pragma dont_zero_bss on
int sMapCameraCollisionMode0[1] = { 0 };
#pragma pop
int sMapCameraCollisionMode1 = 1;
char sMapCameraUpdateLabel[] = "Update";

const void* gMapTypeInfoCInterfaceCameraAction[] = { sMapTypeNameCInterfaceCameraAction, 0 };
const void* sMapBaseTypesCCameraActionAreaExM[]  = { gMapTypeInfoCInterfaceCameraAction, 0, 0 };
const void* gMapTypeInfoCCameraActionAreaExM[]   = { sMapTypeNameCCameraActionAreaExM, sMapBaseTypesCCameraActionAreaExM };
const void* sBaseTypesCCameraActionMap[]         = { gMapTypeInfoCInterfaceCameraAction, 0, gMapTypeInfoCCameraActionAreaExM, 0, 0 };
const void* gTypeInfoCCameraActionMap[]          = { sTypeNameCCameraActionMap, sBaseTypesCCameraActionMap };

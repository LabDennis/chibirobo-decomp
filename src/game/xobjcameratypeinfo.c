const char sTypeNameXObjCamera[]      = "XObjCamera";
const char sCameraTypeNameCXObjIntp[] = "CXObjIntp";
const char sCameraTypeNameCJObj[]     = "CJObj";
const char sCameraTypeNameCXObj[]     = "CXObj";
const char sCameraTypeNameXObj[]      = "XObj";

const void* gCameraTypeInfoCJObj[]      = { sCameraTypeNameCJObj, 0 };
const void* sCameraBaseTypesCXObj[]     = { gCameraTypeInfoCJObj, 0, 0 };
const void* gCameraTypeInfoCXObj[]      = { sCameraTypeNameCXObj, sCameraBaseTypesCXObj };
const void* sCameraBaseTypesCXObjIntp[] = { gCameraTypeInfoCJObj, 0, gCameraTypeInfoCXObj, 0, 0 };
const void* gCameraTypeInfoCXObjIntp[]  = { sCameraTypeNameCXObjIntp, sCameraBaseTypesCXObjIntp };
const void* sCameraBaseTypesXObj[]      = { gCameraTypeInfoCJObj, 0, gCameraTypeInfoCXObj, 0, gCameraTypeInfoCXObjIntp, 0, 0 };
const void* gCameraTypeInfoXObj[]       = { sCameraTypeNameXObj, sCameraBaseTypesXObj };
const void* sBaseTypesXObjCamera[]
    = { gCameraTypeInfoCJObj, 0, gCameraTypeInfoCXObj, 0, gCameraTypeInfoCXObjIntp, 0, gCameraTypeInfoXObj, 0, 0 };
const void* __RTTI__10XObjCamera[] = { sTypeNameXObjCamera, sBaseTypesXObjCamera };

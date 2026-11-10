const char sTypeNameXObjLight[]      = "XObjLight";
const char sLightTypeNameCXObjIntp[] = "CXObjIntp";
const char sLightTypeNameCJObj[]     = "CJObj";
const char sLightTypeNameCXObj[]     = "CXObj";
const char sLightTypeNameXObj[]      = "XObj";

const void* gLightTypeInfoCJObj[]      = { sLightTypeNameCJObj, 0 };
const void* sLightBaseTypesCXObj[]     = { gLightTypeInfoCJObj, 0, 0 };
const void* gLightTypeInfoCXObj[]      = { sLightTypeNameCXObj, sLightBaseTypesCXObj };
const void* sLightBaseTypesCXObjIntp[] = { gLightTypeInfoCJObj, 0, gLightTypeInfoCXObj, 0, 0 };
const void* gLightTypeInfoCXObjIntp[]  = { sLightTypeNameCXObjIntp, sLightBaseTypesCXObjIntp };
const void* sLightBaseTypesXObj[]      = { gLightTypeInfoCJObj, 0, gLightTypeInfoCXObj, 0, gLightTypeInfoCXObjIntp, 0, 0 };
const void* gLightTypeInfoXObj[]       = { sLightTypeNameXObj, sLightBaseTypesXObj };
const void* sBaseTypesXObjLight[]
    = { gLightTypeInfoCJObj, 0, gLightTypeInfoCXObj, 0, gLightTypeInfoCXObjIntp, 0, gLightTypeInfoXObj, 0, 0 };
const void* __RTTI__9XObjLight[] = { sTypeNameXObjLight, sBaseTypesXObjLight };

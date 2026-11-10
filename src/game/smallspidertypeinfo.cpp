extern "C" {
extern const char sTypeNameXObjSpiderSmall[16]       = "XObjSpiderSmall";
extern const char sSmallSpiderTypeNameCXObjIntp[12]  = "CXObjIntp";
extern const char sSmallSpiderRotationAssert[40]     = "!(jobj->flags & JOBJ_USE_QUATERNION)";
extern const char sSmallSpiderTranslateCondition[12] = "translate";
extern const char sSmallSpiderTypeNameCStatus64[12]  = "CStatus64";
extern const char sSmallSpiderTypeNameCoordInfo[12]  = "CoordInfo";
extern const char sSmallSpiderTypeNameAttrInfo[12]   = "AttrInfo";
extern const char sSmallSpiderTypeNameCEvtData[12]   = "CEvtData";
#if defined(VERSION_GGTP01)
extern const char sSmallSpiderTypeNameCAnmQueue[12] = "CAnmQueue";
#else
extern const char sSmallSpiderTypeNameCAnmQueue[12] = { 'C', 'A', 'n', 'm', 'Q', 'u', 'e', 'u', 'e', 0, '0', '0' };
#endif
extern const char sSmallSpiderTypeNameCJObj[] = "CJObj";
extern const char sSmallSpiderTypeNameCXObj[] = "CXObj";
extern const char sSmallSpiderTypeNameXObj[]  = "XObj";

const void* gSmallSpiderTypeInfoCJObj[]      = { sSmallSpiderTypeNameCJObj, 0 };
const void* sSmallSpiderBaseTypesCXObj[]     = { gSmallSpiderTypeInfoCJObj, 0, 0 };
const void* gSmallSpiderTypeInfoCXObj[]      = { sSmallSpiderTypeNameCXObj, sSmallSpiderBaseTypesCXObj };
const void* sSmallSpiderBaseTypesCXObjIntp[] = { gSmallSpiderTypeInfoCJObj, 0, gSmallSpiderTypeInfoCXObj, 0, 0 };
const void* gSmallSpiderTypeInfoCXObjIntp[]  = { sSmallSpiderTypeNameCXObjIntp, sSmallSpiderBaseTypesCXObjIntp };
const void* sSmallSpiderBaseTypesXObj[]
    = { gSmallSpiderTypeInfoCJObj, 0, gSmallSpiderTypeInfoCXObj, 0, gSmallSpiderTypeInfoCXObjIntp, 0, 0 };
const void* gSmallSpiderTypeInfoXObj[] = { sSmallSpiderTypeNameXObj, sSmallSpiderBaseTypesXObj };
const void* sBaseTypesXObjSpiderSmall[]
    = { gSmallSpiderTypeInfoCJObj, 0, gSmallSpiderTypeInfoCXObj, 0, gSmallSpiderTypeInfoCXObjIntp, 0, gSmallSpiderTypeInfoXObj, 0, 0 };
const void* __RTTI__15XObjSpiderSmall[] = { sTypeNameXObjSpiderSmall, sBaseTypesXObjSpiderSmall };
}

extern "C" {
extern const char sTypeNameXObjSpider[12]      = "XObjSpider";
extern const char sSpiderTypeNameCXObjIntp[12] = "CXObjIntp";
extern const char sSpiderTypeNameCJObj[]       = "CJObj";
extern const char sSpiderTypeNameCXObj[]       = "CXObj";
extern const char sSpiderTypeNameXObj[]        = "XObj";

const void* gSpiderTypeInfoCJObj[]      = { sSpiderTypeNameCJObj, 0 };
const void* sSpiderBaseTypesCXObj[]     = { gSpiderTypeInfoCJObj, 0, 0 };
const void* gSpiderTypeInfoCXObj[]      = { sSpiderTypeNameCXObj, sSpiderBaseTypesCXObj };
const void* sSpiderBaseTypesCXObjIntp[] = { gSpiderTypeInfoCJObj, 0, gSpiderTypeInfoCXObj, 0, 0 };
const void* gSpiderTypeInfoCXObjIntp[]  = { sSpiderTypeNameCXObjIntp, sSpiderBaseTypesCXObjIntp };
const void* sSpiderBaseTypesXObj[]      = { gSpiderTypeInfoCJObj, 0, gSpiderTypeInfoCXObj, 0, gSpiderTypeInfoCXObjIntp, 0, 0 };
const void* gSpiderTypeInfoXObj[]       = { sSpiderTypeNameXObj, sSpiderBaseTypesXObj };
const void* sBaseTypesXObjSpider[]
    = { gSpiderTypeInfoCJObj, 0, gSpiderTypeInfoCXObj, 0, gSpiderTypeInfoCXObjIntp, 0, gSpiderTypeInfoXObj, 0, 0 };
const void* __RTTI__10XObjSpider[] = { sTypeNameXObjSpider, sBaseTypesXObjSpider };
}

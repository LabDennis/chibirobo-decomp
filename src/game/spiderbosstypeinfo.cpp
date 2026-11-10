extern "C" {
extern const char sTypeNameXObjSpiderBoss[16];
extern const char sSpiderBossTypeNameCXObjIntp[12];
extern const char sSpiderBossTypeNameCJObj[] = "CJObj";
extern const char sSpiderBossTypeNameCXObj[] = "CXObj";
extern const char sSpiderBossTypeNameXObj[]  = "XObj";

const void* gSpiderBossTypeInfoCJObj[]      = { sSpiderBossTypeNameCJObj, 0 };
const void* sSpiderBossBaseTypesCXObj[]     = { gSpiderBossTypeInfoCJObj, 0, 0 };
const void* gSpiderBossTypeInfoCXObj[]      = { sSpiderBossTypeNameCXObj, sSpiderBossBaseTypesCXObj };
const void* sSpiderBossBaseTypesCXObjIntp[] = { gSpiderBossTypeInfoCJObj, 0, gSpiderBossTypeInfoCXObj, 0, 0 };
const void* gSpiderBossTypeInfoCXObjIntp[]  = { sSpiderBossTypeNameCXObjIntp, sSpiderBossBaseTypesCXObjIntp };
const void* sSpiderBossBaseTypesXObj[] = { gSpiderBossTypeInfoCJObj, 0, gSpiderBossTypeInfoCXObj, 0, gSpiderBossTypeInfoCXObjIntp, 0, 0 };
const void* gSpiderBossTypeInfoXObj[]  = { sSpiderBossTypeNameXObj, sSpiderBossBaseTypesXObj };
const void* sBaseTypesXObjSpiderBoss[]
    = { gSpiderBossTypeInfoCJObj, 0, gSpiderBossTypeInfoCXObj, 0, gSpiderBossTypeInfoCXObjIntp, 0, gSpiderBossTypeInfoXObj, 0, 0 };
const void* gTypeInfoXObjSpiderBoss[] = { sTypeNameXObjSpiderBoss, sBaseTypesXObjSpiderBoss };
}

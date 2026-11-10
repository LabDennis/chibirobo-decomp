const double sTrigonSignedConversionBias   = 4503601774854144.0;
const double sTrigonUnsignedConversionBias = 4503599627370496.0;
const float sTrigonZero                    = 0.0f;
#if defined(VERSION_GGTP01)
#define ALIGN_PAD ""
#else
#define ALIGN_PAD "0"
const unsigned int sTrigonDoubleAlignment = 0x30303030;
#endif
const double sTrigonSqrtHalf       = 0.5;
const double sTrigonSqrtThree      = 3.0;
const double sTrigonSqrtZero       = 0.0;
const float sTrigonHalfPi          = 1.5707964f;
const float sTrigonPi              = 3.1415927f;
const char sTrigonJObjHeader[8]    = "jobj.h\0" ALIGN_PAD;
const char sTrigonJObjCondition[8] = "jobj\0" ALIGN_PAD ALIGN_PAD ALIGN_PAD;
const float sTrigonOne             = 1.0f;
const float sTrigonVertexCount     = 3.0f;
#if defined(VERSION_GGTP01)
const char sTrigonAllocationTag[] = "(Tri)";
#else
const char sTrigonAllocationTag[8] = "(Tri)\0"
                                     "00";
#endif

const char sTrigonAllocationFormat[]   = "(tri:%4d * %2d)";
const char sTypeName_trigonChecker[16] = "_trigonChecker\0" ALIGN_PAD;
#if defined(VERSION_GGTP01)
const char sTypeNameTRIGON_INFO[] = "TRIGON_INFO";
#else
const char sTypeNameTRIGON_INFO[16] = "TRIGON_INFO\0"
                                      "0000";
#endif

extern const void* sBaseTypes_trigonChecker[];
const void* __RTTI__11TRIGON_INFO[]    = { sTypeNameTRIGON_INFO, 0 };
const void* __RTTI__14_trigonChecker[] = { sTypeName_trigonChecker, sBaseTypes_trigonChecker };

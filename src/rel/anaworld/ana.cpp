#include "rel/ana.h"
#include "rel/anaobj.h"

extern "C" {
extern const char sAnaJObjHeader[]          = "jobj.h";
extern const char sAnaJObjExpression[]      = "jobj";
extern const char sAnaJObjEulerExpression[] = "!(jobj->flags & JOBJ_USE_QUATERNION)";
#if !defined(VERSION_PAL)
extern const char sAnaPlacementFormat[]  = "Ana::%4.2f, %4.2f, %4.2f\n";
extern const char sAnaPositionFormat[]   = "%4.2f %4.2f %4.2f\n";
extern const char sAnaIdPositionFormat[] = "id=%d,%4.2f,%4.2f,%4.2f\n";
#endif
extern const float sAnaMinimumSpacing        = 5.0f;
extern const float sAnaDegreesPerRadian      = 57.295776f;
extern const double sAnaSignedConversionBias = 4503601774854144.0;
extern const float sAnaPositionScale         = 100.0f;
}

s32 activeAnaNum;
Ana anas[ANA_NUM];
AnaNull anaNull;
AnaObj* tutiAnaObj;
AnaObj* tutiHutaObj;
AnaObj* flowerObjs[3];

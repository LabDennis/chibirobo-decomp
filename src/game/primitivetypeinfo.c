#include "game/gmath.h"

extern const char sTypeNameCPrimitive[];
extern const void* sBaseTypesCSprite[];
extern const void* sBaseTypesCTile[];
extern const void* sBaseTypesCCircle[];

const char sTypeNameCSprite[]                 = "CSprite";
const float sPrimitiveOne                     = 1.0f;
const float sPrimitiveZero                    = 0.0f;
const float sPrimitiveFullTurn                = TWO_PI;
const double sPrimitiveUnsignedConversionBias = 4503599627370496.0;
const double sPrimitiveSignedConversionBias   = 4503601774854144.0;
const char sTypeNameCTile[]                   = "CTile";
const char sTypeNameCCircle[]                 = "CCircle";

const void* __RTTI__10CPrimitive[] = { sTypeNameCPrimitive, 0 };
const void* __RTTI__7CSprite[]     = { sTypeNameCSprite, sBaseTypesCSprite };
const void* __RTTI__5CTile[]       = { sTypeNameCTile, sBaseTypesCTile };
const void* __RTTI__7CCircle[]     = { sTypeNameCCircle, sBaseTypesCCircle };

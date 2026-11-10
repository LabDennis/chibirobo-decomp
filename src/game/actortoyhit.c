#include "types.h"

const double sActorToySignedConversionBias = 4503601774854144.0;
const f32 sActorToyHitReverseAngle         = 3.1415927f;
const f32 sActorToyStrongHitImpulse        = 6.8f;
const f32 sActorToyNormalHitImpulse        = 4.8f;
const f32 sActorToyRecoveryDeceleration    = 0.04f;
const f32 sActorToyReturnSpeed             = 0.5f;

#if defined(VERSION_GGTP01)
const char sActorToyHitPadding[4] = { 0 };
#else
const char sActorToyHitPadding[4] = { '0', '0', '0', '0' };
#endif

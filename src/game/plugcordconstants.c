#include "types.h"

const f64 sPlugJointRecipSqrtHalf  = 0.5;
const f64 sPlugJointRecipSqrtThree = 3.0;
const f64 sPlugJointSqrtZero       = 0.0;
const f32 sPlugCordPitchHalfPi     = 1.5707964f;
const f32 sPlugPlayerJointOffsetZ  = -0.4f;

const f32 sPlugCordDefaultWidth           = 0.2f;
const f32 sPlugCordZero                   = 0.0f;
const f64 sPlugCordRecipSqrtHalf          = 0.5;
const f64 sPlugCordRecipSqrtThree         = 3.0;
const f64 sPlugCordSqrtZero               = 0.0;
const f32 sPlugCordFloorUnitsPerWorldUnit = 100.0f;
const f32 sPlugCordDefaultVerticalMargin  = 1.0f;
const f32 sPlugCordObjectCullDistance     = 150.0f;
const f32 sPlugCordDegreesToRadians       = 0.017453292f;
const f64 sPlugCordSignedConversionBias   = 4503601774854144.0;

s8 sPlugCordVerticalMarginInitialized;
f32 sPlugCordVerticalMargin;

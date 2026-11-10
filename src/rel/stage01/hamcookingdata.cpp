#include "dolphin/mtx.h"

extern "C" {
extern const f32 sHamDegreesToRadians  = 0.017453292f;
extern const f64 sHamCommandSignedBias = 4503601774854144.0;
extern const f32 sHamSlideFactor       = 0.2f;
extern const f32 sHamSoundPosition1X   = -92.52f;
extern const f32 sHamSoundPosition1Y   = 75.87f;
extern const f32 sHamSoundPosition1Z   = -165.98f;
extern const f32 sHamSoundPosition2X   = -281.1f;
extern const f32 sHamSoundPosition2Y   = 76.14f;
extern const f32 sHamSoundPosition2Z   = 110.23f;

extern const Vec sHamCookLevelRotation ATTRIBUTE_ALIGN(8) = { 0.0f, 0.0f, 0.0f };
extern const Vec sHamOvercookRotation                     = { 0.0f, 0.0f, 0.0f };
extern const Vec sHamBurnRotation                         = { 0.0f, 0.0f, 0.0f };
extern const s32 sHamCookingJoint                         = 0;

extern const f32 sHamResultMinX              = -295.0f;
extern const f64 sHamResultMaxX              = -225.0;
extern const f32 sHamResultMinZ              = 10.0f;
extern const f32 sHamResultMaxZ              = 74.0f;
extern const f32 sHamRatePercent             = 100.0f;
extern const f64 sHamRateSignedBias          = 4503601774854144.0;
extern const f32 sHamGasPlateMinX            = -289.0f;
extern const f32 sHamGasPlateMaxX            = -239.0f;
extern const f32 sHamGasPlateMinZ            = 21.4f;
extern const f32 sHamGasPlateMaxZ            = 62.6f;
extern const f32 sHamZero                    = 0.0f;
extern const f64 sHamSlideHitAngle           = 45.0;
extern const f32 sHamSlideHalf               = 0.5f;
extern const f32 sHamPickupSearchRadius      = 7.0f;
extern const f64 sHamFacingHalfAngle         = (f64)0.7853982f;
extern const f32 sHamFacingDistance          = 15.0f;
extern const f32 sHamDefaultRate             = 1.0f;
extern const f32 sHamLandingTolerance        = 0.001f;
extern const char sStage01PathAobjH[]        = "aobj.h";
extern const char sHamAnimCondition[]        = "aobj";
extern const f32 sHamStackLevelHeight        = 2.0f;
extern const f32 sHamReturnSlideSpeed        = 5.0f;
extern const f32 sHamReturnSlideStep         = 0.2f;
extern const f32 sHamStackDetachDistance     = 8.0f;
extern const char sHamJointHeader[]          = "jobj.h";
extern const char sHamJointCondition[]       = "jobj";
extern const f32 sHamHalfTurn                = 3.1415927f;
extern const f32 sHamStackLoweringStep       = 0.1f;
extern const f32 sHamRespawnX                = -231.8f;
extern const f32 sHamRespawnY                = 77.5f;
extern const f32 sHamRespawnZ                = 50.0f;
extern const f32 sHamRespawnDegreesToRadians = 0.017453292f;
extern const f32 sHamRespawnMinSpeed         = 30.0f;
extern const f64 sHamFlipTimeUnsignedBias    = 4503599627370496.0;
extern const f32 sHamEffectHeight            = 2.5f;
extern const f32 sHamGasPlateCenterX         = -263.9f;
extern const f32 sHamGasPlateCenterZ         = 42.0f;
extern const f32 sHamResetHeight             = 78.0f;
}

#include "Dolphin/vec.h"

extern "C" {
Vec gToyPlacementPosition;
Vec gToyPlacementRotation;
Vec gToyJointScaleTemplate;
Vec gToyFirstJointRestPosition;
Vec gToySecondJointRestPosition;
f32 gToy6FSpinAngle;
s8 gToy6FSpinAngleInitialized;
f32 gToyJointExtensionProgress;
s8 gToyJointExtensionProgressInitialized;
extern const double sActorToySqrtHalf               = 0.5;
extern const double sActorToySqrtThree              = 3.0;
extern const double sActorToySqrtZero               = 0.0;
extern const double sActorToyUnsignedConversionBias = 4503599627370496.0;
extern const f32 sActorToy25ECarryOffsetY           = 1.1f;
extern const f32 sActorToy25ECarryOffsetZ           = 1.4f;
extern const f32 sActorToy26DSpinStep               = 0.2f;
extern const f32 sActorToyFullTurn                  = 6.2831855f;
extern const f32 sActorToyJointExtensionStep        = 0.16f;
extern const f32 sActorToyJointExtensionDistance    = 16.0f;
extern const f32 sActorToy6FSpinStep                = 0.4f;
extern const f32 sActorToyCarryMovementScale        = 0.1f;
extern const f32 sActorToyHeldPropNegativeOffset    = -2.0f;
extern const f32 sActorToyHeldPropForwardOffset     = 8.0f;
extern const f32 sActorToy3BHeldPropOffsetZ         = -10.0f;
extern const f32 sActorToyPieceMountOffsetY         = -6.5f;
extern const f32 sActorToyPieceMountYaw             = 0.19634955f;
extern const f32 sActorToy21EHeldPropOffsetY        = -4.5f;
extern const f32 sActorToy0FTargetScale             = 20.0f;
extern const f32 sActorToy117And159TargetScale      = 3.0f;
extern const f32 sActorToy26CAnd485TargetScale      = 4.0f;
extern const f32 sActorToyScaleApproachFraction     = 0.25f;
extern const f32 sActorToyMinimumScaleStep          = 0.01f;
extern const f32 sActorToy157TargetDistance         = 14.0f;
extern const f32 sActorToyDegreesToRadians          = 0.017453292f;
}

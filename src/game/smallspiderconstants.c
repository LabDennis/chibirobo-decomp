const float sSmallSpiderCatchInitialA98 = 40.0f;
const float sSmallSpiderScalarZero      = 0.0f;
// The US and Japanese objects fill unused bytes in the assertion strings with ASCII '0'.
#if defined(VERSION_GGTP01)
const char sSmallSpiderJObjAssertFile[8]      = "jobj.h";
const char sSmallSpiderJObjAssertCondition[8] = "jobj";
const char sSmallSpiderAObjAssertFile[8]      = "aobj.h";
const char sSmallSpiderAObjAssertCondition[8] = "aobj";
#else
const char sSmallSpiderJObjAssertFile[8]      = "jobj.h\0"
                                                "0";
const char sSmallSpiderJObjAssertCondition[8] = "jobj\0"
                                                "000";
const char sSmallSpiderAObjAssertFile[8]      = "aobj.h\0"
                                                "0";
const char sSmallSpiderAObjAssertCondition[8] = "aobj\0"
                                                "000";
#endif
const double sSmallSpiderUnsignedConversionBias = 4503599627370496.0;
const float sSmallSpiderScalarOne               = 1.0f;
const float sSmallSpiderScalarTen               = 10.0f;
const float sSmallSpiderScalarHalf              = 0.5f;
const float sSmallSpiderAreaVerticalTolerance   = 20.0f;
const double sSmallSpiderSqrtHalf               = 0.5;
const double sSmallSpiderSqrtThree              = 3.0;
const double sSmallSpiderSqrtZero               = 0.0;
const float sSmallSpiderContactRadiusAdjustment = 1.5f;
const float sSmallSpiderTerminalFallSpeed       = -2.3f;
const float sSmallSpiderFallAcceleration        = 0.36f;
const float sSmallSpiderScalarTwo               = 2.0f;
const float sSmallSpiderHomeHeightTolerance     = 12.0f;
const float sSmallSpiderHundredthsDivisor       = 100.0f;
const float sSmallSpiderPi                      = 3.1415927f;
const float sSmallSpiderScaleStep               = 0.01f;
const double sSmallSpiderSignedConversionBias   = 4503601774854144.0;
const float sSmallSpiderApproachDistance        = 48.0f;
const float sSmallSpiderInitialMotion984        = 0.3f;
const float sSmallSpiderTargetCheckDistance     = 64.0f;
const float sSmallSpiderSlowFallAcceleration    = 0.08f;
const float sSmallSpiderScalarOnePointOne       = 1.1f;
const float sSmallSpiderJumpSpeed               = 4.0f;
const float sSmallSpiderDegreesToRadians        = 0.0174532925f;
const float sSmallSpiderEight                   = 8.0f;
const float sSmallSpiderPointFour               = 0.4f;
const float sSmallSpiderWebTriggerDistance      = 30.0f;
const float sSmallSpiderScalarThree             = 3.0f;
const char sSmallSpiderStage07[8]               = "stage07";
const float sSmallSpiderDamageFlashRed          = 128.0f;
#if !defined(VERSION_GGTP01)
const float sSmallSpiderFrameRate60 = 60.0f;
#endif
const float sSmallSpiderColorCenter              = 160.0f;
const float sSmallSpiderThirtyDegrees            = 0.5235988f;
const char sSmallSpiderStage00[8]                = "stage00";
const char sSmallSpiderStage19[8]                = "stage19";
const float sSmallSpiderDefaultRadius            = 16.0f;
const float sSmallSpiderActiveRadius             = 1.3333334f;
const float sSmallSpiderTargetRadius             = 96.0f;
const float sSmallSpiderStage07AreaX             = -102.85f;
const float sSmallSpiderStage07AreaZ             = -335.27f;
const float sSmallSpiderStage07AreaWidth         = 150.0f;
const float sSmallSpiderStage07AreaDepth         = 134.0f;
const char sSmallSpiderStage01[8]                = "stage01";
const float sSmallSpiderStage01HeightThreshold   = 70.0f;
const float sSmallSpiderStage01UpperAreaX        = -306.38f;
const float sSmallSpiderStage01UpperAreaY        = 75.84f;
const float sSmallSpiderStage01UpperAreaZ        = -33.31f;
const float sSmallSpiderStage01UpperAreaWidth    = 80.0f;
const float sSmallSpiderStage01LowerAreaX        = -150.97f;
const float sSmallSpiderStage01LowerAreaZ        = -100.32f;
const char sSmallSpiderStage02[8]                = "stage02";
const float sSmallSpiderStage02PositiveAreaX     = 25.65f;
const float sSmallSpiderStage02PositiveAreaY     = 0.31f;
const float sSmallSpiderStage02PositiveAreaZ     = 2.28f;
const float sSmallSpiderStage02PositiveAreaWidth = 166.0f;
const float sSmallSpiderStage02PositiveAreaDepth = 197.0f;
const float sSmallSpiderStage02NegativeAreaX     = -184.75f;
const float sSmallSpiderStage02NegativeAreaZ     = -219.72f;
const float sSmallSpiderStage02NegativeAreaWidth = 138.0f;
const float sSmallSpiderStage02NegativeAreaDepth = 171.0f;
const float sSmallSpiderDefaultAreaX             = 2.97f;
const float sSmallSpiderDefaultAreaZ             = 47.15f;
const float sSmallSpiderDefaultAreaExtent        = 180.0f;
const float sSmallSpiderInitial98C               = 0.1f;

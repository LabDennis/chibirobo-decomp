#include "Version.h"

extern "C" {
extern const float sSpiderBossPlacementUnitsPerWorldUnit = 100.0f;
extern const double sSpiderBossSignedIntConversionBias   = 4503601774854144.0;
extern const float sSpiderBossLowBatteryThreshold        = 30.0f;
extern const float sSpiderBossContactDrainAmount         = 4.0f;
extern const float sSpiderBossDefaultRadius              = 28.0f;
extern const float sSpiderBossDamageFadeMaxRed           = 128.0f;
#if !defined(VERSION_PAL)
extern const float sSpiderBossDamageFadeFrameRate = 60.0f;
#endif
extern const float sSpiderBossDegreesToRadians = 0.01745329238474369f;
extern const float sSpiderBossInitialTurnSpeed = 0.1f;
}

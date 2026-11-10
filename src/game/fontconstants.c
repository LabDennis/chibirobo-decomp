// The US and JP compilers padded these diagnostic strings with ASCII zeroes.
#if defined(VERSION_GGTP01)
#define PAD "\0"
#else
#define PAD "0"
#endif

char sFontInitFunctionName[8]            = "Init\0" PAD PAD PAD;
const float sFontZero                    = 0.0f;
const float sFontOne                     = 1.0f;
const double sFontSignedConversionBias   = 4503601774854144.0;
const char sFontPanicFormat[8]           = "%s%s\0" PAD PAD PAD;
const char sFontTempAllocationLabel[8]   = "(temp)\0" PAD;
const double sFontUnsignedConversionBias = 4503599627370496.0;

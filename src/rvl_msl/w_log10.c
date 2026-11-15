#include "PowerPC_EABI_Support/MSL_C/MSL_Common_Embedded/Math/fdlibm.h"

double log10(double x) {
    return __ieee754_log10(x);
}

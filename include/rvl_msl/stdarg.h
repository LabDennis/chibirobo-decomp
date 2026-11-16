#ifndef _RVL_MSL_STDARG_H
#define _RVL_MSL_STDARG_H

// The Wii MSL's stdarg.h: va_list is the named __va_list_struct, which is what C++ names of the Wii
// libraries carry (VPanic__Q24nw4r2dbFPCciPCcP16__va_list_structb). include/stl/stdarg.h, the GameCube
// header, has an unnamed struct there; its guard is set so that it stays out.
#define _STDARG_H_
#include "msl/stdarg.h"

#endif

#ifndef _RVL_MSL_STDARG_H
#define _RVL_MSL_STDARG_H

// The Wii MSL's stdarg.h: va_list is the named __va_list_struct, which is what C++ names of the Wii
// libraries carry (VPanic__Q24nw4r2dbFPCciPCcP16__va_list_structb). include/stl/stdarg.h, the GameCube
// header, has an unnamed struct there. Only one of them may define va_list in a unit: whichever comes
// first sets _STDARG_H_ and keeps the other out. Units built with -i include/rvl_msl ahead of the
// others (the Wii libraries) get this one; game units that read include/stl first keep that one.
#ifndef _STDARG_H_
#define _STDARG_H_
#include "msl/stdarg.h"
#endif

#endif

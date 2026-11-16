#ifndef REVOLUTION_OS_H
#define REVOLUTION_OS_H

// The Wii register map and OSAlarm come first: both set the guards of the GameCube headers.
#include "revolution/hw_regs.h"
#include "revolution/os/OSAlarm.h"
#include "Dolphin/os.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void OSVReport(const char* msg, va_list list);

#ifdef __cplusplus
}
#endif

#ifndef ASSERTLINE
#ifdef _DEBUG
#define ASSERTLINE(exp, line) (void)((exp) || (OSPanic(__FILE__, line, "Failed assertion " #exp), 0))
#else
#define ASSERTLINE(exp, line) ((void)0)
#endif
#endif

#endif

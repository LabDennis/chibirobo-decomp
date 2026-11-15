#ifndef MSL_SIGNAL_H
#define MSL_SIGNAL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void exit(int);

typedef void (*sig_func)(int sig);

int raise(int sig);

#ifdef __cplusplus
}
#endif

#endif

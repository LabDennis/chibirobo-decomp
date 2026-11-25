#ifndef _DOLPHIN_OS_OSEXCEPTION_H
#define _DOLPHIN_OS_OSEXCEPTION_H

#include "types.h"
#include "Dolphin/OS/OSContext.h"
#include "Dolphin/OS/OSUtil.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

//////// EXCEPTION TYPES /////////
// Useful typedef for exceptions.
typedef u8 __OSException;

// Exception handler function type.
typedef void (*__OSExceptionHandler)(__OSException exception, OSContext* context);

// Exception handling functions.
__OSExceptionHandler __OSSetExceptionHandler(__OSException exception, __OSExceptionHandler handler);
__OSExceptionHandler __OSGetExceptionHandler(__OSException exception);

//////////////////////////////////

/////////// EXCEPTIONS ///////////
// Exception codes.
#define __OS_EXCEPTION_SYSTEM_RESET       0
#define __OS_EXCEPTION_MACHINE_CHECK      1
#define __OS_EXCEPTION_DSI                2
#define __OS_EXCEPTION_ISI                3
#define __OS_EXCEPTION_EXTERNAL_INTERRUPT 4
#define __OS_EXCEPTION_ALIGNMENT          5
#define __OS_EXCEPTION_PROGRAM            6
#define __OS_EXCEPTION_FLOATING_POINT     7
#define __OS_EXCEPTION_DECREMENTER        8
#define __OS_EXCEPTION_SYSTEM_CALL        9
#define __OS_EXCEPTION_TRACE              10
#define __OS_EXCEPTION_PERFORMACE_MONITOR 11
#define __OS_EXCEPTION_BREAKPOINT         12
#define __OS_EXCEPTION_SYSTEM_INTERRUPT   13
#define __OS_EXCEPTION_THERMAL_INTERRUPT  14
#define __OS_EXCEPTION_MAX                (__OS_EXCEPTION_THERMAL_INTERRUPT + 1)

//////////////////////////////////

// Moved from OSContext.h due to include looping.
void OSSwitchFPUContext(__OSException exception, OSContext* context);

//////////////////////////////////

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

#endif

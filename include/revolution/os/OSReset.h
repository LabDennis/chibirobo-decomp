#ifndef _REVOLUTION_OS_OSRESET_H
#define _REVOLUTION_OS_OSRESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

// Wii variant of the reset hooks: the Revolution SDK calls them shutdown functions,
// passes the event along and keeps them in a doubly linked list.
typedef BOOL (*OSShutdownFunction)(BOOL final, u32 event);
typedef struct OSShutdownFunctionInfo OSShutdownFunctionInfo;

struct OSShutdownFunctionInfo {
	OSShutdownFunction func;      // _00
	u32 priority;                 // _04
	OSShutdownFunctionInfo* next; // _08
	OSShutdownFunctionInfo* prev; // _0C
};

void OSRegisterShutdownFunction(OSShutdownFunctionInfo* info);

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif

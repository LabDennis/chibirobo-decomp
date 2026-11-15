#ifndef _REVOLUTION_OS_OSEXEC_H
#define _REVOLUTION_OS_OSEXEC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

typedef struct OSExecParams {
	BOOL valid;          // _00
	u32 restartCode;     // _04
	u32 bootDol;         // _08
	void* regionStart;   // _0C
	void* regionEnd;     // _10
	BOOL argsUseDefault; // _14
	void* argsAddr;      // _18
} OSExecParams;

extern OSExecParams __OSRebootParams;

void __OSGetExecParams(OSExecParams* params);
void __OSSetExecParams(const OSExecParams* params, OSExecParams* addr);
void __OSBootDolSimple(u32 doloffset, u32 restartCode, void* regionStart, void* regionEnd, BOOL argsUseDefault, s32 argc, char** argv);
void __OSBootDol(u32 doloffset, u32 restartCode, const char** argv);

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif

// NdevExi2A, the Wii's NDEV debugger link over EXI channel 2, after xenoblade's libs/NdevExi2A headers
// (DebuggerDriver.h, exi2.h).
#ifndef _REVOLUTION_NDEVEXI2A_H_
#define _REVOLUTION_NDEVEXI2A_H_

#include "revolution/hw_regs.h"
#include "revolution/os.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// exi2.c
BOOL __EXI2Imm(void* mem, s32 size, u32 type);
void __DBEXIInit(void);
BOOL __DBEXIReadReg(u32 cmd, void* mem, s32 size);
BOOL __DBEXIWriteReg(u32 cmd, const void* mem, s32 size);
BOOL __DBEXIReadRam(u32 cmd, void* mem, s32 size);
BOOL __DBEXIWriteRam(u32 cmd, const void* mem, s32 size);

// DebuggerDriver.c
// the monitor callback gets a full int where the interrupt handlers get an __OSInterrupt (DBInitComm's caller is TRK)
typedef void (*MTRCallbackType)(int, OSContext*);

void DBInitComm(u8** flagOut, MTRCallbackType handler);
void DBInitInterrupts(void);
u32 DBQueryData(void);
BOOL DBRead(void* dst, u32 size);
BOOL DBWrite(const void* src, u32 size);
void DBOpen(void);
void DBClose(void);

#ifdef __cplusplus
}
#endif

#endif

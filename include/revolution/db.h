#ifndef _REVOLUTION_DB_H
#define _REVOLUTION_DB_H

// Wii variant of db.h: the Revolution SDK shares __DBInterface and DBVerbose
// across the OS units, so they are extern here rather than TU-local statics.
// It also defines the Dolphin and sdk guard macros so the GameCube variants
// are skipped when this header is included first in a Wii unit.
#ifndef _DOLPHIN_DB_H
#define _DOLPHIN_DB_H
#endif
#ifndef _DOLPHIN_DB_H_
#define _DOLPHIN_DB_H_
#endif

#include "types.h"
#include "Dolphin/os.h"
#include "Dolphin/AmcExi2Stubs.h"

#define ExceptionHookDestination 0x80000048
#define IsDebuggerPresent        0x80000040

// static int __DBInterface;

struct DBInterface {
	u8 filler0[4];
	u32 unk4;
};

extern struct DBInterface* __DBInterface;
extern int DBVerbose;

void DBInit(void);
void DBInitComm(vu8**, AmcEXICallback); // possibly not this type, but some similar construction

void DBInitInterrupts();
u32 DBQueryData();
BOOL DBRead(void*, u32);
BOOL DBWrite(const void*, u32);
void DBOpen();
void DBClose();

static void __DBExceptionDestination(void);
void DBPrintf(const char* format, ...);

#endif

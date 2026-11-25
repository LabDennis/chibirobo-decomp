#ifndef _DOLPHIN_DB_H
#define _DOLPHIN_DB_H

#include "types.h"
#include "Dolphin/os.h"
#include "Dolphin/AmcExi2Stubs.h"

void DBInit(void);
void DBInitComm(vu8** inputFlag, AmcEXICallback callback);

void DBInitInterrupts();
u32 DBQueryData();
BOOL DBRead(void*, u32);
BOOL DBWrite(const void*, u32);
void DBOpen();
void DBClose();

void DBPrintf(const char* format, ...);

#endif

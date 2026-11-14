#ifndef REVOLUTION_NAND_INTERNAL_H
#define REVOLUTION_NAND_INTERNAL_H
#include "types.h"
BEGIN_SCOPE_EXTERN_C
BOOL nandIsInitialized(void);
s32 nandConvertErrorCode(s32 result);
const char* nandGetHomeDir(void);
void nandGenerateAbsPath(char* absolute, const char* relative);
END_SCOPE_EXTERN_C
#endif

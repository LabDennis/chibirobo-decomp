#ifndef REVOLUTION_NAND_INTERNAL_H
#define REVOLUTION_NAND_INTERNAL_H
#include "types.h"

#include "revolution/fs.h"
#include "revolution/nand.h"

BEGIN_SCOPE_EXTERN_C

// NANDCore.c
BOOL nandIsInitialized(void);
void nandRemoveTailToken(char* newp, const char* oldp);
void nandGetHeadToken(char* token, char* newp, const char* oldp);
void nandGetRelativeName(char* name, const char* path);
void nandConvertPath(char* abspath, const char* wd, const char* relpath);
BOOL nandIsPrivatePath(const char* path);
BOOL nandIsUnderPrivatePath(const char* path);
void nandReportErrorCode(ISFSError err);
s32 nandConvertErrorCode(ISFSError err);
void nandGenerateAbsPath(char* absPath, const char* path);
void nandGetParentDirectory(char* parentDir, const char* absPath);
const char* nandGetHomeDir(void);
void nandCallback(ISFSError result, void* ctxt);

// NANDLogging.c
typedef void (*NANDLoggingCallback)(BOOL result);
BOOL NANDLoggingAddMessageAsync(NANDLoggingCallback cb, const char* fmt, ...);

END_SCOPE_EXTERN_C

#endif

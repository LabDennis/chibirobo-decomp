#ifndef _DOLPHIN_ARC_H
#define _DOLPHIN_ARC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

// sdk ARC archive reader, a file system image already sitting in memory

typedef struct ARCHeader {
	u32 magic;      // _00
	s32 fstStart;   // _04
	s32 fstSize;    // _08
	s32 fileStart;  // _0C
	s32 reserve[4]; // _10
} ARCHeader;

typedef struct ARCHandle {
	void* archiveStartAddr; // _00
	void* FSTStart;         // _04
	void* fileStart;        // _08
	u32 entryNum;           // _0C
	char* FSTStringStart;   // _10
	u32 FSTLength;          // _14
	u32 currDir;            // _18
} ARCHandle;

typedef struct ARCFileInfo {
	ARCHandle* handle; // _00
	u32 startOffset;   // _04
	u32 length;        // _08
} ARCFileInfo;

BOOL ARCInitHandle(void* arcStart, ARCHandle* handle);
BOOL ARCFastOpen(ARCHandle* handle, s32 entrynum, ARCFileInfo* af);
s32 ARCConvertPathToEntrynum(ARCHandle* handle, const char* pathPtr);
u32 ARCGetStartOffset(ARCFileInfo* af);
u32 ARCGetLength(ARCFileInfo* af);
BOOL ARCClose(ARCFileInfo* af);

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

#endif

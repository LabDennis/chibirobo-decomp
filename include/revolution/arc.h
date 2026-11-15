#ifndef _REVOLUTION_ARC_H
#define _REVOLUTION_ARC_H

// Wii variant of the ARC API: the Revolution SDK adds the directory traversal
// functions and the path-based ARCOpen.
#include "Dolphin/arc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ARCDir {
	ARCHandle* handle; // _00
	u32 entryNum;      // _04
	u32 location;      // _08
	u32 next;          // _0C
} ARCDir;

typedef struct ARCDirEntry {
	ARCHandle* handle; // _00
	u32 entryNum;      // _04
	BOOL isDir;        // _08
	char* name;        // _0C
} ARCDirEntry;

BOOL ARCOpen(ARCHandle* handle, const char* fileName, ARCFileInfo* af);
void* ARCGetStartAddrInMem(ARCFileInfo* af);
BOOL ARCChangeDir(ARCHandle* handle, const char* dirName);
BOOL ARCOpenDir(ARCHandle* handle, const char* dirName, ARCDir* dir);
BOOL ARCReadDir(ARCDir* dir, ARCDirEntry* dirent);
BOOL ARCCloseDir(ARCDir* dir);

#ifdef __cplusplus
};
#endif

#endif


#include "types.h"
#include "Dolphin/os.h"
#include "revolution/arc.h"

// sdk ARC archive reader, the "arc.c" assert string and the 0x55AA382D magic
// are what names ARCInitHandle, ARCFastOpen, ARCConvertPathToEntrynum and the rest

#include "rvl_msl/ctype.h"

typedef struct FSTEntry {
	u32 isDirAndStringOff; // _00
	u32 parentOrPosition;  // _04
	u32 nextEntryOrLength; // _08
} FSTEntry;

#define DARCH_MAGIC 0x55AA382D

#define entryIsDir(i)   (((FSTEntries[i].isDirAndStringOff & 0xFF000000) == 0) ? FALSE : TRUE)
#define stringOff(i)    (FSTEntries[i].isDirAndStringOff & ~0xFF000000)
#define parentDir(i)    (FSTEntries[i].parentOrPosition)
#define nextDir(i)      (FSTEntries[i].nextEntryOrLength)
#define filePosition(i) (FSTEntries[i].parentOrPosition)
#define fileLength(i)   (FSTEntries[i].nextEntryOrLength)

/**
 * @note Address: 0x8018D0D0
 * @note Size: 0xA4
 */
BOOL ARCInitHandle(void* arcStart, ARCHandle* handle)
{
	FSTEntry* FSTEntries;
	ARCHeader* arcHeader = (ARCHeader*)arcStart;

	if (arcHeader->magic != DARCH_MAGIC) {
		OSErrorLine(74, "ARCInitHandle: bad archive format");
	}

	handle->archiveStartAddr = arcStart;
	handle->FSTStart = FSTEntries = (FSTEntry*)((u32)arcStart + arcHeader->fstStart);
	handle->fileStart             = (void*)((u32)arcStart + arcHeader->fileStart);
	handle->entryNum              = nextDir(0);
	handle->FSTStringStart        = (char*)&FSTEntries[handle->entryNum];
	handle->FSTLength             = (u32)arcHeader->fstSize;
	handle->currDir               = 0;

	return TRUE;
}

static BOOL ARCGetCurrentDir(ARCHandle* handle, char* path, u32 maxlen);

BOOL ARCOpen(ARCHandle* handle, const char* fileName, ARCFileInfo* af)
{
	s32 entrynum;
	FSTEntry* FSTEntries = (FSTEntry*)handle->FSTStart;
	char currentDir[128];

	entrynum = ARCConvertPathToEntrynum(handle, fileName);
	if (entrynum < 0) {
		ARCGetCurrentDir(handle, currentDir, sizeof(currentDir));
		OSReport("Warning: ARCOpen(): file '%s' was not found under %s in the archive.\n", fileName, currentDir);
		return FALSE;
	}

	if (entrynum < 0 || entryIsDir(entrynum)) {
		return FALSE;
	}

	af->handle      = handle;
	af->startOffset = filePosition(entrynum);
	af->length      = fileLength(entrynum);
	return TRUE;
}

/**
 * @note Address: 0x8018D174
 * @note Size: 0x64
 */
BOOL ARCFastOpen(ARCHandle* handle, s32 entrynum, ARCFileInfo* af)
{
	FSTEntry* FSTEntries = (FSTEntry*)handle->FSTStart;

	if (entrynum < 0 || entrynum >= handle->entryNum || entryIsDir(entrynum)) {
		return FALSE;
	}

	af->handle      = handle;
	af->startOffset = filePosition(entrynum);
	af->length      = fileLength(entrynum);

	return TRUE;
}

static BOOL isSame(const char* path, const char* string)
{
	while (*string != '\0') {
		if (_tolower(*path++) != _tolower(*string++)) {
			return FALSE;
		}
	}

	if (*path == '/' || *path == '\0') {
		return TRUE;
	}

	return FALSE;
}

/**
 * @note Address: 0x8018D1D8
 * @note Size: 0x214
 */
s32 ARCConvertPathToEntrynum(ARCHandle* handle, const char* pathPtr)
{
	const char* ptr;
	char* stringPtr;
	BOOL isDir;
	u32 length;
	u32 dirLookAt;
	u32 i;
	const char* origPathPtr = pathPtr; // never read, the dvd version prints it on bad 8.3 names
	FSTEntry* FSTEntries;

	dirLookAt  = handle->currDir;
	FSTEntries = (FSTEntry*)handle->FSTStart;

	while (1) {
		if (*pathPtr == '\0') {
			return (s32)dirLookAt;
		} else if (*pathPtr == '/') {
			dirLookAt = 0;
			pathPtr++;
			continue;
		} else if (*pathPtr == '.') {
			if (*(pathPtr + 1) == '.') {
				if (*(pathPtr + 2) == '/') {
					dirLookAt = parentDir(dirLookAt);
					pathPtr += 3;
					continue;
				} else if (*(pathPtr + 2) == '\0') {
					return (s32)parentDir(dirLookAt);
				}
			} else if (*(pathPtr + 1) == '/') {
				pathPtr += 2;
				continue;
			} else if (*(pathPtr + 1) == '\0') {
				return (s32)dirLookAt;
			}
		}

		for (ptr = pathPtr; *ptr != '\0' && *ptr != '/'; ptr++)
			;

		isDir  = (*ptr == '\0') ? FALSE : TRUE;
		length = (u32)(ptr - pathPtr);

		ptr = pathPtr;

		for (i = dirLookAt + 1; i < nextDir(dirLookAt); i = entryIsDir(i) ? nextDir(i) : (i + 1)) {
		dot:
			if (entryIsDir(i) == FALSE && isDir == TRUE) {
				continue;
			}

			stringPtr = handle->FSTStringStart + stringOff(i);
			// a "." entry is stepped over without the loop's own check and increment
			if (stringPtr[0] == '.' && stringPtr[1] == '\0') {
				i++;
				goto dot;
			}

			if (isSame(ptr, stringPtr) == TRUE) {
				goto next_hier;
			}
		}

		return -1;

	next_hier:
		if (!isDir) {
			return (s32)i;
		}

		dirLookAt = i;
		pathPtr += length + 1;
	}
}

static u32 myStrncpy(char* dst, char* src, u32 maxlen)
{
	u32 i = maxlen;

	while (i > 0 && *src != 0) {
		*dst++ = *src++;
		i--;
	}

	return maxlen - i;
}

static u32 entryToPath(ARCHandle* handle, u32 entry, char* path, u32 maxlen)
{
	char* name;
	u32 loc;
	FSTEntry* FSTEntries = (FSTEntry*)handle->FSTStart;

	if (entry == 0) {
		return 0;
	}

	name = handle->FSTStringStart + stringOff(entry);
	loc  = entryToPath(handle, parentDir(entry), path, maxlen);
	if (loc == maxlen) {
		return loc;
	}
	*(path + loc++) = '/';
	loc += myStrncpy(path + loc, name, maxlen - loc);
	return loc;
}

static BOOL ARCConvertEntrynumToPath(ARCHandle* handle, s32 entrynum, char* path, u32 maxlen)
{
	u32 loc;
	FSTEntry* FSTEntries = (FSTEntry*)handle->FSTStart;

	loc = entryToPath(handle, (u32)entrynum, path, maxlen);
	if (loc == maxlen) {
		path[maxlen - 1] = '\0';
		return FALSE;
	}
	if (entryIsDir(entrynum)) {
		if (loc == maxlen - 1) {
			path[loc] = '\0';
			return FALSE;
		}
		path[loc++] = '/';
	}
	path[loc] = '\0';
	return TRUE;
}

static BOOL ARCGetCurrentDir(ARCHandle* handle, char* path, u32 maxlen)
{
	return ARCConvertEntrynumToPath(handle, (s32)handle->currDir, path, maxlen);
}

void* ARCGetStartAddrInMem(ARCFileInfo* af)
{
	return (void*)((u32)af->handle->archiveStartAddr + af->startOffset);
}

/**
 * @note Address: 0x8018D3EC
 * @note Size: 0x8
 */
u32 ARCGetStartOffset(ARCFileInfo* af)
{
	return af->startOffset;
}

/**
 * @note Address: 0x8018D3F4
 * @note Size: 0x8
 */
u32 ARCGetLength(ARCFileInfo* af)
{
	return af->length;
}

/**
 * @note Address: 0x8018D3FC
 * @note Size: 0x8
 */
BOOL ARCClose(ARCFileInfo* af)
{
	return TRUE;
}

BOOL ARCChangeDir(ARCHandle* handle, const char* dirName)
{
	s32 entry;
	FSTEntry* FSTEntries;

	entry      = ARCConvertPathToEntrynum(handle, dirName);
	FSTEntries = (FSTEntry*)handle->FSTStart;

	if (entry < 0 || !entryIsDir(entry)) {
		return FALSE;
	}

	handle->currDir = entry;
	return TRUE;
}

BOOL ARCOpenDir(ARCHandle* handle, const char* dirName, ARCDir* dir)
{
	s32 entry;
	FSTEntry* FSTEntries;

	entry      = ARCConvertPathToEntrynum(handle, dirName);
	FSTEntries = (FSTEntry*)handle->FSTStart;

	if (entry < 0 || !entryIsDir(entry)) {
		return FALSE;
	}

	dir->handle   = handle;
	dir->entryNum = entry;
	dir->location = entry + 1;
	dir->next     = nextDir(entry);
	return TRUE;
}

BOOL ARCReadDir(ARCDir* dir, ARCDirEntry* dirent)
{
	u32 loc;
	FSTEntry* FSTEntries;
	ARCHandle* handle;

	handle     = dir->handle;
	FSTEntries = (FSTEntry*)handle->FSTStart;
	loc        = dir->location;

retry:
	if (loc <= dir->entryNum || dir->next <= loc) {
		return FALSE;
	}

	dirent->handle   = handle;
	dirent->entryNum = loc;
	dirent->isDir    = entryIsDir(loc);
	dirent->name     = handle->FSTStringStart + stringOff(loc);

	if (dirent->name[0] == '.' && dirent->name[1] == '\0') {
		loc++;
		goto retry;
	}

	dir->location = entryIsDir(loc) ? nextDir(loc) : (loc + 1);
	return TRUE;
}

BOOL ARCCloseDir(ARCDir* dir)
{
	return TRUE;
}

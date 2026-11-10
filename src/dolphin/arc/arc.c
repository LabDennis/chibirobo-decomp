// retail compares path bytes unsigned like the rest of the 2004 sdk, but this
// unit builds with the default signed char flags
#pragma unsigned_char on

#include "types.h"
#include "Dolphin/os.h"
#include "Dolphin/arc.h"

// sdk ARC archive reader, the "arc.c" assert string and the 0x55AA382D magic
// are what names ARCInitHandle, ARCFastOpen, ARCConvertPathToEntrynum and the rest

extern int tolower(int c);

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
		OSErrorLine(59, "ARCInitHandle: bad archive format");
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
		if (tolower(*path++) != tolower(*string++)) {
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
			if (entryIsDir(i) == FALSE && isDir == TRUE) {
				continue;
			}

			stringPtr = handle->FSTStringStart + stringOff(i);

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

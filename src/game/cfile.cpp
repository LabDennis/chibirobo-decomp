#include "game/cfile.h"
#include "game/mm.h"
#include "Dolphin/os.h"
#include <string.h>

extern "C" {
s32 HSD_DevComRequest(s32 file, u32 src, void* dest, u32 size, s32 type, s32 pri, void (*callback)(s32, DVDFileInfo*, void*, BOOL),
                      DVDFileInfo* args);
}

// report, then panic with the function name. the line goes in by hand like the asserts
#define HALT(line)              \
	OSReport("\n!!!HALT!!!\n"); \
	OSPanic(__FILE__, line, "%s%s", "", __FUNCTION__)

// the archive files get read out of, set by CFileArchive::ReadAsync
static CFileArchive* gCurrentFileArchive;

/**
 * @note Address: 0x8019FD78
 * @note Size: 0x24
 */
static void ReadCallback(s32, DVDFileInfo* info, void*, BOOL cancel)
{
	s32* result = (s32*)info->cBlock.userData;
	if (cancel) {
		*result = -2; // cancelled
	} else {
		*result = 0; // done
	}
}

/**
 * @note Address: 0x8019FD4C
 * @note Size: 0x2C
 */
void* CFile::Alloc(u32 size)
{
	return mm::Alloc(size, mPath);
}

/**
 * @note Address: 0x8019FD08
 * @note Size: 0x44
 */
void CFile::Free()
{
	if (mBuffer != NULL) {
		mm::Free(mBuffer, 1);
		mBuffer = NULL;
	}
}

/**
 * @note Address: 0x8019FCE4
 * @note Size: 0x24
 */
void* CFileA::Alloc(u32 size)
{
	return OSAlloc(size);
}

/**
 * @note Address: 0x8019FCA0
 * @note Size: 0x44
 */
void CFileA::Free()
{
	if (mBuffer != NULL) {
		OSFree(mBuffer);
		mBuffer = NULL;
	}
}

/**
 * @note Address: 0x8019FC50
 * @note Size: 0x50
 */
s32 CFile::ConvertPathToEntrynum(const char* path)
{
	s32 entrynum = -1;
	if (gCurrentFileArchive != NULL) { // the open archive wins over the disc
		entrynum = ARCConvertPathToEntrynum(&gCurrentFileArchive->mHandle, path);
	}
	if (entrynum < 0) {
		entrynum = DVDConvertPathToEntrynum((char*)path);
	}
	return entrynum;
}

inline s32 CFile::StartRead(BOOL fromAram)
{
	s32 status;
	if (fromAram) { // reads straight out of aram
		u32 pos               = mOffset;
		u32 len               = OSRoundUp32B(mSize);
		DVDFileInfo* info     = mArchive != NULL ? (DVDFileInfo*)mArchive : this;
		mResult               = 1;
		info->cBlock.userData = (void*)&mResult;
		HSD_DevComRequest(0, pos, mBuffer, len, 0x19, 1, ReadCallback, info);
		status = 0;
	} else {
		u32 pos = mOffset;
		s32 entrynum;
		u32 len = OSRoundUp32B(mSize);
		if (mArchive != NULL) {
			entrynum = mArchive->mEntrynum;
			memcpy(this, mArchive, sizeof(DVDFileInfo)); // the archive's file info becomes ours
			pos += ARCGetStartOffset(&mArcInfo);         // plus the member's offset inside it
		} else {
			entrynum = mEntrynum;
		}
		mResult         = 1;
		cBlock.userData = (void*)&mResult;
		HSD_DevComRequest(entrynum, pos, mBuffer, len, 0x21, 1, ReadCallback, this);
		status = 0;
	}
	return status;
}

// todo: cleanup
inline s32 CFile::Open()
{
	if (mArchive != NULL) {
		mEntrynum = ARCConvertPathToEntrynum(&mArchive->mHandle, mPath);
		if (mEntrynum < 0) {
			return -3;
		}
		if (!ARCFastOpen(&mArchive->mHandle, mEntrynum, &mArcInfo)) {
			return -4; // -3: unknown path, -4: open failed
		}
		mSize = ARCGetLength(&mArcInfo);
	} else {
		mEntrynum = DVDConvertPathToEntrynum(mPath);
		if (mEntrynum < 0) {
			return -3;
		}
		if (!DVDFastOpen(mEntrynum, this)) {
			return -4;
		}
		mSize = length;
	}
	return mEntrynum;
}

/**
 * @note Address: 0x8019F91C
 * @note Size: 0x334
 */
s32 CFile::ReadAsync(const char* path, u32 offset, u32 size, BOOL fromAram)
{
	strcpy(mPath, path);
	BOOL inArchive = gCurrentFileArchive != NULL ? ARCConvertPathToEntrynum(&gCurrentFileArchive->mHandle, mPath) >= 0 : 0;
	if (inArchive) {
		mArchive = gCurrentFileArchive;
	} else {
		mArchive = NULL;
	}

	if (mBuffer != NULL) {
		Close();
	}
	mOffset = offset;

	s32 result = Open();
	if (result < 0) {
		if (mArchive != NULL) {
			OSReport("file not found - '%s//%s'!\n", mArchive->mPath, mPath);
		} else {
			OSReport("file not found - '%s'!\n", mPath);
		}
		return result;
	}

	if (size != 0) {
		mSize = size;
	}
	mBuffer = Alloc(OSRoundUp32B(mSize));
	if (mBuffer == NULL) {
		if (mArchive != NULL) {
			OSReport("cannot allocate memory for '%s//%s'!\n", mArchive->mPath, mPath);
		} else {
			OSReport("cannot allocate memory for '%s'!\n", mPath);
		}
		return -7;
	}

	s32 error = StartRead(fromAram);
	if (error < 0) {
		if (mArchive != NULL) {
			OSReport("cannot read file - '%s//%s'!\n", mArchive->mPath, mPath);
		} else {
			OSReport("cannot read file - '%s'!\n", mPath);
		}
		return error;
	}
	return mResult;
}

/**
 * @note Address: 0x8019F8D8
 * @note Size: 0x44
 */
s32 CFile::GetResult()
{
	if (mResult == 0) { // a finished read gets its cache invalidated
		DCInvalidateRange(mBuffer, mSize);
	}
	return mResult;
}

/**
 * @note Address: 0x8019F878
 * @note Size: 0x60
 */
s32 CFile::Read(const char* path, u32 offset, u32 size)
{
	s32 result = ReadAsync(path, offset, size, FALSE);
	if (result >= 0) {
		do {
			result = GetResult();
		} while (result == 1); // 1 means still in flight
	}
	return result;
}

/**
 * @note Address: 0x8019F804
 * @note Size: 0x74
 */
void CFile::Close()
{
	if (mEntrynum >= 0) {
		mEntrynum = -1;
		if (mArchive != NULL) {
			ARCClose(&mArcInfo);
		} else {
			DVDClose(this);
		}
	}
	Free();
	mState = 1;
}

/**
 * @note Address: 0x8019F6EC
 * @note Size: 0x118
 */
BOOL CFile::ReadEx(const char* path, u32 offset, u32 size)
{
	switch (mState) { // 0 idle, 1 starts the read, 2 waits on it
	case 0:
		return TRUE;
	case 1:
		if (ReadAsync(path, offset, size, FALSE) >= 0) {
			mState++;
		} else {
			HALT(314);
		}
	case 2:
		s32 result = GetResult();
		if (result == 0) {
			mState = 0;
			return TRUE;
		}
		if (result < 0) {
			HALT(323);
		}
		break;
	}
	return FALSE;
}

/**
 * @note Address: 0x8019F668
 * @note Size: 0x84
 */
s32 CFileArchive::ReadAsync(const char* path, u32 offset, u32 size, BOOL fromAram)
{
	if (gCurrentFileArchive != NULL) { // a new archive replaces the current one
		gCurrentFileArchive->Close();
	}
	gCurrentFileArchive = NULL;
	s32 result          = CFile::ReadAsync(path, offset, size, fromAram);
	if (result >= 0) {
		gCurrentFileArchive = this;
	}
	return result;
}

/**
 * @note Address: 0x8019F594
 * @note Size: 0xD4
 */
s32 CFileArchive::GetResult()
{
	s32 result = CFile::GetResult();
	if (result == 0 && !ARCInitHandle(mBuffer, &mHandle)) {
		OSReport("cannot open - '%s'!\n", mPath);
		CFile::Close();
		result = -4;
	}
	return result;
}

/**
 * @note Address: 0x8019F40C
 * @note Size: 0x188
 */
BOOL CFileArchive::ReadEx(const char* path, u32 offset, u32 size)
{
	switch (mState) {
	case 0:
		return TRUE;
	case 1:
		// the header first, it says how much of the image to read
		if (CFile::ReadAsync(path, offset, sizeof(ARCHeader), FALSE) >= 0) {
			mState++;
		} else {
			return TRUE;
		}
	case 2: {
		s32 result = CFile::GetResult();
		if (result == 0) {
			// the header's file data offset, only the part before it is read
			size = ((const ARCHeader*)mBuffer)->fileStart;
			Close();
			mState = 3;
		} else if (result < 0) {
			return TRUE;
		} else {
			break;
		}
	}
	case 3:
		if (ReadAsync(path, offset, size, FALSE) >= 0) {
			mState++;
		} else {
			return TRUE;
		}
	case 4: {
		s32 result = GetResult();
		if (result == 0) {
			mState = 0;
			return TRUE;
		}
		if (result < 0) {
			return TRUE;
		}
		break;
	}
	}
	return FALSE;
}

/**
 * @note Address: 0x8019F378
 * @note Size: 0x94
 */
BOOL CFileText::GetLine(char* line)
{
	if (mPos >= mSize) {
		return FALSE;
	}

	char* p      = (char*)mBuffer + mPos;
	BOOL lineEnd = FALSE;
	do {
		char c = *p;
		if (c == '\r' || c == '\n' || c == 0x1A) { // \r, \n and 0x1a all end a line
			lineEnd = TRUE;
			p++;
		} else {
			if (lineEnd) {
				break;
			}
			*line++ = c;
			p++;
		}
	} while ((u32)(p - (char*)mBuffer) < mSize);

	*line = '\0';
	mPos  = p - (char*)mBuffer;
	return TRUE;
}

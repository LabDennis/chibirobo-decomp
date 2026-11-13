#include "game/cfile.h"
#include "game/mm.h"
#include "Dolphin/os.h"
#include <string.h>

// The Wii CFile keeps two flags where the GameCube one keeps the archive pointer: the archive is
// always the current one, and a buffer handed in by the caller is never freed.

extern "C" {
s32 fn_80190238(const char* path); // DVDConvertPathToEntrynum
BOOL fn_80190540(s32 entrynum, DVDFileInfo* info);
BOOL fn_801905A8(DVDFileInfo* info);
BOOL fn_80190618(DVDFileInfo* info, void* addr, s32 length, s32 offset, DVDCallback callback, s32 prio);
BOOL fn_801FEAB4(void* arcStart, ARCHandle* handle); // ARCInitHandle
BOOL fn_801FEDF8(ARCHandle* handle, s32 entrynum, ARCFileInfo* info);
s32 fn_801FEE48(ARCHandle* handle, const char* path); // ARCConvertPathToEntrynum
u32 fn_801FF288(ARCFileInfo* info);
u32 fn_801FF290(ARCFileInfo* info);
BOOL fn_801FF298(ARCFileInfo* info);
}

// the archive files get read out of, set by CFileArchive::ReadAsync
static CFileArchive* gCurrentFileArchive;

static void ReadCallback(s32 result, DVDFileInfo* info);

s32 CFile::Open(const char* path)
{
	if (mInArchive) {
		mEntrynum = fn_801FEE48(&gCurrentFileArchive->mHandle, path);
		if (mEntrynum < 0) {
			return -3;
		}
		if (!fn_801FEDF8(&gCurrentFileArchive->mHandle, mEntrynum, &mArcInfo)) {
			return -4;
		}
		mSize = fn_801FF290(&mArcInfo);
	} else {
		mEntrynum = fn_80190238(path);
		if (mEntrynum < 0) {
			return -3;
		}
		if (!fn_80190540(mEntrynum, this)) {
			return -4;
		}
		mSize = length;
	}
	return mEntrynum;
}

s32 CFile::CloseHandle()
{
	if (mEntrynum >= 0) {
		mEntrynum = -1;
		if (mInArchive) {
			if (!fn_801FF298(&mArcInfo)) {
				return -5;
			}
		} else if (!fn_801905A8(this)) {
			return -5;
		}
	}
	return 0;
}

s32 CFile::StartRead(u32 size, u32 offset)
{
	if (mInArchive) {
		memcpy(this, gCurrentFileArchive, sizeof(DVDFileInfo));
		offset += fn_801FF288(&mArcInfo);
	}
	mResult         = 1;
	cBlock.userData = &mResult;
	fn_80190618(this, mBuffer, size, offset, ReadCallback, 2);
	return 0;
}

static void ReadCallback(s32 result, DVDFileInfo* info)
{
	s32* status = (s32*)info->cBlock.userData;
	if (result == -1) {
		*status = -6;
	} else if (result == -3) {
		*status = -2;
	} else {
		*status = 0;
	}
}

void* CFile::Alloc(u32 size)
{
	return mm::Alloc(size, mPath);
}

void CFile::Free()
{
	if (mBuffer != NULL) {
		if (mFromMemory) {
			mFromMemory = FALSE;
		} else {
			mm::Free(mBuffer);
		}
		mBuffer = NULL;
	}
}

void* CFileA::Alloc(u32 size)
{
	return OSAlloc(size);
}

void CFileA::Free()
{
	if (mBuffer != NULL) {
		if (mFromMemory) {
			mFromMemory = FALSE;
		} else {
			OSFree(mBuffer);
		}
		mBuffer = NULL;
	}
}

BOOL CFile::IsInArchive(const char* path)
{
	return gCurrentFileArchive != NULL ? fn_801FEE48(&gCurrentFileArchive->mHandle, path) >= 0 : FALSE;
}

s32 CFile::ConvertPathToEntrynum(const char* path)
{
	s32 entrynum = -1;
	if (gCurrentFileArchive != NULL) {
		entrynum = fn_801FEE48(&gCurrentFileArchive->mHandle, path);
	}
	if (entrynum < 0) {
		entrynum = fn_80190238(path);
	}
	return entrynum;
}

s32 CFile::OpenPath(const char* path)
{
	strcpy(mPath, path);
	mInArchive = IsInArchive(mPath);
	s32 result = Open(mPath);
	if (result < 0) {
		if (mInArchive) {
			OSReport("file not found - '%s//%s'!\n", gCurrentFileArchive->mPath, mPath);
		} else {
			OSReport("file not found - '%s'!\n", mPath);
		}
	}
	return result;
}

// fromAram is the GameCube name: on the Wii a set flag means offset is a buffer already in memory
s32 CFile::ReadAsync(const char* path, u32 offset, u32 size, BOOL fromAram)
{
	if (mBuffer != NULL && !mFromMemory) {
		Close();
	}
	if (fromAram) {
		mBuffer     = (void*)offset;
		mEntrynum   = -1;
		mOffset     = 0;
		mSize       = size;
		mFromMemory = fromAram;
		mResult     = 0;
		mState      = 0;
		return 0;
	}

	mOffset    = offset;
	s32 result = OpenPath(path);
	if (result < 0) {
		return result;
	}

	mOffset = offset;
	if (size != 0) {
		mSize = size;
	}
	mBuffer = Alloc(OSRoundUp32B(mSize));
	if (mBuffer == NULL) {
		if (mInArchive) {
			OSReport("cannot allocate memory for '%s//%s'!\n", gCurrentFileArchive->mPath, mPath);
		} else {
			OSReport("cannot allocate memory for '%s'!\n", mPath);
		}
		return -7;
	}
	DCInvalidateRange(mBuffer, OSRoundUp32B(mSize));

	s32 error = fromAram ? -3 : StartRead(OSRoundUp32B(mSize), mOffset);
	if (error < 0) {
		if (mInArchive) {
			OSReport("cannot read file - '%s//%s'!\n", gCurrentFileArchive->mPath, mPath);
		} else {
			OSReport("cannot read file - '%s'!\n", mPath);
		}
		return error;
	}
	mState++;
	return mResult;
}

s32 CFile::GetResult()
{
	if (mResult == 0) {
		mState = 0;
	} else if (mResult == -2) {
		Close();
		mState = 0;
	}
	return mResult;
}

s32 CFile::Read(const char* path, u32 offset, u32 size)
{
	s32 result = ReadAsync(path, offset, size, FALSE);
	if (result >= 0) {
		do {
			result = GetResult();
		} while (result == 1);
	}
	return result;
}

// The Wii Close reports TRUE: fn_802B63E0 drops its file pointer only when it does.
BOOL CFile::Close()
{
	CloseHandle();
	Free();
	mState = 1;
	return TRUE;
}

BOOL CFile::ReadEx(const char* path, u32 offset, u32 size)
{
	switch (mState) {
	case 0:
		return TRUE;
	case 1:
		if (ReadAsync(path, offset, size, FALSE) < 0) {
			OSPanic(__FILE__, 318, "");
		}
	case 2:
		s32 result = GetResult();
		if (result == 0) {
			return TRUE;
		}
		if (result < 0) {
			OSPanic(__FILE__, 326, "");
		}
		break;
	}
	return FALSE;
}

s32 CFileArchive::ReadAsync(const char* path, u32 offset, u32 size, BOOL fromAram)
{
	if (gCurrentFileArchive != NULL) {
		gCurrentFileArchive->Close();
	}
	gCurrentFileArchive = NULL;
	s32 result          = CFile::ReadAsync(path, offset, size, fromAram);
	if (result >= 0) {
		gCurrentFileArchive = this;
	}
	return result;
}

s32 CFileArchive::GetResult()
{
	s32 result = CFile::GetResult();
	if (result == 0 && !fn_801FEAB4(mBuffer, &mHandle)) {
		OSReport("cannot open - '%s'!\n", mPath);
		CFile::Close();
		result = -4;
	}
	return result;
}

BOOL CFileArchive::ReadEx(const char* path, u32 offset, u32 size)
{
	switch (mState) {
	case 0:
		return TRUE;
	case 1:
		// the header first, it says how much of the image to read
		if (CFile::ReadAsync(path, offset, sizeof(ARCHeader), FALSE) < 0) {
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
		if (ReadAsync(path, offset, size, FALSE) < 0) {
			return TRUE;
		}
	case 4: {
		s32 result = GetResult();
		if (result == 0) {
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

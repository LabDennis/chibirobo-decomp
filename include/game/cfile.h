#ifndef _GAME_CFILE_H
#define _GAME_CFILE_H

// relative, so files built with the HSD header tree first still get the sdk ones
#include "../Dolphin/dvd.h"
#include "../Dolphin/arc.h"

// file loaders own their buffers. class names from RTTI, ReadEx from a halt string

class CFileArchive;

class CFile : public DVDFileInfo {
public:
	CFile()
	{
#if defined(VERSION_R24J01)
		mFromMemory = mInArchive = 0;
#else
		mArchive = NULL;
#endif
		mEntrynum = -1;
		mBuffer   = NULL;
		mResult   = 0;
		mState    = 1;
	}

	// _00 DVDFileInfo
	char mPath[0x84]; // _3C
	s32 mEntrynum;    // _C0
	u32 mOffset;      // _C4
	u32 mSize;        // _C8
	void* mBuffer;    // _CC
	s32 mResult;      // _D0, 1 while the read is running, then 0 or an error
#if defined(VERSION_R24J01)
	u8 mFromMemory; // _D4, mBuffer is the caller's memory, not read from disc
	u8 mInArchive;  // _D5, read out of the current CFileArchive
	u8 _D6[2];
#else
	CFileArchive* mArchive; // _D4
#endif
	ARCFileInfo mArcInfo; // _D8
	s32 mState;           // _E4, ReadEx step
	                      // _E8 vtable

	virtual ~CFile() { }                                                          // _08 (weak)
	virtual void* Alloc(u32 size);                                                // _0C
	virtual void Free();                                                          // _10
	virtual s32 ReadAsync(const char* path, u32 offset, u32 size, BOOL fromAram); // _14
	virtual s32 GetResult();                                                      // _18
	virtual s32 Read(const char* path, u32 offset, u32 size);                     // _1C
	virtual void Close();                                                         // _20
	virtual BOOL ReadEx(const char* path, u32 offset, u32 size);                  // _24
#if defined(VERSION_R24J01)
	virtual BOOL IsLoaded() { return mBuffer != NULL && mState == 0; } // _28 (weak)
#endif

	s32 ConvertPathToEntrynum(const char* path);
#if defined(VERSION_R24J01)
	s32 Open(const char* path);
	s32 CloseHandle();
	s32 StartRead(u32 size, u32 offset);
	BOOL IsInArchive(const char* path);
	s32 OpenPath(const char* path);
#else
	s32 CloseHandle(); // in ctpl.cpp

	// inlined:
	s32 Open();
	s32 StartRead(BOOL fromAram);
#endif
};
// size checks by hand, STATIC_ASSERT lives in types.h
typedef char CFile_size_check[sizeof(CFile) == 0xEC ? 1 : -1];

// takes its buffer straight from the OS heap
class CFileA : public CFile {
public:
	virtual ~CFileA() { }          // _08 (weak)
	virtual void* Alloc(u32 size); // _0C
	virtual void Free();           // _10
};

// an ARC image, files read while it's the current archive come out of it
class CFileArchive : public CFile {
public:
	virtual ~CFileArchive() { }                                                   // _08 (weak)
	virtual s32 ReadAsync(const char* path, u32 offset, u32 size, BOOL fromAram); // _14
	virtual s32 GetResult();                                                      // _18
	virtual BOOL ReadEx(const char* path, u32 offset, u32 size);                  // _24

	ARCHandle mHandle; // _EC
};
typedef char CFileArchive_size_check[sizeof(CFileArchive) == 0x108 ? 1 : -1];

// a text file read a line at a time
class CFileText : public CFile {
public:
	virtual ~CFileText() { } // _08 (weak)

	BOOL GetLine(char* line);

	u32 mPos; // _EC
};
typedef char CFileText_size_check[sizeof(CFileText) == 0xF0 ? 1 : -1];

#endif

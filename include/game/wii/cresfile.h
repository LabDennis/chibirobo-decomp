#ifndef _GAME_WII_CRESFILE_H
#define _GAME_WII_CRESFILE_H

#include "game/cfile.h"

// the asynchronous read a resource owner keeps next to the loaded resource
class CFileRequest {
public:
	void Reset();
	BOOL Request(ResourceFile* file, const char* path, u32 flags, u32 offset, u32 size);
	BOOL Update();
	void Release();

	ResourceFile* mFile; // _00
	s32 mState;          // _04, 0 idle / 1 reading / 2 done / 10 sharing an already-loaded file
	u32 mFlags;          // _08, 0x01000000 closes the file when a request fails
	s32 mResult;         // _0C
	u32 mOffset;         // _10
	u32 mSize;           // _14
};

// an accessor over a brres image's per-type dictionaries; the calls it makes
// into the library's resource walkers still sit in the unnamed nw4r unit
// around 0x8025B000
class CResFile {
public:
	void Init(void* data, BOOL bindCommon, BOOL bindAll);
	void BindTop(s32 index, BOOL bind);

	void* mRef;     // _00, the raw resource image
	void* mTop;     // _04, its top dictionary
	u8 _08[0x2C];   // _08
	void* mDics[9]; // _34.._54
};

#endif

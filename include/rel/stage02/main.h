#ifndef STAGE02_MAIN_H
#define STAGE02_MAIN_H

#include "Version.h"
#include "types.h"

class XObj;

struct Stage02Main {
#if defined(VERSION_JP)
	u8 _00[0x9818]; // _00
#else
	u8 _00[0x9860]; // _00
#endif
	XObj* mObjects[0x400]; // _9860
	s32 mObjectCount;      // _A860

	XObj* GetObject(s32 index) const { return mObjects[index]; }
};

#endif

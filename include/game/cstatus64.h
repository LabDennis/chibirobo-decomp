#ifndef _GAME_CSTATUS64_H
#define _GAME_CSTATUS64_H

#include "types.h"

#if defined(VERSION_R24J01)
class CBitStatus64 {
public:
	u64 mFlags;
	virtual ~CBitStatus64();
};
STATIC_ASSERT(sizeof(CBitStatus64) == 0x10);

class CStatus64 : public CBitStatus64 {
public:
	virtual ~CStatus64();
};
#else
// CStatus with 64 flags, class name from RTTI. XObj keeps its state bits in one, so does CBase.
// its own header since cgame.h can't take ut.h (cpad.h's CPad clashes with ut.h's)
class CStatus64 {
public:
	CStatus64() { mFlags = 0; }

	u64 mFlags; // _00
	// _08 vtable

	virtual ~CStatus64() { } // _08
};
#endif
typedef char CStatus64_size_check[sizeof(CStatus64) == 0x10 ? 1 : -1];

#endif

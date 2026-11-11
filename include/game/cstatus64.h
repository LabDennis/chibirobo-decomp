#ifndef _GAME_CSTATUS64_H
#define _GAME_CSTATUS64_H

#include "types.h"

// CStatus with 64 flags, class name from RTTI. XObj keeps its state bits in one, so does CBase.
// its own header since cgame.h can't take ut.h (cpad.h's CPad clashes with ut.h's)
class CStatus64 {
public:
	CStatus64() { mFlags = 0; }

	u64 mFlags; // _00
	// _08 vtable

	virtual ~CStatus64(); // _08
};
inline CStatus64::~CStatus64()
{
}
typedef char CStatus64_size_check[sizeof(CStatus64) == 0x10 ? 1 : -1];

#endif

#include "types.h"
#include <stdlib.h>

namespace std {
class exception;
}

inline void operator delete(void* ptr) throw()
{
	if (ptr != 0) {
		free(ptr);
	}
}

extern "C" {
extern void* __vt__Q23std9exception[];

// Weak ABI copy of std::exception::~exception().
WEAK std::exception* __dt__Q23std9exceptionFv(std::exception* object, s32 flags)
{
	if (object != 0) {
		*(void**)object = __vt__Q23std9exception;
		if ((s16)flags > 0) {
			::operator delete(object);
		}
	}
	return object;
}
}

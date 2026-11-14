#include <stdlib.h>

void operator delete(void* ptr) throw()
{
	if (ptr != 0) {
		free(ptr);
	}
}

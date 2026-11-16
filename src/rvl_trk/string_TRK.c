#include "rvl_trk/trk.h"

size_t TRK_strlen(const char* str)
{
	size_t len = -1;
	u8* p      = (u8*)str - 1;

	do {
		len++;
	} while (*++p);

	return len;
}

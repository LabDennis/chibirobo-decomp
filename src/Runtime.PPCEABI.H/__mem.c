#include "types.h"
#include "PowerPC_EABI_Support/Runtime/__mem.h"
void* memcpy(void* dst, const void* src, size_t n)
{
	const char* p;
	char* q;
	int rev = ((u32)src < (u32)dst);

	if (!rev) {

		for (p = (const char*)src - 1, q = (char*)dst - 1, n++; --n;)
			*++q = *++p;

	} else {
		for (p = (const char*)src + n, q = (char*)dst + n, n++; --n;)
			*--q = *--p;
	}
	return (dst);
}

void __fill_mem(void* dst, int val, u32 n)
{
	u8* bytes = (u8*)dst - 1;
	u32* words;
	u32 pattern = (u8)val;
	u32 count;

	if (n >= 32) {
		// Align the next byte address so that word stores are safe.
		count = (~(u32)bytes) & 3;

		if (count) {
			n -= count;

			do
				*++bytes = pattern;
			while (--count);
		}

		if (pattern)
			pattern |= pattern << 24 | pattern << 16 | pattern << 8;

		words = (u32*)(bytes + 1) - 1;

		// Write eight words per iteration for large fills.
		count = n >> 5;

		if (count)
			do {
				*++words = pattern;
				*++words = pattern;
				*++words = pattern;
				*++words = pattern;
				*++words = pattern;
				*++words = pattern;
				*++words = pattern;
				*++words = pattern;
			} while (--count);

		count = (n & 31) >> 2;

		if (count)
			do
				*++words = pattern;
			while (--count);

		bytes = (u8*)(words + 1) - 1;

		n &= 3;
	}

	if (n)
		do
			*++bytes = pattern;
		while (--n);
}

void* memset(void* dst, int val, size_t n)
{
	__fill_mem(dst, val, n);

	return (dst);
}

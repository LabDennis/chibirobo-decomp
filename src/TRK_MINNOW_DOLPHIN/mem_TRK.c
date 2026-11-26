#include "types.h"

INIT void* TRK_memcpy(void* dst, const void* src, size_t n);
INIT void* TRK_memset(void* dst, int val, size_t n);

enum { FILL_BLOCK_SIZE = 32 };

void TRK_fill_mem(void* dst, int val, size_t n)
{
	u8* bytes = dst;
	u8 byte   = (u8)val;

	if (n >= FILL_BLOCK_SIZE) {
		size_t leading = (-(size_t)bytes) & (sizeof(u32) - 1);
		size_t blocks;
		size_t words;
		u32* aligned;
		u32 word = byte;

		if (leading) {
			n -= leading;
			do {
				*bytes++ = byte;
			} while (--leading != 0);
		}

		// Replicate the fill byte across a word once the destination is aligned.
		word |= word << 8;
		word |= word << 16;
		aligned = (u32*)bytes;
		blocks = n / FILL_BLOCK_SIZE;
		if (blocks) {
			do {
				unsigned i;
				for (i = 0; i < FILL_BLOCK_SIZE / sizeof(*aligned); ++i) {
					aligned[i] = word;
				}
				aligned += FILL_BLOCK_SIZE / sizeof(*aligned);
			} while (--blocks != 0);
		}
		words = (n % FILL_BLOCK_SIZE) / sizeof(*aligned);
		if (words) {
			do {
				*aligned++ = word;
			} while (--words != 0);
		}

		bytes = (u8*)aligned;
		n %= sizeof(*aligned);
	}

	if (n) {
		do {
			*bytes++ = byte;
		} while (--n != 0);
	}
}

void* TRK_memcpy(void* dst, const void* src, size_t n)
{
	u8* destBytes      = dst;
	const u8* srcBytes = src;

	if (n) {
		do {
			*destBytes++ = *srcBytes++;
		} while (--n != 0);
	}
	return dst;
}

void* TRK_memset(void* dst, int val, size_t n)
{
	TRK_fill_mem(dst, val, n);
	return dst;
}

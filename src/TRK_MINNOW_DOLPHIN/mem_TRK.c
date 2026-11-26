#include "types.h"

INIT void* TRK_memcpy(void* dst, const void* src, size_t n);
INIT void* TRK_memset(void* dst, int val, size_t n);

// NONMATCHING: retail allocates the 32-byte fill counter/pointer as r4/r3 instead of r3/r4.
// The pre-incremented copy and fill loops are the MetroTRK source form kept by ogws:
// https://github.com/zeldaret/ogws/blob/main/src/MetroTRK/debugger/Portable/mem_TRK.c
void TRK_fill_mem(void* dst, int val, size_t n)
{
	int work;
	char* bdst  = (char*)dst;
	int* wdst = (int*)dst;
	unsigned int byte = (unsigned char)val;

	bdst--;

	if (n >= 32) {
		work = ~(unsigned int)bdst & 3;

		if (work) {
			n -= work;
			do {
				*++bdst = byte;
			} while (--work);
		}

		if (byte) {
			byte = (byte << 24) | (byte << 16) | (byte << 8) | byte;
		}

		work = n / 32;
		wdst = (int*)(bdst - 3);
		if (work) {
			do {
				wdst[1] = byte;
				wdst[2] = byte;
				wdst[3] = byte;
				wdst[4] = byte;
				wdst[5] = byte;
				wdst[6] = byte;
				wdst[7] = byte;
				wdst[8] = byte;
				wdst += 8;
			} while (--work);
		}

		work = (n / 4) & 7;
		if (work) {
			do {
				*++wdst = byte;
			} while (--work);
		}

		bdst = (char*)wdst + 3;
		n &= 3;
	}

	if (n) {
		do {
			*++bdst = byte;
		} while (--n);
	}
}

void* TRK_memcpy(void* dst, const void* src, size_t n)
{
	const char* srcBytes = (const char*)src;
	char* destBytes      = (char*)dst;

	srcBytes--;
	destBytes--;
	n++;

	while (--n != 0) {
		*++destBytes = *++srcBytes;
	}

	return dst;
}

void* TRK_memset(void* dst, int val, size_t n)
{
	TRK_fill_mem(dst, val, n);
	return dst;
}

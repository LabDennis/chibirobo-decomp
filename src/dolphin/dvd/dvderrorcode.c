#include "types.h"

static u32 ErrorTable[18] = { 0x00000000, 0x00023A00, 0x00062800, 0x00030200, 0x00031100, 0x00052000, 0x00052001, 0x00052100, 0x00052400,
	                   0x00052401, 0x00052402, 0x000B5A01, 0x00056300, 0x00020401, 0x00020400, 0x00040800, 0x00100007, 0x00000000 };
/**
 * @note Address: 0x801705DC
 * @note Size: 0x11C
 */
u8 ErrorCode2Num(u32 errorCode)
{
	int i;

	for (i = 0; i < 18; i++) {
		if (errorCode == ErrorTable[i]) {
			return i;
		}
	}

	if (errorCode >= 0x100000 && errorCode <= 0x100008) {
		return 17;
	}

	return 29;
}

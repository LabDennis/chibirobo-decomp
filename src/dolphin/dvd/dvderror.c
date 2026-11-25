#include "Dolphin/dvd.h"
#include "Dolphin/os.h"

u8 ErrorCode2Num(u32 errorCode);

/**
 * @note Address: 0x801706F8
 * @note Size: 0x7C
 */
void __DVDStoreErrorCode(u32 errCode)
{
	u8 storedCode;
	OSSramEx* sramPtr;
	u32 upperByte;

	if (errCode == 0x1234567) {
		storedCode = -1;
	} else if (errCode == 0x1234568) {
		storedCode = -2;
	} else {
		upperByte  = errCode >> 0x18;
		storedCode = ErrorCode2Num(errCode & 0xffffff);
		if (errCode >> 0x18 >= 6) {
			upperByte = 6;
		}
		storedCode = storedCode + upperByte * 30;
	}

	sramPtr               = __OSLockSramEx();
	sramPtr->dvdErrorCode = storedCode;
	__OSUnlockSramEx(TRUE);
}

#include "revolution/hw_regs.h"
#include "revolution/os/OSLaunch.h"

#include "Dolphin/os.h"

BOOL __OSCreateNandbootInfo(void)
{
	NANDStatus status;
	s32 err;

	err = NANDPrivateGetStatus("/shared2/sys/NANDBOOTINFO", &status);
	if (err == NAND_RESULT_OK && status.permission == 0x3F) {
		return TRUE;
	}

	if (err == NAND_RESULT_OK && status.permission != 0x3F) {
		if (NANDPrivateDelete("/shared2/sys/NANDBOOTINFO") != NAND_RESULT_OK) {
			return FALSE;
		}
	} else if (err != NAND_RESULT_NOEXISTS) {
		return FALSE;
	}

	if (NANDPrivateCreate("/shared2/sys/NANDBOOTINFO", 0x3F, 0) != NAND_RESULT_OK) {
		return FALSE;
	}

	return TRUE;
}

BOOL __OSWriteNandbootInfo(OSNandbootInfo* info)
{
	NANDFileInfo file;
	u32 sum = 0;
	u32* p  = (u32*)info + 1;
	int i;

	for (i = 0; i < (int)(sizeof(OSNandbootInfo) / sizeof(u32)) - 1; i++) {
		sum += *p++;
	}
	info->checksum = sum;

	if (NANDPrivateOpen("/shared2/sys/NANDBOOTINFO", &file, NAND_ACCESS_WRITE) == NAND_RESULT_OK) {
		if (NANDWrite(&file, info, sizeof(OSNandbootInfo)) != sizeof(OSNandbootInfo)) {
			NANDClose(&file);
			return FALSE;
		}

		return NANDClose(&file) == NAND_RESULT_OK;
	}

	return FALSE;
}

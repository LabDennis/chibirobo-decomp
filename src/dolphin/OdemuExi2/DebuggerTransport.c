#include "sdk/private/exi/DebuggerTransport.h"

/**
 * @note Address: 0x8015D72C
 * @note Size: 0x298
 */
BOOL DBGEXIImm(void* buffer, s32 bytecounter, u32 write)
{
	u8* tempPointer;
	u32 writeOutValue;
	int i;

	if (write) {
		writeOutValue = 0;
		for (i = 0; i < bytecounter; i++) {
			writeOutValue |= (u32)((const u8*)buffer)[i] << ((3 - i) << 3);
		}
		EXI_REG(2, EXI_REG_DATA) = writeOutValue;
	}

	EXI_REG(2, EXI_REG_CONTROL) = EXI_CR_IMM(write, bytecounter);
	DBGEXISync();

	if (!write) {
		writeOutValue = EXI_REG(2, EXI_REG_DATA);
		tempPointer   = buffer;
		for (i = 0; i < bytecounter; i++) {
			*tempPointer++ = writeOutValue >> ((3 - i) << 3);
		}
	}

	return TRUE;
}

/**
 * @note Address: 0x8015D680
 * @note Size: 0xAC
 */
BOOL DBGReadMailbox(u32* value)
{
	return DBGReadRegister(DBG_READ_MAILBOX, value);
}

/**
 * @note Address: 0x8015D5A4
 * @note Size: 0xDC
 */
BOOL DBGRead(u32 address, u32* buffer, s32 count)
{
	BOOL total = FALSE;
	u32* buf_p = buffer;
	u32 v1;
	u32 v;

	DBGEXISelect(EXI_FREQ_16M);

	v1 = (address & 0x1fffc) << 8 | 0x20000000;
	total |= IS_FALSE(DBGEXIImm(&v1, sizeof(v1), EXI_WRITE));
	total |= IS_FALSE(DBGEXISync());

	while (count) {
		total |= IS_FALSE(DBGEXIImm(&v, sizeof(v), EXI_READ));
		total |= IS_FALSE(DBGEXISync());

		*buf_p++ = v;

		count -= 4;
		if (count < 0) {
			count = 0;
		}
	}

	total |= IS_FALSE(DBGEXIDeselect());
	return IS_FALSE(total);
}

/**
 * @note Address: 0x8015D4C8
 * @note Size: 0xDC
 */
BOOL DBGWrite(u32 address, const void* buffer, s32 count)
{
	BOOL total = FALSE;
	const u32* buf_p = buffer;
	u32 v1;
	u32 v;

	DBGEXISelect(EXI_FREQ_16M);

	v1 = (address & 0x1fffc) << 8 | 0xa0000000;
	total |= IS_FALSE(DBGEXIImm(&v1, sizeof(v1), EXI_WRITE));
	total |= IS_FALSE(DBGEXISync());

	while (count != 0) {
		v = *buf_p++;

		total |= IS_FALSE(DBGEXIImm(&v, sizeof(v), EXI_WRITE));
		total |= IS_FALSE(DBGEXISync());

		count -= 4;
		if (count < 0) {
			count = 0;
		}
	}

	total |= IS_FALSE(DBGEXIDeselect());
	return IS_FALSE(total);
}

/**
 * @note Address: 0x8015D41C
 * @note Size: 0xAC
 */
BOOL DBGReadStatus(u32* value)
{
	return DBGReadRegister(DBG_READ_STATUS, value);
}

#ifndef DOLPHIN_DEBUGGER_TRANSPORT_H
#define DOLPHIN_DEBUGGER_TRANSPORT_H

#include "Dolphin/exi.h"

#define IS_TRUE(x) ((x) != FALSE)
#define IS_FALSE(x) !IS_TRUE(x)

#define DBG_READ_STATUS  0x40000000u
#define DBG_READ_MAILBOX 0x60000000u

BOOL DBGEXIImm(void* buffer, s32 byteCount, u32 write);
BOOL DBGRead(u32 address, u32* buffer, s32 count);
BOOL DBGWrite(u32 address, const void* buffer, s32 count);
BOOL DBGReadStatus(u32* value);
BOOL DBGReadMailbox(u32* value);

static inline BOOL DBGEXISelect(u32 frequency)
{
	u32 regs = EXI_REG(2, EXI_REG_CPR);
	regs &= EXI_CPR_INT_ENABLES;
	regs |= EXI_CPR_CS(0) | EXI_CPR_CLK(frequency);
	EXI_REG(2, EXI_REG_CPR) = regs;
	return TRUE;
}

static inline BOOL DBGEXIDeselect(void)
{
	EXI_REG(2, EXI_REG_CPR) &= EXI_CPR_INT_ENABLES;
	return TRUE;
}

static inline BOOL DBGEXISync(void)
{
	while (EXI_REG(2, EXI_REG_CONTROL) & EXI_CR_START)
		;

	return TRUE;
}

/* Send the two-byte register command, then read the 32-bit reply. */
static inline BOOL DBGReadRegister(u32 command, u32* value)
{
	BOOL total = FALSE;
	u32 v;

	DBGEXISelect(EXI_FREQ_16M);

	v = command;
	total |= IS_FALSE(DBGEXIImm(&v, 2, EXI_WRITE));
	total |= IS_FALSE(DBGEXISync());

	total |= IS_FALSE(DBGEXIImm(value, sizeof(*value), EXI_READ));
	total |= IS_FALSE(DBGEXISync());

	total |= IS_FALSE(DBGEXIDeselect());

	return IS_FALSE(total);
}

#endif

// EXI channel 2 access for the NDEV debugger link, after xenoblade's libs/NdevExi2A/src/exi2.c.
#include "revolution/NdevExi2A.h"

#define EXI2_CPR  (__EXIRegs[10])
#define EXI2_CR   (__EXIRegs[13])
#define EXI2_DATA (__EXIRegs[14])

#define EXI_READ  0
#define EXI_WRITE 1

static u32 __EXISwap32(u32 val);

static BOOL __EXI2Select(void)
{
	u32 regs = EXI2_CPR;
	regs &= 0x405;
	regs |= 0xC0;
	EXI2_CPR = regs;
	return TRUE;
}

static BOOL __EXI2Deselect(void)
{
	u32 temp  = EXI2_CPR;
	EXI2_CPR = temp & 0x405;
	return TRUE;
}

static BOOL __EXI2Sync(void)
{
	while (EXI2_CR & 0x1) { }
	return TRUE;
}

// NONMATCHING: 99.1 %, same instructions. Retail numbers the registers type r27, size r26 and the read value
// r25; here data sits above the parameters (r27) and type and size one lower. At -O0 the order follows the
// reference counts (decl order makes no difference), so retail's data seems to carry one reference fewer.
// Tried: every order of the locals, data block-scoped or initialised, data set in the for header (comma temp).
BOOL __EXI2Imm(void* mem, s32 size, u32 type)
{
	int i;
	int j;
	u8* bmem;
	u32 imm;
	u32 data;

	if (type != EXI_READ) {
		imm = 0;

		for (i = 0; i < size; i++) {
			imm |= ((u8*)mem)[i] << (3 - i) * 8;
		}

		EXI2_DATA = imm;
	}

	EXI2_CR = type << 2 | 1 | (size - 1) * 16;
	__EXI2Sync();

	if (type == EXI_READ) {
		bmem = (u8*)mem;
		data = EXI2_DATA;

		for (j = 0; j < size; j++) {
			*bmem++ = data >> (3 - j) * 8;
		}
	}

	return TRUE;
}

// The July 2008 library writes the 0xD4 byte to register 0xB4000000 through __DBEXIWriteReg; xenoblade's
// 2009 copy sends both words by hand.
void __DBEXIInit(void)
{
	u8 val;

	__OSMaskInterrupts(OS_INTERRUPTMASK_EXI_2_EXI | OS_INTERRUPTMASK_EXI_2_TC);
	while ((EXI2_CR & 1) == 1U) { }
	EXI2_CPR = 0;

	val = 0xD4;
	__DBEXIWriteReg(0xB4000000, &val, sizeof(val));
}

BOOL __DBEXIReadReg(u32 cmd, void* mem, s32 size)
{
	BOOL error   = FALSE;
	u32 read_val = 0;

	error = error | !__EXI2Select();
	error = error | !__EXI2Imm(&cmd, sizeof(cmd), EXI_WRITE);
	error = error | !__EXI2Sync();
	error = error | !__EXI2Imm(&read_val, sizeof(read_val), EXI_READ);
	error = error | !__EXI2Sync();
	error = error | !__EXI2Deselect();

	switch (size) {
	case 1:
		*(u8*)mem = read_val >> 24;
		break;
	case 2:
		*(u16*)mem = read_val >> 24 | read_val >> 8 & 0xFF00;
		break;
	default:
		*(u32*)mem = __EXISwap32(read_val);
		break;
	}

	return error == FALSE;
}

BOOL __DBEXIWriteReg(u32 cmd, const void* mem, s32 size)
{
	BOOL error = FALSE;
	u32 write_val;

	switch (size) {
	case 1:
		write_val = (u8)*(u8*)mem << 24;
		break;
	case 2:
		write_val = (u8)*(u16*)mem << 24 | (*(u16*)mem & 0xFF00) << 8;
		break;
	default:
		write_val = __EXISwap32(*(u32*)mem);
		break;
	}

	error = error | !__EXI2Select();
	error = error | !__EXI2Imm(&cmd, sizeof(cmd), EXI_WRITE);
	error = error | !__EXI2Sync();
	error = error | !__EXI2Imm(&write_val, sizeof(write_val), EXI_WRITE);
	error = error | !__EXI2Sync();
	error = error | !__EXI2Deselect();

	return error == FALSE;
}

BOOL __DBEXIReadRam(u32 cmd, void* mem, s32 size)
{
	BOOL error = FALSE;
	u32 read_val;
	u32* lmem = (u32*)mem;

	error = error | !__EXI2Select();
	error = error | !__EXI2Imm(&cmd, sizeof(cmd), EXI_WRITE);
	error = error | !__EXI2Sync();

	for (; size > 0; size -= 4) {
		error   = error | !__EXI2Imm(&read_val, sizeof(read_val), EXI_READ);
		error   = error | !__EXI2Sync();
		*lmem++ = read_val;
	}

	error = error | !__EXI2Deselect();

	return error == FALSE;
}

BOOL __DBEXIWriteRam(u32 cmd, const void* mem, s32 size)
{
	BOOL error = FALSE;
	u32* lmem  = (u32*)mem;
	u32 write_val;

	error = error | !__EXI2Select();
	error = error | !__EXI2Imm(&cmd, sizeof(cmd), EXI_WRITE);
	error = error | !__EXI2Sync();

	for (; size > 0; size -= 4) {
		write_val = *lmem++;
		error     = error | !__EXI2Imm(&write_val, sizeof(write_val), EXI_WRITE);
		error     = error | !__EXI2Sync();
	}

	error = error | !__EXI2Deselect();

	return error == FALSE;
}

static u32 __EXISwap32(u32 val)
{
	return val >> 24 & 0x000000FF | val >> 8 & 0x0000FF00 | val << 8 & 0x00FF0000 | val << 24 & 0xFF000000;
}

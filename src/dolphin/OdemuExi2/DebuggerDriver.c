#include "types.h"
#include "Dolphin/db.h"
#include "sdk/private/exi/DebuggerTransport.h"

static AmcEXICallback MTRCallback;
static void (*DBGCallback)(u32, OSContext*);

static u32 SendMailData;
static s32 RecvDataLeng;

static vu8* pEXIInputFlag;
static vu8 EXIInputFlag;

static u8 SendCount = 0x80;

#define ROUND_UP(x, align) (((x) + (align) - 1) & (-(align)))

/**
 * @note Address: N/A
 * @note Size: 0x34
 */
void DBGEXIInit(void)
{
	__OSMaskInterrupts(OS_INTERRUPTMASK_EXI_2);
	EXI_REG(2, EXI_REG_CPR) = 0;
}

/**
 * @note Address: N/A
 * @note Size: 0x8C
 */
static BOOL DBGWriteMailbox(u32 p1)
{
	u32 cmd = 0xc0000000;
	u32 v;
	u32 base   = p1;
	BOOL total = FALSE;

	DBGEXISelect(EXI_FREQ_16M);
	v = (base & 0x1fffffff) | (cmd);
	total |= IS_FALSE(DBGEXIImm(&v, sizeof(v), EXI_WRITE));
	total |= IS_FALSE(DBGEXISync());
	total |= IS_FALSE(DBGEXIDeselect());

	return IS_FALSE(total);
}

/**
 * @note Address: 0x8015D3E0
 * @note Size: 0x3C
 */

static void MWCallback(u32 interrupt, OSContext* context)
{
	EXIInputFlag = TRUE;
	if (MTRCallback) {
		MTRCallback(0, context);
	}
}

/**
 * @note Address: 0x8015D3A0
 * @note Size: 0x40
 */
static void DBGHandler(__OSInterrupt interrupt, OSContext* context)
{
	__PIRegs[PI_INTRPT_SRC] = PI_INTRPT_DEBUG;
	if (DBGCallback) {
		DBGCallback(interrupt, context);
	}
}

/**
 * @note Address: 0x8015D328
 * @note Size: 0x78
 */
void DBInitComm(vu8** inputFlag, AmcEXICallback callback)
{
	BOOL interrupts = OSDisableInterrupts();
	pEXIInputFlag = &EXIInputFlag;
	*inputFlag = pEXIInputFlag;
	MTRCallback = callback;
	DBGEXIInit();
	OSRestoreInterrupts(interrupts);
}

/**
 * @note Address: 0x8015D2D4
 * @note Size: 0x54
 */
void DBInitInterrupts(void)
{
	__OSMaskInterrupts(OS_INTERRUPTMASK_EXI_2);
	__OSMaskInterrupts(OS_INTERRUPTMASK_PI_DEBUG);
	DBGCallback = &MWCallback;
	__OSSetInterruptHandler(__OS_INTERRUPT_PI_DEBUG, DBGHandler);
	__OSUnmaskInterrupts(OS_INTERRUPTMASK_PI_DEBUG);
}

/**
 * @note Address: N/A
 * @note Size: 0x150
 */
static void CheckMailBox(void)
{
	u32 v;
	DBGReadStatus(&v);
	if (v & 1) {
		DBGReadMailbox(&v);
		v &= 0x1fffffff;

		if ((v & 0x1f000000) == 0x1f000000) {
			SendMailData = v;
			RecvDataLeng = v & 0x7fff;
			EXIInputFlag = 1;
		}
	}
}

/**
 * @note Address: 0x8015D238
 * @note Size: 0x9C
 */
u32 DBQueryData(void)
{
	EXIInputFlag = 0;
	if (!RecvDataLeng) {
		BOOL interrupts = OSDisableInterrupts();
		CheckMailBox();
		OSRestoreInterrupts(interrupts);
	}
	return RecvDataLeng;
}

/**
 * @note Address: 0x8015D1AC
 * @note Size: 0x8C
 */
BOOL DBRead(void* buffer, u32 count)
{
	u32 interrupts = OSDisableInterrupts();
	u32 v          = SendMailData & 0x10000 ? 0x1000 : 0;

	DBGRead(v + 0x1e000, buffer, ROUND_UP(count, 4));

	RecvDataLeng = 0;
	EXIInputFlag = 0;

	OSRestoreInterrupts(interrupts);

	return 0;
}

/**
 * @note Address: 0x8015CF4C
 * @note Size: 0x260
 */
BOOL DBWrite(const void* src, u32 size)
{
	u32 v;
	u32 busyFlag;
	BOOL interrupts = OSDisableInterrupts();

	do {
		DBGReadRegister(DBG_READ_STATUS, &busyFlag);
	} while (busyFlag & 2);

	SendCount++;
	v = ((SendCount & 1) ? 0x1000 : 0);

	while (!DBGWrite(v | 0x1c000, src, ROUND_UP(size, 4)))
		;

	do {
		DBGReadRegister(DBG_READ_STATUS, &busyFlag);
	} while (busyFlag & 2);

	v = SendCount;
	while (!DBGWriteMailbox((0x1f000000) | v << 0x10 | size))
		;

	do {
		while (!DBGReadRegister(DBG_READ_STATUS, &busyFlag))
			;
	} while (busyFlag & 2);

	OSRestoreInterrupts(interrupts);

	return 0;
}

/**
 * @note Address: 0x8015CF48
 * @note Size: 0x4
 */
void DBOpen(void)
{
	return;
}

/**
 * @note Address: 0x8015CF44
 * @note Size: 0x4
 */
void DBClose(void)
{
	return;
}

// The Wii's NDEV debugger driver, after xenoblade's libs/NdevExi2A/src/DebuggerDriver.c. Retail builds the whole
// library unoptimised and without inlining (see configure.py), so the mail and transfer helpers are calls;
// retail has them after DBClose, in this order.
#include "revolution/NdevExi2A.h"

static u32 ODEMUGetSize(u32 mail);
static BOOL ODEMUIsValidMail(u32 mail);
static u32 ODEMUGetPc2NngcOffset(u32 mail);
static u32 ODEMUGetPage(u32 mail);
static u32 ODEMUGenMailData(u32 ofs, u32 size);
static BOOL __DBReadMailbox(u32* mailOut);
static BOOL __DBRead(u32 ofs, void* dest, u32 size);
static BOOL __DBWriteMailbox(u32 mail);
static BOOL __DBWrite(u32 ofs, const void* src, u32 size);

static u32 __DBRecvDataSize;
static u32 __DBRecvMail;
static u8 __DBEXIInputFlag;
static __OSInterruptHandler __DBDbgCallback;
static MTRCallbackType __DBMtrCallback;

static u8 __DBReadUSB_CSR(void);
static void __DBWaitForSendMail(void);

void __DBMtrHandler(__OSInterrupt type, OSContext* ctx)
{
	__DBEXIInputFlag = TRUE;

	if (__DBMtrCallback != NULL) {
		__DBMtrCallback(0, ctx);
	}
}

void __DBIntrHandler(__OSInterrupt type, OSContext* ctx)
{
	__PIRegs[PI_INTRPT_SRC] = PI_INTRPT_DEBUG;

	if (__DBDbgCallback) {
		__DBDbgCallback(type, ctx);
	}
}

static void __DBCheckMailBox(void)
{
	u32 mail;
	const u8 csr = __DBReadUSB_CSR();

	if (!(csr & 0x8)) {
		__DBReadMailbox(&mail);

		if (ODEMUIsValidMail(mail)) {
			__DBRecvMail     = mail;
			__DBRecvDataSize = ODEMUGetSize(mail);
			__DBEXIInputFlag = TRUE;
		}
	}
}

static u8 __DBReadUSB_CSR(void)
{
	u8 val;
	__DBEXIReadReg(0x34000000, &val, sizeof(val));
	return val;
}

void DBInitComm(u8** flagOut, MTRCallbackType handler)
{
	const BOOL enabled = OSDisableInterrupts();

	*flagOut        = &__DBEXIInputFlag;
	__DBMtrCallback = handler;
	__DBEXIInit();

	OSRestoreInterrupts(enabled);
}

void DBInitInterrupts(void)
{
	__OSMaskInterrupts(OS_INTERRUPTMASK_EXI_2_EXI | OS_INTERRUPTMASK_EXI_2_TC);
	__OSMaskInterrupts(OS_INTERRUPTMASK_PI_DEBUG);
	__DBDbgCallback = __DBMtrHandler;
	__OSSetInterruptHandler(__OS_INTERRUPT_PI_DEBUG, __DBIntrHandler);
	__OSUnmaskInterrupts(OS_INTERRUPTMASK_PI_DEBUG);
}

u32 DBQueryData(void)
{
	BOOL enabled;

	__DBEXIInputFlag = FALSE;

	if (__DBRecvDataSize == 0) {
		enabled = OSDisableInterrupts();
		__DBCheckMailBox();
		OSRestoreInterrupts(enabled);
	}

	return __DBRecvDataSize;
}

BOOL DBRead(void* dst, u32 size)
{
	u32 ofs;
	const BOOL enabled = OSDisableInterrupts();

	ofs = ODEMUGetPc2NngcOffset(__DBRecvMail) + 0x1000;
	__DBRead(ofs, dst, (size + 3) & ~3);
	__DBRecvDataSize = 0;
	__DBEXIInputFlag = FALSE;

	OSRestoreInterrupts(enabled);

	return FALSE;
}

BOOL DBWrite(const void* src, u32 size)
{
	static u8 l_byOffsetCounter = 128;
	u32 page, ofs, mail; // page: which half of the 0x1000-byte send buffer
	const BOOL enabled = OSDisableInterrupts();

	__DBWaitForSendMail();

	l_byOffsetCounter++;
	if (l_byOffsetCounter & 0x1) {
		page = 0x800;
	} else {
		page = 0;
	}
	ofs = page;

	while (__DBWrite(ofs, src, (size + 3) & ~3) == 0) { }

	__DBWaitForSendMail();

	mail = ODEMUGenMailData(l_byOffsetCounter, size);
	while (__DBWriteMailbox(mail) == 0) { }

	__DBWaitForSendMail();

	OSRestoreInterrupts(enabled);

	return FALSE;
}

static void __DBWaitForSendMail(void)
{
	u8 csr;

	do {
		csr = __DBReadUSB_CSR();
	} while (csr & 0x4);
}

void DBOpen(void) { }

void DBClose(void) { }

// ODEMU mail words: 0x1F in the top byte, the page in bits 16-23, the size in the low 13 bits
static u32 ODEMUGetSize(u32 mail)
{
	return mail & 0x1FFF;
}

static BOOL ODEMUIsValidMail(u32 mail)
{
	return (mail & 0x1F000000) == 0x1F000000;
}

static u32 ODEMUGetPc2NngcOffset(u32 mail)
{
	return (ODEMUGetPage(mail) & 0x1) ? 0x800 : 0;
}

static u32 ODEMUGetPage(u32 mail)
{
	return (mail & 0xFF0000) >> 16;
}

static u32 ODEMUGenMailData(u32 ofs, u32 size)
{
	return ofs << 16 & 0xFF0000 | 0x1F000000 | size & 0x1FFF;
}

static BOOL __DBReadMailbox(u32* mailOut)
{
	return __DBEXIReadReg(0x34000200, mailOut, sizeof(*mailOut));
}

static BOOL __DBRead(u32 ofs, void* dest, u32 size)
{
	return __DBEXIReadRam(((ofs + 0xD10000) << 6) & 0x3FFFFF00, dest, size);
}

static BOOL __DBWriteMailbox(u32 mail)
{
	return __DBEXIWriteReg(0xB4000100, &mail, sizeof(mail));
}

static BOOL __DBWrite(u32 ofs, const void* src, u32 size)
{
	return __DBEXIWriteRam((((ofs + 0xD10000) << 6) & 0x3FFFFF00) | 0x80000000, src, size);
}

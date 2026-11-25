#include "Dolphin/ar.h"
#include "Dolphin/os.h"
#include "Dolphin/hw_regs.h"

#define ARAM_PROBE_SIZE DSP_DMA_BLOCK_SIZE
/* Each buffer needs room for one DMA block after rounding its start up. */
#define ARAM_PROBE_BUFFER_WORDS ((ARAM_PROBE_SIZE + ARQ_DMA_ALIGNMENT) / sizeof(u32))
#define ARAM_SIZE_ADDRESS 0x00D0
#define ARAM_MIB (1024 * 1024)

char* __ARVersion = "<< Dolphin SDK - AR\trelease build: Apr  5 2004 04:15:03 (0x2301) >>";

static ARCallback __AR_Callback;
static u32 __AR_Size;
static u32 __AR_InternalSize;
static u32 __AR_ExpansionSize;

static u32 __AR_StackPointer;
static u32 __AR_FreeBlocks;
static u32* __AR_BlockLength;

static volatile BOOL __AR_init_flag = FALSE;

static void __ARHandler(__OSInterrupt interrupt, OSContext* context);
static void __ARChecksize(void);
static void __ARClearArea(u32 start_addr, u32 length);

/**
 * @note Address: 0x80175000
 * @note Size: 0x44
 */
ARCallback ARRegisterDMACallback(ARCallback callback)
{
	ARCallback oldCb;
	BOOL enabled;
	oldCb         = __AR_Callback;
	enabled       = OSDisableInterrupts();
	__AR_Callback = callback;
	OSRestoreInterrupts(enabled);
	return oldCb;
}

/**
 * @note Address: 0x80175044
 * @note Size: 0x3C
 */
u32 ARGetDMAStatus(void)
{
	BOOL enabled;
	u32 val;
	enabled = OSDisableInterrupts();
	val     = __DSPRegs[DSP_CONTROL_STATUS] & DSP_STATUS_DMA_BUSY;
	OSRestoreInterrupts(enabled);
	return val;
}

static inline void SetARAMDMAAddresses(u32 mainmem_addr, u32 aram_addr)
{
	__DSPRegs[DSP_ARAM_DMA_MM_HI] = (u16)(__DSPRegs[DSP_ARAM_DMA_MM_HI] & ~DSP_DMA_HI_FIELD_MASK)
	    | (u16)(mainmem_addr >> 16);
	__DSPRegs[DSP_ARAM_DMA_MM_LO] = (u16)(__DSPRegs[DSP_ARAM_DMA_MM_LO] & ~DSP_DMA_LO_FIELD_MASK)
	    | (u16)(mainmem_addr & 0xffff);

	__DSPRegs[DSP_ARAM_DMA_ARAM_HI] = (u16)(__DSPRegs[DSP_ARAM_DMA_ARAM_HI] & ~DSP_DMA_HI_FIELD_MASK)
	    | (u16)(aram_addr >> 16);
	__DSPRegs[DSP_ARAM_DMA_ARAM_LO] = (u16)(__DSPRegs[DSP_ARAM_DMA_ARAM_LO] & ~DSP_DMA_LO_FIELD_MASK)
	    | (u16)(aram_addr & 0xffff);
}

static inline void SetARAMDMALength(u32 length)
{
	__DSPRegs[DSP_ARAM_DMA_SIZE_HI] = (u16)(__DSPRegs[DSP_ARAM_DMA_SIZE_HI] & ~DSP_DMA_HI_FIELD_MASK)
	    | (u16)(length >> 16);
	__DSPRegs[DSP_ARAM_DMA_SIZE_LO] = (u16)(__DSPRegs[DSP_ARAM_DMA_SIZE_LO] & ~DSP_DMA_LO_FIELD_MASK)
	    | (u16)(length & 0xffff);
}

/**
 * @note Address: 0x80175080
 * @note Size: 0xF0
 */
void ARStartDMA(u32 type, u32 mainmem_addr, u32 aram_addr, u32 length)
{
	BOOL enabled;

	enabled = OSDisableInterrupts();

	SetARAMDMAAddresses(mainmem_addr, aram_addr);
	__DSPRegs[DSP_ARAM_DMA_SIZE_HI] = (u16)((__DSPRegs[DSP_ARAM_DMA_SIZE_HI] & ~DSP_ARAM_DMA_DIRECTION_MASK)
	    | (type << DSP_ARAM_DMA_DIRECTION_BIT));
	SetARAMDMALength(length);

	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x80175170
 * @note Size: 0x68
 */
u32 ARAlloc(u32 length)
{
	u32 oldStackPtr;
	BOOL enabled;

	enabled     = OSDisableInterrupts();
	oldStackPtr = __AR_StackPointer;
	__AR_StackPointer += length;
	*__AR_BlockLength = length;
	__AR_BlockLength++;
	__AR_FreeBlocks--;
	OSRestoreInterrupts(enabled);

	return oldStackPtr;
}

/**
 * @note Address: 0x801751D8
 * @note Size: 0x74
 */
u32 ARFree(u32* length)
{
	BOOL enabled;

	enabled = OSDisableInterrupts();
	__AR_BlockLength--;
	if (length) {
		*length = *__AR_BlockLength;
	}
	__AR_StackPointer -= *__AR_BlockLength;
	__AR_FreeBlocks++;
	OSRestoreInterrupts(enabled);

	return __AR_StackPointer;
}

/**
 * @note Address: 0x8017524C
 * @note Size: 0xC4
 */
u32 ARInit(u32* stack_index_addr, u32 num_entries)
{
	BOOL old;
	u16 refresh;

	if (__AR_init_flag == TRUE) {
		return __AR_ARAM_USR_BASE_ADDR;
	}

	OSRegisterVersion(__ARVersion);

	old = OSDisableInterrupts();

	__AR_Callback = NULL;

	__OSSetInterruptHandler(__OS_INTERRUPT_DSP_ARAM, __ARHandler);
	__OSUnmaskInterrupts(OS_INTERRUPTMASK_DSP_ARAM);

	__AR_StackPointer = __AR_ARAM_USR_BASE_ADDR;
	__AR_FreeBlocks   = num_entries;
	__AR_BlockLength  = stack_index_addr;

	refresh = (u16)(__DSPRegs[DSP_ARAM_REFRESH] & 0xFF);

	__DSPRegs[DSP_ARAM_REFRESH] = (u16)((__DSPRegs[DSP_ARAM_REFRESH] & ~0xFF) | (refresh & 0xFF));

	__ARChecksize();

	__AR_init_flag = TRUE;

	OSRestoreInterrupts(old);

	return __AR_StackPointer;
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
u32 ARGetBaseAddress(void)
{
	return __AR_ARAM_USR_BASE_ADDR;
}

/**
 * @note Address: 0x80175310
 * @note Size: 0x8
 */
u32 ARGetSize(void)
{
	return __AR_Size;
}

/**
 * @note Address: 0x80175318
 * @note Size: 0x78
 */
void __ARHandler(__OSInterrupt interrupt, OSContext* context)
{
	OSContext exceptionContext;
	u16 tmp;

	tmp = __DSPRegs[DSP_CONTROL_STATUS];
	tmp = (u16)((tmp & ~(DSP_STATUS_DSP_INTERRUPT | DSP_STATUS_AI_INTERRUPT)) | DSP_STATUS_ARAM_INTERRUPT);
	__DSPRegs[DSP_CONTROL_STATUS] = tmp;

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(&exceptionContext);

	if (__AR_Callback) {
		(*__AR_Callback)();
	}

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(context);
}

/**
 * @note Address: 0x80175390
 * @note Size: 0x20
 */
void __ARClearInterrupt(void)
{
	u16 tmp;
	tmp = __DSPRegs[DSP_CONTROL_STATUS];
	tmp = (u16)((tmp & ~(DSP_STATUS_DSP_INTERRUPT | DSP_STATUS_AI_INTERRUPT)) | DSP_STATUS_ARAM_INTERRUPT);
	__DSPRegs[DSP_CONTROL_STATUS] = tmp;
}

/**
 * @note Address: 0x801753B0
 * @note Size: 0x10
 */
u16 __ARGetInterruptStatus(void)
{
	return ((u16)(__DSPRegs[DSP_CONTROL_STATUS] & DSP_STATUS_ARAM_INTERRUPT));
}

/**
 * @note Address: N/A
 * @note Size: 0x18
 */
void __ARWaitForDMA(void)
{
	while (__DSPRegs[DSP_CONTROL_STATUS] & DSP_STATUS_DMA_BUSY) { }
}

/**
 * @note Address: N/A
 * @note Size: 0xB0
 */
void __ARWriteDMA(u32 mmem_addr, u32 aram_addr, u32 length)
{
	SetARAMDMAAddresses(mmem_addr, aram_addr);
	__DSPRegs[DSP_ARAM_DMA_SIZE_HI] = (u16)(__DSPRegs[DSP_ARAM_DMA_SIZE_HI] & ~DSP_ARAM_DMA_DIRECTION_MASK);
	SetARAMDMALength(length);

	__ARWaitForDMA();

	__ARClearInterrupt();
}

/**
 * @note Address: N/A
 * @note Size: 0xB0
 */
void __ARReadDMA(u32 mmem_addr, u32 aram_addr, u32 length)
{
	SetARAMDMAAddresses(mmem_addr, aram_addr);
	__DSPRegs[DSP_ARAM_DMA_SIZE_HI] = (u16)(__DSPRegs[DSP_ARAM_DMA_SIZE_HI] | DSP_ARAM_DMA_DIRECTION_MASK);
	SetARAMDMALength(length);

	__ARWaitForDMA();

	__ARClearInterrupt();
}

/**
 * @note Address: 0x801753C0
 * @note Size: 0x17F4
 */
void __ARChecksize(void)
{
	u32 test_data_pad[ARAM_PROBE_BUFFER_WORDS];
	u32 dummy_data_pad[ARAM_PROBE_BUFFER_WORDS];
	u32 buffer_pad[ARAM_PROBE_BUFFER_WORDS];

	u32 save_pad_1[ARAM_PROBE_BUFFER_WORDS];
	u32 save_pad_2[ARAM_PROBE_BUFFER_WORDS];
	u32 save_pad_3[ARAM_PROBE_BUFFER_WORDS];
	u32 save_pad_4[ARAM_PROBE_BUFFER_WORDS];
	u32 save_pad_5[ARAM_PROBE_BUFFER_WORDS];

	u32* test_data;
	u32* dummy_data;
	u32* buffer;
	u32* save1;
	u32* save2;
	u32* save3;
	u32* save4;
	u32* save5;

	u16 ARAM_mode = 0;
	u32 ARAM_size = 0;

	u32 i;

	while (!(__DSPRegs[DSP_ARAM_MODE] & 1)) { }

	ARAM_mode = 3;
	ARAM_size = __AR_InternalSize = 16 * ARAM_MIB;
	__DSPRegs[DSP_ARAM_SIZE]      = (u16)((__DSPRegs[DSP_ARAM_SIZE] & ~(0x7 | 0x38)) | 0x20 | 2 | 1);

	test_data  = (u32*)(OSRoundUp32B((u32)(test_data_pad)));
	dummy_data = (u32*)(OSRoundUp32B((u32)(dummy_data_pad)));
	buffer     = (u32*)(OSRoundUp32B((u32)(buffer_pad)));

	save1 = (u32*)(OSRoundUp32B((u32)(save_pad_1)));
	save2 = (u32*)(OSRoundUp32B((u32)(save_pad_2)));
	save3 = (u32*)(OSRoundUp32B((u32)(save_pad_3)));
	save4 = (u32*)(OSRoundUp32B((u32)(save_pad_4)));
	save5 = (u32*)(OSRoundUp32B((u32)(save_pad_5)));

	for (i = 0; i < ARAM_PROBE_SIZE / sizeof(u32); i++) {
		test_data[i]  = 0xDEADBEEF;
		dummy_data[i] = 0xBAD0BAD0;
	}

	DCFlushRange((void*)test_data, ARAM_PROBE_SIZE);
	DCFlushRange((void*)dummy_data, ARAM_PROBE_SIZE);

	__AR_ExpansionSize = 0;

	// Save the contents that will be overwritten while testing for address mirroring.
	DCInvalidateRange((void*)save1, ARAM_PROBE_SIZE);
	__ARReadDMA((u32)save1, ARAM_size, ARAM_PROBE_SIZE);
	PPCSync();

	__ARWriteDMA((u32)test_data, ARAM_size, ARAM_PROBE_SIZE);

	memset((void*)buffer, 0, ARAM_PROBE_SIZE);
	DCFlushRange((void*)buffer, ARAM_PROBE_SIZE);

	__ARReadDMA((u32)buffer, ARAM_size, ARAM_PROBE_SIZE);
	PPCSync();

	if (buffer[0] == test_data[0]) {

		DCInvalidateRange((void*)save2, ARAM_PROBE_SIZE);
		__ARReadDMA((u32)save2, ARAM_size + 2 * ARAM_MIB, ARAM_PROBE_SIZE);
		PPCSync();

		DCInvalidateRange((void*)save3, ARAM_PROBE_SIZE);
		__ARReadDMA((u32)save3, ARAM_size + 16 * ARAM_MIB, ARAM_PROBE_SIZE);
		PPCSync();

		DCInvalidateRange((void*)save4, ARAM_PROBE_SIZE);
		__ARReadDMA((u32)save4, ARAM_size + 0x200, ARAM_PROBE_SIZE);
		PPCSync();

		DCInvalidateRange((void*)save5, ARAM_PROBE_SIZE);
		__ARReadDMA((u32)save5, ARAM_size + 4 * ARAM_MIB, ARAM_PROBE_SIZE);
		PPCSync();

		__ARWriteDMA((u32)dummy_data, ARAM_size + 2 * ARAM_MIB, ARAM_PROBE_SIZE);

		__ARWriteDMA((u32)test_data, ARAM_size, ARAM_PROBE_SIZE);

		memset((void*)buffer, 0, ARAM_PROBE_SIZE);
		DCFlushRange((void*)buffer, ARAM_PROBE_SIZE);

		__ARReadDMA((u32)buffer, ARAM_size + 2 * ARAM_MIB, ARAM_PROBE_SIZE);
		PPCSync();

		if (buffer[0] == test_data[0]) {
			__ARWriteDMA((u32)save1, ARAM_size, ARAM_PROBE_SIZE);

			ARAM_size += 2 * ARAM_MIB;
			__AR_ExpansionSize = 2 * ARAM_MIB;
		} else {
			__ARWriteDMA((u32)dummy_data, ARAM_size + 16 * ARAM_MIB, ARAM_PROBE_SIZE);

			__ARWriteDMA((u32)test_data, ARAM_size, ARAM_PROBE_SIZE);

			memset((void*)buffer, 0, ARAM_PROBE_SIZE);
			DCFlushRange((void*)buffer, ARAM_PROBE_SIZE);

			__ARReadDMA((u32)buffer, ARAM_size + 16 * ARAM_MIB, ARAM_PROBE_SIZE);
			PPCSync();

			if (buffer[0] == test_data[0]) {
				__ARWriteDMA((u32)save1, ARAM_size, ARAM_PROBE_SIZE);
				__ARWriteDMA((u32)save2, ARAM_size + 2 * ARAM_MIB, ARAM_PROBE_SIZE);

				ARAM_mode |= 4 << 1;
				ARAM_size += 4 * ARAM_MIB;
				__AR_ExpansionSize = 4 * ARAM_MIB;
			} else {
				__ARWriteDMA((u32)dummy_data, ARAM_size + 0x200, ARAM_PROBE_SIZE);

				__ARWriteDMA((u32)test_data, ARAM_size, ARAM_PROBE_SIZE);

				memset((void*)buffer, 0, ARAM_PROBE_SIZE);
				DCFlushRange((void*)buffer, ARAM_PROBE_SIZE);

				__ARReadDMA((u32)buffer, ARAM_size + 0x200, ARAM_PROBE_SIZE);
				PPCSync();

				if (buffer[0] == test_data[0]) {
					__ARWriteDMA((u32)save1, ARAM_size, ARAM_PROBE_SIZE);
					__ARWriteDMA((u32)save2, ARAM_size + 2 * ARAM_MIB, ARAM_PROBE_SIZE);
					__ARWriteDMA((u32)save3, ARAM_size + 16 * ARAM_MIB, ARAM_PROBE_SIZE);

					ARAM_mode |= 8 << 1;
					ARAM_size += 8 * ARAM_MIB;
					__AR_ExpansionSize = 8 * ARAM_MIB;
				} else {
					__ARWriteDMA((u32)dummy_data, ARAM_size + 4 * ARAM_MIB, ARAM_PROBE_SIZE);

					__ARWriteDMA((u32)test_data, ARAM_size, ARAM_PROBE_SIZE);

					memset((void*)buffer, 0, ARAM_PROBE_SIZE);
					DCFlushRange((void*)buffer, ARAM_PROBE_SIZE);

					__ARReadDMA((u32)buffer, ARAM_size + 4 * ARAM_MIB, ARAM_PROBE_SIZE);
					PPCSync();

					if (buffer[0] == test_data[0]) {
						__ARWriteDMA((u32)save1, ARAM_size, ARAM_PROBE_SIZE);
						__ARWriteDMA((u32)save2, ARAM_size + 2 * ARAM_MIB, ARAM_PROBE_SIZE);
						__ARWriteDMA((u32)save3, ARAM_size + 16 * ARAM_MIB, ARAM_PROBE_SIZE);
						__ARWriteDMA((u32)save4, ARAM_size + 0x200, ARAM_PROBE_SIZE);

						ARAM_mode |= 12 << 1;
						ARAM_size += 16 * ARAM_MIB;
						__AR_ExpansionSize = 16 * ARAM_MIB;
					} else {
						__ARWriteDMA((u32)save1, ARAM_size, ARAM_PROBE_SIZE);
						__ARWriteDMA((u32)save2, ARAM_size + 2 * ARAM_MIB, ARAM_PROBE_SIZE);
						__ARWriteDMA((u32)save3, ARAM_size + 16 * ARAM_MIB, ARAM_PROBE_SIZE);
						__ARWriteDMA((u32)save4, ARAM_size + 0x200, ARAM_PROBE_SIZE);
						__ARWriteDMA((u32)save5, ARAM_size + 4 * ARAM_MIB, ARAM_PROBE_SIZE);

						ARAM_mode |= 16 << 1;
						ARAM_size += 32 * ARAM_MIB;
						__AR_ExpansionSize = 32 * ARAM_MIB;
					}
				}
			}
		}
		__DSPRegs[DSP_ARAM_SIZE] = (u16)((__DSPRegs[DSP_ARAM_SIZE] & ~(0x7 | 0x38)) | ARAM_mode);
	}

	*(u32*)OSPhysicalToUncached(ARAM_SIZE_ADDRESS) = ARAM_size;

	__AR_Size = ARAM_size;
}

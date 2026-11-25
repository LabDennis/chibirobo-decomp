#include "PowerPC_EABI_Support/MetroTRK/trk.h"
#include "Dolphin/os.h"

BOOL _MetroTRK_Has_Framing; // UNUSED
// GPVE01     _MetroTRK_Has_Framing = .bss:0x804F4814; // type:object size:0x4 scope:global align:4
// GPVE01_D17 _MetroTRK_Has_Framing = .bss:0x804F4754; // type:object size:0x4 scope:global align:4
u8 TRK_Use_BBA;

DBCommTable gDBCommTable = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

/**
 * @note Address: 0x801500E4
 * @note Size: 0x38
 */
void TRKEXICallBack(s16 r3, OSContext* ctx)
{
	OSEnableScheduler();
	TRKLoadContext(ctx, 0x500);
}

/**
 * @note Address: 0x8014FE78
 * @note Size: 0x26C
 */
int InitMetroTRKCommTable(int hwId)
{
	int result = 1;
	OSReport("Devkit set to : %ld\n", hwId);
	TRK_Use_BBA = 0;

	if (hwId == HARDWARE_BBA) { // BBA hardware
		OSReport("MetroTRK : Set to BBA\n");
		// Initialize gDBCommTable
		TRK_Use_BBA                       = 1;
		gDBCommTable.initialize_func      = udp_cc_initialize;
		gDBCommTable.open_func            = udp_cc_open;
		gDBCommTable.close_func           = udp_cc_close;
		gDBCommTable.read_func            = udp_cc_read;
		gDBCommTable.write_func           = udp_cc_write;
		gDBCommTable.shutdown_func        = udp_cc_shutdown;
		gDBCommTable.peek_func            = udp_cc_peek;
		gDBCommTable.pre_continue_func    = udp_cc_pre_continue;
		gDBCommTable.post_stop_func       = udp_cc_post_stop;
		gDBCommTable.init_interrupts_func = nullptr;
		return 0;
	}

	if (hwId == HARDWARE_GDEV) { // NDEV hardware
		OSReport("MetroTRK : Set to GDEV hardware\n");
		// Initialize gDBCommTable
		result                            = Hu_IsStub();
		gDBCommTable.initialize_func      = gdev_cc_initialize;
		gDBCommTable.open_func            = gdev_cc_open;
		gDBCommTable.close_func           = gdev_cc_close;
		gDBCommTable.read_func            = gdev_cc_read;
		gDBCommTable.write_func           = gdev_cc_write;
		gDBCommTable.shutdown_func        = gdev_cc_shutdown;
		gDBCommTable.peek_func            = gdev_cc_peek;
		gDBCommTable.pre_continue_func    = gdev_cc_pre_continue;
		gDBCommTable.post_stop_func       = gdev_cc_post_stop;
		gDBCommTable.init_interrupts_func = gdev_cc_initinterrupts;

	} else if (hwId == HARDWARE_AMC_DDH) {
		OSReport("MetroTRK : Set to AMC DDH hardware\n");
		result = AMC_IsStub();
		// Initialize gDBCommTable
		gDBCommTable.initialize_func      = ddh_cc_initialize;
		gDBCommTable.open_func            = ddh_cc_open;
		gDBCommTable.close_func           = ddh_cc_close;
		gDBCommTable.read_func            = ddh_cc_read;
		gDBCommTable.write_func           = ddh_cc_write;
		gDBCommTable.shutdown_func        = ddh_cc_shutdown;
		gDBCommTable.peek_func            = ddh_cc_peek;
		gDBCommTable.pre_continue_func    = ddh_cc_pre_continue;
		gDBCommTable.post_stop_func       = ddh_cc_post_stop;
		gDBCommTable.init_interrupts_func = ddh_cc_initinterrupts;

	} else { // unknown hardware
		OSReport("MetroTRK : Set to UNKNOWN hardware. (%ld)\n", hwId);
		OSReport("MetroTRK : Invalid hardware ID passed from OS\n");
		OSReport("MetroTRK : Defaulting to GDEV Hardware\n");
	}

	return result;
}

/**
 * @note Address: 0x8014FE28
 * @note Size: 0x50
 */
DSError TRKInitializeIntDrivenUART(u32, u32, u32, void* r5)
{
	gDBCommTable.initialize_func(r5, TRKEXICallBack);
	gDBCommTable.open_func();
	return DS_NoError;
}

/**
 * @note Address: 0x8014FDE0
 * @note Size: 0x48
 */
void EnableEXI2Interrupts(void)
{
	if (!TRK_Use_BBA) {
		if (gDBCommTable.init_interrupts_func != nullptr) {
			gDBCommTable.init_interrupts_func();
		}
	}
}

/**
 * @note Address: 0x8014FDB0
 * @note Size: 0x30
 */
int TRKPollUART(void)
{
	return gDBCommTable.peek_func();
}

/**
 * @note Address: 0x8014FD74
 * @note Size: 0x3C
 */
UARTError TRKReadUARTN(void* bytes, u32 length)
{
	int readErr = gDBCommTable.read_func(bytes, length);
	return readErr == 0 ? UART_NoError : -1;
}

/**
 * @note Address: 0x8014FD38
 * @note Size: 0x3C
 */
UARTError TRKWriteUARTN(const void* bytes, u32 length)
{
	int writeErr = gDBCommTable.write_func(bytes, length);
	return writeErr == 0 ? UART_NoError : -1;
}

/**
 * @note Address: 0x8014FD08
 * @note Size: 0x30
 */
void ReserveEXI2Port(void)
{
	gDBCommTable.post_stop_func();
}

/**
 * @note Address: 0x8014FCD8
 * @note Size: 0x30
 */
void UnreserveEXI2Port(void)
{
	gDBCommTable.pre_continue_func();
}

/**
 * @note Address: 0x8014FCA8
 * @note Size: 0x30
 */
void TRK_board_display(char* str)
{
	OSReport("%s\n", str);
}

/**
 * @note Address: 0x8014FC50
 * @note Size: 0x58
 */
DSError InitializeProgramEndTrap(void)
{
	static const u32 EndofProgramInstruction = 'END';

	u8* endOfProgramInstructionBytes = (u8*)&EndofProgramInstruction;
	u8* ppcHaltPtr                   = (u8*)PPCHalt;
	TRK_memcpy(ppcHaltPtr + 4, endOfProgramInstructionBytes, 4);
	ICInvalidateRange(ppcHaltPtr + 4, 4);
	DCFlushRange(ppcHaltPtr + 4, 4);
}

/**
 * @note Address: 0x8014FC4C
 * @note Size: 0x4
 */
void TRKUARTInterruptHandler(void)
{
}
